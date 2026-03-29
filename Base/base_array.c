void* array_alloc(Arena* arena, u64 size, u64 element_size) {
    _Array* header = (_Array*)arena_push(arena, size*element_size, element_size, true);
    header->size = size;
    return (void*)(header + 1);
}
