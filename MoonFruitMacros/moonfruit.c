#include "moonfruit.h"
#include "gooey_tui.c"

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
                skip_comment = true;
            } else if (*(c+1) == '*') {
                c += 2;
                for EachCharContinueUntil(c, str, (*c == '*' && *(c+1) == '/'));
                c += 2;
                skip_comment = true;
                c++;
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
    // string constants, char constants, and firster names (in between <>)

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
                MoonFruit_MacroArray_Last.has_args = !char_is_whitespace((token+1)->data.str[(token+1)->data.size]);
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
            u64 temp = def->macro_idx;
            def->macro_idx = macro_idx;
            macro_idx = temp;
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

void moonfruit_definition_tree_insert(Arena* arena, MoonFruit_MacroInfo* macro_info, u64 macro_idx, String definition) {
    u64 start_idx = _moonfruit_definition_tree_start_idx(definition.str[0]);

    MutexBlock(macro_info->def_tree_mutexes.data[start_idx]) {
        _moonfruit_definition_tree_insert(arena, &macro_info->def_tree, macro_idx, definition, start_idx);
    }
}

MoonFruit_MacroInfo* moonfruit_macro_info_build(Arena* arena, MoonFruit_File* f) {
    MoonFruit_MacroInfo* macro_info = push_struct(arena, MoonFruit_MacroInfo);

    macro_info->def_tree_mutexes = Array(arena, MutexPtr, MOONFRUIT_DEFINITION_TREE_INIT_SIZE);
    for (u64 mutex_idx = 0; mutex_idx < macro_info->def_tree_mutexes.count; mutex_idx++) {
        mutex_assign(&macro_info->def_tree_mutexes.data[mutex_idx]);
    }

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
            moonfruit_definition_tree_insert(arena, macro_info, (u64)(macro - macro_info->macros.data), definition);
        }
    }

    return macro_info;
}

u64 moonfruit_definition_tree_find_idx(MoonFruit_DefinitionTree tree, String definition, u64 idx) {
    Assert(definition.size > 0);
    Assert(idx != 0);

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
    u64 start_idx = _moonfruit_definition_tree_start_idx(definition.str[0]);
    u64 tree_idx = 0;
    MutexBlock(macro_info->def_tree_mutexes.data[start_idx]) {
        tree_idx = moonfruit_definition_tree_find_idx(macro_info->def_tree, definition, start_idx);
    }
    u64 macro_idx = macro_info->def_tree.data[tree_idx].macro_idx;
    return macro_info->macros.data[macro_idx];
}

#define MoonFruit_MatchIsPartial (pair.matched.size != def->radix.size || def->macro_idx == 0)
u64 moonfruit_definition_tree_partial_match_idx(MoonFruit_DefinitionTree tree, String definition, u64 idx, b32* match_is_partial) {
    Assert(definition.size > 0);
    Assert(idx != 0);

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
    if (definition.size == 0) return matches;

    u64 start_idx = _moonfruit_definition_tree_start_idx(definition.str[0]);
    MutexBlock(macro_info->def_tree_mutexes.data[start_idx]) {
        b32 match_is_partial = false; // maybe don't need this?
        u64 tree_idx = moonfruit_definition_tree_partial_match_idx(macro_info->def_tree, definition, start_idx, &match_is_partial); 
        ArrayBuilderBlock(arena, matches, MoonFruit_Macro) {
            MoonFruit_Definition* def = &macro_info->def_tree.data[tree_idx];
            u64 macro_idx = def->macro_idx;
            if (macro_idx > 0) array_builder_push(arena, matches, macro_info->macros.data[macro_idx]);
            //if (!match_is_partial) return matches;

            if (def->child_idx > 0) _moonfruit_definition_tree_find_other_matches(arena, macro_info, &matches, def->child_idx);
        }
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
            if (macro.has_args) {
                string_builder_append(arena, &macro_formatted, token.data);
                for (; token_idx <= macro.token_idx_last; token_idx++) {
                    token = macro_info->tokens.data[token_idx];
                    if (token.data.str[0] == ')') {
                        if (macro_info->tokens.data[token_idx-1].data.str[0] != '(') string_builder_step_back(arena, &macro_formatted, 1);
                        string_builder_append(arena, &macro_formatted, token.data);
                        token_idx++;
                        break;
                    }

                    if (token.data.str[0] == ',') string_builder_step_back(arena, &macro_formatted, 1);
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
            if (macro.has_args) {
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

internal u64 _moonfruit_macro_count_args(MoonFruit_MacroInfo* macro_info, MoonFruit_Macro macro) {
    u64 num_args = 0;
    if (!macro.has_args) return 0;

    MoonFruit_Token token = {0};
    for (u64 token_idx = macro.token_idx_first + 2; token_idx <= macro.token_idx_last; token_idx++) {
        token = macro_info->tokens.data[token_idx];
        if (token.data.str[0] == ')') return num_args;
        if (token.data.str[0] != ',') num_args++;
    }

    return num_args;
}

internal MoonFruit_ArgArray _moonfruit_macro_build_arg_array(Arena* arena, MoonFruit_MacroInfo* macro_info, MoonFruit_Macro* macro, u64* next_token_idx) {
    u64 token_idx = macro->token_idx_first + 1;
    MoonFruit_Token token = {0};

    MoonFruit_ArgArray args = {0};
    ArrayBuilderBlock(arena, args, MoonFruit_Arg) {
        if (macro->has_args) {
            for (token_idx = macro->token_idx_first + 2; token_idx <= macro->token_idx_last; token_idx++) {
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

    *next_token_idx = token_idx;
    return args;
}

internal i64 _moonfruit_token_find_arg(MoonFruit_ArgArray args, MoonFruit_Token token) {
    for EachElement(curr_arg, MoonFruit_Arg, args) {
        if (string_equal(curr_arg->data, token.data)) {
            return (i64)(curr_arg - args.data);
        }
    }
    return -1;
}

internal void _moonfruit_expr_push_token(Arena* arena, MoonFruit_Expr* expr, MoonFruit_Token* token) {
    MoonFruit_TokenNode* next_node = push_struct(arena, MoonFruit_TokenNode);
    next_node->token = token;

    if (!expr->first) {
        expr->first = next_node;
        expr->last = next_node;
        return;
    }

    SLLQueuePush(expr->first, expr->last, next_node);
}

internal void _moonfruit_expr_push_arg_val(Arena* arena, MoonFruit_MacroInfo* macro_info, MoonFruit_Expr* expr, MoonFruit_ArgVal arg_val) {
    if (!arg_val.first) return;

    MoonFruit_TokenNode* node = arg_val.first;
    while (node) {
        _moonfruit_expr_push_token(arena, expr, node->token);
        if (node == arg_val.last) return;
        node = node->next;
    }
}

MoonFruit_Expr moonfruit_macro_to_expr(Arena* arena, MoonFruit_MacroInfo* macro_info, MoonFruit_Macro macro, MoonFruit_ArgValArray arg_vals) {
    MoonFruit_Expr expr = {0};

    u64 token_idx = macro.token_idx_first;
    expr.args = _moonfruit_macro_build_arg_array(arena, macro_info, &macro, &token_idx);

    if (arg_vals.count == 0) {
        for (; token_idx <= macro.token_idx_last; token_idx++) {
            _moonfruit_expr_push_token(arena, &expr, &macro_info->tokens.data[token_idx]);
        }
        return expr;
    }

    for (; token_idx <= macro.token_idx_last; token_idx++) {
        MoonFruit_Token* token = &macro_info->tokens.data[token_idx];
        i64 arg_idx = _moonfruit_token_find_arg(expr.args, *token);
        if (arg_idx >= 0) {
            _moonfruit_expr_push_arg_val(arena, macro_info, &expr, arg_vals.data[arg_idx]);
        } else {
            _moonfruit_expr_push_token(arena, &expr, token);
        }
    }

    return expr;
}

internal MoonFruit_ArgValArray _moonfruit_expr_find_arg_vals(Arena* arena, MoonFruit_MacroInfo* macro_info, MoonFruit_Macro macro, MoonFruit_TokenNode** curr_node, b32 save_arg_vals);

internal void _moonfruit_expr_skip_nested_macro(MoonFruit_MacroInfo* macro_info, MoonFruit_Macro nested_macro, MoonFruit_TokenNode** curr_node) {
    (void)_moonfruit_expr_find_arg_vals(0L, macro_info, nested_macro, curr_node, false);
}

internal MoonFruit_ArgValArray _moonfruit_expr_find_arg_vals(Arena* arena, MoonFruit_MacroInfo* macro_info, MoonFruit_Macro macro, MoonFruit_TokenNode** curr_node, b32 save_arg_vals) {
    MoonFruit_ArgValArray arg_vals = {0};
    if (macro.type == MF_IGNORE) return arg_vals;

    MoonFruit_TokenNode* node = *curr_node;

    if (!macro.has_args) {
        if (node) node = node->next;
        goto _moonfruit_expr_find_arg_vals_return;
    }

    u64 num_args = _moonfruit_macro_count_args(macro_info, macro);
    if (num_args == 0) {
        while (node && node->token && node->token->data.str[0] != ')') {
            node = node->next;
        }
        goto _moonfruit_expr_find_arg_vals_return;
    }

    if (save_arg_vals) arg_vals = Array(arena, MoonFruit_ArgVal, num_args);

    while (node && node->token && node->token->data.str[0] != '(') {
        node = node->next;
    }
    if (node) node = node->next;

    for (u64 arg_idx = 0; arg_idx < num_args; arg_idx++) {
        u8 delimiter = (arg_idx == num_args-1) ? ')' : ',';

        if (save_arg_vals) arg_vals.data[arg_idx].first = node;

        while (node && node->token) {
            if (node->token->type == MF_TOKEN_IDENTIFIER) {
                MoonFruit_Macro nested_macro = moonfruit_macro_find(macro_info, node->token->data);
                if (nested_macro.type != MF_IGNORE) {
                    _moonfruit_expr_skip_nested_macro(macro_info, nested_macro, &node);
                    if (save_arg_vals) {
                        arg_vals.data[arg_idx].last = node;
                    }
                    node = node->next;
                    continue;
                }
            }

            if (node->token->data.str[0] == delimiter) break;
            if (save_arg_vals) {
                arg_vals.data[arg_idx].last = node;
            }

            node = node->next;
        }
        if (node && node->token && node->token->data.str[0] == ',') {
            node = node->next;
        }
    }

_moonfruit_expr_find_arg_vals_return:
    *curr_node = node;
    return arg_vals;
}

internal MoonFruit_TokenNode* _moonfruit_expr_eval_token(Arena* arena, MoonFruit_MacroInfo* macro_info, MoonFruit_Expr prev_expr, MoonFruit_Expr* next_expr, MoonFruit_ArgValArray arg_vals, MoonFruit_TokenNode* node) {
    Assert(node);

    if (!node->token) return node;
    if (node->token->type != MF_TOKEN_IDENTIFIER) {
        _moonfruit_expr_push_token(arena, next_expr, node->token);
        return node;
    }

    i64 arg_idx = _moonfruit_token_find_arg(prev_expr.args, *node->token);
    if (arg_idx >= 0) {
        if (arg_vals.count == 0) {
            _moonfruit_expr_push_token(arena, next_expr, node->token);
        } else {
            _moonfruit_expr_push_arg_val(arena, macro_info, next_expr, arg_vals.data[arg_idx]);
        }
        return node;
    }

    MoonFruit_Macro macro = moonfruit_macro_find(macro_info, node->token->data);
    if (macro.type == MF_IGNORE) {
        _moonfruit_expr_push_token(arena, next_expr, node->token);
        return node;
    }

    next_expr->depends_on_other_macros = true;

    MoonFruit_ArgValArray macro_arg_vals = _moonfruit_expr_find_arg_vals(arena, macro_info, macro, &node, true);
    MoonFruit_Expr macro_expr = moonfruit_macro_to_expr(arena, macro_info, macro, macro_arg_vals);

    if (!macro_expr.first) {
        return node;
    }

    if (!next_expr->first) {
        next_expr->first = macro_expr.first;
        next_expr->last = macro_expr.last;
    } else {
        next_expr->last->next = macro_expr.first;
        next_expr->last = macro_expr.last;
    }

    return node;
}

MoonFruit_Expr moonfruit_expr_eval(Arena* arena, MoonFruit_MacroInfo* macro_info, MoonFruit_Expr prev_expr, MoonFruit_ArgValArray arg_vals) {
    MoonFruit_TokenNode* node = prev_expr.first;
    if (!node) return prev_expr;

    MoonFruit_Expr next_expr = {0};

    while (node) {
        node = _moonfruit_expr_eval_token(arena, macro_info, prev_expr, &next_expr, arg_vals, node);
        if (node) node = node->next;
    }

    return next_expr;
}

String moonfruit_expr_format(Arena* arena, MoonFruit_Expr expr) {
    String expr_str;
    MoonFruit_TokenNode* node = expr.first;
    StringBuilderBlock(arena, expr_str) {
        while (node) {
            if (node->token) {
                string_builder_append(arena, &expr_str, node->token->data);
            }
            if (node == expr.last) break;
            node = node->next;
        }
    }

    return expr_str;
}

void moonfruit_arg_val_print(MoonFruit_ArgVal arg_val) {
    MoonFruit_TokenNode* node = arg_val.first;
    printf("\r\nMacro arg val: ");
    while (node) {
        if (node->token) {
            printf("%.*s", node->token->data.size, node->token->data.str);
        }
        if (node == arg_val.last) break;
        node = node->next;
    }
    printf("\r\n\r\n");
}

void moonfruit_arg_val_print_all(MoonFruit_ArgValArray arg_vals) {
    for (u64 arg_idx = 0; arg_idx < arg_vals.count; arg_idx++) {
        moonfruit_arg_val_print(arg_vals.data[arg_idx]);
    }
}


void moonfruit_expr_print(MoonFruit_Expr expr) {
    MoonFruit_TokenNode* node = expr.first;
    printf("\r\nMacro Expr: ");
    while (node) {
        if (node->token) {
            printf("%.*s", node->token->data.size, node->token->data.str);
        }
        if (node == expr.last) break;
        node = node->next;
    }
    printf("\r\n\r\n");
}

void moonfruit_chain_print(MoonFruit_TokenNode* node) {
    printf("\r\nChain: ");
    while (node) {
        if (node->token) {
            printf("%.*s", node->token->data.size, node->token->data.str);
        }
        node = node->next;
    }
    printf("\r\n\r\n");
}

MoonFruit_ExprList moonfruit_macro_eval(Arena* arena, MoonFruit_MacroInfo* macro_info, MoonFruit_Macro macro, MoonFruit_ArgValArray arg_vals) {
    MoonFruit_ExprList expr_list = {0};

    MoonFruit_ExprNode* new_node = push_struct(arena, MoonFruit_ExprNode);
    new_node->expr = moonfruit_macro_to_expr(LaneArena(), macro_info, macro, arg_vals);
    expr_list.first = new_node;
    expr_list.last = new_node;

    MoonFruit_Expr curr_expr;
    curr_expr = moonfruit_expr_eval(LaneArena(), macro_info, new_node->expr, (MoonFruit_ArgValArray){0});

    while (curr_expr.depends_on_other_macros) {
        new_node = push_struct(arena, MoonFruit_ExprNode);
        new_node->expr = curr_expr;
        SLLQueuePush(expr_list.first, expr_list.last, new_node);

        curr_expr = moonfruit_expr_eval(LaneArena(), macro_info, curr_expr, (MoonFruit_ArgValArray){0});
    }

    return expr_list;
}
