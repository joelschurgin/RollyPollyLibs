#ifndef LADYBUGGER_PROC_CNT_H
#define LADYBUGGER_PROC_CNT_H

#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <sys/user.h>
#include <sys/syscall.h>
#include <signal.h>

typedef struct {
    i32 pipe_read;
    i32 pipe_write;
    u64 target_loop_counter_address;
} LadybuggerCtx;

LadybuggerCtx* ladybugger_ctx = 0L;

#define CHILD_PROCESS 0
pid_t   proc_launch_and_pause(String path);
u64     proc_base_addr(Arena* arena, pid_t pid);

i32     proc_continue(pid_t pid);
i32     proc_single_step(pid_t pid);

void*   remote_mmap(pid_t pid, void* addr, size_t len, int prot, int flags, int fd, off_t offset);
i32     remote_mprotect(pid_t pid, void* addr, size_t len, int prot);
void    remote_write(pid_t pid, void* remote_addr, void* write_buf, u64 size);
void    remote_read(pid_t pid, void* remote_addr, void* read_buf, u64 size);

u8      trap_insert(pid_t pid, u64 addr);
void    trap_restore(pid_t pid, u64 addr, u8 data);

#endif

