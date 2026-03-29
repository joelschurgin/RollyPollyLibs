typedef struct _Array _Array;
struct _Array {
    u64 size;
};

void* array_alloc(Arena* arena, u64 size, u64 element_size);
#define Array(arena, size, type) (type *)array_alloc((arena), (size), sizeof(type))

#define ArraySize(arr) (*(_Array*)((arr)-1))

typedef u8*  u8Array;
typedef u16* u16Array;
typedef u32* u32Array;
typedef u64* u64Array;

typedef i8*  i8Array;
typedef i16* i16Array;
typedef i32* i32Array;
typedef i64* i64Array;

typedef b8*  b8Array;
typedef b16* b16Array;
typedef b32* b32Array;
typedef b64* b64Array;

typedef f32* f32Array;
typedef f64* f64Array;
