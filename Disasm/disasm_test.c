#include "disasm.h"

#define BASE_ENTRY_POINT
#include "base.h"

String curr_dir(String path) {
    i64 idx = path.size-1;
    for (; idx >= 0 && path.str[idx] != '/'; idx--);
    path.size = idx;
    return path;
}

void test_decoder_bytes(Arena* arena, u8* instr) {
    u64 instr_len = 0;
    Disasm_Opcode opcode = disasm_decode_opcode_and_length_64(instr, &instr_len);
    String mnemonic = disasm_opcode_format(arena, opcode);
    printf("%.*s = %d bytes | ", mnemonic.size, mnemonic.str, instr_len);
    for (u64 i = 0; i < instr_len; i++) {
        printf("%0.2x ", instr[i]);
    }
    printf("\n");
}

u64 test_decoder_at_addr(Arena* arena, File* f, u64 addr) {
    u64 instr_len = 0;
    u8* instr = (u8*)f->data + addr;
    Disasm_Opcode opcode = disasm_decode_opcode_and_length_64(instr, &instr_len);
    String mnemonic = disasm_opcode_format(arena, opcode);
    printf("0x%lx: %.*s = %d bytes | ", addr, mnemonic.size, mnemonic.str, instr_len);
    for (u64 i = 0; i < instr_len; i++) {
        printf("%0.2x ", instr[i]);
    }
    printf("\n");
    return instr_len;
}

i32 main(i32 argc, u8 **argv) {
    Arena* arena = default_arena();

    String dir = curr_dir(String(argv[0]));
    //String test_path = string_format(arena, "%.*s/dwarf_tests/test64_dwarf5", dir.size, dir.str);
    String test_path = string_format(arena, "%.*s/misty_test", dir.size, dir.str);

    FileBlock(arena, test_path, FILE_READ_ONLY, f) {
        u64 instr_len = 0;
        instr_len = test_decoder_at_addr(arena, f, 0x5f55);
    }

    u8 instr[] = {0x48, 0xD3, 0xF8};
    test_decoder_bytes(arena, instr);

    return 0;
}
