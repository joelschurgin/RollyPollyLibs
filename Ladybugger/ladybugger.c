#include "misty.h"
#include "disasm.h"

#define BASE_ENTRY_POINT
#include "base.h"

#include "types.h"

#include "breakpoint.h"
#include "trampoline.h"
#include "proc_cntl.h"
#include "jump_instr.h"

#include "breakpoint.c"
#include "trampoline.c"
#include "proc_cntl.c"
#include "jump_instr.c"

typedef struct {
    i32 argc;
    u8** argv;
} MainArgs;

b32 lady_event(Lady_Ctx* ctx, Lady_Event event) {
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

            union {
                u64 word;
                u8 bytes[8];
            } instr;
            instr.word = ptrace(PTRACE_PEEKDATA, ctx->pid, regs.rip, 0L);

            printf("\033[31m[Ladybugger] Proc Segfaulted!\033[0m\n");
            return false;
        }
        break;
        case LADY_SIGILL:
        {
            struct user_regs_struct regs;
            ptrace(PTRACE_GETREGS, ctx->pid, NULL, &regs);

            union {
                u64 word;
                u8 bytes[8];
            } instr;
            instr.word = ptrace(PTRACE_PEEKDATA, ctx->pid, regs.rip, 0L);

            printf("\033[31m[Ladybugger] Illegal instruction!\033[0m\n");
            return false;
        }
        break;
        case LADY_EXIT:
            return false;
        default:
            TODO("Handle Other Event Type");
    }
    return true;
}

void lady_debug_event_loop(Lady_Ctx* ctx) {
    Lady_Event event = LADY_NONE;
    b32 run = true;
    do {
        event = lady_continue(ctx);
        run = lady_event(ctx, event);
    } while (run);
}

void lady_test_trap(Lady_Ctx* ctx, u64 target_addr) {
    Arena* arena = thread_ctx.shared_arena;
    TempArenaBlock(arena) {
        lady_launch_process(arena, ctx);

        u64 bp_key = lady_bp_set(ctx, target_addr, LADY_BP_TRAP);
        Lady_Bp* bp = lady_bp_hash_get(&ctx->bp_hash, bp_key);

        ThreadLocalTimer("TRAP was hit %lux", bp->hit_count) {
            lady_debug_event_loop(ctx);
        }
    }
}

void lady_test_trampoline_trap(Lady_Ctx* ctx, u64 target_addr) {
    Arena* arena = thread_ctx.shared_arena;
    TempArenaBlock(arena) {
        lady_launch_process(arena, ctx);

        u64 bp_key = lady_bp_set(ctx, target_addr, LADY_BP_TRAMPOLINE_TRAP);
        Lady_Bp* bp = lady_bp_hash_get(&ctx->bp_hash, bp_key);

        ThreadLocalTimer("TRAMPOLINE TRAP was hit %lux", bp->hit_count) {
            lady_debug_event_loop(ctx);
        }
    }
}

void lady_test_trampoline(Lady_Ctx* ctx, u64 target_addr) {
    Arena* arena = thread_ctx.shared_arena;
    TempArenaBlock(arena) {
        lady_launch_process(arena, ctx);

        u64 bp_key = lady_bp_set(ctx, target_addr, LADY_BP_TRAMPOLINE_COUNTER);
        Lady_Bp* bp = lady_bp_hash_get(&ctx->bp_hash, bp_key);

        ThreadLocalTimer("TRAMPOLINE COUNTER was hit %lux", (bp->trampoline.hit_count) ? *bp->trampoline.hit_count : 0) {
            lady_debug_event_loop(ctx);
        }
    }
}

void lady_sanity_check(Lady_Ctx* ctx) {
    Arena* arena = thread_ctx.shared_arena;
    TempArenaBlock(arena) {
        lady_launch_process(arena, ctx);
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

    Lady_Ctx* ctx;
    AssignLane(0) {
        ctx = lady_ctx_create(thread_ctx.shared_arena, path);
        ctx->line_info = misty_read_line_info(mountain, f);
        mutex_assign(&ctx->mutex);
    }
    LaneSyncPtr(ctx, 0);

    {
        Misty_LineInfoArray line_info = ctx->line_info;

        ThreadArraySplit split = ThreadArraySplit(line_info.count-2);
        for (u64 line_info_idx = split.start_idx; line_info_idx < split.end_idx; line_info_idx++) {
            u64 num_bytes_in_line = line_info.data[line_info_idx+2].addr - line_info.data[line_info_idx+1].addr;
            u8 num_bytes_read = 0;

            while (num_bytes_read < num_bytes_in_line) {
                u64 curr_addr = line_info.data[line_info_idx+1].addr + num_bytes_read;
                Disasm_Instr instr = disasm_decode(f->data + curr_addr);
                num_bytes_read += Max(1, instr.instr_len);

                u64 jump_addr = lady_check_jump_and_return_addr(&instr);
                if (jump_addr > 0) {
                    // add to hash
                    Assert(instr.num_operands == 1);
                    u64 dest_addr = 0;
                    switch (instr.operand[0].type) {
                        case DISASM_OP_TYPE_REL:
                            dest_addr = instr.operand[0].rel + instr.instr_len + curr_addr;
                        break;
                        case DISASM_OP_TYPE_REG:

                        break;
                        default:
                            TODO("Other operands");
                    }

                    MutexBlock(ctx->mutex) {
                        lady_jmp_hash_insert(&ctx->jmp_hash, curr_addr, (Lady_Jmp){
                            .dest_addr = dest_addr,
                        });
                        //printf("%d: ", LaneIdx());
                        //disasm_format(LaneArena(), instr, curr_addr);
                    }
                }
            }
        }
    }
    LaneSync();

    AssignLane(0) {

        for (u64 i = 1; i < ctx->line_info.count; i++) {
            u64 target_addr = ctx->line_info.data[i].addr;

            printf("DEBUGGING: 0x%lx | Line Info: %d / %d\n", target_addr, i, ctx->line_info.count - 1);
            lady_test_trampoline(ctx, target_addr);
            printf("\n");
        }

        /*
        u64 target_addr = ctx->line_info.data[12].addr;
        printf("DEBUGGING: 0x%lx | Line Info: %d\n", target_addr, 12);
        lady_test_trampoline(ctx, target_addr);
        printf("\n");
        */

        /*
        u64 target_addr = ctx->line_info.data[2].addr;
        lady_test_trap(ctx, target_addr);
        lady_test_trampoline_trap(ctx, target_addr);
        lady_test_trampoline(ctx, target_addr);
        lady_sanity_check(ctx);
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

        create_parallel_entry_point(num_threads, 1, parallel_main, &main_args);
        temp_arena_end(temp_arena);
    }

    return 0;
}
