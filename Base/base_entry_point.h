void cleanup();

i32 cleanup_and_pass_through_main_ret(i32 main_out) {
    cleanup();
    return main_out;
}

#if COMPILER_GCC
__attribute__((naked)) void entry_point() {
    __asm__(
        "movq (%rsp), %rdi\n" 
        "lea 8(%rsp), %rsi\n"
        "call main\n"
        "movq %rax, %rdi\n"
        "call cleanup_and_pass_through_main_ret\n"
        "movq %rax, %rdi\n"
        "movq $60, %rax\n"
        "syscall\n"
        "hlt\n"
    );
}
#else
# error no entry point defined for this compiler
#endif
