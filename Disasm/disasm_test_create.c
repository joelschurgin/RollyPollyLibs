#define BASE_ENTRY_POINT
#include "base.h"

#include <stdio.h>
#include <stdlib.h>

String parent_dir(String path) {
    i64 idx = path.size-1;
    for (; idx >= 0 && path.str[idx] != '/'; idx--);
    path.size = idx;
    return path;
}

void test_create_00(File* f) {
    u64 pos = 0;

    u8 instr[3];
    instr[0] = 0x00;
    for (u16 i = 0; i < (u16)(-1); i++) {
        *(u16*)(instr+1) = i;
        file_write_bytes(f, pos, instr, sizeof(instr));
        pos += sizeof(instr);
    }
}

void test_create_file(Arena* arena, String curr_dir, u8 opcode) {
    String test_path = string_format(arena, "test_bytes_%0.2x", 0x00);

    FileWrite(arena, test_path, f) {
        switch (opcode) {
            case 0x00:
                test_create_00(f);
        }
    }
    String final_path = string_format(arena, "%.*s/disasm/%.*s", curr_dir.size, curr_dir.str, test_path.size, test_path.str);
    rename(test_path.str, final_path.str);
}

i32 main(i32 argc, u8 **argv) {
    Arena* arena = default_arena();

    String curr_dir = parent_dir(String(argv[0]));
    test_create_file(arena, curr_dir, 0x00);

    return 0;
}
