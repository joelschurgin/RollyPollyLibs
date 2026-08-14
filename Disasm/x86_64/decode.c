#include "decode.h"

#include "instr_set.c"
#include "reg.c"
#include "operands.c"
#include "flops.c"

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
            case 0xf0:
                prefix.lock = 1;
                *instr_ptr += 1;
            break;
            case 0xf2:
                prefix.repeat_nz = 1;
                *instr_ptr += 1;
            break;
            case 0xf3:
                prefix.repeat = 1;
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

Disasm_Instr disasm_decode(u8* instr_ptr) {
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
            if (GetReg(*ModRMBytePtr) != 0) {
                DisasmInvalid;
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
        case 0xd7:
            instr.opcode = DISASM_XLAT;
            instr.instr_len = 1;
            instr.num_operands = 1;
            instr.operand[0] = _disasm_decode_mem_reg(prefix, 1, DISASM_REG_EBX, DISASM_REG_RBX, prefix.segment ? prefix.segment : DISASM_SEG_DS);
            instr.instr_len += prefix.count;
            return instr;
        case 0xd8: return _disasm_decode_flops_d8(instr_ptr, prefix);
        case 0xd9: return _disasm_decode_flops_d9(instr_ptr, prefix);
        case 0xda: return _disasm_decode_flops_da(instr_ptr, prefix);
        case 0xdb: return _disasm_decode_flops_db(instr_ptr, prefix);
        case 0xdc: return _disasm_decode_flops_dc(instr_ptr, prefix);
        case 0xdd: return _disasm_decode_flops_dd(instr_ptr, prefix);
        case 0xde: return _disasm_decode_flops_de(instr_ptr, prefix);
        case 0xdf: return _disasm_decode_flops_df(instr_ptr, prefix);
        case 0xe0:
            instr.opcode = DISASM_LOOPNE;
            instr.instr_len = 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_specific_reg(DISASM_REG_RCX);
            instr.operand[1] = _disasm_decode_rel8(InstrNext, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0xe1:
            instr.opcode = DISASM_LOOPE;
            instr.instr_len = 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_specific_reg(DISASM_REG_RCX);
            instr.operand[1] = _disasm_decode_rel8(InstrNext, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0xe2:
            instr.opcode = DISASM_LOOP;
            instr.instr_len = 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_specific_reg(DISASM_REG_RCX);
            instr.operand[1] = _disasm_decode_rel8(InstrNext, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0xe3:
            instr.instr_len = 1;
            instr.num_operands = 2;
            if (prefix.addr_override) {
                instr.opcode = DISASM_JECXZ;
                instr.operand[0] = _disasm_specific_reg(DISASM_REG_ECX);
            } else {
                instr.opcode = DISASM_JRCXZ;
                instr.operand[0] = _disasm_specific_reg(DISASM_REG_RCX);
            }
            instr.operand[1] = _disasm_decode_rel8(InstrNext, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0xe4:
            instr.opcode = DISASM_IN;
            instr.instr_len = 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_specific_reg(DISASM_REG_AL);
            instr.operand[1] = _disasm_decode_imm8(InstrNext, instr.operand[0].size_bytes, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0xe5:
            instr.opcode = DISASM_IN;
            instr.instr_len = 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_specific_reg(prefix.op_override ? DISASM_REG_AX : DISASM_REG_EAX);
            instr.operand[1] = _disasm_decode_imm8(InstrNext, instr.operand[0].size_bytes, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0xe6:
            instr.opcode = DISASM_OUT;
            instr.instr_len = 1;
            instr.num_operands = 2;
            instr.operand[1] = _disasm_specific_reg(DISASM_REG_AL);
            instr.operand[0] = _disasm_decode_imm8(InstrNext, instr.operand[1].size_bytes, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0xe7:
            instr.opcode = DISASM_OUT;
            instr.instr_len = 1;
            instr.num_operands = 2;
            instr.operand[1] = _disasm_specific_reg(prefix.op_override ? DISASM_REG_AX : DISASM_REG_EAX);
            instr.operand[0] = _disasm_decode_imm8(InstrNext, instr.operand[1].size_bytes, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0xe8:
            instr.opcode = DISASM_CALL;
            instr.instr_len = 1;
            instr.num_operands = 1;
            instr.operand[0] = _disasm_decode_rel16_32(InstrNext, prefix, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0xe9:
            instr.opcode = DISASM_JMP;
            instr.instr_len = 1;
            instr.num_operands = 1;
            instr.operand[0] = _disasm_decode_rel16_32(InstrNext, prefix, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0xeb:
            instr.opcode = DISASM_JMP;
            instr.instr_len = 1;
            instr.num_operands = 1;
            instr.operand[0] = _disasm_decode_rel8(InstrNext, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0xec:
            instr.opcode = DISASM_IN;
            instr.instr_len = 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_specific_reg(DISASM_REG_AL);
            instr.operand[1] = _disasm_specific_reg(DISASM_REG_DX);
            instr.instr_len += prefix.count;
            return instr;
        case 0xed:
            instr.opcode = DISASM_IN;
            instr.instr_len = 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_specific_reg(prefix.op_override ? DISASM_REG_AX : DISASM_REG_EAX);
            instr.operand[1] = _disasm_specific_reg(DISASM_REG_DX);
            instr.instr_len += prefix.count;
            return instr;
        case 0xee:
            instr.opcode = DISASM_OUT;
            instr.instr_len = 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_specific_reg(DISASM_REG_DX);
            instr.operand[1] = _disasm_specific_reg(DISASM_REG_AL);
            instr.instr_len += prefix.count;
            return instr;
        case 0xef:
            instr.opcode = DISASM_OUT;
            instr.instr_len = 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_specific_reg(DISASM_REG_DX);
            instr.operand[1] = _disasm_specific_reg(prefix.op_override ? DISASM_REG_AX : DISASM_REG_EAX);
            instr.instr_len += prefix.count;
            return instr;
    }
 
    return instr;
}

#undef InstrNext
#undef ModRMBytePtr
