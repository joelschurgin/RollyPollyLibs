internal Disasm_Instr _disasm_decode_flops_d8(u8* instr_ptr, Disasm_Prefix prefix) {
    Disasm_Instr instr = {0};
    instr.instr = instr_ptr - prefix.count;
    switch (GetReg(*ModRMBytePtr)) {
        case 0: instr.opcode = DISASM_FADD; break;
        case 1: instr.opcode = DISASM_FMUL; break;
        case 2: instr.opcode = DISASM_FCOM; break;
        case 3: instr.opcode = DISASM_FCOMP; break;
        case 4: instr.opcode = DISASM_FSUB; break;
        case 5: instr.opcode = DISASM_FSUBR; break;
        case 6: instr.opcode = DISASM_FDIV; break;
        case 7: instr.opcode = DISASM_FDIVR; break;
    }
    instr.instr_len = 1;
    instr.num_operands = 2;
    instr.operand[0] = _disasm_specific_reg(DISASM_REG_ST);
    instr.operand[1] = _disasm_decode_sti_m32real(ModRMBytePtr, prefix, &instr.instr_len);
    instr.instr_len += prefix.count;
    return instr;
}

internal Disasm_Instr _disasm_decode_flops_d9(u8* instr_ptr, Disasm_Prefix prefix) {
    Disasm_Instr instr = {0};
    instr.instr = instr_ptr - prefix.count;
    instr.instr_len = 1;
    switch (GetReg(*ModRMBytePtr)) {
        case 0:
            instr.opcode = DISASM_FLD;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_specific_reg(DISASM_REG_ST);
            instr.operand[1] = _disasm_decode_sti_m32real(ModRMBytePtr, prefix, &instr.instr_len);
        break;
        case 1:
            instr.opcode = DISASM_FXCH;
            if (GetMod(*ModRMBytePtr) != 3) {
                DisasmInvalid;
                instr.instr_len += 1;
                return instr;
            }
            instr.num_operands = 2;
            instr.operand[0] = _disasm_specific_reg(DISASM_REG_ST);
            instr.operand[1] = _disasm_decode_sti_m32real(ModRMBytePtr, prefix, &instr.instr_len);
        break;
        case 2:
            if (GetMod(*ModRMBytePtr) == 3) {
                instr.opcode = GetRM(*ModRMBytePtr) == 0 ? DISASM_FNOP : DISASM_INVALID;
                instr.num_operands = 0;
                instr.instr_len += 1;
            } else {
                instr.opcode = DISASM_FST;
                instr.num_operands = 2;
                instr.operand[0] = _disasm_decode_sti_m32real(ModRMBytePtr, prefix, &instr.instr_len);
                instr.operand[1] = _disasm_specific_reg(DISASM_REG_ST);
            }
        break;
        case 3:
            if (GetMod(*ModRMBytePtr) == 3) {
                DisasmInvalid;
                instr.instr_len += 1;
                return instr;
            }
            instr.opcode = DISASM_FSTP;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_sti_m32real(ModRMBytePtr, prefix, &instr.instr_len);
            instr.operand[1] = _disasm_specific_reg(DISASM_REG_ST);
        break;
        case 4:
            if (GetMod(*ModRMBytePtr) != 3) {
                instr.opcode = DISASM_FLDENV;
                instr.num_operands = 1;
                instr.instr_len += 1;
                instr.operand[0] = _disasm_decode_m14_28(ModRMBytePtr, prefix, &instr.instr_len);
            } else {
                instr.num_operands = 1;
                instr.operand[0] = _disasm_specific_reg(DISASM_REG_ST);
                switch (GetRM(*ModRMBytePtr)) {
                    case 0: instr.opcode = DISASM_FCHS; break;
                    case 1: instr.opcode = DISASM_FABS; break;
                    case 4: instr.opcode = DISASM_FTST; break;
                    case 5: instr.opcode = DISASM_FXAM; break;
                    default:
                        DisasmInvalid;
                        instr.instr_len += 1;
                        return instr;
                }
                instr.instr_len += 1;
            }
        break;
        case 5:
            if (GetMod(*ModRMBytePtr) != 3) {
                instr.opcode = DISASM_FLDCW;
                instr.num_operands = 1;
                instr.operand[0] = _disasm_decode_m16(ModRMBytePtr, prefix, &instr.instr_len);
            } else {
                instr.num_operands = 1;
                instr.operand[0] = _disasm_specific_reg(DISASM_REG_ST);
                switch (GetRM(*ModRMBytePtr)) {
                    case 0: instr.opcode = DISASM_FLD1; break;
                    case 1: instr.opcode = DISASM_FLDL2T; break;
                    case 2: instr.opcode = DISASM_FLDL2E; break;
                    case 3: instr.opcode = DISASM_FLDPI; break;
                    case 4: instr.opcode = DISASM_FLDLG2; break;
                    case 5: instr.opcode = DISASM_FLDLN2; break;
                    case 6: instr.opcode = DISASM_FLDZ; break;
                    default:
                        DisasmInvalid;
                        instr.instr_len += 1;
                        return instr;
                }
                instr.instr_len += 1;
            }
        break;
        case 6:
            if (GetMod(*ModRMBytePtr) != 3) {
                instr.opcode = DISASM_FNSTENV;
                instr.num_operands = 1;
                instr.operand[0] = _disasm_decode_m14_28(ModRMBytePtr, prefix, &instr.instr_len);
            } else {
                instr.instr_len += 1;
                switch (GetRM(*ModRMBytePtr)) {
                    case 0:
                        instr.opcode = DISASM_F2XM1;
                        instr.num_operands = 1;
                        instr.operand[0] = _disasm_specific_reg(DISASM_REG_ST);
                    break;
                    case 1:
                        instr.opcode = DISASM_FYL2X;
                        instr.num_operands = 2;
                        instr.operand[0] = _disasm_specific_reg(DISASM_REG_ST1);
                        instr.operand[1] = _disasm_specific_reg(DISASM_REG_ST);
                    break;
                    case 2:
                        instr.opcode = DISASM_FPTAN;
                        instr.num_operands = 1;
                        instr.operand[0] = _disasm_specific_reg(DISASM_REG_ST);
                    break;
                    case 3:
                        instr.opcode = DISASM_FPATAN;
                        instr.num_operands = 2;
                        instr.operand[0] = _disasm_specific_reg(DISASM_REG_ST1);
                        instr.operand[1] = _disasm_specific_reg(DISASM_REG_ST);
                    break;
                    case 4:
                        instr.opcode = DISASM_FXTRACT;
                        instr.num_operands = 1;
                        instr.operand[0] = _disasm_specific_reg(DISASM_REG_ST);
                    break;
                    case 5:
                        instr.opcode = DISASM_FPREM1;
                        instr.num_operands = 2;
                        instr.operand[0] = _disasm_specific_reg(DISASM_REG_ST1);
                        instr.operand[1] = _disasm_specific_reg(DISASM_REG_ST);
                    break;
                    case 6:
                        instr.opcode = DISASM_FDECSTP;
                        instr.num_operands = 0;
                    break;
                    case 7:
                        instr.opcode = DISASM_FINCSTP;
                        instr.num_operands = 0;
                    break;
                }
            }
        break;
        case 7:
            if (GetMod(*ModRMBytePtr) != 3) {
                instr.opcode = DISASM_FNSTCW;
                instr.num_operands = 1;
                instr.operand[0] = _disasm_decode_m16(ModRMBytePtr, prefix, &instr.instr_len);
            } else {
                instr.instr_len += 1;
                switch (GetRM(*ModRMBytePtr)) {
                    case 0:
                        instr.opcode = DISASM_FPREM;
                        instr.num_operands = 1;
                        instr.operand[0] = _disasm_specific_reg(DISASM_REG_ST);
                    break;
                    case 1:
                        instr.opcode = DISASM_FYL2XP1;
                        instr.num_operands = 2;
                        instr.operand[0] = _disasm_specific_reg(DISASM_REG_ST1);
                        instr.operand[1] = _disasm_specific_reg(DISASM_REG_ST);
                    break;
                    case 2:
                        instr.opcode = DISASM_FSQRT;
                        instr.num_operands = 1;
                        instr.operand[0] = _disasm_specific_reg(DISASM_REG_ST);
                    break;
                    case 3:
                        instr.opcode = DISASM_FSINCOS;
                        instr.num_operands = 1;
                        instr.operand[0] = _disasm_specific_reg(DISASM_REG_ST);
                    break;
                    case 4:
                        instr.opcode = DISASM_FRNDINT;
                        instr.num_operands = 1;
                        instr.operand[0] = _disasm_specific_reg(DISASM_REG_ST);
                    break;
                    case 5:
                        instr.opcode = DISASM_FSCALE;
                        instr.num_operands = 2;
                        instr.operand[0] = _disasm_specific_reg(DISASM_REG_ST1);
                        instr.operand[1] = _disasm_specific_reg(DISASM_REG_ST);
                    break;
                    case 6:
                        instr.opcode = DISASM_FSIN;
                        instr.num_operands = 1;
                        instr.operand[0] = _disasm_specific_reg(DISASM_REG_ST);
                    break;
                    case 7:
                        instr.opcode = DISASM_FCOS;
                        instr.num_operands = 1;
                        instr.operand[0] = _disasm_specific_reg(DISASM_REG_ST);
                    break;
                }
            }
        break;
    }
    instr.instr_len += prefix.count;
    return instr;
}

internal Disasm_Instr _disasm_decode_flops_da(u8* instr_ptr, Disasm_Prefix prefix) {
    Disasm_Instr instr = {0};
    instr.instr = instr_ptr - prefix.count;
    instr.instr_len = 1;
    switch (GetReg(*ModRMBytePtr)) {
        case 0:
            instr.num_operands = 2;
            instr.operand[0] = _disasm_specific_reg(DISASM_REG_ST);
            if (GetMod(*ModRMBytePtr) != 3) {
                instr.opcode = DISASM_FIADD;
                instr.operand[1] = _disasm_decode_m32int(ModRMBytePtr, prefix, &instr.instr_len);
            } else {
                instr.opcode = DISASM_FCMOVB;
                instr.operand[1] = _disasm_decode_sti(ModRMBytePtr, prefix, &instr.instr_len);
            }
        break;
        case 1:
            instr.num_operands = 2;
            instr.operand[0] = _disasm_specific_reg(DISASM_REG_ST);
            if (GetMod(*ModRMBytePtr) != 3) {
                instr.opcode = DISASM_FIMUL;
                instr.operand[1] = _disasm_decode_m32int(ModRMBytePtr, prefix, &instr.instr_len);
            } else {
                instr.opcode = DISASM_FCMOVE;
                instr.operand[1] = _disasm_decode_sti(ModRMBytePtr, prefix, &instr.instr_len);
            }
        break;
        case 2:
            instr.num_operands = 2;
            instr.operand[0] = _disasm_specific_reg(DISASM_REG_ST);
            if (GetMod(*ModRMBytePtr) != 3) {
                instr.opcode = DISASM_FICOM;
                instr.operand[1] = _disasm_decode_m32int(ModRMBytePtr, prefix, &instr.instr_len);
            } else {
                instr.opcode = DISASM_FCMOVBE;
                instr.operand[1] = _disasm_decode_sti(ModRMBytePtr, prefix, &instr.instr_len);
            }
        break;
        case 3:
            instr.num_operands = 2;
            instr.operand[0] = _disasm_specific_reg(DISASM_REG_ST);
            if (GetMod(*ModRMBytePtr) != 3) {
                instr.opcode = DISASM_FICOMP;
                instr.operand[1] = _disasm_decode_m32int(ModRMBytePtr, prefix, &instr.instr_len);
            } else {
                instr.opcode = DISASM_FCMOVU;
                instr.operand[1] = _disasm_decode_sti(ModRMBytePtr, prefix, &instr.instr_len);
            }
        break;
        case 4:
            instr.num_operands = 2;
            instr.operand[0] = _disasm_specific_reg(DISASM_REG_ST);
            if (GetMod(*ModRMBytePtr) != 3) {
                instr.opcode = DISASM_FISUB;
                instr.operand[1] = _disasm_decode_m32int(ModRMBytePtr, prefix, &instr.instr_len);
            } else {
                DisasmInvalid;
                instr.instr_len += 1;
                return instr;
            }
        break;
        case 5:
            instr.num_operands = 2;
            instr.operand[0] = _disasm_specific_reg(DISASM_REG_ST);
            if (GetMod(*ModRMBytePtr) != 3) {
                instr.opcode = DISASM_FISUBR;
                instr.operand[1] = _disasm_decode_m32int(ModRMBytePtr, prefix, &instr.instr_len);
            } else if (*ModRMBytePtr == 0xe9){
                instr.opcode = DISASM_FUCOMPP;
                instr.operand[1] = _disasm_specific_reg(DISASM_REG_ST1);
                instr.instr_len += 1;
            } else {
                DisasmInvalid;
                instr.instr_len += 1;
                return instr;
            }
        break;
        case 6:
            instr.num_operands = 2;
            instr.operand[0] = _disasm_specific_reg(DISASM_REG_ST);
            if (GetMod(*ModRMBytePtr) != 3) {
                instr.opcode = DISASM_FIDIV;
                instr.operand[1] = _disasm_decode_m32int(ModRMBytePtr, prefix, &instr.instr_len);
            } else {
                DisasmInvalid;
                instr.instr_len += 1;
                return instr;
            }
        break;
        case 7:
            instr.num_operands = 2;
            instr.operand[0] = _disasm_specific_reg(DISASM_REG_ST);
            if (GetMod(*ModRMBytePtr) != 3) {
                instr.opcode = DISASM_FIDIVR;
                instr.operand[1] = _disasm_decode_m32int(ModRMBytePtr, prefix, &instr.instr_len);
            } else {
                DisasmInvalid;
                instr.instr_len += 1;
                return instr;
            }
        break;
    }
    instr.instr_len += prefix.count;
    return instr;
}
