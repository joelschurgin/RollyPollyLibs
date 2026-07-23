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
        fdatasync(f->fd); 
        if (fstat(f->fd, &st) < 0) {
            perror("fstat failed");
        }
    } else if (f && f->path.size > 0) {
        u8 str[f->path.size+1];
        MemoryCopy(str, f->path.str, f->path.size+1);
        str[f->path.size] = 0;
        stat(str, &st);
    }
    return st.st_size;
}

internal i32 _file_flag_to_prot_flag(FileFlag flag) {
    flag &= ~FILE_CREATE;
    flag &= ~FILE_CLEAR;
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

void file_open_for_writing(File* f, b32 clear) {
    i32 flags = O_RDWR | O_CREAT;
    if (clear) flags |= O_TRUNC;

    f->fd = open(f->path.str, flags, S_IRUSR | S_IWUSR);
    if (!file_is_open(f)) {
        perror("File Not Open");
        Assert(!"File Not Open");
    }

    f->size = file_size(f);
}

void file_write_bytes(File* f, u64 file_pos, void* buf, u64 num_bytes) {
    u64 file_size = file_pos + num_bytes;
    if (f->size < file_size || !f->data) {
        file_size = Max(file_size, f->size);
        ftruncate(f->fd, file_size);
 
        f->data = MemoryMap(NULL, file_size, PROT_READ | PROT_WRITE, MAP_SHARED, f->fd, 0);
        if (f->data == MAP_FAILED) {
            perror("Error resizing file!!");
        }
    }

    MemoryCopy(f->data + file_pos, buf, num_bytes);
}

void file_modify_bytes(File* f, u64 file_pos, void* buf, u64 num_bytes) {
    file_write_bytes(f, file_pos, buf, num_bytes);
}

void file_open_(File* f, FileFlag flag, i32 other_flags) {
    f->fd = open(f->path.str, flag);
    if (!file_is_open(f)) {
        perror("File Not Open");
        Assert(!"File Not Open");
    }

    f->size = file_size(f);
    f->data = MemoryMap(NULL, _file_page_aligned_size(f->size), _file_flag_to_prot_flag(flag), other_flags, f->fd, 0);
    if (f->data == MAP_FAILED) {
        perror("Error opening file!!");
    }
}

void file_close(File* f) {
    if (!file_is_open(f)) return;

    MemoryUnmap(f->data, _file_page_aligned_size(f->size));

    close(f->fd);
    f->fd = -1;
    f->data = NULL;
}

void file_read_bytes(File* f, u64 file_pos, void* buf, u64 num_bytes_to_read) {
    if (!file_is_open(f)) file_open(f, FILE_READ_ONLY);
    MemoryCopy(buf, f->data + file_pos, num_bytes_to_read);
}

String file_read_cstring_no_copy(File* f, u64 file_pos, u64* num_bytes_read) {
    if (!file_is_open(f)) file_open(f, FILE_READ_ONLY);
    String s = String(f->data + file_pos);
    *num_bytes_read = s.size+1;
    return s;
}

String file_read_cstring(Arena* arena, File* f, u64 file_pos, u64* num_bytes_read) {
    return string_copy(arena, file_read_cstring_no_copy(f, file_pos, num_bytes_read));
}
