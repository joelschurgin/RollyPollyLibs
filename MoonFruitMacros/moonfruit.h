#ifndef MOONFRUIT_H
#define MOONFRUIT_H

#include "base.h"

#define MOONFRUIT_CHUNK_SIZE 1024

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
    u64 token_idx_first;
    u64 token_idx_last;
    //u64 line_num;
} MoonFruit_Macro;

/*
typedef struct MoonFruit_DefinitionTreeData MoonFruit_DefinitionTreeData;
struct MoonFruit_DefinitionTreeData {
    MoonFruit_DefinitionTreeData* next;
    u64 data;
};

typedef struct MoonFruit_DefinitionTreeNode MoonFruit_DefinitionTreeNode;
struct MoonFruit_DefinitionTreeNode {
    MoonFruit_DefinitionTreeNode* child;
    MoonFruit_DefinitionTreeNode* sibling;

    String letters;

    MoonFruit_DefinitionTreeData* data;
};

typedef struct {
    MoonFruit_DefinitionTreeNode*
} MoonFruit_DefinitionTree;
*/

DefineArray(MoonFruit_Macro);

typedef struct {
    u64 start_line;
    MoonFruit_TokenArray tokens;
    MoonFruit_MacroArray macros;
    //MoonFruit_DefinitionTree;
} MoonFruit_PerChunkInfo;

DefineArray(MoonFruit_PerChunkInfo);

typedef struct {
    File file;
    u64 pos;
    u64 chunk_count;
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

void moonfruit_chunk_align_to_line(MoonFruit_Chunk* chunk);

internal b32  moonfruit_tokenize_skip_comment(u8** c_iter, String str);
internal void moonfruit_tokenize_read_identifier(u8** c_iter, String str, MoonFruit_Token* token);
internal void moonfruit_tokenize_read_number(u8** c_iter, String str, MoonFruit_Token* token);
internal void moonfruit_tokenize_read_string_literal(u8** c_iter, u8 end_char, String str, MoonFruit_Token* token);
u64 moonfruit_tokenize(MoonFruit_Chunk chunk);

#define moonfruit_chunk_empty(chunk) (chunk.text.size == 0)
void moonfruit_chunk_process(MoonFruit_Chunk chunk, MoonFruit_ChunkQueue *Q);

#endif
