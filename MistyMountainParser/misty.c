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

    for EachElement(sec, Misty_Section, mountain->sections) {
        sec->pos = Array(mountain->arena, u64, LaneCount());
    }

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
            else if (string_equal(debug_name, String("abbrev")))        mountain->debug_abbrev      = section_idx;
            else if (string_equal(debug_name, String("info")))          mountain->debug_info        = section_idx;
        }
    }
}

#define DWARF_HEADER_OFFSET_SIZE_FLAG            0b001
#define DWARF_HEADER_LINE_OFFSET_FLAG            0b010
#define DWARF_HEADER_OPCODE_OPERANDS_TABLE_FLAG  0b100

#define Misty_SectionPos(section) ((section)->pos.data[LaneIdx()])

#define DwarfSectionRead_Data(file, section, output_value_ptr, num_bytes) do { \
        file_read_bytes((file), (section)->offset + Misty_SectionPos(section), (output_value_ptr), (num_bytes)); \
        Misty_SectionPos(section) += (num_bytes); \
    } while (0)
#define DwarfSectionRead_Struct(file, section, output_value) DwarfSectionRead_Data(file, section, &output_value, sizeof(output_value));
#define DwarfSectionRead_Addr(file, section, output_value, type) DwarfSectionRead_Data(file, section, &output_value, ((type) == DWARF_64) ? sizeof(u64) : sizeof(u32))
#define DwarfSectionRead_Array(file, section, arr) DwarfSectionRead_Data(file, section, arr.data, arr.count * sizeof(*arr.data))

internal String DwarfSectionRead_String(Arena* arena, File* f, Misty_Section* section) {
    u64 num_bytes_read = 0;
    String s = file_read_cstring(arena, f, section->offset + Misty_SectionPos(section), &num_bytes_read);
    Misty_SectionPos(section) += num_bytes_read;
    return s;
}

internal String DwarfSectionRead_String_no_copy(File* f, Misty_Section* section) {
    u64 num_bytes_read = 0;
    String s = file_read_cstring_no_copy(f, section->offset + Misty_SectionPos(section), &num_bytes_read);
    Misty_SectionPos(section) += num_bytes_read;
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

internal i64 read_sleb128(u8* data, u64* num_bytes_read) {
    u64 result = 0;
    u64 shift = 0;
    u64 count = 0;
    u8 byte;
    for (;;) {
        byte = data[count++];
        result |= (u64)(byte & 0x7f) << shift;
        shift += 7;
        if ((byte & 0x80) == 0) break;
    }

    if (shift < 64 && (byte & 0x40)) {
        result |= (~0ULL << shift);
    }
 
    *num_bytes_read = count;
    return (i64)result;
}

internal u64 DwarfSectionRead_uleb128(File* f, Misty_Section* section) {
    u64 num_bytes_read = 0;
    u64 result = read_uleb128(f->data + section->offset + Misty_SectionPos(section), &num_bytes_read);
    Misty_SectionPos(section) += num_bytes_read;
    return result;
}

internal i64 DwarfSectionRead_sleb128(File* f, Misty_Section* section) {
    u64 num_bytes_read = 0;
    i64 result = read_sleb128(f->data + section->offset + Misty_SectionPos(section), &num_bytes_read);
    Misty_SectionPos(section) += num_bytes_read;
    return result;
}

Misty_Value misty_read_form_val(Misty* mountain, File* f, Misty_Section* section, DwarfFormType form, u8 addr_size_bytes) {
    Misty_Value val = {0};

    switch (form) {
        case DW_FORM_string:
        {
            val.type = MISTY_STRING;
            val.val.str =  DwarfSectionRead_String(mountain->arena, f, section);
        }
        break;
        case DW_FORM_strp:
        {
            val.type = MISTY_STRING;
            u64 offset = 0;
            DwarfSectionRead_Addr(f, section, offset, section->addr_type);
            val.val.str = DwarfSectionRead_StringAt(mountain->arena, f, &misty_section(mountain, debug_str), offset);
        }
        break;
        case DW_FORM_line_strp:
        {
            val.type = MISTY_STRING;
            u64 offset = 0;
            DwarfSectionRead_Addr(f, section, offset, section->addr_type);
            val.val.str = DwarfSectionRead_StringAt(mountain->arena, f, &misty_section(mountain, debug_line_str), offset);
        }
        break;
        case DW_FORM_ref1:
        {
            val.type = MISTY_REF;
            DwarfSectionRead_Data(f, section, &val.val.offset, 1);
        }
        break;
        case DW_FORM_ref2:
        {
            val.type = MISTY_REF;
            DwarfSectionRead_Data(f, section, &val.val.offset, 2);
        }
        break;
        case DW_FORM_ref4:
        {
            val.type = MISTY_REF;
            DwarfSectionRead_Data(f, section, &val.val.offset, 4);
        }
        break;
        case DW_FORM_ref8:
        {
            val.type = MISTY_REF;
            DwarfSectionRead_Data(f, section, &val.val.offset, 8);
        }
        break;
        case DW_FORM_ref_udata:
        {
            val.type = MISTY_REF;
            val.val.offset = DwarfSectionRead_uleb128(f, section);
        }
        break;
        case DW_FORM_data1:
        {
            val.type = MISTY_UINT;
            DwarfSectionRead_Data(f, section, &val.val.offset, 1);
        }
        break;
        case DW_FORM_data2:
        {
            val.type = MISTY_UINT;
            DwarfSectionRead_Data(f, section, &val.val.uint, 2);
        }
        break;
        case DW_FORM_data4:
        {
            val.type = MISTY_UINT;
            DwarfSectionRead_Data(f, section, &val.val.uint, 4);
        }
        break;
        case DW_FORM_data8:
        {
            val.type = MISTY_UINT;
            DwarfSectionRead_Data(f, section, &val.val.uint, 8);
        }
        break;
        case DW_FORM_data16:
        {
            val.type = MISTY_UINT;
            DwarfSectionRead_Data(f, section, &val.val.uint, 16);
        }
        break;
        case DW_FORM_udata:
        {
            val.type = MISTY_UINT;
            val.val.uint = DwarfSectionRead_uleb128(f, section);
        }
        break;
        case DW_FORM_addr:
        {
            val.type = MISTY_ADDR;
            DwarfSectionRead_Data(f, section, &val.val.addr, addr_size_bytes);
        }
        break;
        case DW_FORM_sec_offset:
        {
            val.type = MISTY_ADDR;
            DwarfSectionRead_Data(f, section, &val.val.addr, (section->addr_type == DWARF_32) ? 4 : 8);
        }
        break;
        case DW_FORM_flag_present:
        {
            val.type = MISTY_FLAG;
            val.val.flag = 1;
        }
        break;
        case DW_FORM_exprloc:
        {
            // DWARF5 pg 26 for expressions
            u64 length = DwarfSectionRead_uleb128(f, section);
            TempArenaBlock(mountain->arena) {
                u8Array expr_bytes = Array(mountain->arena, u8, length);
                DwarfSectionRead_Data(f, section, expr_bytes.data, expr_bytes.count);

                for (u64 idx = 0; idx < expr_bytes.count; idx++) {
                    DwarfOpType op = expr_bytes.data[idx];
                    switch (op) {
                        case DW_OP_call_frame_cfa:
                            // not sure what needs to happen here yet
                        break;
                        case DW_OP_fbreg:
                        {
                            val.type = MISTY_SINT;
                            u64 num_bytes_read = 0;
                            val.val.sint = read_sleb128(&expr_bytes.data[idx+1], &num_bytes_read);
                            idx += num_bytes_read;
                        }
                        break;
                        default:
                            TODO("DW_OP not handled");
                    }
                }
            }
        }
        break;
        default:
            TODO("Form Type Not Handled");
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
    u8 addr_size_bytes = (section->addr_type == DWARF_32) ? 4 : 8;
    switch (entry_format->type) {
        case DW_LNCT_path:
            val = misty_read_form_val(mountain, f, section, entry_format->form, addr_size_bytes);
            Assert(val.type == MISTY_STRING);
        break;
        case DW_LNCT_directory_index:
            val = misty_read_form_val(mountain, f, section, entry_format->form, addr_size_bytes);
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
        u64 section_pos = Misty_SectionPos(section);

        String dir = {0};
        u64 num_dirs = 0;
        do {
            dir = DwarfSectionRead_String_no_copy(f, section);
            num_dirs++;
        } while (dir.size > 0);

        Misty_SectionPos(section) = section_pos;

        // read
        mountain->dirs = Array(mountain->arena, String, num_dirs+1);
        for (u64 dir_idx = 1; dir_idx < num_dirs; dir_idx++) {
            mountain->dirs.data[dir_idx] = DwarfSectionRead_String(mountain->arena, f, section);
            String dir = mountain->dirs.data[dir_idx];
            printf("%d: %.*s\n", dir_idx, dir.size, dir.str);
        }

        Misty_SectionPos(section) += 1;
    }

    // files
    {
        // count
        u64 section_pos = Misty_SectionPos(section);

        String file_path = {0};
        u64 num_file_paths = 0;
        do {
            file_path = DwarfSectionRead_String_no_copy(f, section);
            (void)DwarfSectionRead_uleb128(f, section);
            (void)DwarfSectionRead_uleb128(f, section);
            (void)DwarfSectionRead_uleb128(f, section);
            num_file_paths++;
        } while (file_path.size > 0);

        Misty_SectionPos(section) = section_pos;

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

    Misty_SectionPos(section) += 1;
}


Misty_LineInfoHeader misty_read_line_info_header(Misty* mountain, File* f, Misty_Section* section) {
    Misty_LineInfoHeader header = {0};
    header.unit_length = misty_read_initial_length_value(f, section, &header.type);

    section->addr_type = header.type;

    DwarfSectionRead_Struct(f, section, header.version);
    Assert(header.version >= 2 && header.version <= 5);

    if (header.version == 5) {
        DwarfSectionRead_Struct(f, section, header.addr_size_bytes);
        DwarfSectionRead_Struct(f, section, header.segment_selector_size_bytes);
    } else {
        header.addr_size_bytes = (header.type == DWARF_32) ? 4 : 8;
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

Misty_LineInfoArray misty_read_line_info(Misty* mountain, File* f) {
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

    Misty_LineInfoArray line_info = {0};
    ArrayBuilderBlock(mountain->arena, line_info, Misty_LineInfo) {

        array_builder_push(mountain->arena, line_info, (Misty_LineInfo){0});

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
                        MemoryCopy(&state.addr, ext_opcode + 1, header.addr_size_bytes);
                        state.op_idx = 0;
                    break;
                    case DW_LNE_set_discriminator:
                        Misty_SectionPos(section) -= ext_opcode_length - 1;
                        state.discriminator = DwarfSectionRead_uleb128(f, section);
                    break;
                    case DW_LNE_define_file:
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

                array_builder_push(mountain->arena, line_info, ((Misty_LineInfo){
                    .addr = state.addr,
                    .line = state.line,
                    .column = state.column,
                    .file_idx = state.file_idx,
                }));
            }
        }
        array_builder_push(mountain->arena, line_info, ((Misty_LineInfo){
            .addr = state.addr,
            .line = state.line,
            .column = state.column,
            .file_idx = state.file_idx,
        }));
    }

    return line_info;
}

void misty_die_read_abbrev_offsets(Misty* mountain, File* f, Misty_Die* die) {
    Misty_Section* section = &misty_section(mountain, debug_abbrev);
    Misty_SectionPos(section) = die->header.debug_abbrev_offset;
    section->addr_type = die->header.type;

    ArrayBuilderBlock(mountain->arena, die->abbrev_offsets, u64) {
        array_builder_push(mountain->arena, die->abbrev_offsets, Misty_SectionPos(section));
        while (1) {
            u64 abbrev_idx = DwarfSectionRead_uleb128(f, section);
            if (abbrev_idx == 0) break;
            array_builder_push(mountain->arena, die->abbrev_offsets, Misty_SectionPos(section));

            DwarfTagType tag = DwarfSectionRead_uleb128(f, section);

            DwarfHasChild has_child = DwarfSectionRead_uleb128(f, section);

            DwarfAttributeType attribute;
            DwarfFormType form;
            u64 implicit_const = 0;
            while (1) {
                attribute = DwarfSectionRead_uleb128(f, section);
                form = DwarfSectionRead_uleb128(f, section);
 
                if (attribute == 0 || form == 0) break;

                switch (form) {
                    case DW_FORM_implicit_const:
                        implicit_const = DwarfSectionRead_uleb128(f, section);
                    break;
                }
            }
        }
    }
}

typedef struct {
    String name;
    String producer;
    String language_name;
    String comp_dir;

    u64 language_version;

    u64 low_pc;
    u64 high_pc;

    u64 stmt_list;
    u64 macros;

    u64 byte_size;
 
    u64 file_idx;
    u64 line;
    u64 column;
 
    u64 type_ref;
    u64 sibling_ref;

    DwarfLang language;
    DwarfAttributeEncodingType encoding;

    b8 external;
    b8 prototyped;
    b8 declaration;
} Misty_AttribValues;

void misty_parse_attrib(DwarfAttributeType attrib, Misty_Value value, Misty_AttribValues* attrib_values) {
    switch (attrib) {
        case DW_AT_producer:            attrib_values->producer         =   value.val.str;                              break;
        case DW_AT_language:            attrib_values->language         =   (DwarfLang)value.val.uint;                  break;
        case DW_AT_language_name:       attrib_values->language_name    =   value.val.str;                              break;
        case DW_AT_language_version:    attrib_values->language_version =   value.val.uint;                             break;
        case DW_AT_name:                attrib_values->name             =   value.val.str;                              break;
        case DW_AT_comp_dir:            attrib_values->comp_dir         =   value.val.str;                              break;
        case DW_AT_low_pc:              attrib_values->low_pc           =   value.val.addr;                             break;
        case DW_AT_high_pc:             attrib_values->high_pc          =   value.val.addr;                             break;
        case DW_AT_stmt_list:           attrib_values->stmt_list        =   value.val.addr;                             break;
        case DW_AT_macros:              attrib_values->macros           =   value.val.addr;                             break;
        case DW_AT_byte_size:           attrib_values->byte_size        =   value.val.uint;                             break;
        case DW_AT_encoding:            attrib_values->encoding         =   (DwarfAttributeEncodingType)value.val.uint; break;
        case DW_AT_type:                attrib_values->type_ref         =   value.val.addr;                             break;
        case DW_AT_external:            attrib_values->external         =   value.val.flag;                             break;
        case DW_AT_prototyped:          attrib_values->prototyped       =   value.val.flag;                             break;
        case DW_AT_decl_file:           attrib_values->file_idx         =   value.val.uint;                             break;
        case DW_AT_decl_line:           attrib_values->line             =   value.val.uint;                             break;
        case DW_AT_decl_column:         attrib_values->column           =   value.val.uint;                             break;
        case DW_AT_declaration:         attrib_values->declaration      =   value.val.flag;                             break;
        case DW_AT_sibling:             attrib_values->sibling_ref      =   value.val.addr;                             break;
        default:
            TODO("Unhandled attrib");
    }
}

void misty_push_type(Misty* mountain, Misty_Die* die, u64 entry_addr) {
    Misty_TypeInfo* new_type = 0L;
    MutexBlock(mountain->mutex) {
        new_type = push_struct(mountain->arena, Misty_TypeInfo);
    }
    if (!die->type_info.first) {
        die->type_info.first = new_type;
        die->type_info.last = new_type;
    } else {
        SLLQueuePush(die->type_info.first, die->type_info.last, new_type);
    }

    die->type_info.last->entry_addr = entry_addr;
}

void misty_push_subprogram(Misty* mountain, Misty_Die* die, u64 entry_addr) {
    Misty_Subprogram* new_subprogram = 0L;
    MutexBlock(mountain->mutex) {
        new_subprogram = push_struct(mountain->arena, Misty_Subprogram);
    }
    if (!die->subprogram.first) {
        die->subprogram.first = new_subprogram;
        die->subprogram.last = new_subprogram;
    } else {
        SLLQueuePush(die->subprogram.first, die->subprogram.last, new_subprogram);
    }

    die->subprogram.last->entry_addr = entry_addr;
}

void misty_push_variable(Misty* mountain, Misty_Die* die, u64 entry_addr) {
    Misty_Variable* new_variable = 0L;
    MutexBlock(mountain->mutex) {
        new_variable = push_struct(mountain->arena, Misty_Variable);
    }
    if (!die->variable.first) {
        die->variable.first = new_variable;
        die->variable.last = new_variable;
    } else {
        SLLQueuePush(die->variable.first, die->variable.last, new_variable);
    }

    die->variable.last->entry_addr = entry_addr;
}

void misty_parse_die(Misty* mountain, File* f, Misty_Die* die) {
    Misty_Section* section = &misty_section(mountain, debug_info);

    // read the data
    while (1) {
        Misty_SectionPos(section) = die->start_addr;
        u64 entry_addr = Misty_SectionPos(section);

        u64 debug_info_code = DwarfSectionRead_uleb128(f, section);
        if (debug_info_code == 0) {
            break;
        }

        u64 abbrev_offset = die->abbrev_offsets.data[debug_info_code];

        Misty_Section* abbrev_section = &misty_section(mountain, debug_abbrev);
        Misty_SectionPos(abbrev_section) = abbrev_offset;

        DwarfTagType tag = DwarfSectionRead_uleb128(f, abbrev_section);

        DwarfHasChild has_child = DwarfSectionRead_uleb128(f, abbrev_section);

        DwarfAttributeType attrib;
        DwarfFormType form;
        Misty_Value value;
        Misty_AttribValues attrib_values = {0};

        while(1) {
            attrib = DwarfSectionRead_uleb128(f, abbrev_section);
            form = DwarfSectionRead_uleb128(f, abbrev_section);

            if (attrib == 0 || form == 0) break;

            if (form == DW_FORM_implicit_const) {
                value.type = MISTY_UINT;
                value.val.uint = DwarfSectionRead_sleb128(f, abbrev_section);
            } else {
                MutexBlock(mountain->mutex) {
                    value = misty_read_form_val(mountain, f, section, form, die->header.addr_size_bytes);
                }
            }

            misty_parse_attrib(attrib, value, &attrib_values);
        }

        switch (tag) {
            case DW_TAG_compile_unit:
            {
                die->comp_unit.name             = attrib_values.name;
                die->comp_unit.producer         = attrib_values.producer;
                die->comp_unit.language_name    = attrib_values.language_name;
                die->comp_unit.comp_dir         = attrib_values.comp_dir;
                die->comp_unit.language_version = attrib_values.language_version;
                die->comp_unit.low_pc           = attrib_values.low_pc;
                die->comp_unit.high_pc          = attrib_values.high_pc;
                die->comp_unit.stmt_list        = attrib_values.stmt_list;
                die->comp_unit.macros           = attrib_values.macros;
                die->comp_unit.language         = attrib_values.language;
            }
            break;
            case DW_TAG_base_type:
            {
                misty_push_type(mountain, die, entry_addr);
                die->type_info.last->name      = attrib_values.name;
                die->type_info.last->encoding  = attrib_values.encoding;
                die->type_info.last->byte_size = attrib_values.byte_size;
            }
            break;
            case DW_TAG_const_type:
            {
                misty_push_type(mountain, die, entry_addr);
                die->type_info.last->modifier = MISTY_CONST;
                die->type_info.last->type_ref = attrib_values.type_ref;
            }
            break;
            case DW_TAG_pointer_type:
            {
                misty_push_type(mountain, die, entry_addr);
                die->type_info.last->modifier = MISTY_POINTER;
                die->type_info.last->type_ref = attrib_values.type_ref;
            }
            break;
            case DW_TAG_subprogram:
            {
                misty_push_subprogram(mountain, die, entry_addr);
                die->subprogram.last->name       = attrib_values.name;
                die->subprogram.last->file_idx   = attrib_values.file_idx;
                die->subprogram.last->line       = attrib_values.line;
                die->subprogram.last->column     = attrib_values.column;
                die->subprogram.last->prototyped = attrib_values.prototyped;
                die->subprogram.last->external   = attrib_values.external;
            }
            break;
            case DW_TAG_formal_parameter:
            {
                misty_push_variable(mountain, die, entry_addr);
                die->variable.last->name     = attrib_values.name;
                die->variable.last->file_idx = attrib_values.file_idx;
                die->variable.last->line     = attrib_values.line;
                die->variable.last->column   = attrib_values.column;
                die->variable.last->type_ref = attrib_values.type_ref;
            }
            break;
            case DW_TAG_unspecified_parameters:
            {
                die->subprogram.last->unspecified_params = 1;
            }
            break;
            default:
                TODO("Unhandled DW_TAG");
        }
    }
}

void misty_read_debug_info(Misty* mountain, File* f) {
    // read CU header DWARF5.pdf page 200
    Misty_Section* section = &misty_section(mountain, debug_info);

    AssignLane(0) {
        // header
        ArrayBuilderBlock(mountain->arena, mountain->die_arr, Misty_Die) 
        for (Misty_SectionPos(section) = 0; Misty_SectionPos(section) < section->size; ){
            Misty_Die die = {0};

            {
                u32 unit_length_32 = 0;
                DwarfSectionRead_Struct(f, section, unit_length_32);
                if (unit_length_32 < 0xfffffff0) {
                    die.header.unit_length = (u64)unit_length_32;
                    die.header.type = DWARF_32;
                } else { 
                    DwarfSectionRead_Struct(f, section, die.header.unit_length);
                    die.header.type = DWARF_64;
                }
                section->addr_type = die.header.type;
            }
 
            u64 section_pos = Misty_SectionPos(section);

            DwarfSectionRead_Struct(f, section, die.header.version);

            {
                u8 unit_type = 0;
                DwarfSectionRead_Struct(f, section, unit_type);
                die.header.unit_type = (DwarfUnitType)unit_type;
            }

            DwarfSectionRead_Struct(f, section, die.header.addr_size_bytes);
            DwarfSectionRead_Data(f, section, &die.header.debug_abbrev_offset, (die.header.type == DWARF_32) ? 4 : 8);

            switch (die.header.unit_type) {
                case DW_UT_compile:
                    // no more bytes to read for the die.header
                    die.start_addr = Misty_SectionPos(section);;
                break;
                default:
                    TODO("Handle Other DWARF_UT types");
            }

            array_builder_push(mountain->arena, mountain->die_arr, die);

            Misty_SectionPos(section) = section_pos + die.header.unit_length;
        }
    }
    LaneSync();

    {
        ThreadArraySplit split = ThreadArraySplit(mountain->die_arr.count);
        for (u64 i = split.start_idx; i < split.end_idx; i++) {
            Misty_Die* die = mountain->die_arr.data + i;
            misty_die_read_abbrev_offsets(mountain, f, die);
            misty_parse_die(mountain, f, die);
        }
    }
    LaneSync();
}

#undef DwarfSectionRead_Data
#undef DwarfSectionRead_Struct
