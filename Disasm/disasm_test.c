#include "disasm.h"

#define BASE_ENTRY_POINT
#include "base.h"

String curr_dir(String path) {
    i64 idx = path.size-1;
    for (; idx >= 0 && path.str[idx] != '/'; idx--);
    path.size = idx;
    return path;
}

void test_decoder_at_addr(File* f, u64 addr) {
    u64 instr_len = 0;
    u8* instr = (u8*)f->data + addr;
    Disasm_Opcode opcode = disasm_decode_opcode_and_length_64(instr, &instr_len);
    printf("%s = %d bytes | ", disasm_opcode_stringify(opcode), instr_len);
    for (u64 i = 0; i < instr_len; i++) {
        printf("%x ", instr[i]);
    }
    printf("\n");
}

i32 main(i32 argc, u8 **argv) {
    Arena* arena = default_arena();

    String dir = curr_dir(String(argv[0]));
    String test_path = string_format(arena, "%.*s/dwarf_tests/test64_dwarf5", dir.size, dir.str);

    FileBlock(arena, test_path, FILE_READ_ONLY, f) {
        test_decoder_at_addr(f, 0x1004);
    }

    return 0;
}
