#include "disasm_x86_64.h"

#include "disasm_x86_64_instr_set.c"

typedef struct {
    u8 value;
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
            case 0x66:
            case 0x67:
            case 0x64:
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

internal Disasm_Reg _disasm_decode_reg(u8 reg_idx, u8 size_bytes, u8 rex) {
    reg_idx |= rex;
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

internal Disasm_Operand _disasm_decode_rm_operand(u8* rm_byte, u8 rex) {
    Disasm_Operand operand = {0};
    u8 mod = (*rm_byte & 0b11000000) >> 6;
    if (mod == 3) {
        operand.type = DISASM_OP_TYPE_REG;
        operand.size_bytes = 1;
        operand.reg = _disasm_decode_reg(*rm_byte & 0b00000111, operand.size_bytes, rex);
        return operand;
    }

    Assert(!"Memory operand");

    return operand;
}

Disasm_Instr disasm_decode(u8* instr_ptr) {
    Disasm_Instr instr = {0};
    instr.instr = instr_ptr;

    Disasm_Prefix prefix = _disasm_decode_prefix(&instr_ptr);

    switch (*instr_ptr) {
        case 0x00:
            instr.opcode = DISASM_ADD;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_rm_operand(instr_ptr + 1, prefix.rex);
            instr.instr_len = 2;
            //, prefix.rexinstr.operand[1] = _disasm_decode_
            return instr;
    }

    return instr;
}

internal u64 _disasm_decode_mod_rm_length(u8* instr) {
    u8 mod = (instr[1] & 0b11000000) >> 6;
    u8 rm  = (instr[1] & 0b00000111);

    u64 instr_len = 1;

    switch (mod) {
        case 0:
            if (rm == 5) instr_len += 4;
        break;
        case 1:
            instr_len += 1;
        break;
        case 2:
            instr_len += 4;
        break;
        case 3:
        break;
    }

    if (rm == 4 && mod != 3) {
        instr_len += 1;
        if (mod == 0 && ((instr[2] & 0b111) == 5)) {
            instr_len += 4;
        }
    }

    return instr_len;
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

internal Disasm_Opcode _disasm_group1_mnemonic(u8 reg, u64* instr_len) {
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
    *instr_len = 1;
	return DISASM_INVALID;
}

internal Disasm_Opcode _disasm_group2_mnemonic(u8 reg, u64* instr_len) {
    switch (reg) {
        case 0: return DISASM_ROL;
        case 1: return DISASM_ROR;
        case 4: return DISASM_SHL;
        case 5: return DISASM_SHR;
        case 7: return DISASM_SAR;
    }
    *instr_len = 1;
	return DISASM_INVALID;
}

internal Disasm_Opcode _disasm_group3_mnemonic(u8 reg, u64* instr_len) {
    switch (reg) {
        case 0: return DISASM_TEST;
        case 3: return DISASM_NOT;
        case 4: return DISASM_NEG;
        case 5: return DISASM_MUL;
        case 6: return DISASM_IMUL;
        case 7: return DISASM_DIV;
        case 8: return DISASM_IDIV;
    }
    *instr_len = 1;
	return DISASM_INVALID;
}

Disasm_Opcode disasm_decode_opcode_and_length_64(u8* instr, u64* instr_len) {
    Disasm_Prefix prefix = _disasm_decode_prefix(&instr);
    *instr_len = prefix.count;

    if (*instr <= 0x3F) {
        u8 flavor = (*instr) & 7;
        u8 type = (*instr) & ~7;

        switch (flavor) {
            case 0:
            case 1:
            case 2:
            case 3:
            case 4:
                *instr_len += 2;
            break;
            case 5:
                *instr_len += 5;
            break;
            case 6:
                if (*instr == 0x0F) Assert(!"Two byte opcode");
            case 7:
                *instr_len = 1;
	            return DISASM_INVALID;
        }

        switch (type) {
            case 0x00: return DISASM_ADD;
            case 0x08: return DISASM_OR;
            case 0x10: return DISASM_ADC;
            case 0x18: return DISASM_SBB;
            case 0x20: return DISASM_AND;
            case 0x28: return DISASM_SUB;
            case 0x30: return DISASM_XOR;
            case 0x38: return DISASM_CMP;
        }

        *instr_len = 1;
	    return DISASM_INVALID;
    }

    switch (*instr) {
        case 0x50:
        case 0x51:
        case 0x52:
        case 0x53:
        case 0x54:
        case 0x55:
        case 0x56:
        case 0x57:
            *instr_len += 1;
            return DISASM_PUSH;
        case 0x58:
        case 0x59:
        case 0x5a:
        case 0x5b:
        case 0x5c:
        case 0x5d:
        case 0x5e:
        case 0x5f:
            *instr_len += 1;
            return DISASM_POP;
        case 0x60:
        case 0x61:
        case 0x62:
        case 0x63:
            *instr_len = 1;
	        return DISASM_INVALID;
        case 0x68:
            *instr_len += 5;
            return DISASM_PUSH;
        case 0x69:
            *instr_len += 5;
            return DISASM_IMUL;
        case 0x6a:
            *instr_len += 2;
            return DISASM_PUSH;
        case 0x6b:
            *instr_len += 2;
            return DISASM_IMUL;
        case 0x6c:
            *instr_len += 1;
            return DISASM_INSB;
        case 0x6d:
            *instr_len += 1;
            return DISASM_INSD;
        case 0x6e:
            *instr_len += 1;
            return DISASM_OUTSB;
        case 0x6f:
            *instr_len += 1;
            return DISASM_OUTSD;
        case 0x70:
            *instr_len += 2;
            return DISASM_JO;
        case 0x71:
            *instr_len += 2;
            return DISASM_JNO;
        case 0x72:
            *instr_len += 2;
            return DISASM_JB;
        case 0x73:
            *instr_len += 2;
            return DISASM_JNB;
        case 0x74:
            *instr_len += 2;
            return DISASM_JZ;
        case 0x75:
            *instr_len += 2;
            return DISASM_JNZ;
        case 0x76:
            *instr_len += 2;
            return DISASM_JBE;
        case 0x77:
            *instr_len += 2;
            return DISASM_JNBE;
        case 0x78:
            *instr_len += 2;
            return DISASM_JS;
        case 0x79:
            *instr_len += 2;
            return DISASM_JNS;
        case 0x7a:
            *instr_len += 2;
            return DISASM_JP;
        case 0x7b:
            *instr_len += 2;
            return DISASM_JNP;
        case 0x7c:
            *instr_len += 2;
            return DISASM_JL;
        case 0x7d:
            *instr_len += 2;
            return DISASM_JNL;
        case 0x7e:
            *instr_len += 2;
            return DISASM_JLE;
        case 0x7f:
            *instr_len += 2;
            return DISASM_JNLE;
        case 0x80:
        case 0x81:
        case 0x83:
        {
            if (*instr == 0x81) {
                *instr_len += prefix.value == 0x66 ? 3 : 5;
            } else {
                *instr_len += 2;
            }

            *instr_len += _disasm_decode_mod_rm_length(instr);

            u8 reg = (instr[1] & 0b00111000) >> 3;
            return _disasm_group1_mnemonic(reg, instr_len);
        }
        break;
        case 0x82:
            *instr_len = 1;
	        return DISASM_INVALID;
        case 0x84:
        case 0x85:
            *instr_len += 1 + _disasm_decode_mod_rm_length(instr);
            return DISASM_TEST;
        case 0x86:
        case 0x87:
            *instr_len += 1 + _disasm_decode_mod_rm_length(instr);
            return DISASM_XCHG;
        case 0x88:
        case 0x89:
        case 0x8a:
        case 0x8b:
        case 0x8c:
        case 0x8e:
            *instr_len += 1 + _disasm_decode_mod_rm_length(instr);
            return DISASM_MOV;
        case 0x8d:
            if (((instr[1] & 0b11000000) >> 6) == 3) {
                *instr_len = 1;
	            return DISASM_INVALID;
            }
            *instr_len += 1 + _disasm_decode_mod_rm_length(instr);
            return DISASM_LEA;
        case 0x8f:
            if (((instr[1] & 0b11000000) >> 6) == 3) {
                *instr_len = 1;
	            return DISASM_INVALID;
            }
            *instr_len += 1 + _disasm_decode_mod_rm_length(instr);
            return DISASM_POP;
        case 0x90:
            *instr_len += 1;
            return prefix.value == 0xf3 ? DISASM_PAUSE : DISASM_NOP;
        case 0x91:
        case 0x92:
        case 0x93:
        case 0x94:
        case 0x95:
        case 0x96:
        case 0x97:
            *instr_len += 1;
            return DISASM_XCHG;
        case 0x98:
            *instr_len += 1;
            if (prefix.rex & 0x08) return DISASM_CDQE;
            else if (prefix.value == 0x66) return DISASM_CBW;
            return DISASM_CWDE;
        case 0x99:
            *instr_len += 1;
            if (prefix.rex & 0x08) return DISASM_CQO;
            else if (prefix.value == 0x66) return DISASM_CWD;
            return DISASM_CDQ;
        case 0x9a:
            *instr_len = 1;
	        return DISASM_INVALID;
        case 0x9b:
            *instr_len += 1;
            return DISASM_FWAIT;
        case 0x9c:
            *instr_len += 1;
            return (prefix.value == 0x66) ? DISASM_PUSHF : DISASM_PUSHFQ;
        case 0x9d:
            *instr_len += 1;
            return (prefix.value == 0x66) ? DISASM_POPF : DISASM_POPFQ;
        case 0x9E:
            *instr_len += 1;
            return DISASM_SAHF;
        case 0x9F:
            *instr_len += 1;
            return DISASM_LAHF;
        case 0xa0:
        case 0xa1:
        case 0xa2:
        case 0xa3:
            *instr_len += 1 + (prefix.value == 0x67 ? 4 : 8);
            return DISASM_MOV;
        case 0xa4:
            *instr_len += 1;
            return DISASM_MOVSB;
        case 0xa5:
            *instr_len += 1;
            return (prefix.value == 0x66) ? DISASM_MOVSW : DISASM_MOVSQ;
        case 0xa6:
            *instr_len += 1;
            return DISASM_CMPSB;
        case 0xa7:
            *instr_len += 1;
            return (prefix.value == 0x66) ? DISASM_CMPSW : DISASM_CMPSQ;
        case 0xa8:
            *instr_len += 2;
            return DISASM_TEST;
        case 0xa9:
            *instr_len += 1 + (prefix.value == 0x66 ? 2 : 4);
            return DISASM_TEST;
        case 0xaa:
            *instr_len += 1;
            return DISASM_STOSB;
        case 0xab:
            *instr_len += 1;
            return (prefix.value == 0x66) ? DISASM_STOSW : DISASM_STOSQ;
        case 0xac:
            *instr_len += 1;
            return DISASM_LODSB;
        case 0xad:
            *instr_len += 1;
            return (prefix.value == 0x66) ? DISASM_LODSW : DISASM_LODSQ;
        case 0xae:
            *instr_len += 1;
            return DISASM_SCASB;
        case 0xaf:
            *instr_len += 1;
            return (prefix.value == 0x66) ? DISASM_SCASW : DISASM_SCASQ;
        case 0xb0:
        case 0xb1:
        case 0xb2:
        case 0xb3:
        case 0xb4:
        case 0xb5:
        case 0xb6:
        case 0xb7:
            *instr_len += 2;
            return DISASM_MOV;
        case 0xb8:
        case 0xb9:
        case 0xba:
        case 0xbb:
        case 0xbc:
        case 0xbd:
        case 0xbe:
        case 0xbf:
            *instr_len += 1;
            if (prefix.rex & 0x08) *instr_len += 8;
            else if (prefix.value == 0x66) *instr_len += 2;
            else *instr_len += 4;
            return DISASM_MOV;
        case 0xc0:
        case 0xc1:
        {
            *instr_len += 2 + _disasm_decode_mod_rm_length(instr);

            u8 reg = (instr[1] & 0b00111000) >> 3;
            return _disasm_group2_mnemonic(reg, instr_len);
        }
        case 0xc2:
            *instr_len += 3;
            return DISASM_RET;
        case 0xc3:
            *instr_len += 1;
            return DISASM_RET;
        case 0xc4:
        case 0xc5:
            *instr_len = 1;
	        return DISASM_INVALID;
        case 0xc6:
            if (((instr[1] & 0b00111000) >> 3) != 0) {
                *instr_len = 1;
	            return DISASM_INVALID;
            }
            *instr_len += 2 + _disasm_decode_mod_rm_length(instr);
            return DISASM_MOV;
        case 0xc7:
            if (((instr[1] & 0b00111000) >> 3) != 0) {
                *instr_len = 1;
	            return DISASM_INVALID;
            }
            *instr_len += 1 + (prefix.value == 0x66 ? 2 : 4) + _disasm_decode_mod_rm_length(instr);
            return DISASM_MOV;
        case 0xc8:
            *instr_len += 4;
            return DISASM_ENTER;
        case 0xc9:
            *instr_len += 1;
            return DISASM_LEAVE;
        case 0xca:
            *instr_len += 3;
            return DISASM_RETF;
        case 0xcb:
            *instr_len += 1;
            return DISASM_RETF;
        case 0xcc:
            *instr_len += 1;
            return DISASM_INT3;
        case 0xcd:
            *instr_len += 2;
            return DISASM_INT;
        case 0xce:
            *instr_len = 1;
	        return DISASM_INVALID;
        case 0xcf:
            *instr_len += 1;
            if (prefix.rex & 0x08) return DISASM_IRETQ;
            else if (prefix.value == 0x66) return DISASM_IRET;
            return DISASM_IRETD;
        case 0xd0:
        case 0xd1:
        case 0xd2:
        case 0xd3:
        {
            *instr_len += 1 + _disasm_decode_mod_rm_length(instr);

            u8 reg = (instr[1] & 0b00111000) >> 3;
            return _disasm_group2_mnemonic(reg, instr_len);
        }
        case 0xd4:
        case 0xd5:
        case 0xd6:
            *instr_len = 1;
	        return DISASM_INVALID;
        case 0xd7:
            *instr_len += 1;
            return DISASM_XLAT;
        case 0xd8:
        case 0xd9:
        case 0xda:
        case 0xdb:
        case 0xdc:
        case 0xdd:
        case 0xde:
        case 0xdf:
            *instr_len += 1 + _disasm_decode_mod_rm_length(instr);
            return _disasm_decode_fpu_mnemonic(prefix, instr, instr_len);
        case 0xe0:
            *instr_len += 2;
            return DISASM_LOOPNE;
        case 0xe1:
            *instr_len += 2;
            return DISASM_LOOPE;
        case 0xe2:
            *instr_len += 2;
            return DISASM_LOOP;
        case 0xe3:
            *instr_len += 2;
            return (prefix.value == 0x67) ? DISASM_JECXZ : DISASM_JRCXZ;
        case 0xe4:
        case 0xe5:
            *instr_len += 2;
            return DISASM_IN;
        case 0xe6:
        case 0xe7:
            *instr_len += 2;
            return DISASM_OUT;
        case 0xe8:
            *instr_len += 1 + ((prefix.value == 0x66) ? 2 : 4);
            return DISASM_CALL;
        case 0xe9:
            *instr_len += 1 + ((prefix.value == 0x66) ? 2 : 4);
            return DISASM_JMP;
        case 0xea:
            *instr_len = 1;
	        return DISASM_INVALID;
        case 0xeb:
            *instr_len += 2;
            return DISASM_JMP;
        case 0xec:
        case 0xed:
            *instr_len += 1;
            return DISASM_IN;
        case 0xee:
        case 0xef:
            *instr_len += 1;
            return DISASM_OUT;
        case 0xf0:
        case 0xf1:
        case 0xf2:
        case 0xf3:
            *instr_len = 1;
	        return DISASM_INVALID;
        case 0xf4:
            *instr_len += 1;
            return DISASM_HLT;
        case 0xf5:
            *instr_len += 1;
            return DISASM_CMC;
        case 0xf6:
        {
            u8 reg = (instr[1] & 0b00111000) >> 3;
            *instr_len += 1 + _disasm_decode_mod_rm_length(instr + 1) + (reg == 0);
            return _disasm_group3_mnemonic(reg, instr_len);
        }
        case 0xf7:
        {
            u8 reg = (instr[1] & 0b00111000) >> 3;

            u32 imm_size = 0;
            if (reg == 0) imm_size = prefix.value == 0x66 ? 2 : 4;

            *instr_len += 1 + _disasm_decode_mod_rm_length(instr + 1) + imm_size;
            return _disasm_group3_mnemonic(reg, instr_len);
        }
    }

    *instr_len = 1;
	return DISASM_INVALID;
}

String disasm_opcode_format(Arena* arena, Disasm_Opcode opcode) {
    String mnemonic = string_skip(String(disasm_opcode_stringify(opcode)), 7);
    return string_to_lower(arena, mnemonic);
}
