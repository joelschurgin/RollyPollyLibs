typedef struct {
    void* remote_base;
    void* base;
    u64 pos;
    u64 size;
    i32 remote_shm_fd;
    i32 shm_fd;
} RemoteFuncAllocator;

typedef enum {
    LADY_NONE,
    LADY_EXIT,
    LADY_KILL,
    LADY_TRAP,
    LADY_SEGFAULT,
    LADY_SIGILL,
} Lady_Event;

typedef struct {
    u64 addr;
    u8 data;
} Lady_Trap;

typedef struct {
    u64* hit_count;
} Lady_Trampoline;

typedef enum {
    LADY_BP_TRAP,
    LADY_BP_TRAMPOLINE_TRAP,
    LADY_BP_TRAMPOLINE_COUNTER,
} Lady_BpType;

typedef struct {
    Lady_BpType type;
    union {
        Lady_Trap trap;
        Lady_Trampoline trampoline;
    };
    u64 line_info_idx;
    u64 hit_count;
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
    u64 dest_addr;
} Lady_Jmp;

typedef struct {
    u64 key;
    Lady_Jmp value;
} Lady_JmpHashEntry;

typedef struct {
    Lady_JmpHashEntry* entries;
    u64 num_entries;
    u64 max_num_entries;
} Lady_JmpHash;

typedef struct {
    pid_t pid;
    u64 base_addr;
    i32 mem_fd;

    String path;

    i32 pipe_read;
    i32 pipe_write;

    Misty_LineInfoArray line_info;
    Lady_BpHash bp_hash;
    Lady_JmpHash jmp_hash;

    RemoteFuncAllocator remote_func_alloc;

    Mutex* mutex;
} Lady_Ctx;
