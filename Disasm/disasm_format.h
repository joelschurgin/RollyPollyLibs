#ifndef DISASM_FORMAT_H
#define DISASM_FORMAT_H

String disasm_opcode_format(Arena* arena, Disasm_Opcode opcode);
String disasm_operand_format(Arena* arena, u64 addr, Disasm_Instr instr, u8 operand_idx);

#endif
