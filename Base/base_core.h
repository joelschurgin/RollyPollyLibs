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

// cool for loops
#define DeferBlock(begin, end)        for(i32 _i_ = ((begin), 0); !_i_; _i_ += 1, (end))

// arrays
#define ArrayName(type) Glue(type, Array)

#define Array(arena, num_elements, type) (ArrayName(type)){ .data = push_array((arena), type, (num_elements), true), .count = (num_elements), }
#define DefineArray(type) typedef struct { \
        type * data;                       \
        u64 count;                         \
    } ArrayName(type)

// atomics
#define ATOMIC_MEMORDER __ATOMIC_SEQ_CST
#if COMPILER_CLANG || COMPILER_GCC

// atomic_load:
// return *ptr;
#define atomic_load(ptr)                                    __atomic_load_n(ptr, __ATOMIC_SEQ_CST)

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

// atomic_<op>:
// *ptr += val; *ptr -= val; *ptr &= val; etc..
#define atomic_add(ptr, val)                                __atomic_fetch_add(ptr, val, __ATOMIC_SEQ_CST)
#define atomic_sub(ptr, val)                                __atomic_fetch_sub(ptr, val, __ATOMIC_SEQ_CST)
#define atomic_and(ptr, val)                                __atomic_fetch_and(ptr, val, __ATOMIC_SEQ_CST)
#define atomic_xor(ptr, val)                                __atomic_fetch_xor(ptr, val, __ATOMIC_SEQ_CST)
#define atomic_or(ptr, val)                                 __atomic_fetch_or(ptr, val, __ATOMIC_SEQ_CST)
#define atomic_nand(ptr, val)                               __atomic_fetch_nand(ptr, val, __ATOMIC_SEQ_CST)
#else
#  error Atomic intrinsics not defined for this compiler / architecture.
#endif

#endif
