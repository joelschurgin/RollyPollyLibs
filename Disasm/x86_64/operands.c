#define RexB(rex) (((rex) & 0x01) << 3)
#define RexX(rex) (((rex) & 0x02) << 2)
#define RexR(rex) (((rex) & 0x04) << 1)
#define RexW(rex)  ((rex) & 0x08)

#define GetMod(byte) (((byte) & 0b11000000) >> 6)
#define GetReg(byte) (((byte) & 0b00111000) >> 3)
#define GetRM(byte) (((byte) & 0b00000111))

#define DisasmInvalid do { instr.opcode = DISASM_INVALID; \
                        instr.instr_len = 1 + prefix.count; \
                        instr.num_operands = 0; } while (0)

#define InstrNext (instr_ptr + instr.instr_len)
#define ModRMBytePtr (instr_ptr + 1)

internal Disasm_Opcode _disasm_group1_mnemonic(u8* byte) {
    u8 reg = (*byte & 0b00111000) >> 3;
    switch (reg) {
        case 0: return DISASM_ADD;
        case 1: return DISASM_OR;
        case 2: return DISASM_ADC;
        case 3: return DISASM_SBB;
        case 4: return DISASM_AND;
        case 5: return DISASM_SUB;
        case 6: return DISASM_XOR;
        case 7: return DISASM_CMP;
    }
	return DISASM_INVALID;
}

internal Disasm_Opcode _disasm_group2_mnemonic(u8* byte) {
    u8 reg = (*byte & 0b00111000) >> 3;
    switch (reg) {
        case 0: return DISASM_ROL;
        case 1: return DISASM_ROR;
        case 2: return DISASM_RCL;
        case 3: return DISASM_RCR;
        case 4: return DISASM_SHL;
        case 5: return DISASM_SHR;
        case 6: return DISASM_SHL;
        case 7: return DISASM_SAR;
    }
	return DISASM_INVALID;
}

internal Disasm_Opcode _disasm_group3_mnemonic(u8* byte) {
    u8 reg = (*byte & 0b00111000) >> 3;
    switch (reg) {
        case 0: return DISASM_TEST;
        case 3: return DISASM_NOT;
        case 4: return DISASM_NEG;
        case 5: return DISASM_MUL;
        case 6: return DISASM_IMUL;
        case 7: return DISASM_DIV;
        case 8: return DISASM_IDIV;
    }
	return DISASM_INVALID;
}

internal inline u8 _disasm_operand_16_32_64_size(Disasm_Prefix prefix) {
    u8 size_bytes = 4;
    if ((prefix.rex & 0x08) == 0x08) size_bytes = 8;
    else if (prefix.op_override) size_bytes = 2;
    return size_bytes;
}

internal inline u8 _disasm_operand_16_32_size(Disasm_Prefix prefix) {
    u8 size_bytes = 4;
    if (prefix.op_override) size_bytes = 2;
    return size_bytes;
}

internal inline u8 _disasm_operand_16_64_size(Disasm_Prefix prefix) {
    u8 size_bytes = 8;
    if (prefix.op_override) size_bytes = 2;
    return size_bytes;
}

internal inline u8 _disasm_operand_32_64_size(Disasm_Prefix prefix) {
    return (RexW(prefix.rex) == 0x08) ? 8 : 4;
}

internal inline u8 _disasm_operand_8_16_size(Disasm_Prefix prefix) {
    return (prefix.op_override) ? 2 : 4;
}

internal Disasm_Operand _disasm_decode_mem_reg(Disasm_Prefix prefix, u8 size_bytes, Disasm_Reg reg32, Disasm_Reg reg64, Disasm_Segment seg) {
    Disasm_Operand operand = {0};

    operand.type = DISASM_OP_TYPE_MEM;
    operand.size_bytes = size_bytes;
    operand.mem.base_reg = (prefix.addr_override) ? reg32 : reg64;
    operand.mem.idx_reg = DISASM_REG_NONE;
    operand.mem.scale = 0;
    operand.mem.displacement = 0;
    operand.mem.segment = seg;

    return operand;
}

internal Disasm_Operand _disasm_decode_string_src(Disasm_Prefix prefix, u8 size_bytes) {
    return _disasm_decode_mem_reg(prefix, size_bytes, DISASM_REG_ESI, DISASM_REG_RSI, prefix.segment ? prefix.segment : DISASM_SEG_DS);
}

internal Disasm_Operand _disasm_decode_string_dest(Disasm_Prefix prefix, u8 size_bytes) {
    return _disasm_decode_mem_reg(prefix, size_bytes, DISASM_REG_EDI, DISASM_REG_RDI, DISASM_SEG_ES);
}

internal Disasm_Reg _disasm_decode_reg(u8 reg_idx, u8 size_bytes, u8 rex) {
    switch (size_bytes) {
        case 1:
            if (reg_idx < 4) return DISASM_REG_AL + reg_idx;
            if (!rex) {
                if (reg_idx < 8) return DISASM_REG_AH + (reg_idx - 4);
            } else {
                if (reg_idx < 8) return DISASM_REG_SPL + (reg_idx - 4);
                return DISASM_REG_R8B + (reg_idx - 8);
            }
        break;
        case 2: return (reg_idx < 8) ? (DISASM_REG_AX + reg_idx) : (DISASM_REG_R8W + (reg_idx - 8));
        case 4: return (reg_idx < 8) ? (DISASM_REG_EAX + reg_idx) : (DISASM_REG_R8D + (reg_idx - 8));
        case 8: return (reg_idx < 8) ? (DISASM_REG_RAX + reg_idx) : (DISASM_REG_R8 + (reg_idx - 8));
    }

    return DISASM_REG_NONE;
}

internal Disasm_Operand _disasm_decode_r(u8* byte, Disasm_Prefix prefix, u8 size_bytes) {
    Disasm_Operand operand = {0};

    operand.type = DISASM_OP_TYPE_REG;
    operand.size_bytes = size_bytes;

    u8 rex = prefix.rex;
    u8 reg = (*byte & 0b00111000) >> 3;
    operand.reg = _disasm_decode_reg(reg | RexR(rex), operand.size_bytes, rex);

    return operand;
}

internal inline Disasm_Operand _disasm_decode_r8(u8* byte, Disasm_Prefix prefix) {
    return _disasm_decode_r(byte, prefix, 1);
}

internal inline Disasm_Operand _disasm_decode_r32_64(u8* byte, Disasm_Prefix prefix) {
    u8 size_bytes = _disasm_operand_32_64_size(prefix);
    return _disasm_decode_r(byte, prefix, size_bytes);
}

internal inline Disasm_Operand _disasm_decode_r16_32(u8* byte, Disasm_Prefix prefix) {
    u8 size_bytes = _disasm_operand_16_32_size(prefix);
    return _disasm_decode_r(byte, prefix, size_bytes);
}

internal inline Disasm_Operand _disasm_decode_r16_32_64(u8* byte, Disasm_Prefix prefix) {
    u8 size_bytes = _disasm_operand_16_32_64_size(prefix);
    return _disasm_decode_r(byte, prefix, size_bytes);
}


internal Disasm_Operand _disasm_decode_m(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read, u8 size_bytes) {
    u8 rex = prefix.rex;

    Disasm_Operand operand = {0};
    u8 mod = GetMod(*byte);
    u8 rm  = GetRM(*byte);
 
    *num_bytes_read += 1;
    byte += 1;

    operand.type = DISASM_OP_TYPE_MEM;
    operand.size_bytes = size_bytes;
    operand.mem.segment = prefix.segment;

    u8 reg_size = prefix.addr_override ? sizeof(u32) : sizeof(u64);
    if (rm == 4) {
        u8 sib = *byte;

        u8 scale_shift = (sib & 0b11000000) >> 6;
        u8 idx_reg     = (sib & 0b00111000) >> 3;
        u8 base        = (sib & 0b00000111);

        *num_bytes_read += 1;
        byte += 1;

        if (idx_reg != 4) {
            operand.mem.idx_reg = _disasm_decode_reg(idx_reg | RexX(prefix.rex), reg_size, rex);
            operand.mem.scale = (1 << scale_shift);
        }

        if (base == 5 && mod == 0) {
            MemoryCopy(&operand.mem.displacement, byte, 4);
            *num_bytes_read += 4;
            byte += 4;
        } else {
            operand.mem.base_reg = _disasm_decode_reg(base | RexB(prefix.rex), reg_size, rex);
        }
    } else if (mod == 0 && rm == 5) {
        operand.mem.base_reg = DISASM_REG_RIP;
        MemoryCopy(&operand.mem.displacement, byte, 4);
        *num_bytes_read += 4;
        byte += 4;
    } else {
        operand.mem.base_reg = _disasm_decode_reg(rm | RexB(prefix.rex), reg_size, rex);
    }

    if (mod == 1) {
        operand.mem.displacement = (i64)*(i8*)byte;
        *num_bytes_read += 1;
        byte += 1;
    } else if (mod == 2) {
        MemoryCopy(&operand.mem.displacement, byte, 4);
        *num_bytes_read += 4;
        byte += 4;
    }

    return operand;
}

internal inline Disasm_Operand _disasm_decode_m8(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read) {
    return _disasm_decode_m(byte, prefix, num_bytes_read, sizeof(u8));
}

internal inline Disasm_Operand _disasm_decode_m16(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read) {
    return _disasm_decode_m(byte, prefix, num_bytes_read, sizeof(u16));
}

internal inline Disasm_Operand _disasm_decode_m16_32(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read) {
    u8 size_bytes = _disasm_operand_16_32_size(prefix);
    return _disasm_decode_m(byte, prefix, num_bytes_read, size_bytes);
}

internal inline Disasm_Operand _disasm_decode_m16int(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read) {
    return _disasm_decode_m(byte, prefix, num_bytes_read, sizeof(u16));
}

internal inline Disasm_Operand _disasm_decode_m32int(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read) {
    return _disasm_decode_m(byte, prefix, num_bytes_read, sizeof(i32));
}

internal inline Disasm_Operand _disasm_decode_m64(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read) {
    return _disasm_decode_m(byte, prefix, num_bytes_read, sizeof(u64));
}

internal inline Disasm_Operand _disasm_decode_m14_28(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read) {
    return _disasm_decode_m(byte, prefix, num_bytes_read, (prefix.op_override) ? 14 : 28);
}

internal inline Disasm_Operand _disasm_decode_m94_108(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read) {
    return _disasm_decode_m(byte, prefix, num_bytes_read, (prefix.op_override) ? 94 : 108);
}

internal inline Disasm_Operand _disasm_decode_m64int(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read) {
    return _disasm_decode_m(byte, prefix, num_bytes_read, sizeof(i64));
}

internal inline Disasm_Operand _disasm_decode_m64real(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read) {
    return _disasm_decode_m(byte, prefix, num_bytes_read, sizeof(f64));
}

internal inline Disasm_Operand _disasm_decode_m80real(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read) {
    return _disasm_decode_m(byte, prefix, num_bytes_read, 10);
}

internal inline Disasm_Operand _disasm_decode_m80dec(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read) {
    return _disasm_decode_m(byte, prefix, num_bytes_read, 10);
}

internal Disasm_Operand _disasm_decode_rm(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read, u8 size_bytes, b8 is_fpu) {
    Disasm_Operand operand = {0};
    u8 mod = GetMod(*byte);
    u8 rm  = GetRM(*byte);
 
    if (mod == 3) {
        *num_bytes_read += 1;
        operand.type = DISASM_OP_TYPE_REG;
        operand.size_bytes = size_bytes;

        if (is_fpu) operand.reg = DISASM_REG_ST0 + rm;
        else        operand.reg = _disasm_decode_reg(rm | RexB(prefix.rex), operand.size_bytes, prefix.rex);

        return operand;
    }

    return _disasm_decode_m(byte, prefix, num_bytes_read, size_bytes);
}

internal inline Disasm_Operand _disasm_decode_rm8(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read) {
    return _disasm_decode_rm(byte, prefix, num_bytes_read, sizeof(u8), 0);
}

internal inline Disasm_Operand _disasm_decode_rm16(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read) {
    return _disasm_decode_rm(byte, prefix, num_bytes_read, sizeof(u16), 0);
}

internal inline Disasm_Operand _disasm_decode_rm32(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read) {
    return _disasm_decode_rm(byte, prefix, num_bytes_read, sizeof(u32), 0);
}

internal inline Disasm_Operand _disasm_decode_rm16_32(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read) {
    u8 size_bytes = _disasm_operand_16_32_size(prefix);
    return _disasm_decode_rm(byte, prefix, num_bytes_read, size_bytes, 0);
}

internal inline Disasm_Operand _disasm_decode_rm16_64(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read) {
    u8 size_bytes = _disasm_operand_16_64_size(prefix);
    return _disasm_decode_rm(byte, prefix, num_bytes_read, size_bytes, 0);
}

internal inline Disasm_Operand _disasm_decode_rm16_32_64(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read) {
    u8 size_bytes = _disasm_operand_16_32_64_size(prefix);
    return _disasm_decode_rm(byte, prefix, num_bytes_read, size_bytes, 0);
}

internal inline Disasm_Operand _disasm_decode_m16_r16_32_64(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read) {
    Disasm_Operand operand = {0};
    u8 mod = GetMod(*byte);
 
    if (mod == 3) {
        *num_bytes_read += 1;
        operand.type = DISASM_OP_TYPE_REG;
        operand.size_bytes = _disasm_operand_16_32_64_size(prefix);

        u8 rm  = (*byte & 0b00000111);
        operand.reg = _disasm_decode_reg(rm | RexB(prefix.rex), operand.size_bytes, prefix.rex);
        return operand;
    }

    return _disasm_decode_m16(byte, prefix, num_bytes_read);
}

internal inline Disasm_Operand _disasm_decode_sti_m32real(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read) {
    return _disasm_decode_rm(byte, prefix, num_bytes_read, sizeof(f32), 1);
}

internal inline Disasm_Operand _disasm_decode_sti(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read) {
    Disasm_Operand operand = {0};

    *num_bytes_read += 1;

    operand.type = DISASM_OP_TYPE_REG;
    operand.size_bytes = sizeof(f32);
    operand.reg = DISASM_REG_ST0 + GetRM(*byte);

    return operand;
}

internal Disasm_Operand _disasm_decode_moffs(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read, u8 size_bytes) {
    Disasm_Operand operand = {0};

    operand.type = DISASM_OP_TYPE_MEM;
    operand.size_bytes = size_bytes;

    u8 addr_size = prefix.addr_override ? 4 : 8;
    MemoryCopy(&operand.mem.displacement, byte, addr_size);
    *num_bytes_read += addr_size;

    return operand;
}

internal inline Disasm_Operand _disasm_decode_moffs8(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read) {
    return _disasm_decode_moffs(byte, prefix, num_bytes_read, sizeof(u8));
}

internal inline Disasm_Operand _disasm_decode_moffs16_32_64(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read) {
    u8 size_bytes = _disasm_operand_16_32_64_size(prefix);
    return _disasm_decode_moffs(byte, prefix, num_bytes_read, size_bytes);
}

internal Disasm_Operand _disasm_Sreg(u8* byte, Disasm_Prefix prefix) {
    Disasm_Operand operand = {0};

    u8 reg = (*byte & 0b00111000) >> 3;
    Assert(reg <= 0x05);
 
    operand.type = DISASM_OP_TYPE_REG;
    operand.size_bytes = 1;
    operand.reg = DISASM_REG_ES + reg;

    return operand;
}

internal Disasm_Operand _disasm_specific_reg(Disasm_Reg reg) {
    Disasm_Operand operand = {0};

    operand.type = DISASM_OP_TYPE_REG;

    switch (reg) {
        case DISASM_REG_ST:
        case DISASM_REG_ST0: case DISASM_REG_ST1: case DISASM_REG_ST2: case DISASM_REG_ST3:
        case DISASM_REG_ST4: case DISASM_REG_ST5: case DISASM_REG_ST6: case DISASM_REG_ST7: 

        case DISASM_REG_AL:   case DISASM_REG_CL:   case DISASM_REG_DL:   case DISASM_REG_BL:
        case DISASM_REG_AH:   case DISASM_REG_CH:   case DISASM_REG_DH:   case DISASM_REG_BH:
        case DISASM_REG_SPL:  case DISASM_REG_BPL:  case DISASM_REG_SIL:  case DISASM_REG_DIL:
        case DISASM_REG_R8B:  case DISASM_REG_R9B:  case DISASM_REG_R10B: case DISASM_REG_R11B:
        case DISASM_REG_R12B: case DISASM_REG_R13B: case DISASM_REG_R14B: case DISASM_REG_R15B:
            operand.size_bytes = 1;
        break;
        case DISASM_REG_AX:   case DISASM_REG_CX:   case DISASM_REG_DX:   case DISASM_REG_BX:
        case DISASM_REG_SP:   case DISASM_REG_BP:   case DISASM_REG_SI:   case DISASM_REG_DI:
        case DISASM_REG_R8W:  case DISASM_REG_R9W:  case DISASM_REG_R10W: case DISASM_REG_R11W:
        case DISASM_REG_R12W: case DISASM_REG_R13W: case DISASM_REG_R14W: case DISASM_REG_R15W:
            operand.size_bytes = 2;
        break;
        case DISASM_REG_EAX:  case DISASM_REG_ECX:  case DISASM_REG_EDX:  case DISASM_REG_EBX:
        case DISASM_REG_ESP:  case DISASM_REG_EBP:  case DISASM_REG_ESI:  case DISASM_REG_EDI:
        case DISASM_REG_R8D:  case DISASM_REG_R9D:  case DISASM_REG_R10D: case DISASM_REG_R11D:
        case DISASM_REG_R12D: case DISASM_REG_R13D: case DISASM_REG_R14D: case DISASM_REG_R15D:
            operand.size_bytes = 4;
        break;
        case DISASM_REG_RAX: case DISASM_REG_RCX: case DISASM_REG_RDX: case DISASM_REG_RBX:
        case DISASM_REG_RSP: case DISASM_REG_RBP: case DISASM_REG_RSI: case DISASM_REG_RDI:
        case DISASM_REG_R8:  case DISASM_REG_R9:  case DISASM_REG_R10: case DISASM_REG_R11:
        case DISASM_REG_R12: case DISASM_REG_R13: case DISASM_REG_R14: case DISASM_REG_R15:
            operand.size_bytes = 8;
        break;
        case DISASM_REG_XMM0:  case DISASM_REG_XMM1:  case DISASM_REG_XMM2:  case DISASM_REG_XMM3:
        case DISASM_REG_XMM4:  case DISASM_REG_XMM5:  case DISASM_REG_XMM6:  case DISASM_REG_XMM7:
        case DISASM_REG_XMM8:  case DISASM_REG_XMM9:  case DISASM_REG_XMM10: case DISASM_REG_XMM11:
        case DISASM_REG_XMM12: case DISASM_REG_XMM13: case DISASM_REG_XMM14: case DISASM_REG_XMM15:
            operand.size_bytes = 16;
        break;
        default:
            Assert(!"Invalid reg");
        break;
    }

    operand.reg = reg;

    return operand;
}

internal Disasm_Operand _disasm_decode_imm(u8* byte, u8 size_bytes, u8 target_size, u8* instr_len) {
    Disasm_Operand operand = {0};
 
    operand.type = DISASM_OP_TYPE_IMM;
    operand.size_bytes = size_bytes;

    *instr_len += size_bytes;

    MemoryCopy(&operand.imm, byte, size_bytes);

    if (size_bytes == target_size) return operand;

    switch (size_bytes) {
        case 1:
            operand.imm = (i64)(i8)operand.imm;
        break;
        case 2:
            operand.imm = (i64)(i16)operand.imm;
        break;
        case 4:
            operand.imm = (i64)(i32)operand.imm;
        break;
    }

    return operand;
}

internal inline Disasm_Operand _disasm_decode_imm8(u8* byte, u8 target_size, u8* instr_len) {
    return _disasm_decode_imm(byte, sizeof(i8), target_size, instr_len);
}

internal inline Disasm_Operand _disasm_decode_imm16(u8* byte, Disasm_Prefix prefix, u8 target_size, u8* instr_len) {
    return _disasm_decode_imm(byte, sizeof(u16), target_size, instr_len);
}

internal inline Disasm_Operand _disasm_decode_imm16_32(u8* byte, Disasm_Prefix prefix, u8 target_size, u8* instr_len) {
    u8 size_bytes = _disasm_operand_16_32_size(prefix);
    return _disasm_decode_imm(byte, Min(4, size_bytes), target_size, instr_len);
}

internal inline Disasm_Operand _disasm_decode_imm16_32_64(u8* byte, Disasm_Prefix prefix, u8 target_size, u8* instr_len) {
    u8 size_bytes = _disasm_operand_16_32_64_size(prefix);
    return _disasm_decode_imm(byte, size_bytes, target_size, instr_len);
}

internal Disasm_Operand _disasm_decode_rel(u8* byte, u8 size_bytes, u8* instr_len) {
    Disasm_Operand operand = {0};
 
    operand.type = DISASM_OP_TYPE_REL;
    operand.size_bytes = size_bytes;

    *instr_len += size_bytes;

    MemoryCopy(&operand.rel, byte, size_bytes);

    switch (size_bytes) {
        case 1:
            operand.rel = (i64)(i8)operand.rel;
        break;
        case 2:
            operand.rel = (i64)(i16)operand.rel;
        break;
        case 4:
            operand.rel = (i64)(i32)operand.rel;
        break;
    }

    return operand;
}

internal inline Disasm_Operand _disasm_decode_rel8(u8* byte, u8* instr_len) {
    return _disasm_decode_rel(byte, sizeof(i8), instr_len);
}

internal inline Disasm_Operand _disasm_decode_rel16_32(u8* byte, Disasm_Prefix prefix, u8* instr_len) {
    u8 size_bytes = _disasm_operand_16_32_size(prefix);
    return _disasm_decode_rel(byte, size_bytes, instr_len);
}

internal Disasm_Operand _disasm_decode_xmm(u8* byte, Disasm_Prefix prefix) {
    u8 xmm_dest_idx = GetReg(*byte) | RexR(prefix.rex);
    return _disasm_specific_reg(DISASM_REG_XMM0 + xmm_dest_idx);
}

internal Disasm_Operand _disasm_decode_xmm_m(u8* byte, Disasm_Prefix prefix, u8* instr_len, u8 size_bytes) {
    if (GetMod(*byte) == 3) {
        *instr_len += 1;
        u8 xmm_src_idx = GetRM(*byte) | RexB(prefix.rex);
        return _disasm_specific_reg(DISASM_REG_XMM0 + xmm_src_idx);
    }

    return _disasm_decode_m(byte, prefix, instr_len, size_bytes);
}

internal inline Disasm_Operand _disasm_decode_xmm_m32(u8* byte, Disasm_Prefix prefix, u8* instr_len) {
    return _disasm_decode_xmm_m(byte, prefix, instr_len, 4);
}

internal inline Disasm_Operand _disasm_decode_xmm_m64(u8* byte, Disasm_Prefix prefix, u8* instr_len) {
    return _disasm_decode_xmm_m(byte, prefix, instr_len, 8);
}

internal inline Disasm_Operand _disasm_decode_xmm_m128(u8* byte, Disasm_Prefix prefix, u8* instr_len) {
    return _disasm_decode_xmm_m(byte, prefix, instr_len, 16);
}

internal Disasm_Operand _disasm_decode_mm(u8* byte, Disasm_Prefix prefix) {
    return _disasm_specific_reg(DISASM_REG_MM0 + GetReg(*byte));
}

internal Disasm_Operand _disasm_decode_mm_m(u8* byte, Disasm_Prefix prefix, u8* instr_len, u8 size_bytes) {
    if (GetMod(*byte) == 3) {
        *instr_len += 1;
        return _disasm_specific_reg(DISASM_REG_MM0 + GetRM(*byte));
    }

    return _disasm_decode_m(byte, prefix, instr_len, size_bytes);
}

internal inline Disasm_Operand _disasm_decode_mm_m32(u8* byte, Disasm_Prefix prefix, u8* instr_len) {
    return _disasm_decode_mm_m(byte, prefix, instr_len, 4);
}

internal inline Disasm_Operand _disasm_decode_mm_m64(u8* byte, Disasm_Prefix prefix, u8* instr_len) {
    return _disasm_decode_mm_m(byte, prefix, instr_len, 8);
}

internal inline Disasm_Operand _disasm_decode_mm_m128(u8* byte, Disasm_Prefix prefix, u8* instr_len) {
    return _disasm_decode_mm_m(byte, prefix, instr_len, 16);
}
