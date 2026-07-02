#include "moonfruit.h"

Implement_MoonFruit_ChunkQueue;

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

MoonFruit_File* moonfruit_file_create_and_open(Arena* arena, String path) {
    MoonFruit_File* f = push_struct(arena, MoonFruit_File);
    f->file = File(arena, path);
    f->pos = 0;

    moonfruit_file_open(f);

    u64 num_chunks = CeilIntDiv(f->file.size, MOONFRUIT_CHUNK_SIZE);
    f->chunk_info = Array(arena, MoonFruit_ChunkInfo, num_chunks);
    for (u64 i = 0; i < num_chunks; i++) {
        f->chunk_info.data[i] = (MoonFruit_ChunkInfo) {
            .start_line = 1,
        };
    }
    return f;
}

void moonfruit_file_open(MoonFruit_File* f) {
    file_open(&f->file, FILE_READ_ONLY);
}

void moonfruit_file_close(MoonFruit_File* f) {
    file_close(&f->file);
}

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

    b8 adjust_end_of_chunk = (!chunk->last_chunk_in_file && chunk->text.str[chunk->text.size - 1] != '\n');
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

typedef enum {
    MF_TOKEN_STATE_NORMAL,
    MF_TOKEN_STATE_HASHTAG,
    MF_TOKEN_STATE_INCLUDE,
} MoonFruit_TokenState;
u64 moonfruit_tokenize(MoonFruit_Chunk chunk) {
    MoonFruit_ChunkInfo* chunk_info = &chunk.file->chunk_info.data[chunk.per_file_chunk_idx];
    MoonFruit_TokenArray* token_arr = &chunk_info->tokens;
    MoonFruit_TokenState token_state = MF_TOKEN_STATE_NORMAL;
    u64 num_macros = 0;

    for EachChar(c, chunk.text) {
        if (*c == '\n') {
            // TODO: line num increments here, but what do we do with it?
            continue;
        }
        if (char_is_whitespace(*c)) continue;
        if (*c == '\\' && *(c+1) == '\n') {
            c++;
            continue;
        }
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
                        num_macros++;
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

    return num_macros;
}

internal void moonfruit_macro_find_remaining_tokens(MoonFruit_Chunk chunk, MoonFruit_Token* token, MoonFruit_Macro* macro) {
    MoonFruit_ChunkInfo* chunk_info = &chunk.file->chunk_info.data[chunk.per_file_chunk_idx];
    u8* c = (token)->data.str;
    for EachCharContinueUntil(c, chunk.text, *c == '\n' && *(c-1) != '\\');
    MoonFruit_Token* operand = token+1;
    for EachElementContinueUntil(operand, MoonFruit_Token, chunk_info->tokens, operand->data.str > c);

    macro->token_idx_first = (u64)(token+1 - chunk_info->tokens.data);
    macro->token_idx_last  = (u64)(operand-1 - chunk_info->tokens.data);
}

internal void moonfruit_macro_find_in_chunk(MoonFruit_Chunk chunk, u64 num_macros) {
    MoonFruit_ChunkInfo* chunk_info = &chunk.file->chunk_info.data[chunk.per_file_chunk_idx];
    MoonFruit_MacroArray* macro_arr = &chunk_info->macros;
    *macro_arr = Array(LaneArena(), MoonFruit_Macro, num_macros+1);
    macro_arr->count = 0;

    #define MoonFruit_MacroArray_Last (macro_arr->data[macro_arr->count-1])
    #define MoonFruit_MacroArray_Push(macro_type) do { \
                        macro_arr->count += 1;\
                        MoonFruit_MacroArray_Last.type = (macro_type);\
                    } while(0)
    #define MoonFruit_MacroArray_Last_NumTokens (i64)(MoonFruit_MacroArray_Last.token_idx_last - MoonFruit_MacroArray_Last.token_idx_first + 1)

    MoonFruit_MacroArray_Push(MF_IGNORE);
    for EachElement(token, MoonFruit_Token, chunk_info->tokens) {
        if (string_equal(token->data, String("#"))) {
            IncElement(token, chunk_info->tokens, 1);
            u64 directive = 0;
            MemoryCopy(&directive, token->data.str, Min(token->data.size, sizeof(u64)));

            switch (directive) {
            case MF_LETTERS_INCLUDE:
                MoonFruit_MacroArray_Push(MF_INCLUDE);
                moonfruit_macro_find_remaining_tokens(chunk, token, &MoonFruit_MacroArray_Last);
                Assert(MoonFruit_MacroArray_Last_NumTokens == 1); 
            break;
            case MF_LETTERS_DEFINE:
                MoonFruit_MacroArray_Push(MF_DEFINE);
                moonfruit_macro_find_remaining_tokens(chunk, token, &MoonFruit_MacroArray_Last);
                Assert(MoonFruit_MacroArray_Last_NumTokens >= 1); 
            break;
            case MF_LETTERS_UNDEF:
                MoonFruit_MacroArray_Push(MF_UNDEF);
                moonfruit_macro_find_remaining_tokens(chunk, token, &MoonFruit_MacroArray_Last);
                Assert(MoonFruit_MacroArray_Last_NumTokens == 1); 
            break;
            case MF_LETTERS_IF:
                MoonFruit_MacroArray_Push(MF_IF);
                moonfruit_macro_find_remaining_tokens(chunk, token, &MoonFruit_MacroArray_Last);
                Assert(MoonFruit_MacroArray_Last_NumTokens >= 1); 
            break;
            case MF_LETTERS_IFDEF:
                MoonFruit_MacroArray_Push(MF_IFDEF);
                moonfruit_macro_find_remaining_tokens(chunk, token, &MoonFruit_MacroArray_Last);
                Assert(MoonFruit_MacroArray_Last_NumTokens == 1);
            break;
            case MF_LETTERS_IFNDEF:
                MoonFruit_MacroArray_Push(MF_IFNDEF);
                moonfruit_macro_find_remaining_tokens(chunk, token, &MoonFruit_MacroArray_Last);
                Assert(MoonFruit_MacroArray_Last_NumTokens == 1);
            break;
            case MF_LETTERS_ELIF:
                MoonFruit_MacroArray_Push(MF_ELIF);
                moonfruit_macro_find_remaining_tokens(chunk, token, &MoonFruit_MacroArray_Last);
                Assert(MoonFruit_MacroArray_Last_NumTokens >= 1);
            break;
            case MF_LETTERS_ELSE:
                MoonFruit_MacroArray_Push(MF_ELSE);
                moonfruit_macro_find_remaining_tokens(chunk, token, &MoonFruit_MacroArray_Last);
                Assert(MoonFruit_MacroArray_Last_NumTokens == 0);
            break;
            case MF_LETTERS_ENDIF:
                MoonFruit_MacroArray_Push(MF_ENDIF);
                moonfruit_macro_find_remaining_tokens(chunk, token, &MoonFruit_MacroArray_Last);
                Assert(MoonFruit_MacroArray_Last_NumTokens == 0);
            break;
            }
        }
    }
}

void moonfruit_chunk_process(MoonFruit_Chunk chunk, MoonFruit_ChunkQueue *chunk_Q) {
    if (moonfruit_chunk_empty(chunk))
        return;

    moonfruit_chunk_align_to_line(&chunk);
    u64 num_macros = moonfruit_tokenize(chunk);
    moonfruit_macro_find_in_chunk(chunk, num_macros);

    if (!chunk.last_chunk_in_file) {
        moonfruit_file_push_next_chunk(chunk.file, chunk_Q);
        return;
    }
}

#define MOONFRUIT_DEFINITION_TREE_INIT_SIZE (u64)(('z' - 'a') + ('Z' - 'A') + 2)
internal u64 _moonfruit_definition_tree_start_idx(u8 c) {
    u64 base_idx = 1; // reserving 0
    if (c >= 'a' && c <= 'z') {
        return (c - 'a') + base_idx;
    }
    base_idx += ('z' - 'a');

    if (c >= 'A' && c <= 'Z') {
        return (c - 'A') + base_idx;
    }
    base_idx += ('Z' - 'A');

    if (c == '_') {
        return base_idx;
    }

    return 0;
}

internal u64 _moonfruit_definition_tree_new_node(Arena* arena, MoonFruit_DefinitionTree* tree) {
    (void)push_struct(arena, MoonFruit_Definition);
    return tree->count++;
}

internal void _moonfruit_definition_tree_insert_child(Arena* arena, MoonFruit_DefinitionTree* tree, u64 macro_idx, String definition, u64 idx) {
    MoonFruit_Definition* def = &tree->data[idx];
    u64 new_idx = _moonfruit_definition_tree_new_node(arena, tree);
    MoonFruit_Definition* new_def = &tree->data[new_idx];
    new_def->child_idx = def->child_idx;
    def->child_idx = new_idx;

    // data
    new_def->radix = definition;
    new_def->macro_idx = macro_idx;
}

internal void _moonfruit_definition_tree_insert_sibling(Arena* arena, MoonFruit_DefinitionTree* tree, u64 macro_idx, String definition, u64 idx) {
    MoonFruit_Definition* def = &tree->data[idx];
    u64 new_idx = _moonfruit_definition_tree_new_node(arena, tree);
    MoonFruit_Definition* new_def = &tree->data[new_idx];
    new_def->sibling_idx = def->sibling_idx;
    def->sibling_idx = new_idx;

    // data
    new_def->radix = definition;
    new_def->macro_idx = macro_idx;
}

// TODO: add list of macros, will also need line numbers for macros too
internal void _moonfruit_definition_tree_insert(Arena* arena, MoonFruit_DefinitionTree* tree, u64 macro_idx, String definition, u64 idx) {
    MoonFruit_Definition* def = &tree->data[idx];
    if (def->radix.size == 0) {
        def->radix = definition;
        def->macro_idx = macro_idx;
        return;
    }

    StringPartiallyMatchedPair pair = string_keep_unmatched_ends(definition, def->radix);
    b32 split_def = (pair.unmatched[0].size > 0 && pair.unmatched[1].size > 0);

    if (pair.matched.size > 0) {
        def->radix = pair.matched;

        if (pair.matched.size == definition.size && !split_def) {
            def->macro_idx = macro_idx;
        }
        for (i32 i = 0; i < 2; i++) {
            if (pair.unmatched[i].size > 0 && (pair.unmatched[i].str != def->radix.str || pair.unmatched[i].size != def->radix.size)) {
                if (def->child_idx == 0) {
                    _moonfruit_definition_tree_insert_child(arena, tree, macro_idx, pair.unmatched[i], idx);
                } else if (pair.matched.size == definition.size) {
                    _moonfruit_definition_tree_insert_child(arena, tree, 0, pair.unmatched[i], idx);
                } else {
                    if (split_def) {
                        macro_idx = def->macro_idx;
                        def->macro_idx = 0;
                    }
                    _moonfruit_definition_tree_insert(arena, tree, macro_idx, pair.unmatched[i], def->child_idx);
                }
            }
        }
    } else {
        for (i32 i = 0; i < 2; i++) {
            if (pair.unmatched[i].size > 0 && (pair.unmatched[i].str != def->radix.str || pair.unmatched[i].size != def->radix.size)) {
                if (def->sibling_idx == 0 || pair.matched.size == definition.size) {
                    _moonfruit_definition_tree_insert_sibling(arena, tree, macro_idx, pair.unmatched[i], idx);
                } else {
                    _moonfruit_definition_tree_insert(arena, tree, macro_idx, pair.unmatched[i], def->sibling_idx);
                }
            }
        }
    }
}

void moonfruit_definition_tree_insert(Arena* arena, MoonFruit_DefinitionTree* tree, u64 macro_idx, String definition) {
    u64 start_idx = _moonfruit_definition_tree_start_idx(definition.str[0]);
    _moonfruit_definition_tree_insert(arena, tree, macro_idx, definition, start_idx);
}

MoonFruit_MacroInfo* moonfruit_macro_info_build(MoonFruit_File* f) {
    Arena* arena = thread_ctx.shared_arena;
    MoonFruit_MacroInfo* macro_info = push_struct(arena, MoonFruit_MacroInfo);

    // count tokens and macros
    {
        MoonFruit_File* curr_file = f;
        while (curr_file) {
            macro_info->file_markers.count++;
            for EachElement(chunk_info, MoonFruit_ChunkInfo, curr_file->chunk_info) {
                macro_info->tokens.count += chunk_info->tokens.count;
                macro_info->macros.count += chunk_info->macros.count;
            }
            curr_file = curr_file->next;
        }

        macro_info->tokens.data = push_array(arena, MoonFruit_Token, macro_info->tokens.count, true);
        macro_info->macros.data = push_array(arena, MoonFruit_Macro, macro_info->macros.count, true);
        macro_info->file_markers.data = push_array(arena, MoonFruit_FileMarker, macro_info->file_markers.count, true);
    }

    // copy macros and tokens
    {
        u64 token_idx = 0;
        u64 macro_idx = 0;
        u64 token_idx_offset = 0;

        u64 file_idx = 0;

        MoonFruit_File* curr_file = f;
        while (curr_file) {
            macro_info->file_markers.data[file_idx++] = (MoonFruit_FileMarker){
                .first_token = token_idx,
                .path = string_copy(arena, curr_file->file.path),
            };

            for EachElement(chunk_info, MoonFruit_ChunkInfo, curr_file->chunk_info) {
                for EachElement(token, MoonFruit_Token, chunk_info->tokens) {
                    macro_info->tokens.data[token_idx] = (MoonFruit_Token) {
                        .type = token->type,
                        .data = string_copy(arena, token->data),
                    };
                    token_idx++;
                }

                for EachElement(macro, MoonFruit_Macro, chunk_info->macros) {
                    macro_info->macros.data[macro_idx] = *macro;
                    macro_info->macros.data[macro_idx].token_idx_first += token_idx_offset;
                    macro_info->macros.data[macro_idx].token_idx_last += token_idx_offset;
                    macro_idx++;
                }

                token_idx_offset += chunk_info->tokens.count;
            }
            curr_file = curr_file->next;
        }
    }

    // build MoonFruit_DefinitionTree
    {
        macro_info->def_tree = Array(arena, MoonFruit_Definition, MOONFRUIT_DEFINITION_TREE_INIT_SIZE);
        for EachElement(macro, MoonFruit_Macro, macro_info->macros) {
            if ((macro->type & (MF_DEFINE | MF_UNDEF | MF_IFDEF | MF_IFNDEF)) == 0) continue;
            String definition = macro_info->tokens.data[macro->token_idx_first].data;
            moonfruit_definition_tree_insert(arena, &macro_info->def_tree, (u64)(macro - macro_info->macros.data), definition);
        }
    }

    return macro_info;
}

u64 moonfruit_definition_tree_find_idx(MoonFruit_DefinitionTree tree, String definition, u64 idx) {
    Assert(definition.size > 0);

    if (idx == 0) idx = _moonfruit_definition_tree_start_idx(definition.str[0]);

    MoonFruit_Definition* def = &tree.data[idx];
    StringPartiallyMatchedPair pair = string_keep_unmatched_ends(definition, def->radix);

    if ((pair.unmatched[0].size <= 0 && pair.unmatched[1].size <= 0) ||
        (pair.matched.size > 0 && pair.matched.size < def->radix.size)) return (def->macro_idx) ? idx : 0;

    for (i32 i = 0; i < 2; i++) {
        if (pair.unmatched[i].size > 0 &&
            (pair.unmatched[i].str + pair.unmatched[i].size) == (definition.str + definition.size)) {
            u64 idx = (pair.matched.size == def->radix.size) ? def->child_idx : def->sibling_idx;
            if (idx != 0) return moonfruit_definition_tree_find_idx(tree, pair.unmatched[i], idx);
        }
    }

    return 0;
}

MoonFruit_Macro moonfruit_macro_find(MoonFruit_MacroInfo* macro_info, String definition) {
    u64 tree_idx = moonfruit_definition_tree_find_idx(macro_info->def_tree, definition, 0);
    u64 macro_idx = macro_info->def_tree.data[tree_idx].macro_idx;
    return macro_info->macros.data[macro_idx];
}

#define MoonFruit_MatchIsPartial (pair.matched.size != def->radix.size || def->macro_idx == 0)
u64 moonfruit_definition_tree_partial_match_idx(MoonFruit_DefinitionTree tree, String definition, u64 idx, b32* match_is_partial) {
    Assert(definition.size > 0);

    if (idx == 0) idx = _moonfruit_definition_tree_start_idx(definition.str[0]);

    MoonFruit_Definition* def = &tree.data[idx];
    StringPartiallyMatchedPair pair = string_keep_unmatched_ends(definition, def->radix);

    if ((pair.unmatched[0].size <= 0 && pair.unmatched[1].size <= 0) ||
        (pair.matched.size > 0 && pair.matched.size < def->radix.size)) {
        *match_is_partial = MoonFruit_MatchIsPartial;
        return idx;
    }

    for (i32 i = 0; i < 2; i++) {
        if (pair.unmatched[i].size > 0 &&
            (pair.unmatched[i].str + pair.unmatched[i].size) == (definition.str + definition.size)) {
            u64 idx = (pair.matched.size == def->radix.size) ? def->child_idx : def->sibling_idx;
            if (idx != 0) return moonfruit_definition_tree_partial_match_idx(tree, pair.unmatched[i], idx, match_is_partial);
        }
    }

    *match_is_partial = MoonFruit_MatchIsPartial;
    return idx;
}

#undef MoonFruit_MatchIsPartial

void _moonfruit_definition_tree_find_other_matches(Arena* arena, MoonFruit_MacroInfo* macro_info, MoonFruit_MacroArray* matches, u64 idx) {
    MoonFruit_Definition* def = &macro_info->def_tree.data[idx];
    u64 macro_idx = def->macro_idx;
    if (macro_idx > 0) array_builder_push(arena, *matches, macro_info->macros.data[macro_idx]);

    if (def->child_idx > 0)   _moonfruit_definition_tree_find_other_matches(arena, macro_info, matches, def->child_idx);
    if (def->sibling_idx > 0) _moonfruit_definition_tree_find_other_matches(arena, macro_info, matches, def->sibling_idx);
}

MoonFruit_MacroArray moonfruit_macro_match(Arena* arena, MoonFruit_MacroInfo* macro_info, String definition) {
    MoonFruit_MacroArray matches = {0};

    b32 match_is_partial = false; // maybe don't need this?
    u64 tree_idx = moonfruit_definition_tree_partial_match_idx(macro_info->def_tree, definition, 0, &match_is_partial); 
    ArrayBuilderBlock(arena, matches, MoonFruit_Macro) {
        MoonFruit_Definition* def = &macro_info->def_tree.data[tree_idx];
        u64 macro_idx = def->macro_idx;
        if (macro_idx > 0) array_builder_push(arena, matches, macro_info->macros.data[macro_idx]);
        //if (!match_is_partial) return matches;

        if (def->child_idx > 0) _moonfruit_definition_tree_find_other_matches(arena, macro_info, &matches, def->child_idx);
    }

    return matches;
}

String moonfruit_macro_format(Arena* arena, MoonFruit_MacroInfo* macro_info, MoonFruit_Macro macro, MoonFruit_MacroFormatFlag flags) {
    if (flags == MF_FORMAT_DEFAULT) flags = MF_FORMAT_FULL;

    // override logic
    switch (macro.type) {
        case MF_DEFINE:
        break;
        case MF_IFDEF:
        case MF_IFNDEF:
        case MF_UNDEF:
            flags &= ~MF_FORMAT_EXPRESSION;
        break;
        case MF_IF:
        case MF_ELIF:
            flags &= ~MF_FORMAT_DEFINITION;
        break;
        default:
            return String("");
    }

    String macro_formatted;
    StringBuilderBlock(arena, macro_formatted) {
        u64 token_idx = 0;

        if ((flags & MF_FORMAT_IDENTIFIER) != 0) {
            for (token_idx = macro.token_idx_first - 2; token_idx < macro.token_idx_first; token_idx++) {
                MoonFruit_Token token = macro_info->tokens.data[token_idx];
                string_builder_append(arena, &macro_formatted, token.data);
            }
 
            if (flags != MF_FORMAT_IDENTIFIER) {
                string_builder_append(arena, &macro_formatted, String(" "));
            }
        }

        if ((flags & MF_FORMAT_DEFINITION) != 0) {
            token_idx = macro.token_idx_first;

            MoonFruit_Token token;
            token = macro_info->tokens.data[token_idx++];
            string_builder_append(arena, &macro_formatted, token.data);
 
            token = macro_info->tokens.data[token_idx++];
            b32 has_args = (token.data.str[0] == '(');
            if (has_args) {
                string_builder_append(arena, &macro_formatted, token.data);
                for (; token_idx <= macro.token_idx_last; token_idx++) {
                    token = macro_info->tokens.data[token_idx];
                    if (token.data.str[0] == ')') {
                        string_builder_step_back(arena, &macro_formatted, 1);
                        string_builder_append(arena, &macro_formatted, token.data);
                        token_idx++;
                        break;
                    }
                    string_builder_append(arena, &macro_formatted, token.data);
                    string_builder_append(arena, &macro_formatted, String(" "));
                }
            }

            if (flags != MF_FORMAT_DEFINITION) {
                string_builder_append(arena, &macro_formatted, String(" "));
            }
        } else {
            token_idx = macro.token_idx_first + 1;

            MoonFruit_Token token;
            token = macro_info->tokens.data[token_idx++];
            b32 has_args = (token.data.str[0] == '(');
            if (has_args) {
                for (; token_idx <= macro.token_idx_last; token_idx++) {
                    token = macro_info->tokens.data[token_idx];
                    if (token.data.str[0] == ')') {
                        token_idx++;
                        break;
                    }
                }
            }
        }

        if ((flags & MF_FORMAT_EXPRESSION) != 0) {
            for (; token_idx <= macro.token_idx_last; token_idx++) {
                MoonFruit_Token token = macro_info->tokens.data[token_idx];
                string_builder_append(arena, &macro_formatted, token.data);
                string_builder_append(arena, &macro_formatted, String(" "));
            }
            // last one, no space
            {
                MoonFruit_Token token = macro_info->tokens.data[macro.token_idx_last];
                string_builder_append(arena, &macro_formatted, token.data);
            }
        }
    }

    return macro_formatted;
}

internal i64 _moonfruit_token_find_arg(MoonFruit_ArgArray args, MoonFruit_Token token) {
    for EachElement(curr_arg, MoonFruit_Arg, args) {
        if (string_equal(curr_arg->data, token.data)) {
            return (i64)(curr_arg - args.data);
        }
    }
    return -1;
}

/*

#define func(a, b) (2 + 6 / (a + b))
#define func2(x) (6 * func(2*(x), 3))


func2(x)
( 6 * func( 2 * ( x ) , 3 ) )
( 6 * ( 2 + 6 / ( 2 * ( x ) + 3 ) )

*/

String moonfruit_macro_eval(Arena* arena, MoonFruit_MacroInfo* macro_info, MoonFruit_Macro macro, MoonFruit_ArgValArray arg_vals) {
    local_persist u8 color = 0;
    color = (color + 1) % (5);

    if (macro.type == MF_IGNORE) return String("");

    u64 token_idx = macro.token_idx_first + 1; // token_idx_first is the name of this macro
    MoonFruit_Token token = {0};

    MoonFruit_ArgArray args = {0};
    ArrayBuilderBlock(arena, args, MoonFruit_Arg) {
        token = macro_info->tokens.data[token_idx++];
        b32 has_args = (token.data.str[0] == '(');
        if (has_args) {
            for (; token_idx <= macro.token_idx_last; token_idx++) {
                token = macro_info->tokens.data[token_idx];
                if (token.data.str[0] == ')') {
                    token_idx++;
                    break;
                } else if (token.data.str[0] != ',') {
                    array_builder_push(arena, args, token);
                }
            }
        }
    }

    // parse expression
    String expr;
    StringBuilderBlock(arena, expr) {
        for (; token_idx <= macro.token_idx_last; token_idx++) {
            token = macro_info->tokens.data[token_idx];
            if (token.type == MF_TOKEN_IDENTIFIER) {
                i64 arg_idx = _moonfruit_token_find_arg(args, token);
                if (arg_idx >= 0) {
                    MoonFruit_ArgVal val = arg_vals.data[arg_idx];
                    for (u64 idx = val.token_idx_first; idx <= val.token_idx_last; idx++) {
                        string_builder_append(arena, &expr, macro_info->tokens.data[idx].data);
                        string_builder_append(arena, &expr, String(" "));
                    }
                } else {
                    MoonFruit_Macro def = moonfruit_macro_find(macro_info, token.data);

                    String def_str;
                    MoonFruit_ArgValArray def_arg_vals;

                    TempArenaBlock(arena)
                    ArrayBuilderBlock(arena, def_arg_vals, MoonFruit_ArgVal) {
                        // find arg values
                        token = macro_info->tokens.data[++token_idx];
                        b32 has_args = (token.data.str[0] == '(');
                        if (has_args) {
                            array_builder_push(arena, def_arg_vals, (MoonFruit_ArgVal){0});
                            ArrayLast(def_arg_vals).token_idx_first = ++token_idx;
                            i32 num_paren = 1;
                            for (; token_idx <= macro.token_idx_last; token_idx++) {
                                token = macro_info->tokens.data[token_idx];
                                num_paren += (token.data.str[0] == '(') - (token.data.str[0] == ')');

                                if (num_paren <= 0) {
                                    ArrayLast(def_arg_vals).token_idx_last = token_idx-1;
                                    break;
                                }

                                if (token.data.str[0] == ',') {
                                    ArrayLast(def_arg_vals).token_idx_last = token_idx-1;
                                    array_builder_push(arena, def_arg_vals, (MoonFruit_ArgVal){0});
                                    ArrayLast(def_arg_vals).token_idx_first = ++token_idx;
                                }
                            }
                        }

                        u8 temp_color = color;
                        def_str = moonfruit_macro_eval(arena, macro_info, def, def_arg_vals);
                        color = temp_color;
                    }
                    if (color == 0) string_builder_append(arena, &expr, String("\033[32m"));
                    if (color == 1) string_builder_append(arena, &expr, String("\033[33m"));
                    if (color == 2) string_builder_append(arena, &expr, String("\033[34m"));
                    if (color == 3) string_builder_append(arena, &expr, String("\033[35m"));
                    if (color == 4) string_builder_append(arena, &expr, String("\033[36m"));
                    string_builder_append(arena, &expr, def_str);
                    string_builder_append(arena, &expr, String("\033[0m"));
                }
            } else {
                string_builder_append(arena, &expr, token.data);
                string_builder_append(arena, &expr, String(" "));
            }
        }
    }

    return expr;
}
