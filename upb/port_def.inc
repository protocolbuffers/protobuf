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
#ifndef UINTPTR_MAX
#error must include stdint.h first
#endif

#if UINTPTR_MAX == 0xffffffff
#define UPB_SIZE(size32, size64) size32
#else
#define UPB_SIZE(size32, size64) size64
#endif

#define UPB_FIELD_AT(msg, fieldtype, offset) \
  *(fieldtype*)((const char*)(msg) + offset)

#define UPB_READ_ONEOF(msg, fieldtype, offset, case_offset, case_val, default) \
  UPB_FIELD_AT(msg, int, case_offset) == case_val                              \
      ? UPB_FIELD_AT(msg, fieldtype, offset)                                   \
      : default

#define UPB_WRITE_ONEOF(msg, fieldtype, offset, value, case_offset, case_val) \
  UPB_FIELD_AT(msg, int, case_offset) = case_val;                             \
  UPB_FIELD_AT(msg, fieldtype, offset) = value;

/* UPB_INLINE: inline if possible, emit standalone code if required. */
#ifdef __cplusplus
#define UPB_INLINE inline
#elif defined (__GNUC__) || defined(__clang__)
#define UPB_INLINE static __inline__
#else
#define UPB_INLINE static
#endif

/* Hints to the compiler about likely/unlikely branches. */
#if defined (__GNUC__) || defined(__clang__)
#define UPB_LIKELY(x) __builtin_expect((x),1)
#define UPB_UNLIKELY(x) __builtin_expect((x),0)
#else
#define UPB_LIKELY(x) (x)
#define UPB_UNLIKELY(x) (x)
#endif

/* Define UPB_BIG_ENDIAN manually if you're on big endian and your compiler
 * doesn't provide these preprocessor symbols. */
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#define UPB_BIG_ENDIAN
#endif

/* Macros for function attributes on compilers that support them. */
#ifdef __GNUC__
#define UPB_FORCEINLINE __inline__ __attribute__((always_inline))
#define UPB_NOINLINE __attribute__((noinline))
#define UPB_NORETURN __attribute__((__noreturn__))
#else  /* !defined(__GNUC__) */
#define UPB_FORCEINLINE
#define UPB_NOINLINE
#define UPB_NORETURN
#endif

#if __STDC_VERSION__ >= 199901L || __cplusplus >= 201103L
/* C99/C++11 versions. */
#include <stdio.h>
#define _upb_snprintf snprintf
#define _upb_vsnprintf vsnprintf
#define _upb_va_copy(a, b) va_copy(a, b)
#elif defined(_MSC_VER)
/* Microsoft C/C++ versions. */
#include <stdarg.h>
#include <stdio.h>
#if _MSC_VER < 1900
int msvc_snprintf(char* s, size_t n, const char* format, ...);
int msvc_vsnprintf(char* s, size_t n, const char* format, va_list arg);
#define UPB_MSVC_VSNPRINTF
#define _upb_snprintf msvc_snprintf
#define _upb_vsnprintf msvc_vsnprintf
#else
#define _upb_snprintf snprintf
#define _upb_vsnprintf vsnprintf
#endif
#define _upb_va_copy(a, b) va_copy(a, b)
#elif defined __GNUC__
/* A few hacky workarounds for functions not in C89.
 * For internal use only!
 * TODO(haberman): fix these by including our own implementations, or finding
 * another workaround.
 */
#define _upb_snprintf __builtin_snprintf
#define _upb_vsnprintf __builtin_vsnprintf
#define _upb_va_copy(a, b) __va_copy(a, b)
#else
#error Need implementations of [v]snprintf and va_copy
#endif

#ifdef __cplusplus
#if __cplusplus >= 201103L || defined(__GXX_EXPERIMENTAL_CXX0X__) || \
    (defined(_MSC_VER) && _MSC_VER >= 1900)
// C++11 is present
#else
#error upb requires C++11 for C++ support
#endif
#endif

#define UPB_MAX(x, y) ((x) > (y) ? (x) : (y))
#define UPB_MIN(x, y) ((x) < (y) ? (x) : (y))

#define UPB_UNUSED(var) (void)var

/* UPB_ASSERT(): in release mode, we use the expression without letting it be
 * evaluated.  This prevents "unused variable" warnings. */
#ifdef NDEBUG
#define UPB_ASSERT(expr) do {} while (false && (expr))
#else
#define UPB_ASSERT(expr) assert(expr)
#endif

/* UPB_ASSERT_DEBUGVAR(): assert that uses functions or variables that only
 * exist in debug mode.  This turns into regular assert. */
#define UPB_ASSERT_DEBUGVAR(expr) assert(expr)

#if defined(__GNUC__) || defined(__clang__)
#define UPB_UNREACHABLE() do { assert(0); __builtin_unreachable(); } while(0)
#else
#define UPB_UNREACHABLE() do { assert(0); } while(0)
#endif

/* UPB_INFINITY representing floating-point positive infinity. */
#include <math.h>
#ifdef INFINITY
#define UPB_INFINITY INFINITY
#else
#define UPB_INFINITY (1.0 / 0.0)
#endif
