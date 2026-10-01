REMOTE_FUNC_ATTRIBS
void trampoline_save_state(void) {
    __asm__ __volatile__ (
        ".intel_syntax noprefix\n"

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

        "__trampoline_save_state_end:\n"

        ".att_syntax\n"
    );
}

REMOTE_FUNC_ATTRIBS
void trampoline_restore_state(void) {
    __asm__ __volatile__ (
        ".intel_syntax noprefix\n"

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

        "__trampoline_restore_state_end:\n"

        ".att_syntax\n"
    );
}

REMOTE_FUNC_ATTRIBS
void trampoline_hit_count(void) {
    __asm__ __volatile__ (
        ".intel_syntax noprefix\n"

        "add qword ptr [rax], 1\n"

        "__trampoline_hit_count_end:\n"

        ".att_syntax\n"
    );
}

REMOTE_FUNC_ATTRIBS
void trampoline_spin_lock(void) {
    __asm__ __volatile__ (
        ".intel_syntax noprefix\n"

        // aquire lock
        "mov rcx, 1\n"
        "xchg qword ptr [rax], rcx\n"
 
        // wait until debugger unlocks
        ".trampoline_spin_lock_wait_loop:"
        "    pause\n"
        "    cmp qword ptr [rax], 0\n"
        "    jne .trampoline_spin_lock_wait_loop\n"

        "__trampoline_spin_lock_end:\n"

        ".att_syntax\n"
    );
}
