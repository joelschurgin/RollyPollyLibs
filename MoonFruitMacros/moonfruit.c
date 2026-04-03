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

MoonFruit_File* moonfruit_file_create(Arena* arena, String path) {
    MoonFruit_File* f = push_struct(arena, MoonFruit_File);
    f->file = File(arena, path);
    f->pos = 0;
    return f;
}

void moonfruit_file_open(MoonFruit_File* f) {
    file_open(&f->file, FILE_READ_ONLY);
}

void moonfruit_file_close(MoonFruit_File* f) {
    file_close(&f->file);
}

void moonfruit_file_push_next_chunk(MoonFruit_File* f, MoonFruit_ChunkQueue* Q) {
    u64 file_pos  = atomic_load(&f->pos);
    u64 file_size = atomic_load(&f->file.size);

    if (file_pos < file_size) {
        u64 new_size = Min(file_size - file_pos, MOONFRUIT_CHUNK_SIZE);
        MoonFruit_Chunk next_chunk = (MoonFruit_Chunk){
            .file = f,
            .text = (String){
                .str = f->file.data + file_pos,
                .size = new_size,
            },
            .pos = 0,
            .last_chunk_in_file = (file_pos + new_size >= file_size),
        };
        atomic_add(&f->pos, new_size);
        moonfruit_chunk_queue_push(Q, next_chunk);
        return;
    }

    // this is not the right place to close the file
    // probably files should be closed after the last chunk is processed
    /*
    i64 fd = atomic_load(&f->file.fd);
    if (fd >= 0) {
        DeferBlock(mutex_lock(Q->mutex), mutex_unlock(Q->mutex)) {
            moonfruit_file_close(f);
        }
    }
    */
}
