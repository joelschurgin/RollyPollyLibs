#ifndef DISASM_X86_64_H
#define DISASM_X86_64_H

#include "base.h"
#include "disasm_x86_64_instr_set.h"

Disasm_Opcode disasm_decode_opcode_and_length_64(u8* instr, u64* instr_len);
u8*           disasm_opcode_stringify(Disasm_Opcode opcode);
String        disasm_opcode_format(Arena* arena, Disasm_Opcode opcode);

#endif
