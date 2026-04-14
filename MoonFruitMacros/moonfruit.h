#ifndef MOONFRUIT_H
#define MOONFRUIT_H

#include "base.h"

#define MOONFRUIT_CHUNK_SIZE 1024

typedef enum {
    MF_IGNORE = 0,
    MF_INCLUDE,
    MF_DEFINE,
    MF_UNDEF,
    MF_IF,
    MF_IFDEF,
    MF_IFNDEF,
    MF_ELIF,
    MF_ELSE,
    MF_ENDIF,
} MoonFruit_MacroType;

typedef struct {
    MoonFruit_MacroType type;
    union {
        struct {
            String file_name;
        } Include;
        struct {
            String name;
            String body;
            StringArray args;
        } Define;
        struct {
            String name;
        } Undef;
        struct {
            String condition;
        } If;
        struct {
            String name;
        } Ifdef;
        struct {
            String name;
        } Ifndef;
        struct {
            String condition;
        } Elif;
        //struct {} Else;
        //struct {} Endif;
    } expr;
} MoonFruit_Macro;

DefineArray(MoonFruit_Macro);

typedef struct {
    u64 start_line;
    MoonFruit_MacroArray macros;
} MoonFruit_PerChunkInfo;

DefineArray(MoonFruit_PerChunkInfo);

typedef struct {
    File file;
    u64 pos;
    u64 chunk_count;
    u64Array per_chunk_line_nums;
    MoonFruit_PerChunkInfoArray per_chunk_info;
} MoonFruit_File;

typedef struct {
    MoonFruit_File *file;
    String text;
    u64 pos;
    u64 per_file_chunk_idx;

    b8 first_chunk_in_file;
    b8 last_chunk_in_file;
} MoonFruit_Chunk;

typedef struct {
    MoonFruit_Chunk *chunks;
    u64 capacity;
    u64 first_idx;
    u64 last_idx;

    Mutex *mutex;
} MoonFruit_ChunkQueue;

MoonFruit_File *moonfruit_file_create_and_open(Arena *arena, String path);
void moonfruit_file_open(MoonFruit_File *f);
void moonfruit_file_close(MoonFruit_File *f);

void moonfruit_file_push_next_chunk(MoonFruit_File *f, MoonFruit_ChunkQueue *Q);

MoonFruit_ChunkQueue *moonfruit_chunk_queue_create(Arena *arena, u64 capacity);
u64 moonfruit_chunk_queue_size(MoonFruit_ChunkQueue *Q);
void moonfruit_chunk_queue_push(MoonFruit_ChunkQueue *Q, MoonFruit_Chunk chunk);
MoonFruit_Chunk moonfruit_chunk_queue_pop(MoonFruit_ChunkQueue *Q);

#define moonfruit_chunk_empty(chunk) (chunk.text.size == 0)
void moonfruit_chunk_process(Arena *arena, MoonFruit_Chunk chunk, MoonFruit_ChunkQueue *Q);

String moonfruit_macro_extract_string(u8* macro_start);
MoonFruit_Macro moonfruit_macro_init(Arena* arena, Mutex* mutex, String raw_macro);

#endif
