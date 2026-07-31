#ifndef DISASM_TYPES_H
#define DISASM_TYPES_H

#include "instr_set.h"
#include "reg.h"

typedef enum {
    DISASM_OP_TYPE_NONE,
    DISASM_OP_TYPE_REG,
    DISASM_OP_TYPE_MEM,
    DISASM_OP_TYPE_IMM,
    DISASM_OP_TYPE_REL,
} Disasm_OperandType;

typedef struct {
    b8 op_override;
    b8 addr_override;
    b8 lock;
    b8 repeat;
    b8 repeat_nz;
    Disasm_Segment segment;
    u8 rex;
    u32 count;
} Disasm_Prefix;

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



#endif
