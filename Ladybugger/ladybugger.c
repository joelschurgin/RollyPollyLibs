#include "misty.h"

#define BASE_ENTRY_POINT
#include "base.h"

#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <sys/syscall.h>

typedef struct {
    i32 argc;
    u8** argv;
} MainArgs;

#define CHILD_PROCESS 0
pid_t launch_process_and_pause(String path) {
    pid_t pid = fork();

    if (pid == CHILD_PROCESS) {
        if (ptrace(PTRACE_TRACEME, pid, 0L, 0L) < 0) {
            perror("Cannot debug process!\n");
        }

        if (execv(path.str, 0L) < 0) {
            perror("Error with execv");
        }

        return pid;
    }

    i32 status = 0;
    waitpid(pid, &status, 0);

    if (!WIFSTOPPED(status)) {
        return 0;
    }

    printf("process stopped!\n");
    return pid;
}

i32 process_continue(pid_t pid) {
    i32 status = 0;
    if (ptrace(PTRACE_CONT, pid, 0L, 0L) < 0) {
        perror("Cannot continue process!\n");
    }
    waitpid(pid, &status, 0);

    return status;
}

i32 process_single_step(pid_t pid) {
    i32 status = 0;
    if (ptrace(PTRACE_SINGLESTEP, pid, 0L, 0L) < 0) {
        perror("Cannot single step process!\n");
    }
    waitpid(pid, &status, 0);

    return status;
}

u64 remote_mmap(pid_t pid, void* addr, size_t len, int prot, int flags, int fd, off_t offset) {
    struct user_regs_struct old_regs, new_regs;
 
    Assert(ptrace(PTRACE_GETREGS, pid, NULL, &old_regs) >= 0);

    u64 orig_instr = ptrace(PTRACE_PEEKDATA, pid, old_regs.rip, 0L);

    u64 syscall_payload = orig_instr;
    u8* payload_bytes = (u8*)&syscall_payload;
    payload_bytes[0] = 0x0f;
    payload_bytes[1] = 0x05;

    Assert(ptrace(PTRACE_POKEDATA, pid, old_regs.rip, syscall_payload) >= 0);

    new_regs = old_regs;

    new_regs.rax = SYS_mmap;
    new_regs.rdi = (u64)addr;
    new_regs.rsi = len;
    new_regs.rdx = prot;
    new_regs.r10 = flags;
    new_regs.r8  = fd;
    new_regs.r9  = offset;

    ptrace(PTRACE_SETREGS, pid, NULL, &new_regs);

    ptrace(PTRACE_SINGLESTEP, pid, NULL, NULL);
    waitpid(pid, NULL, 0);

    ptrace(PTRACE_GETREGS, pid, NULL, &new_regs);
    u64 allocated_mem = (u64)new_regs.rax;

    Assert(ptrace(PTRACE_POKEDATA, pid, old_regs.rip, orig_instr) >= 0);
    ptrace(PTRACE_SETREGS, pid, NULL, &old_regs);

    return allocated_mem;
}

void target_func() {
    printf("Hooked!\n");
}

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
            pid = launch_process_and_pause(path);
        }
        printf("Thread 0 done\n");
    }
    LaneSyncStruct(pid, 0);
    LaneSyncStruct(line_info, 1);

    AssignLane(0) {
        if (pid != 0) {
            printf("child pid: %d\n", pid);
            Arena* arena = LaneArena();
            u8 bytes[12] = {0};
            TempArenaBlock(arena) {
                String proc_path = string_format(arena, "/proc/%d/maps", (i32)pid);

                i32 fd = open(proc_path.str, O_RDONLY);
                read(fd, bytes, sizeof(bytes));
                close(fd);
            }

            u64 base_addr = 0;
            for (u8* c = bytes; (u64)(c - bytes) < sizeof(bytes); c++) {
                base_addr <<= 4;
                if (*c >= '0' && *c <= '9') base_addr += (*c - '0');
                else if (*c >= 'a' && *c <= 'f') base_addr += (*c - 'a' + 10);
                else if (*c >= 'A' && *c <= 'Z') base_addr += (*c - 'A' + 10);
            }

            u64 target_addr = base_addr + line_info.data[0].addr;

            i64 dist = ((i64)target_func - ((i64)target_addr + 5));
            Assert((u64)Abs(dist) <= 0x7fffff00); // can handle a 32 bit jump

            struct {
                u8 opcode;
                u32 addr;
            } jmp_instr = {
                .opcode = 0xe9,
                .addr = dist,
            };

            u64 addr = remote_mmap(pid,
                                   (void*)base_addr + MB(128),
                                   PAGE_SIZE,
                                   PROT_READ | PROT_WRITE | PROT_EXEC,
                                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE,
                                   -1,
                                   0);

            read(STDIN_FILENO, 0L, 1);
            process_continue(pid);
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
