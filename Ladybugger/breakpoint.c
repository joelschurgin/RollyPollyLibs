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

u64 lady_addr_to_line_info_idx(Misty_LineInfoArray line_info, u64 addr) {
    Assert(line_info.count > 0); // something went wrong way before this

    for (u64 idx = 0; idx < line_info.count - 1; idx++) {
        if (addr >= line_info.data[idx].addr && addr < line_info.data[idx + 1].addr) {
            return idx;
        }
    }

    return (line_info.data[line_info.count-1].addr == addr) ? line_info.count-1 : 0;
}

void lady_bp_set(Lady_Ctx* ctx, u64 addr, Lady_BpType type) {
    switch (type) {
        case LADY_BP_TRAP:
            lady_bp_hash_insert(&ctx->bp_hash, addr, (Lady_Bp){
                .type = type,
                .trap = lady_trap_set(ctx, addr),
                .line_info_idx = lady_addr_to_line_info_idx(ctx->line_info, addr),
            });
        break;
        case LADY_BP_FAST:
        {
            u64 target_addr = addr + ctx->base_addr;
            u64 remote_func_ptr = (u64)remote_func_alloc_push(&ctx->remote_func_alloc, trampoline, (void*)target_addr);
            insert_jmp(ctx->pid, target_addr, remote_func_ptr);
            u64 trap_offset = (u64)&__trampoline_trap_label - (u64)&trampoline;
            lady_bp_hash_insert(&ctx->bp_hash, remote_func_ptr + trap_offset - ctx->base_addr, (Lady_Bp){
                .type = LADY_BP_FAST,
                //.fast = (Lady_Fast){0},
                .line_info_idx = lady_addr_to_line_info_idx(ctx->line_info, addr),
            });
        }
        break;
        default:
            TODO("Unhandled breakpoint type");
    }
}
