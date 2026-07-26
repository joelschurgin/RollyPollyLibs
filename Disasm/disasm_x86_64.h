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
    DISASM_OP_TYPE_REL,
} Disasm_OperandType;

typedef enum {
    DISASM_SEG_NONE = 0x00,
    DISASM_SEG_ES   = 0x26,
    DISASM_SEG_CS   = 0x2E,
    DISASM_SEG_SS   = 0x36,
    DISASM_SEG_DS   = 0x3E,
    DISASM_SEG_FS   = 0x64,
    DISASM_SEG_GS   = 0x65,
} Disasm_Segment;

typedef struct {
    Disasm_OperandType type;
    u8 size_bytes;
    union {
        Disasm_Reg reg;
        i64 imm;
        i64 rel;
        struct {
            Disasm_Reg base_reg;
            u8 idx_reg;
            u8 scale;
            i64 displacement;
            Disasm_Segment segment;
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

String        disasm_opcode_format(Arena* arena, Disasm_Opcode opcode);
String        disasm_operand_format(Arena* arena, Disasm_Instr instr, u8 operand_idx);

#endif
