b32 char_is_whitespace(u8 c) {
    return (c == ' ' || c == '\n' || c == '\t' || c == '\r' || c == '\f' || c == '\v');
}

b32 char_is_upper(u8 c) {
    return ('A' <= c && c <= 'Z');
}

b32 char_is_lower(u8 c) {
    return ('a' <= c && c <= 'z');
}

b32 char_is_alpha(u8 c) {
    return (char_is_upper(c) || char_is_lower(c));
}

b32 char_is_slash(u8 c) {
    return c == '/' || c == '\\';
}

b32 char_is_digit(u8 c, u32 base) {
    Assert(base > 0 && base <= 16);
    if (base > 10) {
        c = char_to_lower(c);
        return (c >= '0' && c <= '9') || (c >= 'a' && c < ('a' + base - 10));
    }
    return (c >= '0' && c < '0' + base);
}

u8 char_to_lower(u8 c) {
    if (char_is_upper(c)) c += ('a' - 'A');
    return c;
}

u8 char_to_upper(u8 c) {
    if (char_is_lower(c)) c += ('A' - 'a');
    return c;
}

u8 char_correct_slash(u8 c) {
    if (char_is_slash(c))  c = '/';
    return c;
}

u64 cstring_size_(u8* cstr) {
    if (!cstr) return 0;
    u8* ptr = cstr;
    for (; *ptr != 0; ptr++);
    return (u64)(ptr - cstr);
}

String string_substr(String s, u64 first_idx, u64 last_idx) {
    first_idx = ClampTop(first_idx, s.size);
    last_idx = ClampTop(last_idx, s.size);
    s.str += first_idx,
    s.size = (u64)ClampBot((i64)last_idx - (i64)first_idx + 1, 0);
    return s;
}

String string_prefix(String s, u64 size) {
    s.size = ClampTop(size, s.size);
    return s;
}

String string_postfix(String s, u64 size) {
    size = ClampTop(size, s.size);
    s.str = (s.str + s.size) - size;
    s.size = size;
    return s;
}

String string_chop(String s, u64 amt) {
    s.size -= ClampTop(amt, s.size);
    return s;
}

String string_skip(String s, u64 amt) {
    amt = ClampTop(amt, s.size);
    s.str += amt;
    s.size -= amt;
    return s;
}

String string_chop_whitespace(String s) {
    u64 amt = 0;
    for (i64 i = s.size-1; i >= 0 && char_is_whitespace(s.str[i]); i--,amt++);
    return string_chop(s, amt);
}

String string_skip_whitespace(String s) {
    u64 amt = 0;
    for (; amt < s.size && char_is_whitespace(s.str[amt]); amt++);
    return string_skip(s, amt);
}

String string_chop_before_whitespace(String s) {
    u64 amt = 0;
    for (; amt < s.size && !char_is_whitespace(s.str[amt]); amt++);
    return string_prefix(s, amt);
}

b32 string_compare(String a, String b) {
    if (a.size != b.size) return false;

    // could make this faster by comparing u64's or even use simd then compare the remaining chars

    for (u64 i = 0; i < a.size; i++) {
        if (a.str[i] != b.str[i]) return false;
    }

    return true;
}

/*
String string_keep_before_perfect_match(String s, String str_match) {
    u8* c = s.str;
    u8* c_match = str_match.str;
    u64 i = 0;
    for (; i < s.size && *c != *c_match; i++, c++);

    u64 j = i;
    for (; j < s.size && *c == *c_match; j++, c++, *c_match++);

    return string_chop(s, (j-i == str_match.size) * i);
}
*/

String string_keep_after_perfect_match(String s, String str_match) {
    u8* c = s.str;
    u8* c_match = str_match.str;
    u64 i = 0;
    for (; i < s.size && *c != *c_match; i++, c++);

    u64 j = i;
    for (; j < s.size && *c == *c_match; j++, c++, *c_match++);

    return string_skip(s, (j-i == str_match.size) ? j : i);
}

StringPartiallyMatchedPair string_keep_unmatched_ends(String a, String b) {
    if (a.size > b.size) {
        String temp = (String){ .str = a.str, .size = a.size };
        a = (String){ .str = b.str, .size = b.size };
        b = (String){ .str = temp.str, .size = temp.size };
    }

    u64 i = 0;
    for (; i < a.size && a.str[i] == b.str[i]; i++);

    return (StringPartiallyMatchedPair) {
        .matched = string_prefix(a, i),
        .unmatched = { string_skip(a, i), string_skip(b, i) },
    };
}

String string_concat(Arena* arena, String s1, String s2) {
    String output = EmptyString(arena, s1.size + s2.size);
    MemoryCopy(output.str, s1.str, s1.size);
    MemoryCopy(output.str + s1.size, s2.str, s2.size);
    return output;
}

String string_copy(Arena* arena, String s) {
    String output = EmptyString(arena, s.size+1);
    MemoryCopy(output.str, s.str, s.size);
    output.size = s.size;
    return output;
}

void string_builder_append(Arena* arena, String* s, String string_to_append) {
    u8* s_ptr = s->str + s->size;
    (void)push_array(arena, u8, string_to_append.size, false);
    s->size += string_to_append.size;
    MemoryCopy(s_ptr, string_to_append.str, string_to_append.size);
}

void string_builder_step_back(Arena* arena, String* s, u64 num_chars) {
    arena_pop(arena, num_chars);
    *s = string_chop(*s, num_chars);
}

internal String string_formatv(Arena* arena, u8* fmt, va_list args) {
    va_list args2;
    va_copy(args2, args);
    u32 needed_bytes = vsnprintf(0, 0, fmt, args) + 1;
    String result = {0};
    result.str = push_array(arena, u8, needed_bytes, false);
    result.size = vsnprintf((u8*)result.str, needed_bytes, fmt, args2);
    result.str[result.size] = 0;
    va_end(args2);
    return result;
}

String string_format(Arena* arena, u8* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    String result = string_formatv(arena, fmt, args);
    va_end(args);
    return result;
}

