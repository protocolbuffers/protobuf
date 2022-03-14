// Ruby is still using proto3 enum semantics for proto2
#define UPB_DISABLE_PROTO2_ENUM_CHECKING
/* Amalgamated source file */
/*
 * Copyright (c) 2009-2021, Google LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Google LLC nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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

/* UPB_PTRADD(ptr, ofs): add pointer while avoiding "NULL + 0" UB */
#define UPB_PTRADD(ptr, ofs) ((ofs) ? (ptr) + (ofs) : (ptr))

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

/* Disable proto2 arena behavior (TEMPORARY) **********************************/

#ifdef UPB_DISABLE_PROTO2_ENUM_CHECKING
#define UPB_TREAT_PROTO2_ENUMS_LIKE_PROTO3 1
#else
#define UPB_TREAT_PROTO2_ENUMS_LIKE_PROTO3 0
#endif

/** upb/decode.h ************************************************************/
/*
 * upb_decode: parsing into a upb_Message using a upb_MiniTable.
 */

#ifndef UPB_DECODE_H_
#define UPB_DECODE_H_


/** upb/msg.h ************************************************************/
/*
 * Public APIs for message operations that do not require descriptors.
 * These functions can be used even in build that does not want to depend on
 * reflection or descriptors.
 *
 * Descriptor-based reflection functionality lives in reflection.h.
 */

#ifndef UPB_MSG_H_
#define UPB_MSG_H_

#include <stddef.h>


/** upb/upb.h ************************************************************/
/*
 * This file contains shared definitions that are widely used across upb.
 */

#ifndef UPB_H_
#define UPB_H_

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>


#ifdef __cplusplus
extern "C" {
#endif

/* upb_Status *****************************************************************/

#define _kUpb_Status_MaxMessage 127

typedef struct {
  bool ok;
  char msg[_kUpb_Status_MaxMessage]; /* Error message; NULL-terminated. */
} upb_Status;

const char* upb_Status_ErrorMessage(const upb_Status* status);
bool upb_Status_IsOk(const upb_Status* status);

/* These are no-op if |status| is NULL. */
void upb_Status_Clear(upb_Status* status);
void upb_Status_SetErrorMessage(upb_Status* status, const char* msg);
void upb_Status_SetErrorFormat(upb_Status* status, const char* fmt, ...)
    UPB_PRINTF(2, 3);
void upb_Status_VSetErrorFormat(upb_Status* status, const char* fmt,
                                va_list args) UPB_PRINTF(2, 0);
void upb_Status_VAppendErrorFormat(upb_Status* status, const char* fmt,
                                   va_list args) UPB_PRINTF(2, 0);

/** upb_StringView ************************************************************/

typedef struct {
  const char* data;
  size_t size;
} upb_StringView;

UPB_INLINE upb_StringView upb_StringView_FromDataAndSize(const char* data,
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

#define UPB_STRINGVIEW_INIT(ptr, len) \
  { ptr, len }

#define UPB_STRINGVIEW_FORMAT "%.*s"
#define UPB_STRINGVIEW_ARGS(view) (int)(view).size, (view).data

/** upb_alloc *****************************************************************/

/* A upb_alloc is a possibly-stateful allocator object.
 *
 * It could either be an arena allocator (which doesn't require individual
 * free() calls) or a regular malloc() (which does).  The client must therefore
 * free memory unless it knows that the allocator is an arena allocator. */

struct upb_alloc;
typedef struct upb_alloc upb_alloc;

/* A malloc()/free() function.
 * If "size" is 0 then the function acts like free(), otherwise it acts like
 * realloc().  Only "oldsize" bytes from a previous allocation are preserved. */
typedef void* upb_alloc_func(upb_alloc* alloc, void* ptr, size_t oldsize,
                             size_t size);

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
  assert(alloc);
  alloc->func(alloc, ptr, 0, 0);
}

/* The global allocator used by upb.  Uses the standard malloc()/free(). */

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

/* upb_Arena ******************************************************************/

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

typedef void upb_CleanupFunc(void* ud);

struct upb_Arena;
typedef struct upb_Arena upb_Arena;

typedef struct {
  /* We implement the allocator interface.
   * This must be the first member of upb_Arena!
   * TODO(haberman): remove once handlers are gone. */
  upb_alloc alloc;

  char *ptr, *end;
} _upb_ArenaHead;

/* Creates an arena from the given initial block (if any -- n may be 0).
 * Additional blocks will be allocated from |alloc|.  If |alloc| is NULL, this
 * is a fixed-size arena and cannot grow. */
upb_Arena* upb_Arena_Init(void* mem, size_t n, upb_alloc* alloc);
void upb_Arena_Free(upb_Arena* a);
bool upb_Arena_AddCleanup(upb_Arena* a, void* ud, upb_CleanupFunc* func);
bool upb_Arena_Fuse(upb_Arena* a, upb_Arena* b);
void* _upb_Arena_SlowMalloc(upb_Arena* a, size_t size);

UPB_INLINE upb_alloc* upb_Arena_Alloc(upb_Arena* a) { return (upb_alloc*)a; }

UPB_INLINE size_t _upb_ArenaHas(upb_Arena* a) {
  _upb_ArenaHead* h = (_upb_ArenaHead*)a;
  return (size_t)(h->end - h->ptr);
}

UPB_INLINE void* upb_Arena_Malloc(upb_Arena* a, size_t size) {
  _upb_ArenaHead* h = (_upb_ArenaHead*)a;
  void* ret;
  size = UPB_ALIGN_MALLOC(size);

  if (UPB_UNLIKELY(_upb_ArenaHas(a) < size)) {
    return _upb_Arena_SlowMalloc(a, size);
  }

  ret = h->ptr;
  h->ptr += size;
  UPB_UNPOISON_MEMORY_REGION(ret, size);

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
UPB_INLINE void upb_Arena_ShrinkLast(upb_Arena* a, void* ptr, size_t oldsize,
                                     size_t size) {
  _upb_ArenaHead* h = (_upb_ArenaHead*)a;
  oldsize = UPB_ALIGN_MALLOC(oldsize);
  size = UPB_ALIGN_MALLOC(size);
  UPB_ASSERT((char*)ptr + oldsize == h->ptr);  // Must be the last alloc.
  UPB_ASSERT(size <= oldsize);
  h->ptr = (char*)ptr + size;
}

UPB_INLINE void* upb_Arena_Realloc(upb_Arena* a, void* ptr, size_t oldsize,
                                   size_t size) {
  _upb_ArenaHead* h = (_upb_ArenaHead*)a;
  oldsize = UPB_ALIGN_MALLOC(oldsize);
  size = UPB_ALIGN_MALLOC(size);
  if (size <= oldsize) {
    if ((char*)ptr + oldsize == h->ptr) {
      upb_Arena_ShrinkLast(a, ptr, oldsize, size);
    }
    return ptr;
  }

  void* ret = upb_Arena_Malloc(a, size);

  if (ret && oldsize > 0) {
    memcpy(ret, ptr, oldsize);
  }

  return ret;
}

UPB_INLINE upb_Arena* upb_Arena_New(void) {
  return upb_Arena_Init(NULL, 0, &upb_alloc_global);
}

/* Constants ******************************************************************/

/* A list of types as they are encoded on-the-wire. */
typedef enum {
  kUpb_WireType_Varint = 0,
  kUpb_WireType_64Bit = 1,
  kUpb_WireType_Delimited = 2,
  kUpb_WireType_StartGroup = 3,
  kUpb_WireType_EndGroup = 4,
  kUpb_WireType_32Bit = 5
} upb_WireType;

/* The types a field can have.  Note that this list is not identical to the
 * types defined in descriptor.proto, which gives INT32 and SINT32 separate
 * types (we distinguish the two with the "integer encoding" enum below). */
typedef enum {
  kUpb_CType_Bool = 1,
  kUpb_CType_Float = 2,
  kUpb_CType_Int32 = 3,
  kUpb_CType_UInt32 = 4,
  kUpb_CType_Enum = 5, /* Enum values are int32. */
  kUpb_CType_Message = 6,
  kUpb_CType_Double = 7,
  kUpb_CType_Int64 = 8,
  kUpb_CType_UInt64 = 9,
  kUpb_CType_String = 10,
  kUpb_CType_Bytes = 11
} upb_CType;

/* The repeated-ness of each field; this matches descriptor.proto. */
typedef enum {
  kUpb_Label_Optional = 1,
  kUpb_Label_Required = 2,
  kUpb_Label_Repeated = 3
} upb_Label;

/* Descriptor types, as defined in descriptor.proto. */
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
  kUpb_FieldType_SInt64 = 18
} upb_FieldType;

#define kUpb_Map_Begin ((size_t)-1)

UPB_INLINE bool _upb_IsLittleEndian(void) {
  int x = 1;
  return *(char*)&x == 1;
}

UPB_INLINE uint32_t _upb_BigEndian_Swap32(uint32_t val) {
  if (_upb_IsLittleEndian()) {
    return val;
  } else {
    return ((val & 0xff) << 24) | ((val & 0xff00) << 8) |
           ((val & 0xff0000) >> 8) | ((val & 0xff000000) >> 24);
  }
}

UPB_INLINE uint64_t _upb_BigEndian_Swap64(uint64_t val) {
  if (_upb_IsLittleEndian()) {
    return val;
  } else {
    return ((uint64_t)_upb_BigEndian_Swap32(val) << 32) |
           _upb_BigEndian_Swap32(val >> 32);
  }
}

UPB_INLINE int _upb_Log2Ceiling(int x) {
  if (x <= 1) return 0;
#ifdef __GNUC__
  return 32 - __builtin_clz(x - 1);
#else
  int lg2 = 0;
  while (1 << lg2 < x) lg2++;
  return lg2;
#endif
}

UPB_INLINE int _upb_Log2CeilingSize(int x) { return 1 << _upb_Log2Ceiling(x); }


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* UPB_H_ */

#ifdef __cplusplus
extern "C" {
#endif

/** upb_Message
 * *******************************************************************/

typedef void upb_Message;

/* For users these are opaque. They can be obtained from
 * upb_MessageDef_MiniTable() but users cannot access any of the members. */
struct upb_MiniTable;
typedef struct upb_MiniTable upb_MiniTable;

/* Adds unknown data (serialized protobuf data) to the given message.  The data
 * is copied into the message instance. */
void upb_Message_AddUnknown(upb_Message* msg, const char* data, size_t len,
                            upb_Arena* arena);

/* Returns a reference to the message's unknown data. */
const char* upb_Message_GetUnknown(const upb_Message* msg, size_t* len);

/* Returns the number of extensions present in this message. */
size_t upb_Message_ExtensionCount(const upb_Message* msg);

/** upb_ExtensionRegistry *****************************************************/

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

struct upb_ExtensionRegistry;
typedef struct upb_ExtensionRegistry upb_ExtensionRegistry;

/* Creates a upb_ExtensionRegistry in the given arena.  The arena must outlive
 * any use of the extreg. */
upb_ExtensionRegistry* upb_ExtensionRegistry_New(upb_Arena* arena);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* UPB_MSG_INT_H_ */

/* Must be last. */

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

#define UPB_DECODE_MAXDEPTH(depth) ((depth) << 16)

typedef enum {
  kUpb_DecodeStatus_Ok = 0,
  kUpb_DecodeStatus_Malformed = 1,         // Wire format was corrupt
  kUpb_DecodeStatus_OutOfMemory = 2,       // Arena alloc failed
  kUpb_DecodeStatus_BadUtf8 = 3,           // String field had bad UTF-8
  kUpb_DecodeStatus_MaxDepthExceeded = 4,  // Exceeded UPB_DECODE_MAXDEPTH

  // kUpb_DecodeOption_CheckRequired failed (see above), but the parse otherwise
  // succeeded.
  kUpb_DecodeStatus_MissingRequired = 5,
} upb_DecodeStatus;

upb_DecodeStatus upb_Decode(const char* buf, size_t size, upb_Message* msg,
                            const upb_MiniTable* l,
                            const upb_ExtensionRegistry* extreg, int options,
                            upb_Arena* arena);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_DECODE_H_ */

/** upb/decode_internal.h ************************************************************/
/*
 * Internal implementation details of the decoder that are shared between
 * decode.c and decode_fast.c.
 */

#ifndef UPB_DECODE_INT_H_
#define UPB_DECODE_INT_H_

#include <setjmp.h>

#include "third_party/utf8_range/utf8_range.h"

/** upb/msg_internal.h ************************************************************/
/*
** Our memory representation for parsing tables and messages themselves.
** Functions in this file are used by generated code and possibly reflection.
**
** The definitions in this file are internal to upb.
**/

#ifndef UPB_MSG_INT_H_
#define UPB_MSG_INT_H_

#include <stdint.h>
#include <stdlib.h>
#include <string.h>


/** upb/table_internal.h ************************************************************/
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

#ifndef UPB_TABLE_H_
#define UPB_TABLE_H_

#include <stdint.h>
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

typedef struct {
  upb_table t;
} upb_strtable;

typedef struct {
  upb_table t;             /* For entries that don't fit in the array part. */
  const upb_tabval* array; /* Array part of the table. See const note above. */
  size_t array_size;       /* Array part size. */
  size_t array_count;      /* Array part number of elements. */
} upb_inttable;

UPB_INLINE size_t upb_table_size(const upb_table* t) {
  if (t->size_lg2 == 0)
    return 0;
  else
    return 1 << t->size_lg2;
}

/* Internal-only functions, in .h file only out of necessity. */
UPB_INLINE bool upb_tabent_isempty(const upb_tabent* e) { return e->key == 0; }

/* Initialize and uninitialize a table, respectively.  If memory allocation
 * failed, false is returned that the table is uninitialized. */
bool upb_inttable_init(upb_inttable* table, upb_Arena* a);
bool upb_strtable_init(upb_strtable* table, size_t expected_size, upb_Arena* a);

/* Returns the number of values in the table. */
size_t upb_inttable_count(const upb_inttable* t);
UPB_INLINE size_t upb_strtable_count(const upb_strtable* t) {
  return t->t.count;
}

void upb_strtable_clear(upb_strtable* t);

/* Inserts the given key into the hashtable with the given value.  The key must
 * not already exist in the hash table.  For string tables, the key must be
 * NULL-terminated, and the table will make an internal copy of the key.
 * Inttables must not insert a value of UINTPTR_MAX.
 *
 * If a table resize was required but memory allocation failed, false is
 * returned and the table is unchanged. */
bool upb_inttable_insert(upb_inttable* t, uintptr_t key, upb_value val,
                         upb_Arena* a);
bool upb_strtable_insert(upb_strtable* t, const char* key, size_t len,
                         upb_value val, upb_Arena* a);

/* Looks up key in this table, returning "true" if the key was found.
 * If v is non-NULL, copies the value for this key into *v. */
bool upb_inttable_lookup(const upb_inttable* t, uintptr_t key, upb_value* v);
bool upb_strtable_lookup2(const upb_strtable* t, const char* key, size_t len,
                          upb_value* v);

/* For NULL-terminated strings. */
UPB_INLINE bool upb_strtable_lookup(const upb_strtable* t, const char* key,
                                    upb_value* v) {
  return upb_strtable_lookup2(t, key, strlen(key), v);
}

/* Removes an item from the table.  Returns true if the remove was successful,
 * and stores the removed item in *val if non-NULL. */
bool upb_inttable_remove(upb_inttable* t, uintptr_t key, upb_value* val);
bool upb_strtable_remove2(upb_strtable* t, const char* key, size_t len,
                          upb_value* val);

UPB_INLINE bool upb_strtable_remove(upb_strtable* t, const char* key,
                                    upb_value* v) {
  return upb_strtable_remove2(t, key, strlen(key), v);
}

/* Updates an existing entry in an inttable.  If the entry does not exist,
 * returns false and does nothing.  Unlike insert/remove, this does not
 * invalidate iterators. */
bool upb_inttable_replace(upb_inttable* t, uintptr_t key, upb_value val);

/* Optimizes the table for the current set of entries, for both memory use and
 * lookup time.  Client should call this after all entries have been inserted;
 * inserting more entries is legal, but will likely require a table resize. */
void upb_inttable_compact(upb_inttable* t, upb_Arena* a);

/* Exposed for testing only. */
bool upb_strtable_resize(upb_strtable* t, size_t size_lg2, upb_Arena* a);

/* Iterators ******************************************************************/

/* Iteration over inttable.
 *
 *   intptr_t iter = UPB_INTTABLE_BEGIN;
 *   uintptr_t key;
 *   upb_value val;
 *   while (upb_inttable_next2(t, &key, &val, &iter)) {
 *      // ...
 *   }
 */

#define UPB_INTTABLE_BEGIN -1

bool upb_inttable_next2(const upb_inttable* t, uintptr_t* key, upb_value* val,
                        intptr_t* iter);
void upb_inttable_removeiter(upb_inttable* t, intptr_t* iter);

/* Iteration over strtable.
 *
 *   intptr_t iter = UPB_INTTABLE_BEGIN;
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
 * Iterators for int and string tables.  We are subject to some kind of unusual
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

void upb_strtable_begin(upb_strtable_iter* i, const upb_strtable* t);
void upb_strtable_next(upb_strtable_iter* i);
bool upb_strtable_done(const upb_strtable_iter* i);
upb_StringView upb_strtable_iter_key(const upb_strtable_iter* i);
upb_value upb_strtable_iter_value(const upb_strtable_iter* i);
void upb_strtable_iter_setdone(upb_strtable_iter* i);
bool upb_strtable_iter_isequal(const upb_strtable_iter* i1,
                               const upb_strtable_iter* i2);

/* upb_inttable_iter **********************************************************/

/*   upb_inttable_iter i;
 *   upb_inttable_begin(&i, t);
 *   for(; !upb_inttable_done(&i); upb_inttable_next(&i)) {
 *     uintptr_t key = upb_inttable_iter_key(&i);
 *     upb_value val = upb_inttable_iter_value(&i);
 *     // ...
 *   }
 */

typedef struct {
  const upb_inttable* t;
  size_t index;
  bool array_part;
} upb_inttable_iter;

UPB_INLINE const upb_tabent* str_tabent(const upb_strtable_iter* i) {
  return &i->t->t.entries[i->index];
}

void upb_inttable_begin(upb_inttable_iter* i, const upb_inttable* t);
void upb_inttable_next(upb_inttable_iter* i);
bool upb_inttable_done(const upb_inttable_iter* i);
uintptr_t upb_inttable_iter_key(const upb_inttable_iter* i);
upb_value upb_inttable_iter_value(const upb_inttable_iter* i);
void upb_inttable_iter_setdone(upb_inttable_iter* i);
bool upb_inttable_iter_isequal(const upb_inttable_iter* i1,
                               const upb_inttable_iter* i2);

uint32_t _upb_Hash(const void* p, size_t n, uint64_t seed);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_TABLE_H_ */

/* Must be last. */

#ifdef __cplusplus
extern "C" {
#endif

/** upb_*Int* conversion routines ********************************************/

UPB_INLINE int32_t _upb_Int32_FromI(int v) { return (int32_t)v; }

UPB_INLINE int64_t _upb_Int64_FromLL(long long v) { return (int64_t)v; }

UPB_INLINE uint32_t _upb_UInt32_FromU(unsigned v) { return (uint32_t)v; }

UPB_INLINE uint64_t _upb_UInt64_FromULL(unsigned long long v) {
  return (uint64_t)v;
}

/** upb_MiniTable *************************************************************/

/* upb_MiniTable represents the memory layout of a given upb_MessageDef.  The
 * members are public so generated code can initialize them, but users MUST NOT
 * read or write any of its members. */

typedef struct {
  uint32_t number;
  uint16_t offset;
  int16_t presence;       // If >0, hasbit_index.  If <0, ~oneof_index
  uint16_t submsg_index;  // undefined if descriptortype != MESSAGE/GROUP/ENUM
  uint8_t descriptortype;
  uint8_t mode; /* upb_FieldMode | upb_LabelFlags |
                   (upb_FieldRep << kUpb_FieldRep_Shift) */
} upb_MiniTable_Field;

typedef enum {
  kUpb_FieldMode_Map = 0,
  kUpb_FieldMode_Array = 1,
  kUpb_FieldMode_Scalar = 2,

  kUpb_FieldMode_Mask = 3, /* Mask to isolate the mode from upb_FieldRep. */
} upb_FieldMode;

/* Extra flags on the mode field. */
typedef enum {
  kUpb_LabelFlags_IsPacked = 4,
  kUpb_LabelFlags_IsExtension = 8,
} upb_LabelFlags;

// Note: we sort by this number when calculating layout order.
typedef enum {
  kUpb_FieldRep_1Byte = 0,
  kUpb_FieldRep_4Byte = 1,
  kUpb_FieldRep_StringView = 2,
  kUpb_FieldRep_Pointer = 3,
  kUpb_FieldRep_8Byte = 4,

  kUpb_FieldRep_Shift = 5,  // Bit offset of the rep in upb_MiniTable_Field.mode
  kUpb_FieldRep_Max = kUpb_FieldRep_8Byte,
} upb_FieldRep;

UPB_INLINE upb_FieldMode upb_FieldMode_Get(const upb_MiniTable_Field* field) {
  return (upb_FieldMode)(field->mode & 3);
}

UPB_INLINE bool upb_IsRepeatedOrMap(const upb_MiniTable_Field* field) {
  /* This works because upb_FieldMode has no value 3. */
  return !(field->mode & kUpb_FieldMode_Scalar);
}

UPB_INLINE bool upb_IsSubMessage(const upb_MiniTable_Field* field) {
  return field->descriptortype == kUpb_FieldType_Message ||
         field->descriptortype == kUpb_FieldType_Group;
}

struct upb_Decoder;
struct upb_MiniTable;

typedef const char* _upb_FieldParser(struct upb_Decoder* d, const char* ptr,
                                     upb_Message* msg, intptr_t table,
                                     uint64_t hasbits, uint64_t data);

typedef struct {
  uint64_t field_data;
  _upb_FieldParser* field_parser;
} _upb_FastTable_Entry;

typedef struct {
  const int32_t* values;  // List of values <0 or >63
  uint64_t mask;          // Bits are set for acceptable value 0 <= x < 64
  int value_count;
} upb_MiniTable_Enum;

UPB_INLINE bool upb_MiniTable_Enum_CheckValue(const upb_MiniTable_Enum* e,
                                              int32_t val) {
  uint32_t uval = (uint32_t)val;
  if (uval < 64) return e->mask & (1 << uval);
  // OPT: binary search long lists?
  int n = e->value_count;
  for (int i = 0; i < n; i++) {
    if (e->values[i] == val) return true;
  }
  return false;
}

typedef union {
  const struct upb_MiniTable* submsg;
  const upb_MiniTable_Enum* subenum;
} upb_MiniTable_Sub;

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

/* MessageSet wire format is:
 *   message MessageSet {
 *     repeated group Item = 1 {
 *       required int32 type_id = 2;
 *       required string message = 3;
 *     }
 *   }
 */
typedef enum {
  _UPB_MSGSET_ITEM = 1,
  _UPB_MSGSET_TYPEID = 2,
  _UPB_MSGSET_MESSAGE = 3,
} upb_msgext_fieldnum;

struct upb_MiniTable {
  const upb_MiniTable_Sub* subs;
  const upb_MiniTable_Field* fields;
  /* Must be aligned to sizeof(void*).  Doesn't include internal members like
   * unknown fields, extension dict, pointer to msglayout, etc. */
  uint16_t size;
  uint16_t field_count;
  uint8_t ext;  // upb_ExtMode, declared as uint8_t so sizeof(ext) == 1
  uint8_t dense_below;
  uint8_t table_mask;
  uint8_t required_count;  // Required fields have the lowest hasbits.
  /* To statically initialize the tables of variable length, we need a flexible
   * array member, and we need to compile in gnu99 mode (constant initialization
   * of flexible array members is a GNU extension, not in C99 unfortunately. */
  _upb_FastTable_Entry fasttable[];
};

typedef struct {
  upb_MiniTable_Field field;
  const upb_MiniTable* extendee;
  upb_MiniTable_Sub sub; /* NULL unless submessage or proto2 enum */
} upb_MiniTable_Extension;

typedef struct {
  const upb_MiniTable** msgs;
  const upb_MiniTable_Enum** enums;
  const upb_MiniTable_Extension** exts;
  int msg_count;
  int enum_count;
  int ext_count;
} upb_MiniTable_File;

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

/** upb_ExtensionRegistry *****************************************************/

/* Adds the given extension info for message type |l| and field number |num|
 * into the registry. Returns false if this message type and field number were
 * already in the map, or if memory allocation fails. */
bool _upb_extreg_add(upb_ExtensionRegistry* r,
                     const upb_MiniTable_Extension** e, size_t count);

/* Looks up the extension (if any) defined for message type |l| and field
 * number |num|.  If an extension was found, copies the field info into |*ext|
 * and returns true. Otherwise returns false. */
const upb_MiniTable_Extension* _upb_extreg_get(const upb_ExtensionRegistry* r,
                                               const upb_MiniTable* l,
                                               uint32_t num);

/** upb_Message ***************************************************************/

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

UPB_INLINE size_t upb_msg_sizeof(const upb_MiniTable* l) {
  return l->size + sizeof(upb_Message_Internal);
}

UPB_INLINE upb_Message* _upb_Message_New_inl(const upb_MiniTable* l,
                                             upb_Arena* a) {
  size_t size = upb_msg_sizeof(l);
  void* mem = upb_Arena_Malloc(a, size);
  upb_Message* msg;
  if (UPB_UNLIKELY(!mem)) return NULL;
  msg = UPB_PTR_AT(mem, sizeof(upb_Message_Internal), upb_Message);
  memset(mem, 0, size);
  return msg;
}

/* Creates a new messages with the given layout on the given arena. */
upb_Message* _upb_Message_New(const upb_MiniTable* l, upb_Arena* a);

UPB_INLINE upb_Message_Internal* upb_Message_Getinternal(upb_Message* msg) {
  ptrdiff_t size = sizeof(upb_Message_Internal);
  return (upb_Message_Internal*)((char*)msg - size);
}

/* Clears the given message. */
void _upb_Message_Clear(upb_Message* msg, const upb_MiniTable* l);

/* Discards the unknown fields for this message only. */
void _upb_Message_DiscardUnknown_shallow(upb_Message* msg);

/* Adds unknown data (serialized protobuf data) to the given message.  The data
 * is copied into the message instance. */
bool _upb_Message_AddUnknown(upb_Message* msg, const char* data, size_t len,
                             upb_Arena* arena);

/** upb_Message_Extension *****************************************************/

/* The internal representation of an extension is self-describing: it contains
 * enough information that we can serialize it to binary format without needing
 * to look it up in a upb_ExtensionRegistry.
 *
 * This representation allocates 16 bytes to data on 64-bit platforms.  This is
 * rather wasteful for scalars (in the extreme case of bool, it wastes 15
 * bytes). We accept this because we expect messages to be the most common
 * extension type. */
typedef struct {
  const upb_MiniTable_Extension* ext;
  union {
    upb_StringView str;
    void* ptr;
    char scalar_data[8];
  } data;
} upb_Message_Extension;

/* Adds the given extension data to the given message. |ext| is copied into the
 * message instance. This logically replaces any previously-added extension with
 * this number */
upb_Message_Extension* _upb_Message_Getorcreateext(
    upb_Message* msg, const upb_MiniTable_Extension* ext, upb_Arena* arena);

/* Returns an array of extensions for this message. Note: the array is
 * ordered in reverse relative to the order of creation. */
const upb_Message_Extension* _upb_Message_Getexts(const upb_Message* msg,
                                                  size_t* count);

/* Returns an extension for the given field number, or NULL if no extension
 * exists for this field number. */
const upb_Message_Extension* _upb_Message_Getext(
    const upb_Message* msg, const upb_MiniTable_Extension* ext);

void _upb_Message_Clearext(upb_Message* msg,
                           const upb_MiniTable_Extension* ext);

void _upb_Message_Clearext(upb_Message* msg,
                           const upb_MiniTable_Extension* ext);

/** Hasbit access *************************************************************/

UPB_INLINE bool _upb_hasbit(const upb_Message* msg, size_t idx) {
  return (*UPB_PTR_AT(msg, idx / 8, const char) & (1 << (idx % 8))) != 0;
}

UPB_INLINE void _upb_sethas(const upb_Message* msg, size_t idx) {
  (*UPB_PTR_AT(msg, idx / 8, char)) |= (char)(1 << (idx % 8));
}

UPB_INLINE void _upb_clearhas(const upb_Message* msg, size_t idx) {
  (*UPB_PTR_AT(msg, idx / 8, char)) &= (char)(~(1 << (idx % 8)));
}

UPB_INLINE size_t _upb_Message_Hasidx(const upb_MiniTable_Field* f) {
  UPB_ASSERT(f->presence > 0);
  return f->presence;
}

UPB_INLINE bool _upb_hasbit_field(const upb_Message* msg,
                                  const upb_MiniTable_Field* f) {
  return _upb_hasbit(msg, _upb_Message_Hasidx(f));
}

UPB_INLINE void _upb_sethas_field(const upb_Message* msg,
                                  const upb_MiniTable_Field* f) {
  _upb_sethas(msg, _upb_Message_Hasidx(f));
}

UPB_INLINE void _upb_clearhas_field(const upb_Message* msg,
                                    const upb_MiniTable_Field* f) {
  _upb_clearhas(msg, _upb_Message_Hasidx(f));
}

/** Oneof case access *********************************************************/

UPB_INLINE uint32_t* _upb_oneofcase(upb_Message* msg, size_t case_ofs) {
  return UPB_PTR_AT(msg, case_ofs, uint32_t);
}

UPB_INLINE uint32_t _upb_getoneofcase(const void* msg, size_t case_ofs) {
  return *UPB_PTR_AT(msg, case_ofs, uint32_t);
}

UPB_INLINE size_t _upb_oneofcase_ofs(const upb_MiniTable_Field* f) {
  UPB_ASSERT(f->presence < 0);
  return ~(ptrdiff_t)f->presence;
}

UPB_INLINE uint32_t* _upb_oneofcase_field(upb_Message* msg,
                                          const upb_MiniTable_Field* f) {
  return _upb_oneofcase(msg, _upb_oneofcase_ofs(f));
}

UPB_INLINE uint32_t _upb_getoneofcase_field(const upb_Message* msg,
                                            const upb_MiniTable_Field* f) {
  return _upb_getoneofcase(msg, _upb_oneofcase_ofs(f));
}

UPB_INLINE bool _upb_has_submsg_nohasbit(const upb_Message* msg, size_t ofs) {
  return *UPB_PTR_AT(msg, ofs, const upb_Message*) != NULL;
}

/** upb_Array *****************************************************************/

/* Our internal representation for repeated fields.  */
typedef struct {
  uintptr_t data; /* Tagged ptr: low 3 bits of ptr are lg2(elem size). */
  size_t len;     /* Measured in elements. */
  size_t size;    /* Measured in elements. */
  uint64_t junk;
} upb_Array;

UPB_INLINE const void* _upb_array_constptr(const upb_Array* arr) {
  UPB_ASSERT((arr->data & 7) <= 4);
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

UPB_INLINE upb_Array* _upb_Array_New(upb_Arena* a, size_t init_size,
                                     int elem_size_lg2) {
  const size_t arr_size = UPB_ALIGN_UP(sizeof(upb_Array), 8);
  const size_t bytes = sizeof(upb_Array) + (init_size << elem_size_lg2);
  upb_Array* arr = (upb_Array*)upb_Arena_Malloc(a, bytes);
  if (!arr) return NULL;
  arr->data = _upb_tag_arrptr(UPB_PTR_AT(arr, arr_size, void), elem_size_lg2);
  arr->len = 0;
  arr->size = init_size;
  return arr;
}

/* Resizes the capacity of the array to be at least min_size. */
bool _upb_array_realloc(upb_Array* arr, size_t min_size, upb_Arena* arena);

/* Fallback functions for when the accessors require a resize. */
void* _upb_Array_Resize_fallback(upb_Array** arr_ptr, size_t size,
                                 int elem_size_lg2, upb_Arena* arena);
bool _upb_Array_Append_fallback(upb_Array** arr_ptr, const void* value,
                                int elem_size_lg2, upb_Arena* arena);

UPB_INLINE bool _upb_array_reserve(upb_Array* arr, size_t size,
                                   upb_Arena* arena) {
  if (arr->size < size) return _upb_array_realloc(arr, size, arena);
  return true;
}

UPB_INLINE bool _upb_Array_Resize(upb_Array* arr, size_t size,
                                  upb_Arena* arena) {
  if (!_upb_array_reserve(arr, size, arena)) return false;
  arr->len = size;
  return true;
}

UPB_INLINE const void* _upb_array_accessor(const void* msg, size_t ofs,
                                           size_t* size) {
  const upb_Array* arr = *UPB_PTR_AT(msg, ofs, const upb_Array*);
  if (arr) {
    if (size) *size = arr->len;
    return _upb_array_constptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}

UPB_INLINE void* _upb_array_mutable_accessor(void* msg, size_t ofs,
                                             size_t* size) {
  upb_Array* arr = *UPB_PTR_AT(msg, ofs, upb_Array*);
  if (arr) {
    if (size) *size = arr->len;
    return _upb_array_ptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}

UPB_INLINE void* _upb_Array_Resize_accessor2(void* msg, size_t ofs, size_t size,
                                             int elem_size_lg2,
                                             upb_Arena* arena) {
  upb_Array** arr_ptr = UPB_PTR_AT(msg, ofs, upb_Array*);
  upb_Array* arr = *arr_ptr;
  if (!arr || arr->size < size) {
    return _upb_Array_Resize_fallback(arr_ptr, size, elem_size_lg2, arena);
  }
  arr->len = size;
  return _upb_array_ptr(arr);
}

UPB_INLINE bool _upb_Array_Append_accessor2(void* msg, size_t ofs,
                                            int elem_size_lg2,
                                            const void* value,
                                            upb_Arena* arena) {
  upb_Array** arr_ptr = UPB_PTR_AT(msg, ofs, upb_Array*);
  size_t elem_size = 1 << elem_size_lg2;
  upb_Array* arr = *arr_ptr;
  void* ptr;
  if (!arr || arr->len == arr->size) {
    return _upb_Array_Append_fallback(arr_ptr, value, elem_size_lg2, arena);
  }
  ptr = _upb_array_ptr(arr);
  memcpy(UPB_PTR_AT(ptr, arr->len * elem_size, char), value, elem_size);
  arr->len++;
  return true;
}

/* Used by old generated code, remove once all code has been regenerated. */
UPB_INLINE int _upb_sizelg2(upb_CType type) {
  switch (type) {
    case kUpb_CType_Bool:
      return 0;
    case kUpb_CType_Float:
    case kUpb_CType_Int32:
    case kUpb_CType_UInt32:
    case kUpb_CType_Enum:
      return 2;
    case kUpb_CType_Message:
      return UPB_SIZE(2, 3);
    case kUpb_CType_Double:
    case kUpb_CType_Int64:
    case kUpb_CType_UInt64:
      return 3;
    case kUpb_CType_String:
    case kUpb_CType_Bytes:
      return UPB_SIZE(3, 4);
  }
  UPB_UNREACHABLE();
}
UPB_INLINE void* _upb_Array_Resize_accessor(void* msg, size_t ofs, size_t size,
                                            upb_CType type, upb_Arena* arena) {
  return _upb_Array_Resize_accessor2(msg, ofs, size, _upb_sizelg2(type), arena);
}
UPB_INLINE bool _upb_Array_Append_accessor(void* msg, size_t ofs,
                                           size_t elem_size, upb_CType type,
                                           const void* value,
                                           upb_Arena* arena) {
  (void)elem_size;
  return _upb_Array_Append_accessor2(msg, ofs, _upb_sizelg2(type), value,
                                     arena);
}

/** upb_Map *******************************************************************/

/* Right now we use strmaps for everything.  We'll likely want to use
 * integer-specific maps for integer-keyed maps.*/
typedef struct {
  /* Size of key and val, based on the map type.  Strings are represented as '0'
   * because they must be handled specially. */
  char key_size;
  char val_size;

  upb_strtable table;
} upb_Map;

/* Map entries aren't actually stored, they are only used during parsing.  For
 * parsing, it helps a lot if all map entry messages have the same layout.
 * The compiler and def.c must ensure that all map entries have this layout. */
typedef struct {
  upb_Message_Internal internal;
  union {
    upb_StringView str; /* For str/bytes. */
    upb_value val;      /* For all other types. */
  } k;
  union {
    upb_StringView str; /* For str/bytes. */
    upb_value val;      /* For all other types. */
  } v;
} upb_MapEntry;

/* Creates a new map on the given arena with this key/value type. */
upb_Map* _upb_Map_New(upb_Arena* a, size_t key_size, size_t value_size);

/* Converting between internal table representation and user values.
 *
 * _upb_map_tokey() and _upb_map_fromkey() are inverses.
 * _upb_map_tovalue() and _upb_map_fromvalue() are inverses.
 *
 * These functions account for the fact that strings are treated differently
 * from other types when stored in a map.
 */

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

/* Map operations, shared by reflection and generated code. */

UPB_INLINE size_t _upb_Map_Size(const upb_Map* map) {
  return map->table.t.count;
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

UPB_INLINE void* _upb_map_next(const upb_Map* map, size_t* iter) {
  upb_strtable_iter it;
  it.t = &map->table;
  it.index = *iter;
  upb_strtable_next(&it);
  *iter = it.index;
  if (upb_strtable_done(&it)) return NULL;
  return (void*)str_tabent(&it);
}

UPB_INLINE bool _upb_Map_Set(upb_Map* map, const void* key, size_t key_size,
                             void* val, size_t val_size, upb_Arena* a) {
  upb_StringView strkey = _upb_map_tokey(key, key_size);
  upb_value tabval = {0};
  if (!_upb_map_tovalue(val, val_size, &tabval, a)) return false;

  /* TODO(haberman): add overwrite operation to minimize number of lookups. */
  upb_strtable_remove2(&map->table, strkey.data, strkey.size, NULL);
  return upb_strtable_insert(&map->table, strkey.data, strkey.size, tabval, a);
}

UPB_INLINE bool _upb_Map_Delete(upb_Map* map, const void* key,
                                size_t key_size) {
  upb_StringView k = _upb_map_tokey(key, key_size);
  return upb_strtable_remove2(&map->table, k.data, k.size, NULL);
}

UPB_INLINE void _upb_Map_Clear(upb_Map* map) {
  upb_strtable_clear(&map->table);
}

/* Message map operations, these get the map from the message first. */

UPB_INLINE size_t _upb_msg_map_size(const upb_Message* msg, size_t ofs) {
  upb_Map* map = *UPB_PTR_AT(msg, ofs, upb_Map*);
  return map ? _upb_Map_Size(map) : 0;
}

UPB_INLINE bool _upb_msg_map_get(const upb_Message* msg, size_t ofs,
                                 const void* key, size_t key_size, void* val,
                                 size_t val_size) {
  upb_Map* map = *UPB_PTR_AT(msg, ofs, upb_Map*);
  if (!map) return false;
  return _upb_Map_Get(map, key, key_size, val, val_size);
}

UPB_INLINE void* _upb_msg_map_next(const upb_Message* msg, size_t ofs,
                                   size_t* iter) {
  upb_Map* map = *UPB_PTR_AT(msg, ofs, upb_Map*);
  if (!map) return NULL;
  return _upb_map_next(map, iter);
}

UPB_INLINE bool _upb_msg_map_set(upb_Message* msg, size_t ofs, const void* key,
                                 size_t key_size, void* val, size_t val_size,
                                 upb_Arena* arena) {
  upb_Map** map = UPB_PTR_AT(msg, ofs, upb_Map*);
  if (!*map) {
    *map = _upb_Map_New(arena, key_size, val_size);
  }
  return _upb_Map_Set(*map, key, key_size, val, val_size, arena);
}

UPB_INLINE bool _upb_msg_map_delete(upb_Message* msg, size_t ofs,
                                    const void* key, size_t key_size) {
  upb_Map* map = *UPB_PTR_AT(msg, ofs, upb_Map*);
  if (!map) return false;
  return _upb_Map_Delete(map, key, key_size);
}

UPB_INLINE void _upb_msg_map_clear(upb_Message* msg, size_t ofs) {
  upb_Map* map = *UPB_PTR_AT(msg, ofs, upb_Map*);
  if (!map) return;
  _upb_Map_Clear(map);
}

/* Accessing map key/value from a pointer, used by generated code only. */

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
  /* This is like _upb_map_tovalue() except the entry already exists so we can
   * reuse the allocated upb_StringView for string fields. */
  if (size == UPB_MAPTYPE_STRING) {
    upb_StringView* strp = (upb_StringView*)(uintptr_t)ent->val.val;
    memcpy(strp, val, sizeof(*strp));
  } else {
    memcpy(&ent->val.val, val, size);
  }
}

/** _upb_mapsorter ************************************************************/

/* _upb_mapsorter sorts maps and provides ordered iteration over the entries.
 * Since maps can be recursive (map values can be messages which contain other
 * maps). _upb_mapsorter can contain a stack of maps. */

typedef struct {
  upb_tabent const** entries;
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

bool _upb_mapsorter_pushmap(_upb_mapsorter* s, upb_FieldType key_type,
                            const upb_Map* map, _upb_sortedmap* sorted);

UPB_INLINE void _upb_mapsorter_popmap(_upb_mapsorter* s,
                                      _upb_sortedmap* sorted) {
  s->size = sorted->start;
}

UPB_INLINE bool _upb_sortedmap_next(_upb_mapsorter* s, const upb_Map* map,
                                    _upb_sortedmap* sorted, upb_MapEntry* ent) {
  if (sorted->pos == sorted->end) return false;
  const upb_tabent* tabent = s->entries[sorted->pos++];
  upb_StringView key = upb_tabstrview(tabent->key);
  _upb_map_fromkey(key, &ent->k, map->key_size);
  upb_value val = {tabent->val.val};
  _upb_map_fromvalue(val, &ent->v, map->val_size);
  return true;
}

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_MSG_INT_H_ */

/** upb/upb_internal.h ************************************************************/
#ifndef UPB_INT_H_
#define UPB_INT_H_


struct mem_block;
typedef struct mem_block mem_block;

struct upb_Arena {
  _upb_ArenaHead head;
  /* Stores cleanup metadata for this arena.
   * - a pointer to the current cleanup counter.
   * - a boolean indicating if there is an unowned initial block.  */
  uintptr_t cleanup_metadata;

  /* Allocator to allocate arena blocks.  We are responsible for freeing these
   * when we are destroyed. */
  upb_alloc* block_alloc;
  uint32_t last_size;

  /* When multiple arenas are fused together, each arena points to a parent
   * arena (root points to itself). The root tracks how many live arenas
   * reference it. */
  uint32_t refcount; /* Only used when a->parent == a */
  struct upb_Arena* parent;

  /* Linked list of blocks to free/cleanup. */
  mem_block *freelist, *freelist_tail;
};

// Encodes a float or double that is round-trippable, but as short as possible.
// These routines are not fully optimal (not guaranteed to be shortest), but are
// short-ish and match the implementation that has been used in protobuf since
// the beginning.
//
// The given buffer size must be at least kUpb_RoundTripBufferSize.
enum { kUpb_RoundTripBufferSize = 32 };
void _upb_EncodeRoundTripDouble(double val, char* buf, size_t size);
void _upb_EncodeRoundTripFloat(float val, char* buf, size_t size);

#endif /* UPB_INT_H_ */

/* Must be last. */

#define DECODE_NOGROUP (uint32_t) - 1

typedef struct upb_Decoder {
  const char* end;          /* Can read up to 16 bytes slop beyond this. */
  const char* limit_ptr;    /* = end + UPB_MIN(limit, 0) */
  upb_Message* unknown_msg; /* If non-NULL, add unknown data at buffer flip. */
  const char* unknown;      /* Start of unknown data. */
  const upb_ExtensionRegistry*
      extreg;         /* For looking up extensions during the parse. */
  int limit;          /* Submessage limit relative to end. */
  int depth;          /* Tracks recursion depth to bound stack usage. */
  uint32_t end_group; /* field number of END_GROUP tag, else DECODE_NOGROUP */
  uint16_t options;
  bool missing_required;
  char patch[32];
  upb_Arena arena;
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
const char* fastdecode_err(upb_Decoder* d, int status);

extern const uint8_t upb_utf8_offsets[];

UPB_INLINE
bool decode_verifyutf8_inl(const char* ptr, int len) {
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

const char* decode_checkrequired(upb_Decoder* d, const char* ptr,
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

UPB_INLINE
const char* decode_isdonefallback_inl(upb_Decoder* d, const char* ptr,
                                      int overrun, int* status) {
  if (overrun < d->limit) {
    /* Need to copy remaining data into patch buffer. */
    UPB_ASSERT(overrun < 16);
    if (d->unknown_msg) {
      if (!_upb_Message_AddUnknown(d->unknown_msg, d->unknown, ptr - d->unknown,
                                   &d->arena)) {
        *status = kUpb_DecodeStatus_OutOfMemory;
        return NULL;
      }
      d->unknown = &d->patch[0] + overrun;
    }
    memset(d->patch + 16, 0, 16);
    memcpy(d->patch, d->end, 16);
    ptr = &d->patch[0] + overrun;
    d->end = &d->patch[16];
    d->limit -= 16;
    d->limit_ptr = d->end + d->limit;
    d->options &= ~kUpb_DecodeOption_AliasString;
    UPB_ASSERT(ptr < d->limit_ptr);
    return ptr;
  } else {
    *status = kUpb_DecodeStatus_Malformed;
    return NULL;
  }
}

const char* decode_isdonefallback(upb_Decoder* d, const char* ptr, int overrun);

UPB_INLINE
bool decode_isdone(upb_Decoder* d, const char** ptr) {
  int overrun = *ptr - d->end;
  if (UPB_LIKELY(*ptr < d->limit_ptr)) {
    return false;
  } else if (UPB_LIKELY(overrun == d->limit)) {
    return true;
  } else {
    *ptr = decode_isdonefallback(d, *ptr, overrun);
    return false;
  }
}

#if UPB_FASTTABLE
UPB_INLINE
const char* fastdecode_tagdispatch(upb_Decoder* d, const char* ptr,
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

UPB_INLINE uint32_t fastdecode_loadtag(const char* ptr) {
  uint16_t tag;
  memcpy(&tag, ptr, 2);
  return tag;
}

UPB_INLINE void decode_checklimit(upb_Decoder* d) {
  UPB_ASSERT(d->limit_ptr == d->end + UPB_MIN(0, d->limit));
}

UPB_INLINE int decode_pushlimit(upb_Decoder* d, const char* ptr, int size) {
  int limit = size + (int)(ptr - d->end);
  int delta = d->limit - limit;
  decode_checklimit(d);
  d->limit = limit;
  d->limit_ptr = d->end + UPB_MIN(0, limit);
  decode_checklimit(d);
  return delta;
}

UPB_INLINE void decode_poplimit(upb_Decoder* d, const char* ptr,
                                int saved_delta) {
  UPB_ASSERT(ptr - d->end == d->limit);
  decode_checklimit(d);
  d->limit += saved_delta;
  d->limit_ptr = d->end + UPB_MIN(0, d->limit);
  decode_checklimit(d);
}


#endif /* UPB_DECODE_INT_H_ */

/** upb/encode.h ************************************************************/
/*
 * upb_Encode: parsing into a upb_Message using a upb_MiniTable.
 */

#ifndef UPB_ENCODE_H_
#define UPB_ENCODE_H_


/* Must be last. */

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
  kUpb_Encode_Deterministic = 1,

  /* When set, unknown fields are not printed. */
  kUpb_Encode_SkipUnknown = 2,

  /* When set, the encode will fail if any required fields are missing. */
  kUpb_Encode_CheckRequired = 4,
};

#define UPB_ENCODE_MAXDEPTH(depth) ((depth) << 16)

char* upb_Encode(const void* msg, const upb_MiniTable* l, int options,
                 upb_Arena* arena, size_t* size);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* UPB_ENCODE_H_ */

/** upb/decode_fast.h ************************************************************/
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

#ifndef UPB_DECODE_FAST_H_
#define UPB_DECODE_FAST_H_


struct upb_Decoder;

// The fallback, generic parsing function that can handle any field type.
// This just uses the regular (non-fast) parser to parse a single field.
const char* fastdecode_generic(struct upb_Decoder* d, const char* ptr,
                               upb_Message* msg, intptr_t table,
                               uint64_t hasbits, uint64_t data);

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

#endif /* UPB_DECODE_FAST_H_ */

/** google/protobuf/descriptor.upb.h ************************************************************//* This file was generated by upbc (the upb compiler) from the input
 * file:
 *
 *     google/protobuf/descriptor.proto
 *
 * Do not edit -- your changes will be discarded when the file is
 * regenerated. */

#ifndef GOOGLE_PROTOBUF_DESCRIPTOR_PROTO_UPB_H_
#define GOOGLE_PROTOBUF_DESCRIPTOR_PROTO_UPB_H_



#ifdef __cplusplus
extern "C" {
#endif

struct google_protobuf_FileDescriptorSet;
struct google_protobuf_FileDescriptorProto;
struct google_protobuf_DescriptorProto;
struct google_protobuf_DescriptorProto_ExtensionRange;
struct google_protobuf_DescriptorProto_ReservedRange;
struct google_protobuf_ExtensionRangeOptions;
struct google_protobuf_FieldDescriptorProto;
struct google_protobuf_OneofDescriptorProto;
struct google_protobuf_EnumDescriptorProto;
struct google_protobuf_EnumDescriptorProto_EnumReservedRange;
struct google_protobuf_EnumValueDescriptorProto;
struct google_protobuf_ServiceDescriptorProto;
struct google_protobuf_MethodDescriptorProto;
struct google_protobuf_FileOptions;
struct google_protobuf_MessageOptions;
struct google_protobuf_FieldOptions;
struct google_protobuf_OneofOptions;
struct google_protobuf_EnumOptions;
struct google_protobuf_EnumValueOptions;
struct google_protobuf_ServiceOptions;
struct google_protobuf_MethodOptions;
struct google_protobuf_UninterpretedOption;
struct google_protobuf_UninterpretedOption_NamePart;
struct google_protobuf_SourceCodeInfo;
struct google_protobuf_SourceCodeInfo_Location;
struct google_protobuf_GeneratedCodeInfo;
struct google_protobuf_GeneratedCodeInfo_Annotation;
typedef struct google_protobuf_FileDescriptorSet google_protobuf_FileDescriptorSet;
typedef struct google_protobuf_FileDescriptorProto google_protobuf_FileDescriptorProto;
typedef struct google_protobuf_DescriptorProto google_protobuf_DescriptorProto;
typedef struct google_protobuf_DescriptorProto_ExtensionRange google_protobuf_DescriptorProto_ExtensionRange;
typedef struct google_protobuf_DescriptorProto_ReservedRange google_protobuf_DescriptorProto_ReservedRange;
typedef struct google_protobuf_ExtensionRangeOptions google_protobuf_ExtensionRangeOptions;
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
extern const upb_MiniTable google_protobuf_FileDescriptorSet_msginit;
extern const upb_MiniTable google_protobuf_FileDescriptorProto_msginit;
extern const upb_MiniTable google_protobuf_DescriptorProto_msginit;
extern const upb_MiniTable google_protobuf_DescriptorProto_ExtensionRange_msginit;
extern const upb_MiniTable google_protobuf_DescriptorProto_ReservedRange_msginit;
extern const upb_MiniTable google_protobuf_ExtensionRangeOptions_msginit;
extern const upb_MiniTable google_protobuf_FieldDescriptorProto_msginit;
extern const upb_MiniTable google_protobuf_OneofDescriptorProto_msginit;
extern const upb_MiniTable google_protobuf_EnumDescriptorProto_msginit;
extern const upb_MiniTable google_protobuf_EnumDescriptorProto_EnumReservedRange_msginit;
extern const upb_MiniTable google_protobuf_EnumValueDescriptorProto_msginit;
extern const upb_MiniTable google_protobuf_ServiceDescriptorProto_msginit;
extern const upb_MiniTable google_protobuf_MethodDescriptorProto_msginit;
extern const upb_MiniTable google_protobuf_FileOptions_msginit;
extern const upb_MiniTable google_protobuf_MessageOptions_msginit;
extern const upb_MiniTable google_protobuf_FieldOptions_msginit;
extern const upb_MiniTable google_protobuf_OneofOptions_msginit;
extern const upb_MiniTable google_protobuf_EnumOptions_msginit;
extern const upb_MiniTable google_protobuf_EnumValueOptions_msginit;
extern const upb_MiniTable google_protobuf_ServiceOptions_msginit;
extern const upb_MiniTable google_protobuf_MethodOptions_msginit;
extern const upb_MiniTable google_protobuf_UninterpretedOption_msginit;
extern const upb_MiniTable google_protobuf_UninterpretedOption_NamePart_msginit;
extern const upb_MiniTable google_protobuf_SourceCodeInfo_msginit;
extern const upb_MiniTable google_protobuf_SourceCodeInfo_Location_msginit;
extern const upb_MiniTable google_protobuf_GeneratedCodeInfo_msginit;
extern const upb_MiniTable google_protobuf_GeneratedCodeInfo_Annotation_msginit;

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
  google_protobuf_FileOptions_SPEED = 1,
  google_protobuf_FileOptions_CODE_SIZE = 2,
  google_protobuf_FileOptions_LITE_RUNTIME = 3
} google_protobuf_FileOptions_OptimizeMode;

typedef enum {
  google_protobuf_MethodOptions_IDEMPOTENCY_UNKNOWN = 0,
  google_protobuf_MethodOptions_NO_SIDE_EFFECTS = 1,
  google_protobuf_MethodOptions_IDEMPOTENT = 2
} google_protobuf_MethodOptions_IdempotencyLevel;


extern const upb_MiniTable_Enum google_protobuf_FieldDescriptorProto_Label_enuminit;
extern const upb_MiniTable_Enum google_protobuf_FieldDescriptorProto_Type_enuminit;
extern const upb_MiniTable_Enum google_protobuf_FieldOptions_CType_enuminit;
extern const upb_MiniTable_Enum google_protobuf_FieldOptions_JSType_enuminit;
extern const upb_MiniTable_Enum google_protobuf_FileOptions_OptimizeMode_enuminit;
extern const upb_MiniTable_Enum google_protobuf_MethodOptions_IdempotencyLevel_enuminit;

/* google.protobuf.FileDescriptorSet */

UPB_INLINE google_protobuf_FileDescriptorSet* google_protobuf_FileDescriptorSet_new(upb_Arena* arena) {
  return (google_protobuf_FileDescriptorSet*)_upb_Message_New(&google_protobuf_FileDescriptorSet_msginit, arena);
}
UPB_INLINE google_protobuf_FileDescriptorSet* google_protobuf_FileDescriptorSet_parse(const char* buf, size_t size, upb_Arena* arena) {
  google_protobuf_FileDescriptorSet* ret = google_protobuf_FileDescriptorSet_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_FileDescriptorSet_msginit, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE google_protobuf_FileDescriptorSet* google_protobuf_FileDescriptorSet_parse_ex(const char* buf, size_t size,
                           const upb_ExtensionRegistry* extreg,
                           int options, upb_Arena* arena) {
  google_protobuf_FileDescriptorSet* ret = google_protobuf_FileDescriptorSet_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_FileDescriptorSet_msginit, extreg, options, arena) !=
      kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE char* google_protobuf_FileDescriptorSet_serialize(const google_protobuf_FileDescriptorSet* msg, upb_Arena* arena, size_t* len) {
  return upb_Encode(msg, &google_protobuf_FileDescriptorSet_msginit, 0, arena, len);
}
UPB_INLINE char* google_protobuf_FileDescriptorSet_serialize_ex(const google_protobuf_FileDescriptorSet* msg, int options,
                                 upb_Arena* arena, size_t* len) {
  return upb_Encode(msg, &google_protobuf_FileDescriptorSet_msginit, options, arena, len);
}
UPB_INLINE bool google_protobuf_FileDescriptorSet_has_file(const google_protobuf_FileDescriptorSet* msg) {
  return _upb_has_submsg_nohasbit(msg, UPB_SIZE(0, 0));
}
UPB_INLINE const google_protobuf_FileDescriptorProto* const* google_protobuf_FileDescriptorSet_file(const google_protobuf_FileDescriptorSet* msg, size_t* len) {
  return (const google_protobuf_FileDescriptorProto* const*)_upb_array_accessor(msg, UPB_SIZE(0, 0), len);
}

UPB_INLINE google_protobuf_FileDescriptorProto** google_protobuf_FileDescriptorSet_mutable_file(google_protobuf_FileDescriptorSet* msg, size_t* len) {
  return (google_protobuf_FileDescriptorProto**)_upb_array_mutable_accessor(msg, UPB_SIZE(0, 0), len);
}
UPB_INLINE google_protobuf_FileDescriptorProto** google_protobuf_FileDescriptorSet_resize_file(google_protobuf_FileDescriptorSet* msg, size_t len, upb_Arena* arena) {
  return (google_protobuf_FileDescriptorProto**)_upb_Array_Resize_accessor2(msg, UPB_SIZE(0, 0), len, UPB_SIZE(2, 3), arena);
}
UPB_INLINE struct google_protobuf_FileDescriptorProto* google_protobuf_FileDescriptorSet_add_file(google_protobuf_FileDescriptorSet* msg, upb_Arena* arena) {
  struct google_protobuf_FileDescriptorProto* sub = (struct google_protobuf_FileDescriptorProto*)_upb_Message_New(&google_protobuf_FileDescriptorProto_msginit, arena);
  bool ok = _upb_Array_Append_accessor2(msg, UPB_SIZE(0, 0), UPB_SIZE(2, 3), &sub, arena);
  if (!ok) return NULL;
  return sub;
}

/* google.protobuf.FileDescriptorProto */

UPB_INLINE google_protobuf_FileDescriptorProto* google_protobuf_FileDescriptorProto_new(upb_Arena* arena) {
  return (google_protobuf_FileDescriptorProto*)_upb_Message_New(&google_protobuf_FileDescriptorProto_msginit, arena);
}
UPB_INLINE google_protobuf_FileDescriptorProto* google_protobuf_FileDescriptorProto_parse(const char* buf, size_t size, upb_Arena* arena) {
  google_protobuf_FileDescriptorProto* ret = google_protobuf_FileDescriptorProto_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_FileDescriptorProto_msginit, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE google_protobuf_FileDescriptorProto* google_protobuf_FileDescriptorProto_parse_ex(const char* buf, size_t size,
                           const upb_ExtensionRegistry* extreg,
                           int options, upb_Arena* arena) {
  google_protobuf_FileDescriptorProto* ret = google_protobuf_FileDescriptorProto_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_FileDescriptorProto_msginit, extreg, options, arena) !=
      kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE char* google_protobuf_FileDescriptorProto_serialize(const google_protobuf_FileDescriptorProto* msg, upb_Arena* arena, size_t* len) {
  return upb_Encode(msg, &google_protobuf_FileDescriptorProto_msginit, 0, arena, len);
}
UPB_INLINE char* google_protobuf_FileDescriptorProto_serialize_ex(const google_protobuf_FileDescriptorProto* msg, int options,
                                 upb_Arena* arena, size_t* len) {
  return upb_Encode(msg, &google_protobuf_FileDescriptorProto_msginit, options, arena, len);
}
UPB_INLINE bool google_protobuf_FileDescriptorProto_has_name(const google_protobuf_FileDescriptorProto* msg) {
  return _upb_hasbit(msg, 1);
}
UPB_INLINE upb_StringView google_protobuf_FileDescriptorProto_name(const google_protobuf_FileDescriptorProto* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(4, 8), upb_StringView);
}
UPB_INLINE bool google_protobuf_FileDescriptorProto_has_package(const google_protobuf_FileDescriptorProto* msg) {
  return _upb_hasbit(msg, 2);
}
UPB_INLINE upb_StringView google_protobuf_FileDescriptorProto_package(const google_protobuf_FileDescriptorProto* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(12, 24), upb_StringView);
}
UPB_INLINE upb_StringView const* google_protobuf_FileDescriptorProto_dependency(const google_protobuf_FileDescriptorProto* msg, size_t* len) {
  return (upb_StringView const*)_upb_array_accessor(msg, UPB_SIZE(36, 72), len);
}
UPB_INLINE bool google_protobuf_FileDescriptorProto_has_message_type(const google_protobuf_FileDescriptorProto* msg) {
  return _upb_has_submsg_nohasbit(msg, UPB_SIZE(40, 80));
}
UPB_INLINE const google_protobuf_DescriptorProto* const* google_protobuf_FileDescriptorProto_message_type(const google_protobuf_FileDescriptorProto* msg, size_t* len) {
  return (const google_protobuf_DescriptorProto* const*)_upb_array_accessor(msg, UPB_SIZE(40, 80), len);
}
UPB_INLINE bool google_protobuf_FileDescriptorProto_has_enum_type(const google_protobuf_FileDescriptorProto* msg) {
  return _upb_has_submsg_nohasbit(msg, UPB_SIZE(44, 88));
}
UPB_INLINE const google_protobuf_EnumDescriptorProto* const* google_protobuf_FileDescriptorProto_enum_type(const google_protobuf_FileDescriptorProto* msg, size_t* len) {
  return (const google_protobuf_EnumDescriptorProto* const*)_upb_array_accessor(msg, UPB_SIZE(44, 88), len);
}
UPB_INLINE bool google_protobuf_FileDescriptorProto_has_service(const google_protobuf_FileDescriptorProto* msg) {
  return _upb_has_submsg_nohasbit(msg, UPB_SIZE(48, 96));
}
UPB_INLINE const google_protobuf_ServiceDescriptorProto* const* google_protobuf_FileDescriptorProto_service(const google_protobuf_FileDescriptorProto* msg, size_t* len) {
  return (const google_protobuf_ServiceDescriptorProto* const*)_upb_array_accessor(msg, UPB_SIZE(48, 96), len);
}
UPB_INLINE bool google_protobuf_FileDescriptorProto_has_extension(const google_protobuf_FileDescriptorProto* msg) {
  return _upb_has_submsg_nohasbit(msg, UPB_SIZE(52, 104));
}
UPB_INLINE const google_protobuf_FieldDescriptorProto* const* google_protobuf_FileDescriptorProto_extension(const google_protobuf_FileDescriptorProto* msg, size_t* len) {
  return (const google_protobuf_FieldDescriptorProto* const*)_upb_array_accessor(msg, UPB_SIZE(52, 104), len);
}
UPB_INLINE bool google_protobuf_FileDescriptorProto_has_options(const google_protobuf_FileDescriptorProto* msg) {
  return _upb_hasbit(msg, 3);
}
UPB_INLINE const google_protobuf_FileOptions* google_protobuf_FileDescriptorProto_options(const google_protobuf_FileDescriptorProto* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(28, 56), const google_protobuf_FileOptions*);
}
UPB_INLINE bool google_protobuf_FileDescriptorProto_has_source_code_info(const google_protobuf_FileDescriptorProto* msg) {
  return _upb_hasbit(msg, 4);
}
UPB_INLINE const google_protobuf_SourceCodeInfo* google_protobuf_FileDescriptorProto_source_code_info(const google_protobuf_FileDescriptorProto* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(32, 64), const google_protobuf_SourceCodeInfo*);
}
UPB_INLINE int32_t const* google_protobuf_FileDescriptorProto_public_dependency(const google_protobuf_FileDescriptorProto* msg, size_t* len) {
  return (int32_t const*)_upb_array_accessor(msg, UPB_SIZE(56, 112), len);
}
UPB_INLINE int32_t const* google_protobuf_FileDescriptorProto_weak_dependency(const google_protobuf_FileDescriptorProto* msg, size_t* len) {
  return (int32_t const*)_upb_array_accessor(msg, UPB_SIZE(60, 120), len);
}
UPB_INLINE bool google_protobuf_FileDescriptorProto_has_syntax(const google_protobuf_FileDescriptorProto* msg) {
  return _upb_hasbit(msg, 5);
}
UPB_INLINE upb_StringView google_protobuf_FileDescriptorProto_syntax(const google_protobuf_FileDescriptorProto* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(20, 40), upb_StringView);
}

UPB_INLINE void google_protobuf_FileDescriptorProto_set_name(google_protobuf_FileDescriptorProto *msg, upb_StringView value) {
  _upb_sethas(msg, 1);
  *UPB_PTR_AT(msg, UPB_SIZE(4, 8), upb_StringView) = value;
}
UPB_INLINE void google_protobuf_FileDescriptorProto_set_package(google_protobuf_FileDescriptorProto *msg, upb_StringView value) {
  _upb_sethas(msg, 2);
  *UPB_PTR_AT(msg, UPB_SIZE(12, 24), upb_StringView) = value;
}
UPB_INLINE upb_StringView* google_protobuf_FileDescriptorProto_mutable_dependency(google_protobuf_FileDescriptorProto* msg, size_t* len) {
  return (upb_StringView*)_upb_array_mutable_accessor(msg, UPB_SIZE(36, 72), len);
}
UPB_INLINE upb_StringView* google_protobuf_FileDescriptorProto_resize_dependency(google_protobuf_FileDescriptorProto* msg, size_t len, upb_Arena* arena) {
  return (upb_StringView*)_upb_Array_Resize_accessor2(msg, UPB_SIZE(36, 72), len, UPB_SIZE(3, 4), arena);
}
UPB_INLINE bool google_protobuf_FileDescriptorProto_add_dependency(google_protobuf_FileDescriptorProto* msg, upb_StringView val, upb_Arena* arena) {
  return _upb_Array_Append_accessor2(msg, UPB_SIZE(36, 72), UPB_SIZE(3, 4), &val, arena);
}
UPB_INLINE google_protobuf_DescriptorProto** google_protobuf_FileDescriptorProto_mutable_message_type(google_protobuf_FileDescriptorProto* msg, size_t* len) {
  return (google_protobuf_DescriptorProto**)_upb_array_mutable_accessor(msg, UPB_SIZE(40, 80), len);
}
UPB_INLINE google_protobuf_DescriptorProto** google_protobuf_FileDescriptorProto_resize_message_type(google_protobuf_FileDescriptorProto* msg, size_t len, upb_Arena* arena) {
  return (google_protobuf_DescriptorProto**)_upb_Array_Resize_accessor2(msg, UPB_SIZE(40, 80), len, UPB_SIZE(2, 3), arena);
}
UPB_INLINE struct google_protobuf_DescriptorProto* google_protobuf_FileDescriptorProto_add_message_type(google_protobuf_FileDescriptorProto* msg, upb_Arena* arena) {
  struct google_protobuf_DescriptorProto* sub = (struct google_protobuf_DescriptorProto*)_upb_Message_New(&google_protobuf_DescriptorProto_msginit, arena);
  bool ok = _upb_Array_Append_accessor2(msg, UPB_SIZE(40, 80), UPB_SIZE(2, 3), &sub, arena);
  if (!ok) return NULL;
  return sub;
}
UPB_INLINE google_protobuf_EnumDescriptorProto** google_protobuf_FileDescriptorProto_mutable_enum_type(google_protobuf_FileDescriptorProto* msg, size_t* len) {
  return (google_protobuf_EnumDescriptorProto**)_upb_array_mutable_accessor(msg, UPB_SIZE(44, 88), len);
}
UPB_INLINE google_protobuf_EnumDescriptorProto** google_protobuf_FileDescriptorProto_resize_enum_type(google_protobuf_FileDescriptorProto* msg, size_t len, upb_Arena* arena) {
  return (google_protobuf_EnumDescriptorProto**)_upb_Array_Resize_accessor2(msg, UPB_SIZE(44, 88), len, UPB_SIZE(2, 3), arena);
}
UPB_INLINE struct google_protobuf_EnumDescriptorProto* google_protobuf_FileDescriptorProto_add_enum_type(google_protobuf_FileDescriptorProto* msg, upb_Arena* arena) {
  struct google_protobuf_EnumDescriptorProto* sub = (struct google_protobuf_EnumDescriptorProto*)_upb_Message_New(&google_protobuf_EnumDescriptorProto_msginit, arena);
  bool ok = _upb_Array_Append_accessor2(msg, UPB_SIZE(44, 88), UPB_SIZE(2, 3), &sub, arena);
  if (!ok) return NULL;
  return sub;
}
UPB_INLINE google_protobuf_ServiceDescriptorProto** google_protobuf_FileDescriptorProto_mutable_service(google_protobuf_FileDescriptorProto* msg, size_t* len) {
  return (google_protobuf_ServiceDescriptorProto**)_upb_array_mutable_accessor(msg, UPB_SIZE(48, 96), len);
}
UPB_INLINE google_protobuf_ServiceDescriptorProto** google_protobuf_FileDescriptorProto_resize_service(google_protobuf_FileDescriptorProto* msg, size_t len, upb_Arena* arena) {
  return (google_protobuf_ServiceDescriptorProto**)_upb_Array_Resize_accessor2(msg, UPB_SIZE(48, 96), len, UPB_SIZE(2, 3), arena);
}
UPB_INLINE struct google_protobuf_ServiceDescriptorProto* google_protobuf_FileDescriptorProto_add_service(google_protobuf_FileDescriptorProto* msg, upb_Arena* arena) {
  struct google_protobuf_ServiceDescriptorProto* sub = (struct google_protobuf_ServiceDescriptorProto*)_upb_Message_New(&google_protobuf_ServiceDescriptorProto_msginit, arena);
  bool ok = _upb_Array_Append_accessor2(msg, UPB_SIZE(48, 96), UPB_SIZE(2, 3), &sub, arena);
  if (!ok) return NULL;
  return sub;
}
UPB_INLINE google_protobuf_FieldDescriptorProto** google_protobuf_FileDescriptorProto_mutable_extension(google_protobuf_FileDescriptorProto* msg, size_t* len) {
  return (google_protobuf_FieldDescriptorProto**)_upb_array_mutable_accessor(msg, UPB_SIZE(52, 104), len);
}
UPB_INLINE google_protobuf_FieldDescriptorProto** google_protobuf_FileDescriptorProto_resize_extension(google_protobuf_FileDescriptorProto* msg, size_t len, upb_Arena* arena) {
  return (google_protobuf_FieldDescriptorProto**)_upb_Array_Resize_accessor2(msg, UPB_SIZE(52, 104), len, UPB_SIZE(2, 3), arena);
}
UPB_INLINE struct google_protobuf_FieldDescriptorProto* google_protobuf_FileDescriptorProto_add_extension(google_protobuf_FileDescriptorProto* msg, upb_Arena* arena) {
  struct google_protobuf_FieldDescriptorProto* sub = (struct google_protobuf_FieldDescriptorProto*)_upb_Message_New(&google_protobuf_FieldDescriptorProto_msginit, arena);
  bool ok = _upb_Array_Append_accessor2(msg, UPB_SIZE(52, 104), UPB_SIZE(2, 3), &sub, arena);
  if (!ok) return NULL;
  return sub;
}
UPB_INLINE void google_protobuf_FileDescriptorProto_set_options(google_protobuf_FileDescriptorProto *msg, google_protobuf_FileOptions* value) {
  _upb_sethas(msg, 3);
  *UPB_PTR_AT(msg, UPB_SIZE(28, 56), google_protobuf_FileOptions*) = value;
}
UPB_INLINE struct google_protobuf_FileOptions* google_protobuf_FileDescriptorProto_mutable_options(google_protobuf_FileDescriptorProto* msg, upb_Arena* arena) {
  struct google_protobuf_FileOptions* sub = (struct google_protobuf_FileOptions*)google_protobuf_FileDescriptorProto_options(msg);
  if (sub == NULL) {
    sub = (struct google_protobuf_FileOptions*)_upb_Message_New(&google_protobuf_FileOptions_msginit, arena);
    if (!sub) return NULL;
    google_protobuf_FileDescriptorProto_set_options(msg, sub);
  }
  return sub;
}
UPB_INLINE void google_protobuf_FileDescriptorProto_set_source_code_info(google_protobuf_FileDescriptorProto *msg, google_protobuf_SourceCodeInfo* value) {
  _upb_sethas(msg, 4);
  *UPB_PTR_AT(msg, UPB_SIZE(32, 64), google_protobuf_SourceCodeInfo*) = value;
}
UPB_INLINE struct google_protobuf_SourceCodeInfo* google_protobuf_FileDescriptorProto_mutable_source_code_info(google_protobuf_FileDescriptorProto* msg, upb_Arena* arena) {
  struct google_protobuf_SourceCodeInfo* sub = (struct google_protobuf_SourceCodeInfo*)google_protobuf_FileDescriptorProto_source_code_info(msg);
  if (sub == NULL) {
    sub = (struct google_protobuf_SourceCodeInfo*)_upb_Message_New(&google_protobuf_SourceCodeInfo_msginit, arena);
    if (!sub) return NULL;
    google_protobuf_FileDescriptorProto_set_source_code_info(msg, sub);
  }
  return sub;
}
UPB_INLINE int32_t* google_protobuf_FileDescriptorProto_mutable_public_dependency(google_protobuf_FileDescriptorProto* msg, size_t* len) {
  return (int32_t*)_upb_array_mutable_accessor(msg, UPB_SIZE(56, 112), len);
}
UPB_INLINE int32_t* google_protobuf_FileDescriptorProto_resize_public_dependency(google_protobuf_FileDescriptorProto* msg, size_t len, upb_Arena* arena) {
  return (int32_t*)_upb_Array_Resize_accessor2(msg, UPB_SIZE(56, 112), len, 2, arena);
}
UPB_INLINE bool google_protobuf_FileDescriptorProto_add_public_dependency(google_protobuf_FileDescriptorProto* msg, int32_t val, upb_Arena* arena) {
  return _upb_Array_Append_accessor2(msg, UPB_SIZE(56, 112), 2, &val, arena);
}
UPB_INLINE int32_t* google_protobuf_FileDescriptorProto_mutable_weak_dependency(google_protobuf_FileDescriptorProto* msg, size_t* len) {
  return (int32_t*)_upb_array_mutable_accessor(msg, UPB_SIZE(60, 120), len);
}
UPB_INLINE int32_t* google_protobuf_FileDescriptorProto_resize_weak_dependency(google_protobuf_FileDescriptorProto* msg, size_t len, upb_Arena* arena) {
  return (int32_t*)_upb_Array_Resize_accessor2(msg, UPB_SIZE(60, 120), len, 2, arena);
}
UPB_INLINE bool google_protobuf_FileDescriptorProto_add_weak_dependency(google_protobuf_FileDescriptorProto* msg, int32_t val, upb_Arena* arena) {
  return _upb_Array_Append_accessor2(msg, UPB_SIZE(60, 120), 2, &val, arena);
}
UPB_INLINE void google_protobuf_FileDescriptorProto_set_syntax(google_protobuf_FileDescriptorProto *msg, upb_StringView value) {
  _upb_sethas(msg, 5);
  *UPB_PTR_AT(msg, UPB_SIZE(20, 40), upb_StringView) = value;
}

/* google.protobuf.DescriptorProto */

UPB_INLINE google_protobuf_DescriptorProto* google_protobuf_DescriptorProto_new(upb_Arena* arena) {
  return (google_protobuf_DescriptorProto*)_upb_Message_New(&google_protobuf_DescriptorProto_msginit, arena);
}
UPB_INLINE google_protobuf_DescriptorProto* google_protobuf_DescriptorProto_parse(const char* buf, size_t size, upb_Arena* arena) {
  google_protobuf_DescriptorProto* ret = google_protobuf_DescriptorProto_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_DescriptorProto_msginit, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE google_protobuf_DescriptorProto* google_protobuf_DescriptorProto_parse_ex(const char* buf, size_t size,
                           const upb_ExtensionRegistry* extreg,
                           int options, upb_Arena* arena) {
  google_protobuf_DescriptorProto* ret = google_protobuf_DescriptorProto_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_DescriptorProto_msginit, extreg, options, arena) !=
      kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE char* google_protobuf_DescriptorProto_serialize(const google_protobuf_DescriptorProto* msg, upb_Arena* arena, size_t* len) {
  return upb_Encode(msg, &google_protobuf_DescriptorProto_msginit, 0, arena, len);
}
UPB_INLINE char* google_protobuf_DescriptorProto_serialize_ex(const google_protobuf_DescriptorProto* msg, int options,
                                 upb_Arena* arena, size_t* len) {
  return upb_Encode(msg, &google_protobuf_DescriptorProto_msginit, options, arena, len);
}
UPB_INLINE bool google_protobuf_DescriptorProto_has_name(const google_protobuf_DescriptorProto* msg) {
  return _upb_hasbit(msg, 1);
}
UPB_INLINE upb_StringView google_protobuf_DescriptorProto_name(const google_protobuf_DescriptorProto* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(4, 8), upb_StringView);
}
UPB_INLINE bool google_protobuf_DescriptorProto_has_field(const google_protobuf_DescriptorProto* msg) {
  return _upb_has_submsg_nohasbit(msg, UPB_SIZE(16, 32));
}
UPB_INLINE const google_protobuf_FieldDescriptorProto* const* google_protobuf_DescriptorProto_field(const google_protobuf_DescriptorProto* msg, size_t* len) {
  return (const google_protobuf_FieldDescriptorProto* const*)_upb_array_accessor(msg, UPB_SIZE(16, 32), len);
}
UPB_INLINE bool google_protobuf_DescriptorProto_has_nested_type(const google_protobuf_DescriptorProto* msg) {
  return _upb_has_submsg_nohasbit(msg, UPB_SIZE(20, 40));
}
UPB_INLINE const google_protobuf_DescriptorProto* const* google_protobuf_DescriptorProto_nested_type(const google_protobuf_DescriptorProto* msg, size_t* len) {
  return (const google_protobuf_DescriptorProto* const*)_upb_array_accessor(msg, UPB_SIZE(20, 40), len);
}
UPB_INLINE bool google_protobuf_DescriptorProto_has_enum_type(const google_protobuf_DescriptorProto* msg) {
  return _upb_has_submsg_nohasbit(msg, UPB_SIZE(24, 48));
}
UPB_INLINE const google_protobuf_EnumDescriptorProto* const* google_protobuf_DescriptorProto_enum_type(const google_protobuf_DescriptorProto* msg, size_t* len) {
  return (const google_protobuf_EnumDescriptorProto* const*)_upb_array_accessor(msg, UPB_SIZE(24, 48), len);
}
UPB_INLINE bool google_protobuf_DescriptorProto_has_extension_range(const google_protobuf_DescriptorProto* msg) {
  return _upb_has_submsg_nohasbit(msg, UPB_SIZE(28, 56));
}
UPB_INLINE const google_protobuf_DescriptorProto_ExtensionRange* const* google_protobuf_DescriptorProto_extension_range(const google_protobuf_DescriptorProto* msg, size_t* len) {
  return (const google_protobuf_DescriptorProto_ExtensionRange* const*)_upb_array_accessor(msg, UPB_SIZE(28, 56), len);
}
UPB_INLINE bool google_protobuf_DescriptorProto_has_extension(const google_protobuf_DescriptorProto* msg) {
  return _upb_has_submsg_nohasbit(msg, UPB_SIZE(32, 64));
}
UPB_INLINE const google_protobuf_FieldDescriptorProto* const* google_protobuf_DescriptorProto_extension(const google_protobuf_DescriptorProto* msg, size_t* len) {
  return (const google_protobuf_FieldDescriptorProto* const*)_upb_array_accessor(msg, UPB_SIZE(32, 64), len);
}
UPB_INLINE bool google_protobuf_DescriptorProto_has_options(const google_protobuf_DescriptorProto* msg) {
  return _upb_hasbit(msg, 2);
}
UPB_INLINE const google_protobuf_MessageOptions* google_protobuf_DescriptorProto_options(const google_protobuf_DescriptorProto* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(12, 24), const google_protobuf_MessageOptions*);
}
UPB_INLINE bool google_protobuf_DescriptorProto_has_oneof_decl(const google_protobuf_DescriptorProto* msg) {
  return _upb_has_submsg_nohasbit(msg, UPB_SIZE(36, 72));
}
UPB_INLINE const google_protobuf_OneofDescriptorProto* const* google_protobuf_DescriptorProto_oneof_decl(const google_protobuf_DescriptorProto* msg, size_t* len) {
  return (const google_protobuf_OneofDescriptorProto* const*)_upb_array_accessor(msg, UPB_SIZE(36, 72), len);
}
UPB_INLINE bool google_protobuf_DescriptorProto_has_reserved_range(const google_protobuf_DescriptorProto* msg) {
  return _upb_has_submsg_nohasbit(msg, UPB_SIZE(40, 80));
}
UPB_INLINE const google_protobuf_DescriptorProto_ReservedRange* const* google_protobuf_DescriptorProto_reserved_range(const google_protobuf_DescriptorProto* msg, size_t* len) {
  return (const google_protobuf_DescriptorProto_ReservedRange* const*)_upb_array_accessor(msg, UPB_SIZE(40, 80), len);
}
UPB_INLINE upb_StringView const* google_protobuf_DescriptorProto_reserved_name(const google_protobuf_DescriptorProto* msg, size_t* len) {
  return (upb_StringView const*)_upb_array_accessor(msg, UPB_SIZE(44, 88), len);
}

UPB_INLINE void google_protobuf_DescriptorProto_set_name(google_protobuf_DescriptorProto *msg, upb_StringView value) {
  _upb_sethas(msg, 1);
  *UPB_PTR_AT(msg, UPB_SIZE(4, 8), upb_StringView) = value;
}
UPB_INLINE google_protobuf_FieldDescriptorProto** google_protobuf_DescriptorProto_mutable_field(google_protobuf_DescriptorProto* msg, size_t* len) {
  return (google_protobuf_FieldDescriptorProto**)_upb_array_mutable_accessor(msg, UPB_SIZE(16, 32), len);
}
UPB_INLINE google_protobuf_FieldDescriptorProto** google_protobuf_DescriptorProto_resize_field(google_protobuf_DescriptorProto* msg, size_t len, upb_Arena* arena) {
  return (google_protobuf_FieldDescriptorProto**)_upb_Array_Resize_accessor2(msg, UPB_SIZE(16, 32), len, UPB_SIZE(2, 3), arena);
}
UPB_INLINE struct google_protobuf_FieldDescriptorProto* google_protobuf_DescriptorProto_add_field(google_protobuf_DescriptorProto* msg, upb_Arena* arena) {
  struct google_protobuf_FieldDescriptorProto* sub = (struct google_protobuf_FieldDescriptorProto*)_upb_Message_New(&google_protobuf_FieldDescriptorProto_msginit, arena);
  bool ok = _upb_Array_Append_accessor2(msg, UPB_SIZE(16, 32), UPB_SIZE(2, 3), &sub, arena);
  if (!ok) return NULL;
  return sub;
}
UPB_INLINE google_protobuf_DescriptorProto** google_protobuf_DescriptorProto_mutable_nested_type(google_protobuf_DescriptorProto* msg, size_t* len) {
  return (google_protobuf_DescriptorProto**)_upb_array_mutable_accessor(msg, UPB_SIZE(20, 40), len);
}
UPB_INLINE google_protobuf_DescriptorProto** google_protobuf_DescriptorProto_resize_nested_type(google_protobuf_DescriptorProto* msg, size_t len, upb_Arena* arena) {
  return (google_protobuf_DescriptorProto**)_upb_Array_Resize_accessor2(msg, UPB_SIZE(20, 40), len, UPB_SIZE(2, 3), arena);
}
UPB_INLINE struct google_protobuf_DescriptorProto* google_protobuf_DescriptorProto_add_nested_type(google_protobuf_DescriptorProto* msg, upb_Arena* arena) {
  struct google_protobuf_DescriptorProto* sub = (struct google_protobuf_DescriptorProto*)_upb_Message_New(&google_protobuf_DescriptorProto_msginit, arena);
  bool ok = _upb_Array_Append_accessor2(msg, UPB_SIZE(20, 40), UPB_SIZE(2, 3), &sub, arena);
  if (!ok) return NULL;
  return sub;
}
UPB_INLINE google_protobuf_EnumDescriptorProto** google_protobuf_DescriptorProto_mutable_enum_type(google_protobuf_DescriptorProto* msg, size_t* len) {
  return (google_protobuf_EnumDescriptorProto**)_upb_array_mutable_accessor(msg, UPB_SIZE(24, 48), len);
}
UPB_INLINE google_protobuf_EnumDescriptorProto** google_protobuf_DescriptorProto_resize_enum_type(google_protobuf_DescriptorProto* msg, size_t len, upb_Arena* arena) {
  return (google_protobuf_EnumDescriptorProto**)_upb_Array_Resize_accessor2(msg, UPB_SIZE(24, 48), len, UPB_SIZE(2, 3), arena);
}
UPB_INLINE struct google_protobuf_EnumDescriptorProto* google_protobuf_DescriptorProto_add_enum_type(google_protobuf_DescriptorProto* msg, upb_Arena* arena) {
  struct google_protobuf_EnumDescriptorProto* sub = (struct google_protobuf_EnumDescriptorProto*)_upb_Message_New(&google_protobuf_EnumDescriptorProto_msginit, arena);
  bool ok = _upb_Array_Append_accessor2(msg, UPB_SIZE(24, 48), UPB_SIZE(2, 3), &sub, arena);
  if (!ok) return NULL;
  return sub;
}
UPB_INLINE google_protobuf_DescriptorProto_ExtensionRange** google_protobuf_DescriptorProto_mutable_extension_range(google_protobuf_DescriptorProto* msg, size_t* len) {
  return (google_protobuf_DescriptorProto_ExtensionRange**)_upb_array_mutable_accessor(msg, UPB_SIZE(28, 56), len);
}
UPB_INLINE google_protobuf_DescriptorProto_ExtensionRange** google_protobuf_DescriptorProto_resize_extension_range(google_protobuf_DescriptorProto* msg, size_t len, upb_Arena* arena) {
  return (google_protobuf_DescriptorProto_ExtensionRange**)_upb_Array_Resize_accessor2(msg, UPB_SIZE(28, 56), len, UPB_SIZE(2, 3), arena);
}
UPB_INLINE struct google_protobuf_DescriptorProto_ExtensionRange* google_protobuf_DescriptorProto_add_extension_range(google_protobuf_DescriptorProto* msg, upb_Arena* arena) {
  struct google_protobuf_DescriptorProto_ExtensionRange* sub = (struct google_protobuf_DescriptorProto_ExtensionRange*)_upb_Message_New(&google_protobuf_DescriptorProto_ExtensionRange_msginit, arena);
  bool ok = _upb_Array_Append_accessor2(msg, UPB_SIZE(28, 56), UPB_SIZE(2, 3), &sub, arena);
  if (!ok) return NULL;
  return sub;
}
UPB_INLINE google_protobuf_FieldDescriptorProto** google_protobuf_DescriptorProto_mutable_extension(google_protobuf_DescriptorProto* msg, size_t* len) {
  return (google_protobuf_FieldDescriptorProto**)_upb_array_mutable_accessor(msg, UPB_SIZE(32, 64), len);
}
UPB_INLINE google_protobuf_FieldDescriptorProto** google_protobuf_DescriptorProto_resize_extension(google_protobuf_DescriptorProto* msg, size_t len, upb_Arena* arena) {
  return (google_protobuf_FieldDescriptorProto**)_upb_Array_Resize_accessor2(msg, UPB_SIZE(32, 64), len, UPB_SIZE(2, 3), arena);
}
UPB_INLINE struct google_protobuf_FieldDescriptorProto* google_protobuf_DescriptorProto_add_extension(google_protobuf_DescriptorProto* msg, upb_Arena* arena) {
  struct google_protobuf_FieldDescriptorProto* sub = (struct google_protobuf_FieldDescriptorProto*)_upb_Message_New(&google_protobuf_FieldDescriptorProto_msginit, arena);
  bool ok = _upb_Array_Append_accessor2(msg, UPB_SIZE(32, 64), UPB_SIZE(2, 3), &sub, arena);
  if (!ok) return NULL;
  return sub;
}
UPB_INLINE void google_protobuf_DescriptorProto_set_options(google_protobuf_DescriptorProto *msg, google_protobuf_MessageOptions* value) {
  _upb_sethas(msg, 2);
  *UPB_PTR_AT(msg, UPB_SIZE(12, 24), google_protobuf_MessageOptions*) = value;
}
UPB_INLINE struct google_protobuf_MessageOptions* google_protobuf_DescriptorProto_mutable_options(google_protobuf_DescriptorProto* msg, upb_Arena* arena) {
  struct google_protobuf_MessageOptions* sub = (struct google_protobuf_MessageOptions*)google_protobuf_DescriptorProto_options(msg);
  if (sub == NULL) {
    sub = (struct google_protobuf_MessageOptions*)_upb_Message_New(&google_protobuf_MessageOptions_msginit, arena);
    if (!sub) return NULL;
    google_protobuf_DescriptorProto_set_options(msg, sub);
  }
  return sub;
}
UPB_INLINE google_protobuf_OneofDescriptorProto** google_protobuf_DescriptorProto_mutable_oneof_decl(google_protobuf_DescriptorProto* msg, size_t* len) {
  return (google_protobuf_OneofDescriptorProto**)_upb_array_mutable_accessor(msg, UPB_SIZE(36, 72), len);
}
UPB_INLINE google_protobuf_OneofDescriptorProto** google_protobuf_DescriptorProto_resize_oneof_decl(google_protobuf_DescriptorProto* msg, size_t len, upb_Arena* arena) {
  return (google_protobuf_OneofDescriptorProto**)_upb_Array_Resize_accessor2(msg, UPB_SIZE(36, 72), len, UPB_SIZE(2, 3), arena);
}
UPB_INLINE struct google_protobuf_OneofDescriptorProto* google_protobuf_DescriptorProto_add_oneof_decl(google_protobuf_DescriptorProto* msg, upb_Arena* arena) {
  struct google_protobuf_OneofDescriptorProto* sub = (struct google_protobuf_OneofDescriptorProto*)_upb_Message_New(&google_protobuf_OneofDescriptorProto_msginit, arena);
  bool ok = _upb_Array_Append_accessor2(msg, UPB_SIZE(36, 72), UPB_SIZE(2, 3), &sub, arena);
  if (!ok) return NULL;
  return sub;
}
UPB_INLINE google_protobuf_DescriptorProto_ReservedRange** google_protobuf_DescriptorProto_mutable_reserved_range(google_protobuf_DescriptorProto* msg, size_t* len) {
  return (google_protobuf_DescriptorProto_ReservedRange**)_upb_array_mutable_accessor(msg, UPB_SIZE(40, 80), len);
}
UPB_INLINE google_protobuf_DescriptorProto_ReservedRange** google_protobuf_DescriptorProto_resize_reserved_range(google_protobuf_DescriptorProto* msg, size_t len, upb_Arena* arena) {
  return (google_protobuf_DescriptorProto_ReservedRange**)_upb_Array_Resize_accessor2(msg, UPB_SIZE(40, 80), len, UPB_SIZE(2, 3), arena);
}
UPB_INLINE struct google_protobuf_DescriptorProto_ReservedRange* google_protobuf_DescriptorProto_add_reserved_range(google_protobuf_DescriptorProto* msg, upb_Arena* arena) {
  struct google_protobuf_DescriptorProto_ReservedRange* sub = (struct google_protobuf_DescriptorProto_ReservedRange*)_upb_Message_New(&google_protobuf_DescriptorProto_ReservedRange_msginit, arena);
  bool ok = _upb_Array_Append_accessor2(msg, UPB_SIZE(40, 80), UPB_SIZE(2, 3), &sub, arena);
  if (!ok) return NULL;
  return sub;
}
UPB_INLINE upb_StringView* google_protobuf_DescriptorProto_mutable_reserved_name(google_protobuf_DescriptorProto* msg, size_t* len) {
  return (upb_StringView*)_upb_array_mutable_accessor(msg, UPB_SIZE(44, 88), len);
}
UPB_INLINE upb_StringView* google_protobuf_DescriptorProto_resize_reserved_name(google_protobuf_DescriptorProto* msg, size_t len, upb_Arena* arena) {
  return (upb_StringView*)_upb_Array_Resize_accessor2(msg, UPB_SIZE(44, 88), len, UPB_SIZE(3, 4), arena);
}
UPB_INLINE bool google_protobuf_DescriptorProto_add_reserved_name(google_protobuf_DescriptorProto* msg, upb_StringView val, upb_Arena* arena) {
  return _upb_Array_Append_accessor2(msg, UPB_SIZE(44, 88), UPB_SIZE(3, 4), &val, arena);
}

/* google.protobuf.DescriptorProto.ExtensionRange */

UPB_INLINE google_protobuf_DescriptorProto_ExtensionRange* google_protobuf_DescriptorProto_ExtensionRange_new(upb_Arena* arena) {
  return (google_protobuf_DescriptorProto_ExtensionRange*)_upb_Message_New(&google_protobuf_DescriptorProto_ExtensionRange_msginit, arena);
}
UPB_INLINE google_protobuf_DescriptorProto_ExtensionRange* google_protobuf_DescriptorProto_ExtensionRange_parse(const char* buf, size_t size, upb_Arena* arena) {
  google_protobuf_DescriptorProto_ExtensionRange* ret = google_protobuf_DescriptorProto_ExtensionRange_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_DescriptorProto_ExtensionRange_msginit, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE google_protobuf_DescriptorProto_ExtensionRange* google_protobuf_DescriptorProto_ExtensionRange_parse_ex(const char* buf, size_t size,
                           const upb_ExtensionRegistry* extreg,
                           int options, upb_Arena* arena) {
  google_protobuf_DescriptorProto_ExtensionRange* ret = google_protobuf_DescriptorProto_ExtensionRange_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_DescriptorProto_ExtensionRange_msginit, extreg, options, arena) !=
      kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE char* google_protobuf_DescriptorProto_ExtensionRange_serialize(const google_protobuf_DescriptorProto_ExtensionRange* msg, upb_Arena* arena, size_t* len) {
  return upb_Encode(msg, &google_protobuf_DescriptorProto_ExtensionRange_msginit, 0, arena, len);
}
UPB_INLINE char* google_protobuf_DescriptorProto_ExtensionRange_serialize_ex(const google_protobuf_DescriptorProto_ExtensionRange* msg, int options,
                                 upb_Arena* arena, size_t* len) {
  return upb_Encode(msg, &google_protobuf_DescriptorProto_ExtensionRange_msginit, options, arena, len);
}
UPB_INLINE bool google_protobuf_DescriptorProto_ExtensionRange_has_start(const google_protobuf_DescriptorProto_ExtensionRange* msg) {
  return _upb_hasbit(msg, 1);
}
UPB_INLINE int32_t google_protobuf_DescriptorProto_ExtensionRange_start(const google_protobuf_DescriptorProto_ExtensionRange* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(4, 4), int32_t);
}
UPB_INLINE bool google_protobuf_DescriptorProto_ExtensionRange_has_end(const google_protobuf_DescriptorProto_ExtensionRange* msg) {
  return _upb_hasbit(msg, 2);
}
UPB_INLINE int32_t google_protobuf_DescriptorProto_ExtensionRange_end(const google_protobuf_DescriptorProto_ExtensionRange* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(8, 8), int32_t);
}
UPB_INLINE bool google_protobuf_DescriptorProto_ExtensionRange_has_options(const google_protobuf_DescriptorProto_ExtensionRange* msg) {
  return _upb_hasbit(msg, 3);
}
UPB_INLINE const google_protobuf_ExtensionRangeOptions* google_protobuf_DescriptorProto_ExtensionRange_options(const google_protobuf_DescriptorProto_ExtensionRange* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(12, 16), const google_protobuf_ExtensionRangeOptions*);
}

UPB_INLINE void google_protobuf_DescriptorProto_ExtensionRange_set_start(google_protobuf_DescriptorProto_ExtensionRange *msg, int32_t value) {
  _upb_sethas(msg, 1);
  *UPB_PTR_AT(msg, UPB_SIZE(4, 4), int32_t) = value;
}
UPB_INLINE void google_protobuf_DescriptorProto_ExtensionRange_set_end(google_protobuf_DescriptorProto_ExtensionRange *msg, int32_t value) {
  _upb_sethas(msg, 2);
  *UPB_PTR_AT(msg, UPB_SIZE(8, 8), int32_t) = value;
}
UPB_INLINE void google_protobuf_DescriptorProto_ExtensionRange_set_options(google_protobuf_DescriptorProto_ExtensionRange *msg, google_protobuf_ExtensionRangeOptions* value) {
  _upb_sethas(msg, 3);
  *UPB_PTR_AT(msg, UPB_SIZE(12, 16), google_protobuf_ExtensionRangeOptions*) = value;
}
UPB_INLINE struct google_protobuf_ExtensionRangeOptions* google_protobuf_DescriptorProto_ExtensionRange_mutable_options(google_protobuf_DescriptorProto_ExtensionRange* msg, upb_Arena* arena) {
  struct google_protobuf_ExtensionRangeOptions* sub = (struct google_protobuf_ExtensionRangeOptions*)google_protobuf_DescriptorProto_ExtensionRange_options(msg);
  if (sub == NULL) {
    sub = (struct google_protobuf_ExtensionRangeOptions*)_upb_Message_New(&google_protobuf_ExtensionRangeOptions_msginit, arena);
    if (!sub) return NULL;
    google_protobuf_DescriptorProto_ExtensionRange_set_options(msg, sub);
  }
  return sub;
}

/* google.protobuf.DescriptorProto.ReservedRange */

UPB_INLINE google_protobuf_DescriptorProto_ReservedRange* google_protobuf_DescriptorProto_ReservedRange_new(upb_Arena* arena) {
  return (google_protobuf_DescriptorProto_ReservedRange*)_upb_Message_New(&google_protobuf_DescriptorProto_ReservedRange_msginit, arena);
}
UPB_INLINE google_protobuf_DescriptorProto_ReservedRange* google_protobuf_DescriptorProto_ReservedRange_parse(const char* buf, size_t size, upb_Arena* arena) {
  google_protobuf_DescriptorProto_ReservedRange* ret = google_protobuf_DescriptorProto_ReservedRange_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_DescriptorProto_ReservedRange_msginit, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE google_protobuf_DescriptorProto_ReservedRange* google_protobuf_DescriptorProto_ReservedRange_parse_ex(const char* buf, size_t size,
                           const upb_ExtensionRegistry* extreg,
                           int options, upb_Arena* arena) {
  google_protobuf_DescriptorProto_ReservedRange* ret = google_protobuf_DescriptorProto_ReservedRange_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_DescriptorProto_ReservedRange_msginit, extreg, options, arena) !=
      kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE char* google_protobuf_DescriptorProto_ReservedRange_serialize(const google_protobuf_DescriptorProto_ReservedRange* msg, upb_Arena* arena, size_t* len) {
  return upb_Encode(msg, &google_protobuf_DescriptorProto_ReservedRange_msginit, 0, arena, len);
}
UPB_INLINE char* google_protobuf_DescriptorProto_ReservedRange_serialize_ex(const google_protobuf_DescriptorProto_ReservedRange* msg, int options,
                                 upb_Arena* arena, size_t* len) {
  return upb_Encode(msg, &google_protobuf_DescriptorProto_ReservedRange_msginit, options, arena, len);
}
UPB_INLINE bool google_protobuf_DescriptorProto_ReservedRange_has_start(const google_protobuf_DescriptorProto_ReservedRange* msg) {
  return _upb_hasbit(msg, 1);
}
UPB_INLINE int32_t google_protobuf_DescriptorProto_ReservedRange_start(const google_protobuf_DescriptorProto_ReservedRange* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(4, 4), int32_t);
}
UPB_INLINE bool google_protobuf_DescriptorProto_ReservedRange_has_end(const google_protobuf_DescriptorProto_ReservedRange* msg) {
  return _upb_hasbit(msg, 2);
}
UPB_INLINE int32_t google_protobuf_DescriptorProto_ReservedRange_end(const google_protobuf_DescriptorProto_ReservedRange* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(8, 8), int32_t);
}

UPB_INLINE void google_protobuf_DescriptorProto_ReservedRange_set_start(google_protobuf_DescriptorProto_ReservedRange *msg, int32_t value) {
  _upb_sethas(msg, 1);
  *UPB_PTR_AT(msg, UPB_SIZE(4, 4), int32_t) = value;
}
UPB_INLINE void google_protobuf_DescriptorProto_ReservedRange_set_end(google_protobuf_DescriptorProto_ReservedRange *msg, int32_t value) {
  _upb_sethas(msg, 2);
  *UPB_PTR_AT(msg, UPB_SIZE(8, 8), int32_t) = value;
}

/* google.protobuf.ExtensionRangeOptions */

UPB_INLINE google_protobuf_ExtensionRangeOptions* google_protobuf_ExtensionRangeOptions_new(upb_Arena* arena) {
  return (google_protobuf_ExtensionRangeOptions*)_upb_Message_New(&google_protobuf_ExtensionRangeOptions_msginit, arena);
}
UPB_INLINE google_protobuf_ExtensionRangeOptions* google_protobuf_ExtensionRangeOptions_parse(const char* buf, size_t size, upb_Arena* arena) {
  google_protobuf_ExtensionRangeOptions* ret = google_protobuf_ExtensionRangeOptions_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_ExtensionRangeOptions_msginit, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE google_protobuf_ExtensionRangeOptions* google_protobuf_ExtensionRangeOptions_parse_ex(const char* buf, size_t size,
                           const upb_ExtensionRegistry* extreg,
                           int options, upb_Arena* arena) {
  google_protobuf_ExtensionRangeOptions* ret = google_protobuf_ExtensionRangeOptions_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_ExtensionRangeOptions_msginit, extreg, options, arena) !=
      kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE char* google_protobuf_ExtensionRangeOptions_serialize(const google_protobuf_ExtensionRangeOptions* msg, upb_Arena* arena, size_t* len) {
  return upb_Encode(msg, &google_protobuf_ExtensionRangeOptions_msginit, 0, arena, len);
}
UPB_INLINE char* google_protobuf_ExtensionRangeOptions_serialize_ex(const google_protobuf_ExtensionRangeOptions* msg, int options,
                                 upb_Arena* arena, size_t* len) {
  return upb_Encode(msg, &google_protobuf_ExtensionRangeOptions_msginit, options, arena, len);
}
UPB_INLINE bool google_protobuf_ExtensionRangeOptions_has_uninterpreted_option(const google_protobuf_ExtensionRangeOptions* msg) {
  return _upb_has_submsg_nohasbit(msg, UPB_SIZE(0, 0));
}
UPB_INLINE const google_protobuf_UninterpretedOption* const* google_protobuf_ExtensionRangeOptions_uninterpreted_option(const google_protobuf_ExtensionRangeOptions* msg, size_t* len) {
  return (const google_protobuf_UninterpretedOption* const*)_upb_array_accessor(msg, UPB_SIZE(0, 0), len);
}

UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_ExtensionRangeOptions_mutable_uninterpreted_option(google_protobuf_ExtensionRangeOptions* msg, size_t* len) {
  return (google_protobuf_UninterpretedOption**)_upb_array_mutable_accessor(msg, UPB_SIZE(0, 0), len);
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_ExtensionRangeOptions_resize_uninterpreted_option(google_protobuf_ExtensionRangeOptions* msg, size_t len, upb_Arena* arena) {
  return (google_protobuf_UninterpretedOption**)_upb_Array_Resize_accessor2(msg, UPB_SIZE(0, 0), len, UPB_SIZE(2, 3), arena);
}
UPB_INLINE struct google_protobuf_UninterpretedOption* google_protobuf_ExtensionRangeOptions_add_uninterpreted_option(google_protobuf_ExtensionRangeOptions* msg, upb_Arena* arena) {
  struct google_protobuf_UninterpretedOption* sub = (struct google_protobuf_UninterpretedOption*)_upb_Message_New(&google_protobuf_UninterpretedOption_msginit, arena);
  bool ok = _upb_Array_Append_accessor2(msg, UPB_SIZE(0, 0), UPB_SIZE(2, 3), &sub, arena);
  if (!ok) return NULL;
  return sub;
}

/* google.protobuf.FieldDescriptorProto */

UPB_INLINE google_protobuf_FieldDescriptorProto* google_protobuf_FieldDescriptorProto_new(upb_Arena* arena) {
  return (google_protobuf_FieldDescriptorProto*)_upb_Message_New(&google_protobuf_FieldDescriptorProto_msginit, arena);
}
UPB_INLINE google_protobuf_FieldDescriptorProto* google_protobuf_FieldDescriptorProto_parse(const char* buf, size_t size, upb_Arena* arena) {
  google_protobuf_FieldDescriptorProto* ret = google_protobuf_FieldDescriptorProto_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_FieldDescriptorProto_msginit, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE google_protobuf_FieldDescriptorProto* google_protobuf_FieldDescriptorProto_parse_ex(const char* buf, size_t size,
                           const upb_ExtensionRegistry* extreg,
                           int options, upb_Arena* arena) {
  google_protobuf_FieldDescriptorProto* ret = google_protobuf_FieldDescriptorProto_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_FieldDescriptorProto_msginit, extreg, options, arena) !=
      kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE char* google_protobuf_FieldDescriptorProto_serialize(const google_protobuf_FieldDescriptorProto* msg, upb_Arena* arena, size_t* len) {
  return upb_Encode(msg, &google_protobuf_FieldDescriptorProto_msginit, 0, arena, len);
}
UPB_INLINE char* google_protobuf_FieldDescriptorProto_serialize_ex(const google_protobuf_FieldDescriptorProto* msg, int options,
                                 upb_Arena* arena, size_t* len) {
  return upb_Encode(msg, &google_protobuf_FieldDescriptorProto_msginit, options, arena, len);
}
UPB_INLINE bool google_protobuf_FieldDescriptorProto_has_name(const google_protobuf_FieldDescriptorProto* msg) {
  return _upb_hasbit(msg, 1);
}
UPB_INLINE upb_StringView google_protobuf_FieldDescriptorProto_name(const google_protobuf_FieldDescriptorProto* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(24, 24), upb_StringView);
}
UPB_INLINE bool google_protobuf_FieldDescriptorProto_has_extendee(const google_protobuf_FieldDescriptorProto* msg) {
  return _upb_hasbit(msg, 2);
}
UPB_INLINE upb_StringView google_protobuf_FieldDescriptorProto_extendee(const google_protobuf_FieldDescriptorProto* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(32, 40), upb_StringView);
}
UPB_INLINE bool google_protobuf_FieldDescriptorProto_has_number(const google_protobuf_FieldDescriptorProto* msg) {
  return _upb_hasbit(msg, 3);
}
UPB_INLINE int32_t google_protobuf_FieldDescriptorProto_number(const google_protobuf_FieldDescriptorProto* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(12, 12), int32_t);
}
UPB_INLINE bool google_protobuf_FieldDescriptorProto_has_label(const google_protobuf_FieldDescriptorProto* msg) {
  return _upb_hasbit(msg, 4);
}
UPB_INLINE int32_t google_protobuf_FieldDescriptorProto_label(const google_protobuf_FieldDescriptorProto* msg) {
  return google_protobuf_FieldDescriptorProto_has_label(msg) ? *UPB_PTR_AT(msg, UPB_SIZE(4, 4), int32_t) : 1;
}
UPB_INLINE bool google_protobuf_FieldDescriptorProto_has_type(const google_protobuf_FieldDescriptorProto* msg) {
  return _upb_hasbit(msg, 5);
}
UPB_INLINE int32_t google_protobuf_FieldDescriptorProto_type(const google_protobuf_FieldDescriptorProto* msg) {
  return google_protobuf_FieldDescriptorProto_has_type(msg) ? *UPB_PTR_AT(msg, UPB_SIZE(8, 8), int32_t) : 1;
}
UPB_INLINE bool google_protobuf_FieldDescriptorProto_has_type_name(const google_protobuf_FieldDescriptorProto* msg) {
  return _upb_hasbit(msg, 6);
}
UPB_INLINE upb_StringView google_protobuf_FieldDescriptorProto_type_name(const google_protobuf_FieldDescriptorProto* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(40, 56), upb_StringView);
}
UPB_INLINE bool google_protobuf_FieldDescriptorProto_has_default_value(const google_protobuf_FieldDescriptorProto* msg) {
  return _upb_hasbit(msg, 7);
}
UPB_INLINE upb_StringView google_protobuf_FieldDescriptorProto_default_value(const google_protobuf_FieldDescriptorProto* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(48, 72), upb_StringView);
}
UPB_INLINE bool google_protobuf_FieldDescriptorProto_has_options(const google_protobuf_FieldDescriptorProto* msg) {
  return _upb_hasbit(msg, 8);
}
UPB_INLINE const google_protobuf_FieldOptions* google_protobuf_FieldDescriptorProto_options(const google_protobuf_FieldDescriptorProto* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(64, 104), const google_protobuf_FieldOptions*);
}
UPB_INLINE bool google_protobuf_FieldDescriptorProto_has_oneof_index(const google_protobuf_FieldDescriptorProto* msg) {
  return _upb_hasbit(msg, 9);
}
UPB_INLINE int32_t google_protobuf_FieldDescriptorProto_oneof_index(const google_protobuf_FieldDescriptorProto* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(16, 16), int32_t);
}
UPB_INLINE bool google_protobuf_FieldDescriptorProto_has_json_name(const google_protobuf_FieldDescriptorProto* msg) {
  return _upb_hasbit(msg, 10);
}
UPB_INLINE upb_StringView google_protobuf_FieldDescriptorProto_json_name(const google_protobuf_FieldDescriptorProto* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(56, 88), upb_StringView);
}
UPB_INLINE bool google_protobuf_FieldDescriptorProto_has_proto3_optional(const google_protobuf_FieldDescriptorProto* msg) {
  return _upb_hasbit(msg, 11);
}
UPB_INLINE bool google_protobuf_FieldDescriptorProto_proto3_optional(const google_protobuf_FieldDescriptorProto* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(20, 20), bool);
}

UPB_INLINE void google_protobuf_FieldDescriptorProto_set_name(google_protobuf_FieldDescriptorProto *msg, upb_StringView value) {
  _upb_sethas(msg, 1);
  *UPB_PTR_AT(msg, UPB_SIZE(24, 24), upb_StringView) = value;
}
UPB_INLINE void google_protobuf_FieldDescriptorProto_set_extendee(google_protobuf_FieldDescriptorProto *msg, upb_StringView value) {
  _upb_sethas(msg, 2);
  *UPB_PTR_AT(msg, UPB_SIZE(32, 40), upb_StringView) = value;
}
UPB_INLINE void google_protobuf_FieldDescriptorProto_set_number(google_protobuf_FieldDescriptorProto *msg, int32_t value) {
  _upb_sethas(msg, 3);
  *UPB_PTR_AT(msg, UPB_SIZE(12, 12), int32_t) = value;
}
UPB_INLINE void google_protobuf_FieldDescriptorProto_set_label(google_protobuf_FieldDescriptorProto *msg, int32_t value) {
  _upb_sethas(msg, 4);
  *UPB_PTR_AT(msg, UPB_SIZE(4, 4), int32_t) = value;
}
UPB_INLINE void google_protobuf_FieldDescriptorProto_set_type(google_protobuf_FieldDescriptorProto *msg, int32_t value) {
  _upb_sethas(msg, 5);
  *UPB_PTR_AT(msg, UPB_SIZE(8, 8), int32_t) = value;
}
UPB_INLINE void google_protobuf_FieldDescriptorProto_set_type_name(google_protobuf_FieldDescriptorProto *msg, upb_StringView value) {
  _upb_sethas(msg, 6);
  *UPB_PTR_AT(msg, UPB_SIZE(40, 56), upb_StringView) = value;
}
UPB_INLINE void google_protobuf_FieldDescriptorProto_set_default_value(google_protobuf_FieldDescriptorProto *msg, upb_StringView value) {
  _upb_sethas(msg, 7);
  *UPB_PTR_AT(msg, UPB_SIZE(48, 72), upb_StringView) = value;
}
UPB_INLINE void google_protobuf_FieldDescriptorProto_set_options(google_protobuf_FieldDescriptorProto *msg, google_protobuf_FieldOptions* value) {
  _upb_sethas(msg, 8);
  *UPB_PTR_AT(msg, UPB_SIZE(64, 104), google_protobuf_FieldOptions*) = value;
}
UPB_INLINE struct google_protobuf_FieldOptions* google_protobuf_FieldDescriptorProto_mutable_options(google_protobuf_FieldDescriptorProto* msg, upb_Arena* arena) {
  struct google_protobuf_FieldOptions* sub = (struct google_protobuf_FieldOptions*)google_protobuf_FieldDescriptorProto_options(msg);
  if (sub == NULL) {
    sub = (struct google_protobuf_FieldOptions*)_upb_Message_New(&google_protobuf_FieldOptions_msginit, arena);
    if (!sub) return NULL;
    google_protobuf_FieldDescriptorProto_set_options(msg, sub);
  }
  return sub;
}
UPB_INLINE void google_protobuf_FieldDescriptorProto_set_oneof_index(google_protobuf_FieldDescriptorProto *msg, int32_t value) {
  _upb_sethas(msg, 9);
  *UPB_PTR_AT(msg, UPB_SIZE(16, 16), int32_t) = value;
}
UPB_INLINE void google_protobuf_FieldDescriptorProto_set_json_name(google_protobuf_FieldDescriptorProto *msg, upb_StringView value) {
  _upb_sethas(msg, 10);
  *UPB_PTR_AT(msg, UPB_SIZE(56, 88), upb_StringView) = value;
}
UPB_INLINE void google_protobuf_FieldDescriptorProto_set_proto3_optional(google_protobuf_FieldDescriptorProto *msg, bool value) {
  _upb_sethas(msg, 11);
  *UPB_PTR_AT(msg, UPB_SIZE(20, 20), bool) = value;
}

/* google.protobuf.OneofDescriptorProto */

UPB_INLINE google_protobuf_OneofDescriptorProto* google_protobuf_OneofDescriptorProto_new(upb_Arena* arena) {
  return (google_protobuf_OneofDescriptorProto*)_upb_Message_New(&google_protobuf_OneofDescriptorProto_msginit, arena);
}
UPB_INLINE google_protobuf_OneofDescriptorProto* google_protobuf_OneofDescriptorProto_parse(const char* buf, size_t size, upb_Arena* arena) {
  google_protobuf_OneofDescriptorProto* ret = google_protobuf_OneofDescriptorProto_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_OneofDescriptorProto_msginit, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE google_protobuf_OneofDescriptorProto* google_protobuf_OneofDescriptorProto_parse_ex(const char* buf, size_t size,
                           const upb_ExtensionRegistry* extreg,
                           int options, upb_Arena* arena) {
  google_protobuf_OneofDescriptorProto* ret = google_protobuf_OneofDescriptorProto_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_OneofDescriptorProto_msginit, extreg, options, arena) !=
      kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE char* google_protobuf_OneofDescriptorProto_serialize(const google_protobuf_OneofDescriptorProto* msg, upb_Arena* arena, size_t* len) {
  return upb_Encode(msg, &google_protobuf_OneofDescriptorProto_msginit, 0, arena, len);
}
UPB_INLINE char* google_protobuf_OneofDescriptorProto_serialize_ex(const google_protobuf_OneofDescriptorProto* msg, int options,
                                 upb_Arena* arena, size_t* len) {
  return upb_Encode(msg, &google_protobuf_OneofDescriptorProto_msginit, options, arena, len);
}
UPB_INLINE bool google_protobuf_OneofDescriptorProto_has_name(const google_protobuf_OneofDescriptorProto* msg) {
  return _upb_hasbit(msg, 1);
}
UPB_INLINE upb_StringView google_protobuf_OneofDescriptorProto_name(const google_protobuf_OneofDescriptorProto* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(4, 8), upb_StringView);
}
UPB_INLINE bool google_protobuf_OneofDescriptorProto_has_options(const google_protobuf_OneofDescriptorProto* msg) {
  return _upb_hasbit(msg, 2);
}
UPB_INLINE const google_protobuf_OneofOptions* google_protobuf_OneofDescriptorProto_options(const google_protobuf_OneofDescriptorProto* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(12, 24), const google_protobuf_OneofOptions*);
}

UPB_INLINE void google_protobuf_OneofDescriptorProto_set_name(google_protobuf_OneofDescriptorProto *msg, upb_StringView value) {
  _upb_sethas(msg, 1);
  *UPB_PTR_AT(msg, UPB_SIZE(4, 8), upb_StringView) = value;
}
UPB_INLINE void google_protobuf_OneofDescriptorProto_set_options(google_protobuf_OneofDescriptorProto *msg, google_protobuf_OneofOptions* value) {
  _upb_sethas(msg, 2);
  *UPB_PTR_AT(msg, UPB_SIZE(12, 24), google_protobuf_OneofOptions*) = value;
}
UPB_INLINE struct google_protobuf_OneofOptions* google_protobuf_OneofDescriptorProto_mutable_options(google_protobuf_OneofDescriptorProto* msg, upb_Arena* arena) {
  struct google_protobuf_OneofOptions* sub = (struct google_protobuf_OneofOptions*)google_protobuf_OneofDescriptorProto_options(msg);
  if (sub == NULL) {
    sub = (struct google_protobuf_OneofOptions*)_upb_Message_New(&google_protobuf_OneofOptions_msginit, arena);
    if (!sub) return NULL;
    google_protobuf_OneofDescriptorProto_set_options(msg, sub);
  }
  return sub;
}

/* google.protobuf.EnumDescriptorProto */

UPB_INLINE google_protobuf_EnumDescriptorProto* google_protobuf_EnumDescriptorProto_new(upb_Arena* arena) {
  return (google_protobuf_EnumDescriptorProto*)_upb_Message_New(&google_protobuf_EnumDescriptorProto_msginit, arena);
}
UPB_INLINE google_protobuf_EnumDescriptorProto* google_protobuf_EnumDescriptorProto_parse(const char* buf, size_t size, upb_Arena* arena) {
  google_protobuf_EnumDescriptorProto* ret = google_protobuf_EnumDescriptorProto_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_EnumDescriptorProto_msginit, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE google_protobuf_EnumDescriptorProto* google_protobuf_EnumDescriptorProto_parse_ex(const char* buf, size_t size,
                           const upb_ExtensionRegistry* extreg,
                           int options, upb_Arena* arena) {
  google_protobuf_EnumDescriptorProto* ret = google_protobuf_EnumDescriptorProto_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_EnumDescriptorProto_msginit, extreg, options, arena) !=
      kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE char* google_protobuf_EnumDescriptorProto_serialize(const google_protobuf_EnumDescriptorProto* msg, upb_Arena* arena, size_t* len) {
  return upb_Encode(msg, &google_protobuf_EnumDescriptorProto_msginit, 0, arena, len);
}
UPB_INLINE char* google_protobuf_EnumDescriptorProto_serialize_ex(const google_protobuf_EnumDescriptorProto* msg, int options,
                                 upb_Arena* arena, size_t* len) {
  return upb_Encode(msg, &google_protobuf_EnumDescriptorProto_msginit, options, arena, len);
}
UPB_INLINE bool google_protobuf_EnumDescriptorProto_has_name(const google_protobuf_EnumDescriptorProto* msg) {
  return _upb_hasbit(msg, 1);
}
UPB_INLINE upb_StringView google_protobuf_EnumDescriptorProto_name(const google_protobuf_EnumDescriptorProto* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(4, 8), upb_StringView);
}
UPB_INLINE bool google_protobuf_EnumDescriptorProto_has_value(const google_protobuf_EnumDescriptorProto* msg) {
  return _upb_has_submsg_nohasbit(msg, UPB_SIZE(16, 32));
}
UPB_INLINE const google_protobuf_EnumValueDescriptorProto* const* google_protobuf_EnumDescriptorProto_value(const google_protobuf_EnumDescriptorProto* msg, size_t* len) {
  return (const google_protobuf_EnumValueDescriptorProto* const*)_upb_array_accessor(msg, UPB_SIZE(16, 32), len);
}
UPB_INLINE bool google_protobuf_EnumDescriptorProto_has_options(const google_protobuf_EnumDescriptorProto* msg) {
  return _upb_hasbit(msg, 2);
}
UPB_INLINE const google_protobuf_EnumOptions* google_protobuf_EnumDescriptorProto_options(const google_protobuf_EnumDescriptorProto* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(12, 24), const google_protobuf_EnumOptions*);
}
UPB_INLINE bool google_protobuf_EnumDescriptorProto_has_reserved_range(const google_protobuf_EnumDescriptorProto* msg) {
  return _upb_has_submsg_nohasbit(msg, UPB_SIZE(20, 40));
}
UPB_INLINE const google_protobuf_EnumDescriptorProto_EnumReservedRange* const* google_protobuf_EnumDescriptorProto_reserved_range(const google_protobuf_EnumDescriptorProto* msg, size_t* len) {
  return (const google_protobuf_EnumDescriptorProto_EnumReservedRange* const*)_upb_array_accessor(msg, UPB_SIZE(20, 40), len);
}
UPB_INLINE upb_StringView const* google_protobuf_EnumDescriptorProto_reserved_name(const google_protobuf_EnumDescriptorProto* msg, size_t* len) {
  return (upb_StringView const*)_upb_array_accessor(msg, UPB_SIZE(24, 48), len);
}

UPB_INLINE void google_protobuf_EnumDescriptorProto_set_name(google_protobuf_EnumDescriptorProto *msg, upb_StringView value) {
  _upb_sethas(msg, 1);
  *UPB_PTR_AT(msg, UPB_SIZE(4, 8), upb_StringView) = value;
}
UPB_INLINE google_protobuf_EnumValueDescriptorProto** google_protobuf_EnumDescriptorProto_mutable_value(google_protobuf_EnumDescriptorProto* msg, size_t* len) {
  return (google_protobuf_EnumValueDescriptorProto**)_upb_array_mutable_accessor(msg, UPB_SIZE(16, 32), len);
}
UPB_INLINE google_protobuf_EnumValueDescriptorProto** google_protobuf_EnumDescriptorProto_resize_value(google_protobuf_EnumDescriptorProto* msg, size_t len, upb_Arena* arena) {
  return (google_protobuf_EnumValueDescriptorProto**)_upb_Array_Resize_accessor2(msg, UPB_SIZE(16, 32), len, UPB_SIZE(2, 3), arena);
}
UPB_INLINE struct google_protobuf_EnumValueDescriptorProto* google_protobuf_EnumDescriptorProto_add_value(google_protobuf_EnumDescriptorProto* msg, upb_Arena* arena) {
  struct google_protobuf_EnumValueDescriptorProto* sub = (struct google_protobuf_EnumValueDescriptorProto*)_upb_Message_New(&google_protobuf_EnumValueDescriptorProto_msginit, arena);
  bool ok = _upb_Array_Append_accessor2(msg, UPB_SIZE(16, 32), UPB_SIZE(2, 3), &sub, arena);
  if (!ok) return NULL;
  return sub;
}
UPB_INLINE void google_protobuf_EnumDescriptorProto_set_options(google_protobuf_EnumDescriptorProto *msg, google_protobuf_EnumOptions* value) {
  _upb_sethas(msg, 2);
  *UPB_PTR_AT(msg, UPB_SIZE(12, 24), google_protobuf_EnumOptions*) = value;
}
UPB_INLINE struct google_protobuf_EnumOptions* google_protobuf_EnumDescriptorProto_mutable_options(google_protobuf_EnumDescriptorProto* msg, upb_Arena* arena) {
  struct google_protobuf_EnumOptions* sub = (struct google_protobuf_EnumOptions*)google_protobuf_EnumDescriptorProto_options(msg);
  if (sub == NULL) {
    sub = (struct google_protobuf_EnumOptions*)_upb_Message_New(&google_protobuf_EnumOptions_msginit, arena);
    if (!sub) return NULL;
    google_protobuf_EnumDescriptorProto_set_options(msg, sub);
  }
  return sub;
}
UPB_INLINE google_protobuf_EnumDescriptorProto_EnumReservedRange** google_protobuf_EnumDescriptorProto_mutable_reserved_range(google_protobuf_EnumDescriptorProto* msg, size_t* len) {
  return (google_protobuf_EnumDescriptorProto_EnumReservedRange**)_upb_array_mutable_accessor(msg, UPB_SIZE(20, 40), len);
}
UPB_INLINE google_protobuf_EnumDescriptorProto_EnumReservedRange** google_protobuf_EnumDescriptorProto_resize_reserved_range(google_protobuf_EnumDescriptorProto* msg, size_t len, upb_Arena* arena) {
  return (google_protobuf_EnumDescriptorProto_EnumReservedRange**)_upb_Array_Resize_accessor2(msg, UPB_SIZE(20, 40), len, UPB_SIZE(2, 3), arena);
}
UPB_INLINE struct google_protobuf_EnumDescriptorProto_EnumReservedRange* google_protobuf_EnumDescriptorProto_add_reserved_range(google_protobuf_EnumDescriptorProto* msg, upb_Arena* arena) {
  struct google_protobuf_EnumDescriptorProto_EnumReservedRange* sub = (struct google_protobuf_EnumDescriptorProto_EnumReservedRange*)_upb_Message_New(&google_protobuf_EnumDescriptorProto_EnumReservedRange_msginit, arena);
  bool ok = _upb_Array_Append_accessor2(msg, UPB_SIZE(20, 40), UPB_SIZE(2, 3), &sub, arena);
  if (!ok) return NULL;
  return sub;
}
UPB_INLINE upb_StringView* google_protobuf_EnumDescriptorProto_mutable_reserved_name(google_protobuf_EnumDescriptorProto* msg, size_t* len) {
  return (upb_StringView*)_upb_array_mutable_accessor(msg, UPB_SIZE(24, 48), len);
}
UPB_INLINE upb_StringView* google_protobuf_EnumDescriptorProto_resize_reserved_name(google_protobuf_EnumDescriptorProto* msg, size_t len, upb_Arena* arena) {
  return (upb_StringView*)_upb_Array_Resize_accessor2(msg, UPB_SIZE(24, 48), len, UPB_SIZE(3, 4), arena);
}
UPB_INLINE bool google_protobuf_EnumDescriptorProto_add_reserved_name(google_protobuf_EnumDescriptorProto* msg, upb_StringView val, upb_Arena* arena) {
  return _upb_Array_Append_accessor2(msg, UPB_SIZE(24, 48), UPB_SIZE(3, 4), &val, arena);
}

/* google.protobuf.EnumDescriptorProto.EnumReservedRange */

UPB_INLINE google_protobuf_EnumDescriptorProto_EnumReservedRange* google_protobuf_EnumDescriptorProto_EnumReservedRange_new(upb_Arena* arena) {
  return (google_protobuf_EnumDescriptorProto_EnumReservedRange*)_upb_Message_New(&google_protobuf_EnumDescriptorProto_EnumReservedRange_msginit, arena);
}
UPB_INLINE google_protobuf_EnumDescriptorProto_EnumReservedRange* google_protobuf_EnumDescriptorProto_EnumReservedRange_parse(const char* buf, size_t size, upb_Arena* arena) {
  google_protobuf_EnumDescriptorProto_EnumReservedRange* ret = google_protobuf_EnumDescriptorProto_EnumReservedRange_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_EnumDescriptorProto_EnumReservedRange_msginit, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE google_protobuf_EnumDescriptorProto_EnumReservedRange* google_protobuf_EnumDescriptorProto_EnumReservedRange_parse_ex(const char* buf, size_t size,
                           const upb_ExtensionRegistry* extreg,
                           int options, upb_Arena* arena) {
  google_protobuf_EnumDescriptorProto_EnumReservedRange* ret = google_protobuf_EnumDescriptorProto_EnumReservedRange_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_EnumDescriptorProto_EnumReservedRange_msginit, extreg, options, arena) !=
      kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE char* google_protobuf_EnumDescriptorProto_EnumReservedRange_serialize(const google_protobuf_EnumDescriptorProto_EnumReservedRange* msg, upb_Arena* arena, size_t* len) {
  return upb_Encode(msg, &google_protobuf_EnumDescriptorProto_EnumReservedRange_msginit, 0, arena, len);
}
UPB_INLINE char* google_protobuf_EnumDescriptorProto_EnumReservedRange_serialize_ex(const google_protobuf_EnumDescriptorProto_EnumReservedRange* msg, int options,
                                 upb_Arena* arena, size_t* len) {
  return upb_Encode(msg, &google_protobuf_EnumDescriptorProto_EnumReservedRange_msginit, options, arena, len);
}
UPB_INLINE bool google_protobuf_EnumDescriptorProto_EnumReservedRange_has_start(const google_protobuf_EnumDescriptorProto_EnumReservedRange* msg) {
  return _upb_hasbit(msg, 1);
}
UPB_INLINE int32_t google_protobuf_EnumDescriptorProto_EnumReservedRange_start(const google_protobuf_EnumDescriptorProto_EnumReservedRange* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(4, 4), int32_t);
}
UPB_INLINE bool google_protobuf_EnumDescriptorProto_EnumReservedRange_has_end(const google_protobuf_EnumDescriptorProto_EnumReservedRange* msg) {
  return _upb_hasbit(msg, 2);
}
UPB_INLINE int32_t google_protobuf_EnumDescriptorProto_EnumReservedRange_end(const google_protobuf_EnumDescriptorProto_EnumReservedRange* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(8, 8), int32_t);
}

UPB_INLINE void google_protobuf_EnumDescriptorProto_EnumReservedRange_set_start(google_protobuf_EnumDescriptorProto_EnumReservedRange *msg, int32_t value) {
  _upb_sethas(msg, 1);
  *UPB_PTR_AT(msg, UPB_SIZE(4, 4), int32_t) = value;
}
UPB_INLINE void google_protobuf_EnumDescriptorProto_EnumReservedRange_set_end(google_protobuf_EnumDescriptorProto_EnumReservedRange *msg, int32_t value) {
  _upb_sethas(msg, 2);
  *UPB_PTR_AT(msg, UPB_SIZE(8, 8), int32_t) = value;
}

/* google.protobuf.EnumValueDescriptorProto */

UPB_INLINE google_protobuf_EnumValueDescriptorProto* google_protobuf_EnumValueDescriptorProto_new(upb_Arena* arena) {
  return (google_protobuf_EnumValueDescriptorProto*)_upb_Message_New(&google_protobuf_EnumValueDescriptorProto_msginit, arena);
}
UPB_INLINE google_protobuf_EnumValueDescriptorProto* google_protobuf_EnumValueDescriptorProto_parse(const char* buf, size_t size, upb_Arena* arena) {
  google_protobuf_EnumValueDescriptorProto* ret = google_protobuf_EnumValueDescriptorProto_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_EnumValueDescriptorProto_msginit, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE google_protobuf_EnumValueDescriptorProto* google_protobuf_EnumValueDescriptorProto_parse_ex(const char* buf, size_t size,
                           const upb_ExtensionRegistry* extreg,
                           int options, upb_Arena* arena) {
  google_protobuf_EnumValueDescriptorProto* ret = google_protobuf_EnumValueDescriptorProto_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_EnumValueDescriptorProto_msginit, extreg, options, arena) !=
      kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE char* google_protobuf_EnumValueDescriptorProto_serialize(const google_protobuf_EnumValueDescriptorProto* msg, upb_Arena* arena, size_t* len) {
  return upb_Encode(msg, &google_protobuf_EnumValueDescriptorProto_msginit, 0, arena, len);
}
UPB_INLINE char* google_protobuf_EnumValueDescriptorProto_serialize_ex(const google_protobuf_EnumValueDescriptorProto* msg, int options,
                                 upb_Arena* arena, size_t* len) {
  return upb_Encode(msg, &google_protobuf_EnumValueDescriptorProto_msginit, options, arena, len);
}
UPB_INLINE bool google_protobuf_EnumValueDescriptorProto_has_name(const google_protobuf_EnumValueDescriptorProto* msg) {
  return _upb_hasbit(msg, 1);
}
UPB_INLINE upb_StringView google_protobuf_EnumValueDescriptorProto_name(const google_protobuf_EnumValueDescriptorProto* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(8, 8), upb_StringView);
}
UPB_INLINE bool google_protobuf_EnumValueDescriptorProto_has_number(const google_protobuf_EnumValueDescriptorProto* msg) {
  return _upb_hasbit(msg, 2);
}
UPB_INLINE int32_t google_protobuf_EnumValueDescriptorProto_number(const google_protobuf_EnumValueDescriptorProto* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(4, 4), int32_t);
}
UPB_INLINE bool google_protobuf_EnumValueDescriptorProto_has_options(const google_protobuf_EnumValueDescriptorProto* msg) {
  return _upb_hasbit(msg, 3);
}
UPB_INLINE const google_protobuf_EnumValueOptions* google_protobuf_EnumValueDescriptorProto_options(const google_protobuf_EnumValueDescriptorProto* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(16, 24), const google_protobuf_EnumValueOptions*);
}

UPB_INLINE void google_protobuf_EnumValueDescriptorProto_set_name(google_protobuf_EnumValueDescriptorProto *msg, upb_StringView value) {
  _upb_sethas(msg, 1);
  *UPB_PTR_AT(msg, UPB_SIZE(8, 8), upb_StringView) = value;
}
UPB_INLINE void google_protobuf_EnumValueDescriptorProto_set_number(google_protobuf_EnumValueDescriptorProto *msg, int32_t value) {
  _upb_sethas(msg, 2);
  *UPB_PTR_AT(msg, UPB_SIZE(4, 4), int32_t) = value;
}
UPB_INLINE void google_protobuf_EnumValueDescriptorProto_set_options(google_protobuf_EnumValueDescriptorProto *msg, google_protobuf_EnumValueOptions* value) {
  _upb_sethas(msg, 3);
  *UPB_PTR_AT(msg, UPB_SIZE(16, 24), google_protobuf_EnumValueOptions*) = value;
}
UPB_INLINE struct google_protobuf_EnumValueOptions* google_protobuf_EnumValueDescriptorProto_mutable_options(google_protobuf_EnumValueDescriptorProto* msg, upb_Arena* arena) {
  struct google_protobuf_EnumValueOptions* sub = (struct google_protobuf_EnumValueOptions*)google_protobuf_EnumValueDescriptorProto_options(msg);
  if (sub == NULL) {
    sub = (struct google_protobuf_EnumValueOptions*)_upb_Message_New(&google_protobuf_EnumValueOptions_msginit, arena);
    if (!sub) return NULL;
    google_protobuf_EnumValueDescriptorProto_set_options(msg, sub);
  }
  return sub;
}

/* google.protobuf.ServiceDescriptorProto */

UPB_INLINE google_protobuf_ServiceDescriptorProto* google_protobuf_ServiceDescriptorProto_new(upb_Arena* arena) {
  return (google_protobuf_ServiceDescriptorProto*)_upb_Message_New(&google_protobuf_ServiceDescriptorProto_msginit, arena);
}
UPB_INLINE google_protobuf_ServiceDescriptorProto* google_protobuf_ServiceDescriptorProto_parse(const char* buf, size_t size, upb_Arena* arena) {
  google_protobuf_ServiceDescriptorProto* ret = google_protobuf_ServiceDescriptorProto_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_ServiceDescriptorProto_msginit, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE google_protobuf_ServiceDescriptorProto* google_protobuf_ServiceDescriptorProto_parse_ex(const char* buf, size_t size,
                           const upb_ExtensionRegistry* extreg,
                           int options, upb_Arena* arena) {
  google_protobuf_ServiceDescriptorProto* ret = google_protobuf_ServiceDescriptorProto_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_ServiceDescriptorProto_msginit, extreg, options, arena) !=
      kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE char* google_protobuf_ServiceDescriptorProto_serialize(const google_protobuf_ServiceDescriptorProto* msg, upb_Arena* arena, size_t* len) {
  return upb_Encode(msg, &google_protobuf_ServiceDescriptorProto_msginit, 0, arena, len);
}
UPB_INLINE char* google_protobuf_ServiceDescriptorProto_serialize_ex(const google_protobuf_ServiceDescriptorProto* msg, int options,
                                 upb_Arena* arena, size_t* len) {
  return upb_Encode(msg, &google_protobuf_ServiceDescriptorProto_msginit, options, arena, len);
}
UPB_INLINE bool google_protobuf_ServiceDescriptorProto_has_name(const google_protobuf_ServiceDescriptorProto* msg) {
  return _upb_hasbit(msg, 1);
}
UPB_INLINE upb_StringView google_protobuf_ServiceDescriptorProto_name(const google_protobuf_ServiceDescriptorProto* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(4, 8), upb_StringView);
}
UPB_INLINE bool google_protobuf_ServiceDescriptorProto_has_method(const google_protobuf_ServiceDescriptorProto* msg) {
  return _upb_has_submsg_nohasbit(msg, UPB_SIZE(16, 32));
}
UPB_INLINE const google_protobuf_MethodDescriptorProto* const* google_protobuf_ServiceDescriptorProto_method(const google_protobuf_ServiceDescriptorProto* msg, size_t* len) {
  return (const google_protobuf_MethodDescriptorProto* const*)_upb_array_accessor(msg, UPB_SIZE(16, 32), len);
}
UPB_INLINE bool google_protobuf_ServiceDescriptorProto_has_options(const google_protobuf_ServiceDescriptorProto* msg) {
  return _upb_hasbit(msg, 2);
}
UPB_INLINE const google_protobuf_ServiceOptions* google_protobuf_ServiceDescriptorProto_options(const google_protobuf_ServiceDescriptorProto* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(12, 24), const google_protobuf_ServiceOptions*);
}

UPB_INLINE void google_protobuf_ServiceDescriptorProto_set_name(google_protobuf_ServiceDescriptorProto *msg, upb_StringView value) {
  _upb_sethas(msg, 1);
  *UPB_PTR_AT(msg, UPB_SIZE(4, 8), upb_StringView) = value;
}
UPB_INLINE google_protobuf_MethodDescriptorProto** google_protobuf_ServiceDescriptorProto_mutable_method(google_protobuf_ServiceDescriptorProto* msg, size_t* len) {
  return (google_protobuf_MethodDescriptorProto**)_upb_array_mutable_accessor(msg, UPB_SIZE(16, 32), len);
}
UPB_INLINE google_protobuf_MethodDescriptorProto** google_protobuf_ServiceDescriptorProto_resize_method(google_protobuf_ServiceDescriptorProto* msg, size_t len, upb_Arena* arena) {
  return (google_protobuf_MethodDescriptorProto**)_upb_Array_Resize_accessor2(msg, UPB_SIZE(16, 32), len, UPB_SIZE(2, 3), arena);
}
UPB_INLINE struct google_protobuf_MethodDescriptorProto* google_protobuf_ServiceDescriptorProto_add_method(google_protobuf_ServiceDescriptorProto* msg, upb_Arena* arena) {
  struct google_protobuf_MethodDescriptorProto* sub = (struct google_protobuf_MethodDescriptorProto*)_upb_Message_New(&google_protobuf_MethodDescriptorProto_msginit, arena);
  bool ok = _upb_Array_Append_accessor2(msg, UPB_SIZE(16, 32), UPB_SIZE(2, 3), &sub, arena);
  if (!ok) return NULL;
  return sub;
}
UPB_INLINE void google_protobuf_ServiceDescriptorProto_set_options(google_protobuf_ServiceDescriptorProto *msg, google_protobuf_ServiceOptions* value) {
  _upb_sethas(msg, 2);
  *UPB_PTR_AT(msg, UPB_SIZE(12, 24), google_protobuf_ServiceOptions*) = value;
}
UPB_INLINE struct google_protobuf_ServiceOptions* google_protobuf_ServiceDescriptorProto_mutable_options(google_protobuf_ServiceDescriptorProto* msg, upb_Arena* arena) {
  struct google_protobuf_ServiceOptions* sub = (struct google_protobuf_ServiceOptions*)google_protobuf_ServiceDescriptorProto_options(msg);
  if (sub == NULL) {
    sub = (struct google_protobuf_ServiceOptions*)_upb_Message_New(&google_protobuf_ServiceOptions_msginit, arena);
    if (!sub) return NULL;
    google_protobuf_ServiceDescriptorProto_set_options(msg, sub);
  }
  return sub;
}

/* google.protobuf.MethodDescriptorProto */

UPB_INLINE google_protobuf_MethodDescriptorProto* google_protobuf_MethodDescriptorProto_new(upb_Arena* arena) {
  return (google_protobuf_MethodDescriptorProto*)_upb_Message_New(&google_protobuf_MethodDescriptorProto_msginit, arena);
}
UPB_INLINE google_protobuf_MethodDescriptorProto* google_protobuf_MethodDescriptorProto_parse(const char* buf, size_t size, upb_Arena* arena) {
  google_protobuf_MethodDescriptorProto* ret = google_protobuf_MethodDescriptorProto_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_MethodDescriptorProto_msginit, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE google_protobuf_MethodDescriptorProto* google_protobuf_MethodDescriptorProto_parse_ex(const char* buf, size_t size,
                           const upb_ExtensionRegistry* extreg,
                           int options, upb_Arena* arena) {
  google_protobuf_MethodDescriptorProto* ret = google_protobuf_MethodDescriptorProto_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_MethodDescriptorProto_msginit, extreg, options, arena) !=
      kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE char* google_protobuf_MethodDescriptorProto_serialize(const google_protobuf_MethodDescriptorProto* msg, upb_Arena* arena, size_t* len) {
  return upb_Encode(msg, &google_protobuf_MethodDescriptorProto_msginit, 0, arena, len);
}
UPB_INLINE char* google_protobuf_MethodDescriptorProto_serialize_ex(const google_protobuf_MethodDescriptorProto* msg, int options,
                                 upb_Arena* arena, size_t* len) {
  return upb_Encode(msg, &google_protobuf_MethodDescriptorProto_msginit, options, arena, len);
}
UPB_INLINE bool google_protobuf_MethodDescriptorProto_has_name(const google_protobuf_MethodDescriptorProto* msg) {
  return _upb_hasbit(msg, 1);
}
UPB_INLINE upb_StringView google_protobuf_MethodDescriptorProto_name(const google_protobuf_MethodDescriptorProto* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(4, 8), upb_StringView);
}
UPB_INLINE bool google_protobuf_MethodDescriptorProto_has_input_type(const google_protobuf_MethodDescriptorProto* msg) {
  return _upb_hasbit(msg, 2);
}
UPB_INLINE upb_StringView google_protobuf_MethodDescriptorProto_input_type(const google_protobuf_MethodDescriptorProto* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(12, 24), upb_StringView);
}
UPB_INLINE bool google_protobuf_MethodDescriptorProto_has_output_type(const google_protobuf_MethodDescriptorProto* msg) {
  return _upb_hasbit(msg, 3);
}
UPB_INLINE upb_StringView google_protobuf_MethodDescriptorProto_output_type(const google_protobuf_MethodDescriptorProto* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(20, 40), upb_StringView);
}
UPB_INLINE bool google_protobuf_MethodDescriptorProto_has_options(const google_protobuf_MethodDescriptorProto* msg) {
  return _upb_hasbit(msg, 4);
}
UPB_INLINE const google_protobuf_MethodOptions* google_protobuf_MethodDescriptorProto_options(const google_protobuf_MethodDescriptorProto* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(28, 56), const google_protobuf_MethodOptions*);
}
UPB_INLINE bool google_protobuf_MethodDescriptorProto_has_client_streaming(const google_protobuf_MethodDescriptorProto* msg) {
  return _upb_hasbit(msg, 5);
}
UPB_INLINE bool google_protobuf_MethodDescriptorProto_client_streaming(const google_protobuf_MethodDescriptorProto* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(1, 1), bool);
}
UPB_INLINE bool google_protobuf_MethodDescriptorProto_has_server_streaming(const google_protobuf_MethodDescriptorProto* msg) {
  return _upb_hasbit(msg, 6);
}
UPB_INLINE bool google_protobuf_MethodDescriptorProto_server_streaming(const google_protobuf_MethodDescriptorProto* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(2, 2), bool);
}

UPB_INLINE void google_protobuf_MethodDescriptorProto_set_name(google_protobuf_MethodDescriptorProto *msg, upb_StringView value) {
  _upb_sethas(msg, 1);
  *UPB_PTR_AT(msg, UPB_SIZE(4, 8), upb_StringView) = value;
}
UPB_INLINE void google_protobuf_MethodDescriptorProto_set_input_type(google_protobuf_MethodDescriptorProto *msg, upb_StringView value) {
  _upb_sethas(msg, 2);
  *UPB_PTR_AT(msg, UPB_SIZE(12, 24), upb_StringView) = value;
}
UPB_INLINE void google_protobuf_MethodDescriptorProto_set_output_type(google_protobuf_MethodDescriptorProto *msg, upb_StringView value) {
  _upb_sethas(msg, 3);
  *UPB_PTR_AT(msg, UPB_SIZE(20, 40), upb_StringView) = value;
}
UPB_INLINE void google_protobuf_MethodDescriptorProto_set_options(google_protobuf_MethodDescriptorProto *msg, google_protobuf_MethodOptions* value) {
  _upb_sethas(msg, 4);
  *UPB_PTR_AT(msg, UPB_SIZE(28, 56), google_protobuf_MethodOptions*) = value;
}
UPB_INLINE struct google_protobuf_MethodOptions* google_protobuf_MethodDescriptorProto_mutable_options(google_protobuf_MethodDescriptorProto* msg, upb_Arena* arena) {
  struct google_protobuf_MethodOptions* sub = (struct google_protobuf_MethodOptions*)google_protobuf_MethodDescriptorProto_options(msg);
  if (sub == NULL) {
    sub = (struct google_protobuf_MethodOptions*)_upb_Message_New(&google_protobuf_MethodOptions_msginit, arena);
    if (!sub) return NULL;
    google_protobuf_MethodDescriptorProto_set_options(msg, sub);
  }
  return sub;
}
UPB_INLINE void google_protobuf_MethodDescriptorProto_set_client_streaming(google_protobuf_MethodDescriptorProto *msg, bool value) {
  _upb_sethas(msg, 5);
  *UPB_PTR_AT(msg, UPB_SIZE(1, 1), bool) = value;
}
UPB_INLINE void google_protobuf_MethodDescriptorProto_set_server_streaming(google_protobuf_MethodDescriptorProto *msg, bool value) {
  _upb_sethas(msg, 6);
  *UPB_PTR_AT(msg, UPB_SIZE(2, 2), bool) = value;
}

/* google.protobuf.FileOptions */

UPB_INLINE google_protobuf_FileOptions* google_protobuf_FileOptions_new(upb_Arena* arena) {
  return (google_protobuf_FileOptions*)_upb_Message_New(&google_protobuf_FileOptions_msginit, arena);
}
UPB_INLINE google_protobuf_FileOptions* google_protobuf_FileOptions_parse(const char* buf, size_t size, upb_Arena* arena) {
  google_protobuf_FileOptions* ret = google_protobuf_FileOptions_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_FileOptions_msginit, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE google_protobuf_FileOptions* google_protobuf_FileOptions_parse_ex(const char* buf, size_t size,
                           const upb_ExtensionRegistry* extreg,
                           int options, upb_Arena* arena) {
  google_protobuf_FileOptions* ret = google_protobuf_FileOptions_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_FileOptions_msginit, extreg, options, arena) !=
      kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE char* google_protobuf_FileOptions_serialize(const google_protobuf_FileOptions* msg, upb_Arena* arena, size_t* len) {
  return upb_Encode(msg, &google_protobuf_FileOptions_msginit, 0, arena, len);
}
UPB_INLINE char* google_protobuf_FileOptions_serialize_ex(const google_protobuf_FileOptions* msg, int options,
                                 upb_Arena* arena, size_t* len) {
  return upb_Encode(msg, &google_protobuf_FileOptions_msginit, options, arena, len);
}
UPB_INLINE bool google_protobuf_FileOptions_has_java_package(const google_protobuf_FileOptions* msg) {
  return _upb_hasbit(msg, 1);
}
UPB_INLINE upb_StringView google_protobuf_FileOptions_java_package(const google_protobuf_FileOptions* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(20, 24), upb_StringView);
}
UPB_INLINE bool google_protobuf_FileOptions_has_java_outer_classname(const google_protobuf_FileOptions* msg) {
  return _upb_hasbit(msg, 2);
}
UPB_INLINE upb_StringView google_protobuf_FileOptions_java_outer_classname(const google_protobuf_FileOptions* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(28, 40), upb_StringView);
}
UPB_INLINE bool google_protobuf_FileOptions_has_optimize_for(const google_protobuf_FileOptions* msg) {
  return _upb_hasbit(msg, 3);
}
UPB_INLINE int32_t google_protobuf_FileOptions_optimize_for(const google_protobuf_FileOptions* msg) {
  return google_protobuf_FileOptions_has_optimize_for(msg) ? *UPB_PTR_AT(msg, UPB_SIZE(4, 4), int32_t) : 1;
}
UPB_INLINE bool google_protobuf_FileOptions_has_java_multiple_files(const google_protobuf_FileOptions* msg) {
  return _upb_hasbit(msg, 4);
}
UPB_INLINE bool google_protobuf_FileOptions_java_multiple_files(const google_protobuf_FileOptions* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(8, 8), bool);
}
UPB_INLINE bool google_protobuf_FileOptions_has_go_package(const google_protobuf_FileOptions* msg) {
  return _upb_hasbit(msg, 5);
}
UPB_INLINE upb_StringView google_protobuf_FileOptions_go_package(const google_protobuf_FileOptions* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(36, 56), upb_StringView);
}
UPB_INLINE bool google_protobuf_FileOptions_has_cc_generic_services(const google_protobuf_FileOptions* msg) {
  return _upb_hasbit(msg, 6);
}
UPB_INLINE bool google_protobuf_FileOptions_cc_generic_services(const google_protobuf_FileOptions* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(9, 9), bool);
}
UPB_INLINE bool google_protobuf_FileOptions_has_java_generic_services(const google_protobuf_FileOptions* msg) {
  return _upb_hasbit(msg, 7);
}
UPB_INLINE bool google_protobuf_FileOptions_java_generic_services(const google_protobuf_FileOptions* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(10, 10), bool);
}
UPB_INLINE bool google_protobuf_FileOptions_has_py_generic_services(const google_protobuf_FileOptions* msg) {
  return _upb_hasbit(msg, 8);
}
UPB_INLINE bool google_protobuf_FileOptions_py_generic_services(const google_protobuf_FileOptions* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(11, 11), bool);
}
UPB_INLINE bool google_protobuf_FileOptions_has_java_generate_equals_and_hash(const google_protobuf_FileOptions* msg) {
  return _upb_hasbit(msg, 9);
}
UPB_INLINE bool google_protobuf_FileOptions_java_generate_equals_and_hash(const google_protobuf_FileOptions* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(12, 12), bool);
}
UPB_INLINE bool google_protobuf_FileOptions_has_deprecated(const google_protobuf_FileOptions* msg) {
  return _upb_hasbit(msg, 10);
}
UPB_INLINE bool google_protobuf_FileOptions_deprecated(const google_protobuf_FileOptions* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(13, 13), bool);
}
UPB_INLINE bool google_protobuf_FileOptions_has_java_string_check_utf8(const google_protobuf_FileOptions* msg) {
  return _upb_hasbit(msg, 11);
}
UPB_INLINE bool google_protobuf_FileOptions_java_string_check_utf8(const google_protobuf_FileOptions* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(14, 14), bool);
}
UPB_INLINE bool google_protobuf_FileOptions_has_cc_enable_arenas(const google_protobuf_FileOptions* msg) {
  return _upb_hasbit(msg, 12);
}
UPB_INLINE bool google_protobuf_FileOptions_cc_enable_arenas(const google_protobuf_FileOptions* msg) {
  return google_protobuf_FileOptions_has_cc_enable_arenas(msg) ? *UPB_PTR_AT(msg, UPB_SIZE(15, 15), bool) : true;
}
UPB_INLINE bool google_protobuf_FileOptions_has_objc_class_prefix(const google_protobuf_FileOptions* msg) {
  return _upb_hasbit(msg, 13);
}
UPB_INLINE upb_StringView google_protobuf_FileOptions_objc_class_prefix(const google_protobuf_FileOptions* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(44, 72), upb_StringView);
}
UPB_INLINE bool google_protobuf_FileOptions_has_csharp_namespace(const google_protobuf_FileOptions* msg) {
  return _upb_hasbit(msg, 14);
}
UPB_INLINE upb_StringView google_protobuf_FileOptions_csharp_namespace(const google_protobuf_FileOptions* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(52, 88), upb_StringView);
}
UPB_INLINE bool google_protobuf_FileOptions_has_swift_prefix(const google_protobuf_FileOptions* msg) {
  return _upb_hasbit(msg, 15);
}
UPB_INLINE upb_StringView google_protobuf_FileOptions_swift_prefix(const google_protobuf_FileOptions* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(60, 104), upb_StringView);
}
UPB_INLINE bool google_protobuf_FileOptions_has_php_class_prefix(const google_protobuf_FileOptions* msg) {
  return _upb_hasbit(msg, 16);
}
UPB_INLINE upb_StringView google_protobuf_FileOptions_php_class_prefix(const google_protobuf_FileOptions* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(68, 120), upb_StringView);
}
UPB_INLINE bool google_protobuf_FileOptions_has_php_namespace(const google_protobuf_FileOptions* msg) {
  return _upb_hasbit(msg, 17);
}
UPB_INLINE upb_StringView google_protobuf_FileOptions_php_namespace(const google_protobuf_FileOptions* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(76, 136), upb_StringView);
}
UPB_INLINE bool google_protobuf_FileOptions_has_php_generic_services(const google_protobuf_FileOptions* msg) {
  return _upb_hasbit(msg, 18);
}
UPB_INLINE bool google_protobuf_FileOptions_php_generic_services(const google_protobuf_FileOptions* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(16, 16), bool);
}
UPB_INLINE bool google_protobuf_FileOptions_has_php_metadata_namespace(const google_protobuf_FileOptions* msg) {
  return _upb_hasbit(msg, 19);
}
UPB_INLINE upb_StringView google_protobuf_FileOptions_php_metadata_namespace(const google_protobuf_FileOptions* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(84, 152), upb_StringView);
}
UPB_INLINE bool google_protobuf_FileOptions_has_ruby_package(const google_protobuf_FileOptions* msg) {
  return _upb_hasbit(msg, 20);
}
UPB_INLINE upb_StringView google_protobuf_FileOptions_ruby_package(const google_protobuf_FileOptions* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(92, 168), upb_StringView);
}
UPB_INLINE bool google_protobuf_FileOptions_has_uninterpreted_option(const google_protobuf_FileOptions* msg) {
  return _upb_has_submsg_nohasbit(msg, UPB_SIZE(100, 184));
}
UPB_INLINE const google_protobuf_UninterpretedOption* const* google_protobuf_FileOptions_uninterpreted_option(const google_protobuf_FileOptions* msg, size_t* len) {
  return (const google_protobuf_UninterpretedOption* const*)_upb_array_accessor(msg, UPB_SIZE(100, 184), len);
}

UPB_INLINE void google_protobuf_FileOptions_set_java_package(google_protobuf_FileOptions *msg, upb_StringView value) {
  _upb_sethas(msg, 1);
  *UPB_PTR_AT(msg, UPB_SIZE(20, 24), upb_StringView) = value;
}
UPB_INLINE void google_protobuf_FileOptions_set_java_outer_classname(google_protobuf_FileOptions *msg, upb_StringView value) {
  _upb_sethas(msg, 2);
  *UPB_PTR_AT(msg, UPB_SIZE(28, 40), upb_StringView) = value;
}
UPB_INLINE void google_protobuf_FileOptions_set_optimize_for(google_protobuf_FileOptions *msg, int32_t value) {
  _upb_sethas(msg, 3);
  *UPB_PTR_AT(msg, UPB_SIZE(4, 4), int32_t) = value;
}
UPB_INLINE void google_protobuf_FileOptions_set_java_multiple_files(google_protobuf_FileOptions *msg, bool value) {
  _upb_sethas(msg, 4);
  *UPB_PTR_AT(msg, UPB_SIZE(8, 8), bool) = value;
}
UPB_INLINE void google_protobuf_FileOptions_set_go_package(google_protobuf_FileOptions *msg, upb_StringView value) {
  _upb_sethas(msg, 5);
  *UPB_PTR_AT(msg, UPB_SIZE(36, 56), upb_StringView) = value;
}
UPB_INLINE void google_protobuf_FileOptions_set_cc_generic_services(google_protobuf_FileOptions *msg, bool value) {
  _upb_sethas(msg, 6);
  *UPB_PTR_AT(msg, UPB_SIZE(9, 9), bool) = value;
}
UPB_INLINE void google_protobuf_FileOptions_set_java_generic_services(google_protobuf_FileOptions *msg, bool value) {
  _upb_sethas(msg, 7);
  *UPB_PTR_AT(msg, UPB_SIZE(10, 10), bool) = value;
}
UPB_INLINE void google_protobuf_FileOptions_set_py_generic_services(google_protobuf_FileOptions *msg, bool value) {
  _upb_sethas(msg, 8);
  *UPB_PTR_AT(msg, UPB_SIZE(11, 11), bool) = value;
}
UPB_INLINE void google_protobuf_FileOptions_set_java_generate_equals_and_hash(google_protobuf_FileOptions *msg, bool value) {
  _upb_sethas(msg, 9);
  *UPB_PTR_AT(msg, UPB_SIZE(12, 12), bool) = value;
}
UPB_INLINE void google_protobuf_FileOptions_set_deprecated(google_protobuf_FileOptions *msg, bool value) {
  _upb_sethas(msg, 10);
  *UPB_PTR_AT(msg, UPB_SIZE(13, 13), bool) = value;
}
UPB_INLINE void google_protobuf_FileOptions_set_java_string_check_utf8(google_protobuf_FileOptions *msg, bool value) {
  _upb_sethas(msg, 11);
  *UPB_PTR_AT(msg, UPB_SIZE(14, 14), bool) = value;
}
UPB_INLINE void google_protobuf_FileOptions_set_cc_enable_arenas(google_protobuf_FileOptions *msg, bool value) {
  _upb_sethas(msg, 12);
  *UPB_PTR_AT(msg, UPB_SIZE(15, 15), bool) = value;
}
UPB_INLINE void google_protobuf_FileOptions_set_objc_class_prefix(google_protobuf_FileOptions *msg, upb_StringView value) {
  _upb_sethas(msg, 13);
  *UPB_PTR_AT(msg, UPB_SIZE(44, 72), upb_StringView) = value;
}
UPB_INLINE void google_protobuf_FileOptions_set_csharp_namespace(google_protobuf_FileOptions *msg, upb_StringView value) {
  _upb_sethas(msg, 14);
  *UPB_PTR_AT(msg, UPB_SIZE(52, 88), upb_StringView) = value;
}
UPB_INLINE void google_protobuf_FileOptions_set_swift_prefix(google_protobuf_FileOptions *msg, upb_StringView value) {
  _upb_sethas(msg, 15);
  *UPB_PTR_AT(msg, UPB_SIZE(60, 104), upb_StringView) = value;
}
UPB_INLINE void google_protobuf_FileOptions_set_php_class_prefix(google_protobuf_FileOptions *msg, upb_StringView value) {
  _upb_sethas(msg, 16);
  *UPB_PTR_AT(msg, UPB_SIZE(68, 120), upb_StringView) = value;
}
UPB_INLINE void google_protobuf_FileOptions_set_php_namespace(google_protobuf_FileOptions *msg, upb_StringView value) {
  _upb_sethas(msg, 17);
  *UPB_PTR_AT(msg, UPB_SIZE(76, 136), upb_StringView) = value;
}
UPB_INLINE void google_protobuf_FileOptions_set_php_generic_services(google_protobuf_FileOptions *msg, bool value) {
  _upb_sethas(msg, 18);
  *UPB_PTR_AT(msg, UPB_SIZE(16, 16), bool) = value;
}
UPB_INLINE void google_protobuf_FileOptions_set_php_metadata_namespace(google_protobuf_FileOptions *msg, upb_StringView value) {
  _upb_sethas(msg, 19);
  *UPB_PTR_AT(msg, UPB_SIZE(84, 152), upb_StringView) = value;
}
UPB_INLINE void google_protobuf_FileOptions_set_ruby_package(google_protobuf_FileOptions *msg, upb_StringView value) {
  _upb_sethas(msg, 20);
  *UPB_PTR_AT(msg, UPB_SIZE(92, 168), upb_StringView) = value;
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_FileOptions_mutable_uninterpreted_option(google_protobuf_FileOptions* msg, size_t* len) {
  return (google_protobuf_UninterpretedOption**)_upb_array_mutable_accessor(msg, UPB_SIZE(100, 184), len);
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_FileOptions_resize_uninterpreted_option(google_protobuf_FileOptions* msg, size_t len, upb_Arena* arena) {
  return (google_protobuf_UninterpretedOption**)_upb_Array_Resize_accessor2(msg, UPB_SIZE(100, 184), len, UPB_SIZE(2, 3), arena);
}
UPB_INLINE struct google_protobuf_UninterpretedOption* google_protobuf_FileOptions_add_uninterpreted_option(google_protobuf_FileOptions* msg, upb_Arena* arena) {
  struct google_protobuf_UninterpretedOption* sub = (struct google_protobuf_UninterpretedOption*)_upb_Message_New(&google_protobuf_UninterpretedOption_msginit, arena);
  bool ok = _upb_Array_Append_accessor2(msg, UPB_SIZE(100, 184), UPB_SIZE(2, 3), &sub, arena);
  if (!ok) return NULL;
  return sub;
}

/* google.protobuf.MessageOptions */

UPB_INLINE google_protobuf_MessageOptions* google_protobuf_MessageOptions_new(upb_Arena* arena) {
  return (google_protobuf_MessageOptions*)_upb_Message_New(&google_protobuf_MessageOptions_msginit, arena);
}
UPB_INLINE google_protobuf_MessageOptions* google_protobuf_MessageOptions_parse(const char* buf, size_t size, upb_Arena* arena) {
  google_protobuf_MessageOptions* ret = google_protobuf_MessageOptions_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_MessageOptions_msginit, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE google_protobuf_MessageOptions* google_protobuf_MessageOptions_parse_ex(const char* buf, size_t size,
                           const upb_ExtensionRegistry* extreg,
                           int options, upb_Arena* arena) {
  google_protobuf_MessageOptions* ret = google_protobuf_MessageOptions_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_MessageOptions_msginit, extreg, options, arena) !=
      kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE char* google_protobuf_MessageOptions_serialize(const google_protobuf_MessageOptions* msg, upb_Arena* arena, size_t* len) {
  return upb_Encode(msg, &google_protobuf_MessageOptions_msginit, 0, arena, len);
}
UPB_INLINE char* google_protobuf_MessageOptions_serialize_ex(const google_protobuf_MessageOptions* msg, int options,
                                 upb_Arena* arena, size_t* len) {
  return upb_Encode(msg, &google_protobuf_MessageOptions_msginit, options, arena, len);
}
UPB_INLINE bool google_protobuf_MessageOptions_has_message_set_wire_format(const google_protobuf_MessageOptions* msg) {
  return _upb_hasbit(msg, 1);
}
UPB_INLINE bool google_protobuf_MessageOptions_message_set_wire_format(const google_protobuf_MessageOptions* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(1, 1), bool);
}
UPB_INLINE bool google_protobuf_MessageOptions_has_no_standard_descriptor_accessor(const google_protobuf_MessageOptions* msg) {
  return _upb_hasbit(msg, 2);
}
UPB_INLINE bool google_protobuf_MessageOptions_no_standard_descriptor_accessor(const google_protobuf_MessageOptions* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(2, 2), bool);
}
UPB_INLINE bool google_protobuf_MessageOptions_has_deprecated(const google_protobuf_MessageOptions* msg) {
  return _upb_hasbit(msg, 3);
}
UPB_INLINE bool google_protobuf_MessageOptions_deprecated(const google_protobuf_MessageOptions* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(3, 3), bool);
}
UPB_INLINE bool google_protobuf_MessageOptions_has_map_entry(const google_protobuf_MessageOptions* msg) {
  return _upb_hasbit(msg, 4);
}
UPB_INLINE bool google_protobuf_MessageOptions_map_entry(const google_protobuf_MessageOptions* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(4, 4), bool);
}
UPB_INLINE bool google_protobuf_MessageOptions_has_uninterpreted_option(const google_protobuf_MessageOptions* msg) {
  return _upb_has_submsg_nohasbit(msg, UPB_SIZE(8, 8));
}
UPB_INLINE const google_protobuf_UninterpretedOption* const* google_protobuf_MessageOptions_uninterpreted_option(const google_protobuf_MessageOptions* msg, size_t* len) {
  return (const google_protobuf_UninterpretedOption* const*)_upb_array_accessor(msg, UPB_SIZE(8, 8), len);
}

UPB_INLINE void google_protobuf_MessageOptions_set_message_set_wire_format(google_protobuf_MessageOptions *msg, bool value) {
  _upb_sethas(msg, 1);
  *UPB_PTR_AT(msg, UPB_SIZE(1, 1), bool) = value;
}
UPB_INLINE void google_protobuf_MessageOptions_set_no_standard_descriptor_accessor(google_protobuf_MessageOptions *msg, bool value) {
  _upb_sethas(msg, 2);
  *UPB_PTR_AT(msg, UPB_SIZE(2, 2), bool) = value;
}
UPB_INLINE void google_protobuf_MessageOptions_set_deprecated(google_protobuf_MessageOptions *msg, bool value) {
  _upb_sethas(msg, 3);
  *UPB_PTR_AT(msg, UPB_SIZE(3, 3), bool) = value;
}
UPB_INLINE void google_protobuf_MessageOptions_set_map_entry(google_protobuf_MessageOptions *msg, bool value) {
  _upb_sethas(msg, 4);
  *UPB_PTR_AT(msg, UPB_SIZE(4, 4), bool) = value;
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_MessageOptions_mutable_uninterpreted_option(google_protobuf_MessageOptions* msg, size_t* len) {
  return (google_protobuf_UninterpretedOption**)_upb_array_mutable_accessor(msg, UPB_SIZE(8, 8), len);
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_MessageOptions_resize_uninterpreted_option(google_protobuf_MessageOptions* msg, size_t len, upb_Arena* arena) {
  return (google_protobuf_UninterpretedOption**)_upb_Array_Resize_accessor2(msg, UPB_SIZE(8, 8), len, UPB_SIZE(2, 3), arena);
}
UPB_INLINE struct google_protobuf_UninterpretedOption* google_protobuf_MessageOptions_add_uninterpreted_option(google_protobuf_MessageOptions* msg, upb_Arena* arena) {
  struct google_protobuf_UninterpretedOption* sub = (struct google_protobuf_UninterpretedOption*)_upb_Message_New(&google_protobuf_UninterpretedOption_msginit, arena);
  bool ok = _upb_Array_Append_accessor2(msg, UPB_SIZE(8, 8), UPB_SIZE(2, 3), &sub, arena);
  if (!ok) return NULL;
  return sub;
}

/* google.protobuf.FieldOptions */

UPB_INLINE google_protobuf_FieldOptions* google_protobuf_FieldOptions_new(upb_Arena* arena) {
  return (google_protobuf_FieldOptions*)_upb_Message_New(&google_protobuf_FieldOptions_msginit, arena);
}
UPB_INLINE google_protobuf_FieldOptions* google_protobuf_FieldOptions_parse(const char* buf, size_t size, upb_Arena* arena) {
  google_protobuf_FieldOptions* ret = google_protobuf_FieldOptions_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_FieldOptions_msginit, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE google_protobuf_FieldOptions* google_protobuf_FieldOptions_parse_ex(const char* buf, size_t size,
                           const upb_ExtensionRegistry* extreg,
                           int options, upb_Arena* arena) {
  google_protobuf_FieldOptions* ret = google_protobuf_FieldOptions_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_FieldOptions_msginit, extreg, options, arena) !=
      kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE char* google_protobuf_FieldOptions_serialize(const google_protobuf_FieldOptions* msg, upb_Arena* arena, size_t* len) {
  return upb_Encode(msg, &google_protobuf_FieldOptions_msginit, 0, arena, len);
}
UPB_INLINE char* google_protobuf_FieldOptions_serialize_ex(const google_protobuf_FieldOptions* msg, int options,
                                 upb_Arena* arena, size_t* len) {
  return upb_Encode(msg, &google_protobuf_FieldOptions_msginit, options, arena, len);
}
UPB_INLINE bool google_protobuf_FieldOptions_has_ctype(const google_protobuf_FieldOptions* msg) {
  return _upb_hasbit(msg, 1);
}
UPB_INLINE int32_t google_protobuf_FieldOptions_ctype(const google_protobuf_FieldOptions* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(4, 4), int32_t);
}
UPB_INLINE bool google_protobuf_FieldOptions_has_packed(const google_protobuf_FieldOptions* msg) {
  return _upb_hasbit(msg, 2);
}
UPB_INLINE bool google_protobuf_FieldOptions_packed(const google_protobuf_FieldOptions* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(12, 12), bool);
}
UPB_INLINE bool google_protobuf_FieldOptions_has_deprecated(const google_protobuf_FieldOptions* msg) {
  return _upb_hasbit(msg, 3);
}
UPB_INLINE bool google_protobuf_FieldOptions_deprecated(const google_protobuf_FieldOptions* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(13, 13), bool);
}
UPB_INLINE bool google_protobuf_FieldOptions_has_lazy(const google_protobuf_FieldOptions* msg) {
  return _upb_hasbit(msg, 4);
}
UPB_INLINE bool google_protobuf_FieldOptions_lazy(const google_protobuf_FieldOptions* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(14, 14), bool);
}
UPB_INLINE bool google_protobuf_FieldOptions_has_jstype(const google_protobuf_FieldOptions* msg) {
  return _upb_hasbit(msg, 5);
}
UPB_INLINE int32_t google_protobuf_FieldOptions_jstype(const google_protobuf_FieldOptions* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(8, 8), int32_t);
}
UPB_INLINE bool google_protobuf_FieldOptions_has_weak(const google_protobuf_FieldOptions* msg) {
  return _upb_hasbit(msg, 6);
}
UPB_INLINE bool google_protobuf_FieldOptions_weak(const google_protobuf_FieldOptions* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(15, 15), bool);
}
UPB_INLINE bool google_protobuf_FieldOptions_has_unverified_lazy(const google_protobuf_FieldOptions* msg) {
  return _upb_hasbit(msg, 7);
}
UPB_INLINE bool google_protobuf_FieldOptions_unverified_lazy(const google_protobuf_FieldOptions* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(16, 16), bool);
}
UPB_INLINE bool google_protobuf_FieldOptions_has_uninterpreted_option(const google_protobuf_FieldOptions* msg) {
  return _upb_has_submsg_nohasbit(msg, UPB_SIZE(20, 24));
}
UPB_INLINE const google_protobuf_UninterpretedOption* const* google_protobuf_FieldOptions_uninterpreted_option(const google_protobuf_FieldOptions* msg, size_t* len) {
  return (const google_protobuf_UninterpretedOption* const*)_upb_array_accessor(msg, UPB_SIZE(20, 24), len);
}

UPB_INLINE void google_protobuf_FieldOptions_set_ctype(google_protobuf_FieldOptions *msg, int32_t value) {
  _upb_sethas(msg, 1);
  *UPB_PTR_AT(msg, UPB_SIZE(4, 4), int32_t) = value;
}
UPB_INLINE void google_protobuf_FieldOptions_set_packed(google_protobuf_FieldOptions *msg, bool value) {
  _upb_sethas(msg, 2);
  *UPB_PTR_AT(msg, UPB_SIZE(12, 12), bool) = value;
}
UPB_INLINE void google_protobuf_FieldOptions_set_deprecated(google_protobuf_FieldOptions *msg, bool value) {
  _upb_sethas(msg, 3);
  *UPB_PTR_AT(msg, UPB_SIZE(13, 13), bool) = value;
}
UPB_INLINE void google_protobuf_FieldOptions_set_lazy(google_protobuf_FieldOptions *msg, bool value) {
  _upb_sethas(msg, 4);
  *UPB_PTR_AT(msg, UPB_SIZE(14, 14), bool) = value;
}
UPB_INLINE void google_protobuf_FieldOptions_set_jstype(google_protobuf_FieldOptions *msg, int32_t value) {
  _upb_sethas(msg, 5);
  *UPB_PTR_AT(msg, UPB_SIZE(8, 8), int32_t) = value;
}
UPB_INLINE void google_protobuf_FieldOptions_set_weak(google_protobuf_FieldOptions *msg, bool value) {
  _upb_sethas(msg, 6);
  *UPB_PTR_AT(msg, UPB_SIZE(15, 15), bool) = value;
}
UPB_INLINE void google_protobuf_FieldOptions_set_unverified_lazy(google_protobuf_FieldOptions *msg, bool value) {
  _upb_sethas(msg, 7);
  *UPB_PTR_AT(msg, UPB_SIZE(16, 16), bool) = value;
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_FieldOptions_mutable_uninterpreted_option(google_protobuf_FieldOptions* msg, size_t* len) {
  return (google_protobuf_UninterpretedOption**)_upb_array_mutable_accessor(msg, UPB_SIZE(20, 24), len);
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_FieldOptions_resize_uninterpreted_option(google_protobuf_FieldOptions* msg, size_t len, upb_Arena* arena) {
  return (google_protobuf_UninterpretedOption**)_upb_Array_Resize_accessor2(msg, UPB_SIZE(20, 24), len, UPB_SIZE(2, 3), arena);
}
UPB_INLINE struct google_protobuf_UninterpretedOption* google_protobuf_FieldOptions_add_uninterpreted_option(google_protobuf_FieldOptions* msg, upb_Arena* arena) {
  struct google_protobuf_UninterpretedOption* sub = (struct google_protobuf_UninterpretedOption*)_upb_Message_New(&google_protobuf_UninterpretedOption_msginit, arena);
  bool ok = _upb_Array_Append_accessor2(msg, UPB_SIZE(20, 24), UPB_SIZE(2, 3), &sub, arena);
  if (!ok) return NULL;
  return sub;
}

/* google.protobuf.OneofOptions */

UPB_INLINE google_protobuf_OneofOptions* google_protobuf_OneofOptions_new(upb_Arena* arena) {
  return (google_protobuf_OneofOptions*)_upb_Message_New(&google_protobuf_OneofOptions_msginit, arena);
}
UPB_INLINE google_protobuf_OneofOptions* google_protobuf_OneofOptions_parse(const char* buf, size_t size, upb_Arena* arena) {
  google_protobuf_OneofOptions* ret = google_protobuf_OneofOptions_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_OneofOptions_msginit, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE google_protobuf_OneofOptions* google_protobuf_OneofOptions_parse_ex(const char* buf, size_t size,
                           const upb_ExtensionRegistry* extreg,
                           int options, upb_Arena* arena) {
  google_protobuf_OneofOptions* ret = google_protobuf_OneofOptions_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_OneofOptions_msginit, extreg, options, arena) !=
      kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE char* google_protobuf_OneofOptions_serialize(const google_protobuf_OneofOptions* msg, upb_Arena* arena, size_t* len) {
  return upb_Encode(msg, &google_protobuf_OneofOptions_msginit, 0, arena, len);
}
UPB_INLINE char* google_protobuf_OneofOptions_serialize_ex(const google_protobuf_OneofOptions* msg, int options,
                                 upb_Arena* arena, size_t* len) {
  return upb_Encode(msg, &google_protobuf_OneofOptions_msginit, options, arena, len);
}
UPB_INLINE bool google_protobuf_OneofOptions_has_uninterpreted_option(const google_protobuf_OneofOptions* msg) {
  return _upb_has_submsg_nohasbit(msg, UPB_SIZE(0, 0));
}
UPB_INLINE const google_protobuf_UninterpretedOption* const* google_protobuf_OneofOptions_uninterpreted_option(const google_protobuf_OneofOptions* msg, size_t* len) {
  return (const google_protobuf_UninterpretedOption* const*)_upb_array_accessor(msg, UPB_SIZE(0, 0), len);
}

UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_OneofOptions_mutable_uninterpreted_option(google_protobuf_OneofOptions* msg, size_t* len) {
  return (google_protobuf_UninterpretedOption**)_upb_array_mutable_accessor(msg, UPB_SIZE(0, 0), len);
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_OneofOptions_resize_uninterpreted_option(google_protobuf_OneofOptions* msg, size_t len, upb_Arena* arena) {
  return (google_protobuf_UninterpretedOption**)_upb_Array_Resize_accessor2(msg, UPB_SIZE(0, 0), len, UPB_SIZE(2, 3), arena);
}
UPB_INLINE struct google_protobuf_UninterpretedOption* google_protobuf_OneofOptions_add_uninterpreted_option(google_protobuf_OneofOptions* msg, upb_Arena* arena) {
  struct google_protobuf_UninterpretedOption* sub = (struct google_protobuf_UninterpretedOption*)_upb_Message_New(&google_protobuf_UninterpretedOption_msginit, arena);
  bool ok = _upb_Array_Append_accessor2(msg, UPB_SIZE(0, 0), UPB_SIZE(2, 3), &sub, arena);
  if (!ok) return NULL;
  return sub;
}

/* google.protobuf.EnumOptions */

UPB_INLINE google_protobuf_EnumOptions* google_protobuf_EnumOptions_new(upb_Arena* arena) {
  return (google_protobuf_EnumOptions*)_upb_Message_New(&google_protobuf_EnumOptions_msginit, arena);
}
UPB_INLINE google_protobuf_EnumOptions* google_protobuf_EnumOptions_parse(const char* buf, size_t size, upb_Arena* arena) {
  google_protobuf_EnumOptions* ret = google_protobuf_EnumOptions_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_EnumOptions_msginit, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE google_protobuf_EnumOptions* google_protobuf_EnumOptions_parse_ex(const char* buf, size_t size,
                           const upb_ExtensionRegistry* extreg,
                           int options, upb_Arena* arena) {
  google_protobuf_EnumOptions* ret = google_protobuf_EnumOptions_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_EnumOptions_msginit, extreg, options, arena) !=
      kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE char* google_protobuf_EnumOptions_serialize(const google_protobuf_EnumOptions* msg, upb_Arena* arena, size_t* len) {
  return upb_Encode(msg, &google_protobuf_EnumOptions_msginit, 0, arena, len);
}
UPB_INLINE char* google_protobuf_EnumOptions_serialize_ex(const google_protobuf_EnumOptions* msg, int options,
                                 upb_Arena* arena, size_t* len) {
  return upb_Encode(msg, &google_protobuf_EnumOptions_msginit, options, arena, len);
}
UPB_INLINE bool google_protobuf_EnumOptions_has_allow_alias(const google_protobuf_EnumOptions* msg) {
  return _upb_hasbit(msg, 1);
}
UPB_INLINE bool google_protobuf_EnumOptions_allow_alias(const google_protobuf_EnumOptions* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(1, 1), bool);
}
UPB_INLINE bool google_protobuf_EnumOptions_has_deprecated(const google_protobuf_EnumOptions* msg) {
  return _upb_hasbit(msg, 2);
}
UPB_INLINE bool google_protobuf_EnumOptions_deprecated(const google_protobuf_EnumOptions* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(2, 2), bool);
}
UPB_INLINE bool google_protobuf_EnumOptions_has_uninterpreted_option(const google_protobuf_EnumOptions* msg) {
  return _upb_has_submsg_nohasbit(msg, UPB_SIZE(4, 8));
}
UPB_INLINE const google_protobuf_UninterpretedOption* const* google_protobuf_EnumOptions_uninterpreted_option(const google_protobuf_EnumOptions* msg, size_t* len) {
  return (const google_protobuf_UninterpretedOption* const*)_upb_array_accessor(msg, UPB_SIZE(4, 8), len);
}

UPB_INLINE void google_protobuf_EnumOptions_set_allow_alias(google_protobuf_EnumOptions *msg, bool value) {
  _upb_sethas(msg, 1);
  *UPB_PTR_AT(msg, UPB_SIZE(1, 1), bool) = value;
}
UPB_INLINE void google_protobuf_EnumOptions_set_deprecated(google_protobuf_EnumOptions *msg, bool value) {
  _upb_sethas(msg, 2);
  *UPB_PTR_AT(msg, UPB_SIZE(2, 2), bool) = value;
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_EnumOptions_mutable_uninterpreted_option(google_protobuf_EnumOptions* msg, size_t* len) {
  return (google_protobuf_UninterpretedOption**)_upb_array_mutable_accessor(msg, UPB_SIZE(4, 8), len);
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_EnumOptions_resize_uninterpreted_option(google_protobuf_EnumOptions* msg, size_t len, upb_Arena* arena) {
  return (google_protobuf_UninterpretedOption**)_upb_Array_Resize_accessor2(msg, UPB_SIZE(4, 8), len, UPB_SIZE(2, 3), arena);
}
UPB_INLINE struct google_protobuf_UninterpretedOption* google_protobuf_EnumOptions_add_uninterpreted_option(google_protobuf_EnumOptions* msg, upb_Arena* arena) {
  struct google_protobuf_UninterpretedOption* sub = (struct google_protobuf_UninterpretedOption*)_upb_Message_New(&google_protobuf_UninterpretedOption_msginit, arena);
  bool ok = _upb_Array_Append_accessor2(msg, UPB_SIZE(4, 8), UPB_SIZE(2, 3), &sub, arena);
  if (!ok) return NULL;
  return sub;
}

/* google.protobuf.EnumValueOptions */

UPB_INLINE google_protobuf_EnumValueOptions* google_protobuf_EnumValueOptions_new(upb_Arena* arena) {
  return (google_protobuf_EnumValueOptions*)_upb_Message_New(&google_protobuf_EnumValueOptions_msginit, arena);
}
UPB_INLINE google_protobuf_EnumValueOptions* google_protobuf_EnumValueOptions_parse(const char* buf, size_t size, upb_Arena* arena) {
  google_protobuf_EnumValueOptions* ret = google_protobuf_EnumValueOptions_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_EnumValueOptions_msginit, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE google_protobuf_EnumValueOptions* google_protobuf_EnumValueOptions_parse_ex(const char* buf, size_t size,
                           const upb_ExtensionRegistry* extreg,
                           int options, upb_Arena* arena) {
  google_protobuf_EnumValueOptions* ret = google_protobuf_EnumValueOptions_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_EnumValueOptions_msginit, extreg, options, arena) !=
      kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE char* google_protobuf_EnumValueOptions_serialize(const google_protobuf_EnumValueOptions* msg, upb_Arena* arena, size_t* len) {
  return upb_Encode(msg, &google_protobuf_EnumValueOptions_msginit, 0, arena, len);
}
UPB_INLINE char* google_protobuf_EnumValueOptions_serialize_ex(const google_protobuf_EnumValueOptions* msg, int options,
                                 upb_Arena* arena, size_t* len) {
  return upb_Encode(msg, &google_protobuf_EnumValueOptions_msginit, options, arena, len);
}
UPB_INLINE bool google_protobuf_EnumValueOptions_has_deprecated(const google_protobuf_EnumValueOptions* msg) {
  return _upb_hasbit(msg, 1);
}
UPB_INLINE bool google_protobuf_EnumValueOptions_deprecated(const google_protobuf_EnumValueOptions* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(1, 1), bool);
}
UPB_INLINE bool google_protobuf_EnumValueOptions_has_uninterpreted_option(const google_protobuf_EnumValueOptions* msg) {
  return _upb_has_submsg_nohasbit(msg, UPB_SIZE(4, 8));
}
UPB_INLINE const google_protobuf_UninterpretedOption* const* google_protobuf_EnumValueOptions_uninterpreted_option(const google_protobuf_EnumValueOptions* msg, size_t* len) {
  return (const google_protobuf_UninterpretedOption* const*)_upb_array_accessor(msg, UPB_SIZE(4, 8), len);
}

UPB_INLINE void google_protobuf_EnumValueOptions_set_deprecated(google_protobuf_EnumValueOptions *msg, bool value) {
  _upb_sethas(msg, 1);
  *UPB_PTR_AT(msg, UPB_SIZE(1, 1), bool) = value;
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_EnumValueOptions_mutable_uninterpreted_option(google_protobuf_EnumValueOptions* msg, size_t* len) {
  return (google_protobuf_UninterpretedOption**)_upb_array_mutable_accessor(msg, UPB_SIZE(4, 8), len);
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_EnumValueOptions_resize_uninterpreted_option(google_protobuf_EnumValueOptions* msg, size_t len, upb_Arena* arena) {
  return (google_protobuf_UninterpretedOption**)_upb_Array_Resize_accessor2(msg, UPB_SIZE(4, 8), len, UPB_SIZE(2, 3), arena);
}
UPB_INLINE struct google_protobuf_UninterpretedOption* google_protobuf_EnumValueOptions_add_uninterpreted_option(google_protobuf_EnumValueOptions* msg, upb_Arena* arena) {
  struct google_protobuf_UninterpretedOption* sub = (struct google_protobuf_UninterpretedOption*)_upb_Message_New(&google_protobuf_UninterpretedOption_msginit, arena);
  bool ok = _upb_Array_Append_accessor2(msg, UPB_SIZE(4, 8), UPB_SIZE(2, 3), &sub, arena);
  if (!ok) return NULL;
  return sub;
}

/* google.protobuf.ServiceOptions */

UPB_INLINE google_protobuf_ServiceOptions* google_protobuf_ServiceOptions_new(upb_Arena* arena) {
  return (google_protobuf_ServiceOptions*)_upb_Message_New(&google_protobuf_ServiceOptions_msginit, arena);
}
UPB_INLINE google_protobuf_ServiceOptions* google_protobuf_ServiceOptions_parse(const char* buf, size_t size, upb_Arena* arena) {
  google_protobuf_ServiceOptions* ret = google_protobuf_ServiceOptions_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_ServiceOptions_msginit, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE google_protobuf_ServiceOptions* google_protobuf_ServiceOptions_parse_ex(const char* buf, size_t size,
                           const upb_ExtensionRegistry* extreg,
                           int options, upb_Arena* arena) {
  google_protobuf_ServiceOptions* ret = google_protobuf_ServiceOptions_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_ServiceOptions_msginit, extreg, options, arena) !=
      kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE char* google_protobuf_ServiceOptions_serialize(const google_protobuf_ServiceOptions* msg, upb_Arena* arena, size_t* len) {
  return upb_Encode(msg, &google_protobuf_ServiceOptions_msginit, 0, arena, len);
}
UPB_INLINE char* google_protobuf_ServiceOptions_serialize_ex(const google_protobuf_ServiceOptions* msg, int options,
                                 upb_Arena* arena, size_t* len) {
  return upb_Encode(msg, &google_protobuf_ServiceOptions_msginit, options, arena, len);
}
UPB_INLINE bool google_protobuf_ServiceOptions_has_deprecated(const google_protobuf_ServiceOptions* msg) {
  return _upb_hasbit(msg, 1);
}
UPB_INLINE bool google_protobuf_ServiceOptions_deprecated(const google_protobuf_ServiceOptions* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(1, 1), bool);
}
UPB_INLINE bool google_protobuf_ServiceOptions_has_uninterpreted_option(const google_protobuf_ServiceOptions* msg) {
  return _upb_has_submsg_nohasbit(msg, UPB_SIZE(4, 8));
}
UPB_INLINE const google_protobuf_UninterpretedOption* const* google_protobuf_ServiceOptions_uninterpreted_option(const google_protobuf_ServiceOptions* msg, size_t* len) {
  return (const google_protobuf_UninterpretedOption* const*)_upb_array_accessor(msg, UPB_SIZE(4, 8), len);
}

UPB_INLINE void google_protobuf_ServiceOptions_set_deprecated(google_protobuf_ServiceOptions *msg, bool value) {
  _upb_sethas(msg, 1);
  *UPB_PTR_AT(msg, UPB_SIZE(1, 1), bool) = value;
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_ServiceOptions_mutable_uninterpreted_option(google_protobuf_ServiceOptions* msg, size_t* len) {
  return (google_protobuf_UninterpretedOption**)_upb_array_mutable_accessor(msg, UPB_SIZE(4, 8), len);
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_ServiceOptions_resize_uninterpreted_option(google_protobuf_ServiceOptions* msg, size_t len, upb_Arena* arena) {
  return (google_protobuf_UninterpretedOption**)_upb_Array_Resize_accessor2(msg, UPB_SIZE(4, 8), len, UPB_SIZE(2, 3), arena);
}
UPB_INLINE struct google_protobuf_UninterpretedOption* google_protobuf_ServiceOptions_add_uninterpreted_option(google_protobuf_ServiceOptions* msg, upb_Arena* arena) {
  struct google_protobuf_UninterpretedOption* sub = (struct google_protobuf_UninterpretedOption*)_upb_Message_New(&google_protobuf_UninterpretedOption_msginit, arena);
  bool ok = _upb_Array_Append_accessor2(msg, UPB_SIZE(4, 8), UPB_SIZE(2, 3), &sub, arena);
  if (!ok) return NULL;
  return sub;
}

/* google.protobuf.MethodOptions */

UPB_INLINE google_protobuf_MethodOptions* google_protobuf_MethodOptions_new(upb_Arena* arena) {
  return (google_protobuf_MethodOptions*)_upb_Message_New(&google_protobuf_MethodOptions_msginit, arena);
}
UPB_INLINE google_protobuf_MethodOptions* google_protobuf_MethodOptions_parse(const char* buf, size_t size, upb_Arena* arena) {
  google_protobuf_MethodOptions* ret = google_protobuf_MethodOptions_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_MethodOptions_msginit, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE google_protobuf_MethodOptions* google_protobuf_MethodOptions_parse_ex(const char* buf, size_t size,
                           const upb_ExtensionRegistry* extreg,
                           int options, upb_Arena* arena) {
  google_protobuf_MethodOptions* ret = google_protobuf_MethodOptions_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_MethodOptions_msginit, extreg, options, arena) !=
      kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE char* google_protobuf_MethodOptions_serialize(const google_protobuf_MethodOptions* msg, upb_Arena* arena, size_t* len) {
  return upb_Encode(msg, &google_protobuf_MethodOptions_msginit, 0, arena, len);
}
UPB_INLINE char* google_protobuf_MethodOptions_serialize_ex(const google_protobuf_MethodOptions* msg, int options,
                                 upb_Arena* arena, size_t* len) {
  return upb_Encode(msg, &google_protobuf_MethodOptions_msginit, options, arena, len);
}
UPB_INLINE bool google_protobuf_MethodOptions_has_deprecated(const google_protobuf_MethodOptions* msg) {
  return _upb_hasbit(msg, 1);
}
UPB_INLINE bool google_protobuf_MethodOptions_deprecated(const google_protobuf_MethodOptions* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(8, 8), bool);
}
UPB_INLINE bool google_protobuf_MethodOptions_has_idempotency_level(const google_protobuf_MethodOptions* msg) {
  return _upb_hasbit(msg, 2);
}
UPB_INLINE int32_t google_protobuf_MethodOptions_idempotency_level(const google_protobuf_MethodOptions* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(4, 4), int32_t);
}
UPB_INLINE bool google_protobuf_MethodOptions_has_uninterpreted_option(const google_protobuf_MethodOptions* msg) {
  return _upb_has_submsg_nohasbit(msg, UPB_SIZE(12, 16));
}
UPB_INLINE const google_protobuf_UninterpretedOption* const* google_protobuf_MethodOptions_uninterpreted_option(const google_protobuf_MethodOptions* msg, size_t* len) {
  return (const google_protobuf_UninterpretedOption* const*)_upb_array_accessor(msg, UPB_SIZE(12, 16), len);
}

UPB_INLINE void google_protobuf_MethodOptions_set_deprecated(google_protobuf_MethodOptions *msg, bool value) {
  _upb_sethas(msg, 1);
  *UPB_PTR_AT(msg, UPB_SIZE(8, 8), bool) = value;
}
UPB_INLINE void google_protobuf_MethodOptions_set_idempotency_level(google_protobuf_MethodOptions *msg, int32_t value) {
  _upb_sethas(msg, 2);
  *UPB_PTR_AT(msg, UPB_SIZE(4, 4), int32_t) = value;
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_MethodOptions_mutable_uninterpreted_option(google_protobuf_MethodOptions* msg, size_t* len) {
  return (google_protobuf_UninterpretedOption**)_upb_array_mutable_accessor(msg, UPB_SIZE(12, 16), len);
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_MethodOptions_resize_uninterpreted_option(google_protobuf_MethodOptions* msg, size_t len, upb_Arena* arena) {
  return (google_protobuf_UninterpretedOption**)_upb_Array_Resize_accessor2(msg, UPB_SIZE(12, 16), len, UPB_SIZE(2, 3), arena);
}
UPB_INLINE struct google_protobuf_UninterpretedOption* google_protobuf_MethodOptions_add_uninterpreted_option(google_protobuf_MethodOptions* msg, upb_Arena* arena) {
  struct google_protobuf_UninterpretedOption* sub = (struct google_protobuf_UninterpretedOption*)_upb_Message_New(&google_protobuf_UninterpretedOption_msginit, arena);
  bool ok = _upb_Array_Append_accessor2(msg, UPB_SIZE(12, 16), UPB_SIZE(2, 3), &sub, arena);
  if (!ok) return NULL;
  return sub;
}

/* google.protobuf.UninterpretedOption */

UPB_INLINE google_protobuf_UninterpretedOption* google_protobuf_UninterpretedOption_new(upb_Arena* arena) {
  return (google_protobuf_UninterpretedOption*)_upb_Message_New(&google_protobuf_UninterpretedOption_msginit, arena);
}
UPB_INLINE google_protobuf_UninterpretedOption* google_protobuf_UninterpretedOption_parse(const char* buf, size_t size, upb_Arena* arena) {
  google_protobuf_UninterpretedOption* ret = google_protobuf_UninterpretedOption_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_UninterpretedOption_msginit, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE google_protobuf_UninterpretedOption* google_protobuf_UninterpretedOption_parse_ex(const char* buf, size_t size,
                           const upb_ExtensionRegistry* extreg,
                           int options, upb_Arena* arena) {
  google_protobuf_UninterpretedOption* ret = google_protobuf_UninterpretedOption_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_UninterpretedOption_msginit, extreg, options, arena) !=
      kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE char* google_protobuf_UninterpretedOption_serialize(const google_protobuf_UninterpretedOption* msg, upb_Arena* arena, size_t* len) {
  return upb_Encode(msg, &google_protobuf_UninterpretedOption_msginit, 0, arena, len);
}
UPB_INLINE char* google_protobuf_UninterpretedOption_serialize_ex(const google_protobuf_UninterpretedOption* msg, int options,
                                 upb_Arena* arena, size_t* len) {
  return upb_Encode(msg, &google_protobuf_UninterpretedOption_msginit, options, arena, len);
}
UPB_INLINE bool google_protobuf_UninterpretedOption_has_name(const google_protobuf_UninterpretedOption* msg) {
  return _upb_has_submsg_nohasbit(msg, UPB_SIZE(56, 80));
}
UPB_INLINE const google_protobuf_UninterpretedOption_NamePart* const* google_protobuf_UninterpretedOption_name(const google_protobuf_UninterpretedOption* msg, size_t* len) {
  return (const google_protobuf_UninterpretedOption_NamePart* const*)_upb_array_accessor(msg, UPB_SIZE(56, 80), len);
}
UPB_INLINE bool google_protobuf_UninterpretedOption_has_identifier_value(const google_protobuf_UninterpretedOption* msg) {
  return _upb_hasbit(msg, 1);
}
UPB_INLINE upb_StringView google_protobuf_UninterpretedOption_identifier_value(const google_protobuf_UninterpretedOption* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(32, 32), upb_StringView);
}
UPB_INLINE bool google_protobuf_UninterpretedOption_has_positive_int_value(const google_protobuf_UninterpretedOption* msg) {
  return _upb_hasbit(msg, 2);
}
UPB_INLINE uint64_t google_protobuf_UninterpretedOption_positive_int_value(const google_protobuf_UninterpretedOption* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(8, 8), uint64_t);
}
UPB_INLINE bool google_protobuf_UninterpretedOption_has_negative_int_value(const google_protobuf_UninterpretedOption* msg) {
  return _upb_hasbit(msg, 3);
}
UPB_INLINE int64_t google_protobuf_UninterpretedOption_negative_int_value(const google_protobuf_UninterpretedOption* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(16, 16), int64_t);
}
UPB_INLINE bool google_protobuf_UninterpretedOption_has_double_value(const google_protobuf_UninterpretedOption* msg) {
  return _upb_hasbit(msg, 4);
}
UPB_INLINE double google_protobuf_UninterpretedOption_double_value(const google_protobuf_UninterpretedOption* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(24, 24), double);
}
UPB_INLINE bool google_protobuf_UninterpretedOption_has_string_value(const google_protobuf_UninterpretedOption* msg) {
  return _upb_hasbit(msg, 5);
}
UPB_INLINE upb_StringView google_protobuf_UninterpretedOption_string_value(const google_protobuf_UninterpretedOption* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(40, 48), upb_StringView);
}
UPB_INLINE bool google_protobuf_UninterpretedOption_has_aggregate_value(const google_protobuf_UninterpretedOption* msg) {
  return _upb_hasbit(msg, 6);
}
UPB_INLINE upb_StringView google_protobuf_UninterpretedOption_aggregate_value(const google_protobuf_UninterpretedOption* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(48, 64), upb_StringView);
}

UPB_INLINE google_protobuf_UninterpretedOption_NamePart** google_protobuf_UninterpretedOption_mutable_name(google_protobuf_UninterpretedOption* msg, size_t* len) {
  return (google_protobuf_UninterpretedOption_NamePart**)_upb_array_mutable_accessor(msg, UPB_SIZE(56, 80), len);
}
UPB_INLINE google_protobuf_UninterpretedOption_NamePart** google_protobuf_UninterpretedOption_resize_name(google_protobuf_UninterpretedOption* msg, size_t len, upb_Arena* arena) {
  return (google_protobuf_UninterpretedOption_NamePart**)_upb_Array_Resize_accessor2(msg, UPB_SIZE(56, 80), len, UPB_SIZE(2, 3), arena);
}
UPB_INLINE struct google_protobuf_UninterpretedOption_NamePart* google_protobuf_UninterpretedOption_add_name(google_protobuf_UninterpretedOption* msg, upb_Arena* arena) {
  struct google_protobuf_UninterpretedOption_NamePart* sub = (struct google_protobuf_UninterpretedOption_NamePart*)_upb_Message_New(&google_protobuf_UninterpretedOption_NamePart_msginit, arena);
  bool ok = _upb_Array_Append_accessor2(msg, UPB_SIZE(56, 80), UPB_SIZE(2, 3), &sub, arena);
  if (!ok) return NULL;
  return sub;
}
UPB_INLINE void google_protobuf_UninterpretedOption_set_identifier_value(google_protobuf_UninterpretedOption *msg, upb_StringView value) {
  _upb_sethas(msg, 1);
  *UPB_PTR_AT(msg, UPB_SIZE(32, 32), upb_StringView) = value;
}
UPB_INLINE void google_protobuf_UninterpretedOption_set_positive_int_value(google_protobuf_UninterpretedOption *msg, uint64_t value) {
  _upb_sethas(msg, 2);
  *UPB_PTR_AT(msg, UPB_SIZE(8, 8), uint64_t) = value;
}
UPB_INLINE void google_protobuf_UninterpretedOption_set_negative_int_value(google_protobuf_UninterpretedOption *msg, int64_t value) {
  _upb_sethas(msg, 3);
  *UPB_PTR_AT(msg, UPB_SIZE(16, 16), int64_t) = value;
}
UPB_INLINE void google_protobuf_UninterpretedOption_set_double_value(google_protobuf_UninterpretedOption *msg, double value) {
  _upb_sethas(msg, 4);
  *UPB_PTR_AT(msg, UPB_SIZE(24, 24), double) = value;
}
UPB_INLINE void google_protobuf_UninterpretedOption_set_string_value(google_protobuf_UninterpretedOption *msg, upb_StringView value) {
  _upb_sethas(msg, 5);
  *UPB_PTR_AT(msg, UPB_SIZE(40, 48), upb_StringView) = value;
}
UPB_INLINE void google_protobuf_UninterpretedOption_set_aggregate_value(google_protobuf_UninterpretedOption *msg, upb_StringView value) {
  _upb_sethas(msg, 6);
  *UPB_PTR_AT(msg, UPB_SIZE(48, 64), upb_StringView) = value;
}

/* google.protobuf.UninterpretedOption.NamePart */

UPB_INLINE google_protobuf_UninterpretedOption_NamePart* google_protobuf_UninterpretedOption_NamePart_new(upb_Arena* arena) {
  return (google_protobuf_UninterpretedOption_NamePart*)_upb_Message_New(&google_protobuf_UninterpretedOption_NamePart_msginit, arena);
}
UPB_INLINE google_protobuf_UninterpretedOption_NamePart* google_protobuf_UninterpretedOption_NamePart_parse(const char* buf, size_t size, upb_Arena* arena) {
  google_protobuf_UninterpretedOption_NamePart* ret = google_protobuf_UninterpretedOption_NamePart_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_UninterpretedOption_NamePart_msginit, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE google_protobuf_UninterpretedOption_NamePart* google_protobuf_UninterpretedOption_NamePart_parse_ex(const char* buf, size_t size,
                           const upb_ExtensionRegistry* extreg,
                           int options, upb_Arena* arena) {
  google_protobuf_UninterpretedOption_NamePart* ret = google_protobuf_UninterpretedOption_NamePart_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_UninterpretedOption_NamePart_msginit, extreg, options, arena) !=
      kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE char* google_protobuf_UninterpretedOption_NamePart_serialize(const google_protobuf_UninterpretedOption_NamePart* msg, upb_Arena* arena, size_t* len) {
  return upb_Encode(msg, &google_protobuf_UninterpretedOption_NamePart_msginit, 0, arena, len);
}
UPB_INLINE char* google_protobuf_UninterpretedOption_NamePart_serialize_ex(const google_protobuf_UninterpretedOption_NamePart* msg, int options,
                                 upb_Arena* arena, size_t* len) {
  return upb_Encode(msg, &google_protobuf_UninterpretedOption_NamePart_msginit, options, arena, len);
}
UPB_INLINE bool google_protobuf_UninterpretedOption_NamePart_has_name_part(const google_protobuf_UninterpretedOption_NamePart* msg) {
  return _upb_hasbit(msg, 1);
}
UPB_INLINE upb_StringView google_protobuf_UninterpretedOption_NamePart_name_part(const google_protobuf_UninterpretedOption_NamePart* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(4, 8), upb_StringView);
}
UPB_INLINE bool google_protobuf_UninterpretedOption_NamePart_has_is_extension(const google_protobuf_UninterpretedOption_NamePart* msg) {
  return _upb_hasbit(msg, 2);
}
UPB_INLINE bool google_protobuf_UninterpretedOption_NamePart_is_extension(const google_protobuf_UninterpretedOption_NamePart* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(1, 1), bool);
}

UPB_INLINE void google_protobuf_UninterpretedOption_NamePart_set_name_part(google_protobuf_UninterpretedOption_NamePart *msg, upb_StringView value) {
  _upb_sethas(msg, 1);
  *UPB_PTR_AT(msg, UPB_SIZE(4, 8), upb_StringView) = value;
}
UPB_INLINE void google_protobuf_UninterpretedOption_NamePart_set_is_extension(google_protobuf_UninterpretedOption_NamePart *msg, bool value) {
  _upb_sethas(msg, 2);
  *UPB_PTR_AT(msg, UPB_SIZE(1, 1), bool) = value;
}

/* google.protobuf.SourceCodeInfo */

UPB_INLINE google_protobuf_SourceCodeInfo* google_protobuf_SourceCodeInfo_new(upb_Arena* arena) {
  return (google_protobuf_SourceCodeInfo*)_upb_Message_New(&google_protobuf_SourceCodeInfo_msginit, arena);
}
UPB_INLINE google_protobuf_SourceCodeInfo* google_protobuf_SourceCodeInfo_parse(const char* buf, size_t size, upb_Arena* arena) {
  google_protobuf_SourceCodeInfo* ret = google_protobuf_SourceCodeInfo_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_SourceCodeInfo_msginit, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE google_protobuf_SourceCodeInfo* google_protobuf_SourceCodeInfo_parse_ex(const char* buf, size_t size,
                           const upb_ExtensionRegistry* extreg,
                           int options, upb_Arena* arena) {
  google_protobuf_SourceCodeInfo* ret = google_protobuf_SourceCodeInfo_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_SourceCodeInfo_msginit, extreg, options, arena) !=
      kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE char* google_protobuf_SourceCodeInfo_serialize(const google_protobuf_SourceCodeInfo* msg, upb_Arena* arena, size_t* len) {
  return upb_Encode(msg, &google_protobuf_SourceCodeInfo_msginit, 0, arena, len);
}
UPB_INLINE char* google_protobuf_SourceCodeInfo_serialize_ex(const google_protobuf_SourceCodeInfo* msg, int options,
                                 upb_Arena* arena, size_t* len) {
  return upb_Encode(msg, &google_protobuf_SourceCodeInfo_msginit, options, arena, len);
}
UPB_INLINE bool google_protobuf_SourceCodeInfo_has_location(const google_protobuf_SourceCodeInfo* msg) {
  return _upb_has_submsg_nohasbit(msg, UPB_SIZE(0, 0));
}
UPB_INLINE const google_protobuf_SourceCodeInfo_Location* const* google_protobuf_SourceCodeInfo_location(const google_protobuf_SourceCodeInfo* msg, size_t* len) {
  return (const google_protobuf_SourceCodeInfo_Location* const*)_upb_array_accessor(msg, UPB_SIZE(0, 0), len);
}

UPB_INLINE google_protobuf_SourceCodeInfo_Location** google_protobuf_SourceCodeInfo_mutable_location(google_protobuf_SourceCodeInfo* msg, size_t* len) {
  return (google_protobuf_SourceCodeInfo_Location**)_upb_array_mutable_accessor(msg, UPB_SIZE(0, 0), len);
}
UPB_INLINE google_protobuf_SourceCodeInfo_Location** google_protobuf_SourceCodeInfo_resize_location(google_protobuf_SourceCodeInfo* msg, size_t len, upb_Arena* arena) {
  return (google_protobuf_SourceCodeInfo_Location**)_upb_Array_Resize_accessor2(msg, UPB_SIZE(0, 0), len, UPB_SIZE(2, 3), arena);
}
UPB_INLINE struct google_protobuf_SourceCodeInfo_Location* google_protobuf_SourceCodeInfo_add_location(google_protobuf_SourceCodeInfo* msg, upb_Arena* arena) {
  struct google_protobuf_SourceCodeInfo_Location* sub = (struct google_protobuf_SourceCodeInfo_Location*)_upb_Message_New(&google_protobuf_SourceCodeInfo_Location_msginit, arena);
  bool ok = _upb_Array_Append_accessor2(msg, UPB_SIZE(0, 0), UPB_SIZE(2, 3), &sub, arena);
  if (!ok) return NULL;
  return sub;
}

/* google.protobuf.SourceCodeInfo.Location */

UPB_INLINE google_protobuf_SourceCodeInfo_Location* google_protobuf_SourceCodeInfo_Location_new(upb_Arena* arena) {
  return (google_protobuf_SourceCodeInfo_Location*)_upb_Message_New(&google_protobuf_SourceCodeInfo_Location_msginit, arena);
}
UPB_INLINE google_protobuf_SourceCodeInfo_Location* google_protobuf_SourceCodeInfo_Location_parse(const char* buf, size_t size, upb_Arena* arena) {
  google_protobuf_SourceCodeInfo_Location* ret = google_protobuf_SourceCodeInfo_Location_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_SourceCodeInfo_Location_msginit, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE google_protobuf_SourceCodeInfo_Location* google_protobuf_SourceCodeInfo_Location_parse_ex(const char* buf, size_t size,
                           const upb_ExtensionRegistry* extreg,
                           int options, upb_Arena* arena) {
  google_protobuf_SourceCodeInfo_Location* ret = google_protobuf_SourceCodeInfo_Location_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_SourceCodeInfo_Location_msginit, extreg, options, arena) !=
      kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE char* google_protobuf_SourceCodeInfo_Location_serialize(const google_protobuf_SourceCodeInfo_Location* msg, upb_Arena* arena, size_t* len) {
  return upb_Encode(msg, &google_protobuf_SourceCodeInfo_Location_msginit, 0, arena, len);
}
UPB_INLINE char* google_protobuf_SourceCodeInfo_Location_serialize_ex(const google_protobuf_SourceCodeInfo_Location* msg, int options,
                                 upb_Arena* arena, size_t* len) {
  return upb_Encode(msg, &google_protobuf_SourceCodeInfo_Location_msginit, options, arena, len);
}
UPB_INLINE int32_t const* google_protobuf_SourceCodeInfo_Location_path(const google_protobuf_SourceCodeInfo_Location* msg, size_t* len) {
  return (int32_t const*)_upb_array_accessor(msg, UPB_SIZE(20, 40), len);
}
UPB_INLINE int32_t const* google_protobuf_SourceCodeInfo_Location_span(const google_protobuf_SourceCodeInfo_Location* msg, size_t* len) {
  return (int32_t const*)_upb_array_accessor(msg, UPB_SIZE(24, 48), len);
}
UPB_INLINE bool google_protobuf_SourceCodeInfo_Location_has_leading_comments(const google_protobuf_SourceCodeInfo_Location* msg) {
  return _upb_hasbit(msg, 1);
}
UPB_INLINE upb_StringView google_protobuf_SourceCodeInfo_Location_leading_comments(const google_protobuf_SourceCodeInfo_Location* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(4, 8), upb_StringView);
}
UPB_INLINE bool google_protobuf_SourceCodeInfo_Location_has_trailing_comments(const google_protobuf_SourceCodeInfo_Location* msg) {
  return _upb_hasbit(msg, 2);
}
UPB_INLINE upb_StringView google_protobuf_SourceCodeInfo_Location_trailing_comments(const google_protobuf_SourceCodeInfo_Location* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(12, 24), upb_StringView);
}
UPB_INLINE upb_StringView const* google_protobuf_SourceCodeInfo_Location_leading_detached_comments(const google_protobuf_SourceCodeInfo_Location* msg, size_t* len) {
  return (upb_StringView const*)_upb_array_accessor(msg, UPB_SIZE(28, 56), len);
}

UPB_INLINE int32_t* google_protobuf_SourceCodeInfo_Location_mutable_path(google_protobuf_SourceCodeInfo_Location* msg, size_t* len) {
  return (int32_t*)_upb_array_mutable_accessor(msg, UPB_SIZE(20, 40), len);
}
UPB_INLINE int32_t* google_protobuf_SourceCodeInfo_Location_resize_path(google_protobuf_SourceCodeInfo_Location* msg, size_t len, upb_Arena* arena) {
  return (int32_t*)_upb_Array_Resize_accessor2(msg, UPB_SIZE(20, 40), len, 2, arena);
}
UPB_INLINE bool google_protobuf_SourceCodeInfo_Location_add_path(google_protobuf_SourceCodeInfo_Location* msg, int32_t val, upb_Arena* arena) {
  return _upb_Array_Append_accessor2(msg, UPB_SIZE(20, 40), 2, &val, arena);
}
UPB_INLINE int32_t* google_protobuf_SourceCodeInfo_Location_mutable_span(google_protobuf_SourceCodeInfo_Location* msg, size_t* len) {
  return (int32_t*)_upb_array_mutable_accessor(msg, UPB_SIZE(24, 48), len);
}
UPB_INLINE int32_t* google_protobuf_SourceCodeInfo_Location_resize_span(google_protobuf_SourceCodeInfo_Location* msg, size_t len, upb_Arena* arena) {
  return (int32_t*)_upb_Array_Resize_accessor2(msg, UPB_SIZE(24, 48), len, 2, arena);
}
UPB_INLINE bool google_protobuf_SourceCodeInfo_Location_add_span(google_protobuf_SourceCodeInfo_Location* msg, int32_t val, upb_Arena* arena) {
  return _upb_Array_Append_accessor2(msg, UPB_SIZE(24, 48), 2, &val, arena);
}
UPB_INLINE void google_protobuf_SourceCodeInfo_Location_set_leading_comments(google_protobuf_SourceCodeInfo_Location *msg, upb_StringView value) {
  _upb_sethas(msg, 1);
  *UPB_PTR_AT(msg, UPB_SIZE(4, 8), upb_StringView) = value;
}
UPB_INLINE void google_protobuf_SourceCodeInfo_Location_set_trailing_comments(google_protobuf_SourceCodeInfo_Location *msg, upb_StringView value) {
  _upb_sethas(msg, 2);
  *UPB_PTR_AT(msg, UPB_SIZE(12, 24), upb_StringView) = value;
}
UPB_INLINE upb_StringView* google_protobuf_SourceCodeInfo_Location_mutable_leading_detached_comments(google_protobuf_SourceCodeInfo_Location* msg, size_t* len) {
  return (upb_StringView*)_upb_array_mutable_accessor(msg, UPB_SIZE(28, 56), len);
}
UPB_INLINE upb_StringView* google_protobuf_SourceCodeInfo_Location_resize_leading_detached_comments(google_protobuf_SourceCodeInfo_Location* msg, size_t len, upb_Arena* arena) {
  return (upb_StringView*)_upb_Array_Resize_accessor2(msg, UPB_SIZE(28, 56), len, UPB_SIZE(3, 4), arena);
}
UPB_INLINE bool google_protobuf_SourceCodeInfo_Location_add_leading_detached_comments(google_protobuf_SourceCodeInfo_Location* msg, upb_StringView val, upb_Arena* arena) {
  return _upb_Array_Append_accessor2(msg, UPB_SIZE(28, 56), UPB_SIZE(3, 4), &val, arena);
}

/* google.protobuf.GeneratedCodeInfo */

UPB_INLINE google_protobuf_GeneratedCodeInfo* google_protobuf_GeneratedCodeInfo_new(upb_Arena* arena) {
  return (google_protobuf_GeneratedCodeInfo*)_upb_Message_New(&google_protobuf_GeneratedCodeInfo_msginit, arena);
}
UPB_INLINE google_protobuf_GeneratedCodeInfo* google_protobuf_GeneratedCodeInfo_parse(const char* buf, size_t size, upb_Arena* arena) {
  google_protobuf_GeneratedCodeInfo* ret = google_protobuf_GeneratedCodeInfo_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_GeneratedCodeInfo_msginit, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE google_protobuf_GeneratedCodeInfo* google_protobuf_GeneratedCodeInfo_parse_ex(const char* buf, size_t size,
                           const upb_ExtensionRegistry* extreg,
                           int options, upb_Arena* arena) {
  google_protobuf_GeneratedCodeInfo* ret = google_protobuf_GeneratedCodeInfo_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_GeneratedCodeInfo_msginit, extreg, options, arena) !=
      kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE char* google_protobuf_GeneratedCodeInfo_serialize(const google_protobuf_GeneratedCodeInfo* msg, upb_Arena* arena, size_t* len) {
  return upb_Encode(msg, &google_protobuf_GeneratedCodeInfo_msginit, 0, arena, len);
}
UPB_INLINE char* google_protobuf_GeneratedCodeInfo_serialize_ex(const google_protobuf_GeneratedCodeInfo* msg, int options,
                                 upb_Arena* arena, size_t* len) {
  return upb_Encode(msg, &google_protobuf_GeneratedCodeInfo_msginit, options, arena, len);
}
UPB_INLINE bool google_protobuf_GeneratedCodeInfo_has_annotation(const google_protobuf_GeneratedCodeInfo* msg) {
  return _upb_has_submsg_nohasbit(msg, UPB_SIZE(0, 0));
}
UPB_INLINE const google_protobuf_GeneratedCodeInfo_Annotation* const* google_protobuf_GeneratedCodeInfo_annotation(const google_protobuf_GeneratedCodeInfo* msg, size_t* len) {
  return (const google_protobuf_GeneratedCodeInfo_Annotation* const*)_upb_array_accessor(msg, UPB_SIZE(0, 0), len);
}

UPB_INLINE google_protobuf_GeneratedCodeInfo_Annotation** google_protobuf_GeneratedCodeInfo_mutable_annotation(google_protobuf_GeneratedCodeInfo* msg, size_t* len) {
  return (google_protobuf_GeneratedCodeInfo_Annotation**)_upb_array_mutable_accessor(msg, UPB_SIZE(0, 0), len);
}
UPB_INLINE google_protobuf_GeneratedCodeInfo_Annotation** google_protobuf_GeneratedCodeInfo_resize_annotation(google_protobuf_GeneratedCodeInfo* msg, size_t len, upb_Arena* arena) {
  return (google_protobuf_GeneratedCodeInfo_Annotation**)_upb_Array_Resize_accessor2(msg, UPB_SIZE(0, 0), len, UPB_SIZE(2, 3), arena);
}
UPB_INLINE struct google_protobuf_GeneratedCodeInfo_Annotation* google_protobuf_GeneratedCodeInfo_add_annotation(google_protobuf_GeneratedCodeInfo* msg, upb_Arena* arena) {
  struct google_protobuf_GeneratedCodeInfo_Annotation* sub = (struct google_protobuf_GeneratedCodeInfo_Annotation*)_upb_Message_New(&google_protobuf_GeneratedCodeInfo_Annotation_msginit, arena);
  bool ok = _upb_Array_Append_accessor2(msg, UPB_SIZE(0, 0), UPB_SIZE(2, 3), &sub, arena);
  if (!ok) return NULL;
  return sub;
}

/* google.protobuf.GeneratedCodeInfo.Annotation */

UPB_INLINE google_protobuf_GeneratedCodeInfo_Annotation* google_protobuf_GeneratedCodeInfo_Annotation_new(upb_Arena* arena) {
  return (google_protobuf_GeneratedCodeInfo_Annotation*)_upb_Message_New(&google_protobuf_GeneratedCodeInfo_Annotation_msginit, arena);
}
UPB_INLINE google_protobuf_GeneratedCodeInfo_Annotation* google_protobuf_GeneratedCodeInfo_Annotation_parse(const char* buf, size_t size, upb_Arena* arena) {
  google_protobuf_GeneratedCodeInfo_Annotation* ret = google_protobuf_GeneratedCodeInfo_Annotation_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_GeneratedCodeInfo_Annotation_msginit, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE google_protobuf_GeneratedCodeInfo_Annotation* google_protobuf_GeneratedCodeInfo_Annotation_parse_ex(const char* buf, size_t size,
                           const upb_ExtensionRegistry* extreg,
                           int options, upb_Arena* arena) {
  google_protobuf_GeneratedCodeInfo_Annotation* ret = google_protobuf_GeneratedCodeInfo_Annotation_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_GeneratedCodeInfo_Annotation_msginit, extreg, options, arena) !=
      kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE char* google_protobuf_GeneratedCodeInfo_Annotation_serialize(const google_protobuf_GeneratedCodeInfo_Annotation* msg, upb_Arena* arena, size_t* len) {
  return upb_Encode(msg, &google_protobuf_GeneratedCodeInfo_Annotation_msginit, 0, arena, len);
}
UPB_INLINE char* google_protobuf_GeneratedCodeInfo_Annotation_serialize_ex(const google_protobuf_GeneratedCodeInfo_Annotation* msg, int options,
                                 upb_Arena* arena, size_t* len) {
  return upb_Encode(msg, &google_protobuf_GeneratedCodeInfo_Annotation_msginit, options, arena, len);
}
UPB_INLINE int32_t const* google_protobuf_GeneratedCodeInfo_Annotation_path(const google_protobuf_GeneratedCodeInfo_Annotation* msg, size_t* len) {
  return (int32_t const*)_upb_array_accessor(msg, UPB_SIZE(20, 32), len);
}
UPB_INLINE bool google_protobuf_GeneratedCodeInfo_Annotation_has_source_file(const google_protobuf_GeneratedCodeInfo_Annotation* msg) {
  return _upb_hasbit(msg, 1);
}
UPB_INLINE upb_StringView google_protobuf_GeneratedCodeInfo_Annotation_source_file(const google_protobuf_GeneratedCodeInfo_Annotation* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(12, 16), upb_StringView);
}
UPB_INLINE bool google_protobuf_GeneratedCodeInfo_Annotation_has_begin(const google_protobuf_GeneratedCodeInfo_Annotation* msg) {
  return _upb_hasbit(msg, 2);
}
UPB_INLINE int32_t google_protobuf_GeneratedCodeInfo_Annotation_begin(const google_protobuf_GeneratedCodeInfo_Annotation* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(4, 4), int32_t);
}
UPB_INLINE bool google_protobuf_GeneratedCodeInfo_Annotation_has_end(const google_protobuf_GeneratedCodeInfo_Annotation* msg) {
  return _upb_hasbit(msg, 3);
}
UPB_INLINE int32_t google_protobuf_GeneratedCodeInfo_Annotation_end(const google_protobuf_GeneratedCodeInfo_Annotation* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(8, 8), int32_t);
}

UPB_INLINE int32_t* google_protobuf_GeneratedCodeInfo_Annotation_mutable_path(google_protobuf_GeneratedCodeInfo_Annotation* msg, size_t* len) {
  return (int32_t*)_upb_array_mutable_accessor(msg, UPB_SIZE(20, 32), len);
}
UPB_INLINE int32_t* google_protobuf_GeneratedCodeInfo_Annotation_resize_path(google_protobuf_GeneratedCodeInfo_Annotation* msg, size_t len, upb_Arena* arena) {
  return (int32_t*)_upb_Array_Resize_accessor2(msg, UPB_SIZE(20, 32), len, 2, arena);
}
UPB_INLINE bool google_protobuf_GeneratedCodeInfo_Annotation_add_path(google_protobuf_GeneratedCodeInfo_Annotation* msg, int32_t val, upb_Arena* arena) {
  return _upb_Array_Append_accessor2(msg, UPB_SIZE(20, 32), 2, &val, arena);
}
UPB_INLINE void google_protobuf_GeneratedCodeInfo_Annotation_set_source_file(google_protobuf_GeneratedCodeInfo_Annotation *msg, upb_StringView value) {
  _upb_sethas(msg, 1);
  *UPB_PTR_AT(msg, UPB_SIZE(12, 16), upb_StringView) = value;
}
UPB_INLINE void google_protobuf_GeneratedCodeInfo_Annotation_set_begin(google_protobuf_GeneratedCodeInfo_Annotation *msg, int32_t value) {
  _upb_sethas(msg, 2);
  *UPB_PTR_AT(msg, UPB_SIZE(4, 4), int32_t) = value;
}
UPB_INLINE void google_protobuf_GeneratedCodeInfo_Annotation_set_end(google_protobuf_GeneratedCodeInfo_Annotation *msg, int32_t value) {
  _upb_sethas(msg, 3);
  *UPB_PTR_AT(msg, UPB_SIZE(8, 8), int32_t) = value;
}

extern const upb_MiniTable_File google_protobuf_descriptor_proto_upb_file_layout;

/* Max size 32 is google.protobuf.FileOptions */
/* Max size 64 is google.protobuf.FileOptions */
#define _UPB_MAXOPT_SIZE UPB_SIZE(104, 192)

#ifdef __cplusplus
}  /* extern "C" */
#endif


#endif  /* GOOGLE_PROTOBUF_DESCRIPTOR_PROTO_UPB_H_ */

/** upb/def.h ************************************************************/
#ifndef UPB_DEF_H_
#define UPB_DEF_H_


/* Must be last. */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct upb_EnumDef;
typedef struct upb_EnumDef upb_EnumDef;
struct upb_EnumValueDef;
typedef struct upb_EnumValueDef upb_EnumValueDef;
struct upb_ExtensionRange;
typedef struct upb_ExtensionRange upb_ExtensionRange;
struct upb_FieldDef;
typedef struct upb_FieldDef upb_FieldDef;
struct upb_FileDef;
typedef struct upb_FileDef upb_FileDef;
struct upb_MethodDef;
typedef struct upb_MethodDef upb_MethodDef;
struct upb_MessageDef;
typedef struct upb_MessageDef upb_MessageDef;
struct upb_OneofDef;
typedef struct upb_OneofDef upb_OneofDef;
struct upb_ServiceDef;
typedef struct upb_ServiceDef upb_ServiceDef;
struct upb_streamdef;
typedef struct upb_streamdef upb_streamdef;
struct upb_DefPool;
typedef struct upb_DefPool upb_DefPool;

typedef enum { kUpb_Syntax_Proto2 = 2, kUpb_Syntax_Proto3 = 3 } upb_Syntax;

/* All the different kind of well known type messages. For simplicity of check,
 * number wrappers and string wrappers are grouped together. Make sure the
 * order and merber of these groups are not changed.
 */
typedef enum {
  kUpb_WellKnown_Unspecified,
  kUpb_WellKnown_Any,
  kUpb_WellKnown_FieldMask,
  kUpb_WellKnown_Duration,
  kUpb_WellKnown_Timestamp,
  /* number wrappers */
  kUpb_WellKnown_DoubleValue,
  kUpb_WellKnown_FloatValue,
  kUpb_WellKnown_Int64Value,
  kUpb_WellKnown_UInt64Value,
  kUpb_WellKnown_Int32Value,
  kUpb_WellKnown_UInt32Value,
  /* string wrappers */
  kUpb_WellKnown_StringValue,
  kUpb_WellKnown_BytesValue,
  kUpb_WellKnown_BoolValue,
  kUpb_WellKnown_Value,
  kUpb_WellKnown_ListValue,
  kUpb_WellKnown_Struct
} upb_WellKnown;

/* upb_FieldDef ***************************************************************/

/* Maximum field number allowed for FieldDefs.  This is an inherent limit of the
 * protobuf wire format. */
#define kUpb_MaxFieldNumber ((1 << 29) - 1)

const google_protobuf_FieldOptions* upb_FieldDef_Options(const upb_FieldDef* f);
bool upb_FieldDef_HasOptions(const upb_FieldDef* f);
const char* upb_FieldDef_FullName(const upb_FieldDef* f);
upb_CType upb_FieldDef_CType(const upb_FieldDef* f);
upb_FieldType upb_FieldDef_Type(const upb_FieldDef* f);
upb_Label upb_FieldDef_Label(const upb_FieldDef* f);
uint32_t upb_FieldDef_Number(const upb_FieldDef* f);
const char* upb_FieldDef_Name(const upb_FieldDef* f);
const char* upb_FieldDef_JsonName(const upb_FieldDef* f);
bool upb_FieldDef_HasJsonName(const upb_FieldDef* f);
bool upb_FieldDef_IsExtension(const upb_FieldDef* f);
bool upb_FieldDef_IsPacked(const upb_FieldDef* f);
const upb_FileDef* upb_FieldDef_File(const upb_FieldDef* f);
const upb_MessageDef* upb_FieldDef_ContainingType(const upb_FieldDef* f);
const upb_MessageDef* upb_FieldDef_ExtensionScope(const upb_FieldDef* f);
const upb_OneofDef* upb_FieldDef_ContainingOneof(const upb_FieldDef* f);
const upb_OneofDef* upb_FieldDef_RealContainingOneof(const upb_FieldDef* f);
uint32_t upb_FieldDef_Index(const upb_FieldDef* f);
bool upb_FieldDef_IsSubMessage(const upb_FieldDef* f);
bool upb_FieldDef_IsString(const upb_FieldDef* f);
bool upb_FieldDef_IsRepeated(const upb_FieldDef* f);
bool upb_FieldDef_IsPrimitive(const upb_FieldDef* f);
bool upb_FieldDef_IsMap(const upb_FieldDef* f);
bool upb_FieldDef_HasDefault(const upb_FieldDef* f);
bool upb_FieldDef_HasSubDef(const upb_FieldDef* f);
bool upb_FieldDef_HasPresence(const upb_FieldDef* f);
const upb_MessageDef* upb_FieldDef_MessageSubDef(const upb_FieldDef* f);
const upb_EnumDef* upb_FieldDef_EnumSubDef(const upb_FieldDef* f);
const upb_MiniTable_Field* upb_FieldDef_MiniTable(const upb_FieldDef* f);
const upb_MiniTable_Extension* _upb_FieldDef_ExtensionMiniTable(
    const upb_FieldDef* f);
bool _upb_FieldDef_IsProto3Optional(const upb_FieldDef* f);

/* upb_OneofDef ***************************************************************/

const google_protobuf_OneofOptions* upb_OneofDef_Options(const upb_OneofDef* o);
bool upb_OneofDef_HasOptions(const upb_OneofDef* o);
const char* upb_OneofDef_Name(const upb_OneofDef* o);
const upb_MessageDef* upb_OneofDef_ContainingType(const upb_OneofDef* o);
uint32_t upb_OneofDef_Index(const upb_OneofDef* o);
bool upb_OneofDef_IsSynthetic(const upb_OneofDef* o);
int upb_OneofDef_FieldCount(const upb_OneofDef* o);
const upb_FieldDef* upb_OneofDef_Field(const upb_OneofDef* o, int i);

/* Oneof lookups:
 * - ntof:  look up a field by name.
 * - ntofz: look up a field by name (as a null-terminated string).
 * - itof:  look up a field by number. */
const upb_FieldDef* upb_OneofDef_LookupNameWithSize(const upb_OneofDef* o,
                                                    const char* name,
                                                    size_t length);
UPB_INLINE const upb_FieldDef* upb_OneofDef_LookupName(const upb_OneofDef* o,
                                                       const char* name) {
  return upb_OneofDef_LookupNameWithSize(o, name, strlen(name));
}
const upb_FieldDef* upb_OneofDef_LookupNumber(const upb_OneofDef* o,
                                              uint32_t num);

/* upb_MessageDef *************************************************************/

/* Well-known field tag numbers for map-entry messages. */
#define kUpb_MapEntry_KeyFieldNumber 1
#define kUpb_MapEntry_ValueFieldNumber 2

/* Well-known field tag numbers for Any messages. */
#define kUpb_Any_TypeFieldNumber 1
#define kUpb_Any_ValueFieldNumber 2

/* Well-known field tag numbers for timestamp messages. */
#define kUpb_Duration_SecondsFieldNumber 1
#define kUpb_Duration_NanosFieldNumber 2

/* Well-known field tag numbers for duration messages. */
#define kUpb_Timestamp_SecondsFieldNumber 1
#define kUpb_Timestamp_NanosFieldNumber 2

const google_protobuf_MessageOptions* upb_MessageDef_Options(
    const upb_MessageDef* m);
bool upb_MessageDef_HasOptions(const upb_MessageDef* m);
const char* upb_MessageDef_FullName(const upb_MessageDef* m);
const upb_FileDef* upb_MessageDef_File(const upb_MessageDef* m);
const upb_MessageDef* upb_MessageDef_ContainingType(const upb_MessageDef* m);
const char* upb_MessageDef_Name(const upb_MessageDef* m);
upb_Syntax upb_MessageDef_Syntax(const upb_MessageDef* m);
upb_WellKnown upb_MessageDef_WellKnownType(const upb_MessageDef* m);
int upb_MessageDef_ExtensionRangeCount(const upb_MessageDef* m);
int upb_MessageDef_FieldCount(const upb_MessageDef* m);
int upb_MessageDef_OneofCount(const upb_MessageDef* m);
const upb_ExtensionRange* upb_MessageDef_ExtensionRange(const upb_MessageDef* m,
                                                        int i);
const upb_FieldDef* upb_MessageDef_Field(const upb_MessageDef* m, int i);
const upb_OneofDef* upb_MessageDef_Oneof(const upb_MessageDef* m, int i);
const upb_FieldDef* upb_MessageDef_FindFieldByNumber(const upb_MessageDef* m,
                                                     uint32_t i);
const upb_FieldDef* upb_MessageDef_FindFieldByNameWithSize(
    const upb_MessageDef* m, const char* name, size_t len);
const upb_OneofDef* upb_MessageDef_FindOneofByNameWithSize(
    const upb_MessageDef* m, const char* name, size_t len);
const upb_MiniTable* upb_MessageDef_MiniTable(const upb_MessageDef* m);

UPB_INLINE const upb_OneofDef* upb_MessageDef_FindOneofByName(
    const upb_MessageDef* m, const char* name) {
  return upb_MessageDef_FindOneofByNameWithSize(m, name, strlen(name));
}

UPB_INLINE const upb_FieldDef* upb_MessageDef_FindFieldByName(
    const upb_MessageDef* m, const char* name) {
  return upb_MessageDef_FindFieldByNameWithSize(m, name, strlen(name));
}

UPB_INLINE bool upb_MessageDef_IsMapEntry(const upb_MessageDef* m) {
  return google_protobuf_MessageOptions_map_entry(upb_MessageDef_Options(m));
}

/* Nested entities. */
int upb_MessageDef_NestedMessageCount(const upb_MessageDef* m);
int upb_MessageDef_NestedEnumCount(const upb_MessageDef* m);
int upb_MessageDef_NestedExtensionCount(const upb_MessageDef* m);
const upb_MessageDef* upb_MessageDef_NestedMessage(const upb_MessageDef* m,
                                                   int i);
const upb_EnumDef* upb_MessageDef_NestedEnum(const upb_MessageDef* m, int i);
const upb_FieldDef* upb_MessageDef_NestedExtension(const upb_MessageDef* m,
                                                   int i);

/* Lookup of either field or oneof by name.  Returns whether either was found.
 * If the return is true, then the found def will be set, and the non-found
 * one set to NULL. */
bool upb_MessageDef_FindByNameWithSize(const upb_MessageDef* m,
                                       const char* name, size_t len,
                                       const upb_FieldDef** f,
                                       const upb_OneofDef** o);

UPB_INLINE bool upb_MessageDef_FindByName(const upb_MessageDef* m,
                                          const char* name,
                                          const upb_FieldDef** f,
                                          const upb_OneofDef** o) {
  return upb_MessageDef_FindByNameWithSize(m, name, strlen(name), f, o);
}

/* Returns a field by either JSON name or regular proto name. */
const upb_FieldDef* upb_MessageDef_FindByJsonNameWithSize(
    const upb_MessageDef* m, const char* name, size_t len);
UPB_INLINE const upb_FieldDef* upb_MessageDef_FindByJsonName(
    const upb_MessageDef* m, const char* name) {
  return upb_MessageDef_FindByJsonNameWithSize(m, name, strlen(name));
}

/* upb_ExtensionRange *********************************************************/

const google_protobuf_ExtensionRangeOptions* upb_ExtensionRange_Options(
    const upb_ExtensionRange* r);
bool upb_ExtensionRange_HasOptions(const upb_ExtensionRange* r);
int32_t upb_ExtensionRange_Start(const upb_ExtensionRange* r);
int32_t upb_ExtensionRange_End(const upb_ExtensionRange* r);

/* upb_EnumDef ****************************************************************/

const google_protobuf_EnumOptions* upb_EnumDef_Options(const upb_EnumDef* e);
bool upb_EnumDef_HasOptions(const upb_EnumDef* e);
const char* upb_EnumDef_FullName(const upb_EnumDef* e);
const char* upb_EnumDef_Name(const upb_EnumDef* e);
const upb_FileDef* upb_EnumDef_File(const upb_EnumDef* e);
const upb_MessageDef* upb_EnumDef_ContainingType(const upb_EnumDef* e);
int32_t upb_EnumDef_Default(const upb_EnumDef* e);
int upb_EnumDef_ValueCount(const upb_EnumDef* e);
const upb_EnumValueDef* upb_EnumDef_Value(const upb_EnumDef* e, int i);

const upb_EnumValueDef* upb_EnumDef_FindValueByNameWithSize(
    const upb_EnumDef* e, const char* name, size_t len);
const upb_EnumValueDef* upb_EnumDef_FindValueByNumber(const upb_EnumDef* e,
                                                      int32_t num);
bool upb_EnumDef_CheckNumber(const upb_EnumDef* e, int32_t num);

// Convenience wrapper.
UPB_INLINE const upb_EnumValueDef* upb_EnumDef_FindValueByName(
    const upb_EnumDef* e, const char* name) {
  return upb_EnumDef_FindValueByNameWithSize(e, name, strlen(name));
}

/* upb_EnumValueDef ***********************************************************/

const google_protobuf_EnumValueOptions* upb_EnumValueDef_Options(
    const upb_EnumValueDef* e);
bool upb_EnumValueDef_HasOptions(const upb_EnumValueDef* e);
const char* upb_EnumValueDef_FullName(const upb_EnumValueDef* e);
const char* upb_EnumValueDef_Name(const upb_EnumValueDef* e);
int32_t upb_EnumValueDef_Number(const upb_EnumValueDef* e);
uint32_t upb_EnumValueDef_Index(const upb_EnumValueDef* e);
const upb_EnumDef* upb_EnumValueDef_Enum(const upb_EnumValueDef* e);

/* upb_FileDef ****************************************************************/

const google_protobuf_FileOptions* upb_FileDef_Options(const upb_FileDef* f);
bool upb_FileDef_HasOptions(const upb_FileDef* f);
const char* upb_FileDef_Name(const upb_FileDef* f);
const char* upb_FileDef_Package(const upb_FileDef* f);
upb_Syntax upb_FileDef_Syntax(const upb_FileDef* f);
int upb_FileDef_DependencyCount(const upb_FileDef* f);
int upb_FileDef_PublicDependencyCount(const upb_FileDef* f);
int upb_FileDef_WeakDependencyCount(const upb_FileDef* f);
int upb_FileDef_TopLevelMessageCount(const upb_FileDef* f);
int upb_FileDef_TopLevelEnumCount(const upb_FileDef* f);
int upb_FileDef_TopLevelExtensionCount(const upb_FileDef* f);
int upb_FileDef_ServiceCount(const upb_FileDef* f);
const upb_FileDef* upb_FileDef_Dependency(const upb_FileDef* f, int i);
const upb_FileDef* upb_FileDef_PublicDependency(const upb_FileDef* f, int i);
const upb_FileDef* upb_FileDef_WeakDependency(const upb_FileDef* f, int i);
const upb_MessageDef* upb_FileDef_TopLevelMessage(const upb_FileDef* f, int i);
const upb_EnumDef* upb_FileDef_TopLevelEnum(const upb_FileDef* f, int i);
const upb_FieldDef* upb_FileDef_TopLevelExtension(const upb_FileDef* f, int i);
const upb_ServiceDef* upb_FileDef_Service(const upb_FileDef* f, int i);
const upb_DefPool* upb_FileDef_Pool(const upb_FileDef* f);
const int32_t* _upb_FileDef_PublicDependencyIndexes(const upb_FileDef* f);
const int32_t* _upb_FileDef_WeakDependencyIndexes(const upb_FileDef* f);

/* upb_MethodDef **************************************************************/

const google_protobuf_MethodOptions* upb_MethodDef_Options(
    const upb_MethodDef* m);
bool upb_MethodDef_HasOptions(const upb_MethodDef* m);
const char* upb_MethodDef_FullName(const upb_MethodDef* m);
int upb_MethodDef_Index(const upb_MethodDef* m);
const char* upb_MethodDef_Name(const upb_MethodDef* m);
const upb_ServiceDef* upb_MethodDef_Service(const upb_MethodDef* m);
const upb_MessageDef* upb_MethodDef_InputType(const upb_MethodDef* m);
const upb_MessageDef* upb_MethodDef_OutputType(const upb_MethodDef* m);
bool upb_MethodDef_ClientStreaming(const upb_MethodDef* m);
bool upb_MethodDef_ServerStreaming(const upb_MethodDef* m);

/* upb_ServiceDef *************************************************************/

const google_protobuf_ServiceOptions* upb_ServiceDef_Options(
    const upb_ServiceDef* s);
bool upb_ServiceDef_HasOptions(const upb_ServiceDef* s);
const char* upb_ServiceDef_FullName(const upb_ServiceDef* s);
const char* upb_ServiceDef_Name(const upb_ServiceDef* s);
int upb_ServiceDef_Index(const upb_ServiceDef* s);
const upb_FileDef* upb_ServiceDef_File(const upb_ServiceDef* s);
int upb_ServiceDef_MethodCount(const upb_ServiceDef* s);
const upb_MethodDef* upb_ServiceDef_Method(const upb_ServiceDef* s, int i);
const upb_MethodDef* upb_ServiceDef_FindMethodByName(const upb_ServiceDef* s,
                                                     const char* name);

/* upb_DefPool ****************************************************************/

upb_DefPool* upb_DefPool_New(void);
void upb_DefPool_Free(upb_DefPool* s);
const upb_MessageDef* upb_DefPool_FindMessageByName(const upb_DefPool* s,
                                                    const char* sym);
const upb_MessageDef* upb_DefPool_FindMessageByNameWithSize(
    const upb_DefPool* s, const char* sym, size_t len);
const upb_EnumDef* upb_DefPool_FindEnumByName(const upb_DefPool* s,
                                              const char* sym);
const upb_EnumValueDef* upb_DefPool_FindEnumByNameval(const upb_DefPool* s,
                                                      const char* sym);
const upb_FieldDef* upb_DefPool_FindExtensionByName(const upb_DefPool* s,
                                                    const char* sym);
const upb_FieldDef* upb_DefPool_FindExtensionByNameWithSize(
    const upb_DefPool* s, const char* sym, size_t len);
const upb_FileDef* upb_DefPool_FindFileByName(const upb_DefPool* s,
                                              const char* name);
const upb_ServiceDef* upb_DefPool_FindServiceByName(const upb_DefPool* s,
                                                    const char* name);
const upb_ServiceDef* upb_DefPool_FindServiceByNameWithSize(
    const upb_DefPool* s, const char* name, size_t size);
const upb_FileDef* upb_DefPool_FindFileContainingSymbol(const upb_DefPool* s,
                                                        const char* name);
const upb_FileDef* upb_DefPool_FindFileByNameWithSize(const upb_DefPool* s,
                                                      const char* name,
                                                      size_t len);
const upb_FileDef* upb_DefPool_AddFile(
    upb_DefPool* s, const google_protobuf_FileDescriptorProto* file,
    upb_Status* status);
size_t _upb_DefPool_BytesLoaded(const upb_DefPool* s);
upb_Arena* _upb_DefPool_Arena(const upb_DefPool* s);
const upb_FieldDef* _upb_DefPool_FindExtensionByMiniTable(
    const upb_DefPool* s, const upb_MiniTable_Extension* ext);
const upb_FieldDef* upb_DefPool_FindExtensionByNumber(const upb_DefPool* s,
                                                      const upb_MessageDef* m,
                                                      int32_t fieldnum);
const upb_ExtensionRegistry* upb_DefPool_ExtensionRegistry(
    const upb_DefPool* s);
const upb_FieldDef** upb_DefPool_GetAllExtensions(const upb_DefPool* s,
                                                  const upb_MessageDef* m,
                                                  size_t* count);

/* For generated code only: loads a generated descriptor. */
typedef struct _upb_DefPool_Init {
  struct _upb_DefPool_Init** deps; /* Dependencies of this file. */
  const upb_MiniTable_File* layout;
  const char* filename;
  upb_StringView descriptor; /* Serialized descriptor. */
} _upb_DefPool_Init;

// Should only be directly called by tests.  This variant lets us suppress
// the use of compiled-in tables, forcing a rebuild of the tables at runtime.
bool _upb_DefPool_LoadDefInitEx(upb_DefPool* s, const _upb_DefPool_Init* init,
                                bool rebuild_minitable);

UPB_INLINE bool _upb_DefPool_LoadDefInit(upb_DefPool* s,
                                         const _upb_DefPool_Init* init) {
  return _upb_DefPool_LoadDefInitEx(s, init, false);
}


#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* UPB_DEF_H_ */

/** upb/reflection.h ************************************************************/
#ifndef UPB_REFLECTION_H_
#define UPB_REFLECTION_H_


#ifdef __cplusplus
extern "C" {
#endif

typedef union {
  bool bool_val;
  float float_val;
  double double_val;
  int32_t int32_val;
  int64_t int64_val;
  uint32_t uint32_val;
  uint64_t uint64_val;
  const upb_Map* map_val;
  const upb_Message* msg_val;
  const upb_Array* array_val;
  upb_StringView str_val;
} upb_MessageValue;

typedef union {
  upb_Map* map;
  upb_Message* msg;
  upb_Array* array;
} upb_MutableMessageValue;

upb_MessageValue upb_FieldDef_Default(const upb_FieldDef* f);

/** upb_Message
 * *******************************************************************/

/* Creates a new message of the given type in the given arena. */
upb_Message* upb_Message_New(const upb_MessageDef* m, upb_Arena* a);

/* Returns the value associated with this field. */
upb_MessageValue upb_Message_Get(const upb_Message* msg, const upb_FieldDef* f);

/* Returns a mutable pointer to a map, array, or submessage value.  If the given
 * arena is non-NULL this will construct a new object if it was not previously
 * present.  May not be called for primitive fields. */
upb_MutableMessageValue upb_Message_Mutable(upb_Message* msg,
                                            const upb_FieldDef* f,
                                            upb_Arena* a);

/* May only be called for fields where upb_FieldDef_HasPresence(f) == true. */
bool upb_Message_Has(const upb_Message* msg, const upb_FieldDef* f);

/* Returns the field that is set in the oneof, or NULL if none are set. */
const upb_FieldDef* upb_Message_WhichOneof(const upb_Message* msg,
                                           const upb_OneofDef* o);

/* Sets the given field to the given value.  For a msg/array/map/string, the
 * caller must ensure that the target data outlives |msg| (by living either in
 * the same arena or a different arena that outlives it).
 *
 * Returns false if allocation fails. */
bool upb_Message_Set(upb_Message* msg, const upb_FieldDef* f,
                     upb_MessageValue val, upb_Arena* a);

/* Clears any field presence and sets the value back to its default. */
void upb_Message_ClearField(upb_Message* msg, const upb_FieldDef* f);

/* Clear all data and unknown fields. */
void upb_Message_Clear(upb_Message* msg, const upb_MessageDef* m);

/* Iterate over present fields.
 *
 * size_t iter = kUpb_Message_Begin;
 * const upb_FieldDef *f;
 * upb_MessageValue val;
 * while (upb_Message_Next(msg, m, ext_pool, &f, &val, &iter)) {
 *   process_field(f, val);
 * }
 *
 * If ext_pool is NULL, no extensions will be returned.  If the given symtab
 * returns extensions that don't match what is in this message, those extensions
 * will be skipped.
 */

#define kUpb_Message_Begin -1
bool upb_Message_Next(const upb_Message* msg, const upb_MessageDef* m,
                      const upb_DefPool* ext_pool, const upb_FieldDef** f,
                      upb_MessageValue* val, size_t* iter);

/* Clears all unknown field data from this message and all submessages. */
bool upb_Message_DiscardUnknown(upb_Message* msg, const upb_MessageDef* m,
                                int maxdepth);

/** upb_Array *****************************************************************/

/* Creates a new array on the given arena that holds elements of this type. */
upb_Array* upb_Array_New(upb_Arena* a, upb_CType type);

/* Returns the size of the array. */
size_t upb_Array_Size(const upb_Array* arr);

/* Returns the given element, which must be within the array's current size. */
upb_MessageValue upb_Array_Get(const upb_Array* arr, size_t i);

/* Sets the given element, which must be within the array's current size. */
void upb_Array_Set(upb_Array* arr, size_t i, upb_MessageValue val);

/* Appends an element to the array.  Returns false on allocation failure. */
bool upb_Array_Append(upb_Array* array, upb_MessageValue val, upb_Arena* arena);

/* Moves elements within the array using memmove(). Like memmove(), the source
 * and destination elements may be overlapping. */
void upb_Array_Move(upb_Array* array, size_t dst_idx, size_t src_idx,
                    size_t count);

/* Inserts one or more empty elements into the array.  Existing elements are
 * shifted right.  The new elements have undefined state and must be set with
 * `upb_Array_Set()`.
 * REQUIRES: `i <= upb_Array_Size(arr)` */
bool upb_Array_Insert(upb_Array* array, size_t i, size_t count,
                      upb_Arena* arena);

/* Deletes one or more elements from the array.  Existing elements are shifted
 * left.
 * REQUIRES: `i + count <= upb_Array_Size(arr)` */
void upb_Array_Delete(upb_Array* array, size_t i, size_t count);

/* Changes the size of a vector.  New elements are initialized to empty/0.
 * Returns false on allocation failure. */
bool upb_Array_Resize(upb_Array* array, size_t size, upb_Arena* arena);

/** upb_Map *******************************************************************/

/* Creates a new map on the given arena with the given key/value size. */
upb_Map* upb_Map_New(upb_Arena* a, upb_CType key_type, upb_CType value_type);

/* Returns the number of entries in the map. */
size_t upb_Map_Size(const upb_Map* map);

/* Stores a value for the given key into |*val| (or the zero value if the key is
 * not present).  Returns whether the key was present.  The |val| pointer may be
 * NULL, in which case the function tests whether the given key is present.  */
bool upb_Map_Get(const upb_Map* map, upb_MessageValue key,
                 upb_MessageValue* val);

/* Removes all entries in the map. */
void upb_Map_Clear(upb_Map* map);

/* Sets the given key to the given value.  Returns true if this was a new key in
 * the map, or false if an existing key was replaced. */
bool upb_Map_Set(upb_Map* map, upb_MessageValue key, upb_MessageValue val,
                 upb_Arena* arena);

/* Deletes this key from the table.  Returns true if the key was present. */
bool upb_Map_Delete(upb_Map* map, upb_MessageValue key);

/* Map iteration:
 *
 * size_t iter = kUpb_Map_Begin;
 * while (upb_MapIterator_Next(map, &iter)) {
 *   upb_MessageValue key = upb_MapIterator_Key(map, iter);
 *   upb_MessageValue val = upb_MapIterator_Value(map, iter);
 *
 *   // If mutating is desired.
 *   upb_MapIterator_SetValue(map, iter, value2);
 * }
 */

/* Advances to the next entry.  Returns false if no more entries are present. */
bool upb_MapIterator_Next(const upb_Map* map, size_t* iter);

/* Returns true if the iterator still points to a valid entry, or false if the
 * iterator is past the last element. It is an error to call this function with
 * kUpb_Map_Begin (you must call next() at least once first). */
bool upb_MapIterator_Done(const upb_Map* map, size_t iter);

/* Returns the key and value for this entry of the map. */
upb_MessageValue upb_MapIterator_Key(const upb_Map* map, size_t iter);
upb_MessageValue upb_MapIterator_Value(const upb_Map* map, size_t iter);

/* Sets the value for this entry.  The iterator must not be done, and the
 * iterator must not have been initialized const. */
void upb_MapIterator_SetValue(upb_Map* map, size_t iter,
                              upb_MessageValue value);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* UPB_REFLECTION_H_ */

/** upb/json_decode.h ************************************************************/
#ifndef UPB_JSONDECODE_H_
#define UPB_JSONDECODE_H_


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

/** upb/json_encode.h ************************************************************/
#ifndef UPB_JSONENCODE_H_
#define UPB_JSONENCODE_H_


#ifdef __cplusplus
extern "C" {
#endif

enum {
  /* When set, emits 0/default values.  TODO(haberman): proto3 only? */
  upb_JsonEncode_EmitDefaults = 1,

  /* When set, use normal (snake_caes) field names instead of JSON (camelCase)
     names. */
  upb_JsonEncode_UseProtoNames = 2
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

/** upb/port_undef.inc ************************************************************/
/* See port_def.inc.  This should #undef all macros #defined there. */

#undef UPB_SIZE
#undef UPB_PTR_AT
#undef UPB_READ_ONEOF
#undef UPB_WRITE_ONEOF
#undef UPB_MAPTYPE_STRING
#undef UPB_INLINE
#undef UPB_ALIGN_UP
#undef UPB_ALIGN_DOWN
#undef UPB_ALIGN_MALLOC
#undef UPB_ALIGN_OF
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
#undef UPB_FASTTABLE
#undef UPB_FASTTABLE_INIT
#undef UPB_POISON_MEMORY_REGION
#undef UPB_UNPOISON_MEMORY_REGION
#undef UPB_ASAN
#undef UPB_TREAT_PROTO2_ENUMS_LIKE_PROTO3
