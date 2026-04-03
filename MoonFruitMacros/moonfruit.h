#ifndef MOONFRUIT_H
#define MOONFRUIT_H

#include "base.h"

#define MOONFRUIT_CHUNK_SIZE 1024

typedef struct {
    File file;
    u64 pos;
} MoonFruit_File;

typedef struct {
    MoonFruit_File* file;
    String text;
    u64 pos;
    b8 last_chunk_in_file;
} MoonFruit_Chunk;

typedef struct {
    MoonFruit_Chunk* chunks;
    u64 capacity;
    u64 first_idx;
    u64 last_idx;

    Mutex* mutex;
} MoonFruit_ChunkQueue;

MoonFruit_ChunkQueue* moonfruit_chunk_queue_create(Arena* arena, u64 capacity);
u64 moonfruit_chunk_queue_size(MoonFruit_ChunkQueue* Q);
void moonfruit_chunk_queue_push(MoonFruit_ChunkQueue* Q, MoonFruit_Chunk chunk);
MoonFruit_Chunk moonfruit_chunk_queue_pop(MoonFruit_ChunkQueue* Q);

#define moonfruit_chunk_empty(chunk) (chunk.text.size == 0)

MoonFruit_File* moonfruit_file_create(Arena* arena, String path);
void moonfruit_file_open(MoonFruit_File* f);
void moonfruit_file_close(MoonFruit_File* f);

void moonfruit_file_push_next_chunk(MoonFruit_File* f, MoonFruit_ChunkQueue* Q);

#endif
