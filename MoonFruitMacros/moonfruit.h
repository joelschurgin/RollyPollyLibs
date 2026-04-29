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
    MF_IGNORE   = 0,
    MF_INCLUDE  = 1,
    MF_DEFINE   = 1 << 1,
    MF_UNDEF    = 1 << 2,
    MF_IF       = 1 << 3,
    MF_IFDEF    = 1 << 4,
    MF_IFNDEF   = 1 << 5,
    MF_ELIF     = 1 << 6,
    MF_ELSE     = 1 << 7,
    MF_ENDIF    = 1 << 8,
} MoonFruit_MacroType;

typedef struct {
    MoonFruit_MacroType type;
    u64 token_idx_first;
    u64 token_idx_last;
    //u64 line_num;
} MoonFruit_Macro;

DefineArray(MoonFruit_Macro);

typedef struct {
    u64 start_line;
    MoonFruit_TokenArray tokens;
    MoonFruit_MacroArray macros;
} MoonFruit_ChunkInfo;

DefineArray(MoonFruit_ChunkInfo);

typedef struct MoonFruit_File MoonFruit_File;
struct MoonFruit_File {
    MoonFruit_File* next;
    File file;
    u64 pos;
    u64 chunk_count;
    MoonFruit_ChunkInfoArray chunk_info; // wondering if this is a good place for this
};

typedef struct {
    MoonFruit_File *file;
    String text;
    u64 pos;
    u64 per_file_chunk_idx;

    b8 first_chunk_in_file;
    b8 last_chunk_in_file;
} MoonFruit_Chunk;

DefineAtomicQueue(MoonFruit_Chunk, moonfruit_chunk_queue);
#define Implement_MoonFruit_ChunkQueue ImplementAtomicQueue(MoonFruit_Chunk, moonfruit_chunk_queue)

MoonFruit_File *moonfruit_file_create_and_open(Arena *arena, String path);
void moonfruit_file_open(MoonFruit_File *f);
void moonfruit_file_close(MoonFruit_File *f);

void moonfruit_file_push_next_chunk(MoonFruit_File *f, MoonFruit_ChunkQueue *Q);

void moonfruit_chunk_align_to_line(MoonFruit_Chunk* chunk);

internal b32  moonfruit_tokenize_skip_comment(u8** c_iter, String str);
internal void moonfruit_tokenize_read_identifier(u8** c_iter, String str, MoonFruit_Token* token);
internal void moonfruit_tokenize_read_number(u8** c_iter, String str, MoonFruit_Token* token);
internal void moonfruit_tokenize_read_string_literal(u8** c_iter, u8 end_char, String str, MoonFruit_Token* token);
u64 moonfruit_tokenize(MoonFruit_Chunk chunk);

#define moonfruit_chunk_empty(chunk) (chunk.text.size == 0)
void moonfruit_chunk_process(MoonFruit_Chunk chunk, MoonFruit_ChunkQueue *chunk_Q);

typedef struct {
    u64 first_token;
    String path;
} MoonFruit_FileMarker;

DefineArray(MoonFruit_FileMarker);

typedef struct {
    String radix;
    u64 child_idx;
    u64 sibling_idx;
    u64 macro_idx;
} MoonFruit_Definition;

DefineArray(MoonFruit_Definition);

typedef MoonFruit_DefinitionArray MoonFruit_DefinitionTree;

typedef struct {
    MoonFruit_TokenArray tokens;
    MoonFruit_MacroArray macros;
    MoonFruit_FileMarkerArray file_markers;
    MoonFruit_DefinitionTree def_tree;
} MoonFruit_MacroInfo;

u64 moonfruit_definition_tree_start_idx(u8 c);

MoonFruit_MacroInfo moonfruit_macro_info_build(MoonFruit_File* f);

#endif
