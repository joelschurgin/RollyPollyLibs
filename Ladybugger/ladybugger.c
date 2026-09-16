#include "misty.h"

#define BASE_ENTRY_POINT
#include "base.h"

#include "breakpoint.h"
#include "proc_cntl.h"

#include "breakpoint.c"
#include "proc_cntl.c"

typedef struct {
    i32 argc;
    u8** argv;
} MainArgs;

// passing in trap is temporary because we need a full data structure for breakpoints
void lady_event(Lady_Ctx* ctx, Lady_Event event) {
    switch (event) {
        case LADY_TRAP:
            printf("[Ladybugger] Intercepted SIGTRAP!\n");

            struct user_regs_struct regs;
            ptrace(PTRACE_GETREGS, ctx->pid, NULL, &regs);
            regs.rip -= 1;
            ptrace(PTRACE_SETREGS, ctx->pid, NULL, &regs);

            u64 proc_addr = regs.rip - ctx->base_addr;
            Lady_Bp* bp = lady_bp_hash_get(&ctx->bp_hash, proc_addr);

            Assert(bp->type == LADY_BP_TRAP);

            lady_trap_unset(ctx, bp->trap);
            lady_single_step(ctx);
            lady_trap_reset(ctx, &bp->trap);
        break;
        case LADY_EXIT:
        break;
        default:
            TODO("Handle Other Event Type");
    }
}

void debug_event_loop(Lady_Ctx* ctx) {
    u64 target_addr = ctx->line_info.data[9].addr;
    Lady_Bp bp = (Lady_Bp){
        .type = LADY_BP_TRAP,
        .trap = lady_trap_set(ctx, target_addr),
        .line_info_idx = 9,
    };

    lady_bp_hash_insert(&ctx->bp_hash, bp.trap.addr, bp);

    ThreadLocalTimer("Timing Int3 Style Breakpoints") {
        Lady_Event event = LADY_NONE;
        do {
            event = lady_continue(ctx);
            lady_event(ctx, event);
        } while (event != LADY_EXIT);
    }
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
        line_info = misty_read_line_info(mountain, f);
    }

    Lady_Ctx* ctx = 0L;
    AssignLane(0) {
        ctx = lady_ctx_create(thread_ctx.shared_arena, path);
    }
    LaneSyncPtr(ctx, 0);
    LaneSyncStruct(line_info, 1);

    if (ctx->pid == 0) ThreadExit(NULL);

    AssignLane(0) {
        ctx->line_info = line_info;
        debug_event_loop(ctx);
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
        //"test64_dwarf5",
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
