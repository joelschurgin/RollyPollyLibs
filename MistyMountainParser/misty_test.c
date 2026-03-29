#include "base.h"
#include "misty.h"

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
    u64 num_threads = 4;
    create_parallel_entry_point(num_threads, parallel_main, NULL);

    return 0;
}
