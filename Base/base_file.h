typedef enum FileFlag FileFlag;
enum FileFlag {
    FILE_READ_ONLY = O_RDONLY,
    FILE_WRITE_ONLY = O_WRONLY,
    FILE_APPEND = O_APPEND,
    FILE_CREATE = O_CREAT,
    FILE_CLEAR = O_TRUNC,
};

typedef struct File File;
struct File {
    String path;
    u64 size;
    void* data;
    i32 fd;
};

File file_new_(Arena* arena, String path);
File* file_ptr_new_(Arena* arena, String path);
#define File(arena, path) file_new_((arena), (path))
#define FilePtr(arena, path) file_ptr_new_((arena), (path))

b8 file_is_open(File* f);
u64 file_size(File* f);

void file_open_for_writing(File* f, b32 clear);

void file_write_bytes(File* f, u64 file_pos, void* buf, u64 num_bytes);
#define file_write_struct(f, file_pos, val) file_write_bytes((f), (file_pos), &(val), sizeof(val))

void file_open_(File* f, FileFlag flag, i32 other_flags);
#define file_open(f, flag) file_open_((f), (flag), MAP_PRIVATE)
void file_close(File* f);

void file_read_bytes(File* f, u64 file_pos, void* buf, u64 num_bytes_to_read);
String file_read_cstring_no_copy(File* f, u64 file_pos, u64* num_bytes_read);
String file_read_cstring(Arena* arena, File* f, u64 file_pos, u64* num_bytes_read);

#define FileBlock(arena, path, flag, file_ptr_name) \
    for (File* file_ptr_name = FilePtr((arena), (path)); file_ptr_name; ) \
    DeferBlock({ file_open(file_ptr_name, (flag)); }, { file_close(file_ptr_name); file_ptr_name = NULL; })

#define FileRead(arena, path, file_ptr_name) FileBlock(arena, path, FILE_READ_ONLY)

// does not clear file
#define FileModify(arena, path, file_ptr_name) \
    DeclareLocal(File* file_ptr_name = FilePtr((arena), (path))) \
    DeferBlock({ file_open_for_writing(file_ptr_name, false); }, { file_close(file_ptr_name); file_ptr_name = 0L; })

// does clear file
#define FileWrite(arena, path, file_ptr_name) \
    DeclareLocal(File* file_ptr_name = FilePtr((arena), (path))) \
    DeferBlock({ file_open_for_writing(file_ptr_name, true); }, { file_close(file_ptr_name); file_ptr_name = 0L; })
