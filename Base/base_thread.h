typedef struct Thread Thread;
struct Thread {
    u64 thread_id;
};

Thread thread_launch(void* (*func)(void*), void* params);
void thread_join(Thread thread);

typedef struct ThreadArraySplit ThreadArraySplit;
struct ThreadArraySplit {
    u64 start_idx;
    u64 end_idx;
};

ThreadArraySplit thread_array_split(u64 thread_count, u64 array_size, u64 thread_idx);

typedef struct Barrier Barrier;
struct Barrier {
    pthread_barrier_t barrier_id;
};

void barrier_init(Barrier *barrier, u64 thread_count);
void barrier_release(Barrier *barrier);
void barrier_sync(Barrier *barrier);

#define ThreadExit(ret) pthread_exit((ret))

typedef struct LaneCtx LaneCtx;
struct LaneCtx {
    u64 lane_idx;
};

typedef LaneCtx* LaneCtxArray;

typedef struct ThreadCtx ThreadCtx;
struct ThreadCtx {
    Arena* shared_arena;
    LaneCtxArray lanes;
    u64 key;
    Barrier barrier;
    void* broadcast_memory;
};

extern ThreadCtx thread_ctx;

#define LaneCtx()   ((LaneCtx*)pthread_getspecific(thread_ctx.key))
#define LaneIdx()   LaneCtx()->lane_idx
#define LaneCount() ArraySize(thread_ctx.lanes)
#define LaneSync()  barrier_sync(&thread_ctx.barrier)

void* lane_alloc(Arena* arena, u64 num_bytes, u64 src_lane_idx);
void lane_sync_data(Arena* arena, void* data, u64 num_bytes, u64 src_lane_idx);

#define LaneSyncData(data_ptr, num_bytes, src_lane_idx) lane_sync_data((thread_ctx.shared_arena), (data_ptr), (num_bytes), (src_lane_idx))
#define LaneSyncStruct(data, src_lane_idx) LaneSyncData(&(data), sizeof(data), (src_lane_idx))
#define LaneSyncArray(data_ptr, type, src_lane_idx) LaneSyncData((data_ptr), ArraySize(data)*sizeof(type), (src_lane_idx))

#define ThreadKeyCreate(key) pthread_key_create((pthread_key_t*)&(key), NULL)
#define ThreadKeyDelete(key) pthread_key_delete((key))

void create_parallel_entry_point(u64 num_threads, void* (*parallel_entry_point)(void*), void* params);


