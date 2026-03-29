b32 memory_is_zero(void* ptr, u64 size) {
    u64 num_u64_chunks = (size >> 3);
    u64 whats_left = (size & 0x7);

    u64* ptr64 = (u64*)ptr;
    for (u64 i = 0; i < num_u64_chunks; i++, ptr64++) {
        if (*ptr64 != 0) return false;
    }

    u8* ptr8 = (u8*)ptr64;
    for (u64 i = 0; i < whats_left; i++, ptr8++) {
        if (*ptr8 != 0) return false;
    }

    return true;
}
