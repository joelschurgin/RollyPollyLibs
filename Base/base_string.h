typedef struct String String;
struct String {
    u8* str;
    u64 size;
};

typedef struct {
    String a;
    String b;
} StringPair;

typedef struct {
    String matched;
    String unmatched[2];
} StringPartiallyMatchedPair;

DefineArray(String);

b32 char_is_whitespace(u8 c);
b32 char_is_upper(u8 c);
b32 char_is_lower(u8 c);
b32 char_is_alpha(u8 c);
b32 char_is_slash(u8 c);
b32 char_is_digit(u8 c, u32 base);
u8 char_to_lower(u8 c);
u8 char_to_upper(u8 c);
u8 char_correct_slash(u8 c);

u64 cstring_size_(u8* s);

#if COMPILER_GCC || COMPILER_CLANG
# define cstring_size(s) ((__builtin_constant_p(s)) ? (sizeof(s)-1) : cstring_size_(s))
#else
# define cstring_size(s) cstring_size_(s)
# error String macro will always find string size at runtime
#endif

String string_substr(String s, u64 first_idx, u64 last_idx);
String string_prefix(String s, u64 size);
String string_postfix(String s, u64 size);
String string_chop(String s, u64 amt);
String string_skip(String s, u64 amt);

String string_chop_whitespace(String s);
String string_skip_whitespace(String s);

String string_chop_before_whitespace(String s);

#define String(s) (String){ .str = s, .size = cstring_size(s) }
#define EmptyString(arena, string_size) (String){ .str = push_array((arena), u8, (string_size), true), .size = (string_size), }
#define SubString(str, first_idx, last_idx) string_substr((str), (first_idx), (last_idx))
#define Prefix(str, size) string_prefix((str), (size))
#define Postfix(str, size) string_postfix((str), (size))

b32 string_equal(String a, String b);
//String string_keep_before_perfect_match(String s, String str_match);
String string_keep_after_perfect_match(String s, String str_match);

StringPartiallyMatchedPair string_keep_unmatched_ends(String a, String b);

String string_concat(Arena* arena, String s1, String s2);
String string_copy(Arena* arena, String s);

#define StringBuilderBlock(arena, string) DeferBlock({ (string).str = (u8*)((arena)->pos + (arena)->base); (string).size = 0; }, { if ((string).size == 0) (string).str = NULL; })
void string_builder_append(Arena* arena, String* s, String string_to_append);
void string_builder_step_back(Arena* arena, String* s, u64 num_chars);

String string_formatv(Arena* arena, u8* fmt, va_list args);
String string_format(Arena* arena, u8* fmt, ...);
#define FormatString(arena, fmt, ...) string_format(arena, fmt, __VA_ARGS__)
