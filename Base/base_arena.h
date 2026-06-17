#define ARENA_HEADER_SIZE 128
#define ARENA_MAX_COUNT 512

typedef struct {
    void* base;
    u64 reserved;
    u64 committed;
    u64 commit_size;
    u64 pos;
} Arena;
StaticAssert(sizeof(Arena) <= ARENA_HEADER_SIZE, arena_header_size_check);

typedef struct TempArena TempArena;
struct TempArena {
    Arena* arena;
    u64 pos;
};

extern Arena* all_arenas[ARENA_MAX_COUNT];
extern u64 all_arenas_size;

extern u64 arena_default_reserve_size;
extern u64 arena_default_commit_size;

Arena* arena_alloc_(u64 reserve_size, u64 commit_size);
#define arena_alloc(reserve_size, commit_size) arena_alloc_((reserve_size), (commit_size))
#define default_arena() arena_alloc(arena_default_reserve_size, arena_default_commit_size)

void arena_release_(Arena **arena);
internal void arena_release_all();
#define arena_release(arena) arena_release_(&(arena))

void *arena_push(Arena *arena, u64 size, u64 align, b8 zero);

#define push_array(arena, type, count, zero) (type *)arena_push((arena), sizeof(type)*(count), sizeof(type), (zero))
#define push_struct(arena, type) (type *)arena_push((arena), sizeof(type), sizeof(type), true)

void arena_pop(Arena *arena, u64 size);
void arena_pop_to(Arena *arena, u64 pos);

u64 arena_pos(Arena *arena);

void arena_clear(Arena *arena);

TempArena temp_arena_begin(Arena* arena);
void temp_arena_end(TempArena arena);

#define TempArenaBlock(arena_name)                                                                              \
    for(TempArena _temp_arena_ = temp_arena_begin(arena_name); _temp_arena_.arena; _temp_arena_.arena = 0)      \
    DeferBlock({}, temp_arena_end(_temp_arena_))
