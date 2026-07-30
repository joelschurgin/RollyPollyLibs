#define DisasmInvalidFlop do { \
                DisasmInvalid; \
                instr.instr_len += 1; \
                return instr; \
            } while (0);

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
    if (GetMod(*ModRMBytePtr) != 3) {
        switch (GetReg(*ModRMBytePtr)) {
            case 0:
                instr.opcode = DISASM_FLD;
                instr.num_operands = 2;
                instr.operand[0] = _disasm_specific_reg(DISASM_REG_ST);
                instr.operand[1] = _disasm_decode_sti_m32real(ModRMBytePtr, prefix, &instr.instr_len);
            break;
            case 2:
                instr.opcode = DISASM_FST;
                instr.num_operands = 2;
                instr.operand[0] = _disasm_decode_sti_m32real(ModRMBytePtr, prefix, &instr.instr_len);
                instr.operand[1] = _disasm_specific_reg(DISASM_REG_ST);
            break;
            case 3:
                instr.opcode = DISASM_FSTP;
                instr.num_operands = 2;
                instr.operand[0] = _disasm_decode_sti_m32real(ModRMBytePtr, prefix, &instr.instr_len);
                instr.operand[1] = _disasm_specific_reg(DISASM_REG_ST);
            break;
            case 4:
                instr.opcode = DISASM_FLDENV;
                instr.num_operands = 1;
                instr.operand[0] = _disasm_decode_m14_28(ModRMBytePtr, prefix, &instr.instr_len);
            break;
            case 5:
                instr.opcode = DISASM_FLDCW;
                instr.num_operands = 1;
                instr.operand[0] = _disasm_decode_m16(ModRMBytePtr, prefix, &instr.instr_len);
            break;
            case 6:
                instr.opcode = DISASM_FNSTENV;
                instr.num_operands = 1;
                instr.operand[0] = _disasm_decode_m14_28(ModRMBytePtr, prefix, &instr.instr_len);
            break;
            case 7:
                instr.opcode = DISASM_FNSTCW;
                instr.num_operands = 1;
                instr.operand[0] = _disasm_decode_m16(ModRMBytePtr, prefix, &instr.instr_len);
            default:
                DisasmInvalidFlop;
        }
    } else {
        switch (GetReg(*ModRMBytePtr)) {
            case 0:
                instr.opcode = DISASM_FLD;
                instr.num_operands = 2;
                instr.operand[0] = _disasm_specific_reg(DISASM_REG_ST);
                instr.operand[1] = _disasm_decode_sti_m32real(ModRMBytePtr, prefix, &instr.instr_len);
            break;
            case 1:
                instr.opcode = DISASM_FXCH;
                instr.num_operands = 2;
                instr.operand[0] = _disasm_specific_reg(DISASM_REG_ST);
                instr.operand[1] = _disasm_decode_sti_m32real(ModRMBytePtr, prefix, &instr.instr_len);
            break;
            case 2:
                instr.opcode = GetRM(*ModRMBytePtr) == 0 ? DISASM_FNOP : DISASM_INVALID;
                instr.num_operands = 0;
                instr.instr_len += 1;
            break;
            case 4:
                switch (GetRM(*ModRMBytePtr)) {
                    case 0: instr.opcode = DISASM_FCHS; break;
                    case 1: instr.opcode = DISASM_FABS; break;
                    case 4: instr.opcode = DISASM_FTST; break;
                    case 5: instr.opcode = DISASM_FXAM; break;
                    default:
                        DisasmInvalidFlop;
                }
                instr.num_operands = 1;
                instr.operand[0] = _disasm_specific_reg(DISASM_REG_ST);
                instr.instr_len += 1;
            break;
            case 5:
                switch (GetRM(*ModRMBytePtr)) {
                    case 0: instr.opcode = DISASM_FLD1; break;
                    case 1: instr.opcode = DISASM_FLDL2T; break;
                    case 2: instr.opcode = DISASM_FLDL2E; break;
                    case 3: instr.opcode = DISASM_FLDPI; break;
                    case 4: instr.opcode = DISASM_FLDLG2; break;
                    case 5: instr.opcode = DISASM_FLDLN2; break;
                    case 6: instr.opcode = DISASM_FLDZ; break;
                    default:
                        DisasmInvalidFlop;
                }
                instr.num_operands = 1;
                instr.operand[0] = _disasm_specific_reg(DISASM_REG_ST);
                instr.instr_len += 1;
            break;
            case 6:
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
            break;
            case 7:
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
            break;
            default:
                DisasmInvalidFlop;
        }
    }
    instr.instr_len += prefix.count;
    return instr;
}

internal Disasm_Instr _disasm_decode_flops_da(u8* instr_ptr, Disasm_Prefix prefix) {
    Disasm_Instr instr = {0};
    instr.instr = instr_ptr - prefix.count;
    instr.instr_len = 1;

    if (GetMod(*ModRMBytePtr) != 3) {
        instr.num_operands = 2;
        instr.operand[0] = _disasm_specific_reg(DISASM_REG_ST);
        instr.operand[1] = _disasm_decode_m32int(ModRMBytePtr, prefix, &instr.instr_len);
        switch (GetReg(*ModRMBytePtr)) {
            case 0: instr.opcode = DISASM_FIADD; break;
            case 1: instr.opcode = DISASM_FIMUL; break;
            case 2: instr.opcode = DISASM_FICOM; break;
            case 3: instr.opcode = DISASM_FICOMP; break;
            case 4: instr.opcode = DISASM_FISUB; break;
            case 5: instr.opcode = DISASM_FISUBR; break;
            case 6: instr.opcode = DISASM_FIDIV; break;
            case 7: instr.opcode = DISASM_FIDIVR; break;
        }
    } else {
        instr.num_operands = 2;
        instr.operand[0] = _disasm_specific_reg(DISASM_REG_ST);
        instr.operand[1] = _disasm_decode_sti(ModRMBytePtr, prefix, &instr.instr_len);
        switch (GetReg(*ModRMBytePtr)) {
            case 0: instr.opcode = DISASM_FCMOVB; break;
            case 1: instr.opcode = DISASM_FCMOVE; break;
            case 2: instr.opcode = DISASM_FCMOVBE; break;
            case 3: instr.opcode = DISASM_FCMOVU; break;
            case 5:
                if (*ModRMBytePtr != 0xe9) DisasmInvalidFlop;
                instr.opcode = DISASM_FUCOMPP;
            break;
            default:
                DisasmInvalidFlop;
        }
    }
    instr.instr_len += prefix.count;
    return instr;
}

internal Disasm_Instr _disasm_decode_flops_db(u8* instr_ptr, Disasm_Prefix prefix) {
    Disasm_Instr instr = {0};
    instr.instr = instr_ptr - prefix.count;
    instr.instr_len = 1;
    switch (GetReg(*ModRMBytePtr)) {
        case 0:
            instr.num_operands = 2;
            if (GetMod(*ModRMBytePtr) != 3) {
                instr.opcode = DISASM_FILD;
                instr.operand[0] = _disasm_specific_reg(DISASM_REG_ST);
                instr.operand[1] = _disasm_decode_m32int(ModRMBytePtr, prefix, &instr.instr_len);
            } else {
                instr.opcode = DISASM_FCMOVNB;
                instr.operand[0] = _disasm_specific_reg(DISASM_REG_ST);
                instr.operand[1] = _disasm_decode_sti(ModRMBytePtr, prefix, &instr.instr_len);
            }
        break;
        case 1:
            instr.num_operands = 2;
            instr.operand[0] = _disasm_specific_reg(DISASM_REG_ST);
            if (GetMod(*ModRMBytePtr) != 3) {
                instr.opcode = DISASM_FISTTP;
                instr.operand[0] = _disasm_decode_m32int(ModRMBytePtr, prefix, &instr.instr_len);
                instr.operand[1] = _disasm_specific_reg(DISASM_REG_ST);
            } else {
                instr.opcode = DISASM_FCMOVNE;
                instr.operand[0] = _disasm_specific_reg(DISASM_REG_ST);
                instr.operand[1] = _disasm_decode_sti(ModRMBytePtr, prefix, &instr.instr_len);
            }
        break;
        case 2:
            instr.num_operands = 2;
            if (GetMod(*ModRMBytePtr) != 3) {
                instr.opcode = DISASM_FIST;
                instr.operand[0] = _disasm_decode_m32int(ModRMBytePtr, prefix, &instr.instr_len);
                instr.operand[1] = _disasm_specific_reg(DISASM_REG_ST);
            } else {
                instr.opcode = DISASM_FCMOVNBE;
                instr.operand[0] = _disasm_specific_reg(DISASM_REG_ST);
                instr.operand[1] = _disasm_decode_sti(ModRMBytePtr, prefix, &instr.instr_len);
            }
        break;
        case 3:
            instr.num_operands = 2;
            if (GetMod(*ModRMBytePtr) != 3) {
                instr.opcode = DISASM_FISTP;
                instr.operand[0] = _disasm_decode_m32int(ModRMBytePtr, prefix, &instr.instr_len);
                instr.operand[1] = _disasm_specific_reg(DISASM_REG_ST);
            } else {
                instr.opcode = DISASM_FCMOVNU;
                instr.operand[0] = _disasm_specific_reg(DISASM_REG_ST);
                instr.operand[1] = _disasm_decode_sti(ModRMBytePtr, prefix, &instr.instr_len);
            }
        break;
        case 4:
            instr.num_operands = 0;
            instr.instr_len += 1;
            switch (GetRM(*ModRMBytePtr)) {
                case 2: instr.opcode = DISASM_FNCLEX; break;
                case 3: instr.opcode = DISASM_FNINIT; break;
                default:
                    DisasmInvalidFlop;
            }
        break;
        case 5:
            instr.num_operands = 2;
            instr.operand[0] = _disasm_specific_reg(DISASM_REG_ST);
            if (GetMod(*ModRMBytePtr) != 3) {
                instr.opcode = DISASM_FLD;
                instr.operand[0] = _disasm_specific_reg(DISASM_REG_ST);
                instr.operand[1] = _disasm_decode_m80real(ModRMBytePtr, prefix, &instr.instr_len);
            } else {
                instr.opcode = DISASM_FUCOMI;
                instr.operand[0] = _disasm_specific_reg(DISASM_REG_ST);
                instr.operand[1] = _disasm_decode_sti(ModRMBytePtr, prefix, &instr.instr_len);
            }
        break;
        case 6:
            instr.num_operands = 2;
            if (GetMod(*ModRMBytePtr) != 3) {
                DisasmInvalidFlop;
            } else {
                instr.opcode = DISASM_FCOMI;
                instr.operand[0] = _disasm_specific_reg(DISASM_REG_ST);
                instr.operand[1] = _disasm_decode_sti(ModRMBytePtr, prefix, &instr.instr_len);
            }
        break;
        case 7:
            instr.num_operands = 2;
            instr.operand[0] = _disasm_specific_reg(DISASM_REG_ST);
            if (GetMod(*ModRMBytePtr) != 3) {
                instr.opcode = DISASM_FSTP;
                instr.operand[0] = _disasm_decode_m80real(ModRMBytePtr, prefix, &instr.instr_len);
                instr.operand[1] = _disasm_specific_reg(DISASM_REG_ST);
            } else {
                DisasmInvalidFlop;
            }
        break;
    }
    instr.instr_len += prefix.count;
    return instr;
}

internal Disasm_Instr _disasm_decode_flops_dc(u8* instr_ptr, Disasm_Prefix prefix) {
    Disasm_Instr instr = {0};
    instr.instr = instr_ptr - prefix.count;
    instr.instr_len = 1;
    if (GetMod(*ModRMBytePtr) != 3) {
        instr.num_operands = 2;
        instr.operand[0] = _disasm_specific_reg(DISASM_REG_ST);
        instr.operand[1] = _disasm_decode_m64real(ModRMBytePtr, prefix, &instr.instr_len);
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
    } else {
        instr.num_operands = 2;
        instr.operand[0] = _disasm_decode_sti(ModRMBytePtr, prefix, &instr.instr_len);
        instr.operand[1] = _disasm_specific_reg(DISASM_REG_ST);
        switch (GetReg(*ModRMBytePtr)) {
            case 0: instr.opcode = DISASM_FADD; break;
            case 1:  instr.opcode = DISASM_FMUL; break;
            case 4:  instr.opcode = DISASM_FSUBR; break;
            case 5:  instr.opcode = DISASM_FSUB; break;
            case 6:  instr.opcode = DISASM_FDIVR; break;
            case 7:  instr.opcode = DISASM_FDIV; break;
            default:
                DisasmInvalidFlop;
        }
    }
    instr.instr_len += prefix.count;
    return instr;
}

internal Disasm_Instr _disasm_decode_flops_dd(u8* instr_ptr, Disasm_Prefix prefix) {
    Disasm_Instr instr = {0};
    instr.instr = instr_ptr - prefix.count;
    instr.instr_len = 1;
    switch (GetReg(*ModRMBytePtr)) {
        case 0:
            if (GetMod(*ModRMBytePtr) != 3) {
                instr.num_operands = 2;
                instr.opcode = DISASM_FLD;
                instr.operand[0] = _disasm_specific_reg(DISASM_REG_ST);
                instr.operand[1] = _disasm_decode_m64real(ModRMBytePtr, prefix, &instr.instr_len);
            } else {
                instr.num_operands = 1;
                instr.opcode = DISASM_FFREE;
                instr.operand[0] = _disasm_decode_sti(ModRMBytePtr, prefix, &instr.instr_len);
            }
        break;
        case 1:
            if (GetMod(*ModRMBytePtr) != 3) {
                instr.num_operands = 2;
                instr.opcode = DISASM_FLD;
                instr.operand[0] = _disasm_decode_m64int(ModRMBytePtr, prefix, &instr.instr_len);
                instr.operand[1] = _disasm_specific_reg(DISASM_REG_ST);
            } else {
                DisasmInvalidFlop;
            }
        break;
        case 2:
            instr.num_operands = 2;
            if (GetMod(*ModRMBytePtr) != 3) {
                instr.opcode = DISASM_FST;
                instr.operand[0] = _disasm_decode_m64real(ModRMBytePtr, prefix, &instr.instr_len);
                instr.operand[1] = _disasm_specific_reg(DISASM_REG_ST);
            } else {
                instr.opcode = DISASM_FST;
                instr.operand[0] = _disasm_specific_reg(DISASM_REG_ST);
                instr.operand[1] = _disasm_decode_sti(ModRMBytePtr, prefix, &instr.instr_len);
            }
        break;
        case 3:
            instr.num_operands = 2;
            if (GetMod(*ModRMBytePtr) != 3) {
                instr.opcode = DISASM_FSTP;
                instr.operand[0] = _disasm_decode_m64real(ModRMBytePtr, prefix, &instr.instr_len);
                instr.operand[1] = _disasm_specific_reg(DISASM_REG_ST);
            } else {
                instr.opcode = DISASM_FSTP;
                instr.operand[0] = _disasm_specific_reg(DISASM_REG_ST);
                instr.operand[1] = _disasm_decode_sti(ModRMBytePtr, prefix, &instr.instr_len);
            }
        break;
        case 4:
            if (GetMod(*ModRMBytePtr) != 3) {
                instr.num_operands = 2;
                instr.opcode = DISASM_FRSTOR;
                instr.operand[0] = _disasm_decode_m64real(ModRMBytePtr, prefix, &instr.instr_len);
                instr.operand[1] = _disasm_specific_reg(DISASM_REG_ST);
            } else {
                instr.num_operands = 2;
                instr.opcode = DISASM_FUCOM;
                instr.operand[0] = _disasm_specific_reg(DISASM_REG_ST);
                instr.operand[1] = _disasm_decode_sti(ModRMBytePtr, prefix, &instr.instr_len);
            }
        break;
        case 5:
            if (GetMod(*ModRMBytePtr) != 3) {
                instr.num_operands = 3;
                instr.opcode = DISASM_FUCOMP;
                instr.operand[0] = _disasm_specific_reg(DISASM_REG_ST);
                instr.operand[1] = _disasm_decode_sti(ModRMBytePtr, prefix, &instr.instr_len);
            } else {
                DisasmInvalidFlop;
            }
        break;
        case 6:
            if (GetMod(*ModRMBytePtr) != 3) {
                instr.num_operands = 1;
                instr.opcode = DISASM_FNSAVE;
                instr.operand[0] = _disasm_decode_m94_108(ModRMBytePtr, prefix, &instr.instr_len);
            } else {
                DisasmInvalidFlop;
            }
        break;
    }
    instr.instr_len += prefix.count;
    return instr;
}
