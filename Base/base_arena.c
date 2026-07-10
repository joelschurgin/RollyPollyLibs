Arena* all_arenas[ARENA_MAX_COUNT] = {0};
u64 all_arenas_size = 0;

u64 arena_default_reserve_size = MB(64);
u64 arena_default_commit_size  = KB(64);

// move to OS layer when it exists
void* memory_reserve(u64 size) {
    void* ptr = MemoryMap(NULL, AlignPow2(size, PAGE_SIZE), PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (ptr == MAP_FAILED) {
        AssertAlways(!"Memory map failed!");
    }
    return ptr;
}

// move to OS layer when it exists
void memory_commit(void* ptr, u64 size) {
    if (MemoryProtect(ptr, AlignPow2(size, PAGE_SIZE), PROT_READ | PROT_WRITE)) {
        AssertAlways(!"Memory commit failed!");
    }
}

// move to OS layer when it exists
void memory_release(void* ptr, u64 size) {
    if (MemoryUnmap(ptr, size)) {
        AssertAlways(!"Memory release failed!");
    }
}

Arena* arena_alloc_(u64 reserve_size, u64 commit_size) {
    Assert(all_arenas_size <= ARENA_MAX_COUNT);

    commit_size = AlignPow2(commit_size, PAGE_SIZE);
    reserve_size = AlignPow2(reserve_size, PAGE_SIZE);

    Arena* arena = memory_reserve(reserve_size);
    memory_commit((void*)arena, commit_size);

    arena->base = (u8*)arena + ARENA_HEADER_SIZE;
    arena->pos = 0;
    arena->reserved = reserve_size;
    arena->committed = commit_size;
    arena->commit_size = commit_size;

    all_arenas[all_arenas_size++] = arena;
    return arena;
}

void arena_release_(Arena **arena) {
    if (!(arena && *arena)) return;
    memory_release(*arena, (*arena)->reserved);

    Arena** begin = all_arenas;
    Arena** end = all_arenas + all_arenas_size;
    for (Arena** a = begin; a != end; a++) {
        if (*arena == *a) {
            *a = NULL;
            break;
        }
    }

    (*arena) = NULL;
}

internal void arena_release_all() {
    Arena** begin = all_arenas;
    Arena** end = all_arenas + all_arenas_size;
    for (Arena** a = begin; a != end; a++) {
        memory_release(*a, (*a)->reserved);
        *a = NULL;
    }
}

void *arena_push(Arena *arena, u64 size, u64 align, b8 zero) {
    size = AlignPow2(size, align);

    if (arena->pos + size >= arena->committed - ARENA_HEADER_SIZE) {
        Assert(arena->pos < arena->committed);
        u64 amount_to_commit = AlignPow2(CeilIntegerDiv(arena->committed - arena->pos + size, arena->commit_size) * arena->commit_size, PAGE_SIZE);
        if (amount_to_commit >= arena->reserved) {
            AssertAlways(!"Out grown arena size");
        }

        arena->committed += amount_to_commit;
        Assert(arena->reserved >= arena->committed);
        memory_commit(arena, arena->committed);
    }

    void *new_alloc = arena->base + arena->pos;
    arena->pos += size;

    if (zero) MemorySet(new_alloc, 0, size);

    return new_alloc;
}

void arena_pop(Arena *arena, u64 size) {
    arena->pos = (u64)ClampBot((i64)arena->pos - (i64)size, 0);
}

void arena_pop_to(Arena *arena, u64 pos) {
    arena->pos = pos;
}

u64 arena_pos(Arena *arena) {
    return arena->pos;
}

void arena_clear(Arena *arena) {
    arena->pos = 0;
}

TempArena temp_arena_begin(Arena* arena) {
    Assert(arena);
    return (TempArena){
        .arena = arena,
        .pos = arena->pos,
    };
}
void temp_arena_end(TempArena temp_arena) {
    temp_arena.arena->pos = temp_arena.pos;
}
