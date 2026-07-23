#include "disasm.h"

#define BASE_ENTRY_POINT
#include "base.h"

String curr_dir(String path) {
    i64 idx = path.size-1;
    for (; idx >= 0 && path.str[idx] != '/'; idx--);
    path.size = idx;
    return path;
}

u64 decode_length_at_addr(Arena* arena, File* f, u64 addr) {
    u64 instr_len = 0;
    u8* instr = (u8*)f->data + addr;

    Disasm_Opcode opcode = disasm_decode_opcode_and_length_64(instr, &instr_len);
    String mnemonic = opcode == DISASM_INVALID ? String("(bad)") : disasm_opcode_format(arena, opcode);

    printf("    \033[95m%0.8lx\033[0m:       ", addr);
    for (u64 i = 0; i < instr_len; i++) {
        printf("%0.2x ", instr[i]);
    }
    printf("\033[50G\033[32m%.*s\033[0m\n", mnemonic.size, mnemonic.str);
    return instr_len;
}

u64 decode(Arena* arena, u8* instr_ptr) {
    Disasm_Instr instr = disasm_decode(instr_ptr);

    String mnemonic = instr.opcode == DISASM_INVALID ? String("(bad)") : disasm_opcode_format(arena, instr.opcode);
    //printf("    \033[95m%0.8lx\033[0m:       ", addr);
    for (u64 i = 0; i < instr.instr_len; i++) {
        printf("%0.2x ", instr.instr[i]);
    }
    printf("\033[50G\033[32m%.*s\033[0m\n", mnemonic.size, mnemonic.str);

    return instr.instr_len;
}

u64 decode_at_addr(Arena* arena, File* f, u64 addr) {
    u8* instr_ptr = (u8*)f->data + addr;
    return decode(arena, instr_ptr);
}

i32 main(i32 argc, u8 **argv) {
    Arena* arena = default_arena();

    u8 instr[3];
    instr[0] = 0x00;
    for (u16 i = 0; i < (u16)(-1); i++) {
        *(u16*)(instr+1) = i;

        for (u8 j = 0; j < sizeof(instr) / sizeof(*instr); j++) {
            printf("%0.2x", *(instr + j));
        }
        printf("\n");
    }

    /*
    String dir = curr_dir(String(argv[0]));
    //String test_path = string_format(arena, "%.*s/dwarf_tests/test64_dwarf5", dir.size, dir.str);
    String test_path = string_format(arena, "%.*s/../../Disasm/test_bytes.bin", dir.size, dir.str);

    FileBlock(arena, test_path, FILE_READ_ONLY, f) {
        u64 addr = 0;
        while (addr < f->size) {
            addr += decode_at_addr(arena, f, addr);
        }
        u64 addr = 0;
        addr += decode_at_addr(arena, f, addr);
    }
    */

    return 0;
}
