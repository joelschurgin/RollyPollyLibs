ssize_t process_vm_readv(pid_t pid,
                          const struct iovec *local_iov, unsigned long liovcnt,
                          const struct iovec *remote_iov, unsigned long riovcnt,
                          unsigned long flags);
ssize_t process_vm_writev(pid_t pid,
                          const struct iovec *local_iov, unsigned long liovcnt,
                          const struct iovec *remote_iov, unsigned long riovcnt,
                          unsigned long flags);

#define CHILD_PROCESS 0
pid_t proc_launch_and_pause(String path) {
    if (!ladybugger_ctx) {
        ladybugger_ctx = push_struct(thread_ctx.shared_arena, LadybuggerCtx);
    } else {
        *ladybugger_ctx = (LadybuggerCtx){0};
    }

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

u64 proc_base_addr(Arena* arena, pid_t pid) {
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

i32 proc_continue(pid_t pid) {
    i32 status = 0;
    if (ptrace(PTRACE_CONT, pid, 0L, 0L) < 0) {
        perror("Cannot continue process!\n");
    }
    waitpid(pid, &status, 0);

    return status;
}

i32 proc_single_step(pid_t pid) {
    i32 status = 0;
    if (ptrace(PTRACE_SINGLESTEP, pid, 0L, 0L) < 0) {
        perror("Cannot single step process!\n");
    }
    waitpid(pid, &status, 0);

    return status;
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

u8 trap_insert(pid_t pid, u64 addr) {
    u64 curr_instr = ptrace(PTRACE_PEEKDATA, pid, addr, 0L);
    u64 trap_instr = (curr_instr & ~0xff) | 0xcc;
    ptrace(PTRACE_POKEDATA, pid, addr, trap_instr);
    return curr_instr & 0xff;
}

void trap_restore(pid_t pid, u64 addr, u8 data) {
    u64 curr_instr = ptrace(PTRACE_PEEKDATA, pid, addr, 0L);
    u64 prev_instr = (curr_instr & ~0xff) | data;
    ptrace(PTRACE_POKEDATA, pid, addr, prev_instr);
}
