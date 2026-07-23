#ifndef DISASM_X86_64_H
#define DISASM_X86_64_H

#include "base.h"
#include "disasm_x86_64_instr_set.h"

typedef enum {
    DISASM_OP_TYPE_NONE,
    DISASM_OP_TYPE_REG,
    DISASM_OP_TYPE_MEM,
    DISASM_OP_TYPE_IMM,
} Disasm_OperandType;


typedef enum {
    DISASM_REG_NONE = 0,

    // --- 8-bit Registers (Legacy & Uniform/Extended) ---
    DISASM_REG_AL, DISASM_REG_CL, DISASM_REG_DL, DISASM_REG_BL,
    DISASM_REG_AH, DISASM_REG_CH, DISASM_REG_DH, DISASM_REG_BH,
    DISASM_REG_SPL, DISASM_REG_BPL, DISASM_REG_SIL, DISASM_REG_DIL,
    DISASM_REG_R8B, DISASM_REG_R9B, DISASM_REG_R10B, DISASM_REG_R11B, DISASM_REG_R12B, DISASM_REG_R13B, DISASM_REG_R14B, DISASM_REG_R15B,

    // --- 16-bit Registers ---
    DISASM_REG_AX, DISASM_REG_CX, DISASM_REG_DX, DISASM_REG_BX, DISASM_REG_SP, DISASM_REG_BP, DISASM_REG_SI, DISASM_REG_DI,
    DISASM_REG_R8W, DISASM_REG_R9W, DISASM_REG_R10W, DISASM_REG_R11W, DISASM_REG_R12W, DISASM_REG_R13W, DISASM_REG_R14W, DISASM_REG_R15W,

    // --- 32-bit Registers ---
    DISASM_REG_EAX, DISASM_REG_ECX, DISASM_REG_EDX, DISASM_REG_EBX, DISASM_REG_ESP, DISASM_REG_EBP, DISASM_REG_ESI, DISASM_REG_EDI,
    DISASM_REG_R8D, DISASM_REG_R9D, DISASM_REG_R10D, DISASM_REG_R11D, DISASM_REG_R12D, DISASM_REG_R13D, DISASM_REG_R14D, DISASM_REG_R15D,

    // --- 64-bit Registers ---
    DISASM_REG_RAX, DISASM_REG_RCX, DISASM_REG_RDX, DISASM_REG_RBX, DISASM_REG_RSP, DISASM_REG_RBP, DISASM_REG_RSI, DISASM_REG_RDI,
    DISASM_REG_R8,  DISASM_REG_R9,  DISASM_REG_R10, DISASM_REG_R11, DISASM_REG_R12, DISASM_REG_R13, DISASM_REG_R14, DISASM_REG_R15,

    // --- Special ---
    DISASM_REG_RIP
} Disasm_Reg;


typedef struct {
    Disasm_OperandType type;
    u8 size_bytes;
    union {
        u8 reg;
        u64 imm;
        struct {
            u8 base_reg;
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

#endif
