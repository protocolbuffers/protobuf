/*
* This is where we define macros used across upb.
*
* All of these macros are undef'd in port_undef.inc to avoid leaking them to
* users.
*
* The correct usage is:
*
*   #include "upb/foobar.h"
*   #include "upb/baz.h"
*
*   // MUST be last included header.
*   #include "upb/port_def.inc"
*
*   // Code for this file.
*   // <...>
*
*   // Can be omitted for .c files, required for .h.
*   #include "upb/port_undef.inc"
*
* This file is private and must not be included by users!
*/

#if !((defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) || \
      (defined(__cplusplus) && __cplusplus >= 201103L) ||           \
      (defined(_MSC_VER) && _MSC_VER >= 1900))
#error upb requires C99 or C++11 or MSVC >= 2015.
#endif

#include <stdint.h>
#include <stddef.h>

#if UINTPTR_MAX == 0xffffffff
#define UPB_SIZE(size32, size64) size32
#else
#define UPB_SIZE(size32, size64) size64
#endif

/* If we always read/write as a consistent type to each address, this shouldn't
 * violate aliasing.
 */
#define UPB_PTR_AT(msg, ofs, type) ((type*)((char*)(msg) + (ofs)))

#define UPB_READ_ONEOF(msg, fieldtype, offset, case_offset, case_val, default) \
  *UPB_PTR_AT(msg, case_offset, int) == case_val                              \
      ? *UPB_PTR_AT(msg, offset, fieldtype)                                   \
      : default

#define UPB_WRITE_ONEOF(msg, fieldtype, offset, value, case_offset, case_val) \
  *UPB_PTR_AT(msg, case_offset, int) = case_val;                             \
  *UPB_PTR_AT(msg, offset, fieldtype) = value;

#define UPB_MAPTYPE_STRING 0

/* UPB_INLINE: inline if possible, emit standalone code if required. */
#ifdef __cplusplus
#define UPB_INLINE inline
#elif defined (__GNUC__) || defined(__clang__)
#define UPB_INLINE static __inline__
#else
#define UPB_INLINE static
#endif

#define UPB_ALIGN_UP(size, align) (((size) + (align) - 1) / (align) * (align))
#define UPB_ALIGN_DOWN(size, align) ((size) / (align) * (align))
#define UPB_ALIGN_MALLOC(size) UPB_ALIGN_UP(size, 16)
#define UPB_ALIGN_OF(type) offsetof (struct { char c; type member; }, member)

/* Hints to the compiler about likely/unlikely branches. */
#if defined (__GNUC__) || defined(__clang__)
#define UPB_LIKELY(x) __builtin_expect((x),1)
#define UPB_UNLIKELY(x) __builtin_expect((x),0)
#else
#define UPB_LIKELY(x) (x)
#define UPB_UNLIKELY(x) (x)
#endif

/* Macros for function attributes on compilers that support them. */
#ifdef __GNUC__
#define UPB_FORCEINLINE __inline__ __attribute__((always_inline))
#define UPB_NOINLINE __attribute__((noinline))
#define UPB_NORETURN __attribute__((__noreturn__))
#elif defined(_MSC_VER)
#define UPB_NOINLINE
#define UPB_FORCEINLINE
#define UPB_NORETURN __declspec(noreturn)
#else  /* !defined(__GNUC__) */
#define UPB_FORCEINLINE
#define UPB_NOINLINE
#define UPB_NORETURN
#endif

#define UPB_MAX(x, y) ((x) > (y) ? (x) : (y))
#define UPB_MIN(x, y) ((x) < (y) ? (x) : (y))

#define UPB_UNUSED(var) (void)var

/* UPB_ASSUME(): in release mode, we tell the compiler to assume this is true.
 */
#ifdef NDEBUG
#ifdef __GNUC__
#define UPB_ASSUME(expr) if (!(expr)) __builtin_unreachable()
#elif defined _MSC_VER
#define UPB_ASSUME(expr) if (!(expr)) __assume(0)
#else
#define UPB_ASSUME(expr) do {} while (false && (expr))
#endif
#else
#define UPB_ASSUME(expr) assert(expr)
#endif

/* UPB_ASSERT(): in release mode, we use the expression without letting it be
 * evaluated.  This prevents "unused variable" warnings. */
#ifdef NDEBUG
#define UPB_ASSERT(expr) do {} while (false && (expr))
#else
#define UPB_ASSERT(expr) assert(expr)
#endif

#if defined(__GNUC__) || defined(__clang__)
#define UPB_UNREACHABLE() do { assert(0); __builtin_unreachable(); } while(0)
#else
#define UPB_UNREACHABLE() do { assert(0); } while(0)
#endif

/* UPB_SETJMP() / UPB_LONGJMP(): avoid setting/restoring signal mask. */
#ifdef __APPLE__
#define UPB_SETJMP(buf) _setjmp(buf)
#define UPB_LONGJMP(buf, val) _longjmp(buf, val)
#else
#define UPB_SETJMP(buf) setjmp(buf)
#define UPB_LONGJMP(buf, val) longjmp(buf, val)
#endif

/* Configure whether fasttable is switched on or not. *************************/

#if defined(__x86_64__) && defined(__GNUC__)
#define UPB_FASTTABLE_SUPPORTED 1
#else
#define UPB_FASTTABLE_SUPPORTED 0
#endif

/* define UPB_ENABLE_FASTTABLE to force fast table support.
 * This is useful when we want to ensure we are really getting fasttable,
 * for example for testing or benchmarking. */
#if defined(UPB_ENABLE_FASTTABLE)
#if !UPB_FASTTABLE_SUPPORTED
#error fasttable is x86-64 + Clang/GCC only
#endif
#define UPB_FASTTABLE 1
/* Define UPB_TRY_ENABLE_FASTTABLE to use fasttable if possible.
 * This is useful for releasing code that might be used on multiple platforms,
 * for example the PHP or Ruby C extensions. */
#elif defined(UPB_TRY_ENABLE_FASTTABLE)
#define UPB_FASTTABLE UPB_FASTTABLE_SUPPORTED
#else
#define UPB_FASTTABLE 0
#endif

/* UPB_FASTTABLE_INIT() allows protos compiled for fasttable to gracefully
 * degrade to non-fasttable if we are using UPB_TRY_ENABLE_FASTTABLE. */
#if !UPB_FASTTABLE && defined(UPB_TRY_ENABLE_FASTTABLE)
#define UPB_FASTTABLE_INIT(...)
#else
#define UPB_FASTTABLE_INIT(...) __VA_ARGS__
#endif

#undef UPB_FASTTABLE_SUPPORTED

/* ASAN poisoning (for arena) *************************************************/

#if defined(__SANITIZE_ADDRESS__)
#define UPB_ASAN 1
#ifdef __cplusplus
extern "C" {
#endif
void __asan_poison_memory_region(void const volatile *addr, size_t size);
void __asan_unpoison_memory_region(void const volatile *addr, size_t size);
#ifdef __cplusplus
}  /* extern "C" */
#endif
#define UPB_POISON_MEMORY_REGION(addr, size) \
  __asan_poison_memory_region((addr), (size))
#define UPB_UNPOISON_MEMORY_REGION(addr, size) \
  __asan_unpoison_memory_region((addr), (size))
#else
#define UPB_ASAN 0
#define UPB_POISON_MEMORY_REGION(addr, size) \
  ((void)(addr), (void)(size))
#define UPB_UNPOISON_MEMORY_REGION(addr, size) \
  ((void)(addr), (void)(size))
#endif 
