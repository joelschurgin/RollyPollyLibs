#define REMOTE_FUNC_ATTRIBS __attribute__((naked, noinline))
#define REMOTE_FUNC_END_PTR(func_name) Glue(func_name, _end)
#define REMOTE_FUNC_END(func_name) __attribute__((noinline)) void REMOTE_FUNC_END_PTR(func_name)(void) { __asm__ __volatile__("nop"); }

REMOTE_FUNC_ATTRIBS
void trampoline_trap(void);

extern void __trampoline_trap(void);
extern void __trampoline_trap_stolen_bytes(void);
extern void __trampoline_trap_return_ptr(void);


