#include "disasm_x86_64.h"

#include "disasm_x86_64_instr_set.c"
#include "disasm_x86_64_reg.c"

#define RexB(rex) (((rex) & 0x01) << 3)
#define RexX(rex) (((rex) & 0x02) << 2)
#define RexR(rex) (((rex) & 0x04) << 1)
#define RexW(rex)  ((rex) & 0x08)

typedef struct {
    u8 value;
    b8 has_op_override;
    b8 has_addr_override;
    b8 lock;
    u8 segment;
    u8 rex;
    u32 count;
} Disasm_Prefix;

internal Disasm_Prefix _disasm_decode_prefix(u8** instr_len) {
    Disasm_Prefix prefix = {0};
    u8* orig_instr_len = *instr_len;

    u8 i = 0;
    for (; i < 15; i++) {
        u8 byte = *instr_len[0];
        switch (byte) {
            case 0x64:
            case 0x65:
                Assert(!"Unhandled case");
            case 0x66:
                prefix.has_op_override = 1;
                *instr_len += 1;
            break;
            case 0x67:
                prefix.has_addr_override = 1;
                *instr_len += 1;
            break;
            case 0xf2:
            case 0xf3:
            case 0x9b:
                prefix.value = byte;
                *instr_len += 1;
            break;
            case 0xf0:
                prefix.lock = 1;
                *instr_len += 1;
            break;
            default:
                if ((byte & 0xf0) == 0x40) {
                    prefix.rex = byte;
                    *instr_len += 1;
                } else {
                    prefix.count = (u32)(*instr_len - orig_instr_len);
                    return prefix;
                }
        }
    }

    Assert(i < 15);

    prefix.count = (u32)(*instr_len - orig_instr_len);
    return prefix;
}



internal Disasm_Opcode _disasm_decode_fpu_mnemonic(Disasm_Prefix prefix, u8* instr, u64* instr_len) {
    u8 mod = (instr[1] & 0b11000000) >> 6;
    u8 reg = (instr[1] & 0b00111000) >> 3;

    switch (*instr) {
        case 0xd8:
            switch (reg) {
                case 0: return DISASM_FADD;
                case 1: return DISASM_FMUL;
                case 3: return DISASM_FCOMP;
                case 4: return DISASM_FSUB;
                case 5: return DISASM_FSUBR;
                case 6: return DISASM_FDIV;
                case 7: return DISASM_FDIVR;
            }
        break;
        case 0xd9:
            switch (reg) {
                case 0: return DISASM_FLD;
                case 1: return DISASM_FXCH;
                case 2: return (mod != 3) ? DISASM_FST : DISASM_FNOP;
                case 3: return DISASM_FSTP;
                case 6: return ((prefix.value == 0x9b) ? DISASM_FSTENV : DISASM_FNSTENV);
                case 7: return ((prefix.value == 0x9b) ? DISASM_FSTCW : DISASM_FNSTCW);
            }
        break;
        case 0xda:
            switch (reg) {
                case 0: return (mod != 3) ? DISASM_FIADD : DISASM_FCMOVB;
                case 1: return (mod != 3) ? DISASM_FIMUL : DISASM_FCMOVE;
                case 2: return (mod != 3) ? DISASM_FICOM : DISASM_FCMOVBE;
                case 3: return (mod != 3) ? DISASM_FICOMP : DISASM_FCMOVU;
                case 4: return (mod != 3) ? DISASM_FISUB : DISASM_INVALID;
                case 5: return (mod != 3) ? DISASM_FISUBR : DISASM_FUCOMPP;
                case 6: return (mod != 3) ? DISASM_FIDIV : DISASM_INVALID;
                case 7: return (mod != 3) ? DISASM_FIDIVR : DISASM_INVALID;
            }
        break;
        case 0xdb:
            switch (reg) {
                case 0: return (mod != 3) ? DISASM_FILD : DISASM_FCMOVNB;
                case 1: return (mod != 3) ? DISASM_FISTTP : DISASM_FCMOVNE;
                case 2: return (mod != 3) ? DISASM_FIST : DISASM_FCMOVNBE;
                case 3: return (mod != 3) ? DISASM_FISTP : DISASM_FCMOVNU;
                case 4: return (mod != 3) ? DISASM_INVALID : ((prefix.value == 0x9b) ? DISASM_FCLEX : DISASM_FNCLEX);
                case 5: return (mod != 3) ? DISASM_FLD : ((prefix.value == 0x9b) ? DISASM_FINIT : DISASM_FNINIT);
                case 7: return (mod != 3) ? DISASM_FSTP : DISASM_INVALID;
            }
        break;
        case 0xdc:
            switch (reg) {
                case 0: return DISASM_FADD;
                case 1: return DISASM_FMUL;
                case 2: return DISASM_FCOM;
                case 3: return DISASM_FCOMP;
                case 4: return DISASM_FSUB;
                case 5: return DISASM_FSUBR;
                case 6: return DISASM_FDIV;
                case 7: return DISASM_FDIVR;
            }
        break;
        case 0xdd:
            switch (reg) {
                case 0: return (mod != 3) ? DISASM_FLD : DISASM_FFREE;
                case 1: return (mod != 3) ? DISASM_FISTTP : DISASM_INVALID;
                case 2: return (mod != 3) ? DISASM_FST : DISASM_FCOM;
                case 3: return (mod != 3) ? DISASM_FSTP : DISASM_FCOMP;
                case 4: return (mod != 3) ? DISASM_FRSTOR : DISASM_FUCOM;
                case 5: return (mod != 3) ? DISASM_INVALID : DISASM_FUCOMP;
                case 6: return (mod != 3) ? ((prefix.value == 0x9b) ? DISASM_FSAVE : DISASM_FNSAVE) : DISASM_INVALID;
                case 7: return (mod != 3) ? ((prefix.value == 0x9b) ? DISASM_FSTSW : DISASM_FNSTSW) : DISASM_INVALID;
            }
        break;
        case 0xde:
            switch (reg) {
                case 0: return (mod != 3) ? DISASM_FIADD : DISASM_FADDP;
                case 1: return (mod != 3) ? DISASM_FIMUL : DISASM_FMULP;
                case 2: return (mod != 3) ? DISASM_FICOM : DISASM_INVALID;
                case 3: return (mod != 3) ? DISASM_FICOMP : DISASM_FCOMPP;
                case 4: return (mod != 3) ? DISASM_FISUB : DISASM_FSUBRP;
                case 5: return (mod != 3) ? DISASM_FISUBR : DISASM_FSUBP;
                case 6: return (mod != 3) ? DISASM_FIDIV : DISASM_FDIVRP;
                case 7: return (mod != 3) ? DISASM_FIDIVR : DISASM_FDIVP;
            }
        break;
        case 0xdf:
            switch (reg) {
                case 0: return (mod != 3) ? DISASM_FILD : DISASM_INVALID;
                case 1: return (mod != 3) ? DISASM_FISTTP : DISASM_INVALID;
                case 2: return (mod != 3) ? DISASM_FIST : DISASM_INVALID;
                case 3: return (mod != 3) ? DISASM_FISTP : DISASM_INVALID;
                case 4: return (mod != 3) ? DISASM_FBSTP : DISASM_FNSTSW;
                case 5: return (mod != 3) ? DISASM_FILD : DISASM_FUCOMIP;
                case 6: return (mod != 3) ? DISASM_FISTP : DISASM_FCOMIP;
                case 7: return (mod != 3) ? DISASM_FISTP : DISASM_INVALID;
            }
        break;
    }

    *instr_len = 1;
	return DISASM_INVALID;
}

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
        case 4: return DISASM_SHL;
        case 5: return DISASM_SHR;
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
    else if (prefix.has_op_override) size_bytes = 2;
    return size_bytes;
}

internal inline u8 _disasm_operand_16_32_size(Disasm_Prefix prefix) {
    u8 size_bytes = 4;
    if (prefix.has_op_override) size_bytes = 2;
    return size_bytes;
}

internal inline u8 _disasm_operand_64_16_size(Disasm_Prefix prefix) {
    u8 size_bytes = 8;
    if (prefix.has_op_override) size_bytes = 2;
    return size_bytes;
}

internal inline u8 _disasm_operand_32_64_size(Disasm_Prefix prefix) {
    return (RexW(prefix.rex) == 0x08) ? 8 : 4;
}

internal inline u8 _disasm_operand_8_16_size(Disasm_Prefix prefix) {
    return (prefix.has_op_override) ? 2 : 4;
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


internal Disasm_Operand _disasm_decode_rm(u8* rm_byte, Disasm_Prefix prefix, u8* num_bytes_read, u8 size_bytes) {
    u8 rex = prefix.rex;

    Disasm_Operand operand = {0};
    u8 mod = (*rm_byte & 0b11000000) >> 6;
    u8 reg = (*rm_byte & 0b00111000) >> 3;
    u8 rm  = (*rm_byte & 0b00000111);
 
    *num_bytes_read += 1;

    operand.size_bytes = size_bytes;

    if (mod == 3) {
        operand.type = DISASM_OP_TYPE_REG;
        operand.reg = _disasm_decode_reg(rm | RexB(prefix.rex), operand.size_bytes, rex);
        return operand;
    }

    operand.type = DISASM_OP_TYPE_MEM;

    if (rm == 4) {
        u8 sib = rm_byte[1];

        u8 scale_shift = (sib & 0b11000000) >> 6;
        u8 idx_reg     = (sib & 0b00111000) >> 3;
        u8 base        = (sib & 0b00000111);

        *num_bytes_read += 1;

        if (idx_reg != 4) {
            operand.mem.idx_reg = _disasm_decode_reg(idx_reg | RexX(prefix.rex), 8, rex);
            operand.mem.scale = (1 << scale_shift);
        }

        if (base == 5 && mod == 0) {
            MemoryCopy(&operand.mem.displacement, &rm_byte[2], 4);
            *num_bytes_read += 4;
        } else {
            operand.mem.base_reg = _disasm_decode_reg(base | RexB(prefix.rex), 8, rex);
        }
    } else if (mod == 0 && rm == 5) {
        operand.mem.base_reg = DISASM_REG_RIP;
        MemoryCopy(&operand.mem.displacement, &rm_byte[1], 4);
        *num_bytes_read += 4;
    } else {
        operand.mem.base_reg = _disasm_decode_reg(rm | RexB(prefix.rex), 8, rex);
    }

    if (mod == 1) {
        operand.mem.displacement = (i32)(i8)rm_byte[1];
        *num_bytes_read += 1;
    } else if (mod == 2) {
        MemoryCopy(&operand.mem.displacement, &rm_byte[1], 4);
        *num_bytes_read += 4;
    }

    return operand;
}

internal inline Disasm_Operand _disasm_decode_rm8(u8* rm_byte, Disasm_Prefix prefix, u8* num_bytes_read) {
    return _disasm_decode_rm(rm_byte, prefix, num_bytes_read, sizeof(u8));
}

internal inline Disasm_Operand _disasm_decode_rm32(u8* rm_byte, Disasm_Prefix prefix, u8* num_bytes_read) {
    return _disasm_decode_rm(rm_byte, prefix, num_bytes_read, sizeof(u32));
}

internal inline Disasm_Operand _disasm_decode_rm16_32_64(u8* rm_byte, Disasm_Prefix prefix, u8* num_bytes_read) {
    u8 size_bytes = _disasm_operand_16_32_64_size(prefix);
    return _disasm_decode_rm(rm_byte, prefix, num_bytes_read, size_bytes);
}

internal Disasm_Operand _disasm_decode_r(u8* rm_byte, Disasm_Prefix prefix, u8 size_bytes) {
    Disasm_Operand operand = {0};

    operand.type = DISASM_OP_TYPE_REG;
    operand.size_bytes = size_bytes;

    u8 rex = prefix.rex;
    u8 reg = (*rm_byte & 0b00111000) >> 3;
    operand.reg = _disasm_decode_reg(reg | RexR(rex), operand.size_bytes, rex);

    return operand;
}

internal inline Disasm_Operand _disasm_decode_r8(u8* rm_byte, Disasm_Prefix prefix) {
    return _disasm_decode_r(rm_byte, prefix, 1);
}

internal inline Disasm_Operand _disasm_decode_r32_64(u8* rm_byte, Disasm_Prefix prefix) {
    u8 size_bytes = _disasm_operand_32_64_size(prefix);
    return _disasm_decode_r(rm_byte, prefix, size_bytes);
}

internal inline Disasm_Operand _disasm_decode_r16_32_64(u8* rm_byte, Disasm_Prefix prefix) {
    u8 size_bytes = _disasm_operand_16_32_64_size(prefix);
    return _disasm_decode_r(rm_byte, prefix, size_bytes);
}


internal Disasm_Operand _disasm_specific_reg(Disasm_Reg reg) {
    Disasm_Operand operand = {0};

    operand.type = DISASM_OP_TYPE_REG;

    switch (reg) {
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

internal inline Disasm_Operand _disasm_decode_imm16_32(u8* byte, Disasm_Prefix prefix, u8 target_size, u8* instr_len) {
    u8 size_bytes = _disasm_operand_16_32_size(prefix);
    return _disasm_decode_imm(byte, Min(4, size_bytes), target_size, instr_len);
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

Disasm_Instr disasm_decode(u8* instr_ptr) {
#define InstrNext (instr_ptr + instr.instr_len)
#define ModRMByte (instr_ptr + 1)
    Disasm_Instr instr = {0};
    instr.instr = instr_ptr;

    Disasm_Prefix prefix = _disasm_decode_prefix(&instr_ptr);

    if (*instr_ptr <= 0x3f) {
        u8 op_combo = (*instr_ptr) & 7;
        u8 op_type = (*instr_ptr) & ~7;

        switch (op_type) {
            case 0x00: instr.opcode = DISASM_ADD; break;
            case 0x08: instr.opcode = DISASM_OR; break;
            case 0x10: instr.opcode = DISASM_ADC; break;
            case 0x18: instr.opcode = DISASM_SBB; break;
            case 0x20: instr.opcode = DISASM_AND; break;
            case 0x28: instr.opcode = DISASM_SUB; break;
            case 0x30: instr.opcode = DISASM_XOR; break;
            case 0x38: instr.opcode = DISASM_CMP; break;
        }

        switch (op_combo) {
        case 0x00:
            instr.instr_len = 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_rm8(ModRMByte, prefix, &instr.instr_len);
            instr.operand[1] = _disasm_decode_r8(ModRMByte, prefix);
            instr.instr_len += prefix.count;
            return instr;
        case 0x01:
            instr.instr_len = 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_rm16_32_64(ModRMByte, prefix, &instr.instr_len);
            instr.operand[1] = _disasm_decode_r16_32_64(ModRMByte, prefix);
            instr.instr_len += prefix.count;
            return instr;
        case 0x02:
            instr.instr_len = 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_rm8(ModRMByte, prefix, &instr.instr_len);
            instr.operand[1] = _disasm_decode_r8(ModRMByte, prefix);
            instr.instr_len += prefix.count;
            return instr;
        case 0x03:
            instr.instr_len = 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_rm16_32_64(ModRMByte, prefix, &instr.instr_len);
            instr.operand[1] = _disasm_decode_r16_32_64(ModRMByte, prefix);
            instr.instr_len += prefix.count;
            return instr;
        case 0x04:
            instr.instr_len = 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_specific_reg(DISASM_REG_AL);
            instr.operand[1] = _disasm_decode_imm8(InstrNext, instr.operand[0].size_bytes, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x05:
            instr.instr_len = 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_specific_reg(DISASM_REG_RAX);
            instr.operand[1] = _disasm_decode_imm16_32(InstrNext, prefix, instr.operand[0].size_bytes, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        }
    }

    if (*instr_ptr >= 0x50 && *instr_ptr <= 0x5f) {
        instr.opcode = (*instr_ptr < 0x58) ? DISASM_PUSH : DISASM_POP;
        instr.num_operands = 1;
        instr.instr_len = 1 + prefix.count;

        instr.operand[0].type = DISASM_OP_TYPE_REG;
        instr.operand[0].size_bytes = _disasm_operand_64_16_size(prefix);

        u8 reg = *instr_ptr & 0b111;
        instr.operand[0].reg = _disasm_decode_reg(reg | RexB(prefix.rex), instr.operand[0].size_bytes, prefix.rex);
        return instr;
    }

    switch (*instr_ptr) {
        case 0x63:
            instr.opcode = DISASM_MOVSXD;
            instr.instr_len = 1 + prefix.count;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_r32_64(ModRMByte, prefix);
            instr.operand[1] = _disasm_decode_rm32(ModRMByte, prefix, &instr.instr_len);
            return instr;
        case 0x68:
            instr.opcode = DISASM_PUSH;
            instr.instr_len = 1;
            instr.num_operands = 1;
            instr.operand[0] = _disasm_decode_imm16_32(InstrNext, prefix, sizeof(i64), &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x69:
            {
                instr.opcode = DISASM_IMUL;
                instr.instr_len = 1 + prefix.count;
                instr.num_operands = 3;
                u8 size_bytes = _disasm_operand_16_32_64_size(prefix);
                instr.operand[0] = _disasm_decode_r16_32_64(ModRMByte, prefix);
                instr.operand[1] = _disasm_decode_rm16_32_64(ModRMByte, prefix, &instr.instr_len);
                instr.operand[2] = _disasm_decode_imm(instr_ptr + instr.instr_len - prefix.count, Min(4, size_bytes), size_bytes, &instr.instr_len);
            }
            return instr;
        case 0x6a:
            instr.opcode = DISASM_PUSH;
            instr.instr_len = 1;
            instr.num_operands = 1;
            instr.operand[0] = _disasm_decode_imm8(InstrNext, sizeof(i64), &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x6b:
            {
                instr.opcode = DISASM_IMUL;
                instr.instr_len = 1;
                instr.num_operands = 3;
                u8 size_bytes = _disasm_operand_16_32_64_size(prefix);
                instr.operand[0] = _disasm_decode_r16_32_64(ModRMByte, prefix);
                instr.operand[1] = _disasm_decode_rm16_32_64(ModRMByte, prefix, &instr.instr_len);
                instr.operand[2] = _disasm_decode_imm8(InstrNext, size_bytes, &instr.instr_len);
                instr.instr_len += prefix.count;
            }
            return instr;
        case 0x6c:
            instr.opcode = DISASM_INSB;
            instr.instr_len = 1 + prefix.count;
            instr.num_operands = 2;

            instr.operand[0].type = DISASM_OP_TYPE_MEM;
            instr.operand[0].size_bytes = 1;
            instr.operand[0].mem.base_reg = (prefix.has_addr_override) ? DISASM_REG_EDI : DISASM_REG_RDI;
            instr.operand[0].mem.idx_reg = DISASM_REG_NONE;
            instr.operand[0].mem.scale = 0;
            instr.operand[0].mem.displacement = 0;

            instr.operand[1] = _disasm_specific_reg(DISASM_REG_DX);

            return instr;
        case 0x6d:
            instr.opcode = (prefix.has_op_override) ? DISASM_INSW : DISASM_INSD;
            instr.instr_len = 1 + prefix.count;
            instr.num_operands = 2;

            instr.operand[0].type = DISASM_OP_TYPE_MEM;
            instr.operand[0].size_bytes = _disasm_operand_8_16_size(prefix);
            instr.operand[0].mem.base_reg = (prefix.has_addr_override) ? DISASM_REG_EDI : DISASM_REG_RDI;
            instr.operand[0].mem.idx_reg = DISASM_REG_NONE;
            instr.operand[0].mem.scale = 0;
            instr.operand[0].mem.displacement = 0;

            instr.operand[1] = _disasm_specific_reg(DISASM_REG_DX);

            return instr;
        case 0x6e:
            instr.opcode = DISASM_OUTSB;
            instr.instr_len = 1 + prefix.count;
            instr.num_operands = 2;

            instr.operand[0] = _disasm_specific_reg(DISASM_REG_DX);

            instr.operand[1].type = DISASM_OP_TYPE_MEM;
            instr.operand[1].size_bytes = 1;
            instr.operand[1].mem.base_reg = (prefix.has_addr_override) ? DISASM_REG_EDI : DISASM_REG_RDI;
            instr.operand[1].mem.idx_reg = DISASM_REG_NONE;
            instr.operand[1].mem.scale = 0;
            instr.operand[1].mem.displacement = 0;

            return instr;
        case 0x6f:
            instr.opcode = (prefix.has_op_override) ? DISASM_OUTSW : DISASM_OUTSD;
            instr.instr_len = 1 + prefix.count;
            instr.num_operands = 2;

            instr.operand[0] = _disasm_specific_reg(DISASM_REG_DX);

            instr.operand[1].type = DISASM_OP_TYPE_MEM;
            instr.operand[1].size_bytes = _disasm_operand_8_16_size(prefix);
            instr.operand[1].mem.base_reg = (prefix.has_addr_override) ? DISASM_REG_EDI : DISASM_REG_RDI;
            instr.operand[1].mem.idx_reg = DISASM_REG_NONE;
            instr.operand[1].mem.scale = 0;
            instr.operand[1].mem.displacement = 0;

            return instr;
        case 0x70:
            instr.opcode = DISASM_JO;
            instr.instr_len = 1;
            instr.num_operands = 1;
            instr.operand[0] = _disasm_decode_rel8(InstrNext, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x71:
            instr.opcode = DISASM_JNO;
            instr.instr_len = 1;
            instr.num_operands = 1;
            instr.operand[0] = _disasm_decode_rel8(InstrNext, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x72:
            instr.opcode = DISASM_JB;
            instr.instr_len = 1;
            instr.num_operands = 1;
            instr.operand[0] = _disasm_decode_rel8(InstrNext, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x73:
            instr.opcode = DISASM_JNB;
            instr.instr_len = 1;
            instr.num_operands = 1;
            instr.operand[0] = _disasm_decode_rel8(InstrNext, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x74:
            instr.opcode = DISASM_JZ;
            instr.instr_len = 1;
            instr.num_operands = 1;
            instr.operand[0] = _disasm_decode_rel8(InstrNext, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x75:
            instr.opcode = DISASM_JNZ;
            instr.instr_len = 1;
            instr.num_operands = 1;
            instr.operand[0] = _disasm_decode_rel8(InstrNext, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x76:
            instr.opcode = DISASM_JBE;
            instr.instr_len = 1;
            instr.num_operands = 1;
            instr.operand[0] = _disasm_decode_rel8(InstrNext, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x77:
            instr.opcode = DISASM_JNBE;
            instr.instr_len = 1;
            instr.num_operands = 1;
            instr.operand[0] = _disasm_decode_rel8(InstrNext, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x78:
            instr.opcode = DISASM_JS;
            instr.instr_len = 1;
            instr.num_operands = 1;
            instr.operand[0] = _disasm_decode_rel8(InstrNext, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x79:
            instr.opcode = DISASM_JNS;
            instr.instr_len = 1;
            instr.num_operands = 1;
            instr.operand[0] = _disasm_decode_rel8(InstrNext, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x7a:
            instr.opcode = DISASM_JP;
            instr.instr_len = 1;
            instr.num_operands = 1;
            instr.operand[0] = _disasm_decode_rel8(InstrNext, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x7b:
            instr.opcode = DISASM_JNP;
            instr.instr_len = 1;
            instr.num_operands = 1;
            instr.operand[0] = _disasm_decode_rel8(InstrNext, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x7c:
            instr.opcode = DISASM_JL;
            instr.instr_len = 1;
            instr.num_operands = 1;
            instr.operand[0] = _disasm_decode_rel8(InstrNext, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x7d:
            instr.opcode = DISASM_JNL;
            instr.instr_len = 1;
            instr.num_operands = 1;
            instr.operand[0] = _disasm_decode_rel8(InstrNext, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x7e:
            instr.opcode = DISASM_JLE;
            instr.instr_len = 1;
            instr.num_operands = 1;
            instr.operand[0] = _disasm_decode_rel8(InstrNext, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x7f:
            instr.opcode = DISASM_JNLE;
            instr.instr_len = 1;
            instr.num_operands = 1;
            instr.operand[0] = _disasm_decode_rel8(InstrNext, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x80:
            instr.opcode = _disasm_group1_mnemonic(ModRMByte);
            instr.instr_len = 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_rm8(ModRMByte, prefix, &instr.instr_len);
            instr.operand[1] = _disasm_decode_imm8(InstrNext, sizeof(i8), &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x81:
            instr.opcode = _disasm_group1_mnemonic(ModRMByte);
            instr.instr_len = 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_rm16_32_64(ModRMByte, prefix, &instr.instr_len);
            instr.operand[1] = _disasm_decode_imm16_32(InstrNext, prefix, instr.operand[0].size_bytes, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x83:
            instr.opcode = _disasm_group1_mnemonic(ModRMByte);
            instr.instr_len = 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_rm16_32_64(ModRMByte, prefix, &instr.instr_len);
            instr.operand[1] = _disasm_decode_imm8(InstrNext, instr.operand[0].size_bytes, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x84:
        case 0x85:
            instr.opcode = DISASM_TEST;
            instr.instr_len = 1;
            instr.num_operands = 2;
            if ((*instr_ptr & 1) == 0) {
                instr.operand[0] = _disasm_decode_rm8(ModRMByte, prefix, &instr.instr_len);
                instr.operand[1] = _disasm_decode_r8(ModRMByte, prefix);
            } else {
                instr.operand[0] = _disasm_decode_rm16_32_64(ModRMByte, prefix, &instr.instr_len);
                instr.operand[1] = _disasm_decode_r16_32_64(ModRMByte, prefix);
            }
            instr.instr_len += prefix.count;
            return instr;
        case 0x86:
        case 0x87:
            instr.opcode = DISASM_XCHG;
            instr.instr_len = 1;
            instr.num_operands = 2;
            if ((*instr_ptr & 1) == 0) {
                instr.operand[0] = _disasm_decode_rm8(ModRMByte, prefix, &instr.instr_len);
                instr.operand[1] = _disasm_decode_r8(ModRMByte, prefix);
            } else {
                instr.operand[0] = _disasm_decode_rm16_32_64(ModRMByte, prefix, &instr.instr_len);
                instr.operand[1] = _disasm_decode_r16_32_64(ModRMByte, prefix);
            }
            instr.instr_len += prefix.count;
            return instr;
        case 0x88:
        case 0x89:
            instr.opcode = DISASM_MOV;
            instr.instr_len = 1;
            instr.num_operands = 2;
            if ((*instr_ptr & 1) == 0) {
                instr.operand[0] = _disasm_decode_rm8(ModRMByte, prefix, &instr.instr_len);
                instr.operand[1] = _disasm_decode_r8(ModRMByte, prefix);
            } else {
                instr.operand[0] = _disasm_decode_rm16_32_64(ModRMByte, prefix, &instr.instr_len);
                instr.operand[1] = _disasm_decode_r16_32_64(ModRMByte, prefix);
            }
            instr.instr_len += prefix.count;
            return instr;
        case 0x8a:
        case 0x8b:
            instr.opcode = DISASM_MOV;
            instr.instr_len = 1;
            instr.num_operands = 2;
            if ((*instr_ptr & 1) == 0) {
                instr.operand[0] = _disasm_decode_r8(ModRMByte, prefix);
                instr.operand[1] = _disasm_decode_rm8(ModRMByte, prefix, &instr.instr_len);
            } else {
                instr.operand[0] = _disasm_decode_r16_32_64(ModRMByte, prefix);
                instr.operand[1] = _disasm_decode_rm16_32_64(ModRMByte, prefix, &instr.instr_len);
            }
            instr.instr_len += prefix.count;
            return instr;

    }
 
    return instr;

#undef InstrNext
#undef ModRMByte
}

String disasm_opcode_format(Arena* arena, Disasm_Opcode opcode) {
    String mnemonic = string_skip(String(disasm_opcode_stringify(opcode)), sizeof("DISASM"));
    return string_to_lower(arena, mnemonic);
}

internal String _disasm_reg_format(Arena* arena, Disasm_Reg reg) {
    return string_skip(String(disasm_reg_stringify(reg)), sizeof("DISASM_REG"));
}

internal String _disasm_mem_format(Arena* arena, Disasm_Operand operand, Disasm_Opcode opcode) {
    String base_reg = _disasm_reg_format(arena, operand.mem.base_reg);
    String str;
    StringBuilderBlock(arena, str) {
        switch(operand.size_bytes) {
            case 1: string_builder_append(arena, &str, String("byte ptr ")); break;
            case 2: string_builder_append(arena, &str, String("word ptr ")); break;
            case 4: string_builder_append(arena, &str, String("dword ptr ")); break;
            case 8: string_builder_append(arena, &str, String("qword ptr ")); break;
        }

        if (opcode == DISASM_INSB || opcode == DISASM_INSW || opcode == DISASM_INSD) {
            string_builder_append(arena, &str, String("es:"));
        } else if (opcode == DISASM_OUTSB || opcode == DISASM_OUTSW || opcode == DISASM_OUTSD) {
            string_builder_append(arena, &str, String("ds:"));
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

internal String _disasm_rel_format(Arena* arena, Disasm_Operand operand, u8 instr_len) {
    String str;
    StringBuilderBlock(arena, str) {
        i64 rel = operand.rel + instr_len;
        if (rel < 0) string_builder_append_char(arena, &str, '-');
        string_builder_append(arena, &str, String("0x"));
        string_builder_append_int(arena, &str, Abs(rel), 16);
    }
    return str;
}

String disasm_operand_format(Arena* arena, Disasm_Instr instr, u8 operand_idx) {
    Disasm_Operand operand = instr.operand[operand_idx];
    switch (operand.type) {
        case DISASM_OP_TYPE_REG: return _disasm_reg_format(arena, operand.reg);
        case DISASM_OP_TYPE_IMM: return _disasm_imm_format(arena, operand);
        case DISASM_OP_TYPE_MEM: return _disasm_mem_format(arena, operand, instr.opcode);
        case DISASM_OP_TYPE_REL: return _disasm_rel_format(arena, operand, instr.instr_len);
        default:                 return String("");
    }
}


#undef RexB
#undef RexX
#undef RexR
#undef RexW

