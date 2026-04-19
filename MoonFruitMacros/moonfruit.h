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

typedef enum {
    MF_TOKEN_IDENTIFIER,     // any sequence of letters, digits, or underscores, which begins with a letter or underscore
    MF_TOKEN_NUMBER,         // begins with optional period, a required decimal digit, and then continue with any sequence of letters, digits, underscores, periods, and exponents
    MF_TOKEN_STRING_LITERAL, // string constants, char constants, and header names (in between <>)
    MF_TOKEN_PUNCTUATOR,     // All but three of the punctuation characters in ASCII are C punctuators. The exceptions are ‘@’, ‘$’, and ‘`’. => !"#%&'()*+,-./:;<=>?[]\`{}|~
    MF_TOKEN_OTHER,          // any other single byte
} MoonFruit_TokenType;

typedef struct {
    MoonFruit_TokenType type;
    String data;
} MoonFruit_Token;

DefineArray(MoonFruit_Token);

typedef struct {
    u64 start_line;
    MoonFruit_TokenArray tokens;
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

MoonFruit_ChunkQueue* moonfruit_chunk_queue_create(Arena *arena, u64 capacity);
u64                   moonfruit_chunk_queue_size(MoonFruit_ChunkQueue *Q);
void                  moonfruit_chunk_queue_push(MoonFruit_ChunkQueue *Q, MoonFruit_Chunk chunk);
MoonFruit_Chunk       moonfruit_chunk_queue_pop(MoonFruit_ChunkQueue *Q);

internal b32  moonfruit_skip_comment(u8** c_iter, String str);
internal void moonfruit_token_identifier(u8** c_iter, String str, MoonFruit_Token* token);
internal void moonfruit_token_number(u8** c_iter, String str, MoonFruit_Token* token);
internal void moonfruit_token_string_literal(u8** c_iter, u8 end_char, String str, MoonFruit_Token* token);

void moonfruit_tokenize(MoonFruit_Chunk chunk);

#define moonfruit_chunk_empty(chunk) (chunk.text.size == 0)
void moonfruit_chunk_process(MoonFruit_Chunk chunk, MoonFruit_ChunkQueue *Q);

//String moonfruit_macro_extract_string(u8* macro_start);
//MoonFruit_Macro moonfruit_macro_init(Arena* arena, Mutex* mutex, String raw_macro);

#endif
