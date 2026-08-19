#define DisasmInvalidTwoByte DisasmInvalid; instr.instr_len += 1; return instr;

internal Disasm_Instr _disasm_decode_two_byte_opcodes(Disasm_Prefix prefix, u8* instr_ptr) {
    Disasm_Instr instr = {0};
    instr.instr = instr_ptr - prefix.count;
    instr.instr_len += 1;
    instr_ptr += 1;

    switch (*instr_ptr) {
        case 0x00:
            switch (GetReg(*ModRMBytePtr)) {
                case 0:
                    instr.opcode = DISASM_SLDT;
                    instr.instr_len += 1;
                    instr.num_operands = 1;
                    instr.operand[0] = _disasm_decode_m16_r16_32_64(ModRMBytePtr, prefix, &instr.instr_len);
                    instr.instr_len += prefix.count;
                    return instr;
                case 1:
                    instr.opcode = DISASM_STR;
                    instr.instr_len += 1;
                    instr.num_operands = 1;
                    instr.operand[0] = _disasm_decode_m16_r16_32_64(ModRMBytePtr, prefix, &instr.instr_len);
                    instr.instr_len += prefix.count;
                    return instr;
                case 2:
                    instr.opcode = DISASM_LLDT;
                    instr.instr_len += 1;
                    instr.num_operands = 1;
                    instr.operand[0] = _disasm_decode_rm16(ModRMBytePtr, prefix, &instr.instr_len);
                    instr.instr_len += prefix.count;
                    return instr;
                case 3:
                    instr.opcode = DISASM_LTR;
                    instr.instr_len += 1;
                    instr.num_operands = 1;
                    instr.operand[0] = _disasm_decode_rm16(ModRMBytePtr, prefix, &instr.instr_len);
                    instr.instr_len += prefix.count;
                    return instr;
                case 4:
                    instr.opcode = DISASM_VERR;
                    instr.instr_len += 1;
                    instr.num_operands = 1;
                    instr.operand[0] = _disasm_decode_rm16(ModRMBytePtr, prefix, &instr.instr_len);
                    instr.instr_len += prefix.count;
                    return instr;
                case 5:
                    instr.opcode = DISASM_VERW;
                    instr.instr_len += 1;
                    instr.num_operands = 1;
                    instr.operand[0] = _disasm_decode_rm16(ModRMBytePtr, prefix, &instr.instr_len);
                    instr.instr_len += prefix.count;
                    return instr;
                default:
                    DisasmInvalidTwoByte;
            }
        case 0x01:
            switch (GetReg(*ModRMBytePtr)) {
                case 0:
                    if (GetMod(*ModRMBytePtr) == 3) {
                        switch (GetRM(*ModRMBytePtr)) {
                            case 0: instr.opcode = DISASM_ENCLV; break;
                            case 1: instr.opcode = DISASM_VMCALL; break;
                            case 2: instr.opcode = DISASM_VMLAUNCH; break;
                            case 3: instr.opcode = DISASM_VMRESUME; break;
                            case 4: instr.opcode = DISASM_VMXOFF; break;
                            case 5: instr.opcode = DISASM_PCONFIG; break;
                            default:
                                DisasmInvalidTwoByte;
                        }

                        instr.instr_len += 2 + prefix.count;
                        instr.num_operands = 0;
                        return instr;
                    }

                    instr.opcode = DISASM_SGDT;
                    instr.instr_len += 1;
                    instr.num_operands = 1;
                    instr.operand[0] = _disasm_decode_m(ModRMBytePtr, prefix, &instr.instr_len, 10);
                    instr.instr_len += prefix.count;
                    return instr;
                case 1:
                    if (GetMod(*ModRMBytePtr) == 3) {
                        switch (GetRM(*ModRMBytePtr)) {
                            case 0:
                                instr.opcode = DISASM_MONITOR;
                                instr.instr_len += 1;
                                instr.num_operands = 3;
                                instr.operand[0] = _disasm_decode_m8(ModRMBytePtr, prefix, &instr.instr_len);
                                instr.operand[1] = _disasm_specific_reg(DISASM_REG_ECX);
                                instr.operand[2] = _disasm_specific_reg(DISASM_REG_EDX);
                                instr.instr_len += prefix.count;
                                return instr;
                            case 1:
                                instr.opcode = DISASM_MWAIT;
                                instr.instr_len += 2;
                                instr.num_operands = 2;
                                instr.operand[0] = _disasm_specific_reg(DISASM_REG_EAX);
                                instr.operand[1] = _disasm_specific_reg(DISASM_REG_ECX);
                                instr.instr_len += prefix.count;
                                return instr;
                            default:
                                DisasmInvalidTwoByte;
                        }
                    }

                    instr.opcode = DISASM_SIDT;
                    instr.instr_len += 1;
                    instr.num_operands = 1;
                    instr.operand[0] = _disasm_decode_m(ModRMBytePtr, prefix, &instr.instr_len, 10);
                    instr.instr_len += prefix.count;
                    return instr;
                case 2:
                    if (GetMod(*ModRMBytePtr) == 3) {
                        switch (GetRM(*ModRMBytePtr)) {
                            case 0: instr.opcode = DISASM_XGETBV; break;
                            case 1: instr.opcode = DISASM_XSETBV; break;
                            case 4: instr.opcode = DISASM_VMFUNC; break;
                            case 5: instr.opcode = DISASM_XEND; break;
                            case 6: instr.opcode = DISASM_XTEST; break;
                            case 7: instr.opcode = DISASM_ENCLU; break;
                            default:
                                DisasmInvalidTwoByte;
                        }

                        instr.instr_len += 2 + prefix.count;
                        instr.num_operands = 0;
                        return instr;
                    }
                    instr.opcode = DISASM_LGDT;
                    instr.instr_len += 1;
                    instr.num_operands = 1;
                    instr.operand[0] = _disasm_decode_m(ModRMBytePtr, prefix, &instr.instr_len, 10);
                    instr.instr_len += prefix.count;
                    return instr;
                case 3:
                    if (GetMod(*ModRMBytePtr) == 3) {
                        switch (GetRM(*ModRMBytePtr)) {
                            case 0: instr.opcode = DISASM_VMRUN; break;
                            case 1: instr.opcode = DISASM_VMMCALL; break;
                            case 2: instr.opcode = DISASM_VMLOAD; break;
                            case 3: instr.opcode = DISASM_VMSAVE; break;
                            case 4: instr.opcode = DISASM_STGI; break;
                            case 5: instr.opcode = DISASM_CLGI; break;
                            case 6: instr.opcode = DISASM_SKINIT; break;
                            case 7: instr.opcode = DISASM_INVLPGA; break;
                            default:
                                DisasmInvalidTwoByte;
                        }

                        instr.instr_len += 2 + prefix.count;
                        instr.num_operands = 0;
                        return instr;
                    }
                    instr.opcode = DISASM_LIDT;
                    instr.instr_len += 1;
                    instr.num_operands = 1;
                    instr.operand[0] = _disasm_decode_m(ModRMBytePtr, prefix, &instr.instr_len, 10);
                    instr.instr_len += prefix.count;
                    return instr;
                case 4:
                    instr.opcode = DISASM_SMSW;
                    instr.instr_len += 1;
                    instr.num_operands = 1;
                    instr.operand[0] = _disasm_decode_m16_r16_32_64(ModRMBytePtr, prefix, &instr.instr_len);
                    instr.instr_len += prefix.count;
                    return instr;
                case 6:
                    instr.opcode = DISASM_LMSW;
                    instr.instr_len += 1;
                    instr.num_operands = 1;
                    instr.operand[0] = _disasm_decode_rm16(ModRMBytePtr, prefix, &instr.instr_len);
                    instr.instr_len += prefix.count;
                    return instr;
                case 7:
                    if (GetMod(*ModRMBytePtr) == 3) {
                        switch (GetRM(*ModRMBytePtr)) {
                            case 0: instr.opcode = DISASM_SWAPGS; break;
                            case 1: instr.opcode = DISASM_RDTSCP; break;
                            default:
                                DisasmInvalidTwoByte;
                        }

                        instr.instr_len += 2 + prefix.count;
                        instr.num_operands = 0;
                        return instr;
                    }
                    instr.opcode = DISASM_INVLPG;
                    instr.instr_len += 1;
                    instr.num_operands = 1;
                    instr.operand[0] = _disasm_decode_m(ModRMBytePtr, prefix, &instr.instr_len, 10);
                    instr.instr_len += prefix.count;
                    return instr;
                default:
                    DisasmInvalidTwoByte;
            }
        case 0x02:
            instr.opcode = DISASM_LAR;
            instr.instr_len += 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_r16_32_64(ModRMBytePtr, prefix);
            instr.operand[1] = _disasm_decode_m16_r16_32_64(ModRMBytePtr, prefix, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x03:
            instr.opcode = DISASM_LSL;
            instr.instr_len += 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_r16_32_64(ModRMBytePtr, prefix);
            instr.operand[1] = _disasm_decode_m16_r16_32_64(ModRMBytePtr, prefix, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x05:
            instr.opcode = DISASM_SYSCALL;
            instr.instr_len += 1 + prefix.count;
            instr.num_operands = 0;
            return instr;
        case 0x06:
            instr.opcode = DISASM_CLTS;
            instr.instr_len += 1 + prefix.count;
            instr.num_operands = 0;
            return instr;
        case 0x07:
            instr.opcode = DISASM_SYSRET;
            instr.instr_len += 1 + prefix.count;
            instr.num_operands = 0;
            return instr;
        case 0x08:
            instr.opcode = DISASM_INVD;
            instr.instr_len += 1 + prefix.count;
            instr.num_operands = 0;
            return instr;
        case 0x09:
            instr.opcode = DISASM_WBINVD;
            instr.instr_len += 1 + prefix.count;
            instr.num_operands = 0;
            return instr;
        case 0x0b:
            instr.opcode = DISASM_UD2;
            instr.instr_len += 1 + prefix.count;
            instr.num_operands = 0;
            return instr;
        case 0x0D:
            if (GetMod(*ModRMBytePtr) == 3) {
                instr.opcode = DISASM_NOP;
                instr.instr_len += 2 + prefix.count;
                instr.num_operands = 0;
            } else {
                switch (GetReg(*ModRMBytePtr)) {
                    case 0: instr.opcode = DISASM_PREFETCH; break;
                    case 1: instr.opcode = DISASM_PREFETCHW; break;
                    case 2: instr.opcode = DISASM_PREFETCHWT1; break;
                    default:
                        instr.opcode = DISASM_NOP;
                        break;
                }
                instr.instr_len += 1;
                instr.num_operands = 1;
                instr.operand[0] = _disasm_decode_m16_32(ModRMBytePtr, prefix, &instr.instr_len);
                instr.instr_len += prefix.count;
            }
            return instr;
        case 0x10:
            instr.instr_len += 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix, &instr.instr_len);
            if (prefix.repeat) {
                instr.opcode = DISASM_MOVSS;
                instr.operand[1] = _disasm_decode_xmm_m32(ModRMBytePtr, prefix, &instr.instr_len);
            } else if (prefix.repeat_nz) {
                instr.opcode = DISASM_MOVSD;
                instr.operand[1] = _disasm_decode_xmm_m64(ModRMBytePtr, prefix, &instr.instr_len);
            } else if (prefix.op_override) {
                instr.opcode = DISASM_MOVUPD;
                instr.operand[1] = _disasm_decode_xmm_m128(ModRMBytePtr, prefix, &instr.instr_len);
            } else {
                instr.opcode = DISASM_MOVUPS;
                instr.operand[1] = _disasm_decode_xmm_m128(ModRMBytePtr, prefix, &instr.instr_len);
            }
            instr.instr_len += prefix.count;
            return instr;
    }

    DisasmInvalidTwoByte;
}
