typedef struct {
    u64 addr;
    u8 data;
} Lady_Trap;

typedef enum {
    LADY_BP_TRAP,
} Lady_BpType;

typedef struct {
    Lady_BpType type;
    union {
        Lady_Trap trap;
    };
    u64 line_info_idx;
} Lady_Bp;

typedef struct {
    u64 key;
    Lady_Bp value;
} Lady_BpHashEntry;

typedef struct {
    Lady_BpHashEntry* entries;
    u64 num_entries;
    u64 max_num_entries;
} Lady_BpHash;

Lady_BpHash lady_bp_hash_create(Arena* arena, u64 max_num_entries);

void        lady_bp_hash_insert(Lady_BpHash* bp_hash, u64 addr, Lady_Bp bp);
Lady_Bp*    lady_bp_hash_get(Lady_BpHash* bp_hash, u64 addr);
void        lady_bp_hash_remove(Lady_BpHash* bp_hash, u64 addr);
