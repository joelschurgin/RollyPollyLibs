ssize_t process_vm_readv(pid_t pid,
                          const struct iovec *local_iov, unsigned long liovcnt,
                          const struct iovec *remote_iov, unsigned long riovcnt,
                          unsigned long flags);
ssize_t process_vm_writev(pid_t pid,
                          const struct iovec *local_iov, unsigned long liovcnt,
                          const struct iovec *remote_iov, unsigned long riovcnt,
                          unsigned long flags);

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

    printf("process stopped!\n");
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

    ctx->pid = proc_launch_and_pause(path);
    ctx->base_addr = proc_base_addr(arena, ctx->pid);

    return ctx;
}

internal Lady_Event lady_status_to_event(i32 status) {
    if (WIFEXITED(status)) {
        return LADY_EXIT;
    } else if (WIFSIGNALED(status)) {
        return LADY_KILL;
    } else if (WIFSTOPPED(status)) {
        if (WSTOPSIG(status) == SIGTRAP) {
            return LADY_TRAP;
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

i32 remote_mprotect(pid_t pid, void* addr, size_t len, int prot) {
    struct user_regs_struct old_regs, new_regs;
 
    Assert(ptrace(PTRACE_GETREGS, pid, NULL, &old_regs) >= 0);

    u64 orig_instr = ptrace(PTRACE_PEEKDATA, pid, old_regs.rip, 0L);

    u64 syscall_payload = orig_instr;
    u8* payload_bytes = (u8*)&syscall_payload;
    payload_bytes[0] = 0x0f;
    payload_bytes[1] = 0x05;

    Assert(ptrace(PTRACE_POKEDATA, pid, old_regs.rip, syscall_payload) >= 0);

    new_regs = old_regs;

    new_regs.rax = SYS_mprotect;
    new_regs.rdi = (u64)addr;
    new_regs.rsi = len;
    new_regs.rdx = prot;

    ptrace(PTRACE_SETREGS, pid, NULL, &new_regs);

    ptrace(PTRACE_SINGLESTEP, pid, NULL, NULL);
    waitpid(pid, NULL, 0);

    ptrace(PTRACE_GETREGS, pid, NULL, &new_regs);
    i32 ret = (i32)new_regs.rax;

    Assert(ptrace(PTRACE_POKEDATA, pid, old_regs.rip, orig_instr) >= 0);
    ptrace(PTRACE_SETREGS, pid, NULL, &old_regs);

    return ret;
}

void remote_write(pid_t pid, void* remote_addr, void* write_buf, u64 size) {
    struct iovec local_iov = {
        .iov_base = write_buf,
        .iov_len = size,
    };

    struct iovec remote_iov = {
        .iov_base = remote_addr,
        .iov_len = size,
    };

    ssize_t num_bytes = process_vm_writev(pid, &local_iov, 1, &remote_iov, 1, 0);
    if (num_bytes < 0) {
        perror("remote_write");
    }
    Assert(num_bytes == (ssize_t)size);
}

void remote_read(pid_t pid, void* remote_addr, void* read_buf, u64 size) {
    struct iovec local_iov = {
        .iov_base = read_buf,
        .iov_len = size,
    };

    struct iovec remote_iov = {
        .iov_base = remote_addr,
        .iov_len = size,
    };

    ssize_t num_bytes = process_vm_readv(pid, &local_iov, 1, &remote_iov, 1, 0);
    if (num_bytes < 0) {
        perror("remote_read");
    }
    Assert(num_bytes == (ssize_t)size);
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

/*
void lady_bp_set(Lady_Ctx* ctx, u64 line_info_idx, Lady_BpType type) {
    Assert(line_info_idx < ctx->line_info.count);
    Assert(ctx->bp.count <= MAX_BREAKPOINTS - 1);

    switch (type) {
        case LADY_BP_TRAP:
            u64 bp_idx = ++ctx->bp.count;
            ctx->bp.data[bp_idx] = (Lady_Bp){
                .type = LADY_BP_TRAP,
                .trap = lady_trap_set(ctx, ctx->line_info.data[line_info_idx].addr),
            };
        break;
        default:
            TODO("Unhandled Breakpoint Type");
    }
}
*/
