#include "moonfruit.h"

MoonFruit_ChunkQueue* moonfruit_chunk_queue_create(Arena* arena, u64 capacity) {
    MoonFruit_ChunkQueue* Q = push_struct(arena, MoonFruit_ChunkQueue);
    Q->chunks = push_array(arena, MoonFruit_Chunk, capacity, true);
    Q->capacity = capacity;
    return Q;
}

u64 moonfruit_chunk_queue_size(MoonFruit_ChunkQueue* Q) {
    u64 capacity = atomic_load(&Q->capacity);
    u64 first_idx = atomic_load(&Q->first_idx);
    u64 last_idx = atomic_load(&Q->last_idx);
    return (last_idx + capacity - first_idx) % capacity;
}

void moonfruit_chunk_queue_push(MoonFruit_ChunkQueue* Q, MoonFruit_Chunk chunk) {
    DeferBlock(mutex_lock(Q->mutex), mutex_unlock(Q->mutex)) {
        u64 size = moonfruit_chunk_queue_size(Q);
        u64 capacity = atomic_load(&Q->capacity);
        if (size+1 >= capacity) {
            Assert(!"Not enough space in the chunk queue");
        }

        Q->chunks[Q->last_idx] = chunk;
        atomic_fetch_add(&Q->last_idx, 1);
        atomic_compare_exchange(&Q->last_idx, &capacity, 0);
    }
}

MoonFruit_Chunk moonfruit_chunk_queue_pop(MoonFruit_ChunkQueue* Q) {
    MoonFruit_Chunk chunk = (MoonFruit_Chunk){0};
    DeferBlock(mutex_lock(Q->mutex), mutex_unlock(Q->mutex)) {
        u64 size = moonfruit_chunk_queue_size(Q);
        if (size != 0) {
            u64 capacity = atomic_load(&Q->capacity);
            chunk = Q->chunks[Q->first_idx];
            atomic_fetch_add(&Q->first_idx, 1);
            atomic_compare_exchange(&Q->first_idx, &capacity, 0);
        }
    }
    return chunk;
}

void moonfruit_chunk_process(MoonFruit_Chunk chunk, MoonFruit_ChunkQueue* Q) {
    if (moonfruit_chunk_empty(chunk)) return;

    b8 adjust_beginning_of_chunk = (!chunk.first_chunk_in_file && *(chunk.text.str-1) != '\n');
    if (adjust_beginning_of_chunk) {
        u8* c = chunk.text.str;
        for (u64 idx = 0; idx < chunk.text.size; idx++) {
            if (*c == '\n') {
                chunk.text = string_skip(chunk.text, idx);
                break;
            }
            c++;
        }
    }

    b8 adjust_end_of_chunk = (!chunk.last_chunk_in_file && chunk.text.str[chunk.text.size-1] != '\n');
    if (adjust_end_of_chunk) {
        u8* c = &chunk.text.str[chunk.text.size-1];
        for (u64 idx = chunk.text.size; idx < chunk.text.size*2; idx++, c++) {
            if (*c == '\n') {
                chunk.text.size = idx;
                break;
            }
        }
    }

    u64 num_lines = 0;
    for (u64 idx = 0; idx < chunk.text.size; idx++) {
        if (chunk.text.str[idx] == '\n') num_lines++;
    }

    u64 num_chunks = atomic_load(&chunk.file->per_chunk_line_nums.count);
    for (u64 idx = chunk.per_file_chunk_idx+1; idx <= num_chunks; idx++) {
        atomic_fetch_add(&chunk.file->per_chunk_line_nums.data[idx], num_lines);
    }

    printf("===================CHUNK===================\n"
            "LAST CHUNK => %d\n"
            "Chunk Idx  => %d\n"
            "num_lines  => %d\n"
            "------------------------------------------\n"
            "%.*s\n",
           chunk.last_chunk_in_file,
           chunk.per_file_chunk_idx,
           num_lines,
           chunk.text.size,
           chunk.text.str);

    if (!chunk.last_chunk_in_file) {
        moonfruit_file_push_next_chunk(chunk.file, Q);
        return;
    }

    // still not the right place because this chunk could get processed before the rest of the chunks
    /*
    i64 fd = atomic_load(&chunk.file->file.fd);
    if (fd >= 0) {
        DeferBlock(mutex_lock(Q->mutex), mutex_unlock(Q->mutex)) {
            moonfruit_file_close(chunk.file);
        }
    }
    */
}

MoonFruit_File* moonfruit_file_create_and_open(Arena* arena, String path) {
    MoonFruit_File* f = push_struct(arena, MoonFruit_File);
    f->file = File(arena, path);
    f->pos = 0;

    moonfruit_file_open(f);

    u64 num_chunks_plus_one = 1+CeilIntDiv(f->file.size, MOONFRUIT_CHUNK_SIZE);
    f->per_chunk_line_nums = Array(arena, num_chunks_plus_one, u64);
    for (u64 i = 0; i < num_chunks_plus_one; i++) {
        f->per_chunk_line_nums.data[i] = 1;
    }
    return f;
}

void moonfruit_file_open(MoonFruit_File* f) {
    file_open(&f->file, FILE_READ_ONLY);
}

void moonfruit_file_close(MoonFruit_File* f) {
    file_close(&f->file);
}

void moonfruit_file_push_next_chunk(MoonFruit_File* f, MoonFruit_ChunkQueue* Q) {
    u64 file_pos  = atomic_load(&f->pos);
    u64 file_size = atomic_load(&f->file.size);

    if (file_pos < file_size) {
        u64 new_size = Min(file_size - file_pos, MOONFRUIT_CHUNK_SIZE);
        u64 old_chunk_count = atomic_fetch_add(&f->chunk_count, 1);
        MoonFruit_Chunk next_chunk = (MoonFruit_Chunk){
            .file = f,
            .text = (String){
                .str = f->file.data + file_pos,
                .size = new_size,
            },
            .pos = 0,
            .per_file_chunk_idx = old_chunk_count,
            .first_chunk_in_file = (file_pos == 0),
            .last_chunk_in_file = (file_pos + new_size >= file_size),
        };
        atomic_fetch_add(&f->pos, new_size);
        moonfruit_chunk_queue_push(Q, next_chunk);
        return;
    }
}
