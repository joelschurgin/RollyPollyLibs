#ifndef BASE_CORE_H
#define BASE_CORE_H

#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>

#define internal      static
#define global        static
#define local_persist static

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;
typedef u8       b8;
typedef u16      b16;
typedef u32      b32;
typedef u64      b64;
typedef float    f32;
typedef double   f64;

// units

#define KB(n)  (((u64)(n)) << 10)
#define MB(n)  (((u64)(n)) << 20)
#define GB(n)  (((u64)(n)) << 30)
#define TB(n)  (((u64)(n)) << 40)
#define Thousand(n)   ((n)*1000)
#define Million(n)    ((n)*1000000)
#define Billion(n)    ((n)*1000000000)

// clamping

#define Min(A,B) (((A)<(B))?(A):(B))
#define Max(A,B) (((A)>(B))?(A):(B))
#define ClampTop(A,X) Min(A,X)
#define ClampBot(X,B) Max(X,B)
#define Clamp(A,X,B) (((X)<(A))?(A):((X)>(B))?(B):(X))

// traps

#if ARCH_X64
# define Trap() __builtin_trap()
#else
# error No Trap defined for this architecture
#endif

// alignment

#if COMPILER_GCC
# define AlignOf(T) __alignof__(T)
#else
# error AlignOf not defined for this compiler
#endif

#if COMPILER_CLANG || COMPILER_GCC
# define AlignType(x) __attribute__((aligned(x)))
#else
# error AlignType not defined for this compiler.
#endif

// asserts

#define AssertAlways(x) do{ if(!(x)) { Trap(); } } while(0)
#if BUILD_DEBUG
# define Assert(x) AssertAlways(x)
#else
# define Assert(x) (void)(x)
#endif
#define InvalidPath     Assert(!"Invalid Path!")
#define NotImplemented  Assert(!"Not Implemented!")
#define StaticAssert(condition, ID) global u8 Glue(ID, __LINE__)[(condition)?1:-1]

// misc

#define PAGE_SIZE sysconf(_SC_PAGE_SIZE)

#define Glue_(A,B) A##B
#define Glue(A,B) Glue_(A,B)

#define Stringify_(S) #S
#define Stringify(S) Stringify_(S)

#define ArrayCount(a) (sizeof(a) / sizeof((a)[0]))

#define CeilIntegerDiv(a,b) (((a) + (b) - 1)/(b))
#define CeilIntDiv(a,b) CeilIntegerDiv(a,b)

#if ARCH_64BIT
# define IntFromPtr(ptr) ((U64)(ptr))
#elif ARCH_32BIT
# define IntFromPtr(ptr) ((U32)(ptr))
#else
# error Missing pointer-to-integer cast for this architecture.
#endif
#define PtrFromInt(i) (void*)(i)

#define AlignPow2(x,b)     (((x) + (b) - 1) & (~((b) - 1)))
#define AlignDownPow2(x,b) ((x) & (~((b) - 1)))
#define AlignPadPow2(x,b)  ((0 - (x)) & ((b) - 1))
#define IsPow2(x)          ((x) != 0 && ((x) & ((x) - 1)) == 0)
#define IsPow2OrZero(x)    ((((x) - 1) & (x)) == 0)

// memory operations
#define MemoryMap(addr, len, protection, flags, fd, offset) mmap((addr), (len), (protection), (flags), (fd), (offset))
#define MemoryUnmap(addr, len)                              munmap((addr), (len))
#define MemoryProtect(addr, len, protection)                mprotect((addr), (len), (protection))

#define MemoryCopy(dst, src, size)    memmove((dst), (src), (size))
#define MemorySet(dst, byte, size)    memset((dst), (byte), (size))
#define MemoryCompare(a, b, size)     memcmp((a), (b), (size))
#define MemoryStrlen(ptr)             strlen(ptr)

#define MemoryCopyStruct(d,s)  MemoryCopy((d),(s),sizeof(*(d)))
#define MemoryCopyArray(d,s)   MemoryCopy((d),(s),sizeof(d))
#define MemoryCopyTyped(d,s,c) MemoryCopy((d),(s),sizeof(*(d))*(c))
#define MemoryCopyStr8(dst, s) MemoryCopy(dst, (s).str, (s).size)

#define MemoryZero(s,z)       memset((s),0,(z))
#define MemoryZeroStruct(s)   MemoryZero((s),sizeof(*(s)))
#define MemoryZeroArray(a)    MemoryZero((a),sizeof(a))
#define MemoryZeroTyped(m,c)  MemoryZero((m),sizeof(*(m))*(c))

#define MemoryMatch(a,b,z)     (MemoryCompare((a),(b),(z)) == 0)
#define MemoryMatchStruct(a,b)  MemoryMatch((a),(b),sizeof(*(a)))
#define MemoryMatchArray(a,b)   MemoryMatch((a),(b),sizeof(a))

#define MemoryIsZeroStruct(ptr) memory_is_zero((ptr), sizeof(*(ptr)))

b32 memory_is_zero(void* ptr, u64 size);

#define CheckNil(nil,p) ((p) == 0 || (p) == nil)
#define SetNil(nil,p) ((p) = nil)

// doubly-linked-lists
#define DLLInsert_NPZ(nil,f,l,p,n,next,prev) (CheckNil(nil,f) ? \
    ((f) = (l) = (n), SetNil(nil,(n)->next), SetNil(nil,(n)->prev)) :\
    CheckNil(nil,p) ? \
    ((n)->next = (f), (f)->prev = (n), (f) = (n), SetNil(nil,(n)->prev)) :\
    ((p)==(l)) ? \
    ((l)->next = (n), (n)->prev = (l), (l) = (n), SetNil(nil, (n)->next)) :\
    (((!CheckNil(nil,p) && CheckNil(nil,(p)->next)) ? (0) : ((p)->next->prev = (n))), ((n)->next = (p)->next), ((p)->next = (n)), ((n)->prev = (p))))
#define DLLPushBack_NPZ(nil,f,l,n,next,prev) DLLInsert_NPZ(nil,f,l,l,n,next,prev)
#define DLLPushFront_NPZ(nil,f,l,n,next,prev) DLLInsert_NPZ(nil,l,f,f,n,prev,next)
#define DLLRemove_NPZ(nil,f,l,n,next,prev) (((n) == (f) ? (f) = (n)->next : (0)),\
    ((n) == (l) ? (l) = (l)->prev : (0)),\
    (CheckNil(nil,(n)->prev) ? (0) :\
    ((n)->prev->next = (n)->next)),\
    (CheckNil(nil,(n)->next) ? (0) :\
    ((n)->next->prev = (n)->prev)))

// singly-linked, doubly-headed lists (queues)
#define SLLQueuePush_NZ(nil,f,l,n,next) (CheckNil(nil,f)?\
    ((f)=(l)=(n),SetNil(nil,(n)->next)):\
    ((l)->next=(n),(l)=(n),SetNil(nil,(n)->next)))
#define SLLQueuePushFront_NZ(nil,f,l,n,next) (CheckNil(nil,f)?\
    ((f)=(l)=(n),SetNil(nil,(n)->next)):\
    ((n)->next=(f),(f)=(n)))
#define SLLQueuePop_NZ(nil,f,l,next) ((f)==(l)?\
    (SetNil(nil,f),SetNil(nil,l)):\
    ((f)=(f)->next))

// singly-linked, singly-headed lists (stacks)
#define SLLStackPush_N(f,n,next) ((n)->next=(f), (f)=(n))
#define SLLStackPop_N(f,next) ((f)=(f)->next)

// doubly-linked-list helpers
#define DLLInsert_NP(f,l,p,n,next,prev) DLLInsert_NPZ(0,f,l,p,n,next,prev)
#define DLLPushBack_NP(f,l,n,next,prev) DLLPushBack_NPZ(0,f,l,n,next,prev)
#define DLLPushFront_NP(f,l,n,next,prev) DLLPushFront_NPZ(0,f,l,n,next,prev)
#define DLLRemove_NP(f,l,n,next,prev) DLLRemove_NPZ(0,f,l,n,next,prev)
#define DLLInsert(f,l,p,n) DLLInsert_NPZ(0,f,l,p,n,next,prev)
#define DLLPushBack(f,l,n) DLLPushBack_NPZ(0,f,l,n,next,prev)
#define DLLPushFront(f,l,n) DLLPushFront_NPZ(0,f,l,n,next,prev)
#define DLLRemove(f,l,n) DLLRemove_NPZ(0,f,l,n,next,prev)

// singly-linked, doubly-headed list helpers
#define SLLQueuePush_N(f,l,n,next) SLLQueuePush_NZ(0,f,l,n,next)
#define SLLQueuePushFront_N(f,l,n,next) SLLQueuePushFront_NZ(0,f,l,n,next)
#define SLLQueuePop_N(f,l,next) SLLQueuePop_NZ(0,f,l,next)
#define SLLQueuePush(f,l,n) SLLQueuePush_NZ(0,f,l,n,next)
#define SLLQueuePushFront(f,l,n) SLLQueuePushFront_NZ(0,f,l,n,next)
#define SLLQueuePop(f,l) SLLQueuePop_NZ(0,f,l,next)

// singly-linked, singly-headed list helpers
#define SLLStackPush(f,n) SLLStackPush_N(f,n,next)
#define SLLStackPop(f) SLLStackPop_N(f,next)

// cool for loops
#define DeferBlock(begin, end)        for(i32 _i_ = ((begin), 0); !_i_; _i_ += 1, (end))
#define EachCharUntil(iter, string, cond) (u8* iter = (string).str; (u64)(iter - (string).str) < (string).size && !(cond); iter++)
#define EachChar(iter, string) EachCharUntil(iter, string, false)
#define EachCharContinueUntil(iter, string, cond) (; (u64)(iter - (string).str) < (string).size && !(cond); iter++)
#define EachCharContinue(iter, string) EachCharContinueUntil(iter, string, false)

// arrays
#define ArrayName(type) Glue(type, Array)

#define Array(arena, type, num_elements) (ArrayName(type)){ .data = push_array((arena), type, (num_elements), true), .count = (num_elements), }
#define DefineArray(type) typedef struct { \
        type * data;                       \
        u64 count;                         \
    } ArrayName(type)

DefineArray(u64);
DefineArray(u32);
DefineArray(u16);
DefineArray(u8);

DefineArray(i64);
DefineArray(i32);
DefineArray(i16);
DefineArray(i8);

DefineArray(b64);
DefineArray(b32);
DefineArray(b16);
DefineArray(b8);

DefineArray(f64);
DefineArray(f32);

// atomics
#define ATOMIC_MEMORDER __ATOMIC_SEQ_CST
#if COMPILER_CLANG || COMPILER_GCC

// atomic_load:
// return *ptr;
#define atomic_load(ptr)                                    __atomic_load_n(ptr, __ATOMIC_SEQ_CST)
#define atomic_load_ptr(ptr)                                __atomic_load_n(&ptr, __ATOMIC_SEQ_CST)

// atomic_store:
// *ptr = val;
#define atomic_store(ptr, val)                              __atomic_store_n(ptr, val, __ATOMIC_SEQ_CST)

// atomic_exchange
// return *ptr;
// *ptr = val;
#define atomic_exchange(ptr, val)                           __atomic_exchange_n(ptr, val, __ATOMIC_SEQ_CST)

// atomic_compare_exchange:
// if (*ptr == *expect) {
//      *ptr = val_if_true;
//      return true;
// } else {
//      return false;
// }
#define atomic_compare_exchange(ptr, expected, val_if_true) __atomic_compare_exchange_n(ptr, expected, val_if_true, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)

// atomic_fetch_<op>:
// *ptr += val; *ptr -= val; *ptr &= val; etc..
#define atomic_fetch_add(ptr, val)                                __atomic_fetch_add(ptr, val, __ATOMIC_SEQ_CST)
#define atomic_fetch_sub(ptr, val)                                __atomic_fetch_sub(ptr, val, __ATOMIC_SEQ_CST)
#define atomic_fetch_and(ptr, val)                                __atomic_fetch_and(ptr, val, __ATOMIC_SEQ_CST)
#define atomic_fetch_xor(ptr, val)                                __atomic_fetch_xor(ptr, val, __ATOMIC_SEQ_CST)
#define atomic_fetch_or(ptr, val)                                 __atomic_fetch_or(ptr, val, __ATOMIC_SEQ_CST)
#define atomic_fetch_nand(ptr, val)                               __atomic_fetch_nand(ptr, val, __ATOMIC_SEQ_CST)

// atomic
#define atomic_add_fetch(ptr, val)                                __atomic_add_fetch(ptr, val, __ATOMIC_SEQ_CST)
#define atomic_sub_fetch(ptr, val)                                __atomic_sub_fetch(ptr, val, __ATOMIC_SEQ_CST)
#define atomic_and_fetch(ptr, val)                                __atomic_and_fetch(ptr, val, __ATOMIC_SEQ_CST)
#define atomic_xor_fetch(ptr, val)                                __atomic_xor_fetch(ptr, val, __ATOMIC_SEQ_CST)
#define atomic_or_fetch(ptr, val)                                 __atomic_or_fetch(ptr, val, __ATOMIC_SEQ_CST)
#define atomic_nand_fetch(ptr, val)                               __atomic_nand_fetch(ptr, val, __ATOMIC_SEQ_CST)
#else
#  error Atomic intrinsics not defined for this compiler / architecture.
#endif

#endif
