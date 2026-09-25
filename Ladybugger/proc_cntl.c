#define CHILD_PROCESS 0
internal pid_t proc_launch_and_pause(String path) {
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

    return pid;
}

internal u64 proc_base_addr(Arena* arena, pid_t pid) {
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

    return base_addr;
}

Lady_Ctx* lady_ctx_create(Arena* arena, String path) {
    Lady_Ctx* ctx = push_struct(arena, Lady_Ctx);

    ctx->bp_hash = lady_bp_hash_create(arena, 256);
    ctx->jmp_hash = lady_jmp_hash_create(arena, 256);
    ctx->path = path;

    return ctx;
}

i32 proc_open_mem(Arena* arena, pid_t pid) {
    i32 mem_fd = -1;
    TempArenaBlock(arena) {
        String path = string_format(arena, "/proc/%d/mem", pid);
        mem_fd = open(path.str, O_RDWR);
        Assert(mem_fd >= 0);
    }
    return mem_fd;
}

void lady_launch_process(Arena* arena, Lady_Ctx* ctx) {
    ctx->pid = proc_launch_and_pause(ctx->path);
    if (ctx->pid == 0) ThreadExit(NULL);

    ctx->base_addr = proc_base_addr(arena, ctx->pid);
    ctx->remote_func_alloc = remote_func_alloc_init(ctx->pid, ctx->base_addr + MB(128), PAGE_SIZE * 4);

    ctx->mem_fd = proc_open_mem(arena, ctx->pid);
}

internal Lady_Event lady_status_to_event(i32 status) {
    if (WIFEXITED(status)) {
        return LADY_EXIT;
    } else if (WIFSIGNALED(status)) {
        return LADY_KILL;
    } else if (WIFSTOPPED(status)) {
        // man 7 signal explains these
        if (WSTOPSIG(status) == SIGTRAP) {
            return LADY_TRAP;
        } else if (WSTOPSIG(status) == SIGSEGV) {
            return LADY_SEGFAULT;
        } else if (WSTOPSIG(status) == SIGILL) {
            return LADY_SIGILL;
        } else {
            TODO("Handle other signals");
        }
    }

    return LADY_NONE;
}

internal i32 proc_continue(pid_t pid) {
    i32 status = 0;
    if (ptrace(PTRACE_CONT, pid, 0L, 0L) < 0) {
        perror("Cannot continue process!\n");
    }
    waitpid(pid, &status, 0);

    return status;
}

internal i32 proc_single_step(pid_t pid) {
    i32 status = 0;
    if (ptrace(PTRACE_SINGLESTEP, pid, 0L, 0L) < 0) {
        perror("Cannot single step process!\n");
    }
    waitpid(pid, &status, 0);

    return status;
}

Lady_Event lady_continue(Lady_Ctx* ctx) {
    return lady_status_to_event(proc_continue(ctx->pid));
}

Lady_Event lady_single_step(Lady_Ctx* ctx) {
    return lady_status_to_event(proc_single_step(ctx->pid));
}

void* remote_mmap(pid_t pid, void* addr, size_t len, int prot, int flags, int fd, off_t offset) {
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
    void* allocated_mem = (void*)new_regs.rax;

    Assert(ptrace(PTRACE_POKEDATA, pid, old_regs.rip, orig_instr) >= 0);
    ptrace(PTRACE_SETREGS, pid, NULL, &old_regs);

    return allocated_mem;
}

i32 remote_open(pid_t pid, const char* path, int flags, mode_t mode) {
    struct user_regs_struct old_regs, new_regs;
 
    Assert(ptrace(PTRACE_GETREGS, pid, NULL, &old_regs) >= 0);

    u64 orig_instr = ptrace(PTRACE_PEEKDATA, pid, old_regs.rip, 0L);

    u64 syscall_payload = orig_instr;
    u8* payload_bytes = (u8*)&syscall_payload;
    payload_bytes[0] = 0x0f;
    payload_bytes[1] = 0x05;

    Assert(ptrace(PTRACE_POKEDATA, pid, old_regs.rip, syscall_payload) >= 0);

    size_t path_len = strlen(path) + 1;
    size_t padded_len = (path_len + 7) & ~7;
    u64 remote_path_addr = old_regs.rsp - padded_len;
 
    for (size_t i = 0; i < padded_len; i += 8) {
        u64 word = 0;

        size_t chunk = (path_len - i > 8) ? 8 : (path_len - i);
        memcpy(&word, path + i, chunk);

        Assert(ptrace(PTRACE_POKEDATA, pid, remote_path_addr + i, word) >= 0);
    }
    new_regs = old_regs;

    new_regs.rax = SYS_open;
    new_regs.rdi = remote_path_addr;
    new_regs.rsi = flags;
    new_regs.rdx = mode;

    ptrace(PTRACE_SETREGS, pid, NULL, &new_regs);

    ptrace(PTRACE_SINGLESTEP, pid, NULL, NULL);
    waitpid(pid, NULL, 0);

    ptrace(PTRACE_GETREGS, pid, NULL, &new_regs);
    i32 child_fd = (i32)new_regs.rax;

    Assert(ptrace(PTRACE_POKEDATA, pid, old_regs.rip, orig_instr) >= 0);
    ptrace(PTRACE_SETREGS, pid, NULL, &old_regs);

    return child_fd;
}

internal inline u8 trap_insert(pid_t pid, u64 addr) {
    u64 curr_instr = ptrace(PTRACE_PEEKDATA, pid, addr, 0L);
    u64 trap_instr = (curr_instr & ~0xff) | 0xcc;
    ptrace(PTRACE_POKEDATA, pid, addr, trap_instr);
    return curr_instr & 0xff;
}

internal inline void trap_restore(pid_t pid, u64 addr, u8 data) {
    u64 curr_instr = ptrace(PTRACE_PEEKDATA, pid, addr, 0L);
    u64 prev_instr = (curr_instr & ~0xff) | data;
    ptrace(PTRACE_POKEDATA, pid, addr, prev_instr);
}

Lady_Trap lady_trap_set(Lady_Ctx* ctx, u64 addr) {
    Lady_Trap trap = {
        .addr = addr,
        .data = trap_insert(ctx->pid, ctx->base_addr + addr),
    };
    return trap;
}

void lady_trap_unset(Lady_Ctx* ctx, Lady_Trap trap) {
    return trap_restore(ctx->pid, ctx->base_addr + trap.addr, trap.data);
}

void lady_trap_reset(Lady_Ctx* ctx, Lady_Trap* trap) {
    trap->data = trap_insert(ctx->pid, ctx->base_addr + trap->addr);
}

void proc_insert_jmp(pid_t pid, u64 addr, u64 func_ptr, u8 instr_len) {
    struct __attribute__((packed)) {
        u8 opcode;
        i32 addr;
    } jmp_instr = {
        .opcode = 0xe9,
        .addr = (i32)((i64)func_ptr - ((i64)addr + 5)),
    };

    u64 curr_instr = ptrace(PTRACE_PEEKDATA, pid, addr, 0L);

    MemoryCopy(&curr_instr, &jmp_instr, sizeof(jmp_instr));
    ptrace(PTRACE_POKEDATA, pid, addr, curr_instr);
}

RemoteFuncAllocator remote_func_alloc_init(pid_t pid, u64 target_addr, u64 size) {
    RemoteFuncAllocator func_alloc = {0};

    {
        mode_t old_mask = umask(0);
        func_alloc.shm_fd = shm_open("/fast_dbg_shm",
                                    O_CREAT | O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO );
        umask(old_mask);

        Assert(func_alloc.shm_fd >= 0);
        Assert(ftruncate(func_alloc.shm_fd, size) >= 0);
    }

    func_alloc.remote_shm_fd = remote_open(pid, "/dev/shm/fast_dbg_shm", O_RDWR, 0);
    Assert(func_alloc.remote_shm_fd >= 0);

    func_alloc.remote_base = remote_mmap(pid,
                                   (void*)target_addr,
                                   size,
                                   PROT_READ | PROT_WRITE | PROT_EXEC,
                                   MAP_SHARED,
                                   func_alloc.remote_shm_fd,
                                   0);

    Assert((i64)func_alloc.remote_base >= 0);

    func_alloc.base = mmap(0L,
                        size,
                        PROT_READ | PROT_WRITE,
                        MAP_SHARED,
                        func_alloc.shm_fd,
                        0);

    Assert((i64)func_alloc.base >= 0);

    func_alloc.pos = 0;
    func_alloc.size = size;

    return func_alloc;
}

/*
void lady_trampoline_trap_set(Lady_Ctx* ctx, u64 addr, u64* bp_addr) {
    addr += ctx->base_addr;

    u64 func_size = (u64)&__trampoline_trap_end - (u64)trampoline_trap;

    RemoteFuncAllocator* alloc = &ctx->remote_func_alloc;
    Assert(alloc->pos + func_size <= alloc->size);

    void* func_write_ptr = (u8*)alloc->base + alloc->pos;
    void* remote_func_ptr = (u8*)alloc->remote_base + alloc->pos;
    alloc->pos += func_size;

    MemoryCopy(func_write_ptr, trampoline_trap, func_size);

    {
        u64 trampoline_trap_stolen_bytes_offset = (u64)&__trampoline_trap_stolen_bytes - (u64)&trampoline_trap;
        u64 stolen_bytes = ptrace(PTRACE_PEEKDATA, ctx->pid, addr, 0L);
        MemoryCopy(func_write_ptr + trampoline_trap_stolen_bytes_offset, &stolen_bytes, 5);
    }

    {
        u64 trampoline_trap_return_ptr_offset = (u64)&__trampoline_trap_return_ptr - (u64)&trampoline_trap;
        u64 ret_addr = (u64)addr + 5;
        MemoryCopy(func_write_ptr + trampoline_trap_return_ptr_offset, &ret_addr, sizeof(u64));
    }

    proc_insert_jmp(ctx->pid, addr, (u64)remote_func_ptr, 5);
 
    u64 trap_offset = (u64)&__trampoline_trap - (u64)&trampoline_trap;
    *bp_addr = (u64)remote_func_ptr + trap_offset - ctx->base_addr;
}
*/

#define remote_func_push_bytes(alloc, func_ptr, write_pos, bytes, num_bytes) \
    do { \
        MemoryCopy((func_ptr) + (write_pos), (bytes), (num_bytes)); \
        (write_pos) += (num_bytes); \
        ((alloc)->pos) += (num_bytes); \
    } while (0)

void lady_trampoline_push_instr(Lady_Ctx* ctx, void* func_local, void* func_remote, u64* write_pos, Disasm_Instr* instr, u64 addr) {
    u8 instr_bytes[14];

    MemoryCopy(instr_bytes, instr->instr, instr->instr_len);
    if (instr->opcode == DISASM_CALL) {
        switch (instr->operand[0].type) {
            case DISASM_OP_TYPE_REL:
            {
                u64 dest_addr = instr->operand[0].rel + instr->instr_len + addr - ctx->base_addr;
                u64 new_dest = dest_addr - ((u64)func_remote + (*write_pos) + instr->instr_len - ctx->base_addr);
                MemoryCopy(instr_bytes + instr->instr_len - instr->operand[0].size_bytes, &new_dest, instr->operand[0].size_bytes);
            }
            break;
            default:
                TODO("Other operands");
        }
    }
    remote_func_push_bytes(&ctx->remote_func_alloc, func_local, *write_pos, instr_bytes, instr->instr_len);
}

typedef struct {
    u64 instr_addr;

    Disasm_InstrArray post_tramp_instr;

    void* func_local;
    void* func_remote;

    u64 write_pos;

    u64 site_size;
    u64 site_start_addr;
    u64 site_end_addr;
} Lady_TrampolineCtx;

Lady_TrampolineCtx lady_trampoline_begin(Arena* arena, Lady_Ctx* ctx, u64 look_ahead_addr, u64 target_addr, u64 next_line_addr) {
    Lady_TrampolineCtx tramp_ctx = {0};

    // read instr bytes
    u8Array instr_bytes = Array(arena, u8, next_line_addr - look_ahead_addr);
    i32 ret = pread(ctx->mem_fd, instr_bytes.data, instr_bytes.count, look_ahead_addr);
    if (ret < 0) {
        perror("pread");
    }

    // disassemble
    Disasm_InstrArray disasm_instr;
    i64 target_addr_instr_idx = 0;
    ArrayBuilderBlock(arena, disasm_instr, Disasm_Instr) {
        u8* instr_ptr = instr_bytes.data;
        while (instr_ptr < (instr_bytes.data + instr_bytes.count)) {
            array_builder_push(arena, disasm_instr, disasm_decode(instr_ptr));
            if ((u64)(instr_ptr - instr_bytes.data) == (target_addr - look_ahead_addr)) {
                target_addr_instr_idx = (i64)disasm_instr.count-1;
            }
            instr_ptr += ArrayLast(disasm_instr).instr_len;
        }
    }

    // find which instructions to replace
    tramp_ctx.site_size = next_line_addr - target_addr;
    u64 disasm_instr_start_idx = target_addr_instr_idx;
    for (i64 instr_idx = target_addr_instr_idx - 1; instr_idx >= 0 && tramp_ctx.site_size < 5; instr_idx--) {
        tramp_ctx.site_size += disasm_instr.data[instr_idx].instr_len;
        disasm_instr_start_idx = instr_idx;
    }

    u64 instr_start_addr = next_line_addr - tramp_ctx.site_size;
    u64 instr_addr = instr_start_addr;

    u64 disasm_instr_end_idx = disasm_instr.count-1;
    for (; disasm_instr_end_idx >= disasm_instr_start_idx; disasm_instr_end_idx--) {
        u64 potential_tramp_site_size = tramp_ctx.site_size - disasm_instr.data[disasm_instr_end_idx].instr_len;
        if (potential_tramp_site_size < 5) break;

        tramp_ctx.site_size = potential_tramp_site_size;
    }

    RemoteFuncAllocator* alloc = &ctx->remote_func_alloc;

    tramp_ctx.func_local = (u8*)alloc->base + alloc->pos;
    tramp_ctx.func_remote = (u8*)alloc->remote_base + alloc->pos;

    // write jump instruction into trampoline site
    tramp_ctx.site_start_addr = instr_start_addr;
    tramp_ctx.site_end_addr = instr_start_addr + tramp_ctx.site_size;
    {
        u8Array jmp_instr_bytes = Array(arena, u8, tramp_ctx.site_size);
        MemorySet(jmp_instr_bytes.data, 0x90, tramp_ctx.site_size);

        struct __attribute__((packed)) {
            u8 opcode;
            i32 addr;
        } jmp_instr = {
            .opcode = 0xe9,
            .addr = (i32)((i64)tramp_ctx.func_remote - (i64)(tramp_ctx.site_end_addr)),
        };
        MemoryCopy(jmp_instr_bytes.data + (tramp_ctx.site_size - sizeof(jmp_instr)), &jmp_instr, sizeof(jmp_instr));

        pwrite(ctx->mem_fd, jmp_instr_bytes.data, jmp_instr_bytes.count, tramp_ctx.site_start_addr);
    }

    // push any instructions before target_addr
    {
        Disasm_InstrArray instr_slice = Disasm_InstrArraySlice(disasm_instr, disasm_instr_start_idx, target_addr_instr_idx-1);
        for EachElement(instr, Disasm_Instr, instr_slice) {
            lady_trampoline_push_instr(ctx, tramp_ctx.func_local, tramp_ctx.func_remote, &tramp_ctx.write_pos, instr, instr_addr);
            instr_addr += instr->instr_len;
        }
    }

    tramp_ctx.instr_addr = target_addr;
    tramp_ctx.post_tramp_instr = Disasm_InstrArraySlice(disasm_instr, target_addr_instr_idx, disasm_instr_end_idx);

    return tramp_ctx;
}

void lady_trampoline_end(Lady_Ctx* ctx, Lady_TrampolineCtx* tramp_ctx) {
    RemoteFuncAllocator* alloc = &ctx->remote_func_alloc;

    // push any instructions after target_addr
    {
        u64 instr_addr = tramp_ctx->instr_addr;
        for EachElement(instr, Disasm_Instr, tramp_ctx->post_tramp_instr) {
            lady_trampoline_push_instr(ctx, tramp_ctx->func_local, tramp_ctx->func_remote, &tramp_ctx->write_pos, instr, instr_addr);
            instr_addr += instr->instr_len;
        }
    }

    // push returning jump
    {
        u8 jmp_instr[] = {0xff, 0x25, 0x00, 0x00, 0x00, 0x00};
        remote_func_push_bytes(alloc, tramp_ctx->func_local, tramp_ctx->write_pos, jmp_instr, sizeof(jmp_instr));

        u64 ret_addr = tramp_ctx->site_end_addr;
        remote_func_push_bytes(alloc, tramp_ctx->func_local, tramp_ctx->write_pos, &ret_addr, sizeof(ret_addr));
    }
}

void lady_trampoline_trap_set(Lady_Ctx* ctx, u64 look_ahead_addr, u64 target_addr, u64 next_line_addr, u64* bp_addr) {
    target_addr += ctx->base_addr;
    look_ahead_addr += ctx->base_addr;
    next_line_addr += ctx->base_addr;

    Arena* arena = LaneArena();
    TempArenaBlock(arena) {
        Lady_TrampolineCtx tramp_ctx = lady_trampoline_begin(arena, ctx, look_ahead_addr, target_addr, next_line_addr);

        *bp_addr = (u64)(uintptr_t)tramp_ctx.func_remote + tramp_ctx.write_pos - ctx->base_addr;

        u8 int3 = 0xcc;
        remote_func_push_bytes(&ctx->remote_func_alloc, tramp_ctx.func_local, tramp_ctx.write_pos, &int3, sizeof(int3));

        lady_trampoline_end(ctx, &tramp_ctx);
    }
}

void lady_trampoline_counter_set(Lady_Ctx* ctx, u64 look_ahead_addr, u64 target_addr, u64 next_line_addr, u64** hit_count) {
    target_addr += ctx->base_addr;
    look_ahead_addr += ctx->base_addr;
    next_line_addr += ctx->base_addr;

    Arena* arena = LaneArena();
    TempArenaBlock(arena) {
        Lady_TrampolineCtx tramp_ctx = lady_trampoline_begin(arena, ctx, look_ahead_addr, target_addr, next_line_addr);
        remote_func_push_bytes(&ctx->remote_func_alloc, tramp_ctx.func_local, tramp_ctx.write_pos, trampoline_counter, TrampolineCounterSize());
        lady_trampoline_end(ctx, &tramp_ctx);

        // set up hit count
        {
            u64 rel_hit_count_addr = tramp_ctx.write_pos;
            u64 hit_count_addr = rel_hit_count_addr + (u64)(uintptr_t)tramp_ctx.func_remote;

            u64 pre_instr_offset = target_addr - tramp_ctx.site_start_addr;
            MemoryCopy(tramp_ctx.func_local + TrampolineHitCounterAddr() + pre_instr_offset, &hit_count_addr, sizeof(u64));

            *hit_count = (u64*)(rel_hit_count_addr + (u64)(uintptr_t)tramp_ctx.func_local);

            u64 hit_count_start_val = 0;
            remote_func_push_bytes(&ctx->remote_func_alloc, tramp_ctx.func_local, tramp_ctx.write_pos, &hit_count_start_val, sizeof(hit_count_start_val));
        }
    }
}

