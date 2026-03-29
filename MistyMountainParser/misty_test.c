#include "base.h"
#include "misty.h"

typedef struct {
    i32 argc;
    u8** argv;
} MainArgs;

void* parallel_main(void* main_args) {
    i32 argc = ((MainArgs*)main_args)->argc;
    u8** argv = ((MainArgs*)main_args)->argv;

    if (LaneIdx() == 0) {
        Arena* arena = default_arena();
        String path = String(argv[0]);
        File* exec = FilePtr(arena, path);

        Elf64_Ehdr header;

        printf("32: %d | 64: %d\n", sizeof(Elf32_Ehdr), sizeof(Elf64_Ehdr));
    }
    LaneSync();

    printf("Thread %d/%d\n", LaneIdx(), LaneCount());
}

i32 main(i32 argc, u8** argv) {
    MainArgs main_args = (MainArgs) {
        .argc = argc,
        .argv = argv,
    };

    u64 num_threads = 4;
    create_parallel_entry_point(num_threads, parallel_main, &main_args);

    return 0;
}
