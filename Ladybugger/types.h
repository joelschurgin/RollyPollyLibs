typedef struct {
    void* base;
    u64 pos;
    u64 size;
    pid_t pid;
} RemoteFuncAllocator;

typedef enum {
    LADY_NONE,
    LADY_EXIT,
    LADY_KILL,
    LADY_TRAP,
    LADY_SEGFAULT,
} Lady_Event;

typedef struct {
    u64 addr;
    u8 data;
} Lady_Trap;

typedef struct {
    //u64 proc_addr;
    //u64 trampoline_addr;
    //u8 data[5];
} Lady_Fast;

typedef enum {
    LADY_BP_TRAP,
    LADY_BP_TRAMPOLINE_TRAP,
} Lady_BpType;

typedef struct {
    Lady_BpType type;
    union {
        Lady_Trap trap;
        //Lady_Fast fast;
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

typedef struct {
    pid_t pid;
    u64 base_addr;

    i32 pipe_read;
    i32 pipe_write;

    Misty_LineInfoArray line_info;
    Lady_BpHash bp_hash;

    RemoteFuncAllocator remote_func_alloc;
} Lady_Ctx;
