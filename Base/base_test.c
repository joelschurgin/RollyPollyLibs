#define BASE_ENTRY_POINT
#include "base.h"

#include <stdio.h>

typedef struct {
    i32 argc;
    u8** argv;
} main_args;

void* parallel_main(void*) {
    LaneSync();
    printf("Thread %d/%d\n", LaneIdx(), LaneCount());
    LaneSync();

    printf("After barrier\n");
}

i32 main(i32 argc, u8** argv) {
    u64 num_threads = 8;
    create_parallel_entry_point(num_threads, 0, parallel_main, NULL);

    return 0;
}
/*
i32 main() {
    Arena* arena = arena_alloc(MB(1), KB(1));

    File* f = FilePtr(arena, String("../base_test.c"));

    DeferBlock(file_open(f, FILE_READ_ONLY), file_close(f)) {
        u8 buf[17];
        file_read_bytes(f, 0, buf, 16);
        buf[16] = 0;
        printf("%s\n", buf);
    }
    //bootstrap_entry_point();

    return 0;
}
*/
