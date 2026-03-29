// Compiler

#if defined(__GNUC__) || defined(__GNUG__)

# define COMPILER_GCC 1

# if defined(__gnu_linux__) || defined(__linux__)
#  define OS_LINUX 1
# else
#  error This compiler/OS combo is not supported.
# endif

# if defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64)
#  define ARCH_X64 1
// not supporting these architectures for now

//# elif defined(i386) || defined(__i386) || defined(__i386__)
//#  define ARCH_X86 1
//# elif defined(__aarch64__)
//#  define ARCH_ARM64 1
//# elif defined(__arm__)
//#  define ARCH_ARM32 1
# else
#  error Architecture not supported.
# endif

#else
# error Compiler not supported.
#endif

// Architecture

#if defined(ARCH_X64)
# define ARCH_64BIT 1
#elif defined(ARCH_X86)
# define ARCH_32BIT 1
#endif

#if ARCH_ARM32 || ARCH_ARM64 || ARCH_X64 || ARCH_X86
# define ARCH_LITTLE_ENDIAN 1
#else
# error Endianness of this architecture not understood by context cracker.
#endif

// language

#if defined(__cplusplus)
# define LANG_CPP 1
#else
# define LANG_C 1
#endif

#if LANG_CPP
# define C_LINKAGE_BEGIN extern "C"{
# define C_LINKAGE_END }
# define C_LINKAGE extern "C"
#else
# define C_LINKAGE_BEGIN
# define C_LINKAGE_END
# define C_LINKAGE
#endif

// build options

#if !defined(BUILD_DEBUG)
# define BUILD_DEBUG 0
#endif

