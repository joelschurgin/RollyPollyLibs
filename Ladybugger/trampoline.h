#define REMOTE_FUNC_ATTRIBS __attribute__((naked, noinline))

REMOTE_FUNC_ATTRIBS void trampoline_save_state(void);
REMOTE_FUNC_ATTRIBS void trampoline_restore_state(void);
REMOTE_FUNC_ATTRIBS void trampoline_hit_count(void);
REMOTE_FUNC_ATTRIBS void trampoline_spin_lock(void);

extern void __trampoline_save_state_end(void);
extern void __trampoline_restore_state_end(void);
extern void __trampoline_hit_count_end(void);
extern void __trampoline_spin_lock_end(void);

