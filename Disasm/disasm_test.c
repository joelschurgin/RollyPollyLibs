#include "disasm.h"

#define BASE_ENTRY_POINT
#include "base.h"

String curr_dir(String path) {
    i64 idx = path.size-1;
    for (; idx >= 0 && path.str[idx] != '/'; idx--);
    path.size = idx;
    return path;
}

u64 decode(Arena* arena, u8* instr_ptr, u64 addr) {
    Disasm_Instr instr = disasm_decode(instr_ptr);

    String mnemonic = instr.opcode == DISASM_INVALID ? String("(bad)") : disasm_opcode_format(arena, instr.opcode);
    for (u64 i = 0; i < Max(1, instr.instr_len); i++) {
        printf("%0.2x ", instr.instr[i]);
    }
    printf("\033[50G\033[32m%.*s\033[0m   \033[62G", mnemonic.size, mnemonic.str);

    for (u8 op_idx = 0; op_idx < instr.num_operands; op_idx++) {
        String operand = disasm_operand_format(arena, addr, instr, op_idx);
        printf("\033[94m%.*s\033[0m", operand.size, operand.str);
        if (op_idx != instr.num_operands - 1) printf(", ");
    }

    printf("\n");

    return Max(1, instr.instr_len);
}

u64 decode_at_addr(Arena* arena, File* f, u64 addr) {
    u8* instr_ptr = (u8*)f->data + addr;
    printf("  \033[95m%0.8lx\033[0m:  ", addr);
    return decode(arena, instr_ptr, addr);
}

i32 main(i32 argc, u8 **argv) {
    Arena* arena = default_arena();

    String dir = curr_dir(String(argv[0]));
    String test_path = string_format(arena, "%.*s/../dev_tools/disasm/test_0f_11_to_0f_17", dir.size, dir.str);

    FileRead(arena, test_path, f) {
        u64 addr = 0x0;
        while (addr < f->size) {
            addr += decode_at_addr(arena, f, addr);
        }
    }

    return 0;
}
