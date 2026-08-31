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
        case 0x20:
            instr.opcode = DISASM_MOV;
            instr.instr_len += 2;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_r64(ModRMBytePtr, prefix);
            instr.operand[1] = _disasm_decode_crn(ModRMBytePtr, prefix);
            instr.instr_len += prefix.count;
            return instr;
        case 0x21:
            instr.opcode = DISASM_MOV;
            instr.instr_len += 2;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_r64(ModRMBytePtr, prefix);
            instr.operand[1] = _disasm_decode_drn(ModRMBytePtr, prefix);
            instr.instr_len += prefix.count;
            return instr;
        case 0x22:
            instr.opcode = DISASM_MOV;
            instr.instr_len += 2;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_crn(ModRMBytePtr, prefix);
            instr.operand[1] = _disasm_decode_r64(ModRMBytePtr, prefix);
            instr.instr_len += prefix.count;
            return instr;
        case 0x23:
            instr.opcode = DISASM_MOV;
            instr.instr_len += 2;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_drn(ModRMBytePtr, prefix);
            instr.operand[1] = _disasm_decode_r64(ModRMBytePtr, prefix);
            instr.instr_len += prefix.count;
            return instr;
        case 0x28:
            instr.instr_len += 1;
            instr.num_operands = 2;
            instr.opcode = (prefix.op_override) ? DISASM_MOVAPD : DISASM_MOVAPS;
            instr.operand[0] = _disasm_decode_xmm_m64(ModRMBytePtr, prefix, &instr.instr_len);
            instr.operand[1] = _disasm_decode_xmm(ModRMBytePtr, prefix);
            instr.instr_len += prefix.count;
            return instr;
        case 0x29:
            instr.instr_len += 1;
            instr.num_operands = 2;
            instr.opcode = (prefix.op_override) ? DISASM_MOVAPD : DISASM_MOVAPS;
            instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
            instr.operand[1] = _disasm_decode_xmm_m64(ModRMBytePtr, prefix, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x2a:
            instr.instr_len += 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
            if (prefix.repeat) { // 0xf3
                instr.opcode = DISASM_CVTSI2SS;
                instr.operand[1] = _disasm_decode_rm32_64(ModRMBytePtr, prefix, &instr.instr_len);
            } else if (prefix.repeat_nz) { // 0xf2
                instr.opcode = DISASM_CVTSI2SD;
                instr.operand[1] = _disasm_decode_rm32_64(ModRMBytePtr, prefix, &instr.instr_len);
            } else if (prefix.op_override) { // 0x66
                instr.opcode = DISASM_CVTPI2PD;
                instr.operand[1] = _disasm_decode_mm_m64(ModRMBytePtr, prefix, &instr.instr_len);
            } else {
                instr.opcode = DISASM_CVTPI2PS;
                instr.operand[1] = _disasm_decode_mm_m64(ModRMBytePtr, prefix, &instr.instr_len);
            }
            instr.instr_len += prefix.count;
            return instr;
        case 0x2b:
            instr.instr_len += 1;
            instr.num_operands = 2;
            instr.opcode = (prefix.op_override) ? DISASM_MOVNTPD : DISASM_MOVNTPS;
            instr.operand[0] = _disasm_decode_m128(ModRMBytePtr, prefix, &instr.instr_len);
            instr.operand[1] = _disasm_decode_xmm(ModRMBytePtr, prefix);
            instr.instr_len += prefix.count;
            return instr;
        case 0x2c:
            instr.instr_len += 1;
            instr.num_operands = 2;
            if (prefix.repeat) { // 0xf3
                instr.opcode = DISASM_CVTTSS2SI;
                instr.operand[0] = _disasm_decode_r32_64(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m32(ModRMBytePtr, prefix, &instr.instr_len);
            } else if (prefix.repeat_nz) { // 0xf2
                instr.opcode = DISASM_CVTSI2SD;
                instr.operand[0] = _disasm_decode_r32_64(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m32(ModRMBytePtr, prefix, &instr.instr_len);
            } else if (prefix.op_override) { // 0x66
                instr.opcode = DISASM_CVTTPD2PI;
                instr.operand[0] = _disasm_decode_mm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m64(ModRMBytePtr, prefix, &instr.instr_len);
            } else {
                instr.opcode = DISASM_CVTTPS2PI;
                instr.operand[0] = _disasm_decode_mm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m64(ModRMBytePtr, prefix, &instr.instr_len);
            }
            instr.instr_len += prefix.count;
            return instr;
        case 0x2d:
            instr.instr_len += 1;
            instr.num_operands = 2;
            if (prefix.repeat) { // 0xf3
                instr.opcode = DISASM_CVTSS2SI;
                instr.operand[0] = _disasm_decode_r32_64(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m32(ModRMBytePtr, prefix, &instr.instr_len);
            } else if (prefix.repeat_nz) { // 0xf2
                instr.opcode = DISASM_CVTSD2SI;
                instr.operand[0] = _disasm_decode_r32_64(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m32(ModRMBytePtr, prefix, &instr.instr_len);
            } else if (prefix.op_override) { // 0x66
                instr.opcode = DISASM_CVTPD2PI;
                instr.operand[0] = _disasm_decode_mm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m64(ModRMBytePtr, prefix, &instr.instr_len);
            } else {
                instr.opcode = DISASM_CVTPS2PI;
                instr.operand[0] = _disasm_decode_mm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m64(ModRMBytePtr, prefix, &instr.instr_len);
            }
            instr.instr_len += prefix.count;
            return instr;
        case 0x2e:
            instr.instr_len += 1;
            instr.num_operands = 2;
            if (prefix.op_override) {
                instr.opcode = DISASM_UCOMISD;
                instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m64(ModRMBytePtr, prefix, &instr.instr_len);
            } else {
                instr.opcode = DISASM_UCOMISS;
                instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m32(ModRMBytePtr, prefix, &instr.instr_len);
            }
            instr.instr_len += prefix.count;
            return instr;
        case 0x2f:
            instr.instr_len += 1;
            instr.num_operands = 2;
            if (prefix.op_override) {
                instr.opcode = DISASM_COMISD;
                instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m64(ModRMBytePtr, prefix, &instr.instr_len);
            } else {
                instr.opcode = DISASM_COMISS;
                instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m32(ModRMBytePtr, prefix, &instr.instr_len);
            }
            instr.instr_len += prefix.count;
            return instr;
        case 0x30:
            instr.instr_len += 1 + prefix.count;
            instr.num_operands = 0;
            instr.opcode = DISASM_WRMSR;
            return instr;
        case 0x31:
            instr.instr_len += 1 + prefix.count;
            instr.num_operands = 0;
            instr.opcode = DISASM_RDTSC;
            return instr;
        case 0x32:
            instr.instr_len += 1 + prefix.count;
            instr.num_operands = 0;
            instr.opcode = DISASM_RDMSR;
            return instr;
        case 0x33:
            instr.instr_len += 1 + prefix.count;
            instr.num_operands = 0;
            instr.opcode = DISASM_RDPMC;
            return instr;
        case 0x34:
            instr.instr_len += 1 + prefix.count;
            instr.num_operands = 0;
            instr.opcode = DISASM_SYSENTER;
            return instr;
        case 0x35:
            instr.instr_len += 1 + prefix.count;
            instr.num_operands = 0;
            instr.opcode = DISASM_SYSEXIT;
            return instr;
        case 0x37:
            instr.instr_len += 1 + prefix.count;
            instr.num_operands = 0;
            instr.opcode = DISASM_GETSEC;
            return instr;
        // TODO: the rest of 0x3... but these don't seem to show up in practice when compiling C99


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
        case 0x50:
            instr.opcode = (prefix.op_override) ?  DISASM_MOVMSKPD : DISASM_MOVMSKPS;
            instr.instr_len += 2 + prefix.count;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_r32_64(ModRMBytePtr, prefix);
            instr.operand[1] = _disasm_decode_xmm(ModRMBytePtr, prefix);
            return instr;
        case 0x51:
            instr.instr_len += 1;
            instr.num_operands = 2;
            if (prefix.repeat) { // 0xf3
                instr.opcode = DISASM_SQRTSS;
                instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m32(ModRMBytePtr, prefix, &instr.instr_len);
            } else if (prefix.repeat_nz) { // 0xf2
                instr.opcode = DISASM_SQRTSD;
                instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m64(ModRMBytePtr, prefix, &instr.instr_len);
            } else if (prefix.op_override) { // 0x66
                instr.opcode = DISASM_SQRTPD;
                instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m128(ModRMBytePtr, prefix, &instr.instr_len);
            } else {
                instr.opcode = DISASM_SQRTPS;
                instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m128(ModRMBytePtr, prefix, &instr.instr_len);
            }
            instr.instr_len += prefix.count;
            return instr;
        case 0x52:
            instr.instr_len += 1;
            instr.num_operands = 2;
            if (prefix.repeat_nz) {
                instr.opcode = DISASM_RSQRTSS;
                instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m32(ModRMBytePtr, prefix, &instr.instr_len);
            } else {
                instr.opcode = DISASM_RSQRTPS;
                instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m128(ModRMBytePtr, prefix, &instr.instr_len);
            }
            instr.instr_len += prefix.count;
            return instr;
        case 0x53:
            instr.instr_len += 1;
            instr.num_operands = 2;
            if (prefix.repeat_nz) {
                instr.opcode = DISASM_RCPSS;
                instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m32(ModRMBytePtr, prefix, &instr.instr_len);
            } else {
                instr.opcode = DISASM_RCPPS;
                instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m128(ModRMBytePtr, prefix, &instr.instr_len);
            }
            instr.instr_len += prefix.count;
            return instr;
        case 0x54:
            instr.opcode = (prefix.op_override) ?  DISASM_ANDPD : DISASM_ANDPS;
            instr.instr_len += 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
            instr.operand[1] = _disasm_decode_xmm_m128(ModRMBytePtr, prefix, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x55:
            instr.opcode = (prefix.op_override) ?  DISASM_ANDNPD : DISASM_ANDNPS;
            instr.instr_len += 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
            instr.operand[1] = _disasm_decode_xmm_m128(ModRMBytePtr, prefix, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x56:
            instr.opcode = (prefix.op_override) ?  DISASM_ORPD : DISASM_ORPS;
            instr.instr_len += 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
            instr.operand[1] = _disasm_decode_xmm_m128(ModRMBytePtr, prefix, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x57:
            instr.opcode = (prefix.op_override) ?  DISASM_XORPD : DISASM_XORPS;
            instr.instr_len += 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
            instr.operand[1] = _disasm_decode_xmm_m128(ModRMBytePtr, prefix, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x58:
            instr.instr_len += 1;
            instr.num_operands = 2;
            if (prefix.repeat) { // 0xf3
                instr.opcode = DISASM_ADDSS;
                instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m32(ModRMBytePtr, prefix, &instr.instr_len);
            } else if (prefix.repeat_nz) { // 0xf2
                instr.opcode = DISASM_ADDSD;
                instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m64(ModRMBytePtr, prefix, &instr.instr_len);
            } else if (prefix.op_override) { // 0x66
                instr.opcode = DISASM_ADDPD;
                instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m128(ModRMBytePtr, prefix, &instr.instr_len);
            } else {
                instr.opcode = DISASM_ADDPS;
                instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m128(ModRMBytePtr, prefix, &instr.instr_len);
            }
            instr.instr_len += prefix.count;
            return instr;
        case 0x59:
            instr.instr_len += 1;
            instr.num_operands = 2;
            if (prefix.repeat) { // 0xf3
                instr.opcode = DISASM_MULSS;
                instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m32(ModRMBytePtr, prefix, &instr.instr_len);
            } else if (prefix.repeat_nz) { // 0xf2
                instr.opcode = DISASM_MULSD;
                instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m64(ModRMBytePtr, prefix, &instr.instr_len);
            } else if (prefix.op_override) { // 0x66
                instr.opcode = DISASM_MULPD;
                instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m128(ModRMBytePtr, prefix, &instr.instr_len);
            } else {
                instr.opcode = DISASM_MULPS;
                instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m128(ModRMBytePtr, prefix, &instr.instr_len);
            }
            instr.instr_len += prefix.count;
            return instr;
        case 0x5a:
            instr.instr_len += 1;
            instr.num_operands = 2;
            if (prefix.repeat) { // 0xf3
                instr.opcode = DISASM_CVTSS2SD;
                instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m32(ModRMBytePtr, prefix, &instr.instr_len);
            } else if (prefix.repeat_nz) { // 0xf2
                instr.opcode = DISASM_CVTSD2SS;
                instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m64(ModRMBytePtr, prefix, &instr.instr_len);
            } else if (prefix.op_override) { // 0x66
                instr.opcode = DISASM_CVTPD2PS;
                instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m128(ModRMBytePtr, prefix, &instr.instr_len);
            } else {
                instr.opcode = DISASM_CVTPS2PD;
                instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m128(ModRMBytePtr, prefix, &instr.instr_len);
            }
            instr.instr_len += prefix.count;
            return instr;
        case 0x5b:
            instr.instr_len += 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
            instr.operand[1] = _disasm_decode_xmm_m32(ModRMBytePtr, prefix, &instr.instr_len);
            if (prefix.repeat) { // 0xf3
                instr.opcode = DISASM_CVTTPS2DQ;
            } else if (prefix.op_override) { // 0x66
                instr.opcode = DISASM_CVTPS2DQ;
            } else {
                instr.opcode = DISASM_CVTDQ2PS;
            }
            instr.instr_len += prefix.count;
            return instr;
        case 0x5c:
            instr.instr_len += 1;
            instr.num_operands = 2;
            if (prefix.repeat) { // 0xf3
                instr.opcode = DISASM_SUBSS;
                instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m32(ModRMBytePtr, prefix, &instr.instr_len);
            } else if (prefix.repeat_nz) { // 0xf2
                instr.opcode = DISASM_SUBSD;
                instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m64(ModRMBytePtr, prefix, &instr.instr_len);
            } else if (prefix.op_override) { // 0x66
                instr.opcode = DISASM_SUBPD;
                instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m128(ModRMBytePtr, prefix, &instr.instr_len);
            } else {
                instr.opcode = DISASM_SUBPS;
                instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m128(ModRMBytePtr, prefix, &instr.instr_len);
            }
            instr.instr_len += prefix.count;
            return instr;
        case 0x5d:
            instr.instr_len += 1;
            instr.num_operands = 2;
            if (prefix.repeat) { // 0xf3
                instr.opcode = DISASM_MINSS;
                instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m32(ModRMBytePtr, prefix, &instr.instr_len);
            } else if (prefix.repeat_nz) { // 0xf2
                instr.opcode = DISASM_MINSD;
                instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m64(ModRMBytePtr, prefix, &instr.instr_len);
            } else if (prefix.op_override) { // 0x66
                instr.opcode = DISASM_MINPD;
                instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m128(ModRMBytePtr, prefix, &instr.instr_len);
            } else {
                instr.opcode = DISASM_MINPS;
                instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m128(ModRMBytePtr, prefix, &instr.instr_len);
            }
            instr.instr_len += prefix.count;
            return instr;
        case 0x5e:
            instr.instr_len += 1;
            instr.num_operands = 2;
            if (prefix.repeat) { // 0xf3
                instr.opcode = DISASM_DIVSS;
                instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m32(ModRMBytePtr, prefix, &instr.instr_len);
            } else if (prefix.repeat_nz) { // 0xf2
                instr.opcode = DISASM_DIVSD;
                instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m64(ModRMBytePtr, prefix, &instr.instr_len);
            } else if (prefix.op_override) { // 0x66
                instr.opcode = DISASM_DIVPD;
                instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m128(ModRMBytePtr, prefix, &instr.instr_len);
            } else {
                instr.opcode = DISASM_DIVPS;
                instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m128(ModRMBytePtr, prefix, &instr.instr_len);
            }
            instr.instr_len += prefix.count;
            return instr;
        case 0x5f:
            instr.instr_len += 1;
            instr.num_operands = 2;
            if (prefix.repeat) { // 0xf3
                instr.opcode = DISASM_MAXSS;
                instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m32(ModRMBytePtr, prefix, &instr.instr_len);
            } else if (prefix.repeat_nz) { // 0xf2
                instr.opcode = DISASM_MAXSD;
                instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m64(ModRMBytePtr, prefix, &instr.instr_len);
            } else if (prefix.op_override) { // 0x66
                instr.opcode = DISASM_MAXPD;
                instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m128(ModRMBytePtr, prefix, &instr.instr_len);
            } else {
                instr.opcode = DISASM_MAXPS;
                instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m128(ModRMBytePtr, prefix, &instr.instr_len);
            }
            instr.instr_len += prefix.count;
            return instr;
        case 0x60:
            instr.opcode = DISASM_PUNPCKLBW;
            instr.instr_len += 1;
            instr.num_operands = 2;
            if (prefix.op_override) {
                instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m128(ModRMBytePtr, prefix, &instr.instr_len);
            } else {
                instr.operand[0] = _disasm_decode_mm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_mm_m128(ModRMBytePtr, prefix, &instr.instr_len);
            }
            instr.instr_len += prefix.count;
            return instr;
        case 0x61:
            instr.opcode = DISASM_PUNPCKLWD;
            instr.instr_len += 1;
            instr.num_operands = 2;
            if (prefix.op_override) {
                instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m128(ModRMBytePtr, prefix, &instr.instr_len);
            } else {
                instr.operand[0] = _disasm_decode_mm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_mm_m128(ModRMBytePtr, prefix, &instr.instr_len);
            }
            instr.instr_len += prefix.count;
            return instr;
        case 0x62:
            instr.opcode = DISASM_PUNPCKLDQ;
            instr.instr_len += 1;
            instr.num_operands = 2;
            if (prefix.op_override) {
                instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m128(ModRMBytePtr, prefix, &instr.instr_len);
            } else {
                instr.operand[0] = _disasm_decode_mm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_mm_m128(ModRMBytePtr, prefix, &instr.instr_len);
            }
            instr.instr_len += prefix.count;
            return instr;
        case 0x63:
            instr.opcode = DISASM_PACKSSWB;
            instr.instr_len += 1;
            instr.num_operands = 2;
            if (prefix.op_override) {
                instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m128(ModRMBytePtr, prefix, &instr.instr_len);
            } else {
                instr.operand[0] = _disasm_decode_mm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_mm_m128(ModRMBytePtr, prefix, &instr.instr_len);
            }
            instr.instr_len += prefix.count;
            return instr;
        case 0x64:
            instr.opcode = DISASM_PCMPGTB;
            instr.instr_len += 1;
            instr.num_operands = 2;
            if (prefix.op_override) {
                instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m128(ModRMBytePtr, prefix, &instr.instr_len);
            } else {
                instr.operand[0] = _disasm_decode_mm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_mm_m128(ModRMBytePtr, prefix, &instr.instr_len);
            }
            instr.instr_len += prefix.count;
            return instr;
        case 0x65:
            instr.opcode = DISASM_PCMPGTB;
            instr.instr_len += 1;
            instr.num_operands = 2;
            if (prefix.op_override) {
                instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m128(ModRMBytePtr, prefix, &instr.instr_len);
            } else {
                instr.operand[0] = _disasm_decode_mm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_mm_m128(ModRMBytePtr, prefix, &instr.instr_len);
            }
            instr.instr_len += prefix.count;
            return instr;
        case 0x66:
            instr.opcode = DISASM_PCMPGTW;
            instr.instr_len += 1;
            instr.num_operands = 2;
            if (prefix.op_override) {
                instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m128(ModRMBytePtr, prefix, &instr.instr_len);
            } else {
                instr.operand[0] = _disasm_decode_mm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_mm_m128(ModRMBytePtr, prefix, &instr.instr_len);
            }
            instr.instr_len += prefix.count;
            return instr;
        case 0x67:
            instr.opcode = DISASM_PCMPGTD;
            instr.instr_len += 1;
            instr.num_operands = 2;
            if (prefix.op_override) {
                instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m128(ModRMBytePtr, prefix, &instr.instr_len);
            } else {
                instr.operand[0] = _disasm_decode_mm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_mm_m128(ModRMBytePtr, prefix, &instr.instr_len);
            }
            instr.instr_len += prefix.count;
            return instr;
        case 0x68:
            instr.opcode = DISASM_PUNPCKHBW;
            instr.instr_len += 1;
            instr.num_operands = 2;
            if (prefix.op_override) {
                instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m128(ModRMBytePtr, prefix, &instr.instr_len);
            } else {
                instr.operand[0] = _disasm_decode_mm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_mm_m128(ModRMBytePtr, prefix, &instr.instr_len);
            }
            instr.instr_len += prefix.count;
            return instr;
        case 0x69:
            instr.opcode = DISASM_PUNPCKHWD;
            instr.instr_len += 1;
            instr.num_operands = 2;
            if (prefix.op_override) {
                instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m128(ModRMBytePtr, prefix, &instr.instr_len);
            } else {
                instr.operand[0] = _disasm_decode_mm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_mm_m128(ModRMBytePtr, prefix, &instr.instr_len);
            }
            instr.instr_len += prefix.count;
            return instr;
        case 0x6a:
            instr.opcode = DISASM_PUNPCKHDQ;
            instr.instr_len += 1;
            instr.num_operands = 2;
            if (prefix.op_override) {
                instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m128(ModRMBytePtr, prefix, &instr.instr_len);
            } else {
                instr.operand[0] = _disasm_decode_mm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_mm_m128(ModRMBytePtr, prefix, &instr.instr_len);
            }
            instr.instr_len += prefix.count;
            return instr;
        case 0x6b:
            instr.opcode = DISASM_PACKSSDW;
            instr.instr_len += 1;
            instr.num_operands = 2;
            if (prefix.op_override) {
                instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m128(ModRMBytePtr, prefix, &instr.instr_len);
            } else {
                instr.operand[0] = _disasm_decode_mm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_mm_m128(ModRMBytePtr, prefix, &instr.instr_len);
            }
            instr.instr_len += prefix.count;
            return instr;
        case 0x6c:
            instr.opcode = DISASM_PUNPCKLQDQ;
            instr.instr_len += 1;
            instr.num_operands = 2;
            if (prefix.op_override) {
                instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m128(ModRMBytePtr, prefix, &instr.instr_len);
            } else {
                DisasmInvalidTwoByte;
            }
            instr.instr_len += prefix.count;
            return instr;
        case 0x6d:
            instr.opcode = DISASM_PUNPCKHQDQ;
            instr.instr_len += 1;
            instr.num_operands = 2;
            if (prefix.op_override) {
                instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m128(ModRMBytePtr, prefix, &instr.instr_len);
            } else {
                DisasmInvalidTwoByte;
            }
            instr.instr_len += prefix.count;
            return instr;
        case 0x6e:
            instr.opcode = (RexW(*ModRMBytePtr) != 0) ? DISASM_MOVQ : DISASM_MOVD;
            instr.instr_len += 1;
            instr.num_operands = 2;
            instr.operand[0] = _disasm_decode_mm(ModRMBytePtr, prefix);
            instr.operand[1] = _disasm_decode_rm32(ModRMBytePtr, prefix, &instr.instr_len);
            instr.instr_len += prefix.count;
            return instr;
        case 0x6f:
            instr.instr_len += 1;
            instr.num_operands = 2;
            if (prefix.repeat) { // 0xf3
                instr.opcode = DISASM_MOVDQU;
                instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m128(ModRMBytePtr, prefix, &instr.instr_len);
            } else if (prefix.op_override) { // 0x66
                instr.opcode = DISASM_MOVDQA;
                instr.operand[0] = _disasm_decode_xmm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_xmm_m128(ModRMBytePtr, prefix, &instr.instr_len);
            } else {
                instr.opcode = DISASM_MOVQ;
                instr.operand[0] = _disasm_decode_mm(ModRMBytePtr, prefix);
                instr.operand[1] = _disasm_decode_mm_m64(ModRMBytePtr, prefix, &instr.instr_len);
            }
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
