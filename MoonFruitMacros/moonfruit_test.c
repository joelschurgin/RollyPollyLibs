#include "moonfruit.h"

#define BASE_ENTRY_POINT
#include "base.h"

#define should_not_have_params (u64)param
#define with_params(a, b,c) (a+b+c)
#define no_params()

#define func(y) y
#define func2(x) (6 * func(2*(x)))

#define my_exponent 4e+3
#define my_exponent2 4e3
#define my_exponent3 4p-3
#define num .4e-3
#define bad_exponent 3tone3 // tokenizer doesn't throw an error while using gcc

#define MULTIPLE_LINES 0, \
                       1, \
                       2

const u8 test[] = "#define skipping this string literal too";

void func_w_va_args(...) {

}

typedef struct {
    i32  argc;
    u8 **argv;
} MainArgs;

void* parallel_main(void* main_args) {
    i32 argc = ((MainArgs*)main_args)->argc;
    u8** argv = ((MainArgs*)main_args)->argv;

    if (argc < 2) {
        ThreadExit(NULL);
    }

    MoonFruit_ChunkQueue* chunk_Q;
    if (LaneIdx() == 0) {
        Arena* Q_arena = default_arena();
        String path = String(argv[1]);
        MoonFruit_File *f = moonfruit_file_create_and_open(Q_arena, path);

        chunk_Q = moonfruit_chunk_queue_create(Q_arena, LaneCount() * 2);
        mutex_assign(&chunk_Q->mutex, 0);
        for (u64 i = 0; i < LaneCount(); i++) {
            moonfruit_file_push_next_chunk(f, chunk_Q);
        }
    }
    LaneSyncPtr(chunk_Q, 0);

    for (; moonfruit_chunk_queue_size(chunk_Q) > 0;) {
        MoonFruit_Chunk chunk = moonfruit_chunk_queue_pop(chunk_Q);
        moonfruit_chunk_process(chunk, chunk_Q);
    }
    LaneSync();
    if (LaneIdx() == 0) {
        // assemble chunk info?
    }
}

String parent_dir(String path, u32 num_dirs) {
    if (num_dirs == 0)
        return path;
    i64 idx = path.size - 1;
    for (; idx >= 0 && path.str[idx] != '/'; idx--);
    path.size = idx;
    return parent_dir(path, num_dirs - 1);
}

i32 main(i32 argc, u8 **argv) {
    u64 num_threads = 4;

    Arena *arena = arena_alloc(1024, 1024);
    String dir   = parent_dir(String(argv[0]), 3);

    u8 *test_files[] = {"MoonFruitMacros/moonfruit_test.c"};

    for (u64 i = 0; i < sizeof(test_files) / sizeof(*test_files); i++) {
        u8 *test_file_name = test_files[i];

        TempArena temp_arena = temp_arena_begin(arena);

        String test_path = string_format(temp_arena.arena, "%.*s/%s", dir.size,
                                         dir.str, test_file_name);
        printf("test path: %.*s\n", test_path.size, test_path.str);

        u8 *argv_test[] = {(u8 *)argv[0], test_path.str};
        MainArgs main_args   = (MainArgs){
            .argc = sizeof(argv_test) / sizeof(*argv_test),
            .argv = argv_test,
        };

        create_parallel_entry_point(num_threads, 1, parallel_main, &main_args);
        temp_arena_end(temp_arena);
    }

    return 0;
}
