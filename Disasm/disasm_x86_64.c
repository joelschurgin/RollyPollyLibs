#include "disasm_x86_64.h"

#include "disasm_x86_64_instr_set.c"
#include "disasm_x86_64_reg.c"

#define RexB(rex) (((rex) & 0x01) << 3)
#define RexX(rex) (((rex) & 0x02) << 2)
#define RexR(rex) (((rex) & 0x04) << 1)
#define RexW(rex)  ((rex) & 0x08)

typedef struct {
    u8 value;
    b8 op_override;
    b8 addr_override;
    b8 lock;
    b8 repeat;
    Disasm_Segment segment;
    u8 rex;
    u32 count;
} Disasm_Prefix;

internal Disasm_Prefix _disasm_decode_prefix(u8** instr_ptr) {
    Disasm_Prefix prefix = {0};
    u8* orig_instr_ptr = *instr_ptr;

    u8 i = 0;
    for (; i <= 15; i++) {
        u8 byte = *instr_ptr[0];
        switch (byte) {
            case 0x26: case 0x2e:
            case 0x36: case 0x3e:
            case 0x64: case 0x65:
                prefix.segment = (Disasm_Segment)byte;
                *instr_ptr += 1;
            break;
            case 0x66:
                prefix.op_override = 1;
                *instr_ptr += 1;
            break;
            case 0x67:
                prefix.addr_override = 1;
                *instr_ptr += 1;
            break;
            case 0xf3:
                prefix.repeat = 1;
                *instr_ptr += 1;
            break;
            case 0xf0:
                prefix.lock = 1;
                *instr_ptr += 1;
            break;
            case 0xf2:
                prefix.value = byte;
                *instr_ptr += 1;
            break;
            default:
                if ((byte & 0xf0) == 0x40) {
                    prefix.rex = byte;
                    *instr_ptr += 1;
                } else {
                    prefix.count = (u32)(*instr_ptr - orig_instr_ptr);
                    return prefix;
                }
        }
    }

    Assert(i < 15);

    prefix.count = (u32)(*instr_ptr - orig_instr_ptr);
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

internal Disasm_Operand _disasm_decode_string_src(Disasm_Prefix prefix, u8 size_bytes) {
    Disasm_Operand operand = {0};

    operand.type = DISASM_OP_TYPE_MEM;
    operand.size_bytes = size_bytes;
    operand.mem.base_reg = (prefix.addr_override) ? DISASM_REG_ESI : DISASM_REG_RSI;
    operand.mem.idx_reg = DISASM_REG_NONE;
    operand.mem.scale = 0;
    operand.mem.displacement = 0;
    operand.mem.segment = prefix.segment ? prefix.segment : DISASM_SEG_DS;

    return operand;
}

internal Disasm_Operand _disasm_decode_string_dest(Disasm_Prefix prefix, u8 size_bytes) {
    Disasm_Operand operand = {0};

    operand.type = DISASM_OP_TYPE_MEM;
    operand.size_bytes = size_bytes;
    operand.mem.base_reg = (prefix.addr_override) ? DISASM_REG_EDI : DISASM_REG_RDI;
    operand.mem.idx_reg = DISASM_REG_NONE;
    operand.mem.scale = 0;
    operand.mem.displacement = 0;
    operand.mem.segment = DISASM_SEG_ES;

    return operand;
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

internal inline Disasm_Operand _disasm_decode_r16_32_64(u8* byte, Disasm_Prefix prefix) {
    u8 size_bytes = _disasm_operand_16_32_64_size(prefix);
    return _disasm_decode_r(byte, prefix, size_bytes);
}


internal Disasm_Operand _disasm_decode_m(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read, u8 size_bytes) {
    u8 rex = prefix.rex;

    Disasm_Operand operand = {0};
    u8 mod = (*byte & 0b11000000) >> 6;
    u8 rm  = (*byte & 0b00000111);
 
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

internal inline Disasm_Operand _disasm_decode_m16(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read) {
    return _disasm_decode_m(byte, prefix, num_bytes_read, sizeof(u16));
}

internal inline Disasm_Operand _disasm_decode_m64(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read) {
    return _disasm_decode_m(byte, prefix, num_bytes_read, sizeof(u64));
}

internal Disasm_Operand _disasm_decode_rm(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read, u8 size_bytes) {
    Disasm_Operand operand = {0};
    u8 mod = (*byte & 0b11000000) >> 6;
    u8 rm  = (*byte & 0b00000111);
 
    if (mod == 3) {
        *num_bytes_read += 1;
        operand.type = DISASM_OP_TYPE_REG;
        operand.size_bytes = size_bytes;
        operand.reg = _disasm_decode_reg(rm | RexB(prefix.rex), operand.size_bytes, prefix.rex);
        return operand;
    }

    return _disasm_decode_m(byte, prefix, num_bytes_read, size_bytes);
}

internal inline Disasm_Operand _disasm_decode_rm8(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read) {
    return _disasm_decode_rm(byte, prefix, num_bytes_read, sizeof(u8));
}

internal inline Disasm_Operand _disasm_decode_rm16(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read) {
    return _disasm_decode_rm(byte, prefix, num_bytes_read, sizeof(u16));
}

internal inline Disasm_Operand _disasm_decode_rm32(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read) {
    return _disasm_decode_rm(byte, prefix, num_bytes_read, sizeof(u32));
}

internal inline Disasm_Operand _disasm_decode_rm16_64(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read) {
    u8 size_bytes = _disasm_operand_16_64_size(prefix);
    return _disasm_decode_rm(byte, prefix, num_bytes_read, size_bytes);
}

internal inline Disasm_Operand _disasm_decode_rm16_32_64(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read) {
    u8 size_bytes = _disasm_operand_16_32_64_size(prefix);
    return _disasm_decode_rm(byte, prefix, num_bytes_read, size_bytes);
}

internal inline Disasm_Operand _disasm_decode_m16_r16_32_64(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read) {
    Disasm_Operand operand = {0};
    u8 mod = (*byte & 0b11000000) >> 6;
 
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

Disasm_Instr disasm_decode(u8* instr_ptr) {
#define InstrNext (instr_ptr + instr.instr_len)
#define ModRMBytePtr (instr_ptr + 1)
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
            instr.operand[0] = _disasm_decode_rm8(ModRMBytePtr, prefix, &instr.instr_len);
            instr.operand[1] = _disasm_decode_r8(ModRMBytePtr, prefix);
            instr.instr_len += prefix.count;
            return instr;
        case 0x01:
            instr.instr_len = 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_rm16_32_64(ModRMBytePtr, prefix, &instr.instr_len);
            instr.operand[1] = _disasm_decode_r16_32_64(ModRMBytePtr, prefix);
            instr.instr_len += prefix.count;
            return instr;
        case 0x02:
            instr.instr_len = 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_rm8(ModRMBytePtr, prefix, &instr.instr_len);
            instr.operand[1] = _disasm_decode_r8(ModRMBytePtr, prefix);
            instr.instr_len += prefix.count;
            return instr;
        case 0x03:
            instr.instr_len = 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_rm16_32_64(ModRMBytePtr, prefix, &instr.instr_len);
            instr.operand[1] = _disasm_decode_r16_32_64(ModRMBytePtr, prefix);
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
        instr.operand[0].size_bytes = _disasm_operand_16_64_size(prefix);

        u8 reg = *instr_ptr & 0b111;
        instr.operand[0].reg = _disasm_decode_reg(reg | RexB(prefix.rex), instr.operand[0].size_bytes, prefix.rex);
        return instr;
    }

    switch (*instr_ptr) {
        case 0x63:
            instr.opcode = DISASM_MOVSXD;
            instr.instr_len = 1 + prefix.count;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_r32_64(ModRMBytePtr, prefix);
            instr.operand[1] = _disasm_decode_rm32(ModRMBytePtr, prefix, &instr.instr_len);
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
                instr.operand[0] = _disasm_decode_r16_32_64(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_rm16_32_64(ModRMBytePtr, prefix, &instr.instr_len);
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
                instr.operand[0] = _disasm_decode_r16_32_64(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_rm16_32_64(ModRMBytePtr, prefix, &instr.instr_len);
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
            instr.operand[0].mem.base_reg = (prefix.addr_override) ? DISASM_REG_EDI : DISASM_REG_RDI;
            instr.operand[0].mem.idx_reg = DISASM_REG_NONE;
            instr.operand[0].mem.scale = 0;
            instr.operand[0].mem.displacement = 0;
            instr.operand[0].mem.segment = DISASM_SEG_ES;

            instr.operand[1] = _disasm_specific_reg(DISASM_REG_DX);

            return instr;
        case 0x6d:
            instr.opcode = (prefix.op_override) ? DISASM_INSW : DISASM_INSD;
            instr.instr_len = 1 + prefix.count;
            instr.num_operands = 2;

            instr.operand[0].type = DISASM_OP_TYPE_MEM;
            instr.operand[0].size_bytes = _disasm_operand_8_16_size(prefix);
            instr.operand[0].mem.base_reg = (prefix.addr_override) ? DISASM_REG_EDI : DISASM_REG_RDI;
            instr.operand[0].mem.idx_reg = DISASM_REG_NONE;
            instr.operand[0].mem.scale = 0;
            instr.operand[0].mem.displacement = 0;
            instr.operand[0].mem.segment = DISASM_SEG_ES;

            instr.operand[1] = _disasm_specific_reg(DISASM_REG_DX);

            return instr;
        case 0x6e:
            instr.opcode = DISASM_OUTSB;
            instr.instr_len = 1 + prefix.count;
            instr.num_operands = 2;

            instr.operand[0] = _disasm_specific_reg(DISASM_REG_DX);

            instr.operand[1].type = DISASM_OP_TYPE_MEM;
            instr.operand[1].size_bytes = 1;
            instr.operand[1].mem.base_reg = (prefix.addr_override) ? DISASM_REG_EDI : DISASM_REG_RDI;
            instr.operand[1].mem.idx_reg = DISASM_REG_NONE;
            instr.operand[1].mem.scale = 0;
            instr.operand[1].mem.displacement = 0;
            instr.operand[1].mem.segment = DISASM_SEG_DS;

            return instr;
        case 0x6f:
            instr.opcode = (prefix.op_override) ? DISASM_OUTSW : DISASM_OUTSD;
            instr.instr_len = 1 + prefix.count;
            instr.num_operands = 2;

            instr.operand[0] = _disasm_specific_reg(DISASM_REG_DX);

            instr.operand[1].type = DISASM_OP_TYPE_MEM;
            instr.operand[1].size_bytes = _disasm_operand_8_16_size(prefix);
            instr.operand[1].mem.base_reg = (prefix.addr_override) ? DISASM_REG_EDI : DISASM_REG_RDI;
            instr.operand[1].mem.idx_reg = DISASM_REG_NONE;
            instr.operand[1].mem.scale = 0;
            instr.operand[1].mem.displacement = 0;
            instr.operand[1].mem.segment = DISASM_SEG_DS;

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
            instr.opcode = _disasm_group1_mnemonic(ModRMBytePtr);
            instr.instr_len = 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_rm8(ModRMBytePtr, prefix, &instr.instr_len);
            instr.operand[1] = _disasm_decode_imm8(InstrNext, sizeof(i8), &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x81:
            instr.opcode = _disasm_group1_mnemonic(ModRMBytePtr);
            instr.instr_len = 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_rm16_32_64(ModRMBytePtr, prefix, &instr.instr_len);
            instr.operand[1] = _disasm_decode_imm16_32(InstrNext, prefix, instr.operand[0].size_bytes, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x83:
            instr.opcode = _disasm_group1_mnemonic(ModRMBytePtr);
            instr.instr_len = 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_rm16_32_64(ModRMBytePtr, prefix, &instr.instr_len);
            instr.operand[1] = _disasm_decode_imm8(InstrNext, instr.operand[0].size_bytes, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x84:
        case 0x85:
            instr.opcode = DISASM_TEST;
            instr.instr_len = 1;
            instr.num_operands = 2;
            if ((*instr_ptr & 1) == 0) {
                instr.operand[0] = _disasm_decode_rm8(ModRMBytePtr, prefix, &instr.instr_len);
                instr.operand[1] = _disasm_decode_r8(ModRMBytePtr, prefix);
            } else {
                instr.operand[0] = _disasm_decode_rm16_32_64(ModRMBytePtr, prefix, &instr.instr_len);
                instr.operand[1] = _disasm_decode_r16_32_64(ModRMBytePtr, prefix);
            }
            instr.instr_len += prefix.count;
            return instr;
        case 0x86:
        case 0x87:
            instr.opcode = DISASM_XCHG;
            instr.instr_len = 1;
            instr.num_operands = 2;
            if ((*instr_ptr & 1) == 0) {
                instr.operand[0] = _disasm_decode_rm8(ModRMBytePtr, prefix, &instr.instr_len);
                instr.operand[1] = _disasm_decode_r8(ModRMBytePtr, prefix);
            } else {
                instr.operand[0] = _disasm_decode_rm16_32_64(ModRMBytePtr, prefix, &instr.instr_len);
                instr.operand[1] = _disasm_decode_r16_32_64(ModRMBytePtr, prefix);
            }
            instr.instr_len += prefix.count;
            return instr;
        case 0x88:
        case 0x89:
            instr.opcode = DISASM_MOV;
            instr.instr_len = 1;
            instr.num_operands = 2;
            if ((*instr_ptr & 1) == 0) {
                instr.operand[0] = _disasm_decode_rm8(ModRMBytePtr, prefix, &instr.instr_len);
                instr.operand[1] = _disasm_decode_r8(ModRMBytePtr, prefix);
            } else {
                instr.operand[0] = _disasm_decode_rm16_32_64(ModRMBytePtr, prefix, &instr.instr_len);
                instr.operand[1] = _disasm_decode_r16_32_64(ModRMBytePtr, prefix);
            }
            instr.instr_len += prefix.count;
            return instr;
        case 0x8a:
        case 0x8b:
            instr.opcode = DISASM_MOV;
            instr.instr_len = 1;
            instr.num_operands = 2;
            if ((*instr_ptr & 1) == 0) {
                instr.operand[0] = _disasm_decode_r8(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_rm8(ModRMBytePtr, prefix, &instr.instr_len);
            } else {
                instr.operand[0] = _disasm_decode_r16_32_64(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_rm16_32_64(ModRMBytePtr, prefix, &instr.instr_len);
            }
            instr.instr_len += prefix.count;
            return instr;
        case 0x8c:
            instr.opcode = DISASM_MOV;
            instr.instr_len = 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_m16_r16_32_64(ModRMBytePtr, prefix, &instr.instr_len);
            instr.operand[1] = _disasm_Sreg(ModRMBytePtr, prefix);
            instr.instr_len += prefix.count;
            return instr;
        case 0x8d:
            instr.opcode = DISASM_LEA;
            instr.instr_len = 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_r16_32_64(ModRMBytePtr, prefix);
            instr.operand[1] = _disasm_decode_m64(ModRMBytePtr, prefix, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x8e:
            instr.opcode = DISASM_MOV;
            instr.instr_len = 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_Sreg(ModRMBytePtr, prefix);
            instr.operand[1] = _disasm_decode_rm16(ModRMBytePtr, prefix, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x8f:
            instr.opcode = DISASM_POP;
            instr.instr_len = 1;
            instr.num_operands = 1;
            instr.operand[0] = _disasm_decode_rm16_64(ModRMBytePtr, prefix, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x90:
            instr.opcode = (prefix.repeat) ? DISASM_PAUSE : DISASM_NOP;
            instr.instr_len = 1 + prefix.count;
            instr.num_operands = 0;
            return instr;
        case 0x91: case 0x92: case 0x93: case 0x94:
        case 0x95: case 0x96: case 0x97:
            instr.opcode = DISASM_XCHG;
            instr.instr_len = 1;
            instr.num_operands = 2;

            instr.operand[0].type = DISASM_OP_TYPE_REG;
            instr.operand[0].size_bytes = _disasm_operand_16_32_64_size(prefix);
            instr.operand[0].reg = _disasm_decode_reg((*instr_ptr & 7) | RexB(prefix.rex), instr.operand[0].size_bytes, prefix.rex);

            switch (instr.operand[0].size_bytes) {
                case 2:
                    instr.operand[1] = _disasm_specific_reg(DISASM_REG_AX);
                break;
                case 4:
                    instr.operand[1] = _disasm_specific_reg(DISASM_REG_EAX);
                break;
                case 8:
                    instr.operand[1] = _disasm_specific_reg(DISASM_REG_RAX);
                break;
            }

            instr.instr_len += prefix.count;
            return instr;
        case 0x98:
            instr.instr_len = 1;
            instr.num_operands = 0;
            switch (_disasm_operand_16_32_64_size(prefix)) {
                case 2: instr.opcode = DISASM_CBW; break;
                case 4: instr.opcode = DISASM_CWDE; break;
                case 8: instr.opcode = DISASM_CDQE; break;
            }
            instr.instr_len += prefix.count;
            return instr;
        case 0x99:
            instr.instr_len = 1;
            instr.num_operands = 0;
            switch (_disasm_operand_16_32_64_size(prefix)) {
                case 2: instr.opcode = DISASM_CWD; break;
                case 4: instr.opcode = DISASM_CDQ; break;
                case 8: instr.opcode = DISASM_CQO; break;
            }
            instr.instr_len += prefix.count;
            return instr;
        case 0x9b:
            instr.opcode = DISASM_FWAIT;
            instr.instr_len = 1 + prefix.count;
            instr.num_operands = 0;
            return instr;
        case 0x9c:
            instr.opcode = (prefix.op_override) ? DISASM_PUSHF : DISASM_PUSHFQ;
            instr.instr_len = 1 + prefix.count;
            instr.num_operands = 0;
            return instr;
        case 0x9d:
            instr.opcode = (prefix.op_override) ? DISASM_POPF : DISASM_POPFQ;
            instr.instr_len = 1 + prefix.count;
            instr.num_operands = 0;
            return instr;
        case 0x9e:
            instr.opcode = DISASM_SAHF;
            instr.instr_len = 1 + prefix.count;
            instr.num_operands = 0;
            return instr;
        case 0x9f:
            instr.opcode = DISASM_LAHF;
            instr.instr_len = 1 + prefix.count;
            instr.num_operands = 0;
            return instr;
        case 0xa0:
            instr.opcode = DISASM_MOV;
            instr.instr_len = 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_specific_reg(DISASM_REG_AL);
            instr.operand[1] = _disasm_decode_moffs8(InstrNext, prefix, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0xa1:
            instr.opcode = DISASM_MOV;
            instr.instr_len = 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_specific_reg(DISASM_REG_RAX);
            instr.operand[1] = _disasm_decode_moffs16_32_64(InstrNext, prefix, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0xa2:
            instr.opcode = DISASM_MOV;
            instr.instr_len = 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_moffs8(InstrNext, prefix, &instr.instr_len);
            instr.operand[1] = _disasm_specific_reg(DISASM_REG_AL);
            instr.instr_len += prefix.count;
            return instr;
        case 0xa3:
            instr.opcode = DISASM_MOV;
            instr.instr_len = 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_moffs16_32_64(InstrNext, prefix, &instr.instr_len);
            instr.operand[1] = _disasm_specific_reg(DISASM_REG_RAX);
            instr.instr_len += prefix.count;
            return instr;
        case 0xa4:
            instr.opcode = DISASM_MOVSB;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_string_dest(prefix, 1);
            instr.operand[1] = _disasm_decode_string_src(prefix, 1);
            instr.instr_len = prefix.count + 1;
            return instr;
        case 0xa5:
            {
                instr.num_operands = 2;

                u8 size_bytes = _disasm_operand_16_32_64_size(prefix);
                switch (size_bytes) {
                    case 2: instr.opcode = DISASM_MOVSW; break;
                    case 4: instr.opcode = DISASM_MOVSD; break;
                    case 8: instr.opcode = DISASM_MOVSQ; break;
                }

                instr.operand[0] = _disasm_decode_string_dest(prefix, size_bytes);
                instr.operand[1] = _disasm_decode_string_src(prefix, size_bytes);

                instr.instr_len = prefix.count + 1;
            }
            return instr;
        case 0xa6:
            instr.opcode = DISASM_CMPSB;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_string_src(prefix, 1);
            instr.operand[1] = _disasm_decode_string_dest(prefix, 1);
            instr.instr_len = prefix.count + 1;
            return instr;
        case 0xa7:
            {
                instr.num_operands = 2;

                u8 size_bytes = _disasm_operand_16_32_64_size(prefix);
                switch (size_bytes) {
                    case 2: instr.opcode = DISASM_CMPSW; break;
                    case 4: instr.opcode = DISASM_CMPSD; break;
                    case 8: instr.opcode = DISASM_CMPSQ; break;
                }

                instr.operand[0] = _disasm_decode_string_src(prefix, size_bytes);
                instr.operand[1] = _disasm_decode_string_dest(prefix, size_bytes);

                instr.instr_len = prefix.count + 1;
            }
            return instr;
        case 0xa8:
            instr.opcode = DISASM_TEST;
            instr.instr_len = 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_specific_reg(DISASM_REG_AL);
            instr.operand[1] = _disasm_decode_imm8(InstrNext, 1, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0xa9:
            instr.opcode = DISASM_TEST;
            instr.instr_len = 1;
            instr.num_operands = 2;
            switch (_disasm_operand_16_32_64_size(prefix)) {
                case 2: instr.operand[0] = _disasm_specific_reg(DISASM_REG_AX); break;
                case 4: instr.operand[0] = _disasm_specific_reg(DISASM_REG_EAX); break;
                case 8: instr.operand[0] = _disasm_specific_reg(DISASM_REG_RAX); break;
            }
            instr.operand[1] = _disasm_decode_imm16_32(InstrNext, prefix, 8, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0xaa:
            instr.opcode = DISASM_STOSB;
            instr.instr_len = 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_string_dest(prefix, 1);
            instr.operand[1] = _disasm_specific_reg(DISASM_REG_AL);
            instr.instr_len += prefix.count;
            return instr;
        case 0xab:
            {
                instr.num_operands = 2;

                u8 size_bytes = _disasm_operand_16_32_64_size(prefix);

                instr.operand[0] = _disasm_decode_string_dest(prefix, size_bytes);
                switch (size_bytes) {
                    case 2:
                        instr.opcode = DISASM_STOSW;
                        instr.operand[1] = _disasm_specific_reg(DISASM_REG_AX);
                    break;
                    case 4:
                        instr.opcode = DISASM_STOSD;
                        instr.operand[1] = _disasm_specific_reg(DISASM_REG_EAX);
                    break;
                    case 8:
                        instr.opcode = DISASM_STOSQ;
                        instr.operand[1] = _disasm_specific_reg(DISASM_REG_RAX);
                    break;
                };

                instr.instr_len = prefix.count + 1;
            }
            return instr;
        case 0xac:
            instr.opcode = DISASM_LODSB;
            instr.instr_len = 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_specific_reg(DISASM_REG_AL);
            instr.operand[1] = _disasm_decode_string_src(prefix, 1);
            instr.instr_len += prefix.count;
            return instr;
        case 0xad:
            {
                instr.num_operands = 2;

                u8 size_bytes = _disasm_operand_16_32_64_size(prefix);

                switch (size_bytes) {
                    case 2:
                        instr.opcode = DISASM_LODSW;
                        instr.operand[0] = _disasm_specific_reg(DISASM_REG_AX);
                    break;
                    case 4:
                        instr.opcode = DISASM_LODSD;
                        instr.operand[0] = _disasm_specific_reg(DISASM_REG_EAX);
                    break;
                    case 8:
                        instr.opcode = DISASM_LODSQ;
                        instr.operand[0] = _disasm_specific_reg(DISASM_REG_RAX);
                    break;
                };

                instr.operand[1] = _disasm_decode_string_src(prefix, size_bytes);
                instr.instr_len = prefix.count + 1;
            }
            return instr;
        case 0xae:
            instr.opcode = DISASM_SCASB;
            instr.instr_len = 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_specific_reg(DISASM_REG_AL);
            instr.operand[1] = _disasm_decode_string_dest(prefix, 1);
            instr.instr_len += prefix.count;
            return instr;
        case 0xaf:
            {
                instr.num_operands = 2;

                u8 size_bytes = _disasm_operand_16_32_64_size(prefix);

                switch (size_bytes) {
                    case 2:
                        instr.opcode = DISASM_SCASW;
                        instr.operand[0] = _disasm_specific_reg(DISASM_REG_AX);
                    break;
                    case 4:
                        instr.opcode = DISASM_SCASD;
                        instr.operand[0] = _disasm_specific_reg(DISASM_REG_EAX);
                    break;
                    case 8:
                        instr.opcode = DISASM_SCASQ;
                        instr.operand[0] = _disasm_specific_reg(DISASM_REG_RAX);
                    break;
                };

                instr.operand[1] = _disasm_decode_string_dest(prefix, size_bytes);
                instr.instr_len = prefix.count + 1;
            }
            return instr;
        case 0xb0: case 0xb1: case 0xb2: case 0xb3:
        case 0xb4: case 0xb5: case 0xb6: case 0xb7:
            instr.opcode = DISASM_MOV;
            instr.instr_len = 1;
            instr.num_operands = 2;

            instr.operand[0].type = DISASM_OP_TYPE_REG;
            instr.operand[0].size_bytes = 1;
            instr.operand[0].reg = _disasm_decode_reg((*instr_ptr & 7) | RexB(prefix.rex), instr.operand[0].size_bytes, prefix.rex);

            instr.operand[1] = _disasm_decode_imm8(InstrNext, 1, &instr.instr_len);

            instr.instr_len += prefix.count;
            return instr;
        case 0xb8: case 0xb9: case 0xba: case 0xbb:
        case 0xbc: case 0xbd: case 0xbe: case 0xbf:
            instr.opcode = DISASM_MOV;
            instr.instr_len = 1;
            instr.num_operands = 2;

            instr.operand[0].type = DISASM_OP_TYPE_REG;
            instr.operand[0].size_bytes = _disasm_operand_16_32_64_size(prefix);
            instr.operand[0].reg = _disasm_decode_reg((*instr_ptr & 7) | RexB(prefix.rex), instr.operand[0].size_bytes, prefix.rex);

            instr.operand[1] = _disasm_decode_imm16_32_64(InstrNext, prefix, instr.operand[0].size_bytes, &instr.instr_len);

            instr.instr_len += prefix.count;
            return instr;
        case 0xc0:
            instr.opcode = _disasm_group2_mnemonic(ModRMBytePtr);
            instr.instr_len = 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_rm8(ModRMBytePtr, prefix, &instr.instr_len);
            instr.operand[1] = _disasm_decode_imm8(InstrNext, sizeof(i8), &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0xc1:
            instr.opcode = _disasm_group2_mnemonic(ModRMBytePtr);
            instr.instr_len = 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_rm16_32_64(ModRMBytePtr, prefix, &instr.instr_len);
            instr.operand[1] = _disasm_decode_imm8(InstrNext, instr.operand[0].size_bytes, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0xc2:
            instr.opcode = DISASM_RET;
            instr.instr_len = 1;
            instr.num_operands = 1;
            instr.operand[0] = _disasm_decode_imm16(InstrNext, prefix, sizeof(u16), &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0xc3:
            instr.opcode = DISASM_RET;
            instr.instr_len = 1 + prefix.count;
            instr.num_operands = 0;
            return instr;
        case 0xc6:
            instr.opcode = DISASM_MOV;
            instr.instr_len = 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_rm8(ModRMBytePtr, prefix, &instr.instr_len);
            instr.operand[1] = _disasm_decode_imm8(InstrNext, sizeof(i8), &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0xc7:
            if (((*ModRMBytePtr) & 0b00111000) != 0) {
                instr.opcode = DISASM_INVALID;
                instr.instr_len = 1 + prefix.count;
                instr.num_operands = 0;
                return instr;
            }
            instr.opcode = DISASM_MOV;
            instr.instr_len = 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_rm16_32_64(ModRMBytePtr, prefix, &instr.instr_len);
            instr.operand[1] = _disasm_decode_imm16_32(InstrNext, prefix, instr.operand[0].size_bytes, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0xc8:
            instr.opcode = prefix.op_override ? DISASM_ENTERW : DISASM_ENTER;
            instr.instr_len = 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_imm16(InstrNext, prefix, sizeof(u16), &instr.instr_len);
            instr.operand[1] = _disasm_decode_imm8(InstrNext, sizeof(u8), &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0xc9:
            instr.opcode = DISASM_LEAVE;
            instr.instr_len = 1 + prefix.count;
            instr.num_operands = 0;
            return instr;
        case 0xca:
            instr.opcode = DISASM_RETF;
            instr.instr_len = 1;
            instr.num_operands = 1;
            instr.operand[0] = _disasm_decode_imm16(InstrNext, prefix, sizeof(u16), &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0xcb:
            instr.opcode = DISASM_RETF;
            instr.instr_len = 1 + prefix.count;
            instr.num_operands = 0;
            return instr;
        case 0xcc:
            instr.opcode = DISASM_INT3;
            instr.instr_len = 1 + prefix.count;
            instr.num_operands = 0;
            return instr;
        case 0xcd:
            instr.opcode = DISASM_INT;
            instr.instr_len = 1;
            instr.num_operands = 1;
            instr.operand[0] = _disasm_decode_imm8(InstrNext, sizeof(u8), &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0xcf:
            instr.instr_len = 1 + prefix.count;
            instr.num_operands = 0;
            switch (_disasm_operand_16_32_64_size(prefix)) {
                case 2: instr.opcode = DISASM_IRETW; break;
                case 4: instr.opcode = DISASM_IRET; break;
                case 8: instr.opcode = DISASM_IRETQ; break;
            }
            return instr;
        case 0xd0:
            instr.opcode = _disasm_group2_mnemonic(ModRMBytePtr);
            instr.instr_len = 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_rm8(ModRMBytePtr, prefix, &instr.instr_len);

            instr.operand[1].type = DISASM_OP_TYPE_IMM;
            instr.operand[1].size_bytes = 1;
            instr.operand[1].imm = 1;

            instr.instr_len += prefix.count;
            return instr;
        case 0xd1:
            instr.opcode = _disasm_group2_mnemonic(ModRMBytePtr);
            instr.instr_len = 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_rm16_32_64(ModRMBytePtr, prefix, &instr.instr_len);

            instr.operand[1].type = DISASM_OP_TYPE_IMM;
            instr.operand[1].size_bytes = instr.operand[0].size_bytes;
            instr.operand[1].imm = 1;

            instr.instr_len += prefix.count;
            return instr;
        case 0xd2:
            instr.opcode = _disasm_group2_mnemonic(ModRMBytePtr);
            instr.instr_len = 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_rm8(ModRMBytePtr, prefix, &instr.instr_len);
            instr.operand[1] = _disasm_specific_reg(DISASM_REG_CL);
            instr.instr_len += prefix.count;
            return instr;
        case 0xd3:
            instr.opcode = _disasm_group2_mnemonic(ModRMBytePtr);
            instr.instr_len = 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_rm16_32_64(ModRMBytePtr, prefix, &instr.instr_len);
            instr.operand[1] = _disasm_specific_reg(DISASM_REG_CL);
            instr.instr_len += prefix.count;
            return instr;

    }
 
    return instr;

#undef InstrNext
#undef ModRMBytePtr
}

u8* disasm_seg_stringify(Disasm_Segment seg) {
    switch (seg) {
        case DISASM_SEG_ES: return Stringify(DISASM_SEG_ES);
        case DISASM_SEG_CS: return Stringify(DISASM_SEG_CS);
        case DISASM_SEG_SS: return Stringify(DISASM_SEG_SS);
        case DISASM_SEG_DS: return Stringify(DISASM_SEG_DS);
        case DISASM_SEG_FS: return Stringify(DISASM_SEG_FS);
        case DISASM_SEG_GS: return Stringify(DISASM_SEG_GS);
    }
    return "";
}

String disasm_opcode_format(Arena* arena, Disasm_Opcode opcode) {
    String mnemonic = string_skip(String(disasm_opcode_stringify(opcode)), sizeof("DISASM"));
    return string_to_lower(arena, mnemonic);
}

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

