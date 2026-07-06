ThreadCtx thread_ctx = {0};

Thread thread_launch(void* (*func)(void*), void* params) {
    Thread thread = (Thread){0};
    i32 err = pthread_create(&thread.thread_id, NULL, func, params);
    if (err != 0) {
        return (Thread){0};
    }
    return thread;
}

void thread_join(Thread thread) {
    pthread_join(thread.thread_id, NULL);
}

ThreadArraySplit thread_array_split(u64 thread_count, u64 array_size, u64 thread_idx) {
    u64 values_per_thread = array_size / thread_count;
    u64 leftover_values_count = array_size % thread_count;
    b32 thread_has_leftover = (thread_idx < leftover_values_count);
    u64 leftovers_before_this_thread_idx = (thread_has_leftover
                                            ? thread_idx
                                            : leftover_values_count);

    ThreadArraySplit split;
    split.start_idx = (values_per_thread * thread_idx + leftovers_before_this_thread_idx);
    split.end_idx = (split.start_idx + values_per_thread + !!thread_has_leftover);
    return split;
}

void barrier_init(Barrier *barrier, u64 thread_count) {
    i32 err = pthread_barrier_init(&barrier->barrier_id, NULL, thread_count);
    if (err != 0) {
        *barrier = (Barrier){0};
    }
}

void barrier_release(Barrier *barrier) {
    pthread_barrier_destroy(&barrier->barrier_id);
}

void barrier_sync(Barrier *barrier) {
    pthread_barrier_wait(&barrier->barrier_id);
}

void mutex_assign(Mutex** mutex) {
    local_persist u64 idx = 0;
    AssertAlways(idx < thread_ctx.mutexes.count);
    *mutex = &thread_ctx.mutexes.data[idx++];
}

void mutex_init(Mutex* mutex) {
    pthread_mutex_init(&mutex->id, NULL);
}

void mutex_release(Mutex* mutex) {
    pthread_mutex_destroy(&mutex->id);
}

void mutex_lock(Mutex* mutex) {
    pthread_mutex_lock(&mutex->id);
}

void mutex_unlock(Mutex* mutex) {
    pthread_mutex_unlock(&mutex->id);
}

internal void thread_ctx_create(Arena* arena, u64 num_threads, u64 num_mutexes, ThreadCtx* ctx) {
    ctx->lanes = Array(arena, LaneCtx, num_threads);
    for (u64 lane_idx = 0; lane_idx < num_threads; lane_idx++) {
        ctx->lanes.data[lane_idx].lane_idx = lane_idx;
        ctx->lanes.data[lane_idx].arena = arena_alloc(MB(1), KB(1));
    }

    ThreadKeyCreate(ctx->key);
    barrier_init(&ctx->barrier, num_threads);
    ctx->shared_arena = arena;

    ctx->mutexes.data = push_array(arena, Mutex, num_mutexes, false);
    ctx->mutexes.count = num_mutexes;
    for (u64 mutex_idx = 0; mutex_idx < num_mutexes; mutex_idx++) {
        mutex_init(&ctx->mutexes.data[mutex_idx]);
    }
}

internal void thread_ctx_delete(ThreadCtx ctx) {
    ThreadKeyDelete(ctx.key);
    barrier_release(&ctx.barrier);
    for (u64 mutex_idx = 0; mutex_idx < ctx.mutexes.count; mutex_idx++) {
        mutex_release(&ctx.mutexes.data[mutex_idx]);
    }
}

internal void thread_ctx_assign(ThreadCtx ctx, u64 thread_idx) {
    pthread_setspecific(ctx.key, &ctx.lanes.data[thread_idx]);
}

typedef struct SetupThreadLocals SetupThreadLocals;
struct SetupThreadLocals {
    u64 thread_idx;
    void* (*parallel_entry_point)(void*);
    void* params;
};

internal void* setup_thread_locals(void* params) {
    SetupThreadLocals* setup = (SetupThreadLocals*)params;
    thread_ctx_assign(thread_ctx, setup->thread_idx);
    return setup->parallel_entry_point(setup->params);
}

void* lane_alloc(Arena* arena, u64 num_bytes, u64 src_lane_idx) {
    void* data = 0L;
    if (LaneIdx() == src_lane_idx) {
        data = arena_push(arena, num_bytes * LaneCount(), 1, false);
    }
    LaneSync();

    return data + (num_bytes * LaneIdx());
}

void lane_sync_data(Arena* arena, void* data, u64 num_bytes, u64 src_lane_idx) {
    if (LaneIdx() == src_lane_idx) {
        thread_ctx.broadcast_memory = arena_push(arena, num_bytes, 1, true);
        MemoryCopy(thread_ctx.broadcast_memory, data, num_bytes);
        arena_pop(arena, num_bytes);
    }
    LaneSync();
    if (LaneIdx() != src_lane_idx) {
        MemoryCopy(data, thread_ctx.broadcast_memory, num_bytes);
    }
    LaneSync();
}

void create_parallel_entry_point(u64 num_threads, u64 num_mutexes, void* (*parallel_entry_point)(void*), void* params) {
    Arena* arena = default_arena();

    DeferBlock({
        thread_ctx_create(arena, num_threads, num_mutexes, &thread_ctx);
    }, {
        thread_ctx_delete(thread_ctx);
    }) {
        Thread threads[num_threads];
        SetupThreadLocals setups[num_threads];
        for (u64 thread_idx = 0; thread_idx < num_threads; thread_idx++) {
            setups[thread_idx] = (SetupThreadLocals){
                .thread_idx = thread_idx,
                .parallel_entry_point = parallel_entry_point,
                .params = params,
            };
            threads[thread_idx] = thread_launch(setup_thread_locals, (void*)&setups[thread_idx]);
        }
        for (u64 thread_idx = 0; thread_idx < num_threads; thread_idx++) {
            thread_join(threads[thread_idx]);
        }
    }

    arena_clear(arena);
}
