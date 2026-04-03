#include "moonfruit.h"

MoonFruit_ChunkQueue* moonfruit_chunk_queue_create(Arena* arena, u64 capacity, u64 mutex_idx) {
    MoonFruit_ChunkQueue* Q = push_struct(arena, MoonFruit_ChunkQueue);
    Q->chunks = push_array(arena, MoonFruit_Chunk, capacity, true);
    Q->capacity = capacity;
    Q->mutex = &thread_ctx.mutexes.data[mutex_idx];
    return Q;
}

u64 moonfruit_chunk_queue_size(MoonFruit_ChunkQueue* Q) {
    u64 capacity = atomic_load(&Q->capacity);
    u64 first_idx = atomic_load(&Q->first_idx);
    u64 last_idx = atomic_load(&Q->last_idx);
    return (last_idx + capacity - first_idx) % capacity;
}

void moonfruit_chunk_queue_push(MoonFruit_ChunkQueue* Q, MoonFruit_Chunk chunk) {
    mutex_lock(Q->mutex);
    u64 size = moonfruit_chunk_queue_size(Q);
    u64 capacity = atomic_load(&Q->capacity);
    if (size >= capacity) {
        Assert(!"Not enough space in the chunk queue");
    }

    atomic_add(&Q->last_idx, 1);
    atomic_compare_exchange(&Q->last_idx, &capacity, 0);

    Q->chunks[Q->last_idx] = chunk;
    mutex_unlock(Q->mutex);
}
