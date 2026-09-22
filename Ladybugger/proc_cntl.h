#ifndef LADYBUGGER_PROC_CNT_H
#define LADYBUGGER_PROC_CNT_H

#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <sys/user.h>
#include <sys/syscall.h>
#include <signal.h>

#define CHILD_PROCESS 0
internal pid_t   proc_launch_and_pause(String path);
internal u64     proc_base_addr(Arena* arena, pid_t pid);

Lady_Ctx*           lady_ctx_create(Arena* arena, String path);
void                lady_launch_process(Arena* arena, Lady_Ctx* ctx);
internal Lady_Event lady_status_to_event(i32 status);

internal i32 proc_continue(pid_t pid);
internal i32 proc_single_step(pid_t pid);

Lady_Event lady_continue(Lady_Ctx* ctx);
Lady_Event lady_single_step(Lady_Ctx* ctx);

void*   remote_mmap(pid_t pid, void* addr, size_t len, int prot, int flags, int fd, off_t offset);
i32     remote_open(pid_t pid, const char* path, int flags, mode_t mode);

internal inline u8      trap_insert(pid_t pid, u64 addr);
internal inline void    trap_restore(pid_t pid, u64 addr, u8 data);

Lady_Trap lady_trap_set(Lady_Ctx* ctx, u64 addr);
void lady_trap_unset(Lady_Ctx* ctx, Lady_Trap trap);
void lady_trap_reset(Lady_Ctx* ctx, Lady_Trap* trap);

void proc_insert_jmp(pid_t pid, u64 addr, u64 func_ptr, u8 instr_len);

RemoteFuncAllocator remote_func_alloc_init(pid_t pid, u64 target_addr, u64 size);

void lady_trampoline_trap_set(Lady_Ctx* ctx, u64 addr, u64* bp_addr);
void lady_trampoline_set(Lady_Ctx* ctx, u64 addr, u64** hit_count);

#endif

