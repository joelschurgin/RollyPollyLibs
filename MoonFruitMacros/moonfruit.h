#ifndef MOONFRUIT_H
#define MOONFRUIT_H

#include "base.h"

#define MOONFRUIT_CHUNK_SIZE 1024

typedef struct {
    File* file;
    u64 size;
    u64 pos;
    u8* data;
} MoonFruit_Chunk;

typedef struct {
    MoonFruit_Chunk* chunks;
    u64 capacity;
    u64 first_idx;
    u64 last_idx;

    Mutex* mutex;
} MoonFruit_ChunkQueue;

MoonFruit_ChunkQueue* moonfruit_chunk_queue_create(Arena* arena, u64 capacity, u64 mutex_idx);
u64 moonfruit_chunk_queue_size(MoonFruit_ChunkQueue* Q);
void moonfruit_chunk_queue_push(MoonFruit_ChunkQueue* Q, MoonFruit_Chunk chunk);

/*
void moonfruit_process_chunk(MoonFruit_Chunk* chunk) {
    
}
*/

#endif
