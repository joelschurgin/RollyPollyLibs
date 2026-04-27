AtomicQueue* atomic_queue_create(Arena *arena, u64 capacity, u64 element_size) {
    AtomicQueue* Q = push_struct(arena, AtomicQueue);
    Q->data = arena_push(arena, element_size * capacity, element_size, true);
    Q->capacity = capacity;
    Q->element_size = element_size;
    return Q;
}

u64 atomic_queue_size(AtomicQueue *Q) {
    u64 capacity = atomic_load(&Q->capacity);
    u64 first_idx = atomic_load(&Q->first_idx);
    u64 last_idx = atomic_load(&Q->last_idx);
    return (last_idx + capacity - first_idx) % capacity;
}

void atomic_queue_push(AtomicQueue *Q, void* data_in) {
    MutexBlock(Q->mutex) {
        u64 size = atomic_queue_size(Q);
        u64 capacity = atomic_load(&Q->capacity);
        if (size + 1 >= capacity) {
            Assert(!"Not enough space in the queue");
        }

        MemoryCopy(((u8*)Q->data + Q->element_size * Q->last_idx), data_in, Q->element_size);
        atomic_fetch_add(&Q->last_idx, 1);
        atomic_compare_exchange(&Q->last_idx, &capacity, 0);
    }
}

void atomic_queue_pop(AtomicQueue *Q, void* data_out) {
    MutexBlock(Q->mutex) {
        u64 size = atomic_queue_size(Q);
        if (size != 0) {
            u64 capacity = atomic_load(&Q->capacity);
            MemoryCopy(data_out, ((u8*)Q->data + Q->element_size * Q->first_idx), Q->element_size);
            atomic_fetch_add(&Q->first_idx, 1);
            atomic_compare_exchange(&Q->first_idx, &capacity, 0);
        }
    }
}
