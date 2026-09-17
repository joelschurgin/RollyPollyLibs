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

#define REMOTE_FUNC_ATTRIBS __attribute__((naked, noinline))
#define REMOTE_FUNC_END_PTR(func_name) Glue(func_name, _end)
#define REMOTE_FUNC_END(func_name) __attribute__((noinline)) void REMOTE_FUNC_END_PTR(func_name)(void) { __asm__ __volatile__("nop"); }

REMOTE_FUNC_ATTRIBS
void trampoline(void) {
    __asm__ __volatile__ (
        ".intel_syntax noprefix\n"
        /*
        "pushfq\n"

        "sub rsp, 128\n"
        "mov [rsp + 0],   rax\n"
        "mov [rsp + 8],   rcx\n"
        "mov [rsp + 16],  rdx\n"
        "mov [rsp + 24],  rbx\n"
        "mov [rsp + 32],  rbp\n"
        "mov [rsp + 40],  rsi\n"
        "mov [rsp + 48],  rdi\n"
        "mov [rsp + 56],  r8\n"
        "mov [rsp + 64],  r9\n"
        "mov [rsp + 72],  r10\n"
        "mov [rsp + 80],  r11\n"
        "mov [rsp + 88],  r12\n"
        "mov [rsp + 96],  r13\n"
        "mov [rsp + 104], r14\n"
        "mov [rsp + 112], r15\n"
        */

        "__trampoline_trap_label:\n"
        "int3\n"
        "nop\n"
 
        /*
        "mov rax, [rsp + 0]\n"
        "mov rcx, [rsp + 8]\n"
        "mov rdx, [rsp + 16]\n"
        "mov rbx, [rsp + 24]\n"
        "mov rbp, [rsp + 32]\n"
        "mov rsi, [rsp + 40]\n"
        "mov rdi, [rsp + 48]\n"
        "mov r8,  [rsp + 56]\n"
        "mov r9,  [rsp + 64]\n"
        "mov r10, [rsp + 72]\n"
        "mov r11, [rsp + 80]\n"
        "mov r12, [rsp + 88]\n"
        "mov r13, [rsp + 96]\n"
        "mov r14, [rsp + 104]\n"
        "mov r15, [rsp + 112]\n"
        "add rsp, 128\n"
        "popfq\n"

        */

        "__trampoline_stolen_bytes_label:\n"
        ".byte 0x00\n"
        ".long 0x00000000\n"

        "jmp qword ptr [rip + 0]\n"
        "__trampoline_return_ptr_label:\n"
        ".quad 0x9999999999999999\n" 

        ".att_syntax\n"
    );
}
REMOTE_FUNC_END(trampoline);

extern void __trampoline_trap_label(void);
extern void __trampoline_stolen_bytes_label(void);
extern void __trampoline_return_ptr_label(void);

void insert_jmp(pid_t pid, u64 addr, u64 func_ptr);

RemoteFuncAllocator remote_func_alloc_init(pid_t pid, u64 target_addr, u64 size);
void* remote_func_alloc_push_(RemoteFuncAllocator* alloc, void* func, void* func_end, void* addr);

#define remote_func_alloc_push(alloc, func, ret_addr) remote_func_alloc_push_((alloc), (func), REMOTE_FUNC_END_PTR(func), ret_addr);

#endif

