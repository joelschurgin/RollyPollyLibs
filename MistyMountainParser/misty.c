#include "misty.h"

Misty* misty_mountain_create(Arena* arena) {
    Misty* mountain = push_struct(arena, Misty);
    mountain->arena = arena;
    return mountain;
}

#define ElfHeaderGet(header, field) (((header).type == ELF_32) ? (header).h.h32.field : (header).h.h64.field)
Misty_SectionHeaderTableInfo misty_read_elf_header(Misty* mountain, File* f) {
    ElfHeader header;
    file_read_bytes(f, 0, &header.h, sizeof(header.h));

    u8* e_ident = ElfHeaderGet(header, e_ident);
    if (e_ident[EI_MAG0] != 0x7f ||
        e_ident[EI_MAG1] != 'E'  ||
        e_ident[EI_MAG2] != 'L'  ||
        e_ident[EI_MAG3] != 'F') {
        Assert(!"Not a valid ELF!!!");
    }

    ElfType type = e_ident[EI_CLASS];
    header.type = type;
    Assert(type == ELFCLASS32 || type == ELFCLASS64);

    ElfEndian endian = e_ident[EI_DATA];
    Assert(endian != ELFDATANONE);

    mountain->type     = type;
    mountain->endian   = endian;
    mountain->sections = Array(mountain->arena, Misty_Section, 1 + ElfHeaderGet(header, e_shnum));

    Misty_SectionHeaderTableInfo section_header_table_info = (Misty_SectionHeaderTableInfo) {
        .table_offset = ElfHeaderGet(header, e_shoff),
        .entry_size = ElfHeaderGet(header, e_shentsize),
    };

    u64 shstrtab_offset = section_header_table_info.table_offset + ElfHeaderGet(header, e_shstrndx) * section_header_table_info.entry_size;
    section_header_table_info.section_names = misty_read_shstrtab(mountain, f, shstrtab_offset);

    return section_header_table_info;
}

u8* misty_read_shstrtab(Misty* mountain, File* f, u64 offset) {
    ElfSectionHeader section_header = (ElfSectionHeader){
        .type = mountain->type,
    };
    file_read_bytes(f, offset, &section_header.h, sizeof(section_header.h));

    u64 section_size = ElfHeaderGet(section_header, sh_size);
    u64 section_offset = ElfHeaderGet(section_header, sh_offset);

    u8* names = push_array(mountain->arena, u8, section_size, false);
    file_read_bytes(f, section_offset, names, section_size);
    return names;
}

void misty_read_elf_section_headers(Misty* mountain, File* f, Misty_SectionHeaderTableInfo section_header_table_info) {
    ThreadArraySplit split = ThreadArraySplit(mountain->sections.count-1);

    for (u64 curr_section = split.start_idx; curr_section < split.end_idx; curr_section++) {
        ElfSectionHeader section_header = (ElfSectionHeader) {
            .type = mountain->type,
        };
        u64 section_header_offset = section_header_table_info.table_offset + curr_section * section_header_table_info.entry_size;
        file_read_bytes(f, section_header_offset, &section_header.h, sizeof(section_header.h));
 
        u64 section_name_idx = ElfHeaderGet(section_header, sh_name);
        u64 section_idx = curr_section+1;

        Misty_Section* section = &mountain->sections.data[section_idx];
        section->name = String(&section_header_table_info.section_names[section_name_idx]);
        section->offset = ElfHeaderGet(section_header, sh_offset);
        section->size = ElfHeaderGet(section_header, sh_size);

        String debug_name = string_keep_after_perfect_match(section->name, String(".debug_"));
        if (debug_name.size != section->name.size) {
            if (string_equal(debug_name, String("str")))                mountain->debug_str         = section_idx;
            else if (string_equal(debug_name, String("str_offsets")))   mountain->debug_str_offsets = section_idx;
            else if (string_equal(debug_name, String("line")))          mountain->debug_line        = section_idx;
            else if (string_equal(debug_name, String("line_str")))      mountain->debug_line_str    = section_idx;
        }
    }
}

#define DWARF_HEADER_OFFSET_SIZE_FLAG            0b001
#define DWARF_HEADER_LINE_OFFSET_FLAG            0b010
#define DWARF_HEADER_OPCODE_OPERANDS_TABLE_FLAG  0b100

#define DwarfSectionRead_Data(file, section, output_value_ptr, num_bytes) do { \
        file_read_bytes((file), (section)->offset + (section)->pos, (output_value_ptr), (num_bytes)); \
        (section)->pos += (num_bytes); \
    } while (0)
#define DwarfSectionRead_Struct(file, section, output_value) DwarfSectionRead_Data(file, section, &output_value, sizeof(output_value));
#define DwarfSectionRead_Addr(file, section, output_value, type) DwarfSectionRead_Data(file, section, &output_value, ((type) == DWARF_64) ? sizeof(u64) : sizeof(u32))
#define DwarfSectionRead_Array(file, section, arr) DwarfSectionRead_Data(file, section, arr.data, arr.count * sizeof(*arr.data))

internal String DwarfSectionRead_String(Arena* arena, File* f, Misty_Section* section) {
    u64 num_bytes_read = 0;
    String s = file_read_cstring(arena, f, section->offset + section->pos, &num_bytes_read);
    section->pos += num_bytes_read;
    return s;
}

internal String DwarfSectionRead_StringAt(Arena* arena, File* f, Misty_Section* section, u64 offset) {
    u64 num_bytes_read = 0;
    return file_read_cstring(arena, f, section->offset + offset, &num_bytes_read);
}

internal u64 read_uleb128(u8* data, u64* num_bytes_read) {
    u64 result = 0;
    u64 shift = 0;
    u64 count = 0;
    for (;;) {
        u8 byte = data[count++];
        result |= (u64)(byte & 0x7f) << shift;
        if ((byte & 0x80) == 0) break;
        shift += 7;
    }
    *num_bytes_read = count;
    return result;
}

internal u64 DwarfSectionRead_uleb128(File* f, Misty_Section* section) {
    u64 num_bytes_read = 0;
    u64 result = read_uleb128(f->data + section->offset + section->pos, &num_bytes_read);
    section->pos += num_bytes_read;
    return result;
}

Misty_Value misty_read_form_val(Misty* mountain, File* f, Misty_Section* section, DwarfFormType form) {
    Assert(section->addr_type != DWARF_NONE);

    Misty_Value val = {0};

    switch (form) {
        case DW_FORM_line_strp:
        {
            val.type = MISTY_STRING;
            u64 offset = 0;
            DwarfSectionRead_Addr(f, section, offset, section->addr_type);
            val.val.str = DwarfSectionRead_StringAt(mountain->arena, f, &misty_section(mountain, debug_line_str), offset);
        }
        break;
        default:
            Assert(!"Form Type Not Handled");
    }

    return val;
}

u64 misty_read_initial_length_value(File* f, Misty_Section* section, DwarfType* type) {
    u32 length_32 = 0;
    DwarfSectionRead_Struct(f, section, length_32);

    u64 length_64 = (u64)length_32;
    if (length_32 >= 0xfffffff0) {
        DwarfSectionRead_Struct(f, section, length_64);
        *type = DWARF_64;
        return length_64;
    }

    *type = DWARF_32;
    return length_64;
}

void misty_read_line_info_header(Misty* mountain, File* f, Misty_Section* section) {
    Misty_LineInfoHeader header = {0};
    header.unit_length = misty_read_initial_length_value(f, section, &header.type);

    section->addr_type = header.type;

    DwarfSectionRead_Struct(f, section, header.version);
    Assert(header.version == 5);

    DwarfSectionRead_Struct(f, section, header.address_size_bytes);
    DwarfSectionRead_Struct(f, section, header.segment_selector_size_bytes);

    DwarfSectionRead_Addr(f, section, header.header_length, header.type);

    DwarfSectionRead_Struct(f, section, header.min_instr_length);
    DwarfSectionRead_Struct(f, section, header.max_ops_per_instr);
    DwarfSectionRead_Struct(f, section, header.default_is_stmt);
    DwarfSectionRead_Struct(f, section, header.line_base);
    DwarfSectionRead_Struct(f, section, header.line_range);

    DwarfSectionRead_Struct(f, section, header.opcode_base);

    header.standard_opcode_lengths = Array(mountain->arena, u8, header.opcode_base-1);
    DwarfSectionRead_Array(f, section, header.standard_opcode_lengths);

    DwarfSectionRead_Data(f, section, &header.dir_entry_format.count, 1);
    header.dir_entry_format = Array(mountain->arena, Misty_EntryFormat, header.dir_entry_format.count);
    Assert(header.dir_entry_format.count == 1);

    for EachElement(entry_format, Misty_EntryFormat, header.dir_entry_format) {
        entry_format->type = DwarfSectionRead_uleb128(f, section);
        entry_format->form = DwarfSectionRead_uleb128(f, section);
        Assert(entry_format->type == DW_TAG_array_type);
    }

    header.dirs.count = DwarfSectionRead_uleb128(f, section);
    header.dirs = Array(mountain->arena, String, header.dirs.count);

    for EachElement(entry_format, Misty_EntryFormat, header.dir_entry_format) {
        for (u64 dir_idx = 0; dir_idx < header.dirs.count; dir_idx++) {
            Misty_Value val = misty_read_form_val(mountain, f, section, entry_format->form);
            Assert(val.type == MISTY_STRING);
            header.dirs.data[dir_idx] = val.val.str;
        }
    }
}

#undef DwarfSectionRead_Data
#undef DwarfSectionRead_Struct
