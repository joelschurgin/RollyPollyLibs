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

internal u64 _lady_look_ahead_addr(Lady_Ctx* ctx, u64 line_info_idx, u64 addr) {
    u64 num_bytes_in_line = ctx->line_info.data[line_info_idx + 1].addr - addr;
    if (num_bytes_in_line < 5) {
        return ctx->line_info.data[line_info_idx - 1].addr;
    }
    return addr;
}

internal u64 _lady_next_line_addr(Lady_Ctx* ctx, u64 line_info_idx, u64 addr) {
    return ctx->line_info.data[Min(line_info_idx+1, ctx->line_info.count-1)].addr;
}

u64 lady_bp_set(Lady_Ctx* ctx, u64 addr, Lady_BpType type) {
    switch (type) {
        case LADY_BP_TRAP:
            lady_bp_hash_insert(&ctx->bp_hash, addr, (Lady_Bp){
                .type = type,
                .trap = lady_trap_set(ctx, addr),
                .line_info_idx = lady_addr_to_line_info_idx(ctx->line_info, addr),
            });
            return addr;
        break;
        case LADY_BP_TRAMPOLINE_TRAP:
        {
            u64 bp_addr = 0;
            lady_trampoline_trap_set(ctx, addr, &bp_addr); // address that we'll get from the trampoline trap
            lady_bp_hash_insert(&ctx->bp_hash, bp_addr, (Lady_Bp){
                .type = LADY_BP_TRAMPOLINE_TRAP,
                .line_info_idx = lady_addr_to_line_info_idx(ctx->line_info, addr),
            });
            return bp_addr;
        }
        break;
        case LADY_BP_TRAMPOLINE_COUNTER:
        {
            u64* hit_count = 0;
            u64 line_info_idx = lady_addr_to_line_info_idx(ctx->line_info, addr);
            lady_trampoline_counter_set(ctx,
                                _lady_look_ahead_addr(ctx, line_info_idx, addr),
                                addr,
                                _lady_next_line_addr(ctx, line_info_idx, addr),
                                &hit_count);
            lady_bp_hash_insert(&ctx->bp_hash, addr, (Lady_Bp){
                .type = LADY_BP_TRAMPOLINE_TRAP,
                .trampoline = (Lady_Trampoline) {
                    .hit_count = hit_count,
                },
                .line_info_idx = line_info_idx,
            });
            return addr;
        }
        break;
        default:
            TODO("Unhandled breakpoint type");
            return addr;
    }
}
