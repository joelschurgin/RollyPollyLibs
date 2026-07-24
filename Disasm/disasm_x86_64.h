#ifndef DISASM_X86_64_H
#define DISASM_X86_64_H

#include "base.h"
#include "disasm_x86_64_instr_set.h"
#include "disasm_x86_64_reg.h"

typedef enum {
    DISASM_OP_TYPE_NONE,
    DISASM_OP_TYPE_REG,
    DISASM_OP_TYPE_MEM,
    DISASM_OP_TYPE_IMM,
} Disasm_OperandType;

typedef struct {
    Disasm_OperandType type;
    u8 size_bytes;
    union {
        Disasm_Reg reg;
        i64 imm;
        struct {
            Disasm_Reg base_reg;
            u8 idx_reg;
            u8 scale;
            i32 displacement;
        } mem;
    };
} Disasm_Operand;

typedef struct {
    Disasm_Operand operand[4];
    u8 num_operands;

    Disasm_Opcode opcode;
    u8* instr;
    u8 instr_len;
} Disasm_Instr;

Disasm_Instr  disasm_decode(u8* instr_ptr);

Disasm_Opcode disasm_decode_opcode_and_length_64(u8* instr, u64* instr_len);

String        disasm_opcode_format(Arena* arena, Disasm_Opcode opcode);
String        disasm_operand_format(Arena* arena, Disasm_Operand operand);

#endif
