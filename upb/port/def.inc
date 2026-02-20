// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

/*
 * This is where we define internal portability macros used across upb.
 *
 * All of these macros are undef'd in undef.inc to avoid leaking them to users.
 *
 * The correct usage is:
 *
 *   #include "upb/foobar.h"
 *   #include "upb/baz.h"
 *
 *   // MUST be last included header.
 *   #include "upb/port/def.inc"
 *
 *   // Code for this file.
 *   // <...>
 *
 *   // Can be omitted for .c files, required for .h.
 *   #include "upb/port/undef.inc"
 *
 * This file is private and must not be included by users!
 */

#if !((defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) || \
      (defined(__cplusplus) && __cplusplus >= 201703L) ||           \
      (defined(_MSC_VER) && _MSC_VER >= 1900))
#error upb requires C99 or C++17 or MSVC >= 2015.
#endif

// Portable check for GCC minimum version:
// https://gcc.gnu.org/onlinedocs/cpp/Common-Predefined-Macros.html
#if defined(__GNUC__) && defined(__GNUC_MINOR__) && defined(__GNUC_PATCHLEVEL__)
#define UPB_GNUC_MIN(x, y) \
  (__GNUC__ > (x) || __GNUC__ == (x) && __GNUC_MINOR__ >= (y))
#else
#define UPB_GNUC_MIN(x, y) 0
#endif

// Macros for checking for compiler attributes, defined here to avoid the
// problem described in
// https://gcc.gnu.org/onlinedocs/cpp/_005f_005fhas_005fattribute.html.
#ifdef __has_attribute
#define UPB_HAS_ATTRIBUTE(x) __has_attribute(x)
#else
#define UPB_HAS_ATTRIBUTE(x) 0
#endif

#if defined(__cplusplus) && defined(__has_cpp_attribute)
// NOTE: requiring __cplusplus above should not be necessary, but
// works around https://bugs.llvm.org/show_bug.cgi?id=23435.
#define UPB_HAS_CPP_ATTRIBUTE(x) __has_cpp_attribute(x)
#else
#define UPB_HAS_CPP_ATTRIBUTE(x) 0
#endif

// Once in a while we want to use this macro in a C++-only portion of an
// otherwise C-compatible header, so we copy and paste this from ABSL.
#if UPB_HAS_CPP_ATTRIBUTE(deprecated) && UPB_HAS_CPP_ATTRIBUTE(clang::annotate)
#define UPB_DEPRECATE_AND_INLINE() [[deprecated, clang::annotate("inline-me")]]
#elif UPB_HAS_CPP_ATTRIBUTE(deprecated)
#define UPB_DEPRECATE_AND_INLINE() [[deprecated]]
#else
#define UPB_DEPRECATE_AND_INLINE()
#endif

#ifdef __has_builtin
#define UPB_HAS_BUILTIN(x) __has_builtin(x)
#else
#define UPB_HAS_BUILTIN(x) 0
#endif

#ifdef __has_extension
#define UPB_HAS_EXTENSION(x) __has_extension(x)
#else
#define UPB_HAS_EXTENSION(x) 0
#endif

#ifdef __has_feature
#define UPB_HAS_FEATURE(x) __has_feature(x)
#else
#define UPB_HAS_FEATURE(x) 0
#endif

#include <assert.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef UINTPTR_MAX
Error, UINTPTR_MAX is undefined
#endif

#if UINTPTR_MAX == 0xffffffff
#define UPB_SIZE(size32, size64) size32
#else
#define UPB_SIZE(size32, size64) size64
#endif

/* If we always read/write as a consistent type to each address, this shouldn't
 * violate aliasing.
 */
#define UPB_PTR_AT(msg, ofs, type) ((type*)((char*)(msg) + (ofs)))

// A flexible array member may have lower alignment requirements than the struct
// overall - in that case, it can overlap with the trailing padding of the rest
// of the struct, and a naive sizeof(base) + sizeof(flex) * count calculation
// will not take into account that overlap, and allocate more than is required.
#define UPB_SIZEOF_FLEX(type, member, count) \
  UPB_MAX(sizeof(type), offsetof(type, member[count]))

#define UPB_SIZEOF_FLEX_WOULD_OVERFLOW(type, member, count) \
  (((SIZE_MAX - offsetof(type, member[0])) /                \
    (offsetof(type, member[1]) - offsetof(type, member[0]))) < (size_t)count)

#define UPB_ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

#define UPB_MAPTYPE_STRING 0

// UPB_EXPORT: always generate a public symbol.
#if defined(__GNUC__) || defined(__clang__)
#define UPB_EXPORT __attribute__((visibility("default"))) __attribute__((used))
#else
#define UPB_EXPORT
#endif

// UPB_INLINE: inline if possible, emit standalone code if required.
#ifdef __cplusplus
#define UPB_INLINE inline
#elif defined(__GNUC__) || defined(__clang__)
#define UPB_INLINE static __inline__
#else
#define UPB_INLINE static
#endif

// UPB_INLINE_IF_NOT_GCC: because gcc can be very noisy at times.
#if defined(__GNUC__) && !defined(__clang__)
#define UPB_INLINE_IF_NOT_GCC static
#else
#define UPB_INLINE_IF_NOT_GCC UPB_INLINE
#endif

#ifdef UPB_BUILD_API
#define UPB_API UPB_EXPORT
#define UPB_API_INLINE UPB_EXPORT
#else
#define UPB_API
#define UPB_API_INLINE UPB_INLINE
#endif

#ifdef EXPORT_UPBC
#define UPBC_API UPB_EXPORT
#else
#define UPBC_API
#endif

#if UPB_HAS_FEATURE(address_sanitizer) || defined(__SANITIZE_ADDRESS__)
#define UPB_ASAN 1
#else
#define UPB_ASAN 0
#endif

#if UPB_HAS_FEATURE(hwaddress_sanitizer)
#define UPB_HWASAN 1
#define UPB_HWASAN_POISON_TAG 17
#define UPB_MALLOC_ALIGN 16
#else
#define UPB_HWASAN 0
#define UPB_MALLOC_ALIGN 8
#endif

#if UPB_HAS_FEATURE(thread_sanitizer) || defined(__SANITIZE_THREAD__)
#define UPB_TSAN 1
#else
#define UPB_TSAN 0
#endif

#if UPB_HAS_FEATURE(memory_sanitizer)
#define UPB_MSAN 1
#else
#define UPB_MSAN 0
#endif

// An unfortunate concession to C++17 and MSVC, which don't support zero-sized
// structs.
#if UPB_ASAN || UPB_HWASAN || UPB_TSAN
#define UPB_XSAN_MEMBER upb_Xsan xsan;
#define UPB_XSAN(st) (&(st)->xsan)
#define UPB_XSAN_STRUCT_SIZE 1
#else
#define UPB_XSAN_MEMBER
#define UPB_XSAN(st) (NULL)
#define UPB_XSAN_STRUCT_SIZE 0
#endif

#define UPB_ALIGN_UP(size, align) (((size) + (align) - 1) / (align) * (align))
#define UPB_ALIGN_DOWN(size, align) ((size) / (align) * (align))
#define UPB_ALIGN_MALLOC(size) UPB_ALIGN_UP(size, UPB_MALLOC_ALIGN)

#if __STDC_VERSION__ >= 202311L || UPB_HAS_EXTENSION(cxx_alignof) || \
    defined(__cplusplus)
#define UPB_ALIGN_OF(type) alignof(type)
#elif __STDC_VERSION__ >= 201112L || UPB_HAS_EXTENSION(c_alignof)
#define UPB_ALIGN_OF(type) _Alignof(type)
#elif UPB_GNUC_MIN(2, 95)
#define UPB_ALIGN_OF(type) __alignof__(type)
#elif defined(_MSC_VER)
#define UPB_ALIGN_OF(type) __alignof(type)
#else
#define UPB_ALIGN_OF(type) \
  offsetof(                \
      struct {             \
        char c;            \
        type member;       \
      },                   \
      member)
#endif

#ifdef _MSC_VER
// Some versions of our Windows compiler don't support the C11 syntax.
#define UPB_ALIGN_AS(x) __declspec(align(x))
#elif defined(__GNUC__)
#define UPB_ALIGN_AS(x) __attribute__((aligned(x)))
#else
#define UPB_ALIGN_AS(x) _Alignas(x)
#endif

#if __STDC_VERSION__ >= 202311L || UPB_HAS_EXTENSION(cxx_static_assert) || \
    defined(__cplusplus)
#define UPB_STATIC_ASSERT(val, msg) static_assert((val), msg)
#elif __STDC_VERSION__ >= 201112L || UPB_HAS_EXTENSION(c_static_assert) || \
    UPB_GNUC_MIN(4, 6)
#define UPB_STATIC_ASSERT(val, msg) _Static_assert((val), msg)
#else
// Unfortunately this hack doesn't work inside struct declarations, but it works
// everywhere else
#define UPB_STATIC_ASSERT_CONCAT_IMPL(s1, s2) s1##s2
#define UPB_STATIC_ASSERT_CONCAT(s1, s2) UPB_STATIC_ASSERT_CONCAT_IMPL(s1, s2)
#ifdef __COUNTER__
#define UPB_STATIC_ASSERT(condition, message)                      \
  typedef char UPB_STATIC_ASSERT_CONCAT(static_assertion_failure_, \
                                        __COUNTER__)[(condition) ? 1 : -1]
#else
#define UPB_STATIC_ASSERT(condition, message)                      \
  typedef char UPB_STATIC_ASSERT_CONCAT(static_assertion_failure_, \
                                        __LINE__)[(condition) ? 1 : -1]
#endif
#endif

// Hints to the compiler about likely/unlikely branches.
#if defined(__GNUC__) || defined(__clang__)
#define UPB_LIKELY(x) __builtin_expect((bool)(x), 1)
#define UPB_UNLIKELY(x) __builtin_expect((bool)(x), 0)
#else
#define UPB_LIKELY(x) (x)
#define UPB_UNLIKELY(x) (x)
#endif

#if UPB_HAS_BUILTIN(__builtin_expect_with_probability)
#define UPB_UNPREDICTABLE(x) \
  __builtin_expect_with_probability((bool)(x), 1, 0.5)
#else
#define UPB_UNPREDICTABLE(x) (x)
#endif

// Macros for function attributes on compilers that support them.
#if defined(__GNUC__) || defined(__clang__)
#define UPB_FORCEINLINE __inline__ __attribute__((always_inline)) static
#define UPB_NOINLINE __attribute__((noinline))
#define UPB_NORETURN __attribute__((__noreturn__))
#define UPB_PRINTF(str, first_vararg) \
  __attribute__((format(printf, str, first_vararg)))
#elif defined(_MSC_VER)
#define UPB_NOINLINE
#define UPB_FORCEINLINE static
#define UPB_NORETURN __declspec(noreturn)
#define UPB_PRINTF(str, first_vararg)
#else /* !defined(__GNUC__) */
#define UPB_FORCEINLINE static
#define UPB_NOINLINE
#define UPB_NORETURN
#define UPB_PRINTF(str, first_vararg)
#endif

#if defined(__clang__)
#define UPB_NODEREF __attribute__((noderef))
#else
#define UPB_NODEREF
#endif

#define UPB_MAX(x, y) ((x) > (y) ? (x) : (y))
#define UPB_MIN(x, y) ((x) < (y) ? (x) : (y))

#define UPB_UNUSED(var) (void)(var)

// UPB_ASSUME(): in release mode, we tell the compiler to assume this is true.
#ifdef NDEBUG
#ifdef __GNUC__
#define UPB_ASSUME(expr) \
  if (!(expr)) __builtin_unreachable()
#elif defined _MSC_VER
#define UPB_ASSUME(expr) \
  if (!(expr)) __assume(0)
#else
#define UPB_ASSUME(expr) \
  do {                   \
  } while (false && (expr))
#endif
#else
#define UPB_ASSUME(expr) assert(expr)
#endif

#if UPB_HAS_BUILTIN(__builtin_constant_p) && UPB_HAS_ATTRIBUTE(const)
#define UPB_MAYBE_ASSUME(pred, x) \
  if (__builtin_constant_p(pred) && pred) UPB_ASSUME(x)
#define UPB_ATTR_CONST __attribute__((const))
#else
#define UPB_MAYBE_ASSUME(pred, x)
#define UPB_ATTR_CONST
#endif

/* UPB_ASSERT(): in release mode, we use the expression without letting it be
 * evaluated.  This prevents "unused variable" warnings. */
#ifdef NDEBUG
#define UPB_ASSERT(expr) \
  do {                   \
  } while (false && (expr))
#else
#define UPB_ASSERT(expr) assert(expr)
#endif

#if !defined(NDEBUG) && !defined(UPB_TSAN)
#define UPB_ENABLE_REF_CYCLE_CHECKS 1
#else
#define UPB_ENABLE_REF_CYCLE_CHECKS 0
#endif

#if defined(__GNUC__) || defined(__clang__)
#define UPB_UNREACHABLE()    \
  do {                       \
    assert(0);               \
    __builtin_unreachable(); \
  } while (0)
#elif defined(_MSC_VER)
#define UPB_UNREACHABLE() \
  do {                    \
    assert(0);            \
    __assume(0);          \
  } while (0)
#else
#define UPB_UNREACHABLE() \
  do {                    \
    assert(0);            \
  } while (0)
#endif

#ifdef __ANDROID__
#define UPB_DEFAULT_MAX_BLOCK_SIZE 8192
#else
#define UPB_DEFAULT_MAX_BLOCK_SIZE 32768
#endif

/* UPB_SETJMP() / UPB_LONGJMP() */
// Android uses a custom libc that does not implement all of posix, but it has
// had sigsetjmp/siglongjmp forever on arm and since API 12 on x86. Apple has
// sigsetjmp, but does not define the posix feature test macro.
#if defined(__APPLE__) || defined(_POSIX_C_SOURCE) || defined(__ANDROID__)
// avoid setting/restoring signal mask, which involves costly syscalls
#define UPB_SETJMP(buf) sigsetjmp(buf, 0)
#define UPB_LONGJMP(buf, val) siglongjmp(buf, val)
#elif defined(WASM_WAMR)
#define UPB_SETJMP(buf) 0
#define UPB_LONGJMP(buf, val) abort()
#else
#define UPB_SETJMP(buf) setjmp(buf)
#define UPB_LONGJMP(buf, val) longjmp(buf, val)
#endif

#if ((__STDC_VERSION__ >= 201112L) && !defined(__STDC_NO_ATOMICS__)) || \
    UPB_HAS_EXTENSION(c_atomic) ||                                      \
    defined(__GNUC__)  // GCC supported atomics as an extension before it
                       // supported __has_extension
#define UPB_USE_C11_ATOMICS
#elif defined(_MSC_VER)
#define UPB_USE_MSC_ATOMICS
#endif

#if defined(UPB_USE_C11_ATOMICS)
#define UPB_ATOMIC(T) _Atomic(T)
#elif defined(UPB_USE_MSC_ATOMICS)
#define UPB_ATOMIC(T) volatile T
#else
#define UPB_ATOMIC(T) T
#endif

/* UPB_PTRADD(ptr, ofs): add pointer while avoiding "NULL + 0" UB */
#define UPB_PTRADD(ptr, ofs) ((ofs) ? (ptr) + (ofs) : (ptr))

#define UPB_PRIVATE(x) x##_dont_copy_me__upb_internal_use_only

#ifdef UPB_ALLOW_PRIVATE_ACCESS__FOR_BITS_ONLY
#define UPB_ONLYBITS(x) x
#else
#define UPB_ONLYBITS(x) UPB_PRIVATE(x)
#endif

/* Configure whether fasttable is switched on or not. *************************/

#if UPB_HAS_ATTRIBUTE(musttail)
#define UPB_MUSTTAIL __attribute__((musttail))
#else
#define UPB_MUSTTAIL
#endif

#if UPB_HAS_ATTRIBUTE(preserve_most) && !defined(__i386__)
#define UPB_PRESERVE_MOST __attribute__((preserve_most))
#else
#define UPB_PRESERVE_MOST
#endif

#if UPB_HAS_ATTRIBUTE(preserve_none)
#define UPB_PRESERVE_NONE __attribute__((preserve_none))
#else
#define UPB_PRESERVE_NONE
#endif

#if defined(__aarch64__) && (defined(__GNUC__) || defined(__clang__))
#define UPB_ARM64_ASM 1
#else
#define UPB_ARM64_ASM 0
#endif

/* When compiling with branch protection, we need to ensure that all branch
 * targets in assembly use the appropriate landing pad instruction. These
 * instructions are backwards compatible with processors that don't have
 * FEAT_BTI and are treated as nops.
 */
#if UPB_ARM64_ASM && defined(__ARM_FEATURE_BTI_DEFAULT)
#if __ARM_FEATURE_BTI_DEFAULT == 1
#define UPB_ARM64_BTI_DEFAULT 1
#else
#define UPB_ARM64_BTI_DEFAULT 0
#endif
#else
#define UPB_ARM64_BTI_DEFAULT 0
#endif

/* This check is not fully robust: it does not require that we have "musttail"
 * support available. We need tail calls to avoid consuming arbitrary amounts
 * of stack space.
 *
 * GCC/Clang can mostly be trusted to generate tail calls as long as
 * optimization is enabled, but, debug builds will not generate tail calls
 * unless "musttail" is available.
 *
 * We should probably either:
 *   1. require that the compiler supports musttail.
 *   2. add some fallback code for when musttail isn't available (ie. return
 *      instead of tail calling). This is safe and portable, but this comes at
 *      a CPU cost.
 */
#if (defined(__x86_64__) || defined(__aarch64__)) && defined(__GNUC__)
#define UPB_FASTTABLE_SUPPORTED 1
#else
#define UPB_FASTTABLE_SUPPORTED 0
#endif

/* define UPB_ENABLE_FASTTABLE to force fast table support.
 * This is useful when we want to ensure we are really getting fasttable,
 * for example for testing or benchmarking. */
#if defined(UPB_ENABLE_FASTTABLE)
#if !UPB_FASTTABLE_SUPPORTED
#error fasttable is x86-64/ARM64 only and requires GCC or Clang.
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
 * degrade to non-fasttable if the runtime or platform do not support it. */
#if !UPB_FASTTABLE
#define UPB_FASTTABLE_INIT(...)
#define UPB_FASTTABLE_MASK(mask) -1
#else
#define UPB_FASTTABLE_INIT(...) __VA_ARGS__
#define UPB_FASTTABLE_MASK(mask) mask
#endif

#undef UPB_FASTTABLE_SUPPORTED

#if defined(__cplusplus)
#if defined(__clang__) || UPB_GNUC_MIN(6, 0)
// https://gcc.gnu.org/gcc-6/changes.html
#define UPB_DEPRECATED [[deprecated]]
#else
#define UPB_DEPRECATED
#endif
#else
#define UPB_DEPRECATED
#endif

#if defined(UPB_IS_GOOGLE3) && \
    (!defined(UPB_BOOTSTRAP_STAGE) || UPB_BOOTSTRAP_STAGE != 0)
#define UPB_DESC_MINITABLE(sym) &proto2__##sym##_msg_init
#elif defined(UPB_IS_GOOGLE3) && defined(UPB_BOOTSTRAP_STAGE) && \
    UPB_BOOTSTRAP_STAGE == 0
#define UPB_DESC_MINITABLE(sym) proto2__##sym##_msg_init()
#elif defined(UPB_BOOTSTRAP_STAGE) && UPB_BOOTSTRAP_STAGE == 0
#define UPB_DESC_MINITABLE(sym) google__protobuf__##sym##_msg_init()
#else
#define UPB_DESC_MINITABLE(sym) &google__protobuf__##sym##_msg_init
#endif

#undef UPB_IS_GOOGLE3

#ifdef __clang__
#define UPB_NO_SANITIZE_ADDRESS __attribute__((no_sanitize("address")))
#else
#define UPB_NO_SANITIZE_ADDRESS
#endif

#if defined(__GNUC__) && (defined(__clang__) || UPB_GNUC_MIN(11, 0))
#define UPB_RETAIN __attribute__((retain))
#else
#define UPB_RETAIN
#endif

// Linker arrays combine elements from multiple translation units into a single
// array that can be iterated over at runtime.
//
// It is an alternative to pre-main "registration" functions.
//
// Usage:
//
//   // In N translation units.
//   UPB_LINKARR_APPEND(foo_array) static int elems[3] = {1, 2, 3};
//
//   // At runtime:
//   UPB_LINKARR_DECLARE(foo_array, int);
//
//   void f() {
//     const int* start = UPB_LINKARR_START(foo_array);
//     const int* stop = UPB_LINKARR_STOP(foo_array);
//     for (const int* p = start; p < stop; p++) {
//       // Windows can introduce zero padding, so we have to skip zeroes.
//       if (*p != 0) {
//         vec.push_back(*p);
//       }
//     }
//   }

#define UPB_LINKARR_ATTR

#define UPB_LINKARR_SENTINEL UPB_RETAIN __attribute__((weak, used))

#if defined(__ELF__) || defined(__wasm__)

#define UPB_LINKARR_APPEND(name) \
  __attribute__((                \
      section("linkarr_" #name))) UPB_LINKARR_ATTR UPB_NO_SANITIZE_ADDRESS
#define UPB_LINKARR_DECLARE(name, type) \
  extern type __start_linkarr_##name;   \
  extern type __stop_linkarr_##name;    \
  UPB_LINKARR_APPEND(name)              \
  UPB_LINKARR_SENTINEL type UPB_linkarr_internal_empty_##name[1]
#define UPB_LINKARR_START(name) (&__start_linkarr_##name)
#define UPB_LINKARR_STOP(name) (&__stop_linkarr_##name)

#elif defined(__MACH__)

/* As described in: https://stackoverflow.com/a/22366882 */
#define UPB_LINKARR_APPEND(name) \
  __attribute__((                \
      section("__DATA,__la_" #name))) UPB_LINKARR_ATTR UPB_NO_SANITIZE_ADDRESS
#define UPB_LINKARR_DECLARE(name, type)     \
  extern type __start_linkarr_##name __asm( \
      "section$start$__DATA$__la_" #name);  \
  extern type __stop_linkarr_##name __asm(  \
      "section$end$__DATA$"                 \
      "__la_" #name);                       \
  UPB_LINKARR_APPEND(name)                  \
  UPB_LINKARR_SENTINEL type UPB_linkarr_internal_empty_##name[1]
#define UPB_LINKARR_START(name) (&__start_linkarr_##name)
#define UPB_LINKARR_STOP(name) (&__stop_linkarr_##name)

#elif defined(_MSC_VER)

/* See:
 *   https://devblogs.microsoft.com/oldnewthing/20181107-00/?p=100155
 *   https://devblogs.microsoft.com/oldnewthing/20181108-00/?p=100165
 *   https://devblogs.microsoft.com/oldnewthing/20181109-00/?p=100175 */
#define UPB_STRINGIFY_INTERNAL(x) #x
#define UPB_STRINGIFY(x) UPB_STRINGIFY_INTERNAL(x)
#define UPB_CONCAT(a, b, c) a##b##c
#define UPB_LINKARR_NAME(name, index) \
  UPB_STRINGIFY(UPB_CONCAT(la_, name, index))
#define UPB_LINKARR_APPEND(name)                      \
  __pragma(section(UPB_LINKARR_NAME(name, $j), read)) \
      __declspec(allocate(UPB_LINKARR_NAME(name, $j)))
// clang-format off
#define UPB_LINKARR_DECLARE(name, type)                          \
  __pragma(message(UPB_LINKARR_NAME(name, $j)))                  \
  __pragma(section(UPB_LINKARR_NAME(name, $a), read))            \
  __pragma(section(UPB_LINKARR_NAME(name, $z), read))            \
  __declspec(allocate(UPB_LINKARR_NAME(name, $a)), selectany)    \
            type __start_linkarr_##name;                         \
  __declspec(allocate(UPB_LINKARR_NAME(name, $z)), selectany)    \
            type __stop_linkarr_##name;                          \
  UPB_LINKARR_APPEND(name)                                       \
  __declspec(selectany) type UPB_linkarr_internal_empty_##name[1] = {0}
// clang-format on
#define UPB_LINKARR_START(name) (&__start_linkarr_##name)
#define UPB_LINKARR_STOP(name) (&__stop_linkarr_##name)

#else

// Linker arrays are not supported on this platform.  Make macros no-ops.
#define UPB_LINKARR_APPEND(name)
#define UPB_LINKARR_DECLARE(name, type)
#define UPB_LINKARR_START(name) (NULL)
#define UPB_LINKARR_STOP(name) (NULL)

#endif

// Workaround for https://github.com/llvm/llvm-project/issues/167577 until it's
// fixed.  Some function must exist for the constructor to work properly.
// TODO Remove this or gate it on a future version of clang.
#if defined(__clang__) && defined(__arm__)
#define _UPB_CONSTRUCTOR_PLACEHOLDER(unique_name)                            \
  __attribute__((used, visibility("hidden"))) void UPB_PRIVATE(unique_name)( \
      void) {}
#else
#define _UPB_CONSTRUCTOR_PLACEHOLDER(unique_name)
#endif

#if defined(__ELF__) || defined(__wasm__) || defined(__MACH__)
#define UPB_CONSTRUCTOR(name, unique_name)                                   \
  _UPB_CONSTRUCTOR_PLACEHOLDER(unique_name)                                  \
  __attribute__((weak, visibility("hidden"), constructor)) void UPB_PRIVATE( \
      name)(void)
#elif defined(_MSC_VER)
/*
 * See: https://stackoverflow.com/questions/1113409
 *
 * The /include pragma suggested in the link above doesn't work in our case
 * because it requires globally unique names. We need a different solution
 * to prevent optimizers from removing the constructor. Our solution is to
 * create a dummy exported weak symbol that prevent this stripping.
 */
#pragma section(".CRT$XCU", long, read)
#define UPB_CONSTRUCTOR(name, unique_name)                                   \
  static void __cdecl UPB_PRIVATE(name)(void);                               \
  __declspec(allocate(".CRT$XCU"), selectany) void(                          \
      __cdecl * UPB_PRIVATE(name##_))(void) = UPB_PRIVATE(name);             \
  __declspec(selectany, dllexport) void* UPB_PRIVATE(name##_force_linkage) = \
      &UPB_PRIVATE(name##_);                                                 \
  static void __cdecl UPB_PRIVATE(name)(void)

#else
// No constructor support, nothing we can do except not break builds.
#define UPB_CONSTRUCTOR(name, unique_name) static void UPB_PRIVATE(name)(void)
#endif

//
// Weak alias platform support. Theoretically this should be possible to do with
// only C using attributes like __attribute__((weak, alias("foo"))), but
// Clang doesn't support this properly on macOS.  So we have to use assembly.
#if defined(__APPLE__)

// TODO: once https://github.com/llvm/llvm-project/issues/167262 is fixed
// in the LLVM linker, we should have all weak variables point to a single
// "default" empty MiniTable instead of having each leaf define its own, like
// we do with ELF below. This will reduce binary size if many messages are tree
// shaken.
#define UPB_WEAK_SINGLETON_PLACEHOLDER_MINITABLE()
#define UPB_WEAK_PLACEHOLDER_MINITABLE(name)           \
  __attribute__((weak)) const upb_MiniTable name = {   \
      .UPB_PRIVATE(fields) = NULL,                     \
      .UPB_PRIVATE(size) = sizeof(struct upb_Message), \
      .UPB_PRIVATE(field_count) = 0,                   \
      .UPB_PRIVATE(ext) = kUpb_ExtMode_NonExtendable,  \
      .UPB_PRIVATE(dense_below) = 0,                   \
      .UPB_PRIVATE(table_mask) = -1,                   \
      .UPB_PRIVATE(required_count) = 0,                \
  };
#define UPB_WEAK_ALIAS(type, from, to) \
  extern type to;                      \
  __asm__(".globl _" #to);             \
  __asm__(".private_extern _" #to);    \
  __asm__(".set _" #to ", _" #from);   \
  __asm__(".weak_definition _" #to);
#define UPB_STRONG_ALIAS(type, from, to) \
  __asm__(".globl _" #to);               \
  __asm__(".private_extern _" #to);      \
  __asm__(".set _" #to ", _" #from);

#elif defined(__ELF__)

// On ELF, weak aliases work properly, so we can have all weak MiniTables point
// to the same empty singleton MiniTable. This reduces code size if many
// MiniTables are tree shaken.
#define UPB_WEAK_SINGLETON_PLACEHOLDER_MINITABLE()               \
  __attribute__((weak))                                          \
  const upb_MiniTable kUpb_WeakSingletonPlaceholderMiniTable = { \
      .UPB_PRIVATE(fields) = NULL,                               \
      .UPB_PRIVATE(size) = sizeof(struct upb_Message),           \
      .UPB_PRIVATE(field_count) = 0,                             \
      .UPB_PRIVATE(ext) = kUpb_ExtMode_NonExtendable,            \
      .UPB_PRIVATE(dense_below) = 0,                             \
      .UPB_PRIVATE(table_mask) = -1,                             \
      .UPB_PRIVATE(required_count) = 0,                          \
  };
#define UPB_WEAK_PLACEHOLDER_MINITABLE(name)
#define UPB_WEAK_ALIAS(type, from, to) \
  extern type to                       \
      __attribute__((weak, alias("kUpb_WeakSingletonPlaceholderMiniTable")));
#define UPB_STRONG_ALIAS(type, from, to) \
  extern type to __attribute__((alias(#from)));

#else
#define UPB_WEAK_SINGLETON_PLACEHOLDER_MINITABLE()
#define UPB_WEAK_PLACEHOLDER_MINITABLE(name)
#define UPB_WEAK_ALIAS(type, from, to) weak_alias_not_supported_on_this_platform
#define UPB_STRONG_ALIAS(type, from, to) \
  strong_alias_not_supported_on_this_platform
#endif

// Future versions of upb will include breaking changes to some APIs.
// This macro can be set to enable these API changes ahead of time, so that
// user code can be updated before upgrading versions of protobuf.
#ifdef UPB_FUTURE_BREAKING_CHANGES

#endif
