Lady_JmpHash lady_jmp_hash_create(Arena* arena, u64 max_num_entries);

void         lady_jmp_hash_insert(Lady_JmpHash* jmp_hash, u64 addr, Lady_Jmp jmp);
Lady_Jmp*    lady_jmp_hash_get(Lady_JmpHash* jmp_hash, u64 addr);
void         lady_jmp_hash_remove(Lady_JmpHash* jmp_hash, u64 addr);

u64 disasm_format(Arena* arena, Disasm_Instr instr, u64 addr);
u64 lady_check_jump_and_return_addr(Disasm_Instr* instr);
