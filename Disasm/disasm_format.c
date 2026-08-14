internal String _disasm_reg_format(Arena* arena, Disasm_Reg reg) {
    return string_skip(String(disasm_reg_stringify(reg)), sizeof("DISASM_REG"));
}

internal String _disasm_seg_format(Arena* arena, Disasm_Segment seg) {
    String str = string_skip(String(disasm_seg_stringify(seg)), sizeof("DISASM_SEG"));
    return string_to_lower(arena, str);
}

internal String _disasm_mem_format(Arena* arena, Disasm_Operand operand, Disasm_Opcode opcode) {
    String base_reg = _disasm_reg_format(arena, operand.mem.base_reg);
    String str;
    StringBuilderBlock(arena, str) {
        switch(operand.size_bytes) {
            case 1: string_builder_append(arena, &str, String("b_ptr ")); break;
            case 2: string_builder_append(arena, &str, String("w_ptr ")); break;
            case 4: string_builder_append(arena, &str, String("d_ptr ")); break;
            case 8: string_builder_append(arena, &str, String("q_ptr ")); break;
            case 10: string_builder_append(arena, &str, String("t_ptr ")); break;
        }

        if (operand.mem.segment != DISASM_SEG_NONE) {
            string_builder_append(arena, &str, _disasm_seg_format(arena, operand.mem.segment));
            string_builder_append_char(arena, &str, ':');
        }

        string_builder_append_char(arena, &str, '[');

        b8 empty_brackets = 1;
        if (operand.mem.base_reg != DISASM_REG_NONE) {
            string_builder_append(arena, &str, base_reg);
            empty_brackets = 0;
        }

        if (operand.mem.idx_reg != DISASM_REG_NONE) {
            if (!empty_brackets) string_builder_append_char(arena, &str, '+');
            if (operand.mem.scale > 1) {
                string_builder_append(arena, &str, _disasm_reg_format(arena, operand.mem.idx_reg));
                string_builder_append_char(arena, &str, '*');
                string_builder_append_int(arena, &str, operand.mem.scale, 10);
            } else {
                string_builder_append(arena, &str, _disasm_reg_format(arena, operand.mem.idx_reg));
            }
            empty_brackets = 0;
        }

        if (empty_brackets) {
            string_builder_append(arena, &str, String("0x"));
            string_builder_append_int(arena, &str, operand.mem.displacement, 16);
        } else if (operand.mem.displacement != 0) {
            string_builder_append_char(arena, &str, (operand.mem.displacement < 0) ? '-' : '+');
            string_builder_append(arena, &str, String("0x"));
            string_builder_append_int(arena, &str, Abs(operand.mem.displacement), 16);
        }

        string_builder_append_char(arena, &str, ']');
    }
    return str;
}

internal String _disasm_imm_format(Arena* arena, Disasm_Operand operand) {
    String str;
    StringBuilderBlock(arena, str) {
        if (operand.imm < 0) string_builder_append_char(arena, &str, '-');
        string_builder_append(arena, &str, String("0x"));
        string_builder_append_int(arena, &str, Abs(operand.imm), 16);
    }
    return str;
}

internal String _disasm_rel_format(Arena* arena, Disasm_Operand operand, u8 instr_len, u64 addr) {
    String str;
    StringBuilderBlock(arena, str) {
        i64 rel = operand.rel + instr_len + addr;
        if (rel < 0) string_builder_append_char(arena, &str, '-');
        string_builder_append(arena, &str, String("0x"));
        string_builder_append_int(arena, &str, Abs(rel), 16);
    }
    return str;
}

String disasm_opcode_format(Arena* arena, Disasm_Opcode opcode) {
    String mnemonic = string_skip(String(disasm_opcode_stringify(opcode)), sizeof("DISASM"));
    return string_to_lower(arena, mnemonic);
}

String disasm_operand_format(Arena* arena, u64 addr, Disasm_Instr instr, u8 operand_idx) {
    Disasm_Operand operand = instr.operand[operand_idx];
    switch (operand.type) {
        case DISASM_OP_TYPE_REG: return _disasm_reg_format(arena, operand.reg);
        case DISASM_OP_TYPE_IMM: return _disasm_imm_format(arena, operand);
        case DISASM_OP_TYPE_MEM: return _disasm_mem_format(arena, operand, instr.opcode);
        case DISASM_OP_TYPE_REL: return _disasm_rel_format(arena, operand, instr.instr_len, addr);
        default:                 return String("");
    }
}
