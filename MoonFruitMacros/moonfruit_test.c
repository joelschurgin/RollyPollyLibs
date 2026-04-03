#include "base.h"
#include "moonfruit.h"

typedef struct {
    i32 argc;
    u8** argv;
} MainArgs;

void* parallel_main(void* main_args) {
    i32 argc = ((MainArgs*)main_args)->argc;
    u8** argv = ((MainArgs*)main_args)->argv;

    if (argc < 2) {
        ThreadExit(NULL);
    }

    File* f;
    DeferBlock({
        if (LaneIdx() == 0) {
            Arena* arena = default_arena();
            String path = String(argv[1]);
            f = FilePtr(arena, path);
            file_open(f, FILE_READ_ONLY);
        }
        LaneSyncPtr(f, 0);
    }, {
        LaneSync();
        if (LaneIdx() == 0) {
            file_close(f);
        }
    }) {
        // read file here
    }
}

String parent_dir(String path, u32 num_dirs) {
    if (num_dirs == 0) return path;
    i64 idx = path.size-1;
    for (; idx >= 0 && path.str[idx] != '/'; idx--);
    path.size = idx;
    return parent_dir(path, num_dirs-1);
}

i32 main(i32 argc, u8** argv) {
    u64 num_threads = 4;
 
    Arena* arena = arena_alloc(1024, 1024);
    String dir = parent_dir(String(argv[0]), 3);

    u8* test_files[] = {
        "MoonFruitMacros/moonfruit_test.c"
    };

    for (u64 i = 0; i < sizeof(test_files)/sizeof(*test_files); i++) {
        u8* test_file_name = test_files[i];

        TempArena temp_arena = temp_arena_begin(arena);

        String test_path = string_format(temp_arena.arena, "%.*s/%s", dir.size, dir.str, test_file_name);
        printf("test path: %.*s\n", test_path.size, test_path.str);

        u8* argv_test[] = { (u8*)argv[0], test_path.str };
        MainArgs main_args = (MainArgs) {
            .argc = sizeof(argv_test)/sizeof(*argv_test),
            .argv = argv_test,
        };

        create_parallel_entry_point(num_threads, parallel_main, &main_args);
        temp_arena_end(temp_arena);
    }

    return 0;
}
