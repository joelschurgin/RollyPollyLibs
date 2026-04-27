#ifndef BASE_QUEUE_H
#define BASE_QUEUE_H

typedef struct {
    void* data;
    u64 element_size;

    u64 capacity;
    u64 first_idx;
    u64 last_idx;

    Mutex *mutex;
} AtomicQueue;

AtomicQueue*  atomic_queue_create(Arena *arena, u64 capacity, u64 element_size);
u64           atomic_queue_size(AtomicQueue *Q);
void          atomic_queue_push(AtomicQueue *Q, void* data_in);
void          atomic_queue_pop(AtomicQueue *Q, void* data_out);

#define AtomicQueueName(type) Glue(type, Queue);

#define ImplementAtomicQueue(type, func_prefix) \
AtomicQueue* Glue(func_prefix, _create) (Arena* arena, u64 capacity) { \
    return atomic_queue_create(arena, capacity, sizeof(type)); \
} \
u64 Glue(func_prefix, _size)(AtomicQueue* Q) { \
    return atomic_queue_size(Q); \
} \
void Glue(func_prefix, _push)(AtomicQueue* Q, type data) { \
    atomic_queue_push(Q, &data); \
} \
type Glue(func_prefix, _pop)(AtomicQueue* Q) { \
    type data = (type){0}; \
    atomic_queue_pop(Q, &data); \
    return data; \
}

#define DefineAtomicQueue(type, func_prefix) \
    typedef AtomicQueue AtomicQueueName(type); \
    AtomicQueue* Glue(func_prefix, _create)(Arena* arena, u64 capacity); \
    u64          Glue(func_prefix, _size)(AtomicQueue* Q); \
    void         Glue(func_prefix, _push)(AtomicQueue* Q, type data); \
    type         Glue(func_prefix, _pop)(AtomicQueue* Q);

/*
 * Example for creating a queue
 *
 *
 * HEADER FILE:
 *
 * typedef struct {} MyStuff;
 *
 * DefineAtomicQueue(MyStuff, mystuff_queue)
 * #define Implement_MyStuffQueue Implement(MyStuff, mystuff_queue)
 *
 *
 * C FILE or wherever you want the implementation to be:
 *
 * Implement_MyStuffQueue;
 *
 * ....rest of your C code goes here
 */

#endif
