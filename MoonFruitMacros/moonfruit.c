#include "moonfruit.h"

MoonFruit_ChunkQueue* moonfruit_chunk_queue_create(Arena* arena, u64 capacity) {
    MoonFruit_ChunkQueue* Q = push_struct(arena, MoonFruit_ChunkQueue);
    Q->chunks = push_array(arena, MoonFruit_Chunk, capacity, true);
    Q->capacity = capacity;
    return Q;
}

u64 moonfruit_chunk_queue_size(MoonFruit_ChunkQueue* Q) {
    u64 capacity = atomic_load(&Q->capacity);
    u64 first_idx = atomic_load(&Q->first_idx);
    u64 last_idx = atomic_load(&Q->last_idx);
    return (last_idx + capacity - first_idx) % capacity;
}

void moonfruit_chunk_queue_push(MoonFruit_ChunkQueue* Q, MoonFruit_Chunk chunk) {
    DeferBlock(mutex_lock(Q->mutex), mutex_unlock(Q->mutex)) {
        u64 size = moonfruit_chunk_queue_size(Q);
        u64 capacity = atomic_load(&Q->capacity);
        if (size+1 >= capacity) {
            Assert(!"Not enough space in the chunk queue");
        }

        Q->chunks[Q->last_idx] = chunk;
        atomic_add(&Q->last_idx, 1);
        atomic_compare_exchange(&Q->last_idx, &capacity, 0);
    }
}

MoonFruit_Chunk moonfruit_chunk_queue_pop(MoonFruit_ChunkQueue* Q) {
    MoonFruit_Chunk chunk = (MoonFruit_Chunk){0};
    DeferBlock(mutex_lock(Q->mutex), mutex_unlock(Q->mutex)) {
        u64 size = moonfruit_chunk_queue_size(Q);
        if (size != 0) {
            u64 capacity = atomic_load(&Q->capacity);
            chunk = Q->chunks[Q->first_idx];
            atomic_add(&Q->first_idx, 1);
            atomic_compare_exchange(&Q->first_idx, &capacity, 0);
        }
    }
    return chunk;
}
