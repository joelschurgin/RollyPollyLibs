#include "moonfruit.h"

MoonFruit_File *moonfruit_file_create_and_open(Arena *arena, String path) {
    MoonFruit_File *f = push_struct(arena, MoonFruit_File);
    f->file = File(arena, path);
    f->pos = 0;

    moonfruit_file_open(f);

    u64 num_chunks          = CeilIntDiv(f->file.size, MOONFRUIT_CHUNK_SIZE);
    u64 num_chunks_plus_one = 1 + num_chunks;
    f->per_chunk_line_nums  = Array(arena, u64, num_chunks_plus_one);
    for (u64 i = 0; i < num_chunks_plus_one; i++) {
        f->per_chunk_line_nums.data[i] = 1;
    }

    f->per_chunk_info = Array(arena, MoonFruit_PerChunkInfo, num_chunks);
    for (u64 i = 0; i < num_chunks; i++) {
        f->per_chunk_info.data[i] = (MoonFruit_PerChunkInfo){
            .start_line = 1,
        };
    }
    return f;
}

void moonfruit_file_open(MoonFruit_File* f) {
    file_open(&f->file, FILE_READ_ONLY);
}

void moonfruit_file_close(MoonFruit_File* f) { file_close(&f->file); }

void moonfruit_file_push_next_chunk(MoonFruit_File* f, MoonFruit_ChunkQueue* Q) {
    u64 file_pos = atomic_load(&f->pos);
    u64 file_size = atomic_load(&f->file.size);

    if (file_pos < file_size) {
        u64 new_size = Min(file_size - file_pos, MOONFRUIT_CHUNK_SIZE);
        u64 old_chunk_count = atomic_fetch_add(&f->chunk_count, 1);
        MoonFruit_Chunk next_chunk = (MoonFruit_Chunk){
            .file = f,
            .text =
                (String){
                    .str  = f->file.data + file_pos,
                    .size = new_size,
                },
            .pos                 = 0,
            .per_file_chunk_idx  = old_chunk_count,
            .first_chunk_in_file = (file_pos == 0),
            .last_chunk_in_file  = (file_pos + new_size >= file_size),
        };
        atomic_fetch_add(&f->pos, new_size);
        moonfruit_chunk_queue_push(Q, next_chunk);
        return;
    }
}

MoonFruit_ChunkQueue* moonfruit_chunk_queue_create(Arena* arena, u64 capacity) {
    MoonFruit_ChunkQueue* Q = push_struct(arena, MoonFruit_ChunkQueue);
    Q->chunks = push_array(arena, MoonFruit_Chunk, capacity, true);
    Q->capacity = capacity;
    return Q;
}

u64 moonfruit_chunk_queue_size(MoonFruit_ChunkQueue *Q) {
    u64 capacity = atomic_load(&Q->capacity);
    u64 first_idx = atomic_load(&Q->first_idx);
    u64 last_idx = atomic_load(&Q->last_idx);
    return (last_idx + capacity - first_idx) % capacity;
}

void moonfruit_chunk_queue_push(MoonFruit_ChunkQueue* Q, MoonFruit_Chunk chunk) {
    MutexBlock(Q->mutex) {
        u64 size = moonfruit_chunk_queue_size(Q);
        u64 capacity = atomic_load(&Q->capacity);
        if (size + 1 >= capacity) {
            Assert(!"Not enough space in the chunk queue");
        }

        Q->chunks[Q->last_idx] = chunk;
        atomic_fetch_add(&Q->last_idx, 1);
        atomic_compare_exchange(&Q->last_idx, &capacity, 0);
    }
}

MoonFruit_Chunk moonfruit_chunk_queue_pop(MoonFruit_ChunkQueue *Q) {
    MoonFruit_Chunk chunk = (MoonFruit_Chunk){0};
    MutexBlock(Q->mutex) {
        u64 size = moonfruit_chunk_queue_size(Q);
        if (size != 0) {
            u64 capacity = atomic_load(&Q->capacity);
            chunk        = Q->chunks[Q->first_idx];
            atomic_fetch_add(&Q->first_idx, 1);
            atomic_compare_exchange(&Q->first_idx, &capacity, 0);
        }
    }
    return chunk;
}

void moonfruit_chunk_align_to_line(MoonFruit_Chunk* chunk) {
    b8 adjust_beginning_of_chunk = (!chunk->first_chunk_in_file && *(chunk->text.str - 1) != '\n');
    if (adjust_beginning_of_chunk) {
        u8 *c = chunk->text.str;
        for (u64 idx = 0; idx < chunk->text.size; idx++, c++) {
            if (*c == '\n') {
                chunk->text = string_skip(chunk->text, idx);
                break;
            }
        }
    }

    b8 adjust_end_of_chunk = (!chunk->last_chunk_in_file &&
                              chunk->text.str[chunk->text.size - 1] != '\n');
    if (adjust_end_of_chunk) {
        u8 *c = &chunk->text.str[chunk->text.size - 1];
        for (u64 idx = chunk->text.size; idx < chunk->text.size * 2; idx++, c++) {
            if (*c == '\n') {
                chunk->text.size = idx;
                break;
            }
        }
    }
}

internal b32 moonfruit_tokenize_skip_comment(u8** c_iter, String str) {
    Assert(c_iter != NULL);
    Assert(*c_iter != NULL);

    b32 skip_comment = false;

    u8* c;
    DeferBlock(c = *c_iter, *c_iter = c) {
        if (*c == '/'){
            if (*(c+1) == '/') {
                c += 2;
                for EachCharContinueUntil(c, str, (*c == '\n'));
                c++;
                skip_comment = true;
            } else if (*(c+1) == '*') {
                c += 2;
                for EachCharContinueUntil(c, str, (*c == '*' && *(c+1) == '/'));
                c += 2;
                skip_comment = true;
            }
        }
    }

    return skip_comment;
}

internal void moonfruit_tokenize_read_identifier(u8** c_iter, String str, MoonFruit_Token* token) {
    // any sequence of letters, digits, or underscores, which begins with a letter or underscore

    Assert(token != NULL);
    Assert(c_iter != NULL);
    Assert(*c_iter != NULL);

    u8* c;
    DeferBlock(c = *c_iter, *c_iter = c) {
        token->type = MF_TOKEN_IDENTIFIER;
        token->data.str = c++;
        for EachCharContinueUntil(c, str, !(char_is_alpha(*c) || char_is_digit(*c, 10) || *c == '_'));
        token->data.size = (u64)(c - token->data.str);
        c--;
    }
}

internal void moonfruit_tokenize_read_number(u8** c_iter, String str, MoonFruit_Token* token) {
    // begins with optional period, a required decimal digit, and then continue with any sequence of letters, digits, underscores, periods, and exponents

    Assert(token != NULL);
    Assert(c_iter != NULL);
    Assert(*c_iter != NULL);

    u8* c;
    DeferBlock(c = *c_iter, *c_iter = c) {
        token->type = MF_TOKEN_NUMBER;
        token->data.str = c++;
        for EachCharContinueUntil(c, str, !(char_is_alpha(*c) || char_is_digit(*c, 10) || *c == '_' || *c == '.')) {
            if ((*c == 'e' || *c == 'E' || *c == 'p' || *c == 'P') &&
                (*(c+1) == '+' || *(c+1)  == '-')) {
                c++;
            }
        }
        token->data.size = (u64)(c - token->data.str);
        c--;
    }
}

internal void moonfruit_tokenize_read_string_literal(u8** c_iter, u8 end_char, String str, MoonFruit_Token* token) {
    // string constants, char constants, and header names (in between <>)

    Assert(token != NULL);
    Assert(c_iter != NULL);
    Assert(*c_iter != NULL);

    u8* c;
    DeferBlock(c = *c_iter, *c_iter = c) {
        token->type = MF_TOKEN_NUMBER;
        token->data.str = ++c;
        for EachCharContinueUntil(c, str, *c == end_char);
        token->data.size = (u64)(c - token->data.str);
    }
}

#define MF_LETTERS_TO_U64(a, b, c, d, e, f, g, h)  (((u64)(a) << (8*0)) + \
                                                    ((u64)(b) << (8*1)) + \
                                                    ((u64)(c) << (8*2)) + \
                                                    ((u64)(d) << (8*3)) + \
                                                    ((u64)(e) << (8*4)) + \
                                                    ((u64)(f) << (8*5)) + \
                                                    ((u64)(g) << (8*6)) + \
                                                    ((u64)(h) << (8*7)))
typedef enum {
    MF_LETTERS_INCLUDE = MF_LETTERS_TO_U64('i', 'n', 'c', 'l', 'u', 'd', 'e', 0),
    MF_LETTERS_DEFINE  = MF_LETTERS_TO_U64('d', 'e', 'f', 'i', 'n', 'e',  0,  0),
    MF_LETTERS_UNDEF   = MF_LETTERS_TO_U64('u', 'n', 'd', 'e', 'f',  0,   0,  0),
    MF_LETTERS_IF      = MF_LETTERS_TO_U64('i', 'f',  0,   0,   0,   0,   0,  0),
    MF_LETTERS_IFDEF   = MF_LETTERS_TO_U64('i', 'f', 'd', 'e', 'f',  0,   0,  0),
    MF_LETTERS_IFNDEF  = MF_LETTERS_TO_U64('i', 'f', 'n', 'd', 'e', 'f',  0,  0),
    MF_LETTERS_ELIF    = MF_LETTERS_TO_U64('e', 'l', 'i', 'f',  0,   0,   0,  0),
    MF_LETTERS_ELSE    = MF_LETTERS_TO_U64('e', 'l', 's', 'e',  0,   0,   0,  0),
    MF_LETTERS_ENDIF   = MF_LETTERS_TO_U64('e', 'n', 'd', 'i', 'f',  0,   0,  0),
    MF_LETTERS_ERROR   = MF_LETTERS_TO_U64('e', 'r', 'r', 'o', 'r',  0,   0,  0),
} MF_Letters;

typedef enum {
    MF_TOKEN_STATE_NORMAL,
    MF_TOKEN_STATE_HASHTAG,
    MF_TOKEN_STATE_INCLUDE,
} MoonFruit_TokenState;
void moonfruit_tokenize(MoonFruit_Chunk chunk) {
    MoonFruit_PerChunkInfo* chunk_info = &chunk.file->per_chunk_info.data[chunk.per_file_chunk_idx];
    MoonFruit_TokenArray* token_arr = &chunk_info->tokens;
    MoonFruit_TokenState token_state = MF_TOKEN_STATE_NORMAL;
    for EachChar(c, chunk.text) {
        if (char_is_whitespace(*c)) continue;
        if (moonfruit_tokenize_skip_comment(&c, chunk.text)) continue;

        #define MoonFruit_TokenArray_Push() do { \
                        MoonFruit_Token* new_token = push_struct(LaneArena(), MoonFruit_Token); \
                        if (token_arr->count == 0) {\
                            token_arr->data = new_token;\
                            token_arr->count = 1;\
                        } else { \
                            token_arr->count += 1;\
                        }\
                    } while(0)
        #define MoonFruit_TokenArray_Last (token_arr->data[token_arr->count-1])
        if (char_is_alpha(*c) || *c == '_') {
            MoonFruit_TokenArray_Push();
            moonfruit_tokenize_read_identifier(&c, chunk.text, &MoonFruit_TokenArray_Last);
            if (token_state == MF_TOKEN_STATE_HASHTAG) {
                u64 temp = 0;
                MemoryCopy(&temp, MoonFruit_TokenArray_Last.data.str, Min(sizeof(u64), MoonFruit_TokenArray_Last.data.size));
                token_state = (temp == MF_LETTERS_INCLUDE) ? MF_TOKEN_STATE_INCLUDE : MF_TOKEN_STATE_NORMAL;
            }
        } else if (char_is_digit(*c, 10)) {
            MoonFruit_TokenArray_Push();
            moonfruit_tokenize_read_number(&c, chunk.text, &MoonFruit_TokenArray_Last);
            token_state = MF_TOKEN_STATE_NORMAL;
        } else if (*c == '"' || *c == '\'') {
            MoonFruit_TokenArray_Push();
            moonfruit_tokenize_read_string_literal(&c, *c, chunk.text, &MoonFruit_TokenArray_Last);
            token_state = MF_TOKEN_STATE_NORMAL;
        } else {
            #define MoonFruit_TokenFromPtr(token_type, ptr, num_bytes) (MoonFruit_Token){ .type=(token_type), .data = (String) { .str=(ptr), .size=(num_bytes)} }
            MoonFruit_TokenArray_Push();
            switch (*c) {
                case '{':
                case '}':
                case '[':
                case ']':
                case ',':
                case '(':
                case ')':
                case ';':
                case '~':
                case '?':
                    MoonFruit_TokenArray_Last = MoonFruit_TokenFromPtr(MF_TOKEN_PUNCTUATOR, c, 1);
                break;
                case '!':
                case '*':
                case '/':
                case '%':
                case '^':
                case '=':
                    MoonFruit_TokenArray_Last = MoonFruit_TokenFromPtr(MF_TOKEN_PUNCTUATOR, c, 1 + (*(c+1) == '='));
                break;
                case '#':
                    MoonFruit_TokenArray_Last = MoonFruit_TokenFromPtr(MF_TOKEN_PUNCTUATOR, c, 1 + (*(c+1) == *c));
                    if (MoonFruit_TokenArray_Last.data.size == 1) {
                        token_state = MF_TOKEN_STATE_HASHTAG;
                    }
                break;
                case ':':
                    MoonFruit_TokenArray_Last = MoonFruit_TokenFromPtr(MF_TOKEN_PUNCTUATOR, c, 1 + (*(c+1) == *c));
                break;
                case '+':
                case '&':
                case '|':
                    MoonFruit_TokenArray_Last = MoonFruit_TokenFromPtr(MF_TOKEN_PUNCTUATOR, c, 1 + (*(c+1) == '=' || *(c+1) == *c));
                break;
                case '-':
                    MoonFruit_TokenArray_Last = MoonFruit_TokenFromPtr(MF_TOKEN_PUNCTUATOR, c, 1 + (*(c+1) == '=' || *(c+1) == '-' || *(c+1) == '>'));
                break;
                case '<':
                    if (token_state == MF_TOKEN_STATE_INCLUDE) {
                        moonfruit_tokenize_read_string_literal(&c, '>', chunk.text, &MoonFruit_TokenArray_Last);
                        token_state = MF_TOKEN_STATE_NORMAL;
                        break;
                    }

                    MoonFruit_TokenArray_Last = MoonFruit_TokenFromPtr(MF_TOKEN_PUNCTUATOR, c, 1 + (*(c+1) == '=' || *(c+1) == '<') + (*(c+1) == '<' && *(c+2) == '='));
                break;
                case '>':
                    MoonFruit_TokenArray_Last = MoonFruit_TokenFromPtr(MF_TOKEN_PUNCTUATOR, c, 1 + (*(c+1) == '=' || *(c+1) == '>') + (*(c+1) == '>' && *(c+2) == '='));
                break;
                case '.':
                    if (char_is_digit(*(c+1), 10)) {
                        c++;
                        moonfruit_tokenize_read_number(&c, chunk.text, &MoonFruit_TokenArray_Last);
                        MoonFruit_TokenArray_Last.data.str--;
                        MoonFruit_TokenArray_Last.data.size++;
                    } else if (*(c+1) == '.' && *(c+2) == '.') {
                        MoonFruit_TokenArray_Last = MoonFruit_TokenFromPtr(MF_TOKEN_PUNCTUATOR, c, 3);
                        c += 2;
                    } else if (*(c+1) == '.' ){
                        AssertAlways(!"Invalid token that starts with .. ");
                    } else {
                        MoonFruit_TokenArray_Last = MoonFruit_TokenFromPtr(MF_TOKEN_PUNCTUATOR, c, 1);
                    }
                break;
                default:
                    MoonFruit_TokenArray_Last = MoonFruit_TokenFromPtr(MF_TOKEN_OTHER, c, 1); // any other single byte
                    token_state = MF_TOKEN_STATE_NORMAL;
                break;
            }
            #undef MoonFruit_TokenFromPtr
        }
    }
}

void moonfruit_chunk_process(MoonFruit_Chunk chunk, MoonFruit_ChunkQueue *Q) {
    if (moonfruit_chunk_empty(chunk))
        return;


    moonfruit_chunk_align_to_line(&chunk);
    moonfruit_tokenize(chunk);

    if (!chunk.last_chunk_in_file) {
        moonfruit_file_push_next_chunk(chunk.file, Q);
        return;
    }
}
