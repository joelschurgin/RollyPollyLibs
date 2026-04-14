#include "moonfruit.h"

MoonFruit_File *moonfruit_file_create_and_open(Arena *arena, String path) {
    MoonFruit_File *f = push_struct(arena, MoonFruit_File);
    f->file           = File(arena, path);
    f->pos            = 0;

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

void moonfruit_file_open(MoonFruit_File *f) {
    file_open(&f->file, FILE_READ_ONLY);
}

void moonfruit_file_close(MoonFruit_File *f) { file_close(&f->file); }

void moonfruit_file_push_next_chunk(MoonFruit_File       *f,
                                    MoonFruit_ChunkQueue *Q) {
    u64 file_pos  = atomic_load(&f->pos);
    u64 file_size = atomic_load(&f->file.size);

    if (file_pos < file_size) {
        u64 new_size        = Min(file_size - file_pos, MOONFRUIT_CHUNK_SIZE);
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

MoonFruit_ChunkQueue *moonfruit_chunk_queue_create(Arena *arena, u64 capacity) {
    MoonFruit_ChunkQueue *Q = push_struct(arena, MoonFruit_ChunkQueue);
    Q->chunks   = push_array(arena, MoonFruit_Chunk, capacity, true);
    Q->capacity = capacity;
    return Q;
}

u64 moonfruit_chunk_queue_size(MoonFruit_ChunkQueue *Q) {
    u64 capacity  = atomic_load(&Q->capacity);
    u64 first_idx = atomic_load(&Q->first_idx);
    u64 last_idx  = atomic_load(&Q->last_idx);
    return (last_idx + capacity - first_idx) % capacity;
}

void moonfruit_chunk_queue_push(MoonFruit_ChunkQueue *Q,
                                MoonFruit_Chunk       chunk) {
    MutexBlock(Q->mutex) {
        u64 size     = moonfruit_chunk_queue_size(Q);
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

void moonfruit_chunk_process(Arena *arena, MoonFruit_Chunk chunk, MoonFruit_ChunkQueue *Q) {
    if (moonfruit_chunk_empty(chunk))
        return;

    b8 adjust_beginning_of_chunk = (!chunk.first_chunk_in_file && *(chunk.text.str - 1) != '\n');
    if (adjust_beginning_of_chunk) {
        u8 *c = chunk.text.str;
        for (u64 idx = 0; idx < chunk.text.size; idx++, c++) {
            if (*c == '\n') {
                chunk.text = string_skip(chunk.text, idx);
                break;
            }
        }
    }

    b8 adjust_end_of_chunk = (!chunk.last_chunk_in_file &&
                              chunk.text.str[chunk.text.size - 1] != '\n');
    if (adjust_end_of_chunk) {
        u8 *c = &chunk.text.str[chunk.text.size - 1];
        for (u64 idx = chunk.text.size; idx < chunk.text.size * 2; idx++, c++) {
            if (*c == '\n') {
                chunk.text.size = idx;
                break;
            }
        }
    }

    u8 macro_starts[MOONFRUIT_CHUNK_SIZE / 8] = {0};

    u64 num_lines  = 0;
    u64 num_macros = 0;
    {
        u8 *c = &chunk.text.str[0];
        for (u64 idx = 0; idx < chunk.text.size-1; idx++, c++) {
            switch (c[0]) {
                case '\n':
                    num_lines++;
                break;
                case '"':
                    for (idx++,c++; idx < chunk.text.size-1 && c[0] != '"'; idx++, c++);
                break;
                case '/':
                if (c[1] == '/') {
                    for (; idx < chunk.text.size-1 && c[1] != '\n'; idx++, c++);
                } else if (c[1] == '*') {
                    for (idx++, c++; idx < chunk.text.size-1 && !(c[0] == '*' && c[1] == '/'); idx++, c++);
                }
                break;
                case '#':
                {
                    u64 macro_starts_idx            = (idx >> 3);
                    u64 macro_starts_bit            = (idx & 0x7);
                    macro_starts[macro_starts_idx] |= (1 << macro_starts_bit);
                    num_macros++;
                    for (; idx < chunk.text.size-1 && c[1] != '\n'; idx++, c++);
                }
                break;
            }
        }
    }

    MoonFruit_PerChunkInfo* chunk_info = &chunk.file->per_chunk_info.data[chunk.per_file_chunk_idx];
    MutexBlock(Q->mutex) {
        chunk_info->macros = Array(arena, MoonFruit_Macro, num_macros);
    }

    // loop through macros and add information
    {
        //printf("==================MACROS==================\n");
        u8* c_start = chunk.text.str;
        for (u64 byte_idx = 0; byte_idx < (sizeof(macro_starts)/sizeof(*macro_starts)); byte_idx++) {
            c_start = chunk.text.str + (byte_idx * 8);
            u8 byte = macro_starts[byte_idx];
            while (byte > 0) {
                if ((byte & 1) == 1) {
                    String raw_macro = moonfruit_macro_extract_string(c_start);
                    //printf("MACRO FOUND: %.*s\n", full_macro_string.size, full_macro_string.str);
                    MoonFruit_Macro parsed_macro = moonfruit_macro_init(arena, Q->mutex, raw_macro);
                }
                c_start++;
                byte >>= 1;
            }
        }
    }

    u64 num_chunks = atomic_load(&chunk.file->per_chunk_info.count);
    for (u64 idx = chunk.per_file_chunk_idx + 1; idx < num_chunks; idx++) {
        atomic_fetch_add(&chunk.file->per_chunk_info.data[idx].start_line, num_lines);
    }

    /*
    printf("==================CHUNK===================\n"
           "Chunk Idx  => %d\n"
           "num_lines  => %d\n"
           "num_macros => %d\n"
           "------------------------------------------\n"
           "%.*s\n",
           chunk.per_file_chunk_idx, num_lines, num_macros,
           chunk.text.size,
           chunk.text.str);
    */

    if (!chunk.last_chunk_in_file) {
        moonfruit_file_push_next_chunk(chunk.file, Q);
        return;
    }

    // still not the right place because this chunk could get processed before
    // the rest of the chunks
    /*
    i64 fd = atomic_load(&chunk.file->file.fd);
    if (fd >= 0) {
        DeferBlock(mutex_lock(Q->mutex), mutex_unlock(Q->mutex)) {
            moonfruit_file_close(chunk.file);
        }
    }
    */
}

String moonfruit_macro_extract_string(u8* macro_start) {
    u8* c = macro_start;
    for (; c[1] != '\n' || (c[0] == '\\' && c[1] ==  '\n'); c++);
    String raw_macro  = (String) {
        .str = macro_start,
        .size = (u64)(c+1 - macro_start),
    };
    return raw_macro;
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

MoonFruit_Macro moonfruit_macro_init(Arena* arena, Mutex* mutex, String raw_macro) {
    Assert(raw_macro.size > 0);
    Assert(raw_macro.str[0] == '#');

    MoonFruit_Macro parsed_macro = (MoonFruit_Macro){0};

    raw_macro = string_skip(raw_macro, 1); // remove #
    raw_macro = string_skip_whitespace(raw_macro);
    raw_macro = string_chop_whitespace(raw_macro);

    String macro_type = string_chop_before_whitespace(raw_macro);

    String raw_expression = string_skip_whitespace((String){
        .str = raw_macro.str + macro_type.size,
        .size = raw_macro.size - macro_type.size,
    });

    u64 first_8_letters = 0;
    MemoryCopy(&first_8_letters, macro_type.str, Min(sizeof(u64), macro_type.size));

    switch (first_8_letters) {
        case MF_LETTERS_INCLUDE:
        {
            parsed_macro.type = MF_INCLUDE;
            if (!((raw_expression.str[0] == '"' && raw_expression.str[raw_expression.size-1] == '"') ||
                  (raw_expression.str[0] == '<' && raw_expression.str[raw_expression.size-1] == '>'))) {
                Assert(!"Invalid include");
            }

            String file_name;
            file_name = string_skip(raw_expression, 1);
            file_name = string_chop(file_name, 1);
            parsed_macro.expr.Include.file_name = file_name;
        }
        break;
        case MF_LETTERS_DEFINE:
        {
            parsed_macro.type = MF_DEFINE;
            u8* c = raw_expression.str;
            for (; !char_is_whitespace(*c) && (*c) != '('; c++);
            parsed_macro.expr.Define.name = (String) {
                .str = raw_expression.str,
                .size = (u64)(c - raw_expression.str),
            };

            if (c[0] == '(' && c[1] != ')') {
                u64 num_args = 1;
                for (u8* _c = c; (*_c) != ')'; _c++) {
                    num_args += (u64)((*_c) == ',');
                }

                MutexBlock(mutex) {
                    parsed_macro.expr.Define.args = Array(arena, String, num_args);
                }

                u8* arg_start = ++c;
                u64 num_args_parsed = 0;
                for (; (*c) != ')'; c++) {
                    if (*c == ',') {
                        parsed_macro.expr.Define.args.data[num_args_parsed++] = string_skip_whitespace((String){
                            .str = arg_start,
                            .size = (u64)(c - arg_start),
                        });
                        arg_start = c+1;
                    }
                }
                parsed_macro.expr.Define.args.data[num_args_parsed++] = string_skip_whitespace((String){
                    .str = arg_start,
                    .size = (u64)(c - arg_start),
                });
                c++;
            }
 
            parsed_macro.expr.Define.body = string_skip_whitespace((String) {
                .str = c,
                .size = raw_expression.size - (u64)(c - raw_expression.str),
            });
        }
        break;
        case MF_LETTERS_UNDEF:
        {
            parsed_macro.type = MF_UNDEF;
            parsed_macro.expr.Undef.name = raw_expression;
        }
        break;
        case MF_LETTERS_IF:
        {
            parsed_macro.type = MF_IF;
            parsed_macro.expr.If.condition = raw_expression;
        }
        break;
        case MF_LETTERS_IFDEF:
        {
            parsed_macro.type = MF_IFDEF;
            parsed_macro.expr.Ifdef.name = raw_expression;
        }
        break;
        case MF_LETTERS_IFNDEF:
        {
            parsed_macro.type = MF_IFNDEF;
            parsed_macro.expr.Ifndef.name = raw_expression;
        }
        break;
        case MF_LETTERS_ELIF:
        {
            parsed_macro.type = MF_ELIF;
            parsed_macro.expr.Elif.condition = raw_expression;
        }
        break;
        case MF_LETTERS_ELSE:
            parsed_macro.type = MF_ELSE;
        break;
        case MF_LETTERS_ENDIF:
            parsed_macro.type = MF_ENDIF;
        break;
        default:
            Assert(!"Unhandled preprocessor directive!!");
    }

    return parsed_macro;
}
