#include "misty.h"

#define BASE_ENTRY_POINT
#include "base.h"

#include "proc_cntl.h"
#include "proc_cntl.c"

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

    File* f = 0L;
    Misty* mountain = 0L;
    Misty_SectionHeaderTableInfo section_header_table_info = {0};
    String path = {0};
    AssignLane(0) {
        Arena* arena = default_arena();
        path = String(argv[1]);
        f = FilePtr(arena, path);

        mountain = Misty(arena);
        section_header_table_info = misty_read_elf_header(mountain, f);
    }
    LaneSyncPtr(f, 0);
    LaneSyncPtr(mountain, 0);
    LaneSyncStruct(section_header_table_info, 0);
    LaneSyncStruct(path, 0);

    misty_read_elf_section_headers(mountain, f, section_header_table_info);

    LaneSync();
    Misty_LineInfoArray line_info = {0};
    AssignLane(1) {
        ThreadLocalTimer(NULL) {
            line_info = misty_read_line_info(mountain, f);
        }
    }

    pid_t pid = 0;
    AssignLane(0) {
        ThreadLocalTimer(NULL) {
            pid = proc_launch_and_pause(path);
        }
    }
    LaneSyncStruct(pid, 0);
    LaneSyncStruct(line_info, 1);

    AssignLane(0) {
        if (pid != 0) {
            u64 base_addr = proc_base_addr(LaneArena(), pid);
            u64 target_addr = base_addr + line_info.data[8].addr;
            u8 prev_instr = trap_insert(pid, target_addr);

            do {
                printf("Press ENTER to continue: ");
                fflush(stdout);
                read(STDIN_FILENO, 0L, 1);
                i32 status = proc_continue(pid);

                if (WIFEXITED(status)) {
                    TODO("Handle exit signal");
                    break;
                } else if (WIFSIGNALED(status)) {
                    printf("killed by signal %d\n", WTERMSIG(status));
                    TODO("Handle kill signal");
                } else if (WIFSTOPPED(status)) {
                    printf("stopped by signal %d\n", WSTOPSIG(status));

                    TODO("testing todo");
                    if (WSTOPSIG(status) == SIGTRAP) {
                        printf("[Debugger] Intercepted SIGTRAP from child!\n");

                        struct user_regs_struct regs;
                        ptrace(PTRACE_GETREGS, pid, NULL, &regs);
                        regs.rip -= 1;
                        ptrace(PTRACE_SETREGS, pid, NULL, &regs);

                        trap_restore(pid, target_addr, prev_instr);
                        proc_single_step(pid);
                        prev_instr = trap_insert(pid, target_addr);
                    } else {
                        TODO("Handle other signals");
                    }
                }
            } while (true);
        }
    }
    LaneSync();
}

String curr_dir(String path) {
    i64 idx = path.size-1;
    for (; idx >= 0 && path.str[idx] != '/'; idx--);
    path.size = idx;
    return path;
}

i32 main(i32 argc, u8** argv) {
    u64 num_threads = 2;
 
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

        String test_path = string_format(temp_arena.arena, "%.*s/tests/dwarf_tests/%s", dir.size, dir.str, test_exec_name);
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
