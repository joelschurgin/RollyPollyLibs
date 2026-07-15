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

internal String DwarfSectionRead_String_no_copy(File* f, Misty_Section* section) {
    u64 num_bytes_read = 0;
    String s = file_read_cstring_no_copy(f, section->offset + section->pos, &num_bytes_read);
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
        case DW_FORM_udata:
        {
            val.type = MISTY_UINT;
            val.val.uint = DwarfSectionRead_uleb128(f, section);
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

Misty_Value misty_read_line_content_description(Misty* mountain, File* f, Misty_Section* section, Misty_LineHeaderEntryFormat* entry_format) {
    Misty_Value val = {0};
    switch (entry_format->type) {
        case DW_LNCT_path:
            val = misty_read_form_val(mountain, f, section, entry_format->form);
            Assert(val.type == MISTY_STRING);
        break;
        case DW_LNCT_directory_index:
            val = misty_read_form_val(mountain, f, section, entry_format->form);
            Assert(val.type == MISTY_UINT);
        break;
        default:
            Assert(!"Line Content Type Not Handled");
    }
    return val;
}

internal void _misty_read_line_info_header_dirs_and_files_dwarf5(Misty* mountain, File* f, Misty_Section* section) {
    // dirs
    {
        Misty_LineHeaderEntryFormatArray dir_entry_format = {0};
        DwarfSectionRead_Data(f, section, &dir_entry_format.count, 1);
        dir_entry_format = Array(mountain->arena, Misty_LineHeaderEntryFormat, dir_entry_format.count);

        for EachElement(entry_format, Misty_LineHeaderEntryFormat, dir_entry_format) {
            entry_format->type = DwarfSectionRead_uleb128(f, section);
            entry_format->form = DwarfSectionRead_uleb128(f, section);
        }

        mountain->dirs.count = DwarfSectionRead_uleb128(f, section);
        mountain->dirs = Array(mountain->arena, String, mountain->dirs.count);

        for (u64 dir_idx = 0; dir_idx < mountain->dirs.count; dir_idx++) {
            for EachElement(entry_format, Misty_LineHeaderEntryFormat, dir_entry_format) {
                Misty_Value val = misty_read_line_content_description(mountain, f, section, entry_format);
                mountain->dirs.data[dir_idx] = val.val.str;
            }
        }
    }

    // file paths
    {
        Misty_LineHeaderEntryFormatArray file_entry_format = {0};
        DwarfSectionRead_Data(f, section, &file_entry_format.count, 1);
        file_entry_format = Array(mountain->arena, Misty_LineHeaderEntryFormat, file_entry_format.count);

        for EachElement(entry_format, Misty_LineHeaderEntryFormat, file_entry_format) {
            entry_format->type = DwarfSectionRead_uleb128(f, section);
            entry_format->form = DwarfSectionRead_uleb128(f, section);
        }

        mountain->file_paths.count = DwarfSectionRead_uleb128(f, section);
        mountain->file_paths = Array(mountain->arena, Misty_FilePath, mountain->file_paths.count);

        for (u64 file_idx = 0; file_idx < mountain->file_paths.count; file_idx++) {
            for EachElement(entry_format, Misty_LineHeaderEntryFormat, file_entry_format) {
                Misty_Value val = misty_read_line_content_description(mountain, f, section, entry_format);
                if (val.type == MISTY_STRING) {
                    mountain->file_paths.data[file_idx].file_name = val.val.str;
                } else if (val.type == MISTY_UINT) {
                    mountain->file_paths.data[file_idx].dir_idx = val.val.uint;
                } else {
                    Assert(!"Unhandled type for mountain->file_paths");
                }
            }
        }
    }
}

internal void _misty_read_line_info_header_dirs_and_files_dwarf234(Misty* mountain, File* f, Misty_Section* section) {
    // dirs
    {
        // count
        u64 section_pos = section->pos;

        String dir = {0};
        u64 num_dirs = 0;
        do {
            dir = DwarfSectionRead_String_no_copy(f, section);
            num_dirs++;
        } while (dir.size > 0);

        section->pos = section_pos;

        // read
        mountain->dirs = Array(mountain->arena, String, num_dirs+1);
        for (u64 dir_idx = 1; dir_idx < num_dirs; dir_idx++) {
            mountain->dirs.data[dir_idx] = DwarfSectionRead_String(mountain->arena, f, section);
            String dir = mountain->dirs.data[dir_idx];
            printf("%d: %.*s\n", dir_idx, dir.size, dir.str);
        }

        section->pos += 1;
    }

    // files
    {
        // count
        u64 section_pos = section->pos;

        String file_path = {0};
        u64 num_file_paths = 0;
        do {
            file_path = DwarfSectionRead_String_no_copy(f, section);
            (void)DwarfSectionRead_uleb128(f, section);
            (void)DwarfSectionRead_uleb128(f, section);
            (void)DwarfSectionRead_uleb128(f, section);
            num_file_paths++;
        } while (file_path.size > 0);

        section->pos = section_pos;

        // read
        mountain->file_paths = Array(mountain->arena, Misty_FilePath, num_file_paths+1);
        for (u64 file_path_idx = 1; file_path_idx < num_file_paths; file_path_idx++) {
            mountain->file_paths.data[file_path_idx].file_name = DwarfSectionRead_String(mountain->arena, f, section);
            mountain->file_paths.data[file_path_idx].dir_idx   = DwarfSectionRead_uleb128(f, section);
            (void)DwarfSectionRead_uleb128(f, section);
            (void)DwarfSectionRead_uleb128(f, section);
            String file_path = mountain->file_paths.data[file_path_idx].file_name;
            printf("%d: %.*s\n", file_path_idx, file_path.size, file_path.str);
        }
    }

    section->pos += 1;
}


Misty_LineInfoHeader misty_read_line_info_header(Misty* mountain, File* f, Misty_Section* section) {
    Misty_LineInfoHeader header = {0};
    header.unit_length = misty_read_initial_length_value(f, section, &header.type);

    section->addr_type = header.type;

    DwarfSectionRead_Struct(f, section, header.version);
    Assert(header.version >= 2 && header.version <= 5);

    if (header.version == 5) {
        DwarfSectionRead_Struct(f, section, header.address_size_bytes);
        DwarfSectionRead_Struct(f, section, header.segment_selector_size_bytes);
    } else {
        header.address_size_bytes = (header.type == DWARF_32) ? 4 : 8;
    }

    DwarfSectionRead_Addr(f, section, header.header_length, header.type);

    DwarfSectionRead_Struct(f, section, header.min_instr_length);
    if (header.version >= 4) {
        DwarfSectionRead_Struct(f, section, header.max_ops_per_instr);
    } else {
        header.max_ops_per_instr = 1;
    }
    DwarfSectionRead_Struct(f, section, header.default_is_stmt);
    DwarfSectionRead_Struct(f, section, header.line_base);
    DwarfSectionRead_Struct(f, section, header.line_range);

    DwarfSectionRead_Struct(f, section, header.opcode_base);

    header.standard_opcode_lengths = Array(mountain->arena, u8, header.opcode_base-1);
    DwarfSectionRead_Array(f, section, header.standard_opcode_lengths);

    if (header.version == 5) {
        _misty_read_line_info_header_dirs_and_files_dwarf5(mountain, f, section);
    } else if (header.version >= 2) {
        _misty_read_line_info_header_dirs_and_files_dwarf234(mountain, f, section);
    }

    return header;
}

typedef struct {
    u64 addr;
    u64 op_idx;
    u64 file_idx;
    u64 line;
    u64 column;
    u64 isa;
    u64 discriminator;
    b8 is_stmt;
    b8 basic_block;
    b8 end_sequence;
    b8 prologue_end;
    b8 epilogue_begin;
} Misty_LineInfoState;

#define Misty_PrintLineInfoState printf("0x%08llx %6d %6d %6d %3d %13d %6d\n", state.addr, state.line, state.column, state.file_idx, state.isa, state.discriminator, state.op_idx, state.is_stmt)

void misty_read_line_info(Misty* mountain, File* f) {
    Misty_Section* section = &misty_section(mountain, debug_line);
    Misty_LineInfoHeader header = misty_read_line_info_header(mountain, f, section);
    Misty_LineInfoState state = {
        .addr = 0,
        .op_idx = 0,
        .file_idx = 1,
        .line = 1,
        .column = 0,
        .isa = 0,
        .discriminator = 0,
        .is_stmt = header.default_is_stmt,
        .basic_block = 0,
        .end_sequence = 0,
        .prologue_end = 0,
        .epilogue_begin = 0,
    };

    printf("Address    Line   Column File   ISA Discriminator OpIndex Flags \n");
    printf("---------- ------ ------ ------ --- ------------- ------- ------\n");
    for (; !state.end_sequence ;) {
        u8 opcode = 0;
        DwarfSectionRead_Struct(f, section, opcode);

        if (opcode == 0) {
            u8 ext_opcode_length = DwarfSectionRead_uleb128(f, section);
            u8 ext_opcode[ext_opcode_length];
            DwarfSectionRead_Data(f, section, &ext_opcode, ext_opcode_length);
            switch (ext_opcode[0]) {
                case DW_LNE_end_sequence:
                    state.end_sequence = true;
                break;
                case DW_LNE_set_address:
                    Assert(sizeof(state.addr) >= (u64)(ext_opcode_length-1));
                    MemoryCopy(&state.addr, ext_opcode + 1, header.address_size_bytes);
                    state.op_idx = 0;
                break;
                case DW_LNE_define_file:
                case DW_LNE_set_discriminator:
                case DW_LNE_lo_user:
                case DW_LNE_hi_user:
                case DW_LNE_NVIDIA_inlined_call:
                case DW_LNE_NVIDIA_set_function_name:
                default:
                    Assert(!"Unhandled line program extended opcode");
            }
        } else if (opcode < header.opcode_base) {
            DwarfLineNumberStandardOpcode standard_opcode = (DwarfLineNumberStandardOpcode)opcode;
            switch (standard_opcode) {
                case DW_LNS_copy:
                    state.discriminator = 0;
                    state.basic_block = 0;
                    state.prologue_end = 0;
                    state.epilogue_begin = 0;
                break;
                case DW_LNS_advance_pc:
                {
                    u64 op_adv = DwarfSectionRead_uleb128(f, section);
                    state.addr += header.min_instr_length * ((state.op_idx + op_adv) / header.max_ops_per_instr);
                    state.op_idx = (state.op_idx + op_adv) % header.max_ops_per_instr;
                }
                break;
                case DW_LNS_advance_line:
                    state.line += DwarfSectionRead_uleb128(f, section);
                break;
                case DW_LNS_set_file:
                    state.file_idx = DwarfSectionRead_uleb128(f, section);
                break;
                case DW_LNS_set_column:
                    state.column = DwarfSectionRead_uleb128(f, section);
                break;
                case DW_LNS_negate_stmt:
                    state.is_stmt = !state.is_stmt;
                break;
                case DW_LNS_set_basic_block:
                    state.basic_block = 1;
                break;
                case DW_LNS_const_add_pc:
                {
                    u8 adjusted_opcode = 255 - header.opcode_base;
                    u64 op_adv = adjusted_opcode / header.line_range;

                    state.addr += header.min_instr_length * ((state.op_idx + op_adv) / header.max_ops_per_instr);
                    state.op_idx = (state.op_idx + op_adv) % header.max_ops_per_instr;
                }
                break;
                case DW_LNS_set_prologue_end:
                    state.prologue_end = 1;
                break;
                case DW_LNS_set_epilogue_begin:
                    state.epilogue_begin = 1;
                break;
                case DW_LNS_set_isa:
                    state.isa = 1;
                break;
                case DW_LNS_fixed_advance_pc:
                    Assert(!"Unhandled line program standard opcode");
                break;
                default:
                {
                    u8 num_ops = header.standard_opcode_lengths.data[opcode - 1];
                    for (u8 i = 0; i < num_ops; i++) {
                        (void)DwarfSectionRead_uleb128(f, section);
                    }
                    Assert(!"Unhandled line program standard opcode");
                }
            }
        } else {
            // special opcode
            u8 adjusted_opcode = opcode - header.opcode_base;
            u64 op_adv = adjusted_opcode / header.line_range;

            state.addr += header.min_instr_length * ((state.op_idx + op_adv) / header.max_ops_per_instr);
            state.op_idx = (state.op_idx + op_adv) % header.max_ops_per_instr;
            state.line += header.line_base + (adjusted_opcode % header.line_range);

            state.discriminator = 0;
            state.basic_block = 0;
            state.prologue_end = 0;
            state.epilogue_begin = 0;
 
            Misty_PrintLineInfoState;
        }
    }
    Misty_PrintLineInfoState;
}


#undef DwarfSectionRead_Data
#undef DwarfSectionRead_Struct
