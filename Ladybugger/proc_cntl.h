#ifndef LADYBUGGER_PROC_CNT_H
#define LADYBUGGER_PROC_CNT_H

#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <sys/user.h>
#include <sys/syscall.h>
#include <signal.h>

#define MAX_BREAKPOINTS 256

typedef struct {
    u64 addr;
    u8 data;
} Lady_Trap;

typedef enum {
    LADY_BP_TRAP,
} Lady_BpType;

typedef struct {
    Lady_BpType type;
    union {
        Lady_Trap trap;
    };
    u64 line_info_idx;
} Lady_Bp;

DefineArray(Lady_Bp);

typedef struct {
    pid_t pid;
    u64 base_addr;

    i32 pipe_read;
    i32 pipe_write;

    Misty_LineInfoArray line_info;
    Lady_BpArray bp;
} Lady_Ctx;

typedef enum {
    LADY_NONE,
    LADY_EXIT,
    LADY_KILL,
    LADY_TRAP,
} Lady_Event;

#define CHILD_PROCESS 0
internal pid_t   proc_launch_and_pause(String path);
internal u64     proc_base_addr(Arena* arena, pid_t pid);

Lady_Ctx* lady_ctx_create(Arena* arena, String path);
internal Lady_Event lady_status_to_event(i32 status);

internal i32 proc_continue(pid_t pid);
internal i32 proc_single_step(pid_t pid);

Lady_Event lady_continue(Lady_Ctx* ctx);
Lady_Event lady_single_step(Lady_Ctx* ctx);

void*   remote_mmap(pid_t pid, void* addr, size_t len, int prot, int flags, int fd, off_t offset);
i32     remote_mprotect(pid_t pid, void* addr, size_t len, int prot);
void    remote_write(pid_t pid, void* remote_addr, void* write_buf, u64 size);
void    remote_read(pid_t pid, void* remote_addr, void* read_buf, u64 size);

internal inline u8      trap_insert(pid_t pid, u64 addr);
internal inline void    trap_restore(pid_t pid, u64 addr, u8 data);

Lady_Trap lady_trap_set(Lady_Ctx* ctx, u64 addr);
void lady_trap_unset(Lady_Ctx* ctx, Lady_Trap trap);
void lady_trap_reset(Lady_Ctx* ctx, Lady_Trap* trap);

void lady_bp_set(Lady_Ctx* ctx, u64 line_info_idx, Lady_BpType type);

#endif

