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
    u64 lane_count;
};

typedef LaneCtx* LaneCtxArray;

typedef struct ThreadCtx ThreadCtx;
struct ThreadCtx {
    LaneCtxArray lanes;
    u64 key;
    Barrier barrier;
};

extern ThreadCtx thread_ctx;

#define LaneCtx()   ((LaneCtx*)pthread_getspecific(thread_ctx.key))
#define LaneIdx()   LaneCtx()->lane_idx
#define LaneCount() LaneCtx()->lane_count //ArraySize(thread_ctx.lanes)
#define LaneSync()  barrier_sync(&thread_ctx.barrier)

#define ThreadKeyCreate(key) pthread_key_create((pthread_key_t*)&(key), NULL)
#define ThreadKeyDelete(key) pthread_key_delete((key))

void create_parallel_entry_point(u64 num_threads, void* (*parallel_entry_point)(void*), void* params);


