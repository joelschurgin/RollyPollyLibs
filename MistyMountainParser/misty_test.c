#include "base.h"
#include "misty.h"

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
    MistyMountain* mountain = 0L;
    MistyMountain_SectionHeaderTableInfo section_header_table_info;
    if (LaneIdx() == 0) {
        Arena* arena = default_arena();
        String path = String(argv[1]);
        f = FilePtr(arena, path);

        mountain = MistyMountain(arena);
        section_header_table_info = misty_read_elf_header(mountain, f);
    }
    LaneSyncPtr(f, 0);
    LaneSyncPtr(mountain, 0);
    LaneSyncStruct(section_header_table_info, 0);

    misty_read_elf_section_headers(mountain, f, section_header_table_info);

    LaneSync();
    if (LaneIdx() == 0) {
        misty_macros(mountain, f);
    }
}

String curr_dir(String path) {
    i64 idx = path.size-1;
    for (; idx >= 0 && path.str[idx] != '/'; idx--);
    path.size = idx;
    return path;
}

i32 main(i32 argc, u8** argv) {
    u64 num_threads = 1;
 
    Arena* arena = arena_alloc(1024, 1024);
    String dir = curr_dir(String(argv[0]));

    u8* test_execs[] = {
        //"test32_dwarf2",
        //"test32_dwarf3",
        //"test32_dwarf4",
        //"test32_dwarf5",
        //"test64_dwarf2",
        //"test64_dwarf3",
        //"test64_dwarf4",
        "test64_dwarf5",
    };

    for (u64 i = 0; i < sizeof(test_execs)/sizeof(*test_execs); i++) {
        u8* test_exec_name = test_execs[i];

        TempArena temp_arena = temp_arena_begin(arena);

        String test_path = string_format(temp_arena.arena, "%.*s/dwarf_tests/%s", dir.size, dir.str, test_exec_name);
        printf("test path: %.*s\n", test_path.size, test_path.str);

        u8* argv_test[] = { (u8*)argv[0], test_path.str };
        MainArgs main_args = (MainArgs) {
            .argc = sizeof(argv_test)/sizeof(*argv_test),
            .argv = argv_test,
        };

        create_parallel_entry_point(num_threads, 0, parallel_main, &main_args);
        temp_arena_end(temp_arena);
    }

    return 0;
}
