#include "misty.h"
#include "disasm.h"

#define BASE_ENTRY_POINT
#include "base.h"

#include "types.h"

#include "breakpoint.h"
#include "trampoline.h"
#include "proc_cntl.h"

#include "breakpoint.c"
#include "trampoline.c"
#include "proc_cntl.c"

typedef struct {
    i32 argc;
    u8** argv;
} MainArgs;

void lady_event(Lady_Ctx* ctx, Lady_Event event) {
    switch (event) {
        case LADY_TRAP:
        {
            struct user_regs_struct regs;
            ptrace(PTRACE_GETREGS, ctx->pid, NULL, &regs);
            regs.rip -= 1;

            u64 proc_addr = regs.rip - ctx->base_addr;
            Lady_Bp* bp = lady_bp_hash_get(&ctx->bp_hash, proc_addr);

            switch (bp->type) {
                case LADY_BP_TRAP:
                {
                    ptrace(PTRACE_SETREGS, ctx->pid, NULL, &regs);

                    bp->hit_count++;

                    lady_trap_unset(ctx, bp->trap);
                    lady_single_step(ctx);
                    lady_trap_reset(ctx, &bp->trap);
                }
                break;
                case LADY_BP_TRAMPOLINE_TRAP:
                    bp->hit_count++;
                break;
            }
        }
        break;
        case LADY_SEGFAULT:
        {
            struct user_regs_struct regs;
            ptrace(PTRACE_GETREGS, ctx->pid, NULL, &regs);

            printf("[Ladybugger] Proc Segfaulted!\n");
        }
        break;
        case LADY_SIGILL:
        {
            struct user_regs_struct regs;
            ptrace(PTRACE_GETREGS, ctx->pid, NULL, &regs);

            printf("[Ladybugger] Illegal instruction!\n");
        }
        break;
        case LADY_EXIT: break;
        default:
            TODO("Handle Other Event Type");
    }
}

void lady_debug_event_loop(Lady_Ctx* ctx) {
    Lady_Event event = LADY_NONE;
    do {
        event = lady_continue(ctx);
        lady_event(ctx, event);
    } while (event != LADY_EXIT);
}

void lady_test_trap(String path, Misty_LineInfoArray line_info, u64 target_addr) {
    Arena* arena = thread_ctx.shared_arena;
    TempArenaBlock(arena) {
        Lady_Ctx* ctx = lady_ctx_create(arena, path);

        if (ctx->pid == 0) ThreadExit(NULL);

        ctx->line_info = line_info;

        u64 bp_key = lady_bp_set(ctx, target_addr, LADY_BP_TRAP);

        Lady_Bp* bp = lady_bp_hash_get(&ctx->bp_hash, bp_key);

        ThreadLocalTimer("TRAP was hit %lux", bp->hit_count) {
            lady_debug_event_loop(ctx);
        }
    }
}

void lady_test_trampoline_trap(String path, Misty_LineInfoArray line_info, u64 target_addr) {
    Arena* arena = thread_ctx.shared_arena;
    TempArenaBlock(arena) {
        Lady_Ctx* ctx = lady_ctx_create(arena, path);

        if (ctx->pid == 0) ThreadExit(NULL);

        ctx->line_info = line_info;

        u64 bp_key = lady_bp_set(ctx, target_addr, LADY_BP_TRAMPOLINE_TRAP);

        Lady_Bp* bp = lady_bp_hash_get(&ctx->bp_hash, bp_key);

        ThreadLocalTimer("TRAMPOLINE TRAP was hit %lux", bp->hit_count) {
            lady_debug_event_loop(ctx);
        }
    }
}

void lady_test_trampoline(String path, Misty_LineInfoArray line_info, u64 target_addr) {
    Arena* arena = thread_ctx.shared_arena;
    TempArenaBlock(arena) {
        Lady_Ctx* ctx = lady_ctx_create(arena, path);

        if (ctx->pid == 0) ThreadExit(NULL);

        ctx->line_info = line_info;

        u64 bp_key = lady_bp_set(ctx, target_addr, LADY_BP_TRAMPOLINE);

        Lady_Bp* bp = lady_bp_hash_get(&ctx->bp_hash, bp_key);

        ThreadLocalTimer("TRAMPOLINE was hit %lux", *bp->trampoline.hit_count) {
            lady_debug_event_loop(ctx);
        }
    }
}

void lady_sanity_check(String path, Misty_LineInfoArray line_info) {
    Arena* arena = thread_ctx.shared_arena;
    TempArenaBlock(arena) {
        Lady_Ctx* ctx = lady_ctx_create(arena, path);

        if (ctx->pid == 0) ThreadExit(NULL);

        ctx->line_info = line_info;

        ThreadLocalTimer("No Breakpoints Set") {
            lady_debug_event_loop(ctx);
        }
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
    AssignLane(0) {
        Misty_LineInfoArray line_info = line_info = misty_read_line_info(mountain, f);

        for (u64 i = 1; i < line_info.count; i++) {
            u64 target_addr = line_info.data[i].addr;
            lady_test_trampoline(path, line_info, target_addr);
        }

        /*
        u64 target_addr = line_info.data[2].addr;
        lady_test_trap(path, line_info, target_addr);
        lady_test_trampoline_trap(path, line_info, target_addr);
        lady_test_trampoline(path, line_info, target_addr);
        lady_sanity_check(path, line_info);
        */
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
