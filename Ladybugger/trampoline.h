#define REMOTE_FUNC_ATTRIBS __attribute__((naked, noinline))

REMOTE_FUNC_ATTRIBS
void trampoline_trap(void);

extern void __trampoline_trap(void);
extern void __trampoline_trap_stolen_bytes(void);
extern void __trampoline_trap_return_ptr(void);
extern void __trampoline_trap_end(void);

REMOTE_FUNC_ATTRIBS
void trampoline_counter(void);

extern void __trampoline_hit_count_addr(void);
extern void __trampoline_counter_end(void);

#define TrampolineHitCounterAddr() ((u64)&__trampoline_hit_count_addr - (u64)&trampoline_counter + 2) // extra bytes for movabs instruction
#define TrampolineCounterSize() ((u64)&__trampoline_counter_end - (u64)&trampoline_counter)
