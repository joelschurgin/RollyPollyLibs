#include "misty.h"

MistyMountain* misty_mountain_create(Arena* arena) {
    MistyMountain* mountain = push_struct(arena, MistyMountain);
    mountain->arena = arena;
    return mountain;
}

#define ElfHeaderGet(header, field) (((header).type == ELF_32) ? (header).h.h32.field : (header).h.h64.field)
MistyMountain_SectionHeaderTableInfo misty_read_elf_header(MistyMountain* mountain, File* f) {
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
    mountain->sections = Array(mountain->arena, MistyMountain_Section, 1 + ElfHeaderGet(header, e_shnum));

    MistyMountain_SectionHeaderTableInfo section_header_table_info = (MistyMountain_SectionHeaderTableInfo) {
        .table_offset = ElfHeaderGet(header, e_shoff),
        .entry_size = ElfHeaderGet(header, e_shentsize),
    };

    u64 shstrtab_offset = section_header_table_info.table_offset + ElfHeaderGet(header, e_shstrndx) * section_header_table_info.entry_size;
    section_header_table_info.section_names = misty_read_shstrtab(mountain, f, shstrtab_offset);

    return section_header_table_info;
}

u8* misty_read_shstrtab(MistyMountain* mountain, File* f, u64 offset) {
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

void misty_read_elf_section_headers(MistyMountain* mountain, File* f, MistyMountain_SectionHeaderTableInfo section_header_table_info) {
    ThreadArraySplit split = ThreadArraySplit(mountain->sections.count-1);

    for (u64 curr_section = split.start_idx; curr_section < split.end_idx; curr_section++) {
        ElfSectionHeader section_header = (ElfSectionHeader) {
            .type = mountain->type,
        };
        u64 section_header_offset = section_header_table_info.table_offset + curr_section * section_header_table_info.entry_size;
        file_read_bytes(f, section_header_offset, &section_header.h, sizeof(section_header.h));
 
        u64 section_name_idx = ElfHeaderGet(section_header, sh_name);
        u64 section_idx = curr_section+1;

        MistyMountain_Section* section = &mountain->sections.data[section_idx];
        section->name = String(&section_header_table_info.section_names[section_name_idx]);
        section->offset = ElfHeaderGet(section_header, sh_offset);
        section->size = ElfHeaderGet(section_header, sh_size);

        String debug_name = string_keep_after_perfect_match(section->name, String(".debug_"));
        if (debug_name.size != section->name.size) {
            if (string_equal(debug_name, String("macro")))            mountain->debug_macro       = section_idx;
            else if (string_equal(debug_name, String("macinfo")))     mountain->debug_macro       = section_idx;
            else if (string_equal(debug_name, String("str")))         mountain->debug_str         = section_idx;
            else if (string_equal(debug_name, String("str_offsets"))) mountain->debug_str_offsets = section_idx;
        }
    }
}

#define DWARF_HEADER_OFFSET_SIZE_FLAG            0b001
#define DWARF_HEADER_LINE_OFFSET_FLAG            0b010
#define DWARF_HEADER_OPCODE_OPERANDS_TABLE_FLAG  0b100

#define DwarfSectionRead_Data(file, section, value_ptr, num_bytes) do { \
        file_read_bytes((file), (section)->offset + (section)->pos, (value_ptr), (num_bytes)); \
        (section)->pos += (num_bytes); \
    } while (0)
#define DwarfSectionRead_Struct(file, section, value) DwarfSectionRead_Data(file, section, &value, sizeof(value));

String DwarfSectionRead_String(Arena* arena, File* f, MistyMountain_Section* section) {
    u64 num_bytes_read = 0;
    String s = file_read_cstring(arena, f, section->offset + section->pos, &num_bytes_read);
    section->pos += num_bytes_read;
    return s;
}

MistyMountain_MacroHeader misty_macros_read_header(MistyMountain* mountain, File* f) {
    MistyMountain_Section* section = misty_section(mountain, debug_macro);
    MistyMountain_MacroHeader header = (MistyMountain_MacroHeader){0};
 
    DwarfSectionRead_Struct(f, section, header.version);
    DwarfSectionRead_Struct(f, section, header.flags);

    header.address_size_bytes = (header.flags & DWARF_HEADER_OFFSET_SIZE_FLAG) ? sizeof(u64) : sizeof(u32);

    if (header.flags & DWARF_HEADER_LINE_OFFSET_FLAG) {
        DwarfSectionRead_Data(f, section, &header.debug_line_offset, header.address_size_bytes);
    }

    if (header.flags & DWARF_HEADER_OPCODE_OPERANDS_TABLE_FLAG) {
        AssertAlways(!"TODO");
    }

    return header;
}

u64 read_uleb128(u8* data, u64* num_bytes_read) {
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

u64 DwarfSectionRead_uleb128(File* f, MistyMountain_Section* section) {
    u64 num_bytes_read = 0;
    u64 result = read_uleb128(f->data + section->offset + section->pos, &num_bytes_read);
    section->pos += num_bytes_read;
    return result;
}

void misty_macros(MistyMountain* mountain, File* f) {
    MistyMountain_Section* section = misty_section(mountain, debug_macro);
    MistyMountain_MacroHeader header = misty_macros_read_header(mountain, f);

    u64 import_offset = 0;
    u64 prev_section_pos = section->pos;

    u8 opcode = 0;
    do {
        DwarfSectionRead_Struct(f, section, opcode);
        printf("opcode %d =>>>> ", opcode);
        switch (opcode) {
            case DW_MACRO_define:
            {
                u64 line_num = DwarfSectionRead_uleb128(f, section);
                String s = DwarfSectionRead_String(mountain->arena, f, section);
                printf("MACRO: define\n");
                printf(" => line_num: %lu\n", line_num);
                printf(" => string:   %.*s\n", s.size, s.str);
            }
            break;
            case DW_MACRO_undef:
            {
                u64 line_num = DwarfSectionRead_uleb128(f, section);
                String s = DwarfSectionRead_String(mountain->arena, f, section);
                printf("MACRO: undef\n");
                printf(" => line_num: %lu\n", line_num);
                printf(" => string:   %.*s\n", s.size, s.str);
            }
            break;
            case DW_MACRO_define_strp:
            {
                u64 line_num = DwarfSectionRead_uleb128(f, section);
                u64 debug_str_offset = 0;
                DwarfSectionRead_Data(f, section, &debug_str_offset, header.address_size_bytes);

                u64 num_bytes_read = 0;
                MistyMountain_Section* debug_str = misty_section(mountain, debug_str);
                String s = file_read_cstring(mountain->arena, f, debug_str->offset + debug_str_offset, &num_bytes_read);
                printf("MACRO: define strp\n");
                printf(" => line_num:  %lu\n", line_num);
                printf(" => debug_str: %.*s\n", s.size, s.str);
            }
            break;
            case DW_MACRO_undef_strp:
            {
                u64 line_num = DwarfSectionRead_uleb128(f, section);
                u64 debug_str_offset = 0;
                DwarfSectionRead_Data(f, section, &debug_str_offset, header.address_size_bytes);
 
                u64 num_bytes_read = 0;
                MistyMountain_Section* debug_str = misty_section(mountain, debug_str);
                String s = file_read_cstring(mountain->arena, f, debug_str->offset + debug_str_offset, &num_bytes_read);
                printf("MACRO: undef strp\n");
                printf(" => line_num:  %lu\n", line_num);
                printf(" => debug_str: %.*s\n", s.size, s.str);
            }
            break;
            case DW_MACRO_define_strx:
            {
                u64 line_num = DwarfSectionRead_uleb128(f, section);
                u64 debug_str_offset = DwarfSectionRead_uleb128(f, section);
                printf("MACRO: define strx\n");
                printf(" => line_num:                   %lu\n", line_num);
                printf(" => debug_str_offsets location: %lu\n", debug_str_offset);
            }
            break;
            case DW_MACRO_undef_strx:
            {
                u64 line_num = DwarfSectionRead_uleb128(f, section);
                u64 debug_str_offset = DwarfSectionRead_uleb128(f, section);
                printf("MACRO: undef strx\n");
                printf(" => line_num:                   %lu\n", line_num);
                printf(" => debug_str_offsets location: %lu\n", debug_str_offset);
            }
            break;
            case DW_MACRO_start_file:
            {
                u64 line_num = DwarfSectionRead_uleb128(f, section);
                u64 name_idx = DwarfSectionRead_uleb128(f, section);
                printf("MACRO: start file\n");
                printf(" => line_num: %lu\n", line_num);
                printf(" => name_idx: %lu\n", name_idx);
            }
            break;
            case DW_MACRO_end_file:
            {
                printf("MACRO: end file\n");
            }
            break;
            case DW_MACRO_import:
            {
                DwarfSectionRead_Data(f, section, &import_offset, header.address_size_bytes);
                printf("MACRO: import offset\n");
                printf(" => 0x%lx\n", import_offset);
                prev_section_pos = section->pos;
                section->pos = import_offset;
                misty_macros(mountain, f);
                section->pos = prev_section_pos;
            }
            break;
            case DW_MACRO_define_sup:
            {
                Assert(!"Need to read sup file");
            }
            break;
            case DW_MACRO_undef_sup:
            {
                Assert(!"Need to read sup file");
            }
            break;
            case DW_MACRO_import_sup:
            {
                Assert(!"Need to read sup file");
            }
            break;
            case DW_MACRO_lo_user:
            {
                Assert(!"MACRO: lo user\n");
            }
            break;
            case DW_MACRO_hi_user:
            {
                Assert(!"MACRO: hi user\n");
            }
            break;
            case 0:
            {
                if (import_offset) {
                    import_offset = 0;
                    section->pos = prev_section_pos;
                }
            }
            break;
            default:
                Assert(!"Invalid Opcode!!!");
        }
    } while (opcode != 0);
}
