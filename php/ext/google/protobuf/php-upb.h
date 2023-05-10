/* Amalgamated source file */

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
      (defined(__cplusplus) && __cplusplus >= 201402L) ||           \
      (defined(_MSC_VER) && _MSC_VER >= 1900))
#error upb requires C99 or C++14 or MSVC >= 2015.
#endif

// Portable check for GCC minimum version:
// https://gcc.gnu.org/onlinedocs/cpp/Common-Predefined-Macros.html
#if defined(__GNUC__) && defined(__GNUC_MINOR__) && defined(__GNUC_PATCHLEVEL__)
#define UPB_GNUC_MIN(x, y) \
  (__GNUC__ > (x) || __GNUC__ == (x) && __GNUC_MINOR__ >= (y))
#else
#define UPB_GNUC_MIN(x, y) 0
#endif

#include <assert.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#if UINTPTR_MAX == 0xffffffff
#define UPB_SIZE(size32, size64) size32
#else
#define UPB_SIZE(size32, size64) size64
#endif

/* If we always read/write as a consistent type to each address, this shouldn't
 * violate aliasing.
 */
#define UPB_PTR_AT(msg, ofs, type) ((type*)((char*)(msg) + (ofs)))

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
#elif defined (__GNUC__) || defined(__clang__)
#define UPB_INLINE static __inline__
#else
#define UPB_INLINE static
#endif

#ifdef UPB_BUILD_API
#define UPB_API UPB_EXPORT
#define UPB_API_INLINE UPB_EXPORT
#else
#define UPB_API
#define UPB_API_INLINE UPB_INLINE
#endif

#define UPB_MALLOC_ALIGN 8
#define UPB_ALIGN_UP(size, align) (((size) + (align) - 1) / (align) * (align))
#define UPB_ALIGN_DOWN(size, align) ((size) / (align) * (align))
#define UPB_ALIGN_MALLOC(size) UPB_ALIGN_UP(size, UPB_MALLOC_ALIGN)
#ifdef __clang__
#define UPB_ALIGN_OF(type) _Alignof(type)
#else
#define UPB_ALIGN_OF(type) offsetof (struct { char c; type member; }, member)
#endif

// Hints to the compiler about likely/unlikely branches.
#if defined (__GNUC__) || defined(__clang__)
#define UPB_LIKELY(x) __builtin_expect((bool)(x), 1)
#define UPB_UNLIKELY(x) __builtin_expect((bool)(x), 0)
#else
#define UPB_LIKELY(x) (x)
#define UPB_UNLIKELY(x) (x)
#endif

// Macros for function attributes on compilers that support them.
#ifdef __GNUC__
#define UPB_FORCEINLINE __inline__ __attribute__((always_inline))
#define UPB_NOINLINE __attribute__((noinline))
#define UPB_NORETURN __attribute__((__noreturn__))
#define UPB_PRINTF(str, first_vararg) __attribute__((format (printf, str, first_vararg)))
#elif defined(_MSC_VER)
#define UPB_NOINLINE
#define UPB_FORCEINLINE
#define UPB_NORETURN __declspec(noreturn)
#define UPB_PRINTF(str, first_vararg)
#else  /* !defined(__GNUC__) */
#define UPB_FORCEINLINE
#define UPB_NOINLINE
#define UPB_NORETURN
#define UPB_PRINTF(str, first_vararg)
#endif

#define UPB_MAX(x, y) ((x) > (y) ? (x) : (y))
#define UPB_MIN(x, y) ((x) < (y) ? (x) : (y))

#define UPB_UNUSED(var) (void)var

// UPB_ASSUME(): in release mode, we tell the compiler to assume this is true.
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
#elif defined(_MSC_VER)
#define UPB_UNREACHABLE() \
  do {                    \
    assert(0);            \
    __assume(0);          \
  } while (0)
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

#ifdef __GNUC__
#define UPB_USE_C11_ATOMICS
#define UPB_ATOMIC(T) _Atomic(T)
#else
#define UPB_ATOMIC(T) T
#endif

/* UPB_PTRADD(ptr, ofs): add pointer while avoiding "NULL + 0" UB */
#define UPB_PTRADD(ptr, ofs) ((ofs) ? (ptr) + (ofs) : (ptr))

#define UPB_PRIVATE(x) x##_dont_copy_me__upb_internal_use_only

/* Configure whether fasttable is switched on or not. *************************/

#ifdef __has_attribute
#define UPB_HAS_ATTRIBUTE(x) __has_attribute(x)
#else
#define UPB_HAS_ATTRIBUTE(x) 0
#endif

#if UPB_HAS_ATTRIBUTE(musttail)
#define UPB_MUSTTAIL __attribute__((musttail))
#else
#define UPB_MUSTTAIL
#endif

#undef UPB_HAS_ATTRIBUTE

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

/* ASAN poisoning (for arena).
 * If using UPB from an interpreted language like Ruby, a build of the
 * interpreter compiled with ASAN enabled must be used in order to get sane and
 * expected behavior.
 */

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

/* Disable proto2 arena behavior (TEMPORARY) **********************************/

#ifdef UPB_DISABLE_PROTO2_ENUM_CHECKING
#define UPB_TREAT_PROTO2_ENUMS_LIKE_PROTO3 1
#else
#define UPB_TREAT_PROTO2_ENUMS_LIKE_PROTO3 0
#endif

#if defined(__cplusplus)
#if defined(__clang__) || UPB_GNUC_MIN(6, 0)
// https://gcc.gnu.org/gcc-6/changes.html
#if __cplusplus >= 201402L
#define UPB_DEPRECATED [[deprecated]]
#else
#define UPB_DEPRECATED __attribute__((deprecated))
#endif
#else
#define UPB_DEPRECATED
#endif
#else
#define UPB_DEPRECATED
#endif

// begin:google_only
// #define UPB_IS_GOOGLE3
// end:google_only

#if defined(UPB_IS_GOOGLE3) && !defined(UPB_BOOTSTRAP_STAGE0)
#define UPB_DESC(sym) proto2_##sym
#else
#define UPB_DESC(sym) google_protobuf_##sym
#endif

#ifndef UPB_BASE_STATUS_H_
#define UPB_BASE_STATUS_H_

#include <stdarg.h>

// Must be last.

#define _kUpb_Status_MaxMessage 127

typedef struct {
  bool ok;
  char msg[_kUpb_Status_MaxMessage];  // Error message; NULL-terminated.
} upb_Status;

#ifdef __cplusplus
extern "C" {
#endif

UPB_API const char* upb_Status_ErrorMessage(const upb_Status* status);
UPB_API bool upb_Status_IsOk(const upb_Status* status);

// These are no-op if |status| is NULL.
UPB_API void upb_Status_Clear(upb_Status* status);
void upb_Status_SetErrorMessage(upb_Status* status, const char* msg);
void upb_Status_SetErrorFormat(upb_Status* status, const char* fmt, ...)
    UPB_PRINTF(2, 3);
void upb_Status_VSetErrorFormat(upb_Status* status, const char* fmt,
                                va_list args) UPB_PRINTF(2, 0);
void upb_Status_VAppendErrorFormat(upb_Status* status, const char* fmt,
                                   va_list args) UPB_PRINTF(2, 0);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_BASE_STATUS_H_ */

#ifndef UPB_INTERNAL_ARRAY_INTERNAL_H_
#define UPB_INTERNAL_ARRAY_INTERNAL_H_

#include <string.h>


#ifndef UPB_COLLECTIONS_ARRAY_H_
#define UPB_COLLECTIONS_ARRAY_H_


#ifndef UPB_BASE_DESCRIPTOR_CONSTANTS_H_
#define UPB_BASE_DESCRIPTOR_CONSTANTS_H_

// Must be last.

// The types a field can have. Note that this list is not identical to the
// types defined in descriptor.proto, which gives INT32 and SINT32 separate
// types (we distinguish the two with the "integer encoding" enum below).
// This enum is an internal convenience only and has no meaning outside of upb.
typedef enum {
  kUpb_CType_Bool = 1,
  kUpb_CType_Float = 2,
  kUpb_CType_Int32 = 3,
  kUpb_CType_UInt32 = 4,
  kUpb_CType_Enum = 5,  // Enum values are int32. TODO(b/279178239): rename
  kUpb_CType_Message = 6,
  kUpb_CType_Double = 7,
  kUpb_CType_Int64 = 8,
  kUpb_CType_UInt64 = 9,
  kUpb_CType_String = 10,
  kUpb_CType_Bytes = 11
} upb_CType;

// The repeated-ness of each field; this matches descriptor.proto.
typedef enum {
  kUpb_Label_Optional = 1,
  kUpb_Label_Required = 2,
  kUpb_Label_Repeated = 3
} upb_Label;

// Descriptor types, as defined in descriptor.proto.
typedef enum {
  kUpb_FieldType_Double = 1,
  kUpb_FieldType_Float = 2,
  kUpb_FieldType_Int64 = 3,
  kUpb_FieldType_UInt64 = 4,
  kUpb_FieldType_Int32 = 5,
  kUpb_FieldType_Fixed64 = 6,
  kUpb_FieldType_Fixed32 = 7,
  kUpb_FieldType_Bool = 8,
  kUpb_FieldType_String = 9,
  kUpb_FieldType_Group = 10,
  kUpb_FieldType_Message = 11,
  kUpb_FieldType_Bytes = 12,
  kUpb_FieldType_UInt32 = 13,
  kUpb_FieldType_Enum = 14,
  kUpb_FieldType_SFixed32 = 15,
  kUpb_FieldType_SFixed64 = 16,
  kUpb_FieldType_SInt32 = 17,
  kUpb_FieldType_SInt64 = 18,
} upb_FieldType;

#define kUpb_FieldType_SizeOf 19

#ifdef __cplusplus
extern "C" {
#endif

UPB_INLINE bool upb_FieldType_IsPackable(upb_FieldType type) {
  // clang-format off
  const unsigned kUnpackableTypes =
      (1 << kUpb_FieldType_String) |
      (1 << kUpb_FieldType_Bytes) |
      (1 << kUpb_FieldType_Message) |
      (1 << kUpb_FieldType_Group);
  // clang-format on
  return (1 << type) & ~kUnpackableTypes;
}

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_BASE_DESCRIPTOR_CONSTANTS_H_ */

// Users should include array.h or map.h instead.
// IWYU pragma: private, include "upb/collections/array.h"

#ifndef UPB_MESSAGE_VALUE_H_
#define UPB_MESSAGE_VALUE_H_


#ifndef UPB_BASE_STRING_VIEW_H_
#define UPB_BASE_STRING_VIEW_H_

#include <string.h>

// Must be last.

#define UPB_STRINGVIEW_INIT(ptr, len) \
  { ptr, len }

#define UPB_STRINGVIEW_FORMAT "%.*s"
#define UPB_STRINGVIEW_ARGS(view) (int)(view).size, (view).data

// LINT.IfChange(struct_definition)
typedef struct {
  const char* data;
  size_t size;
} upb_StringView;
// LINT.ThenChange(GoogleInternalName0)

#ifdef __cplusplus
extern "C" {
#endif

UPB_API_INLINE upb_StringView upb_StringView_FromDataAndSize(const char* data,
                                                             size_t size) {
  upb_StringView ret;
  ret.data = data;
  ret.size = size;
  return ret;
}

UPB_INLINE upb_StringView upb_StringView_FromString(const char* data) {
  return upb_StringView_FromDataAndSize(data, strlen(data));
}

UPB_INLINE bool upb_StringView_IsEqual(upb_StringView a, upb_StringView b) {
  return a.size == b.size && memcmp(a.data, b.data, a.size) == 0;
}

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_BASE_STRING_VIEW_H_ */

#ifndef UPB_MINI_TABLE_TYPES_H_
#define UPB_MINI_TABLE_TYPES_H_

typedef void upb_Message;

typedef struct upb_MiniTable upb_MiniTable;
typedef struct upb_MiniTableEnum upb_MiniTableEnum;
typedef struct upb_MiniTableExtension upb_MiniTableExtension;
typedef struct upb_MiniTableField upb_MiniTableField;
typedef struct upb_MiniTableFile upb_MiniTableFile;
typedef union upb_MiniTableSub upb_MiniTableSub;

#endif /* UPB_MINI_TABLE_TYPES_H_ */

// Must be last.

typedef struct upb_Array upb_Array;
typedef struct upb_Map upb_Map;

typedef union {
  bool bool_val;
  float float_val;
  double double_val;
  int32_t int32_val;
  int64_t int64_val;
  uint32_t uint32_val;
  uint64_t uint64_val;
  const upb_Array* array_val;
  const upb_Map* map_val;
  const upb_Message* msg_val;
  upb_StringView str_val;
} upb_MessageValue;

typedef union {
  upb_Array* array;
  upb_Map* map;
  upb_Message* msg;
} upb_MutableMessageValue;


#endif /* UPB_MESSAGE_VALUE_H_ */

/* upb_Arena is a specific allocator implementation that uses arena allocation.
 * The user provides an allocator that will be used to allocate the underlying
 * arena blocks.  Arenas by nature do not require the individual allocations
 * to be freed.  However the Arena does allow users to register cleanup
 * functions that will run when the arena is destroyed.
 *
 * A upb_Arena is *not* thread-safe.
 *
 * You could write a thread-safe arena allocator that satisfies the
 * upb_alloc interface, but it would not be as efficient for the
 * single-threaded case. */

#ifndef UPB_MEM_ARENA_H_
#define UPB_MEM_ARENA_H_

#include <string.h>


#ifndef UPB_MEM_ALLOC_H_
#define UPB_MEM_ALLOC_H_

// Must be last.

#ifdef __cplusplus
extern "C" {
#endif

typedef struct upb_alloc upb_alloc;

/* A combined `malloc()`/`free()` function.
 * If `size` is 0 then the function acts like `free()`, otherwise it acts like
 * `realloc()`.  Only `oldsize` bytes from a previous allocation are
 * preserved. */
typedef void* upb_alloc_func(upb_alloc* alloc, void* ptr, size_t oldsize,
                             size_t size);

/* A upb_alloc is a possibly-stateful allocator object.
 *
 * It could either be an arena allocator (which doesn't require individual
 * `free()` calls) or a regular `malloc()` (which does).  The client must
 * therefore free memory unless it knows that the allocator is an arena
 * allocator. */
struct upb_alloc {
  upb_alloc_func* func;
};

UPB_INLINE void* upb_malloc(upb_alloc* alloc, size_t size) {
  UPB_ASSERT(alloc);
  return alloc->func(alloc, NULL, 0, size);
}

UPB_INLINE void* upb_realloc(upb_alloc* alloc, void* ptr, size_t oldsize,
                             size_t size) {
  UPB_ASSERT(alloc);
  return alloc->func(alloc, ptr, oldsize, size);
}

UPB_INLINE void upb_free(upb_alloc* alloc, void* ptr) {
  UPB_ASSERT(alloc);
  alloc->func(alloc, ptr, 0, 0);
}

// The global allocator used by upb. Uses the standard malloc()/free().

extern upb_alloc upb_alloc_global;

/* Functions that hard-code the global malloc.
 *
 * We still get benefit because we can put custom logic into our global
 * allocator, like injecting out-of-memory faults in debug/testing builds. */

UPB_INLINE void* upb_gmalloc(size_t size) {
  return upb_malloc(&upb_alloc_global, size);
}

UPB_INLINE void* upb_grealloc(void* ptr, size_t oldsize, size_t size) {
  return upb_realloc(&upb_alloc_global, ptr, oldsize, size);
}

UPB_INLINE void upb_gfree(void* ptr) { upb_free(&upb_alloc_global, ptr); }

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_MEM_ALLOC_H_ */

// Must be last.

typedef struct upb_Arena upb_Arena;

// LINT.IfChange(arena_head)

typedef struct {
  char *ptr, *end;
} _upb_ArenaHead;

// LINT.ThenChange(//depot/google3/third_party/upb/js/impl/upb_bits/arena.ts:arena_head)

#ifdef __cplusplus
extern "C" {
#endif

// Creates an arena from the given initial block (if any -- n may be 0).
// Additional blocks will be allocated from |alloc|.  If |alloc| is NULL, this
// is a fixed-size arena and cannot grow.
UPB_API upb_Arena* upb_Arena_Init(void* mem, size_t n, upb_alloc* alloc);

UPB_API void upb_Arena_Free(upb_Arena* a);
UPB_API bool upb_Arena_Fuse(upb_Arena* a, upb_Arena* b);

void* _upb_Arena_SlowMalloc(upb_Arena* a, size_t size);
size_t upb_Arena_SpaceAllocated(upb_Arena* arena);
uint32_t upb_Arena_DebugRefCount(upb_Arena* arena);

UPB_INLINE size_t _upb_ArenaHas(upb_Arena* a) {
  _upb_ArenaHead* h = (_upb_ArenaHead*)a;
  return (size_t)(h->end - h->ptr);
}

UPB_API_INLINE void* upb_Arena_Malloc(upb_Arena* a, size_t size) {
  size = UPB_ALIGN_MALLOC(size);
  if (UPB_UNLIKELY(_upb_ArenaHas(a) < size)) {
    return _upb_Arena_SlowMalloc(a, size);
  }

  // We have enough space to do a fast malloc.
  _upb_ArenaHead* h = (_upb_ArenaHead*)a;
  void* ret = h->ptr;
  UPB_ASSERT(UPB_ALIGN_MALLOC((uintptr_t)ret) == (uintptr_t)ret);
  UPB_ASSERT(UPB_ALIGN_MALLOC(size) == size);
  UPB_UNPOISON_MEMORY_REGION(ret, size);

  h->ptr += size;

#if UPB_ASAN
  {
    size_t guard_size = 32;
    if (_upb_ArenaHas(a) >= guard_size) {
      h->ptr += guard_size;
    } else {
      h->ptr = h->end;
    }
  }
#endif

  return ret;
}

// Shrinks the last alloc from arena.
// REQUIRES: (ptr, oldsize) was the last malloc/realloc from this arena.
// We could also add a upb_Arena_TryShrinkLast() which is simply a no-op if
// this was not the last alloc.
UPB_API_INLINE void upb_Arena_ShrinkLast(upb_Arena* a, void* ptr,
                                         size_t oldsize, size_t size) {
  _upb_ArenaHead* h = (_upb_ArenaHead*)a;
  oldsize = UPB_ALIGN_MALLOC(oldsize);
  size = UPB_ALIGN_MALLOC(size);
  UPB_ASSERT((char*)ptr + oldsize == h->ptr);  // Must be the last alloc.
  UPB_ASSERT(size <= oldsize);
  h->ptr = (char*)ptr + size;
}

UPB_API_INLINE void* upb_Arena_Realloc(upb_Arena* a, void* ptr, size_t oldsize,
                                       size_t size) {
  _upb_ArenaHead* h = (_upb_ArenaHead*)a;
  oldsize = UPB_ALIGN_MALLOC(oldsize);
  size = UPB_ALIGN_MALLOC(size);
  bool is_most_recent_alloc = (uintptr_t)ptr + oldsize == (uintptr_t)h->ptr;

  if (is_most_recent_alloc) {
    ptrdiff_t diff = size - oldsize;
    if ((ptrdiff_t)_upb_ArenaHas(a) >= diff) {
      h->ptr += diff;
      return ptr;
    }
  } else if (size <= oldsize) {
    return ptr;
  }

  void* ret = upb_Arena_Malloc(a, size);

  if (ret && oldsize > 0) {
    memcpy(ret, ptr, UPB_MIN(oldsize, size));
  }

  return ret;
}

UPB_API_INLINE upb_Arena* upb_Arena_New(void) {
  return upb_Arena_Init(NULL, 0, &upb_alloc_global);
}

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_MEM_ARENA_H_ */

// Must be last.

#ifdef __cplusplus
extern "C" {
#endif

// Creates a new array on the given arena that holds elements of this type.
UPB_API upb_Array* upb_Array_New(upb_Arena* a, upb_CType type);

// Returns the number of elements in the array.
UPB_API size_t upb_Array_Size(const upb_Array* arr);

// Returns the given element, which must be within the array's current size.
UPB_API upb_MessageValue upb_Array_Get(const upb_Array* arr, size_t i);

// Sets the given element, which must be within the array's current size.
UPB_API void upb_Array_Set(upb_Array* arr, size_t i, upb_MessageValue val);

// Appends an element to the array. Returns false on allocation failure.
UPB_API bool upb_Array_Append(upb_Array* array, upb_MessageValue val,
                              upb_Arena* arena);

// Moves elements within the array using memmove().
// Like memmove(), the source and destination elements may be overlapping.
UPB_API void upb_Array_Move(upb_Array* array, size_t dst_idx, size_t src_idx,
                            size_t count);

// Inserts one or more empty elements into the array.
// Existing elements are shifted right.
// The new elements have undefined state and must be set with `upb_Array_Set()`.
// REQUIRES: `i <= upb_Array_Size(arr)`
UPB_API bool upb_Array_Insert(upb_Array* array, size_t i, size_t count,
                              upb_Arena* arena);

// Deletes one or more elements from the array.
// Existing elements are shifted left.
// REQUIRES: `i + count <= upb_Array_Size(arr)`
UPB_API void upb_Array_Delete(upb_Array* array, size_t i, size_t count);

// Changes the size of a vector. New elements are initialized to NULL/0.
// Returns false on allocation failure.
UPB_API bool upb_Array_Resize(upb_Array* array, size_t size, upb_Arena* arena);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_COLLECTIONS_ARRAY_H_ */

// Must be last.

#ifdef __cplusplus
extern "C" {
#endif

// LINT.IfChange(struct_definition)
// Our internal representation for repeated fields.
struct upb_Array {
  uintptr_t data;  /* Tagged ptr: low 3 bits of ptr are lg2(elem size). */
  size_t size;     /* The number of elements in the array. */
  size_t capacity; /* Allocated storage. Measured in elements. */
};
// LINT.ThenChange(GoogleInternalName1)

UPB_INLINE size_t _upb_Array_ElementSizeLg2(const upb_Array* arr) {
  size_t ret = arr->data & 7;
  UPB_ASSERT(ret <= 4);
  return ret;
}

UPB_INLINE const void* _upb_array_constptr(const upb_Array* arr) {
  _upb_Array_ElementSizeLg2(arr);  // Check assertion.
  return (void*)(arr->data & ~(uintptr_t)7);
}

UPB_INLINE uintptr_t _upb_array_tagptr(void* ptr, int elem_size_lg2) {
  UPB_ASSERT(elem_size_lg2 <= 4);
  return (uintptr_t)ptr | elem_size_lg2;
}

UPB_INLINE void* _upb_array_ptr(upb_Array* arr) {
  return (void*)_upb_array_constptr(arr);
}

UPB_INLINE uintptr_t _upb_tag_arrptr(void* ptr, int elem_size_lg2) {
  UPB_ASSERT(elem_size_lg2 <= 4);
  UPB_ASSERT(((uintptr_t)ptr & 7) == 0);
  return (uintptr_t)ptr | (unsigned)elem_size_lg2;
}

extern const char _upb_Array_CTypeSizeLg2Table[];

UPB_INLINE size_t _upb_Array_CTypeSizeLg2(upb_CType ctype) {
  return _upb_Array_CTypeSizeLg2Table[ctype];
}

UPB_INLINE upb_Array* _upb_Array_New(upb_Arena* a, size_t init_capacity,
                                     int elem_size_lg2) {
  UPB_ASSERT(elem_size_lg2 <= 4);
  const size_t arr_size = UPB_ALIGN_UP(sizeof(upb_Array), UPB_MALLOC_ALIGN);
  const size_t bytes = arr_size + (init_capacity << elem_size_lg2);
  upb_Array* arr = (upb_Array*)upb_Arena_Malloc(a, bytes);
  if (!arr) return NULL;
  arr->data = _upb_tag_arrptr(UPB_PTR_AT(arr, arr_size, void), elem_size_lg2);
  arr->size = 0;
  arr->capacity = init_capacity;
  return arr;
}

// Resizes the capacity of the array to be at least min_size.
bool _upb_array_realloc(upb_Array* arr, size_t min_size, upb_Arena* arena);

UPB_INLINE bool _upb_array_reserve(upb_Array* arr, size_t size,
                                   upb_Arena* arena) {
  if (arr->capacity < size) return _upb_array_realloc(arr, size, arena);
  return true;
}

// Resize without initializing new elements.
UPB_INLINE bool _upb_Array_ResizeUninitialized(upb_Array* arr, size_t size,
                                               upb_Arena* arena) {
  UPB_ASSERT(size <= arr->size || arena);  // Allow NULL arena when shrinking.
  if (!_upb_array_reserve(arr, size, arena)) return false;
  arr->size = size;
  return true;
}

// This function is intended for situations where elem_size is compile-time
// constant or a known expression of the form (1 << lg2), so that the expression
// i*elem_size does not result in an actual multiplication.
UPB_INLINE void _upb_Array_Set(upb_Array* arr, size_t i, const void* data,
                               size_t elem_size) {
  UPB_ASSERT(i < arr->size);
  UPB_ASSERT(elem_size == 1U << _upb_Array_ElementSizeLg2(arr));
  char* arr_data = (char*)_upb_array_ptr(arr);
  memcpy(arr_data + (i * elem_size), data, elem_size);
}

UPB_INLINE void _upb_array_detach(const void* msg, size_t ofs) {
  *UPB_PTR_AT(msg, ofs, upb_Array*) = NULL;
}

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_INTERNAL_ARRAY_INTERNAL_H_ */

#ifndef UPB_COLLECTIONS_MAP_H_
#define UPB_COLLECTIONS_MAP_H_


// Must be last.

#ifdef __cplusplus
extern "C" {
#endif

// Creates a new map on the given arena with the given key/value size.
UPB_API upb_Map* upb_Map_New(upb_Arena* a, upb_CType key_type,
                             upb_CType value_type);

// Returns the number of entries in the map.
UPB_API size_t upb_Map_Size(const upb_Map* map);

// Stores a value for the given key into |*val| (or the zero value if the key is
// not present). Returns whether the key was present. The |val| pointer may be
// NULL, in which case the function tests whether the given key is present.
UPB_API bool upb_Map_Get(const upb_Map* map, upb_MessageValue key,
                         upb_MessageValue* val);

// Removes all entries in the map.
UPB_API void upb_Map_Clear(upb_Map* map);

typedef enum {
  kUpb_MapInsertStatus_Inserted = 0,
  kUpb_MapInsertStatus_Replaced = 1,
  kUpb_MapInsertStatus_OutOfMemory = 2,
} upb_MapInsertStatus;

// Sets the given key to the given value, returning whether the key was inserted
// or replaced. If the key was inserted, then any existing iterators will be
// invalidated.
UPB_API upb_MapInsertStatus upb_Map_Insert(upb_Map* map, upb_MessageValue key,
                                           upb_MessageValue val,
                                           upb_Arena* arena);

// Sets the given key to the given value. Returns false if memory allocation
// failed. If the key is newly inserted, then any existing iterators will be
// invalidated.
UPB_API_INLINE bool upb_Map_Set(upb_Map* map, upb_MessageValue key,
                                upb_MessageValue val, upb_Arena* arena) {
  return upb_Map_Insert(map, key, val, arena) !=
         kUpb_MapInsertStatus_OutOfMemory;
}

// Deletes this key from the table. Returns true if the key was present.
// If present and |val| is non-NULL, stores the deleted value.
UPB_API bool upb_Map_Delete(upb_Map* map, upb_MessageValue key,
                            upb_MessageValue* val);

// (DEPRECATED and going away soon. Do not use.)
UPB_INLINE bool upb_Map_Delete2(upb_Map* map, upb_MessageValue key,
                                upb_MessageValue* val) {
  return upb_Map_Delete(map, key, val);
}

// Map iteration:
//
// size_t iter = kUpb_Map_Begin;
// upb_MessageValue key, val;
// while (upb_Map_Next(map, &key, &val, &iter)) {
//   ...
// }

#define kUpb_Map_Begin ((size_t)-1)

// Advances to the next entry. Returns false if no more entries are present.
// Otherwise returns true and populates both *key and *value.
UPB_API bool upb_Map_Next(const upb_Map* map, upb_MessageValue* key,
                          upb_MessageValue* val, size_t* iter);

// DEPRECATED iterator, slated for removal.

/* Map iteration:
 *
 * size_t iter = kUpb_Map_Begin;
 * while (upb_MapIterator_Next(map, &iter)) {
 *   upb_MessageValue key = upb_MapIterator_Key(map, iter);
 *   upb_MessageValue val = upb_MapIterator_Value(map, iter);
 * }
 */

// Advances to the next entry. Returns false if no more entries are present.
bool upb_MapIterator_Next(const upb_Map* map, size_t* iter);

// Returns true if the iterator still points to a valid entry, or false if the
// iterator is past the last element. It is an error to call this function with
// kUpb_Map_Begin (you must call next() at least once first).
bool upb_MapIterator_Done(const upb_Map* map, size_t iter);

// Returns the key and value for this entry of the map.
upb_MessageValue upb_MapIterator_Key(const upb_Map* map, size_t iter);
upb_MessageValue upb_MapIterator_Value(const upb_Map* map, size_t iter);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_COLLECTIONS_MAP_H_ */

// EVERYTHING BELOW THIS LINE IS INTERNAL - DO NOT USE /////////////////////////

#ifndef UPB_COLLECTIONS_MAP_INTERNAL_H_
#define UPB_COLLECTIONS_MAP_INTERNAL_H_


#ifndef UPB_HASH_STR_TABLE_H_
#define UPB_HASH_STR_TABLE_H_


/*
 * upb_table
 *
 * This header is INTERNAL-ONLY!  Its interfaces are not public or stable!
 * This file defines very fast int->upb_value (inttable) and string->upb_value
 * (strtable) hash tables.
 *
 * The table uses chained scatter with Brent's variation (inspired by the Lua
 * implementation of hash tables).  The hash function for strings is Austin
 * Appleby's "MurmurHash."
 *
 * The inttable uses uintptr_t as its key, which guarantees it can be used to
 * store pointers or integers of at least 32 bits (upb isn't really useful on
 * systems where sizeof(void*) < 4).
 *
 * The table must be homogeneous (all values of the same type).  In debug
 * mode, we check this on insert and lookup.
 */

#ifndef UPB_HASH_COMMON_H_
#define UPB_HASH_COMMON_H_

#include <string.h>


// Must be last.

#ifdef __cplusplus
extern "C" {
#endif

/* upb_value ******************************************************************/

typedef struct {
  uint64_t val;
} upb_value;

/* Variant that works with a length-delimited rather than NULL-delimited string,
 * as supported by strtable. */
char* upb_strdup2(const char* s, size_t len, upb_Arena* a);

UPB_INLINE void _upb_value_setval(upb_value* v, uint64_t val) { v->val = val; }

/* For each value ctype, define the following set of functions:
 *
 * // Get/set an int32 from a upb_value.
 * int32_t upb_value_getint32(upb_value val);
 * void upb_value_setint32(upb_value *val, int32_t cval);
 *
 * // Construct a new upb_value from an int32.
 * upb_value upb_value_int32(int32_t val); */
#define FUNCS(name, membername, type_t, converter, proto_type)       \
  UPB_INLINE void upb_value_set##name(upb_value* val, type_t cval) { \
    val->val = (converter)cval;                                      \
  }                                                                  \
  UPB_INLINE upb_value upb_value_##name(type_t val) {                \
    upb_value ret;                                                   \
    upb_value_set##name(&ret, val);                                  \
    return ret;                                                      \
  }                                                                  \
  UPB_INLINE type_t upb_value_get##name(upb_value val) {             \
    return (type_t)(converter)val.val;                               \
  }

FUNCS(int32, int32, int32_t, int32_t, UPB_CTYPE_INT32)
FUNCS(int64, int64, int64_t, int64_t, UPB_CTYPE_INT64)
FUNCS(uint32, uint32, uint32_t, uint32_t, UPB_CTYPE_UINT32)
FUNCS(uint64, uint64, uint64_t, uint64_t, UPB_CTYPE_UINT64)
FUNCS(bool, _bool, bool, bool, UPB_CTYPE_BOOL)
FUNCS(cstr, cstr, char*, uintptr_t, UPB_CTYPE_CSTR)
FUNCS(ptr, ptr, void*, uintptr_t, UPB_CTYPE_PTR)
FUNCS(constptr, constptr, const void*, uintptr_t, UPB_CTYPE_CONSTPTR)

#undef FUNCS

UPB_INLINE void upb_value_setfloat(upb_value* val, float cval) {
  memcpy(&val->val, &cval, sizeof(cval));
}

UPB_INLINE void upb_value_setdouble(upb_value* val, double cval) {
  memcpy(&val->val, &cval, sizeof(cval));
}

UPB_INLINE upb_value upb_value_float(float cval) {
  upb_value ret;
  upb_value_setfloat(&ret, cval);
  return ret;
}

UPB_INLINE upb_value upb_value_double(double cval) {
  upb_value ret;
  upb_value_setdouble(&ret, cval);
  return ret;
}

#undef SET_TYPE

/* upb_tabkey *****************************************************************/

/* Either:
 *   1. an actual integer key, or
 *   2. a pointer to a string prefixed by its uint32_t length, owned by us.
 *
 * ...depending on whether this is a string table or an int table.  We would
 * make this a union of those two types, but C89 doesn't support statically
 * initializing a non-first union member. */
typedef uintptr_t upb_tabkey;

UPB_INLINE char* upb_tabstr(upb_tabkey key, uint32_t* len) {
  char* mem = (char*)key;
  if (len) memcpy(len, mem, sizeof(*len));
  return mem + sizeof(*len);
}

UPB_INLINE upb_StringView upb_tabstrview(upb_tabkey key) {
  upb_StringView ret;
  uint32_t len;
  ret.data = upb_tabstr(key, &len);
  ret.size = len;
  return ret;
}

/* upb_tabval *****************************************************************/

typedef struct upb_tabval {
  uint64_t val;
} upb_tabval;

#define UPB_TABVALUE_EMPTY_INIT \
  { -1 }

/* upb_table ******************************************************************/

typedef struct _upb_tabent {
  upb_tabkey key;
  upb_tabval val;

  /* Internal chaining.  This is const so we can create static initializers for
   * tables.  We cast away const sometimes, but *only* when the containing
   * upb_table is known to be non-const.  This requires a bit of care, but
   * the subtlety is confined to table.c. */
  const struct _upb_tabent* next;
} upb_tabent;

typedef struct {
  size_t count;       /* Number of entries in the hash part. */
  uint32_t mask;      /* Mask to turn hash value -> bucket. */
  uint32_t max_count; /* Max count before we hit our load limit. */
  uint8_t size_lg2;   /* Size of the hashtable part is 2^size_lg2 entries. */
  upb_tabent* entries;
} upb_table;

UPB_INLINE size_t upb_table_size(const upb_table* t) {
  return t->size_lg2 ? 1 << t->size_lg2 : 0;
}

// Internal-only functions, in .h file only out of necessity.

UPB_INLINE bool upb_tabent_isempty(const upb_tabent* e) { return e->key == 0; }

uint32_t _upb_Hash(const void* p, size_t n, uint64_t seed);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_HASH_COMMON_H_ */

// Must be last.

typedef struct {
  upb_table t;
} upb_strtable;

#ifdef __cplusplus
extern "C" {
#endif

// Initialize a table. If memory allocation failed, false is returned and
// the table is uninitialized.
bool upb_strtable_init(upb_strtable* table, size_t expected_size, upb_Arena* a);

// Returns the number of values in the table.
UPB_INLINE size_t upb_strtable_count(const upb_strtable* t) {
  return t->t.count;
}

void upb_strtable_clear(upb_strtable* t);

// Inserts the given key into the hashtable with the given value.
// The key must not already exist in the hash table. The key is not required
// to be NULL-terminated, and the table will make an internal copy of the key.
//
// If a table resize was required but memory allocation failed, false is
// returned and the table is unchanged. */
bool upb_strtable_insert(upb_strtable* t, const char* key, size_t len,
                         upb_value val, upb_Arena* a);

// Looks up key in this table, returning "true" if the key was found.
// If v is non-NULL, copies the value for this key into *v.
bool upb_strtable_lookup2(const upb_strtable* t, const char* key, size_t len,
                          upb_value* v);

// For NULL-terminated strings.
UPB_INLINE bool upb_strtable_lookup(const upb_strtable* t, const char* key,
                                    upb_value* v) {
  return upb_strtable_lookup2(t, key, strlen(key), v);
}

// Removes an item from the table. Returns true if the remove was successful,
// and stores the removed item in *val if non-NULL.
bool upb_strtable_remove2(upb_strtable* t, const char* key, size_t len,
                          upb_value* val);

UPB_INLINE bool upb_strtable_remove(upb_strtable* t, const char* key,
                                    upb_value* v) {
  return upb_strtable_remove2(t, key, strlen(key), v);
}

// Exposed for testing only.
bool upb_strtable_resize(upb_strtable* t, size_t size_lg2, upb_Arena* a);

/* Iteration over strtable:
 *
 *   intptr_t iter = UPB_STRTABLE_BEGIN;
 *   upb_StringView key;
 *   upb_value val;
 *   while (upb_strtable_next2(t, &key, &val, &iter)) {
 *      // ...
 *   }
 */

#define UPB_STRTABLE_BEGIN -1

bool upb_strtable_next2(const upb_strtable* t, upb_StringView* key,
                        upb_value* val, intptr_t* iter);
void upb_strtable_removeiter(upb_strtable* t, intptr_t* iter);

/* DEPRECATED iterators, slated for removal.
 *
 * Iterators for string tables.  We are subject to some kind of unusual
 * design constraints:
 *
 * For high-level languages:
 *  - we must be able to guarantee that we don't crash or corrupt memory even if
 *    the program accesses an invalidated iterator.
 *
 * For C++11 range-based for:
 *  - iterators must be copyable
 *  - iterators must be comparable
 *  - it must be possible to construct an "end" value.
 *
 * Iteration order is undefined.
 *
 * Modifying the table invalidates iterators.  upb_{str,int}table_done() is
 * guaranteed to work even on an invalidated iterator, as long as the table it
 * is iterating over has not been freed.  Calling next() or accessing data from
 * an invalidated iterator yields unspecified elements from the table, but it is
 * guaranteed not to crash and to return real table elements (except when done()
 * is true). */
/* upb_strtable_iter **********************************************************/

/*   upb_strtable_iter i;
 *   upb_strtable_begin(&i, t);
 *   for(; !upb_strtable_done(&i); upb_strtable_next(&i)) {
 *     const char *key = upb_strtable_iter_key(&i);
 *     const upb_value val = upb_strtable_iter_value(&i);
 *     // ...
 *   }
 */

typedef struct {
  const upb_strtable* t;
  size_t index;
} upb_strtable_iter;

UPB_INLINE const upb_tabent* str_tabent(const upb_strtable_iter* i) {
  return &i->t->t.entries[i->index];
}

void upb_strtable_begin(upb_strtable_iter* i, const upb_strtable* t);
void upb_strtable_next(upb_strtable_iter* i);
bool upb_strtable_done(const upb_strtable_iter* i);
upb_StringView upb_strtable_iter_key(const upb_strtable_iter* i);
upb_value upb_strtable_iter_value(const upb_strtable_iter* i);
void upb_strtable_iter_setdone(upb_strtable_iter* i);
bool upb_strtable_iter_isequal(const upb_strtable_iter* i1,
                               const upb_strtable_iter* i2);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_HASH_STR_TABLE_H_ */

// Must be last.

struct upb_Map {
  // Size of key and val, based on the map type.
  // Strings are represented as '0' because they must be handled specially.
  char key_size;
  char val_size;

  upb_strtable table;
};

#ifdef __cplusplus
extern "C" {
#endif

// Converting between internal table representation and user values.
//
// _upb_map_tokey() and _upb_map_fromkey() are inverses.
// _upb_map_tovalue() and _upb_map_fromvalue() are inverses.
//
// These functions account for the fact that strings are treated differently
// from other types when stored in a map.

UPB_INLINE upb_StringView _upb_map_tokey(const void* key, size_t size) {
  if (size == UPB_MAPTYPE_STRING) {
    return *(upb_StringView*)key;
  } else {
    return upb_StringView_FromDataAndSize((const char*)key, size);
  }
}

UPB_INLINE void _upb_map_fromkey(upb_StringView key, void* out, size_t size) {
  if (size == UPB_MAPTYPE_STRING) {
    memcpy(out, &key, sizeof(key));
  } else {
    memcpy(out, key.data, size);
  }
}

UPB_INLINE bool _upb_map_tovalue(const void* val, size_t size,
                                 upb_value* msgval, upb_Arena* a) {
  if (size == UPB_MAPTYPE_STRING) {
    upb_StringView* strp = (upb_StringView*)upb_Arena_Malloc(a, sizeof(*strp));
    if (!strp) return false;
    *strp = *(upb_StringView*)val;
    *msgval = upb_value_ptr(strp);
  } else {
    memcpy(msgval, val, size);
  }
  return true;
}

UPB_INLINE void _upb_map_fromvalue(upb_value val, void* out, size_t size) {
  if (size == UPB_MAPTYPE_STRING) {
    const upb_StringView* strp = (const upb_StringView*)upb_value_getptr(val);
    memcpy(out, strp, sizeof(upb_StringView));
  } else {
    memcpy(out, &val, size);
  }
}

UPB_INLINE void* _upb_map_next(const upb_Map* map, size_t* iter) {
  upb_strtable_iter it;
  it.t = &map->table;
  it.index = *iter;
  upb_strtable_next(&it);
  *iter = it.index;
  if (upb_strtable_done(&it)) return NULL;
  return (void*)str_tabent(&it);
}

UPB_INLINE void _upb_Map_Clear(upb_Map* map) {
  upb_strtable_clear(&map->table);
}

UPB_INLINE bool _upb_Map_Delete(upb_Map* map, const void* key, size_t key_size,
                                upb_value* val) {
  upb_StringView k = _upb_map_tokey(key, key_size);
  return upb_strtable_remove2(&map->table, k.data, k.size, val);
}

UPB_INLINE bool _upb_Map_Get(const upb_Map* map, const void* key,
                             size_t key_size, void* val, size_t val_size) {
  upb_value tabval;
  upb_StringView k = _upb_map_tokey(key, key_size);
  bool ret = upb_strtable_lookup2(&map->table, k.data, k.size, &tabval);
  if (ret && val) {
    _upb_map_fromvalue(tabval, val, val_size);
  }
  return ret;
}

UPB_INLINE upb_MapInsertStatus _upb_Map_Insert(upb_Map* map, const void* key,
                                               size_t key_size, void* val,
                                               size_t val_size, upb_Arena* a) {
  upb_StringView strkey = _upb_map_tokey(key, key_size);
  upb_value tabval = {0};
  if (!_upb_map_tovalue(val, val_size, &tabval, a)) {
    return kUpb_MapInsertStatus_OutOfMemory;
  }

  // TODO(haberman): add overwrite operation to minimize number of lookups.
  bool removed =
      upb_strtable_remove2(&map->table, strkey.data, strkey.size, NULL);
  if (!upb_strtable_insert(&map->table, strkey.data, strkey.size, tabval, a)) {
    return kUpb_MapInsertStatus_OutOfMemory;
  }
  return removed ? kUpb_MapInsertStatus_Replaced
                 : kUpb_MapInsertStatus_Inserted;
}

UPB_INLINE size_t _upb_Map_Size(const upb_Map* map) {
  return map->table.t.count;
}

// Strings/bytes are special-cased in maps.
extern char _upb_Map_CTypeSizeTable[12];

UPB_INLINE size_t _upb_Map_CTypeSize(upb_CType ctype) {
  return _upb_Map_CTypeSizeTable[ctype];
}

// Creates a new map on the given arena with this key/value type.
upb_Map* _upb_Map_New(upb_Arena* a, size_t key_size, size_t value_size);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_COLLECTIONS_MAP_INTERNAL_H_ */

#ifndef UPB_BASE_LOG2_H_
#define UPB_BASE_LOG2_H_

// Must be last.

#ifdef __cplusplus
extern "C" {
#endif

UPB_INLINE int upb_Log2Ceiling(int x) {
  if (x <= 1) return 0;
#ifdef __GNUC__
  return 32 - __builtin_clz(x - 1);
#else
  int lg2 = 0;
  while (1 << lg2 < x) lg2++;
  return lg2;
#endif
}

UPB_INLINE int upb_Log2CeilingSize(int x) { return 1 << upb_Log2Ceiling(x); }

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_BASE_LOG2_H_ */

// EVERYTHING BELOW THIS LINE IS INTERNAL - DO NOT USE /////////////////////////

#ifndef UPB_COLLECTIONS_MAP_SORTER_INTERNAL_H_
#define UPB_COLLECTIONS_MAP_SORTER_INTERNAL_H_

#include <stdlib.h>


#ifndef UPB_MESSAGE_EXTENSION_INTERNAL_H_
#define UPB_MESSAGE_EXTENSION_INTERNAL_H_


// Public APIs for message operations that do not depend on the schema.
//
// MiniTable-based accessors live in accessors.h.

#ifndef UPB_MESSAGE_MESSAGE_H_
#define UPB_MESSAGE_MESSAGE_H_


// Must be last.

#ifdef __cplusplus
extern "C" {
#endif

// Creates a new message with the given mini_table on the given arena.
UPB_API upb_Message* upb_Message_New(const upb_MiniTable* mini_table,
                                     upb_Arena* arena);

// Adds unknown data (serialized protobuf data) to the given message.
// The data is copied into the message instance.
void upb_Message_AddUnknown(upb_Message* msg, const char* data, size_t len,
                            upb_Arena* arena);

// Returns a reference to the message's unknown data.
const char* upb_Message_GetUnknown(const upb_Message* msg, size_t* len);

// Removes partial unknown data from message.
void upb_Message_DeleteUnknown(upb_Message* msg, const char* data, size_t len);

// Returns the number of extensions present in this message.
size_t upb_Message_ExtensionCount(const upb_Message* msg);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_MESSAGE_MESSAGE_H_ */

#ifndef UPB_MINI_TABLE_EXTENSION_INTERNAL_H_
#define UPB_MINI_TABLE_EXTENSION_INTERNAL_H_


#ifndef UPB_MINI_TABLE_FIELD_INTERNAL_H_
#define UPB_MINI_TABLE_FIELD_INTERNAL_H_


// Must be last.

// LINT.IfChange(mini_table_field_layout)

struct upb_MiniTableField {
  uint32_t number;
  uint16_t offset;
  int16_t presence;       // If >0, hasbit_index.  If <0, ~oneof_index

  // Indexes into `upb_MiniTable.subs`
  // Will be set to `kUpb_NoSub` if `descriptortype` != MESSAGE/GROUP/ENUM
  uint16_t UPB_PRIVATE(submsg_index);

  uint8_t UPB_PRIVATE(descriptortype);

  // upb_FieldMode | upb_LabelFlags | (upb_FieldRep << kUpb_FieldRep_Shift)
  uint8_t mode;
};

#define kUpb_NoSub ((uint16_t)-1)

typedef enum {
  kUpb_FieldMode_Map = 0,
  kUpb_FieldMode_Array = 1,
  kUpb_FieldMode_Scalar = 2,
} upb_FieldMode;

// Mask to isolate the upb_FieldMode from field.mode.
#define kUpb_FieldMode_Mask 3

// Extra flags on the mode field.
typedef enum {
  kUpb_LabelFlags_IsPacked = 4,
  kUpb_LabelFlags_IsExtension = 8,
  // Indicates that this descriptor type is an "alternate type":
  //   - for Int32, this indicates that the actual type is Enum (but was
  //     rewritten to Int32 because it is an open enum that requires no check).
  //   - for Bytes, this indicates that the actual type is String (but does
  //     not require any UTF-8 check).
  kUpb_LabelFlags_IsAlternate = 16,
} upb_LabelFlags;

// Note: we sort by this number when calculating layout order.
typedef enum {
  kUpb_FieldRep_1Byte = 0,
  kUpb_FieldRep_4Byte = 1,
  kUpb_FieldRep_StringView = 2,
  kUpb_FieldRep_8Byte = 3,

  kUpb_FieldRep_NativePointer =
      UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte),
  kUpb_FieldRep_Max = kUpb_FieldRep_8Byte,
} upb_FieldRep;

#define kUpb_FieldRep_Shift 6

// LINT.ThenChange(//depot/google3/third_party/upb/js/impl/upb_bits/mini_table_field.ts:mini_table_field_layout)

UPB_INLINE upb_FieldRep
_upb_MiniTableField_GetRep(const upb_MiniTableField* field) {
  return (upb_FieldRep)(field->mode >> kUpb_FieldRep_Shift);
}

#ifdef __cplusplus
extern "C" {
#endif

UPB_INLINE upb_FieldMode upb_FieldMode_Get(const upb_MiniTableField* field) {
  return (upb_FieldMode)(field->mode & 3);
}

UPB_INLINE void _upb_MiniTableField_CheckIsArray(
    const upb_MiniTableField* field) {
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_NativePointer);
  UPB_ASSUME(upb_FieldMode_Get(field) == kUpb_FieldMode_Array);
  UPB_ASSUME(field->presence == 0);
}

UPB_INLINE void _upb_MiniTableField_CheckIsMap(
    const upb_MiniTableField* field) {
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_NativePointer);
  UPB_ASSUME(upb_FieldMode_Get(field) == kUpb_FieldMode_Map);
  UPB_ASSUME(field->presence == 0);
}

UPB_INLINE bool upb_IsRepeatedOrMap(const upb_MiniTableField* field) {
  // This works because upb_FieldMode has no value 3.
  return !(field->mode & kUpb_FieldMode_Scalar);
}

UPB_INLINE bool upb_IsSubMessage(const upb_MiniTableField* field) {
  return field->UPB_PRIVATE(descriptortype) == kUpb_FieldType_Message ||
         field->UPB_PRIVATE(descriptortype) == kUpb_FieldType_Group;
}

// LINT.IfChange(presence_logic)

// Hasbit access ///////////////////////////////////////////////////////////////

UPB_INLINE size_t _upb_hasbit_ofs(size_t idx) { return idx / 8; }

UPB_INLINE char _upb_hasbit_mask(size_t idx) { return 1 << (idx % 8); }

UPB_INLINE bool _upb_hasbit(const upb_Message* msg, size_t idx) {
  return (*UPB_PTR_AT(msg, _upb_hasbit_ofs(idx), const char) &
          _upb_hasbit_mask(idx)) != 0;
}

UPB_INLINE void _upb_sethas(const upb_Message* msg, size_t idx) {
  (*UPB_PTR_AT(msg, _upb_hasbit_ofs(idx), char)) |= _upb_hasbit_mask(idx);
}

UPB_INLINE void _upb_clearhas(const upb_Message* msg, size_t idx) {
  (*UPB_PTR_AT(msg, _upb_hasbit_ofs(idx), char)) &= ~_upb_hasbit_mask(idx);
}

UPB_INLINE size_t _upb_Message_Hasidx(const upb_MiniTableField* f) {
  UPB_ASSERT(f->presence > 0);
  return f->presence;
}

UPB_INLINE bool _upb_hasbit_field(const upb_Message* msg,
                                  const upb_MiniTableField* f) {
  return _upb_hasbit(msg, _upb_Message_Hasidx(f));
}

UPB_INLINE void _upb_sethas_field(const upb_Message* msg,
                                  const upb_MiniTableField* f) {
  _upb_sethas(msg, _upb_Message_Hasidx(f));
}

// Oneof case access ///////////////////////////////////////////////////////////

UPB_INLINE size_t _upb_oneofcase_ofs(const upb_MiniTableField* f) {
  UPB_ASSERT(f->presence < 0);
  return ~(ptrdiff_t)f->presence;
}

UPB_INLINE uint32_t* _upb_oneofcase_field(upb_Message* msg,
                                          const upb_MiniTableField* f) {
  return UPB_PTR_AT(msg, _upb_oneofcase_ofs(f), uint32_t);
}

UPB_INLINE uint32_t _upb_getoneofcase_field(const upb_Message* msg,
                                            const upb_MiniTableField* f) {
  return *_upb_oneofcase_field((upb_Message*)msg, f);
}

// LINT.ThenChange(GoogleInternalName2)
// LINT.ThenChange(//depot/google3/third_party/upb/js/impl/upb_bits/presence.ts:presence_logic)

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_MINI_TABLE_FIELD_INTERNAL_H_ */

#ifndef UPB_MINI_TABLE_SUB_INTERNAL_H_
#define UPB_MINI_TABLE_SUB_INTERNAL_H_


union upb_MiniTableSub {
  const upb_MiniTable* submsg;
  const upb_MiniTableEnum* subenum;
};

#endif /* UPB_MINI_TABLE_SUB_INTERNAL_H_ */

// Must be last.

struct upb_MiniTableExtension {
  // Do not move this field. We need to be able to alias pointers.
  upb_MiniTableField field;

  const upb_MiniTable* extendee;
  upb_MiniTableSub sub;  // NULL unless submessage or proto2 enum
};


#endif /* UPB_MINI_TABLE_EXTENSION_INTERNAL_H_ */

// Must be last.

// The internal representation of an extension is self-describing: it contains
// enough information that we can serialize it to binary format without needing
// to look it up in a upb_ExtensionRegistry.
//
// This representation allocates 16 bytes to data on 64-bit platforms.
// This is rather wasteful for scalars (in the extreme case of bool,
// it wastes 15 bytes). We accept this because we expect messages to be
// the most common extension type.
typedef struct {
  const upb_MiniTableExtension* ext;
  union {
    upb_StringView str;
    void* ptr;
    char scalar_data[8];
  } data;
} upb_Message_Extension;

#ifdef __cplusplus
extern "C" {
#endif

// Adds the given extension data to the given message.
// |ext| is copied into the message instance.
// This logically replaces any previously-added extension with this number.
upb_Message_Extension* _upb_Message_GetOrCreateExtension(
    upb_Message* msg, const upb_MiniTableExtension* ext, upb_Arena* arena);

// Returns an array of extensions for this message.
// Note: the array is ordered in reverse relative to the order of creation.
const upb_Message_Extension* _upb_Message_Getexts(const upb_Message* msg,
                                                  size_t* count);

// Returns an extension for the given field number, or NULL if no extension
// exists for this field number.
const upb_Message_Extension* _upb_Message_Getext(
    const upb_Message* msg, const upb_MiniTableExtension* ext);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_MESSAGE_EXTENSION_INTERNAL_H_ */

#ifndef UPB_MINI_TABLE_MESSAGE_INTERNAL_H_
#define UPB_MINI_TABLE_MESSAGE_INTERNAL_H_


// Must be last.

struct upb_Decoder;
typedef const char* _upb_FieldParser(struct upb_Decoder* d, const char* ptr,
                                     upb_Message* msg, intptr_t table,
                                     uint64_t hasbits, uint64_t data);
typedef struct {
  uint64_t field_data;
  _upb_FieldParser* field_parser;
} _upb_FastTable_Entry;

typedef enum {
  kUpb_ExtMode_NonExtendable = 0,  // Non-extendable message.
  kUpb_ExtMode_Extendable = 1,     // Normal extendable message.
  kUpb_ExtMode_IsMessageSet = 2,   // MessageSet message.
  kUpb_ExtMode_IsMessageSet_ITEM =
      3,  // MessageSet item (temporary only, see decode.c)

  // During table building we steal a bit to indicate that the message is a map
  // entry.  *Only* used during table building!
  kUpb_ExtMode_IsMapEntry = 4,
} upb_ExtMode;

// LINT.IfChange(mini_table_layout)

// upb_MiniTable represents the memory layout of a given upb_MessageDef.
// The members are public so generated code can initialize them,
// but users MUST NOT directly read or write any of its members.
struct upb_MiniTable {
  const upb_MiniTableSub* subs;
  const upb_MiniTableField* fields;

  // Must be aligned to sizeof(void*). Doesn't include internal members like
  // unknown fields, extension dict, pointer to msglayout, etc.
  uint16_t size;

  uint16_t field_count;
  uint8_t ext;  // upb_ExtMode, declared as uint8_t so sizeof(ext) == 1
  uint8_t dense_below;
  uint8_t table_mask;
  uint8_t required_count;  // Required fields have the lowest hasbits.

  // To statically initialize the tables of variable length, we need a flexible
  // array member, and we need to compile in gnu99 mode (constant initialization
  // of flexible array members is a GNU extension, not in C99 unfortunately.
  _upb_FastTable_Entry fasttable[];
};

// LINT.ThenChange(//depot/google3/third_party/upb/js/impl/upb_bits/mini_table.ts:presence_logic)

// Map entries aren't actually stored for map fields, they are only used during
// parsing. For parsing, it helps a lot if all map entry messages have the same
// layout. The layout code in mini_table/decode.c will ensure that all map
// entries have this layout.
//
// Note that users can and do create map entries directly, which will also use
// this layout.
//
// NOTE: sync with mini_table/decode.c.
typedef struct {
  // We only need 2 hasbits max, but due to alignment we'll use 8 bytes here,
  // and the uint64_t helps make this clear.
  uint64_t hasbits;
  union {
    upb_StringView str;  // For str/bytes.
    upb_value val;       // For all other types.
  } k;
  union {
    upb_StringView str;  // For str/bytes.
    upb_value val;       // For all other types.
  } v;
} upb_MapEntryData;

typedef struct {
  void* internal_data;
  upb_MapEntryData data;
} upb_MapEntry;

#ifdef __cplusplus
extern "C" {
#endif

// Computes a bitmask in which the |l->required_count| lowest bits are set,
// except that we skip the lowest bit (because upb never uses hasbit 0).
//
// Sample output:
//    requiredmask(1) => 0b10 (0x2)
//    requiredmask(5) => 0b111110 (0x3e)
UPB_INLINE uint64_t upb_MiniTable_requiredmask(const upb_MiniTable* l) {
  int n = l->required_count;
  assert(0 < n && n <= 63);
  return ((1ULL << n) - 1) << 1;
}

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_MINI_TABLE_MESSAGE_INTERNAL_H_ */

// Must be last.

#ifdef __cplusplus
extern "C" {
#endif

// _upb_mapsorter sorts maps and provides ordered iteration over the entries.
// Since maps can be recursive (map values can be messages which contain other
// maps), _upb_mapsorter can contain a stack of maps.

typedef struct {
  void const** entries;
  int size;
  int cap;
} _upb_mapsorter;

typedef struct {
  int start;
  int pos;
  int end;
} _upb_sortedmap;

UPB_INLINE void _upb_mapsorter_init(_upb_mapsorter* s) {
  s->entries = NULL;
  s->size = 0;
  s->cap = 0;
}

UPB_INLINE void _upb_mapsorter_destroy(_upb_mapsorter* s) {
  if (s->entries) free(s->entries);
}

UPB_INLINE bool _upb_sortedmap_next(_upb_mapsorter* s, const upb_Map* map,
                                    _upb_sortedmap* sorted, upb_MapEntry* ent) {
  if (sorted->pos == sorted->end) return false;
  const upb_tabent* tabent = (const upb_tabent*)s->entries[sorted->pos++];
  upb_StringView key = upb_tabstrview(tabent->key);
  _upb_map_fromkey(key, &ent->data.k, map->key_size);
  upb_value val = {tabent->val.val};
  _upb_map_fromvalue(val, &ent->data.v, map->val_size);
  return true;
}

UPB_INLINE bool _upb_sortedmap_nextext(_upb_mapsorter* s,
                                       _upb_sortedmap* sorted,
                                       const upb_Message_Extension** ext) {
  if (sorted->pos == sorted->end) return false;
  *ext = (const upb_Message_Extension*)s->entries[sorted->pos++];
  return true;
}

UPB_INLINE void _upb_mapsorter_popmap(_upb_mapsorter* s,
                                      _upb_sortedmap* sorted) {
  s->size = sorted->start;
}

bool _upb_mapsorter_pushmap(_upb_mapsorter* s, upb_FieldType key_type,
                            const upb_Map* map, _upb_sortedmap* sorted);

bool _upb_mapsorter_pushexts(_upb_mapsorter* s,
                             const upb_Message_Extension* exts, size_t count,
                             _upb_sortedmap* sorted);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_COLLECTIONS_MAP_SORTER_INTERNAL_H_ */

/*
** Our memory representation for parsing tables and messages themselves.
** Functions in this file are used by generated code and possibly reflection.
**
** The definitions in this file are internal to upb.
**/

#ifndef UPB_MESSAGE_INTERNAL_H_
#define UPB_MESSAGE_INTERNAL_H_

#include <stdlib.h>
#include <string.h>


#ifndef UPB_MINI_TABLE_EXTENSION_REGISTRY_H_
#define UPB_MINI_TABLE_EXTENSION_REGISTRY_H_


// Must be last.

#ifdef __cplusplus
extern "C" {
#endif

/* Extension registry: a dynamic data structure that stores a map of:
 *   (upb_MiniTable, number) -> extension info
 *
 * upb_decode() uses upb_ExtensionRegistry to look up extensions while parsing
 * binary format.
 *
 * upb_ExtensionRegistry is part of the mini-table (msglayout) family of
 * objects. Like all mini-table objects, it is suitable for reflection-less
 * builds that do not want to expose names into the binary.
 *
 * Unlike most mini-table types, upb_ExtensionRegistry requires dynamic memory
 * allocation and dynamic initialization:
 * * If reflection is being used, then upb_DefPool will construct an appropriate
 *   upb_ExtensionRegistry automatically.
 * * For a mini-table only build, the user must manually construct the
 *   upb_ExtensionRegistry and populate it with all of the extensions the user
 * cares about.
 * * A third alternative is to manually unpack relevant extensions after the
 *   main parse is complete, similar to how Any works. This is perhaps the
 *   nicest solution from the perspective of reducing dependencies, avoiding
 *   dynamic memory allocation, and avoiding the need to parse uninteresting
 *   extensions.  The downsides are:
 *     (1) parse errors are not caught during the main parse
 *     (2) the CPU hit of parsing comes during access, which could cause an
 *         undesirable stutter in application performance.
 *
 * Users cannot directly get or put into this map. Users can only add the
 * extensions from a generated module and pass the extension registry to the
 * binary decoder.
 *
 * A upb_DefPool provides a upb_ExtensionRegistry, so any users who use
 * reflection do not need to populate a upb_ExtensionRegistry directly.
 */

typedef struct upb_ExtensionRegistry upb_ExtensionRegistry;

// Creates a upb_ExtensionRegistry in the given arena.
// The arena must outlive any use of the extreg.
UPB_API upb_ExtensionRegistry* upb_ExtensionRegistry_New(upb_Arena* arena);

UPB_API bool upb_ExtensionRegistry_Add(upb_ExtensionRegistry* r,
                                       const upb_MiniTableExtension* e);

// Adds the given extension info for the array |e| of size |count| into the
// registry. If there are any errors, the entire array is backed out.
// The extensions must outlive the registry.
// Possible errors include OOM or an extension number that already exists.
// TODO(salo): There is currently no way to know the exact reason for failure.
bool upb_ExtensionRegistry_AddArray(upb_ExtensionRegistry* r,
                                    const upb_MiniTableExtension** e,
                                    size_t count);

// Looks up the extension (if any) defined for message type |t| and field
// number |num|. Returns the extension if found, otherwise NULL.
UPB_API const upb_MiniTableExtension* upb_ExtensionRegistry_Lookup(
    const upb_ExtensionRegistry* r, const upb_MiniTable* t, uint32_t num);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_MINI_TABLE_EXTENSION_REGISTRY_H_ */

#ifndef UPB_MINI_TABLE_FILE_INTERNAL_H_
#define UPB_MINI_TABLE_FILE_INTERNAL_H_


// Must be last.

struct upb_MiniTableFile {
  const upb_MiniTable** msgs;
  const upb_MiniTableEnum** enums;
  const upb_MiniTableExtension** exts;
  int msg_count;
  int enum_count;
  int ext_count;
};


#endif /* UPB_MINI_TABLE_FILE_INTERNAL_H_ */

// Must be last.

#ifdef __cplusplus
extern "C" {
#endif

extern const float kUpb_FltInfinity;
extern const double kUpb_Infinity;
extern const double kUpb_NaN;

/* Internal members of a upb_Message that track unknown fields and/or
 * extensions. We can change this without breaking binary compatibility.  We put
 * these before the user's data.  The user's upb_Message* points after the
 * upb_Message_Internal. */

typedef struct {
  /* Total size of this structure, including the data that follows.
   * Must be aligned to 8, which is alignof(upb_Message_Extension) */
  uint32_t size;

  /* Offsets relative to the beginning of this structure.
   *
   * Unknown data grows forward from the beginning to unknown_end.
   * Extension data grows backward from size to ext_begin.
   * When the two meet, we're out of data and have to realloc.
   *
   * If we imagine that the final member of this struct is:
   *   char data[size - overhead];  // overhead =
   * sizeof(upb_Message_InternalData)
   *
   * Then we have:
   *   unknown data: data[0 .. (unknown_end - overhead)]
   *   extensions data: data[(ext_begin - overhead) .. (size - overhead)] */
  uint32_t unknown_end;
  uint32_t ext_begin;
  /* Data follows, as if there were an array:
   *   char data[size - sizeof(upb_Message_InternalData)]; */
} upb_Message_InternalData;

typedef struct {
  upb_Message_InternalData* internal;
  /* Message data follows. */
} upb_Message_Internal;

/* Maps upb_CType -> memory size. */
extern char _upb_CTypeo_size[12];

UPB_INLINE size_t upb_msg_sizeof(const upb_MiniTable* t) {
  return t->size + sizeof(upb_Message_Internal);
}

// Inline version upb_Message_New(), for internal use.
UPB_INLINE upb_Message* _upb_Message_New(const upb_MiniTable* mini_table,
                                         upb_Arena* arena) {
  size_t size = upb_msg_sizeof(mini_table);
  void* mem = upb_Arena_Malloc(arena, size + sizeof(upb_Message_Internal));
  if (UPB_UNLIKELY(!mem)) return NULL;
  upb_Message* msg = UPB_PTR_AT(mem, sizeof(upb_Message_Internal), upb_Message);
  memset(mem, 0, size);
  return msg;
}

UPB_INLINE upb_Message_Internal* upb_Message_Getinternal(
    const upb_Message* msg) {
  ptrdiff_t size = sizeof(upb_Message_Internal);
  return (upb_Message_Internal*)((char*)msg - size);
}

// Clears the given message.
void _upb_Message_Clear(upb_Message* msg, const upb_MiniTable* l);

// Discards the unknown fields for this message only.
void _upb_Message_DiscardUnknown_shallow(upb_Message* msg);

// Adds unknown data (serialized protobuf data) to the given message.
// The data is copied into the message instance.
bool _upb_Message_AddUnknown(upb_Message* msg, const char* data, size_t len,
                             upb_Arena* arena);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_MESSAGE_INTERNAL_H_ */

#ifndef UPB_MINI_TABLE_ENUM_INTERNAL_H_
#define UPB_MINI_TABLE_ENUM_INTERNAL_H_


// Must be last.

struct upb_MiniTableEnum {
  uint32_t mask_limit;   // Limit enum value that can be tested with mask.
  uint32_t value_count;  // Number of values after the bitfield.
  uint32_t data[];       // Bitmask + enumerated values follow.
};

typedef enum {
  _kUpb_FastEnumCheck_ValueIsInEnum = 0,
  _kUpb_FastEnumCheck_ValueIsNotInEnum = 1,
  _kUpb_FastEnumCheck_CannotCheckFast = 2,
} _kUpb_FastEnumCheck_Status;

#ifdef __cplusplus
extern "C" {
#endif

UPB_INLINE _kUpb_FastEnumCheck_Status
_upb_MiniTable_CheckEnumValueFast(const upb_MiniTableEnum* e, uint32_t val) {
  if (UPB_UNLIKELY(val >= 64)) return _kUpb_FastEnumCheck_CannotCheckFast;
  uint64_t mask = e->data[0] | ((uint64_t)e->data[1] << 32);
  return (mask & (1ULL << val)) ? _kUpb_FastEnumCheck_ValueIsInEnum
                                : _kUpb_FastEnumCheck_ValueIsNotInEnum;
}

UPB_INLINE bool _upb_MiniTable_CheckEnumValueSlow(const upb_MiniTableEnum* e,
                                                  uint32_t val) {
  if (val < e->mask_limit) return e->data[val / 32] & (1ULL << (val % 32));
  // OPT: binary search long lists?
  const uint32_t* start = &e->data[e->mask_limit / 32];
  const uint32_t* limit = &e->data[(e->mask_limit / 32) + e->value_count];
  for (const uint32_t* p = start; p < limit; p++) {
    if (*p == val) return true;
  }
  return false;
}

// Validates enum value against range defined by enum mini table.
UPB_INLINE bool upb_MiniTableEnum_CheckValue(const upb_MiniTableEnum* e,
                                             uint32_t val) {
  _kUpb_FastEnumCheck_Status status = _upb_MiniTable_CheckEnumValueFast(e, val);
  if (UPB_UNLIKELY(status == _kUpb_FastEnumCheck_CannotCheckFast)) {
    return _upb_MiniTable_CheckEnumValueSlow(e, val);
  }
  return status == _kUpb_FastEnumCheck_ValueIsInEnum ? true : false;
}

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_MINI_TABLE_ENUM_INTERNAL_H_ */
/* This file was generated by upbc (the upb compiler) from the input
 * file:
 *
 *     google/protobuf/descriptor.proto
 *
 * Do not edit -- your changes will be discarded when the file is
 * regenerated. */

#ifndef GOOGLE_PROTOBUF_DESCRIPTOR_PROTO_UPB_H_
#define GOOGLE_PROTOBUF_DESCRIPTOR_PROTO_UPB_H_


// These functions are only used by generated code.

#ifndef UPB_COLLECTIONS_MAP_GENCODE_UTIL_H_
#define UPB_COLLECTIONS_MAP_GENCODE_UTIL_H_


// Must be last.

#ifdef __cplusplus
extern "C" {
#endif

// Message map operations, these get the map from the message first.

UPB_INLINE void _upb_msg_map_key(const void* msg, void* key, size_t size) {
  const upb_tabent* ent = (const upb_tabent*)msg;
  uint32_t u32len;
  upb_StringView k;
  k.data = upb_tabstr(ent->key, &u32len);
  k.size = u32len;
  _upb_map_fromkey(k, key, size);
}

UPB_INLINE void _upb_msg_map_value(const void* msg, void* val, size_t size) {
  const upb_tabent* ent = (const upb_tabent*)msg;
  upb_value v = {ent->val.val};
  _upb_map_fromvalue(v, val, size);
}

UPB_INLINE void _upb_msg_map_set_value(void* msg, const void* val,
                                       size_t size) {
  upb_tabent* ent = (upb_tabent*)msg;
  // This is like _upb_map_tovalue() except the entry already exists
  // so we can reuse the allocated upb_StringView for string fields.
  if (size == UPB_MAPTYPE_STRING) {
    upb_StringView* strp = (upb_StringView*)(uintptr_t)ent->val.val;
    memcpy(strp, val, sizeof(*strp));
  } else {
    memcpy(&ent->val.val, val, size);
  }
}

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_COLLECTIONS_MAP_GENCODE_UTIL_H_ */

#ifndef UPB_MESSAGE_ACCESSORS_H_
#define UPB_MESSAGE_ACCESSORS_H_


#ifndef UPB_MESSAGE_ACCESSORS_INTERNAL_H_
#define UPB_MESSAGE_ACCESSORS_INTERNAL_H_


#ifndef UPB_MINI_TABLE_COMMON_H_
#define UPB_MINI_TABLE_COMMON_H_


// Must be last.

typedef enum {
  kUpb_FieldModifier_IsRepeated = 1 << 0,
  kUpb_FieldModifier_IsPacked = 1 << 1,
  kUpb_FieldModifier_IsClosedEnum = 1 << 2,
  kUpb_FieldModifier_IsProto3Singular = 1 << 3,
  kUpb_FieldModifier_IsRequired = 1 << 4,
} kUpb_FieldModifier;

typedef enum {
  kUpb_MessageModifier_ValidateUtf8 = 1 << 0,
  kUpb_MessageModifier_DefaultIsPacked = 1 << 1,
  kUpb_MessageModifier_IsExtendable = 1 << 2,
} kUpb_MessageModifier;

#ifdef __cplusplus
extern "C" {
#endif

UPB_API const upb_MiniTableField* upb_MiniTable_FindFieldByNumber(
    const upb_MiniTable* table, uint32_t number);

UPB_API_INLINE const upb_MiniTableField* upb_MiniTable_GetFieldByIndex(
    const upb_MiniTable* t, uint32_t index) {
  return &t->fields[index];
}

UPB_API upb_FieldType upb_MiniTableField_Type(const upb_MiniTableField* field);

UPB_API_INLINE upb_CType upb_MiniTableField_CType(const upb_MiniTableField* f) {
  switch (f->UPB_PRIVATE(descriptortype)) {
    case kUpb_FieldType_Double:
      return kUpb_CType_Double;
    case kUpb_FieldType_Float:
      return kUpb_CType_Float;
    case kUpb_FieldType_Int64:
    case kUpb_FieldType_SInt64:
    case kUpb_FieldType_SFixed64:
      return kUpb_CType_Int64;
    case kUpb_FieldType_Int32:
    case kUpb_FieldType_SFixed32:
    case kUpb_FieldType_SInt32:
      return kUpb_CType_Int32;
    case kUpb_FieldType_UInt64:
    case kUpb_FieldType_Fixed64:
      return kUpb_CType_UInt64;
    case kUpb_FieldType_UInt32:
    case kUpb_FieldType_Fixed32:
      return kUpb_CType_UInt32;
    case kUpb_FieldType_Enum:
      return kUpb_CType_Enum;
    case kUpb_FieldType_Bool:
      return kUpb_CType_Bool;
    case kUpb_FieldType_String:
      return kUpb_CType_String;
    case kUpb_FieldType_Bytes:
      return kUpb_CType_Bytes;
    case kUpb_FieldType_Group:
    case kUpb_FieldType_Message:
      return kUpb_CType_Message;
  }
  UPB_UNREACHABLE();
}

UPB_API_INLINE bool upb_MiniTableField_IsExtension(
    const upb_MiniTableField* field) {
  return field->mode & kUpb_LabelFlags_IsExtension;
}

UPB_API_INLINE bool upb_MiniTableField_IsClosedEnum(
    const upb_MiniTableField* field) {
  return field->UPB_PRIVATE(descriptortype) == kUpb_FieldType_Enum;
}

UPB_API_INLINE bool upb_MiniTableField_HasPresence(
    const upb_MiniTableField* field) {
  if (upb_MiniTableField_IsExtension(field)) {
    return !upb_IsRepeatedOrMap(field);
  } else {
    return field->presence != 0;
  }
}

// Returns the MiniTable for this message field.  If the field is unlinked,
// returns NULL.
UPB_API_INLINE const upb_MiniTable* upb_MiniTable_GetSubMessageTable(
    const upb_MiniTable* mini_table, const upb_MiniTableField* field) {
  UPB_ASSERT(upb_MiniTableField_CType(field) == kUpb_CType_Message);
  return mini_table->subs[field->UPB_PRIVATE(submsg_index)].submsg;
}

// Returns the MiniTableEnum for this enum field.  If the field is unlinked,
// returns NULL.
UPB_API_INLINE const upb_MiniTableEnum* upb_MiniTable_GetSubEnumTable(
    const upb_MiniTable* mini_table, const upb_MiniTableField* field) {
  UPB_ASSERT(upb_MiniTableField_CType(field) == kUpb_CType_Enum);
  return mini_table->subs[field->UPB_PRIVATE(submsg_index)].subenum;
}

// Returns true if this MiniTable field is linked to a MiniTable for the
// sub-message.
UPB_API_INLINE bool upb_MiniTable_MessageFieldIsLinked(
    const upb_MiniTable* mini_table, const upb_MiniTableField* field) {
  return upb_MiniTable_GetSubMessageTable(mini_table, field) != NULL;
}

// If this field is in a oneof, returns the first field in the oneof.
//
// Otherwise returns NULL.
//
// Usage:
//   const upb_MiniTableField* field = upb_MiniTable_GetOneof(m, f);
//   do {
//       ..
//   } while (upb_MiniTable_NextOneofField(m, &field);
//
const upb_MiniTableField* upb_MiniTable_GetOneof(const upb_MiniTable* m,
                                                 const upb_MiniTableField* f);

// Iterates to the next field in the oneof. If this is the last field in the
// oneof, returns false. The ordering of fields in the oneof is not
// guaranteed.
// REQUIRES: |f| is the field initialized by upb_MiniTable_GetOneof and updated
//           by prior upb_MiniTable_NextOneofField calls.
bool upb_MiniTable_NextOneofField(const upb_MiniTable* m,
                                  const upb_MiniTableField** f);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_MINI_TABLE_COMMON_H_ */

// Must be last.

#if defined(__GNUC__) && !defined(__clang__)
// GCC raises incorrect warnings in these functions.  It thinks that we are
// overrunning buffers, but we carefully write the functions in this file to
// guarantee that this is impossible.  GCC gets this wrong due it its failure
// to perform constant propagation as we expect:
//   - https://gcc.gnu.org/bugzilla/show_bug.cgi?id=108217
//   - https://gcc.gnu.org/bugzilla/show_bug.cgi?id=108226
//
// Unfortunately this also indicates that GCC is not optimizing away the
// switch() in cases where it should be, compromising the performance.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#if __GNUC__ >= 11
#pragma GCC diagnostic ignored "-Wstringop-overread"
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

UPB_INLINE bool _upb_MiniTableField_InOneOf(const upb_MiniTableField* field) {
  return field->presence < 0;
}

UPB_INLINE void* _upb_MiniTableField_GetPtr(upb_Message* msg,
                                            const upb_MiniTableField* field) {
  return (char*)msg + field->offset;
}

UPB_INLINE const void* _upb_MiniTableField_GetConstPtr(
    const upb_Message* msg, const upb_MiniTableField* field) {
  return (char*)msg + field->offset;
}

UPB_INLINE void _upb_Message_SetPresence(upb_Message* msg,
                                         const upb_MiniTableField* field) {
  if (field->presence > 0) {
    _upb_sethas_field(msg, field);
  } else if (_upb_MiniTableField_InOneOf(field)) {
    *_upb_oneofcase_field(msg, field) = field->number;
  }
}

// LINT.IfChange(message_raw_fields)

UPB_INLINE bool _upb_MiniTable_ValueIsNonZero(const void* default_val,
                                              const upb_MiniTableField* field) {
  char zero[16] = {0};
  switch (_upb_MiniTableField_GetRep(field)) {
    case kUpb_FieldRep_1Byte:
      return memcmp(&zero, default_val, 1) != 0;
    case kUpb_FieldRep_4Byte:
      return memcmp(&zero, default_val, 4) != 0;
    case kUpb_FieldRep_8Byte:
      return memcmp(&zero, default_val, 8) != 0;
    case kUpb_FieldRep_StringView: {
      const upb_StringView* sv = (const upb_StringView*)default_val;
      return sv->size != 0;
    }
  }
  UPB_UNREACHABLE();
}

UPB_INLINE void _upb_MiniTable_CopyFieldData(void* to, const void* from,
                                             const upb_MiniTableField* field) {
  switch (_upb_MiniTableField_GetRep(field)) {
    case kUpb_FieldRep_1Byte:
      memcpy(to, from, 1);
      return;
    case kUpb_FieldRep_4Byte:
      memcpy(to, from, 4);
      return;
    case kUpb_FieldRep_8Byte:
      memcpy(to, from, 8);
      return;
    case kUpb_FieldRep_StringView: {
      memcpy(to, from, sizeof(upb_StringView));
      return;
    }
  }
  UPB_UNREACHABLE();
}

// LINT.ThenChange(//depot/google3/third_party/upb/js/impl/upb_bits/message.ts:message_raw_fields)

UPB_INLINE size_t
_upb_MiniTable_ElementSizeLg2(const upb_MiniTableField* field) {
  const unsigned char table[] = {
      0,
      3,               // kUpb_FieldType_Double = 1,
      2,               // kUpb_FieldType_Float = 2,
      3,               // kUpb_FieldType_Int64 = 3,
      3,               // kUpb_FieldType_UInt64 = 4,
      2,               // kUpb_FieldType_Int32 = 5,
      3,               // kUpb_FieldType_Fixed64 = 6,
      2,               // kUpb_FieldType_Fixed32 = 7,
      0,               // kUpb_FieldType_Bool = 8,
      UPB_SIZE(3, 4),  // kUpb_FieldType_String = 9,
      UPB_SIZE(2, 3),  // kUpb_FieldType_Group = 10,
      UPB_SIZE(2, 3),  // kUpb_FieldType_Message = 11,
      UPB_SIZE(3, 4),  // kUpb_FieldType_Bytes = 12,
      2,               // kUpb_FieldType_UInt32 = 13,
      2,               // kUpb_FieldType_Enum = 14,
      2,               // kUpb_FieldType_SFixed32 = 15,
      3,               // kUpb_FieldType_SFixed64 = 16,
      2,               // kUpb_FieldType_SInt32 = 17,
      3,               // kUpb_FieldType_SInt64 = 18,
  };
  return table[field->UPB_PRIVATE(descriptortype)];
}

// Here we define universal getter/setter functions for message fields.
// These look very branchy and inefficient, but as long as the MiniTableField
// values are known at compile time, all the branches are optimized away and
// we are left with ideal code.  This can happen either through through
// literals or UPB_ASSUME():
//
//   // Via struct literals.
//   bool FooMessage_set_bool_field(const upb_Message* msg, bool val) {
//     const upb_MiniTableField field = {1, 0, 0, /* etc... */};
//     // All value in "field" are compile-time known.
//     _upb_Message_SetNonExtensionField(msg, &field, &value);
//   }
//
//   // Via UPB_ASSUME().
//   UPB_INLINE bool upb_Message_SetBool(upb_Message* msg,
//                                       const upb_MiniTableField* field,
//                                       bool value, upb_Arena* a) {
//     UPB_ASSUME(field->UPB_PRIVATE(descriptortype) == kUpb_FieldType_Bool);
//     UPB_ASSUME(!upb_IsRepeatedOrMap(field));
//     UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_1Byte);
//     _upb_Message_SetField(msg, field, &value, a);
//   }
//
// As a result, we can use these universal getters/setters for *all* message
// accessors: generated code, MiniTable accessors, and reflection.  The only
// exception is the binary encoder/decoder, which need to be a bit more clever
// about how they read/write the message data, for efficiency.
//
// These functions work on both extensions and non-extensions. If the field
// of a setter is known to be a non-extension, the arena may be NULL and the
// returned bool value may be ignored since it will always succeed.

UPB_INLINE bool _upb_Message_HasExtensionField(
    const upb_Message* msg, const upb_MiniTableExtension* ext) {
  UPB_ASSERT(upb_MiniTableField_HasPresence(&ext->field));
  return _upb_Message_Getext(msg, ext) != NULL;
}

UPB_INLINE bool _upb_Message_HasNonExtensionField(
    const upb_Message* msg, const upb_MiniTableField* field) {
  UPB_ASSERT(upb_MiniTableField_HasPresence(field));
  UPB_ASSUME(!upb_MiniTableField_IsExtension(field));
  if (_upb_MiniTableField_InOneOf(field)) {
    return _upb_getoneofcase_field(msg, field) == field->number;
  } else {
    return _upb_hasbit_field(msg, field);
  }
}

static UPB_FORCEINLINE void _upb_Message_GetNonExtensionField(
    const upb_Message* msg, const upb_MiniTableField* field,
    const void* default_val, void* val) {
  UPB_ASSUME(!upb_MiniTableField_IsExtension(field));
  if ((_upb_MiniTableField_InOneOf(field) ||
       _upb_MiniTable_ValueIsNonZero(default_val, field)) &&
      !_upb_Message_HasNonExtensionField(msg, field)) {
    _upb_MiniTable_CopyFieldData(val, default_val, field);
    return;
  }
  _upb_MiniTable_CopyFieldData(val, _upb_MiniTableField_GetConstPtr(msg, field),
                               field);
}

UPB_INLINE void _upb_Message_GetExtensionField(
    const upb_Message* msg, const upb_MiniTableExtension* mt_ext,
    const void* default_val, void* val) {
  UPB_ASSUME(upb_MiniTableField_IsExtension(&mt_ext->field));
  const upb_Message_Extension* ext = _upb_Message_Getext(msg, mt_ext);
  if (ext) {
    _upb_MiniTable_CopyFieldData(val, &ext->data, &mt_ext->field);
  } else {
    _upb_MiniTable_CopyFieldData(val, default_val, &mt_ext->field);
  }
}

UPB_INLINE void _upb_Message_GetField(const upb_Message* msg,
                                      const upb_MiniTableField* field,
                                      const void* default_val, void* val) {
  if (upb_MiniTableField_IsExtension(field)) {
    _upb_Message_GetExtensionField(msg, (upb_MiniTableExtension*)field,
                                   default_val, val);
  } else {
    _upb_Message_GetNonExtensionField(msg, field, default_val, val);
  }
}

UPB_INLINE void _upb_Message_SetNonExtensionField(
    upb_Message* msg, const upb_MiniTableField* field, const void* val) {
  UPB_ASSUME(!upb_MiniTableField_IsExtension(field));
  _upb_Message_SetPresence(msg, field);
  _upb_MiniTable_CopyFieldData(_upb_MiniTableField_GetPtr(msg, field), val,
                               field);
}

UPB_INLINE bool _upb_Message_SetExtensionField(
    upb_Message* msg, const upb_MiniTableExtension* mt_ext, const void* val,
    upb_Arena* a) {
  UPB_ASSERT(a);
  upb_Message_Extension* ext =
      _upb_Message_GetOrCreateExtension(msg, mt_ext, a);
  if (!ext) return false;
  _upb_MiniTable_CopyFieldData(&ext->data, val, &mt_ext->field);
  return true;
}

UPB_INLINE bool _upb_Message_SetField(upb_Message* msg,
                                      const upb_MiniTableField* field,
                                      const void* val, upb_Arena* a) {
  if (upb_MiniTableField_IsExtension(field)) {
    const upb_MiniTableExtension* ext = (const upb_MiniTableExtension*)field;
    return _upb_Message_SetExtensionField(msg, ext, val, a);
  } else {
    _upb_Message_SetNonExtensionField(msg, field, val);
    return true;
  }
}

UPB_INLINE void _upb_Message_ClearExtensionField(
    upb_Message* msg, const upb_MiniTableExtension* ext_l) {
  upb_Message_Internal* in = upb_Message_Getinternal(msg);
  if (!in->internal) return;
  const upb_Message_Extension* base =
      UPB_PTR_AT(in->internal, in->internal->ext_begin, upb_Message_Extension);
  upb_Message_Extension* ext =
      (upb_Message_Extension*)_upb_Message_Getext(msg, ext_l);
  if (ext) {
    *ext = *base;
    in->internal->ext_begin += sizeof(upb_Message_Extension);
  }
}

UPB_INLINE void _upb_Message_ClearNonExtensionField(
    upb_Message* msg, const upb_MiniTableField* field) {
  if (field->presence > 0) {
    _upb_clearhas(msg, _upb_Message_Hasidx(field));
  } else if (_upb_MiniTableField_InOneOf(field)) {
    uint32_t* oneof_case = _upb_oneofcase_field(msg, field);
    if (*oneof_case != field->number) return;
    *oneof_case = 0;
  }
  const char zeros[16] = {0};
  _upb_MiniTable_CopyFieldData(_upb_MiniTableField_GetPtr(msg, field), zeros,
                               field);
}

UPB_INLINE upb_Map* _upb_Message_GetOrCreateMutableMap(
    upb_Message* msg, const upb_MiniTableField* field, size_t key_size,
    size_t val_size, upb_Arena* arena) {
  _upb_MiniTableField_CheckIsMap(field);
  upb_Map* map = NULL;
  upb_Map* default_map_value = NULL;
  _upb_Message_GetNonExtensionField(msg, field, &default_map_value, &map);
  if (!map) {
    map = _upb_Map_New(arena, key_size, val_size);
    // Check again due to: https://godbolt.org/z/7WfaoKG1r
    _upb_MiniTableField_CheckIsMap(field);
    _upb_Message_SetNonExtensionField(msg, field, &map);
  }
  return map;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif


#endif  // UPB_MESSAGE_ACCESSORS_INTERNAL_H_

// Must be last.

#ifdef __cplusplus
extern "C" {
#endif

UPB_API_INLINE void upb_Message_ClearField(upb_Message* msg,
                                           const upb_MiniTableField* field) {
  if (upb_MiniTableField_IsExtension(field)) {
    const upb_MiniTableExtension* ext = (const upb_MiniTableExtension*)field;
    _upb_Message_ClearExtensionField(msg, ext);
  } else {
    _upb_Message_ClearNonExtensionField(msg, field);
  }
}

UPB_API_INLINE bool upb_Message_HasField(const upb_Message* msg,
                                         const upb_MiniTableField* field) {
  if (upb_MiniTableField_IsExtension(field)) {
    const upb_MiniTableExtension* ext = (const upb_MiniTableExtension*)field;
    return _upb_Message_HasExtensionField(msg, ext);
  } else {
    return _upb_Message_HasNonExtensionField(msg, field);
  }
}

UPB_API_INLINE uint32_t upb_Message_WhichOneofFieldNumber(
    const upb_Message* message, const upb_MiniTableField* oneof_field) {
  UPB_ASSUME(_upb_MiniTableField_InOneOf(oneof_field));
  return _upb_getoneofcase_field(message, oneof_field);
}

UPB_API_INLINE bool upb_Message_GetBool(const upb_Message* msg,
                                        const upb_MiniTableField* field,
                                        bool default_val) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_Bool);
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_1Byte);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  bool ret;
  _upb_Message_GetField(msg, field, &default_val, &ret);
  return ret;
}

UPB_API_INLINE bool upb_Message_SetBool(upb_Message* msg,
                                        const upb_MiniTableField* field,
                                        bool value, upb_Arena* a) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_Bool);
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_1Byte);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  return _upb_Message_SetField(msg, field, &value, a);
}

UPB_API_INLINE int32_t upb_Message_GetInt32(const upb_Message* msg,
                                            const upb_MiniTableField* field,
                                            int32_t default_val) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_Int32 ||
             upb_MiniTableField_CType(field) == kUpb_CType_Enum);
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_4Byte);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  int32_t ret;
  _upb_Message_GetField(msg, field, &default_val, &ret);
  return ret;
}

UPB_API_INLINE bool upb_Message_SetInt32(upb_Message* msg,
                                         const upb_MiniTableField* field,
                                         int32_t value, upb_Arena* a) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_Int32 ||
             upb_MiniTableField_CType(field) == kUpb_CType_Enum);
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_4Byte);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  return _upb_Message_SetField(msg, field, &value, a);
}

UPB_API_INLINE uint32_t upb_Message_GetUInt32(const upb_Message* msg,
                                              const upb_MiniTableField* field,
                                              uint32_t default_val) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_UInt32);
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_4Byte);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  uint32_t ret;
  _upb_Message_GetField(msg, field, &default_val, &ret);
  return ret;
}

UPB_API_INLINE bool upb_Message_SetUInt32(upb_Message* msg,
                                          const upb_MiniTableField* field,
                                          uint32_t value, upb_Arena* a) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_UInt32);
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_4Byte);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  return _upb_Message_SetField(msg, field, &value, a);
}

UPB_API_INLINE void upb_Message_SetClosedEnum(
    upb_Message* msg, const upb_MiniTable* msg_mini_table,
    const upb_MiniTableField* field, int32_t value) {
  UPB_ASSERT(upb_MiniTableField_IsClosedEnum(field));
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_4Byte);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  UPB_ASSERT(upb_MiniTableEnum_CheckValue(
      upb_MiniTable_GetSubEnumTable(msg_mini_table, field), value));
  _upb_Message_SetNonExtensionField(msg, field, &value);
}

UPB_API_INLINE int64_t upb_Message_GetInt64(const upb_Message* msg,
                                            const upb_MiniTableField* field,
                                            uint64_t default_val) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_Int64);
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_8Byte);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  int64_t ret;
  _upb_Message_GetField(msg, field, &default_val, &ret);
  return ret;
}

UPB_API_INLINE bool upb_Message_SetInt64(upb_Message* msg,
                                         const upb_MiniTableField* field,
                                         int64_t value, upb_Arena* a) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_Int64);
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_8Byte);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  return _upb_Message_SetField(msg, field, &value, a);
}

UPB_API_INLINE uint64_t upb_Message_GetUInt64(const upb_Message* msg,
                                              const upb_MiniTableField* field,
                                              uint64_t default_val) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_UInt64);
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_8Byte);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  uint64_t ret;
  _upb_Message_GetField(msg, field, &default_val, &ret);
  return ret;
}

UPB_API_INLINE bool upb_Message_SetUInt64(upb_Message* msg,
                                          const upb_MiniTableField* field,
                                          uint64_t value, upb_Arena* a) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_UInt64);
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_8Byte);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  return _upb_Message_SetField(msg, field, &value, a);
}

UPB_API_INLINE float upb_Message_GetFloat(const upb_Message* msg,
                                          const upb_MiniTableField* field,
                                          float default_val) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_Float);
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_4Byte);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  float ret;
  _upb_Message_GetField(msg, field, &default_val, &ret);
  return ret;
}

UPB_API_INLINE bool upb_Message_SetFloat(upb_Message* msg,
                                         const upb_MiniTableField* field,
                                         float value, upb_Arena* a) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_Float);
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_4Byte);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  return _upb_Message_SetField(msg, field, &value, a);
}

UPB_API_INLINE double upb_Message_GetDouble(const upb_Message* msg,
                                            const upb_MiniTableField* field,
                                            double default_val) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_Double);
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_8Byte);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  double ret;
  _upb_Message_GetField(msg, field, &default_val, &ret);
  return ret;
}

UPB_API_INLINE bool upb_Message_SetDouble(upb_Message* msg,
                                          const upb_MiniTableField* field,
                                          double value, upb_Arena* a) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_Double);
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_8Byte);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  return _upb_Message_SetField(msg, field, &value, a);
}

UPB_API_INLINE upb_StringView
upb_Message_GetString(const upb_Message* msg, const upb_MiniTableField* field,
                      upb_StringView def_val) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_String ||
             upb_MiniTableField_CType(field) == kUpb_CType_Bytes);
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_StringView);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  upb_StringView ret;
  _upb_Message_GetField(msg, field, &def_val, &ret);
  return ret;
}

UPB_API_INLINE bool upb_Message_SetString(upb_Message* msg,
                                          const upb_MiniTableField* field,
                                          upb_StringView value, upb_Arena* a) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_String ||
             upb_MiniTableField_CType(field) == kUpb_CType_Bytes);
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_StringView);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  return _upb_Message_SetField(msg, field, &value, a);
}

UPB_API_INLINE const upb_Message* upb_Message_GetMessage(
    const upb_Message* msg, const upb_MiniTableField* field,
    upb_Message* default_val) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_Message);
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) ==
             UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte));
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  upb_Message* ret;
  _upb_Message_GetNonExtensionField(msg, field, &default_val, &ret);
  return ret;
}

UPB_API_INLINE void upb_Message_SetMessage(upb_Message* msg,
                                           const upb_MiniTable* mini_table,
                                           const upb_MiniTableField* field,
                                           upb_Message* sub_message) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_Message);
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) ==
             UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte));
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  UPB_ASSERT(mini_table->subs[field->UPB_PRIVATE(submsg_index)].submsg);
  _upb_Message_SetNonExtensionField(msg, field, &sub_message);
}

UPB_API_INLINE upb_Message* upb_Message_GetOrCreateMutableMessage(
    upb_Message* msg, const upb_MiniTable* mini_table,
    const upb_MiniTableField* field, upb_Arena* arena) {
  UPB_ASSERT(arena);
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_Message);
  upb_Message* sub_message = *UPB_PTR_AT(msg, field->offset, upb_Message*);
  if (!sub_message) {
    const upb_MiniTable* sub_mini_table =
        mini_table->subs[field->UPB_PRIVATE(submsg_index)].submsg;
    UPB_ASSERT(sub_mini_table);
    sub_message = _upb_Message_New(sub_mini_table, arena);
    *UPB_PTR_AT(msg, field->offset, upb_Message*) = sub_message;
    _upb_Message_SetPresence(msg, field);
  }
  return sub_message;
}

UPB_API_INLINE const upb_Array* upb_Message_GetArray(
    const upb_Message* msg, const upb_MiniTableField* field) {
  _upb_MiniTableField_CheckIsArray(field);
  upb_Array* ret;
  const upb_Array* default_val = NULL;
  _upb_Message_GetNonExtensionField(msg, field, &default_val, &ret);
  return ret;
}

UPB_API_INLINE upb_Array* upb_Message_GetMutableArray(
    upb_Message* msg, const upb_MiniTableField* field) {
  _upb_MiniTableField_CheckIsArray(field);
  return (upb_Array*)upb_Message_GetArray(msg, field);
}

UPB_API_INLINE upb_Array* upb_Message_GetOrCreateMutableArray(
    upb_Message* msg, const upb_MiniTableField* field, upb_Arena* arena) {
  UPB_ASSERT(arena);
  _upb_MiniTableField_CheckIsArray(field);
  upb_Array* array = upb_Message_GetMutableArray(msg, field);
  if (!array) {
    array = _upb_Array_New(arena, 4, _upb_MiniTable_ElementSizeLg2(field));
    // Check again due to: https://godbolt.org/z/7WfaoKG1r
    _upb_MiniTableField_CheckIsArray(field);
    _upb_Message_SetField(msg, field, &array, arena);
  }
  return array;
}

UPB_INLINE upb_Array* upb_Message_ResizeArrayUninitialized(
    upb_Message* msg, const upb_MiniTableField* field, size_t size,
    upb_Arena* arena) {
  _upb_MiniTableField_CheckIsArray(field);
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(msg, field, arena);
  if (!arr || !_upb_Array_ResizeUninitialized(arr, size, arena)) return NULL;
  return arr;
}

// TODO: remove, migrate users to upb_Message_ResizeArrayUninitialized(), which
// has the same semantics but a clearer name. Alternatively, if users want an
// initialized variant, we can also offer that.
UPB_API_INLINE void* upb_Message_ResizeArray(upb_Message* msg,
                                             const upb_MiniTableField* field,
                                             size_t size, upb_Arena* arena) {
  _upb_MiniTableField_CheckIsArray(field);
  upb_Array* arr =
      upb_Message_ResizeArrayUninitialized(msg, field, size, arena);
  return _upb_array_ptr(arr);
}

UPB_API_INLINE const upb_Map* upb_Message_GetMap(
    const upb_Message* msg, const upb_MiniTableField* field) {
  _upb_MiniTableField_CheckIsMap(field);
  upb_Map* ret;
  const upb_Map* default_val = NULL;
  _upb_Message_GetNonExtensionField(msg, field, &default_val, &ret);
  return ret;
}

UPB_API_INLINE upb_Map* upb_Message_GetOrCreateMutableMap(
    upb_Message* msg, const upb_MiniTable* map_entry_mini_table,
    const upb_MiniTableField* field, upb_Arena* arena) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_Message);
  const upb_MiniTableField* map_entry_key_field =
      &map_entry_mini_table->fields[0];
  const upb_MiniTableField* map_entry_value_field =
      &map_entry_mini_table->fields[1];
  return _upb_Message_GetOrCreateMutableMap(
      msg, field,
      _upb_Map_CTypeSize(upb_MiniTableField_CType(map_entry_key_field)),
      _upb_Map_CTypeSize(upb_MiniTableField_CType(map_entry_value_field)),
      arena);
}

// Updates a map entry given an entry message.
upb_MapInsertStatus upb_Message_InsertMapEntry(upb_Map* map,
                                               const upb_MiniTable* mini_table,
                                               const upb_MiniTableField* field,
                                               upb_Message* map_entry_message,
                                               upb_Arena* arena);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif  // UPB_MESSAGE_ACCESSORS_H_

// upb_decode: parsing into a upb_Message using a upb_MiniTable.

#ifndef UPB_WIRE_DECODE_H_
#define UPB_WIRE_DECODE_H_


// Must be last.

#ifdef __cplusplus
extern "C" {
#endif

enum {
  /* If set, strings will alias the input buffer instead of copying into the
   * arena. */
  kUpb_DecodeOption_AliasString = 1,

  /* If set, the parse will return failure if any message is missing any
   * required fields when the message data ends.  The parse will still continue,
   * and the failure will only be reported at the end.
   *
   * IMPORTANT CAVEATS:
   *
   * 1. This can throw a false positive failure if an incomplete message is seen
   *    on the wire but is later completed when the sub-message occurs again.
   *    For this reason, a second pass is required to verify a failure, to be
   *    truly robust.
   *
   * 2. This can return a false success if you are decoding into a message that
   *    already has some sub-message fields present.  If the sub-message does
   *    not occur in the binary payload, we will never visit it and discover the
   *    incomplete sub-message.  For this reason, this check is only useful for
   *    implemting ParseFromString() semantics.  For MergeFromString(), a
   *    post-parse validation step will always be necessary. */
  kUpb_DecodeOption_CheckRequired = 2,
};

UPB_INLINE uint32_t upb_DecodeOptions_MaxDepth(uint16_t depth) {
  return (uint32_t)depth << 16;
}

UPB_INLINE uint16_t upb_DecodeOptions_GetMaxDepth(uint32_t options) {
  return options >> 16;
}

// Enforce an upper bound on recursion depth.
UPB_INLINE int upb_Decode_LimitDepth(uint32_t decode_options, uint32_t limit) {
  uint32_t max_depth = upb_DecodeOptions_GetMaxDepth(decode_options);
  if (max_depth > limit) max_depth = limit;
  return upb_DecodeOptions_MaxDepth(max_depth) | (decode_options & 0xffff);
}

typedef enum {
  kUpb_DecodeStatus_Ok = 0,
  kUpb_DecodeStatus_Malformed = 1,    // Wire format was corrupt
  kUpb_DecodeStatus_OutOfMemory = 2,  // Arena alloc failed
  kUpb_DecodeStatus_BadUtf8 = 3,      // String field had bad UTF-8
  kUpb_DecodeStatus_MaxDepthExceeded =
      4,  // Exceeded upb_DecodeOptions_MaxDepth

  // kUpb_DecodeOption_CheckRequired failed (see above), but the parse otherwise
  // succeeded.
  kUpb_DecodeStatus_MissingRequired = 5,
} upb_DecodeStatus;

UPB_API upb_DecodeStatus upb_Decode(const char* buf, size_t size,
                                    upb_Message* msg, const upb_MiniTable* l,
                                    const upb_ExtensionRegistry* extreg,
                                    int options, upb_Arena* arena);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_WIRE_DECODE_H_ */

// These are the specialized field parser functions for the fast parser.
// Generated tables will refer to these by name.
//
// The function names are encoded with names like:
//
//   //  123 4
//   upb_pss_1bt();   // Parse singular string, 1 byte tag.
//
// In position 1:
//   - 'p' for parse, most function use this
//   - 'c' for copy, for when we are copying strings instead of aliasing
//
// In position 2 (cardinality):
//   - 's' for singular, with or without hasbit
//   - 'o' for oneof
//   - 'r' for non-packed repeated
//   - 'p' for packed repeated
//
// In position 3 (type):
//   - 'b1' for bool
//   - 'v4' for 4-byte varint
//   - 'v8' for 8-byte varint
//   - 'z4' for zig-zag-encoded 4-byte varint
//   - 'z8' for zig-zag-encoded 8-byte varint
//   - 'f4' for 4-byte fixed
//   - 'f8' for 8-byte fixed
//   - 'm' for sub-message
//   - 's' for string (validate UTF-8)
//   - 'b' for bytes
//
// In position 4 (tag length):
//   - '1' for one-byte tags (field numbers 1-15)
//   - '2' for two-byte tags (field numbers 16-2048)

#ifndef UPB_WIRE_DECODE_FAST_H_
#define UPB_WIRE_DECODE_FAST_H_


// Must be last.

#ifdef __cplusplus
extern "C" {
#endif

struct upb_Decoder;

// The fallback, generic parsing function that can handle any field type.
// This just uses the regular (non-fast) parser to parse a single field.
const char* _upb_FastDecoder_DecodeGeneric(struct upb_Decoder* d,
                                           const char* ptr, upb_Message* msg,
                                           intptr_t table, uint64_t hasbits,
                                           uint64_t data);

#define UPB_PARSE_PARAMS                                                    \
  struct upb_Decoder *d, const char *ptr, upb_Message *msg, intptr_t table, \
      uint64_t hasbits, uint64_t data

/* primitive fields ***********************************************************/

#define F(card, type, valbytes, tagbytes) \
  const char* upb_p##card##type##valbytes##_##tagbytes##bt(UPB_PARSE_PARAMS);

#define TYPES(card, tagbytes) \
  F(card, b, 1, tagbytes)     \
  F(card, v, 4, tagbytes)     \
  F(card, v, 8, tagbytes)     \
  F(card, z, 4, tagbytes)     \
  F(card, z, 8, tagbytes)     \
  F(card, f, 4, tagbytes)     \
  F(card, f, 8, tagbytes)

#define TAGBYTES(card) \
  TYPES(card, 1)       \
  TYPES(card, 2)

TAGBYTES(s)
TAGBYTES(o)
TAGBYTES(r)
TAGBYTES(p)

#undef F
#undef TYPES
#undef TAGBYTES

/* string fields **************************************************************/

#define F(card, tagbytes, type)                                     \
  const char* upb_p##card##type##_##tagbytes##bt(UPB_PARSE_PARAMS); \
  const char* upb_c##card##type##_##tagbytes##bt(UPB_PARSE_PARAMS);

#define UTF8(card, tagbytes) \
  F(card, tagbytes, s)       \
  F(card, tagbytes, b)

#define TAGBYTES(card) \
  UTF8(card, 1)        \
  UTF8(card, 2)

TAGBYTES(s)
TAGBYTES(o)
TAGBYTES(r)

#undef F
#undef TAGBYTES

/* sub-message fields *********************************************************/

#define F(card, tagbytes, size_ceil, ceil_arg) \
  const char* upb_p##card##m_##tagbytes##bt_max##size_ceil##b(UPB_PARSE_PARAMS);

#define SIZES(card, tagbytes) \
  F(card, tagbytes, 64, 64)   \
  F(card, tagbytes, 128, 128) \
  F(card, tagbytes, 192, 192) \
  F(card, tagbytes, 256, 256) \
  F(card, tagbytes, max, -1)

#define TAGBYTES(card) \
  SIZES(card, 1)       \
  SIZES(card, 2)

TAGBYTES(s)
TAGBYTES(o)
TAGBYTES(r)

#undef TAGBYTES
#undef SIZES
#undef F

#undef UPB_PARSE_PARAMS

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_WIRE_DECODE_FAST_H_ */

// upb_Encode: parsing from a upb_Message using a upb_MiniTable.

#ifndef UPB_WIRE_ENCODE_H_
#define UPB_WIRE_ENCODE_H_


// Must be last.

#ifdef __cplusplus
extern "C" {
#endif

enum {
  /* If set, the results of serializing will be deterministic across all
   * instances of this binary. There are no guarantees across different
   * binary builds.
   *
   * If your proto contains maps, the encoder will need to malloc()/free()
   * memory during encode. */
  kUpb_EncodeOption_Deterministic = 1,

  // When set, unknown fields are not printed.
  kUpb_EncodeOption_SkipUnknown = 2,

  // When set, the encode will fail if any required fields are missing.
  kUpb_EncodeOption_CheckRequired = 4,
};

typedef enum {
  kUpb_EncodeStatus_Ok = 0,
  kUpb_EncodeStatus_OutOfMemory = 1,  // Arena alloc failed
  kUpb_EncodeStatus_MaxDepthExceeded = 2,

  // kUpb_EncodeOption_CheckRequired failed but the parse otherwise succeeded.
  kUpb_EncodeStatus_MissingRequired = 3,
} upb_EncodeStatus;

UPB_INLINE uint32_t upb_EncodeOptions_MaxDepth(uint16_t depth) {
  return (uint32_t)depth << 16;
}

UPB_INLINE uint16_t upb_EncodeOptions_GetMaxDepth(uint32_t options) {
  return options >> 16;
}

// Enforce an upper bound on recursion depth.
UPB_INLINE int upb_Encode_LimitDepth(uint32_t encode_options, uint32_t limit) {
  uint32_t max_depth = upb_EncodeOptions_GetMaxDepth(encode_options);
  if (max_depth > limit) max_depth = limit;
  return upb_EncodeOptions_MaxDepth(max_depth) | (encode_options & 0xffff);
}

upb_EncodeStatus upb_Encode(const void* msg, const upb_MiniTable* l,
                            int options, upb_Arena* arena, char** buf,
                            size_t* size);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_WIRE_ENCODE_H_ */

// Must be last. 

#ifdef __cplusplus
extern "C" {
#endif

typedef struct google_protobuf_FileDescriptorSet google_protobuf_FileDescriptorSet;
typedef struct google_protobuf_FileDescriptorProto google_protobuf_FileDescriptorProto;
typedef struct google_protobuf_DescriptorProto google_protobuf_DescriptorProto;
typedef struct google_protobuf_DescriptorProto_ExtensionRange google_protobuf_DescriptorProto_ExtensionRange;
typedef struct google_protobuf_DescriptorProto_ReservedRange google_protobuf_DescriptorProto_ReservedRange;
typedef struct google_protobuf_ExtensionRangeOptions google_protobuf_ExtensionRangeOptions;
typedef struct google_protobuf_ExtensionRangeOptions_Declaration google_protobuf_ExtensionRangeOptions_Declaration;
typedef struct google_protobuf_FieldDescriptorProto google_protobuf_FieldDescriptorProto;
typedef struct google_protobuf_OneofDescriptorProto google_protobuf_OneofDescriptorProto;
typedef struct google_protobuf_EnumDescriptorProto google_protobuf_EnumDescriptorProto;
typedef struct google_protobuf_EnumDescriptorProto_EnumReservedRange google_protobuf_EnumDescriptorProto_EnumReservedRange;
typedef struct google_protobuf_EnumValueDescriptorProto google_protobuf_EnumValueDescriptorProto;
typedef struct google_protobuf_ServiceDescriptorProto google_protobuf_ServiceDescriptorProto;
typedef struct google_protobuf_MethodDescriptorProto google_protobuf_MethodDescriptorProto;
typedef struct google_protobuf_FileOptions google_protobuf_FileOptions;
typedef struct google_protobuf_MessageOptions google_protobuf_MessageOptions;
typedef struct google_protobuf_FieldOptions google_protobuf_FieldOptions;
typedef struct google_protobuf_OneofOptions google_protobuf_OneofOptions;
typedef struct google_protobuf_EnumOptions google_protobuf_EnumOptions;
typedef struct google_protobuf_EnumValueOptions google_protobuf_EnumValueOptions;
typedef struct google_protobuf_ServiceOptions google_protobuf_ServiceOptions;
typedef struct google_protobuf_MethodOptions google_protobuf_MethodOptions;
typedef struct google_protobuf_UninterpretedOption google_protobuf_UninterpretedOption;
typedef struct google_protobuf_UninterpretedOption_NamePart google_protobuf_UninterpretedOption_NamePart;
typedef struct google_protobuf_SourceCodeInfo google_protobuf_SourceCodeInfo;
typedef struct google_protobuf_SourceCodeInfo_Location google_protobuf_SourceCodeInfo_Location;
typedef struct google_protobuf_GeneratedCodeInfo google_protobuf_GeneratedCodeInfo;
typedef struct google_protobuf_GeneratedCodeInfo_Annotation google_protobuf_GeneratedCodeInfo_Annotation;
extern const upb_MiniTable google_protobuf_FileDescriptorSet_msg_init;
extern const upb_MiniTable google_protobuf_FileDescriptorProto_msg_init;
extern const upb_MiniTable google_protobuf_DescriptorProto_msg_init;
extern const upb_MiniTable google_protobuf_DescriptorProto_ExtensionRange_msg_init;
extern const upb_MiniTable google_protobuf_DescriptorProto_ReservedRange_msg_init;
extern const upb_MiniTable google_protobuf_ExtensionRangeOptions_msg_init;
extern const upb_MiniTable google_protobuf_ExtensionRangeOptions_Declaration_msg_init;
extern const upb_MiniTable google_protobuf_FieldDescriptorProto_msg_init;
extern const upb_MiniTable google_protobuf_OneofDescriptorProto_msg_init;
extern const upb_MiniTable google_protobuf_EnumDescriptorProto_msg_init;
extern const upb_MiniTable google_protobuf_EnumDescriptorProto_EnumReservedRange_msg_init;
extern const upb_MiniTable google_protobuf_EnumValueDescriptorProto_msg_init;
extern const upb_MiniTable google_protobuf_ServiceDescriptorProto_msg_init;
extern const upb_MiniTable google_protobuf_MethodDescriptorProto_msg_init;
extern const upb_MiniTable google_protobuf_FileOptions_msg_init;
extern const upb_MiniTable google_protobuf_MessageOptions_msg_init;
extern const upb_MiniTable google_protobuf_FieldOptions_msg_init;
extern const upb_MiniTable google_protobuf_OneofOptions_msg_init;
extern const upb_MiniTable google_protobuf_EnumOptions_msg_init;
extern const upb_MiniTable google_protobuf_EnumValueOptions_msg_init;
extern const upb_MiniTable google_protobuf_ServiceOptions_msg_init;
extern const upb_MiniTable google_protobuf_MethodOptions_msg_init;
extern const upb_MiniTable google_protobuf_UninterpretedOption_msg_init;
extern const upb_MiniTable google_protobuf_UninterpretedOption_NamePart_msg_init;
extern const upb_MiniTable google_protobuf_SourceCodeInfo_msg_init;
extern const upb_MiniTable google_protobuf_SourceCodeInfo_Location_msg_init;
extern const upb_MiniTable google_protobuf_GeneratedCodeInfo_msg_init;
extern const upb_MiniTable google_protobuf_GeneratedCodeInfo_Annotation_msg_init;

typedef enum {
  google_protobuf_ExtensionRangeOptions_DECLARATION = 0,
  google_protobuf_ExtensionRangeOptions_UNVERIFIED = 1
} google_protobuf_ExtensionRangeOptions_VerificationState;

typedef enum {
  google_protobuf_FieldDescriptorProto_LABEL_OPTIONAL = 1,
  google_protobuf_FieldDescriptorProto_LABEL_REQUIRED = 2,
  google_protobuf_FieldDescriptorProto_LABEL_REPEATED = 3
} google_protobuf_FieldDescriptorProto_Label;

typedef enum {
  google_protobuf_FieldDescriptorProto_TYPE_DOUBLE = 1,
  google_protobuf_FieldDescriptorProto_TYPE_FLOAT = 2,
  google_protobuf_FieldDescriptorProto_TYPE_INT64 = 3,
  google_protobuf_FieldDescriptorProto_TYPE_UINT64 = 4,
  google_protobuf_FieldDescriptorProto_TYPE_INT32 = 5,
  google_protobuf_FieldDescriptorProto_TYPE_FIXED64 = 6,
  google_protobuf_FieldDescriptorProto_TYPE_FIXED32 = 7,
  google_protobuf_FieldDescriptorProto_TYPE_BOOL = 8,
  google_protobuf_FieldDescriptorProto_TYPE_STRING = 9,
  google_protobuf_FieldDescriptorProto_TYPE_GROUP = 10,
  google_protobuf_FieldDescriptorProto_TYPE_MESSAGE = 11,
  google_protobuf_FieldDescriptorProto_TYPE_BYTES = 12,
  google_protobuf_FieldDescriptorProto_TYPE_UINT32 = 13,
  google_protobuf_FieldDescriptorProto_TYPE_ENUM = 14,
  google_protobuf_FieldDescriptorProto_TYPE_SFIXED32 = 15,
  google_protobuf_FieldDescriptorProto_TYPE_SFIXED64 = 16,
  google_protobuf_FieldDescriptorProto_TYPE_SINT32 = 17,
  google_protobuf_FieldDescriptorProto_TYPE_SINT64 = 18
} google_protobuf_FieldDescriptorProto_Type;

typedef enum {
  google_protobuf_FieldOptions_STRING = 0,
  google_protobuf_FieldOptions_CORD = 1,
  google_protobuf_FieldOptions_STRING_PIECE = 2
} google_protobuf_FieldOptions_CType;

typedef enum {
  google_protobuf_FieldOptions_JS_NORMAL = 0,
  google_protobuf_FieldOptions_JS_STRING = 1,
  google_protobuf_FieldOptions_JS_NUMBER = 2
} google_protobuf_FieldOptions_JSType;

typedef enum {
  google_protobuf_FieldOptions_RETENTION_UNKNOWN = 0,
  google_protobuf_FieldOptions_RETENTION_RUNTIME = 1,
  google_protobuf_FieldOptions_RETENTION_SOURCE = 2
} google_protobuf_FieldOptions_OptionRetention;

typedef enum {
  google_protobuf_FieldOptions_TARGET_TYPE_UNKNOWN = 0,
  google_protobuf_FieldOptions_TARGET_TYPE_FILE = 1,
  google_protobuf_FieldOptions_TARGET_TYPE_EXTENSION_RANGE = 2,
  google_protobuf_FieldOptions_TARGET_TYPE_MESSAGE = 3,
  google_protobuf_FieldOptions_TARGET_TYPE_FIELD = 4,
  google_protobuf_FieldOptions_TARGET_TYPE_ONEOF = 5,
  google_protobuf_FieldOptions_TARGET_TYPE_ENUM = 6,
  google_protobuf_FieldOptions_TARGET_TYPE_ENUM_ENTRY = 7,
  google_protobuf_FieldOptions_TARGET_TYPE_SERVICE = 8,
  google_protobuf_FieldOptions_TARGET_TYPE_METHOD = 9
} google_protobuf_FieldOptions_OptionTargetType;

typedef enum {
  google_protobuf_FileOptions_SPEED = 1,
  google_protobuf_FileOptions_CODE_SIZE = 2,
  google_protobuf_FileOptions_LITE_RUNTIME = 3
} google_protobuf_FileOptions_OptimizeMode;

typedef enum {
  google_protobuf_GeneratedCodeInfo_Annotation_NONE = 0,
  google_protobuf_GeneratedCodeInfo_Annotation_SET = 1,
  google_protobuf_GeneratedCodeInfo_Annotation_ALIAS = 2
} google_protobuf_GeneratedCodeInfo_Annotation_Semantic;

typedef enum {
  google_protobuf_MethodOptions_IDEMPOTENCY_UNKNOWN = 0,
  google_protobuf_MethodOptions_NO_SIDE_EFFECTS = 1,
  google_protobuf_MethodOptions_IDEMPOTENT = 2
} google_protobuf_MethodOptions_IdempotencyLevel;


extern const upb_MiniTableEnum google_protobuf_ExtensionRangeOptions_VerificationState_enum_init;
extern const upb_MiniTableEnum google_protobuf_FieldDescriptorProto_Label_enum_init;
extern const upb_MiniTableEnum google_protobuf_FieldDescriptorProto_Type_enum_init;
extern const upb_MiniTableEnum google_protobuf_FieldOptions_CType_enum_init;
extern const upb_MiniTableEnum google_protobuf_FieldOptions_JSType_enum_init;
extern const upb_MiniTableEnum google_protobuf_FieldOptions_OptionRetention_enum_init;
extern const upb_MiniTableEnum google_protobuf_FieldOptions_OptionTargetType_enum_init;
extern const upb_MiniTableEnum google_protobuf_FileOptions_OptimizeMode_enum_init;
extern const upb_MiniTableEnum google_protobuf_GeneratedCodeInfo_Annotation_Semantic_enum_init;
extern const upb_MiniTableEnum google_protobuf_MethodOptions_IdempotencyLevel_enum_init;

/* google.protobuf.FileDescriptorSet */

UPB_INLINE google_protobuf_FileDescriptorSet* google_protobuf_FileDescriptorSet_new(upb_Arena* arena) {
  return (google_protobuf_FileDescriptorSet*)_upb_Message_New(&google_protobuf_FileDescriptorSet_msg_init, arena);
}
UPB_INLINE google_protobuf_FileDescriptorSet* google_protobuf_FileDescriptorSet_parse(const char* buf, size_t size, upb_Arena* arena) {
  google_protobuf_FileDescriptorSet* ret = google_protobuf_FileDescriptorSet_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_FileDescriptorSet_msg_init, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE google_protobuf_FileDescriptorSet* google_protobuf_FileDescriptorSet_parse_ex(const char* buf, size_t size,
                           const upb_ExtensionRegistry* extreg,
                           int options, upb_Arena* arena) {
  google_protobuf_FileDescriptorSet* ret = google_protobuf_FileDescriptorSet_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_FileDescriptorSet_msg_init, extreg, options, arena) !=
      kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE char* google_protobuf_FileDescriptorSet_serialize(const google_protobuf_FileDescriptorSet* msg, upb_Arena* arena, size_t* len) {
  char* ptr;
  (void)upb_Encode(msg, &google_protobuf_FileDescriptorSet_msg_init, 0, arena, &ptr, len);
  return ptr;
}
UPB_INLINE char* google_protobuf_FileDescriptorSet_serialize_ex(const google_protobuf_FileDescriptorSet* msg, int options,
                                 upb_Arena* arena, size_t* len) {
  char* ptr;
  (void)upb_Encode(msg, &google_protobuf_FileDescriptorSet_msg_init, options, arena, &ptr, len);
  return ptr;
}
UPB_INLINE void google_protobuf_FileDescriptorSet_clear_file(google_protobuf_FileDescriptorSet* msg) {
  const upb_MiniTableField field = {1, 0, 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE const google_protobuf_FileDescriptorProto* const* google_protobuf_FileDescriptorSet_file(const google_protobuf_FileDescriptorSet* msg, size_t* size) {
  const upb_MiniTableField field = {1, 0, 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (const google_protobuf_FileDescriptorProto* const*)_upb_array_constptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE const upb_Array* _google_protobuf_FileDescriptorSet_file_upb_array(const google_protobuf_FileDescriptorSet* msg, size_t* size) {
  const upb_MiniTableField field = {1, 0, 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE upb_Array* _google_protobuf_FileDescriptorSet_file_mutable_upb_array(const google_protobuf_FileDescriptorSet* msg, size_t* size, upb_Arena* arena) {
  const upb_MiniTableField field = {1, 0, 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(
      (upb_Message*)msg, &field, arena);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE bool google_protobuf_FileDescriptorSet_has_file(const google_protobuf_FileDescriptorSet* msg) {
  size_t size;
  google_protobuf_FileDescriptorSet_file(msg, &size);
  return size != 0;
}

UPB_INLINE google_protobuf_FileDescriptorProto** google_protobuf_FileDescriptorSet_mutable_file(google_protobuf_FileDescriptorSet* msg, size_t* size) {
  upb_MiniTableField field = {1, 0, 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetMutableArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (google_protobuf_FileDescriptorProto**)_upb_array_ptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE google_protobuf_FileDescriptorProto** google_protobuf_FileDescriptorSet_resize_file(google_protobuf_FileDescriptorSet* msg, size_t size, upb_Arena* arena) {
  upb_MiniTableField field = {1, 0, 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  return (google_protobuf_FileDescriptorProto**)upb_Message_ResizeArray(msg, &field, size, arena);
}
UPB_INLINE struct google_protobuf_FileDescriptorProto* google_protobuf_FileDescriptorSet_add_file(google_protobuf_FileDescriptorSet* msg, upb_Arena* arena) {
  upb_MiniTableField field = {1, 0, 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(msg, &field, arena);
  if (!arr || !_upb_Array_ResizeUninitialized(arr, arr->size + 1, arena)) {
    return NULL;
  }
  struct google_protobuf_FileDescriptorProto* sub = (struct google_protobuf_FileDescriptorProto*)_upb_Message_New(&google_protobuf_FileDescriptorProto_msg_init, arena);
  if (!arr || !sub) return NULL;
  _upb_Array_Set(arr, arr->size - 1, &sub, sizeof(sub));
  return sub;
}

/* google.protobuf.FileDescriptorProto */

UPB_INLINE google_protobuf_FileDescriptorProto* google_protobuf_FileDescriptorProto_new(upb_Arena* arena) {
  return (google_protobuf_FileDescriptorProto*)_upb_Message_New(&google_protobuf_FileDescriptorProto_msg_init, arena);
}
UPB_INLINE google_protobuf_FileDescriptorProto* google_protobuf_FileDescriptorProto_parse(const char* buf, size_t size, upb_Arena* arena) {
  google_protobuf_FileDescriptorProto* ret = google_protobuf_FileDescriptorProto_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_FileDescriptorProto_msg_init, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE google_protobuf_FileDescriptorProto* google_protobuf_FileDescriptorProto_parse_ex(const char* buf, size_t size,
                           const upb_ExtensionRegistry* extreg,
                           int options, upb_Arena* arena) {
  google_protobuf_FileDescriptorProto* ret = google_protobuf_FileDescriptorProto_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_FileDescriptorProto_msg_init, extreg, options, arena) !=
      kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE char* google_protobuf_FileDescriptorProto_serialize(const google_protobuf_FileDescriptorProto* msg, upb_Arena* arena, size_t* len) {
  char* ptr;
  (void)upb_Encode(msg, &google_protobuf_FileDescriptorProto_msg_init, 0, arena, &ptr, len);
  return ptr;
}
UPB_INLINE char* google_protobuf_FileDescriptorProto_serialize_ex(const google_protobuf_FileDescriptorProto* msg, int options,
                                 upb_Arena* arena, size_t* len) {
  char* ptr;
  (void)upb_Encode(msg, &google_protobuf_FileDescriptorProto_msg_init, options, arena, &ptr, len);
  return ptr;
}
UPB_INLINE void google_protobuf_FileDescriptorProto_clear_name(google_protobuf_FileDescriptorProto* msg) {
  const upb_MiniTableField field = {1, UPB_SIZE(40, 8), 1, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE upb_StringView google_protobuf_FileDescriptorProto_name(const google_protobuf_FileDescriptorProto* msg) {
  upb_StringView default_val = upb_StringView_FromString("");
  upb_StringView ret;
  const upb_MiniTableField field = {1, UPB_SIZE(40, 8), 1, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_FileDescriptorProto_has_name(const google_protobuf_FileDescriptorProto* msg) {
  const upb_MiniTableField field = {1, UPB_SIZE(40, 8), 1, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_FileDescriptorProto_clear_package(google_protobuf_FileDescriptorProto* msg) {
  const upb_MiniTableField field = {2, UPB_SIZE(48, 24), 2, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE upb_StringView google_protobuf_FileDescriptorProto_package(const google_protobuf_FileDescriptorProto* msg) {
  upb_StringView default_val = upb_StringView_FromString("");
  upb_StringView ret;
  const upb_MiniTableField field = {2, UPB_SIZE(48, 24), 2, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_FileDescriptorProto_has_package(const google_protobuf_FileDescriptorProto* msg) {
  const upb_MiniTableField field = {2, UPB_SIZE(48, 24), 2, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_FileDescriptorProto_clear_dependency(google_protobuf_FileDescriptorProto* msg) {
  const upb_MiniTableField field = {3, UPB_SIZE(4, 40), 0, kUpb_NoSub, 12, kUpb_FieldMode_Array | kUpb_LabelFlags_IsAlternate | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE upb_StringView const* google_protobuf_FileDescriptorProto_dependency(const google_protobuf_FileDescriptorProto* msg, size_t* size) {
  const upb_MiniTableField field = {3, UPB_SIZE(4, 40), 0, kUpb_NoSub, 12, kUpb_FieldMode_Array | kUpb_LabelFlags_IsAlternate | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (upb_StringView const*)_upb_array_constptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE const upb_Array* _google_protobuf_FileDescriptorProto_dependency_upb_array(const google_protobuf_FileDescriptorProto* msg, size_t* size) {
  const upb_MiniTableField field = {3, UPB_SIZE(4, 40), 0, kUpb_NoSub, 12, kUpb_FieldMode_Array | kUpb_LabelFlags_IsAlternate | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE upb_Array* _google_protobuf_FileDescriptorProto_dependency_mutable_upb_array(const google_protobuf_FileDescriptorProto* msg, size_t* size, upb_Arena* arena) {
  const upb_MiniTableField field = {3, UPB_SIZE(4, 40), 0, kUpb_NoSub, 12, kUpb_FieldMode_Array | kUpb_LabelFlags_IsAlternate | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(
      (upb_Message*)msg, &field, arena);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE bool google_protobuf_FileDescriptorProto_has_dependency(const google_protobuf_FileDescriptorProto* msg) {
  size_t size;
  google_protobuf_FileDescriptorProto_dependency(msg, &size);
  return size != 0;
}
UPB_INLINE void google_protobuf_FileDescriptorProto_clear_message_type(google_protobuf_FileDescriptorProto* msg) {
  const upb_MiniTableField field = {4, UPB_SIZE(8, 48), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE const google_protobuf_DescriptorProto* const* google_protobuf_FileDescriptorProto_message_type(const google_protobuf_FileDescriptorProto* msg, size_t* size) {
  const upb_MiniTableField field = {4, UPB_SIZE(8, 48), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (const google_protobuf_DescriptorProto* const*)_upb_array_constptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE const upb_Array* _google_protobuf_FileDescriptorProto_message_type_upb_array(const google_protobuf_FileDescriptorProto* msg, size_t* size) {
  const upb_MiniTableField field = {4, UPB_SIZE(8, 48), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE upb_Array* _google_protobuf_FileDescriptorProto_message_type_mutable_upb_array(const google_protobuf_FileDescriptorProto* msg, size_t* size, upb_Arena* arena) {
  const upb_MiniTableField field = {4, UPB_SIZE(8, 48), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(
      (upb_Message*)msg, &field, arena);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE bool google_protobuf_FileDescriptorProto_has_message_type(const google_protobuf_FileDescriptorProto* msg) {
  size_t size;
  google_protobuf_FileDescriptorProto_message_type(msg, &size);
  return size != 0;
}
UPB_INLINE void google_protobuf_FileDescriptorProto_clear_enum_type(google_protobuf_FileDescriptorProto* msg) {
  const upb_MiniTableField field = {5, UPB_SIZE(12, 56), 0, 1, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE const google_protobuf_EnumDescriptorProto* const* google_protobuf_FileDescriptorProto_enum_type(const google_protobuf_FileDescriptorProto* msg, size_t* size) {
  const upb_MiniTableField field = {5, UPB_SIZE(12, 56), 0, 1, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (const google_protobuf_EnumDescriptorProto* const*)_upb_array_constptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE const upb_Array* _google_protobuf_FileDescriptorProto_enum_type_upb_array(const google_protobuf_FileDescriptorProto* msg, size_t* size) {
  const upb_MiniTableField field = {5, UPB_SIZE(12, 56), 0, 1, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE upb_Array* _google_protobuf_FileDescriptorProto_enum_type_mutable_upb_array(const google_protobuf_FileDescriptorProto* msg, size_t* size, upb_Arena* arena) {
  const upb_MiniTableField field = {5, UPB_SIZE(12, 56), 0, 1, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(
      (upb_Message*)msg, &field, arena);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE bool google_protobuf_FileDescriptorProto_has_enum_type(const google_protobuf_FileDescriptorProto* msg) {
  size_t size;
  google_protobuf_FileDescriptorProto_enum_type(msg, &size);
  return size != 0;
}
UPB_INLINE void google_protobuf_FileDescriptorProto_clear_service(google_protobuf_FileDescriptorProto* msg) {
  const upb_MiniTableField field = {6, UPB_SIZE(16, 64), 0, 2, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE const google_protobuf_ServiceDescriptorProto* const* google_protobuf_FileDescriptorProto_service(const google_protobuf_FileDescriptorProto* msg, size_t* size) {
  const upb_MiniTableField field = {6, UPB_SIZE(16, 64), 0, 2, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (const google_protobuf_ServiceDescriptorProto* const*)_upb_array_constptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE const upb_Array* _google_protobuf_FileDescriptorProto_service_upb_array(const google_protobuf_FileDescriptorProto* msg, size_t* size) {
  const upb_MiniTableField field = {6, UPB_SIZE(16, 64), 0, 2, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE upb_Array* _google_protobuf_FileDescriptorProto_service_mutable_upb_array(const google_protobuf_FileDescriptorProto* msg, size_t* size, upb_Arena* arena) {
  const upb_MiniTableField field = {6, UPB_SIZE(16, 64), 0, 2, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(
      (upb_Message*)msg, &field, arena);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE bool google_protobuf_FileDescriptorProto_has_service(const google_protobuf_FileDescriptorProto* msg) {
  size_t size;
  google_protobuf_FileDescriptorProto_service(msg, &size);
  return size != 0;
}
UPB_INLINE void google_protobuf_FileDescriptorProto_clear_extension(google_protobuf_FileDescriptorProto* msg) {
  const upb_MiniTableField field = {7, UPB_SIZE(20, 72), 0, 3, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE const google_protobuf_FieldDescriptorProto* const* google_protobuf_FileDescriptorProto_extension(const google_protobuf_FileDescriptorProto* msg, size_t* size) {
  const upb_MiniTableField field = {7, UPB_SIZE(20, 72), 0, 3, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (const google_protobuf_FieldDescriptorProto* const*)_upb_array_constptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE const upb_Array* _google_protobuf_FileDescriptorProto_extension_upb_array(const google_protobuf_FileDescriptorProto* msg, size_t* size) {
  const upb_MiniTableField field = {7, UPB_SIZE(20, 72), 0, 3, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE upb_Array* _google_protobuf_FileDescriptorProto_extension_mutable_upb_array(const google_protobuf_FileDescriptorProto* msg, size_t* size, upb_Arena* arena) {
  const upb_MiniTableField field = {7, UPB_SIZE(20, 72), 0, 3, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(
      (upb_Message*)msg, &field, arena);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE bool google_protobuf_FileDescriptorProto_has_extension(const google_protobuf_FileDescriptorProto* msg) {
  size_t size;
  google_protobuf_FileDescriptorProto_extension(msg, &size);
  return size != 0;
}
UPB_INLINE void google_protobuf_FileDescriptorProto_clear_options(google_protobuf_FileDescriptorProto* msg) {
  const upb_MiniTableField field = {8, UPB_SIZE(24, 80), 3, 4, 11, kUpb_FieldMode_Scalar | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE const google_protobuf_FileOptions* google_protobuf_FileDescriptorProto_options(const google_protobuf_FileDescriptorProto* msg) {
  const google_protobuf_FileOptions* default_val = NULL;
  const google_protobuf_FileOptions* ret;
  const upb_MiniTableField field = {8, UPB_SIZE(24, 80), 3, 4, 11, kUpb_FieldMode_Scalar | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_FileDescriptorProto_has_options(const google_protobuf_FileDescriptorProto* msg) {
  const upb_MiniTableField field = {8, UPB_SIZE(24, 80), 3, 4, 11, kUpb_FieldMode_Scalar | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_FileDescriptorProto_clear_source_code_info(google_protobuf_FileDescriptorProto* msg) {
  const upb_MiniTableField field = {9, UPB_SIZE(28, 88), 4, 5, 11, kUpb_FieldMode_Scalar | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE const google_protobuf_SourceCodeInfo* google_protobuf_FileDescriptorProto_source_code_info(const google_protobuf_FileDescriptorProto* msg) {
  const google_protobuf_SourceCodeInfo* default_val = NULL;
  const google_protobuf_SourceCodeInfo* ret;
  const upb_MiniTableField field = {9, UPB_SIZE(28, 88), 4, 5, 11, kUpb_FieldMode_Scalar | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_FileDescriptorProto_has_source_code_info(const google_protobuf_FileDescriptorProto* msg) {
  const upb_MiniTableField field = {9, UPB_SIZE(28, 88), 4, 5, 11, kUpb_FieldMode_Scalar | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_FileDescriptorProto_clear_public_dependency(google_protobuf_FileDescriptorProto* msg) {
  const upb_MiniTableField field = {10, UPB_SIZE(32, 96), 0, kUpb_NoSub, 5, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE int32_t const* google_protobuf_FileDescriptorProto_public_dependency(const google_protobuf_FileDescriptorProto* msg, size_t* size) {
  const upb_MiniTableField field = {10, UPB_SIZE(32, 96), 0, kUpb_NoSub, 5, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (int32_t const*)_upb_array_constptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE const upb_Array* _google_protobuf_FileDescriptorProto_public_dependency_upb_array(const google_protobuf_FileDescriptorProto* msg, size_t* size) {
  const upb_MiniTableField field = {10, UPB_SIZE(32, 96), 0, kUpb_NoSub, 5, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE upb_Array* _google_protobuf_FileDescriptorProto_public_dependency_mutable_upb_array(const google_protobuf_FileDescriptorProto* msg, size_t* size, upb_Arena* arena) {
  const upb_MiniTableField field = {10, UPB_SIZE(32, 96), 0, kUpb_NoSub, 5, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(
      (upb_Message*)msg, &field, arena);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE bool google_protobuf_FileDescriptorProto_has_public_dependency(const google_protobuf_FileDescriptorProto* msg) {
  size_t size;
  google_protobuf_FileDescriptorProto_public_dependency(msg, &size);
  return size != 0;
}
UPB_INLINE void google_protobuf_FileDescriptorProto_clear_weak_dependency(google_protobuf_FileDescriptorProto* msg) {
  const upb_MiniTableField field = {11, UPB_SIZE(36, 104), 0, kUpb_NoSub, 5, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE int32_t const* google_protobuf_FileDescriptorProto_weak_dependency(const google_protobuf_FileDescriptorProto* msg, size_t* size) {
  const upb_MiniTableField field = {11, UPB_SIZE(36, 104), 0, kUpb_NoSub, 5, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (int32_t const*)_upb_array_constptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE const upb_Array* _google_protobuf_FileDescriptorProto_weak_dependency_upb_array(const google_protobuf_FileDescriptorProto* msg, size_t* size) {
  const upb_MiniTableField field = {11, UPB_SIZE(36, 104), 0, kUpb_NoSub, 5, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE upb_Array* _google_protobuf_FileDescriptorProto_weak_dependency_mutable_upb_array(const google_protobuf_FileDescriptorProto* msg, size_t* size, upb_Arena* arena) {
  const upb_MiniTableField field = {11, UPB_SIZE(36, 104), 0, kUpb_NoSub, 5, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(
      (upb_Message*)msg, &field, arena);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE bool google_protobuf_FileDescriptorProto_has_weak_dependency(const google_protobuf_FileDescriptorProto* msg) {
  size_t size;
  google_protobuf_FileDescriptorProto_weak_dependency(msg, &size);
  return size != 0;
}
UPB_INLINE void google_protobuf_FileDescriptorProto_clear_syntax(google_protobuf_FileDescriptorProto* msg) {
  const upb_MiniTableField field = {12, UPB_SIZE(56, 112), 5, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE upb_StringView google_protobuf_FileDescriptorProto_syntax(const google_protobuf_FileDescriptorProto* msg) {
  upb_StringView default_val = upb_StringView_FromString("");
  upb_StringView ret;
  const upb_MiniTableField field = {12, UPB_SIZE(56, 112), 5, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_FileDescriptorProto_has_syntax(const google_protobuf_FileDescriptorProto* msg) {
  const upb_MiniTableField field = {12, UPB_SIZE(56, 112), 5, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_FileDescriptorProto_clear_edition(google_protobuf_FileDescriptorProto* msg) {
  const upb_MiniTableField field = {13, UPB_SIZE(64, 128), 6, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE upb_StringView google_protobuf_FileDescriptorProto_edition(const google_protobuf_FileDescriptorProto* msg) {
  upb_StringView default_val = upb_StringView_FromString("");
  upb_StringView ret;
  const upb_MiniTableField field = {13, UPB_SIZE(64, 128), 6, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_FileDescriptorProto_has_edition(const google_protobuf_FileDescriptorProto* msg) {
  const upb_MiniTableField field = {13, UPB_SIZE(64, 128), 6, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}

UPB_INLINE void google_protobuf_FileDescriptorProto_set_name(google_protobuf_FileDescriptorProto *msg, upb_StringView value) {
  const upb_MiniTableField field = {1, UPB_SIZE(40, 8), 1, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_FileDescriptorProto_set_package(google_protobuf_FileDescriptorProto *msg, upb_StringView value) {
  const upb_MiniTableField field = {2, UPB_SIZE(48, 24), 2, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE upb_StringView* google_protobuf_FileDescriptorProto_mutable_dependency(google_protobuf_FileDescriptorProto* msg, size_t* size) {
  upb_MiniTableField field = {3, UPB_SIZE(4, 40), 0, kUpb_NoSub, 12, kUpb_FieldMode_Array | kUpb_LabelFlags_IsAlternate | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetMutableArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (upb_StringView*)_upb_array_ptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE upb_StringView* google_protobuf_FileDescriptorProto_resize_dependency(google_protobuf_FileDescriptorProto* msg, size_t size, upb_Arena* arena) {
  upb_MiniTableField field = {3, UPB_SIZE(4, 40), 0, kUpb_NoSub, 12, kUpb_FieldMode_Array | kUpb_LabelFlags_IsAlternate | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  return (upb_StringView*)upb_Message_ResizeArray(msg, &field, size, arena);
}
UPB_INLINE bool google_protobuf_FileDescriptorProto_add_dependency(google_protobuf_FileDescriptorProto* msg, upb_StringView val, upb_Arena* arena) {
  upb_MiniTableField field = {3, UPB_SIZE(4, 40), 0, kUpb_NoSub, 12, kUpb_FieldMode_Array | kUpb_LabelFlags_IsAlternate | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(msg, &field, arena);
  if (!arr || !_upb_Array_ResizeUninitialized(arr, arr->size + 1, arena)) {
    return false;
  }
  _upb_Array_Set(arr, arr->size - 1, &val, sizeof(val));
  return true;
}
UPB_INLINE google_protobuf_DescriptorProto** google_protobuf_FileDescriptorProto_mutable_message_type(google_protobuf_FileDescriptorProto* msg, size_t* size) {
  upb_MiniTableField field = {4, UPB_SIZE(8, 48), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetMutableArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (google_protobuf_DescriptorProto**)_upb_array_ptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE google_protobuf_DescriptorProto** google_protobuf_FileDescriptorProto_resize_message_type(google_protobuf_FileDescriptorProto* msg, size_t size, upb_Arena* arena) {
  upb_MiniTableField field = {4, UPB_SIZE(8, 48), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  return (google_protobuf_DescriptorProto**)upb_Message_ResizeArray(msg, &field, size, arena);
}
UPB_INLINE struct google_protobuf_DescriptorProto* google_protobuf_FileDescriptorProto_add_message_type(google_protobuf_FileDescriptorProto* msg, upb_Arena* arena) {
  upb_MiniTableField field = {4, UPB_SIZE(8, 48), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(msg, &field, arena);
  if (!arr || !_upb_Array_ResizeUninitialized(arr, arr->size + 1, arena)) {
    return NULL;
  }
  struct google_protobuf_DescriptorProto* sub = (struct google_protobuf_DescriptorProto*)_upb_Message_New(&google_protobuf_DescriptorProto_msg_init, arena);
  if (!arr || !sub) return NULL;
  _upb_Array_Set(arr, arr->size - 1, &sub, sizeof(sub));
  return sub;
}
UPB_INLINE google_protobuf_EnumDescriptorProto** google_protobuf_FileDescriptorProto_mutable_enum_type(google_protobuf_FileDescriptorProto* msg, size_t* size) {
  upb_MiniTableField field = {5, UPB_SIZE(12, 56), 0, 1, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetMutableArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (google_protobuf_EnumDescriptorProto**)_upb_array_ptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE google_protobuf_EnumDescriptorProto** google_protobuf_FileDescriptorProto_resize_enum_type(google_protobuf_FileDescriptorProto* msg, size_t size, upb_Arena* arena) {
  upb_MiniTableField field = {5, UPB_SIZE(12, 56), 0, 1, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  return (google_protobuf_EnumDescriptorProto**)upb_Message_ResizeArray(msg, &field, size, arena);
}
UPB_INLINE struct google_protobuf_EnumDescriptorProto* google_protobuf_FileDescriptorProto_add_enum_type(google_protobuf_FileDescriptorProto* msg, upb_Arena* arena) {
  upb_MiniTableField field = {5, UPB_SIZE(12, 56), 0, 1, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(msg, &field, arena);
  if (!arr || !_upb_Array_ResizeUninitialized(arr, arr->size + 1, arena)) {
    return NULL;
  }
  struct google_protobuf_EnumDescriptorProto* sub = (struct google_protobuf_EnumDescriptorProto*)_upb_Message_New(&google_protobuf_EnumDescriptorProto_msg_init, arena);
  if (!arr || !sub) return NULL;
  _upb_Array_Set(arr, arr->size - 1, &sub, sizeof(sub));
  return sub;
}
UPB_INLINE google_protobuf_ServiceDescriptorProto** google_protobuf_FileDescriptorProto_mutable_service(google_protobuf_FileDescriptorProto* msg, size_t* size) {
  upb_MiniTableField field = {6, UPB_SIZE(16, 64), 0, 2, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetMutableArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (google_protobuf_ServiceDescriptorProto**)_upb_array_ptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE google_protobuf_ServiceDescriptorProto** google_protobuf_FileDescriptorProto_resize_service(google_protobuf_FileDescriptorProto* msg, size_t size, upb_Arena* arena) {
  upb_MiniTableField field = {6, UPB_SIZE(16, 64), 0, 2, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  return (google_protobuf_ServiceDescriptorProto**)upb_Message_ResizeArray(msg, &field, size, arena);
}
UPB_INLINE struct google_protobuf_ServiceDescriptorProto* google_protobuf_FileDescriptorProto_add_service(google_protobuf_FileDescriptorProto* msg, upb_Arena* arena) {
  upb_MiniTableField field = {6, UPB_SIZE(16, 64), 0, 2, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(msg, &field, arena);
  if (!arr || !_upb_Array_ResizeUninitialized(arr, arr->size + 1, arena)) {
    return NULL;
  }
  struct google_protobuf_ServiceDescriptorProto* sub = (struct google_protobuf_ServiceDescriptorProto*)_upb_Message_New(&google_protobuf_ServiceDescriptorProto_msg_init, arena);
  if (!arr || !sub) return NULL;
  _upb_Array_Set(arr, arr->size - 1, &sub, sizeof(sub));
  return sub;
}
UPB_INLINE google_protobuf_FieldDescriptorProto** google_protobuf_FileDescriptorProto_mutable_extension(google_protobuf_FileDescriptorProto* msg, size_t* size) {
  upb_MiniTableField field = {7, UPB_SIZE(20, 72), 0, 3, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetMutableArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (google_protobuf_FieldDescriptorProto**)_upb_array_ptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE google_protobuf_FieldDescriptorProto** google_protobuf_FileDescriptorProto_resize_extension(google_protobuf_FileDescriptorProto* msg, size_t size, upb_Arena* arena) {
  upb_MiniTableField field = {7, UPB_SIZE(20, 72), 0, 3, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  return (google_protobuf_FieldDescriptorProto**)upb_Message_ResizeArray(msg, &field, size, arena);
}
UPB_INLINE struct google_protobuf_FieldDescriptorProto* google_protobuf_FileDescriptorProto_add_extension(google_protobuf_FileDescriptorProto* msg, upb_Arena* arena) {
  upb_MiniTableField field = {7, UPB_SIZE(20, 72), 0, 3, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(msg, &field, arena);
  if (!arr || !_upb_Array_ResizeUninitialized(arr, arr->size + 1, arena)) {
    return NULL;
  }
  struct google_protobuf_FieldDescriptorProto* sub = (struct google_protobuf_FieldDescriptorProto*)_upb_Message_New(&google_protobuf_FieldDescriptorProto_msg_init, arena);
  if (!arr || !sub) return NULL;
  _upb_Array_Set(arr, arr->size - 1, &sub, sizeof(sub));
  return sub;
}
UPB_INLINE void google_protobuf_FileDescriptorProto_set_options(google_protobuf_FileDescriptorProto *msg, google_protobuf_FileOptions* value) {
  const upb_MiniTableField field = {8, UPB_SIZE(24, 80), 3, 4, 11, kUpb_FieldMode_Scalar | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE struct google_protobuf_FileOptions* google_protobuf_FileDescriptorProto_mutable_options(google_protobuf_FileDescriptorProto* msg, upb_Arena* arena) {
  struct google_protobuf_FileOptions* sub = (struct google_protobuf_FileOptions*)google_protobuf_FileDescriptorProto_options(msg);
  if (sub == NULL) {
    sub = (struct google_protobuf_FileOptions*)_upb_Message_New(&google_protobuf_FileOptions_msg_init, arena);
    if (sub) google_protobuf_FileDescriptorProto_set_options(msg, sub);
  }
  return sub;
}
UPB_INLINE void google_protobuf_FileDescriptorProto_set_source_code_info(google_protobuf_FileDescriptorProto *msg, google_protobuf_SourceCodeInfo* value) {
  const upb_MiniTableField field = {9, UPB_SIZE(28, 88), 4, 5, 11, kUpb_FieldMode_Scalar | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE struct google_protobuf_SourceCodeInfo* google_protobuf_FileDescriptorProto_mutable_source_code_info(google_protobuf_FileDescriptorProto* msg, upb_Arena* arena) {
  struct google_protobuf_SourceCodeInfo* sub = (struct google_protobuf_SourceCodeInfo*)google_protobuf_FileDescriptorProto_source_code_info(msg);
  if (sub == NULL) {
    sub = (struct google_protobuf_SourceCodeInfo*)_upb_Message_New(&google_protobuf_SourceCodeInfo_msg_init, arena);
    if (sub) google_protobuf_FileDescriptorProto_set_source_code_info(msg, sub);
  }
  return sub;
}
UPB_INLINE int32_t* google_protobuf_FileDescriptorProto_mutable_public_dependency(google_protobuf_FileDescriptorProto* msg, size_t* size) {
  upb_MiniTableField field = {10, UPB_SIZE(32, 96), 0, kUpb_NoSub, 5, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetMutableArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (int32_t*)_upb_array_ptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE int32_t* google_protobuf_FileDescriptorProto_resize_public_dependency(google_protobuf_FileDescriptorProto* msg, size_t size, upb_Arena* arena) {
  upb_MiniTableField field = {10, UPB_SIZE(32, 96), 0, kUpb_NoSub, 5, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  return (int32_t*)upb_Message_ResizeArray(msg, &field, size, arena);
}
UPB_INLINE bool google_protobuf_FileDescriptorProto_add_public_dependency(google_protobuf_FileDescriptorProto* msg, int32_t val, upb_Arena* arena) {
  upb_MiniTableField field = {10, UPB_SIZE(32, 96), 0, kUpb_NoSub, 5, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(msg, &field, arena);
  if (!arr || !_upb_Array_ResizeUninitialized(arr, arr->size + 1, arena)) {
    return false;
  }
  _upb_Array_Set(arr, arr->size - 1, &val, sizeof(val));
  return true;
}
UPB_INLINE int32_t* google_protobuf_FileDescriptorProto_mutable_weak_dependency(google_protobuf_FileDescriptorProto* msg, size_t* size) {
  upb_MiniTableField field = {11, UPB_SIZE(36, 104), 0, kUpb_NoSub, 5, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetMutableArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (int32_t*)_upb_array_ptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE int32_t* google_protobuf_FileDescriptorProto_resize_weak_dependency(google_protobuf_FileDescriptorProto* msg, size_t size, upb_Arena* arena) {
  upb_MiniTableField field = {11, UPB_SIZE(36, 104), 0, kUpb_NoSub, 5, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  return (int32_t*)upb_Message_ResizeArray(msg, &field, size, arena);
}
UPB_INLINE bool google_protobuf_FileDescriptorProto_add_weak_dependency(google_protobuf_FileDescriptorProto* msg, int32_t val, upb_Arena* arena) {
  upb_MiniTableField field = {11, UPB_SIZE(36, 104), 0, kUpb_NoSub, 5, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(msg, &field, arena);
  if (!arr || !_upb_Array_ResizeUninitialized(arr, arr->size + 1, arena)) {
    return false;
  }
  _upb_Array_Set(arr, arr->size - 1, &val, sizeof(val));
  return true;
}
UPB_INLINE void google_protobuf_FileDescriptorProto_set_syntax(google_protobuf_FileDescriptorProto *msg, upb_StringView value) {
  const upb_MiniTableField field = {12, UPB_SIZE(56, 112), 5, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_FileDescriptorProto_set_edition(google_protobuf_FileDescriptorProto *msg, upb_StringView value) {
  const upb_MiniTableField field = {13, UPB_SIZE(64, 128), 6, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}

/* google.protobuf.DescriptorProto */

UPB_INLINE google_protobuf_DescriptorProto* google_protobuf_DescriptorProto_new(upb_Arena* arena) {
  return (google_protobuf_DescriptorProto*)_upb_Message_New(&google_protobuf_DescriptorProto_msg_init, arena);
}
UPB_INLINE google_protobuf_DescriptorProto* google_protobuf_DescriptorProto_parse(const char* buf, size_t size, upb_Arena* arena) {
  google_protobuf_DescriptorProto* ret = google_protobuf_DescriptorProto_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_DescriptorProto_msg_init, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE google_protobuf_DescriptorProto* google_protobuf_DescriptorProto_parse_ex(const char* buf, size_t size,
                           const upb_ExtensionRegistry* extreg,
                           int options, upb_Arena* arena) {
  google_protobuf_DescriptorProto* ret = google_protobuf_DescriptorProto_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_DescriptorProto_msg_init, extreg, options, arena) !=
      kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE char* google_protobuf_DescriptorProto_serialize(const google_protobuf_DescriptorProto* msg, upb_Arena* arena, size_t* len) {
  char* ptr;
  (void)upb_Encode(msg, &google_protobuf_DescriptorProto_msg_init, 0, arena, &ptr, len);
  return ptr;
}
UPB_INLINE char* google_protobuf_DescriptorProto_serialize_ex(const google_protobuf_DescriptorProto* msg, int options,
                                 upb_Arena* arena, size_t* len) {
  char* ptr;
  (void)upb_Encode(msg, &google_protobuf_DescriptorProto_msg_init, options, arena, &ptr, len);
  return ptr;
}
UPB_INLINE void google_protobuf_DescriptorProto_clear_name(google_protobuf_DescriptorProto* msg) {
  const upb_MiniTableField field = {1, UPB_SIZE(40, 8), 1, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE upb_StringView google_protobuf_DescriptorProto_name(const google_protobuf_DescriptorProto* msg) {
  upb_StringView default_val = upb_StringView_FromString("");
  upb_StringView ret;
  const upb_MiniTableField field = {1, UPB_SIZE(40, 8), 1, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_DescriptorProto_has_name(const google_protobuf_DescriptorProto* msg) {
  const upb_MiniTableField field = {1, UPB_SIZE(40, 8), 1, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_DescriptorProto_clear_field(google_protobuf_DescriptorProto* msg) {
  const upb_MiniTableField field = {2, UPB_SIZE(4, 24), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE const google_protobuf_FieldDescriptorProto* const* google_protobuf_DescriptorProto_field(const google_protobuf_DescriptorProto* msg, size_t* size) {
  const upb_MiniTableField field = {2, UPB_SIZE(4, 24), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (const google_protobuf_FieldDescriptorProto* const*)_upb_array_constptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE const upb_Array* _google_protobuf_DescriptorProto_field_upb_array(const google_protobuf_DescriptorProto* msg, size_t* size) {
  const upb_MiniTableField field = {2, UPB_SIZE(4, 24), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE upb_Array* _google_protobuf_DescriptorProto_field_mutable_upb_array(const google_protobuf_DescriptorProto* msg, size_t* size, upb_Arena* arena) {
  const upb_MiniTableField field = {2, UPB_SIZE(4, 24), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(
      (upb_Message*)msg, &field, arena);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE bool google_protobuf_DescriptorProto_has_field(const google_protobuf_DescriptorProto* msg) {
  size_t size;
  google_protobuf_DescriptorProto_field(msg, &size);
  return size != 0;
}
UPB_INLINE void google_protobuf_DescriptorProto_clear_nested_type(google_protobuf_DescriptorProto* msg) {
  const upb_MiniTableField field = {3, UPB_SIZE(8, 32), 0, 1, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE const google_protobuf_DescriptorProto* const* google_protobuf_DescriptorProto_nested_type(const google_protobuf_DescriptorProto* msg, size_t* size) {
  const upb_MiniTableField field = {3, UPB_SIZE(8, 32), 0, 1, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (const google_protobuf_DescriptorProto* const*)_upb_array_constptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE const upb_Array* _google_protobuf_DescriptorProto_nested_type_upb_array(const google_protobuf_DescriptorProto* msg, size_t* size) {
  const upb_MiniTableField field = {3, UPB_SIZE(8, 32), 0, 1, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE upb_Array* _google_protobuf_DescriptorProto_nested_type_mutable_upb_array(const google_protobuf_DescriptorProto* msg, size_t* size, upb_Arena* arena) {
  const upb_MiniTableField field = {3, UPB_SIZE(8, 32), 0, 1, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(
      (upb_Message*)msg, &field, arena);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE bool google_protobuf_DescriptorProto_has_nested_type(const google_protobuf_DescriptorProto* msg) {
  size_t size;
  google_protobuf_DescriptorProto_nested_type(msg, &size);
  return size != 0;
}
UPB_INLINE void google_protobuf_DescriptorProto_clear_enum_type(google_protobuf_DescriptorProto* msg) {
  const upb_MiniTableField field = {4, UPB_SIZE(12, 40), 0, 2, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE const google_protobuf_EnumDescriptorProto* const* google_protobuf_DescriptorProto_enum_type(const google_protobuf_DescriptorProto* msg, size_t* size) {
  const upb_MiniTableField field = {4, UPB_SIZE(12, 40), 0, 2, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (const google_protobuf_EnumDescriptorProto* const*)_upb_array_constptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE const upb_Array* _google_protobuf_DescriptorProto_enum_type_upb_array(const google_protobuf_DescriptorProto* msg, size_t* size) {
  const upb_MiniTableField field = {4, UPB_SIZE(12, 40), 0, 2, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE upb_Array* _google_protobuf_DescriptorProto_enum_type_mutable_upb_array(const google_protobuf_DescriptorProto* msg, size_t* size, upb_Arena* arena) {
  const upb_MiniTableField field = {4, UPB_SIZE(12, 40), 0, 2, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(
      (upb_Message*)msg, &field, arena);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE bool google_protobuf_DescriptorProto_has_enum_type(const google_protobuf_DescriptorProto* msg) {
  size_t size;
  google_protobuf_DescriptorProto_enum_type(msg, &size);
  return size != 0;
}
UPB_INLINE void google_protobuf_DescriptorProto_clear_extension_range(google_protobuf_DescriptorProto* msg) {
  const upb_MiniTableField field = {5, UPB_SIZE(16, 48), 0, 3, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE const google_protobuf_DescriptorProto_ExtensionRange* const* google_protobuf_DescriptorProto_extension_range(const google_protobuf_DescriptorProto* msg, size_t* size) {
  const upb_MiniTableField field = {5, UPB_SIZE(16, 48), 0, 3, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (const google_protobuf_DescriptorProto_ExtensionRange* const*)_upb_array_constptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE const upb_Array* _google_protobuf_DescriptorProto_extension_range_upb_array(const google_protobuf_DescriptorProto* msg, size_t* size) {
  const upb_MiniTableField field = {5, UPB_SIZE(16, 48), 0, 3, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE upb_Array* _google_protobuf_DescriptorProto_extension_range_mutable_upb_array(const google_protobuf_DescriptorProto* msg, size_t* size, upb_Arena* arena) {
  const upb_MiniTableField field = {5, UPB_SIZE(16, 48), 0, 3, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(
      (upb_Message*)msg, &field, arena);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE bool google_protobuf_DescriptorProto_has_extension_range(const google_protobuf_DescriptorProto* msg) {
  size_t size;
  google_protobuf_DescriptorProto_extension_range(msg, &size);
  return size != 0;
}
UPB_INLINE void google_protobuf_DescriptorProto_clear_extension(google_protobuf_DescriptorProto* msg) {
  const upb_MiniTableField field = {6, UPB_SIZE(20, 56), 0, 4, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE const google_protobuf_FieldDescriptorProto* const* google_protobuf_DescriptorProto_extension(const google_protobuf_DescriptorProto* msg, size_t* size) {
  const upb_MiniTableField field = {6, UPB_SIZE(20, 56), 0, 4, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (const google_protobuf_FieldDescriptorProto* const*)_upb_array_constptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE const upb_Array* _google_protobuf_DescriptorProto_extension_upb_array(const google_protobuf_DescriptorProto* msg, size_t* size) {
  const upb_MiniTableField field = {6, UPB_SIZE(20, 56), 0, 4, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE upb_Array* _google_protobuf_DescriptorProto_extension_mutable_upb_array(const google_protobuf_DescriptorProto* msg, size_t* size, upb_Arena* arena) {
  const upb_MiniTableField field = {6, UPB_SIZE(20, 56), 0, 4, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(
      (upb_Message*)msg, &field, arena);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE bool google_protobuf_DescriptorProto_has_extension(const google_protobuf_DescriptorProto* msg) {
  size_t size;
  google_protobuf_DescriptorProto_extension(msg, &size);
  return size != 0;
}
UPB_INLINE void google_protobuf_DescriptorProto_clear_options(google_protobuf_DescriptorProto* msg) {
  const upb_MiniTableField field = {7, UPB_SIZE(24, 64), 2, 5, 11, kUpb_FieldMode_Scalar | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE const google_protobuf_MessageOptions* google_protobuf_DescriptorProto_options(const google_protobuf_DescriptorProto* msg) {
  const google_protobuf_MessageOptions* default_val = NULL;
  const google_protobuf_MessageOptions* ret;
  const upb_MiniTableField field = {7, UPB_SIZE(24, 64), 2, 5, 11, kUpb_FieldMode_Scalar | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_DescriptorProto_has_options(const google_protobuf_DescriptorProto* msg) {
  const upb_MiniTableField field = {7, UPB_SIZE(24, 64), 2, 5, 11, kUpb_FieldMode_Scalar | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_DescriptorProto_clear_oneof_decl(google_protobuf_DescriptorProto* msg) {
  const upb_MiniTableField field = {8, UPB_SIZE(28, 72), 0, 6, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE const google_protobuf_OneofDescriptorProto* const* google_protobuf_DescriptorProto_oneof_decl(const google_protobuf_DescriptorProto* msg, size_t* size) {
  const upb_MiniTableField field = {8, UPB_SIZE(28, 72), 0, 6, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (const google_protobuf_OneofDescriptorProto* const*)_upb_array_constptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE const upb_Array* _google_protobuf_DescriptorProto_oneof_decl_upb_array(const google_protobuf_DescriptorProto* msg, size_t* size) {
  const upb_MiniTableField field = {8, UPB_SIZE(28, 72), 0, 6, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE upb_Array* _google_protobuf_DescriptorProto_oneof_decl_mutable_upb_array(const google_protobuf_DescriptorProto* msg, size_t* size, upb_Arena* arena) {
  const upb_MiniTableField field = {8, UPB_SIZE(28, 72), 0, 6, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(
      (upb_Message*)msg, &field, arena);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE bool google_protobuf_DescriptorProto_has_oneof_decl(const google_protobuf_DescriptorProto* msg) {
  size_t size;
  google_protobuf_DescriptorProto_oneof_decl(msg, &size);
  return size != 0;
}
UPB_INLINE void google_protobuf_DescriptorProto_clear_reserved_range(google_protobuf_DescriptorProto* msg) {
  const upb_MiniTableField field = {9, UPB_SIZE(32, 80), 0, 7, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE const google_protobuf_DescriptorProto_ReservedRange* const* google_protobuf_DescriptorProto_reserved_range(const google_protobuf_DescriptorProto* msg, size_t* size) {
  const upb_MiniTableField field = {9, UPB_SIZE(32, 80), 0, 7, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (const google_protobuf_DescriptorProto_ReservedRange* const*)_upb_array_constptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE const upb_Array* _google_protobuf_DescriptorProto_reserved_range_upb_array(const google_protobuf_DescriptorProto* msg, size_t* size) {
  const upb_MiniTableField field = {9, UPB_SIZE(32, 80), 0, 7, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE upb_Array* _google_protobuf_DescriptorProto_reserved_range_mutable_upb_array(const google_protobuf_DescriptorProto* msg, size_t* size, upb_Arena* arena) {
  const upb_MiniTableField field = {9, UPB_SIZE(32, 80), 0, 7, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(
      (upb_Message*)msg, &field, arena);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE bool google_protobuf_DescriptorProto_has_reserved_range(const google_protobuf_DescriptorProto* msg) {
  size_t size;
  google_protobuf_DescriptorProto_reserved_range(msg, &size);
  return size != 0;
}
UPB_INLINE void google_protobuf_DescriptorProto_clear_reserved_name(google_protobuf_DescriptorProto* msg) {
  const upb_MiniTableField field = {10, UPB_SIZE(36, 88), 0, kUpb_NoSub, 12, kUpb_FieldMode_Array | kUpb_LabelFlags_IsAlternate | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE upb_StringView const* google_protobuf_DescriptorProto_reserved_name(const google_protobuf_DescriptorProto* msg, size_t* size) {
  const upb_MiniTableField field = {10, UPB_SIZE(36, 88), 0, kUpb_NoSub, 12, kUpb_FieldMode_Array | kUpb_LabelFlags_IsAlternate | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (upb_StringView const*)_upb_array_constptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE const upb_Array* _google_protobuf_DescriptorProto_reserved_name_upb_array(const google_protobuf_DescriptorProto* msg, size_t* size) {
  const upb_MiniTableField field = {10, UPB_SIZE(36, 88), 0, kUpb_NoSub, 12, kUpb_FieldMode_Array | kUpb_LabelFlags_IsAlternate | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE upb_Array* _google_protobuf_DescriptorProto_reserved_name_mutable_upb_array(const google_protobuf_DescriptorProto* msg, size_t* size, upb_Arena* arena) {
  const upb_MiniTableField field = {10, UPB_SIZE(36, 88), 0, kUpb_NoSub, 12, kUpb_FieldMode_Array | kUpb_LabelFlags_IsAlternate | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(
      (upb_Message*)msg, &field, arena);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE bool google_protobuf_DescriptorProto_has_reserved_name(const google_protobuf_DescriptorProto* msg) {
  size_t size;
  google_protobuf_DescriptorProto_reserved_name(msg, &size);
  return size != 0;
}

UPB_INLINE void google_protobuf_DescriptorProto_set_name(google_protobuf_DescriptorProto *msg, upb_StringView value) {
  const upb_MiniTableField field = {1, UPB_SIZE(40, 8), 1, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE google_protobuf_FieldDescriptorProto** google_protobuf_DescriptorProto_mutable_field(google_protobuf_DescriptorProto* msg, size_t* size) {
  upb_MiniTableField field = {2, UPB_SIZE(4, 24), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetMutableArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (google_protobuf_FieldDescriptorProto**)_upb_array_ptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE google_protobuf_FieldDescriptorProto** google_protobuf_DescriptorProto_resize_field(google_protobuf_DescriptorProto* msg, size_t size, upb_Arena* arena) {
  upb_MiniTableField field = {2, UPB_SIZE(4, 24), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  return (google_protobuf_FieldDescriptorProto**)upb_Message_ResizeArray(msg, &field, size, arena);
}
UPB_INLINE struct google_protobuf_FieldDescriptorProto* google_protobuf_DescriptorProto_add_field(google_protobuf_DescriptorProto* msg, upb_Arena* arena) {
  upb_MiniTableField field = {2, UPB_SIZE(4, 24), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(msg, &field, arena);
  if (!arr || !_upb_Array_ResizeUninitialized(arr, arr->size + 1, arena)) {
    return NULL;
  }
  struct google_protobuf_FieldDescriptorProto* sub = (struct google_protobuf_FieldDescriptorProto*)_upb_Message_New(&google_protobuf_FieldDescriptorProto_msg_init, arena);
  if (!arr || !sub) return NULL;
  _upb_Array_Set(arr, arr->size - 1, &sub, sizeof(sub));
  return sub;
}
UPB_INLINE google_protobuf_DescriptorProto** google_protobuf_DescriptorProto_mutable_nested_type(google_protobuf_DescriptorProto* msg, size_t* size) {
  upb_MiniTableField field = {3, UPB_SIZE(8, 32), 0, 1, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetMutableArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (google_protobuf_DescriptorProto**)_upb_array_ptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE google_protobuf_DescriptorProto** google_protobuf_DescriptorProto_resize_nested_type(google_protobuf_DescriptorProto* msg, size_t size, upb_Arena* arena) {
  upb_MiniTableField field = {3, UPB_SIZE(8, 32), 0, 1, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  return (google_protobuf_DescriptorProto**)upb_Message_ResizeArray(msg, &field, size, arena);
}
UPB_INLINE struct google_protobuf_DescriptorProto* google_protobuf_DescriptorProto_add_nested_type(google_protobuf_DescriptorProto* msg, upb_Arena* arena) {
  upb_MiniTableField field = {3, UPB_SIZE(8, 32), 0, 1, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(msg, &field, arena);
  if (!arr || !_upb_Array_ResizeUninitialized(arr, arr->size + 1, arena)) {
    return NULL;
  }
  struct google_protobuf_DescriptorProto* sub = (struct google_protobuf_DescriptorProto*)_upb_Message_New(&google_protobuf_DescriptorProto_msg_init, arena);
  if (!arr || !sub) return NULL;
  _upb_Array_Set(arr, arr->size - 1, &sub, sizeof(sub));
  return sub;
}
UPB_INLINE google_protobuf_EnumDescriptorProto** google_protobuf_DescriptorProto_mutable_enum_type(google_protobuf_DescriptorProto* msg, size_t* size) {
  upb_MiniTableField field = {4, UPB_SIZE(12, 40), 0, 2, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetMutableArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (google_protobuf_EnumDescriptorProto**)_upb_array_ptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE google_protobuf_EnumDescriptorProto** google_protobuf_DescriptorProto_resize_enum_type(google_protobuf_DescriptorProto* msg, size_t size, upb_Arena* arena) {
  upb_MiniTableField field = {4, UPB_SIZE(12, 40), 0, 2, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  return (google_protobuf_EnumDescriptorProto**)upb_Message_ResizeArray(msg, &field, size, arena);
}
UPB_INLINE struct google_protobuf_EnumDescriptorProto* google_protobuf_DescriptorProto_add_enum_type(google_protobuf_DescriptorProto* msg, upb_Arena* arena) {
  upb_MiniTableField field = {4, UPB_SIZE(12, 40), 0, 2, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(msg, &field, arena);
  if (!arr || !_upb_Array_ResizeUninitialized(arr, arr->size + 1, arena)) {
    return NULL;
  }
  struct google_protobuf_EnumDescriptorProto* sub = (struct google_protobuf_EnumDescriptorProto*)_upb_Message_New(&google_protobuf_EnumDescriptorProto_msg_init, arena);
  if (!arr || !sub) return NULL;
  _upb_Array_Set(arr, arr->size - 1, &sub, sizeof(sub));
  return sub;
}
UPB_INLINE google_protobuf_DescriptorProto_ExtensionRange** google_protobuf_DescriptorProto_mutable_extension_range(google_protobuf_DescriptorProto* msg, size_t* size) {
  upb_MiniTableField field = {5, UPB_SIZE(16, 48), 0, 3, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetMutableArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (google_protobuf_DescriptorProto_ExtensionRange**)_upb_array_ptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE google_protobuf_DescriptorProto_ExtensionRange** google_protobuf_DescriptorProto_resize_extension_range(google_protobuf_DescriptorProto* msg, size_t size, upb_Arena* arena) {
  upb_MiniTableField field = {5, UPB_SIZE(16, 48), 0, 3, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  return (google_protobuf_DescriptorProto_ExtensionRange**)upb_Message_ResizeArray(msg, &field, size, arena);
}
UPB_INLINE struct google_protobuf_DescriptorProto_ExtensionRange* google_protobuf_DescriptorProto_add_extension_range(google_protobuf_DescriptorProto* msg, upb_Arena* arena) {
  upb_MiniTableField field = {5, UPB_SIZE(16, 48), 0, 3, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(msg, &field, arena);
  if (!arr || !_upb_Array_ResizeUninitialized(arr, arr->size + 1, arena)) {
    return NULL;
  }
  struct google_protobuf_DescriptorProto_ExtensionRange* sub = (struct google_protobuf_DescriptorProto_ExtensionRange*)_upb_Message_New(&google_protobuf_DescriptorProto_ExtensionRange_msg_init, arena);
  if (!arr || !sub) return NULL;
  _upb_Array_Set(arr, arr->size - 1, &sub, sizeof(sub));
  return sub;
}
UPB_INLINE google_protobuf_FieldDescriptorProto** google_protobuf_DescriptorProto_mutable_extension(google_protobuf_DescriptorProto* msg, size_t* size) {
  upb_MiniTableField field = {6, UPB_SIZE(20, 56), 0, 4, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetMutableArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (google_protobuf_FieldDescriptorProto**)_upb_array_ptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE google_protobuf_FieldDescriptorProto** google_protobuf_DescriptorProto_resize_extension(google_protobuf_DescriptorProto* msg, size_t size, upb_Arena* arena) {
  upb_MiniTableField field = {6, UPB_SIZE(20, 56), 0, 4, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  return (google_protobuf_FieldDescriptorProto**)upb_Message_ResizeArray(msg, &field, size, arena);
}
UPB_INLINE struct google_protobuf_FieldDescriptorProto* google_protobuf_DescriptorProto_add_extension(google_protobuf_DescriptorProto* msg, upb_Arena* arena) {
  upb_MiniTableField field = {6, UPB_SIZE(20, 56), 0, 4, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(msg, &field, arena);
  if (!arr || !_upb_Array_ResizeUninitialized(arr, arr->size + 1, arena)) {
    return NULL;
  }
  struct google_protobuf_FieldDescriptorProto* sub = (struct google_protobuf_FieldDescriptorProto*)_upb_Message_New(&google_protobuf_FieldDescriptorProto_msg_init, arena);
  if (!arr || !sub) return NULL;
  _upb_Array_Set(arr, arr->size - 1, &sub, sizeof(sub));
  return sub;
}
UPB_INLINE void google_protobuf_DescriptorProto_set_options(google_protobuf_DescriptorProto *msg, google_protobuf_MessageOptions* value) {
  const upb_MiniTableField field = {7, UPB_SIZE(24, 64), 2, 5, 11, kUpb_FieldMode_Scalar | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE struct google_protobuf_MessageOptions* google_protobuf_DescriptorProto_mutable_options(google_protobuf_DescriptorProto* msg, upb_Arena* arena) {
  struct google_protobuf_MessageOptions* sub = (struct google_protobuf_MessageOptions*)google_protobuf_DescriptorProto_options(msg);
  if (sub == NULL) {
    sub = (struct google_protobuf_MessageOptions*)_upb_Message_New(&google_protobuf_MessageOptions_msg_init, arena);
    if (sub) google_protobuf_DescriptorProto_set_options(msg, sub);
  }
  return sub;
}
UPB_INLINE google_protobuf_OneofDescriptorProto** google_protobuf_DescriptorProto_mutable_oneof_decl(google_protobuf_DescriptorProto* msg, size_t* size) {
  upb_MiniTableField field = {8, UPB_SIZE(28, 72), 0, 6, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetMutableArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (google_protobuf_OneofDescriptorProto**)_upb_array_ptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE google_protobuf_OneofDescriptorProto** google_protobuf_DescriptorProto_resize_oneof_decl(google_protobuf_DescriptorProto* msg, size_t size, upb_Arena* arena) {
  upb_MiniTableField field = {8, UPB_SIZE(28, 72), 0, 6, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  return (google_protobuf_OneofDescriptorProto**)upb_Message_ResizeArray(msg, &field, size, arena);
}
UPB_INLINE struct google_protobuf_OneofDescriptorProto* google_protobuf_DescriptorProto_add_oneof_decl(google_protobuf_DescriptorProto* msg, upb_Arena* arena) {
  upb_MiniTableField field = {8, UPB_SIZE(28, 72), 0, 6, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(msg, &field, arena);
  if (!arr || !_upb_Array_ResizeUninitialized(arr, arr->size + 1, arena)) {
    return NULL;
  }
  struct google_protobuf_OneofDescriptorProto* sub = (struct google_protobuf_OneofDescriptorProto*)_upb_Message_New(&google_protobuf_OneofDescriptorProto_msg_init, arena);
  if (!arr || !sub) return NULL;
  _upb_Array_Set(arr, arr->size - 1, &sub, sizeof(sub));
  return sub;
}
UPB_INLINE google_protobuf_DescriptorProto_ReservedRange** google_protobuf_DescriptorProto_mutable_reserved_range(google_protobuf_DescriptorProto* msg, size_t* size) {
  upb_MiniTableField field = {9, UPB_SIZE(32, 80), 0, 7, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetMutableArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (google_protobuf_DescriptorProto_ReservedRange**)_upb_array_ptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE google_protobuf_DescriptorProto_ReservedRange** google_protobuf_DescriptorProto_resize_reserved_range(google_protobuf_DescriptorProto* msg, size_t size, upb_Arena* arena) {
  upb_MiniTableField field = {9, UPB_SIZE(32, 80), 0, 7, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  return (google_protobuf_DescriptorProto_ReservedRange**)upb_Message_ResizeArray(msg, &field, size, arena);
}
UPB_INLINE struct google_protobuf_DescriptorProto_ReservedRange* google_protobuf_DescriptorProto_add_reserved_range(google_protobuf_DescriptorProto* msg, upb_Arena* arena) {
  upb_MiniTableField field = {9, UPB_SIZE(32, 80), 0, 7, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(msg, &field, arena);
  if (!arr || !_upb_Array_ResizeUninitialized(arr, arr->size + 1, arena)) {
    return NULL;
  }
  struct google_protobuf_DescriptorProto_ReservedRange* sub = (struct google_protobuf_DescriptorProto_ReservedRange*)_upb_Message_New(&google_protobuf_DescriptorProto_ReservedRange_msg_init, arena);
  if (!arr || !sub) return NULL;
  _upb_Array_Set(arr, arr->size - 1, &sub, sizeof(sub));
  return sub;
}
UPB_INLINE upb_StringView* google_protobuf_DescriptorProto_mutable_reserved_name(google_protobuf_DescriptorProto* msg, size_t* size) {
  upb_MiniTableField field = {10, UPB_SIZE(36, 88), 0, kUpb_NoSub, 12, kUpb_FieldMode_Array | kUpb_LabelFlags_IsAlternate | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetMutableArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (upb_StringView*)_upb_array_ptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE upb_StringView* google_protobuf_DescriptorProto_resize_reserved_name(google_protobuf_DescriptorProto* msg, size_t size, upb_Arena* arena) {
  upb_MiniTableField field = {10, UPB_SIZE(36, 88), 0, kUpb_NoSub, 12, kUpb_FieldMode_Array | kUpb_LabelFlags_IsAlternate | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  return (upb_StringView*)upb_Message_ResizeArray(msg, &field, size, arena);
}
UPB_INLINE bool google_protobuf_DescriptorProto_add_reserved_name(google_protobuf_DescriptorProto* msg, upb_StringView val, upb_Arena* arena) {
  upb_MiniTableField field = {10, UPB_SIZE(36, 88), 0, kUpb_NoSub, 12, kUpb_FieldMode_Array | kUpb_LabelFlags_IsAlternate | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(msg, &field, arena);
  if (!arr || !_upb_Array_ResizeUninitialized(arr, arr->size + 1, arena)) {
    return false;
  }
  _upb_Array_Set(arr, arr->size - 1, &val, sizeof(val));
  return true;
}

/* google.protobuf.DescriptorProto.ExtensionRange */

UPB_INLINE google_protobuf_DescriptorProto_ExtensionRange* google_protobuf_DescriptorProto_ExtensionRange_new(upb_Arena* arena) {
  return (google_protobuf_DescriptorProto_ExtensionRange*)_upb_Message_New(&google_protobuf_DescriptorProto_ExtensionRange_msg_init, arena);
}
UPB_INLINE google_protobuf_DescriptorProto_ExtensionRange* google_protobuf_DescriptorProto_ExtensionRange_parse(const char* buf, size_t size, upb_Arena* arena) {
  google_protobuf_DescriptorProto_ExtensionRange* ret = google_protobuf_DescriptorProto_ExtensionRange_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_DescriptorProto_ExtensionRange_msg_init, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE google_protobuf_DescriptorProto_ExtensionRange* google_protobuf_DescriptorProto_ExtensionRange_parse_ex(const char* buf, size_t size,
                           const upb_ExtensionRegistry* extreg,
                           int options, upb_Arena* arena) {
  google_protobuf_DescriptorProto_ExtensionRange* ret = google_protobuf_DescriptorProto_ExtensionRange_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_DescriptorProto_ExtensionRange_msg_init, extreg, options, arena) !=
      kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE char* google_protobuf_DescriptorProto_ExtensionRange_serialize(const google_protobuf_DescriptorProto_ExtensionRange* msg, upb_Arena* arena, size_t* len) {
  char* ptr;
  (void)upb_Encode(msg, &google_protobuf_DescriptorProto_ExtensionRange_msg_init, 0, arena, &ptr, len);
  return ptr;
}
UPB_INLINE char* google_protobuf_DescriptorProto_ExtensionRange_serialize_ex(const google_protobuf_DescriptorProto_ExtensionRange* msg, int options,
                                 upb_Arena* arena, size_t* len) {
  char* ptr;
  (void)upb_Encode(msg, &google_protobuf_DescriptorProto_ExtensionRange_msg_init, options, arena, &ptr, len);
  return ptr;
}
UPB_INLINE void google_protobuf_DescriptorProto_ExtensionRange_clear_start(google_protobuf_DescriptorProto_ExtensionRange* msg) {
  const upb_MiniTableField field = {1, 4, 1, kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE int32_t google_protobuf_DescriptorProto_ExtensionRange_start(const google_protobuf_DescriptorProto_ExtensionRange* msg) {
  int32_t default_val = (int32_t)0;
  int32_t ret;
  const upb_MiniTableField field = {1, 4, 1, kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_DescriptorProto_ExtensionRange_has_start(const google_protobuf_DescriptorProto_ExtensionRange* msg) {
  const upb_MiniTableField field = {1, 4, 1, kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_DescriptorProto_ExtensionRange_clear_end(google_protobuf_DescriptorProto_ExtensionRange* msg) {
  const upb_MiniTableField field = {2, 8, 2, kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE int32_t google_protobuf_DescriptorProto_ExtensionRange_end(const google_protobuf_DescriptorProto_ExtensionRange* msg) {
  int32_t default_val = (int32_t)0;
  int32_t ret;
  const upb_MiniTableField field = {2, 8, 2, kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_DescriptorProto_ExtensionRange_has_end(const google_protobuf_DescriptorProto_ExtensionRange* msg) {
  const upb_MiniTableField field = {2, 8, 2, kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_DescriptorProto_ExtensionRange_clear_options(google_protobuf_DescriptorProto_ExtensionRange* msg) {
  const upb_MiniTableField field = {3, UPB_SIZE(12, 16), 3, 0, 11, kUpb_FieldMode_Scalar | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE const google_protobuf_ExtensionRangeOptions* google_protobuf_DescriptorProto_ExtensionRange_options(const google_protobuf_DescriptorProto_ExtensionRange* msg) {
  const google_protobuf_ExtensionRangeOptions* default_val = NULL;
  const google_protobuf_ExtensionRangeOptions* ret;
  const upb_MiniTableField field = {3, UPB_SIZE(12, 16), 3, 0, 11, kUpb_FieldMode_Scalar | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_DescriptorProto_ExtensionRange_has_options(const google_protobuf_DescriptorProto_ExtensionRange* msg) {
  const upb_MiniTableField field = {3, UPB_SIZE(12, 16), 3, 0, 11, kUpb_FieldMode_Scalar | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}

UPB_INLINE void google_protobuf_DescriptorProto_ExtensionRange_set_start(google_protobuf_DescriptorProto_ExtensionRange *msg, int32_t value) {
  const upb_MiniTableField field = {1, 4, 1, kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_DescriptorProto_ExtensionRange_set_end(google_protobuf_DescriptorProto_ExtensionRange *msg, int32_t value) {
  const upb_MiniTableField field = {2, 8, 2, kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_DescriptorProto_ExtensionRange_set_options(google_protobuf_DescriptorProto_ExtensionRange *msg, google_protobuf_ExtensionRangeOptions* value) {
  const upb_MiniTableField field = {3, UPB_SIZE(12, 16), 3, 0, 11, kUpb_FieldMode_Scalar | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE struct google_protobuf_ExtensionRangeOptions* google_protobuf_DescriptorProto_ExtensionRange_mutable_options(google_protobuf_DescriptorProto_ExtensionRange* msg, upb_Arena* arena) {
  struct google_protobuf_ExtensionRangeOptions* sub = (struct google_protobuf_ExtensionRangeOptions*)google_protobuf_DescriptorProto_ExtensionRange_options(msg);
  if (sub == NULL) {
    sub = (struct google_protobuf_ExtensionRangeOptions*)_upb_Message_New(&google_protobuf_ExtensionRangeOptions_msg_init, arena);
    if (sub) google_protobuf_DescriptorProto_ExtensionRange_set_options(msg, sub);
  }
  return sub;
}

/* google.protobuf.DescriptorProto.ReservedRange */

UPB_INLINE google_protobuf_DescriptorProto_ReservedRange* google_protobuf_DescriptorProto_ReservedRange_new(upb_Arena* arena) {
  return (google_protobuf_DescriptorProto_ReservedRange*)_upb_Message_New(&google_protobuf_DescriptorProto_ReservedRange_msg_init, arena);
}
UPB_INLINE google_protobuf_DescriptorProto_ReservedRange* google_protobuf_DescriptorProto_ReservedRange_parse(const char* buf, size_t size, upb_Arena* arena) {
  google_protobuf_DescriptorProto_ReservedRange* ret = google_protobuf_DescriptorProto_ReservedRange_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_DescriptorProto_ReservedRange_msg_init, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE google_protobuf_DescriptorProto_ReservedRange* google_protobuf_DescriptorProto_ReservedRange_parse_ex(const char* buf, size_t size,
                           const upb_ExtensionRegistry* extreg,
                           int options, upb_Arena* arena) {
  google_protobuf_DescriptorProto_ReservedRange* ret = google_protobuf_DescriptorProto_ReservedRange_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_DescriptorProto_ReservedRange_msg_init, extreg, options, arena) !=
      kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE char* google_protobuf_DescriptorProto_ReservedRange_serialize(const google_protobuf_DescriptorProto_ReservedRange* msg, upb_Arena* arena, size_t* len) {
  char* ptr;
  (void)upb_Encode(msg, &google_protobuf_DescriptorProto_ReservedRange_msg_init, 0, arena, &ptr, len);
  return ptr;
}
UPB_INLINE char* google_protobuf_DescriptorProto_ReservedRange_serialize_ex(const google_protobuf_DescriptorProto_ReservedRange* msg, int options,
                                 upb_Arena* arena, size_t* len) {
  char* ptr;
  (void)upb_Encode(msg, &google_protobuf_DescriptorProto_ReservedRange_msg_init, options, arena, &ptr, len);
  return ptr;
}
UPB_INLINE void google_protobuf_DescriptorProto_ReservedRange_clear_start(google_protobuf_DescriptorProto_ReservedRange* msg) {
  const upb_MiniTableField field = {1, 4, 1, kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE int32_t google_protobuf_DescriptorProto_ReservedRange_start(const google_protobuf_DescriptorProto_ReservedRange* msg) {
  int32_t default_val = (int32_t)0;
  int32_t ret;
  const upb_MiniTableField field = {1, 4, 1, kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_DescriptorProto_ReservedRange_has_start(const google_protobuf_DescriptorProto_ReservedRange* msg) {
  const upb_MiniTableField field = {1, 4, 1, kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_DescriptorProto_ReservedRange_clear_end(google_protobuf_DescriptorProto_ReservedRange* msg) {
  const upb_MiniTableField field = {2, 8, 2, kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE int32_t google_protobuf_DescriptorProto_ReservedRange_end(const google_protobuf_DescriptorProto_ReservedRange* msg) {
  int32_t default_val = (int32_t)0;
  int32_t ret;
  const upb_MiniTableField field = {2, 8, 2, kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_DescriptorProto_ReservedRange_has_end(const google_protobuf_DescriptorProto_ReservedRange* msg) {
  const upb_MiniTableField field = {2, 8, 2, kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}

UPB_INLINE void google_protobuf_DescriptorProto_ReservedRange_set_start(google_protobuf_DescriptorProto_ReservedRange *msg, int32_t value) {
  const upb_MiniTableField field = {1, 4, 1, kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_DescriptorProto_ReservedRange_set_end(google_protobuf_DescriptorProto_ReservedRange *msg, int32_t value) {
  const upb_MiniTableField field = {2, 8, 2, kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}

/* google.protobuf.ExtensionRangeOptions */

UPB_INLINE google_protobuf_ExtensionRangeOptions* google_protobuf_ExtensionRangeOptions_new(upb_Arena* arena) {
  return (google_protobuf_ExtensionRangeOptions*)_upb_Message_New(&google_protobuf_ExtensionRangeOptions_msg_init, arena);
}
UPB_INLINE google_protobuf_ExtensionRangeOptions* google_protobuf_ExtensionRangeOptions_parse(const char* buf, size_t size, upb_Arena* arena) {
  google_protobuf_ExtensionRangeOptions* ret = google_protobuf_ExtensionRangeOptions_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_ExtensionRangeOptions_msg_init, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE google_protobuf_ExtensionRangeOptions* google_protobuf_ExtensionRangeOptions_parse_ex(const char* buf, size_t size,
                           const upb_ExtensionRegistry* extreg,
                           int options, upb_Arena* arena) {
  google_protobuf_ExtensionRangeOptions* ret = google_protobuf_ExtensionRangeOptions_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_ExtensionRangeOptions_msg_init, extreg, options, arena) !=
      kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE char* google_protobuf_ExtensionRangeOptions_serialize(const google_protobuf_ExtensionRangeOptions* msg, upb_Arena* arena, size_t* len) {
  char* ptr;
  (void)upb_Encode(msg, &google_protobuf_ExtensionRangeOptions_msg_init, 0, arena, &ptr, len);
  return ptr;
}
UPB_INLINE char* google_protobuf_ExtensionRangeOptions_serialize_ex(const google_protobuf_ExtensionRangeOptions* msg, int options,
                                 upb_Arena* arena, size_t* len) {
  char* ptr;
  (void)upb_Encode(msg, &google_protobuf_ExtensionRangeOptions_msg_init, options, arena, &ptr, len);
  return ptr;
}
UPB_INLINE void google_protobuf_ExtensionRangeOptions_clear_declaration(google_protobuf_ExtensionRangeOptions* msg) {
  const upb_MiniTableField field = {2, UPB_SIZE(4, 8), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE const google_protobuf_ExtensionRangeOptions_Declaration* const* google_protobuf_ExtensionRangeOptions_declaration(const google_protobuf_ExtensionRangeOptions* msg, size_t* size) {
  const upb_MiniTableField field = {2, UPB_SIZE(4, 8), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (const google_protobuf_ExtensionRangeOptions_Declaration* const*)_upb_array_constptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE const upb_Array* _google_protobuf_ExtensionRangeOptions_declaration_upb_array(const google_protobuf_ExtensionRangeOptions* msg, size_t* size) {
  const upb_MiniTableField field = {2, UPB_SIZE(4, 8), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE upb_Array* _google_protobuf_ExtensionRangeOptions_declaration_mutable_upb_array(const google_protobuf_ExtensionRangeOptions* msg, size_t* size, upb_Arena* arena) {
  const upb_MiniTableField field = {2, UPB_SIZE(4, 8), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(
      (upb_Message*)msg, &field, arena);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE bool google_protobuf_ExtensionRangeOptions_has_declaration(const google_protobuf_ExtensionRangeOptions* msg) {
  size_t size;
  google_protobuf_ExtensionRangeOptions_declaration(msg, &size);
  return size != 0;
}
UPB_INLINE void google_protobuf_ExtensionRangeOptions_clear_verification(google_protobuf_ExtensionRangeOptions* msg) {
  const upb_MiniTableField field = {3, UPB_SIZE(8, 4), 1, 1, 14, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE int32_t google_protobuf_ExtensionRangeOptions_verification(const google_protobuf_ExtensionRangeOptions* msg) {
  int32_t default_val = 1;
  int32_t ret;
  const upb_MiniTableField field = {3, UPB_SIZE(8, 4), 1, 1, 14, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_ExtensionRangeOptions_has_verification(const google_protobuf_ExtensionRangeOptions* msg) {
  const upb_MiniTableField field = {3, UPB_SIZE(8, 4), 1, 1, 14, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_ExtensionRangeOptions_clear_uninterpreted_option(google_protobuf_ExtensionRangeOptions* msg) {
  const upb_MiniTableField field = {999, UPB_SIZE(12, 16), 0, 2, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE const google_protobuf_UninterpretedOption* const* google_protobuf_ExtensionRangeOptions_uninterpreted_option(const google_protobuf_ExtensionRangeOptions* msg, size_t* size) {
  const upb_MiniTableField field = {999, UPB_SIZE(12, 16), 0, 2, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (const google_protobuf_UninterpretedOption* const*)_upb_array_constptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE const upb_Array* _google_protobuf_ExtensionRangeOptions_uninterpreted_option_upb_array(const google_protobuf_ExtensionRangeOptions* msg, size_t* size) {
  const upb_MiniTableField field = {999, UPB_SIZE(12, 16), 0, 2, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE upb_Array* _google_protobuf_ExtensionRangeOptions_uninterpreted_option_mutable_upb_array(const google_protobuf_ExtensionRangeOptions* msg, size_t* size, upb_Arena* arena) {
  const upb_MiniTableField field = {999, UPB_SIZE(12, 16), 0, 2, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(
      (upb_Message*)msg, &field, arena);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE bool google_protobuf_ExtensionRangeOptions_has_uninterpreted_option(const google_protobuf_ExtensionRangeOptions* msg) {
  size_t size;
  google_protobuf_ExtensionRangeOptions_uninterpreted_option(msg, &size);
  return size != 0;
}

UPB_INLINE google_protobuf_ExtensionRangeOptions_Declaration** google_protobuf_ExtensionRangeOptions_mutable_declaration(google_protobuf_ExtensionRangeOptions* msg, size_t* size) {
  upb_MiniTableField field = {2, UPB_SIZE(4, 8), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetMutableArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (google_protobuf_ExtensionRangeOptions_Declaration**)_upb_array_ptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE google_protobuf_ExtensionRangeOptions_Declaration** google_protobuf_ExtensionRangeOptions_resize_declaration(google_protobuf_ExtensionRangeOptions* msg, size_t size, upb_Arena* arena) {
  upb_MiniTableField field = {2, UPB_SIZE(4, 8), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  return (google_protobuf_ExtensionRangeOptions_Declaration**)upb_Message_ResizeArray(msg, &field, size, arena);
}
UPB_INLINE struct google_protobuf_ExtensionRangeOptions_Declaration* google_protobuf_ExtensionRangeOptions_add_declaration(google_protobuf_ExtensionRangeOptions* msg, upb_Arena* arena) {
  upb_MiniTableField field = {2, UPB_SIZE(4, 8), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(msg, &field, arena);
  if (!arr || !_upb_Array_ResizeUninitialized(arr, arr->size + 1, arena)) {
    return NULL;
  }
  struct google_protobuf_ExtensionRangeOptions_Declaration* sub = (struct google_protobuf_ExtensionRangeOptions_Declaration*)_upb_Message_New(&google_protobuf_ExtensionRangeOptions_Declaration_msg_init, arena);
  if (!arr || !sub) return NULL;
  _upb_Array_Set(arr, arr->size - 1, &sub, sizeof(sub));
  return sub;
}
UPB_INLINE void google_protobuf_ExtensionRangeOptions_set_verification(google_protobuf_ExtensionRangeOptions *msg, int32_t value) {
  const upb_MiniTableField field = {3, UPB_SIZE(8, 4), 1, 1, 14, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_ExtensionRangeOptions_mutable_uninterpreted_option(google_protobuf_ExtensionRangeOptions* msg, size_t* size) {
  upb_MiniTableField field = {999, UPB_SIZE(12, 16), 0, 2, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetMutableArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (google_protobuf_UninterpretedOption**)_upb_array_ptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_ExtensionRangeOptions_resize_uninterpreted_option(google_protobuf_ExtensionRangeOptions* msg, size_t size, upb_Arena* arena) {
  upb_MiniTableField field = {999, UPB_SIZE(12, 16), 0, 2, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  return (google_protobuf_UninterpretedOption**)upb_Message_ResizeArray(msg, &field, size, arena);
}
UPB_INLINE struct google_protobuf_UninterpretedOption* google_protobuf_ExtensionRangeOptions_add_uninterpreted_option(google_protobuf_ExtensionRangeOptions* msg, upb_Arena* arena) {
  upb_MiniTableField field = {999, UPB_SIZE(12, 16), 0, 2, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(msg, &field, arena);
  if (!arr || !_upb_Array_ResizeUninitialized(arr, arr->size + 1, arena)) {
    return NULL;
  }
  struct google_protobuf_UninterpretedOption* sub = (struct google_protobuf_UninterpretedOption*)_upb_Message_New(&google_protobuf_UninterpretedOption_msg_init, arena);
  if (!arr || !sub) return NULL;
  _upb_Array_Set(arr, arr->size - 1, &sub, sizeof(sub));
  return sub;
}

/* google.protobuf.ExtensionRangeOptions.Declaration */

UPB_INLINE google_protobuf_ExtensionRangeOptions_Declaration* google_protobuf_ExtensionRangeOptions_Declaration_new(upb_Arena* arena) {
  return (google_protobuf_ExtensionRangeOptions_Declaration*)_upb_Message_New(&google_protobuf_ExtensionRangeOptions_Declaration_msg_init, arena);
}
UPB_INLINE google_protobuf_ExtensionRangeOptions_Declaration* google_protobuf_ExtensionRangeOptions_Declaration_parse(const char* buf, size_t size, upb_Arena* arena) {
  google_protobuf_ExtensionRangeOptions_Declaration* ret = google_protobuf_ExtensionRangeOptions_Declaration_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_ExtensionRangeOptions_Declaration_msg_init, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE google_protobuf_ExtensionRangeOptions_Declaration* google_protobuf_ExtensionRangeOptions_Declaration_parse_ex(const char* buf, size_t size,
                           const upb_ExtensionRegistry* extreg,
                           int options, upb_Arena* arena) {
  google_protobuf_ExtensionRangeOptions_Declaration* ret = google_protobuf_ExtensionRangeOptions_Declaration_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_ExtensionRangeOptions_Declaration_msg_init, extreg, options, arena) !=
      kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE char* google_protobuf_ExtensionRangeOptions_Declaration_serialize(const google_protobuf_ExtensionRangeOptions_Declaration* msg, upb_Arena* arena, size_t* len) {
  char* ptr;
  (void)upb_Encode(msg, &google_protobuf_ExtensionRangeOptions_Declaration_msg_init, 0, arena, &ptr, len);
  return ptr;
}
UPB_INLINE char* google_protobuf_ExtensionRangeOptions_Declaration_serialize_ex(const google_protobuf_ExtensionRangeOptions_Declaration* msg, int options,
                                 upb_Arena* arena, size_t* len) {
  char* ptr;
  (void)upb_Encode(msg, &google_protobuf_ExtensionRangeOptions_Declaration_msg_init, options, arena, &ptr, len);
  return ptr;
}
UPB_INLINE void google_protobuf_ExtensionRangeOptions_Declaration_clear_number(google_protobuf_ExtensionRangeOptions_Declaration* msg) {
  const upb_MiniTableField field = {1, 4, 1, kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE int32_t google_protobuf_ExtensionRangeOptions_Declaration_number(const google_protobuf_ExtensionRangeOptions_Declaration* msg) {
  int32_t default_val = (int32_t)0;
  int32_t ret;
  const upb_MiniTableField field = {1, 4, 1, kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_ExtensionRangeOptions_Declaration_has_number(const google_protobuf_ExtensionRangeOptions_Declaration* msg) {
  const upb_MiniTableField field = {1, 4, 1, kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_ExtensionRangeOptions_Declaration_clear_full_name(google_protobuf_ExtensionRangeOptions_Declaration* msg) {
  const upb_MiniTableField field = {2, UPB_SIZE(12, 16), 2, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE upb_StringView google_protobuf_ExtensionRangeOptions_Declaration_full_name(const google_protobuf_ExtensionRangeOptions_Declaration* msg) {
  upb_StringView default_val = upb_StringView_FromString("");
  upb_StringView ret;
  const upb_MiniTableField field = {2, UPB_SIZE(12, 16), 2, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_ExtensionRangeOptions_Declaration_has_full_name(const google_protobuf_ExtensionRangeOptions_Declaration* msg) {
  const upb_MiniTableField field = {2, UPB_SIZE(12, 16), 2, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_ExtensionRangeOptions_Declaration_clear_type(google_protobuf_ExtensionRangeOptions_Declaration* msg) {
  const upb_MiniTableField field = {3, UPB_SIZE(20, 32), 3, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE upb_StringView google_protobuf_ExtensionRangeOptions_Declaration_type(const google_protobuf_ExtensionRangeOptions_Declaration* msg) {
  upb_StringView default_val = upb_StringView_FromString("");
  upb_StringView ret;
  const upb_MiniTableField field = {3, UPB_SIZE(20, 32), 3, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_ExtensionRangeOptions_Declaration_has_type(const google_protobuf_ExtensionRangeOptions_Declaration* msg) {
  const upb_MiniTableField field = {3, UPB_SIZE(20, 32), 3, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_ExtensionRangeOptions_Declaration_clear_is_repeated(google_protobuf_ExtensionRangeOptions_Declaration* msg) {
  const upb_MiniTableField field = {4, 8, 4, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE bool google_protobuf_ExtensionRangeOptions_Declaration_is_repeated(const google_protobuf_ExtensionRangeOptions_Declaration* msg) {
  bool default_val = false;
  bool ret;
  const upb_MiniTableField field = {4, 8, 4, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_ExtensionRangeOptions_Declaration_has_is_repeated(const google_protobuf_ExtensionRangeOptions_Declaration* msg) {
  const upb_MiniTableField field = {4, 8, 4, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_ExtensionRangeOptions_Declaration_clear_reserved(google_protobuf_ExtensionRangeOptions_Declaration* msg) {
  const upb_MiniTableField field = {5, 9, 5, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE bool google_protobuf_ExtensionRangeOptions_Declaration_reserved(const google_protobuf_ExtensionRangeOptions_Declaration* msg) {
  bool default_val = false;
  bool ret;
  const upb_MiniTableField field = {5, 9, 5, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_ExtensionRangeOptions_Declaration_has_reserved(const google_protobuf_ExtensionRangeOptions_Declaration* msg) {
  const upb_MiniTableField field = {5, 9, 5, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_ExtensionRangeOptions_Declaration_clear_repeated(google_protobuf_ExtensionRangeOptions_Declaration* msg) {
  const upb_MiniTableField field = {6, 10, 6, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE bool google_protobuf_ExtensionRangeOptions_Declaration_repeated(const google_protobuf_ExtensionRangeOptions_Declaration* msg) {
  bool default_val = false;
  bool ret;
  const upb_MiniTableField field = {6, 10, 6, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_ExtensionRangeOptions_Declaration_has_repeated(const google_protobuf_ExtensionRangeOptions_Declaration* msg) {
  const upb_MiniTableField field = {6, 10, 6, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}

UPB_INLINE void google_protobuf_ExtensionRangeOptions_Declaration_set_number(google_protobuf_ExtensionRangeOptions_Declaration *msg, int32_t value) {
  const upb_MiniTableField field = {1, 4, 1, kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_ExtensionRangeOptions_Declaration_set_full_name(google_protobuf_ExtensionRangeOptions_Declaration *msg, upb_StringView value) {
  const upb_MiniTableField field = {2, UPB_SIZE(12, 16), 2, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_ExtensionRangeOptions_Declaration_set_type(google_protobuf_ExtensionRangeOptions_Declaration *msg, upb_StringView value) {
  const upb_MiniTableField field = {3, UPB_SIZE(20, 32), 3, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_ExtensionRangeOptions_Declaration_set_is_repeated(google_protobuf_ExtensionRangeOptions_Declaration *msg, bool value) {
  const upb_MiniTableField field = {4, 8, 4, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_ExtensionRangeOptions_Declaration_set_reserved(google_protobuf_ExtensionRangeOptions_Declaration *msg, bool value) {
  const upb_MiniTableField field = {5, 9, 5, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_ExtensionRangeOptions_Declaration_set_repeated(google_protobuf_ExtensionRangeOptions_Declaration *msg, bool value) {
  const upb_MiniTableField field = {6, 10, 6, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}

/* google.protobuf.FieldDescriptorProto */

UPB_INLINE google_protobuf_FieldDescriptorProto* google_protobuf_FieldDescriptorProto_new(upb_Arena* arena) {
  return (google_protobuf_FieldDescriptorProto*)_upb_Message_New(&google_protobuf_FieldDescriptorProto_msg_init, arena);
}
UPB_INLINE google_protobuf_FieldDescriptorProto* google_protobuf_FieldDescriptorProto_parse(const char* buf, size_t size, upb_Arena* arena) {
  google_protobuf_FieldDescriptorProto* ret = google_protobuf_FieldDescriptorProto_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_FieldDescriptorProto_msg_init, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE google_protobuf_FieldDescriptorProto* google_protobuf_FieldDescriptorProto_parse_ex(const char* buf, size_t size,
                           const upb_ExtensionRegistry* extreg,
                           int options, upb_Arena* arena) {
  google_protobuf_FieldDescriptorProto* ret = google_protobuf_FieldDescriptorProto_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_FieldDescriptorProto_msg_init, extreg, options, arena) !=
      kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE char* google_protobuf_FieldDescriptorProto_serialize(const google_protobuf_FieldDescriptorProto* msg, upb_Arena* arena, size_t* len) {
  char* ptr;
  (void)upb_Encode(msg, &google_protobuf_FieldDescriptorProto_msg_init, 0, arena, &ptr, len);
  return ptr;
}
UPB_INLINE char* google_protobuf_FieldDescriptorProto_serialize_ex(const google_protobuf_FieldDescriptorProto* msg, int options,
                                 upb_Arena* arena, size_t* len) {
  char* ptr;
  (void)upb_Encode(msg, &google_protobuf_FieldDescriptorProto_msg_init, options, arena, &ptr, len);
  return ptr;
}
UPB_INLINE void google_protobuf_FieldDescriptorProto_clear_name(google_protobuf_FieldDescriptorProto* msg) {
  const upb_MiniTableField field = {1, UPB_SIZE(28, 24), 1, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE upb_StringView google_protobuf_FieldDescriptorProto_name(const google_protobuf_FieldDescriptorProto* msg) {
  upb_StringView default_val = upb_StringView_FromString("");
  upb_StringView ret;
  const upb_MiniTableField field = {1, UPB_SIZE(28, 24), 1, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_FieldDescriptorProto_has_name(const google_protobuf_FieldDescriptorProto* msg) {
  const upb_MiniTableField field = {1, UPB_SIZE(28, 24), 1, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_FieldDescriptorProto_clear_extendee(google_protobuf_FieldDescriptorProto* msg) {
  const upb_MiniTableField field = {2, UPB_SIZE(36, 40), 2, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE upb_StringView google_protobuf_FieldDescriptorProto_extendee(const google_protobuf_FieldDescriptorProto* msg) {
  upb_StringView default_val = upb_StringView_FromString("");
  upb_StringView ret;
  const upb_MiniTableField field = {2, UPB_SIZE(36, 40), 2, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_FieldDescriptorProto_has_extendee(const google_protobuf_FieldDescriptorProto* msg) {
  const upb_MiniTableField field = {2, UPB_SIZE(36, 40), 2, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_FieldDescriptorProto_clear_number(google_protobuf_FieldDescriptorProto* msg) {
  const upb_MiniTableField field = {3, 4, 3, kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE int32_t google_protobuf_FieldDescriptorProto_number(const google_protobuf_FieldDescriptorProto* msg) {
  int32_t default_val = (int32_t)0;
  int32_t ret;
  const upb_MiniTableField field = {3, 4, 3, kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_FieldDescriptorProto_has_number(const google_protobuf_FieldDescriptorProto* msg) {
  const upb_MiniTableField field = {3, 4, 3, kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_FieldDescriptorProto_clear_label(google_protobuf_FieldDescriptorProto* msg) {
  const upb_MiniTableField field = {4, 8, 4, 0, 14, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE int32_t google_protobuf_FieldDescriptorProto_label(const google_protobuf_FieldDescriptorProto* msg) {
  int32_t default_val = 1;
  int32_t ret;
  const upb_MiniTableField field = {4, 8, 4, 0, 14, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_FieldDescriptorProto_has_label(const google_protobuf_FieldDescriptorProto* msg) {
  const upb_MiniTableField field = {4, 8, 4, 0, 14, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_FieldDescriptorProto_clear_type(google_protobuf_FieldDescriptorProto* msg) {
  const upb_MiniTableField field = {5, 12, 5, 1, 14, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE int32_t google_protobuf_FieldDescriptorProto_type(const google_protobuf_FieldDescriptorProto* msg) {
  int32_t default_val = 1;
  int32_t ret;
  const upb_MiniTableField field = {5, 12, 5, 1, 14, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_FieldDescriptorProto_has_type(const google_protobuf_FieldDescriptorProto* msg) {
  const upb_MiniTableField field = {5, 12, 5, 1, 14, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_FieldDescriptorProto_clear_type_name(google_protobuf_FieldDescriptorProto* msg) {
  const upb_MiniTableField field = {6, UPB_SIZE(44, 56), 6, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE upb_StringView google_protobuf_FieldDescriptorProto_type_name(const google_protobuf_FieldDescriptorProto* msg) {
  upb_StringView default_val = upb_StringView_FromString("");
  upb_StringView ret;
  const upb_MiniTableField field = {6, UPB_SIZE(44, 56), 6, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_FieldDescriptorProto_has_type_name(const google_protobuf_FieldDescriptorProto* msg) {
  const upb_MiniTableField field = {6, UPB_SIZE(44, 56), 6, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_FieldDescriptorProto_clear_default_value(google_protobuf_FieldDescriptorProto* msg) {
  const upb_MiniTableField field = {7, UPB_SIZE(52, 72), 7, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE upb_StringView google_protobuf_FieldDescriptorProto_default_value(const google_protobuf_FieldDescriptorProto* msg) {
  upb_StringView default_val = upb_StringView_FromString("");
  upb_StringView ret;
  const upb_MiniTableField field = {7, UPB_SIZE(52, 72), 7, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_FieldDescriptorProto_has_default_value(const google_protobuf_FieldDescriptorProto* msg) {
  const upb_MiniTableField field = {7, UPB_SIZE(52, 72), 7, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_FieldDescriptorProto_clear_options(google_protobuf_FieldDescriptorProto* msg) {
  const upb_MiniTableField field = {8, UPB_SIZE(16, 88), 8, 2, 11, kUpb_FieldMode_Scalar | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE const google_protobuf_FieldOptions* google_protobuf_FieldDescriptorProto_options(const google_protobuf_FieldDescriptorProto* msg) {
  const google_protobuf_FieldOptions* default_val = NULL;
  const google_protobuf_FieldOptions* ret;
  const upb_MiniTableField field = {8, UPB_SIZE(16, 88), 8, 2, 11, kUpb_FieldMode_Scalar | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_FieldDescriptorProto_has_options(const google_protobuf_FieldDescriptorProto* msg) {
  const upb_MiniTableField field = {8, UPB_SIZE(16, 88), 8, 2, 11, kUpb_FieldMode_Scalar | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_FieldDescriptorProto_clear_oneof_index(google_protobuf_FieldDescriptorProto* msg) {
  const upb_MiniTableField field = {9, UPB_SIZE(20, 16), 9, kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE int32_t google_protobuf_FieldDescriptorProto_oneof_index(const google_protobuf_FieldDescriptorProto* msg) {
  int32_t default_val = (int32_t)0;
  int32_t ret;
  const upb_MiniTableField field = {9, UPB_SIZE(20, 16), 9, kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_FieldDescriptorProto_has_oneof_index(const google_protobuf_FieldDescriptorProto* msg) {
  const upb_MiniTableField field = {9, UPB_SIZE(20, 16), 9, kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_FieldDescriptorProto_clear_json_name(google_protobuf_FieldDescriptorProto* msg) {
  const upb_MiniTableField field = {10, UPB_SIZE(60, 96), 10, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE upb_StringView google_protobuf_FieldDescriptorProto_json_name(const google_protobuf_FieldDescriptorProto* msg) {
  upb_StringView default_val = upb_StringView_FromString("");
  upb_StringView ret;
  const upb_MiniTableField field = {10, UPB_SIZE(60, 96), 10, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_FieldDescriptorProto_has_json_name(const google_protobuf_FieldDescriptorProto* msg) {
  const upb_MiniTableField field = {10, UPB_SIZE(60, 96), 10, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_FieldDescriptorProto_clear_proto3_optional(google_protobuf_FieldDescriptorProto* msg) {
  const upb_MiniTableField field = {17, UPB_SIZE(24, 20), 11, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE bool google_protobuf_FieldDescriptorProto_proto3_optional(const google_protobuf_FieldDescriptorProto* msg) {
  bool default_val = false;
  bool ret;
  const upb_MiniTableField field = {17, UPB_SIZE(24, 20), 11, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_FieldDescriptorProto_has_proto3_optional(const google_protobuf_FieldDescriptorProto* msg) {
  const upb_MiniTableField field = {17, UPB_SIZE(24, 20), 11, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}

UPB_INLINE void google_protobuf_FieldDescriptorProto_set_name(google_protobuf_FieldDescriptorProto *msg, upb_StringView value) {
  const upb_MiniTableField field = {1, UPB_SIZE(28, 24), 1, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_FieldDescriptorProto_set_extendee(google_protobuf_FieldDescriptorProto *msg, upb_StringView value) {
  const upb_MiniTableField field = {2, UPB_SIZE(36, 40), 2, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_FieldDescriptorProto_set_number(google_protobuf_FieldDescriptorProto *msg, int32_t value) {
  const upb_MiniTableField field = {3, 4, 3, kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_FieldDescriptorProto_set_label(google_protobuf_FieldDescriptorProto *msg, int32_t value) {
  const upb_MiniTableField field = {4, 8, 4, 0, 14, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_FieldDescriptorProto_set_type(google_protobuf_FieldDescriptorProto *msg, int32_t value) {
  const upb_MiniTableField field = {5, 12, 5, 1, 14, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_FieldDescriptorProto_set_type_name(google_protobuf_FieldDescriptorProto *msg, upb_StringView value) {
  const upb_MiniTableField field = {6, UPB_SIZE(44, 56), 6, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_FieldDescriptorProto_set_default_value(google_protobuf_FieldDescriptorProto *msg, upb_StringView value) {
  const upb_MiniTableField field = {7, UPB_SIZE(52, 72), 7, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_FieldDescriptorProto_set_options(google_protobuf_FieldDescriptorProto *msg, google_protobuf_FieldOptions* value) {
  const upb_MiniTableField field = {8, UPB_SIZE(16, 88), 8, 2, 11, kUpb_FieldMode_Scalar | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE struct google_protobuf_FieldOptions* google_protobuf_FieldDescriptorProto_mutable_options(google_protobuf_FieldDescriptorProto* msg, upb_Arena* arena) {
  struct google_protobuf_FieldOptions* sub = (struct google_protobuf_FieldOptions*)google_protobuf_FieldDescriptorProto_options(msg);
  if (sub == NULL) {
    sub = (struct google_protobuf_FieldOptions*)_upb_Message_New(&google_protobuf_FieldOptions_msg_init, arena);
    if (sub) google_protobuf_FieldDescriptorProto_set_options(msg, sub);
  }
  return sub;
}
UPB_INLINE void google_protobuf_FieldDescriptorProto_set_oneof_index(google_protobuf_FieldDescriptorProto *msg, int32_t value) {
  const upb_MiniTableField field = {9, UPB_SIZE(20, 16), 9, kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_FieldDescriptorProto_set_json_name(google_protobuf_FieldDescriptorProto *msg, upb_StringView value) {
  const upb_MiniTableField field = {10, UPB_SIZE(60, 96), 10, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_FieldDescriptorProto_set_proto3_optional(google_protobuf_FieldDescriptorProto *msg, bool value) {
  const upb_MiniTableField field = {17, UPB_SIZE(24, 20), 11, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}

/* google.protobuf.OneofDescriptorProto */

UPB_INLINE google_protobuf_OneofDescriptorProto* google_protobuf_OneofDescriptorProto_new(upb_Arena* arena) {
  return (google_protobuf_OneofDescriptorProto*)_upb_Message_New(&google_protobuf_OneofDescriptorProto_msg_init, arena);
}
UPB_INLINE google_protobuf_OneofDescriptorProto* google_protobuf_OneofDescriptorProto_parse(const char* buf, size_t size, upb_Arena* arena) {
  google_protobuf_OneofDescriptorProto* ret = google_protobuf_OneofDescriptorProto_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_OneofDescriptorProto_msg_init, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE google_protobuf_OneofDescriptorProto* google_protobuf_OneofDescriptorProto_parse_ex(const char* buf, size_t size,
                           const upb_ExtensionRegistry* extreg,
                           int options, upb_Arena* arena) {
  google_protobuf_OneofDescriptorProto* ret = google_protobuf_OneofDescriptorProto_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_OneofDescriptorProto_msg_init, extreg, options, arena) !=
      kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE char* google_protobuf_OneofDescriptorProto_serialize(const google_protobuf_OneofDescriptorProto* msg, upb_Arena* arena, size_t* len) {
  char* ptr;
  (void)upb_Encode(msg, &google_protobuf_OneofDescriptorProto_msg_init, 0, arena, &ptr, len);
  return ptr;
}
UPB_INLINE char* google_protobuf_OneofDescriptorProto_serialize_ex(const google_protobuf_OneofDescriptorProto* msg, int options,
                                 upb_Arena* arena, size_t* len) {
  char* ptr;
  (void)upb_Encode(msg, &google_protobuf_OneofDescriptorProto_msg_init, options, arena, &ptr, len);
  return ptr;
}
UPB_INLINE void google_protobuf_OneofDescriptorProto_clear_name(google_protobuf_OneofDescriptorProto* msg) {
  const upb_MiniTableField field = {1, 8, 1, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE upb_StringView google_protobuf_OneofDescriptorProto_name(const google_protobuf_OneofDescriptorProto* msg) {
  upb_StringView default_val = upb_StringView_FromString("");
  upb_StringView ret;
  const upb_MiniTableField field = {1, 8, 1, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_OneofDescriptorProto_has_name(const google_protobuf_OneofDescriptorProto* msg) {
  const upb_MiniTableField field = {1, 8, 1, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_OneofDescriptorProto_clear_options(google_protobuf_OneofDescriptorProto* msg) {
  const upb_MiniTableField field = {2, UPB_SIZE(4, 24), 2, 0, 11, kUpb_FieldMode_Scalar | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE const google_protobuf_OneofOptions* google_protobuf_OneofDescriptorProto_options(const google_protobuf_OneofDescriptorProto* msg) {
  const google_protobuf_OneofOptions* default_val = NULL;
  const google_protobuf_OneofOptions* ret;
  const upb_MiniTableField field = {2, UPB_SIZE(4, 24), 2, 0, 11, kUpb_FieldMode_Scalar | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_OneofDescriptorProto_has_options(const google_protobuf_OneofDescriptorProto* msg) {
  const upb_MiniTableField field = {2, UPB_SIZE(4, 24), 2, 0, 11, kUpb_FieldMode_Scalar | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}

UPB_INLINE void google_protobuf_OneofDescriptorProto_set_name(google_protobuf_OneofDescriptorProto *msg, upb_StringView value) {
  const upb_MiniTableField field = {1, 8, 1, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_OneofDescriptorProto_set_options(google_protobuf_OneofDescriptorProto *msg, google_protobuf_OneofOptions* value) {
  const upb_MiniTableField field = {2, UPB_SIZE(4, 24), 2, 0, 11, kUpb_FieldMode_Scalar | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE struct google_protobuf_OneofOptions* google_protobuf_OneofDescriptorProto_mutable_options(google_protobuf_OneofDescriptorProto* msg, upb_Arena* arena) {
  struct google_protobuf_OneofOptions* sub = (struct google_protobuf_OneofOptions*)google_protobuf_OneofDescriptorProto_options(msg);
  if (sub == NULL) {
    sub = (struct google_protobuf_OneofOptions*)_upb_Message_New(&google_protobuf_OneofOptions_msg_init, arena);
    if (sub) google_protobuf_OneofDescriptorProto_set_options(msg, sub);
  }
  return sub;
}

/* google.protobuf.EnumDescriptorProto */

UPB_INLINE google_protobuf_EnumDescriptorProto* google_protobuf_EnumDescriptorProto_new(upb_Arena* arena) {
  return (google_protobuf_EnumDescriptorProto*)_upb_Message_New(&google_protobuf_EnumDescriptorProto_msg_init, arena);
}
UPB_INLINE google_protobuf_EnumDescriptorProto* google_protobuf_EnumDescriptorProto_parse(const char* buf, size_t size, upb_Arena* arena) {
  google_protobuf_EnumDescriptorProto* ret = google_protobuf_EnumDescriptorProto_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_EnumDescriptorProto_msg_init, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE google_protobuf_EnumDescriptorProto* google_protobuf_EnumDescriptorProto_parse_ex(const char* buf, size_t size,
                           const upb_ExtensionRegistry* extreg,
                           int options, upb_Arena* arena) {
  google_protobuf_EnumDescriptorProto* ret = google_protobuf_EnumDescriptorProto_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_EnumDescriptorProto_msg_init, extreg, options, arena) !=
      kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE char* google_protobuf_EnumDescriptorProto_serialize(const google_protobuf_EnumDescriptorProto* msg, upb_Arena* arena, size_t* len) {
  char* ptr;
  (void)upb_Encode(msg, &google_protobuf_EnumDescriptorProto_msg_init, 0, arena, &ptr, len);
  return ptr;
}
UPB_INLINE char* google_protobuf_EnumDescriptorProto_serialize_ex(const google_protobuf_EnumDescriptorProto* msg, int options,
                                 upb_Arena* arena, size_t* len) {
  char* ptr;
  (void)upb_Encode(msg, &google_protobuf_EnumDescriptorProto_msg_init, options, arena, &ptr, len);
  return ptr;
}
UPB_INLINE void google_protobuf_EnumDescriptorProto_clear_name(google_protobuf_EnumDescriptorProto* msg) {
  const upb_MiniTableField field = {1, UPB_SIZE(20, 8), 1, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE upb_StringView google_protobuf_EnumDescriptorProto_name(const google_protobuf_EnumDescriptorProto* msg) {
  upb_StringView default_val = upb_StringView_FromString("");
  upb_StringView ret;
  const upb_MiniTableField field = {1, UPB_SIZE(20, 8), 1, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_EnumDescriptorProto_has_name(const google_protobuf_EnumDescriptorProto* msg) {
  const upb_MiniTableField field = {1, UPB_SIZE(20, 8), 1, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_EnumDescriptorProto_clear_value(google_protobuf_EnumDescriptorProto* msg) {
  const upb_MiniTableField field = {2, UPB_SIZE(4, 24), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE const google_protobuf_EnumValueDescriptorProto* const* google_protobuf_EnumDescriptorProto_value(const google_protobuf_EnumDescriptorProto* msg, size_t* size) {
  const upb_MiniTableField field = {2, UPB_SIZE(4, 24), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (const google_protobuf_EnumValueDescriptorProto* const*)_upb_array_constptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE const upb_Array* _google_protobuf_EnumDescriptorProto_value_upb_array(const google_protobuf_EnumDescriptorProto* msg, size_t* size) {
  const upb_MiniTableField field = {2, UPB_SIZE(4, 24), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE upb_Array* _google_protobuf_EnumDescriptorProto_value_mutable_upb_array(const google_protobuf_EnumDescriptorProto* msg, size_t* size, upb_Arena* arena) {
  const upb_MiniTableField field = {2, UPB_SIZE(4, 24), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(
      (upb_Message*)msg, &field, arena);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE bool google_protobuf_EnumDescriptorProto_has_value(const google_protobuf_EnumDescriptorProto* msg) {
  size_t size;
  google_protobuf_EnumDescriptorProto_value(msg, &size);
  return size != 0;
}
UPB_INLINE void google_protobuf_EnumDescriptorProto_clear_options(google_protobuf_EnumDescriptorProto* msg) {
  const upb_MiniTableField field = {3, UPB_SIZE(8, 32), 2, 1, 11, kUpb_FieldMode_Scalar | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE const google_protobuf_EnumOptions* google_protobuf_EnumDescriptorProto_options(const google_protobuf_EnumDescriptorProto* msg) {
  const google_protobuf_EnumOptions* default_val = NULL;
  const google_protobuf_EnumOptions* ret;
  const upb_MiniTableField field = {3, UPB_SIZE(8, 32), 2, 1, 11, kUpb_FieldMode_Scalar | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_EnumDescriptorProto_has_options(const google_protobuf_EnumDescriptorProto* msg) {
  const upb_MiniTableField field = {3, UPB_SIZE(8, 32), 2, 1, 11, kUpb_FieldMode_Scalar | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_EnumDescriptorProto_clear_reserved_range(google_protobuf_EnumDescriptorProto* msg) {
  const upb_MiniTableField field = {4, UPB_SIZE(12, 40), 0, 2, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE const google_protobuf_EnumDescriptorProto_EnumReservedRange* const* google_protobuf_EnumDescriptorProto_reserved_range(const google_protobuf_EnumDescriptorProto* msg, size_t* size) {
  const upb_MiniTableField field = {4, UPB_SIZE(12, 40), 0, 2, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (const google_protobuf_EnumDescriptorProto_EnumReservedRange* const*)_upb_array_constptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE const upb_Array* _google_protobuf_EnumDescriptorProto_reserved_range_upb_array(const google_protobuf_EnumDescriptorProto* msg, size_t* size) {
  const upb_MiniTableField field = {4, UPB_SIZE(12, 40), 0, 2, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE upb_Array* _google_protobuf_EnumDescriptorProto_reserved_range_mutable_upb_array(const google_protobuf_EnumDescriptorProto* msg, size_t* size, upb_Arena* arena) {
  const upb_MiniTableField field = {4, UPB_SIZE(12, 40), 0, 2, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(
      (upb_Message*)msg, &field, arena);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE bool google_protobuf_EnumDescriptorProto_has_reserved_range(const google_protobuf_EnumDescriptorProto* msg) {
  size_t size;
  google_protobuf_EnumDescriptorProto_reserved_range(msg, &size);
  return size != 0;
}
UPB_INLINE void google_protobuf_EnumDescriptorProto_clear_reserved_name(google_protobuf_EnumDescriptorProto* msg) {
  const upb_MiniTableField field = {5, UPB_SIZE(16, 48), 0, kUpb_NoSub, 12, kUpb_FieldMode_Array | kUpb_LabelFlags_IsAlternate | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE upb_StringView const* google_protobuf_EnumDescriptorProto_reserved_name(const google_protobuf_EnumDescriptorProto* msg, size_t* size) {
  const upb_MiniTableField field = {5, UPB_SIZE(16, 48), 0, kUpb_NoSub, 12, kUpb_FieldMode_Array | kUpb_LabelFlags_IsAlternate | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (upb_StringView const*)_upb_array_constptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE const upb_Array* _google_protobuf_EnumDescriptorProto_reserved_name_upb_array(const google_protobuf_EnumDescriptorProto* msg, size_t* size) {
  const upb_MiniTableField field = {5, UPB_SIZE(16, 48), 0, kUpb_NoSub, 12, kUpb_FieldMode_Array | kUpb_LabelFlags_IsAlternate | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE upb_Array* _google_protobuf_EnumDescriptorProto_reserved_name_mutable_upb_array(const google_protobuf_EnumDescriptorProto* msg, size_t* size, upb_Arena* arena) {
  const upb_MiniTableField field = {5, UPB_SIZE(16, 48), 0, kUpb_NoSub, 12, kUpb_FieldMode_Array | kUpb_LabelFlags_IsAlternate | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(
      (upb_Message*)msg, &field, arena);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE bool google_protobuf_EnumDescriptorProto_has_reserved_name(const google_protobuf_EnumDescriptorProto* msg) {
  size_t size;
  google_protobuf_EnumDescriptorProto_reserved_name(msg, &size);
  return size != 0;
}

UPB_INLINE void google_protobuf_EnumDescriptorProto_set_name(google_protobuf_EnumDescriptorProto *msg, upb_StringView value) {
  const upb_MiniTableField field = {1, UPB_SIZE(20, 8), 1, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE google_protobuf_EnumValueDescriptorProto** google_protobuf_EnumDescriptorProto_mutable_value(google_protobuf_EnumDescriptorProto* msg, size_t* size) {
  upb_MiniTableField field = {2, UPB_SIZE(4, 24), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetMutableArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (google_protobuf_EnumValueDescriptorProto**)_upb_array_ptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE google_protobuf_EnumValueDescriptorProto** google_protobuf_EnumDescriptorProto_resize_value(google_protobuf_EnumDescriptorProto* msg, size_t size, upb_Arena* arena) {
  upb_MiniTableField field = {2, UPB_SIZE(4, 24), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  return (google_protobuf_EnumValueDescriptorProto**)upb_Message_ResizeArray(msg, &field, size, arena);
}
UPB_INLINE struct google_protobuf_EnumValueDescriptorProto* google_protobuf_EnumDescriptorProto_add_value(google_protobuf_EnumDescriptorProto* msg, upb_Arena* arena) {
  upb_MiniTableField field = {2, UPB_SIZE(4, 24), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(msg, &field, arena);
  if (!arr || !_upb_Array_ResizeUninitialized(arr, arr->size + 1, arena)) {
    return NULL;
  }
  struct google_protobuf_EnumValueDescriptorProto* sub = (struct google_protobuf_EnumValueDescriptorProto*)_upb_Message_New(&google_protobuf_EnumValueDescriptorProto_msg_init, arena);
  if (!arr || !sub) return NULL;
  _upb_Array_Set(arr, arr->size - 1, &sub, sizeof(sub));
  return sub;
}
UPB_INLINE void google_protobuf_EnumDescriptorProto_set_options(google_protobuf_EnumDescriptorProto *msg, google_protobuf_EnumOptions* value) {
  const upb_MiniTableField field = {3, UPB_SIZE(8, 32), 2, 1, 11, kUpb_FieldMode_Scalar | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE struct google_protobuf_EnumOptions* google_protobuf_EnumDescriptorProto_mutable_options(google_protobuf_EnumDescriptorProto* msg, upb_Arena* arena) {
  struct google_protobuf_EnumOptions* sub = (struct google_protobuf_EnumOptions*)google_protobuf_EnumDescriptorProto_options(msg);
  if (sub == NULL) {
    sub = (struct google_protobuf_EnumOptions*)_upb_Message_New(&google_protobuf_EnumOptions_msg_init, arena);
    if (sub) google_protobuf_EnumDescriptorProto_set_options(msg, sub);
  }
  return sub;
}
UPB_INLINE google_protobuf_EnumDescriptorProto_EnumReservedRange** google_protobuf_EnumDescriptorProto_mutable_reserved_range(google_protobuf_EnumDescriptorProto* msg, size_t* size) {
  upb_MiniTableField field = {4, UPB_SIZE(12, 40), 0, 2, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetMutableArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (google_protobuf_EnumDescriptorProto_EnumReservedRange**)_upb_array_ptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE google_protobuf_EnumDescriptorProto_EnumReservedRange** google_protobuf_EnumDescriptorProto_resize_reserved_range(google_protobuf_EnumDescriptorProto* msg, size_t size, upb_Arena* arena) {
  upb_MiniTableField field = {4, UPB_SIZE(12, 40), 0, 2, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  return (google_protobuf_EnumDescriptorProto_EnumReservedRange**)upb_Message_ResizeArray(msg, &field, size, arena);
}
UPB_INLINE struct google_protobuf_EnumDescriptorProto_EnumReservedRange* google_protobuf_EnumDescriptorProto_add_reserved_range(google_protobuf_EnumDescriptorProto* msg, upb_Arena* arena) {
  upb_MiniTableField field = {4, UPB_SIZE(12, 40), 0, 2, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(msg, &field, arena);
  if (!arr || !_upb_Array_ResizeUninitialized(arr, arr->size + 1, arena)) {
    return NULL;
  }
  struct google_protobuf_EnumDescriptorProto_EnumReservedRange* sub = (struct google_protobuf_EnumDescriptorProto_EnumReservedRange*)_upb_Message_New(&google_protobuf_EnumDescriptorProto_EnumReservedRange_msg_init, arena);
  if (!arr || !sub) return NULL;
  _upb_Array_Set(arr, arr->size - 1, &sub, sizeof(sub));
  return sub;
}
UPB_INLINE upb_StringView* google_protobuf_EnumDescriptorProto_mutable_reserved_name(google_protobuf_EnumDescriptorProto* msg, size_t* size) {
  upb_MiniTableField field = {5, UPB_SIZE(16, 48), 0, kUpb_NoSub, 12, kUpb_FieldMode_Array | kUpb_LabelFlags_IsAlternate | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetMutableArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (upb_StringView*)_upb_array_ptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE upb_StringView* google_protobuf_EnumDescriptorProto_resize_reserved_name(google_protobuf_EnumDescriptorProto* msg, size_t size, upb_Arena* arena) {
  upb_MiniTableField field = {5, UPB_SIZE(16, 48), 0, kUpb_NoSub, 12, kUpb_FieldMode_Array | kUpb_LabelFlags_IsAlternate | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  return (upb_StringView*)upb_Message_ResizeArray(msg, &field, size, arena);
}
UPB_INLINE bool google_protobuf_EnumDescriptorProto_add_reserved_name(google_protobuf_EnumDescriptorProto* msg, upb_StringView val, upb_Arena* arena) {
  upb_MiniTableField field = {5, UPB_SIZE(16, 48), 0, kUpb_NoSub, 12, kUpb_FieldMode_Array | kUpb_LabelFlags_IsAlternate | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(msg, &field, arena);
  if (!arr || !_upb_Array_ResizeUninitialized(arr, arr->size + 1, arena)) {
    return false;
  }
  _upb_Array_Set(arr, arr->size - 1, &val, sizeof(val));
  return true;
}

/* google.protobuf.EnumDescriptorProto.EnumReservedRange */

UPB_INLINE google_protobuf_EnumDescriptorProto_EnumReservedRange* google_protobuf_EnumDescriptorProto_EnumReservedRange_new(upb_Arena* arena) {
  return (google_protobuf_EnumDescriptorProto_EnumReservedRange*)_upb_Message_New(&google_protobuf_EnumDescriptorProto_EnumReservedRange_msg_init, arena);
}
UPB_INLINE google_protobuf_EnumDescriptorProto_EnumReservedRange* google_protobuf_EnumDescriptorProto_EnumReservedRange_parse(const char* buf, size_t size, upb_Arena* arena) {
  google_protobuf_EnumDescriptorProto_EnumReservedRange* ret = google_protobuf_EnumDescriptorProto_EnumReservedRange_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_EnumDescriptorProto_EnumReservedRange_msg_init, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE google_protobuf_EnumDescriptorProto_EnumReservedRange* google_protobuf_EnumDescriptorProto_EnumReservedRange_parse_ex(const char* buf, size_t size,
                           const upb_ExtensionRegistry* extreg,
                           int options, upb_Arena* arena) {
  google_protobuf_EnumDescriptorProto_EnumReservedRange* ret = google_protobuf_EnumDescriptorProto_EnumReservedRange_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_EnumDescriptorProto_EnumReservedRange_msg_init, extreg, options, arena) !=
      kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE char* google_protobuf_EnumDescriptorProto_EnumReservedRange_serialize(const google_protobuf_EnumDescriptorProto_EnumReservedRange* msg, upb_Arena* arena, size_t* len) {
  char* ptr;
  (void)upb_Encode(msg, &google_protobuf_EnumDescriptorProto_EnumReservedRange_msg_init, 0, arena, &ptr, len);
  return ptr;
}
UPB_INLINE char* google_protobuf_EnumDescriptorProto_EnumReservedRange_serialize_ex(const google_protobuf_EnumDescriptorProto_EnumReservedRange* msg, int options,
                                 upb_Arena* arena, size_t* len) {
  char* ptr;
  (void)upb_Encode(msg, &google_protobuf_EnumDescriptorProto_EnumReservedRange_msg_init, options, arena, &ptr, len);
  return ptr;
}
UPB_INLINE void google_protobuf_EnumDescriptorProto_EnumReservedRange_clear_start(google_protobuf_EnumDescriptorProto_EnumReservedRange* msg) {
  const upb_MiniTableField field = {1, 4, 1, kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE int32_t google_protobuf_EnumDescriptorProto_EnumReservedRange_start(const google_protobuf_EnumDescriptorProto_EnumReservedRange* msg) {
  int32_t default_val = (int32_t)0;
  int32_t ret;
  const upb_MiniTableField field = {1, 4, 1, kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_EnumDescriptorProto_EnumReservedRange_has_start(const google_protobuf_EnumDescriptorProto_EnumReservedRange* msg) {
  const upb_MiniTableField field = {1, 4, 1, kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_EnumDescriptorProto_EnumReservedRange_clear_end(google_protobuf_EnumDescriptorProto_EnumReservedRange* msg) {
  const upb_MiniTableField field = {2, 8, 2, kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE int32_t google_protobuf_EnumDescriptorProto_EnumReservedRange_end(const google_protobuf_EnumDescriptorProto_EnumReservedRange* msg) {
  int32_t default_val = (int32_t)0;
  int32_t ret;
  const upb_MiniTableField field = {2, 8, 2, kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_EnumDescriptorProto_EnumReservedRange_has_end(const google_protobuf_EnumDescriptorProto_EnumReservedRange* msg) {
  const upb_MiniTableField field = {2, 8, 2, kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}

UPB_INLINE void google_protobuf_EnumDescriptorProto_EnumReservedRange_set_start(google_protobuf_EnumDescriptorProto_EnumReservedRange *msg, int32_t value) {
  const upb_MiniTableField field = {1, 4, 1, kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_EnumDescriptorProto_EnumReservedRange_set_end(google_protobuf_EnumDescriptorProto_EnumReservedRange *msg, int32_t value) {
  const upb_MiniTableField field = {2, 8, 2, kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}

/* google.protobuf.EnumValueDescriptorProto */

UPB_INLINE google_protobuf_EnumValueDescriptorProto* google_protobuf_EnumValueDescriptorProto_new(upb_Arena* arena) {
  return (google_protobuf_EnumValueDescriptorProto*)_upb_Message_New(&google_protobuf_EnumValueDescriptorProto_msg_init, arena);
}
UPB_INLINE google_protobuf_EnumValueDescriptorProto* google_protobuf_EnumValueDescriptorProto_parse(const char* buf, size_t size, upb_Arena* arena) {
  google_protobuf_EnumValueDescriptorProto* ret = google_protobuf_EnumValueDescriptorProto_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_EnumValueDescriptorProto_msg_init, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE google_protobuf_EnumValueDescriptorProto* google_protobuf_EnumValueDescriptorProto_parse_ex(const char* buf, size_t size,
                           const upb_ExtensionRegistry* extreg,
                           int options, upb_Arena* arena) {
  google_protobuf_EnumValueDescriptorProto* ret = google_protobuf_EnumValueDescriptorProto_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_EnumValueDescriptorProto_msg_init, extreg, options, arena) !=
      kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE char* google_protobuf_EnumValueDescriptorProto_serialize(const google_protobuf_EnumValueDescriptorProto* msg, upb_Arena* arena, size_t* len) {
  char* ptr;
  (void)upb_Encode(msg, &google_protobuf_EnumValueDescriptorProto_msg_init, 0, arena, &ptr, len);
  return ptr;
}
UPB_INLINE char* google_protobuf_EnumValueDescriptorProto_serialize_ex(const google_protobuf_EnumValueDescriptorProto* msg, int options,
                                 upb_Arena* arena, size_t* len) {
  char* ptr;
  (void)upb_Encode(msg, &google_protobuf_EnumValueDescriptorProto_msg_init, options, arena, &ptr, len);
  return ptr;
}
UPB_INLINE void google_protobuf_EnumValueDescriptorProto_clear_name(google_protobuf_EnumValueDescriptorProto* msg) {
  const upb_MiniTableField field = {1, UPB_SIZE(12, 8), 1, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE upb_StringView google_protobuf_EnumValueDescriptorProto_name(const google_protobuf_EnumValueDescriptorProto* msg) {
  upb_StringView default_val = upb_StringView_FromString("");
  upb_StringView ret;
  const upb_MiniTableField field = {1, UPB_SIZE(12, 8), 1, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_EnumValueDescriptorProto_has_name(const google_protobuf_EnumValueDescriptorProto* msg) {
  const upb_MiniTableField field = {1, UPB_SIZE(12, 8), 1, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_EnumValueDescriptorProto_clear_number(google_protobuf_EnumValueDescriptorProto* msg) {
  const upb_MiniTableField field = {2, 4, 2, kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE int32_t google_protobuf_EnumValueDescriptorProto_number(const google_protobuf_EnumValueDescriptorProto* msg) {
  int32_t default_val = (int32_t)0;
  int32_t ret;
  const upb_MiniTableField field = {2, 4, 2, kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_EnumValueDescriptorProto_has_number(const google_protobuf_EnumValueDescriptorProto* msg) {
  const upb_MiniTableField field = {2, 4, 2, kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_EnumValueDescriptorProto_clear_options(google_protobuf_EnumValueDescriptorProto* msg) {
  const upb_MiniTableField field = {3, UPB_SIZE(8, 24), 3, 0, 11, kUpb_FieldMode_Scalar | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE const google_protobuf_EnumValueOptions* google_protobuf_EnumValueDescriptorProto_options(const google_protobuf_EnumValueDescriptorProto* msg) {
  const google_protobuf_EnumValueOptions* default_val = NULL;
  const google_protobuf_EnumValueOptions* ret;
  const upb_MiniTableField field = {3, UPB_SIZE(8, 24), 3, 0, 11, kUpb_FieldMode_Scalar | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_EnumValueDescriptorProto_has_options(const google_protobuf_EnumValueDescriptorProto* msg) {
  const upb_MiniTableField field = {3, UPB_SIZE(8, 24), 3, 0, 11, kUpb_FieldMode_Scalar | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}

UPB_INLINE void google_protobuf_EnumValueDescriptorProto_set_name(google_protobuf_EnumValueDescriptorProto *msg, upb_StringView value) {
  const upb_MiniTableField field = {1, UPB_SIZE(12, 8), 1, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_EnumValueDescriptorProto_set_number(google_protobuf_EnumValueDescriptorProto *msg, int32_t value) {
  const upb_MiniTableField field = {2, 4, 2, kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_EnumValueDescriptorProto_set_options(google_protobuf_EnumValueDescriptorProto *msg, google_protobuf_EnumValueOptions* value) {
  const upb_MiniTableField field = {3, UPB_SIZE(8, 24), 3, 0, 11, kUpb_FieldMode_Scalar | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE struct google_protobuf_EnumValueOptions* google_protobuf_EnumValueDescriptorProto_mutable_options(google_protobuf_EnumValueDescriptorProto* msg, upb_Arena* arena) {
  struct google_protobuf_EnumValueOptions* sub = (struct google_protobuf_EnumValueOptions*)google_protobuf_EnumValueDescriptorProto_options(msg);
  if (sub == NULL) {
    sub = (struct google_protobuf_EnumValueOptions*)_upb_Message_New(&google_protobuf_EnumValueOptions_msg_init, arena);
    if (sub) google_protobuf_EnumValueDescriptorProto_set_options(msg, sub);
  }
  return sub;
}

/* google.protobuf.ServiceDescriptorProto */

UPB_INLINE google_protobuf_ServiceDescriptorProto* google_protobuf_ServiceDescriptorProto_new(upb_Arena* arena) {
  return (google_protobuf_ServiceDescriptorProto*)_upb_Message_New(&google_protobuf_ServiceDescriptorProto_msg_init, arena);
}
UPB_INLINE google_protobuf_ServiceDescriptorProto* google_protobuf_ServiceDescriptorProto_parse(const char* buf, size_t size, upb_Arena* arena) {
  google_protobuf_ServiceDescriptorProto* ret = google_protobuf_ServiceDescriptorProto_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_ServiceDescriptorProto_msg_init, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE google_protobuf_ServiceDescriptorProto* google_protobuf_ServiceDescriptorProto_parse_ex(const char* buf, size_t size,
                           const upb_ExtensionRegistry* extreg,
                           int options, upb_Arena* arena) {
  google_protobuf_ServiceDescriptorProto* ret = google_protobuf_ServiceDescriptorProto_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_ServiceDescriptorProto_msg_init, extreg, options, arena) !=
      kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE char* google_protobuf_ServiceDescriptorProto_serialize(const google_protobuf_ServiceDescriptorProto* msg, upb_Arena* arena, size_t* len) {
  char* ptr;
  (void)upb_Encode(msg, &google_protobuf_ServiceDescriptorProto_msg_init, 0, arena, &ptr, len);
  return ptr;
}
UPB_INLINE char* google_protobuf_ServiceDescriptorProto_serialize_ex(const google_protobuf_ServiceDescriptorProto* msg, int options,
                                 upb_Arena* arena, size_t* len) {
  char* ptr;
  (void)upb_Encode(msg, &google_protobuf_ServiceDescriptorProto_msg_init, options, arena, &ptr, len);
  return ptr;
}
UPB_INLINE void google_protobuf_ServiceDescriptorProto_clear_name(google_protobuf_ServiceDescriptorProto* msg) {
  const upb_MiniTableField field = {1, UPB_SIZE(12, 8), 1, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE upb_StringView google_protobuf_ServiceDescriptorProto_name(const google_protobuf_ServiceDescriptorProto* msg) {
  upb_StringView default_val = upb_StringView_FromString("");
  upb_StringView ret;
  const upb_MiniTableField field = {1, UPB_SIZE(12, 8), 1, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_ServiceDescriptorProto_has_name(const google_protobuf_ServiceDescriptorProto* msg) {
  const upb_MiniTableField field = {1, UPB_SIZE(12, 8), 1, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_ServiceDescriptorProto_clear_method(google_protobuf_ServiceDescriptorProto* msg) {
  const upb_MiniTableField field = {2, UPB_SIZE(4, 24), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE const google_protobuf_MethodDescriptorProto* const* google_protobuf_ServiceDescriptorProto_method(const google_protobuf_ServiceDescriptorProto* msg, size_t* size) {
  const upb_MiniTableField field = {2, UPB_SIZE(4, 24), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (const google_protobuf_MethodDescriptorProto* const*)_upb_array_constptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE const upb_Array* _google_protobuf_ServiceDescriptorProto_method_upb_array(const google_protobuf_ServiceDescriptorProto* msg, size_t* size) {
  const upb_MiniTableField field = {2, UPB_SIZE(4, 24), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE upb_Array* _google_protobuf_ServiceDescriptorProto_method_mutable_upb_array(const google_protobuf_ServiceDescriptorProto* msg, size_t* size, upb_Arena* arena) {
  const upb_MiniTableField field = {2, UPB_SIZE(4, 24), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(
      (upb_Message*)msg, &field, arena);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE bool google_protobuf_ServiceDescriptorProto_has_method(const google_protobuf_ServiceDescriptorProto* msg) {
  size_t size;
  google_protobuf_ServiceDescriptorProto_method(msg, &size);
  return size != 0;
}
UPB_INLINE void google_protobuf_ServiceDescriptorProto_clear_options(google_protobuf_ServiceDescriptorProto* msg) {
  const upb_MiniTableField field = {3, UPB_SIZE(8, 32), 2, 1, 11, kUpb_FieldMode_Scalar | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE const google_protobuf_ServiceOptions* google_protobuf_ServiceDescriptorProto_options(const google_protobuf_ServiceDescriptorProto* msg) {
  const google_protobuf_ServiceOptions* default_val = NULL;
  const google_protobuf_ServiceOptions* ret;
  const upb_MiniTableField field = {3, UPB_SIZE(8, 32), 2, 1, 11, kUpb_FieldMode_Scalar | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_ServiceDescriptorProto_has_options(const google_protobuf_ServiceDescriptorProto* msg) {
  const upb_MiniTableField field = {3, UPB_SIZE(8, 32), 2, 1, 11, kUpb_FieldMode_Scalar | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}

UPB_INLINE void google_protobuf_ServiceDescriptorProto_set_name(google_protobuf_ServiceDescriptorProto *msg, upb_StringView value) {
  const upb_MiniTableField field = {1, UPB_SIZE(12, 8), 1, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE google_protobuf_MethodDescriptorProto** google_protobuf_ServiceDescriptorProto_mutable_method(google_protobuf_ServiceDescriptorProto* msg, size_t* size) {
  upb_MiniTableField field = {2, UPB_SIZE(4, 24), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetMutableArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (google_protobuf_MethodDescriptorProto**)_upb_array_ptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE google_protobuf_MethodDescriptorProto** google_protobuf_ServiceDescriptorProto_resize_method(google_protobuf_ServiceDescriptorProto* msg, size_t size, upb_Arena* arena) {
  upb_MiniTableField field = {2, UPB_SIZE(4, 24), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  return (google_protobuf_MethodDescriptorProto**)upb_Message_ResizeArray(msg, &field, size, arena);
}
UPB_INLINE struct google_protobuf_MethodDescriptorProto* google_protobuf_ServiceDescriptorProto_add_method(google_protobuf_ServiceDescriptorProto* msg, upb_Arena* arena) {
  upb_MiniTableField field = {2, UPB_SIZE(4, 24), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(msg, &field, arena);
  if (!arr || !_upb_Array_ResizeUninitialized(arr, arr->size + 1, arena)) {
    return NULL;
  }
  struct google_protobuf_MethodDescriptorProto* sub = (struct google_protobuf_MethodDescriptorProto*)_upb_Message_New(&google_protobuf_MethodDescriptorProto_msg_init, arena);
  if (!arr || !sub) return NULL;
  _upb_Array_Set(arr, arr->size - 1, &sub, sizeof(sub));
  return sub;
}
UPB_INLINE void google_protobuf_ServiceDescriptorProto_set_options(google_protobuf_ServiceDescriptorProto *msg, google_protobuf_ServiceOptions* value) {
  const upb_MiniTableField field = {3, UPB_SIZE(8, 32), 2, 1, 11, kUpb_FieldMode_Scalar | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE struct google_protobuf_ServiceOptions* google_protobuf_ServiceDescriptorProto_mutable_options(google_protobuf_ServiceDescriptorProto* msg, upb_Arena* arena) {
  struct google_protobuf_ServiceOptions* sub = (struct google_protobuf_ServiceOptions*)google_protobuf_ServiceDescriptorProto_options(msg);
  if (sub == NULL) {
    sub = (struct google_protobuf_ServiceOptions*)_upb_Message_New(&google_protobuf_ServiceOptions_msg_init, arena);
    if (sub) google_protobuf_ServiceDescriptorProto_set_options(msg, sub);
  }
  return sub;
}

/* google.protobuf.MethodDescriptorProto */

UPB_INLINE google_protobuf_MethodDescriptorProto* google_protobuf_MethodDescriptorProto_new(upb_Arena* arena) {
  return (google_protobuf_MethodDescriptorProto*)_upb_Message_New(&google_protobuf_MethodDescriptorProto_msg_init, arena);
}
UPB_INLINE google_protobuf_MethodDescriptorProto* google_protobuf_MethodDescriptorProto_parse(const char* buf, size_t size, upb_Arena* arena) {
  google_protobuf_MethodDescriptorProto* ret = google_protobuf_MethodDescriptorProto_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_MethodDescriptorProto_msg_init, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE google_protobuf_MethodDescriptorProto* google_protobuf_MethodDescriptorProto_parse_ex(const char* buf, size_t size,
                           const upb_ExtensionRegistry* extreg,
                           int options, upb_Arena* arena) {
  google_protobuf_MethodDescriptorProto* ret = google_protobuf_MethodDescriptorProto_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_MethodDescriptorProto_msg_init, extreg, options, arena) !=
      kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE char* google_protobuf_MethodDescriptorProto_serialize(const google_protobuf_MethodDescriptorProto* msg, upb_Arena* arena, size_t* len) {
  char* ptr;
  (void)upb_Encode(msg, &google_protobuf_MethodDescriptorProto_msg_init, 0, arena, &ptr, len);
  return ptr;
}
UPB_INLINE char* google_protobuf_MethodDescriptorProto_serialize_ex(const google_protobuf_MethodDescriptorProto* msg, int options,
                                 upb_Arena* arena, size_t* len) {
  char* ptr;
  (void)upb_Encode(msg, &google_protobuf_MethodDescriptorProto_msg_init, options, arena, &ptr, len);
  return ptr;
}
UPB_INLINE void google_protobuf_MethodDescriptorProto_clear_name(google_protobuf_MethodDescriptorProto* msg) {
  const upb_MiniTableField field = {1, UPB_SIZE(12, 8), 1, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE upb_StringView google_protobuf_MethodDescriptorProto_name(const google_protobuf_MethodDescriptorProto* msg) {
  upb_StringView default_val = upb_StringView_FromString("");
  upb_StringView ret;
  const upb_MiniTableField field = {1, UPB_SIZE(12, 8), 1, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_MethodDescriptorProto_has_name(const google_protobuf_MethodDescriptorProto* msg) {
  const upb_MiniTableField field = {1, UPB_SIZE(12, 8), 1, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_MethodDescriptorProto_clear_input_type(google_protobuf_MethodDescriptorProto* msg) {
  const upb_MiniTableField field = {2, UPB_SIZE(20, 24), 2, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE upb_StringView google_protobuf_MethodDescriptorProto_input_type(const google_protobuf_MethodDescriptorProto* msg) {
  upb_StringView default_val = upb_StringView_FromString("");
  upb_StringView ret;
  const upb_MiniTableField field = {2, UPB_SIZE(20, 24), 2, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_MethodDescriptorProto_has_input_type(const google_protobuf_MethodDescriptorProto* msg) {
  const upb_MiniTableField field = {2, UPB_SIZE(20, 24), 2, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_MethodDescriptorProto_clear_output_type(google_protobuf_MethodDescriptorProto* msg) {
  const upb_MiniTableField field = {3, UPB_SIZE(28, 40), 3, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE upb_StringView google_protobuf_MethodDescriptorProto_output_type(const google_protobuf_MethodDescriptorProto* msg) {
  upb_StringView default_val = upb_StringView_FromString("");
  upb_StringView ret;
  const upb_MiniTableField field = {3, UPB_SIZE(28, 40), 3, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_MethodDescriptorProto_has_output_type(const google_protobuf_MethodDescriptorProto* msg) {
  const upb_MiniTableField field = {3, UPB_SIZE(28, 40), 3, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_MethodDescriptorProto_clear_options(google_protobuf_MethodDescriptorProto* msg) {
  const upb_MiniTableField field = {4, UPB_SIZE(4, 56), 4, 0, 11, kUpb_FieldMode_Scalar | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE const google_protobuf_MethodOptions* google_protobuf_MethodDescriptorProto_options(const google_protobuf_MethodDescriptorProto* msg) {
  const google_protobuf_MethodOptions* default_val = NULL;
  const google_protobuf_MethodOptions* ret;
  const upb_MiniTableField field = {4, UPB_SIZE(4, 56), 4, 0, 11, kUpb_FieldMode_Scalar | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_MethodDescriptorProto_has_options(const google_protobuf_MethodDescriptorProto* msg) {
  const upb_MiniTableField field = {4, UPB_SIZE(4, 56), 4, 0, 11, kUpb_FieldMode_Scalar | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_MethodDescriptorProto_clear_client_streaming(google_protobuf_MethodDescriptorProto* msg) {
  const upb_MiniTableField field = {5, UPB_SIZE(8, 1), 5, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE bool google_protobuf_MethodDescriptorProto_client_streaming(const google_protobuf_MethodDescriptorProto* msg) {
  bool default_val = false;
  bool ret;
  const upb_MiniTableField field = {5, UPB_SIZE(8, 1), 5, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_MethodDescriptorProto_has_client_streaming(const google_protobuf_MethodDescriptorProto* msg) {
  const upb_MiniTableField field = {5, UPB_SIZE(8, 1), 5, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_MethodDescriptorProto_clear_server_streaming(google_protobuf_MethodDescriptorProto* msg) {
  const upb_MiniTableField field = {6, UPB_SIZE(9, 2), 6, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE bool google_protobuf_MethodDescriptorProto_server_streaming(const google_protobuf_MethodDescriptorProto* msg) {
  bool default_val = false;
  bool ret;
  const upb_MiniTableField field = {6, UPB_SIZE(9, 2), 6, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_MethodDescriptorProto_has_server_streaming(const google_protobuf_MethodDescriptorProto* msg) {
  const upb_MiniTableField field = {6, UPB_SIZE(9, 2), 6, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}

UPB_INLINE void google_protobuf_MethodDescriptorProto_set_name(google_protobuf_MethodDescriptorProto *msg, upb_StringView value) {
  const upb_MiniTableField field = {1, UPB_SIZE(12, 8), 1, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_MethodDescriptorProto_set_input_type(google_protobuf_MethodDescriptorProto *msg, upb_StringView value) {
  const upb_MiniTableField field = {2, UPB_SIZE(20, 24), 2, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_MethodDescriptorProto_set_output_type(google_protobuf_MethodDescriptorProto *msg, upb_StringView value) {
  const upb_MiniTableField field = {3, UPB_SIZE(28, 40), 3, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_MethodDescriptorProto_set_options(google_protobuf_MethodDescriptorProto *msg, google_protobuf_MethodOptions* value) {
  const upb_MiniTableField field = {4, UPB_SIZE(4, 56), 4, 0, 11, kUpb_FieldMode_Scalar | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE struct google_protobuf_MethodOptions* google_protobuf_MethodDescriptorProto_mutable_options(google_protobuf_MethodDescriptorProto* msg, upb_Arena* arena) {
  struct google_protobuf_MethodOptions* sub = (struct google_protobuf_MethodOptions*)google_protobuf_MethodDescriptorProto_options(msg);
  if (sub == NULL) {
    sub = (struct google_protobuf_MethodOptions*)_upb_Message_New(&google_protobuf_MethodOptions_msg_init, arena);
    if (sub) google_protobuf_MethodDescriptorProto_set_options(msg, sub);
  }
  return sub;
}
UPB_INLINE void google_protobuf_MethodDescriptorProto_set_client_streaming(google_protobuf_MethodDescriptorProto *msg, bool value) {
  const upb_MiniTableField field = {5, UPB_SIZE(8, 1), 5, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_MethodDescriptorProto_set_server_streaming(google_protobuf_MethodDescriptorProto *msg, bool value) {
  const upb_MiniTableField field = {6, UPB_SIZE(9, 2), 6, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}

/* google.protobuf.FileOptions */

UPB_INLINE google_protobuf_FileOptions* google_protobuf_FileOptions_new(upb_Arena* arena) {
  return (google_protobuf_FileOptions*)_upb_Message_New(&google_protobuf_FileOptions_msg_init, arena);
}
UPB_INLINE google_protobuf_FileOptions* google_protobuf_FileOptions_parse(const char* buf, size_t size, upb_Arena* arena) {
  google_protobuf_FileOptions* ret = google_protobuf_FileOptions_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_FileOptions_msg_init, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE google_protobuf_FileOptions* google_protobuf_FileOptions_parse_ex(const char* buf, size_t size,
                           const upb_ExtensionRegistry* extreg,
                           int options, upb_Arena* arena) {
  google_protobuf_FileOptions* ret = google_protobuf_FileOptions_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_FileOptions_msg_init, extreg, options, arena) !=
      kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE char* google_protobuf_FileOptions_serialize(const google_protobuf_FileOptions* msg, upb_Arena* arena, size_t* len) {
  char* ptr;
  (void)upb_Encode(msg, &google_protobuf_FileOptions_msg_init, 0, arena, &ptr, len);
  return ptr;
}
UPB_INLINE char* google_protobuf_FileOptions_serialize_ex(const google_protobuf_FileOptions* msg, int options,
                                 upb_Arena* arena, size_t* len) {
  char* ptr;
  (void)upb_Encode(msg, &google_protobuf_FileOptions_msg_init, options, arena, &ptr, len);
  return ptr;
}
UPB_INLINE void google_protobuf_FileOptions_clear_java_package(google_protobuf_FileOptions* msg) {
  const upb_MiniTableField field = {1, 24, 1, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE upb_StringView google_protobuf_FileOptions_java_package(const google_protobuf_FileOptions* msg) {
  upb_StringView default_val = upb_StringView_FromString("");
  upb_StringView ret;
  const upb_MiniTableField field = {1, 24, 1, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_FileOptions_has_java_package(const google_protobuf_FileOptions* msg) {
  const upb_MiniTableField field = {1, 24, 1, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_FileOptions_clear_java_outer_classname(google_protobuf_FileOptions* msg) {
  const upb_MiniTableField field = {8, UPB_SIZE(32, 40), 2, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE upb_StringView google_protobuf_FileOptions_java_outer_classname(const google_protobuf_FileOptions* msg) {
  upb_StringView default_val = upb_StringView_FromString("");
  upb_StringView ret;
  const upb_MiniTableField field = {8, UPB_SIZE(32, 40), 2, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_FileOptions_has_java_outer_classname(const google_protobuf_FileOptions* msg) {
  const upb_MiniTableField field = {8, UPB_SIZE(32, 40), 2, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_FileOptions_clear_optimize_for(google_protobuf_FileOptions* msg) {
  const upb_MiniTableField field = {9, 4, 3, 0, 14, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE int32_t google_protobuf_FileOptions_optimize_for(const google_protobuf_FileOptions* msg) {
  int32_t default_val = 1;
  int32_t ret;
  const upb_MiniTableField field = {9, 4, 3, 0, 14, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_FileOptions_has_optimize_for(const google_protobuf_FileOptions* msg) {
  const upb_MiniTableField field = {9, 4, 3, 0, 14, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_FileOptions_clear_java_multiple_files(google_protobuf_FileOptions* msg) {
  const upb_MiniTableField field = {10, 8, 4, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE bool google_protobuf_FileOptions_java_multiple_files(const google_protobuf_FileOptions* msg) {
  bool default_val = false;
  bool ret;
  const upb_MiniTableField field = {10, 8, 4, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_FileOptions_has_java_multiple_files(const google_protobuf_FileOptions* msg) {
  const upb_MiniTableField field = {10, 8, 4, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_FileOptions_clear_go_package(google_protobuf_FileOptions* msg) {
  const upb_MiniTableField field = {11, UPB_SIZE(40, 56), 5, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE upb_StringView google_protobuf_FileOptions_go_package(const google_protobuf_FileOptions* msg) {
  upb_StringView default_val = upb_StringView_FromString("");
  upb_StringView ret;
  const upb_MiniTableField field = {11, UPB_SIZE(40, 56), 5, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_FileOptions_has_go_package(const google_protobuf_FileOptions* msg) {
  const upb_MiniTableField field = {11, UPB_SIZE(40, 56), 5, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_FileOptions_clear_cc_generic_services(google_protobuf_FileOptions* msg) {
  const upb_MiniTableField field = {16, 9, 6, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE bool google_protobuf_FileOptions_cc_generic_services(const google_protobuf_FileOptions* msg) {
  bool default_val = false;
  bool ret;
  const upb_MiniTableField field = {16, 9, 6, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_FileOptions_has_cc_generic_services(const google_protobuf_FileOptions* msg) {
  const upb_MiniTableField field = {16, 9, 6, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_FileOptions_clear_java_generic_services(google_protobuf_FileOptions* msg) {
  const upb_MiniTableField field = {17, 10, 7, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE bool google_protobuf_FileOptions_java_generic_services(const google_protobuf_FileOptions* msg) {
  bool default_val = false;
  bool ret;
  const upb_MiniTableField field = {17, 10, 7, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_FileOptions_has_java_generic_services(const google_protobuf_FileOptions* msg) {
  const upb_MiniTableField field = {17, 10, 7, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_FileOptions_clear_py_generic_services(google_protobuf_FileOptions* msg) {
  const upb_MiniTableField field = {18, 11, 8, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE bool google_protobuf_FileOptions_py_generic_services(const google_protobuf_FileOptions* msg) {
  bool default_val = false;
  bool ret;
  const upb_MiniTableField field = {18, 11, 8, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_FileOptions_has_py_generic_services(const google_protobuf_FileOptions* msg) {
  const upb_MiniTableField field = {18, 11, 8, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_FileOptions_clear_java_generate_equals_and_hash(google_protobuf_FileOptions* msg) {
  const upb_MiniTableField field = {20, 12, 9, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE bool google_protobuf_FileOptions_java_generate_equals_and_hash(const google_protobuf_FileOptions* msg) {
  bool default_val = false;
  bool ret;
  const upb_MiniTableField field = {20, 12, 9, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_FileOptions_has_java_generate_equals_and_hash(const google_protobuf_FileOptions* msg) {
  const upb_MiniTableField field = {20, 12, 9, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_FileOptions_clear_deprecated(google_protobuf_FileOptions* msg) {
  const upb_MiniTableField field = {23, 13, 10, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE bool google_protobuf_FileOptions_deprecated(const google_protobuf_FileOptions* msg) {
  bool default_val = false;
  bool ret;
  const upb_MiniTableField field = {23, 13, 10, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_FileOptions_has_deprecated(const google_protobuf_FileOptions* msg) {
  const upb_MiniTableField field = {23, 13, 10, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_FileOptions_clear_java_string_check_utf8(google_protobuf_FileOptions* msg) {
  const upb_MiniTableField field = {27, 14, 11, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE bool google_protobuf_FileOptions_java_string_check_utf8(const google_protobuf_FileOptions* msg) {
  bool default_val = false;
  bool ret;
  const upb_MiniTableField field = {27, 14, 11, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_FileOptions_has_java_string_check_utf8(const google_protobuf_FileOptions* msg) {
  const upb_MiniTableField field = {27, 14, 11, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_FileOptions_clear_cc_enable_arenas(google_protobuf_FileOptions* msg) {
  const upb_MiniTableField field = {31, 15, 12, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE bool google_protobuf_FileOptions_cc_enable_arenas(const google_protobuf_FileOptions* msg) {
  bool default_val = true;
  bool ret;
  const upb_MiniTableField field = {31, 15, 12, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_FileOptions_has_cc_enable_arenas(const google_protobuf_FileOptions* msg) {
  const upb_MiniTableField field = {31, 15, 12, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_FileOptions_clear_objc_class_prefix(google_protobuf_FileOptions* msg) {
  const upb_MiniTableField field = {36, UPB_SIZE(48, 72), 13, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE upb_StringView google_protobuf_FileOptions_objc_class_prefix(const google_protobuf_FileOptions* msg) {
  upb_StringView default_val = upb_StringView_FromString("");
  upb_StringView ret;
  const upb_MiniTableField field = {36, UPB_SIZE(48, 72), 13, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_FileOptions_has_objc_class_prefix(const google_protobuf_FileOptions* msg) {
  const upb_MiniTableField field = {36, UPB_SIZE(48, 72), 13, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_FileOptions_clear_csharp_namespace(google_protobuf_FileOptions* msg) {
  const upb_MiniTableField field = {37, UPB_SIZE(56, 88), 14, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE upb_StringView google_protobuf_FileOptions_csharp_namespace(const google_protobuf_FileOptions* msg) {
  upb_StringView default_val = upb_StringView_FromString("");
  upb_StringView ret;
  const upb_MiniTableField field = {37, UPB_SIZE(56, 88), 14, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_FileOptions_has_csharp_namespace(const google_protobuf_FileOptions* msg) {
  const upb_MiniTableField field = {37, UPB_SIZE(56, 88), 14, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_FileOptions_clear_swift_prefix(google_protobuf_FileOptions* msg) {
  const upb_MiniTableField field = {39, UPB_SIZE(64, 104), 15, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE upb_StringView google_protobuf_FileOptions_swift_prefix(const google_protobuf_FileOptions* msg) {
  upb_StringView default_val = upb_StringView_FromString("");
  upb_StringView ret;
  const upb_MiniTableField field = {39, UPB_SIZE(64, 104), 15, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_FileOptions_has_swift_prefix(const google_protobuf_FileOptions* msg) {
  const upb_MiniTableField field = {39, UPB_SIZE(64, 104), 15, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_FileOptions_clear_php_class_prefix(google_protobuf_FileOptions* msg) {
  const upb_MiniTableField field = {40, UPB_SIZE(72, 120), 16, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE upb_StringView google_protobuf_FileOptions_php_class_prefix(const google_protobuf_FileOptions* msg) {
  upb_StringView default_val = upb_StringView_FromString("");
  upb_StringView ret;
  const upb_MiniTableField field = {40, UPB_SIZE(72, 120), 16, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_FileOptions_has_php_class_prefix(const google_protobuf_FileOptions* msg) {
  const upb_MiniTableField field = {40, UPB_SIZE(72, 120), 16, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_FileOptions_clear_php_namespace(google_protobuf_FileOptions* msg) {
  const upb_MiniTableField field = {41, UPB_SIZE(80, 136), 17, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE upb_StringView google_protobuf_FileOptions_php_namespace(const google_protobuf_FileOptions* msg) {
  upb_StringView default_val = upb_StringView_FromString("");
  upb_StringView ret;
  const upb_MiniTableField field = {41, UPB_SIZE(80, 136), 17, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_FileOptions_has_php_namespace(const google_protobuf_FileOptions* msg) {
  const upb_MiniTableField field = {41, UPB_SIZE(80, 136), 17, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_FileOptions_clear_php_generic_services(google_protobuf_FileOptions* msg) {
  const upb_MiniTableField field = {42, 16, 18, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE bool google_protobuf_FileOptions_php_generic_services(const google_protobuf_FileOptions* msg) {
  bool default_val = false;
  bool ret;
  const upb_MiniTableField field = {42, 16, 18, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_FileOptions_has_php_generic_services(const google_protobuf_FileOptions* msg) {
  const upb_MiniTableField field = {42, 16, 18, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_FileOptions_clear_php_metadata_namespace(google_protobuf_FileOptions* msg) {
  const upb_MiniTableField field = {44, UPB_SIZE(88, 152), 19, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE upb_StringView google_protobuf_FileOptions_php_metadata_namespace(const google_protobuf_FileOptions* msg) {
  upb_StringView default_val = upb_StringView_FromString("");
  upb_StringView ret;
  const upb_MiniTableField field = {44, UPB_SIZE(88, 152), 19, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_FileOptions_has_php_metadata_namespace(const google_protobuf_FileOptions* msg) {
  const upb_MiniTableField field = {44, UPB_SIZE(88, 152), 19, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_FileOptions_clear_ruby_package(google_protobuf_FileOptions* msg) {
  const upb_MiniTableField field = {45, UPB_SIZE(96, 168), 20, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE upb_StringView google_protobuf_FileOptions_ruby_package(const google_protobuf_FileOptions* msg) {
  upb_StringView default_val = upb_StringView_FromString("");
  upb_StringView ret;
  const upb_MiniTableField field = {45, UPB_SIZE(96, 168), 20, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_FileOptions_has_ruby_package(const google_protobuf_FileOptions* msg) {
  const upb_MiniTableField field = {45, UPB_SIZE(96, 168), 20, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_FileOptions_clear_uninterpreted_option(google_protobuf_FileOptions* msg) {
  const upb_MiniTableField field = {999, UPB_SIZE(20, 184), 0, 1, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE const google_protobuf_UninterpretedOption* const* google_protobuf_FileOptions_uninterpreted_option(const google_protobuf_FileOptions* msg, size_t* size) {
  const upb_MiniTableField field = {999, UPB_SIZE(20, 184), 0, 1, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (const google_protobuf_UninterpretedOption* const*)_upb_array_constptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE const upb_Array* _google_protobuf_FileOptions_uninterpreted_option_upb_array(const google_protobuf_FileOptions* msg, size_t* size) {
  const upb_MiniTableField field = {999, UPB_SIZE(20, 184), 0, 1, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE upb_Array* _google_protobuf_FileOptions_uninterpreted_option_mutable_upb_array(const google_protobuf_FileOptions* msg, size_t* size, upb_Arena* arena) {
  const upb_MiniTableField field = {999, UPB_SIZE(20, 184), 0, 1, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(
      (upb_Message*)msg, &field, arena);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE bool google_protobuf_FileOptions_has_uninterpreted_option(const google_protobuf_FileOptions* msg) {
  size_t size;
  google_protobuf_FileOptions_uninterpreted_option(msg, &size);
  return size != 0;
}

UPB_INLINE void google_protobuf_FileOptions_set_java_package(google_protobuf_FileOptions *msg, upb_StringView value) {
  const upb_MiniTableField field = {1, 24, 1, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_FileOptions_set_java_outer_classname(google_protobuf_FileOptions *msg, upb_StringView value) {
  const upb_MiniTableField field = {8, UPB_SIZE(32, 40), 2, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_FileOptions_set_optimize_for(google_protobuf_FileOptions *msg, int32_t value) {
  const upb_MiniTableField field = {9, 4, 3, 0, 14, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_FileOptions_set_java_multiple_files(google_protobuf_FileOptions *msg, bool value) {
  const upb_MiniTableField field = {10, 8, 4, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_FileOptions_set_go_package(google_protobuf_FileOptions *msg, upb_StringView value) {
  const upb_MiniTableField field = {11, UPB_SIZE(40, 56), 5, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_FileOptions_set_cc_generic_services(google_protobuf_FileOptions *msg, bool value) {
  const upb_MiniTableField field = {16, 9, 6, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_FileOptions_set_java_generic_services(google_protobuf_FileOptions *msg, bool value) {
  const upb_MiniTableField field = {17, 10, 7, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_FileOptions_set_py_generic_services(google_protobuf_FileOptions *msg, bool value) {
  const upb_MiniTableField field = {18, 11, 8, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_FileOptions_set_java_generate_equals_and_hash(google_protobuf_FileOptions *msg, bool value) {
  const upb_MiniTableField field = {20, 12, 9, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_FileOptions_set_deprecated(google_protobuf_FileOptions *msg, bool value) {
  const upb_MiniTableField field = {23, 13, 10, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_FileOptions_set_java_string_check_utf8(google_protobuf_FileOptions *msg, bool value) {
  const upb_MiniTableField field = {27, 14, 11, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_FileOptions_set_cc_enable_arenas(google_protobuf_FileOptions *msg, bool value) {
  const upb_MiniTableField field = {31, 15, 12, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_FileOptions_set_objc_class_prefix(google_protobuf_FileOptions *msg, upb_StringView value) {
  const upb_MiniTableField field = {36, UPB_SIZE(48, 72), 13, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_FileOptions_set_csharp_namespace(google_protobuf_FileOptions *msg, upb_StringView value) {
  const upb_MiniTableField field = {37, UPB_SIZE(56, 88), 14, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_FileOptions_set_swift_prefix(google_protobuf_FileOptions *msg, upb_StringView value) {
  const upb_MiniTableField field = {39, UPB_SIZE(64, 104), 15, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_FileOptions_set_php_class_prefix(google_protobuf_FileOptions *msg, upb_StringView value) {
  const upb_MiniTableField field = {40, UPB_SIZE(72, 120), 16, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_FileOptions_set_php_namespace(google_protobuf_FileOptions *msg, upb_StringView value) {
  const upb_MiniTableField field = {41, UPB_SIZE(80, 136), 17, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_FileOptions_set_php_generic_services(google_protobuf_FileOptions *msg, bool value) {
  const upb_MiniTableField field = {42, 16, 18, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_FileOptions_set_php_metadata_namespace(google_protobuf_FileOptions *msg, upb_StringView value) {
  const upb_MiniTableField field = {44, UPB_SIZE(88, 152), 19, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_FileOptions_set_ruby_package(google_protobuf_FileOptions *msg, upb_StringView value) {
  const upb_MiniTableField field = {45, UPB_SIZE(96, 168), 20, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_FileOptions_mutable_uninterpreted_option(google_protobuf_FileOptions* msg, size_t* size) {
  upb_MiniTableField field = {999, UPB_SIZE(20, 184), 0, 1, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetMutableArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (google_protobuf_UninterpretedOption**)_upb_array_ptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_FileOptions_resize_uninterpreted_option(google_protobuf_FileOptions* msg, size_t size, upb_Arena* arena) {
  upb_MiniTableField field = {999, UPB_SIZE(20, 184), 0, 1, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  return (google_protobuf_UninterpretedOption**)upb_Message_ResizeArray(msg, &field, size, arena);
}
UPB_INLINE struct google_protobuf_UninterpretedOption* google_protobuf_FileOptions_add_uninterpreted_option(google_protobuf_FileOptions* msg, upb_Arena* arena) {
  upb_MiniTableField field = {999, UPB_SIZE(20, 184), 0, 1, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(msg, &field, arena);
  if (!arr || !_upb_Array_ResizeUninitialized(arr, arr->size + 1, arena)) {
    return NULL;
  }
  struct google_protobuf_UninterpretedOption* sub = (struct google_protobuf_UninterpretedOption*)_upb_Message_New(&google_protobuf_UninterpretedOption_msg_init, arena);
  if (!arr || !sub) return NULL;
  _upb_Array_Set(arr, arr->size - 1, &sub, sizeof(sub));
  return sub;
}

/* google.protobuf.MessageOptions */

UPB_INLINE google_protobuf_MessageOptions* google_protobuf_MessageOptions_new(upb_Arena* arena) {
  return (google_protobuf_MessageOptions*)_upb_Message_New(&google_protobuf_MessageOptions_msg_init, arena);
}
UPB_INLINE google_protobuf_MessageOptions* google_protobuf_MessageOptions_parse(const char* buf, size_t size, upb_Arena* arena) {
  google_protobuf_MessageOptions* ret = google_protobuf_MessageOptions_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_MessageOptions_msg_init, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE google_protobuf_MessageOptions* google_protobuf_MessageOptions_parse_ex(const char* buf, size_t size,
                           const upb_ExtensionRegistry* extreg,
                           int options, upb_Arena* arena) {
  google_protobuf_MessageOptions* ret = google_protobuf_MessageOptions_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_MessageOptions_msg_init, extreg, options, arena) !=
      kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE char* google_protobuf_MessageOptions_serialize(const google_protobuf_MessageOptions* msg, upb_Arena* arena, size_t* len) {
  char* ptr;
  (void)upb_Encode(msg, &google_protobuf_MessageOptions_msg_init, 0, arena, &ptr, len);
  return ptr;
}
UPB_INLINE char* google_protobuf_MessageOptions_serialize_ex(const google_protobuf_MessageOptions* msg, int options,
                                 upb_Arena* arena, size_t* len) {
  char* ptr;
  (void)upb_Encode(msg, &google_protobuf_MessageOptions_msg_init, options, arena, &ptr, len);
  return ptr;
}
UPB_INLINE void google_protobuf_MessageOptions_clear_message_set_wire_format(google_protobuf_MessageOptions* msg) {
  const upb_MiniTableField field = {1, 1, 1, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE bool google_protobuf_MessageOptions_message_set_wire_format(const google_protobuf_MessageOptions* msg) {
  bool default_val = false;
  bool ret;
  const upb_MiniTableField field = {1, 1, 1, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_MessageOptions_has_message_set_wire_format(const google_protobuf_MessageOptions* msg) {
  const upb_MiniTableField field = {1, 1, 1, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_MessageOptions_clear_no_standard_descriptor_accessor(google_protobuf_MessageOptions* msg) {
  const upb_MiniTableField field = {2, 2, 2, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE bool google_protobuf_MessageOptions_no_standard_descriptor_accessor(const google_protobuf_MessageOptions* msg) {
  bool default_val = false;
  bool ret;
  const upb_MiniTableField field = {2, 2, 2, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_MessageOptions_has_no_standard_descriptor_accessor(const google_protobuf_MessageOptions* msg) {
  const upb_MiniTableField field = {2, 2, 2, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_MessageOptions_clear_deprecated(google_protobuf_MessageOptions* msg) {
  const upb_MiniTableField field = {3, 3, 3, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE bool google_protobuf_MessageOptions_deprecated(const google_protobuf_MessageOptions* msg) {
  bool default_val = false;
  bool ret;
  const upb_MiniTableField field = {3, 3, 3, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_MessageOptions_has_deprecated(const google_protobuf_MessageOptions* msg) {
  const upb_MiniTableField field = {3, 3, 3, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_MessageOptions_clear_map_entry(google_protobuf_MessageOptions* msg) {
  const upb_MiniTableField field = {7, 4, 4, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE bool google_protobuf_MessageOptions_map_entry(const google_protobuf_MessageOptions* msg) {
  bool default_val = false;
  bool ret;
  const upb_MiniTableField field = {7, 4, 4, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_MessageOptions_has_map_entry(const google_protobuf_MessageOptions* msg) {
  const upb_MiniTableField field = {7, 4, 4, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_MessageOptions_clear_deprecated_legacy_json_field_conflicts(google_protobuf_MessageOptions* msg) {
  const upb_MiniTableField field = {11, 5, 5, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE bool google_protobuf_MessageOptions_deprecated_legacy_json_field_conflicts(const google_protobuf_MessageOptions* msg) {
  bool default_val = false;
  bool ret;
  const upb_MiniTableField field = {11, 5, 5, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_MessageOptions_has_deprecated_legacy_json_field_conflicts(const google_protobuf_MessageOptions* msg) {
  const upb_MiniTableField field = {11, 5, 5, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_MessageOptions_clear_uninterpreted_option(google_protobuf_MessageOptions* msg) {
  const upb_MiniTableField field = {999, 8, 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE const google_protobuf_UninterpretedOption* const* google_protobuf_MessageOptions_uninterpreted_option(const google_protobuf_MessageOptions* msg, size_t* size) {
  const upb_MiniTableField field = {999, 8, 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (const google_protobuf_UninterpretedOption* const*)_upb_array_constptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE const upb_Array* _google_protobuf_MessageOptions_uninterpreted_option_upb_array(const google_protobuf_MessageOptions* msg, size_t* size) {
  const upb_MiniTableField field = {999, 8, 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE upb_Array* _google_protobuf_MessageOptions_uninterpreted_option_mutable_upb_array(const google_protobuf_MessageOptions* msg, size_t* size, upb_Arena* arena) {
  const upb_MiniTableField field = {999, 8, 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(
      (upb_Message*)msg, &field, arena);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE bool google_protobuf_MessageOptions_has_uninterpreted_option(const google_protobuf_MessageOptions* msg) {
  size_t size;
  google_protobuf_MessageOptions_uninterpreted_option(msg, &size);
  return size != 0;
}

UPB_INLINE void google_protobuf_MessageOptions_set_message_set_wire_format(google_protobuf_MessageOptions *msg, bool value) {
  const upb_MiniTableField field = {1, 1, 1, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_MessageOptions_set_no_standard_descriptor_accessor(google_protobuf_MessageOptions *msg, bool value) {
  const upb_MiniTableField field = {2, 2, 2, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_MessageOptions_set_deprecated(google_protobuf_MessageOptions *msg, bool value) {
  const upb_MiniTableField field = {3, 3, 3, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_MessageOptions_set_map_entry(google_protobuf_MessageOptions *msg, bool value) {
  const upb_MiniTableField field = {7, 4, 4, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_MessageOptions_set_deprecated_legacy_json_field_conflicts(google_protobuf_MessageOptions *msg, bool value) {
  const upb_MiniTableField field = {11, 5, 5, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_MessageOptions_mutable_uninterpreted_option(google_protobuf_MessageOptions* msg, size_t* size) {
  upb_MiniTableField field = {999, 8, 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetMutableArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (google_protobuf_UninterpretedOption**)_upb_array_ptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_MessageOptions_resize_uninterpreted_option(google_protobuf_MessageOptions* msg, size_t size, upb_Arena* arena) {
  upb_MiniTableField field = {999, 8, 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  return (google_protobuf_UninterpretedOption**)upb_Message_ResizeArray(msg, &field, size, arena);
}
UPB_INLINE struct google_protobuf_UninterpretedOption* google_protobuf_MessageOptions_add_uninterpreted_option(google_protobuf_MessageOptions* msg, upb_Arena* arena) {
  upb_MiniTableField field = {999, 8, 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(msg, &field, arena);
  if (!arr || !_upb_Array_ResizeUninitialized(arr, arr->size + 1, arena)) {
    return NULL;
  }
  struct google_protobuf_UninterpretedOption* sub = (struct google_protobuf_UninterpretedOption*)_upb_Message_New(&google_protobuf_UninterpretedOption_msg_init, arena);
  if (!arr || !sub) return NULL;
  _upb_Array_Set(arr, arr->size - 1, &sub, sizeof(sub));
  return sub;
}

/* google.protobuf.FieldOptions */

UPB_INLINE google_protobuf_FieldOptions* google_protobuf_FieldOptions_new(upb_Arena* arena) {
  return (google_protobuf_FieldOptions*)_upb_Message_New(&google_protobuf_FieldOptions_msg_init, arena);
}
UPB_INLINE google_protobuf_FieldOptions* google_protobuf_FieldOptions_parse(const char* buf, size_t size, upb_Arena* arena) {
  google_protobuf_FieldOptions* ret = google_protobuf_FieldOptions_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_FieldOptions_msg_init, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE google_protobuf_FieldOptions* google_protobuf_FieldOptions_parse_ex(const char* buf, size_t size,
                           const upb_ExtensionRegistry* extreg,
                           int options, upb_Arena* arena) {
  google_protobuf_FieldOptions* ret = google_protobuf_FieldOptions_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_FieldOptions_msg_init, extreg, options, arena) !=
      kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE char* google_protobuf_FieldOptions_serialize(const google_protobuf_FieldOptions* msg, upb_Arena* arena, size_t* len) {
  char* ptr;
  (void)upb_Encode(msg, &google_protobuf_FieldOptions_msg_init, 0, arena, &ptr, len);
  return ptr;
}
UPB_INLINE char* google_protobuf_FieldOptions_serialize_ex(const google_protobuf_FieldOptions* msg, int options,
                                 upb_Arena* arena, size_t* len) {
  char* ptr;
  (void)upb_Encode(msg, &google_protobuf_FieldOptions_msg_init, options, arena, &ptr, len);
  return ptr;
}
UPB_INLINE void google_protobuf_FieldOptions_clear_ctype(google_protobuf_FieldOptions* msg) {
  const upb_MiniTableField field = {1, 4, 1, 0, 14, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE int32_t google_protobuf_FieldOptions_ctype(const google_protobuf_FieldOptions* msg) {
  int32_t default_val = 0;
  int32_t ret;
  const upb_MiniTableField field = {1, 4, 1, 0, 14, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_FieldOptions_has_ctype(const google_protobuf_FieldOptions* msg) {
  const upb_MiniTableField field = {1, 4, 1, 0, 14, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_FieldOptions_clear_packed(google_protobuf_FieldOptions* msg) {
  const upb_MiniTableField field = {2, 8, 2, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE bool google_protobuf_FieldOptions_packed(const google_protobuf_FieldOptions* msg) {
  bool default_val = false;
  bool ret;
  const upb_MiniTableField field = {2, 8, 2, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_FieldOptions_has_packed(const google_protobuf_FieldOptions* msg) {
  const upb_MiniTableField field = {2, 8, 2, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_FieldOptions_clear_deprecated(google_protobuf_FieldOptions* msg) {
  const upb_MiniTableField field = {3, 9, 3, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE bool google_protobuf_FieldOptions_deprecated(const google_protobuf_FieldOptions* msg) {
  bool default_val = false;
  bool ret;
  const upb_MiniTableField field = {3, 9, 3, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_FieldOptions_has_deprecated(const google_protobuf_FieldOptions* msg) {
  const upb_MiniTableField field = {3, 9, 3, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_FieldOptions_clear_lazy(google_protobuf_FieldOptions* msg) {
  const upb_MiniTableField field = {5, 10, 4, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE bool google_protobuf_FieldOptions_lazy(const google_protobuf_FieldOptions* msg) {
  bool default_val = false;
  bool ret;
  const upb_MiniTableField field = {5, 10, 4, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_FieldOptions_has_lazy(const google_protobuf_FieldOptions* msg) {
  const upb_MiniTableField field = {5, 10, 4, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_FieldOptions_clear_jstype(google_protobuf_FieldOptions* msg) {
  const upb_MiniTableField field = {6, 12, 5, 1, 14, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE int32_t google_protobuf_FieldOptions_jstype(const google_protobuf_FieldOptions* msg) {
  int32_t default_val = 0;
  int32_t ret;
  const upb_MiniTableField field = {6, 12, 5, 1, 14, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_FieldOptions_has_jstype(const google_protobuf_FieldOptions* msg) {
  const upb_MiniTableField field = {6, 12, 5, 1, 14, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_FieldOptions_clear_weak(google_protobuf_FieldOptions* msg) {
  const upb_MiniTableField field = {10, 16, 6, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE bool google_protobuf_FieldOptions_weak(const google_protobuf_FieldOptions* msg) {
  bool default_val = false;
  bool ret;
  const upb_MiniTableField field = {10, 16, 6, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_FieldOptions_has_weak(const google_protobuf_FieldOptions* msg) {
  const upb_MiniTableField field = {10, 16, 6, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_FieldOptions_clear_unverified_lazy(google_protobuf_FieldOptions* msg) {
  const upb_MiniTableField field = {15, 17, 7, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE bool google_protobuf_FieldOptions_unverified_lazy(const google_protobuf_FieldOptions* msg) {
  bool default_val = false;
  bool ret;
  const upb_MiniTableField field = {15, 17, 7, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_FieldOptions_has_unverified_lazy(const google_protobuf_FieldOptions* msg) {
  const upb_MiniTableField field = {15, 17, 7, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_FieldOptions_clear_debug_redact(google_protobuf_FieldOptions* msg) {
  const upb_MiniTableField field = {16, 18, 8, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE bool google_protobuf_FieldOptions_debug_redact(const google_protobuf_FieldOptions* msg) {
  bool default_val = false;
  bool ret;
  const upb_MiniTableField field = {16, 18, 8, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_FieldOptions_has_debug_redact(const google_protobuf_FieldOptions* msg) {
  const upb_MiniTableField field = {16, 18, 8, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_FieldOptions_clear_retention(google_protobuf_FieldOptions* msg) {
  const upb_MiniTableField field = {17, 20, 9, 2, 14, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE int32_t google_protobuf_FieldOptions_retention(const google_protobuf_FieldOptions* msg) {
  int32_t default_val = 0;
  int32_t ret;
  const upb_MiniTableField field = {17, 20, 9, 2, 14, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_FieldOptions_has_retention(const google_protobuf_FieldOptions* msg) {
  const upb_MiniTableField field = {17, 20, 9, 2, 14, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_FieldOptions_clear_target(google_protobuf_FieldOptions* msg) {
  const upb_MiniTableField field = {18, 24, 10, 3, 14, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE int32_t google_protobuf_FieldOptions_target(const google_protobuf_FieldOptions* msg) {
  int32_t default_val = 0;
  int32_t ret;
  const upb_MiniTableField field = {18, 24, 10, 3, 14, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_FieldOptions_has_target(const google_protobuf_FieldOptions* msg) {
  const upb_MiniTableField field = {18, 24, 10, 3, 14, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_FieldOptions_clear_targets(google_protobuf_FieldOptions* msg) {
  const upb_MiniTableField field = {19, UPB_SIZE(28, 32), 0, 4, 14, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE int32_t const* google_protobuf_FieldOptions_targets(const google_protobuf_FieldOptions* msg, size_t* size) {
  const upb_MiniTableField field = {19, UPB_SIZE(28, 32), 0, 4, 14, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (int32_t const*)_upb_array_constptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE const upb_Array* _google_protobuf_FieldOptions_targets_upb_array(const google_protobuf_FieldOptions* msg, size_t* size) {
  const upb_MiniTableField field = {19, UPB_SIZE(28, 32), 0, 4, 14, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE upb_Array* _google_protobuf_FieldOptions_targets_mutable_upb_array(const google_protobuf_FieldOptions* msg, size_t* size, upb_Arena* arena) {
  const upb_MiniTableField field = {19, UPB_SIZE(28, 32), 0, 4, 14, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(
      (upb_Message*)msg, &field, arena);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE bool google_protobuf_FieldOptions_has_targets(const google_protobuf_FieldOptions* msg) {
  size_t size;
  google_protobuf_FieldOptions_targets(msg, &size);
  return size != 0;
}
UPB_INLINE void google_protobuf_FieldOptions_clear_uninterpreted_option(google_protobuf_FieldOptions* msg) {
  const upb_MiniTableField field = {999, UPB_SIZE(32, 40), 0, 5, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE const google_protobuf_UninterpretedOption* const* google_protobuf_FieldOptions_uninterpreted_option(const google_protobuf_FieldOptions* msg, size_t* size) {
  const upb_MiniTableField field = {999, UPB_SIZE(32, 40), 0, 5, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (const google_protobuf_UninterpretedOption* const*)_upb_array_constptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE const upb_Array* _google_protobuf_FieldOptions_uninterpreted_option_upb_array(const google_protobuf_FieldOptions* msg, size_t* size) {
  const upb_MiniTableField field = {999, UPB_SIZE(32, 40), 0, 5, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE upb_Array* _google_protobuf_FieldOptions_uninterpreted_option_mutable_upb_array(const google_protobuf_FieldOptions* msg, size_t* size, upb_Arena* arena) {
  const upb_MiniTableField field = {999, UPB_SIZE(32, 40), 0, 5, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(
      (upb_Message*)msg, &field, arena);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE bool google_protobuf_FieldOptions_has_uninterpreted_option(const google_protobuf_FieldOptions* msg) {
  size_t size;
  google_protobuf_FieldOptions_uninterpreted_option(msg, &size);
  return size != 0;
}

UPB_INLINE void google_protobuf_FieldOptions_set_ctype(google_protobuf_FieldOptions *msg, int32_t value) {
  const upb_MiniTableField field = {1, 4, 1, 0, 14, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_FieldOptions_set_packed(google_protobuf_FieldOptions *msg, bool value) {
  const upb_MiniTableField field = {2, 8, 2, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_FieldOptions_set_deprecated(google_protobuf_FieldOptions *msg, bool value) {
  const upb_MiniTableField field = {3, 9, 3, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_FieldOptions_set_lazy(google_protobuf_FieldOptions *msg, bool value) {
  const upb_MiniTableField field = {5, 10, 4, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_FieldOptions_set_jstype(google_protobuf_FieldOptions *msg, int32_t value) {
  const upb_MiniTableField field = {6, 12, 5, 1, 14, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_FieldOptions_set_weak(google_protobuf_FieldOptions *msg, bool value) {
  const upb_MiniTableField field = {10, 16, 6, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_FieldOptions_set_unverified_lazy(google_protobuf_FieldOptions *msg, bool value) {
  const upb_MiniTableField field = {15, 17, 7, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_FieldOptions_set_debug_redact(google_protobuf_FieldOptions *msg, bool value) {
  const upb_MiniTableField field = {16, 18, 8, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_FieldOptions_set_retention(google_protobuf_FieldOptions *msg, int32_t value) {
  const upb_MiniTableField field = {17, 20, 9, 2, 14, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_FieldOptions_set_target(google_protobuf_FieldOptions *msg, int32_t value) {
  const upb_MiniTableField field = {18, 24, 10, 3, 14, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE int32_t* google_protobuf_FieldOptions_mutable_targets(google_protobuf_FieldOptions* msg, size_t* size) {
  upb_MiniTableField field = {19, UPB_SIZE(28, 32), 0, 4, 14, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetMutableArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (int32_t*)_upb_array_ptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE int32_t* google_protobuf_FieldOptions_resize_targets(google_protobuf_FieldOptions* msg, size_t size, upb_Arena* arena) {
  upb_MiniTableField field = {19, UPB_SIZE(28, 32), 0, 4, 14, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  return (int32_t*)upb_Message_ResizeArray(msg, &field, size, arena);
}
UPB_INLINE bool google_protobuf_FieldOptions_add_targets(google_protobuf_FieldOptions* msg, int32_t val, upb_Arena* arena) {
  upb_MiniTableField field = {19, UPB_SIZE(28, 32), 0, 4, 14, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(msg, &field, arena);
  if (!arr || !_upb_Array_ResizeUninitialized(arr, arr->size + 1, arena)) {
    return false;
  }
  _upb_Array_Set(arr, arr->size - 1, &val, sizeof(val));
  return true;
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_FieldOptions_mutable_uninterpreted_option(google_protobuf_FieldOptions* msg, size_t* size) {
  upb_MiniTableField field = {999, UPB_SIZE(32, 40), 0, 5, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetMutableArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (google_protobuf_UninterpretedOption**)_upb_array_ptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_FieldOptions_resize_uninterpreted_option(google_protobuf_FieldOptions* msg, size_t size, upb_Arena* arena) {
  upb_MiniTableField field = {999, UPB_SIZE(32, 40), 0, 5, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  return (google_protobuf_UninterpretedOption**)upb_Message_ResizeArray(msg, &field, size, arena);
}
UPB_INLINE struct google_protobuf_UninterpretedOption* google_protobuf_FieldOptions_add_uninterpreted_option(google_protobuf_FieldOptions* msg, upb_Arena* arena) {
  upb_MiniTableField field = {999, UPB_SIZE(32, 40), 0, 5, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(msg, &field, arena);
  if (!arr || !_upb_Array_ResizeUninitialized(arr, arr->size + 1, arena)) {
    return NULL;
  }
  struct google_protobuf_UninterpretedOption* sub = (struct google_protobuf_UninterpretedOption*)_upb_Message_New(&google_protobuf_UninterpretedOption_msg_init, arena);
  if (!arr || !sub) return NULL;
  _upb_Array_Set(arr, arr->size - 1, &sub, sizeof(sub));
  return sub;
}

/* google.protobuf.OneofOptions */

UPB_INLINE google_protobuf_OneofOptions* google_protobuf_OneofOptions_new(upb_Arena* arena) {
  return (google_protobuf_OneofOptions*)_upb_Message_New(&google_protobuf_OneofOptions_msg_init, arena);
}
UPB_INLINE google_protobuf_OneofOptions* google_protobuf_OneofOptions_parse(const char* buf, size_t size, upb_Arena* arena) {
  google_protobuf_OneofOptions* ret = google_protobuf_OneofOptions_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_OneofOptions_msg_init, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE google_protobuf_OneofOptions* google_protobuf_OneofOptions_parse_ex(const char* buf, size_t size,
                           const upb_ExtensionRegistry* extreg,
                           int options, upb_Arena* arena) {
  google_protobuf_OneofOptions* ret = google_protobuf_OneofOptions_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_OneofOptions_msg_init, extreg, options, arena) !=
      kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE char* google_protobuf_OneofOptions_serialize(const google_protobuf_OneofOptions* msg, upb_Arena* arena, size_t* len) {
  char* ptr;
  (void)upb_Encode(msg, &google_protobuf_OneofOptions_msg_init, 0, arena, &ptr, len);
  return ptr;
}
UPB_INLINE char* google_protobuf_OneofOptions_serialize_ex(const google_protobuf_OneofOptions* msg, int options,
                                 upb_Arena* arena, size_t* len) {
  char* ptr;
  (void)upb_Encode(msg, &google_protobuf_OneofOptions_msg_init, options, arena, &ptr, len);
  return ptr;
}
UPB_INLINE void google_protobuf_OneofOptions_clear_uninterpreted_option(google_protobuf_OneofOptions* msg) {
  const upb_MiniTableField field = {999, 0, 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE const google_protobuf_UninterpretedOption* const* google_protobuf_OneofOptions_uninterpreted_option(const google_protobuf_OneofOptions* msg, size_t* size) {
  const upb_MiniTableField field = {999, 0, 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (const google_protobuf_UninterpretedOption* const*)_upb_array_constptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE const upb_Array* _google_protobuf_OneofOptions_uninterpreted_option_upb_array(const google_protobuf_OneofOptions* msg, size_t* size) {
  const upb_MiniTableField field = {999, 0, 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE upb_Array* _google_protobuf_OneofOptions_uninterpreted_option_mutable_upb_array(const google_protobuf_OneofOptions* msg, size_t* size, upb_Arena* arena) {
  const upb_MiniTableField field = {999, 0, 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(
      (upb_Message*)msg, &field, arena);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE bool google_protobuf_OneofOptions_has_uninterpreted_option(const google_protobuf_OneofOptions* msg) {
  size_t size;
  google_protobuf_OneofOptions_uninterpreted_option(msg, &size);
  return size != 0;
}

UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_OneofOptions_mutable_uninterpreted_option(google_protobuf_OneofOptions* msg, size_t* size) {
  upb_MiniTableField field = {999, 0, 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetMutableArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (google_protobuf_UninterpretedOption**)_upb_array_ptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_OneofOptions_resize_uninterpreted_option(google_protobuf_OneofOptions* msg, size_t size, upb_Arena* arena) {
  upb_MiniTableField field = {999, 0, 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  return (google_protobuf_UninterpretedOption**)upb_Message_ResizeArray(msg, &field, size, arena);
}
UPB_INLINE struct google_protobuf_UninterpretedOption* google_protobuf_OneofOptions_add_uninterpreted_option(google_protobuf_OneofOptions* msg, upb_Arena* arena) {
  upb_MiniTableField field = {999, 0, 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(msg, &field, arena);
  if (!arr || !_upb_Array_ResizeUninitialized(arr, arr->size + 1, arena)) {
    return NULL;
  }
  struct google_protobuf_UninterpretedOption* sub = (struct google_protobuf_UninterpretedOption*)_upb_Message_New(&google_protobuf_UninterpretedOption_msg_init, arena);
  if (!arr || !sub) return NULL;
  _upb_Array_Set(arr, arr->size - 1, &sub, sizeof(sub));
  return sub;
}

/* google.protobuf.EnumOptions */

UPB_INLINE google_protobuf_EnumOptions* google_protobuf_EnumOptions_new(upb_Arena* arena) {
  return (google_protobuf_EnumOptions*)_upb_Message_New(&google_protobuf_EnumOptions_msg_init, arena);
}
UPB_INLINE google_protobuf_EnumOptions* google_protobuf_EnumOptions_parse(const char* buf, size_t size, upb_Arena* arena) {
  google_protobuf_EnumOptions* ret = google_protobuf_EnumOptions_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_EnumOptions_msg_init, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE google_protobuf_EnumOptions* google_protobuf_EnumOptions_parse_ex(const char* buf, size_t size,
                           const upb_ExtensionRegistry* extreg,
                           int options, upb_Arena* arena) {
  google_protobuf_EnumOptions* ret = google_protobuf_EnumOptions_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_EnumOptions_msg_init, extreg, options, arena) !=
      kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE char* google_protobuf_EnumOptions_serialize(const google_protobuf_EnumOptions* msg, upb_Arena* arena, size_t* len) {
  char* ptr;
  (void)upb_Encode(msg, &google_protobuf_EnumOptions_msg_init, 0, arena, &ptr, len);
  return ptr;
}
UPB_INLINE char* google_protobuf_EnumOptions_serialize_ex(const google_protobuf_EnumOptions* msg, int options,
                                 upb_Arena* arena, size_t* len) {
  char* ptr;
  (void)upb_Encode(msg, &google_protobuf_EnumOptions_msg_init, options, arena, &ptr, len);
  return ptr;
}
UPB_INLINE void google_protobuf_EnumOptions_clear_allow_alias(google_protobuf_EnumOptions* msg) {
  const upb_MiniTableField field = {2, 1, 1, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE bool google_protobuf_EnumOptions_allow_alias(const google_protobuf_EnumOptions* msg) {
  bool default_val = false;
  bool ret;
  const upb_MiniTableField field = {2, 1, 1, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_EnumOptions_has_allow_alias(const google_protobuf_EnumOptions* msg) {
  const upb_MiniTableField field = {2, 1, 1, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_EnumOptions_clear_deprecated(google_protobuf_EnumOptions* msg) {
  const upb_MiniTableField field = {3, 2, 2, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE bool google_protobuf_EnumOptions_deprecated(const google_protobuf_EnumOptions* msg) {
  bool default_val = false;
  bool ret;
  const upb_MiniTableField field = {3, 2, 2, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_EnumOptions_has_deprecated(const google_protobuf_EnumOptions* msg) {
  const upb_MiniTableField field = {3, 2, 2, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_EnumOptions_clear_deprecated_legacy_json_field_conflicts(google_protobuf_EnumOptions* msg) {
  const upb_MiniTableField field = {6, 3, 3, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE bool google_protobuf_EnumOptions_deprecated_legacy_json_field_conflicts(const google_protobuf_EnumOptions* msg) {
  bool default_val = false;
  bool ret;
  const upb_MiniTableField field = {6, 3, 3, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_EnumOptions_has_deprecated_legacy_json_field_conflicts(const google_protobuf_EnumOptions* msg) {
  const upb_MiniTableField field = {6, 3, 3, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_EnumOptions_clear_uninterpreted_option(google_protobuf_EnumOptions* msg) {
  const upb_MiniTableField field = {999, UPB_SIZE(4, 8), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE const google_protobuf_UninterpretedOption* const* google_protobuf_EnumOptions_uninterpreted_option(const google_protobuf_EnumOptions* msg, size_t* size) {
  const upb_MiniTableField field = {999, UPB_SIZE(4, 8), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (const google_protobuf_UninterpretedOption* const*)_upb_array_constptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE const upb_Array* _google_protobuf_EnumOptions_uninterpreted_option_upb_array(const google_protobuf_EnumOptions* msg, size_t* size) {
  const upb_MiniTableField field = {999, UPB_SIZE(4, 8), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE upb_Array* _google_protobuf_EnumOptions_uninterpreted_option_mutable_upb_array(const google_protobuf_EnumOptions* msg, size_t* size, upb_Arena* arena) {
  const upb_MiniTableField field = {999, UPB_SIZE(4, 8), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(
      (upb_Message*)msg, &field, arena);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE bool google_protobuf_EnumOptions_has_uninterpreted_option(const google_protobuf_EnumOptions* msg) {
  size_t size;
  google_protobuf_EnumOptions_uninterpreted_option(msg, &size);
  return size != 0;
}

UPB_INLINE void google_protobuf_EnumOptions_set_allow_alias(google_protobuf_EnumOptions *msg, bool value) {
  const upb_MiniTableField field = {2, 1, 1, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_EnumOptions_set_deprecated(google_protobuf_EnumOptions *msg, bool value) {
  const upb_MiniTableField field = {3, 2, 2, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_EnumOptions_set_deprecated_legacy_json_field_conflicts(google_protobuf_EnumOptions *msg, bool value) {
  const upb_MiniTableField field = {6, 3, 3, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_EnumOptions_mutable_uninterpreted_option(google_protobuf_EnumOptions* msg, size_t* size) {
  upb_MiniTableField field = {999, UPB_SIZE(4, 8), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetMutableArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (google_protobuf_UninterpretedOption**)_upb_array_ptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_EnumOptions_resize_uninterpreted_option(google_protobuf_EnumOptions* msg, size_t size, upb_Arena* arena) {
  upb_MiniTableField field = {999, UPB_SIZE(4, 8), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  return (google_protobuf_UninterpretedOption**)upb_Message_ResizeArray(msg, &field, size, arena);
}
UPB_INLINE struct google_protobuf_UninterpretedOption* google_protobuf_EnumOptions_add_uninterpreted_option(google_protobuf_EnumOptions* msg, upb_Arena* arena) {
  upb_MiniTableField field = {999, UPB_SIZE(4, 8), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(msg, &field, arena);
  if (!arr || !_upb_Array_ResizeUninitialized(arr, arr->size + 1, arena)) {
    return NULL;
  }
  struct google_protobuf_UninterpretedOption* sub = (struct google_protobuf_UninterpretedOption*)_upb_Message_New(&google_protobuf_UninterpretedOption_msg_init, arena);
  if (!arr || !sub) return NULL;
  _upb_Array_Set(arr, arr->size - 1, &sub, sizeof(sub));
  return sub;
}

/* google.protobuf.EnumValueOptions */

UPB_INLINE google_protobuf_EnumValueOptions* google_protobuf_EnumValueOptions_new(upb_Arena* arena) {
  return (google_protobuf_EnumValueOptions*)_upb_Message_New(&google_protobuf_EnumValueOptions_msg_init, arena);
}
UPB_INLINE google_protobuf_EnumValueOptions* google_protobuf_EnumValueOptions_parse(const char* buf, size_t size, upb_Arena* arena) {
  google_protobuf_EnumValueOptions* ret = google_protobuf_EnumValueOptions_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_EnumValueOptions_msg_init, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE google_protobuf_EnumValueOptions* google_protobuf_EnumValueOptions_parse_ex(const char* buf, size_t size,
                           const upb_ExtensionRegistry* extreg,
                           int options, upb_Arena* arena) {
  google_protobuf_EnumValueOptions* ret = google_protobuf_EnumValueOptions_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_EnumValueOptions_msg_init, extreg, options, arena) !=
      kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE char* google_protobuf_EnumValueOptions_serialize(const google_protobuf_EnumValueOptions* msg, upb_Arena* arena, size_t* len) {
  char* ptr;
  (void)upb_Encode(msg, &google_protobuf_EnumValueOptions_msg_init, 0, arena, &ptr, len);
  return ptr;
}
UPB_INLINE char* google_protobuf_EnumValueOptions_serialize_ex(const google_protobuf_EnumValueOptions* msg, int options,
                                 upb_Arena* arena, size_t* len) {
  char* ptr;
  (void)upb_Encode(msg, &google_protobuf_EnumValueOptions_msg_init, options, arena, &ptr, len);
  return ptr;
}
UPB_INLINE void google_protobuf_EnumValueOptions_clear_deprecated(google_protobuf_EnumValueOptions* msg) {
  const upb_MiniTableField field = {1, 1, 1, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE bool google_protobuf_EnumValueOptions_deprecated(const google_protobuf_EnumValueOptions* msg) {
  bool default_val = false;
  bool ret;
  const upb_MiniTableField field = {1, 1, 1, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_EnumValueOptions_has_deprecated(const google_protobuf_EnumValueOptions* msg) {
  const upb_MiniTableField field = {1, 1, 1, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_EnumValueOptions_clear_uninterpreted_option(google_protobuf_EnumValueOptions* msg) {
  const upb_MiniTableField field = {999, UPB_SIZE(4, 8), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE const google_protobuf_UninterpretedOption* const* google_protobuf_EnumValueOptions_uninterpreted_option(const google_protobuf_EnumValueOptions* msg, size_t* size) {
  const upb_MiniTableField field = {999, UPB_SIZE(4, 8), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (const google_protobuf_UninterpretedOption* const*)_upb_array_constptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE const upb_Array* _google_protobuf_EnumValueOptions_uninterpreted_option_upb_array(const google_protobuf_EnumValueOptions* msg, size_t* size) {
  const upb_MiniTableField field = {999, UPB_SIZE(4, 8), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE upb_Array* _google_protobuf_EnumValueOptions_uninterpreted_option_mutable_upb_array(const google_protobuf_EnumValueOptions* msg, size_t* size, upb_Arena* arena) {
  const upb_MiniTableField field = {999, UPB_SIZE(4, 8), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(
      (upb_Message*)msg, &field, arena);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE bool google_protobuf_EnumValueOptions_has_uninterpreted_option(const google_protobuf_EnumValueOptions* msg) {
  size_t size;
  google_protobuf_EnumValueOptions_uninterpreted_option(msg, &size);
  return size != 0;
}

UPB_INLINE void google_protobuf_EnumValueOptions_set_deprecated(google_protobuf_EnumValueOptions *msg, bool value) {
  const upb_MiniTableField field = {1, 1, 1, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_EnumValueOptions_mutable_uninterpreted_option(google_protobuf_EnumValueOptions* msg, size_t* size) {
  upb_MiniTableField field = {999, UPB_SIZE(4, 8), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetMutableArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (google_protobuf_UninterpretedOption**)_upb_array_ptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_EnumValueOptions_resize_uninterpreted_option(google_protobuf_EnumValueOptions* msg, size_t size, upb_Arena* arena) {
  upb_MiniTableField field = {999, UPB_SIZE(4, 8), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  return (google_protobuf_UninterpretedOption**)upb_Message_ResizeArray(msg, &field, size, arena);
}
UPB_INLINE struct google_protobuf_UninterpretedOption* google_protobuf_EnumValueOptions_add_uninterpreted_option(google_protobuf_EnumValueOptions* msg, upb_Arena* arena) {
  upb_MiniTableField field = {999, UPB_SIZE(4, 8), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(msg, &field, arena);
  if (!arr || !_upb_Array_ResizeUninitialized(arr, arr->size + 1, arena)) {
    return NULL;
  }
  struct google_protobuf_UninterpretedOption* sub = (struct google_protobuf_UninterpretedOption*)_upb_Message_New(&google_protobuf_UninterpretedOption_msg_init, arena);
  if (!arr || !sub) return NULL;
  _upb_Array_Set(arr, arr->size - 1, &sub, sizeof(sub));
  return sub;
}

/* google.protobuf.ServiceOptions */

UPB_INLINE google_protobuf_ServiceOptions* google_protobuf_ServiceOptions_new(upb_Arena* arena) {
  return (google_protobuf_ServiceOptions*)_upb_Message_New(&google_protobuf_ServiceOptions_msg_init, arena);
}
UPB_INLINE google_protobuf_ServiceOptions* google_protobuf_ServiceOptions_parse(const char* buf, size_t size, upb_Arena* arena) {
  google_protobuf_ServiceOptions* ret = google_protobuf_ServiceOptions_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_ServiceOptions_msg_init, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE google_protobuf_ServiceOptions* google_protobuf_ServiceOptions_parse_ex(const char* buf, size_t size,
                           const upb_ExtensionRegistry* extreg,
                           int options, upb_Arena* arena) {
  google_protobuf_ServiceOptions* ret = google_protobuf_ServiceOptions_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_ServiceOptions_msg_init, extreg, options, arena) !=
      kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE char* google_protobuf_ServiceOptions_serialize(const google_protobuf_ServiceOptions* msg, upb_Arena* arena, size_t* len) {
  char* ptr;
  (void)upb_Encode(msg, &google_protobuf_ServiceOptions_msg_init, 0, arena, &ptr, len);
  return ptr;
}
UPB_INLINE char* google_protobuf_ServiceOptions_serialize_ex(const google_protobuf_ServiceOptions* msg, int options,
                                 upb_Arena* arena, size_t* len) {
  char* ptr;
  (void)upb_Encode(msg, &google_protobuf_ServiceOptions_msg_init, options, arena, &ptr, len);
  return ptr;
}
UPB_INLINE void google_protobuf_ServiceOptions_clear_deprecated(google_protobuf_ServiceOptions* msg) {
  const upb_MiniTableField field = {33, 1, 1, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE bool google_protobuf_ServiceOptions_deprecated(const google_protobuf_ServiceOptions* msg) {
  bool default_val = false;
  bool ret;
  const upb_MiniTableField field = {33, 1, 1, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_ServiceOptions_has_deprecated(const google_protobuf_ServiceOptions* msg) {
  const upb_MiniTableField field = {33, 1, 1, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_ServiceOptions_clear_uninterpreted_option(google_protobuf_ServiceOptions* msg) {
  const upb_MiniTableField field = {999, UPB_SIZE(4, 8), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE const google_protobuf_UninterpretedOption* const* google_protobuf_ServiceOptions_uninterpreted_option(const google_protobuf_ServiceOptions* msg, size_t* size) {
  const upb_MiniTableField field = {999, UPB_SIZE(4, 8), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (const google_protobuf_UninterpretedOption* const*)_upb_array_constptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE const upb_Array* _google_protobuf_ServiceOptions_uninterpreted_option_upb_array(const google_protobuf_ServiceOptions* msg, size_t* size) {
  const upb_MiniTableField field = {999, UPB_SIZE(4, 8), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE upb_Array* _google_protobuf_ServiceOptions_uninterpreted_option_mutable_upb_array(const google_protobuf_ServiceOptions* msg, size_t* size, upb_Arena* arena) {
  const upb_MiniTableField field = {999, UPB_SIZE(4, 8), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(
      (upb_Message*)msg, &field, arena);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE bool google_protobuf_ServiceOptions_has_uninterpreted_option(const google_protobuf_ServiceOptions* msg) {
  size_t size;
  google_protobuf_ServiceOptions_uninterpreted_option(msg, &size);
  return size != 0;
}

UPB_INLINE void google_protobuf_ServiceOptions_set_deprecated(google_protobuf_ServiceOptions *msg, bool value) {
  const upb_MiniTableField field = {33, 1, 1, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_ServiceOptions_mutable_uninterpreted_option(google_protobuf_ServiceOptions* msg, size_t* size) {
  upb_MiniTableField field = {999, UPB_SIZE(4, 8), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetMutableArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (google_protobuf_UninterpretedOption**)_upb_array_ptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_ServiceOptions_resize_uninterpreted_option(google_protobuf_ServiceOptions* msg, size_t size, upb_Arena* arena) {
  upb_MiniTableField field = {999, UPB_SIZE(4, 8), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  return (google_protobuf_UninterpretedOption**)upb_Message_ResizeArray(msg, &field, size, arena);
}
UPB_INLINE struct google_protobuf_UninterpretedOption* google_protobuf_ServiceOptions_add_uninterpreted_option(google_protobuf_ServiceOptions* msg, upb_Arena* arena) {
  upb_MiniTableField field = {999, UPB_SIZE(4, 8), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(msg, &field, arena);
  if (!arr || !_upb_Array_ResizeUninitialized(arr, arr->size + 1, arena)) {
    return NULL;
  }
  struct google_protobuf_UninterpretedOption* sub = (struct google_protobuf_UninterpretedOption*)_upb_Message_New(&google_protobuf_UninterpretedOption_msg_init, arena);
  if (!arr || !sub) return NULL;
  _upb_Array_Set(arr, arr->size - 1, &sub, sizeof(sub));
  return sub;
}

/* google.protobuf.MethodOptions */

UPB_INLINE google_protobuf_MethodOptions* google_protobuf_MethodOptions_new(upb_Arena* arena) {
  return (google_protobuf_MethodOptions*)_upb_Message_New(&google_protobuf_MethodOptions_msg_init, arena);
}
UPB_INLINE google_protobuf_MethodOptions* google_protobuf_MethodOptions_parse(const char* buf, size_t size, upb_Arena* arena) {
  google_protobuf_MethodOptions* ret = google_protobuf_MethodOptions_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_MethodOptions_msg_init, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE google_protobuf_MethodOptions* google_protobuf_MethodOptions_parse_ex(const char* buf, size_t size,
                           const upb_ExtensionRegistry* extreg,
                           int options, upb_Arena* arena) {
  google_protobuf_MethodOptions* ret = google_protobuf_MethodOptions_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_MethodOptions_msg_init, extreg, options, arena) !=
      kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE char* google_protobuf_MethodOptions_serialize(const google_protobuf_MethodOptions* msg, upb_Arena* arena, size_t* len) {
  char* ptr;
  (void)upb_Encode(msg, &google_protobuf_MethodOptions_msg_init, 0, arena, &ptr, len);
  return ptr;
}
UPB_INLINE char* google_protobuf_MethodOptions_serialize_ex(const google_protobuf_MethodOptions* msg, int options,
                                 upb_Arena* arena, size_t* len) {
  char* ptr;
  (void)upb_Encode(msg, &google_protobuf_MethodOptions_msg_init, options, arena, &ptr, len);
  return ptr;
}
UPB_INLINE void google_protobuf_MethodOptions_clear_deprecated(google_protobuf_MethodOptions* msg) {
  const upb_MiniTableField field = {33, 1, 1, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE bool google_protobuf_MethodOptions_deprecated(const google_protobuf_MethodOptions* msg) {
  bool default_val = false;
  bool ret;
  const upb_MiniTableField field = {33, 1, 1, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_MethodOptions_has_deprecated(const google_protobuf_MethodOptions* msg) {
  const upb_MiniTableField field = {33, 1, 1, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_MethodOptions_clear_idempotency_level(google_protobuf_MethodOptions* msg) {
  const upb_MiniTableField field = {34, 4, 2, 0, 14, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE int32_t google_protobuf_MethodOptions_idempotency_level(const google_protobuf_MethodOptions* msg) {
  int32_t default_val = 0;
  int32_t ret;
  const upb_MiniTableField field = {34, 4, 2, 0, 14, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_MethodOptions_has_idempotency_level(const google_protobuf_MethodOptions* msg) {
  const upb_MiniTableField field = {34, 4, 2, 0, 14, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_MethodOptions_clear_uninterpreted_option(google_protobuf_MethodOptions* msg) {
  const upb_MiniTableField field = {999, 8, 0, 1, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE const google_protobuf_UninterpretedOption* const* google_protobuf_MethodOptions_uninterpreted_option(const google_protobuf_MethodOptions* msg, size_t* size) {
  const upb_MiniTableField field = {999, 8, 0, 1, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (const google_protobuf_UninterpretedOption* const*)_upb_array_constptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE const upb_Array* _google_protobuf_MethodOptions_uninterpreted_option_upb_array(const google_protobuf_MethodOptions* msg, size_t* size) {
  const upb_MiniTableField field = {999, 8, 0, 1, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE upb_Array* _google_protobuf_MethodOptions_uninterpreted_option_mutable_upb_array(const google_protobuf_MethodOptions* msg, size_t* size, upb_Arena* arena) {
  const upb_MiniTableField field = {999, 8, 0, 1, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(
      (upb_Message*)msg, &field, arena);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE bool google_protobuf_MethodOptions_has_uninterpreted_option(const google_protobuf_MethodOptions* msg) {
  size_t size;
  google_protobuf_MethodOptions_uninterpreted_option(msg, &size);
  return size != 0;
}

UPB_INLINE void google_protobuf_MethodOptions_set_deprecated(google_protobuf_MethodOptions *msg, bool value) {
  const upb_MiniTableField field = {33, 1, 1, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_MethodOptions_set_idempotency_level(google_protobuf_MethodOptions *msg, int32_t value) {
  const upb_MiniTableField field = {34, 4, 2, 0, 14, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_MethodOptions_mutable_uninterpreted_option(google_protobuf_MethodOptions* msg, size_t* size) {
  upb_MiniTableField field = {999, 8, 0, 1, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetMutableArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (google_protobuf_UninterpretedOption**)_upb_array_ptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_MethodOptions_resize_uninterpreted_option(google_protobuf_MethodOptions* msg, size_t size, upb_Arena* arena) {
  upb_MiniTableField field = {999, 8, 0, 1, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  return (google_protobuf_UninterpretedOption**)upb_Message_ResizeArray(msg, &field, size, arena);
}
UPB_INLINE struct google_protobuf_UninterpretedOption* google_protobuf_MethodOptions_add_uninterpreted_option(google_protobuf_MethodOptions* msg, upb_Arena* arena) {
  upb_MiniTableField field = {999, 8, 0, 1, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(msg, &field, arena);
  if (!arr || !_upb_Array_ResizeUninitialized(arr, arr->size + 1, arena)) {
    return NULL;
  }
  struct google_protobuf_UninterpretedOption* sub = (struct google_protobuf_UninterpretedOption*)_upb_Message_New(&google_protobuf_UninterpretedOption_msg_init, arena);
  if (!arr || !sub) return NULL;
  _upb_Array_Set(arr, arr->size - 1, &sub, sizeof(sub));
  return sub;
}

/* google.protobuf.UninterpretedOption */

UPB_INLINE google_protobuf_UninterpretedOption* google_protobuf_UninterpretedOption_new(upb_Arena* arena) {
  return (google_protobuf_UninterpretedOption*)_upb_Message_New(&google_protobuf_UninterpretedOption_msg_init, arena);
}
UPB_INLINE google_protobuf_UninterpretedOption* google_protobuf_UninterpretedOption_parse(const char* buf, size_t size, upb_Arena* arena) {
  google_protobuf_UninterpretedOption* ret = google_protobuf_UninterpretedOption_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_UninterpretedOption_msg_init, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE google_protobuf_UninterpretedOption* google_protobuf_UninterpretedOption_parse_ex(const char* buf, size_t size,
                           const upb_ExtensionRegistry* extreg,
                           int options, upb_Arena* arena) {
  google_protobuf_UninterpretedOption* ret = google_protobuf_UninterpretedOption_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_UninterpretedOption_msg_init, extreg, options, arena) !=
      kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE char* google_protobuf_UninterpretedOption_serialize(const google_protobuf_UninterpretedOption* msg, upb_Arena* arena, size_t* len) {
  char* ptr;
  (void)upb_Encode(msg, &google_protobuf_UninterpretedOption_msg_init, 0, arena, &ptr, len);
  return ptr;
}
UPB_INLINE char* google_protobuf_UninterpretedOption_serialize_ex(const google_protobuf_UninterpretedOption* msg, int options,
                                 upb_Arena* arena, size_t* len) {
  char* ptr;
  (void)upb_Encode(msg, &google_protobuf_UninterpretedOption_msg_init, options, arena, &ptr, len);
  return ptr;
}
UPB_INLINE void google_protobuf_UninterpretedOption_clear_name(google_protobuf_UninterpretedOption* msg) {
  const upb_MiniTableField field = {2, UPB_SIZE(4, 8), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE const google_protobuf_UninterpretedOption_NamePart* const* google_protobuf_UninterpretedOption_name(const google_protobuf_UninterpretedOption* msg, size_t* size) {
  const upb_MiniTableField field = {2, UPB_SIZE(4, 8), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (const google_protobuf_UninterpretedOption_NamePart* const*)_upb_array_constptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE const upb_Array* _google_protobuf_UninterpretedOption_name_upb_array(const google_protobuf_UninterpretedOption* msg, size_t* size) {
  const upb_MiniTableField field = {2, UPB_SIZE(4, 8), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE upb_Array* _google_protobuf_UninterpretedOption_name_mutable_upb_array(const google_protobuf_UninterpretedOption* msg, size_t* size, upb_Arena* arena) {
  const upb_MiniTableField field = {2, UPB_SIZE(4, 8), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(
      (upb_Message*)msg, &field, arena);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE bool google_protobuf_UninterpretedOption_has_name(const google_protobuf_UninterpretedOption* msg) {
  size_t size;
  google_protobuf_UninterpretedOption_name(msg, &size);
  return size != 0;
}
UPB_INLINE void google_protobuf_UninterpretedOption_clear_identifier_value(google_protobuf_UninterpretedOption* msg) {
  const upb_MiniTableField field = {3, UPB_SIZE(8, 16), 1, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE upb_StringView google_protobuf_UninterpretedOption_identifier_value(const google_protobuf_UninterpretedOption* msg) {
  upb_StringView default_val = upb_StringView_FromString("");
  upb_StringView ret;
  const upb_MiniTableField field = {3, UPB_SIZE(8, 16), 1, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_UninterpretedOption_has_identifier_value(const google_protobuf_UninterpretedOption* msg) {
  const upb_MiniTableField field = {3, UPB_SIZE(8, 16), 1, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_UninterpretedOption_clear_positive_int_value(google_protobuf_UninterpretedOption* msg) {
  const upb_MiniTableField field = {4, UPB_SIZE(16, 32), 2, kUpb_NoSub, 4, kUpb_FieldMode_Scalar | (kUpb_FieldRep_8Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE uint64_t google_protobuf_UninterpretedOption_positive_int_value(const google_protobuf_UninterpretedOption* msg) {
  uint64_t default_val = (uint64_t)0ull;
  uint64_t ret;
  const upb_MiniTableField field = {4, UPB_SIZE(16, 32), 2, kUpb_NoSub, 4, kUpb_FieldMode_Scalar | (kUpb_FieldRep_8Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_UninterpretedOption_has_positive_int_value(const google_protobuf_UninterpretedOption* msg) {
  const upb_MiniTableField field = {4, UPB_SIZE(16, 32), 2, kUpb_NoSub, 4, kUpb_FieldMode_Scalar | (kUpb_FieldRep_8Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_UninterpretedOption_clear_negative_int_value(google_protobuf_UninterpretedOption* msg) {
  const upb_MiniTableField field = {5, UPB_SIZE(24, 40), 3, kUpb_NoSub, 3, kUpb_FieldMode_Scalar | (kUpb_FieldRep_8Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE int64_t google_protobuf_UninterpretedOption_negative_int_value(const google_protobuf_UninterpretedOption* msg) {
  int64_t default_val = (int64_t)0ll;
  int64_t ret;
  const upb_MiniTableField field = {5, UPB_SIZE(24, 40), 3, kUpb_NoSub, 3, kUpb_FieldMode_Scalar | (kUpb_FieldRep_8Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_UninterpretedOption_has_negative_int_value(const google_protobuf_UninterpretedOption* msg) {
  const upb_MiniTableField field = {5, UPB_SIZE(24, 40), 3, kUpb_NoSub, 3, kUpb_FieldMode_Scalar | (kUpb_FieldRep_8Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_UninterpretedOption_clear_double_value(google_protobuf_UninterpretedOption* msg) {
  const upb_MiniTableField field = {6, UPB_SIZE(32, 48), 4, kUpb_NoSub, 1, kUpb_FieldMode_Scalar | (kUpb_FieldRep_8Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE double google_protobuf_UninterpretedOption_double_value(const google_protobuf_UninterpretedOption* msg) {
  double default_val = 0;
  double ret;
  const upb_MiniTableField field = {6, UPB_SIZE(32, 48), 4, kUpb_NoSub, 1, kUpb_FieldMode_Scalar | (kUpb_FieldRep_8Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_UninterpretedOption_has_double_value(const google_protobuf_UninterpretedOption* msg) {
  const upb_MiniTableField field = {6, UPB_SIZE(32, 48), 4, kUpb_NoSub, 1, kUpb_FieldMode_Scalar | (kUpb_FieldRep_8Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_UninterpretedOption_clear_string_value(google_protobuf_UninterpretedOption* msg) {
  const upb_MiniTableField field = {7, UPB_SIZE(40, 56), 5, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE upb_StringView google_protobuf_UninterpretedOption_string_value(const google_protobuf_UninterpretedOption* msg) {
  upb_StringView default_val = upb_StringView_FromString("");
  upb_StringView ret;
  const upb_MiniTableField field = {7, UPB_SIZE(40, 56), 5, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_UninterpretedOption_has_string_value(const google_protobuf_UninterpretedOption* msg) {
  const upb_MiniTableField field = {7, UPB_SIZE(40, 56), 5, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_UninterpretedOption_clear_aggregate_value(google_protobuf_UninterpretedOption* msg) {
  const upb_MiniTableField field = {8, UPB_SIZE(48, 72), 6, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE upb_StringView google_protobuf_UninterpretedOption_aggregate_value(const google_protobuf_UninterpretedOption* msg) {
  upb_StringView default_val = upb_StringView_FromString("");
  upb_StringView ret;
  const upb_MiniTableField field = {8, UPB_SIZE(48, 72), 6, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_UninterpretedOption_has_aggregate_value(const google_protobuf_UninterpretedOption* msg) {
  const upb_MiniTableField field = {8, UPB_SIZE(48, 72), 6, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}

UPB_INLINE google_protobuf_UninterpretedOption_NamePart** google_protobuf_UninterpretedOption_mutable_name(google_protobuf_UninterpretedOption* msg, size_t* size) {
  upb_MiniTableField field = {2, UPB_SIZE(4, 8), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetMutableArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (google_protobuf_UninterpretedOption_NamePart**)_upb_array_ptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE google_protobuf_UninterpretedOption_NamePart** google_protobuf_UninterpretedOption_resize_name(google_protobuf_UninterpretedOption* msg, size_t size, upb_Arena* arena) {
  upb_MiniTableField field = {2, UPB_SIZE(4, 8), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  return (google_protobuf_UninterpretedOption_NamePart**)upb_Message_ResizeArray(msg, &field, size, arena);
}
UPB_INLINE struct google_protobuf_UninterpretedOption_NamePart* google_protobuf_UninterpretedOption_add_name(google_protobuf_UninterpretedOption* msg, upb_Arena* arena) {
  upb_MiniTableField field = {2, UPB_SIZE(4, 8), 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(msg, &field, arena);
  if (!arr || !_upb_Array_ResizeUninitialized(arr, arr->size + 1, arena)) {
    return NULL;
  }
  struct google_protobuf_UninterpretedOption_NamePart* sub = (struct google_protobuf_UninterpretedOption_NamePart*)_upb_Message_New(&google_protobuf_UninterpretedOption_NamePart_msg_init, arena);
  if (!arr || !sub) return NULL;
  _upb_Array_Set(arr, arr->size - 1, &sub, sizeof(sub));
  return sub;
}
UPB_INLINE void google_protobuf_UninterpretedOption_set_identifier_value(google_protobuf_UninterpretedOption *msg, upb_StringView value) {
  const upb_MiniTableField field = {3, UPB_SIZE(8, 16), 1, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_UninterpretedOption_set_positive_int_value(google_protobuf_UninterpretedOption *msg, uint64_t value) {
  const upb_MiniTableField field = {4, UPB_SIZE(16, 32), 2, kUpb_NoSub, 4, kUpb_FieldMode_Scalar | (kUpb_FieldRep_8Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_UninterpretedOption_set_negative_int_value(google_protobuf_UninterpretedOption *msg, int64_t value) {
  const upb_MiniTableField field = {5, UPB_SIZE(24, 40), 3, kUpb_NoSub, 3, kUpb_FieldMode_Scalar | (kUpb_FieldRep_8Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_UninterpretedOption_set_double_value(google_protobuf_UninterpretedOption *msg, double value) {
  const upb_MiniTableField field = {6, UPB_SIZE(32, 48), 4, kUpb_NoSub, 1, kUpb_FieldMode_Scalar | (kUpb_FieldRep_8Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_UninterpretedOption_set_string_value(google_protobuf_UninterpretedOption *msg, upb_StringView value) {
  const upb_MiniTableField field = {7, UPB_SIZE(40, 56), 5, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_UninterpretedOption_set_aggregate_value(google_protobuf_UninterpretedOption *msg, upb_StringView value) {
  const upb_MiniTableField field = {8, UPB_SIZE(48, 72), 6, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}

/* google.protobuf.UninterpretedOption.NamePart */

UPB_INLINE google_protobuf_UninterpretedOption_NamePart* google_protobuf_UninterpretedOption_NamePart_new(upb_Arena* arena) {
  return (google_protobuf_UninterpretedOption_NamePart*)_upb_Message_New(&google_protobuf_UninterpretedOption_NamePart_msg_init, arena);
}
UPB_INLINE google_protobuf_UninterpretedOption_NamePart* google_protobuf_UninterpretedOption_NamePart_parse(const char* buf, size_t size, upb_Arena* arena) {
  google_protobuf_UninterpretedOption_NamePart* ret = google_protobuf_UninterpretedOption_NamePart_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_UninterpretedOption_NamePart_msg_init, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE google_protobuf_UninterpretedOption_NamePart* google_protobuf_UninterpretedOption_NamePart_parse_ex(const char* buf, size_t size,
                           const upb_ExtensionRegistry* extreg,
                           int options, upb_Arena* arena) {
  google_protobuf_UninterpretedOption_NamePart* ret = google_protobuf_UninterpretedOption_NamePart_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_UninterpretedOption_NamePart_msg_init, extreg, options, arena) !=
      kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE char* google_protobuf_UninterpretedOption_NamePart_serialize(const google_protobuf_UninterpretedOption_NamePart* msg, upb_Arena* arena, size_t* len) {
  char* ptr;
  (void)upb_Encode(msg, &google_protobuf_UninterpretedOption_NamePart_msg_init, 0, arena, &ptr, len);
  return ptr;
}
UPB_INLINE char* google_protobuf_UninterpretedOption_NamePart_serialize_ex(const google_protobuf_UninterpretedOption_NamePart* msg, int options,
                                 upb_Arena* arena, size_t* len) {
  char* ptr;
  (void)upb_Encode(msg, &google_protobuf_UninterpretedOption_NamePart_msg_init, options, arena, &ptr, len);
  return ptr;
}
UPB_INLINE void google_protobuf_UninterpretedOption_NamePart_clear_name_part(google_protobuf_UninterpretedOption_NamePart* msg) {
  const upb_MiniTableField field = {1, UPB_SIZE(4, 8), 1, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE upb_StringView google_protobuf_UninterpretedOption_NamePart_name_part(const google_protobuf_UninterpretedOption_NamePart* msg) {
  upb_StringView default_val = upb_StringView_FromString("");
  upb_StringView ret;
  const upb_MiniTableField field = {1, UPB_SIZE(4, 8), 1, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_UninterpretedOption_NamePart_has_name_part(const google_protobuf_UninterpretedOption_NamePart* msg) {
  const upb_MiniTableField field = {1, UPB_SIZE(4, 8), 1, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_UninterpretedOption_NamePart_clear_is_extension(google_protobuf_UninterpretedOption_NamePart* msg) {
  const upb_MiniTableField field = {2, 1, 2, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE bool google_protobuf_UninterpretedOption_NamePart_is_extension(const google_protobuf_UninterpretedOption_NamePart* msg) {
  bool default_val = false;
  bool ret;
  const upb_MiniTableField field = {2, 1, 2, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_UninterpretedOption_NamePart_has_is_extension(const google_protobuf_UninterpretedOption_NamePart* msg) {
  const upb_MiniTableField field = {2, 1, 2, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}

UPB_INLINE void google_protobuf_UninterpretedOption_NamePart_set_name_part(google_protobuf_UninterpretedOption_NamePart *msg, upb_StringView value) {
  const upb_MiniTableField field = {1, UPB_SIZE(4, 8), 1, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_UninterpretedOption_NamePart_set_is_extension(google_protobuf_UninterpretedOption_NamePart *msg, bool value) {
  const upb_MiniTableField field = {2, 1, 2, kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}

/* google.protobuf.SourceCodeInfo */

UPB_INLINE google_protobuf_SourceCodeInfo* google_protobuf_SourceCodeInfo_new(upb_Arena* arena) {
  return (google_protobuf_SourceCodeInfo*)_upb_Message_New(&google_protobuf_SourceCodeInfo_msg_init, arena);
}
UPB_INLINE google_protobuf_SourceCodeInfo* google_protobuf_SourceCodeInfo_parse(const char* buf, size_t size, upb_Arena* arena) {
  google_protobuf_SourceCodeInfo* ret = google_protobuf_SourceCodeInfo_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_SourceCodeInfo_msg_init, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE google_protobuf_SourceCodeInfo* google_protobuf_SourceCodeInfo_parse_ex(const char* buf, size_t size,
                           const upb_ExtensionRegistry* extreg,
                           int options, upb_Arena* arena) {
  google_protobuf_SourceCodeInfo* ret = google_protobuf_SourceCodeInfo_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_SourceCodeInfo_msg_init, extreg, options, arena) !=
      kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE char* google_protobuf_SourceCodeInfo_serialize(const google_protobuf_SourceCodeInfo* msg, upb_Arena* arena, size_t* len) {
  char* ptr;
  (void)upb_Encode(msg, &google_protobuf_SourceCodeInfo_msg_init, 0, arena, &ptr, len);
  return ptr;
}
UPB_INLINE char* google_protobuf_SourceCodeInfo_serialize_ex(const google_protobuf_SourceCodeInfo* msg, int options,
                                 upb_Arena* arena, size_t* len) {
  char* ptr;
  (void)upb_Encode(msg, &google_protobuf_SourceCodeInfo_msg_init, options, arena, &ptr, len);
  return ptr;
}
UPB_INLINE void google_protobuf_SourceCodeInfo_clear_location(google_protobuf_SourceCodeInfo* msg) {
  const upb_MiniTableField field = {1, 0, 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE const google_protobuf_SourceCodeInfo_Location* const* google_protobuf_SourceCodeInfo_location(const google_protobuf_SourceCodeInfo* msg, size_t* size) {
  const upb_MiniTableField field = {1, 0, 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (const google_protobuf_SourceCodeInfo_Location* const*)_upb_array_constptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE const upb_Array* _google_protobuf_SourceCodeInfo_location_upb_array(const google_protobuf_SourceCodeInfo* msg, size_t* size) {
  const upb_MiniTableField field = {1, 0, 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE upb_Array* _google_protobuf_SourceCodeInfo_location_mutable_upb_array(const google_protobuf_SourceCodeInfo* msg, size_t* size, upb_Arena* arena) {
  const upb_MiniTableField field = {1, 0, 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(
      (upb_Message*)msg, &field, arena);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE bool google_protobuf_SourceCodeInfo_has_location(const google_protobuf_SourceCodeInfo* msg) {
  size_t size;
  google_protobuf_SourceCodeInfo_location(msg, &size);
  return size != 0;
}

UPB_INLINE google_protobuf_SourceCodeInfo_Location** google_protobuf_SourceCodeInfo_mutable_location(google_protobuf_SourceCodeInfo* msg, size_t* size) {
  upb_MiniTableField field = {1, 0, 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetMutableArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (google_protobuf_SourceCodeInfo_Location**)_upb_array_ptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE google_protobuf_SourceCodeInfo_Location** google_protobuf_SourceCodeInfo_resize_location(google_protobuf_SourceCodeInfo* msg, size_t size, upb_Arena* arena) {
  upb_MiniTableField field = {1, 0, 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  return (google_protobuf_SourceCodeInfo_Location**)upb_Message_ResizeArray(msg, &field, size, arena);
}
UPB_INLINE struct google_protobuf_SourceCodeInfo_Location* google_protobuf_SourceCodeInfo_add_location(google_protobuf_SourceCodeInfo* msg, upb_Arena* arena) {
  upb_MiniTableField field = {1, 0, 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(msg, &field, arena);
  if (!arr || !_upb_Array_ResizeUninitialized(arr, arr->size + 1, arena)) {
    return NULL;
  }
  struct google_protobuf_SourceCodeInfo_Location* sub = (struct google_protobuf_SourceCodeInfo_Location*)_upb_Message_New(&google_protobuf_SourceCodeInfo_Location_msg_init, arena);
  if (!arr || !sub) return NULL;
  _upb_Array_Set(arr, arr->size - 1, &sub, sizeof(sub));
  return sub;
}

/* google.protobuf.SourceCodeInfo.Location */

UPB_INLINE google_protobuf_SourceCodeInfo_Location* google_protobuf_SourceCodeInfo_Location_new(upb_Arena* arena) {
  return (google_protobuf_SourceCodeInfo_Location*)_upb_Message_New(&google_protobuf_SourceCodeInfo_Location_msg_init, arena);
}
UPB_INLINE google_protobuf_SourceCodeInfo_Location* google_protobuf_SourceCodeInfo_Location_parse(const char* buf, size_t size, upb_Arena* arena) {
  google_protobuf_SourceCodeInfo_Location* ret = google_protobuf_SourceCodeInfo_Location_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_SourceCodeInfo_Location_msg_init, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE google_protobuf_SourceCodeInfo_Location* google_protobuf_SourceCodeInfo_Location_parse_ex(const char* buf, size_t size,
                           const upb_ExtensionRegistry* extreg,
                           int options, upb_Arena* arena) {
  google_protobuf_SourceCodeInfo_Location* ret = google_protobuf_SourceCodeInfo_Location_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_SourceCodeInfo_Location_msg_init, extreg, options, arena) !=
      kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE char* google_protobuf_SourceCodeInfo_Location_serialize(const google_protobuf_SourceCodeInfo_Location* msg, upb_Arena* arena, size_t* len) {
  char* ptr;
  (void)upb_Encode(msg, &google_protobuf_SourceCodeInfo_Location_msg_init, 0, arena, &ptr, len);
  return ptr;
}
UPB_INLINE char* google_protobuf_SourceCodeInfo_Location_serialize_ex(const google_protobuf_SourceCodeInfo_Location* msg, int options,
                                 upb_Arena* arena, size_t* len) {
  char* ptr;
  (void)upb_Encode(msg, &google_protobuf_SourceCodeInfo_Location_msg_init, options, arena, &ptr, len);
  return ptr;
}
UPB_INLINE void google_protobuf_SourceCodeInfo_Location_clear_path(google_protobuf_SourceCodeInfo_Location* msg) {
  const upb_MiniTableField field = {1, UPB_SIZE(4, 8), 0, kUpb_NoSub, 5, kUpb_FieldMode_Array | kUpb_LabelFlags_IsPacked | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE int32_t const* google_protobuf_SourceCodeInfo_Location_path(const google_protobuf_SourceCodeInfo_Location* msg, size_t* size) {
  const upb_MiniTableField field = {1, UPB_SIZE(4, 8), 0, kUpb_NoSub, 5, kUpb_FieldMode_Array | kUpb_LabelFlags_IsPacked | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (int32_t const*)_upb_array_constptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE const upb_Array* _google_protobuf_SourceCodeInfo_Location_path_upb_array(const google_protobuf_SourceCodeInfo_Location* msg, size_t* size) {
  const upb_MiniTableField field = {1, UPB_SIZE(4, 8), 0, kUpb_NoSub, 5, kUpb_FieldMode_Array | kUpb_LabelFlags_IsPacked | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE upb_Array* _google_protobuf_SourceCodeInfo_Location_path_mutable_upb_array(const google_protobuf_SourceCodeInfo_Location* msg, size_t* size, upb_Arena* arena) {
  const upb_MiniTableField field = {1, UPB_SIZE(4, 8), 0, kUpb_NoSub, 5, kUpb_FieldMode_Array | kUpb_LabelFlags_IsPacked | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(
      (upb_Message*)msg, &field, arena);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE bool google_protobuf_SourceCodeInfo_Location_has_path(const google_protobuf_SourceCodeInfo_Location* msg) {
  size_t size;
  google_protobuf_SourceCodeInfo_Location_path(msg, &size);
  return size != 0;
}
UPB_INLINE void google_protobuf_SourceCodeInfo_Location_clear_span(google_protobuf_SourceCodeInfo_Location* msg) {
  const upb_MiniTableField field = {2, UPB_SIZE(8, 16), 0, kUpb_NoSub, 5, kUpb_FieldMode_Array | kUpb_LabelFlags_IsPacked | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE int32_t const* google_protobuf_SourceCodeInfo_Location_span(const google_protobuf_SourceCodeInfo_Location* msg, size_t* size) {
  const upb_MiniTableField field = {2, UPB_SIZE(8, 16), 0, kUpb_NoSub, 5, kUpb_FieldMode_Array | kUpb_LabelFlags_IsPacked | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (int32_t const*)_upb_array_constptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE const upb_Array* _google_protobuf_SourceCodeInfo_Location_span_upb_array(const google_protobuf_SourceCodeInfo_Location* msg, size_t* size) {
  const upb_MiniTableField field = {2, UPB_SIZE(8, 16), 0, kUpb_NoSub, 5, kUpb_FieldMode_Array | kUpb_LabelFlags_IsPacked | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE upb_Array* _google_protobuf_SourceCodeInfo_Location_span_mutable_upb_array(const google_protobuf_SourceCodeInfo_Location* msg, size_t* size, upb_Arena* arena) {
  const upb_MiniTableField field = {2, UPB_SIZE(8, 16), 0, kUpb_NoSub, 5, kUpb_FieldMode_Array | kUpb_LabelFlags_IsPacked | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(
      (upb_Message*)msg, &field, arena);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE bool google_protobuf_SourceCodeInfo_Location_has_span(const google_protobuf_SourceCodeInfo_Location* msg) {
  size_t size;
  google_protobuf_SourceCodeInfo_Location_span(msg, &size);
  return size != 0;
}
UPB_INLINE void google_protobuf_SourceCodeInfo_Location_clear_leading_comments(google_protobuf_SourceCodeInfo_Location* msg) {
  const upb_MiniTableField field = {3, UPB_SIZE(16, 24), 1, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE upb_StringView google_protobuf_SourceCodeInfo_Location_leading_comments(const google_protobuf_SourceCodeInfo_Location* msg) {
  upb_StringView default_val = upb_StringView_FromString("");
  upb_StringView ret;
  const upb_MiniTableField field = {3, UPB_SIZE(16, 24), 1, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_SourceCodeInfo_Location_has_leading_comments(const google_protobuf_SourceCodeInfo_Location* msg) {
  const upb_MiniTableField field = {3, UPB_SIZE(16, 24), 1, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_SourceCodeInfo_Location_clear_trailing_comments(google_protobuf_SourceCodeInfo_Location* msg) {
  const upb_MiniTableField field = {4, UPB_SIZE(24, 40), 2, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE upb_StringView google_protobuf_SourceCodeInfo_Location_trailing_comments(const google_protobuf_SourceCodeInfo_Location* msg) {
  upb_StringView default_val = upb_StringView_FromString("");
  upb_StringView ret;
  const upb_MiniTableField field = {4, UPB_SIZE(24, 40), 2, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_SourceCodeInfo_Location_has_trailing_comments(const google_protobuf_SourceCodeInfo_Location* msg) {
  const upb_MiniTableField field = {4, UPB_SIZE(24, 40), 2, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_SourceCodeInfo_Location_clear_leading_detached_comments(google_protobuf_SourceCodeInfo_Location* msg) {
  const upb_MiniTableField field = {6, UPB_SIZE(12, 56), 0, kUpb_NoSub, 12, kUpb_FieldMode_Array | kUpb_LabelFlags_IsAlternate | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE upb_StringView const* google_protobuf_SourceCodeInfo_Location_leading_detached_comments(const google_protobuf_SourceCodeInfo_Location* msg, size_t* size) {
  const upb_MiniTableField field = {6, UPB_SIZE(12, 56), 0, kUpb_NoSub, 12, kUpb_FieldMode_Array | kUpb_LabelFlags_IsAlternate | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (upb_StringView const*)_upb_array_constptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE const upb_Array* _google_protobuf_SourceCodeInfo_Location_leading_detached_comments_upb_array(const google_protobuf_SourceCodeInfo_Location* msg, size_t* size) {
  const upb_MiniTableField field = {6, UPB_SIZE(12, 56), 0, kUpb_NoSub, 12, kUpb_FieldMode_Array | kUpb_LabelFlags_IsAlternate | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE upb_Array* _google_protobuf_SourceCodeInfo_Location_leading_detached_comments_mutable_upb_array(const google_protobuf_SourceCodeInfo_Location* msg, size_t* size, upb_Arena* arena) {
  const upb_MiniTableField field = {6, UPB_SIZE(12, 56), 0, kUpb_NoSub, 12, kUpb_FieldMode_Array | kUpb_LabelFlags_IsAlternate | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(
      (upb_Message*)msg, &field, arena);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE bool google_protobuf_SourceCodeInfo_Location_has_leading_detached_comments(const google_protobuf_SourceCodeInfo_Location* msg) {
  size_t size;
  google_protobuf_SourceCodeInfo_Location_leading_detached_comments(msg, &size);
  return size != 0;
}

UPB_INLINE int32_t* google_protobuf_SourceCodeInfo_Location_mutable_path(google_protobuf_SourceCodeInfo_Location* msg, size_t* size) {
  upb_MiniTableField field = {1, UPB_SIZE(4, 8), 0, kUpb_NoSub, 5, kUpb_FieldMode_Array | kUpb_LabelFlags_IsPacked | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetMutableArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (int32_t*)_upb_array_ptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE int32_t* google_protobuf_SourceCodeInfo_Location_resize_path(google_protobuf_SourceCodeInfo_Location* msg, size_t size, upb_Arena* arena) {
  upb_MiniTableField field = {1, UPB_SIZE(4, 8), 0, kUpb_NoSub, 5, kUpb_FieldMode_Array | kUpb_LabelFlags_IsPacked | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  return (int32_t*)upb_Message_ResizeArray(msg, &field, size, arena);
}
UPB_INLINE bool google_protobuf_SourceCodeInfo_Location_add_path(google_protobuf_SourceCodeInfo_Location* msg, int32_t val, upb_Arena* arena) {
  upb_MiniTableField field = {1, UPB_SIZE(4, 8), 0, kUpb_NoSub, 5, kUpb_FieldMode_Array | kUpb_LabelFlags_IsPacked | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(msg, &field, arena);
  if (!arr || !_upb_Array_ResizeUninitialized(arr, arr->size + 1, arena)) {
    return false;
  }
  _upb_Array_Set(arr, arr->size - 1, &val, sizeof(val));
  return true;
}
UPB_INLINE int32_t* google_protobuf_SourceCodeInfo_Location_mutable_span(google_protobuf_SourceCodeInfo_Location* msg, size_t* size) {
  upb_MiniTableField field = {2, UPB_SIZE(8, 16), 0, kUpb_NoSub, 5, kUpb_FieldMode_Array | kUpb_LabelFlags_IsPacked | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetMutableArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (int32_t*)_upb_array_ptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE int32_t* google_protobuf_SourceCodeInfo_Location_resize_span(google_protobuf_SourceCodeInfo_Location* msg, size_t size, upb_Arena* arena) {
  upb_MiniTableField field = {2, UPB_SIZE(8, 16), 0, kUpb_NoSub, 5, kUpb_FieldMode_Array | kUpb_LabelFlags_IsPacked | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  return (int32_t*)upb_Message_ResizeArray(msg, &field, size, arena);
}
UPB_INLINE bool google_protobuf_SourceCodeInfo_Location_add_span(google_protobuf_SourceCodeInfo_Location* msg, int32_t val, upb_Arena* arena) {
  upb_MiniTableField field = {2, UPB_SIZE(8, 16), 0, kUpb_NoSub, 5, kUpb_FieldMode_Array | kUpb_LabelFlags_IsPacked | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(msg, &field, arena);
  if (!arr || !_upb_Array_ResizeUninitialized(arr, arr->size + 1, arena)) {
    return false;
  }
  _upb_Array_Set(arr, arr->size - 1, &val, sizeof(val));
  return true;
}
UPB_INLINE void google_protobuf_SourceCodeInfo_Location_set_leading_comments(google_protobuf_SourceCodeInfo_Location *msg, upb_StringView value) {
  const upb_MiniTableField field = {3, UPB_SIZE(16, 24), 1, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_SourceCodeInfo_Location_set_trailing_comments(google_protobuf_SourceCodeInfo_Location *msg, upb_StringView value) {
  const upb_MiniTableField field = {4, UPB_SIZE(24, 40), 2, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE upb_StringView* google_protobuf_SourceCodeInfo_Location_mutable_leading_detached_comments(google_protobuf_SourceCodeInfo_Location* msg, size_t* size) {
  upb_MiniTableField field = {6, UPB_SIZE(12, 56), 0, kUpb_NoSub, 12, kUpb_FieldMode_Array | kUpb_LabelFlags_IsAlternate | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetMutableArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (upb_StringView*)_upb_array_ptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE upb_StringView* google_protobuf_SourceCodeInfo_Location_resize_leading_detached_comments(google_protobuf_SourceCodeInfo_Location* msg, size_t size, upb_Arena* arena) {
  upb_MiniTableField field = {6, UPB_SIZE(12, 56), 0, kUpb_NoSub, 12, kUpb_FieldMode_Array | kUpb_LabelFlags_IsAlternate | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  return (upb_StringView*)upb_Message_ResizeArray(msg, &field, size, arena);
}
UPB_INLINE bool google_protobuf_SourceCodeInfo_Location_add_leading_detached_comments(google_protobuf_SourceCodeInfo_Location* msg, upb_StringView val, upb_Arena* arena) {
  upb_MiniTableField field = {6, UPB_SIZE(12, 56), 0, kUpb_NoSub, 12, kUpb_FieldMode_Array | kUpb_LabelFlags_IsAlternate | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(msg, &field, arena);
  if (!arr || !_upb_Array_ResizeUninitialized(arr, arr->size + 1, arena)) {
    return false;
  }
  _upb_Array_Set(arr, arr->size - 1, &val, sizeof(val));
  return true;
}

/* google.protobuf.GeneratedCodeInfo */

UPB_INLINE google_protobuf_GeneratedCodeInfo* google_protobuf_GeneratedCodeInfo_new(upb_Arena* arena) {
  return (google_protobuf_GeneratedCodeInfo*)_upb_Message_New(&google_protobuf_GeneratedCodeInfo_msg_init, arena);
}
UPB_INLINE google_protobuf_GeneratedCodeInfo* google_protobuf_GeneratedCodeInfo_parse(const char* buf, size_t size, upb_Arena* arena) {
  google_protobuf_GeneratedCodeInfo* ret = google_protobuf_GeneratedCodeInfo_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_GeneratedCodeInfo_msg_init, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE google_protobuf_GeneratedCodeInfo* google_protobuf_GeneratedCodeInfo_parse_ex(const char* buf, size_t size,
                           const upb_ExtensionRegistry* extreg,
                           int options, upb_Arena* arena) {
  google_protobuf_GeneratedCodeInfo* ret = google_protobuf_GeneratedCodeInfo_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_GeneratedCodeInfo_msg_init, extreg, options, arena) !=
      kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE char* google_protobuf_GeneratedCodeInfo_serialize(const google_protobuf_GeneratedCodeInfo* msg, upb_Arena* arena, size_t* len) {
  char* ptr;
  (void)upb_Encode(msg, &google_protobuf_GeneratedCodeInfo_msg_init, 0, arena, &ptr, len);
  return ptr;
}
UPB_INLINE char* google_protobuf_GeneratedCodeInfo_serialize_ex(const google_protobuf_GeneratedCodeInfo* msg, int options,
                                 upb_Arena* arena, size_t* len) {
  char* ptr;
  (void)upb_Encode(msg, &google_protobuf_GeneratedCodeInfo_msg_init, options, arena, &ptr, len);
  return ptr;
}
UPB_INLINE void google_protobuf_GeneratedCodeInfo_clear_annotation(google_protobuf_GeneratedCodeInfo* msg) {
  const upb_MiniTableField field = {1, 0, 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE const google_protobuf_GeneratedCodeInfo_Annotation* const* google_protobuf_GeneratedCodeInfo_annotation(const google_protobuf_GeneratedCodeInfo* msg, size_t* size) {
  const upb_MiniTableField field = {1, 0, 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (const google_protobuf_GeneratedCodeInfo_Annotation* const*)_upb_array_constptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE const upb_Array* _google_protobuf_GeneratedCodeInfo_annotation_upb_array(const google_protobuf_GeneratedCodeInfo* msg, size_t* size) {
  const upb_MiniTableField field = {1, 0, 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE upb_Array* _google_protobuf_GeneratedCodeInfo_annotation_mutable_upb_array(const google_protobuf_GeneratedCodeInfo* msg, size_t* size, upb_Arena* arena) {
  const upb_MiniTableField field = {1, 0, 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(
      (upb_Message*)msg, &field, arena);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE bool google_protobuf_GeneratedCodeInfo_has_annotation(const google_protobuf_GeneratedCodeInfo* msg) {
  size_t size;
  google_protobuf_GeneratedCodeInfo_annotation(msg, &size);
  return size != 0;
}

UPB_INLINE google_protobuf_GeneratedCodeInfo_Annotation** google_protobuf_GeneratedCodeInfo_mutable_annotation(google_protobuf_GeneratedCodeInfo* msg, size_t* size) {
  upb_MiniTableField field = {1, 0, 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetMutableArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (google_protobuf_GeneratedCodeInfo_Annotation**)_upb_array_ptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE google_protobuf_GeneratedCodeInfo_Annotation** google_protobuf_GeneratedCodeInfo_resize_annotation(google_protobuf_GeneratedCodeInfo* msg, size_t size, upb_Arena* arena) {
  upb_MiniTableField field = {1, 0, 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  return (google_protobuf_GeneratedCodeInfo_Annotation**)upb_Message_ResizeArray(msg, &field, size, arena);
}
UPB_INLINE struct google_protobuf_GeneratedCodeInfo_Annotation* google_protobuf_GeneratedCodeInfo_add_annotation(google_protobuf_GeneratedCodeInfo* msg, upb_Arena* arena) {
  upb_MiniTableField field = {1, 0, 0, 0, 11, kUpb_FieldMode_Array | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(msg, &field, arena);
  if (!arr || !_upb_Array_ResizeUninitialized(arr, arr->size + 1, arena)) {
    return NULL;
  }
  struct google_protobuf_GeneratedCodeInfo_Annotation* sub = (struct google_protobuf_GeneratedCodeInfo_Annotation*)_upb_Message_New(&google_protobuf_GeneratedCodeInfo_Annotation_msg_init, arena);
  if (!arr || !sub) return NULL;
  _upb_Array_Set(arr, arr->size - 1, &sub, sizeof(sub));
  return sub;
}

/* google.protobuf.GeneratedCodeInfo.Annotation */

UPB_INLINE google_protobuf_GeneratedCodeInfo_Annotation* google_protobuf_GeneratedCodeInfo_Annotation_new(upb_Arena* arena) {
  return (google_protobuf_GeneratedCodeInfo_Annotation*)_upb_Message_New(&google_protobuf_GeneratedCodeInfo_Annotation_msg_init, arena);
}
UPB_INLINE google_protobuf_GeneratedCodeInfo_Annotation* google_protobuf_GeneratedCodeInfo_Annotation_parse(const char* buf, size_t size, upb_Arena* arena) {
  google_protobuf_GeneratedCodeInfo_Annotation* ret = google_protobuf_GeneratedCodeInfo_Annotation_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_GeneratedCodeInfo_Annotation_msg_init, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE google_protobuf_GeneratedCodeInfo_Annotation* google_protobuf_GeneratedCodeInfo_Annotation_parse_ex(const char* buf, size_t size,
                           const upb_ExtensionRegistry* extreg,
                           int options, upb_Arena* arena) {
  google_protobuf_GeneratedCodeInfo_Annotation* ret = google_protobuf_GeneratedCodeInfo_Annotation_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_GeneratedCodeInfo_Annotation_msg_init, extreg, options, arena) !=
      kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE char* google_protobuf_GeneratedCodeInfo_Annotation_serialize(const google_protobuf_GeneratedCodeInfo_Annotation* msg, upb_Arena* arena, size_t* len) {
  char* ptr;
  (void)upb_Encode(msg, &google_protobuf_GeneratedCodeInfo_Annotation_msg_init, 0, arena, &ptr, len);
  return ptr;
}
UPB_INLINE char* google_protobuf_GeneratedCodeInfo_Annotation_serialize_ex(const google_protobuf_GeneratedCodeInfo_Annotation* msg, int options,
                                 upb_Arena* arena, size_t* len) {
  char* ptr;
  (void)upb_Encode(msg, &google_protobuf_GeneratedCodeInfo_Annotation_msg_init, options, arena, &ptr, len);
  return ptr;
}
UPB_INLINE void google_protobuf_GeneratedCodeInfo_Annotation_clear_path(google_protobuf_GeneratedCodeInfo_Annotation* msg) {
  const upb_MiniTableField field = {1, UPB_SIZE(4, 16), 0, kUpb_NoSub, 5, kUpb_FieldMode_Array | kUpb_LabelFlags_IsPacked | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE int32_t const* google_protobuf_GeneratedCodeInfo_Annotation_path(const google_protobuf_GeneratedCodeInfo_Annotation* msg, size_t* size) {
  const upb_MiniTableField field = {1, UPB_SIZE(4, 16), 0, kUpb_NoSub, 5, kUpb_FieldMode_Array | kUpb_LabelFlags_IsPacked | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (int32_t const*)_upb_array_constptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE const upb_Array* _google_protobuf_GeneratedCodeInfo_Annotation_path_upb_array(const google_protobuf_GeneratedCodeInfo_Annotation* msg, size_t* size) {
  const upb_MiniTableField field = {1, UPB_SIZE(4, 16), 0, kUpb_NoSub, 5, kUpb_FieldMode_Array | kUpb_LabelFlags_IsPacked | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  const upb_Array* arr = upb_Message_GetArray(msg, &field);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE upb_Array* _google_protobuf_GeneratedCodeInfo_Annotation_path_mutable_upb_array(const google_protobuf_GeneratedCodeInfo_Annotation* msg, size_t* size, upb_Arena* arena) {
  const upb_MiniTableField field = {1, UPB_SIZE(4, 16), 0, kUpb_NoSub, 5, kUpb_FieldMode_Array | kUpb_LabelFlags_IsPacked | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(
      (upb_Message*)msg, &field, arena);
  if (size) {
    *size = arr ? arr->size : 0;
  }
  return arr;
}
UPB_INLINE bool google_protobuf_GeneratedCodeInfo_Annotation_has_path(const google_protobuf_GeneratedCodeInfo_Annotation* msg) {
  size_t size;
  google_protobuf_GeneratedCodeInfo_Annotation_path(msg, &size);
  return size != 0;
}
UPB_INLINE void google_protobuf_GeneratedCodeInfo_Annotation_clear_source_file(google_protobuf_GeneratedCodeInfo_Annotation* msg) {
  const upb_MiniTableField field = {2, UPB_SIZE(20, 24), 1, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE upb_StringView google_protobuf_GeneratedCodeInfo_Annotation_source_file(const google_protobuf_GeneratedCodeInfo_Annotation* msg) {
  upb_StringView default_val = upb_StringView_FromString("");
  upb_StringView ret;
  const upb_MiniTableField field = {2, UPB_SIZE(20, 24), 1, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_GeneratedCodeInfo_Annotation_has_source_file(const google_protobuf_GeneratedCodeInfo_Annotation* msg) {
  const upb_MiniTableField field = {2, UPB_SIZE(20, 24), 1, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_GeneratedCodeInfo_Annotation_clear_begin(google_protobuf_GeneratedCodeInfo_Annotation* msg) {
  const upb_MiniTableField field = {3, UPB_SIZE(8, 4), 2, kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE int32_t google_protobuf_GeneratedCodeInfo_Annotation_begin(const google_protobuf_GeneratedCodeInfo_Annotation* msg) {
  int32_t default_val = (int32_t)0;
  int32_t ret;
  const upb_MiniTableField field = {3, UPB_SIZE(8, 4), 2, kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_GeneratedCodeInfo_Annotation_has_begin(const google_protobuf_GeneratedCodeInfo_Annotation* msg) {
  const upb_MiniTableField field = {3, UPB_SIZE(8, 4), 2, kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_GeneratedCodeInfo_Annotation_clear_end(google_protobuf_GeneratedCodeInfo_Annotation* msg) {
  const upb_MiniTableField field = {4, UPB_SIZE(12, 8), 3, kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE int32_t google_protobuf_GeneratedCodeInfo_Annotation_end(const google_protobuf_GeneratedCodeInfo_Annotation* msg) {
  int32_t default_val = (int32_t)0;
  int32_t ret;
  const upb_MiniTableField field = {4, UPB_SIZE(12, 8), 3, kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_GeneratedCodeInfo_Annotation_has_end(const google_protobuf_GeneratedCodeInfo_Annotation* msg) {
  const upb_MiniTableField field = {4, UPB_SIZE(12, 8), 3, kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}
UPB_INLINE void google_protobuf_GeneratedCodeInfo_Annotation_clear_semantic(google_protobuf_GeneratedCodeInfo_Annotation* msg) {
  const upb_MiniTableField field = {5, UPB_SIZE(16, 12), 4, 0, 14, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_ClearNonExtensionField(msg, &field);
}
UPB_INLINE int32_t google_protobuf_GeneratedCodeInfo_Annotation_semantic(const google_protobuf_GeneratedCodeInfo_Annotation* msg) {
  int32_t default_val = 0;
  int32_t ret;
  const upb_MiniTableField field = {5, UPB_SIZE(16, 12), 4, 0, 14, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_GetNonExtensionField(msg, &field, &default_val, &ret);
  return ret;
}
UPB_INLINE bool google_protobuf_GeneratedCodeInfo_Annotation_has_semantic(const google_protobuf_GeneratedCodeInfo_Annotation* msg) {
  const upb_MiniTableField field = {5, UPB_SIZE(16, 12), 4, 0, 14, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  return _upb_Message_HasNonExtensionField(msg, &field);
}

UPB_INLINE int32_t* google_protobuf_GeneratedCodeInfo_Annotation_mutable_path(google_protobuf_GeneratedCodeInfo_Annotation* msg, size_t* size) {
  upb_MiniTableField field = {1, UPB_SIZE(4, 16), 0, kUpb_NoSub, 5, kUpb_FieldMode_Array | kUpb_LabelFlags_IsPacked | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetMutableArray(msg, &field);
  if (arr) {
    if (size) *size = arr->size;
    return (int32_t*)_upb_array_ptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}
UPB_INLINE int32_t* google_protobuf_GeneratedCodeInfo_Annotation_resize_path(google_protobuf_GeneratedCodeInfo_Annotation* msg, size_t size, upb_Arena* arena) {
  upb_MiniTableField field = {1, UPB_SIZE(4, 16), 0, kUpb_NoSub, 5, kUpb_FieldMode_Array | kUpb_LabelFlags_IsPacked | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  return (int32_t*)upb_Message_ResizeArray(msg, &field, size, arena);
}
UPB_INLINE bool google_protobuf_GeneratedCodeInfo_Annotation_add_path(google_protobuf_GeneratedCodeInfo_Annotation* msg, int32_t val, upb_Arena* arena) {
  upb_MiniTableField field = {1, UPB_SIZE(4, 16), 0, kUpb_NoSub, 5, kUpb_FieldMode_Array | kUpb_LabelFlags_IsPacked | (UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)};
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(msg, &field, arena);
  if (!arr || !_upb_Array_ResizeUninitialized(arr, arr->size + 1, arena)) {
    return false;
  }
  _upb_Array_Set(arr, arr->size - 1, &val, sizeof(val));
  return true;
}
UPB_INLINE void google_protobuf_GeneratedCodeInfo_Annotation_set_source_file(google_protobuf_GeneratedCodeInfo_Annotation *msg, upb_StringView value) {
  const upb_MiniTableField field = {2, UPB_SIZE(20, 24), 1, kUpb_NoSub, 12, kUpb_FieldMode_Scalar | kUpb_LabelFlags_IsAlternate | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_GeneratedCodeInfo_Annotation_set_begin(google_protobuf_GeneratedCodeInfo_Annotation *msg, int32_t value) {
  const upb_MiniTableField field = {3, UPB_SIZE(8, 4), 2, kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_GeneratedCodeInfo_Annotation_set_end(google_protobuf_GeneratedCodeInfo_Annotation *msg, int32_t value) {
  const upb_MiniTableField field = {4, UPB_SIZE(12, 8), 3, kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}
UPB_INLINE void google_protobuf_GeneratedCodeInfo_Annotation_set_semantic(google_protobuf_GeneratedCodeInfo_Annotation *msg, int32_t value) {
  const upb_MiniTableField field = {5, UPB_SIZE(16, 12), 4, 0, 14, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)};
  _upb_Message_SetNonExtensionField(msg, &field, &value);
}

extern const upb_MiniTableFile google_protobuf_descriptor_proto_upb_file_layout;

/* Max size 32 is google.protobuf.FileOptions */
/* Max size 64 is google.protobuf.FileOptions */
#define _UPB_MAXOPT_SIZE UPB_SIZE(104, 192)

#ifdef __cplusplus
}  /* extern "C" */
#endif


#endif  /* GOOGLE_PROTOBUF_DESCRIPTOR_PROTO_UPB_H_ */

#ifndef UPB_REFLECTION_DEF_H_
#define UPB_REFLECTION_DEF_H_


// IWYU pragma: private, include "upb/reflection/def.h"

#ifndef UPB_REFLECTION_DEF_POOL_H_
#define UPB_REFLECTION_DEF_POOL_H_


// IWYU pragma: private, include "upb/reflection/def.h"

// Declarations common to all public def types.

#ifndef UPB_REFLECTION_COMMON_H_
#define UPB_REFLECTION_COMMON_H_

// begin:google_only
// #ifndef UPB_BOOTSTRAP_STAGE0
// #include "net/proto2/proto/descriptor.upb.h"
// #else
// #include "google/protobuf/descriptor.upb.h"
// #endif
// end:google_only

// begin:github_only
// end:github_only

typedef enum { kUpb_Syntax_Proto2 = 2, kUpb_Syntax_Proto3 = 3 } upb_Syntax;

// Forward declarations for circular references.
typedef struct upb_DefPool upb_DefPool;
typedef struct upb_EnumDef upb_EnumDef;
typedef struct upb_EnumReservedRange upb_EnumReservedRange;
typedef struct upb_EnumValueDef upb_EnumValueDef;
typedef struct upb_ExtensionRange upb_ExtensionRange;
typedef struct upb_FieldDef upb_FieldDef;
typedef struct upb_FileDef upb_FileDef;
typedef struct upb_MessageDef upb_MessageDef;
typedef struct upb_MessageReservedRange upb_MessageReservedRange;
typedef struct upb_MethodDef upb_MethodDef;
typedef struct upb_OneofDef upb_OneofDef;
typedef struct upb_ServiceDef upb_ServiceDef;

// EVERYTHING BELOW THIS LINE IS INTERNAL - DO NOT USE /////////////////////////

typedef struct upb_DefBuilder upb_DefBuilder;

#endif /* UPB_REFLECTION_COMMON_H_ */

#ifndef UPB_REFLECTION_DEF_TYPE_H_
#define UPB_REFLECTION_DEF_TYPE_H_


// Must be last.

// Inside a symtab we store tagged pointers to specific def types.
typedef enum {
  UPB_DEFTYPE_MASK = 7,

  // Only inside symtab table.
  UPB_DEFTYPE_EXT = 0,
  UPB_DEFTYPE_MSG = 1,
  UPB_DEFTYPE_ENUM = 2,
  UPB_DEFTYPE_ENUMVAL = 3,
  UPB_DEFTYPE_SERVICE = 4,

  // Only inside message table.
  UPB_DEFTYPE_FIELD = 0,
  UPB_DEFTYPE_ONEOF = 1,
  UPB_DEFTYPE_FIELD_JSONNAME = 2,
} upb_deftype_t;

#ifdef __cplusplus
extern "C" {
#endif

// Our 3-bit pointer tagging requires all pointers to be multiples of 8.
// The arena will always yield 8-byte-aligned addresses, however we put
// the defs into arrays. For each element in the array to be 8-byte-aligned,
// the sizes of each def type must also be a multiple of 8.
//
// If any of these asserts fail, we need to add or remove padding on 32-bit
// machines (64-bit machines will have 8-byte alignment already due to
// pointers, which all of these structs have).
UPB_INLINE void _upb_DefType_CheckPadding(size_t size) {
  UPB_ASSERT((size & UPB_DEFTYPE_MASK) == 0);
}

upb_deftype_t _upb_DefType_Type(upb_value v);

upb_value _upb_DefType_Pack(const void* ptr, upb_deftype_t type);

const void* _upb_DefType_Unpack(upb_value v, upb_deftype_t type);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_REFLECTION_DEF_TYPE_H_ */

// Must be last.

#ifdef __cplusplus
extern "C" {
#endif

void upb_DefPool_Free(upb_DefPool* s);

upb_DefPool* upb_DefPool_New(void);

const upb_MessageDef* upb_DefPool_FindMessageByName(const upb_DefPool* s,
                                                    const char* sym);

const upb_MessageDef* upb_DefPool_FindMessageByNameWithSize(
    const upb_DefPool* s, const char* sym, size_t len);

const upb_EnumDef* upb_DefPool_FindEnumByName(const upb_DefPool* s,
                                              const char* sym);

const upb_EnumValueDef* upb_DefPool_FindEnumByNameval(const upb_DefPool* s,
                                                      const char* sym);

const upb_FileDef* upb_DefPool_FindFileByName(const upb_DefPool* s,
                                              const char* name);

const upb_FileDef* upb_DefPool_FindFileByNameWithSize(const upb_DefPool* s,
                                                      const char* name,
                                                      size_t len);

const upb_FieldDef* upb_DefPool_FindExtensionByMiniTable(
    const upb_DefPool* s, const upb_MiniTableExtension* ext);

const upb_FieldDef* upb_DefPool_FindExtensionByName(const upb_DefPool* s,
                                                    const char* sym);

const upb_FieldDef* upb_DefPool_FindExtensionByNameWithSize(
    const upb_DefPool* s, const char* name, size_t size);

const upb_FieldDef* upb_DefPool_FindExtensionByNumber(const upb_DefPool* s,
                                                      const upb_MessageDef* m,
                                                      int32_t fieldnum);

const upb_ServiceDef* upb_DefPool_FindServiceByName(const upb_DefPool* s,
                                                    const char* name);

const upb_ServiceDef* upb_DefPool_FindServiceByNameWithSize(
    const upb_DefPool* s, const char* name, size_t size);

const upb_FileDef* upb_DefPool_FindFileContainingSymbol(const upb_DefPool* s,
                                                        const char* name);

const upb_FileDef* upb_DefPool_AddFile(upb_DefPool* s,
                                       const UPB_DESC(FileDescriptorProto) *
                                           file_proto,
                                       upb_Status* status);

const upb_ExtensionRegistry* upb_DefPool_ExtensionRegistry(
    const upb_DefPool* s);

const upb_FieldDef** upb_DefPool_GetAllExtensions(const upb_DefPool* s,
                                                  const upb_MessageDef* m,
                                                  size_t* count);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_REFLECTION_DEF_POOL_H_ */

// IWYU pragma: private, include "upb/reflection/def.h"

#ifndef UPB_REFLECTION_ENUM_DEF_H_
#define UPB_REFLECTION_ENUM_DEF_H_


// Must be last.

#ifdef __cplusplus
extern "C" {
#endif

bool upb_EnumDef_CheckNumber(const upb_EnumDef* e, int32_t num);
const upb_MessageDef* upb_EnumDef_ContainingType(const upb_EnumDef* e);
int32_t upb_EnumDef_Default(const upb_EnumDef* e);
const upb_FileDef* upb_EnumDef_File(const upb_EnumDef* e);
const upb_EnumValueDef* upb_EnumDef_FindValueByName(const upb_EnumDef* e,
                                                    const char* name);
const upb_EnumValueDef* upb_EnumDef_FindValueByNameWithSize(
    const upb_EnumDef* e, const char* name, size_t size);
const upb_EnumValueDef* upb_EnumDef_FindValueByNumber(const upb_EnumDef* e,
                                                      int32_t num);
const char* upb_EnumDef_FullName(const upb_EnumDef* e);
bool upb_EnumDef_HasOptions(const upb_EnumDef* e);
bool upb_EnumDef_IsClosed(const upb_EnumDef* e);

// Creates a mini descriptor string for an enum, returns true on success.
bool upb_EnumDef_MiniDescriptorEncode(const upb_EnumDef* e, upb_Arena* a,
                                      upb_StringView* out);

const char* upb_EnumDef_Name(const upb_EnumDef* e);
const UPB_DESC(EnumOptions) * upb_EnumDef_Options(const upb_EnumDef* e);

upb_StringView upb_EnumDef_ReservedName(const upb_EnumDef* e, int i);
int upb_EnumDef_ReservedNameCount(const upb_EnumDef* e);

const upb_EnumReservedRange* upb_EnumDef_ReservedRange(const upb_EnumDef* e,
                                                       int i);
int upb_EnumDef_ReservedRangeCount(const upb_EnumDef* e);

const upb_EnumValueDef* upb_EnumDef_Value(const upb_EnumDef* e, int i);
int upb_EnumDef_ValueCount(const upb_EnumDef* e);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_REFLECTION_ENUM_DEF_H_ */

// IWYU pragma: private, include "upb/reflection/def.h"

#ifndef UPB_REFLECTION_ENUM_VALUE_DEF_H_
#define UPB_REFLECTION_ENUM_VALUE_DEF_H_


// Must be last.

#ifdef __cplusplus
extern "C" {
#endif

const upb_EnumDef* upb_EnumValueDef_Enum(const upb_EnumValueDef* v);
const char* upb_EnumValueDef_FullName(const upb_EnumValueDef* v);
bool upb_EnumValueDef_HasOptions(const upb_EnumValueDef* v);
uint32_t upb_EnumValueDef_Index(const upb_EnumValueDef* v);
const char* upb_EnumValueDef_Name(const upb_EnumValueDef* v);
int32_t upb_EnumValueDef_Number(const upb_EnumValueDef* v);
const UPB_DESC(EnumValueOptions) *
    upb_EnumValueDef_Options(const upb_EnumValueDef* v);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_REFLECTION_ENUM_VALUE_DEF_H_ */

// IWYU pragma: private, include "upb/reflection/def.h"

#ifndef UPB_REFLECTION_EXTENSION_RANGE_H_
#define UPB_REFLECTION_EXTENSION_RANGE_H_


// Must be last.

#ifdef __cplusplus
extern "C" {
#endif

int32_t upb_ExtensionRange_Start(const upb_ExtensionRange* r);
int32_t upb_ExtensionRange_End(const upb_ExtensionRange* r);

bool upb_ExtensionRange_HasOptions(const upb_ExtensionRange* r);
const UPB_DESC(ExtensionRangeOptions) *
    upb_ExtensionRange_Options(const upb_ExtensionRange* r);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_REFLECTION_EXTENSION_RANGE_H_ */

// IWYU pragma: private, include "upb/reflection/def.h"

#ifndef UPB_REFLECTION_FIELD_DEF_H_
#define UPB_REFLECTION_FIELD_DEF_H_


// Must be last.

// Maximum field number allowed for FieldDefs.
// This is an inherent limit of the protobuf wire format.
#define kUpb_MaxFieldNumber ((1 << 29) - 1)

#ifdef __cplusplus
extern "C" {
#endif

const upb_OneofDef* upb_FieldDef_ContainingOneof(const upb_FieldDef* f);
const upb_MessageDef* upb_FieldDef_ContainingType(const upb_FieldDef* f);
upb_CType upb_FieldDef_CType(const upb_FieldDef* f);
upb_MessageValue upb_FieldDef_Default(const upb_FieldDef* f);
const upb_EnumDef* upb_FieldDef_EnumSubDef(const upb_FieldDef* f);
const upb_MessageDef* upb_FieldDef_ExtensionScope(const upb_FieldDef* f);
const upb_FileDef* upb_FieldDef_File(const upb_FieldDef* f);
const char* upb_FieldDef_FullName(const upb_FieldDef* f);
bool upb_FieldDef_HasDefault(const upb_FieldDef* f);
bool upb_FieldDef_HasJsonName(const upb_FieldDef* f);
bool upb_FieldDef_HasOptions(const upb_FieldDef* f);
bool upb_FieldDef_HasPresence(const upb_FieldDef* f);
bool upb_FieldDef_HasSubDef(const upb_FieldDef* f);
uint32_t upb_FieldDef_Index(const upb_FieldDef* f);
bool upb_FieldDef_IsExtension(const upb_FieldDef* f);
bool upb_FieldDef_IsMap(const upb_FieldDef* f);
bool upb_FieldDef_IsOptional(const upb_FieldDef* f);
bool upb_FieldDef_IsPacked(const upb_FieldDef* f);
bool upb_FieldDef_IsPrimitive(const upb_FieldDef* f);
bool upb_FieldDef_IsRepeated(const upb_FieldDef* f);
bool upb_FieldDef_IsRequired(const upb_FieldDef* f);
bool upb_FieldDef_IsString(const upb_FieldDef* f);
bool upb_FieldDef_IsSubMessage(const upb_FieldDef* f);
const char* upb_FieldDef_JsonName(const upb_FieldDef* f);
upb_Label upb_FieldDef_Label(const upb_FieldDef* f);
const upb_MessageDef* upb_FieldDef_MessageSubDef(const upb_FieldDef* f);

// Creates a mini descriptor string for a field, returns true on success.
bool upb_FieldDef_MiniDescriptorEncode(const upb_FieldDef* f, upb_Arena* a,
                                       upb_StringView* out);

const upb_MiniTableField* upb_FieldDef_MiniTable(const upb_FieldDef* f);
const char* upb_FieldDef_Name(const upb_FieldDef* f);
uint32_t upb_FieldDef_Number(const upb_FieldDef* f);
const UPB_DESC(FieldOptions) * upb_FieldDef_Options(const upb_FieldDef* f);
const upb_OneofDef* upb_FieldDef_RealContainingOneof(const upb_FieldDef* f);
upb_FieldType upb_FieldDef_Type(const upb_FieldDef* f);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_REFLECTION_FIELD_DEF_H_ */

// IWYU pragma: private, include "upb/reflection/def.h"

#ifndef UPB_REFLECTION_FILE_DEF_H_
#define UPB_REFLECTION_FILE_DEF_H_


// Must be last.

#ifdef __cplusplus
extern "C" {
#endif

const upb_FileDef* upb_FileDef_Dependency(const upb_FileDef* f, int i);
int upb_FileDef_DependencyCount(const upb_FileDef* f);
bool upb_FileDef_HasOptions(const upb_FileDef* f);
const char* upb_FileDef_Name(const upb_FileDef* f);
const UPB_DESC(FileOptions) * upb_FileDef_Options(const upb_FileDef* f);
const char* upb_FileDef_Package(const upb_FileDef* f);
const char* upb_FileDef_Edition(const upb_FileDef* f);
const upb_DefPool* upb_FileDef_Pool(const upb_FileDef* f);

const upb_FileDef* upb_FileDef_PublicDependency(const upb_FileDef* f, int i);
int upb_FileDef_PublicDependencyCount(const upb_FileDef* f);

const upb_ServiceDef* upb_FileDef_Service(const upb_FileDef* f, int i);
int upb_FileDef_ServiceCount(const upb_FileDef* f);

upb_Syntax upb_FileDef_Syntax(const upb_FileDef* f);

const upb_EnumDef* upb_FileDef_TopLevelEnum(const upb_FileDef* f, int i);
int upb_FileDef_TopLevelEnumCount(const upb_FileDef* f);

const upb_FieldDef* upb_FileDef_TopLevelExtension(const upb_FileDef* f, int i);
int upb_FileDef_TopLevelExtensionCount(const upb_FileDef* f);

const upb_MessageDef* upb_FileDef_TopLevelMessage(const upb_FileDef* f, int i);
int upb_FileDef_TopLevelMessageCount(const upb_FileDef* f);

const upb_FileDef* upb_FileDef_WeakDependency(const upb_FileDef* f, int i);
int upb_FileDef_WeakDependencyCount(const upb_FileDef* f);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_REFLECTION_FILE_DEF_H_ */

// IWYU pragma: private, include "upb/reflection/def.h"

#ifndef UPB_REFLECTION_MESSAGE_DEF_H_
#define UPB_REFLECTION_MESSAGE_DEF_H_


// Must be last.

// Well-known field tag numbers for map-entry messages.
#define kUpb_MapEntry_KeyFieldNumber 1
#define kUpb_MapEntry_ValueFieldNumber 2

// Well-known field tag numbers for Any messages.
#define kUpb_Any_TypeFieldNumber 1
#define kUpb_Any_ValueFieldNumber 2

// Well-known field tag numbers for duration messages.
#define kUpb_Duration_SecondsFieldNumber 1
#define kUpb_Duration_NanosFieldNumber 2

// Well-known field tag numbers for timestamp messages.
#define kUpb_Timestamp_SecondsFieldNumber 1
#define kUpb_Timestamp_NanosFieldNumber 2

// All the different kind of well known type messages. For simplicity of check,
// number wrappers and string wrappers are grouped together. Make sure the
// order and number of these groups are not changed.
typedef enum {
  kUpb_WellKnown_Unspecified,
  kUpb_WellKnown_Any,
  kUpb_WellKnown_FieldMask,
  kUpb_WellKnown_Duration,
  kUpb_WellKnown_Timestamp,

  // number wrappers
  kUpb_WellKnown_DoubleValue,
  kUpb_WellKnown_FloatValue,
  kUpb_WellKnown_Int64Value,
  kUpb_WellKnown_UInt64Value,
  kUpb_WellKnown_Int32Value,
  kUpb_WellKnown_UInt32Value,

  // string wrappers
  kUpb_WellKnown_StringValue,
  kUpb_WellKnown_BytesValue,
  kUpb_WellKnown_BoolValue,
  kUpb_WellKnown_Value,
  kUpb_WellKnown_ListValue,
  kUpb_WellKnown_Struct,
} upb_WellKnown;

#ifdef __cplusplus
extern "C" {
#endif

const upb_MessageDef* upb_MessageDef_ContainingType(const upb_MessageDef* m);

const upb_ExtensionRange* upb_MessageDef_ExtensionRange(const upb_MessageDef* m,
                                                        int i);
int upb_MessageDef_ExtensionRangeCount(const upb_MessageDef* m);

const upb_FieldDef* upb_MessageDef_Field(const upb_MessageDef* m, int i);
int upb_MessageDef_FieldCount(const upb_MessageDef* m);

const upb_FileDef* upb_MessageDef_File(const upb_MessageDef* m);

// Returns a field by either JSON name or regular proto name.
const upb_FieldDef* upb_MessageDef_FindByJsonNameWithSize(
    const upb_MessageDef* m, const char* name, size_t size);
UPB_INLINE const upb_FieldDef* upb_MessageDef_FindByJsonName(
    const upb_MessageDef* m, const char* name) {
  return upb_MessageDef_FindByJsonNameWithSize(m, name, strlen(name));
}

// Lookup of either field or oneof by name. Returns whether either was found.
// If the return is true, then the found def will be set, and the non-found
// one set to NULL.
bool upb_MessageDef_FindByNameWithSize(const upb_MessageDef* m,
                                       const char* name, size_t size,
                                       const upb_FieldDef** f,
                                       const upb_OneofDef** o);
UPB_INLINE bool upb_MessageDef_FindByName(const upb_MessageDef* m,
                                          const char* name,
                                          const upb_FieldDef** f,
                                          const upb_OneofDef** o) {
  return upb_MessageDef_FindByNameWithSize(m, name, strlen(name), f, o);
}

const upb_FieldDef* upb_MessageDef_FindFieldByName(const upb_MessageDef* m,
                                                   const char* name);
const upb_FieldDef* upb_MessageDef_FindFieldByNameWithSize(
    const upb_MessageDef* m, const char* name, size_t size);
const upb_FieldDef* upb_MessageDef_FindFieldByNumber(const upb_MessageDef* m,
                                                     uint32_t i);
const upb_OneofDef* upb_MessageDef_FindOneofByName(const upb_MessageDef* m,
                                                   const char* name);
const upb_OneofDef* upb_MessageDef_FindOneofByNameWithSize(
    const upb_MessageDef* m, const char* name, size_t size);
const char* upb_MessageDef_FullName(const upb_MessageDef* m);
bool upb_MessageDef_HasOptions(const upb_MessageDef* m);
bool upb_MessageDef_IsMapEntry(const upb_MessageDef* m);
bool upb_MessageDef_IsMessageSet(const upb_MessageDef* m);

// Creates a mini descriptor string for a message, returns true on success.
bool upb_MessageDef_MiniDescriptorEncode(const upb_MessageDef* m, upb_Arena* a,
                                         upb_StringView* out);

const upb_MiniTable* upb_MessageDef_MiniTable(const upb_MessageDef* m);
const char* upb_MessageDef_Name(const upb_MessageDef* m);

const upb_EnumDef* upb_MessageDef_NestedEnum(const upb_MessageDef* m, int i);
const upb_FieldDef* upb_MessageDef_NestedExtension(const upb_MessageDef* m,
                                                   int i);
const upb_MessageDef* upb_MessageDef_NestedMessage(const upb_MessageDef* m,
                                                   int i);

int upb_MessageDef_NestedEnumCount(const upb_MessageDef* m);
int upb_MessageDef_NestedExtensionCount(const upb_MessageDef* m);
int upb_MessageDef_NestedMessageCount(const upb_MessageDef* m);

const upb_OneofDef* upb_MessageDef_Oneof(const upb_MessageDef* m, int i);
int upb_MessageDef_OneofCount(const upb_MessageDef* m);
int upb_MessageDef_RealOneofCount(const upb_MessageDef* m);

const UPB_DESC(MessageOptions) *
    upb_MessageDef_Options(const upb_MessageDef* m);

upb_StringView upb_MessageDef_ReservedName(const upb_MessageDef* m, int i);
int upb_MessageDef_ReservedNameCount(const upb_MessageDef* m);

const upb_MessageReservedRange* upb_MessageDef_ReservedRange(
    const upb_MessageDef* m, int i);
int upb_MessageDef_ReservedRangeCount(const upb_MessageDef* m);

upb_Syntax upb_MessageDef_Syntax(const upb_MessageDef* m);
upb_WellKnown upb_MessageDef_WellKnownType(const upb_MessageDef* m);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_REFLECTION_MESSAGE_DEF_H_ */

// IWYU pragma: private, include "upb/reflection/def.h"

#ifndef UPB_REFLECTION_METHOD_DEF_H_
#define UPB_REFLECTION_METHOD_DEF_H_


// Must be last.

#ifdef __cplusplus
extern "C" {
#endif

bool upb_MethodDef_ClientStreaming(const upb_MethodDef* m);
const char* upb_MethodDef_FullName(const upb_MethodDef* m);
bool upb_MethodDef_HasOptions(const upb_MethodDef* m);
int upb_MethodDef_Index(const upb_MethodDef* m);
const upb_MessageDef* upb_MethodDef_InputType(const upb_MethodDef* m);
const char* upb_MethodDef_Name(const upb_MethodDef* m);
const UPB_DESC(MethodOptions) * upb_MethodDef_Options(const upb_MethodDef* m);
const upb_MessageDef* upb_MethodDef_OutputType(const upb_MethodDef* m);
bool upb_MethodDef_ServerStreaming(const upb_MethodDef* m);
const upb_ServiceDef* upb_MethodDef_Service(const upb_MethodDef* m);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_REFLECTION_METHOD_DEF_H_ */

// IWYU pragma: private, include "upb/reflection/def.h"

#ifndef UPB_REFLECTION_ONEOF_DEF_H_
#define UPB_REFLECTION_ONEOF_DEF_H_


// Must be last.

#ifdef __cplusplus
extern "C" {
#endif

const upb_MessageDef* upb_OneofDef_ContainingType(const upb_OneofDef* o);
const upb_FieldDef* upb_OneofDef_Field(const upb_OneofDef* o, int i);
int upb_OneofDef_FieldCount(const upb_OneofDef* o);
const char* upb_OneofDef_FullName(const upb_OneofDef* o);
bool upb_OneofDef_HasOptions(const upb_OneofDef* o);
uint32_t upb_OneofDef_Index(const upb_OneofDef* o);
bool upb_OneofDef_IsSynthetic(const upb_OneofDef* o);
const upb_FieldDef* upb_OneofDef_LookupName(const upb_OneofDef* o,
                                            const char* name);
const upb_FieldDef* upb_OneofDef_LookupNameWithSize(const upb_OneofDef* o,
                                                    const char* name,
                                                    size_t size);
const upb_FieldDef* upb_OneofDef_LookupNumber(const upb_OneofDef* o,
                                              uint32_t num);
const char* upb_OneofDef_Name(const upb_OneofDef* o);
int upb_OneofDef_numfields(const upb_OneofDef* o);
const UPB_DESC(OneofOptions) * upb_OneofDef_Options(const upb_OneofDef* o);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_REFLECTION_ONEOF_DEF_H_ */

// IWYU pragma: private, include "upb/reflection/def.h"

#ifndef UPB_REFLECTION_SERVICE_DEF_H_
#define UPB_REFLECTION_SERVICE_DEF_H_


// Must be last.

#ifdef __cplusplus
extern "C" {
#endif

const upb_FileDef* upb_ServiceDef_File(const upb_ServiceDef* s);
const upb_MethodDef* upb_ServiceDef_FindMethodByName(const upb_ServiceDef* s,
                                                     const char* name);
const char* upb_ServiceDef_FullName(const upb_ServiceDef* s);
bool upb_ServiceDef_HasOptions(const upb_ServiceDef* s);
int upb_ServiceDef_Index(const upb_ServiceDef* s);
const upb_MethodDef* upb_ServiceDef_Method(const upb_ServiceDef* s, int i);
int upb_ServiceDef_MethodCount(const upb_ServiceDef* s);
const char* upb_ServiceDef_Name(const upb_ServiceDef* s);
const UPB_DESC(ServiceOptions) *
    upb_ServiceDef_Options(const upb_ServiceDef* s);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_REFLECTION_SERVICE_DEF_H_ */

#endif /* UPB_REFLECTION_DEF_H_ */
/* This file was generated by upbc (the upb compiler) from the input
 * file:
 *
 *     google/protobuf/descriptor.proto
 *
 * Do not edit -- your changes will be discarded when the file is
 * regenerated. */

#ifndef GOOGLE_PROTOBUF_DESCRIPTOR_PROTO_UPBDEFS_H_
#define GOOGLE_PROTOBUF_DESCRIPTOR_PROTO_UPBDEFS_H_


#ifndef UPB_REFLECTION_DEF_POOL_INTERNAL_H_
#define UPB_REFLECTION_DEF_POOL_INTERNAL_H_


#ifndef UPB_MINI_TABLE_DECODE_H_
#define UPB_MINI_TABLE_DECODE_H_


// Must be last.

typedef enum {
  kUpb_MiniTablePlatform_32Bit,
  kUpb_MiniTablePlatform_64Bit,
  kUpb_MiniTablePlatform_Native =
      UPB_SIZE(kUpb_MiniTablePlatform_32Bit, kUpb_MiniTablePlatform_64Bit),
} upb_MiniTablePlatform;

#ifdef __cplusplus
extern "C" {
#endif

// Builds a mini table from the data encoded in the buffer [data, len]. If any
// errors occur, returns NULL and sets a status message. In the success case,
// the caller must call upb_MiniTable_SetSub*() for all message or proto2 enum
// fields to link the table to the appropriate sub-tables.
upb_MiniTable* _upb_MiniTable_Build(const char* data, size_t len,
                                    upb_MiniTablePlatform platform,
                                    upb_Arena* arena, upb_Status* status);

UPB_API_INLINE upb_MiniTable* upb_MiniTable_Build(const char* data, size_t len,
                                                  upb_Arena* arena,
                                                  upb_Status* status) {
  return _upb_MiniTable_Build(data, len, kUpb_MiniTablePlatform_Native, arena,
                              status);
}

// Links a sub-message field to a MiniTable for that sub-message. If a
// sub-message field is not linked, it will be treated as an unknown field
// during parsing, and setting the field will not be allowed. It is possible
// to link the message field later, at which point it will no longer be treated
// as unknown. However there is no synchronization for this operation, which
// means parallel mutation requires external synchronization.
// Returns success/failure.
UPB_API bool upb_MiniTable_SetSubMessage(upb_MiniTable* table,
                                         upb_MiniTableField* field,
                                         const upb_MiniTable* sub);

// Links an enum field to a MiniTable for that enum.
// All enum fields must be linked prior to parsing.
// Returns success/failure.
UPB_API bool upb_MiniTable_SetSubEnum(upb_MiniTable* table,
                                      upb_MiniTableField* field,
                                      const upb_MiniTableEnum* sub);

// Initializes a MiniTableExtension buffer that has already been allocated.
// This is needed by upb_FileDef and upb_MessageDef, which allocate all of the
// extensions together in a single contiguous array.
const char* _upb_MiniTableExtension_Init(const char* data, size_t len,
                                         upb_MiniTableExtension* ext,
                                         const upb_MiniTable* extendee,
                                         upb_MiniTableSub sub,
                                         upb_MiniTablePlatform platform,
                                         upb_Status* status);

UPB_API_INLINE const char* upb_MiniTableExtension_Init(
    const char* data, size_t len, upb_MiniTableExtension* ext,
    const upb_MiniTable* extendee, upb_MiniTableSub sub, upb_Status* status) {
  return _upb_MiniTableExtension_Init(data, len, ext, extendee, sub,
                                      kUpb_MiniTablePlatform_Native, status);
}

UPB_API upb_MiniTableExtension* _upb_MiniTableExtension_Build(
    const char* data, size_t len, const upb_MiniTable* extendee,
    upb_MiniTableSub sub, upb_MiniTablePlatform platform, upb_Arena* arena,
    upb_Status* status);

UPB_API_INLINE upb_MiniTableExtension* upb_MiniTableExtension_Build(
    const char* data, size_t len, const upb_MiniTable* extendee,
    upb_Arena* arena, upb_Status* status) {
  upb_MiniTableSub sub;
  sub.submsg = NULL;
  return _upb_MiniTableExtension_Build(
      data, len, extendee, sub, kUpb_MiniTablePlatform_Native, arena, status);
}

UPB_API_INLINE upb_MiniTableExtension* upb_MiniTableExtension_BuildMessage(
    const char* data, size_t len, const upb_MiniTable* extendee,
    upb_MiniTable* submsg, upb_Arena* arena, upb_Status* status) {
  upb_MiniTableSub sub;
  sub.submsg = submsg;
  return _upb_MiniTableExtension_Build(
      data, len, extendee, sub, kUpb_MiniTablePlatform_Native, arena, status);
}

UPB_API_INLINE upb_MiniTableExtension* upb_MiniTableExtension_BuildEnum(
    const char* data, size_t len, const upb_MiniTable* extendee,
    upb_MiniTableEnum* subenum, upb_Arena* arena, upb_Status* status) {
  upb_MiniTableSub sub;
  sub.subenum = subenum;
  return _upb_MiniTableExtension_Build(
      data, len, extendee, sub, kUpb_MiniTablePlatform_Native, arena, status);
}

UPB_API upb_MiniTableEnum* upb_MiniTableEnum_Build(const char* data, size_t len,
                                                   upb_Arena* arena,
                                                   upb_Status* status);

// Like upb_MiniTable_Build(), but the user provides a buffer of layout data so
// it can be reused from call to call, avoiding repeated realloc()/free().
//
// The caller owns `*buf` both before and after the call, and must free() it
// when it is no longer in use.  The function will realloc() `*buf` as
// necessary, updating `*size` accordingly.
upb_MiniTable* upb_MiniTable_BuildWithBuf(const char* data, size_t len,
                                          upb_MiniTablePlatform platform,
                                          upb_Arena* arena, void** buf,
                                          size_t* buf_size, upb_Status* status);

// Returns a list of fields that require linking at runtime, to connect the
// MiniTable to its sub-messages and sub-enums.  The list of fields will be
// written to the `subs` array, which must have been allocated by the caller
// and must be large enough to hold a list of all fields in the message.
//
// The order of the fields returned by this function is significant: it matches
// the order expected by upb_MiniTable_Link() below.
//
// The return value packs the sub-message count and sub-enum count into a single
// integer like so:
//  return (msg_count << 16) | enum_count;
UPB_API uint32_t upb_MiniTable_GetSubList(const upb_MiniTable* mt,
                                          const upb_MiniTableField** subs);

// Links a message to its sub-messages and sub-enums.  The caller must pass
// arrays of sub-tables and sub-enums, in the same length and order as is
// returned by upb_MiniTable_GetSubList() above.  However, individual elements
// of the sub_tables may be NULL if those sub-messages were tree shaken.
//
// Returns false if either array is too short, or if any of the tables fails
// to link.
UPB_API bool upb_MiniTable_Link(upb_MiniTable* mt,
                                const upb_MiniTable** sub_tables,
                                size_t sub_table_count,
                                const upb_MiniTableEnum** sub_enums,
                                size_t sub_enum_count);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_MINI_TABLE_DECODE_H_ */

// Must be last.

#ifdef __cplusplus
extern "C" {
#endif

upb_Arena* _upb_DefPool_Arena(const upb_DefPool* s);
size_t _upb_DefPool_BytesLoaded(const upb_DefPool* s);
upb_ExtensionRegistry* _upb_DefPool_ExtReg(const upb_DefPool* s);

bool _upb_DefPool_InsertExt(upb_DefPool* s, const upb_MiniTableExtension* ext,
                            const upb_FieldDef* f);
bool _upb_DefPool_InsertSym(upb_DefPool* s, upb_StringView sym, upb_value v,
                            upb_Status* status);
bool _upb_DefPool_LookupSym(const upb_DefPool* s, const char* sym, size_t size,
                            upb_value* v);

void** _upb_DefPool_ScratchData(const upb_DefPool* s);
size_t* _upb_DefPool_ScratchSize(const upb_DefPool* s);
void _upb_DefPool_SetPlatform(upb_DefPool* s, upb_MiniTablePlatform platform);

// For generated code only: loads a generated descriptor.
typedef struct _upb_DefPool_Init {
  struct _upb_DefPool_Init** deps;  // Dependencies of this file.
  const upb_MiniTableFile* layout;
  const char* filename;
  upb_StringView descriptor;  // Serialized descriptor.
} _upb_DefPool_Init;

bool _upb_DefPool_LoadDefInit(upb_DefPool* s, const _upb_DefPool_Init* init);

// Should only be directly called by tests. This variant lets us suppress
// the use of compiled-in tables, forcing a rebuild of the tables at runtime.
bool _upb_DefPool_LoadDefInitEx(upb_DefPool* s, const _upb_DefPool_Init* init,
                                bool rebuild_minitable);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_REFLECTION_DEF_POOL_INTERNAL_H_ */
#ifdef __cplusplus
extern "C" {
#endif



extern _upb_DefPool_Init google_protobuf_descriptor_proto_upbdefinit;

UPB_INLINE const upb_MessageDef *google_protobuf_FileDescriptorSet_getmsgdef(upb_DefPool *s) {
  _upb_DefPool_LoadDefInit(s, &google_protobuf_descriptor_proto_upbdefinit);
  return upb_DefPool_FindMessageByName(s, "google.protobuf.FileDescriptorSet");
}

UPB_INLINE const upb_MessageDef *google_protobuf_FileDescriptorProto_getmsgdef(upb_DefPool *s) {
  _upb_DefPool_LoadDefInit(s, &google_protobuf_descriptor_proto_upbdefinit);
  return upb_DefPool_FindMessageByName(s, "google.protobuf.FileDescriptorProto");
}

UPB_INLINE const upb_MessageDef *google_protobuf_DescriptorProto_getmsgdef(upb_DefPool *s) {
  _upb_DefPool_LoadDefInit(s, &google_protobuf_descriptor_proto_upbdefinit);
  return upb_DefPool_FindMessageByName(s, "google.protobuf.DescriptorProto");
}

UPB_INLINE const upb_MessageDef *google_protobuf_DescriptorProto_ExtensionRange_getmsgdef(upb_DefPool *s) {
  _upb_DefPool_LoadDefInit(s, &google_protobuf_descriptor_proto_upbdefinit);
  return upb_DefPool_FindMessageByName(s, "google.protobuf.DescriptorProto.ExtensionRange");
}

UPB_INLINE const upb_MessageDef *google_protobuf_DescriptorProto_ReservedRange_getmsgdef(upb_DefPool *s) {
  _upb_DefPool_LoadDefInit(s, &google_protobuf_descriptor_proto_upbdefinit);
  return upb_DefPool_FindMessageByName(s, "google.protobuf.DescriptorProto.ReservedRange");
}

UPB_INLINE const upb_MessageDef *google_protobuf_ExtensionRangeOptions_getmsgdef(upb_DefPool *s) {
  _upb_DefPool_LoadDefInit(s, &google_protobuf_descriptor_proto_upbdefinit);
  return upb_DefPool_FindMessageByName(s, "google.protobuf.ExtensionRangeOptions");
}

UPB_INLINE const upb_MessageDef *google_protobuf_ExtensionRangeOptions_Declaration_getmsgdef(upb_DefPool *s) {
  _upb_DefPool_LoadDefInit(s, &google_protobuf_descriptor_proto_upbdefinit);
  return upb_DefPool_FindMessageByName(s, "google.protobuf.ExtensionRangeOptions.Declaration");
}

UPB_INLINE const upb_MessageDef *google_protobuf_FieldDescriptorProto_getmsgdef(upb_DefPool *s) {
  _upb_DefPool_LoadDefInit(s, &google_protobuf_descriptor_proto_upbdefinit);
  return upb_DefPool_FindMessageByName(s, "google.protobuf.FieldDescriptorProto");
}

UPB_INLINE const upb_MessageDef *google_protobuf_OneofDescriptorProto_getmsgdef(upb_DefPool *s) {
  _upb_DefPool_LoadDefInit(s, &google_protobuf_descriptor_proto_upbdefinit);
  return upb_DefPool_FindMessageByName(s, "google.protobuf.OneofDescriptorProto");
}

UPB_INLINE const upb_MessageDef *google_protobuf_EnumDescriptorProto_getmsgdef(upb_DefPool *s) {
  _upb_DefPool_LoadDefInit(s, &google_protobuf_descriptor_proto_upbdefinit);
  return upb_DefPool_FindMessageByName(s, "google.protobuf.EnumDescriptorProto");
}

UPB_INLINE const upb_MessageDef *google_protobuf_EnumDescriptorProto_EnumReservedRange_getmsgdef(upb_DefPool *s) {
  _upb_DefPool_LoadDefInit(s, &google_protobuf_descriptor_proto_upbdefinit);
  return upb_DefPool_FindMessageByName(s, "google.protobuf.EnumDescriptorProto.EnumReservedRange");
}

UPB_INLINE const upb_MessageDef *google_protobuf_EnumValueDescriptorProto_getmsgdef(upb_DefPool *s) {
  _upb_DefPool_LoadDefInit(s, &google_protobuf_descriptor_proto_upbdefinit);
  return upb_DefPool_FindMessageByName(s, "google.protobuf.EnumValueDescriptorProto");
}

UPB_INLINE const upb_MessageDef *google_protobuf_ServiceDescriptorProto_getmsgdef(upb_DefPool *s) {
  _upb_DefPool_LoadDefInit(s, &google_protobuf_descriptor_proto_upbdefinit);
  return upb_DefPool_FindMessageByName(s, "google.protobuf.ServiceDescriptorProto");
}

UPB_INLINE const upb_MessageDef *google_protobuf_MethodDescriptorProto_getmsgdef(upb_DefPool *s) {
  _upb_DefPool_LoadDefInit(s, &google_protobuf_descriptor_proto_upbdefinit);
  return upb_DefPool_FindMessageByName(s, "google.protobuf.MethodDescriptorProto");
}

UPB_INLINE const upb_MessageDef *google_protobuf_FileOptions_getmsgdef(upb_DefPool *s) {
  _upb_DefPool_LoadDefInit(s, &google_protobuf_descriptor_proto_upbdefinit);
  return upb_DefPool_FindMessageByName(s, "google.protobuf.FileOptions");
}

UPB_INLINE const upb_MessageDef *google_protobuf_MessageOptions_getmsgdef(upb_DefPool *s) {
  _upb_DefPool_LoadDefInit(s, &google_protobuf_descriptor_proto_upbdefinit);
  return upb_DefPool_FindMessageByName(s, "google.protobuf.MessageOptions");
}

UPB_INLINE const upb_MessageDef *google_protobuf_FieldOptions_getmsgdef(upb_DefPool *s) {
  _upb_DefPool_LoadDefInit(s, &google_protobuf_descriptor_proto_upbdefinit);
  return upb_DefPool_FindMessageByName(s, "google.protobuf.FieldOptions");
}

UPB_INLINE const upb_MessageDef *google_protobuf_OneofOptions_getmsgdef(upb_DefPool *s) {
  _upb_DefPool_LoadDefInit(s, &google_protobuf_descriptor_proto_upbdefinit);
  return upb_DefPool_FindMessageByName(s, "google.protobuf.OneofOptions");
}

UPB_INLINE const upb_MessageDef *google_protobuf_EnumOptions_getmsgdef(upb_DefPool *s) {
  _upb_DefPool_LoadDefInit(s, &google_protobuf_descriptor_proto_upbdefinit);
  return upb_DefPool_FindMessageByName(s, "google.protobuf.EnumOptions");
}

UPB_INLINE const upb_MessageDef *google_protobuf_EnumValueOptions_getmsgdef(upb_DefPool *s) {
  _upb_DefPool_LoadDefInit(s, &google_protobuf_descriptor_proto_upbdefinit);
  return upb_DefPool_FindMessageByName(s, "google.protobuf.EnumValueOptions");
}

UPB_INLINE const upb_MessageDef *google_protobuf_ServiceOptions_getmsgdef(upb_DefPool *s) {
  _upb_DefPool_LoadDefInit(s, &google_protobuf_descriptor_proto_upbdefinit);
  return upb_DefPool_FindMessageByName(s, "google.protobuf.ServiceOptions");
}

UPB_INLINE const upb_MessageDef *google_protobuf_MethodOptions_getmsgdef(upb_DefPool *s) {
  _upb_DefPool_LoadDefInit(s, &google_protobuf_descriptor_proto_upbdefinit);
  return upb_DefPool_FindMessageByName(s, "google.protobuf.MethodOptions");
}

UPB_INLINE const upb_MessageDef *google_protobuf_UninterpretedOption_getmsgdef(upb_DefPool *s) {
  _upb_DefPool_LoadDefInit(s, &google_protobuf_descriptor_proto_upbdefinit);
  return upb_DefPool_FindMessageByName(s, "google.protobuf.UninterpretedOption");
}

UPB_INLINE const upb_MessageDef *google_protobuf_UninterpretedOption_NamePart_getmsgdef(upb_DefPool *s) {
  _upb_DefPool_LoadDefInit(s, &google_protobuf_descriptor_proto_upbdefinit);
  return upb_DefPool_FindMessageByName(s, "google.protobuf.UninterpretedOption.NamePart");
}

UPB_INLINE const upb_MessageDef *google_protobuf_SourceCodeInfo_getmsgdef(upb_DefPool *s) {
  _upb_DefPool_LoadDefInit(s, &google_protobuf_descriptor_proto_upbdefinit);
  return upb_DefPool_FindMessageByName(s, "google.protobuf.SourceCodeInfo");
}

UPB_INLINE const upb_MessageDef *google_protobuf_SourceCodeInfo_Location_getmsgdef(upb_DefPool *s) {
  _upb_DefPool_LoadDefInit(s, &google_protobuf_descriptor_proto_upbdefinit);
  return upb_DefPool_FindMessageByName(s, "google.protobuf.SourceCodeInfo.Location");
}

UPB_INLINE const upb_MessageDef *google_protobuf_GeneratedCodeInfo_getmsgdef(upb_DefPool *s) {
  _upb_DefPool_LoadDefInit(s, &google_protobuf_descriptor_proto_upbdefinit);
  return upb_DefPool_FindMessageByName(s, "google.protobuf.GeneratedCodeInfo");
}

UPB_INLINE const upb_MessageDef *google_protobuf_GeneratedCodeInfo_Annotation_getmsgdef(upb_DefPool *s) {
  _upb_DefPool_LoadDefInit(s, &google_protobuf_descriptor_proto_upbdefinit);
  return upb_DefPool_FindMessageByName(s, "google.protobuf.GeneratedCodeInfo.Annotation");
}

#ifdef __cplusplus
}  /* extern "C" */
#endif


#endif  /* GOOGLE_PROTOBUF_DESCRIPTOR_PROTO_UPBDEFS_H_ */

#ifndef UPB_WIRE_EPS_COPY_INPUT_STREAM_H_
#define UPB_WIRE_EPS_COPY_INPUT_STREAM_H_

#include <string.h>


// Must be last.

#ifdef __cplusplus
extern "C" {
#endif

// The maximum number of bytes a single protobuf field can take up in the
// wire format.  We only want to do one bounds check per field, so the input
// stream guarantees that after upb_EpsCopyInputStream_IsDone() is called,
// the decoder can read this many bytes without performing another bounds
// check.  The stream will copy into a patch buffer as necessary to guarantee
// this invariant.
#define kUpb_EpsCopyInputStream_SlopBytes 16

enum {
  kUpb_EpsCopyInputStream_NoAliasing = 0,
  kUpb_EpsCopyInputStream_OnPatch = 1,
  kUpb_EpsCopyInputStream_NoDelta = 2
};

typedef struct {
  const char* end;        // Can read up to SlopBytes bytes beyond this.
  const char* limit_ptr;  // For bounds checks, = end + UPB_MIN(limit, 0)
  uintptr_t aliasing;
  int limit;              // Submessage limit relative to end
  bool error;             // To distinguish between EOF and error.
  char patch[kUpb_EpsCopyInputStream_SlopBytes * 2];
} upb_EpsCopyInputStream;

// Returns true if the stream is in the error state. A stream enters the error
// state when the user reads past a limit (caught in IsDone()) or the
// ZeroCopyInputStream returns an error.
UPB_INLINE bool upb_EpsCopyInputStream_IsError(upb_EpsCopyInputStream* e) {
  return e->error;
}

typedef const char* upb_EpsCopyInputStream_BufferFlipCallback(
    upb_EpsCopyInputStream* e, const char* old_end, const char* new_start);

typedef const char* upb_EpsCopyInputStream_IsDoneFallbackFunc(
    upb_EpsCopyInputStream* e, const char* ptr, int overrun);

// Initializes a upb_EpsCopyInputStream using the contents of the buffer
// [*ptr, size].  Updates `*ptr` as necessary to guarantee that at least
// kUpb_EpsCopyInputStream_SlopBytes are available to read.
UPB_INLINE void upb_EpsCopyInputStream_Init(upb_EpsCopyInputStream* e,
                                            const char** ptr, size_t size,
                                            bool enable_aliasing) {
  if (size <= kUpb_EpsCopyInputStream_SlopBytes) {
    memset(&e->patch, 0, 32);
    if (size) memcpy(&e->patch, *ptr, size);
    e->aliasing = enable_aliasing ? (uintptr_t)*ptr - (uintptr_t)e->patch
                                  : kUpb_EpsCopyInputStream_NoAliasing;
    *ptr = e->patch;
    e->end = *ptr + size;
    e->limit = 0;
  } else {
    e->end = *ptr + size - kUpb_EpsCopyInputStream_SlopBytes;
    e->limit = kUpb_EpsCopyInputStream_SlopBytes;
    e->aliasing = enable_aliasing ? kUpb_EpsCopyInputStream_NoDelta
                                  : kUpb_EpsCopyInputStream_NoAliasing;
  }
  e->limit_ptr = e->end;
  e->error = false;
}

typedef enum {
  // The current stream position is at a limit.
  kUpb_IsDoneStatus_Done,

  // The current stream position is not at a limit.
  kUpb_IsDoneStatus_NotDone,

  // The current stream position is not at a limit, and the stream needs to
  // be flipped to a new buffer before more data can be read.
  kUpb_IsDoneStatus_NeedFallback,
} upb_IsDoneStatus;

// Returns the status of the current stream position.  This is a low-level
// function, it is simpler to call upb_EpsCopyInputStream_IsDone() if possible.
UPB_INLINE upb_IsDoneStatus upb_EpsCopyInputStream_IsDoneStatus(
    upb_EpsCopyInputStream* e, const char* ptr, int* overrun) {
  *overrun = ptr - e->end;
  if (UPB_LIKELY(ptr < e->limit_ptr)) {
    return kUpb_IsDoneStatus_NotDone;
  } else if (UPB_LIKELY(*overrun == e->limit)) {
    return kUpb_IsDoneStatus_Done;
  } else {
    return kUpb_IsDoneStatus_NeedFallback;
  }
}

// Returns true if the stream has hit a limit, either the current delimited
// limit or the overall end-of-stream. As a side effect, this function may flip
// the pointer to a new buffer if there are less than
// kUpb_EpsCopyInputStream_SlopBytes of data to be read in the current buffer.
//
// Postcondition: if the function returns false, there are at least
// kUpb_EpsCopyInputStream_SlopBytes of data available to read at *ptr.
UPB_INLINE bool upb_EpsCopyInputStream_IsDoneWithCallback(
    upb_EpsCopyInputStream* e, const char** ptr,
    upb_EpsCopyInputStream_IsDoneFallbackFunc* func) {
  int overrun;
  switch (upb_EpsCopyInputStream_IsDoneStatus(e, *ptr, &overrun)) {
    case kUpb_IsDoneStatus_Done:
      return true;
    case kUpb_IsDoneStatus_NotDone:
      return false;
    case kUpb_IsDoneStatus_NeedFallback:
      *ptr = func(e, *ptr, overrun);
      return *ptr == NULL;
  }
  UPB_UNREACHABLE();
}

const char* _upb_EpsCopyInputStream_IsDoneFallbackNoCallback(
    upb_EpsCopyInputStream* e, const char* ptr, int overrun);

// A simpler version of IsDoneWithCallback() that does not support a buffer flip
// callback. Useful in cases where we do not need to insert custom logic at
// every buffer flip.
//
// If this returns true, the user must call upb_EpsCopyInputStream_IsError()
// to distinguish between EOF and error.
UPB_INLINE bool upb_EpsCopyInputStream_IsDone(upb_EpsCopyInputStream* e,
                                              const char** ptr) {
  return upb_EpsCopyInputStream_IsDoneWithCallback(
      e, ptr, _upb_EpsCopyInputStream_IsDoneFallbackNoCallback);
}

// Returns the total number of bytes that are safe to read from the current
// buffer without reading uninitialized or unallocated memory.
//
// Note that this check does not respect any semantic limits on the stream,
// either limits from PushLimit() or the overall stream end, so some of these
// bytes may have unpredictable, nonsense values in them. The guarantee is only
// that the bytes are valid to read from the perspective of the C language
// (ie. you can read without triggering UBSAN or ASAN).
UPB_INLINE size_t upb_EpsCopyInputStream_BytesAvailable(
    upb_EpsCopyInputStream* e, const char* ptr) {
  return (e->end - ptr) + kUpb_EpsCopyInputStream_SlopBytes;
}

// Returns true if the given delimited field size is valid (it does not extend
// beyond any previously-pushed limits).  `ptr` should point to the beginning
// of the field data, after the delimited size.
//
// Note that this does *not* guarantee that all of the data for this field is in
// the current buffer.
UPB_INLINE bool upb_EpsCopyInputStream_CheckSize(
    const upb_EpsCopyInputStream* e, const char* ptr, int size) {
  UPB_ASSERT(size >= 0);
  return ptr - e->end + size <= e->limit;
}

UPB_INLINE bool _upb_EpsCopyInputStream_CheckSizeAvailable(
    upb_EpsCopyInputStream* e, const char* ptr, int size, bool submessage) {
  // This is one extra branch compared to the more normal:
  //   return (size_t)(end - ptr) < size;
  // However it is one less computation if we are just about to use "ptr + len":
  //   https://godbolt.org/z/35YGPz
  // In microbenchmarks this shows a small improvement.
  uintptr_t uptr = (uintptr_t)ptr;
  uintptr_t uend = (uintptr_t)e->limit_ptr;
  uintptr_t res = uptr + (size_t)size;
  if (!submessage) uend += kUpb_EpsCopyInputStream_SlopBytes;
  // NOTE: this check depends on having a linear address space.  This is not
  // technically guaranteed by uintptr_t.
  bool ret = res >= uptr && res <= uend;
  if (size < 0) UPB_ASSERT(!ret);
  return ret;
}

// Returns true if the given delimited field size is valid (it does not extend
// beyond any previously-pushed limited) *and* all of the data for this field is
// available to be read in the current buffer.
//
// If the size is negative, this function will always return false. This
// property can be useful in some cases.
UPB_INLINE bool upb_EpsCopyInputStream_CheckDataSizeAvailable(
    upb_EpsCopyInputStream* e, const char* ptr, int size) {
  return _upb_EpsCopyInputStream_CheckSizeAvailable(e, ptr, size, false);
}

// Returns true if the given sub-message size is valid (it does not extend
// beyond any previously-pushed limited) *and* all of the data for this
// sub-message is available to be parsed in the current buffer.
//
// This implies that all fields from the sub-message can be parsed from the
// current buffer while maintaining the invariant that we always have at least
// kUpb_EpsCopyInputStream_SlopBytes of data available past the beginning of
// any individual field start.
//
// If the size is negative, this function will always return false. This
// property can be useful in some cases.
UPB_INLINE bool upb_EpsCopyInputStream_CheckSubMessageSizeAvailable(
    upb_EpsCopyInputStream* e, const char* ptr, int size) {
  return _upb_EpsCopyInputStream_CheckSizeAvailable(e, ptr, size, true);
}

// Returns true if aliasing_enabled=true was passed to
// upb_EpsCopyInputStream_Init() when this stream was initialized.
UPB_INLINE bool upb_EpsCopyInputStream_AliasingEnabled(
    upb_EpsCopyInputStream* e) {
  return e->aliasing != kUpb_EpsCopyInputStream_NoAliasing;
}

// Returns true if aliasing_enabled=true was passed to
// upb_EpsCopyInputStream_Init() when this stream was initialized *and* we can
// alias into the region [ptr, size] in an input buffer.
UPB_INLINE bool upb_EpsCopyInputStream_AliasingAvailable(
    upb_EpsCopyInputStream* e, const char* ptr, size_t size) {
  // When EpsCopyInputStream supports streaming, this will need to become a
  // runtime check.
  return upb_EpsCopyInputStream_CheckDataSizeAvailable(e, ptr, size) &&
         e->aliasing >= kUpb_EpsCopyInputStream_NoDelta;
}

// Returns a pointer into an input buffer that corresponds to the parsing
// pointer `ptr`.  The returned pointer may be the same as `ptr`, but also may
// be different if we are currently parsing out of the patch buffer.
//
// REQUIRES: Aliasing must be available for the given pointer. If the input is a
// flat buffer and aliasing is enabled, then aliasing will always be available.
UPB_INLINE const char* upb_EpsCopyInputStream_GetAliasedPtr(
    upb_EpsCopyInputStream* e, const char* ptr) {
  UPB_ASSUME(upb_EpsCopyInputStream_AliasingAvailable(e, ptr, 0));
  uintptr_t delta =
      e->aliasing == kUpb_EpsCopyInputStream_NoDelta ? 0 : e->aliasing;
  return (const char*)((uintptr_t)ptr + delta);
}

// Reads string data from the input, aliasing into the input buffer instead of
// copying. The parsing pointer is passed in `*ptr`, and will be updated if
// necessary to point to the actual input buffer. Returns the new parsing
// pointer, which will be advanced past the string data.
//
// REQUIRES: Aliasing must be available for this data region (test with
// upb_EpsCopyInputStream_AliasingAvailable().
UPB_INLINE const char* upb_EpsCopyInputStream_ReadStringAliased(
    upb_EpsCopyInputStream* e, const char** ptr, size_t size) {
  UPB_ASSUME(upb_EpsCopyInputStream_AliasingAvailable(e, *ptr, size));
  const char* ret = *ptr + size;
  *ptr = upb_EpsCopyInputStream_GetAliasedPtr(e, *ptr);
  UPB_ASSUME(ret != NULL);
  return ret;
}

// Skips `size` bytes of data from the input and returns a pointer past the end.
// Returns NULL on end of stream or error.
UPB_INLINE const char* upb_EpsCopyInputStream_Skip(upb_EpsCopyInputStream* e,
                                                   const char* ptr, int size) {
  if (!upb_EpsCopyInputStream_CheckDataSizeAvailable(e, ptr, size)) return NULL;
  return ptr + size;
}

// Copies `size` bytes of data from the input `ptr` into the buffer `to`, and
// returns a pointer past the end. Returns NULL on end of stream or error.
UPB_INLINE const char* upb_EpsCopyInputStream_Copy(upb_EpsCopyInputStream* e,
                                                   const char* ptr, void* to,
                                                   int size) {
  if (!upb_EpsCopyInputStream_CheckDataSizeAvailable(e, ptr, size)) return NULL;
  memcpy(to, ptr, size);
  return ptr + size;
}

// Reads string data from the stream and advances the pointer accordingly.
// If aliasing was enabled when the stream was initialized, then the returned
// pointer will point into the input buffer if possible, otherwise new data
// will be allocated from arena and copied into. We may be forced to copy even
// if aliasing was enabled if the input data spans input buffers.
//
// Returns NULL if memory allocation failed, or we reached a premature EOF.
UPB_INLINE const char* upb_EpsCopyInputStream_ReadString(
    upb_EpsCopyInputStream* e, const char** ptr, size_t size,
    upb_Arena* arena) {
  if (upb_EpsCopyInputStream_AliasingAvailable(e, *ptr, size)) {
    return upb_EpsCopyInputStream_ReadStringAliased(e, ptr, size);
  } else {
    // We need to allocate and copy.
    if (!upb_EpsCopyInputStream_CheckDataSizeAvailable(e, *ptr, size)) {
      return NULL;
    }
    UPB_ASSERT(arena);
    char* data = (char*)upb_Arena_Malloc(arena, size);
    if (!data) return NULL;
    const char* ret = upb_EpsCopyInputStream_Copy(e, *ptr, data, size);
    *ptr = data;
    return ret;
  }
}

UPB_INLINE void _upb_EpsCopyInputStream_CheckLimit(upb_EpsCopyInputStream* e) {
  UPB_ASSERT(e->limit_ptr == e->end + UPB_MIN(0, e->limit));
}

// Pushes a limit onto the stack of limits for the current stream.  The limit
// will extend for `size` bytes beyond the position in `ptr`.  Future calls to
// upb_EpsCopyInputStream_IsDone() will return `true` when the stream position
// reaches this limit.
//
// Returns a delta that the caller must store and supply to PopLimit() below.
UPB_INLINE int upb_EpsCopyInputStream_PushLimit(upb_EpsCopyInputStream* e,
                                                const char* ptr, int size) {
  int limit = size + (int)(ptr - e->end);
  int delta = e->limit - limit;
  _upb_EpsCopyInputStream_CheckLimit(e);
  UPB_ASSERT(limit <= e->limit);
  e->limit = limit;
  e->limit_ptr = e->end + UPB_MIN(0, limit);
  _upb_EpsCopyInputStream_CheckLimit(e);
  return delta;
}

// Pops the last limit that was pushed on this stream.  This may only be called
// once IsDone() returns true.  The user must pass the delta that was returned
// from PushLimit().
UPB_INLINE void upb_EpsCopyInputStream_PopLimit(upb_EpsCopyInputStream* e,
                                                const char* ptr,
                                                int saved_delta) {
  UPB_ASSERT(ptr - e->end == e->limit);
  _upb_EpsCopyInputStream_CheckLimit(e);
  e->limit += saved_delta;
  e->limit_ptr = e->end + UPB_MIN(0, e->limit);
  _upb_EpsCopyInputStream_CheckLimit(e);
}

UPB_INLINE const char* _upb_EpsCopyInputStream_IsDoneFallbackInline(
    upb_EpsCopyInputStream* e, const char* ptr, int overrun,
    upb_EpsCopyInputStream_BufferFlipCallback* callback) {
  if (overrun < e->limit) {
    // Need to copy remaining data into patch buffer.
    UPB_ASSERT(overrun < kUpb_EpsCopyInputStream_SlopBytes);
    const char* old_end = ptr;
    const char* new_start = &e->patch[0] + overrun;
    memset(e->patch + kUpb_EpsCopyInputStream_SlopBytes, 0,
           kUpb_EpsCopyInputStream_SlopBytes);
    memcpy(e->patch, e->end, kUpb_EpsCopyInputStream_SlopBytes);
    ptr = new_start;
    e->end = &e->patch[kUpb_EpsCopyInputStream_SlopBytes];
    e->limit -= kUpb_EpsCopyInputStream_SlopBytes;
    e->limit_ptr = e->end + e->limit;
    UPB_ASSERT(ptr < e->limit_ptr);
    if (e->aliasing != kUpb_EpsCopyInputStream_NoAliasing) {
      e->aliasing = (uintptr_t)old_end - (uintptr_t)new_start;
    }
    return callback(e, old_end, new_start);
  } else {
    UPB_ASSERT(overrun > e->limit);
    e->error = true;
    return callback(e, NULL, NULL);
  }
}

typedef const char* upb_EpsCopyInputStream_ParseDelimitedFunc(
    upb_EpsCopyInputStream* e, const char* ptr, void* ctx);

// Tries to perform a fast-path handling of the given delimited message data.
// If the sub-message beginning at `*ptr` and extending for `len` is short and
// fits within this buffer, calls `func` with `ctx` as a parameter, where the
// pushing and popping of limits is handled automatically and with lower cost
// than the normal PushLimit()/PopLimit() sequence.
static UPB_FORCEINLINE bool upb_EpsCopyInputStream_TryParseDelimitedFast(
    upb_EpsCopyInputStream* e, const char** ptr, int len,
    upb_EpsCopyInputStream_ParseDelimitedFunc* func, void* ctx) {
  if (!upb_EpsCopyInputStream_CheckSubMessageSizeAvailable(e, *ptr, len)) {
    return false;
  }

  // Fast case: Sub-message is <128 bytes and fits in the current buffer.
  // This means we can preserve limit/limit_ptr verbatim.
  const char* saved_limit_ptr = e->limit_ptr;
  int saved_limit = e->limit;
  e->limit_ptr = *ptr + len;
  e->limit = e->limit_ptr - e->end;
  UPB_ASSERT(e->limit_ptr == e->end + UPB_MIN(0, e->limit));
  *ptr = func(e, *ptr, ctx);
  e->limit_ptr = saved_limit_ptr;
  e->limit = saved_limit;
  UPB_ASSERT(e->limit_ptr == e->end + UPB_MIN(0, e->limit));
  return true;
}

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif  // UPB_WIRE_EPS_COPY_INPUT_STREAM_H_

#ifndef UPB_HASH_INT_TABLE_H_
#define UPB_HASH_INT_TABLE_H_


// Must be last.

typedef struct {
  upb_table t;              // For entries that don't fit in the array part.
  const upb_tabval* array;  // Array part of the table. See const note above.
  size_t array_size;        // Array part size.
  size_t array_count;       // Array part number of elements.
} upb_inttable;

#ifdef __cplusplus
extern "C" {
#endif

// Initialize a table. If memory allocation failed, false is returned and
// the table is uninitialized.
bool upb_inttable_init(upb_inttable* table, upb_Arena* a);

// Returns the number of values in the table.
size_t upb_inttable_count(const upb_inttable* t);

// Inserts the given key into the hashtable with the given value.
// The key must not already exist in the hash table.
// The value must not be UINTPTR_MAX.
//
// If a table resize was required but memory allocation failed, false is
// returned and the table is unchanged.
bool upb_inttable_insert(upb_inttable* t, uintptr_t key, upb_value val,
                         upb_Arena* a);

// Looks up key in this table, returning "true" if the key was found.
// If v is non-NULL, copies the value for this key into *v.
bool upb_inttable_lookup(const upb_inttable* t, uintptr_t key, upb_value* v);

// Removes an item from the table. Returns true if the remove was successful,
// and stores the removed item in *val if non-NULL.
bool upb_inttable_remove(upb_inttable* t, uintptr_t key, upb_value* val);

// Updates an existing entry in an inttable.
// If the entry does not exist, returns false and does nothing.
// Unlike insert/remove, this does not invalidate iterators.
bool upb_inttable_replace(upb_inttable* t, uintptr_t key, upb_value val);

// Optimizes the table for the current set of entries, for both memory use and
// lookup time. Client should call this after all entries have been inserted;
// inserting more entries is legal, but will likely require a table resize.
void upb_inttable_compact(upb_inttable* t, upb_Arena* a);

// Iteration over inttable:
//
//   intptr_t iter = UPB_INTTABLE_BEGIN;
//   uintptr_t key;
//   upb_value val;
//   while (upb_inttable_next(t, &key, &val, &iter)) {
//      // ...
//   }

#define UPB_INTTABLE_BEGIN -1

bool upb_inttable_next(const upb_inttable* t, uintptr_t* key, upb_value* val,
                       intptr_t* iter);
void upb_inttable_removeiter(upb_inttable* t, intptr_t* iter);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_HASH_INT_TABLE_H_ */

#ifndef UPB_JSON_DECODE_H_
#define UPB_JSON_DECODE_H_


// Must be last.

#ifdef __cplusplus
extern "C" {
#endif

enum { upb_JsonDecode_IgnoreUnknown = 1 };

bool upb_JsonDecode(const char* buf, size_t size, upb_Message* msg,
                    const upb_MessageDef* m, const upb_DefPool* symtab,
                    int options, upb_Arena* arena, upb_Status* status);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_JSONDECODE_H_ */

#ifndef UPB_LEX_ATOI_H_
#define UPB_LEX_ATOI_H_

// Must be last.

#ifdef __cplusplus
extern "C" {
#endif

// We use these hand-written routines instead of strto[u]l() because the "long
// long" variants aren't in c89. Also our version allows setting a ptr limit.
// Return the new position of the pointer after parsing the int, or NULL on
// integer overflow.

const char* upb_BufToUint64(const char* ptr, const char* end, uint64_t* val);
const char* upb_BufToInt64(const char* ptr, const char* end, int64_t* val,
                           bool* is_neg);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_LEX_ATOI_H_ */

#ifndef UPB_LEX_UNICODE_H_
#define UPB_LEX_UNICODE_H_

// Must be last.

#ifdef __cplusplus
extern "C" {
#endif

// Returns true iff a codepoint is the value for a high surrogate.
UPB_INLINE bool upb_Unicode_IsHigh(uint32_t cp) {
  return (cp >= 0xd800 && cp <= 0xdbff);
}

// Returns true iff a codepoint is the value for a low surrogate.
UPB_INLINE bool upb_Unicode_IsLow(uint32_t cp) {
  return (cp >= 0xdc00 && cp <= 0xdfff);
}

// Returns the high 16-bit surrogate value for a supplementary codepoint.
// Does not sanity-check the input.
UPB_INLINE uint16_t upb_Unicode_ToHigh(uint32_t cp) {
  return (cp >> 10) + 0xd7c0;
}

// Returns the low 16-bit surrogate value for a supplementary codepoint.
// Does not sanity-check the input.
UPB_INLINE uint16_t upb_Unicode_ToLow(uint32_t cp) {
  return (cp & 0x3ff) | 0xdc00;
}

// Returns the 32-bit value corresponding to a pair of 16-bit surrogates.
// Does not sanity-check the input.
UPB_INLINE uint32_t upb_Unicode_FromPair(uint32_t high, uint32_t low) {
  return ((high & 0x3ff) << 10) + (low & 0x3ff) + 0x10000;
}

// Outputs a codepoint as UTF8.
// Returns the number of bytes written (1-4 on success, 0 on error).
// Does not sanity-check the input. Specifically does not check for surrogates.
int upb_Unicode_ToUTF8(uint32_t cp, char* out);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_LEX_UNICODE_H_ */

#ifndef UPB_REFLECTION_MESSAGE_H_
#define UPB_REFLECTION_MESSAGE_H_


// Must be last.

#ifdef __cplusplus
extern "C" {
#endif

// Returns a mutable pointer to a map, array, or submessage value. If the given
// arena is non-NULL this will construct a new object if it was not previously
// present. May not be called for primitive fields.
upb_MutableMessageValue upb_Message_Mutable(upb_Message* msg,
                                            const upb_FieldDef* f,
                                            upb_Arena* a);

// Returns the field that is set in the oneof, or NULL if none are set.
const upb_FieldDef* upb_Message_WhichOneof(const upb_Message* msg,
                                           const upb_OneofDef* o);

// Clear all data and unknown fields.
void upb_Message_ClearByDef(upb_Message* msg, const upb_MessageDef* m);

// Clears any field presence and sets the value back to its default.
void upb_Message_ClearFieldByDef(upb_Message* msg, const upb_FieldDef* f);

// May only be called for fields where upb_FieldDef_HasPresence(f) == true.
bool upb_Message_HasFieldByDef(const upb_Message* msg, const upb_FieldDef* f);

// Returns the value in the message associated with this field def.
upb_MessageValue upb_Message_GetFieldByDef(const upb_Message* msg,
                                           const upb_FieldDef* f);

// Sets the given field to the given value. For a msg/array/map/string, the
// caller must ensure that the target data outlives |msg| (by living either in
// the same arena or a different arena that outlives it).
//
// Returns false if allocation fails.
bool upb_Message_SetFieldByDef(upb_Message* msg, const upb_FieldDef* f,
                               upb_MessageValue val, upb_Arena* a);

// Iterate over present fields.
//
// size_t iter = kUpb_Message_Begin;
// const upb_FieldDef *f;
// upb_MessageValue val;
// while (upb_Message_Next(msg, m, ext_pool, &f, &val, &iter)) {
//   process_field(f, val);
// }
//
// If ext_pool is NULL, no extensions will be returned.  If the given symtab
// returns extensions that don't match what is in this message, those extensions
// will be skipped.

#define kUpb_Message_Begin -1

bool upb_Message_Next(const upb_Message* msg, const upb_MessageDef* m,
                      const upb_DefPool* ext_pool, const upb_FieldDef** f,
                      upb_MessageValue* val, size_t* iter);

// Clears all unknown field data from this message and all submessages.
bool upb_Message_DiscardUnknown(upb_Message* msg, const upb_MessageDef* m,
                                int maxdepth);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_REFLECTION_MESSAGE_H_ */

#ifndef UPB_JSON_ENCODE_H_
#define UPB_JSON_ENCODE_H_


// Must be last.

#ifdef __cplusplus
extern "C" {
#endif

enum {
  /* When set, emits 0/default values.  TODO(haberman): proto3 only? */
  upb_JsonEncode_EmitDefaults = 1 << 0,

  /* When set, use normal (snake_case) field names instead of JSON (camelCase)
     names. */
  upb_JsonEncode_UseProtoNames = 1 << 1,

  /* When set, emits enums as their integer values instead of as their names. */
  upb_JsonEncode_FormatEnumsAsIntegers = 1 << 2
};

/* Encodes the given |msg| to JSON format.  The message's reflection is given in
 * |m|.  The symtab in |symtab| is used to find extensions (if NULL, extensions
 * will not be printed).
 *
 * Output is placed in the given buffer, and always NULL-terminated.  The output
 * size (excluding NULL) is returned.  This means that a return value >= |size|
 * implies that the output was truncated.  (These are the same semantics as
 * snprintf()). */
size_t upb_JsonEncode(const upb_Message* msg, const upb_MessageDef* m,
                      const upb_DefPool* ext_pool, int options, char* buf,
                      size_t size, upb_Status* status);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_JSONENCODE_H_ */

#ifndef UPB_LEX_ROUND_TRIP_H_
#define UPB_LEX_ROUND_TRIP_H_

// Must be last.

// Encodes a float or double that is round-trippable, but as short as possible.
// These routines are not fully optimal (not guaranteed to be shortest), but are
// short-ish and match the implementation that has been used in protobuf since
// the beginning.

// The given buffer size must be at least kUpb_RoundTripBufferSize.
enum { kUpb_RoundTripBufferSize = 32 };

#ifdef __cplusplus
extern "C" {
#endif

void _upb_EncodeRoundTripDouble(double val, char* buf, size_t size);
void _upb_EncodeRoundTripFloat(float val, char* buf, size_t size);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_LEX_ROUND_TRIP_H_ */

#ifndef UPB_PORT_VSNPRINTF_COMPAT_H_
#define UPB_PORT_VSNPRINTF_COMPAT_H_

// Must be last.

UPB_INLINE int _upb_vsnprintf(char* buf, size_t size, const char* fmt,
                              va_list ap) {
#if defined(__MINGW64__) || defined(__MINGW32__) || defined(_MSC_VER)
  // The msvc runtime has a non-conforming vsnprintf() that requires the
  // following compatibility code to become conformant.
  int n = -1;
  if (size != 0) n = _vsnprintf_s(buf, size, _TRUNCATE, fmt, ap);
  if (n == -1) n = _vscprintf(fmt, ap);
  return n;
#else
  return vsnprintf(buf, size, fmt, ap);
#endif
}


#endif  // UPB_PORT_VSNPRINTF_COMPAT_H_

#ifndef UPB_LEX_STRTOD_H_
#define UPB_LEX_STRTOD_H_

// Must be last.

#ifdef __cplusplus
extern "C" {
#endif

double _upb_NoLocaleStrtod(const char *str, char **endptr);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_LEX_STRTOD_H_ */

#ifndef UPB_MEM_ARENA_INTERNAL_H_
#define UPB_MEM_ARENA_INTERNAL_H_


// Must be last.

typedef struct _upb_MemBlock _upb_MemBlock;

struct upb_Arena {
  _upb_ArenaHead head;

  // upb_alloc* together with a low bit which signals if there is an initial
  // block.
  uintptr_t block_alloc;

  // When multiple arenas are fused together, each arena points to a parent
  // arena (root points to itself). The root tracks how many live arenas
  // reference it.

  // The low bit is tagged:
  //   0: pointer to parent
  //   1: count, left shifted by one
  UPB_ATOMIC(uintptr_t) parent_or_count;

  // All nodes that are fused together are in a singly-linked list.
  UPB_ATOMIC(upb_Arena*) next;  // NULL at end of list.

  // The last element of the linked list.  This is present only as an
  // optimization, so that we do not have to iterate over all members for every
  // fuse.  Only significant for an arena root.  In other cases it is ignored.
  UPB_ATOMIC(upb_Arena*) tail;  // == self when no other list members.

  // Linked list of blocks to free/cleanup.  Atomic only for the benefit of
  // upb_Arena_SpaceAllocated().
  UPB_ATOMIC(_upb_MemBlock*) blocks;
};

UPB_INLINE bool _upb_Arena_IsTaggedRefcount(uintptr_t parent_or_count) {
  return (parent_or_count & 1) == 1;
}

UPB_INLINE bool _upb_Arena_IsTaggedPointer(uintptr_t parent_or_count) {
  return (parent_or_count & 1) == 0;
}

UPB_INLINE uintptr_t _upb_Arena_RefCountFromTagged(uintptr_t parent_or_count) {
  UPB_ASSERT(_upb_Arena_IsTaggedRefcount(parent_or_count));
  return parent_or_count >> 1;
}

UPB_INLINE uintptr_t _upb_Arena_TaggedFromRefcount(uintptr_t refcount) {
  uintptr_t parent_or_count = (refcount << 1) | 1;
  UPB_ASSERT(_upb_Arena_IsTaggedRefcount(parent_or_count));
  return parent_or_count;
}

UPB_INLINE upb_Arena* _upb_Arena_PointerFromTagged(uintptr_t parent_or_count) {
  UPB_ASSERT(_upb_Arena_IsTaggedPointer(parent_or_count));
  return (upb_Arena*)parent_or_count;
}

UPB_INLINE uintptr_t _upb_Arena_TaggedFromPointer(upb_Arena* a) {
  uintptr_t parent_or_count = (uintptr_t)a;
  UPB_ASSERT(_upb_Arena_IsTaggedPointer(parent_or_count));
  return parent_or_count;
}

UPB_INLINE upb_alloc* upb_Arena_BlockAlloc(upb_Arena* arena) {
  return (upb_alloc*)(arena->block_alloc & ~0x1);
}

UPB_INLINE uintptr_t upb_Arena_MakeBlockAlloc(upb_alloc* alloc,
                                              bool has_initial) {
  uintptr_t alloc_uint = (uintptr_t)alloc;
  UPB_ASSERT((alloc_uint & 1) == 0);
  return alloc_uint | (has_initial ? 1 : 0);
}

UPB_INLINE bool upb_Arena_HasInitialBlock(upb_Arena* arena) {
  return arena->block_alloc & 0x1;
}


#endif /* UPB_MEM_ARENA_INTERNAL_H_ */

#ifndef UPB_PORT_ATOMIC_H_
#define UPB_PORT_ATOMIC_H_


#ifdef UPB_USE_C11_ATOMICS

#include <stdatomic.h>
#include <stdbool.h>

#define upb_Atomic_Init(addr, val) atomic_init(addr, val)
#define upb_Atomic_Load(addr, order) atomic_load_explicit(addr, order)
#define upb_Atomic_Store(addr, val, order) \
  atomic_store_explicit(addr, val, order)
#define upb_Atomic_Add(addr, val, order) \
  atomic_fetch_add_explicit(addr, val, order)
#define upb_Atomic_Sub(addr, val, order) \
  atomic_fetch_sub_explicit(addr, val, order)
#define upb_Atomic_Exchange(addr, val, order) \
  atomic_exchange_explicit(addr, val, order)
#define upb_Atomic_CompareExchangeStrong(addr, expected, desired,      \
                                         success_order, failure_order) \
  atomic_compare_exchange_strong_explicit(addr, expected, desired,     \
                                          success_order, failure_order)
#define upb_Atomic_CompareExchangeWeak(addr, expected, desired, success_order, \
                                       failure_order)                          \
  atomic_compare_exchange_weak_explicit(addr, expected, desired,               \
                                        success_order, failure_order)

#else  // !UPB_USE_C11_ATOMICS

#include <string.h>

#define upb_Atomic_Init(addr, val) (*addr = val)
#define upb_Atomic_Load(addr, order) (*addr)
#define upb_Atomic_Store(addr, val, order) (*(addr) = val)
#define upb_Atomic_Add(addr, val, order) (*(addr) += val)
#define upb_Atomic_Sub(addr, val, order) (*(addr) -= val)

UPB_INLINE void* _upb_NonAtomic_Exchange(void* addr, void* value) {
  void* old;
  memcpy(&old, addr, sizeof(value));
  memcpy(addr, &value, sizeof(value));
  return old;
}

#define upb_Atomic_Exchange(addr, val, order) _upb_NonAtomic_Exchange(addr, val)

// `addr` and `expected` are logically double pointers.
UPB_INLINE bool _upb_NonAtomic_CompareExchangeStrongP(void* addr,
                                                      void* expected,
                                                      void* desired) {
  if (memcmp(addr, expected, sizeof(desired)) == 0) {
    memcpy(addr, &desired, sizeof(desired));
    return true;
  } else {
    memcpy(expected, addr, sizeof(desired));
    return false;
  }
}

#define upb_Atomic_CompareExchangeStrong(addr, expected, desired,      \
                                         success_order, failure_order) \
  _upb_NonAtomic_CompareExchangeStrongP((void*)addr, (void*)expected,  \
                                        (void*)desired)
#define upb_Atomic_CompareExchangeWeak(addr, expected, desired, success_order, \
                                       failure_order)                          \
  upb_Atomic_CompareExchangeStrong(addr, expected, desired, 0, 0)

#endif


#endif  // UPB_PORT_ATOMIC_H_

#ifndef UPB_WIRE_COMMON_H_
#define UPB_WIRE_COMMON_H_

// Must be last.

#ifdef __cplusplus
extern "C" {
#endif

#define kUpb_WireFormat_DefaultDepthLimit 100

#ifdef __cplusplus
}
#endif

#endif  // UPB_WIRE_COMMON_H_

#ifndef UPB_WIRE_READER_H_
#define UPB_WIRE_READER_H_


#ifndef UPB_WIRE_SWAP_INTERNAL_H_
#define UPB_WIRE_SWAP_INTERNAL_H_

// Must be last.

#ifdef __cplusplus
extern "C" {
#endif

UPB_INLINE bool _upb_IsLittleEndian(void) {
  int x = 1;
  return *(char*)&x == 1;
}

UPB_INLINE uint32_t _upb_BigEndian_Swap32(uint32_t val) {
  if (_upb_IsLittleEndian()) return val;

  return ((val & 0xff) << 24) | ((val & 0xff00) << 8) |
         ((val & 0xff0000) >> 8) | ((val & 0xff000000) >> 24);
}

UPB_INLINE uint64_t _upb_BigEndian_Swap64(uint64_t val) {
  if (_upb_IsLittleEndian()) return val;

  return ((uint64_t)_upb_BigEndian_Swap32((uint32_t)val) << 32) |
         _upb_BigEndian_Swap32((uint32_t)(val >> 32));
}

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_WIRE_SWAP_INTERNAL_H_ */

#ifndef UPB_WIRE_TYPES_H_
#define UPB_WIRE_TYPES_H_

// A list of types as they are encoded on the wire.
typedef enum {
  kUpb_WireType_Varint = 0,
  kUpb_WireType_64Bit = 1,
  kUpb_WireType_Delimited = 2,
  kUpb_WireType_StartGroup = 3,
  kUpb_WireType_EndGroup = 4,
  kUpb_WireType_32Bit = 5
} upb_WireType;

#endif /* UPB_WIRE_TYPES_H_ */

// Must be last.

#ifdef __cplusplus
extern "C" {
#endif

// The upb_WireReader interface is suitable for general-purpose parsing of
// protobuf binary wire format.  It is designed to be used along with
// upb_EpsCopyInputStream for buffering, and all parsing routines in this file
// assume that at least kUpb_EpsCopyInputStream_SlopBytes worth of data is
// available to read without any bounds checks.

#define kUpb_WireReader_WireTypeMask 7
#define kUpb_WireReader_WireTypeBits 3

typedef struct {
  const char* ptr;
  uint64_t val;
} _upb_WireReader_ReadLongVarintRet;

_upb_WireReader_ReadLongVarintRet _upb_WireReader_ReadLongVarint(
    const char* ptr, uint64_t val);

static UPB_FORCEINLINE const char* _upb_WireReader_ReadVarint(const char* ptr,
                                                              uint64_t* val,
                                                              int maxlen,
                                                              uint64_t maxval) {
  uint64_t byte = (uint8_t)*ptr;
  if (UPB_LIKELY((byte & 0x80) == 0)) {
    *val = (uint32_t)byte;
    return ptr + 1;
  }
  const char* start = ptr;
  _upb_WireReader_ReadLongVarintRet res =
      _upb_WireReader_ReadLongVarint(ptr, byte);
  if (!res.ptr || (maxlen < 10 && res.ptr - start > maxlen) ||
      res.val > maxval) {
    return NULL;  // Malformed.
  }
  *val = res.val;
  return res.ptr;
}

// Parses a tag into `tag`, and returns a pointer past the end of the tag, or
// NULL if there was an error in the tag data.
//
// REQUIRES: there must be at least 10 bytes of data available at `ptr`.
// Bounds checks must be performed before calling this function, preferably
// by calling upb_EpsCopyInputStream_IsDone().
static UPB_FORCEINLINE const char* upb_WireReader_ReadTag(const char* ptr,
                                                          uint32_t* tag) {
  uint64_t val;
  ptr = _upb_WireReader_ReadVarint(ptr, &val, 5, UINT32_MAX);
  if (!ptr) return NULL;
  *tag = val;
  return ptr;
}

// Given a tag, returns the field number.
UPB_INLINE uint32_t upb_WireReader_GetFieldNumber(uint32_t tag) {
  return tag >> kUpb_WireReader_WireTypeBits;
}

// Given a tag, returns the wire type.
UPB_INLINE uint8_t upb_WireReader_GetWireType(uint32_t tag) {
  return tag & kUpb_WireReader_WireTypeMask;
}

UPB_INLINE const char* upb_WireReader_ReadVarint(const char* ptr,
                                                 uint64_t* val) {
  return _upb_WireReader_ReadVarint(ptr, val, 10, UINT64_MAX);
}

// Skips data for a varint, returning a pointer past the end of the varint, or
// NULL if there was an error in the varint data.
//
// REQUIRES: there must be at least 10 bytes of data available at `ptr`.
// Bounds checks must be performed before calling this function, preferably
// by calling upb_EpsCopyInputStream_IsDone().
UPB_INLINE const char* upb_WireReader_SkipVarint(const char* ptr) {
  uint64_t val;
  return upb_WireReader_ReadVarint(ptr, &val);
}

// Reads a varint indicating the size of a delimited field into `size`, or
// NULL if there was an error in the varint data.
//
// REQUIRES: there must be at least 10 bytes of data available at `ptr`.
// Bounds checks must be performed before calling this function, preferably
// by calling upb_EpsCopyInputStream_IsDone().
UPB_INLINE const char* upb_WireReader_ReadSize(const char* ptr, int* size) {
  uint64_t size64;
  ptr = upb_WireReader_ReadVarint(ptr, &size64);
  if (!ptr || size64 >= INT32_MAX) return NULL;
  *size = size64;
  return ptr;
}

// Reads a fixed32 field, performing byte swapping if necessary.
//
// REQUIRES: there must be at least 4 bytes of data available at `ptr`.
// Bounds checks must be performed before calling this function, preferably
// by calling upb_EpsCopyInputStream_IsDone().
UPB_INLINE const char* upb_WireReader_ReadFixed32(const char* ptr, void* val) {
  uint32_t uval;
  memcpy(&uval, ptr, 4);
  uval = _upb_BigEndian_Swap32(uval);
  memcpy(val, &uval, 4);
  return ptr + 4;
}

// Reads a fixed64 field, performing byte swapping if necessary.
//
// REQUIRES: there must be at least 4 bytes of data available at `ptr`.
// Bounds checks must be performed before calling this function, preferably
// by calling upb_EpsCopyInputStream_IsDone().
UPB_INLINE const char* upb_WireReader_ReadFixed64(const char* ptr, void* val) {
  uint64_t uval;
  memcpy(&uval, ptr, 8);
  uval = _upb_BigEndian_Swap64(uval);
  memcpy(val, &uval, 8);
  return ptr + 8;
}

const char* _upb_WireReader_SkipGroup(const char* ptr, uint32_t tag,
                                      int depth_limit,
                                      upb_EpsCopyInputStream* stream);

// Skips data for a group, returning a pointer past the end of the group, or
// NULL if there was an error parsing the group.  The `tag` argument should be
// the start group tag that begins the group.  The `depth_limit` argument
// indicates how many levels of recursion the group is allowed to have before
// reporting a parse error (this limit exists to protect against stack
// overflow).
//
// TODO: evaluate how the depth_limit should be specified. Do users need
// control over this?
UPB_INLINE const char* upb_WireReader_SkipGroup(
    const char* ptr, uint32_t tag, upb_EpsCopyInputStream* stream) {
  return _upb_WireReader_SkipGroup(ptr, tag, 100, stream);
}

UPB_INLINE const char* _upb_WireReader_SkipValue(
    const char* ptr, uint32_t tag, int depth_limit,
    upb_EpsCopyInputStream* stream) {
  switch (upb_WireReader_GetWireType(tag)) {
    case kUpb_WireType_Varint:
      return upb_WireReader_SkipVarint(ptr);
    case kUpb_WireType_32Bit:
      return ptr + 4;
    case kUpb_WireType_64Bit:
      return ptr + 8;
    case kUpb_WireType_Delimited: {
      int size;
      ptr = upb_WireReader_ReadSize(ptr, &size);
      if (!ptr) return NULL;
      ptr += size;
      return ptr;
    }
    case kUpb_WireType_StartGroup:
      return _upb_WireReader_SkipGroup(ptr, tag, depth_limit, stream);
    case kUpb_WireType_EndGroup:
      return NULL;  // Should be handled before now.
    default:
      return NULL;  // Unknown wire type.
  }
}

// Skips data for a wire value of any type, returning a pointer past the end of
// the data, or NULL if there was an error parsing the group. The `tag` argument
// should be the tag that was just parsed. The `depth_limit` argument indicates
// how many levels of recursion a group is allowed to have before reporting a
// parse error (this limit exists to protect against stack overflow).
//
// REQUIRES: there must be at least 10 bytes of data available at `ptr`.
// Bounds checks must be performed before calling this function, preferably
// by calling upb_EpsCopyInputStream_IsDone().
//
// TODO: evaluate how the depth_limit should be specified. Do users need
// control over this?
UPB_INLINE const char* upb_WireReader_SkipValue(
    const char* ptr, uint32_t tag, upb_EpsCopyInputStream* stream) {
  return _upb_WireReader_SkipValue(ptr, tag, 100, stream);
}

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif  // UPB_WIRE_READER_H_

#ifndef UPB_MINI_TABLE_COMMON_INTERNAL_H_
#define UPB_MINI_TABLE_COMMON_INTERNAL_H_


// Must be last.

typedef enum {
  kUpb_EncodedType_Double = 0,
  kUpb_EncodedType_Float = 1,
  kUpb_EncodedType_Fixed32 = 2,
  kUpb_EncodedType_Fixed64 = 3,
  kUpb_EncodedType_SFixed32 = 4,
  kUpb_EncodedType_SFixed64 = 5,
  kUpb_EncodedType_Int32 = 6,
  kUpb_EncodedType_UInt32 = 7,
  kUpb_EncodedType_SInt32 = 8,
  kUpb_EncodedType_Int64 = 9,
  kUpb_EncodedType_UInt64 = 10,
  kUpb_EncodedType_SInt64 = 11,
  kUpb_EncodedType_OpenEnum = 12,
  kUpb_EncodedType_Bool = 13,
  kUpb_EncodedType_Bytes = 14,
  kUpb_EncodedType_String = 15,
  kUpb_EncodedType_Group = 16,
  kUpb_EncodedType_Message = 17,
  kUpb_EncodedType_ClosedEnum = 18,

  kUpb_EncodedType_RepeatedBase = 20,
} upb_EncodedType;

typedef enum {
  kUpb_EncodedFieldModifier_FlipPacked = 1 << 0,
  kUpb_EncodedFieldModifier_IsRequired = 1 << 1,
  kUpb_EncodedFieldModifier_IsProto3Singular = 1 << 2,
} upb_EncodedFieldModifier;

enum {
  kUpb_EncodedValue_MinField = ' ',
  kUpb_EncodedValue_MaxField = 'I',
  kUpb_EncodedValue_MinModifier = 'L',
  kUpb_EncodedValue_MaxModifier = '[',
  kUpb_EncodedValue_End = '^',
  kUpb_EncodedValue_MinSkip = '_',
  kUpb_EncodedValue_MaxSkip = '~',
  kUpb_EncodedValue_OneofSeparator = '~',
  kUpb_EncodedValue_FieldSeparator = '|',
  kUpb_EncodedValue_MinOneofField = ' ',
  kUpb_EncodedValue_MaxOneofField = 'b',
  kUpb_EncodedValue_MaxEnumMask = 'A',
};

enum {
  kUpb_EncodedVersion_EnumV1 = '!',
  kUpb_EncodedVersion_ExtensionV1 = '#',
  kUpb_EncodedVersion_MapV1 = '%',
  kUpb_EncodedVersion_MessageV1 = '$',
  kUpb_EncodedVersion_MessageSetV1 = '&',
};

#ifdef __cplusplus
extern "C" {
#endif

UPB_INLINE char _upb_ToBase92(int8_t ch) {
  extern const char _kUpb_ToBase92[];
  UPB_ASSERT(0 <= ch && ch < 92);
  return _kUpb_ToBase92[ch];
}

UPB_INLINE char _upb_FromBase92(uint8_t ch) {
  extern const int8_t _kUpb_FromBase92[];
  if (' ' > ch || ch > '~') return -1;
  return _kUpb_FromBase92[ch - ' '];
}

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_MINI_TABLE_COMMON_INTERNAL_H_ */

#ifndef UPB_MINI_TABLE_ENCODE_INTERNAL_H_
#define UPB_MINI_TABLE_ENCODE_INTERNAL_H_


// Must be last.

// If the input buffer has at least this many bytes available, the encoder call
// is guaranteed to succeed (as long as field number order is maintained).
#define kUpb_MtDataEncoder_MinSize 16

typedef struct {
  char* end;  // Limit of the buffer passed as a parameter.
  // Aliased to internal-only members in .cc.
  char internal[32];
} upb_MtDataEncoder;

#ifdef __cplusplus
extern "C" {
#endif

// Encodes field/oneof information for a given message.  The sequence of calls
// should look like:
//
//   upb_MtDataEncoder e;
//   char buf[256];
//   char* ptr = buf;
//   e.end = ptr + sizeof(buf);
//   unit64_t msg_mod = ...; // bitwise & of kUpb_MessageModifiers or zero
//   ptr = upb_MtDataEncoder_StartMessage(&e, ptr, msg_mod);
//   // Fields *must* be in field number order.
//   ptr = upb_MtDataEncoder_PutField(&e, ptr, ...);
//   ptr = upb_MtDataEncoder_PutField(&e, ptr, ...);
//   ptr = upb_MtDataEncoder_PutField(&e, ptr, ...);
//
//   // If oneofs are present.  Oneofs must be encoded after regular fields.
//   ptr = upb_MiniTable_StartOneof(&e, ptr)
//   ptr = upb_MiniTable_PutOneofField(&e, ptr, ...);
//   ptr = upb_MiniTable_PutOneofField(&e, ptr, ...);
//
//   ptr = upb_MiniTable_StartOneof(&e, ptr);
//   ptr = upb_MiniTable_PutOneofField(&e, ptr, ...);
//   ptr = upb_MiniTable_PutOneofField(&e, ptr, ...);
//
// Oneofs must be encoded after all regular fields.
char* upb_MtDataEncoder_StartMessage(upb_MtDataEncoder* e, char* ptr,
                                     uint64_t msg_mod);
char* upb_MtDataEncoder_PutField(upb_MtDataEncoder* e, char* ptr,
                                 upb_FieldType type, uint32_t field_num,
                                 uint64_t field_mod);
char* upb_MtDataEncoder_StartOneof(upb_MtDataEncoder* e, char* ptr);
char* upb_MtDataEncoder_PutOneofField(upb_MtDataEncoder* e, char* ptr,
                                      uint32_t field_num);

// Encodes the set of values for a given enum. The values must be given in
// order (after casting to uint32_t), and repeats are not allowed.
char* upb_MtDataEncoder_StartEnum(upb_MtDataEncoder* e, char* ptr);
char* upb_MtDataEncoder_PutEnumValue(upb_MtDataEncoder* e, char* ptr,
                                     uint32_t val);
char* upb_MtDataEncoder_EndEnum(upb_MtDataEncoder* e, char* ptr);

// Encodes an entire mini descriptor for an extension.
char* upb_MtDataEncoder_EncodeExtension(upb_MtDataEncoder* e, char* ptr,
                                        upb_FieldType type, uint32_t field_num,
                                        uint64_t field_mod);

// Encodes an entire mini descriptor for a map.
char* upb_MtDataEncoder_EncodeMap(upb_MtDataEncoder* e, char* ptr,
                                  upb_FieldType key_type,
                                  upb_FieldType value_type, uint64_t key_mod,
                                  uint64_t value_mod);

// Encodes an entire mini descriptor for a message set.
char* upb_MtDataEncoder_EncodeMessageSet(upb_MtDataEncoder* e, char* ptr);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_MINI_TABLE_ENCODE_INTERNAL_H_ */

#ifndef UPB_REFLECTION_DEF_BUILDER_INTERNAL_H_
#define UPB_REFLECTION_DEF_BUILDER_INTERNAL_H_


// Must be last.

// We want to copy the options verbatim into the destination options proto.
// We use serialize+parse as our deep copy.
#define UPB_DEF_SET_OPTIONS(target, desc_type, options_type, proto)           \
  if (UPB_DESC(desc_type##_has_options)(proto)) {                             \
    size_t size;                                                              \
    char* pb = UPB_DESC(options_type##_serialize)(                            \
        UPB_DESC(desc_type##_options)(proto), ctx->tmp_arena, &size);         \
    if (!pb) _upb_DefBuilder_OomErr(ctx);                                     \
    target =                                                                  \
        UPB_DESC(options_type##_parse)(pb, size, _upb_DefBuilder_Arena(ctx)); \
    if (!target) _upb_DefBuilder_OomErr(ctx);                                 \
  } else {                                                                    \
    target = (const UPB_DESC(options_type)*)kUpbDefOptDefault;                \
  }

#ifdef __cplusplus
extern "C" {
#endif

struct upb_DefBuilder {
  upb_DefPool* symtab;
  upb_FileDef* file;                 // File we are building.
  upb_Arena* arena;                  // Allocate defs here.
  upb_Arena* tmp_arena;              // For temporary allocations.
  upb_Status* status;                // Record errors here.
  const upb_MiniTableFile* layout;   // NULL if we should build layouts.
  upb_MiniTablePlatform platform;    // Platform we are targeting.
  int enum_count;                    // Count of enums built so far.
  int msg_count;                     // Count of messages built so far.
  int ext_count;                     // Count of extensions built so far.
  jmp_buf err;                       // longjmp() on error.
};

extern const char* kUpbDefOptDefault;

// ctx->status has already been set elsewhere so just fail/longjmp()
UPB_NORETURN void _upb_DefBuilder_FailJmp(upb_DefBuilder* ctx);

UPB_NORETURN void _upb_DefBuilder_Errf(upb_DefBuilder* ctx, const char* fmt,
                                       ...) UPB_PRINTF(2, 3);
UPB_NORETURN void _upb_DefBuilder_OomErr(upb_DefBuilder* ctx);

const char* _upb_DefBuilder_MakeFullName(upb_DefBuilder* ctx,
                                         const char* prefix,
                                         upb_StringView name);

// Given a symbol and the base symbol inside which it is defined,
// find the symbol's definition.
const void* _upb_DefBuilder_ResolveAny(upb_DefBuilder* ctx,
                                       const char* from_name_dbg,
                                       const char* base, upb_StringView sym,
                                       upb_deftype_t* type);

const void* _upb_DefBuilder_Resolve(upb_DefBuilder* ctx,
                                    const char* from_name_dbg, const char* base,
                                    upb_StringView sym, upb_deftype_t type);

char _upb_DefBuilder_ParseEscape(upb_DefBuilder* ctx, const upb_FieldDef* f,
                                 const char** src, const char* end);

const char* _upb_DefBuilder_FullToShort(const char* fullname);

UPB_INLINE void* _upb_DefBuilder_Alloc(upb_DefBuilder* ctx, size_t bytes) {
  if (bytes == 0) return NULL;
  void* ret = upb_Arena_Malloc(ctx->arena, bytes);
  if (!ret) _upb_DefBuilder_OomErr(ctx);
  return ret;
}

// Adds a symbol |v| to the symtab, which must be a def pointer previously
// packed with pack_def(). The def's pointer to upb_FileDef* must be set before
// adding, so we know which entries to remove if building this file fails.
UPB_INLINE void _upb_DefBuilder_Add(upb_DefBuilder* ctx, const char* name,
                                    upb_value v) {
  upb_StringView sym = {.data = name, .size = strlen(name)};
  bool ok = _upb_DefPool_InsertSym(ctx->symtab, sym, v, ctx->status);
  if (!ok) _upb_DefBuilder_FailJmp(ctx);
}

UPB_INLINE upb_Arena* _upb_DefBuilder_Arena(const upb_DefBuilder* ctx) {
  return ctx->arena;
}

UPB_INLINE upb_FileDef* _upb_DefBuilder_File(const upb_DefBuilder* ctx) {
  return ctx->file;
}

// This version of CheckIdent() is only called by other, faster versions after
// they detect a parsing error.
void _upb_DefBuilder_CheckIdentSlow(upb_DefBuilder* ctx, upb_StringView name,
                                    bool full);

// Verify a full identifier string. This is slightly more complicated than
// verifying a relative identifier string because we must track '.' chars.
UPB_INLINE void _upb_DefBuilder_CheckIdentFull(upb_DefBuilder* ctx,
                                               upb_StringView name) {
  bool good = name.size > 0;
  bool start = true;

  for (size_t i = 0; i < name.size; i++) {
    const char c = name.data[i];
    const char d = c | 0x20;  // force lowercase
    const bool is_alpha = (('a' <= d) & (d <= 'z')) | (c == '_');
    const bool is_numer = ('0' <= c) & (c <= '9') & !start;
    const bool is_dot = (c == '.') & !start;

    good &= is_alpha | is_numer | is_dot;
    start = is_dot;
  }

  if (!good) _upb_DefBuilder_CheckIdentSlow(ctx, name, true);
}

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_REFLECTION_DEF_BUILDER_INTERNAL_H_ */

#ifndef UPB_REFLECTION_ENUM_DEF_INTERNAL_H_
#define UPB_REFLECTION_ENUM_DEF_INTERNAL_H_


// Must be last.

#ifdef __cplusplus
extern "C" {
#endif

upb_EnumDef* _upb_EnumDef_At(const upb_EnumDef* e, int i);
bool _upb_EnumDef_Insert(upb_EnumDef* e, upb_EnumValueDef* v, upb_Arena* a);
const upb_MiniTableEnum* _upb_EnumDef_MiniTable(const upb_EnumDef* e);

// Allocate and initialize an array of |n| enum defs.
upb_EnumDef* _upb_EnumDefs_New(
    upb_DefBuilder* ctx, int n,
    const UPB_DESC(EnumDescriptorProto) * const* protos,
    const upb_MessageDef* containing_type);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_REFLECTION_ENUM_DEF_INTERNAL_H_ */

#ifndef UPB_REFLECTION_ENUM_VALUE_DEF_INTERNAL_H_
#define UPB_REFLECTION_ENUM_VALUE_DEF_INTERNAL_H_


// Must be last.

#ifdef __cplusplus
extern "C" {
#endif

upb_EnumValueDef* _upb_EnumValueDef_At(const upb_EnumValueDef* v, int i);

// Allocate and initialize an array of |n| enum value defs owned by |e|.
upb_EnumValueDef* _upb_EnumValueDefs_New(
    upb_DefBuilder* ctx, const char* prefix, int n,
    const UPB_DESC(EnumValueDescriptorProto) * const* protos, upb_EnumDef* e,
    bool* is_sorted);

const upb_EnumValueDef** _upb_EnumValueDefs_Sorted(const upb_EnumValueDef* v,
                                                   int n, upb_Arena* a);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_REFLECTION_ENUM_VALUE_DEF_INTERNAL_H_ */

#ifndef UPB_REFLECTION_FIELD_DEF_INTERNAL_H_
#define UPB_REFLECTION_FIELD_DEF_INTERNAL_H_


// Must be last.

#ifdef __cplusplus
extern "C" {
#endif

upb_FieldDef* _upb_FieldDef_At(const upb_FieldDef* f, int i);

const upb_MiniTableExtension* _upb_FieldDef_ExtensionMiniTable(
    const upb_FieldDef* f);
bool _upb_FieldDef_IsClosedEnum(const upb_FieldDef* f);
bool _upb_FieldDef_IsProto3Optional(const upb_FieldDef* f);
int _upb_FieldDef_LayoutIndex(const upb_FieldDef* f);
uint64_t _upb_FieldDef_Modifiers(const upb_FieldDef* f);
void _upb_FieldDef_Resolve(upb_DefBuilder* ctx, const char* prefix,
                           upb_FieldDef* f);
void _upb_FieldDef_BuildMiniTableExtension(upb_DefBuilder* ctx,
                                           const upb_FieldDef* f);

// Allocate and initialize an array of |n| extensions (field defs).
upb_FieldDef* _upb_Extensions_New(
    upb_DefBuilder* ctx, int n,
    const UPB_DESC(FieldDescriptorProto) * const* protos, const char* prefix,
    upb_MessageDef* m);

// Allocate and initialize an array of |n| field defs.
upb_FieldDef* _upb_FieldDefs_New(
    upb_DefBuilder* ctx, int n,
    const UPB_DESC(FieldDescriptorProto) * const* protos, const char* prefix,
    upb_MessageDef* m, bool* is_sorted);

// Allocate and return a list of pointers to the |n| field defs in |ff|,
// sorted by field number.
const upb_FieldDef** _upb_FieldDefs_Sorted(const upb_FieldDef* f, int n,
                                           upb_Arena* a);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_REFLECTION_FIELD_DEF_INTERNAL_H_ */

#ifndef UPB_REFLECTION_FILE_DEF_INTERNAL_H_
#define UPB_REFLECTION_FILE_DEF_INTERNAL_H_


// Must be last.

#ifdef __cplusplus
extern "C" {
#endif

const upb_MiniTableExtension* _upb_FileDef_ExtensionMiniTable(
    const upb_FileDef* f, int i);
const int32_t* _upb_FileDef_PublicDependencyIndexes(const upb_FileDef* f);
const int32_t* _upb_FileDef_WeakDependencyIndexes(const upb_FileDef* f);

// upb_FileDef_Package() returns "" if f->package is NULL, this does not.
const char* _upb_FileDef_RawPackage(const upb_FileDef* f);

void _upb_FileDef_Create(upb_DefBuilder* ctx,
                         const UPB_DESC(FileDescriptorProto) * file_proto);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_REFLECTION_FILE_DEF_INTERNAL_H_ */

#ifndef UPB_REFLECTION_MESSAGE_DEF_INTERNAL_H_
#define UPB_REFLECTION_MESSAGE_DEF_INTERNAL_H_


// Must be last.

#ifdef __cplusplus
extern "C" {
#endif

upb_MessageDef* _upb_MessageDef_At(const upb_MessageDef* m, int i);
bool _upb_MessageDef_InMessageSet(const upb_MessageDef* m);
bool _upb_MessageDef_Insert(upb_MessageDef* m, const char* name, size_t size,
                            upb_value v, upb_Arena* a);
void _upb_MessageDef_InsertField(upb_DefBuilder* ctx, upb_MessageDef* m,
                                 const upb_FieldDef* f);
bool _upb_MessageDef_IsValidExtensionNumber(const upb_MessageDef* m, int n);
void _upb_MessageDef_CreateMiniTable(upb_DefBuilder* ctx, upb_MessageDef* m);
void _upb_MessageDef_LinkMiniTable(upb_DefBuilder* ctx,
                                   const upb_MessageDef* m);
void _upb_MessageDef_Resolve(upb_DefBuilder* ctx, upb_MessageDef* m);

// Allocate and initialize an array of |n| message defs.
upb_MessageDef* _upb_MessageDefs_New(
    upb_DefBuilder* ctx, int n, const UPB_DESC(DescriptorProto) * const* protos,
    const upb_MessageDef* containing_type);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_REFLECTION_MESSAGE_DEF_INTERNAL_H_ */

#ifndef UPB_REFLECTION_SERVICE_DEF_INTERNAL_H_
#define UPB_REFLECTION_SERVICE_DEF_INTERNAL_H_


// Must be last.

#ifdef __cplusplus
extern "C" {
#endif

upb_ServiceDef* _upb_ServiceDef_At(const upb_ServiceDef* s, int i);

// Allocate and initialize an array of |n| service defs.
upb_ServiceDef* _upb_ServiceDefs_New(
    upb_DefBuilder* ctx, int n,
    const UPB_DESC(ServiceDescriptorProto) * const* protos);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_REFLECTION_SERVICE_DEF_INTERNAL_H_ */

#ifndef UPB_REFLECTION_DESC_STATE_INTERNAL_H_
#define UPB_REFLECTION_DESC_STATE_INTERNAL_H_


// Must be last.

// Manages the storage for mini descriptor strings as they are being encoded.
// TODO(b/234740652): Move some of this state directly into the encoder, maybe.
typedef struct {
  upb_MtDataEncoder e;
  size_t bufsize;
  char* buf;
  char* ptr;
} upb_DescState;

#ifdef __cplusplus
extern "C" {
#endif

UPB_INLINE void _upb_DescState_Init(upb_DescState* d) {
  d->bufsize = kUpb_MtDataEncoder_MinSize * 2;
  d->buf = NULL;
  d->ptr = NULL;
}

bool _upb_DescState_Grow(upb_DescState* d, upb_Arena* a);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_REFLECTION_DESC_STATE_INTERNAL_H_ */

#ifndef UPB_REFLECTION_ENUM_RESERVED_RANGE_INTERNAL_H_
#define UPB_REFLECTION_ENUM_RESERVED_RANGE_INTERNAL_H_


// IWYU pragma: private, include "upb/reflection/def.h"

#ifndef UPB_REFLECTION_ENUM_RESERVED_RANGE_H_
#define UPB_REFLECTION_ENUM_RESERVED_RANGE_H_


// Must be last.

#ifdef __cplusplus
extern "C" {
#endif

int32_t upb_EnumReservedRange_Start(const upb_EnumReservedRange* r);
int32_t upb_EnumReservedRange_End(const upb_EnumReservedRange* r);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_REFLECTION_ENUM_RESERVED_RANGE_H_ */

// Must be last.

#ifdef __cplusplus
extern "C" {
#endif

upb_EnumReservedRange* _upb_EnumReservedRange_At(const upb_EnumReservedRange* r,
                                                 int i);

// Allocate and initialize an array of |n| reserved ranges owned by |e|.
upb_EnumReservedRange* _upb_EnumReservedRanges_New(
    upb_DefBuilder* ctx, int n,
    const UPB_DESC(EnumDescriptorProto_EnumReservedRange) * const* protos,
    const upb_EnumDef* e);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_REFLECTION_ENUM_RESERVED_RANGE_INTERNAL_H_ */

#ifndef UPB_REFLECTION_EXTENSION_RANGE_INTERNAL_H_
#define UPB_REFLECTION_EXTENSION_RANGE_INTERNAL_H_


// Must be last.

#ifdef __cplusplus
extern "C" {
#endif

upb_ExtensionRange* _upb_ExtensionRange_At(const upb_ExtensionRange* r, int i);

// Allocate and initialize an array of |n| extension ranges owned by |m|.
upb_ExtensionRange* _upb_ExtensionRanges_New(
    upb_DefBuilder* ctx, int n,
    const UPB_DESC(DescriptorProto_ExtensionRange) * const* protos,
    const upb_MessageDef* m);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_REFLECTION_EXTENSION_RANGE_INTERNAL_H_ */

#ifndef UPB_REFLECTION_ONEOF_DEF_INTERNAL_H_
#define UPB_REFLECTION_ONEOF_DEF_INTERNAL_H_


// Must be last.

#ifdef __cplusplus
extern "C" {
#endif

upb_OneofDef* _upb_OneofDef_At(const upb_OneofDef* o, int i);
void _upb_OneofDef_Insert(upb_DefBuilder* ctx, upb_OneofDef* o,
                          const upb_FieldDef* f, const char* name, size_t size);

// Allocate and initialize an array of |n| oneof defs owned by |m|.
upb_OneofDef* _upb_OneofDefs_New(
    upb_DefBuilder* ctx, int n,
    const UPB_DESC(OneofDescriptorProto) * const* protos, upb_MessageDef* m);

size_t _upb_OneofDefs_Finalize(upb_DefBuilder* ctx, upb_MessageDef* m);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_REFLECTION_ONEOF_DEF_INTERNAL_H_ */

#ifndef UPB_REFLECTION_MESSAGE_RESERVED_RANGE_INTERNAL_H_
#define UPB_REFLECTION_MESSAGE_RESERVED_RANGE_INTERNAL_H_


// IWYU pragma: private, include "upb/reflection/def.h"

#ifndef UPB_REFLECTION_MESSAGE_RESERVED_RANGE_H_
#define UPB_REFLECTION_MESSAGE_RESERVED_RANGE_H_


// Must be last.

#ifdef __cplusplus
extern "C" {
#endif

int32_t upb_MessageReservedRange_Start(const upb_MessageReservedRange* r);
int32_t upb_MessageReservedRange_End(const upb_MessageReservedRange* r);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_REFLECTION_MESSAGE_RESERVED_RANGE_H_ */

// Must be last.

#ifdef __cplusplus
extern "C" {
#endif

upb_MessageReservedRange* _upb_MessageReservedRange_At(
    const upb_MessageReservedRange* r, int i);

// Allocate and initialize an array of |n| reserved ranges owned by |m|.
upb_MessageReservedRange* _upb_MessageReservedRanges_New(
    upb_DefBuilder* ctx, int n,
    const UPB_DESC(DescriptorProto_ReservedRange) * const* protos,
    const upb_MessageDef* m);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_REFLECTION_MESSAGE_RESERVED_RANGE_INTERNAL_H_ */

#ifndef UPB_REFLECTION_METHOD_DEF_INTERNAL_H_
#define UPB_REFLECTION_METHOD_DEF_INTERNAL_H_


// Must be last.

#ifdef __cplusplus
extern "C" {
#endif

upb_MethodDef* _upb_MethodDef_At(const upb_MethodDef* m, int i);

// Allocate and initialize an array of |n| method defs owned by |s|.
upb_MethodDef* _upb_MethodDefs_New(
    upb_DefBuilder* ctx, int n,
    const UPB_DESC(MethodDescriptorProto) * const* protos, upb_ServiceDef* s);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_REFLECTION_METHOD_DEF_INTERNAL_H_ */

#ifndef UPB_WIRE_COMMON_INTERNAL_H_
#define UPB_WIRE_COMMON_INTERNAL_H_

// Must be last.

// MessageSet wire format is:
//   message MessageSet {
//     repeated group Item = 1 {
//       required int32 type_id = 2;
//       required bytes message = 3;
//     }
//   }

enum {
  kUpb_MsgSet_Item = 1,
  kUpb_MsgSet_TypeId = 2,
  kUpb_MsgSet_Message = 3,
};


#endif /* UPB_WIRE_COMMON_INTERNAL_H_ */

/*
 * Internal implementation details of the decoder that are shared between
 * decode.c and decode_fast.c.
 */

#ifndef UPB_WIRE_DECODE_INTERNAL_H_
#define UPB_WIRE_DECODE_INTERNAL_H_

#include "utf8_range.h"

// Must be last.

#define DECODE_NOGROUP (uint32_t) - 1

typedef struct upb_Decoder {
  upb_EpsCopyInputStream input;
  const upb_ExtensionRegistry* extreg;
  const char* unknown;       // Start of unknown data, preserve at buffer flip
  upb_Message* unknown_msg;  // Pointer to preserve data to
  int depth;                 // Tracks recursion depth to bound stack usage.
  uint32_t end_group;  // field number of END_GROUP tag, else DECODE_NOGROUP.
  uint16_t options;
  bool missing_required;
  upb_Arena arena;
  upb_DecodeStatus status;
  jmp_buf err;

#ifndef NDEBUG
  const char* debug_tagstart;
  const char* debug_valstart;
#endif
} upb_Decoder;

/* Error function that will abort decoding with longjmp(). We can't declare this
 * UPB_NORETURN, even though it is appropriate, because if we do then compilers
 * will "helpfully" refuse to tailcall to it
 * (see: https://stackoverflow.com/a/55657013), which will defeat a major goal
 * of our optimizations. That is also why we must declare it in a separate file,
 * otherwise the compiler will see that it calls longjmp() and deduce that it is
 * noreturn. */
const char* _upb_FastDecoder_ErrorJmp(upb_Decoder* d, int status);

extern const uint8_t upb_utf8_offsets[];

UPB_INLINE
bool _upb_Decoder_VerifyUtf8Inline(const char* ptr, int len) {
  const char* end = ptr + len;

  // Check 8 bytes at a time for any non-ASCII char.
  while (end - ptr >= 8) {
    uint64_t data;
    memcpy(&data, ptr, 8);
    if (data & 0x8080808080808080) goto non_ascii;
    ptr += 8;
  }

  // Check one byte at a time for non-ASCII.
  while (ptr < end) {
    if (*ptr & 0x80) goto non_ascii;
    ptr++;
  }

  return true;

non_ascii:
  return utf8_range2((const unsigned char*)ptr, end - ptr) == 0;
}

const char* _upb_Decoder_CheckRequired(upb_Decoder* d, const char* ptr,
                                       const upb_Message* msg,
                                       const upb_MiniTable* l);

/* x86-64 pointers always have the high 16 bits matching. So we can shift
 * left 8 and right 8 without loss of information. */
UPB_INLINE intptr_t decode_totable(const upb_MiniTable* tablep) {
  return ((intptr_t)tablep << 8) | tablep->table_mask;
}

UPB_INLINE const upb_MiniTable* decode_totablep(intptr_t table) {
  return (const upb_MiniTable*)(table >> 8);
}

const char* _upb_Decoder_IsDoneFallback(upb_EpsCopyInputStream* e,
                                        const char* ptr, int overrun);

UPB_INLINE bool _upb_Decoder_IsDone(upb_Decoder* d, const char** ptr) {
  return upb_EpsCopyInputStream_IsDoneWithCallback(
      &d->input, ptr, &_upb_Decoder_IsDoneFallback);
}

UPB_INLINE const char* _upb_Decoder_BufferFlipCallback(
    upb_EpsCopyInputStream* e, const char* old_end, const char* new_start) {
  upb_Decoder* d = (upb_Decoder*)e;
  if (!old_end) _upb_FastDecoder_ErrorJmp(d, kUpb_DecodeStatus_Malformed);

  if (d->unknown) {
    if (!_upb_Message_AddUnknown(d->unknown_msg, d->unknown,
                                 old_end - d->unknown, &d->arena)) {
      _upb_FastDecoder_ErrorJmp(d, kUpb_DecodeStatus_OutOfMemory);
    }
    d->unknown = new_start;
  }
  return new_start;
}

#if UPB_FASTTABLE
UPB_INLINE
const char* _upb_FastDecoder_TagDispatch(upb_Decoder* d, const char* ptr,
                                         upb_Message* msg, intptr_t table,
                                         uint64_t hasbits, uint64_t tag) {
  const upb_MiniTable* table_p = decode_totablep(table);
  uint8_t mask = table;
  uint64_t data;
  size_t idx = tag & mask;
  UPB_ASSUME((idx & 7) == 0);
  idx >>= 3;
  data = table_p->fasttable[idx].field_data ^ tag;
  UPB_MUSTTAIL return table_p->fasttable[idx].field_parser(d, ptr, msg, table,
                                                           hasbits, data);
}
#endif

UPB_INLINE uint32_t _upb_FastDecoder_LoadTag(const char* ptr) {
  uint16_t tag;
  memcpy(&tag, ptr, 2);
  return tag;
}


#endif /* UPB_WIRE_DECODE_INTERNAL_H_ */

// This should #undef all macros #defined in def.inc

#undef UPB_SIZE
#undef UPB_PTR_AT
#undef UPB_MAPTYPE_STRING
#undef UPB_EXPORT
#undef UPB_INLINE
#undef UPB_API
#undef UPB_API_INLINE
#undef UPB_ALIGN_UP
#undef UPB_ALIGN_DOWN
#undef UPB_ALIGN_MALLOC
#undef UPB_ALIGN_OF
#undef UPB_MALLOC_ALIGN
#undef UPB_LIKELY
#undef UPB_UNLIKELY
#undef UPB_FORCEINLINE
#undef UPB_NOINLINE
#undef UPB_NORETURN
#undef UPB_PRINTF
#undef UPB_MAX
#undef UPB_MIN
#undef UPB_UNUSED
#undef UPB_ASSUME
#undef UPB_ASSERT
#undef UPB_UNREACHABLE
#undef UPB_SETJMP
#undef UPB_LONGJMP
#undef UPB_PTRADD
#undef UPB_MUSTTAIL
#undef UPB_FASTTABLE_SUPPORTED
#undef UPB_FASTTABLE_MASK
#undef UPB_FASTTABLE
#undef UPB_FASTTABLE_INIT
#undef UPB_POISON_MEMORY_REGION
#undef UPB_UNPOISON_MEMORY_REGION
#undef UPB_ASAN
#undef UPB_TREAT_PROTO2_ENUMS_LIKE_PROTO3
#undef UPB_DEPRECATED
#undef UPB_GNUC_MIN
#undef UPB_DESCRIPTOR_UPB_H_FILENAME
#undef UPB_DESC
#undef UPB_IS_GOOGLE3
#undef UPB_ATOMIC
#undef UPB_USE_C11_ATOMICS
#undef UPB_PRIVATE
