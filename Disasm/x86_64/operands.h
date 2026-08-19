#ifndef DISASM_OPERANDS_H
#define DISASM_OPERANDS_H

#include "reg.h"

internal Disasm_Opcode _disasm_group1_mnemonic(u8* byte);
internal Disasm_Opcode _disasm_group2_mnemonic(u8* byte);
internal Disasm_Opcode _disasm_group3_mnemonic(u8* byte);

internal inline u8 _disasm_operand_16_32_64_size(Disasm_Prefix prefix);
internal inline u8 _disasm_operand_16_32_size(Disasm_Prefix prefix);
internal inline u8 _disasm_operand_16_64_size(Disasm_Prefix prefix);
internal inline u8 _disasm_operand_32_64_size(Disasm_Prefix prefix);
internal inline u8 _disasm_operand_8_16_size(Disasm_Prefix prefix);

internal Disasm_Operand _disasm_decode_mem_reg(Disasm_Prefix prefix, u8 size_bytes, Disasm_Reg reg32, Disasm_Reg reg64, Disasm_Segment seg);

internal Disasm_Operand _disasm_decode_string_src(Disasm_Prefix prefix, u8 size_bytes);
internal Disasm_Operand _disasm_decode_string_dest(Disasm_Prefix prefix, u8 size_bytes);

internal Disasm_Reg _disasm_decode_reg(u8 reg_idx, u8 size_bytes, u8 rex);

internal Disasm_Operand _disasm_decode_r(u8* byte, Disasm_Prefix prefix, u8 size_bytes);
internal inline Disasm_Operand _disasm_decode_r8(u8* byte, Disasm_Prefix prefix);
internal inline Disasm_Operand _disasm_decode_r16_32(u8* byte, Disasm_Prefix prefix);
internal inline Disasm_Operand _disasm_decode_r32_64(u8* byte, Disasm_Prefix prefix);
internal inline Disasm_Operand _disasm_decode_r16_32_64(u8* byte, Disasm_Prefix prefix);

internal Disasm_Operand _disasm_decode_m(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read, u8 size_bytes);
internal inline Disasm_Operand _disasm_decode_m8(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read);
internal inline Disasm_Operand _disasm_decode_m16(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read);
internal inline Disasm_Operand _disasm_decode_m16_32(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read);
internal inline Disasm_Operand _disasm_decode_m16int(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read);
internal inline Disasm_Operand _disasm_decode_m32int(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read);
internal inline Disasm_Operand _disasm_decode_m64(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read);
internal inline Disasm_Operand _disasm_decode_m14_28(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read);
internal inline Disasm_Operand _disasm_decode_m94_108(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read);
internal inline Disasm_Operand _disasm_decode_m64int(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read);
internal inline Disasm_Operand _disasm_decode_m64real(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read);
internal inline Disasm_Operand _disasm_decode_m80real(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read);
internal inline Disasm_Operand _disasm_decode_m80dec(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read);

internal Disasm_Operand _disasm_decode_rm(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read, u8 size_bytes, b8 is_fpu);
internal inline Disasm_Operand _disasm_decode_rm8(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read);
internal inline Disasm_Operand _disasm_decode_rm16(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read);
internal inline Disasm_Operand _disasm_decode_rm32(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read);
internal inline Disasm_Operand _disasm_decode_rm16_32(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read);
internal inline Disasm_Operand _disasm_decode_rm16_64(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read);
internal inline Disasm_Operand _disasm_decode_rm16_32_64(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read);

internal inline Disasm_Operand _disasm_decode_m16_r16_32(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read);
internal inline Disasm_Operand _disasm_decode_m16_r16_32_64(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read);

internal inline Disasm_Operand _disasm_decode_sti_m32real(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read);
internal inline Disasm_Operand _disasm_decode_sti(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read);

internal Disasm_Operand _disasm_decode_moffs(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read, u8 size_bytes);
internal inline Disasm_Operand _disasm_decode_moffs8(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read);
internal inline Disasm_Operand _disasm_decode_moffs16_32_64(u8* byte, Disasm_Prefix prefix, u8* num_bytes_read);

internal Disasm_Operand _disasm_Sreg(u8* byte, Disasm_Prefix prefix);
internal Disasm_Operand _disasm_specific_reg(Disasm_Reg reg);
internal Disasm_Operand _disasm_decode_imm(u8* byte, u8 size_bytes, u8 target_size, u8* instr_len);
internal inline Disasm_Operand _disasm_decode_imm8(u8* byte, u8 target_size, u8* instr_len);
internal inline Disasm_Operand _disasm_decode_imm16(u8* byte, Disasm_Prefix prefix, u8 target_size, u8* instr_len);
internal inline Disasm_Operand _disasm_decode_imm16_32(u8* byte, Disasm_Prefix prefix, u8 target_size, u8* instr_len);
internal inline Disasm_Operand _disasm_decode_imm16_32_64(u8* byte, Disasm_Prefix prefix, u8 target_size, u8* instr_len);

internal Disasm_Operand _disasm_decode_rel(u8* byte, u8 size_bytes, u8* instr_len);
internal inline Disasm_Operand _disasm_decode_rel8(u8* byte, u8* instr_len);
internal inline Disasm_Operand _disasm_decode_rel16_32(u8* byte, Disasm_Prefix prefix, u8* instr_len);

internal Disasm_Operand _disasm_decode_xmm(u8* byte, Disasm_Prefix prefix, u8* instr_len);
internal Disasm_Operand _disasm_decode_xmm_m(u8* byte, Disasm_Prefix prefix, u8* instr_len, u8 size_bytes);
internal inline Disasm_Operand _disasm_decode_xmm_m32(u8* byte, Disasm_Prefix prefix, u8* instr_len);
internal inline Disasm_Operand _disasm_decode_xmm_m64(u8* byte, Disasm_Prefix prefix, u8* instr_len);
internal inline Disasm_Operand _disasm_decode_xmm_m128(u8* byte, Disasm_Prefix prefix, u8* instr_len);

#endif
