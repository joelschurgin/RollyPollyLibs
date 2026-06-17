typedef enum FileFlag FileFlag;
enum FileFlag {
    FILE_READ_ONLY = O_RDONLY,
    FILE_WRITE_ONLY = O_WRONLY,
    FILE_APPEND = O_APPEND,
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

void file_open(File* f, FileFlag flag);
void file_close(File* f);

void file_read_bytes(File* f, u64 file_pos, void* buf, u64 num_bytes_to_read);
String file_read_cstring(Arena* arena, File* f, u64 file_pos, u64* num_bytes_read);

#define FileBlock(arena, path, flag, file_ptr_name) \
    for (File* file_ptr_name = FilePtr((arena), (path)); file_ptr_name; ) \
    DeferBlock({ file_open(file_ptr_name, (flag)); }, { file_close(file_ptr_name); file_ptr_name = NULL; })
