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

internal void thread_ctx_create(Arena* arena, u64 num_threads, ThreadCtx* ctx) {
    ctx->lanes = Array(arena, num_threads, LaneCtx);
    for (u64 lane_idx = 0; lane_idx < num_threads; lane_idx++) {
        ctx->lanes[lane_idx].lane_idx = lane_idx;
        ctx->lanes[lane_idx].lane_count = num_threads;
    }

    ThreadKeyCreate(ctx->key);
    barrier_init(&ctx->barrier, num_threads);
}

internal void thread_ctx_delete(ThreadCtx ctx) {
    ThreadKeyDelete(ctx.key);
    barrier_release(&ctx.barrier);
}

internal void thread_ctx_assign(ThreadCtx ctx, u64 thread_idx) {
    pthread_setspecific(ctx.key, &ctx.lanes[thread_idx]);
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

void create_parallel_entry_point(u64 num_threads, void* (*parallel_entry_point)(void*), void* params) {
    Arena* arena = default_arena();

    DeferBlock({
        thread_ctx_create(arena, num_threads, &thread_ctx);
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
