Lady_BpHash lady_bp_hash_create(Arena* arena, u64 max_num_entries) {
    Assert(IsPow2(max_num_entries));

    Lady_BpHash bp_hash = {0};
    bp_hash.entries = push_array(arena, Lady_BpHashEntry, max_num_entries, true);
    bp_hash.max_num_entries = max_num_entries;
    return bp_hash;
}

internal inline u64 _lady_bp_hash_func(Lady_BpHash* bp_hash, u64 addr) {
    u64 start_idx = (addr & (bp_hash->max_num_entries - 1)) + 1;
    u64 idx = start_idx;

    for (; bp_hash->entries[idx].key != 0 && bp_hash->entries[idx].key != addr; idx = (idx + 1) % bp_hash->max_num_entries) {
        if (idx == start_idx - 1) return 0;
    }

    return idx;
}

void lady_bp_hash_insert(Lady_BpHash* bp_hash, u64 addr, Lady_Bp bp) {
    u64 idx = _lady_bp_hash_func(bp_hash, addr);
    if (idx == 0) return;

    bp_hash->entries[idx].key = addr;
    bp_hash->entries[idx].value = bp;
}

Lady_Bp* lady_bp_hash_get(Lady_BpHash* bp_hash, u64 addr) {
    u64 idx = _lady_bp_hash_func(bp_hash, addr);
    return &bp_hash->entries[idx].value;
}

void lady_bp_hash_remove(Lady_BpHash* bp_hash, u64 addr) {
    u64 idx = _lady_bp_hash_func(bp_hash, addr);
    bp_hash->entries[idx].key = 0;
    TODO("Untested");
}
