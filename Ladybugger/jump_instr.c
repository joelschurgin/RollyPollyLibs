Lady_JmpHash lady_jmp_hash_create(Arena* arena, u64 max_num_entries) {
    Assert(IsPow2(max_num_entries));

    Lady_JmpHash jmp_hash = {0};
    jmp_hash.entries = push_array(arena, Lady_JmpHashEntry, max_num_entries, true);
    jmp_hash.max_num_entries = max_num_entries;
    return jmp_hash;
}

internal inline u64 _lady_jmp_hash_func(Lady_JmpHash* jmp_hash, u64 addr) {
    u64 start_idx = (addr & (jmp_hash->max_num_entries - 1)) + 1;
    u64 idx = start_idx;

    for (; jmp_hash->entries[idx].key != 0 && jmp_hash->entries[idx].key != addr; idx = (idx + 1) % jmp_hash->max_num_entries) {
        if (idx == start_idx - 1) return 0;
    }

    return idx;
}


void lady_jmp_hash_insert(Lady_JmpHash* jmp_hash, u64 addr, Lady_Jmp jmp) {
    u64 idx = _lady_jmp_hash_func(jmp_hash, addr);
    if (idx == 0) return;

    jmp_hash->entries[idx].key = addr;
    jmp_hash->entries[idx].value = jmp;
}

Lady_Jmp* lady_jmp_hash_get(Lady_JmpHash* jmp_hash, u64 addr) {
    u64 idx = _lady_jmp_hash_func(jmp_hash, addr);
    idx = (jmp_hash->entries[idx].key) ? idx : 0;
    return &jmp_hash->entries[idx].value;
}

void lady_jmp_hash_remove(Lady_JmpHash* jmp_hash, u64 addr) {
    u64 idx = _lady_jmp_hash_func(jmp_hash, addr);
    jmp_hash->entries[idx].key = 0;
    TODO("Untested");
}

u64 disasm_format(Arena* arena, Disasm_Instr instr, u64 addr) {
    printf("0x%08lx: ", addr);

    String mnemonic = instr.opcode == DISASM_INVALID ? String("(bad)") : disasm_opcode_format(arena, instr.opcode);
    for (u64 i = 0; i < Max(1, instr.instr_len); i++) {
        printf("%0.2x ", instr.instr[i]);
    }
    printf("\033[50G\033[32m%.*s\033[0m   \033[62G", mnemonic.size, mnemonic.str);

    for (u8 op_idx = 0; op_idx < instr.num_operands; op_idx++) {
        String operand = disasm_operand_format(arena, addr, instr, op_idx);
        printf("\033[94m%.*s\033[0m", operand.size, operand.str);
        if (op_idx != instr.num_operands - 1) printf(", ");
    }

    printf("\n");

    return Max(1, instr.instr_len);
}

u64 lady_check_jump_and_return_addr(Disasm_Instr* instr) {
    switch (instr->opcode) {
        case DISASM_CALL:
        //case DISASM_RET:
        case DISASM_RETF:
        case DISASM_SEAMCALL:
        case DISASM_SEAMRET:
        case DISASM_UIRET:
        case DISASM_JMP:
        case DISASM_JMPABS:
        case DISASM_JB:
        case DISASM_JBE:
        case DISASM_JCXZ:
        case DISASM_JECXZ:
        case DISASM_JRCXZ:
        case DISASM_JKNZD:
        case DISASM_JKZD:
        case DISASM_JL:
        case DISASM_JLE:
        case DISASM_JNB:
        case DISASM_JNBE:
        case DISASM_JNL:
        case DISASM_JNLE:
        case DISASM_JNO:
        case DISASM_JNP:
        case DISASM_JNS:
        case DISASM_JNZ:
        case DISASM_JO:
        case DISASM_JP:
        case DISASM_JS:
        case DISASM_JZ:
        case DISASM_LOOP:
        case DISASM_LOOPE:
        case DISASM_LOOPNE:
        case DISASM_ERETS:
        case DISASM_ERETU:
        case DISASM_INT:
        case DISASM_INT1:
        case DISASM_INT3:
        case DISASM_INTO:
        case DISASM_IRET:
        case DISASM_IRETQ:
        case DISASM_IRETW:
        case DISASM_RSM:
        case DISASM_SYSCALL:
        case DISASM_SYSENTER:
        case DISASM_SYSEXIT:
        case DISASM_SYSRET:
        case DISASM_TDCALL:
        case DISASM_VMCALL:
        case DISASM_VMLAUNCH:
        case DISASM_VMRESUME:
        case DISASM_VMMCALL:
            return 1;
        default:
            return 0;
    }
}
