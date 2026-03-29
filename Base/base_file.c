File file_new_(Arena* arena, String path) {
    return (File){
        .path = string_copy(arena, path),
        .fd = -1,
    };
}

File* file_ptr_new_(Arena* arena, String path) {
    File* f = push_struct(arena, File);
    *f = File(arena, path);
    return f;
}

b8 file_is_open(File* f) {
    return f && f->fd >= 0;
}

u64 file_size(File* f) {
    struct stat st = (struct stat){0};
    if (file_is_open(f)) {
        fstat(f->fd, &st);
    } else if (f && f->path.size > 0) {
        u8 str[f->path.size+1];
        MemoryCopy(str, f->path.str, f->path.size+1);
        stat(str, &st);
    }
    return st.st_size;
}

internal i32 _file_flag_to_prot_flag(FileFlag flag) {
    switch(flag) {
        case FILE_READ_ONLY:  return PROT_READ;
        case FILE_WRITE_ONLY: return PROT_WRITE;
        case FILE_APPEND:     return PROT_WRITE;
    }

    Assert(!"File flag is invalid");
    return -1; // will throw an error if flag is something else
}

internal inline u64 _file_page_aligned_size(u64 size) {
    return AlignPow2(size, PAGE_SIZE);
}

void file_open(File* f, FileFlag flag) {
    f->fd = open(f->path.str, flag);
    Assert(file_is_open(f));

    f->size = file_size(f);
    f->data = MemoryMap(NULL, _file_page_aligned_size(f->size), _file_flag_to_prot_flag(flag), MAP_PRIVATE, f->fd, 0);
}

void file_close(File* f) {
    if (!file_is_open(f)) return;

    MemoryUnmap(f->data, _file_page_aligned_size(f->size));

    close(f->fd);
    f->fd = -1;
}

void file_read_bytes(File* f, u64 file_pos, void* buf, u64 num_bytes_to_read) {
    if (!file_is_open(f)) file_open(f, FILE_READ_ONLY);
    MemoryCopy(buf, f->data + file_pos, num_bytes_to_read);
}
