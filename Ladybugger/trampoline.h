#define REMOTE_FUNC_ATTRIBS __attribute__((naked, noinline))

REMOTE_FUNC_ATTRIBS
void trampoline_counter(void);

extern void __trampoline_hit_count_addr(void);
extern void __trampoline_counter_end(void);

#define Trampoline_HitCounterAddr() ((u64)&__trampoline_hit_count_addr - (u64)&trampoline_counter + 2) // extra bytes for movabs instruction
#define TrampolineCounter_Size() ((u64)&__trampoline_counter_end - (u64)&trampoline_counter)


REMOTE_FUNC_ATTRIBS
void trampoline_locking_mechanism(void);

extern void __trampoline_locking_mechanism_hit_count_addr(void);
extern void __trampoline_locking_mechanism_end(void);

#define TrampolineLockingMechanism_HitCounterAddr() ((u64)&__trampoline_locking_mechanism_hit_count_addr - (u64)&trampoline_locking_mechanism + 2) // extra bytes for movabs instruction
#define TrampolineLockingMechanism_LockAddr() (TrampolineLockingMechanism_HitCounterAddr() + 8)
#define TrampolineLockingMechanism_Size() ((u64)&__trampoline_locking_mechanism_end - (u64)&trampoline_locking_mechanism)



