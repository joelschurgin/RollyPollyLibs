typedef struct Thread Thread;
struct Thread {
    u64 thread_id;
};

Thread thread_launch(void *(*func)(void *), void *params);
void thread_join(Thread thread);

typedef struct ThreadArraySplit ThreadArraySplit;
struct ThreadArraySplit {
    u64 start_idx;
    u64 end_idx;
};

ThreadArraySplit thread_array_split(u64 thread_count, u64 array_size,
                                    u64 thread_idx);

typedef struct Barrier Barrier;
struct Barrier {
    pthread_barrier_t barrier_id;
};

void barrier_init(Barrier *barrier, u64 thread_count);
void barrier_release(Barrier *barrier);
void barrier_sync(Barrier *barrier);

typedef struct {
    pthread_mutex_t id;
} Mutex;

DefineArray(Mutex);

typedef Mutex* MutexPtr;
DefineArray(MutexPtr);

void mutex_assign(Mutex **mutex);
void mutex_init(Mutex *mutex);
void mutex_release(Mutex *mutex);
void mutex_lock(Mutex *mutex);
void mutex_unlock(Mutex *mutex);

#define MutexBlock(mutex_ptr)                                                  \
    DeferBlock(mutex_lock(mutex_ptr), mutex_unlock(mutex_ptr))

#define ThreadExit(ret) pthread_exit((ret))

typedef struct LaneCtx LaneCtx;
struct LaneCtx {
    Arena* arena;
    u64 lane_idx;
};

DefineArray(LaneCtx);

typedef struct ThreadCtx ThreadCtx;
struct ThreadCtx {
    Arena* shared_arena;
    LaneCtxArray lanes;
    u64 key;
    Barrier barrier;
    void* broadcast_memory;
    MutexArray mutexes;
};

extern ThreadCtx thread_ctx;

#define LaneCtx()   ((LaneCtx *)pthread_getspecific(thread_ctx.key))
#define LaneIdx()   (LaneCtx()->lane_idx)
#define LaneCount() thread_ctx.lanes.count
#define LaneSync()  barrier_sync(&thread_ctx.barrier)
#define LaneArena() (LaneCtx()->arena)

void *lane_alloc(Arena *arena, u64 num_bytes, u64 src_lane_idx);
void lane_sync_data(Arena *arena, void *data, u64 num_bytes, u64 src_lane_idx);

#define LaneSyncData(data_ptr, num_bytes, src_lane_idx)                        \
    lane_sync_data((thread_ctx.shared_arena), (data_ptr), (num_bytes),         \
                   (src_lane_idx))
#define LaneSyncStruct(data, src_lane_idx)                                     \
    LaneSyncData(&(data), sizeof(data), (src_lane_idx))
#define LaneSyncPtr(ptr, src_lane_idx) LaneSyncStruct((ptr), (src_lane_idx))

#define ThreadKeyCreate(key) pthread_key_create((pthread_key_t *)&(key), NULL)
#define ThreadKeyDelete(key) pthread_key_delete((key))

void create_parallel_entry_point(u64 num_threads, u64 num_mutexes,
                                 void *(*parallel_entry_point)(void *),
                                 void *params);

#define ThreadArraySplit(array_size)                                           \
    thread_array_split(LaneCount(), (array_size), LaneIdx())
