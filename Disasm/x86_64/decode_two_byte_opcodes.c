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
            instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
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
        case 0x11:
            instr.instr_len += 1;
            instr.num_operands = 2;
            if (prefix.repeat) {
                instr.opcode = DISASM_MOVSS;
                instr.operand[0] = _disasm_decode_xmm_m32(ModRMBytePtr, prefix, &instr.instr_len);
            } else if (prefix.repeat_nz) {
                instr.opcode = DISASM_MOVSD;
                instr.operand[0] = _disasm_decode_xmm_m64(ModRMBytePtr, prefix, &instr.instr_len);
            } else if (prefix.op_override) {
                instr.opcode = DISASM_MOVUPD;
                instr.operand[0] = _disasm_decode_xmm_m128(ModRMBytePtr, prefix, &instr.instr_len);
            } else {
                instr.opcode = DISASM_MOVUPS;
                instr.operand[0] = _disasm_decode_xmm_m128(ModRMBytePtr, prefix, &instr.instr_len);
            }
            instr.operand[1] = _disasm_decode_xmm(ModRMBytePtr, prefix);
            instr.instr_len += prefix.count;
            return instr;
        case 0x12:
            instr.instr_len += 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
            if (prefix.repeat) {
                instr.opcode = DISASM_MOVSLDUP;
                instr.operand[1] = _disasm_decode_xmm_m64(ModRMBytePtr, prefix, &instr.instr_len);
            } else if (prefix.repeat_nz) {
                instr.opcode = DISASM_MOVDDUP;
                instr.operand[1] = _disasm_decode_xmm_m64(ModRMBytePtr, prefix, &instr.instr_len);
            } else if (prefix.op_override) {
                instr.opcode = DISASM_MOVLPD;
                instr.operand[1] = _disasm_decode_m64(ModRMBytePtr, prefix, &instr.instr_len);
            } else {
                instr.opcode = (GetMod(*ModRMBytePtr) == 3) ? DISASM_MOVHLPS : DISASM_MOVLPS;
                instr.operand[1] = _disasm_decode_xmm_m64(ModRMBytePtr, prefix, &instr.instr_len);
            }
            instr.instr_len += prefix.count;
            return instr;
        case 0x13:
            instr.instr_len += 1;
            instr.num_operands = 2;
            instr.opcode = (prefix.op_override) ? DISASM_MOVLPD : DISASM_MOVLPS;
            instr.operand[0] = _disasm_decode_m64(ModRMBytePtr, prefix, &instr.instr_len);
            instr.operand[1] = _disasm_decode_xmm(ModRMBytePtr, prefix);
            instr.instr_len += prefix.count;
            return instr;
        case 0x14:
            instr.instr_len += 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
            if (prefix.op_override) {
                instr.opcode = DISASM_UNPCKLPD;
                instr.operand[1] = _disasm_decode_xmm_m64(ModRMBytePtr, prefix, &instr.instr_len);
            } else {
                instr.opcode = DISASM_UNPCKLPS;
                instr.operand[1] = _disasm_decode_xmm_m128(ModRMBytePtr, prefix, &instr.instr_len);
            }
            instr.instr_len += prefix.count;
            return instr;
        case 0x15:
            instr.instr_len += 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
            if (prefix.op_override) {
                instr.opcode = DISASM_UNPCKHPD;
                instr.operand[1] = _disasm_decode_xmm_m64(ModRMBytePtr, prefix, &instr.instr_len);
            } else {
                instr.opcode = DISASM_UNPCKHPS;
                instr.operand[1] = _disasm_decode_xmm_m128(ModRMBytePtr, prefix, &instr.instr_len);
            }
            instr.instr_len += prefix.count;
            return instr;
        case 0x16:
            instr.instr_len += 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
            if (prefix.repeat) {
                instr.opcode = DISASM_MOVSHDUP;
                instr.operand[1] = _disasm_decode_xmm_m64(ModRMBytePtr, prefix, &instr.instr_len);
            } else if (prefix.op_override) {
                instr.opcode = DISASM_MOVHPD;
                instr.operand[1] = _disasm_decode_m64(ModRMBytePtr, prefix, &instr.instr_len);
            } else {
                instr.opcode = (GetMod(*ModRMBytePtr) == 3) ? DISASM_MOVLHPS : DISASM_MOVHPS;
                instr.operand[1] = _disasm_decode_xmm_m64(ModRMBytePtr, prefix, &instr.instr_len);
            }
            instr.instr_len += prefix.count;
            return instr;
        case 0x17:
            instr.instr_len += 1;
            instr.num_operands = 2;
            instr.opcode = (prefix.op_override) ? DISASM_MOVHPD : DISASM_MOVHPS;
            instr.operand[0] = _disasm_decode_m64(ModRMBytePtr, prefix, &instr.instr_len);
            instr.operand[1] = _disasm_decode_xmm(ModRMBytePtr, prefix);
            instr.instr_len += prefix.count;
            return instr;
        case 0x18:
            if (GetMod(*ModRMBytePtr) == 3) {
                instr.opcode = DISASM_NOP;
                instr.instr_len += 1;
                instr.num_operands = 1;
                instr.operand[0] = _disasm_decode_rm16_32(ModRMBytePtr, prefix, &instr.instr_len);
                instr.instr_len += prefix.count;
                return instr;
            }
            switch (GetReg(*ModRMBytePtr)) {
                case 0:
                    instr.opcode = DISASM_PREFETCHNTA;
                    instr.instr_len += 1;
                    instr.num_operands = 1;
                    instr.operand[0] = _disasm_decode_m8(ModRMBytePtr, prefix, &instr.instr_len);
                    instr.instr_len += prefix.count;
                    return instr;
                case 1:
                    instr.opcode = DISASM_PREFETCHT0;
                    instr.instr_len += 1;
                    instr.num_operands = 1;
                    instr.operand[0] = _disasm_decode_m8(ModRMBytePtr, prefix, &instr.instr_len);
                    instr.instr_len += prefix.count;
                    return instr;
                case 2:
                    instr.opcode = DISASM_PREFETCHT1;
                    instr.instr_len += 1;
                    instr.num_operands = 1;
                    instr.operand[0] = _disasm_decode_m8(ModRMBytePtr, prefix, &instr.instr_len);
                    instr.instr_len += prefix.count;
                    return instr;
                case 3:
                    instr.opcode = DISASM_PREFETCHT2;
                    instr.instr_len += 1;
                    instr.num_operands = 1;
                    instr.operand[0] = _disasm_decode_m8(ModRMBytePtr, prefix, &instr.instr_len);
                    instr.instr_len += prefix.count;
                    return instr;
                default:
                    instr.opcode = DISASM_NOP;
                    instr.instr_len += 1;
                    instr.num_operands = 1;
                    instr.operand[0] = _disasm_decode_rm16_32(ModRMBytePtr, prefix, &instr.instr_len);
                    instr.instr_len += prefix.count;
                    return instr;
            }
        case 0x19: case 0x1a: case 0x1b: case 0x1c:
        case 0x1d: case 0x1e: case 0x1f:
            instr.opcode = DISASM_NOP;
            instr.instr_len += 1;
            instr.num_operands = 1;
            instr.operand[0] = _disasm_decode_rm16_32(ModRMBytePtr, prefix, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;

        case 0x40:
            instr.opcode = DISASM_CMOVO;
            instr.instr_len += 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_r16_32_64(ModRMBytePtr, prefix);
            instr.operand[1] = _disasm_decode_rm16_32_64(ModRMBytePtr, prefix, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x41:
            instr.opcode = DISASM_CMOVNO;
            instr.instr_len += 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_r16_32_64(ModRMBytePtr, prefix);
            instr.operand[1] = _disasm_decode_rm16_32_64(ModRMBytePtr, prefix, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x42:
            instr.opcode = DISASM_CMOVB;
            instr.instr_len += 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_r16_32_64(ModRMBytePtr, prefix);
            instr.operand[1] = _disasm_decode_rm16_32_64(ModRMBytePtr, prefix, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x43:
            instr.opcode = DISASM_CMOVNB;
            instr.instr_len += 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_r16_32_64(ModRMBytePtr, prefix);
            instr.operand[1] = _disasm_decode_rm16_32_64(ModRMBytePtr, prefix, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x44:
            instr.opcode = DISASM_CMOVZ;
            instr.instr_len += 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_r16_32_64(ModRMBytePtr, prefix);
            instr.operand[1] = _disasm_decode_rm16_32_64(ModRMBytePtr, prefix, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x45:
            instr.opcode = DISASM_CMOVNZ;
            instr.instr_len += 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_r16_32_64(ModRMBytePtr, prefix);
            instr.operand[1] = _disasm_decode_rm16_32_64(ModRMBytePtr, prefix, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x46:
            instr.opcode = DISASM_CMOVBE;
            instr.instr_len += 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_r16_32_64(ModRMBytePtr, prefix);
            instr.operand[1] = _disasm_decode_rm16_32_64(ModRMBytePtr, prefix, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x47:
            instr.opcode = DISASM_CMOVNBE;
            instr.instr_len += 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_r16_32_64(ModRMBytePtr, prefix);
            instr.operand[1] = _disasm_decode_rm16_32_64(ModRMBytePtr, prefix, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x48:
            instr.opcode = DISASM_CMOVS;
            instr.instr_len += 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_r16_32_64(ModRMBytePtr, prefix);
            instr.operand[1] = _disasm_decode_rm16_32_64(ModRMBytePtr, prefix, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x49:
            instr.opcode = DISASM_CMOVNS;
            instr.instr_len += 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_r16_32_64(ModRMBytePtr, prefix);
            instr.operand[1] = _disasm_decode_rm16_32_64(ModRMBytePtr, prefix, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x4a:
            instr.opcode = DISASM_CMOVP;
            instr.instr_len += 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_r16_32_64(ModRMBytePtr, prefix);
            instr.operand[1] = _disasm_decode_rm16_32_64(ModRMBytePtr, prefix, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x4b:
            instr.opcode = DISASM_CMOVNP;
            instr.instr_len += 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_r16_32_64(ModRMBytePtr, prefix);
            instr.operand[1] = _disasm_decode_rm16_32_64(ModRMBytePtr, prefix, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x4c:
            instr.opcode = DISASM_CMOVL;
            instr.instr_len += 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_r16_32_64(ModRMBytePtr, prefix);
            instr.operand[1] = _disasm_decode_rm16_32_64(ModRMBytePtr, prefix, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x4d:
            instr.opcode = DISASM_CMOVNL;
            instr.instr_len += 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_r16_32_64(ModRMBytePtr, prefix);
            instr.operand[1] = _disasm_decode_rm16_32_64(ModRMBytePtr, prefix, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x4e:
            instr.opcode = DISASM_CMOVLE;
            instr.instr_len += 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_r16_32_64(ModRMBytePtr, prefix);
            instr.operand[1] = _disasm_decode_rm16_32_64(ModRMBytePtr, prefix, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x4f:
            instr.opcode = DISASM_CMOVNLE;
            instr.instr_len += 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_r16_32_64(ModRMBytePtr, prefix);
            instr.operand[1] = _disasm_decode_rm16_32_64(ModRMBytePtr, prefix, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;


        case 0x90:
            instr.opcode = DISASM_SETO;
            instr.instr_len += 1;
            instr.num_operands = 1;
            instr.operand[0] = _disasm_decode_rm8(ModRMBytePtr, prefix, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x91:
            instr.opcode = DISASM_SETNO;
            instr.instr_len += 1;
            instr.num_operands = 1;
            instr.operand[0] = _disasm_decode_rm8(ModRMBytePtr, prefix, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x92:
            instr.opcode = DISASM_SETB;
            instr.instr_len += 1;
            instr.num_operands = 1;
            instr.operand[0] = _disasm_decode_rm8(ModRMBytePtr, prefix, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x93:
            instr.opcode = DISASM_SETNB;
            instr.instr_len += 1;
            instr.num_operands = 1;
            instr.operand[0] = _disasm_decode_rm8(ModRMBytePtr, prefix, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x94:
            instr.opcode = DISASM_SETZ;
            instr.instr_len += 1;
            instr.num_operands = 1;
            instr.operand[0] = _disasm_decode_rm8(ModRMBytePtr, prefix, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x95:
            instr.opcode = DISASM_SETNZ;
            instr.instr_len += 1;
            instr.num_operands = 1;
            instr.operand[0] = _disasm_decode_rm8(ModRMBytePtr, prefix, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x96:
            instr.opcode = DISASM_SETBE;
            instr.instr_len += 1;
            instr.num_operands = 1;
            instr.operand[0] = _disasm_decode_rm8(ModRMBytePtr, prefix, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x97:
            instr.opcode = DISASM_SETNBE;
            instr.instr_len += 1;
            instr.num_operands = 1;
            instr.operand[0] = _disasm_decode_rm8(ModRMBytePtr, prefix, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x98:
            instr.opcode = DISASM_SETS;
            instr.instr_len += 1;
            instr.num_operands = 1;
            instr.operand[0] = _disasm_decode_rm8(ModRMBytePtr, prefix, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x99:
            instr.opcode = DISASM_SETNS;
            instr.instr_len += 1;
            instr.num_operands = 1;
            instr.operand[0] = _disasm_decode_rm8(ModRMBytePtr, prefix, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x9a:
            instr.opcode = DISASM_SETP;
            instr.instr_len += 1;
            instr.num_operands = 1;
            instr.operand[0] = _disasm_decode_rm8(ModRMBytePtr, prefix, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x9b:
            instr.opcode = DISASM_SETNP;
            instr.instr_len += 1;
            instr.num_operands = 1;
            instr.operand[0] = _disasm_decode_rm8(ModRMBytePtr, prefix, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x9c:
            instr.opcode = DISASM_SETL;
            instr.instr_len += 1;
            instr.num_operands = 1;
            instr.operand[0] = _disasm_decode_rm8(ModRMBytePtr, prefix, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x9d:
            instr.opcode = DISASM_SETNL;
            instr.instr_len += 1;
            instr.num_operands = 1;
            instr.operand[0] = _disasm_decode_rm8(ModRMBytePtr, prefix, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x9e:
            instr.opcode = DISASM_SETLE;
            instr.instr_len += 1;
            instr.num_operands = 1;
            instr.operand[0] = _disasm_decode_rm8(ModRMBytePtr, prefix, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x9f:
            instr.opcode = DISASM_SETNLE;
            instr.instr_len += 1;
            instr.num_operands = 1;
            instr.operand[0] = _disasm_decode_rm8(ModRMBytePtr, prefix, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
    }

    DisasmInvalidTwoByte;
}
