Lady_BpHash lady_bp_hash_create(Arena* arena, u64 max_num_entries);

void        lady_bp_hash_insert(Lady_BpHash* bp_hash, u64 addr, Lady_Bp bp);
Lady_Bp*    lady_bp_hash_get(Lady_BpHash* bp_hash, u64 addr);
void        lady_bp_hash_remove(Lady_BpHash* bp_hash, u64 addr);

u64         lady_bp_set(Lady_Ctx* ctx, u64 addr, Lady_BpType type);
