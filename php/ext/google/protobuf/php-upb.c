/* Amalgamated source file */
#include "php-upb.h"
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

#define UPB_MALLOC_ALIGN 8
#define UPB_ALIGN_UP(size, align) (((size) + (align) - 1) / (align) * (align))
#define UPB_ALIGN_DOWN(size, align) ((size) / (align) * (align))
#define UPB_ALIGN_MALLOC(size) UPB_ALIGN_UP(size, UPB_MALLOC_ALIGN)
#define UPB_ALIGN_OF(type) offsetof (struct { char c; type member; }, member)

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

/** upb/collections.c ************************************************************/

#include <string.h>


/* Strings/bytes are special-cased in maps. */
static char _upb_CTypeo_mapsize[12] = {
    0,
    1,             /* kUpb_CType_Bool */
    4,             /* kUpb_CType_Float */
    4,             /* kUpb_CType_Int32 */
    4,             /* kUpb_CType_UInt32 */
    4,             /* kUpb_CType_Enum */
    sizeof(void*), /* kUpb_CType_Message */
    8,             /* kUpb_CType_Double */
    8,             /* kUpb_CType_Int64 */
    8,             /* kUpb_CType_UInt64 */
    0,             /* kUpb_CType_String */
    0,             /* kUpb_CType_Bytes */
};

static const char _upb_CTypeo_sizelg2[12] = {
    0,
    0,              /* kUpb_CType_Bool */
    2,              /* kUpb_CType_Float */
    2,              /* kUpb_CType_Int32 */
    2,              /* kUpb_CType_UInt32 */
    2,              /* kUpb_CType_Enum */
    UPB_SIZE(2, 3), /* kUpb_CType_Message */
    3,              /* kUpb_CType_Double */
    3,              /* kUpb_CType_Int64 */
    3,              /* kUpb_CType_UInt64 */
    UPB_SIZE(3, 4), /* kUpb_CType_String */
    UPB_SIZE(3, 4), /* kUpb_CType_Bytes */
};

/** upb_Array *****************************************************************/

upb_Array* upb_Array_New(upb_Arena* a, upb_CType type) {
  return _upb_Array_New(a, 4, _upb_CTypeo_sizelg2[type]);
}

size_t upb_Array_Size(const upb_Array* arr) { return arr->len; }

upb_MessageValue upb_Array_Get(const upb_Array* arr, size_t i) {
  upb_MessageValue ret;
  const char* data = _upb_array_constptr(arr);
  int lg2 = arr->data & 7;
  UPB_ASSERT(i < arr->len);
  memcpy(&ret, data + (i << lg2), 1 << lg2);
  return ret;
}

void upb_Array_Set(upb_Array* arr, size_t i, upb_MessageValue val) {
  char* data = _upb_array_ptr(arr);
  int lg2 = arr->data & 7;
  UPB_ASSERT(i < arr->len);
  memcpy(data + (i << lg2), &val, 1 << lg2);
}

bool upb_Array_Append(upb_Array* arr, upb_MessageValue val, upb_Arena* arena) {
  if (!upb_Array_Resize(arr, arr->len + 1, arena)) {
    return false;
  }
  upb_Array_Set(arr, arr->len - 1, val);
  return true;
}

void upb_Array_Move(upb_Array* arr, size_t dst_idx, size_t src_idx,
                    size_t count) {
  char* data = _upb_array_ptr(arr);
  int lg2 = arr->data & 7;
  memmove(&data[dst_idx << lg2], &data[src_idx << lg2], count << lg2);
}

bool upb_Array_Insert(upb_Array* arr, size_t i, size_t count,
                      upb_Arena* arena) {
  UPB_ASSERT(i <= arr->len);
  UPB_ASSERT(count + arr->len >= count);
  size_t oldsize = arr->len;
  if (!upb_Array_Resize(arr, arr->len + count, arena)) {
    return false;
  }
  upb_Array_Move(arr, i + count, i, oldsize - i);
  return true;
}

/*
 *              i        end      arr->len
 * |------------|XXXXXXXX|--------|
 */
void upb_Array_Delete(upb_Array* arr, size_t i, size_t count) {
  size_t end = i + count;
  UPB_ASSERT(i <= end);
  UPB_ASSERT(end <= arr->len);
  upb_Array_Move(arr, i, end, arr->len - end);
  arr->len -= count;
}

bool upb_Array_Resize(upb_Array* arr, size_t size, upb_Arena* arena) {
  return _upb_Array_Resize(arr, size, arena);
}

/** upb_Map *******************************************************************/

upb_Map* upb_Map_New(upb_Arena* a, upb_CType key_type, upb_CType value_type) {
  return _upb_Map_New(a, _upb_CTypeo_mapsize[key_type],
                      _upb_CTypeo_mapsize[value_type]);
}

size_t upb_Map_Size(const upb_Map* map) { return _upb_Map_Size(map); }

bool upb_Map_Get(const upb_Map* map, upb_MessageValue key,
                 upb_MessageValue* val) {
  return _upb_Map_Get(map, &key, map->key_size, val, map->val_size);
}

void upb_Map_Clear(upb_Map* map) { _upb_Map_Clear(map); }

upb_MapInsertStatus upb_Map_Insert(upb_Map* map, upb_MessageValue key,
                                   upb_MessageValue val, upb_Arena* arena) {
  return (upb_MapInsertStatus)_upb_Map_Insert(map, &key, map->key_size, &val,
                                              map->val_size, arena);
}

bool upb_Map_Delete(upb_Map* map, upb_MessageValue key) {
  return _upb_Map_Delete(map, &key, map->key_size);
}

bool upb_MapIterator_Next(const upb_Map* map, size_t* iter) {
  return _upb_map_next(map, iter);
}

bool upb_MapIterator_Done(const upb_Map* map, size_t iter) {
  upb_strtable_iter i;
  UPB_ASSERT(iter != kUpb_Map_Begin);
  i.t = &map->table;
  i.index = iter;
  return upb_strtable_done(&i);
}

/* Returns the key and value for this entry of the map. */
upb_MessageValue upb_MapIterator_Key(const upb_Map* map, size_t iter) {
  upb_strtable_iter i;
  upb_MessageValue ret;
  i.t = &map->table;
  i.index = iter;
  _upb_map_fromkey(upb_strtable_iter_key(&i), &ret, map->key_size);
  return ret;
}

upb_MessageValue upb_MapIterator_Value(const upb_Map* map, size_t iter) {
  upb_strtable_iter i;
  upb_MessageValue ret;
  i.t = &map->table;
  i.index = iter;
  _upb_map_fromvalue(upb_strtable_iter_value(&i), &ret, map->val_size);
  return ret;
}

/* void upb_MapIterator_SetValue(upb_Map *map, size_t iter, upb_MessageValue
 * value); */

/** bazel-out/k8-fastbuild/bin/external/com_google_protobuf/google/protobuf/descriptor.upb.c ************************************************************//* This file was generated by upbc (the upb compiler) from the input
 * file:
 *
 *     google/protobuf/descriptor.proto
 *
 * Do not edit -- your changes will be discarded when the file is
 * regenerated. */

#include <stddef.h>


static const upb_MiniTable_Sub google_protobuf_FileDescriptorSet_submsgs[1] = {
  {.submsg = &google_protobuf_FileDescriptorProto_msginit},
};

static const upb_MiniTable_Field google_protobuf_FileDescriptorSet__fields[1] = {
  {1, UPB_SIZE(0, 0), UPB_SIZE(0, 0), 0, 11, kUpb_FieldMode_Array | (kUpb_FieldRep_Pointer << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_FileDescriptorSet_msginit = {
  &google_protobuf_FileDescriptorSet_submsgs[0],
  &google_protobuf_FileDescriptorSet__fields[0],
  UPB_SIZE(8, 8), 1, kUpb_ExtMode_NonExtendable, 1, 255, 0,
};

static const upb_MiniTable_Sub google_protobuf_FileDescriptorProto_submsgs[6] = {
  {.submsg = &google_protobuf_DescriptorProto_msginit},
  {.submsg = &google_protobuf_EnumDescriptorProto_msginit},
  {.submsg = &google_protobuf_ServiceDescriptorProto_msginit},
  {.submsg = &google_protobuf_FieldDescriptorProto_msginit},
  {.submsg = &google_protobuf_FileOptions_msginit},
  {.submsg = &google_protobuf_SourceCodeInfo_msginit},
};

static const upb_MiniTable_Field google_protobuf_FileDescriptorProto__fields[12] = {
  {1, UPB_SIZE(4, 8), UPB_SIZE(1, 1), kUpb_NoSub, 12, kUpb_FieldMode_Scalar | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {2, UPB_SIZE(12, 24), UPB_SIZE(2, 2), kUpb_NoSub, 12, kUpb_FieldMode_Scalar | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {3, UPB_SIZE(20, 40), UPB_SIZE(0, 0), kUpb_NoSub, 12, kUpb_FieldMode_Array | (kUpb_FieldRep_Pointer << kUpb_FieldRep_Shift)},
  {4, UPB_SIZE(24, 48), UPB_SIZE(0, 0), 0, 11, kUpb_FieldMode_Array | (kUpb_FieldRep_Pointer << kUpb_FieldRep_Shift)},
  {5, UPB_SIZE(28, 56), UPB_SIZE(0, 0), 1, 11, kUpb_FieldMode_Array | (kUpb_FieldRep_Pointer << kUpb_FieldRep_Shift)},
  {6, UPB_SIZE(32, 64), UPB_SIZE(0, 0), 2, 11, kUpb_FieldMode_Array | (kUpb_FieldRep_Pointer << kUpb_FieldRep_Shift)},
  {7, UPB_SIZE(36, 72), UPB_SIZE(0, 0), 3, 11, kUpb_FieldMode_Array | (kUpb_FieldRep_Pointer << kUpb_FieldRep_Shift)},
  {8, UPB_SIZE(40, 80), UPB_SIZE(3, 3), 4, 11, kUpb_FieldMode_Scalar | (kUpb_FieldRep_Pointer << kUpb_FieldRep_Shift)},
  {9, UPB_SIZE(44, 88), UPB_SIZE(4, 4), 5, 11, kUpb_FieldMode_Scalar | (kUpb_FieldRep_Pointer << kUpb_FieldRep_Shift)},
  {10, UPB_SIZE(48, 96), UPB_SIZE(0, 0), kUpb_NoSub, 5, kUpb_FieldMode_Array | (kUpb_FieldRep_Pointer << kUpb_FieldRep_Shift)},
  {11, UPB_SIZE(52, 104), UPB_SIZE(0, 0), kUpb_NoSub, 5, kUpb_FieldMode_Array | (kUpb_FieldRep_Pointer << kUpb_FieldRep_Shift)},
  {12, UPB_SIZE(56, 112), UPB_SIZE(5, 5), kUpb_NoSub, 12, kUpb_FieldMode_Scalar | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_FileDescriptorProto_msginit = {
  &google_protobuf_FileDescriptorProto_submsgs[0],
  &google_protobuf_FileDescriptorProto__fields[0],
  UPB_SIZE(64, 128), 12, kUpb_ExtMode_NonExtendable, 12, 255, 0,
};

static const upb_MiniTable_Sub google_protobuf_DescriptorProto_submsgs[8] = {
  {.submsg = &google_protobuf_FieldDescriptorProto_msginit},
  {.submsg = &google_protobuf_DescriptorProto_msginit},
  {.submsg = &google_protobuf_EnumDescriptorProto_msginit},
  {.submsg = &google_protobuf_DescriptorProto_ExtensionRange_msginit},
  {.submsg = &google_protobuf_FieldDescriptorProto_msginit},
  {.submsg = &google_protobuf_MessageOptions_msginit},
  {.submsg = &google_protobuf_OneofDescriptorProto_msginit},
  {.submsg = &google_protobuf_DescriptorProto_ReservedRange_msginit},
};

static const upb_MiniTable_Field google_protobuf_DescriptorProto__fields[10] = {
  {1, UPB_SIZE(4, 8), UPB_SIZE(1, 1), kUpb_NoSub, 12, kUpb_FieldMode_Scalar | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {2, UPB_SIZE(12, 24), UPB_SIZE(0, 0), 0, 11, kUpb_FieldMode_Array | (kUpb_FieldRep_Pointer << kUpb_FieldRep_Shift)},
  {3, UPB_SIZE(16, 32), UPB_SIZE(0, 0), 1, 11, kUpb_FieldMode_Array | (kUpb_FieldRep_Pointer << kUpb_FieldRep_Shift)},
  {4, UPB_SIZE(20, 40), UPB_SIZE(0, 0), 2, 11, kUpb_FieldMode_Array | (kUpb_FieldRep_Pointer << kUpb_FieldRep_Shift)},
  {5, UPB_SIZE(24, 48), UPB_SIZE(0, 0), 3, 11, kUpb_FieldMode_Array | (kUpb_FieldRep_Pointer << kUpb_FieldRep_Shift)},
  {6, UPB_SIZE(28, 56), UPB_SIZE(0, 0), 4, 11, kUpb_FieldMode_Array | (kUpb_FieldRep_Pointer << kUpb_FieldRep_Shift)},
  {7, UPB_SIZE(32, 64), UPB_SIZE(2, 2), 5, 11, kUpb_FieldMode_Scalar | (kUpb_FieldRep_Pointer << kUpb_FieldRep_Shift)},
  {8, UPB_SIZE(36, 72), UPB_SIZE(0, 0), 6, 11, kUpb_FieldMode_Array | (kUpb_FieldRep_Pointer << kUpb_FieldRep_Shift)},
  {9, UPB_SIZE(40, 80), UPB_SIZE(0, 0), 7, 11, kUpb_FieldMode_Array | (kUpb_FieldRep_Pointer << kUpb_FieldRep_Shift)},
  {10, UPB_SIZE(44, 88), UPB_SIZE(0, 0), kUpb_NoSub, 12, kUpb_FieldMode_Array | (kUpb_FieldRep_Pointer << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_DescriptorProto_msginit = {
  &google_protobuf_DescriptorProto_submsgs[0],
  &google_protobuf_DescriptorProto__fields[0],
  UPB_SIZE(48, 96), 10, kUpb_ExtMode_NonExtendable, 10, 255, 0,
};

static const upb_MiniTable_Sub google_protobuf_DescriptorProto_ExtensionRange_submsgs[1] = {
  {.submsg = &google_protobuf_ExtensionRangeOptions_msginit},
};

static const upb_MiniTable_Field google_protobuf_DescriptorProto_ExtensionRange__fields[3] = {
  {1, UPB_SIZE(4, 4), UPB_SIZE(1, 1), kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)},
  {2, UPB_SIZE(8, 8), UPB_SIZE(2, 2), kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)},
  {3, UPB_SIZE(12, 16), UPB_SIZE(3, 3), 0, 11, kUpb_FieldMode_Scalar | (kUpb_FieldRep_Pointer << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_DescriptorProto_ExtensionRange_msginit = {
  &google_protobuf_DescriptorProto_ExtensionRange_submsgs[0],
  &google_protobuf_DescriptorProto_ExtensionRange__fields[0],
  UPB_SIZE(16, 24), 3, kUpb_ExtMode_NonExtendable, 3, 255, 0,
};

static const upb_MiniTable_Field google_protobuf_DescriptorProto_ReservedRange__fields[2] = {
  {1, UPB_SIZE(4, 4), UPB_SIZE(1, 1), kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)},
  {2, UPB_SIZE(8, 8), UPB_SIZE(2, 2), kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_DescriptorProto_ReservedRange_msginit = {
  NULL,
  &google_protobuf_DescriptorProto_ReservedRange__fields[0],
  UPB_SIZE(16, 16), 2, kUpb_ExtMode_NonExtendable, 2, 255, 0,
};

static const upb_MiniTable_Sub google_protobuf_ExtensionRangeOptions_submsgs[1] = {
  {.submsg = &google_protobuf_UninterpretedOption_msginit},
};

static const upb_MiniTable_Field google_protobuf_ExtensionRangeOptions__fields[1] = {
  {999, UPB_SIZE(0, 0), UPB_SIZE(0, 0), 0, 11, kUpb_FieldMode_Array | (kUpb_FieldRep_Pointer << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_ExtensionRangeOptions_msginit = {
  &google_protobuf_ExtensionRangeOptions_submsgs[0],
  &google_protobuf_ExtensionRangeOptions__fields[0],
  UPB_SIZE(8, 8), 1, kUpb_ExtMode_Extendable, 0, 255, 0,
};

static const upb_MiniTable_Sub google_protobuf_FieldDescriptorProto_submsgs[3] = {
  {.subenum = &google_protobuf_FieldDescriptorProto_Label_enuminit},
  {.subenum = &google_protobuf_FieldDescriptorProto_Type_enuminit},
  {.submsg = &google_protobuf_FieldOptions_msginit},
};

static const upb_MiniTable_Field google_protobuf_FieldDescriptorProto__fields[11] = {
  {1, UPB_SIZE(24, 24), UPB_SIZE(1, 1), kUpb_NoSub, 12, kUpb_FieldMode_Scalar | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {2, UPB_SIZE(32, 40), UPB_SIZE(2, 2), kUpb_NoSub, 12, kUpb_FieldMode_Scalar | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {3, UPB_SIZE(4, 4), UPB_SIZE(3, 3), kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)},
  {4, UPB_SIZE(8, 8), UPB_SIZE(4, 4), 0, 14, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)},
  {5, UPB_SIZE(12, 12), UPB_SIZE(5, 5), 1, 14, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)},
  {6, UPB_SIZE(40, 56), UPB_SIZE(6, 6), kUpb_NoSub, 12, kUpb_FieldMode_Scalar | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {7, UPB_SIZE(48, 72), UPB_SIZE(7, 7), kUpb_NoSub, 12, kUpb_FieldMode_Scalar | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {8, UPB_SIZE(56, 88), UPB_SIZE(8, 8), 2, 11, kUpb_FieldMode_Scalar | (kUpb_FieldRep_Pointer << kUpb_FieldRep_Shift)},
  {9, UPB_SIZE(16, 16), UPB_SIZE(9, 9), kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)},
  {10, UPB_SIZE(60, 96), UPB_SIZE(10, 10), kUpb_NoSub, 12, kUpb_FieldMode_Scalar | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {17, UPB_SIZE(20, 20), UPB_SIZE(11, 11), kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_FieldDescriptorProto_msginit = {
  &google_protobuf_FieldDescriptorProto_submsgs[0],
  &google_protobuf_FieldDescriptorProto__fields[0],
  UPB_SIZE(72, 112), 11, kUpb_ExtMode_NonExtendable, 10, 255, 0,
};

static const upb_MiniTable_Sub google_protobuf_OneofDescriptorProto_submsgs[1] = {
  {.submsg = &google_protobuf_OneofOptions_msginit},
};

static const upb_MiniTable_Field google_protobuf_OneofDescriptorProto__fields[2] = {
  {1, UPB_SIZE(4, 8), UPB_SIZE(1, 1), kUpb_NoSub, 12, kUpb_FieldMode_Scalar | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {2, UPB_SIZE(12, 24), UPB_SIZE(2, 2), 0, 11, kUpb_FieldMode_Scalar | (kUpb_FieldRep_Pointer << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_OneofDescriptorProto_msginit = {
  &google_protobuf_OneofDescriptorProto_submsgs[0],
  &google_protobuf_OneofDescriptorProto__fields[0],
  UPB_SIZE(16, 32), 2, kUpb_ExtMode_NonExtendable, 2, 255, 0,
};

static const upb_MiniTable_Sub google_protobuf_EnumDescriptorProto_submsgs[3] = {
  {.submsg = &google_protobuf_EnumValueDescriptorProto_msginit},
  {.submsg = &google_protobuf_EnumOptions_msginit},
  {.submsg = &google_protobuf_EnumDescriptorProto_EnumReservedRange_msginit},
};

static const upb_MiniTable_Field google_protobuf_EnumDescriptorProto__fields[5] = {
  {1, UPB_SIZE(4, 8), UPB_SIZE(1, 1), kUpb_NoSub, 12, kUpb_FieldMode_Scalar | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {2, UPB_SIZE(12, 24), UPB_SIZE(0, 0), 0, 11, kUpb_FieldMode_Array | (kUpb_FieldRep_Pointer << kUpb_FieldRep_Shift)},
  {3, UPB_SIZE(16, 32), UPB_SIZE(2, 2), 1, 11, kUpb_FieldMode_Scalar | (kUpb_FieldRep_Pointer << kUpb_FieldRep_Shift)},
  {4, UPB_SIZE(20, 40), UPB_SIZE(0, 0), 2, 11, kUpb_FieldMode_Array | (kUpb_FieldRep_Pointer << kUpb_FieldRep_Shift)},
  {5, UPB_SIZE(24, 48), UPB_SIZE(0, 0), kUpb_NoSub, 12, kUpb_FieldMode_Array | (kUpb_FieldRep_Pointer << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_EnumDescriptorProto_msginit = {
  &google_protobuf_EnumDescriptorProto_submsgs[0],
  &google_protobuf_EnumDescriptorProto__fields[0],
  UPB_SIZE(32, 56), 5, kUpb_ExtMode_NonExtendable, 5, 255, 0,
};

static const upb_MiniTable_Field google_protobuf_EnumDescriptorProto_EnumReservedRange__fields[2] = {
  {1, UPB_SIZE(4, 4), UPB_SIZE(1, 1), kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)},
  {2, UPB_SIZE(8, 8), UPB_SIZE(2, 2), kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_EnumDescriptorProto_EnumReservedRange_msginit = {
  NULL,
  &google_protobuf_EnumDescriptorProto_EnumReservedRange__fields[0],
  UPB_SIZE(16, 16), 2, kUpb_ExtMode_NonExtendable, 2, 255, 0,
};

static const upb_MiniTable_Sub google_protobuf_EnumValueDescriptorProto_submsgs[1] = {
  {.submsg = &google_protobuf_EnumValueOptions_msginit},
};

static const upb_MiniTable_Field google_protobuf_EnumValueDescriptorProto__fields[3] = {
  {1, UPB_SIZE(8, 8), UPB_SIZE(1, 1), kUpb_NoSub, 12, kUpb_FieldMode_Scalar | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {2, UPB_SIZE(4, 4), UPB_SIZE(2, 2), kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)},
  {3, UPB_SIZE(16, 24), UPB_SIZE(3, 3), 0, 11, kUpb_FieldMode_Scalar | (kUpb_FieldRep_Pointer << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_EnumValueDescriptorProto_msginit = {
  &google_protobuf_EnumValueDescriptorProto_submsgs[0],
  &google_protobuf_EnumValueDescriptorProto__fields[0],
  UPB_SIZE(24, 32), 3, kUpb_ExtMode_NonExtendable, 3, 255, 0,
};

static const upb_MiniTable_Sub google_protobuf_ServiceDescriptorProto_submsgs[2] = {
  {.submsg = &google_protobuf_MethodDescriptorProto_msginit},
  {.submsg = &google_protobuf_ServiceOptions_msginit},
};

static const upb_MiniTable_Field google_protobuf_ServiceDescriptorProto__fields[3] = {
  {1, UPB_SIZE(4, 8), UPB_SIZE(1, 1), kUpb_NoSub, 12, kUpb_FieldMode_Scalar | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {2, UPB_SIZE(12, 24), UPB_SIZE(0, 0), 0, 11, kUpb_FieldMode_Array | (kUpb_FieldRep_Pointer << kUpb_FieldRep_Shift)},
  {3, UPB_SIZE(16, 32), UPB_SIZE(2, 2), 1, 11, kUpb_FieldMode_Scalar | (kUpb_FieldRep_Pointer << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_ServiceDescriptorProto_msginit = {
  &google_protobuf_ServiceDescriptorProto_submsgs[0],
  &google_protobuf_ServiceDescriptorProto__fields[0],
  UPB_SIZE(24, 40), 3, kUpb_ExtMode_NonExtendable, 3, 255, 0,
};

static const upb_MiniTable_Sub google_protobuf_MethodDescriptorProto_submsgs[1] = {
  {.submsg = &google_protobuf_MethodOptions_msginit},
};

static const upb_MiniTable_Field google_protobuf_MethodDescriptorProto__fields[6] = {
  {1, UPB_SIZE(4, 8), UPB_SIZE(1, 1), kUpb_NoSub, 12, kUpb_FieldMode_Scalar | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {2, UPB_SIZE(12, 24), UPB_SIZE(2, 2), kUpb_NoSub, 12, kUpb_FieldMode_Scalar | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {3, UPB_SIZE(20, 40), UPB_SIZE(3, 3), kUpb_NoSub, 12, kUpb_FieldMode_Scalar | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {4, UPB_SIZE(28, 56), UPB_SIZE(4, 4), 0, 11, kUpb_FieldMode_Scalar | (kUpb_FieldRep_Pointer << kUpb_FieldRep_Shift)},
  {5, UPB_SIZE(1, 1), UPB_SIZE(5, 5), kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
  {6, UPB_SIZE(2, 2), UPB_SIZE(6, 6), kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_MethodDescriptorProto_msginit = {
  &google_protobuf_MethodDescriptorProto_submsgs[0],
  &google_protobuf_MethodDescriptorProto__fields[0],
  UPB_SIZE(32, 64), 6, kUpb_ExtMode_NonExtendable, 6, 255, 0,
};

static const upb_MiniTable_Sub google_protobuf_FileOptions_submsgs[2] = {
  {.subenum = &google_protobuf_FileOptions_OptimizeMode_enuminit},
  {.submsg = &google_protobuf_UninterpretedOption_msginit},
};

static const upb_MiniTable_Field google_protobuf_FileOptions__fields[21] = {
  {1, UPB_SIZE(20, 24), UPB_SIZE(1, 1), kUpb_NoSub, 12, kUpb_FieldMode_Scalar | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {8, UPB_SIZE(28, 40), UPB_SIZE(2, 2), kUpb_NoSub, 12, kUpb_FieldMode_Scalar | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {9, UPB_SIZE(4, 4), UPB_SIZE(3, 3), 0, 14, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)},
  {10, UPB_SIZE(8, 8), UPB_SIZE(4, 4), kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
  {11, UPB_SIZE(36, 56), UPB_SIZE(5, 5), kUpb_NoSub, 12, kUpb_FieldMode_Scalar | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {16, UPB_SIZE(9, 9), UPB_SIZE(6, 6), kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
  {17, UPB_SIZE(10, 10), UPB_SIZE(7, 7), kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
  {18, UPB_SIZE(11, 11), UPB_SIZE(8, 8), kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
  {20, UPB_SIZE(12, 12), UPB_SIZE(9, 9), kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
  {23, UPB_SIZE(13, 13), UPB_SIZE(10, 10), kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
  {27, UPB_SIZE(14, 14), UPB_SIZE(11, 11), kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
  {31, UPB_SIZE(15, 15), UPB_SIZE(12, 12), kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
  {36, UPB_SIZE(44, 72), UPB_SIZE(13, 13), kUpb_NoSub, 12, kUpb_FieldMode_Scalar | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {37, UPB_SIZE(52, 88), UPB_SIZE(14, 14), kUpb_NoSub, 12, kUpb_FieldMode_Scalar | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {39, UPB_SIZE(60, 104), UPB_SIZE(15, 15), kUpb_NoSub, 12, kUpb_FieldMode_Scalar | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {40, UPB_SIZE(68, 120), UPB_SIZE(16, 16), kUpb_NoSub, 12, kUpb_FieldMode_Scalar | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {41, UPB_SIZE(76, 136), UPB_SIZE(17, 17), kUpb_NoSub, 12, kUpb_FieldMode_Scalar | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {42, UPB_SIZE(16, 16), UPB_SIZE(18, 18), kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
  {44, UPB_SIZE(84, 152), UPB_SIZE(19, 19), kUpb_NoSub, 12, kUpb_FieldMode_Scalar | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {45, UPB_SIZE(92, 168), UPB_SIZE(20, 20), kUpb_NoSub, 12, kUpb_FieldMode_Scalar | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {999, UPB_SIZE(100, 184), UPB_SIZE(0, 0), 1, 11, kUpb_FieldMode_Array | (kUpb_FieldRep_Pointer << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_FileOptions_msginit = {
  &google_protobuf_FileOptions_submsgs[0],
  &google_protobuf_FileOptions__fields[0],
  UPB_SIZE(104, 192), 21, kUpb_ExtMode_Extendable, 1, 255, 0,
};

static const upb_MiniTable_Sub google_protobuf_MessageOptions_submsgs[1] = {
  {.submsg = &google_protobuf_UninterpretedOption_msginit},
};

static const upb_MiniTable_Field google_protobuf_MessageOptions__fields[5] = {
  {1, UPB_SIZE(1, 1), UPB_SIZE(1, 1), kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
  {2, UPB_SIZE(2, 2), UPB_SIZE(2, 2), kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
  {3, UPB_SIZE(3, 3), UPB_SIZE(3, 3), kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
  {7, UPB_SIZE(4, 4), UPB_SIZE(4, 4), kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
  {999, UPB_SIZE(8, 8), UPB_SIZE(0, 0), 0, 11, kUpb_FieldMode_Array | (kUpb_FieldRep_Pointer << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_MessageOptions_msginit = {
  &google_protobuf_MessageOptions_submsgs[0],
  &google_protobuf_MessageOptions__fields[0],
  UPB_SIZE(16, 16), 5, kUpb_ExtMode_Extendable, 3, 255, 0,
};

static const upb_MiniTable_Sub google_protobuf_FieldOptions_submsgs[3] = {
  {.subenum = &google_protobuf_FieldOptions_CType_enuminit},
  {.subenum = &google_protobuf_FieldOptions_JSType_enuminit},
  {.submsg = &google_protobuf_UninterpretedOption_msginit},
};

static const upb_MiniTable_Field google_protobuf_FieldOptions__fields[8] = {
  {1, UPB_SIZE(4, 4), UPB_SIZE(1, 1), 0, 14, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)},
  {2, UPB_SIZE(8, 8), UPB_SIZE(2, 2), kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
  {3, UPB_SIZE(9, 9), UPB_SIZE(3, 3), kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
  {5, UPB_SIZE(10, 10), UPB_SIZE(4, 4), kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
  {6, UPB_SIZE(12, 12), UPB_SIZE(5, 5), 1, 14, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)},
  {10, UPB_SIZE(16, 16), UPB_SIZE(6, 6), kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
  {15, UPB_SIZE(17, 17), UPB_SIZE(7, 7), kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
  {999, UPB_SIZE(20, 24), UPB_SIZE(0, 0), 2, 11, kUpb_FieldMode_Array | (kUpb_FieldRep_Pointer << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_FieldOptions_msginit = {
  &google_protobuf_FieldOptions_submsgs[0],
  &google_protobuf_FieldOptions__fields[0],
  UPB_SIZE(24, 32), 8, kUpb_ExtMode_Extendable, 3, 255, 0,
};

static const upb_MiniTable_Sub google_protobuf_OneofOptions_submsgs[1] = {
  {.submsg = &google_protobuf_UninterpretedOption_msginit},
};

static const upb_MiniTable_Field google_protobuf_OneofOptions__fields[1] = {
  {999, UPB_SIZE(0, 0), UPB_SIZE(0, 0), 0, 11, kUpb_FieldMode_Array | (kUpb_FieldRep_Pointer << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_OneofOptions_msginit = {
  &google_protobuf_OneofOptions_submsgs[0],
  &google_protobuf_OneofOptions__fields[0],
  UPB_SIZE(8, 8), 1, kUpb_ExtMode_Extendable, 0, 255, 0,
};

static const upb_MiniTable_Sub google_protobuf_EnumOptions_submsgs[1] = {
  {.submsg = &google_protobuf_UninterpretedOption_msginit},
};

static const upb_MiniTable_Field google_protobuf_EnumOptions__fields[3] = {
  {2, UPB_SIZE(1, 1), UPB_SIZE(1, 1), kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
  {3, UPB_SIZE(2, 2), UPB_SIZE(2, 2), kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
  {999, UPB_SIZE(4, 8), UPB_SIZE(0, 0), 0, 11, kUpb_FieldMode_Array | (kUpb_FieldRep_Pointer << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_EnumOptions_msginit = {
  &google_protobuf_EnumOptions_submsgs[0],
  &google_protobuf_EnumOptions__fields[0],
  UPB_SIZE(8, 16), 3, kUpb_ExtMode_Extendable, 0, 255, 0,
};

static const upb_MiniTable_Sub google_protobuf_EnumValueOptions_submsgs[1] = {
  {.submsg = &google_protobuf_UninterpretedOption_msginit},
};

static const upb_MiniTable_Field google_protobuf_EnumValueOptions__fields[2] = {
  {1, UPB_SIZE(1, 1), UPB_SIZE(1, 1), kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
  {999, UPB_SIZE(4, 8), UPB_SIZE(0, 0), 0, 11, kUpb_FieldMode_Array | (kUpb_FieldRep_Pointer << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_EnumValueOptions_msginit = {
  &google_protobuf_EnumValueOptions_submsgs[0],
  &google_protobuf_EnumValueOptions__fields[0],
  UPB_SIZE(8, 16), 2, kUpb_ExtMode_Extendable, 1, 255, 0,
};

static const upb_MiniTable_Sub google_protobuf_ServiceOptions_submsgs[1] = {
  {.submsg = &google_protobuf_UninterpretedOption_msginit},
};

static const upb_MiniTable_Field google_protobuf_ServiceOptions__fields[2] = {
  {33, UPB_SIZE(1, 1), UPB_SIZE(1, 1), kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
  {999, UPB_SIZE(4, 8), UPB_SIZE(0, 0), 0, 11, kUpb_FieldMode_Array | (kUpb_FieldRep_Pointer << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_ServiceOptions_msginit = {
  &google_protobuf_ServiceOptions_submsgs[0],
  &google_protobuf_ServiceOptions__fields[0],
  UPB_SIZE(8, 16), 2, kUpb_ExtMode_Extendable, 0, 255, 0,
};

static const upb_MiniTable_Sub google_protobuf_MethodOptions_submsgs[2] = {
  {.subenum = &google_protobuf_MethodOptions_IdempotencyLevel_enuminit},
  {.submsg = &google_protobuf_UninterpretedOption_msginit},
};

static const upb_MiniTable_Field google_protobuf_MethodOptions__fields[3] = {
  {33, UPB_SIZE(1, 1), UPB_SIZE(1, 1), kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
  {34, UPB_SIZE(4, 4), UPB_SIZE(2, 2), 0, 14, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)},
  {999, UPB_SIZE(8, 8), UPB_SIZE(0, 0), 1, 11, kUpb_FieldMode_Array | (kUpb_FieldRep_Pointer << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_MethodOptions_msginit = {
  &google_protobuf_MethodOptions_submsgs[0],
  &google_protobuf_MethodOptions__fields[0],
  UPB_SIZE(16, 16), 3, kUpb_ExtMode_Extendable, 0, 255, 0,
};

static const upb_MiniTable_Sub google_protobuf_UninterpretedOption_submsgs[1] = {
  {.submsg = &google_protobuf_UninterpretedOption_NamePart_msginit},
};

static const upb_MiniTable_Field google_protobuf_UninterpretedOption__fields[7] = {
  {2, UPB_SIZE(4, 8), UPB_SIZE(0, 0), 0, 11, kUpb_FieldMode_Array | (kUpb_FieldRep_Pointer << kUpb_FieldRep_Shift)},
  {3, UPB_SIZE(8, 16), UPB_SIZE(1, 1), kUpb_NoSub, 12, kUpb_FieldMode_Scalar | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {4, UPB_SIZE(32, 64), UPB_SIZE(2, 2), kUpb_NoSub, 4, kUpb_FieldMode_Scalar | (kUpb_FieldRep_8Byte << kUpb_FieldRep_Shift)},
  {5, UPB_SIZE(40, 72), UPB_SIZE(3, 3), kUpb_NoSub, 3, kUpb_FieldMode_Scalar | (kUpb_FieldRep_8Byte << kUpb_FieldRep_Shift)},
  {6, UPB_SIZE(48, 80), UPB_SIZE(4, 4), kUpb_NoSub, 1, kUpb_FieldMode_Scalar | (kUpb_FieldRep_8Byte << kUpb_FieldRep_Shift)},
  {7, UPB_SIZE(16, 32), UPB_SIZE(5, 5), kUpb_NoSub, 12, kUpb_FieldMode_Scalar | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {8, UPB_SIZE(24, 48), UPB_SIZE(6, 6), kUpb_NoSub, 12, kUpb_FieldMode_Scalar | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_UninterpretedOption_msginit = {
  &google_protobuf_UninterpretedOption_submsgs[0],
  &google_protobuf_UninterpretedOption__fields[0],
  UPB_SIZE(56, 88), 7, kUpb_ExtMode_NonExtendable, 0, 255, 0,
};

static const upb_MiniTable_Field google_protobuf_UninterpretedOption_NamePart__fields[2] = {
  {1, UPB_SIZE(4, 8), UPB_SIZE(1, 1), kUpb_NoSub, 12, kUpb_FieldMode_Scalar | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {2, UPB_SIZE(1, 1), UPB_SIZE(2, 2), kUpb_NoSub, 8, kUpb_FieldMode_Scalar | (kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_UninterpretedOption_NamePart_msginit = {
  NULL,
  &google_protobuf_UninterpretedOption_NamePart__fields[0],
  UPB_SIZE(16, 24), 2, kUpb_ExtMode_NonExtendable, 2, 255, 2,
};

static const upb_MiniTable_Sub google_protobuf_SourceCodeInfo_submsgs[1] = {
  {.submsg = &google_protobuf_SourceCodeInfo_Location_msginit},
};

static const upb_MiniTable_Field google_protobuf_SourceCodeInfo__fields[1] = {
  {1, UPB_SIZE(0, 0), UPB_SIZE(0, 0), 0, 11, kUpb_FieldMode_Array | (kUpb_FieldRep_Pointer << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_SourceCodeInfo_msginit = {
  &google_protobuf_SourceCodeInfo_submsgs[0],
  &google_protobuf_SourceCodeInfo__fields[0],
  UPB_SIZE(8, 8), 1, kUpb_ExtMode_NonExtendable, 1, 255, 0,
};

static const upb_MiniTable_Field google_protobuf_SourceCodeInfo_Location__fields[5] = {
  {1, UPB_SIZE(4, 8), UPB_SIZE(0, 0), kUpb_NoSub, 5, kUpb_FieldMode_Array | kUpb_LabelFlags_IsPacked | (kUpb_FieldRep_Pointer << kUpb_FieldRep_Shift)},
  {2, UPB_SIZE(8, 16), UPB_SIZE(0, 0), kUpb_NoSub, 5, kUpb_FieldMode_Array | kUpb_LabelFlags_IsPacked | (kUpb_FieldRep_Pointer << kUpb_FieldRep_Shift)},
  {3, UPB_SIZE(12, 24), UPB_SIZE(1, 1), kUpb_NoSub, 12, kUpb_FieldMode_Scalar | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {4, UPB_SIZE(20, 40), UPB_SIZE(2, 2), kUpb_NoSub, 12, kUpb_FieldMode_Scalar | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {6, UPB_SIZE(28, 56), UPB_SIZE(0, 0), kUpb_NoSub, 12, kUpb_FieldMode_Array | (kUpb_FieldRep_Pointer << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_SourceCodeInfo_Location_msginit = {
  NULL,
  &google_protobuf_SourceCodeInfo_Location__fields[0],
  UPB_SIZE(32, 64), 5, kUpb_ExtMode_NonExtendable, 4, 255, 0,
};

static const upb_MiniTable_Sub google_protobuf_GeneratedCodeInfo_submsgs[1] = {
  {.submsg = &google_protobuf_GeneratedCodeInfo_Annotation_msginit},
};

static const upb_MiniTable_Field google_protobuf_GeneratedCodeInfo__fields[1] = {
  {1, UPB_SIZE(0, 0), UPB_SIZE(0, 0), 0, 11, kUpb_FieldMode_Array | (kUpb_FieldRep_Pointer << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_GeneratedCodeInfo_msginit = {
  &google_protobuf_GeneratedCodeInfo_submsgs[0],
  &google_protobuf_GeneratedCodeInfo__fields[0],
  UPB_SIZE(8, 8), 1, kUpb_ExtMode_NonExtendable, 1, 255, 0,
};

static const upb_MiniTable_Field google_protobuf_GeneratedCodeInfo_Annotation__fields[4] = {
  {1, UPB_SIZE(12, 16), UPB_SIZE(0, 0), kUpb_NoSub, 5, kUpb_FieldMode_Array | kUpb_LabelFlags_IsPacked | (kUpb_FieldRep_Pointer << kUpb_FieldRep_Shift)},
  {2, UPB_SIZE(16, 24), UPB_SIZE(1, 1), kUpb_NoSub, 12, kUpb_FieldMode_Scalar | (kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {3, UPB_SIZE(4, 4), UPB_SIZE(2, 2), kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)},
  {4, UPB_SIZE(8, 8), UPB_SIZE(3, 3), kUpb_NoSub, 5, kUpb_FieldMode_Scalar | (kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_GeneratedCodeInfo_Annotation_msginit = {
  NULL,
  &google_protobuf_GeneratedCodeInfo_Annotation__fields[0],
  UPB_SIZE(24, 40), 4, kUpb_ExtMode_NonExtendable, 4, 255, 0,
};

static const upb_MiniTable *messages_layout[27] = {
  &google_protobuf_FileDescriptorSet_msginit,
  &google_protobuf_FileDescriptorProto_msginit,
  &google_protobuf_DescriptorProto_msginit,
  &google_protobuf_DescriptorProto_ExtensionRange_msginit,
  &google_protobuf_DescriptorProto_ReservedRange_msginit,
  &google_protobuf_ExtensionRangeOptions_msginit,
  &google_protobuf_FieldDescriptorProto_msginit,
  &google_protobuf_OneofDescriptorProto_msginit,
  &google_protobuf_EnumDescriptorProto_msginit,
  &google_protobuf_EnumDescriptorProto_EnumReservedRange_msginit,
  &google_protobuf_EnumValueDescriptorProto_msginit,
  &google_protobuf_ServiceDescriptorProto_msginit,
  &google_protobuf_MethodDescriptorProto_msginit,
  &google_protobuf_FileOptions_msginit,
  &google_protobuf_MessageOptions_msginit,
  &google_protobuf_FieldOptions_msginit,
  &google_protobuf_OneofOptions_msginit,
  &google_protobuf_EnumOptions_msginit,
  &google_protobuf_EnumValueOptions_msginit,
  &google_protobuf_ServiceOptions_msginit,
  &google_protobuf_MethodOptions_msginit,
  &google_protobuf_UninterpretedOption_msginit,
  &google_protobuf_UninterpretedOption_NamePart_msginit,
  &google_protobuf_SourceCodeInfo_msginit,
  &google_protobuf_SourceCodeInfo_Location_msginit,
  &google_protobuf_GeneratedCodeInfo_msginit,
  &google_protobuf_GeneratedCodeInfo_Annotation_msginit,
};

const upb_MiniTable_Enum google_protobuf_FieldDescriptorProto_Type_enuminit = {
    NULL,
    0x7fffeULL,
    0,
};

const upb_MiniTable_Enum google_protobuf_FieldDescriptorProto_Label_enuminit = {
    NULL,
    0xeULL,
    0,
};

const upb_MiniTable_Enum google_protobuf_FileOptions_OptimizeMode_enuminit = {
    NULL,
    0xeULL,
    0,
};

const upb_MiniTable_Enum google_protobuf_FieldOptions_CType_enuminit = {
    NULL,
    0x7ULL,
    0,
};

const upb_MiniTable_Enum google_protobuf_FieldOptions_JSType_enuminit = {
    NULL,
    0x7ULL,
    0,
};

const upb_MiniTable_Enum google_protobuf_MethodOptions_IdempotencyLevel_enuminit = {
    NULL,
    0x7ULL,
    0,
};

static const upb_MiniTable_Enum *enums_layout[6] = {
  &google_protobuf_FieldDescriptorProto_Type_enuminit,
  &google_protobuf_FieldDescriptorProto_Label_enuminit,
  &google_protobuf_FileOptions_OptimizeMode_enuminit,
  &google_protobuf_FieldOptions_CType_enuminit,
  &google_protobuf_FieldOptions_JSType_enuminit,
  &google_protobuf_MethodOptions_IdempotencyLevel_enuminit,
};

const upb_MiniTable_File google_protobuf_descriptor_proto_upb_file_layout = {
  messages_layout,
  enums_layout,
  NULL,
  27,
  6,
  0,
};



/** bazel-out/k8-fastbuild/bin/external/com_google_protobuf/google/protobuf/descriptor.upbdefs.c ************************************************************//* This file was generated by upbc (the upb compiler) from the input
 * file:
 *
 *     google/protobuf/descriptor.proto
 *
 * Do not edit -- your changes will be discarded when the file is
 * regenerated. */


static const char descriptor[7667] = {'\n', ' ', 'g', 'o', 'o', 'g', 'l', 'e', '/', 'p', 'r', 'o', 't', 'o', 'b', 'u', 'f', '/', 'd', 'e', 's', 'c', 'r', 'i', 'p', 
't', 'o', 'r', '.', 'p', 'r', 'o', 't', 'o', '\022', '\017', 'g', 'o', 'o', 'g', 'l', 'e', '.', 'p', 'r', 'o', 't', 'o', 'b', 'u', 
'f', '\"', 'M', '\n', '\021', 'F', 'i', 'l', 'e', 'D', 'e', 's', 'c', 'r', 'i', 'p', 't', 'o', 'r', 'S', 'e', 't', '\022', '8', '\n', 
'\004', 'f', 'i', 'l', 'e', '\030', '\001', ' ', '\003', '(', '\013', '2', '$', '.', 'g', 'o', 'o', 'g', 'l', 'e', '.', 'p', 'r', 'o', 't', 
'o', 'b', 'u', 'f', '.', 'F', 'i', 'l', 'e', 'D', 'e', 's', 'c', 'r', 'i', 'p', 't', 'o', 'r', 'P', 'r', 'o', 't', 'o', 'R', 
'\004', 'f', 'i', 'l', 'e', '\"', '\344', '\004', '\n', '\023', 'F', 'i', 'l', 'e', 'D', 'e', 's', 'c', 'r', 'i', 'p', 't', 'o', 'r', 'P', 
'r', 'o', 't', 'o', '\022', '\022', '\n', '\004', 'n', 'a', 'm', 'e', '\030', '\001', ' ', '\001', '(', '\t', 'R', '\004', 'n', 'a', 'm', 'e', '\022', 
'\030', '\n', '\007', 'p', 'a', 'c', 'k', 'a', 'g', 'e', '\030', '\002', ' ', '\001', '(', '\t', 'R', '\007', 'p', 'a', 'c', 'k', 'a', 'g', 'e', 
'\022', '\036', '\n', '\n', 'd', 'e', 'p', 'e', 'n', 'd', 'e', 'n', 'c', 'y', '\030', '\003', ' ', '\003', '(', '\t', 'R', '\n', 'd', 'e', 'p', 
'e', 'n', 'd', 'e', 'n', 'c', 'y', '\022', '+', '\n', '\021', 'p', 'u', 'b', 'l', 'i', 'c', '_', 'd', 'e', 'p', 'e', 'n', 'd', 'e', 
'n', 'c', 'y', '\030', '\n', ' ', '\003', '(', '\005', 'R', '\020', 'p', 'u', 'b', 'l', 'i', 'c', 'D', 'e', 'p', 'e', 'n', 'd', 'e', 'n', 
'c', 'y', '\022', '\'', '\n', '\017', 'w', 'e', 'a', 'k', '_', 'd', 'e', 'p', 'e', 'n', 'd', 'e', 'n', 'c', 'y', '\030', '\013', ' ', '\003', 
'(', '\005', 'R', '\016', 'w', 'e', 'a', 'k', 'D', 'e', 'p', 'e', 'n', 'd', 'e', 'n', 'c', 'y', '\022', 'C', '\n', '\014', 'm', 'e', 's', 
's', 'a', 'g', 'e', '_', 't', 'y', 'p', 'e', '\030', '\004', ' ', '\003', '(', '\013', '2', ' ', '.', 'g', 'o', 'o', 'g', 'l', 'e', '.', 
'p', 'r', 'o', 't', 'o', 'b', 'u', 'f', '.', 'D', 'e', 's', 'c', 'r', 'i', 'p', 't', 'o', 'r', 'P', 'r', 'o', 't', 'o', 'R', 
'\013', 'm', 'e', 's', 's', 'a', 'g', 'e', 'T', 'y', 'p', 'e', '\022', 'A', '\n', '\t', 'e', 'n', 'u', 'm', '_', 't', 'y', 'p', 'e', 
'\030', '\005', ' ', '\003', '(', '\013', '2', '$', '.', 'g', 'o', 'o', 'g', 'l', 'e', '.', 'p', 'r', 'o', 't', 'o', 'b', 'u', 'f', '.', 
'E', 'n', 'u', 'm', 'D', 'e', 's', 'c', 'r', 'i', 'p', 't', 'o', 'r', 'P', 'r', 'o', 't', 'o', 'R', '\010', 'e', 'n', 'u', 'm', 
'T', 'y', 'p', 'e', '\022', 'A', '\n', '\007', 's', 'e', 'r', 'v', 'i', 'c', 'e', '\030', '\006', ' ', '\003', '(', '\013', '2', '\'', '.', 'g', 
'o', 'o', 'g', 'l', 'e', '.', 'p', 'r', 'o', 't', 'o', 'b', 'u', 'f', '.', 'S', 'e', 'r', 'v', 'i', 'c', 'e', 'D', 'e', 's', 
'c', 'r', 'i', 'p', 't', 'o', 'r', 'P', 'r', 'o', 't', 'o', 'R', '\007', 's', 'e', 'r', 'v', 'i', 'c', 'e', '\022', 'C', '\n', '\t', 
'e', 'x', 't', 'e', 'n', 's', 'i', 'o', 'n', '\030', '\007', ' ', '\003', '(', '\013', '2', '%', '.', 'g', 'o', 'o', 'g', 'l', 'e', '.', 
'p', 'r', 'o', 't', 'o', 'b', 'u', 'f', '.', 'F', 'i', 'e', 'l', 'd', 'D', 'e', 's', 'c', 'r', 'i', 'p', 't', 'o', 'r', 'P', 
'r', 'o', 't', 'o', 'R', '\t', 'e', 'x', 't', 'e', 'n', 's', 'i', 'o', 'n', '\022', '6', '\n', '\007', 'o', 'p', 't', 'i', 'o', 'n', 
's', '\030', '\010', ' ', '\001', '(', '\013', '2', '\034', '.', 'g', 'o', 'o', 'g', 'l', 'e', '.', 'p', 'r', 'o', 't', 'o', 'b', 'u', 'f', 
'.', 'F', 'i', 'l', 'e', 'O', 'p', 't', 'i', 'o', 'n', 's', 'R', '\007', 'o', 'p', 't', 'i', 'o', 'n', 's', '\022', 'I', '\n', '\020', 
's', 'o', 'u', 'r', 'c', 'e', '_', 'c', 'o', 'd', 'e', '_', 'i', 'n', 'f', 'o', '\030', '\t', ' ', '\001', '(', '\013', '2', '\037', '.', 
'g', 'o', 'o', 'g', 'l', 'e', '.', 'p', 'r', 'o', 't', 'o', 'b', 'u', 'f', '.', 'S', 'o', 'u', 'r', 'c', 'e', 'C', 'o', 'd', 
'e', 'I', 'n', 'f', 'o', 'R', '\016', 's', 'o', 'u', 'r', 'c', 'e', 'C', 'o', 'd', 'e', 'I', 'n', 'f', 'o', '\022', '\026', '\n', '\006', 
's', 'y', 'n', 't', 'a', 'x', '\030', '\014', ' ', '\001', '(', '\t', 'R', '\006', 's', 'y', 'n', 't', 'a', 'x', '\"', '\271', '\006', '\n', '\017', 
'D', 'e', 's', 'c', 'r', 'i', 'p', 't', 'o', 'r', 'P', 'r', 'o', 't', 'o', '\022', '\022', '\n', '\004', 'n', 'a', 'm', 'e', '\030', '\001', 
' ', '\001', '(', '\t', 'R', '\004', 'n', 'a', 'm', 'e', '\022', ';', '\n', '\005', 'f', 'i', 'e', 'l', 'd', '\030', '\002', ' ', '\003', '(', '\013', 
'2', '%', '.', 'g', 'o', 'o', 'g', 'l', 'e', '.', 'p', 'r', 'o', 't', 'o', 'b', 'u', 'f', '.', 'F', 'i', 'e', 'l', 'd', 'D', 
'e', 's', 'c', 'r', 'i', 'p', 't', 'o', 'r', 'P', 'r', 'o', 't', 'o', 'R', '\005', 'f', 'i', 'e', 'l', 'd', '\022', 'C', '\n', '\t', 
'e', 'x', 't', 'e', 'n', 's', 'i', 'o', 'n', '\030', '\006', ' ', '\003', '(', '\013', '2', '%', '.', 'g', 'o', 'o', 'g', 'l', 'e', '.', 
'p', 'r', 'o', 't', 'o', 'b', 'u', 'f', '.', 'F', 'i', 'e', 'l', 'd', 'D', 'e', 's', 'c', 'r', 'i', 'p', 't', 'o', 'r', 'P', 
'r', 'o', 't', 'o', 'R', '\t', 'e', 'x', 't', 'e', 'n', 's', 'i', 'o', 'n', '\022', 'A', '\n', '\013', 'n', 'e', 's', 't', 'e', 'd', 
'_', 't', 'y', 'p', 'e', '\030', '\003', ' ', '\003', '(', '\013', '2', ' ', '.', 'g', 'o', 'o', 'g', 'l', 'e', '.', 'p', 'r', 'o', 't', 
'o', 'b', 'u', 'f', '.', 'D', 'e', 's', 'c', 'r', 'i', 'p', 't', 'o', 'r', 'P', 'r', 'o', 't', 'o', 'R', '\n', 'n', 'e', 's', 
't', 'e', 'd', 'T', 'y', 'p', 'e', '\022', 'A', '\n', '\t', 'e', 'n', 'u', 'm', '_', 't', 'y', 'p', 'e', '\030', '\004', ' ', '\003', '(', 
'\013', '2', '$', '.', 'g', 'o', 'o', 'g', 'l', 'e', '.', 'p', 'r', 'o', 't', 'o', 'b', 'u', 'f', '.', 'E', 'n', 'u', 'm', 'D', 
'e', 's', 'c', 'r', 'i', 'p', 't', 'o', 'r', 'P', 'r', 'o', 't', 'o', 'R', '\010', 'e', 'n', 'u', 'm', 'T', 'y', 'p', 'e', '\022', 
'X', '\n', '\017', 'e', 'x', 't', 'e', 'n', 's', 'i', 'o', 'n', '_', 'r', 'a', 'n', 'g', 'e', '\030', '\005', ' ', '\003', '(', '\013', '2', 
'/', '.', 'g', 'o', 'o', 'g', 'l', 'e', '.', 'p', 'r', 'o', 't', 'o', 'b', 'u', 'f', '.', 'D', 'e', 's', 'c', 'r', 'i', 'p', 
't', 'o', 'r', 'P', 'r', 'o', 't', 'o', '.', 'E', 'x', 't', 'e', 'n', 's', 'i', 'o', 'n', 'R', 'a', 'n', 'g', 'e', 'R', '\016', 
'e', 'x', 't', 'e', 'n', 's', 'i', 'o', 'n', 'R', 'a', 'n', 'g', 'e', '\022', 'D', '\n', '\n', 'o', 'n', 'e', 'o', 'f', '_', 'd', 
'e', 'c', 'l', '\030', '\010', ' ', '\003', '(', '\013', '2', '%', '.', 'g', 'o', 'o', 'g', 'l', 'e', '.', 'p', 'r', 'o', 't', 'o', 'b', 
'u', 'f', '.', 'O', 'n', 'e', 'o', 'f', 'D', 'e', 's', 'c', 'r', 'i', 'p', 't', 'o', 'r', 'P', 'r', 'o', 't', 'o', 'R', '\t', 
'o', 'n', 'e', 'o', 'f', 'D', 'e', 'c', 'l', '\022', '9', '\n', '\007', 'o', 'p', 't', 'i', 'o', 'n', 's', '\030', '\007', ' ', '\001', '(', 
'\013', '2', '\037', '.', 'g', 'o', 'o', 'g', 'l', 'e', '.', 'p', 'r', 'o', 't', 'o', 'b', 'u', 'f', '.', 'M', 'e', 's', 's', 'a', 
'g', 'e', 'O', 'p', 't', 'i', 'o', 'n', 's', 'R', '\007', 'o', 'p', 't', 'i', 'o', 'n', 's', '\022', 'U', '\n', '\016', 'r', 'e', 's', 
'e', 'r', 'v', 'e', 'd', '_', 'r', 'a', 'n', 'g', 'e', '\030', '\t', ' ', '\003', '(', '\013', '2', '.', '.', 'g', 'o', 'o', 'g', 'l', 
'e', '.', 'p', 'r', 'o', 't', 'o', 'b', 'u', 'f', '.', 'D', 'e', 's', 'c', 'r', 'i', 'p', 't', 'o', 'r', 'P', 'r', 'o', 't', 
'o', '.', 'R', 'e', 's', 'e', 'r', 'v', 'e', 'd', 'R', 'a', 'n', 'g', 'e', 'R', '\r', 'r', 'e', 's', 'e', 'r', 'v', 'e', 'd', 
'R', 'a', 'n', 'g', 'e', '\022', '#', '\n', '\r', 'r', 'e', 's', 'e', 'r', 'v', 'e', 'd', '_', 'n', 'a', 'm', 'e', '\030', '\n', ' ', 
'\003', '(', '\t', 'R', '\014', 'r', 'e', 's', 'e', 'r', 'v', 'e', 'd', 'N', 'a', 'm', 'e', '\032', 'z', '\n', '\016', 'E', 'x', 't', 'e', 
'n', 's', 'i', 'o', 'n', 'R', 'a', 'n', 'g', 'e', '\022', '\024', '\n', '\005', 's', 't', 'a', 'r', 't', '\030', '\001', ' ', '\001', '(', '\005', 
'R', '\005', 's', 't', 'a', 'r', 't', '\022', '\020', '\n', '\003', 'e', 'n', 'd', '\030', '\002', ' ', '\001', '(', '\005', 'R', '\003', 'e', 'n', 'd', 
'\022', '@', '\n', '\007', 'o', 'p', 't', 'i', 'o', 'n', 's', '\030', '\003', ' ', '\001', '(', '\013', '2', '&', '.', 'g', 'o', 'o', 'g', 'l', 
'e', '.', 'p', 'r', 'o', 't', 'o', 'b', 'u', 'f', '.', 'E', 'x', 't', 'e', 'n', 's', 'i', 'o', 'n', 'R', 'a', 'n', 'g', 'e', 
'O', 'p', 't', 'i', 'o', 'n', 's', 'R', '\007', 'o', 'p', 't', 'i', 'o', 'n', 's', '\032', '7', '\n', '\r', 'R', 'e', 's', 'e', 'r', 
'v', 'e', 'd', 'R', 'a', 'n', 'g', 'e', '\022', '\024', '\n', '\005', 's', 't', 'a', 'r', 't', '\030', '\001', ' ', '\001', '(', '\005', 'R', '\005', 
's', 't', 'a', 'r', 't', '\022', '\020', '\n', '\003', 'e', 'n', 'd', '\030', '\002', ' ', '\001', '(', '\005', 'R', '\003', 'e', 'n', 'd', '\"', '|', 
'\n', '\025', 'E', 'x', 't', 'e', 'n', 's', 'i', 'o', 'n', 'R', 'a', 'n', 'g', 'e', 'O', 'p', 't', 'i', 'o', 'n', 's', '\022', 'X', 
'\n', '\024', 'u', 'n', 'i', 'n', 't', 'e', 'r', 'p', 'r', 'e', 't', 'e', 'd', '_', 'o', 'p', 't', 'i', 'o', 'n', '\030', '\347', '\007', 
' ', '\003', '(', '\013', '2', '$', '.', 'g', 'o', 'o', 'g', 'l', 'e', '.', 'p', 'r', 'o', 't', 'o', 'b', 'u', 'f', '.', 'U', 'n', 
'i', 'n', 't', 'e', 'r', 'p', 'r', 'e', 't', 'e', 'd', 'O', 'p', 't', 'i', 'o', 'n', 'R', '\023', 'u', 'n', 'i', 'n', 't', 'e', 
'r', 'p', 'r', 'e', 't', 'e', 'd', 'O', 'p', 't', 'i', 'o', 'n', '*', '\t', '\010', '\350', '\007', '\020', '\200', '\200', '\200', '\200', '\002', '\"', 
'\301', '\006', '\n', '\024', 'F', 'i', 'e', 'l', 'd', 'D', 'e', 's', 'c', 'r', 'i', 'p', 't', 'o', 'r', 'P', 'r', 'o', 't', 'o', '\022', 
'\022', '\n', '\004', 'n', 'a', 'm', 'e', '\030', '\001', ' ', '\001', '(', '\t', 'R', '\004', 'n', 'a', 'm', 'e', '\022', '\026', '\n', '\006', 'n', 'u', 
'm', 'b', 'e', 'r', '\030', '\003', ' ', '\001', '(', '\005', 'R', '\006', 'n', 'u', 'm', 'b', 'e', 'r', '\022', 'A', '\n', '\005', 'l', 'a', 'b', 
'e', 'l', '\030', '\004', ' ', '\001', '(', '\016', '2', '+', '.', 'g', 'o', 'o', 'g', 'l', 'e', '.', 'p', 'r', 'o', 't', 'o', 'b', 'u', 
'f', '.', 'F', 'i', 'e', 'l', 'd', 'D', 'e', 's', 'c', 'r', 'i', 'p', 't', 'o', 'r', 'P', 'r', 'o', 't', 'o', '.', 'L', 'a', 
'b', 'e', 'l', 'R', '\005', 'l', 'a', 'b', 'e', 'l', '\022', '>', '\n', '\004', 't', 'y', 'p', 'e', '\030', '\005', ' ', '\001', '(', '\016', '2', 
'*', '.', 'g', 'o', 'o', 'g', 'l', 'e', '.', 'p', 'r', 'o', 't', 'o', 'b', 'u', 'f', '.', 'F', 'i', 'e', 'l', 'd', 'D', 'e', 
's', 'c', 'r', 'i', 'p', 't', 'o', 'r', 'P', 'r', 'o', 't', 'o', '.', 'T', 'y', 'p', 'e', 'R', '\004', 't', 'y', 'p', 'e', '\022', 
'\033', '\n', '\t', 't', 'y', 'p', 'e', '_', 'n', 'a', 'm', 'e', '\030', '\006', ' ', '\001', '(', '\t', 'R', '\010', 't', 'y', 'p', 'e', 'N', 
'a', 'm', 'e', '\022', '\032', '\n', '\010', 'e', 'x', 't', 'e', 'n', 'd', 'e', 'e', '\030', '\002', ' ', '\001', '(', '\t', 'R', '\010', 'e', 'x', 
't', 'e', 'n', 'd', 'e', 'e', '\022', '#', '\n', '\r', 'd', 'e', 'f', 'a', 'u', 'l', 't', '_', 'v', 'a', 'l', 'u', 'e', '\030', '\007', 
' ', '\001', '(', '\t', 'R', '\014', 'd', 'e', 'f', 'a', 'u', 'l', 't', 'V', 'a', 'l', 'u', 'e', '\022', '\037', '\n', '\013', 'o', 'n', 'e', 
'o', 'f', '_', 'i', 'n', 'd', 'e', 'x', '\030', '\t', ' ', '\001', '(', '\005', 'R', '\n', 'o', 'n', 'e', 'o', 'f', 'I', 'n', 'd', 'e', 
'x', '\022', '\033', '\n', '\t', 'j', 's', 'o', 'n', '_', 'n', 'a', 'm', 'e', '\030', '\n', ' ', '\001', '(', '\t', 'R', '\010', 'j', 's', 'o', 
'n', 'N', 'a', 'm', 'e', '\022', '7', '\n', '\007', 'o', 'p', 't', 'i', 'o', 'n', 's', '\030', '\010', ' ', '\001', '(', '\013', '2', '\035', '.', 
'g', 'o', 'o', 'g', 'l', 'e', '.', 'p', 'r', 'o', 't', 'o', 'b', 'u', 'f', '.', 'F', 'i', 'e', 'l', 'd', 'O', 'p', 't', 'i', 
'o', 'n', 's', 'R', '\007', 'o', 'p', 't', 'i', 'o', 'n', 's', '\022', '\'', '\n', '\017', 'p', 'r', 'o', 't', 'o', '3', '_', 'o', 'p', 
't', 'i', 'o', 'n', 'a', 'l', '\030', '\021', ' ', '\001', '(', '\010', 'R', '\016', 'p', 'r', 'o', 't', 'o', '3', 'O', 'p', 't', 'i', 'o', 
'n', 'a', 'l', '\"', '\266', '\002', '\n', '\004', 'T', 'y', 'p', 'e', '\022', '\017', '\n', '\013', 'T', 'Y', 'P', 'E', '_', 'D', 'O', 'U', 'B', 
'L', 'E', '\020', '\001', '\022', '\016', '\n', '\n', 'T', 'Y', 'P', 'E', '_', 'F', 'L', 'O', 'A', 'T', '\020', '\002', '\022', '\016', '\n', '\n', 'T', 
'Y', 'P', 'E', '_', 'I', 'N', 'T', '6', '4', '\020', '\003', '\022', '\017', '\n', '\013', 'T', 'Y', 'P', 'E', '_', 'U', 'I', 'N', 'T', '6', 
'4', '\020', '\004', '\022', '\016', '\n', '\n', 'T', 'Y', 'P', 'E', '_', 'I', 'N', 'T', '3', '2', '\020', '\005', '\022', '\020', '\n', '\014', 'T', 'Y', 
'P', 'E', '_', 'F', 'I', 'X', 'E', 'D', '6', '4', '\020', '\006', '\022', '\020', '\n', '\014', 'T', 'Y', 'P', 'E', '_', 'F', 'I', 'X', 'E', 
'D', '3', '2', '\020', '\007', '\022', '\r', '\n', '\t', 'T', 'Y', 'P', 'E', '_', 'B', 'O', 'O', 'L', '\020', '\010', '\022', '\017', '\n', '\013', 'T', 
'Y', 'P', 'E', '_', 'S', 'T', 'R', 'I', 'N', 'G', '\020', '\t', '\022', '\016', '\n', '\n', 'T', 'Y', 'P', 'E', '_', 'G', 'R', 'O', 'U', 
'P', '\020', '\n', '\022', '\020', '\n', '\014', 'T', 'Y', 'P', 'E', '_', 'M', 'E', 'S', 'S', 'A', 'G', 'E', '\020', '\013', '\022', '\016', '\n', '\n', 
'T', 'Y', 'P', 'E', '_', 'B', 'Y', 'T', 'E', 'S', '\020', '\014', '\022', '\017', '\n', '\013', 'T', 'Y', 'P', 'E', '_', 'U', 'I', 'N', 'T', 
'3', '2', '\020', '\r', '\022', '\r', '\n', '\t', 'T', 'Y', 'P', 'E', '_', 'E', 'N', 'U', 'M', '\020', '\016', '\022', '\021', '\n', '\r', 'T', 'Y', 
'P', 'E', '_', 'S', 'F', 'I', 'X', 'E', 'D', '3', '2', '\020', '\017', '\022', '\021', '\n', '\r', 'T', 'Y', 'P', 'E', '_', 'S', 'F', 'I', 
'X', 'E', 'D', '6', '4', '\020', '\020', '\022', '\017', '\n', '\013', 'T', 'Y', 'P', 'E', '_', 'S', 'I', 'N', 'T', '3', '2', '\020', '\021', '\022', 
'\017', '\n', '\013', 'T', 'Y', 'P', 'E', '_', 'S', 'I', 'N', 'T', '6', '4', '\020', '\022', '\"', 'C', '\n', '\005', 'L', 'a', 'b', 'e', 'l', 
'\022', '\022', '\n', '\016', 'L', 'A', 'B', 'E', 'L', '_', 'O', 'P', 'T', 'I', 'O', 'N', 'A', 'L', '\020', '\001', '\022', '\022', '\n', '\016', 'L', 
'A', 'B', 'E', 'L', '_', 'R', 'E', 'Q', 'U', 'I', 'R', 'E', 'D', '\020', '\002', '\022', '\022', '\n', '\016', 'L', 'A', 'B', 'E', 'L', '_', 
'R', 'E', 'P', 'E', 'A', 'T', 'E', 'D', '\020', '\003', '\"', 'c', '\n', '\024', 'O', 'n', 'e', 'o', 'f', 'D', 'e', 's', 'c', 'r', 'i', 
'p', 't', 'o', 'r', 'P', 'r', 'o', 't', 'o', '\022', '\022', '\n', '\004', 'n', 'a', 'm', 'e', '\030', '\001', ' ', '\001', '(', '\t', 'R', '\004', 
'n', 'a', 'm', 'e', '\022', '7', '\n', '\007', 'o', 'p', 't', 'i', 'o', 'n', 's', '\030', '\002', ' ', '\001', '(', '\013', '2', '\035', '.', 'g', 
'o', 'o', 'g', 'l', 'e', '.', 'p', 'r', 'o', 't', 'o', 'b', 'u', 'f', '.', 'O', 'n', 'e', 'o', 'f', 'O', 'p', 't', 'i', 'o', 
'n', 's', 'R', '\007', 'o', 'p', 't', 'i', 'o', 'n', 's', '\"', '\343', '\002', '\n', '\023', 'E', 'n', 'u', 'm', 'D', 'e', 's', 'c', 'r', 
'i', 'p', 't', 'o', 'r', 'P', 'r', 'o', 't', 'o', '\022', '\022', '\n', '\004', 'n', 'a', 'm', 'e', '\030', '\001', ' ', '\001', '(', '\t', 'R', 
'\004', 'n', 'a', 'm', 'e', '\022', '?', '\n', '\005', 'v', 'a', 'l', 'u', 'e', '\030', '\002', ' ', '\003', '(', '\013', '2', ')', '.', 'g', 'o', 
'o', 'g', 'l', 'e', '.', 'p', 'r', 'o', 't', 'o', 'b', 'u', 'f', '.', 'E', 'n', 'u', 'm', 'V', 'a', 'l', 'u', 'e', 'D', 'e', 
's', 'c', 'r', 'i', 'p', 't', 'o', 'r', 'P', 'r', 'o', 't', 'o', 'R', '\005', 'v', 'a', 'l', 'u', 'e', '\022', '6', '\n', '\007', 'o', 
'p', 't', 'i', 'o', 'n', 's', '\030', '\003', ' ', '\001', '(', '\013', '2', '\034', '.', 'g', 'o', 'o', 'g', 'l', 'e', '.', 'p', 'r', 'o', 
't', 'o', 'b', 'u', 'f', '.', 'E', 'n', 'u', 'm', 'O', 'p', 't', 'i', 'o', 'n', 's', 'R', '\007', 'o', 'p', 't', 'i', 'o', 'n', 
's', '\022', ']', '\n', '\016', 'r', 'e', 's', 'e', 'r', 'v', 'e', 'd', '_', 'r', 'a', 'n', 'g', 'e', '\030', '\004', ' ', '\003', '(', '\013', 
'2', '6', '.', 'g', 'o', 'o', 'g', 'l', 'e', '.', 'p', 'r', 'o', 't', 'o', 'b', 'u', 'f', '.', 'E', 'n', 'u', 'm', 'D', 'e', 
's', 'c', 'r', 'i', 'p', 't', 'o', 'r', 'P', 'r', 'o', 't', 'o', '.', 'E', 'n', 'u', 'm', 'R', 'e', 's', 'e', 'r', 'v', 'e', 
'd', 'R', 'a', 'n', 'g', 'e', 'R', '\r', 'r', 'e', 's', 'e', 'r', 'v', 'e', 'd', 'R', 'a', 'n', 'g', 'e', '\022', '#', '\n', '\r', 
'r', 'e', 's', 'e', 'r', 'v', 'e', 'd', '_', 'n', 'a', 'm', 'e', '\030', '\005', ' ', '\003', '(', '\t', 'R', '\014', 'r', 'e', 's', 'e', 
'r', 'v', 'e', 'd', 'N', 'a', 'm', 'e', '\032', ';', '\n', '\021', 'E', 'n', 'u', 'm', 'R', 'e', 's', 'e', 'r', 'v', 'e', 'd', 'R', 
'a', 'n', 'g', 'e', '\022', '\024', '\n', '\005', 's', 't', 'a', 'r', 't', '\030', '\001', ' ', '\001', '(', '\005', 'R', '\005', 's', 't', 'a', 'r', 
't', '\022', '\020', '\n', '\003', 'e', 'n', 'd', '\030', '\002', ' ', '\001', '(', '\005', 'R', '\003', 'e', 'n', 'd', '\"', '\203', '\001', '\n', '\030', 'E', 
'n', 'u', 'm', 'V', 'a', 'l', 'u', 'e', 'D', 'e', 's', 'c', 'r', 'i', 'p', 't', 'o', 'r', 'P', 'r', 'o', 't', 'o', '\022', '\022', 
'\n', '\004', 'n', 'a', 'm', 'e', '\030', '\001', ' ', '\001', '(', '\t', 'R', '\004', 'n', 'a', 'm', 'e', '\022', '\026', '\n', '\006', 'n', 'u', 'm', 
'b', 'e', 'r', '\030', '\002', ' ', '\001', '(', '\005', 'R', '\006', 'n', 'u', 'm', 'b', 'e', 'r', '\022', ';', '\n', '\007', 'o', 'p', 't', 'i', 
'o', 'n', 's', '\030', '\003', ' ', '\001', '(', '\013', '2', '!', '.', 'g', 'o', 'o', 'g', 'l', 'e', '.', 'p', 'r', 'o', 't', 'o', 'b', 
'u', 'f', '.', 'E', 'n', 'u', 'm', 'V', 'a', 'l', 'u', 'e', 'O', 'p', 't', 'i', 'o', 'n', 's', 'R', '\007', 'o', 'p', 't', 'i', 
'o', 'n', 's', '\"', '\247', '\001', '\n', '\026', 'S', 'e', 'r', 'v', 'i', 'c', 'e', 'D', 'e', 's', 'c', 'r', 'i', 'p', 't', 'o', 'r', 
'P', 'r', 'o', 't', 'o', '\022', '\022', '\n', '\004', 'n', 'a', 'm', 'e', '\030', '\001', ' ', '\001', '(', '\t', 'R', '\004', 'n', 'a', 'm', 'e', 
'\022', '>', '\n', '\006', 'm', 'e', 't', 'h', 'o', 'd', '\030', '\002', ' ', '\003', '(', '\013', '2', '&', '.', 'g', 'o', 'o', 'g', 'l', 'e', 
'.', 'p', 'r', 'o', 't', 'o', 'b', 'u', 'f', '.', 'M', 'e', 't', 'h', 'o', 'd', 'D', 'e', 's', 'c', 'r', 'i', 'p', 't', 'o', 
'r', 'P', 'r', 'o', 't', 'o', 'R', '\006', 'm', 'e', 't', 'h', 'o', 'd', '\022', '9', '\n', '\007', 'o', 'p', 't', 'i', 'o', 'n', 's', 
'\030', '\003', ' ', '\001', '(', '\013', '2', '\037', '.', 'g', 'o', 'o', 'g', 'l', 'e', '.', 'p', 'r', 'o', 't', 'o', 'b', 'u', 'f', '.', 
'S', 'e', 'r', 'v', 'i', 'c', 'e', 'O', 'p', 't', 'i', 'o', 'n', 's', 'R', '\007', 'o', 'p', 't', 'i', 'o', 'n', 's', '\"', '\211', 
'\002', '\n', '\025', 'M', 'e', 't', 'h', 'o', 'd', 'D', 'e', 's', 'c', 'r', 'i', 'p', 't', 'o', 'r', 'P', 'r', 'o', 't', 'o', '\022', 
'\022', '\n', '\004', 'n', 'a', 'm', 'e', '\030', '\001', ' ', '\001', '(', '\t', 'R', '\004', 'n', 'a', 'm', 'e', '\022', '\035', '\n', '\n', 'i', 'n', 
'p', 'u', 't', '_', 't', 'y', 'p', 'e', '\030', '\002', ' ', '\001', '(', '\t', 'R', '\t', 'i', 'n', 'p', 'u', 't', 'T', 'y', 'p', 'e', 
'\022', '\037', '\n', '\013', 'o', 'u', 't', 'p', 'u', 't', '_', 't', 'y', 'p', 'e', '\030', '\003', ' ', '\001', '(', '\t', 'R', '\n', 'o', 'u', 
't', 'p', 'u', 't', 'T', 'y', 'p', 'e', '\022', '8', '\n', '\007', 'o', 'p', 't', 'i', 'o', 'n', 's', '\030', '\004', ' ', '\001', '(', '\013', 
'2', '\036', '.', 'g', 'o', 'o', 'g', 'l', 'e', '.', 'p', 'r', 'o', 't', 'o', 'b', 'u', 'f', '.', 'M', 'e', 't', 'h', 'o', 'd', 
'O', 'p', 't', 'i', 'o', 'n', 's', 'R', '\007', 'o', 'p', 't', 'i', 'o', 'n', 's', '\022', '0', '\n', '\020', 'c', 'l', 'i', 'e', 'n', 
't', '_', 's', 't', 'r', 'e', 'a', 'm', 'i', 'n', 'g', '\030', '\005', ' ', '\001', '(', '\010', ':', '\005', 'f', 'a', 'l', 's', 'e', 'R', 
'\017', 'c', 'l', 'i', 'e', 'n', 't', 'S', 't', 'r', 'e', 'a', 'm', 'i', 'n', 'g', '\022', '0', '\n', '\020', 's', 'e', 'r', 'v', 'e', 
'r', '_', 's', 't', 'r', 'e', 'a', 'm', 'i', 'n', 'g', '\030', '\006', ' ', '\001', '(', '\010', ':', '\005', 'f', 'a', 'l', 's', 'e', 'R', 
'\017', 's', 'e', 'r', 'v', 'e', 'r', 'S', 't', 'r', 'e', 'a', 'm', 'i', 'n', 'g', '\"', '\221', '\t', '\n', '\013', 'F', 'i', 'l', 'e', 
'O', 'p', 't', 'i', 'o', 'n', 's', '\022', '!', '\n', '\014', 'j', 'a', 'v', 'a', '_', 'p', 'a', 'c', 'k', 'a', 'g', 'e', '\030', '\001', 
' ', '\001', '(', '\t', 'R', '\013', 'j', 'a', 'v', 'a', 'P', 'a', 'c', 'k', 'a', 'g', 'e', '\022', '0', '\n', '\024', 'j', 'a', 'v', 'a', 
'_', 'o', 'u', 't', 'e', 'r', '_', 'c', 'l', 'a', 's', 's', 'n', 'a', 'm', 'e', '\030', '\010', ' ', '\001', '(', '\t', 'R', '\022', 'j', 
'a', 'v', 'a', 'O', 'u', 't', 'e', 'r', 'C', 'l', 'a', 's', 's', 'n', 'a', 'm', 'e', '\022', '5', '\n', '\023', 'j', 'a', 'v', 'a', 
'_', 'm', 'u', 'l', 't', 'i', 'p', 'l', 'e', '_', 'f', 'i', 'l', 'e', 's', '\030', '\n', ' ', '\001', '(', '\010', ':', '\005', 'f', 'a', 
'l', 's', 'e', 'R', '\021', 'j', 'a', 'v', 'a', 'M', 'u', 'l', 't', 'i', 'p', 'l', 'e', 'F', 'i', 'l', 'e', 's', '\022', 'D', '\n', 
'\035', 'j', 'a', 'v', 'a', '_', 'g', 'e', 'n', 'e', 'r', 'a', 't', 'e', '_', 'e', 'q', 'u', 'a', 'l', 's', '_', 'a', 'n', 'd', 
'_', 'h', 'a', 's', 'h', '\030', '\024', ' ', '\001', '(', '\010', 'B', '\002', '\030', '\001', 'R', '\031', 'j', 'a', 'v', 'a', 'G', 'e', 'n', 'e', 
'r', 'a', 't', 'e', 'E', 'q', 'u', 'a', 'l', 's', 'A', 'n', 'd', 'H', 'a', 's', 'h', '\022', ':', '\n', '\026', 'j', 'a', 'v', 'a', 
'_', 's', 't', 'r', 'i', 'n', 'g', '_', 'c', 'h', 'e', 'c', 'k', '_', 'u', 't', 'f', '8', '\030', '\033', ' ', '\001', '(', '\010', ':', 
'\005', 'f', 'a', 'l', 's', 'e', 'R', '\023', 'j', 'a', 'v', 'a', 'S', 't', 'r', 'i', 'n', 'g', 'C', 'h', 'e', 'c', 'k', 'U', 't', 
'f', '8', '\022', 'S', '\n', '\014', 'o', 'p', 't', 'i', 'm', 'i', 'z', 'e', '_', 'f', 'o', 'r', '\030', '\t', ' ', '\001', '(', '\016', '2', 
')', '.', 'g', 'o', 'o', 'g', 'l', 'e', '.', 'p', 'r', 'o', 't', 'o', 'b', 'u', 'f', '.', 'F', 'i', 'l', 'e', 'O', 'p', 't', 
'i', 'o', 'n', 's', '.', 'O', 'p', 't', 'i', 'm', 'i', 'z', 'e', 'M', 'o', 'd', 'e', ':', '\005', 'S', 'P', 'E', 'E', 'D', 'R', 
'\013', 'o', 'p', 't', 'i', 'm', 'i', 'z', 'e', 'F', 'o', 'r', '\022', '\035', '\n', '\n', 'g', 'o', '_', 'p', 'a', 'c', 'k', 'a', 'g', 
'e', '\030', '\013', ' ', '\001', '(', '\t', 'R', '\t', 'g', 'o', 'P', 'a', 'c', 'k', 'a', 'g', 'e', '\022', '5', '\n', '\023', 'c', 'c', '_', 
'g', 'e', 'n', 'e', 'r', 'i', 'c', '_', 's', 'e', 'r', 'v', 'i', 'c', 'e', 's', '\030', '\020', ' ', '\001', '(', '\010', ':', '\005', 'f', 
'a', 'l', 's', 'e', 'R', '\021', 'c', 'c', 'G', 'e', 'n', 'e', 'r', 'i', 'c', 'S', 'e', 'r', 'v', 'i', 'c', 'e', 's', '\022', '9', 
'\n', '\025', 'j', 'a', 'v', 'a', '_', 'g', 'e', 'n', 'e', 'r', 'i', 'c', '_', 's', 'e', 'r', 'v', 'i', 'c', 'e', 's', '\030', '\021', 
' ', '\001', '(', '\010', ':', '\005', 'f', 'a', 'l', 's', 'e', 'R', '\023', 'j', 'a', 'v', 'a', 'G', 'e', 'n', 'e', 'r', 'i', 'c', 'S', 
'e', 'r', 'v', 'i', 'c', 'e', 's', '\022', '5', '\n', '\023', 'p', 'y', '_', 'g', 'e', 'n', 'e', 'r', 'i', 'c', '_', 's', 'e', 'r', 
'v', 'i', 'c', 'e', 's', '\030', '\022', ' ', '\001', '(', '\010', ':', '\005', 'f', 'a', 'l', 's', 'e', 'R', '\021', 'p', 'y', 'G', 'e', 'n', 
'e', 'r', 'i', 'c', 'S', 'e', 'r', 'v', 'i', 'c', 'e', 's', '\022', '7', '\n', '\024', 'p', 'h', 'p', '_', 'g', 'e', 'n', 'e', 'r', 
'i', 'c', '_', 's', 'e', 'r', 'v', 'i', 'c', 'e', 's', '\030', '*', ' ', '\001', '(', '\010', ':', '\005', 'f', 'a', 'l', 's', 'e', 'R', 
'\022', 'p', 'h', 'p', 'G', 'e', 'n', 'e', 'r', 'i', 'c', 'S', 'e', 'r', 'v', 'i', 'c', 'e', 's', '\022', '%', '\n', '\n', 'd', 'e', 
'p', 'r', 'e', 'c', 'a', 't', 'e', 'd', '\030', '\027', ' ', '\001', '(', '\010', ':', '\005', 'f', 'a', 'l', 's', 'e', 'R', '\n', 'd', 'e', 
'p', 'r', 'e', 'c', 'a', 't', 'e', 'd', '\022', '.', '\n', '\020', 'c', 'c', '_', 'e', 'n', 'a', 'b', 'l', 'e', '_', 'a', 'r', 'e', 
'n', 'a', 's', '\030', '\037', ' ', '\001', '(', '\010', ':', '\004', 't', 'r', 'u', 'e', 'R', '\016', 'c', 'c', 'E', 'n', 'a', 'b', 'l', 'e', 
'A', 'r', 'e', 'n', 'a', 's', '\022', '*', '\n', '\021', 'o', 'b', 'j', 'c', '_', 'c', 'l', 'a', 's', 's', '_', 'p', 'r', 'e', 'f', 
'i', 'x', '\030', '$', ' ', '\001', '(', '\t', 'R', '\017', 'o', 'b', 'j', 'c', 'C', 'l', 'a', 's', 's', 'P', 'r', 'e', 'f', 'i', 'x', 
'\022', ')', '\n', '\020', 'c', 's', 'h', 'a', 'r', 'p', '_', 'n', 'a', 'm', 'e', 's', 'p', 'a', 'c', 'e', '\030', '%', ' ', '\001', '(', 
'\t', 'R', '\017', 'c', 's', 'h', 'a', 'r', 'p', 'N', 'a', 'm', 'e', 's', 'p', 'a', 'c', 'e', '\022', '!', '\n', '\014', 's', 'w', 'i', 
'f', 't', '_', 'p', 'r', 'e', 'f', 'i', 'x', '\030', '\'', ' ', '\001', '(', '\t', 'R', '\013', 's', 'w', 'i', 'f', 't', 'P', 'r', 'e', 
'f', 'i', 'x', '\022', '(', '\n', '\020', 'p', 'h', 'p', '_', 'c', 'l', 'a', 's', 's', '_', 'p', 'r', 'e', 'f', 'i', 'x', '\030', '(', 
' ', '\001', '(', '\t', 'R', '\016', 'p', 'h', 'p', 'C', 'l', 'a', 's', 's', 'P', 'r', 'e', 'f', 'i', 'x', '\022', '#', '\n', '\r', 'p', 
'h', 'p', '_', 'n', 'a', 'm', 'e', 's', 'p', 'a', 'c', 'e', '\030', ')', ' ', '\001', '(', '\t', 'R', '\014', 'p', 'h', 'p', 'N', 'a', 
'm', 'e', 's', 'p', 'a', 'c', 'e', '\022', '4', '\n', '\026', 'p', 'h', 'p', '_', 'm', 'e', 't', 'a', 'd', 'a', 't', 'a', '_', 'n', 
'a', 'm', 'e', 's', 'p', 'a', 'c', 'e', '\030', ',', ' ', '\001', '(', '\t', 'R', '\024', 'p', 'h', 'p', 'M', 'e', 't', 'a', 'd', 'a', 
't', 'a', 'N', 'a', 'm', 'e', 's', 'p', 'a', 'c', 'e', '\022', '!', '\n', '\014', 'r', 'u', 'b', 'y', '_', 'p', 'a', 'c', 'k', 'a', 
'g', 'e', '\030', '-', ' ', '\001', '(', '\t', 'R', '\013', 'r', 'u', 'b', 'y', 'P', 'a', 'c', 'k', 'a', 'g', 'e', '\022', 'X', '\n', '\024', 
'u', 'n', 'i', 'n', 't', 'e', 'r', 'p', 'r', 'e', 't', 'e', 'd', '_', 'o', 'p', 't', 'i', 'o', 'n', '\030', '\347', '\007', ' ', '\003', 
'(', '\013', '2', '$', '.', 'g', 'o', 'o', 'g', 'l', 'e', '.', 'p', 'r', 'o', 't', 'o', 'b', 'u', 'f', '.', 'U', 'n', 'i', 'n', 
't', 'e', 'r', 'p', 'r', 'e', 't', 'e', 'd', 'O', 'p', 't', 'i', 'o', 'n', 'R', '\023', 'u', 'n', 'i', 'n', 't', 'e', 'r', 'p', 
'r', 'e', 't', 'e', 'd', 'O', 'p', 't', 'i', 'o', 'n', '\"', ':', '\n', '\014', 'O', 'p', 't', 'i', 'm', 'i', 'z', 'e', 'M', 'o', 
'd', 'e', '\022', '\t', '\n', '\005', 'S', 'P', 'E', 'E', 'D', '\020', '\001', '\022', '\r', '\n', '\t', 'C', 'O', 'D', 'E', '_', 'S', 'I', 'Z', 
'E', '\020', '\002', '\022', '\020', '\n', '\014', 'L', 'I', 'T', 'E', '_', 'R', 'U', 'N', 'T', 'I', 'M', 'E', '\020', '\003', '*', '\t', '\010', '\350', 
'\007', '\020', '\200', '\200', '\200', '\200', '\002', 'J', '\004', '\010', '&', '\020', '\'', '\"', '\343', '\002', '\n', '\016', 'M', 'e', 's', 's', 'a', 'g', 'e', 
'O', 'p', 't', 'i', 'o', 'n', 's', '\022', '<', '\n', '\027', 'm', 'e', 's', 's', 'a', 'g', 'e', '_', 's', 'e', 't', '_', 'w', 'i', 
'r', 'e', '_', 'f', 'o', 'r', 'm', 'a', 't', '\030', '\001', ' ', '\001', '(', '\010', ':', '\005', 'f', 'a', 'l', 's', 'e', 'R', '\024', 'm', 
'e', 's', 's', 'a', 'g', 'e', 'S', 'e', 't', 'W', 'i', 'r', 'e', 'F', 'o', 'r', 'm', 'a', 't', '\022', 'L', '\n', '\037', 'n', 'o', 
'_', 's', 't', 'a', 'n', 'd', 'a', 'r', 'd', '_', 'd', 'e', 's', 'c', 'r', 'i', 'p', 't', 'o', 'r', '_', 'a', 'c', 'c', 'e', 
's', 's', 'o', 'r', '\030', '\002', ' ', '\001', '(', '\010', ':', '\005', 'f', 'a', 'l', 's', 'e', 'R', '\034', 'n', 'o', 'S', 't', 'a', 'n', 
'd', 'a', 'r', 'd', 'D', 'e', 's', 'c', 'r', 'i', 'p', 't', 'o', 'r', 'A', 'c', 'c', 'e', 's', 's', 'o', 'r', '\022', '%', '\n', 
'\n', 'd', 'e', 'p', 'r', 'e', 'c', 'a', 't', 'e', 'd', '\030', '\003', ' ', '\001', '(', '\010', ':', '\005', 'f', 'a', 'l', 's', 'e', 'R', 
'\n', 'd', 'e', 'p', 'r', 'e', 'c', 'a', 't', 'e', 'd', '\022', '\033', '\n', '\t', 'm', 'a', 'p', '_', 'e', 'n', 't', 'r', 'y', '\030', 
'\007', ' ', '\001', '(', '\010', 'R', '\010', 'm', 'a', 'p', 'E', 'n', 't', 'r', 'y', '\022', 'X', '\n', '\024', 'u', 'n', 'i', 'n', 't', 'e', 
'r', 'p', 'r', 'e', 't', 'e', 'd', '_', 'o', 'p', 't', 'i', 'o', 'n', '\030', '\347', '\007', ' ', '\003', '(', '\013', '2', '$', '.', 'g', 
'o', 'o', 'g', 'l', 'e', '.', 'p', 'r', 'o', 't', 'o', 'b', 'u', 'f', '.', 'U', 'n', 'i', 'n', 't', 'e', 'r', 'p', 'r', 'e', 
't', 'e', 'd', 'O', 'p', 't', 'i', 'o', 'n', 'R', '\023', 'u', 'n', 'i', 'n', 't', 'e', 'r', 'p', 'r', 'e', 't', 'e', 'd', 'O', 
'p', 't', 'i', 'o', 'n', '*', '\t', '\010', '\350', '\007', '\020', '\200', '\200', '\200', '\200', '\002', 'J', '\004', '\010', '\004', '\020', '\005', 'J', '\004', '\010', 
'\005', '\020', '\006', 'J', '\004', '\010', '\006', '\020', '\007', 'J', '\004', '\010', '\010', '\020', '\t', 'J', '\004', '\010', '\t', '\020', '\n', '\"', '\222', '\004', '\n', 
'\014', 'F', 'i', 'e', 'l', 'd', 'O', 'p', 't', 'i', 'o', 'n', 's', '\022', 'A', '\n', '\005', 'c', 't', 'y', 'p', 'e', '\030', '\001', ' ', 
'\001', '(', '\016', '2', '#', '.', 'g', 'o', 'o', 'g', 'l', 'e', '.', 'p', 'r', 'o', 't', 'o', 'b', 'u', 'f', '.', 'F', 'i', 'e', 
'l', 'd', 'O', 'p', 't', 'i', 'o', 'n', 's', '.', 'C', 'T', 'y', 'p', 'e', ':', '\006', 'S', 'T', 'R', 'I', 'N', 'G', 'R', '\005', 
'c', 't', 'y', 'p', 'e', '\022', '\026', '\n', '\006', 'p', 'a', 'c', 'k', 'e', 'd', '\030', '\002', ' ', '\001', '(', '\010', 'R', '\006', 'p', 'a', 
'c', 'k', 'e', 'd', '\022', 'G', '\n', '\006', 'j', 's', 't', 'y', 'p', 'e', '\030', '\006', ' ', '\001', '(', '\016', '2', '$', '.', 'g', 'o', 
'o', 'g', 'l', 'e', '.', 'p', 'r', 'o', 't', 'o', 'b', 'u', 'f', '.', 'F', 'i', 'e', 'l', 'd', 'O', 'p', 't', 'i', 'o', 'n', 
's', '.', 'J', 'S', 'T', 'y', 'p', 'e', ':', '\t', 'J', 'S', '_', 'N', 'O', 'R', 'M', 'A', 'L', 'R', '\006', 'j', 's', 't', 'y', 
'p', 'e', '\022', '\031', '\n', '\004', 'l', 'a', 'z', 'y', '\030', '\005', ' ', '\001', '(', '\010', ':', '\005', 'f', 'a', 'l', 's', 'e', 'R', '\004', 
'l', 'a', 'z', 'y', '\022', '.', '\n', '\017', 'u', 'n', 'v', 'e', 'r', 'i', 'f', 'i', 'e', 'd', '_', 'l', 'a', 'z', 'y', '\030', '\017', 
' ', '\001', '(', '\010', ':', '\005', 'f', 'a', 'l', 's', 'e', 'R', '\016', 'u', 'n', 'v', 'e', 'r', 'i', 'f', 'i', 'e', 'd', 'L', 'a', 
'z', 'y', '\022', '%', '\n', '\n', 'd', 'e', 'p', 'r', 'e', 'c', 'a', 't', 'e', 'd', '\030', '\003', ' ', '\001', '(', '\010', ':', '\005', 'f', 
'a', 'l', 's', 'e', 'R', '\n', 'd', 'e', 'p', 'r', 'e', 'c', 'a', 't', 'e', 'd', '\022', '\031', '\n', '\004', 'w', 'e', 'a', 'k', '\030', 
'\n', ' ', '\001', '(', '\010', ':', '\005', 'f', 'a', 'l', 's', 'e', 'R', '\004', 'w', 'e', 'a', 'k', '\022', 'X', '\n', '\024', 'u', 'n', 'i', 
'n', 't', 'e', 'r', 'p', 'r', 'e', 't', 'e', 'd', '_', 'o', 'p', 't', 'i', 'o', 'n', '\030', '\347', '\007', ' ', '\003', '(', '\013', '2', 
'$', '.', 'g', 'o', 'o', 'g', 'l', 'e', '.', 'p', 'r', 'o', 't', 'o', 'b', 'u', 'f', '.', 'U', 'n', 'i', 'n', 't', 'e', 'r', 
'p', 'r', 'e', 't', 'e', 'd', 'O', 'p', 't', 'i', 'o', 'n', 'R', '\023', 'u', 'n', 'i', 'n', 't', 'e', 'r', 'p', 'r', 'e', 't', 
'e', 'd', 'O', 'p', 't', 'i', 'o', 'n', '\"', '/', '\n', '\005', 'C', 'T', 'y', 'p', 'e', '\022', '\n', '\n', '\006', 'S', 'T', 'R', 'I', 
'N', 'G', '\020', '\000', '\022', '\010', '\n', '\004', 'C', 'O', 'R', 'D', '\020', '\001', '\022', '\020', '\n', '\014', 'S', 'T', 'R', 'I', 'N', 'G', '_', 
'P', 'I', 'E', 'C', 'E', '\020', '\002', '\"', '5', '\n', '\006', 'J', 'S', 'T', 'y', 'p', 'e', '\022', '\r', '\n', '\t', 'J', 'S', '_', 'N', 
'O', 'R', 'M', 'A', 'L', '\020', '\000', '\022', '\r', '\n', '\t', 'J', 'S', '_', 'S', 'T', 'R', 'I', 'N', 'G', '\020', '\001', '\022', '\r', '\n', 
'\t', 'J', 'S', '_', 'N', 'U', 'M', 'B', 'E', 'R', '\020', '\002', '*', '\t', '\010', '\350', '\007', '\020', '\200', '\200', '\200', '\200', '\002', 'J', '\004', 
'\010', '\004', '\020', '\005', '\"', 's', '\n', '\014', 'O', 'n', 'e', 'o', 'f', 'O', 'p', 't', 'i', 'o', 'n', 's', '\022', 'X', '\n', '\024', 'u', 
'n', 'i', 'n', 't', 'e', 'r', 'p', 'r', 'e', 't', 'e', 'd', '_', 'o', 'p', 't', 'i', 'o', 'n', '\030', '\347', '\007', ' ', '\003', '(', 
'\013', '2', '$', '.', 'g', 'o', 'o', 'g', 'l', 'e', '.', 'p', 'r', 'o', 't', 'o', 'b', 'u', 'f', '.', 'U', 'n', 'i', 'n', 't', 
'e', 'r', 'p', 'r', 'e', 't', 'e', 'd', 'O', 'p', 't', 'i', 'o', 'n', 'R', '\023', 'u', 'n', 'i', 'n', 't', 'e', 'r', 'p', 'r', 
'e', 't', 'e', 'd', 'O', 'p', 't', 'i', 'o', 'n', '*', '\t', '\010', '\350', '\007', '\020', '\200', '\200', '\200', '\200', '\002', '\"', '\300', '\001', '\n', 
'\013', 'E', 'n', 'u', 'm', 'O', 'p', 't', 'i', 'o', 'n', 's', '\022', '\037', '\n', '\013', 'a', 'l', 'l', 'o', 'w', '_', 'a', 'l', 'i', 
'a', 's', '\030', '\002', ' ', '\001', '(', '\010', 'R', '\n', 'a', 'l', 'l', 'o', 'w', 'A', 'l', 'i', 'a', 's', '\022', '%', '\n', '\n', 'd', 
'e', 'p', 'r', 'e', 'c', 'a', 't', 'e', 'd', '\030', '\003', ' ', '\001', '(', '\010', ':', '\005', 'f', 'a', 'l', 's', 'e', 'R', '\n', 'd', 
'e', 'p', 'r', 'e', 'c', 'a', 't', 'e', 'd', '\022', 'X', '\n', '\024', 'u', 'n', 'i', 'n', 't', 'e', 'r', 'p', 'r', 'e', 't', 'e', 
'd', '_', 'o', 'p', 't', 'i', 'o', 'n', '\030', '\347', '\007', ' ', '\003', '(', '\013', '2', '$', '.', 'g', 'o', 'o', 'g', 'l', 'e', '.', 
'p', 'r', 'o', 't', 'o', 'b', 'u', 'f', '.', 'U', 'n', 'i', 'n', 't', 'e', 'r', 'p', 'r', 'e', 't', 'e', 'd', 'O', 'p', 't', 
'i', 'o', 'n', 'R', '\023', 'u', 'n', 'i', 'n', 't', 'e', 'r', 'p', 'r', 'e', 't', 'e', 'd', 'O', 'p', 't', 'i', 'o', 'n', '*', 
'\t', '\010', '\350', '\007', '\020', '\200', '\200', '\200', '\200', '\002', 'J', '\004', '\010', '\005', '\020', '\006', '\"', '\236', '\001', '\n', '\020', 'E', 'n', 'u', 'm', 
'V', 'a', 'l', 'u', 'e', 'O', 'p', 't', 'i', 'o', 'n', 's', '\022', '%', '\n', '\n', 'd', 'e', 'p', 'r', 'e', 'c', 'a', 't', 'e', 
'd', '\030', '\001', ' ', '\001', '(', '\010', ':', '\005', 'f', 'a', 'l', 's', 'e', 'R', '\n', 'd', 'e', 'p', 'r', 'e', 'c', 'a', 't', 'e', 
'd', '\022', 'X', '\n', '\024', 'u', 'n', 'i', 'n', 't', 'e', 'r', 'p', 'r', 'e', 't', 'e', 'd', '_', 'o', 'p', 't', 'i', 'o', 'n', 
'\030', '\347', '\007', ' ', '\003', '(', '\013', '2', '$', '.', 'g', 'o', 'o', 'g', 'l', 'e', '.', 'p', 'r', 'o', 't', 'o', 'b', 'u', 'f', 
'.', 'U', 'n', 'i', 'n', 't', 'e', 'r', 'p', 'r', 'e', 't', 'e', 'd', 'O', 'p', 't', 'i', 'o', 'n', 'R', '\023', 'u', 'n', 'i', 
'n', 't', 'e', 'r', 'p', 'r', 'e', 't', 'e', 'd', 'O', 'p', 't', 'i', 'o', 'n', '*', '\t', '\010', '\350', '\007', '\020', '\200', '\200', '\200', 
'\200', '\002', '\"', '\234', '\001', '\n', '\016', 'S', 'e', 'r', 'v', 'i', 'c', 'e', 'O', 'p', 't', 'i', 'o', 'n', 's', '\022', '%', '\n', '\n', 
'd', 'e', 'p', 'r', 'e', 'c', 'a', 't', 'e', 'd', '\030', '!', ' ', '\001', '(', '\010', ':', '\005', 'f', 'a', 'l', 's', 'e', 'R', '\n', 
'd', 'e', 'p', 'r', 'e', 'c', 'a', 't', 'e', 'd', '\022', 'X', '\n', '\024', 'u', 'n', 'i', 'n', 't', 'e', 'r', 'p', 'r', 'e', 't', 
'e', 'd', '_', 'o', 'p', 't', 'i', 'o', 'n', '\030', '\347', '\007', ' ', '\003', '(', '\013', '2', '$', '.', 'g', 'o', 'o', 'g', 'l', 'e', 
'.', 'p', 'r', 'o', 't', 'o', 'b', 'u', 'f', '.', 'U', 'n', 'i', 'n', 't', 'e', 'r', 'p', 'r', 'e', 't', 'e', 'd', 'O', 'p', 
't', 'i', 'o', 'n', 'R', '\023', 'u', 'n', 'i', 'n', 't', 'e', 'r', 'p', 'r', 'e', 't', 'e', 'd', 'O', 'p', 't', 'i', 'o', 'n', 
'*', '\t', '\010', '\350', '\007', '\020', '\200', '\200', '\200', '\200', '\002', '\"', '\340', '\002', '\n', '\r', 'M', 'e', 't', 'h', 'o', 'd', 'O', 'p', 't', 
'i', 'o', 'n', 's', '\022', '%', '\n', '\n', 'd', 'e', 'p', 'r', 'e', 'c', 'a', 't', 'e', 'd', '\030', '!', ' ', '\001', '(', '\010', ':', 
'\005', 'f', 'a', 'l', 's', 'e', 'R', '\n', 'd', 'e', 'p', 'r', 'e', 'c', 'a', 't', 'e', 'd', '\022', 'q', '\n', '\021', 'i', 'd', 'e', 
'm', 'p', 'o', 't', 'e', 'n', 'c', 'y', '_', 'l', 'e', 'v', 'e', 'l', '\030', '\"', ' ', '\001', '(', '\016', '2', '/', '.', 'g', 'o', 
'o', 'g', 'l', 'e', '.', 'p', 'r', 'o', 't', 'o', 'b', 'u', 'f', '.', 'M', 'e', 't', 'h', 'o', 'd', 'O', 'p', 't', 'i', 'o', 
'n', 's', '.', 'I', 'd', 'e', 'm', 'p', 'o', 't', 'e', 'n', 'c', 'y', 'L', 'e', 'v', 'e', 'l', ':', '\023', 'I', 'D', 'E', 'M', 
'P', 'O', 'T', 'E', 'N', 'C', 'Y', '_', 'U', 'N', 'K', 'N', 'O', 'W', 'N', 'R', '\020', 'i', 'd', 'e', 'm', 'p', 'o', 't', 'e', 
'n', 'c', 'y', 'L', 'e', 'v', 'e', 'l', '\022', 'X', '\n', '\024', 'u', 'n', 'i', 'n', 't', 'e', 'r', 'p', 'r', 'e', 't', 'e', 'd', 
'_', 'o', 'p', 't', 'i', 'o', 'n', '\030', '\347', '\007', ' ', '\003', '(', '\013', '2', '$', '.', 'g', 'o', 'o', 'g', 'l', 'e', '.', 'p', 
'r', 'o', 't', 'o', 'b', 'u', 'f', '.', 'U', 'n', 'i', 'n', 't', 'e', 'r', 'p', 'r', 'e', 't', 'e', 'd', 'O', 'p', 't', 'i', 
'o', 'n', 'R', '\023', 'u', 'n', 'i', 'n', 't', 'e', 'r', 'p', 'r', 'e', 't', 'e', 'd', 'O', 'p', 't', 'i', 'o', 'n', '\"', 'P', 
'\n', '\020', 'I', 'd', 'e', 'm', 'p', 'o', 't', 'e', 'n', 'c', 'y', 'L', 'e', 'v', 'e', 'l', '\022', '\027', '\n', '\023', 'I', 'D', 'E', 
'M', 'P', 'O', 'T', 'E', 'N', 'C', 'Y', '_', 'U', 'N', 'K', 'N', 'O', 'W', 'N', '\020', '\000', '\022', '\023', '\n', '\017', 'N', 'O', '_', 
'S', 'I', 'D', 'E', '_', 'E', 'F', 'F', 'E', 'C', 'T', 'S', '\020', '\001', '\022', '\016', '\n', '\n', 'I', 'D', 'E', 'M', 'P', 'O', 'T', 
'E', 'N', 'T', '\020', '\002', '*', '\t', '\010', '\350', '\007', '\020', '\200', '\200', '\200', '\200', '\002', '\"', '\232', '\003', '\n', '\023', 'U', 'n', 'i', 'n', 
't', 'e', 'r', 'p', 'r', 'e', 't', 'e', 'd', 'O', 'p', 't', 'i', 'o', 'n', '\022', 'A', '\n', '\004', 'n', 'a', 'm', 'e', '\030', '\002', 
' ', '\003', '(', '\013', '2', '-', '.', 'g', 'o', 'o', 'g', 'l', 'e', '.', 'p', 'r', 'o', 't', 'o', 'b', 'u', 'f', '.', 'U', 'n', 
'i', 'n', 't', 'e', 'r', 'p', 'r', 'e', 't', 'e', 'd', 'O', 'p', 't', 'i', 'o', 'n', '.', 'N', 'a', 'm', 'e', 'P', 'a', 'r', 
't', 'R', '\004', 'n', 'a', 'm', 'e', '\022', ')', '\n', '\020', 'i', 'd', 'e', 'n', 't', 'i', 'f', 'i', 'e', 'r', '_', 'v', 'a', 'l', 
'u', 'e', '\030', '\003', ' ', '\001', '(', '\t', 'R', '\017', 'i', 'd', 'e', 'n', 't', 'i', 'f', 'i', 'e', 'r', 'V', 'a', 'l', 'u', 'e', 
'\022', ',', '\n', '\022', 'p', 'o', 's', 'i', 't', 'i', 'v', 'e', '_', 'i', 'n', 't', '_', 'v', 'a', 'l', 'u', 'e', '\030', '\004', ' ', 
'\001', '(', '\004', 'R', '\020', 'p', 'o', 's', 'i', 't', 'i', 'v', 'e', 'I', 'n', 't', 'V', 'a', 'l', 'u', 'e', '\022', ',', '\n', '\022', 
'n', 'e', 'g', 'a', 't', 'i', 'v', 'e', '_', 'i', 'n', 't', '_', 'v', 'a', 'l', 'u', 'e', '\030', '\005', ' ', '\001', '(', '\003', 'R', 
'\020', 'n', 'e', 'g', 'a', 't', 'i', 'v', 'e', 'I', 'n', 't', 'V', 'a', 'l', 'u', 'e', '\022', '!', '\n', '\014', 'd', 'o', 'u', 'b', 
'l', 'e', '_', 'v', 'a', 'l', 'u', 'e', '\030', '\006', ' ', '\001', '(', '\001', 'R', '\013', 'd', 'o', 'u', 'b', 'l', 'e', 'V', 'a', 'l', 
'u', 'e', '\022', '!', '\n', '\014', 's', 't', 'r', 'i', 'n', 'g', '_', 'v', 'a', 'l', 'u', 'e', '\030', '\007', ' ', '\001', '(', '\014', 'R', 
'\013', 's', 't', 'r', 'i', 'n', 'g', 'V', 'a', 'l', 'u', 'e', '\022', '\'', '\n', '\017', 'a', 'g', 'g', 'r', 'e', 'g', 'a', 't', 'e', 
'_', 'v', 'a', 'l', 'u', 'e', '\030', '\010', ' ', '\001', '(', '\t', 'R', '\016', 'a', 'g', 'g', 'r', 'e', 'g', 'a', 't', 'e', 'V', 'a', 
'l', 'u', 'e', '\032', 'J', '\n', '\010', 'N', 'a', 'm', 'e', 'P', 'a', 'r', 't', '\022', '\033', '\n', '\t', 'n', 'a', 'm', 'e', '_', 'p', 
'a', 'r', 't', '\030', '\001', ' ', '\002', '(', '\t', 'R', '\010', 'n', 'a', 'm', 'e', 'P', 'a', 'r', 't', '\022', '!', '\n', '\014', 'i', 's', 
'_', 'e', 'x', 't', 'e', 'n', 's', 'i', 'o', 'n', '\030', '\002', ' ', '\002', '(', '\010', 'R', '\013', 'i', 's', 'E', 'x', 't', 'e', 'n', 
's', 'i', 'o', 'n', '\"', '\247', '\002', '\n', '\016', 'S', 'o', 'u', 'r', 'c', 'e', 'C', 'o', 'd', 'e', 'I', 'n', 'f', 'o', '\022', 'D', 
'\n', '\010', 'l', 'o', 'c', 'a', 't', 'i', 'o', 'n', '\030', '\001', ' ', '\003', '(', '\013', '2', '(', '.', 'g', 'o', 'o', 'g', 'l', 'e', 
'.', 'p', 'r', 'o', 't', 'o', 'b', 'u', 'f', '.', 'S', 'o', 'u', 'r', 'c', 'e', 'C', 'o', 'd', 'e', 'I', 'n', 'f', 'o', '.', 
'L', 'o', 'c', 'a', 't', 'i', 'o', 'n', 'R', '\010', 'l', 'o', 'c', 'a', 't', 'i', 'o', 'n', '\032', '\316', '\001', '\n', '\010', 'L', 'o', 
'c', 'a', 't', 'i', 'o', 'n', '\022', '\026', '\n', '\004', 'p', 'a', 't', 'h', '\030', '\001', ' ', '\003', '(', '\005', 'B', '\002', '\020', '\001', 'R', 
'\004', 'p', 'a', 't', 'h', '\022', '\026', '\n', '\004', 's', 'p', 'a', 'n', '\030', '\002', ' ', '\003', '(', '\005', 'B', '\002', '\020', '\001', 'R', '\004', 
's', 'p', 'a', 'n', '\022', ')', '\n', '\020', 'l', 'e', 'a', 'd', 'i', 'n', 'g', '_', 'c', 'o', 'm', 'm', 'e', 'n', 't', 's', '\030', 
'\003', ' ', '\001', '(', '\t', 'R', '\017', 'l', 'e', 'a', 'd', 'i', 'n', 'g', 'C', 'o', 'm', 'm', 'e', 'n', 't', 's', '\022', '+', '\n', 
'\021', 't', 'r', 'a', 'i', 'l', 'i', 'n', 'g', '_', 'c', 'o', 'm', 'm', 'e', 'n', 't', 's', '\030', '\004', ' ', '\001', '(', '\t', 'R', 
'\020', 't', 'r', 'a', 'i', 'l', 'i', 'n', 'g', 'C', 'o', 'm', 'm', 'e', 'n', 't', 's', '\022', ':', '\n', '\031', 'l', 'e', 'a', 'd', 
'i', 'n', 'g', '_', 'd', 'e', 't', 'a', 'c', 'h', 'e', 'd', '_', 'c', 'o', 'm', 'm', 'e', 'n', 't', 's', '\030', '\006', ' ', '\003', 
'(', '\t', 'R', '\027', 'l', 'e', 'a', 'd', 'i', 'n', 'g', 'D', 'e', 't', 'a', 'c', 'h', 'e', 'd', 'C', 'o', 'm', 'm', 'e', 'n', 
't', 's', '\"', '\321', '\001', '\n', '\021', 'G', 'e', 'n', 'e', 'r', 'a', 't', 'e', 'd', 'C', 'o', 'd', 'e', 'I', 'n', 'f', 'o', '\022', 
'M', '\n', '\n', 'a', 'n', 'n', 'o', 't', 'a', 't', 'i', 'o', 'n', '\030', '\001', ' ', '\003', '(', '\013', '2', '-', '.', 'g', 'o', 'o', 
'g', 'l', 'e', '.', 'p', 'r', 'o', 't', 'o', 'b', 'u', 'f', '.', 'G', 'e', 'n', 'e', 'r', 'a', 't', 'e', 'd', 'C', 'o', 'd', 
'e', 'I', 'n', 'f', 'o', '.', 'A', 'n', 'n', 'o', 't', 'a', 't', 'i', 'o', 'n', 'R', '\n', 'a', 'n', 'n', 'o', 't', 'a', 't', 
'i', 'o', 'n', '\032', 'm', '\n', '\n', 'A', 'n', 'n', 'o', 't', 'a', 't', 'i', 'o', 'n', '\022', '\026', '\n', '\004', 'p', 'a', 't', 'h', 
'\030', '\001', ' ', '\003', '(', '\005', 'B', '\002', '\020', '\001', 'R', '\004', 'p', 'a', 't', 'h', '\022', '\037', '\n', '\013', 's', 'o', 'u', 'r', 'c', 
'e', '_', 'f', 'i', 'l', 'e', '\030', '\002', ' ', '\001', '(', '\t', 'R', '\n', 's', 'o', 'u', 'r', 'c', 'e', 'F', 'i', 'l', 'e', '\022', 
'\024', '\n', '\005', 'b', 'e', 'g', 'i', 'n', '\030', '\003', ' ', '\001', '(', '\005', 'R', '\005', 'b', 'e', 'g', 'i', 'n', '\022', '\020', '\n', '\003', 
'e', 'n', 'd', '\030', '\004', ' ', '\001', '(', '\005', 'R', '\003', 'e', 'n', 'd', 'B', '~', '\n', '\023', 'c', 'o', 'm', '.', 'g', 'o', 'o', 
'g', 'l', 'e', '.', 'p', 'r', 'o', 't', 'o', 'b', 'u', 'f', 'B', '\020', 'D', 'e', 's', 'c', 'r', 'i', 'p', 't', 'o', 'r', 'P', 
'r', 'o', 't', 'o', 's', 'H', '\001', 'Z', '-', 'g', 'o', 'o', 'g', 'l', 'e', '.', 'g', 'o', 'l', 'a', 'n', 'g', '.', 'o', 'r', 
'g', '/', 'p', 'r', 'o', 't', 'o', 'b', 'u', 'f', '/', 't', 'y', 'p', 'e', 's', '/', 'd', 'e', 's', 'c', 'r', 'i', 'p', 't', 
'o', 'r', 'p', 'b', '\370', '\001', '\001', '\242', '\002', '\003', 'G', 'P', 'B', '\252', '\002', '\032', 'G', 'o', 'o', 'g', 'l', 'e', '.', 'P', 'r', 
'o', 't', 'o', 'b', 'u', 'f', '.', 'R', 'e', 'f', 'l', 'e', 'c', 't', 'i', 'o', 'n', 
};

static _upb_DefPool_Init *deps[1] = {
  NULL
};

_upb_DefPool_Init google_protobuf_descriptor_proto_upbdefinit = {
  deps,
  &google_protobuf_descriptor_proto_upb_file_layout,
  "google/protobuf/descriptor.proto",
  UPB_STRINGVIEW_INIT(descriptor, 7667)
};

/** upb/decode_fast.c ************************************************************/
// Fast decoder: ~3x the speed of decode.c, but requires x86-64/ARM64.
// Also the table size grows by 2x.
//
// Could potentially be ported to other 64-bit archs that pass at least six
// arguments in registers and have 8 unused high bits in pointers.
//
// The overall design is to create specialized functions for every possible
// field type (eg. oneof boolean field with a 1 byte tag) and then dispatch
// to the specialized function as quickly as possible.



/* Must be last. */

#if UPB_FASTTABLE

// The standard set of arguments passed to each parsing function.
// Thanks to x86-64 calling conventions, these will stay in registers.
#define UPB_PARSE_PARAMS                                             \
  upb_Decoder *d, const char *ptr, upb_Message *msg, intptr_t table, \
      uint64_t hasbits, uint64_t data

#define UPB_PARSE_ARGS d, ptr, msg, table, hasbits, data

#define RETURN_GENERIC(m)                                 \
  /* Uncomment either of these for debugging purposes. */ \
  /* fprintf(stderr, m); */                               \
  /*__builtin_trap(); */                                  \
  return fastdecode_generic(d, ptr, msg, table, hasbits, 0);

typedef enum {
  CARD_s = 0, /* Singular (optional, non-repeated) */
  CARD_o = 1, /* Oneof */
  CARD_r = 2, /* Repeated */
  CARD_p = 3  /* Packed Repeated */
} upb_card;

UPB_NOINLINE
static const char* fastdecode_isdonefallback(UPB_PARSE_PARAMS) {
  int overrun = data;
  int status;
  ptr = decode_isdonefallback_inl(d, ptr, overrun, &status);
  if (ptr == NULL) {
    return fastdecode_err(d, status);
  }
  data = fastdecode_loadtag(ptr);
  UPB_MUSTTAIL return fastdecode_tagdispatch(UPB_PARSE_ARGS);
}

UPB_FORCEINLINE
static const char* fastdecode_dispatch(UPB_PARSE_PARAMS) {
  if (UPB_UNLIKELY(ptr >= d->limit_ptr)) {
    int overrun = ptr - d->end;
    if (UPB_LIKELY(overrun == d->limit)) {
      // Parse is finished.
      *(uint32_t*)msg |= hasbits;  // Sync hasbits.
      const upb_MiniTable* l = decode_totablep(table);
      return UPB_UNLIKELY(l->required_count)
                 ? decode_checkrequired(d, ptr, msg, l)
                 : ptr;
    } else {
      data = overrun;
      UPB_MUSTTAIL return fastdecode_isdonefallback(UPB_PARSE_ARGS);
    }
  }

  // Read two bytes of tag data (for a one-byte tag, the high byte is junk).
  data = fastdecode_loadtag(ptr);
  UPB_MUSTTAIL return fastdecode_tagdispatch(UPB_PARSE_ARGS);
}

UPB_FORCEINLINE
static bool fastdecode_checktag(uint16_t data, int tagbytes) {
  if (tagbytes == 1) {
    return (data & 0xff) == 0;
  } else {
    return data == 0;
  }
}

UPB_FORCEINLINE
static const char* fastdecode_longsize(const char* ptr, int* size) {
  int i;
  UPB_ASSERT(*size & 0x80);
  *size &= 0xff;
  for (i = 0; i < 3; i++) {
    ptr++;
    size_t byte = (uint8_t)ptr[-1];
    *size += (byte - 1) << (7 + 7 * i);
    if (UPB_LIKELY((byte & 0x80) == 0)) return ptr;
  }
  ptr++;
  size_t byte = (uint8_t)ptr[-1];
  // len is limited by 2gb not 4gb, hence 8 and not 16 as normally expected
  // for a 32 bit varint.
  if (UPB_UNLIKELY(byte >= 8)) return NULL;
  *size += (byte - 1) << 28;
  return ptr;
}

UPB_FORCEINLINE
static bool fastdecode_boundscheck(const char* ptr, size_t len,
                                   const char* end) {
  uintptr_t uptr = (uintptr_t)ptr;
  uintptr_t uend = (uintptr_t)end + 16;
  uintptr_t res = uptr + len;
  return res < uptr || res > uend;
}

UPB_FORCEINLINE
static bool fastdecode_boundscheck2(const char* ptr, size_t len,
                                    const char* end) {
  // This is one extra branch compared to the more normal:
  //   return (size_t)(end - ptr) < size;
  // However it is one less computation if we are just about to use "ptr + len":
  //   https://godbolt.org/z/35YGPz
  // In microbenchmarks this shows an overall 4% improvement.
  uintptr_t uptr = (uintptr_t)ptr;
  uintptr_t uend = (uintptr_t)end;
  uintptr_t res = uptr + len;
  return res < uptr || res > uend;
}

typedef const char* fastdecode_delimfunc(upb_Decoder* d, const char* ptr,
                                         void* ctx);

UPB_FORCEINLINE
static const char* fastdecode_delimited(upb_Decoder* d, const char* ptr,
                                        fastdecode_delimfunc* func, void* ctx) {
  ptr++;
  int len = (int8_t)ptr[-1];
  if (fastdecode_boundscheck2(ptr, len, d->limit_ptr)) {
    // Slow case: Sub-message is >=128 bytes and/or exceeds the current buffer.
    // If it exceeds the buffer limit, limit/limit_ptr will change during
    // sub-message parsing, so we need to preserve delta, not limit.
    if (UPB_UNLIKELY(len & 0x80)) {
      // Size varint >1 byte (length >= 128).
      ptr = fastdecode_longsize(ptr, &len);
      if (!ptr) {
        // Corrupt wire format: size exceeded INT_MAX.
        return NULL;
      }
    }
    if (ptr - d->end + (int)len > d->limit) {
      // Corrupt wire format: invalid limit.
      return NULL;
    }
    int delta = decode_pushlimit(d, ptr, len);
    ptr = func(d, ptr, ctx);
    decode_poplimit(d, ptr, delta);
  } else {
    // Fast case: Sub-message is <128 bytes and fits in the current buffer.
    // This means we can preserve limit/limit_ptr verbatim.
    const char* saved_limit_ptr = d->limit_ptr;
    int saved_limit = d->limit;
    d->limit_ptr = ptr + len;
    d->limit = d->limit_ptr - d->end;
    UPB_ASSERT(d->limit_ptr == d->end + UPB_MIN(0, d->limit));
    ptr = func(d, ptr, ctx);
    d->limit_ptr = saved_limit_ptr;
    d->limit = saved_limit;
    UPB_ASSERT(d->limit_ptr == d->end + UPB_MIN(0, d->limit));
  }
  return ptr;
}

/* singular, oneof, repeated field handling ***********************************/

typedef struct {
  upb_Array* arr;
  void* end;
} fastdecode_arr;

typedef enum {
  FD_NEXT_ATLIMIT,
  FD_NEXT_SAMEFIELD,
  FD_NEXT_OTHERFIELD
} fastdecode_next;

typedef struct {
  void* dst;
  fastdecode_next next;
  uint32_t tag;
} fastdecode_nextret;

UPB_FORCEINLINE
static void* fastdecode_resizearr(upb_Decoder* d, void* dst,
                                  fastdecode_arr* farr, int valbytes) {
  if (UPB_UNLIKELY(dst == farr->end)) {
    size_t old_size = farr->arr->size;
    size_t old_bytes = old_size * valbytes;
    size_t new_size = old_size * 2;
    size_t new_bytes = new_size * valbytes;
    char* old_ptr = _upb_array_ptr(farr->arr);
    char* new_ptr = upb_Arena_Realloc(&d->arena, old_ptr, old_bytes, new_bytes);
    uint8_t elem_size_lg2 = __builtin_ctz(valbytes);
    farr->arr->size = new_size;
    farr->arr->data = _upb_array_tagptr(new_ptr, elem_size_lg2);
    dst = (void*)(new_ptr + (old_size * valbytes));
    farr->end = (void*)(new_ptr + (new_size * valbytes));
  }
  return dst;
}

UPB_FORCEINLINE
static bool fastdecode_tagmatch(uint32_t tag, uint64_t data, int tagbytes) {
  if (tagbytes == 1) {
    return (uint8_t)tag == (uint8_t)data;
  } else {
    return (uint16_t)tag == (uint16_t)data;
  }
}

UPB_FORCEINLINE
static void fastdecode_commitarr(void* dst, fastdecode_arr* farr,
                                 int valbytes) {
  farr->arr->len =
      (size_t)((char*)dst - (char*)_upb_array_ptr(farr->arr)) / valbytes;
}

UPB_FORCEINLINE
static fastdecode_nextret fastdecode_nextrepeated(upb_Decoder* d, void* dst,
                                                  const char** ptr,
                                                  fastdecode_arr* farr,
                                                  uint64_t data, int tagbytes,
                                                  int valbytes) {
  fastdecode_nextret ret;
  dst = (char*)dst + valbytes;

  if (UPB_LIKELY(!decode_isdone(d, ptr))) {
    ret.tag = fastdecode_loadtag(*ptr);
    if (fastdecode_tagmatch(ret.tag, data, tagbytes)) {
      ret.next = FD_NEXT_SAMEFIELD;
    } else {
      fastdecode_commitarr(dst, farr, valbytes);
      ret.next = FD_NEXT_OTHERFIELD;
    }
  } else {
    fastdecode_commitarr(dst, farr, valbytes);
    ret.next = FD_NEXT_ATLIMIT;
  }

  ret.dst = dst;
  return ret;
}

UPB_FORCEINLINE
static void* fastdecode_fieldmem(upb_Message* msg, uint64_t data) {
  size_t ofs = data >> 48;
  return (char*)msg + ofs;
}

UPB_FORCEINLINE
static void* fastdecode_getfield(upb_Decoder* d, const char* ptr,
                                 upb_Message* msg, uint64_t* data,
                                 uint64_t* hasbits, fastdecode_arr* farr,
                                 int valbytes, upb_card card) {
  switch (card) {
    case CARD_s: {
      uint8_t hasbit_index = *data >> 24;
      // Set hasbit and return pointer to scalar field.
      *hasbits |= 1ull << hasbit_index;
      return fastdecode_fieldmem(msg, *data);
    }
    case CARD_o: {
      uint16_t case_ofs = *data >> 32;
      uint32_t* oneof_case = UPB_PTR_AT(msg, case_ofs, uint32_t);
      uint8_t field_number = *data >> 24;
      *oneof_case = field_number;
      return fastdecode_fieldmem(msg, *data);
    }
    case CARD_r: {
      // Get pointer to upb_Array and allocate/expand if necessary.
      uint8_t elem_size_lg2 = __builtin_ctz(valbytes);
      upb_Array** arr_p = fastdecode_fieldmem(msg, *data);
      char* begin;
      *(uint32_t*)msg |= *hasbits;
      *hasbits = 0;
      if (UPB_LIKELY(!*arr_p)) {
        farr->arr = _upb_Array_New(&d->arena, 8, elem_size_lg2);
        *arr_p = farr->arr;
      } else {
        farr->arr = *arr_p;
      }
      begin = _upb_array_ptr(farr->arr);
      farr->end = begin + (farr->arr->size * valbytes);
      *data = fastdecode_loadtag(ptr);
      return begin + (farr->arr->len * valbytes);
    }
    default:
      UPB_UNREACHABLE();
  }
}

UPB_FORCEINLINE
static bool fastdecode_flippacked(uint64_t* data, int tagbytes) {
  *data ^= (0x2 ^ 0x0);  // Patch data to match packed wiretype.
  return fastdecode_checktag(*data, tagbytes);
}

#define FASTDECODE_CHECKPACKED(tagbytes, card, func)                \
  if (UPB_UNLIKELY(!fastdecode_checktag(data, tagbytes))) {         \
    if (card == CARD_r && fastdecode_flippacked(&data, tagbytes)) { \
      UPB_MUSTTAIL return func(UPB_PARSE_ARGS);                     \
    }                                                               \
    RETURN_GENERIC("packed check tag mismatch\n");                  \
  }

/* varint fields **************************************************************/

UPB_FORCEINLINE
static uint64_t fastdecode_munge(uint64_t val, int valbytes, bool zigzag) {
  if (valbytes == 1) {
    return val != 0;
  } else if (zigzag) {
    if (valbytes == 4) {
      uint32_t n = val;
      return (n >> 1) ^ -(int32_t)(n & 1);
    } else if (valbytes == 8) {
      return (val >> 1) ^ -(int64_t)(val & 1);
    }
    UPB_UNREACHABLE();
  }
  return val;
}

UPB_FORCEINLINE
static const char* fastdecode_varint64(const char* ptr, uint64_t* val) {
  ptr++;
  *val = (uint8_t)ptr[-1];
  if (UPB_UNLIKELY(*val & 0x80)) {
    int i;
    for (i = 0; i < 8; i++) {
      ptr++;
      uint64_t byte = (uint8_t)ptr[-1];
      *val += (byte - 1) << (7 + 7 * i);
      if (UPB_LIKELY((byte & 0x80) == 0)) goto done;
    }
    ptr++;
    uint64_t byte = (uint8_t)ptr[-1];
    if (byte > 1) {
      return NULL;
    }
    *val += (byte - 1) << 63;
  }
done:
  UPB_ASSUME(ptr != NULL);
  return ptr;
}

#define FASTDECODE_UNPACKEDVARINT(d, ptr, msg, table, hasbits, data, tagbytes, \
                                  valbytes, card, zigzag, packed)              \
  uint64_t val;                                                                \
  void* dst;                                                                   \
  fastdecode_arr farr;                                                         \
                                                                               \
  FASTDECODE_CHECKPACKED(tagbytes, card, packed);                              \
                                                                               \
  dst = fastdecode_getfield(d, ptr, msg, &data, &hasbits, &farr, valbytes,     \
                            card);                                             \
  if (card == CARD_r) {                                                        \
    if (UPB_UNLIKELY(!dst)) {                                                  \
      RETURN_GENERIC("need array resize\n");                                   \
    }                                                                          \
  }                                                                            \
                                                                               \
  again:                                                                       \
  if (card == CARD_r) {                                                        \
    dst = fastdecode_resizearr(d, dst, &farr, valbytes);                       \
  }                                                                            \
                                                                               \
  ptr += tagbytes;                                                             \
  ptr = fastdecode_varint64(ptr, &val);                                        \
  if (ptr == NULL) return fastdecode_err(d, kUpb_DecodeStatus_Malformed);      \
  val = fastdecode_munge(val, valbytes, zigzag);                               \
  memcpy(dst, &val, valbytes);                                                 \
                                                                               \
  if (card == CARD_r) {                                                        \
    fastdecode_nextret ret = fastdecode_nextrepeated(                          \
        d, dst, &ptr, &farr, data, tagbytes, valbytes);                        \
    switch (ret.next) {                                                        \
      case FD_NEXT_SAMEFIELD:                                                  \
        dst = ret.dst;                                                         \
        goto again;                                                            \
      case FD_NEXT_OTHERFIELD:                                                 \
        data = ret.tag;                                                        \
        UPB_MUSTTAIL return fastdecode_tagdispatch(UPB_PARSE_ARGS);            \
      case FD_NEXT_ATLIMIT:                                                    \
        return ptr;                                                            \
    }                                                                          \
  }                                                                            \
                                                                               \
  UPB_MUSTTAIL return fastdecode_dispatch(UPB_PARSE_ARGS);

typedef struct {
  uint8_t valbytes;
  bool zigzag;
  void* dst;
  fastdecode_arr farr;
} fastdecode_varintdata;

UPB_FORCEINLINE
static const char* fastdecode_topackedvarint(upb_Decoder* d, const char* ptr,
                                             void* ctx) {
  fastdecode_varintdata* data = ctx;
  void* dst = data->dst;
  uint64_t val;

  while (!decode_isdone(d, &ptr)) {
    dst = fastdecode_resizearr(d, dst, &data->farr, data->valbytes);
    ptr = fastdecode_varint64(ptr, &val);
    if (ptr == NULL) return NULL;
    val = fastdecode_munge(val, data->valbytes, data->zigzag);
    memcpy(dst, &val, data->valbytes);
    dst = (char*)dst + data->valbytes;
  }

  fastdecode_commitarr(dst, &data->farr, data->valbytes);
  return ptr;
}

#define FASTDECODE_PACKEDVARINT(d, ptr, msg, table, hasbits, data, tagbytes, \
                                valbytes, zigzag, unpacked)                  \
  fastdecode_varintdata ctx = {valbytes, zigzag};                            \
                                                                             \
  FASTDECODE_CHECKPACKED(tagbytes, CARD_r, unpacked);                        \
                                                                             \
  ctx.dst = fastdecode_getfield(d, ptr, msg, &data, &hasbits, &ctx.farr,     \
                                valbytes, CARD_r);                           \
  if (UPB_UNLIKELY(!ctx.dst)) {                                              \
    RETURN_GENERIC("need array resize\n");                                   \
  }                                                                          \
                                                                             \
  ptr += tagbytes;                                                           \
  ptr = fastdecode_delimited(d, ptr, &fastdecode_topackedvarint, &ctx);      \
                                                                             \
  if (UPB_UNLIKELY(ptr == NULL)) {                                           \
    return fastdecode_err(d, kUpb_DecodeStatus_Malformed);                   \
  }                                                                          \
                                                                             \
  UPB_MUSTTAIL return fastdecode_dispatch(d, ptr, msg, table, hasbits, 0);

#define FASTDECODE_VARINT(d, ptr, msg, table, hasbits, data, tagbytes,     \
                          valbytes, card, zigzag, unpacked, packed)        \
  if (card == CARD_p) {                                                    \
    FASTDECODE_PACKEDVARINT(d, ptr, msg, table, hasbits, data, tagbytes,   \
                            valbytes, zigzag, unpacked);                   \
  } else {                                                                 \
    FASTDECODE_UNPACKEDVARINT(d, ptr, msg, table, hasbits, data, tagbytes, \
                              valbytes, card, zigzag, packed);             \
  }

#define z_ZZ true
#define b_ZZ false
#define v_ZZ false

/* Generate all combinations:
 * {s,o,r,p} x {b1,v4,z4,v8,z8} x {1bt,2bt} */

#define F(card, type, valbytes, tagbytes)                                      \
  UPB_NOINLINE                                                                 \
  const char* upb_p##card##type##valbytes##_##tagbytes##bt(UPB_PARSE_PARAMS) { \
    FASTDECODE_VARINT(d, ptr, msg, table, hasbits, data, tagbytes, valbytes,   \
                      CARD_##card, type##_ZZ,                                  \
                      upb_pr##type##valbytes##_##tagbytes##bt,                 \
                      upb_pp##type##valbytes##_##tagbytes##bt);                \
  }

#define TYPES(card, tagbytes) \
  F(card, b, 1, tagbytes)     \
  F(card, v, 4, tagbytes)     \
  F(card, v, 8, tagbytes)     \
  F(card, z, 4, tagbytes)     \
  F(card, z, 8, tagbytes)

#define TAGBYTES(card) \
  TYPES(card, 1)       \
  TYPES(card, 2)

TAGBYTES(s)
TAGBYTES(o)
TAGBYTES(r)
TAGBYTES(p)

#undef z_ZZ
#undef b_ZZ
#undef v_ZZ
#undef o_ONEOF
#undef s_ONEOF
#undef r_ONEOF
#undef F
#undef TYPES
#undef TAGBYTES
#undef FASTDECODE_UNPACKEDVARINT
#undef FASTDECODE_PACKEDVARINT
#undef FASTDECODE_VARINT

/* fixed fields ***************************************************************/

#define FASTDECODE_UNPACKEDFIXED(d, ptr, msg, table, hasbits, data, tagbytes, \
                                 valbytes, card, packed)                      \
  void* dst;                                                                  \
  fastdecode_arr farr;                                                        \
                                                                              \
  FASTDECODE_CHECKPACKED(tagbytes, card, packed)                              \
                                                                              \
  dst = fastdecode_getfield(d, ptr, msg, &data, &hasbits, &farr, valbytes,    \
                            card);                                            \
  if (card == CARD_r) {                                                       \
    if (UPB_UNLIKELY(!dst)) {                                                 \
      RETURN_GENERIC("couldn't allocate array in arena\n");                   \
    }                                                                         \
  }                                                                           \
                                                                              \
  again:                                                                      \
  if (card == CARD_r) {                                                       \
    dst = fastdecode_resizearr(d, dst, &farr, valbytes);                      \
  }                                                                           \
                                                                              \
  ptr += tagbytes;                                                            \
  memcpy(dst, ptr, valbytes);                                                 \
  ptr += valbytes;                                                            \
                                                                              \
  if (card == CARD_r) {                                                       \
    fastdecode_nextret ret = fastdecode_nextrepeated(                         \
        d, dst, &ptr, &farr, data, tagbytes, valbytes);                       \
    switch (ret.next) {                                                       \
      case FD_NEXT_SAMEFIELD:                                                 \
        dst = ret.dst;                                                        \
        goto again;                                                           \
      case FD_NEXT_OTHERFIELD:                                                \
        data = ret.tag;                                                       \
        UPB_MUSTTAIL return fastdecode_tagdispatch(UPB_PARSE_ARGS);           \
      case FD_NEXT_ATLIMIT:                                                   \
        return ptr;                                                           \
    }                                                                         \
  }                                                                           \
                                                                              \
  UPB_MUSTTAIL return fastdecode_dispatch(UPB_PARSE_ARGS);

#define FASTDECODE_PACKEDFIXED(d, ptr, msg, table, hasbits, data, tagbytes, \
                               valbytes, unpacked)                          \
  FASTDECODE_CHECKPACKED(tagbytes, CARD_r, unpacked)                        \
                                                                            \
  ptr += tagbytes;                                                          \
  int size = (uint8_t)ptr[0];                                               \
  ptr++;                                                                    \
  if (size & 0x80) {                                                        \
    ptr = fastdecode_longsize(ptr, &size);                                  \
  }                                                                         \
                                                                            \
  if (UPB_UNLIKELY(fastdecode_boundscheck(ptr, size, d->limit_ptr) ||       \
                   (size % valbytes) != 0)) {                               \
    return fastdecode_err(d, kUpb_DecodeStatus_Malformed);                  \
  }                                                                         \
                                                                            \
  upb_Array** arr_p = fastdecode_fieldmem(msg, data);                       \
  upb_Array* arr = *arr_p;                                                  \
  uint8_t elem_size_lg2 = __builtin_ctz(valbytes);                          \
  int elems = size / valbytes;                                              \
                                                                            \
  if (UPB_LIKELY(!arr)) {                                                   \
    *arr_p = arr = _upb_Array_New(&d->arena, elems, elem_size_lg2);         \
    if (!arr) {                                                             \
      return fastdecode_err(d, kUpb_DecodeStatus_Malformed);                \
    }                                                                       \
  } else {                                                                  \
    _upb_Array_Resize(arr, elems, &d->arena);                               \
  }                                                                         \
                                                                            \
  char* dst = _upb_array_ptr(arr);                                          \
  memcpy(dst, ptr, size);                                                   \
  arr->len = elems;                                                         \
                                                                            \
  ptr += size;                                                              \
  UPB_MUSTTAIL return fastdecode_dispatch(UPB_PARSE_ARGS);

#define FASTDECODE_FIXED(d, ptr, msg, table, hasbits, data, tagbytes,     \
                         valbytes, card, unpacked, packed)                \
  if (card == CARD_p) {                                                   \
    FASTDECODE_PACKEDFIXED(d, ptr, msg, table, hasbits, data, tagbytes,   \
                           valbytes, unpacked);                           \
  } else {                                                                \
    FASTDECODE_UNPACKEDFIXED(d, ptr, msg, table, hasbits, data, tagbytes, \
                             valbytes, card, packed);                     \
  }

/* Generate all combinations:
 * {s,o,r,p} x {f4,f8} x {1bt,2bt} */

#define F(card, valbytes, tagbytes)                                         \
  UPB_NOINLINE                                                              \
  const char* upb_p##card##f##valbytes##_##tagbytes##bt(UPB_PARSE_PARAMS) { \
    FASTDECODE_FIXED(d, ptr, msg, table, hasbits, data, tagbytes, valbytes, \
                     CARD_##card, upb_ppf##valbytes##_##tagbytes##bt,       \
                     upb_prf##valbytes##_##tagbytes##bt);                   \
  }

#define TYPES(card, tagbytes) \
  F(card, 4, tagbytes)        \
  F(card, 8, tagbytes)

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
#undef FASTDECODE_UNPACKEDFIXED
#undef FASTDECODE_PACKEDFIXED

/* string fields **************************************************************/

typedef const char* fastdecode_copystr_func(struct upb_Decoder* d,
                                            const char* ptr, upb_Message* msg,
                                            const upb_MiniTable* table,
                                            uint64_t hasbits,
                                            upb_StringView* dst);

UPB_NOINLINE
static const char* fastdecode_verifyutf8(upb_Decoder* d, const char* ptr,
                                         upb_Message* msg, intptr_t table,
                                         uint64_t hasbits, uint64_t data) {
  upb_StringView* dst = (upb_StringView*)data;
  if (!decode_verifyutf8_inl(dst->data, dst->size)) {
    return fastdecode_err(d, kUpb_DecodeStatus_BadUtf8);
  }
  UPB_MUSTTAIL return fastdecode_dispatch(UPB_PARSE_ARGS);
}

#define FASTDECODE_LONGSTRING(d, ptr, msg, table, hasbits, dst, validate_utf8) \
  int size = (uint8_t)ptr[0]; /* Could plumb through hasbits. */               \
  ptr++;                                                                       \
  if (size & 0x80) {                                                           \
    ptr = fastdecode_longsize(ptr, &size);                                     \
  }                                                                            \
                                                                               \
  if (UPB_UNLIKELY(fastdecode_boundscheck(ptr, size, d->limit_ptr))) {         \
    dst->size = 0;                                                             \
    return fastdecode_err(d, kUpb_DecodeStatus_Malformed);                     \
  }                                                                            \
                                                                               \
  if (d->options & kUpb_DecodeOption_AliasString) {                            \
    dst->data = ptr;                                                           \
    dst->size = size;                                                          \
  } else {                                                                     \
    char* data = upb_Arena_Malloc(&d->arena, size);                            \
    if (!data) {                                                               \
      return fastdecode_err(d, kUpb_DecodeStatus_OutOfMemory);                 \
    }                                                                          \
    memcpy(data, ptr, size);                                                   \
    dst->data = data;                                                          \
    dst->size = size;                                                          \
  }                                                                            \
                                                                               \
  ptr += size;                                                                 \
  if (validate_utf8) {                                                         \
    data = (uint64_t)dst;                                                      \
    UPB_MUSTTAIL return fastdecode_verifyutf8(UPB_PARSE_ARGS);                 \
  } else {                                                                     \
    UPB_MUSTTAIL return fastdecode_dispatch(UPB_PARSE_ARGS);                   \
  }

UPB_NOINLINE
static const char* fastdecode_longstring_utf8(struct upb_Decoder* d,
                                              const char* ptr, upb_Message* msg,
                                              intptr_t table, uint64_t hasbits,
                                              uint64_t data) {
  upb_StringView* dst = (upb_StringView*)data;
  FASTDECODE_LONGSTRING(d, ptr, msg, table, hasbits, dst, true);
}

UPB_NOINLINE
static const char* fastdecode_longstring_noutf8(
    struct upb_Decoder* d, const char* ptr, upb_Message* msg, intptr_t table,
    uint64_t hasbits, uint64_t data) {
  upb_StringView* dst = (upb_StringView*)data;
  FASTDECODE_LONGSTRING(d, ptr, msg, table, hasbits, dst, false);
}

UPB_FORCEINLINE
static void fastdecode_docopy(upb_Decoder* d, const char* ptr, uint32_t size,
                              int copy, char* data, upb_StringView* dst) {
  d->arena.head.ptr += copy;
  dst->data = data;
  UPB_UNPOISON_MEMORY_REGION(data, copy);
  memcpy(data, ptr, copy);
  UPB_POISON_MEMORY_REGION(data + size, copy - size);
}

#define FASTDECODE_COPYSTRING(d, ptr, msg, table, hasbits, data, tagbytes,    \
                              card, validate_utf8)                            \
  upb_StringView* dst;                                                        \
  fastdecode_arr farr;                                                        \
  int64_t size;                                                               \
  size_t arena_has;                                                           \
  size_t common_has;                                                          \
  char* buf;                                                                  \
                                                                              \
  UPB_ASSERT((d->options & kUpb_DecodeOption_AliasString) == 0);              \
  UPB_ASSERT(fastdecode_checktag(data, tagbytes));                            \
                                                                              \
  dst = fastdecode_getfield(d, ptr, msg, &data, &hasbits, &farr,              \
                            sizeof(upb_StringView), card);                    \
                                                                              \
  again:                                                                      \
  if (card == CARD_r) {                                                       \
    dst = fastdecode_resizearr(d, dst, &farr, sizeof(upb_StringView));        \
  }                                                                           \
                                                                              \
  size = (uint8_t)ptr[tagbytes];                                              \
  ptr += tagbytes + 1;                                                        \
  dst->size = size;                                                           \
                                                                              \
  buf = d->arena.head.ptr;                                                    \
  arena_has = _upb_ArenaHas(&d->arena);                                       \
  common_has = UPB_MIN(arena_has, (d->end - ptr) + 16);                       \
                                                                              \
  if (UPB_LIKELY(size <= 15 - tagbytes)) {                                    \
    if (arena_has < 16) goto longstr;                                         \
    d->arena.head.ptr += 16;                                                  \
    memcpy(buf, ptr - tagbytes - 1, 16);                                      \
    dst->data = buf + tagbytes + 1;                                           \
  } else if (UPB_LIKELY(size <= 32)) {                                        \
    if (UPB_UNLIKELY(common_has < 32)) goto longstr;                          \
    fastdecode_docopy(d, ptr, size, 32, buf, dst);                            \
  } else if (UPB_LIKELY(size <= 64)) {                                        \
    if (UPB_UNLIKELY(common_has < 64)) goto longstr;                          \
    fastdecode_docopy(d, ptr, size, 64, buf, dst);                            \
  } else if (UPB_LIKELY(size < 128)) {                                        \
    if (UPB_UNLIKELY(common_has < 128)) goto longstr;                         \
    fastdecode_docopy(d, ptr, size, 128, buf, dst);                           \
  } else {                                                                    \
    goto longstr;                                                             \
  }                                                                           \
                                                                              \
  ptr += size;                                                                \
                                                                              \
  if (card == CARD_r) {                                                       \
    if (validate_utf8 && !decode_verifyutf8_inl(dst->data, dst->size)) {      \
      return fastdecode_err(d, kUpb_DecodeStatus_BadUtf8);                    \
    }                                                                         \
    fastdecode_nextret ret = fastdecode_nextrepeated(                         \
        d, dst, &ptr, &farr, data, tagbytes, sizeof(upb_StringView));         \
    switch (ret.next) {                                                       \
      case FD_NEXT_SAMEFIELD:                                                 \
        dst = ret.dst;                                                        \
        goto again;                                                           \
      case FD_NEXT_OTHERFIELD:                                                \
        data = ret.tag;                                                       \
        UPB_MUSTTAIL return fastdecode_tagdispatch(UPB_PARSE_ARGS);           \
      case FD_NEXT_ATLIMIT:                                                   \
        return ptr;                                                           \
    }                                                                         \
  }                                                                           \
                                                                              \
  if (card != CARD_r && validate_utf8) {                                      \
    data = (uint64_t)dst;                                                     \
    UPB_MUSTTAIL return fastdecode_verifyutf8(UPB_PARSE_ARGS);                \
  }                                                                           \
                                                                              \
  UPB_MUSTTAIL return fastdecode_dispatch(UPB_PARSE_ARGS);                    \
                                                                              \
  longstr:                                                                    \
  if (card == CARD_r) {                                                       \
    fastdecode_commitarr(dst + 1, &farr, sizeof(upb_StringView));             \
  }                                                                           \
  ptr--;                                                                      \
  if (validate_utf8) {                                                        \
    UPB_MUSTTAIL return fastdecode_longstring_utf8(d, ptr, msg, table,        \
                                                   hasbits, (uint64_t)dst);   \
  } else {                                                                    \
    UPB_MUSTTAIL return fastdecode_longstring_noutf8(d, ptr, msg, table,      \
                                                     hasbits, (uint64_t)dst); \
  }

#define FASTDECODE_STRING(d, ptr, msg, table, hasbits, data, tagbytes, card,   \
                          copyfunc, validate_utf8)                             \
  upb_StringView* dst;                                                         \
  fastdecode_arr farr;                                                         \
  int64_t size;                                                                \
                                                                               \
  if (UPB_UNLIKELY(!fastdecode_checktag(data, tagbytes))) {                    \
    RETURN_GENERIC("string field tag mismatch\n");                             \
  }                                                                            \
                                                                               \
  if (UPB_UNLIKELY((d->options & kUpb_DecodeOption_AliasString) == 0)) {       \
    UPB_MUSTTAIL return copyfunc(UPB_PARSE_ARGS);                              \
  }                                                                            \
                                                                               \
  dst = fastdecode_getfield(d, ptr, msg, &data, &hasbits, &farr,               \
                            sizeof(upb_StringView), card);                     \
                                                                               \
  again:                                                                       \
  if (card == CARD_r) {                                                        \
    dst = fastdecode_resizearr(d, dst, &farr, sizeof(upb_StringView));         \
  }                                                                            \
                                                                               \
  size = (int8_t)ptr[tagbytes];                                                \
  ptr += tagbytes + 1;                                                         \
  dst->data = ptr;                                                             \
  dst->size = size;                                                            \
                                                                               \
  if (UPB_UNLIKELY(fastdecode_boundscheck(ptr, size, d->end))) {               \
    ptr--;                                                                     \
    if (validate_utf8) {                                                       \
      return fastdecode_longstring_utf8(d, ptr, msg, table, hasbits,           \
                                        (uint64_t)dst);                        \
    } else {                                                                   \
      return fastdecode_longstring_noutf8(d, ptr, msg, table, hasbits,         \
                                          (uint64_t)dst);                      \
    }                                                                          \
  }                                                                            \
                                                                               \
  ptr += size;                                                                 \
                                                                               \
  if (card == CARD_r) {                                                        \
    if (validate_utf8 && !decode_verifyutf8_inl(dst->data, dst->size)) {       \
      return fastdecode_err(d, kUpb_DecodeStatus_BadUtf8);                     \
    }                                                                          \
    fastdecode_nextret ret = fastdecode_nextrepeated(                          \
        d, dst, &ptr, &farr, data, tagbytes, sizeof(upb_StringView));          \
    switch (ret.next) {                                                        \
      case FD_NEXT_SAMEFIELD:                                                  \
        dst = ret.dst;                                                         \
        if (UPB_UNLIKELY((d->options & kUpb_DecodeOption_AliasString) == 0)) { \
          /* Buffer flipped and we can't alias any more. Bounce to */          \
          /* copyfunc(), but via dispatch since we need to reload table */     \
          /* data also. */                                                     \
          fastdecode_commitarr(dst, &farr, sizeof(upb_StringView));            \
          data = ret.tag;                                                      \
          UPB_MUSTTAIL return fastdecode_tagdispatch(UPB_PARSE_ARGS);          \
        }                                                                      \
        goto again;                                                            \
      case FD_NEXT_OTHERFIELD:                                                 \
        data = ret.tag;                                                        \
        UPB_MUSTTAIL return fastdecode_tagdispatch(UPB_PARSE_ARGS);            \
      case FD_NEXT_ATLIMIT:                                                    \
        return ptr;                                                            \
    }                                                                          \
  }                                                                            \
                                                                               \
  if (card != CARD_r && validate_utf8) {                                       \
    data = (uint64_t)dst;                                                      \
    UPB_MUSTTAIL return fastdecode_verifyutf8(UPB_PARSE_ARGS);                 \
  }                                                                            \
                                                                               \
  UPB_MUSTTAIL return fastdecode_dispatch(UPB_PARSE_ARGS);

/* Generate all combinations:
 * {p,c} x {s,o,r} x {s, b} x {1bt,2bt} */

#define s_VALIDATE true
#define b_VALIDATE false

#define F(card, tagbytes, type)                                        \
  UPB_NOINLINE                                                         \
  const char* upb_c##card##type##_##tagbytes##bt(UPB_PARSE_PARAMS) {   \
    FASTDECODE_COPYSTRING(d, ptr, msg, table, hasbits, data, tagbytes, \
                          CARD_##card, type##_VALIDATE);               \
  }                                                                    \
  const char* upb_p##card##type##_##tagbytes##bt(UPB_PARSE_PARAMS) {   \
    FASTDECODE_STRING(d, ptr, msg, table, hasbits, data, tagbytes,     \
                      CARD_##card, upb_c##card##type##_##tagbytes##bt, \
                      type##_VALIDATE);                                \
  }

#define UTF8(card, tagbytes) \
  F(card, tagbytes, s)       \
  F(card, tagbytes, b)

#define TAGBYTES(card) \
  UTF8(card, 1)        \
  UTF8(card, 2)

TAGBYTES(s)
TAGBYTES(o)
TAGBYTES(r)

#undef s_VALIDATE
#undef b_VALIDATE
#undef F
#undef TAGBYTES
#undef FASTDECODE_LONGSTRING
#undef FASTDECODE_COPYSTRING
#undef FASTDECODE_STRING

/* message fields *************************************************************/

UPB_INLINE
upb_Message* decode_newmsg_ceil(upb_Decoder* d, const upb_MiniTable* l,
                                int msg_ceil_bytes) {
  size_t size = l->size + sizeof(upb_Message_Internal);
  char* msg_data;
  if (UPB_LIKELY(msg_ceil_bytes > 0 &&
                 _upb_ArenaHas(&d->arena) >= msg_ceil_bytes)) {
    UPB_ASSERT(size <= (size_t)msg_ceil_bytes);
    msg_data = d->arena.head.ptr;
    d->arena.head.ptr += size;
    UPB_UNPOISON_MEMORY_REGION(msg_data, msg_ceil_bytes);
    memset(msg_data, 0, msg_ceil_bytes);
    UPB_POISON_MEMORY_REGION(msg_data + size, msg_ceil_bytes - size);
  } else {
    msg_data = (char*)upb_Arena_Malloc(&d->arena, size);
    memset(msg_data, 0, size);
  }
  return msg_data + sizeof(upb_Message_Internal);
}

typedef struct {
  intptr_t table;
  upb_Message* msg;
} fastdecode_submsgdata;

UPB_FORCEINLINE
static const char* fastdecode_tosubmsg(upb_Decoder* d, const char* ptr,
                                       void* ctx) {
  fastdecode_submsgdata* submsg = ctx;
  ptr = fastdecode_dispatch(d, ptr, submsg->msg, submsg->table, 0, 0);
  UPB_ASSUME(ptr != NULL);
  return ptr;
}

#define FASTDECODE_SUBMSG(d, ptr, msg, table, hasbits, data, tagbytes,    \
                          msg_ceil_bytes, card)                           \
                                                                          \
  if (UPB_UNLIKELY(!fastdecode_checktag(data, tagbytes))) {               \
    RETURN_GENERIC("submessage field tag mismatch\n");                    \
  }                                                                       \
                                                                          \
  if (--d->depth == 0) {                                                  \
    return fastdecode_err(d, kUpb_DecodeStatus_MaxDepthExceeded);         \
  }                                                                       \
                                                                          \
  upb_Message** dst;                                                      \
  uint32_t submsg_idx = (data >> 16) & 0xff;                              \
  const upb_MiniTable* tablep = decode_totablep(table);                   \
  const upb_MiniTable* subtablep = tablep->subs[submsg_idx].submsg;       \
  fastdecode_submsgdata submsg = {decode_totable(subtablep)};             \
  fastdecode_arr farr;                                                    \
                                                                          \
  if (subtablep->table_mask == (uint8_t)-1) {                             \
    RETURN_GENERIC("submessage doesn't have fast tables.");               \
  }                                                                       \
                                                                          \
  dst = fastdecode_getfield(d, ptr, msg, &data, &hasbits, &farr,          \
                            sizeof(upb_Message*), card);                  \
                                                                          \
  if (card == CARD_s) {                                                   \
    *(uint32_t*)msg |= hasbits;                                           \
    hasbits = 0;                                                          \
  }                                                                       \
                                                                          \
  again:                                                                  \
  if (card == CARD_r) {                                                   \
    dst = fastdecode_resizearr(d, dst, &farr, sizeof(upb_Message*));      \
  }                                                                       \
                                                                          \
  submsg.msg = *dst;                                                      \
                                                                          \
  if (card == CARD_r || UPB_LIKELY(!submsg.msg)) {                        \
    *dst = submsg.msg = decode_newmsg_ceil(d, subtablep, msg_ceil_bytes); \
  }                                                                       \
                                                                          \
  ptr += tagbytes;                                                        \
  ptr = fastdecode_delimited(d, ptr, fastdecode_tosubmsg, &submsg);       \
                                                                          \
  if (UPB_UNLIKELY(ptr == NULL || d->end_group != DECODE_NOGROUP)) {      \
    return fastdecode_err(d, kUpb_DecodeStatus_Malformed);                \
  }                                                                       \
                                                                          \
  if (card == CARD_r) {                                                   \
    fastdecode_nextret ret = fastdecode_nextrepeated(                     \
        d, dst, &ptr, &farr, data, tagbytes, sizeof(upb_Message*));       \
    switch (ret.next) {                                                   \
      case FD_NEXT_SAMEFIELD:                                             \
        dst = ret.dst;                                                    \
        goto again;                                                       \
      case FD_NEXT_OTHERFIELD:                                            \
        d->depth++;                                                       \
        data = ret.tag;                                                   \
        UPB_MUSTTAIL return fastdecode_tagdispatch(UPB_PARSE_ARGS);       \
      case FD_NEXT_ATLIMIT:                                               \
        d->depth++;                                                       \
        return ptr;                                                       \
    }                                                                     \
  }                                                                       \
                                                                          \
  d->depth++;                                                             \
  UPB_MUSTTAIL return fastdecode_dispatch(UPB_PARSE_ARGS);

#define F(card, tagbytes, size_ceil, ceil_arg)                               \
  const char* upb_p##card##m_##tagbytes##bt_max##size_ceil##b(               \
      UPB_PARSE_PARAMS) {                                                    \
    FASTDECODE_SUBMSG(d, ptr, msg, table, hasbits, data, tagbytes, ceil_arg, \
                      CARD_##card);                                          \
  }

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
#undef FASTDECODE_SUBMSG

#endif /* UPB_FASTTABLE */

/** upb/json_decode.c ************************************************************/

#include <errno.h>
#include <float.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>


/* Special header, must be included last. */

typedef struct {
  const char *ptr, *end;
  upb_Arena* arena; /* TODO: should we have a tmp arena for tmp data? */
  const upb_DefPool* symtab;
  int depth;
  upb_Status* status;
  jmp_buf err;
  int line;
  const char* line_begin;
  bool is_first;
  int options;
  const upb_FieldDef* debug_field;
} jsondec;

enum { JD_OBJECT, JD_ARRAY, JD_STRING, JD_NUMBER, JD_TRUE, JD_FALSE, JD_NULL };

/* Forward declarations of mutually-recursive functions. */
static void jsondec_wellknown(jsondec* d, upb_Message* msg,
                              const upb_MessageDef* m);
static upb_MessageValue jsondec_value(jsondec* d, const upb_FieldDef* f);
static void jsondec_wellknownvalue(jsondec* d, upb_Message* msg,
                                   const upb_MessageDef* m);
static void jsondec_object(jsondec* d, upb_Message* msg,
                           const upb_MessageDef* m);

static bool jsondec_streql(upb_StringView str, const char* lit) {
  return str.size == strlen(lit) && memcmp(str.data, lit, str.size) == 0;
}

static bool jsondec_isnullvalue(const upb_FieldDef* f) {
  return upb_FieldDef_CType(f) == kUpb_CType_Enum &&
         strcmp(upb_EnumDef_FullName(upb_FieldDef_EnumSubDef(f)),
                "google.protobuf.NullValue") == 0;
}

static bool jsondec_isvalue(const upb_FieldDef* f) {
  return (upb_FieldDef_CType(f) == kUpb_CType_Message &&
          upb_MessageDef_WellKnownType(upb_FieldDef_MessageSubDef(f)) ==
              kUpb_WellKnown_Value) ||
         jsondec_isnullvalue(f);
}

UPB_NORETURN static void jsondec_err(jsondec* d, const char* msg) {
  upb_Status_SetErrorFormat(d->status, "Error parsing JSON @%d:%d: %s", d->line,
                            (int)(d->ptr - d->line_begin), msg);
  UPB_LONGJMP(d->err, 1);
}

UPB_PRINTF(2, 3)
UPB_NORETURN static void jsondec_errf(jsondec* d, const char* fmt, ...) {
  va_list argp;
  upb_Status_SetErrorFormat(d->status, "Error parsing JSON @%d:%d: ", d->line,
                            (int)(d->ptr - d->line_begin));
  va_start(argp, fmt);
  upb_Status_VAppendErrorFormat(d->status, fmt, argp);
  va_end(argp);
  UPB_LONGJMP(d->err, 1);
}

static void jsondec_skipws(jsondec* d) {
  while (d->ptr != d->end) {
    switch (*d->ptr) {
      case '\n':
        d->line++;
        d->line_begin = d->ptr;
        /* Fallthrough. */
      case '\r':
      case '\t':
      case ' ':
        d->ptr++;
        break;
      default:
        return;
    }
  }
  jsondec_err(d, "Unexpected EOF");
}

static bool jsondec_tryparsech(jsondec* d, char ch) {
  if (d->ptr == d->end || *d->ptr != ch) return false;
  d->ptr++;
  return true;
}

static void jsondec_parselit(jsondec* d, const char* lit) {
  size_t avail = d->end - d->ptr;
  size_t len = strlen(lit);
  if (avail < len || memcmp(d->ptr, lit, len) != 0) {
    jsondec_errf(d, "Expected: '%s'", lit);
  }
  d->ptr += len;
}

static void jsondec_wsch(jsondec* d, char ch) {
  jsondec_skipws(d);
  if (!jsondec_tryparsech(d, ch)) {
    jsondec_errf(d, "Expected: '%c'", ch);
  }
}

static void jsondec_true(jsondec* d) { jsondec_parselit(d, "true"); }
static void jsondec_false(jsondec* d) { jsondec_parselit(d, "false"); }
static void jsondec_null(jsondec* d) { jsondec_parselit(d, "null"); }

static void jsondec_entrysep(jsondec* d) {
  jsondec_skipws(d);
  jsondec_parselit(d, ":");
}

static int jsondec_rawpeek(jsondec* d) {
  switch (*d->ptr) {
    case '{':
      return JD_OBJECT;
    case '[':
      return JD_ARRAY;
    case '"':
      return JD_STRING;
    case '-':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      return JD_NUMBER;
    case 't':
      return JD_TRUE;
    case 'f':
      return JD_FALSE;
    case 'n':
      return JD_NULL;
    default:
      jsondec_errf(d, "Unexpected character: '%c'", *d->ptr);
  }
}

/* JSON object/array **********************************************************/

/* These are used like so:
 *
 * jsondec_objstart(d);
 * while (jsondec_objnext(d)) {
 *   ...
 * }
 * jsondec_objend(d) */

static int jsondec_peek(jsondec* d) {
  jsondec_skipws(d);
  return jsondec_rawpeek(d);
}

static void jsondec_push(jsondec* d) {
  if (--d->depth < 0) {
    jsondec_err(d, "Recursion limit exceeded");
  }
  d->is_first = true;
}

static bool jsondec_seqnext(jsondec* d, char end_ch) {
  bool is_first = d->is_first;
  d->is_first = false;
  jsondec_skipws(d);
  if (*d->ptr == end_ch) return false;
  if (!is_first) jsondec_parselit(d, ",");
  return true;
}

static void jsondec_arrstart(jsondec* d) {
  jsondec_push(d);
  jsondec_wsch(d, '[');
}

static void jsondec_arrend(jsondec* d) {
  d->depth++;
  jsondec_wsch(d, ']');
}

static bool jsondec_arrnext(jsondec* d) { return jsondec_seqnext(d, ']'); }

static void jsondec_objstart(jsondec* d) {
  jsondec_push(d);
  jsondec_wsch(d, '{');
}

static void jsondec_objend(jsondec* d) {
  d->depth++;
  jsondec_wsch(d, '}');
}

static bool jsondec_objnext(jsondec* d) {
  if (!jsondec_seqnext(d, '}')) return false;
  if (jsondec_peek(d) != JD_STRING) {
    jsondec_err(d, "Object must start with string");
  }
  return true;
}

/* JSON number ****************************************************************/

static bool jsondec_tryskipdigits(jsondec* d) {
  const char* start = d->ptr;

  while (d->ptr < d->end) {
    if (*d->ptr < '0' || *d->ptr > '9') {
      break;
    }
    d->ptr++;
  }

  return d->ptr != start;
}

static void jsondec_skipdigits(jsondec* d) {
  if (!jsondec_tryskipdigits(d)) {
    jsondec_err(d, "Expected one or more digits");
  }
}

static double jsondec_number(jsondec* d) {
  const char* start = d->ptr;

  assert(jsondec_rawpeek(d) == JD_NUMBER);

  /* Skip over the syntax of a number, as specified by JSON. */
  if (*d->ptr == '-') d->ptr++;

  if (jsondec_tryparsech(d, '0')) {
    if (jsondec_tryskipdigits(d)) {
      jsondec_err(d, "number cannot have leading zero");
    }
  } else {
    jsondec_skipdigits(d);
  }

  if (d->ptr == d->end) goto parse;
  if (jsondec_tryparsech(d, '.')) {
    jsondec_skipdigits(d);
  }
  if (d->ptr == d->end) goto parse;

  if (*d->ptr == 'e' || *d->ptr == 'E') {
    d->ptr++;
    if (d->ptr == d->end) {
      jsondec_err(d, "Unexpected EOF in number");
    }
    if (*d->ptr == '+' || *d->ptr == '-') {
      d->ptr++;
    }
    jsondec_skipdigits(d);
  }

parse:
  /* Having verified the syntax of a JSON number, use strtod() to parse
   * (strtod() accepts a superset of JSON syntax). */
  errno = 0;
  {
    char* end;
    double val = strtod(start, &end);
    assert(end == d->ptr);

    /* Currently the min/max-val conformance tests fail if we check this.  Does
     * this mean the conformance tests are wrong or strtod() is wrong, or
     * something else?  Investigate further. */
    /*
    if (errno == ERANGE) {
      jsondec_err(d, "Number out of range");
    }
    */

    if (val > DBL_MAX || val < -DBL_MAX) {
      jsondec_err(d, "Number out of range");
    }

    return val;
  }
}

/* JSON string ****************************************************************/

static char jsondec_escape(jsondec* d) {
  switch (*d->ptr++) {
    case '"':
      return '\"';
    case '\\':
      return '\\';
    case '/':
      return '/';
    case 'b':
      return '\b';
    case 'f':
      return '\f';
    case 'n':
      return '\n';
    case 'r':
      return '\r';
    case 't':
      return '\t';
    default:
      jsondec_err(d, "Invalid escape char");
  }
}

static uint32_t jsondec_codepoint(jsondec* d) {
  uint32_t cp = 0;
  const char* end;

  if (d->end - d->ptr < 4) {
    jsondec_err(d, "EOF inside string");
  }

  end = d->ptr + 4;
  while (d->ptr < end) {
    char ch = *d->ptr++;
    if (ch >= '0' && ch <= '9') {
      ch -= '0';
    } else if (ch >= 'a' && ch <= 'f') {
      ch = ch - 'a' + 10;
    } else if (ch >= 'A' && ch <= 'F') {
      ch = ch - 'A' + 10;
    } else {
      jsondec_err(d, "Invalid hex digit");
    }
    cp = (cp << 4) | ch;
  }

  return cp;
}

/* Parses a \uXXXX unicode escape (possibly a surrogate pair). */
static size_t jsondec_unicode(jsondec* d, char* out) {
  uint32_t cp = jsondec_codepoint(d);
  if (cp >= 0xd800 && cp <= 0xdbff) {
    /* Surrogate pair: two 16-bit codepoints become a 32-bit codepoint. */
    uint32_t high = cp;
    uint32_t low;
    jsondec_parselit(d, "\\u");
    low = jsondec_codepoint(d);
    if (low < 0xdc00 || low > 0xdfff) {
      jsondec_err(d, "Invalid low surrogate");
    }
    cp = (high & 0x3ff) << 10;
    cp |= (low & 0x3ff);
    cp += 0x10000;
  } else if (cp >= 0xdc00 && cp <= 0xdfff) {
    jsondec_err(d, "Unpaired low surrogate");
  }

  /* Write to UTF-8 */
  if (cp <= 0x7f) {
    out[0] = cp;
    return 1;
  } else if (cp <= 0x07FF) {
    out[0] = ((cp >> 6) & 0x1F) | 0xC0;
    out[1] = ((cp >> 0) & 0x3F) | 0x80;
    return 2;
  } else if (cp <= 0xFFFF) {
    out[0] = ((cp >> 12) & 0x0F) | 0xE0;
    out[1] = ((cp >> 6) & 0x3F) | 0x80;
    out[2] = ((cp >> 0) & 0x3F) | 0x80;
    return 3;
  } else if (cp < 0x10FFFF) {
    out[0] = ((cp >> 18) & 0x07) | 0xF0;
    out[1] = ((cp >> 12) & 0x3f) | 0x80;
    out[2] = ((cp >> 6) & 0x3f) | 0x80;
    out[3] = ((cp >> 0) & 0x3f) | 0x80;
    return 4;
  } else {
    jsondec_err(d, "Invalid codepoint");
  }
}

static void jsondec_resize(jsondec* d, char** buf, char** end, char** buf_end) {
  size_t oldsize = *buf_end - *buf;
  size_t len = *end - *buf;
  size_t size = UPB_MAX(8, 2 * oldsize);

  *buf = upb_Arena_Realloc(d->arena, *buf, len, size);
  if (!*buf) jsondec_err(d, "Out of memory");

  *end = *buf + len;
  *buf_end = *buf + size;
}

static upb_StringView jsondec_string(jsondec* d) {
  char* buf = NULL;
  char* end = NULL;
  char* buf_end = NULL;

  jsondec_skipws(d);

  if (*d->ptr++ != '"') {
    jsondec_err(d, "Expected string");
  }

  while (d->ptr < d->end) {
    char ch = *d->ptr++;

    if (end == buf_end) {
      jsondec_resize(d, &buf, &end, &buf_end);
    }

    switch (ch) {
      case '"': {
        upb_StringView ret;
        ret.data = buf;
        ret.size = end - buf;
        *end = '\0'; /* Needed for possible strtod(). */
        return ret;
      }
      case '\\':
        if (d->ptr == d->end) goto eof;
        if (*d->ptr == 'u') {
          d->ptr++;
          if (buf_end - end < 4) {
            /* Allow space for maximum-sized code point (4 bytes). */
            jsondec_resize(d, &buf, &end, &buf_end);
          }
          end += jsondec_unicode(d, end);
        } else {
          *end++ = jsondec_escape(d);
        }
        break;
      default:
        if ((unsigned char)*d->ptr < 0x20) {
          jsondec_err(d, "Invalid char in JSON string");
        }
        *end++ = ch;
        break;
    }
  }

eof:
  jsondec_err(d, "EOF inside string");
}

static void jsondec_skipval(jsondec* d) {
  switch (jsondec_peek(d)) {
    case JD_OBJECT:
      jsondec_objstart(d);
      while (jsondec_objnext(d)) {
        jsondec_string(d);
        jsondec_entrysep(d);
        jsondec_skipval(d);
      }
      jsondec_objend(d);
      break;
    case JD_ARRAY:
      jsondec_arrstart(d);
      while (jsondec_arrnext(d)) {
        jsondec_skipval(d);
      }
      jsondec_arrend(d);
      break;
    case JD_TRUE:
      jsondec_true(d);
      break;
    case JD_FALSE:
      jsondec_false(d);
      break;
    case JD_NULL:
      jsondec_null(d);
      break;
    case JD_STRING:
      jsondec_string(d);
      break;
    case JD_NUMBER:
      jsondec_number(d);
      break;
  }
}

/* Base64 decoding for bytes fields. ******************************************/

static unsigned int jsondec_base64_tablelookup(const char ch) {
  /* Table includes the normal base64 chars plus the URL-safe variant. */
  const signed char table[256] = {
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       62 /*+*/, -1,       62 /*-*/, -1,       63 /*/ */, 52 /*0*/,
      53 /*1*/, 54 /*2*/, 55 /*3*/, 56 /*4*/, 57 /*5*/, 58 /*6*/,  59 /*7*/,
      60 /*8*/, 61 /*9*/, -1,       -1,       -1,       -1,        -1,
      -1,       -1,       0 /*A*/,  1 /*B*/,  2 /*C*/,  3 /*D*/,   4 /*E*/,
      5 /*F*/,  6 /*G*/,  07 /*H*/, 8 /*I*/,  9 /*J*/,  10 /*K*/,  11 /*L*/,
      12 /*M*/, 13 /*N*/, 14 /*O*/, 15 /*P*/, 16 /*Q*/, 17 /*R*/,  18 /*S*/,
      19 /*T*/, 20 /*U*/, 21 /*V*/, 22 /*W*/, 23 /*X*/, 24 /*Y*/,  25 /*Z*/,
      -1,       -1,       -1,       -1,       63 /*_*/, -1,        26 /*a*/,
      27 /*b*/, 28 /*c*/, 29 /*d*/, 30 /*e*/, 31 /*f*/, 32 /*g*/,  33 /*h*/,
      34 /*i*/, 35 /*j*/, 36 /*k*/, 37 /*l*/, 38 /*m*/, 39 /*n*/,  40 /*o*/,
      41 /*p*/, 42 /*q*/, 43 /*r*/, 44 /*s*/, 45 /*t*/, 46 /*u*/,  47 /*v*/,
      48 /*w*/, 49 /*x*/, 50 /*y*/, 51 /*z*/, -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1,       -1,       -1,        -1,
      -1,       -1,       -1,       -1};

  /* Sign-extend return value so high bit will be set on any unexpected char. */
  return table[(unsigned)ch];
}

static char* jsondec_partialbase64(jsondec* d, const char* ptr, const char* end,
                                   char* out) {
  int32_t val = -1;

  switch (end - ptr) {
    case 2:
      val = jsondec_base64_tablelookup(ptr[0]) << 18 |
            jsondec_base64_tablelookup(ptr[1]) << 12;
      out[0] = val >> 16;
      out += 1;
      break;
    case 3:
      val = jsondec_base64_tablelookup(ptr[0]) << 18 |
            jsondec_base64_tablelookup(ptr[1]) << 12 |
            jsondec_base64_tablelookup(ptr[2]) << 6;
      out[0] = val >> 16;
      out[1] = (val >> 8) & 0xff;
      out += 2;
      break;
  }

  if (val < 0) {
    jsondec_err(d, "Corrupt base64");
  }

  return out;
}

static size_t jsondec_base64(jsondec* d, upb_StringView str) {
  /* We decode in place. This is safe because this is a new buffer (not
   * aliasing the input) and because base64 decoding shrinks 4 bytes into 3. */
  char* out = (char*)str.data;
  const char* ptr = str.data;
  const char* end = ptr + str.size;
  const char* end4 = ptr + (str.size & -4); /* Round down to multiple of 4. */

  for (; ptr < end4; ptr += 4, out += 3) {
    int val = jsondec_base64_tablelookup(ptr[0]) << 18 |
              jsondec_base64_tablelookup(ptr[1]) << 12 |
              jsondec_base64_tablelookup(ptr[2]) << 6 |
              jsondec_base64_tablelookup(ptr[3]) << 0;

    if (val < 0) {
      /* Junk chars or padding. Remove trailing padding, if any. */
      if (end - ptr == 4 && ptr[3] == '=') {
        if (ptr[2] == '=') {
          end -= 2;
        } else {
          end -= 1;
        }
      }
      break;
    }

    out[0] = val >> 16;
    out[1] = (val >> 8) & 0xff;
    out[2] = val & 0xff;
  }

  if (ptr < end) {
    /* Process remaining chars. We do not require padding. */
    out = jsondec_partialbase64(d, ptr, end, out);
  }

  return out - str.data;
}

/* Low-level integer parsing **************************************************/

/* We use these hand-written routines instead of strto[u]l() because the "long
 * long" variants aren't in c89. Also our version allows setting a ptr limit. */

static const char* jsondec_buftouint64(jsondec* d, const char* ptr,
                                       const char* end, uint64_t* val) {
  uint64_t u64 = 0;
  while (ptr < end) {
    unsigned ch = *ptr - '0';
    if (ch >= 10) break;
    if (u64 > UINT64_MAX / 10 || u64 * 10 > UINT64_MAX - ch) {
      jsondec_err(d, "Integer overflow");
    }
    u64 *= 10;
    u64 += ch;
    ptr++;
  }

  *val = u64;
  return ptr;
}

static const char* jsondec_buftoint64(jsondec* d, const char* ptr,
                                      const char* end, int64_t* val) {
  bool neg = false;
  uint64_t u64;

  if (ptr != end && *ptr == '-') {
    ptr++;
    neg = true;
  }

  ptr = jsondec_buftouint64(d, ptr, end, &u64);
  if (u64 > (uint64_t)INT64_MAX + neg) {
    jsondec_err(d, "Integer overflow");
  }

  *val = neg ? -u64 : u64;
  return ptr;
}

static uint64_t jsondec_strtouint64(jsondec* d, upb_StringView str) {
  const char* end = str.data + str.size;
  uint64_t ret;
  if (jsondec_buftouint64(d, str.data, end, &ret) != end) {
    jsondec_err(d, "Non-number characters in quoted integer");
  }
  return ret;
}

static int64_t jsondec_strtoint64(jsondec* d, upb_StringView str) {
  const char* end = str.data + str.size;
  int64_t ret;
  if (jsondec_buftoint64(d, str.data, end, &ret) != end) {
    jsondec_err(d, "Non-number characters in quoted integer");
  }
  return ret;
}

/* Primitive value types ******************************************************/

/* Parse INT32 or INT64 value. */
static upb_MessageValue jsondec_int(jsondec* d, const upb_FieldDef* f) {
  upb_MessageValue val;

  switch (jsondec_peek(d)) {
    case JD_NUMBER: {
      double dbl = jsondec_number(d);
      if (dbl > 9223372036854774784.0 || dbl < -9223372036854775808.0) {
        jsondec_err(d, "JSON number is out of range.");
      }
      val.int64_val = dbl; /* must be guarded, overflow here is UB */
      if (val.int64_val != dbl) {
        jsondec_errf(d, "JSON number was not integral (%f != %" PRId64 ")", dbl,
                     val.int64_val);
      }
      break;
    }
    case JD_STRING: {
      upb_StringView str = jsondec_string(d);
      val.int64_val = jsondec_strtoint64(d, str);
      break;
    }
    default:
      jsondec_err(d, "Expected number or string");
  }

  if (upb_FieldDef_CType(f) == kUpb_CType_Int32 ||
      upb_FieldDef_CType(f) == kUpb_CType_Enum) {
    if (val.int64_val > INT32_MAX || val.int64_val < INT32_MIN) {
      jsondec_err(d, "Integer out of range.");
    }
    val.int32_val = (int32_t)val.int64_val;
  }

  return val;
}

/* Parse UINT32 or UINT64 value. */
static upb_MessageValue jsondec_uint(jsondec* d, const upb_FieldDef* f) {
  upb_MessageValue val = {0};

  switch (jsondec_peek(d)) {
    case JD_NUMBER: {
      double dbl = jsondec_number(d);
      if (dbl > 18446744073709549568.0 || dbl < 0) {
        jsondec_err(d, "JSON number is out of range.");
      }
      val.uint64_val = dbl; /* must be guarded, overflow here is UB */
      if (val.uint64_val != dbl) {
        jsondec_errf(d, "JSON number was not integral (%f != %" PRIu64 ")", dbl,
                     val.uint64_val);
      }
      break;
    }
    case JD_STRING: {
      upb_StringView str = jsondec_string(d);
      val.uint64_val = jsondec_strtouint64(d, str);
      break;
    }
    default:
      jsondec_err(d, "Expected number or string");
  }

  if (upb_FieldDef_CType(f) == kUpb_CType_UInt32) {
    if (val.uint64_val > UINT32_MAX) {
      jsondec_err(d, "Integer out of range.");
    }
    val.uint32_val = (uint32_t)val.uint64_val;
  }

  return val;
}

/* Parse DOUBLE or FLOAT value. */
static upb_MessageValue jsondec_double(jsondec* d, const upb_FieldDef* f) {
  upb_StringView str;
  upb_MessageValue val = {0};

  switch (jsondec_peek(d)) {
    case JD_NUMBER:
      val.double_val = jsondec_number(d);
      break;
    case JD_STRING:
      str = jsondec_string(d);
      if (jsondec_streql(str, "NaN")) {
        val.double_val = NAN;
      } else if (jsondec_streql(str, "Infinity")) {
        val.double_val = INFINITY;
      } else if (jsondec_streql(str, "-Infinity")) {
        val.double_val = -INFINITY;
      } else {
        val.double_val = strtod(str.data, NULL);
      }
      break;
    default:
      jsondec_err(d, "Expected number or string");
  }

  if (upb_FieldDef_CType(f) == kUpb_CType_Float) {
    if (val.double_val != INFINITY && val.double_val != -INFINITY &&
        (val.double_val > FLT_MAX || val.double_val < -FLT_MAX)) {
      jsondec_err(d, "Float out of range");
    }
    val.float_val = val.double_val;
  }

  return val;
}

/* Parse STRING or BYTES value. */
static upb_MessageValue jsondec_strfield(jsondec* d, const upb_FieldDef* f) {
  upb_MessageValue val;
  val.str_val = jsondec_string(d);
  if (upb_FieldDef_CType(f) == kUpb_CType_Bytes) {
    val.str_val.size = jsondec_base64(d, val.str_val);
  }
  return val;
}

static upb_MessageValue jsondec_enum(jsondec* d, const upb_FieldDef* f) {
  switch (jsondec_peek(d)) {
    case JD_STRING: {
      upb_StringView str = jsondec_string(d);
      const upb_EnumDef* e = upb_FieldDef_EnumSubDef(f);
      const upb_EnumValueDef* ev =
          upb_EnumDef_FindValueByNameWithSize(e, str.data, str.size);
      upb_MessageValue val;
      if (ev) {
        val.int32_val = upb_EnumValueDef_Number(ev);
      } else {
        if (d->options & upb_JsonDecode_IgnoreUnknown) {
          val.int32_val = 0;
        } else {
          jsondec_errf(d, "Unknown enumerator: '" UPB_STRINGVIEW_FORMAT "'",
                       UPB_STRINGVIEW_ARGS(str));
        }
      }
      return val;
    }
    case JD_NULL: {
      if (jsondec_isnullvalue(f)) {
        upb_MessageValue val;
        jsondec_null(d);
        val.int32_val = 0;
        return val;
      }
    }
      /* Fallthrough. */
    default:
      return jsondec_int(d, f);
  }
}

static upb_MessageValue jsondec_bool(jsondec* d, const upb_FieldDef* f) {
  bool is_map_key = upb_FieldDef_Number(f) == 1 &&
                    upb_MessageDef_IsMapEntry(upb_FieldDef_ContainingType(f));
  upb_MessageValue val;

  if (is_map_key) {
    upb_StringView str = jsondec_string(d);
    if (jsondec_streql(str, "true")) {
      val.bool_val = true;
    } else if (jsondec_streql(str, "false")) {
      val.bool_val = false;
    } else {
      jsondec_err(d, "Invalid boolean map key");
    }
  } else {
    switch (jsondec_peek(d)) {
      case JD_TRUE:
        val.bool_val = true;
        jsondec_true(d);
        break;
      case JD_FALSE:
        val.bool_val = false;
        jsondec_false(d);
        break;
      default:
        jsondec_err(d, "Expected true or false");
    }
  }

  return val;
}

/* Composite types (array/message/map) ****************************************/

static void jsondec_array(jsondec* d, upb_Message* msg, const upb_FieldDef* f) {
  upb_Array* arr = upb_Message_Mutable(msg, f, d->arena).array;

  jsondec_arrstart(d);
  while (jsondec_arrnext(d)) {
    upb_MessageValue elem = jsondec_value(d, f);
    upb_Array_Append(arr, elem, d->arena);
  }
  jsondec_arrend(d);
}

static void jsondec_map(jsondec* d, upb_Message* msg, const upb_FieldDef* f) {
  upb_Map* map = upb_Message_Mutable(msg, f, d->arena).map;
  const upb_MessageDef* entry = upb_FieldDef_MessageSubDef(f);
  const upb_FieldDef* key_f = upb_MessageDef_FindFieldByNumber(entry, 1);
  const upb_FieldDef* val_f = upb_MessageDef_FindFieldByNumber(entry, 2);

  jsondec_objstart(d);
  while (jsondec_objnext(d)) {
    upb_MessageValue key, val;
    key = jsondec_value(d, key_f);
    jsondec_entrysep(d);
    val = jsondec_value(d, val_f);
    upb_Map_Set(map, key, val, d->arena);
  }
  jsondec_objend(d);
}

static void jsondec_tomsg(jsondec* d, upb_Message* msg,
                          const upb_MessageDef* m) {
  if (upb_MessageDef_WellKnownType(m) == kUpb_WellKnown_Unspecified) {
    jsondec_object(d, msg, m);
  } else {
    jsondec_wellknown(d, msg, m);
  }
}

static upb_MessageValue jsondec_msg(jsondec* d, const upb_FieldDef* f) {
  const upb_MessageDef* m = upb_FieldDef_MessageSubDef(f);
  upb_Message* msg = upb_Message_New(m, d->arena);
  upb_MessageValue val;

  jsondec_tomsg(d, msg, m);
  val.msg_val = msg;
  return val;
}

static void jsondec_field(jsondec* d, upb_Message* msg,
                          const upb_MessageDef* m) {
  upb_StringView name;
  const upb_FieldDef* f;
  const upb_FieldDef* preserved;

  name = jsondec_string(d);
  jsondec_entrysep(d);

  if (name.size >= 2 && name.data[0] == '[' &&
      name.data[name.size - 1] == ']') {
    f = upb_DefPool_FindExtensionByNameWithSize(d->symtab, name.data + 1,
                                                name.size - 2);
    if (f && upb_FieldDef_ContainingType(f) != m) {
      jsondec_errf(
          d, "Extension %s extends message %s, but was seen in message %s",
          upb_FieldDef_FullName(f),
          upb_MessageDef_FullName(upb_FieldDef_ContainingType(f)),
          upb_MessageDef_FullName(m));
    }
  } else {
    f = upb_MessageDef_FindByJsonNameWithSize(m, name.data, name.size);
  }

  if (!f) {
    if ((d->options & upb_JsonDecode_IgnoreUnknown) == 0) {
      jsondec_errf(d, "No such field: " UPB_STRINGVIEW_FORMAT,
                   UPB_STRINGVIEW_ARGS(name));
    }
    jsondec_skipval(d);
    return;
  }

  if (jsondec_peek(d) == JD_NULL && !jsondec_isvalue(f)) {
    /* JSON "null" indicates a default value, so no need to set anything. */
    jsondec_null(d);
    return;
  }

  if (upb_FieldDef_RealContainingOneof(f) &&
      upb_Message_WhichOneof(msg, upb_FieldDef_ContainingOneof(f))) {
    jsondec_err(d, "More than one field for this oneof.");
  }

  preserved = d->debug_field;
  d->debug_field = f;

  if (upb_FieldDef_IsMap(f)) {
    jsondec_map(d, msg, f);
  } else if (upb_FieldDef_IsRepeated(f)) {
    jsondec_array(d, msg, f);
  } else if (upb_FieldDef_IsSubMessage(f)) {
    upb_Message* submsg = upb_Message_Mutable(msg, f, d->arena).msg;
    const upb_MessageDef* subm = upb_FieldDef_MessageSubDef(f);
    jsondec_tomsg(d, submsg, subm);
  } else {
    upb_MessageValue val = jsondec_value(d, f);
    upb_Message_Set(msg, f, val, d->arena);
  }

  d->debug_field = preserved;
}

static void jsondec_object(jsondec* d, upb_Message* msg,
                           const upb_MessageDef* m) {
  jsondec_objstart(d);
  while (jsondec_objnext(d)) {
    jsondec_field(d, msg, m);
  }
  jsondec_objend(d);
}

static upb_MessageValue jsondec_value(jsondec* d, const upb_FieldDef* f) {
  switch (upb_FieldDef_CType(f)) {
    case kUpb_CType_Bool:
      return jsondec_bool(d, f);
    case kUpb_CType_Float:
    case kUpb_CType_Double:
      return jsondec_double(d, f);
    case kUpb_CType_UInt32:
    case kUpb_CType_UInt64:
      return jsondec_uint(d, f);
    case kUpb_CType_Int32:
    case kUpb_CType_Int64:
      return jsondec_int(d, f);
    case kUpb_CType_String:
    case kUpb_CType_Bytes:
      return jsondec_strfield(d, f);
    case kUpb_CType_Enum:
      return jsondec_enum(d, f);
    case kUpb_CType_Message:
      return jsondec_msg(d, f);
    default:
      UPB_UNREACHABLE();
  }
}

/* Well-known types ***********************************************************/

static int jsondec_tsdigits(jsondec* d, const char** ptr, size_t digits,
                            const char* after) {
  uint64_t val;
  const char* p = *ptr;
  const char* end = p + digits;
  size_t after_len = after ? strlen(after) : 0;

  UPB_ASSERT(digits <= 9); /* int can't overflow. */

  if (jsondec_buftouint64(d, p, end, &val) != end ||
      (after_len && memcmp(end, after, after_len) != 0)) {
    jsondec_err(d, "Malformed timestamp");
  }

  UPB_ASSERT(val < INT_MAX);

  *ptr = end + after_len;
  return (int)val;
}

static int jsondec_nanos(jsondec* d, const char** ptr, const char* end) {
  uint64_t nanos = 0;
  const char* p = *ptr;

  if (p != end && *p == '.') {
    const char* nano_end = jsondec_buftouint64(d, p + 1, end, &nanos);
    int digits = (int)(nano_end - p - 1);
    int exp_lg10 = 9 - digits;
    if (digits > 9) {
      jsondec_err(d, "Too many digits for partial seconds");
    }
    while (exp_lg10--) nanos *= 10;
    *ptr = nano_end;
  }

  UPB_ASSERT(nanos < INT_MAX);

  return (int)nanos;
}

/* jsondec_epochdays(1970, 1, 1) == 1970-01-01 == 0. */
int jsondec_epochdays(int y, int m, int d) {
  const uint32_t year_base = 4800; /* Before min year, multiple of 400. */
  const uint32_t m_adj = m - 3;    /* March-based month. */
  const uint32_t carry = m_adj > (uint32_t)m ? 1 : 0;
  const uint32_t adjust = carry ? 12 : 0;
  const uint32_t y_adj = y + year_base - carry;
  const uint32_t month_days = ((m_adj + adjust) * 62719 + 769) / 2048;
  const uint32_t leap_days = y_adj / 4 - y_adj / 100 + y_adj / 400;
  return y_adj * 365 + leap_days + month_days + (d - 1) - 2472632;
}

static int64_t jsondec_unixtime(int y, int m, int d, int h, int min, int s) {
  return (int64_t)jsondec_epochdays(y, m, d) * 86400 + h * 3600 + min * 60 + s;
}

static void jsondec_timestamp(jsondec* d, upb_Message* msg,
                              const upb_MessageDef* m) {
  upb_MessageValue seconds;
  upb_MessageValue nanos;
  upb_StringView str = jsondec_string(d);
  const char* ptr = str.data;
  const char* end = ptr + str.size;

  if (str.size < 20) goto malformed;

  {
    /* 1972-01-01T01:00:00 */
    int year = jsondec_tsdigits(d, &ptr, 4, "-");
    int mon = jsondec_tsdigits(d, &ptr, 2, "-");
    int day = jsondec_tsdigits(d, &ptr, 2, "T");
    int hour = jsondec_tsdigits(d, &ptr, 2, ":");
    int min = jsondec_tsdigits(d, &ptr, 2, ":");
    int sec = jsondec_tsdigits(d, &ptr, 2, NULL);

    seconds.int64_val = jsondec_unixtime(year, mon, day, hour, min, sec);
  }

  nanos.int32_val = jsondec_nanos(d, &ptr, end);

  {
    /* [+-]08:00 or Z */
    int ofs_hour = 0;
    int ofs_min = 0;
    bool neg = false;

    if (ptr == end) goto malformed;

    switch (*ptr++) {
      case '-':
        neg = true;
        /* fallthrough */
      case '+':
        if ((end - ptr) != 5) goto malformed;
        ofs_hour = jsondec_tsdigits(d, &ptr, 2, ":");
        ofs_min = jsondec_tsdigits(d, &ptr, 2, NULL);
        ofs_min = ((ofs_hour * 60) + ofs_min) * 60;
        seconds.int64_val += (neg ? ofs_min : -ofs_min);
        break;
      case 'Z':
        if (ptr != end) goto malformed;
        break;
      default:
        goto malformed;
    }
  }

  if (seconds.int64_val < -62135596800) {
    jsondec_err(d, "Timestamp out of range");
  }

  upb_Message_Set(msg, upb_MessageDef_FindFieldByNumber(m, 1), seconds,
                  d->arena);
  upb_Message_Set(msg, upb_MessageDef_FindFieldByNumber(m, 2), nanos, d->arena);
  return;

malformed:
  jsondec_err(d, "Malformed timestamp");
}

static void jsondec_duration(jsondec* d, upb_Message* msg,
                             const upb_MessageDef* m) {
  upb_MessageValue seconds;
  upb_MessageValue nanos;
  upb_StringView str = jsondec_string(d);
  const char* ptr = str.data;
  const char* end = ptr + str.size;
  const int64_t max = (uint64_t)3652500 * 86400;

  /* "3.000000001s", "3s", etc. */
  ptr = jsondec_buftoint64(d, ptr, end, &seconds.int64_val);
  nanos.int32_val = jsondec_nanos(d, &ptr, end);

  if (end - ptr != 1 || *ptr != 's') {
    jsondec_err(d, "Malformed duration");
  }

  if (seconds.int64_val < -max || seconds.int64_val > max) {
    jsondec_err(d, "Duration out of range");
  }

  if (seconds.int64_val < 0) {
    nanos.int32_val = -nanos.int32_val;
  }

  upb_Message_Set(msg, upb_MessageDef_FindFieldByNumber(m, 1), seconds,
                  d->arena);
  upb_Message_Set(msg, upb_MessageDef_FindFieldByNumber(m, 2), nanos, d->arena);
}

static void jsondec_listvalue(jsondec* d, upb_Message* msg,
                              const upb_MessageDef* m) {
  const upb_FieldDef* values_f = upb_MessageDef_FindFieldByNumber(m, 1);
  const upb_MessageDef* value_m = upb_FieldDef_MessageSubDef(values_f);
  upb_Array* values = upb_Message_Mutable(msg, values_f, d->arena).array;

  jsondec_arrstart(d);
  while (jsondec_arrnext(d)) {
    upb_Message* value_msg = upb_Message_New(value_m, d->arena);
    upb_MessageValue value;
    value.msg_val = value_msg;
    upb_Array_Append(values, value, d->arena);
    jsondec_wellknownvalue(d, value_msg, value_m);
  }
  jsondec_arrend(d);
}

static void jsondec_struct(jsondec* d, upb_Message* msg,
                           const upb_MessageDef* m) {
  const upb_FieldDef* fields_f = upb_MessageDef_FindFieldByNumber(m, 1);
  const upb_MessageDef* entry_m = upb_FieldDef_MessageSubDef(fields_f);
  const upb_FieldDef* value_f = upb_MessageDef_FindFieldByNumber(entry_m, 2);
  const upb_MessageDef* value_m = upb_FieldDef_MessageSubDef(value_f);
  upb_Map* fields = upb_Message_Mutable(msg, fields_f, d->arena).map;

  jsondec_objstart(d);
  while (jsondec_objnext(d)) {
    upb_MessageValue key, value;
    upb_Message* value_msg = upb_Message_New(value_m, d->arena);
    key.str_val = jsondec_string(d);
    value.msg_val = value_msg;
    upb_Map_Set(fields, key, value, d->arena);
    jsondec_entrysep(d);
    jsondec_wellknownvalue(d, value_msg, value_m);
  }
  jsondec_objend(d);
}

static void jsondec_wellknownvalue(jsondec* d, upb_Message* msg,
                                   const upb_MessageDef* m) {
  upb_MessageValue val;
  const upb_FieldDef* f;
  upb_Message* submsg;

  switch (jsondec_peek(d)) {
    case JD_NUMBER:
      /* double number_value = 2; */
      f = upb_MessageDef_FindFieldByNumber(m, 2);
      val.double_val = jsondec_number(d);
      break;
    case JD_STRING:
      /* string string_value = 3; */
      f = upb_MessageDef_FindFieldByNumber(m, 3);
      val.str_val = jsondec_string(d);
      break;
    case JD_FALSE:
      /* bool bool_value = 4; */
      f = upb_MessageDef_FindFieldByNumber(m, 4);
      val.bool_val = false;
      jsondec_false(d);
      break;
    case JD_TRUE:
      /* bool bool_value = 4; */
      f = upb_MessageDef_FindFieldByNumber(m, 4);
      val.bool_val = true;
      jsondec_true(d);
      break;
    case JD_NULL:
      /* NullValue null_value = 1; */
      f = upb_MessageDef_FindFieldByNumber(m, 1);
      val.int32_val = 0;
      jsondec_null(d);
      break;
    /* Note: these cases return, because upb_Message_Mutable() is enough. */
    case JD_OBJECT:
      /* Struct struct_value = 5; */
      f = upb_MessageDef_FindFieldByNumber(m, 5);
      submsg = upb_Message_Mutable(msg, f, d->arena).msg;
      jsondec_struct(d, submsg, upb_FieldDef_MessageSubDef(f));
      return;
    case JD_ARRAY:
      /* ListValue list_value = 6; */
      f = upb_MessageDef_FindFieldByNumber(m, 6);
      submsg = upb_Message_Mutable(msg, f, d->arena).msg;
      jsondec_listvalue(d, submsg, upb_FieldDef_MessageSubDef(f));
      return;
    default:
      UPB_UNREACHABLE();
  }

  upb_Message_Set(msg, f, val, d->arena);
}

static upb_StringView jsondec_mask(jsondec* d, const char* buf,
                                   const char* end) {
  /* FieldMask fields grow due to inserted '_' characters, so we can't do the
   * transform in place. */
  const char* ptr = buf;
  upb_StringView ret;
  char* out;

  ret.size = end - ptr;
  while (ptr < end) {
    ret.size += (*ptr >= 'A' && *ptr <= 'Z');
    ptr++;
  }

  out = upb_Arena_Malloc(d->arena, ret.size);
  ptr = buf;
  ret.data = out;

  while (ptr < end) {
    char ch = *ptr++;
    if (ch >= 'A' && ch <= 'Z') {
      *out++ = '_';
      *out++ = ch + 32;
    } else if (ch == '_') {
      jsondec_err(d, "field mask may not contain '_'");
    } else {
      *out++ = ch;
    }
  }

  return ret;
}

static void jsondec_fieldmask(jsondec* d, upb_Message* msg,
                              const upb_MessageDef* m) {
  /* repeated string paths = 1; */
  const upb_FieldDef* paths_f = upb_MessageDef_FindFieldByNumber(m, 1);
  upb_Array* arr = upb_Message_Mutable(msg, paths_f, d->arena).array;
  upb_StringView str = jsondec_string(d);
  const char* ptr = str.data;
  const char* end = ptr + str.size;
  upb_MessageValue val;

  while (ptr < end) {
    const char* elem_end = memchr(ptr, ',', end - ptr);
    if (elem_end) {
      val.str_val = jsondec_mask(d, ptr, elem_end);
      ptr = elem_end + 1;
    } else {
      val.str_val = jsondec_mask(d, ptr, end);
      ptr = end;
    }
    upb_Array_Append(arr, val, d->arena);
  }
}

static void jsondec_anyfield(jsondec* d, upb_Message* msg,
                             const upb_MessageDef* m) {
  if (upb_MessageDef_WellKnownType(m) == kUpb_WellKnown_Unspecified) {
    /* For regular types: {"@type": "[user type]", "f1": <V1>, "f2": <V2>}
     * where f1, f2, etc. are the normal fields of this type. */
    jsondec_field(d, msg, m);
  } else {
    /* For well-known types: {"@type": "[well-known type]", "value": <X>}
     * where <X> is whatever encoding the WKT normally uses. */
    upb_StringView str = jsondec_string(d);
    jsondec_entrysep(d);
    if (!jsondec_streql(str, "value")) {
      jsondec_err(d, "Key for well-known type must be 'value'");
    }
    jsondec_wellknown(d, msg, m);
  }
}

static const upb_MessageDef* jsondec_typeurl(jsondec* d, upb_Message* msg,
                                             const upb_MessageDef* m) {
  const upb_FieldDef* type_url_f = upb_MessageDef_FindFieldByNumber(m, 1);
  const upb_MessageDef* type_m;
  upb_StringView type_url = jsondec_string(d);
  const char* end = type_url.data + type_url.size;
  const char* ptr = end;
  upb_MessageValue val;

  val.str_val = type_url;
  upb_Message_Set(msg, type_url_f, val, d->arena);

  /* Find message name after the last '/' */
  while (ptr > type_url.data && *--ptr != '/') {
  }

  if (ptr == type_url.data || ptr == end) {
    jsondec_err(d, "Type url must have at least one '/' and non-empty host");
  }

  ptr++;
  type_m = upb_DefPool_FindMessageByNameWithSize(d->symtab, ptr, end - ptr);

  if (!type_m) {
    jsondec_err(d, "Type was not found");
  }

  return type_m;
}

static void jsondec_any(jsondec* d, upb_Message* msg, const upb_MessageDef* m) {
  /* string type_url = 1;
   * bytes value = 2; */
  const upb_FieldDef* value_f = upb_MessageDef_FindFieldByNumber(m, 2);
  upb_Message* any_msg;
  const upb_MessageDef* any_m = NULL;
  const char* pre_type_data = NULL;
  const char* pre_type_end = NULL;
  upb_MessageValue encoded;

  jsondec_objstart(d);

  /* Scan looking for "@type", which is not necessarily first. */
  while (!any_m && jsondec_objnext(d)) {
    const char* start = d->ptr;
    upb_StringView name = jsondec_string(d);
    jsondec_entrysep(d);
    if (jsondec_streql(name, "@type")) {
      any_m = jsondec_typeurl(d, msg, m);
      if (pre_type_data) {
        pre_type_end = start;
        while (*pre_type_end != ',') pre_type_end--;
      }
    } else {
      if (!pre_type_data) pre_type_data = start;
      jsondec_skipval(d);
    }
  }

  if (!any_m) {
    jsondec_err(d, "Any object didn't contain a '@type' field");
  }

  any_msg = upb_Message_New(any_m, d->arena);

  if (pre_type_data) {
    size_t len = pre_type_end - pre_type_data + 1;
    char* tmp = upb_Arena_Malloc(d->arena, len);
    const char* saved_ptr = d->ptr;
    const char* saved_end = d->end;
    memcpy(tmp, pre_type_data, len - 1);
    tmp[len - 1] = '}';
    d->ptr = tmp;
    d->end = tmp + len;
    d->is_first = true;
    while (jsondec_objnext(d)) {
      jsondec_anyfield(d, any_msg, any_m);
    }
    d->ptr = saved_ptr;
    d->end = saved_end;
  }

  while (jsondec_objnext(d)) {
    jsondec_anyfield(d, any_msg, any_m);
  }

  jsondec_objend(d);

  encoded.str_val.data = upb_Encode(any_msg, upb_MessageDef_MiniTable(any_m), 0,
                                    d->arena, &encoded.str_val.size);
  upb_Message_Set(msg, value_f, encoded, d->arena);
}

static void jsondec_wrapper(jsondec* d, upb_Message* msg,
                            const upb_MessageDef* m) {
  const upb_FieldDef* value_f = upb_MessageDef_FindFieldByNumber(m, 1);
  upb_MessageValue val = jsondec_value(d, value_f);
  upb_Message_Set(msg, value_f, val, d->arena);
}

static void jsondec_wellknown(jsondec* d, upb_Message* msg,
                              const upb_MessageDef* m) {
  switch (upb_MessageDef_WellKnownType(m)) {
    case kUpb_WellKnown_Any:
      jsondec_any(d, msg, m);
      break;
    case kUpb_WellKnown_FieldMask:
      jsondec_fieldmask(d, msg, m);
      break;
    case kUpb_WellKnown_Duration:
      jsondec_duration(d, msg, m);
      break;
    case kUpb_WellKnown_Timestamp:
      jsondec_timestamp(d, msg, m);
      break;
    case kUpb_WellKnown_Value:
      jsondec_wellknownvalue(d, msg, m);
      break;
    case kUpb_WellKnown_ListValue:
      jsondec_listvalue(d, msg, m);
      break;
    case kUpb_WellKnown_Struct:
      jsondec_struct(d, msg, m);
      break;
    case kUpb_WellKnown_DoubleValue:
    case kUpb_WellKnown_FloatValue:
    case kUpb_WellKnown_Int64Value:
    case kUpb_WellKnown_UInt64Value:
    case kUpb_WellKnown_Int32Value:
    case kUpb_WellKnown_UInt32Value:
    case kUpb_WellKnown_StringValue:
    case kUpb_WellKnown_BytesValue:
    case kUpb_WellKnown_BoolValue:
      jsondec_wrapper(d, msg, m);
      break;
    default:
      UPB_UNREACHABLE();
  }
}

bool upb_JsonDecode(const char* buf, size_t size, upb_Message* msg,
                    const upb_MessageDef* m, const upb_DefPool* symtab,
                    int options, upb_Arena* arena, upb_Status* status) {
  jsondec d;

  if (size == 0) return true;

  d.ptr = buf;
  d.end = buf + size;
  d.arena = arena;
  d.symtab = symtab;
  d.status = status;
  d.options = options;
  d.depth = 64;
  d.line = 1;
  d.line_begin = d.ptr;
  d.debug_field = NULL;
  d.is_first = false;

  if (UPB_SETJMP(d.err)) return false;

  jsondec_tomsg(&d, msg, m);
  return true;
}

/** upb/json_encode.c ************************************************************/

#include <ctype.h>
#include <float.h>
#include <inttypes.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>


/* Must be last. */

typedef struct {
  char *buf, *ptr, *end;
  size_t overflow;
  int indent_depth;
  int options;
  const upb_DefPool* ext_pool;
  jmp_buf err;
  upb_Status* status;
  upb_Arena* arena;
} jsonenc;

static void jsonenc_msg(jsonenc* e, const upb_Message* msg,
                        const upb_MessageDef* m);
static void jsonenc_scalar(jsonenc* e, upb_MessageValue val,
                           const upb_FieldDef* f);
static void jsonenc_msgfield(jsonenc* e, const upb_Message* msg,
                             const upb_MessageDef* m);
static void jsonenc_msgfields(jsonenc* e, const upb_Message* msg,
                              const upb_MessageDef* m, bool first);
static void jsonenc_value(jsonenc* e, const upb_Message* msg,
                          const upb_MessageDef* m);

UPB_NORETURN static void jsonenc_err(jsonenc* e, const char* msg) {
  upb_Status_SetErrorMessage(e->status, msg);
  longjmp(e->err, 1);
}

UPB_PRINTF(2, 3)
UPB_NORETURN static void jsonenc_errf(jsonenc* e, const char* fmt, ...) {
  va_list argp;
  va_start(argp, fmt);
  upb_Status_VSetErrorFormat(e->status, fmt, argp);
  va_end(argp);
  longjmp(e->err, 1);
}

static upb_Arena* jsonenc_arena(jsonenc* e) {
  /* Create lazily, since it's only needed for Any */
  if (!e->arena) {
    e->arena = upb_Arena_New();
  }
  return e->arena;
}

static void jsonenc_putbytes(jsonenc* e, const void* data, size_t len) {
  size_t have = e->end - e->ptr;
  if (UPB_LIKELY(have >= len)) {
    memcpy(e->ptr, data, len);
    e->ptr += len;
  } else {
    if (have) {
      memcpy(e->ptr, data, have);
      e->ptr += have;
    }
    e->overflow += (len - have);
  }
}

static void jsonenc_putstr(jsonenc* e, const char* str) {
  jsonenc_putbytes(e, str, strlen(str));
}

UPB_PRINTF(2, 3)
static void jsonenc_printf(jsonenc* e, const char* fmt, ...) {
  size_t n;
  size_t have = e->end - e->ptr;
  va_list args;

  va_start(args, fmt);
  n = _upb_vsnprintf(e->ptr, have, fmt, args);
  va_end(args);

  if (UPB_LIKELY(have > n)) {
    e->ptr += n;
  } else {
    e->ptr = UPB_PTRADD(e->ptr, have);
    e->overflow += (n - have);
  }
}

static void jsonenc_nanos(jsonenc* e, int32_t nanos) {
  int digits = 9;

  if (nanos == 0) return;
  if (nanos < 0 || nanos >= 1000000000) {
    jsonenc_err(e, "error formatting timestamp as JSON: invalid nanos");
  }

  while (nanos % 1000 == 0) {
    nanos /= 1000;
    digits -= 3;
  }

  jsonenc_printf(e, ".%.*" PRId32, digits, nanos);
}

static void jsonenc_timestamp(jsonenc* e, const upb_Message* msg,
                              const upb_MessageDef* m) {
  const upb_FieldDef* seconds_f = upb_MessageDef_FindFieldByNumber(m, 1);
  const upb_FieldDef* nanos_f = upb_MessageDef_FindFieldByNumber(m, 2);
  int64_t seconds = upb_Message_Get(msg, seconds_f).int64_val;
  int32_t nanos = upb_Message_Get(msg, nanos_f).int32_val;
  int L, N, I, J, K, hour, min, sec;

  if (seconds < -62135596800) {
    jsonenc_err(e,
                "error formatting timestamp as JSON: minimum acceptable value "
                "is 0001-01-01T00:00:00Z");
  } else if (seconds > 253402300799) {
    jsonenc_err(e,
                "error formatting timestamp as JSON: maximum acceptable value "
                "is 9999-12-31T23:59:59Z");
  }

  /* Julian Day -> Y/M/D, Algorithm from:
   * Fliegel, H. F., and Van Flandern, T. C., "A Machine Algorithm for
   *   Processing Calendar Dates," Communications of the Association of
   *   Computing Machines, vol. 11 (1968), p. 657.  */
  seconds += 62135596800;  // Ensure seconds is positive.
  L = (int)(seconds / 86400) - 719162 + 68569 + 2440588;
  N = 4 * L / 146097;
  L = L - (146097 * N + 3) / 4;
  I = 4000 * (L + 1) / 1461001;
  L = L - 1461 * I / 4 + 31;
  J = 80 * L / 2447;
  K = L - 2447 * J / 80;
  L = J / 11;
  J = J + 2 - 12 * L;
  I = 100 * (N - 49) + I + L;

  sec = seconds % 60;
  min = (seconds / 60) % 60;
  hour = (seconds / 3600) % 24;

  jsonenc_printf(e, "\"%04d-%02d-%02dT%02d:%02d:%02d", I, J, K, hour, min, sec);
  jsonenc_nanos(e, nanos);
  jsonenc_putstr(e, "Z\"");
}

static void jsonenc_duration(jsonenc* e, const upb_Message* msg,
                             const upb_MessageDef* m) {
  const upb_FieldDef* seconds_f = upb_MessageDef_FindFieldByNumber(m, 1);
  const upb_FieldDef* nanos_f = upb_MessageDef_FindFieldByNumber(m, 2);
  int64_t seconds = upb_Message_Get(msg, seconds_f).int64_val;
  int32_t nanos = upb_Message_Get(msg, nanos_f).int32_val;

  if (seconds > 315576000000 || seconds < -315576000000 ||
      (seconds < 0) != (nanos < 0)) {
    jsonenc_err(e, "bad duration");
  }

  if (nanos < 0) {
    nanos = -nanos;
  }

  jsonenc_printf(e, "\"%" PRId64, seconds);
  jsonenc_nanos(e, nanos);
  jsonenc_putstr(e, "s\"");
}

static void jsonenc_enum(int32_t val, const upb_FieldDef* f, jsonenc* e) {
  const upb_EnumDef* e_def = upb_FieldDef_EnumSubDef(f);

  if (strcmp(upb_EnumDef_FullName(e_def), "google.protobuf.NullValue") == 0) {
    jsonenc_putstr(e, "null");
  } else {
    const upb_EnumValueDef* ev = upb_EnumDef_FindValueByNumber(e_def, val);

    if (ev) {
      jsonenc_printf(e, "\"%s\"", upb_EnumValueDef_Name(ev));
    } else {
      jsonenc_printf(e, "%" PRId32, val);
    }
  }
}

static void jsonenc_bytes(jsonenc* e, upb_StringView str) {
  /* This is the regular base64, not the "web-safe" version. */
  static const char base64[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  const unsigned char* ptr = (unsigned char*)str.data;
  const unsigned char* end = UPB_PTRADD(ptr, str.size);
  char buf[4];

  jsonenc_putstr(e, "\"");

  while (end - ptr >= 3) {
    buf[0] = base64[ptr[0] >> 2];
    buf[1] = base64[((ptr[0] & 0x3) << 4) | (ptr[1] >> 4)];
    buf[2] = base64[((ptr[1] & 0xf) << 2) | (ptr[2] >> 6)];
    buf[3] = base64[ptr[2] & 0x3f];
    jsonenc_putbytes(e, buf, 4);
    ptr += 3;
  }

  switch (end - ptr) {
    case 2:
      buf[0] = base64[ptr[0] >> 2];
      buf[1] = base64[((ptr[0] & 0x3) << 4) | (ptr[1] >> 4)];
      buf[2] = base64[(ptr[1] & 0xf) << 2];
      buf[3] = '=';
      jsonenc_putbytes(e, buf, 4);
      break;
    case 1:
      buf[0] = base64[ptr[0] >> 2];
      buf[1] = base64[((ptr[0] & 0x3) << 4)];
      buf[2] = '=';
      buf[3] = '=';
      jsonenc_putbytes(e, buf, 4);
      break;
  }

  jsonenc_putstr(e, "\"");
}

static void jsonenc_stringbody(jsonenc* e, upb_StringView str) {
  const char* ptr = str.data;
  const char* end = UPB_PTRADD(ptr, str.size);

  while (ptr < end) {
    switch (*ptr) {
      case '\n':
        jsonenc_putstr(e, "\\n");
        break;
      case '\r':
        jsonenc_putstr(e, "\\r");
        break;
      case '\t':
        jsonenc_putstr(e, "\\t");
        break;
      case '\"':
        jsonenc_putstr(e, "\\\"");
        break;
      case '\f':
        jsonenc_putstr(e, "\\f");
        break;
      case '\b':
        jsonenc_putstr(e, "\\b");
        break;
      case '\\':
        jsonenc_putstr(e, "\\\\");
        break;
      default:
        if ((uint8_t)*ptr < 0x20) {
          jsonenc_printf(e, "\\u%04x", (int)(uint8_t)*ptr);
        } else {
          /* This could be a non-ASCII byte.  We rely on the string being valid
           * UTF-8. */
          jsonenc_putbytes(e, ptr, 1);
        }
        break;
    }
    ptr++;
  }
}

static void jsonenc_string(jsonenc* e, upb_StringView str) {
  jsonenc_putstr(e, "\"");
  jsonenc_stringbody(e, str);
  jsonenc_putstr(e, "\"");
}

static bool upb_JsonEncode_HandleSpecialDoubles(jsonenc* e, double val) {
  if (val == INFINITY) {
    jsonenc_putstr(e, "\"Infinity\"");
  } else if (val == -INFINITY) {
    jsonenc_putstr(e, "\"-Infinity\"");
  } else if (val != val) {
    jsonenc_putstr(e, "\"NaN\"");
  } else {
    return false;
  }
  return true;
}

static void upb_JsonEncode_Double(jsonenc* e, double val) {
  if (upb_JsonEncode_HandleSpecialDoubles(e, val)) return;
  char buf[32];
  _upb_EncodeRoundTripDouble(val, buf, sizeof(buf));
  jsonenc_putstr(e, buf);
}

static void upb_JsonEncode_Float(jsonenc* e, float val) {
  if (upb_JsonEncode_HandleSpecialDoubles(e, val)) return;
  char buf[32];
  _upb_EncodeRoundTripFloat(val, buf, sizeof(buf));
  jsonenc_putstr(e, buf);
}

static void jsonenc_wrapper(jsonenc* e, const upb_Message* msg,
                            const upb_MessageDef* m) {
  const upb_FieldDef* val_f = upb_MessageDef_FindFieldByNumber(m, 1);
  upb_MessageValue val = upb_Message_Get(msg, val_f);
  jsonenc_scalar(e, val, val_f);
}

static const upb_MessageDef* jsonenc_getanymsg(jsonenc* e,
                                               upb_StringView type_url) {
  /* Find last '/', if any. */
  const char* end = type_url.data + type_url.size;
  const char* ptr = end;
  const upb_MessageDef* ret;

  if (!e->ext_pool) {
    jsonenc_err(e, "Tried to encode Any, but no symtab was provided");
  }

  if (type_url.size == 0) goto badurl;

  while (true) {
    if (--ptr == type_url.data) {
      /* Type URL must contain at least one '/', with host before. */
      goto badurl;
    }
    if (*ptr == '/') {
      ptr++;
      break;
    }
  }

  ret = upb_DefPool_FindMessageByNameWithSize(e->ext_pool, ptr, end - ptr);

  if (!ret) {
    jsonenc_errf(e, "Couldn't find Any type: %.*s", (int)(end - ptr), ptr);
  }

  return ret;

badurl:
  jsonenc_errf(e, "Bad type URL: " UPB_STRINGVIEW_FORMAT,
               UPB_STRINGVIEW_ARGS(type_url));
}

static void jsonenc_any(jsonenc* e, const upb_Message* msg,
                        const upb_MessageDef* m) {
  const upb_FieldDef* type_url_f = upb_MessageDef_FindFieldByNumber(m, 1);
  const upb_FieldDef* value_f = upb_MessageDef_FindFieldByNumber(m, 2);
  upb_StringView type_url = upb_Message_Get(msg, type_url_f).str_val;
  upb_StringView value = upb_Message_Get(msg, value_f).str_val;
  const upb_MessageDef* any_m = jsonenc_getanymsg(e, type_url);
  const upb_MiniTable* any_layout = upb_MessageDef_MiniTable(any_m);
  upb_Arena* arena = jsonenc_arena(e);
  upb_Message* any = upb_Message_New(any_m, arena);

  if (upb_Decode(value.data, value.size, any, any_layout, NULL, 0, arena) !=
      kUpb_DecodeStatus_Ok) {
    jsonenc_err(e, "Error decoding message in Any");
  }

  jsonenc_putstr(e, "{\"@type\":");
  jsonenc_string(e, type_url);

  if (upb_MessageDef_WellKnownType(any_m) == kUpb_WellKnown_Unspecified) {
    /* Regular messages: {"@type": "...","foo": 1, "bar": 2} */
    jsonenc_msgfields(e, any, any_m, false);
  } else {
    /* Well-known type: {"@type": "...","value": <well-known encoding>} */
    jsonenc_putstr(e, ",\"value\":");
    jsonenc_msgfield(e, any, any_m);
  }

  jsonenc_putstr(e, "}");
}

static void jsonenc_putsep(jsonenc* e, const char* str, bool* first) {
  if (*first) {
    *first = false;
  } else {
    jsonenc_putstr(e, str);
  }
}

static void jsonenc_fieldpath(jsonenc* e, upb_StringView path) {
  const char* ptr = path.data;
  const char* end = ptr + path.size;

  while (ptr < end) {
    char ch = *ptr;

    if (ch >= 'A' && ch <= 'Z') {
      jsonenc_err(e, "Field mask element may not have upper-case letter.");
    } else if (ch == '_') {
      if (ptr == end - 1 || *(ptr + 1) < 'a' || *(ptr + 1) > 'z') {
        jsonenc_err(e, "Underscore must be followed by a lowercase letter.");
      }
      ch = *++ptr - 32;
    }

    jsonenc_putbytes(e, &ch, 1);
    ptr++;
  }
}

static void jsonenc_fieldmask(jsonenc* e, const upb_Message* msg,
                              const upb_MessageDef* m) {
  const upb_FieldDef* paths_f = upb_MessageDef_FindFieldByNumber(m, 1);
  const upb_Array* paths = upb_Message_Get(msg, paths_f).array_val;
  bool first = true;
  size_t i, n = 0;

  if (paths) n = upb_Array_Size(paths);

  jsonenc_putstr(e, "\"");

  for (i = 0; i < n; i++) {
    jsonenc_putsep(e, ",", &first);
    jsonenc_fieldpath(e, upb_Array_Get(paths, i).str_val);
  }

  jsonenc_putstr(e, "\"");
}

static void jsonenc_struct(jsonenc* e, const upb_Message* msg,
                           const upb_MessageDef* m) {
  const upb_FieldDef* fields_f = upb_MessageDef_FindFieldByNumber(m, 1);
  const upb_Map* fields = upb_Message_Get(msg, fields_f).map_val;
  const upb_MessageDef* entry_m = upb_FieldDef_MessageSubDef(fields_f);
  const upb_FieldDef* value_f = upb_MessageDef_FindFieldByNumber(entry_m, 2);
  size_t iter = kUpb_Map_Begin;
  bool first = true;

  jsonenc_putstr(e, "{");

  if (fields) {
    while (upb_MapIterator_Next(fields, &iter)) {
      upb_MessageValue key = upb_MapIterator_Key(fields, iter);
      upb_MessageValue val = upb_MapIterator_Value(fields, iter);

      jsonenc_putsep(e, ",", &first);
      jsonenc_string(e, key.str_val);
      jsonenc_putstr(e, ":");
      jsonenc_value(e, val.msg_val, upb_FieldDef_MessageSubDef(value_f));
    }
  }

  jsonenc_putstr(e, "}");
}

static void jsonenc_listvalue(jsonenc* e, const upb_Message* msg,
                              const upb_MessageDef* m) {
  const upb_FieldDef* values_f = upb_MessageDef_FindFieldByNumber(m, 1);
  const upb_MessageDef* values_m = upb_FieldDef_MessageSubDef(values_f);
  const upb_Array* values = upb_Message_Get(msg, values_f).array_val;
  size_t i;
  bool first = true;

  jsonenc_putstr(e, "[");

  if (values) {
    const size_t size = upb_Array_Size(values);
    for (i = 0; i < size; i++) {
      upb_MessageValue elem = upb_Array_Get(values, i);

      jsonenc_putsep(e, ",", &first);
      jsonenc_value(e, elem.msg_val, values_m);
    }
  }

  jsonenc_putstr(e, "]");
}

static void jsonenc_value(jsonenc* e, const upb_Message* msg,
                          const upb_MessageDef* m) {
  /* TODO(haberman): do we want a reflection method to get oneof case? */
  size_t iter = kUpb_Message_Begin;
  const upb_FieldDef* f;
  upb_MessageValue val;

  if (!upb_Message_Next(msg, m, NULL, &f, &val, &iter)) {
    jsonenc_err(e, "No value set in Value proto");
  }

  switch (upb_FieldDef_Number(f)) {
    case 1:
      jsonenc_putstr(e, "null");
      break;
    case 2:
      upb_JsonEncode_Double(e, val.double_val);
      break;
    case 3:
      jsonenc_string(e, val.str_val);
      break;
    case 4:
      jsonenc_putstr(e, val.bool_val ? "true" : "false");
      break;
    case 5:
      jsonenc_struct(e, val.msg_val, upb_FieldDef_MessageSubDef(f));
      break;
    case 6:
      jsonenc_listvalue(e, val.msg_val, upb_FieldDef_MessageSubDef(f));
      break;
  }
}

static void jsonenc_msgfield(jsonenc* e, const upb_Message* msg,
                             const upb_MessageDef* m) {
  switch (upb_MessageDef_WellKnownType(m)) {
    case kUpb_WellKnown_Unspecified:
      jsonenc_msg(e, msg, m);
      break;
    case kUpb_WellKnown_Any:
      jsonenc_any(e, msg, m);
      break;
    case kUpb_WellKnown_FieldMask:
      jsonenc_fieldmask(e, msg, m);
      break;
    case kUpb_WellKnown_Duration:
      jsonenc_duration(e, msg, m);
      break;
    case kUpb_WellKnown_Timestamp:
      jsonenc_timestamp(e, msg, m);
      break;
    case kUpb_WellKnown_DoubleValue:
    case kUpb_WellKnown_FloatValue:
    case kUpb_WellKnown_Int64Value:
    case kUpb_WellKnown_UInt64Value:
    case kUpb_WellKnown_Int32Value:
    case kUpb_WellKnown_UInt32Value:
    case kUpb_WellKnown_StringValue:
    case kUpb_WellKnown_BytesValue:
    case kUpb_WellKnown_BoolValue:
      jsonenc_wrapper(e, msg, m);
      break;
    case kUpb_WellKnown_Value:
      jsonenc_value(e, msg, m);
      break;
    case kUpb_WellKnown_ListValue:
      jsonenc_listvalue(e, msg, m);
      break;
    case kUpb_WellKnown_Struct:
      jsonenc_struct(e, msg, m);
      break;
  }
}

static void jsonenc_scalar(jsonenc* e, upb_MessageValue val,
                           const upb_FieldDef* f) {
  switch (upb_FieldDef_CType(f)) {
    case kUpb_CType_Bool:
      jsonenc_putstr(e, val.bool_val ? "true" : "false");
      break;
    case kUpb_CType_Float:
      upb_JsonEncode_Float(e, val.float_val);
      break;
    case kUpb_CType_Double:
      upb_JsonEncode_Double(e, val.double_val);
      break;
    case kUpb_CType_Int32:
      jsonenc_printf(e, "%" PRId32, val.int32_val);
      break;
    case kUpb_CType_UInt32:
      jsonenc_printf(e, "%" PRIu32, val.uint32_val);
      break;
    case kUpb_CType_Int64:
      jsonenc_printf(e, "\"%" PRId64 "\"", val.int64_val);
      break;
    case kUpb_CType_UInt64:
      jsonenc_printf(e, "\"%" PRIu64 "\"", val.uint64_val);
      break;
    case kUpb_CType_String:
      jsonenc_string(e, val.str_val);
      break;
    case kUpb_CType_Bytes:
      jsonenc_bytes(e, val.str_val);
      break;
    case kUpb_CType_Enum:
      jsonenc_enum(val.int32_val, f, e);
      break;
    case kUpb_CType_Message:
      jsonenc_msgfield(e, val.msg_val, upb_FieldDef_MessageSubDef(f));
      break;
  }
}

static void jsonenc_mapkey(jsonenc* e, upb_MessageValue val,
                           const upb_FieldDef* f) {
  jsonenc_putstr(e, "\"");

  switch (upb_FieldDef_CType(f)) {
    case kUpb_CType_Bool:
      jsonenc_putstr(e, val.bool_val ? "true" : "false");
      break;
    case kUpb_CType_Int32:
      jsonenc_printf(e, "%" PRId32, val.int32_val);
      break;
    case kUpb_CType_UInt32:
      jsonenc_printf(e, "%" PRIu32, val.uint32_val);
      break;
    case kUpb_CType_Int64:
      jsonenc_printf(e, "%" PRId64, val.int64_val);
      break;
    case kUpb_CType_UInt64:
      jsonenc_printf(e, "%" PRIu64, val.uint64_val);
      break;
    case kUpb_CType_String:
      jsonenc_stringbody(e, val.str_val);
      break;
    default:
      UPB_UNREACHABLE();
  }

  jsonenc_putstr(e, "\":");
}

static void jsonenc_array(jsonenc* e, const upb_Array* arr,
                          const upb_FieldDef* f) {
  size_t i;
  size_t size = arr ? upb_Array_Size(arr) : 0;
  bool first = true;

  jsonenc_putstr(e, "[");

  for (i = 0; i < size; i++) {
    jsonenc_putsep(e, ",", &first);
    jsonenc_scalar(e, upb_Array_Get(arr, i), f);
  }

  jsonenc_putstr(e, "]");
}

static void jsonenc_map(jsonenc* e, const upb_Map* map, const upb_FieldDef* f) {
  const upb_MessageDef* entry = upb_FieldDef_MessageSubDef(f);
  const upb_FieldDef* key_f = upb_MessageDef_FindFieldByNumber(entry, 1);
  const upb_FieldDef* val_f = upb_MessageDef_FindFieldByNumber(entry, 2);
  size_t iter = kUpb_Map_Begin;
  bool first = true;

  jsonenc_putstr(e, "{");

  if (map) {
    while (upb_MapIterator_Next(map, &iter)) {
      jsonenc_putsep(e, ",", &first);
      jsonenc_mapkey(e, upb_MapIterator_Key(map, iter), key_f);
      jsonenc_scalar(e, upb_MapIterator_Value(map, iter), val_f);
    }
  }

  jsonenc_putstr(e, "}");
}

static void jsonenc_fieldval(jsonenc* e, const upb_FieldDef* f,
                             upb_MessageValue val, bool* first) {
  const char* name;

  jsonenc_putsep(e, ",", first);

  if (upb_FieldDef_IsExtension(f)) {
    // TODO: For MessageSet, I would have expected this to print the message
    // name here, but Python doesn't appear to do this. We should do more
    // research here about what various implementations do.
    jsonenc_printf(e, "\"[%s]\":", upb_FieldDef_FullName(f));
  } else {
    if (e->options & upb_JsonEncode_UseProtoNames) {
      name = upb_FieldDef_Name(f);
    } else {
      name = upb_FieldDef_JsonName(f);
    }
    jsonenc_printf(e, "\"%s\":", name);
  }

  if (upb_FieldDef_IsMap(f)) {
    jsonenc_map(e, val.map_val, f);
  } else if (upb_FieldDef_IsRepeated(f)) {
    jsonenc_array(e, val.array_val, f);
  } else {
    jsonenc_scalar(e, val, f);
  }
}

static void jsonenc_msgfields(jsonenc* e, const upb_Message* msg,
                              const upb_MessageDef* m, bool first) {
  upb_MessageValue val;
  const upb_FieldDef* f;

  if (e->options & upb_JsonEncode_EmitDefaults) {
    /* Iterate over all fields. */
    int i = 0;
    int n = upb_MessageDef_FieldCount(m);
    for (i = 0; i < n; i++) {
      f = upb_MessageDef_Field(m, i);
      if (!upb_FieldDef_HasPresence(f) || upb_Message_Has(msg, f)) {
        jsonenc_fieldval(e, f, upb_Message_Get(msg, f), &first);
      }
    }
  } else {
    /* Iterate over non-empty fields. */
    size_t iter = kUpb_Message_Begin;
    while (upb_Message_Next(msg, m, e->ext_pool, &f, &val, &iter)) {
      jsonenc_fieldval(e, f, val, &first);
    }
  }
}

static void jsonenc_msg(jsonenc* e, const upb_Message* msg,
                        const upb_MessageDef* m) {
  jsonenc_putstr(e, "{");
  jsonenc_msgfields(e, msg, m, true);
  jsonenc_putstr(e, "}");
}

static size_t jsonenc_nullz(jsonenc* e, size_t size) {
  size_t ret = e->ptr - e->buf + e->overflow;

  if (size > 0) {
    if (e->ptr == e->end) e->ptr--;
    *e->ptr = '\0';
  }

  return ret;
}

size_t upb_JsonEncode(const upb_Message* msg, const upb_MessageDef* m,
                      const upb_DefPool* ext_pool, int options, char* buf,
                      size_t size, upb_Status* status) {
  jsonenc e;

  e.buf = buf;
  e.ptr = buf;
  e.end = UPB_PTRADD(buf, size);
  e.overflow = 0;
  e.options = options;
  e.ext_pool = ext_pool;
  e.status = status;
  e.arena = NULL;

  if (setjmp(e.err)) return -1;

  jsonenc_msgfield(&e, msg, m);
  if (e.arena) upb_Arena_Free(e.arena);
  return jsonenc_nullz(&e, size);
}

/** upb/mini_table.c ************************************************************/

#include <inttypes.h>
#include <setjmp.h>


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
  kUpb_EncodedType_Enum = 12,
  kUpb_EncodedType_Bool = 13,
  kUpb_EncodedType_Bytes = 14,
  kUpb_EncodedType_String = 15,
  kUpb_EncodedType_Group = 16,
  kUpb_EncodedType_Message = 17,

  kUpb_EncodedType_RepeatedBase = 20,
} upb_EncodedType;

typedef enum {
  kUpb_EncodedFieldModifier_FlipPacked = 1 << 0,
  kUpb_EncodedFieldModifier_IsClosedEnum = 1 << 1,
  // upb only.
  kUpb_EncodedFieldModifier_IsProto3Singular = 1 << 2,
  kUpb_EncodedFieldModifier_IsRequired = 1 << 3,
} upb_EncodedFieldModifier;

enum {
  kUpb_EncodedValue_MinField = ' ',
  kUpb_EncodedValue_MaxField = 'K',
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

char upb_ToBase92(int8_t ch) {
  static const char kUpb_ToBase92[] = {
      ' ', '!', '#', '$', '%', '&', '(', ')', '*', '+', ',', '-', '.', '/',
      '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', ';', '<', '=',
      '>', '?', '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K',
      'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y',
      'Z', '[', ']', '^', '_', '`', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
      'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
      'w', 'x', 'y', 'z', '{', '|', '}', '~',
  };

  UPB_ASSERT(0 <= ch && ch < 92);
  return kUpb_ToBase92[ch];
}

char upb_FromBase92(uint8_t ch) {
  static const int8_t kUpb_FromBase92[] = {
      0,  1,  -1, 2,  3,  4,  5,  -1, 6,  7,  8,  9,  10, 11, 12, 13,
      14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
      30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45,
      46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, -1, 58, 59, 60,
      61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76,
      77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91,
  };

  if (' ' > ch || ch > '~') return -1;
  return kUpb_FromBase92[ch - ' '];
}

bool upb_IsTypePackable(upb_FieldType type) {
  // clang-format off
  static const unsigned kUnpackableTypes =
      (1 << kUpb_FieldType_String) |
      (1 << kUpb_FieldType_Bytes) |
      (1 << kUpb_FieldType_Message) |
      (1 << kUpb_FieldType_Group);
  // clang-format on
  return (1 << type) & ~kUnpackableTypes;
}

/** upb_MtDataEncoder *********************************************************/

typedef struct {
  uint64_t present_values_mask;
  uint32_t last_written_value;
} upb_MtDataEncoderInternal_EnumState;

typedef struct {
  uint64_t msg_modifiers;
  uint32_t last_field_num;
  enum {
    kUpb_OneofState_NotStarted,
    kUpb_OneofState_StartedOneof,
    kUpb_OneofState_EmittedOneofField,
  } oneof_state;
} upb_MtDataEncoderInternal_MsgState;

typedef struct {
  char* buf_start;  // Only for checking kUpb_MtDataEncoder_MinSize.
  union {
    upb_MtDataEncoderInternal_EnumState enum_state;
    upb_MtDataEncoderInternal_MsgState msg_state;
  } state;
} upb_MtDataEncoderInternal;

static upb_MtDataEncoderInternal* upb_MtDataEncoder_GetInternal(
    upb_MtDataEncoder* e, char* buf_start) {
  UPB_ASSERT(sizeof(upb_MtDataEncoderInternal) <= sizeof(e->internal));
  upb_MtDataEncoderInternal* ret = (upb_MtDataEncoderInternal*)e->internal;
  ret->buf_start = buf_start;
  return ret;
}

static char* upb_MtDataEncoder_Put(upb_MtDataEncoder* e, char* ptr, char ch) {
  upb_MtDataEncoderInternal* in = (upb_MtDataEncoderInternal*)e->internal;
  UPB_ASSERT(ptr - in->buf_start < kUpb_MtDataEncoder_MinSize);
  if (ptr == e->end) return NULL;
  *ptr++ = upb_ToBase92(ch);
  return ptr;
}

static char* upb_MtDataEncoder_PutBase92Varint(upb_MtDataEncoder* e, char* ptr,
                                               uint32_t val, int min, int max) {
  int shift = _upb_Log2Ceiling(upb_FromBase92(max) - upb_FromBase92(min) + 1);
  UPB_ASSERT(shift <= 6);
  uint32_t mask = (1 << shift) - 1;
  do {
    uint32_t bits = val & mask;
    ptr = upb_MtDataEncoder_Put(e, ptr, bits + upb_FromBase92(min));
    if (!ptr) return NULL;
    val >>= shift;
  } while (val);
  return ptr;
}

char* upb_MtDataEncoder_PutModifier(upb_MtDataEncoder* e, char* ptr,
                                    uint64_t mod) {
  if (mod) {
    ptr = upb_MtDataEncoder_PutBase92Varint(e, ptr, mod,
                                            kUpb_EncodedValue_MinModifier,
                                            kUpb_EncodedValue_MaxModifier);
  }
  return ptr;
}

char* upb_MtDataEncoder_StartMessage(upb_MtDataEncoder* e, char* ptr,
                                     uint64_t msg_mod) {
  upb_MtDataEncoderInternal* in = upb_MtDataEncoder_GetInternal(e, ptr);
  in->state.msg_state.msg_modifiers = msg_mod;
  in->state.msg_state.last_field_num = 0;
  in->state.msg_state.oneof_state = kUpb_OneofState_NotStarted;
  return upb_MtDataEncoder_PutModifier(e, ptr, msg_mod);
}

char* upb_MtDataEncoder_PutField(upb_MtDataEncoder* e, char* ptr,
                                 upb_FieldType type, uint32_t field_num,
                                 uint64_t field_mod) {
  static const char kUpb_TypeToEncoded[] = {
      [kUpb_FieldType_Double] = kUpb_EncodedType_Double,
      [kUpb_FieldType_Float] = kUpb_EncodedType_Float,
      [kUpb_FieldType_Int64] = kUpb_EncodedType_Int64,
      [kUpb_FieldType_UInt64] = kUpb_EncodedType_UInt64,
      [kUpb_FieldType_Int32] = kUpb_EncodedType_Int32,
      [kUpb_FieldType_Fixed64] = kUpb_EncodedType_Fixed64,
      [kUpb_FieldType_Fixed32] = kUpb_EncodedType_Fixed32,
      [kUpb_FieldType_Bool] = kUpb_EncodedType_Bool,
      [kUpb_FieldType_String] = kUpb_EncodedType_String,
      [kUpb_FieldType_Group] = kUpb_EncodedType_Group,
      [kUpb_FieldType_Message] = kUpb_EncodedType_Message,
      [kUpb_FieldType_Bytes] = kUpb_EncodedType_Bytes,
      [kUpb_FieldType_UInt32] = kUpb_EncodedType_UInt32,
      [kUpb_FieldType_Enum] = kUpb_EncodedType_Enum,
      [kUpb_FieldType_SFixed32] = kUpb_EncodedType_SFixed32,
      [kUpb_FieldType_SFixed64] = kUpb_EncodedType_SFixed64,
      [kUpb_FieldType_SInt32] = kUpb_EncodedType_SInt32,
      [kUpb_FieldType_SInt64] = kUpb_EncodedType_SInt64,
  };

  upb_MtDataEncoderInternal* in = upb_MtDataEncoder_GetInternal(e, ptr);
  if (field_num <= in->state.msg_state.last_field_num) return NULL;
  if (in->state.msg_state.last_field_num + 1 != field_num) {
    // Put skip.
    UPB_ASSERT(field_num > in->state.msg_state.last_field_num);
    uint32_t skip = field_num - in->state.msg_state.last_field_num;
    ptr = upb_MtDataEncoder_PutBase92Varint(
        e, ptr, skip, kUpb_EncodedValue_MinSkip, kUpb_EncodedValue_MaxSkip);
    if (!ptr) return NULL;
  }
  in->state.msg_state.last_field_num = field_num;

  uint32_t encoded_modifiers = 0;

  // Put field type.
  if (type == kUpb_FieldType_Enum &&
      !(field_mod & kUpb_FieldModifier_IsClosedEnum)) {
    type = kUpb_FieldType_Int32;
  }

  int encoded_type = kUpb_TypeToEncoded[type];
  if (field_mod & kUpb_FieldModifier_IsRepeated) {
    // Repeated fields shift the type number up (unlike other modifiers which
    // are bit flags).
    encoded_type += kUpb_EncodedType_RepeatedBase;

    if (upb_IsTypePackable(type)) {
      bool field_is_packed = field_mod & kUpb_FieldModifier_IsPacked;
      bool default_is_packed = in->state.msg_state.msg_modifiers &
                               kUpb_MessageModifier_DefaultIsPacked;
      if (field_is_packed != default_is_packed) {
        encoded_modifiers |= kUpb_EncodedFieldModifier_FlipPacked;
      }
    }
  }
  ptr = upb_MtDataEncoder_Put(e, ptr, encoded_type);
  if (!ptr) return NULL;

  if (field_mod & kUpb_FieldModifier_IsProto3Singular) {
    encoded_modifiers |= kUpb_EncodedFieldModifier_IsProto3Singular;
  }
  if (field_mod & kUpb_FieldModifier_IsRequired) {
    encoded_modifiers |= kUpb_EncodedFieldModifier_IsRequired;
  }
  return upb_MtDataEncoder_PutModifier(e, ptr, encoded_modifiers);
}

char* upb_MtDataEncoder_StartOneof(upb_MtDataEncoder* e, char* ptr) {
  upb_MtDataEncoderInternal* in = upb_MtDataEncoder_GetInternal(e, ptr);
  if (in->state.msg_state.oneof_state == kUpb_OneofState_NotStarted) {
    ptr = upb_MtDataEncoder_Put(e, ptr, upb_FromBase92(kUpb_EncodedValue_End));
  } else {
    ptr = upb_MtDataEncoder_Put(
        e, ptr, upb_FromBase92(kUpb_EncodedValue_OneofSeparator));
  }
  in->state.msg_state.oneof_state = kUpb_OneofState_StartedOneof;
  return ptr;
}

char* upb_MtDataEncoder_PutOneofField(upb_MtDataEncoder* e, char* ptr,
                                      uint32_t field_num) {
  upb_MtDataEncoderInternal* in = upb_MtDataEncoder_GetInternal(e, ptr);
  if (in->state.msg_state.oneof_state == kUpb_OneofState_EmittedOneofField) {
    ptr = upb_MtDataEncoder_Put(
        e, ptr, upb_FromBase92(kUpb_EncodedValue_FieldSeparator));
    if (!ptr) return NULL;
  }
  ptr = upb_MtDataEncoder_PutBase92Varint(e, ptr, field_num, upb_ToBase92(0),
                                          upb_ToBase92(63));
  in->state.msg_state.oneof_state = kUpb_OneofState_EmittedOneofField;
  return ptr;
}

void upb_MtDataEncoder_StartEnum(upb_MtDataEncoder* e) {
  upb_MtDataEncoderInternal* in = upb_MtDataEncoder_GetInternal(e, NULL);
  in->state.enum_state.present_values_mask = 0;
  in->state.enum_state.last_written_value = 0;
}

static char* upb_MtDataEncoder_FlushDenseEnumMask(upb_MtDataEncoder* e,
                                                  char* ptr) {
  upb_MtDataEncoderInternal* in = (upb_MtDataEncoderInternal*)e->internal;
  ptr = upb_MtDataEncoder_Put(e, ptr, in->state.enum_state.present_values_mask);
  in->state.enum_state.present_values_mask = 0;
  in->state.enum_state.last_written_value += 5;
  return ptr;
}

char* upb_MtDataEncoder_PutEnumValue(upb_MtDataEncoder* e, char* ptr,
                                     uint32_t val) {
  // TODO(b/229641772): optimize this encoding.
  upb_MtDataEncoderInternal* in = upb_MtDataEncoder_GetInternal(e, ptr);
  UPB_ASSERT(val >= in->state.enum_state.last_written_value);
  uint32_t delta = val - in->state.enum_state.last_written_value;
  if (delta >= 5 && in->state.enum_state.present_values_mask) {
    ptr = upb_MtDataEncoder_FlushDenseEnumMask(e, ptr);
    delta -= 5;
  }

  if (delta >= 5) {
    ptr = upb_MtDataEncoder_PutBase92Varint(
        e, ptr, delta, kUpb_EncodedValue_MinSkip, kUpb_EncodedValue_MaxSkip);
    in->state.enum_state.last_written_value += delta;
    delta = 0;
  }

  UPB_ASSERT((in->state.enum_state.present_values_mask >> delta) == 0);
  in->state.enum_state.present_values_mask |= 1ULL << delta;
  return ptr;
}

char* upb_MtDataEncoder_EndEnum(upb_MtDataEncoder* e, char* ptr) {
  upb_MtDataEncoderInternal* in = upb_MtDataEncoder_GetInternal(e, ptr);
  if (!in->state.enum_state.present_values_mask) return ptr;
  return upb_MtDataEncoder_FlushDenseEnumMask(e, ptr);
}

const upb_MiniTable_Field* upb_MiniTable_FindFieldByNumber(
    const upb_MiniTable* table, uint32_t number) {
  int n = table->field_count;
  for (int i = 0; i < n; i++) {
    if (table->fields[i].number == number) {
      return &table->fields[i];
    }
  }
  return NULL;
}

/** Data decoder **************************************************************/

// Note: we sort by this number when calculating layout order.
typedef enum {
  kUpb_LayoutItemType_OneofCase,   // Oneof case.
  kUpb_LayoutItemType_OneofField,  // Oneof field data.
  kUpb_LayoutItemType_Field,       // Non-oneof field data.

  kUpb_LayoutItemType_Max = kUpb_LayoutItemType_Field,
} upb_LayoutItemType;

#define kUpb_LayoutItem_IndexSentinel ((uint16_t)-1)

typedef struct {
  // Index of the corresponding field.  When this is a oneof field, the field's
  // offset will be the index of the next field in a linked list.
  uint16_t field_index;
  uint16_t offset;
  upb_FieldRep rep;
  upb_LayoutItemType type;
} upb_LayoutItem;

typedef struct {
  upb_LayoutItem* data;
  size_t size;
  size_t capacity;
} upb_LayoutItemVector;

typedef struct {
  const char* end;
  upb_MiniTable* table;
  upb_MiniTable_Field* fields;
  upb_MiniTablePlatform platform;
  upb_LayoutItemVector vec;
  upb_Arena* arena;
  upb_Status* status;
  jmp_buf err;
} upb_MtDecoder;

UPB_PRINTF(2, 3)
UPB_NORETURN static void upb_MtDecoder_ErrorFormat(upb_MtDecoder* d,
                                                   const char* fmt, ...) {
  va_list argp;
  upb_Status_SetErrorMessage(d->status, "Error building mini table: ");
  va_start(argp, fmt);
  upb_Status_VAppendErrorFormat(d->status, fmt, argp);
  va_end(argp);
  UPB_LONGJMP(d->err, 1);
}

static void upb_MtDecoder_CheckOutOfMemory(upb_MtDecoder* d, const void* ptr) {
  if (!ptr) upb_MtDecoder_ErrorFormat(d, "Out of memory");
}

// In each field's offset, we temporarily store a presence classifier:
enum PresenceClass {
  kNoPresence = 0,
  kHasbitPresence = 1,
  kRequiredPresence = 2,
  kOneofBase = 3,
  // Negative values refer to a specific oneof with that number.  Positive
  // values >= kOneofBase indicate that this field is in a oneof, and specify
  // the next field in this oneof's linked list.
};

static const char* upb_MiniTable_DecodeBase92Varint(upb_MtDecoder* d,
                                                    const char* ptr,
                                                    char first_ch, uint8_t min,
                                                    uint8_t max,
                                                    uint32_t* out_val) {
  uint32_t val = 0;
  uint32_t shift = 0;
  const int bits_per_char =
      _upb_Log2Ceiling(upb_FromBase92(max) - upb_FromBase92(min));
  char ch = first_ch;
  while (1) {
    uint32_t bits = upb_FromBase92(ch) - upb_FromBase92(min);
    UPB_ASSERT(shift < 32);
    val |= bits << shift;
    if (ptr == d->end || *ptr < min || max < *ptr) {
      *out_val = val;
      return ptr;
    }
    ch = *ptr++;
    shift += bits_per_char;
  }
}

static bool upb_MiniTable_HasSub(upb_MiniTable_Field* field,
                                 uint64_t msg_modifiers) {
  switch (field->descriptortype) {
    case kUpb_FieldType_Message:
    case kUpb_FieldType_Group:
    case kUpb_FieldType_Enum:
      return true;
    case kUpb_FieldType_String:
      if (!(msg_modifiers & kUpb_MessageModifier_ValidateUtf8)) {
        field->descriptortype = kUpb_FieldType_Bytes;
      }
      return false;
    default:
      return false;
  }
}

static bool upb_MtDecoder_FieldIsPackable(upb_MiniTable_Field* field) {
  return (field->mode & kUpb_FieldMode_Array) &&
         upb_IsTypePackable(field->descriptortype);
}

static void upb_MiniTable_SetTypeAndSub(upb_MiniTable_Field* field,
                                        upb_FieldType type, uint32_t* sub_count,
                                        uint64_t msg_modifiers) {
  field->descriptortype = type;
  if (upb_MiniTable_HasSub(field, msg_modifiers)) {
    field->submsg_index = sub_count ? (*sub_count)++ : 0;
  } else {
    field->submsg_index = kUpb_NoSub;
  }

  if (upb_MtDecoder_FieldIsPackable(field) &&
      (msg_modifiers & kUpb_MessageModifier_DefaultIsPacked)) {
    field->mode |= kUpb_LabelFlags_IsPacked;
  }
}

static void upb_MiniTable_SetField(upb_MtDecoder* d, uint8_t ch,
                                   upb_MiniTable_Field* field,
                                   uint64_t msg_modifiers,
                                   uint32_t* sub_count) {
  static const char kUpb_EncodedToFieldRep[] = {
      [kUpb_EncodedType_Double] = kUpb_FieldRep_8Byte,
      [kUpb_EncodedType_Float] = kUpb_FieldRep_4Byte,
      [kUpb_EncodedType_Int64] = kUpb_FieldRep_8Byte,
      [kUpb_EncodedType_UInt64] = kUpb_FieldRep_8Byte,
      [kUpb_EncodedType_Int32] = kUpb_FieldRep_4Byte,
      [kUpb_EncodedType_Fixed64] = kUpb_FieldRep_8Byte,
      [kUpb_EncodedType_Fixed32] = kUpb_FieldRep_4Byte,
      [kUpb_EncodedType_Bool] = kUpb_FieldRep_1Byte,
      [kUpb_EncodedType_String] = kUpb_FieldRep_StringView,
      [kUpb_EncodedType_Group] = kUpb_FieldRep_Pointer,
      [kUpb_EncodedType_Message] = kUpb_FieldRep_Pointer,
      [kUpb_EncodedType_Bytes] = kUpb_FieldRep_StringView,
      [kUpb_EncodedType_UInt32] = kUpb_FieldRep_4Byte,
      [kUpb_EncodedType_Enum] = kUpb_FieldRep_4Byte,
      [kUpb_EncodedType_SFixed32] = kUpb_FieldRep_4Byte,
      [kUpb_EncodedType_SFixed64] = kUpb_FieldRep_8Byte,
      [kUpb_EncodedType_SInt32] = kUpb_FieldRep_4Byte,
      [kUpb_EncodedType_SInt64] = kUpb_FieldRep_8Byte,
  };

  static const char kUpb_EncodedToType[] = {
      [kUpb_EncodedType_Double] = kUpb_FieldType_Double,
      [kUpb_EncodedType_Float] = kUpb_FieldType_Float,
      [kUpb_EncodedType_Int64] = kUpb_FieldType_Int64,
      [kUpb_EncodedType_UInt64] = kUpb_FieldType_UInt64,
      [kUpb_EncodedType_Int32] = kUpb_FieldType_Int32,
      [kUpb_EncodedType_Fixed64] = kUpb_FieldType_Fixed64,
      [kUpb_EncodedType_Fixed32] = kUpb_FieldType_Fixed32,
      [kUpb_EncodedType_Bool] = kUpb_FieldType_Bool,
      [kUpb_EncodedType_String] = kUpb_FieldType_String,
      [kUpb_EncodedType_Group] = kUpb_FieldType_Group,
      [kUpb_EncodedType_Message] = kUpb_FieldType_Message,
      [kUpb_EncodedType_Bytes] = kUpb_FieldType_Bytes,
      [kUpb_EncodedType_UInt32] = kUpb_FieldType_UInt32,
      [kUpb_EncodedType_Enum] = kUpb_FieldType_Enum,
      [kUpb_EncodedType_SFixed32] = kUpb_FieldType_SFixed32,
      [kUpb_EncodedType_SFixed64] = kUpb_FieldType_SFixed64,
      [kUpb_EncodedType_SInt32] = kUpb_FieldType_SInt32,
      [kUpb_EncodedType_SInt64] = kUpb_FieldType_SInt64,
  };

  int8_t type = upb_FromBase92(ch);
  if (ch >= upb_ToBase92(kUpb_EncodedType_RepeatedBase)) {
    type -= kUpb_EncodedType_RepeatedBase;
    field->mode = kUpb_FieldMode_Array;
    field->mode |= kUpb_FieldRep_Pointer << kUpb_FieldRep_Shift;
    field->offset = kNoPresence;
  } else {
    field->mode = kUpb_FieldMode_Scalar;
    field->mode |= kUpb_EncodedToFieldRep[type] << kUpb_FieldRep_Shift;
    field->offset = kHasbitPresence;
  }
  if (type >= 18) {
    upb_MtDecoder_ErrorFormat(d, "Invalid field type: %d", (int)type);
    UPB_UNREACHABLE();
  }
  upb_MiniTable_SetTypeAndSub(field, kUpb_EncodedToType[type], sub_count,
                              msg_modifiers);
}

static void upb_MtDecoder_ModifyField(upb_MtDecoder* d,
                                      uint32_t message_modifiers,
                                      uint32_t field_modifiers,
                                      upb_MiniTable_Field* field) {
  if (field_modifiers & kUpb_EncodedFieldModifier_FlipPacked) {
    if (!upb_MtDecoder_FieldIsPackable(field)) {
      upb_MtDecoder_ErrorFormat(
          d, "Cannot flip packed on unpackable field %" PRIu32, field->number);
      UPB_UNREACHABLE();
    }
    field->mode ^= kUpb_LabelFlags_IsPacked;
  }

  bool singular = field_modifiers & kUpb_EncodedFieldModifier_IsProto3Singular;
  bool required = field_modifiers & kUpb_EncodedFieldModifier_IsRequired;

  // Validate.
  if ((singular || required) && field->offset != kHasbitPresence) {
    upb_MtDecoder_ErrorFormat(
        d, "Invalid modifier(s) for repeated field %" PRIu32, field->number);
    UPB_UNREACHABLE();
  }
  if (singular && required) {
    upb_MtDecoder_ErrorFormat(
        d, "Field %" PRIu32 " cannot be both singular and required",
        field->number);
    UPB_UNREACHABLE();
  }

  if (singular) field->offset = kNoPresence;
  if (required) {
    field->offset = kRequiredPresence;
  }
}

static void upb_MtDecoder_PushItem(upb_MtDecoder* d, upb_LayoutItem item) {
  if (d->vec.size == d->vec.capacity) {
    size_t new_cap = UPB_MAX(8, d->vec.size * 2);
    d->vec.data = realloc(d->vec.data, new_cap * sizeof(*d->vec.data));
    upb_MtDecoder_CheckOutOfMemory(d, d->vec.data);
    d->vec.capacity = new_cap;
  }
  d->vec.data[d->vec.size++] = item;
}

static void upb_MtDecoder_PushOneof(upb_MtDecoder* d, upb_LayoutItem item) {
  if (item.field_index == kUpb_LayoutItem_IndexSentinel) {
    upb_MtDecoder_ErrorFormat(d, "Empty oneof");
    UPB_UNREACHABLE();
  }
  item.field_index -= kOneofBase;

  // Push oneof data.
  item.type = kUpb_LayoutItemType_OneofField;
  upb_MtDecoder_PushItem(d, item);

  // Push oneof case.
  item.rep = kUpb_FieldRep_4Byte;  // Field Number.
  item.type = kUpb_LayoutItemType_OneofCase;
  upb_MtDecoder_PushItem(d, item);
}

size_t upb_MtDecoder_SizeOfRep(upb_FieldRep rep,
                               upb_MiniTablePlatform platform) {
  static const uint8_t kRepToSize32[] = {
      [kUpb_FieldRep_1Byte] = 1,   [kUpb_FieldRep_4Byte] = 4,
      [kUpb_FieldRep_Pointer] = 4, [kUpb_FieldRep_StringView] = 8,
      [kUpb_FieldRep_8Byte] = 8,
  };
  static const uint8_t kRepToSize64[] = {
      [kUpb_FieldRep_1Byte] = 1,   [kUpb_FieldRep_4Byte] = 4,
      [kUpb_FieldRep_Pointer] = 8, [kUpb_FieldRep_StringView] = 16,
      [kUpb_FieldRep_8Byte] = 8,
  };
  UPB_ASSERT(sizeof(upb_StringView) ==
             UPB_SIZE(kRepToSize32, kRepToSize64)[kUpb_FieldRep_StringView]);
  return platform == kUpb_MiniTablePlatform_32Bit ? kRepToSize32[rep]
                                                  : kRepToSize64[rep];
}

size_t upb_MtDecoder_AlignOfRep(upb_FieldRep rep,
                                upb_MiniTablePlatform platform) {
  static const uint8_t kRepToAlign32[] = {
      [kUpb_FieldRep_1Byte] = 1,   [kUpb_FieldRep_4Byte] = 4,
      [kUpb_FieldRep_Pointer] = 4, [kUpb_FieldRep_StringView] = 4,
      [kUpb_FieldRep_8Byte] = 8,
  };
  static const uint8_t kRepToAlign64[] = {
      [kUpb_FieldRep_1Byte] = 1,   [kUpb_FieldRep_4Byte] = 4,
      [kUpb_FieldRep_Pointer] = 8, [kUpb_FieldRep_StringView] = 8,
      [kUpb_FieldRep_8Byte] = 8,
  };
  UPB_ASSERT(UPB_ALIGN_OF(upb_StringView) ==
             UPB_SIZE(kRepToAlign32, kRepToAlign64)[kUpb_FieldRep_StringView]);
  return platform == kUpb_MiniTablePlatform_32Bit ? kRepToAlign32[rep]
                                                  : kRepToAlign64[rep];
}

static const char* upb_MtDecoder_DecodeOneofField(upb_MtDecoder* d,
                                                  const char* ptr,
                                                  char first_ch,
                                                  upb_LayoutItem* item) {
  uint32_t field_num;
  ptr = upb_MiniTable_DecodeBase92Varint(
      d, ptr, first_ch, kUpb_EncodedValue_MinOneofField,
      kUpb_EncodedValue_MaxOneofField, &field_num);
  upb_MiniTable_Field* f =
      (void*)upb_MiniTable_FindFieldByNumber(d->table, field_num);

  if (!f) {
    upb_MtDecoder_ErrorFormat(d,
                              "Couldn't add field number %" PRIu32
                              " to oneof, no such field number.",
                              field_num);
    UPB_UNREACHABLE();
  }
  if (f->offset != kHasbitPresence) {
    upb_MtDecoder_ErrorFormat(
        d,
        "Cannot add repeated, required, or singular field %" PRIu32
        " to oneof.",
        field_num);
    UPB_UNREACHABLE();
  }

  // Oneof storage must be large enough to accommodate the largest member.
  int rep = f->mode >> kUpb_FieldRep_Shift;
  if (upb_MtDecoder_SizeOfRep(rep, d->platform) >
      upb_MtDecoder_SizeOfRep(item->rep, d->platform)) {
    item->rep = rep;
  }
  // Prepend this field to the linked list.
  f->offset = item->field_index;
  item->field_index = (f - d->fields) + kOneofBase;
  return ptr;
}

static const char* upb_MtDecoder_DecodeOneofs(upb_MtDecoder* d,
                                              const char* ptr) {
  upb_LayoutItem item = {.rep = 0,
                         .field_index = kUpb_LayoutItem_IndexSentinel};
  while (ptr < d->end) {
    char ch = *ptr++;
    if (ch == kUpb_EncodedValue_FieldSeparator) {
      // Field separator, no action needed.
    } else if (ch == kUpb_EncodedValue_OneofSeparator) {
      // End of oneof.
      upb_MtDecoder_PushOneof(d, item);
      item.field_index = kUpb_LayoutItem_IndexSentinel;  // Move to next oneof.
    } else {
      ptr = upb_MtDecoder_DecodeOneofField(d, ptr, ch, &item);
    }
  }

  // Push final oneof.
  upb_MtDecoder_PushOneof(d, item);
  return ptr;
}

static const char* upb_MtDecoder_ParseModifier(upb_MtDecoder* d,
                                               const char* ptr, char first_ch,
                                               upb_MiniTable_Field* last_field,
                                               uint64_t* msg_modifiers) {
  uint32_t mod;
  ptr = upb_MiniTable_DecodeBase92Varint(d, ptr, first_ch,
                                         kUpb_EncodedValue_MinModifier,
                                         kUpb_EncodedValue_MaxModifier, &mod);
  if (last_field) {
    upb_MtDecoder_ModifyField(d, *msg_modifiers, mod, last_field);
  } else {
    if (!d->table) {
      upb_MtDecoder_ErrorFormat(d, "Extensions cannot have message modifiers");
      UPB_UNREACHABLE();
    }
    *msg_modifiers = mod;
  }

  return ptr;
}

static void upb_MtDecoder_AllocateSubs(upb_MtDecoder* d, uint32_t sub_count) {
  size_t subs_bytes = sizeof(*d->table->subs) * sub_count;
  d->table->subs = upb_Arena_Malloc(d->arena, subs_bytes);
  upb_MtDecoder_CheckOutOfMemory(d, d->table->subs);
}

static void upb_MtDecoder_Parse(upb_MtDecoder* d, const char* ptr, size_t len,
                                void* fields, size_t field_size,
                                uint16_t* field_count, uint32_t* sub_count) {
  uint64_t msg_modifiers = 0;
  uint32_t last_field_number = 0;
  upb_MiniTable_Field* last_field = NULL;
  bool need_dense_below = d->table != NULL;

  d->end = UPB_PTRADD(ptr, len);

  while (ptr < d->end) {
    char ch = *ptr++;
    if (ch <= kUpb_EncodedValue_MaxField) {
      upb_MiniTable_Field* field = fields;
      *field_count += 1;
      fields = (char*)fields + field_size;
      field->number = ++last_field_number;
      last_field = field;
      upb_MiniTable_SetField(d, ch, field, msg_modifiers, sub_count);
    } else if (kUpb_EncodedValue_MinModifier <= ch &&
               ch <= kUpb_EncodedValue_MaxModifier) {
      ptr = upb_MtDecoder_ParseModifier(d, ptr, ch, last_field, &msg_modifiers);
      if (msg_modifiers & kUpb_MessageModifier_IsExtendable) {
        d->table->ext |= kUpb_ExtMode_Extendable;
      }
    } else if (ch == kUpb_EncodedValue_End) {
      if (!d->table) {
        upb_MtDecoder_ErrorFormat(d, "Extensions cannot have oneofs.");
        UPB_UNREACHABLE();
      }
      ptr = upb_MtDecoder_DecodeOneofs(d, ptr);
    } else if (kUpb_EncodedValue_MinSkip <= ch &&
               ch <= kUpb_EncodedValue_MaxSkip) {
      if (need_dense_below) {
        d->table->dense_below = d->table->field_count;
        need_dense_below = false;
      }
      uint32_t skip;
      ptr = upb_MiniTable_DecodeBase92Varint(d, ptr, ch,
                                             kUpb_EncodedValue_MinSkip,
                                             kUpb_EncodedValue_MaxSkip, &skip);
      last_field_number += skip;
      last_field_number--;  // Next field seen will increment.
    }
  }

  if (need_dense_below) {
    d->table->dense_below = d->table->field_count;
  }
}

static void upb_MtDecoder_ParseMessage(upb_MtDecoder* d, const char* data,
                                       size_t len) {
  // Buffer length is an upper bound on the number of fields. We will return
  // what we don't use.
  d->fields = upb_Arena_Malloc(d->arena, sizeof(*d->fields) * len);
  upb_MtDecoder_CheckOutOfMemory(d, d->fields);

  uint32_t sub_count = 0;
  d->table->field_count = 0;
  d->table->fields = d->fields;
  upb_MtDecoder_Parse(d, data, len, d->fields, sizeof(*d->fields),
                      &d->table->field_count, &sub_count);

  upb_Arena_ShrinkLast(d->arena, d->fields, sizeof(*d->fields) * len,
                       sizeof(*d->fields) * d->table->field_count);
  d->table->fields = d->fields;
  upb_MtDecoder_AllocateSubs(d, sub_count);
}

int upb_MtDecoder_CompareFields(const void* _a, const void* _b) {
  const upb_LayoutItem* a = _a;
  const upb_LayoutItem* b = _b;
  // Currently we just sort by:
  //  1. rep (smallest fields first)
  //  2. type (oneof cases first)
  //  2. field_index (smallest numbers first)
  // The main goal of this is to reduce space lost to padding.
  // Later we may have more subtle reasons to prefer a different ordering.
  const int rep_bits = _upb_Log2Ceiling(kUpb_FieldRep_Max);
  const int type_bits = _upb_Log2Ceiling(kUpb_LayoutItemType_Max);
  const int idx_bits = (sizeof(a->field_index) * 8);
  UPB_ASSERT(idx_bits + rep_bits + type_bits < 32);
#define UPB_COMBINE(rep, ty, idx) (((rep << type_bits) | ty) << idx_bits) | idx
  uint32_t a_packed = UPB_COMBINE(a->rep, a->type, a->field_index);
  uint32_t b_packed = UPB_COMBINE(b->rep, b->type, b->field_index);
  assert(a_packed != b_packed);
#undef UPB_COMBINE
  return a_packed < b_packed ? -1 : 1;
}

static bool upb_MtDecoder_SortLayoutItems(upb_MtDecoder* d) {
  // Add items for all non-oneof fields (oneofs were already added).
  int n = d->table->field_count;
  for (int i = 0; i < n; i++) {
    upb_MiniTable_Field* f = &d->fields[i];
    if (f->offset >= kOneofBase) continue;
    upb_LayoutItem item = {.field_index = i,
                           .rep = f->mode >> kUpb_FieldRep_Shift,
                           .type = kUpb_LayoutItemType_Field};
    upb_MtDecoder_PushItem(d, item);
  }

  if (d->vec.size) {
    qsort(d->vec.data, d->vec.size, sizeof(*d->vec.data),
          upb_MtDecoder_CompareFields);
  }

  return true;
}

static size_t upb_MiniTable_DivideRoundUp(size_t n, size_t d) {
  return (n + d - 1) / d;
}

static void upb_MtDecoder_AssignHasbits(upb_MiniTable* ret) {
  int n = ret->field_count;
  int last_hasbit = 0;  // 0 cannot be used.

  // First assign required fields, which must have the lowest hasbits.
  for (int i = 0; i < n; i++) {
    upb_MiniTable_Field* field = (upb_MiniTable_Field*)&ret->fields[i];
    if (field->offset == kRequiredPresence) {
      field->presence = ++last_hasbit;
    } else if (field->offset == kNoPresence) {
      field->presence = 0;
    }
  }
  ret->required_count = last_hasbit;

  // Next assign non-required hasbit fields.
  for (int i = 0; i < n; i++) {
    upb_MiniTable_Field* field = (upb_MiniTable_Field*)&ret->fields[i];
    if (field->offset == kHasbitPresence) {
      field->presence = ++last_hasbit;
    }
  }

  ret->size = last_hasbit ? upb_MiniTable_DivideRoundUp(last_hasbit + 1, 8) : 0;
}

size_t upb_MtDecoder_Place(upb_MtDecoder* d, upb_FieldRep rep) {
  size_t size = upb_MtDecoder_SizeOfRep(rep, d->platform);
  size_t align = upb_MtDecoder_AlignOfRep(rep, d->platform);
  size_t ret = UPB_ALIGN_UP(d->table->size, align);
  d->table->size = ret + size;
  return ret;
}

static void upb_MtDecoder_AssignOffsets(upb_MtDecoder* d) {
  upb_LayoutItem* end = UPB_PTRADD(d->vec.data, d->vec.size);

  // Compute offsets.
  for (upb_LayoutItem* item = d->vec.data; item < end; item++) {
    item->offset = upb_MtDecoder_Place(d, item->rep);
  }

  // Assign oneof case offsets.  We must do these first, since assigning
  // actual offsets will overwrite the links of the linked list.
  for (upb_LayoutItem* item = d->vec.data; item < end; item++) {
    if (item->type != kUpb_LayoutItemType_OneofCase) continue;
    upb_MiniTable_Field* f = &d->fields[item->field_index];
    while (true) {
      f->presence = ~item->offset;
      if (f->offset == kUpb_LayoutItem_IndexSentinel) break;
      UPB_ASSERT(f->offset - kOneofBase < d->table->field_count);
      f = &d->fields[f->offset - kOneofBase];
    }
  }

  // Assign offsets.
  for (upb_LayoutItem* item = d->vec.data; item < end; item++) {
    upb_MiniTable_Field* f = &d->fields[item->field_index];
    switch (item->type) {
      case kUpb_LayoutItemType_OneofField:
        while (true) {
          uint16_t next_offset = f->offset;
          f->offset = item->offset;
          if (next_offset == kUpb_LayoutItem_IndexSentinel) break;
          f = &d->fields[next_offset - kOneofBase];
        }
        break;
      case kUpb_LayoutItemType_Field:
        f->offset = item->offset;
        break;
      default:
        break;
    }
  }

  // The fasttable parser (supported on 64-bit only) depends on this being a
  // multiple of 8 in order to satisfy UPB_MALLOC_ALIGN, which is also 8.
  //
  // On 32-bit we could potentially make this smaller, but there is no
  // compelling reason to optimize this right now.
  d->table->size = UPB_ALIGN_UP(d->table->size, 8);
}

upb_MiniTable* upb_MiniTable_BuildWithBuf(const char* data, size_t len,
                                          upb_MiniTablePlatform platform,
                                          upb_Arena* arena, void** buf,
                                          size_t* buf_size,
                                          upb_Status* status) {
  upb_MtDecoder decoder = {
      .platform = platform,
      .vec =
          {
              .data = *buf,
              .capacity = *buf_size / sizeof(*decoder.vec.data),
              .size = 0,
          },
      .arena = arena,
      .status = status,
      .table = upb_Arena_Malloc(arena, sizeof(*decoder.table)),
  };

  if (UPB_SETJMP(decoder.err)) {
    decoder.table = NULL;
    goto done;
  }

  upb_MtDecoder_CheckOutOfMemory(&decoder, decoder.table);

  decoder.table->size = 0;
  decoder.table->field_count = 0;
  decoder.table->ext = kUpb_ExtMode_NonExtendable;
  decoder.table->dense_below = 0;
  decoder.table->table_mask = -1;
  decoder.table->required_count = 0;

  upb_MtDecoder_ParseMessage(&decoder, data, len);
  upb_MtDecoder_AssignHasbits(decoder.table);
  upb_MtDecoder_SortLayoutItems(&decoder);
  upb_MtDecoder_AssignOffsets(&decoder);

done:
  *buf = decoder.vec.data;
  *buf_size = decoder.vec.capacity / sizeof(*decoder.vec.data);
  return decoder.table;
}

upb_MiniTable* upb_MiniTable_BuildMessageSet(upb_MiniTablePlatform platform,
                                             upb_Arena* arena) {
  upb_MiniTable* ret = upb_Arena_Malloc(arena, sizeof(*ret));
  if (!ret) return NULL;

  ret->size = 0;
  ret->field_count = 0;
  ret->ext = kUpb_ExtMode_IsMessageSet;
  ret->dense_below = 0;
  ret->table_mask = -1;
  ret->required_count = 0;
  return ret;
}

upb_MiniTable* upb_MiniTable_BuildMapEntry(upb_FieldType key_type,
                                           upb_FieldType value_type,
                                           bool value_is_proto3_enum,
                                           upb_MiniTablePlatform platform,
                                           upb_Arena* arena) {
  upb_MiniTable* ret = upb_Arena_Malloc(arena, sizeof(*ret));
  upb_MiniTable_Field* fields = upb_Arena_Malloc(arena, sizeof(*fields) * 2);
  if (!ret || !fields) return NULL;

  upb_MiniTable_Sub* subs = NULL;
  if (value_is_proto3_enum) value_type = kUpb_FieldType_Int32;
  if (value_type == kUpb_FieldType_Message ||
      value_type == kUpb_FieldType_Group || value_type == kUpb_FieldType_Enum) {
    subs = upb_Arena_Malloc(arena, sizeof(*subs));
    if (!subs) return NULL;
  }

  size_t field_size =
      upb_MtDecoder_SizeOfRep(kUpb_FieldRep_StringView, platform);

  fields[0].number = 1;
  fields[1].number = 2;
  fields[0].mode = kUpb_FieldMode_Scalar;
  fields[1].mode = kUpb_FieldMode_Scalar;
  fields[0].presence = 0;
  fields[1].presence = 0;
  fields[0].offset = 0;
  fields[1].offset = field_size;

  upb_MiniTable_SetTypeAndSub(&fields[0], key_type, NULL, 0);
  upb_MiniTable_SetTypeAndSub(&fields[1], value_type, NULL, 0);

  ret->size = UPB_ALIGN_UP(2 * field_size, 8);
  ret->field_count = 2;
  ret->ext = kUpb_ExtMode_NonExtendable | kUpb_ExtMode_IsMapEntry;
  ret->dense_below = 2;
  ret->table_mask = -1;
  ret->required_count = 0;
  ret->subs = subs;
  ret->fields = fields;
  return ret;
}

static bool upb_MiniTable_BuildEnumValue(upb_MtDecoder* d,
                                         upb_MiniTable_Enum* table,
                                         uint32_t val, upb_Arena* arena) {
  if (val < 64) {
    table->mask |= 1ULL << val;
    return true;
  }

  int32_t* values = (void*)table->values;
  values = upb_Arena_Realloc(arena, values, table->value_count * 4,
                             (table->value_count + 1) * 4);
  upb_MtDecoder_CheckOutOfMemory(d, values);
  values[table->value_count++] = (int32_t)val;
  table->values = values;
  return true;
}

upb_MiniTable_Enum* upb_MiniTable_BuildEnum(const char* data, size_t len,
                                            upb_Arena* arena,
                                            upb_Status* status) {
  upb_MtDecoder d = {
      .status = status,
      .end = UPB_PTRADD(data, len),
  };

  if (UPB_SETJMP(d.err)) {
    return NULL;
  }

  upb_MiniTable_Enum* table = upb_Arena_Malloc(arena, sizeof(*table));
  upb_MtDecoder_CheckOutOfMemory(&d, table);

  table->mask = 0;
  table->value_count = 0;
  table->values = NULL;

  const char* ptr = data;
  uint32_t base = 0;

  while (ptr < d.end) {
    char ch = *ptr++;
    if (ch <= kUpb_EncodedValue_MaxEnumMask) {
      uint32_t mask = upb_FromBase92(ch);
      for (int i = 0; i < 5; i++, base++, mask >>= 1) {
        if (mask & 1) {
          if (!upb_MiniTable_BuildEnumValue(&d, table, base, arena)) {
            return NULL;
          }
        }
      }
    } else if (kUpb_EncodedValue_MinSkip <= ch &&
               ch <= kUpb_EncodedValue_MaxSkip) {
      uint32_t skip;
      ptr = upb_MiniTable_DecodeBase92Varint(&d, ptr, ch,
                                             kUpb_EncodedValue_MinSkip,
                                             kUpb_EncodedValue_MaxSkip, &skip);
      base += skip;
    } else {
      upb_Status_SetErrorFormat(status, "Unexpected character: %c", ch);
      return NULL;
    }
  }

  return table;
}

bool upb_MiniTable_BuildExtension(const char* data, size_t len,
                                  upb_MiniTable_Extension* ext,
                                  upb_MiniTable_Sub sub, upb_Status* status) {
  upb_MtDecoder decoder = {
      .arena = NULL,
      .status = status,
      .table = NULL,
  };

  if (UPB_SETJMP(decoder.err)) {
    return false;
  }

  uint16_t count = 0;
  upb_MtDecoder_Parse(&decoder, data, len, ext, sizeof(*ext), &count, NULL);
  ext->field.mode |= kUpb_LabelFlags_IsExtension;
  ext->field.offset = 0;
  return true;
}

upb_MiniTable* upb_MiniTable_Build(const char* data, size_t len,
                                   upb_MiniTablePlatform platform,
                                   upb_Arena* arena, upb_Status* status) {
  void* buf = NULL;
  size_t size = 0;
  upb_MiniTable* ret = upb_MiniTable_BuildWithBuf(data, len, platform, arena,
                                                  &buf, &size, status);
  free(buf);
  return ret;
}

void upb_MiniTable_SetSubMessage(upb_MiniTable* table,
                                 upb_MiniTable_Field* field,
                                 const upb_MiniTable* sub) {
  UPB_ASSERT((uintptr_t)table->fields <= (uintptr_t)field &&
             (uintptr_t)field <
                 (uintptr_t)(table->fields + table->field_count));
  if (sub->ext & kUpb_ExtMode_IsMapEntry) {
    field->mode =
        (kUpb_FieldRep_Pointer << kUpb_FieldRep_Shift) | kUpb_FieldMode_Map;
  }
  upb_MiniTable_Sub* table_sub = (void*)&table->subs[field->submsg_index];
  table_sub->submsg = sub;
}

void upb_MiniTable_SetSubEnum(upb_MiniTable* table, upb_MiniTable_Field* field,
                              const upb_MiniTable_Enum* sub) {
  UPB_ASSERT((uintptr_t)table->fields <= (uintptr_t)field &&
             (uintptr_t)field <
                 (uintptr_t)(table->fields + table->field_count));
  upb_MiniTable_Sub* table_sub = (void*)&table->subs[field->submsg_index];
  table_sub->subenum = sub;
}

/** upb/def.c ************************************************************/

#include <ctype.h>
#include <errno.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>


/* Must be last. */

typedef struct {
  size_t len;
  char str[1]; /* Null-terminated string data follows. */
} str_t;

/* The upb core does not generally have a concept of default instances. However
 * for descriptor options we make an exception since the max size is known and
 * modest (<200 bytes). All types can share a default instance since it is
 * initialized to zeroes.
 *
 * We have to allocate an extra pointer for upb's internal metadata. */
static const char opt_default_buf[_UPB_MAXOPT_SIZE + sizeof(void*)] = {0};
static const char* opt_default = &opt_default_buf[sizeof(void*)];

struct upb_FieldDef {
  const google_protobuf_FieldOptions* opts;
  const upb_FileDef* file;
  const upb_MessageDef* msgdef;
  const char* full_name;
  const char* json_name;
  union {
    int64_t sint;
    uint64_t uint;
    double dbl;
    float flt;
    bool boolean;
    str_t* str;
  } defaultval;
  union {
    const upb_OneofDef* oneof;
    const upb_MessageDef* extension_scope;
  } scope;
  union {
    const upb_MessageDef* msgdef;
    const upb_EnumDef* enumdef;
    const google_protobuf_FieldDescriptorProto* unresolved;
  } sub;
  uint32_t number_;
  uint16_t index_;
  uint16_t layout_index; /* Index into msgdef->layout->fields or file->exts */
  bool has_default;
  bool is_extension_;
  bool packed_;
  bool proto3_optional_;
  bool has_json_name_;
  upb_FieldType type_;
  upb_Label label_;
#if UINTPTR_MAX == 0xffffffff
  uint32_t padding;  // Increase size to a multiple of 8.
#endif
};

struct upb_ExtensionRange {
  const google_protobuf_ExtensionRangeOptions* opts;
  int32_t start;
  int32_t end;
};

struct upb_MessageDef {
  const google_protobuf_MessageOptions* opts;
  const upb_MiniTable* layout;
  const upb_FileDef* file;
  const upb_MessageDef* containing_type;
  const char* full_name;

  /* Tables for looking up fields by number and name. */
  upb_inttable itof;
  upb_strtable ntof;

  /* All nested defs.
   * MEM: We could save some space here by putting nested defs in a contiguous
   * region and calculating counts from offsets or vice-versa. */
  const upb_FieldDef* fields;
  const upb_OneofDef* oneofs;
  const upb_ExtensionRange* ext_ranges;
  const upb_StringView* res_names;
  const upb_MessageDef* nested_msgs;
  const upb_MessageReservedRange* res_ranges;
  const upb_EnumDef* nested_enums;
  const upb_FieldDef* nested_exts;
  int field_count;
  int real_oneof_count;
  int oneof_count;
  int ext_range_count;
  int res_range_count;
  int res_name_count;
  int nested_msg_count;
  int nested_enum_count;
  int nested_ext_count;
  bool in_message_set;
  upb_WellKnown well_known_type;
#if UINTPTR_MAX == 0xffffffff
  uint32_t padding;  // Increase size to a multiple of 8.
#endif
};

struct upb_EnumDef {
  const google_protobuf_EnumOptions* opts;
  const upb_MiniTable_Enum* layout;  // Only for proto2.
  const upb_FileDef* file;
  const upb_MessageDef* containing_type;  // Could be merged with "file".
  const char* full_name;
  upb_strtable ntoi;
  upb_inttable iton;
  const upb_EnumValueDef* values;
  const upb_EnumReservedRange* res_ranges;
  const upb_StringView* res_names;
  int value_count;
  int res_range_count;
  int res_name_count;
  int32_t defaultval;
#if UINTPTR_MAX == 0xffffffff
  uint32_t padding;  // Increase size to a multiple of 8.
#endif
};

struct upb_EnumValueDef {
  const google_protobuf_EnumValueOptions* opts;
  const upb_EnumDef* parent;
  const char* full_name;
  int32_t number;
};

struct upb_OneofDef {
  const google_protobuf_OneofOptions* opts;
  const upb_MessageDef* parent;
  const char* full_name;
  int field_count;
  bool synthetic;
  const upb_FieldDef** fields;
  upb_strtable ntof;
  upb_inttable itof;
#if UINTPTR_MAX == 0xffffffff
  uint32_t padding;  // Increase size to a multiple of 8.
#endif
};

struct upb_FileDef {
  const google_protobuf_FileOptions* opts;
  const char* name;
  const char* package;

  const upb_FileDef** deps;
  const int32_t* public_deps;
  const int32_t* weak_deps;
  const upb_MessageDef* top_lvl_msgs;
  const upb_EnumDef* top_lvl_enums;
  const upb_FieldDef* top_lvl_exts;
  const upb_ServiceDef* services;
  const upb_MiniTable_Extension** ext_layouts;
  const upb_DefPool* symtab;

  int dep_count;
  int public_dep_count;
  int weak_dep_count;
  int top_lvl_msg_count;
  int top_lvl_enum_count;
  int top_lvl_ext_count;
  int service_count;
  int ext_count; /* All exts in the file. */
  upb_Syntax syntax;
};

struct upb_MethodDef {
  const google_protobuf_MethodOptions* opts;
  upb_ServiceDef* service;
  const char* full_name;
  const upb_MessageDef* input_type;
  const upb_MessageDef* output_type;
  int index;
  bool client_streaming;
  bool server_streaming;
};

struct upb_ServiceDef {
  const google_protobuf_ServiceOptions* opts;
  const upb_FileDef* file;
  const char* full_name;
  upb_MethodDef* methods;
  int method_count;
  int index;
};

struct upb_DefPool {
  upb_Arena* arena;
  upb_strtable syms;  /* full_name -> packed def ptr */
  upb_strtable files; /* file_name -> upb_FileDef* */
  upb_inttable exts;  /* upb_MiniTable_Extension* -> upb_FieldDef* */
  upb_ExtensionRegistry* extreg;
  size_t bytes_loaded;
};

/* Inside a symtab we store tagged pointers to specific def types. */
typedef enum {
  UPB_DEFTYPE_MASK = 7,

  /* Only inside symtab table. */
  UPB_DEFTYPE_EXT = 0,
  UPB_DEFTYPE_MSG = 1,
  UPB_DEFTYPE_ENUM = 2,
  UPB_DEFTYPE_ENUMVAL = 3,
  UPB_DEFTYPE_SERVICE = 4,

  /* Only inside message table. */
  UPB_DEFTYPE_FIELD = 0,
  UPB_DEFTYPE_ONEOF = 1,
  UPB_DEFTYPE_FIELD_JSONNAME = 2,

  /* Only inside file table. */
  UPB_DEFTYPE_FILE = 0,
  UPB_DEFTYPE_LAYOUT = 1
} upb_deftype_t;

#define FIELD_TYPE_UNSPECIFIED 0

struct upb_MessageReservedRange {
  int32_t start;
  int32_t end;
};

struct symtab_addctx {
  upb_DefPool* symtab;
  upb_FileDef* file;                /* File we are building. */
  upb_Arena* arena;                 /* Allocate defs here. */
  upb_Arena* tmp_arena;             /* For temporary allocations. */
  const upb_MiniTable_File* layout; /* NULL if we should build layouts. */
  int enum_count;                   /* Count of enums built so far. */
  int msg_count;                    /* Count of messages built so far. */
  int ext_count;                    /* Count of extensions built so far. */
  upb_Status* status;               /* Record errors here. */
  jmp_buf err;                      /* longjmp() on error. */
};

static upb_deftype_t deftype(upb_value v) {
  uintptr_t num = (uintptr_t)upb_value_getconstptr(v);
  return num & UPB_DEFTYPE_MASK;
}

static const void* unpack_def(upb_value v, upb_deftype_t type) {
  uintptr_t num = (uintptr_t)upb_value_getconstptr(v);
  return (num & UPB_DEFTYPE_MASK) == type
             ? (const void*)(num & ~UPB_DEFTYPE_MASK)
             : NULL;
}

static upb_value pack_def(const void* ptr, upb_deftype_t type) {
  // Our 3-bit pointer tagging requires all pointers to be multiples of 8.
  // The arena will always yield 8-byte-aligned addresses, however we put
  // the defs into arrays.  For each element in the array to be 8-byte-aligned,
  // the sizes of each def type must also be a multiple of 8.
  //
  // If any of these asserts fail, we need to add or remove padding on 32-bit
  // machines (64-bit machines will have 8-byte alignment already due to
  // pointers, which all of these structs have).
  UPB_ASSERT((sizeof(upb_FieldDef) & UPB_DEFTYPE_MASK) == 0);
  UPB_ASSERT((sizeof(upb_MessageDef) & UPB_DEFTYPE_MASK) == 0);
  UPB_ASSERT((sizeof(upb_EnumDef) & UPB_DEFTYPE_MASK) == 0);
  UPB_ASSERT((sizeof(upb_EnumValueDef) & UPB_DEFTYPE_MASK) == 0);
  UPB_ASSERT((sizeof(upb_ServiceDef) & UPB_DEFTYPE_MASK) == 0);
  UPB_ASSERT((sizeof(upb_OneofDef) & UPB_DEFTYPE_MASK) == 0);
  uintptr_t num = (uintptr_t)ptr;
  UPB_ASSERT((num & UPB_DEFTYPE_MASK) == 0);
  num |= type;
  return upb_value_constptr((const void*)num);
}

/* isalpha() etc. from <ctype.h> are locale-dependent, which we don't want. */
static bool upb_isbetween(uint8_t c, uint8_t low, uint8_t high) {
  return c >= low && c <= high;
}

static char upb_ascii_lower(char ch) {
  // Per ASCII this will lower-case a letter.  If the result is a letter, the
  // input was definitely a letter.  If the output is not a letter, this may
  // have transformed the character unpredictably.
  return ch | 0x20;
}

static bool upb_isletter(char c) {
  char lower = upb_ascii_lower(c);
  return upb_isbetween(lower, 'a', 'z') || c == '_';
}

static bool upb_isalphanum(char c) {
  return upb_isletter(c) || upb_isbetween(c, '0', '9');
}

static const char* shortdefname(const char* fullname) {
  const char* p;

  if (fullname == NULL) {
    return NULL;
  } else if ((p = strrchr(fullname, '.')) == NULL) {
    /* No '.' in the name, return the full string. */
    return fullname;
  } else {
    /* Return one past the last '.'. */
    return p + 1;
  }
}

/* All submessage fields are lower than all other fields.
 * Secondly, fields are increasing in order. */
uint32_t field_rank(const upb_FieldDef* f) {
  uint32_t ret = upb_FieldDef_Number(f);
  const uint32_t high_bit = 1 << 30;
  UPB_ASSERT(ret < high_bit);
  if (!upb_FieldDef_IsSubMessage(f)) ret |= high_bit;
  return ret;
}

int cmp_fields(const void* p1, const void* p2) {
  const upb_FieldDef* f1 = *(upb_FieldDef* const*)p1;
  const upb_FieldDef* f2 = *(upb_FieldDef* const*)p2;
  return field_rank(f1) - field_rank(f2);
}

static void upb_Status_setoom(upb_Status* status) {
  upb_Status_SetErrorMessage(status, "out of memory");
}

static void assign_msg_wellknowntype(upb_MessageDef* m) {
  const char* name = upb_MessageDef_FullName(m);
  if (name == NULL) {
    m->well_known_type = kUpb_WellKnown_Unspecified;
    return;
  }
  if (!strcmp(name, "google.protobuf.Any")) {
    m->well_known_type = kUpb_WellKnown_Any;
  } else if (!strcmp(name, "google.protobuf.FieldMask")) {
    m->well_known_type = kUpb_WellKnown_FieldMask;
  } else if (!strcmp(name, "google.protobuf.Duration")) {
    m->well_known_type = kUpb_WellKnown_Duration;
  } else if (!strcmp(name, "google.protobuf.Timestamp")) {
    m->well_known_type = kUpb_WellKnown_Timestamp;
  } else if (!strcmp(name, "google.protobuf.DoubleValue")) {
    m->well_known_type = kUpb_WellKnown_DoubleValue;
  } else if (!strcmp(name, "google.protobuf.FloatValue")) {
    m->well_known_type = kUpb_WellKnown_FloatValue;
  } else if (!strcmp(name, "google.protobuf.Int64Value")) {
    m->well_known_type = kUpb_WellKnown_Int64Value;
  } else if (!strcmp(name, "google.protobuf.UInt64Value")) {
    m->well_known_type = kUpb_WellKnown_UInt64Value;
  } else if (!strcmp(name, "google.protobuf.Int32Value")) {
    m->well_known_type = kUpb_WellKnown_Int32Value;
  } else if (!strcmp(name, "google.protobuf.UInt32Value")) {
    m->well_known_type = kUpb_WellKnown_UInt32Value;
  } else if (!strcmp(name, "google.protobuf.BoolValue")) {
    m->well_known_type = kUpb_WellKnown_BoolValue;
  } else if (!strcmp(name, "google.protobuf.StringValue")) {
    m->well_known_type = kUpb_WellKnown_StringValue;
  } else if (!strcmp(name, "google.protobuf.BytesValue")) {
    m->well_known_type = kUpb_WellKnown_BytesValue;
  } else if (!strcmp(name, "google.protobuf.Value")) {
    m->well_known_type = kUpb_WellKnown_Value;
  } else if (!strcmp(name, "google.protobuf.ListValue")) {
    m->well_known_type = kUpb_WellKnown_ListValue;
  } else if (!strcmp(name, "google.protobuf.Struct")) {
    m->well_known_type = kUpb_WellKnown_Struct;
  } else {
    m->well_known_type = kUpb_WellKnown_Unspecified;
  }
}

/* upb_EnumDef ****************************************************************/

const google_protobuf_EnumOptions* upb_EnumDef_Options(const upb_EnumDef* e) {
  return e->opts;
}

bool upb_EnumDef_HasOptions(const upb_EnumDef* e) {
  return e->opts != (void*)opt_default;
}

const char* upb_EnumDef_FullName(const upb_EnumDef* e) { return e->full_name; }

const char* upb_EnumDef_Name(const upb_EnumDef* e) {
  return shortdefname(e->full_name);
}

const upb_FileDef* upb_EnumDef_File(const upb_EnumDef* e) { return e->file; }

const upb_MessageDef* upb_EnumDef_ContainingType(const upb_EnumDef* e) {
  return e->containing_type;
}

int32_t upb_EnumDef_Default(const upb_EnumDef* e) {
  UPB_ASSERT(upb_EnumDef_FindValueByNumber(e, e->defaultval));
  return e->defaultval;
}

int upb_EnumDef_ReservedRangeCount(const upb_EnumDef* e) {
  return e->res_range_count;
}

/* upb_EnumReservedRange ******************************************************/

struct upb_EnumReservedRange {
  int32_t start;
  int32_t end;
};

upb_EnumReservedRange* _upb_EnumReservedRange_At(const upb_EnumReservedRange* r,
                                                 int i) {
  return (upb_EnumReservedRange*)&r[i];
}

int32_t upb_EnumReservedRange_Start(const upb_EnumReservedRange* r) {
  return r->start;
}
int32_t upb_EnumReservedRange_End(const upb_EnumReservedRange* r) {
  return r->end;
}

UPB_NORETURN UPB_NOINLINE UPB_PRINTF(2, 3) static void symtab_errf(
    symtab_addctx* ctx, const char* fmt, ...) {
  va_list argp;
  va_start(argp, fmt);
  upb_Status_VSetErrorFormat(ctx->status, fmt, argp);
  va_end(argp);
  UPB_LONGJMP(ctx->err, 1);
}

upb_EnumReservedRange* _upb_EnumReservedRanges_New(
    symtab_addctx* ctx, int n,
    const google_protobuf_EnumDescriptorProto_EnumReservedRange* const* protos,
    const upb_EnumDef* e) {
  upb_EnumReservedRange* r =
      upb_Arena_Malloc(ctx->arena, sizeof(upb_EnumReservedRange) * n);

  for (int i = 0; i < n; i++) {
    const int32_t start =
        google_protobuf_EnumDescriptorProto_EnumReservedRange_start(protos[i]);
    const int32_t end =
        google_protobuf_EnumDescriptorProto_EnumReservedRange_end(protos[i]);

    // A full validation would also check that each range is disjoint, and that
    // none of the fields overlap with the extension ranges, but we are just
    // sanity checking here.

    // Note: Not a typo! Unlike extension ranges and message reserved ranges,
    // the end value of an enum reserved range is *inclusive*!
    if (end < start) {
      symtab_errf(ctx, "Reserved range (%d, %d) is invalid, enum=%s\n",
                           (int)start, (int)end, upb_EnumDef_FullName(e));
    }

    r[i].start = start;
    r[i].end = end;
  }

  return r;
}

const upb_EnumReservedRange* upb_EnumDef_ReservedRange(const upb_EnumDef* e,
                                                       int i) {
  UPB_ASSERT(0 <= i && i < e->res_range_count);
  return _upb_EnumReservedRange_At(e->res_ranges, i);
}

int upb_EnumDef_ReservedNameCount(const upb_EnumDef* e) {
  return e->res_name_count;
}

upb_StringView upb_EnumDef_ReservedName(const upb_EnumDef* e, int i) {
  UPB_ASSERT(0 <= i && i < e->res_name_count);
  return e->res_names[i];
}

int upb_EnumDef_ValueCount(const upb_EnumDef* e) { return e->value_count; }

const upb_EnumValueDef* upb_EnumDef_FindValueByNameWithSize(
    const upb_EnumDef* def, const char* name, size_t len) {
  upb_value v;
  return upb_strtable_lookup2(&def->ntoi, name, len, &v)
             ? upb_value_getconstptr(v)
             : NULL;
}

const upb_EnumValueDef* upb_EnumDef_FindValueByNumber(const upb_EnumDef* def,
                                                      int32_t num) {
  upb_value v;
  return upb_inttable_lookup(&def->iton, num, &v) ? upb_value_getconstptr(v)
                                                  : NULL;
}

bool upb_EnumDef_CheckNumber(const upb_EnumDef* e, int32_t num) {
  // We could use upb_EnumDef_FindValueByNumber(e, num) != NULL, but we expect
  // this to be faster (especially for small numbers).
  return upb_MiniTable_Enum_CheckValue(e->layout, num);
}

const upb_EnumValueDef* upb_EnumDef_Value(const upb_EnumDef* e, int i) {
  UPB_ASSERT(0 <= i && i < e->value_count);
  return &e->values[i];
}

/* upb_EnumValueDef ***********************************************************/

const google_protobuf_EnumValueOptions* upb_EnumValueDef_Options(
    const upb_EnumValueDef* e) {
  return e->opts;
}

bool upb_EnumValueDef_HasOptions(const upb_EnumValueDef* e) {
  return e->opts != (void*)opt_default;
}

const upb_EnumDef* upb_EnumValueDef_Enum(const upb_EnumValueDef* ev) {
  return ev->parent;
}

const char* upb_EnumValueDef_FullName(const upb_EnumValueDef* ev) {
  return ev->full_name;
}

const char* upb_EnumValueDef_Name(const upb_EnumValueDef* ev) {
  return shortdefname(ev->full_name);
}

int32_t upb_EnumValueDef_Number(const upb_EnumValueDef* ev) {
  return ev->number;
}

uint32_t upb_EnumValueDef_Index(const upb_EnumValueDef* ev) {
  // Compute index in our parent's array.
  return ev - ev->parent->values;
}

/* upb_ExtensionRange
 * ***************************************************************/

const google_protobuf_ExtensionRangeOptions* upb_ExtensionRange_Options(
    const upb_ExtensionRange* r) {
  return r->opts;
}

bool upb_ExtensionRange_HasOptions(const upb_ExtensionRange* r) {
  return r->opts != (void*)opt_default;
}

int32_t upb_ExtensionRange_Start(const upb_ExtensionRange* e) {
  return e->start;
}

int32_t upb_ExtensionRange_End(const upb_ExtensionRange* e) { return e->end; }

/* upb_FieldDef ***************************************************************/

const google_protobuf_FieldOptions* upb_FieldDef_Options(
    const upb_FieldDef* f) {
  return f->opts;
}

bool upb_FieldDef_HasOptions(const upb_FieldDef* f) {
  return f->opts != (void*)opt_default;
}

const char* upb_FieldDef_FullName(const upb_FieldDef* f) {
  return f->full_name;
}

upb_CType upb_FieldDef_CType(const upb_FieldDef* f) {
  switch (f->type_) {
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

upb_FieldType upb_FieldDef_Type(const upb_FieldDef* f) { return f->type_; }

uint32_t upb_FieldDef_Index(const upb_FieldDef* f) { return f->index_; }

upb_Label upb_FieldDef_Label(const upb_FieldDef* f) { return f->label_; }

uint32_t upb_FieldDef_Number(const upb_FieldDef* f) { return f->number_; }

bool upb_FieldDef_IsExtension(const upb_FieldDef* f) {
  return f->is_extension_;
}

bool upb_FieldDef_IsPacked(const upb_FieldDef* f) { return f->packed_; }

const char* upb_FieldDef_Name(const upb_FieldDef* f) {
  return shortdefname(f->full_name);
}

const char* upb_FieldDef_JsonName(const upb_FieldDef* f) {
  return f->json_name;
}

bool upb_FieldDef_HasJsonName(const upb_FieldDef* f) {
  return f->has_json_name_;
}

const upb_FileDef* upb_FieldDef_File(const upb_FieldDef* f) { return f->file; }

const upb_MessageDef* upb_FieldDef_ContainingType(const upb_FieldDef* f) {
  return f->msgdef;
}

const upb_MessageDef* upb_FieldDef_ExtensionScope(const upb_FieldDef* f) {
  return f->is_extension_ ? f->scope.extension_scope : NULL;
}

const upb_OneofDef* upb_FieldDef_ContainingOneof(const upb_FieldDef* f) {
  return f->is_extension_ ? NULL : f->scope.oneof;
}

const upb_OneofDef* upb_FieldDef_RealContainingOneof(const upb_FieldDef* f) {
  const upb_OneofDef* oneof = upb_FieldDef_ContainingOneof(f);
  if (!oneof || upb_OneofDef_IsSynthetic(oneof)) return NULL;
  return oneof;
}

upb_MessageValue upb_FieldDef_Default(const upb_FieldDef* f) {
  UPB_ASSERT(!upb_FieldDef_IsSubMessage(f));
  upb_MessageValue ret;

  switch (upb_FieldDef_CType(f)) {
    case kUpb_CType_Bool:
      return (upb_MessageValue){.bool_val = f->defaultval.boolean};
    case kUpb_CType_Int64:
      return (upb_MessageValue){.int64_val = f->defaultval.sint};
    case kUpb_CType_UInt64:
      return (upb_MessageValue){.uint64_val = f->defaultval.uint};
    case kUpb_CType_Enum:
    case kUpb_CType_Int32:
      return (upb_MessageValue){.int32_val = (int32_t)f->defaultval.sint};
    case kUpb_CType_UInt32:
      return (upb_MessageValue){.uint32_val = (uint32_t)f->defaultval.uint};
    case kUpb_CType_Float:
      return (upb_MessageValue){.float_val = f->defaultval.flt};
    case kUpb_CType_Double:
      return (upb_MessageValue){.double_val = f->defaultval.dbl};
    case kUpb_CType_String:
    case kUpb_CType_Bytes: {
      str_t* str = f->defaultval.str;
      if (str) {
        return (upb_MessageValue){
            .str_val = (upb_StringView){.data = str->str, .size = str->len}};
      } else {
        return (upb_MessageValue){
            .str_val = (upb_StringView){.data = NULL, .size = 0}};
      }
    }
    default:
      UPB_UNREACHABLE();
  }

  return ret;
}

const upb_MessageDef* upb_FieldDef_MessageSubDef(const upb_FieldDef* f) {
  return upb_FieldDef_CType(f) == kUpb_CType_Message ? f->sub.msgdef : NULL;
}

const upb_EnumDef* upb_FieldDef_EnumSubDef(const upb_FieldDef* f) {
  return upb_FieldDef_CType(f) == kUpb_CType_Enum ? f->sub.enumdef : NULL;
}

const upb_MiniTable_Field* upb_FieldDef_MiniTable(const upb_FieldDef* f) {
  UPB_ASSERT(!upb_FieldDef_IsExtension(f));
  return &f->msgdef->layout->fields[f->layout_index];
}

const upb_MiniTable_Extension* _upb_FieldDef_ExtensionMiniTable(
    const upb_FieldDef* f) {
  UPB_ASSERT(upb_FieldDef_IsExtension(f));
  return f->file->ext_layouts[f->layout_index];
}

bool _upb_FieldDef_IsProto3Optional(const upb_FieldDef* f) {
  return f->proto3_optional_;
}

bool upb_FieldDef_IsSubMessage(const upb_FieldDef* f) {
  return upb_FieldDef_CType(f) == kUpb_CType_Message;
}

bool upb_FieldDef_IsString(const upb_FieldDef* f) {
  return upb_FieldDef_CType(f) == kUpb_CType_String ||
         upb_FieldDef_CType(f) == kUpb_CType_Bytes;
}

bool upb_FieldDef_IsRepeated(const upb_FieldDef* f) {
  return upb_FieldDef_Label(f) == kUpb_Label_Repeated;
}

bool upb_FieldDef_IsPrimitive(const upb_FieldDef* f) {
  return !upb_FieldDef_IsString(f) && !upb_FieldDef_IsSubMessage(f);
}

bool upb_FieldDef_IsMap(const upb_FieldDef* f) {
  return upb_FieldDef_IsRepeated(f) && upb_FieldDef_IsSubMessage(f) &&
         upb_MessageDef_IsMapEntry(upb_FieldDef_MessageSubDef(f));
}

bool upb_FieldDef_HasDefault(const upb_FieldDef* f) { return f->has_default; }

bool upb_FieldDef_HasSubDef(const upb_FieldDef* f) {
  return upb_FieldDef_IsSubMessage(f) ||
         upb_FieldDef_CType(f) == kUpb_CType_Enum;
}

bool upb_FieldDef_HasPresence(const upb_FieldDef* f) {
  if (upb_FieldDef_IsRepeated(f)) return false;
  return upb_FieldDef_IsSubMessage(f) || upb_FieldDef_ContainingOneof(f) ||
         f->file->syntax == kUpb_Syntax_Proto2;
}

static bool between(int32_t x, int32_t low, int32_t high) {
  return x >= low && x <= high;
}

bool upb_FieldDef_checklabel(int32_t label) { return between(label, 1, 3); }
bool upb_FieldDef_checktype(int32_t type) { return between(type, 1, 11); }
bool upb_FieldDef_checkintfmt(int32_t fmt) { return between(fmt, 1, 3); }

bool upb_FieldDef_checkdescriptortype(int32_t type) {
  return between(type, 1, 18);
}

/* upb_MessageDef
 * *****************************************************************/

const google_protobuf_MessageOptions* upb_MessageDef_Options(
    const upb_MessageDef* m) {
  return m->opts;
}

bool upb_MessageDef_HasOptions(const upb_MessageDef* m) {
  return m->opts != (void*)opt_default;
}

const char* upb_MessageDef_FullName(const upb_MessageDef* m) {
  return m->full_name;
}

const upb_FileDef* upb_MessageDef_File(const upb_MessageDef* m) {
  return m->file;
}

const upb_MessageDef* upb_MessageDef_ContainingType(const upb_MessageDef* m) {
  return m->containing_type;
}

const char* upb_MessageDef_Name(const upb_MessageDef* m) {
  return shortdefname(m->full_name);
}

upb_Syntax upb_MessageDef_Syntax(const upb_MessageDef* m) {
  return m->file->syntax;
}

const upb_FieldDef* upb_MessageDef_FindFieldByNumber(const upb_MessageDef* m,
                                                     uint32_t i) {
  upb_value val;
  return upb_inttable_lookup(&m->itof, i, &val) ? upb_value_getconstptr(val)
                                                : NULL;
}

const upb_FieldDef* upb_MessageDef_FindFieldByNameWithSize(
    const upb_MessageDef* m, const char* name, size_t len) {
  upb_value val;

  if (!upb_strtable_lookup2(&m->ntof, name, len, &val)) {
    return NULL;
  }

  return unpack_def(val, UPB_DEFTYPE_FIELD);
}

const upb_OneofDef* upb_MessageDef_FindOneofByNameWithSize(
    const upb_MessageDef* m, const char* name, size_t len) {
  upb_value val;

  if (!upb_strtable_lookup2(&m->ntof, name, len, &val)) {
    return NULL;
  }

  return unpack_def(val, UPB_DEFTYPE_ONEOF);
}

bool upb_MessageDef_FindByNameWithSize(const upb_MessageDef* m,
                                       const char* name, size_t len,
                                       const upb_FieldDef** out_f,
                                       const upb_OneofDef** out_o) {
  upb_value val;

  if (!upb_strtable_lookup2(&m->ntof, name, len, &val)) {
    return false;
  }

  const upb_FieldDef* f = unpack_def(val, UPB_DEFTYPE_FIELD);
  const upb_OneofDef* o = unpack_def(val, UPB_DEFTYPE_ONEOF);
  if (out_f) *out_f = f;
  if (out_o) *out_o = o;
  return f || o; /* False if this was a JSON name. */
}

const upb_FieldDef* upb_MessageDef_FindByJsonNameWithSize(
    const upb_MessageDef* m, const char* name, size_t len) {
  upb_value val;
  const upb_FieldDef* f;

  if (!upb_strtable_lookup2(&m->ntof, name, len, &val)) {
    return NULL;
  }

  f = unpack_def(val, UPB_DEFTYPE_FIELD);
  if (!f) f = unpack_def(val, UPB_DEFTYPE_FIELD_JSONNAME);

  return f;
}

int upb_MessageDef_numfields(const upb_MessageDef* m) { return m->field_count; }

int upb_MessageDef_numoneofs(const upb_MessageDef* m) { return m->oneof_count; }

int upb_MessageDef_numrealoneofs(const upb_MessageDef* m) {
  return m->real_oneof_count;
}

int upb_MessageDef_ExtensionRangeCount(const upb_MessageDef* m) {
  return m->ext_range_count;
}

int upb_MessageDef_ReservedRangeCount(const upb_MessageDef* m) {
  return m->res_range_count;
}

int upb_MessageDef_ReservedNameCount(const upb_MessageDef* m) {
  return m->res_name_count;
}

int upb_MessageDef_FieldCount(const upb_MessageDef* m) {
  return m->field_count;
}

int upb_MessageDef_OneofCount(const upb_MessageDef* m) {
  return m->oneof_count;
}

int upb_MessageDef_NestedMessageCount(const upb_MessageDef* m) {
  return m->nested_msg_count;
}

int upb_MessageDef_NestedEnumCount(const upb_MessageDef* m) {
  return m->nested_enum_count;
}

int upb_MessageDef_NestedExtensionCount(const upb_MessageDef* m) {
  return m->nested_ext_count;
}

int upb_MessageDef_realoneofcount(const upb_MessageDef* m) {
  return m->real_oneof_count;
}

const upb_MiniTable* upb_MessageDef_MiniTable(const upb_MessageDef* m) {
  return m->layout;
}

const upb_ExtensionRange* upb_MessageDef_ExtensionRange(const upb_MessageDef* m,
                                                        int i) {
  UPB_ASSERT(0 <= i && i < m->ext_range_count);
  return &m->ext_ranges[i];
}

upb_MessageReservedRange* _upb_MessageReservedRange_At(
    const upb_MessageReservedRange* r, int i) {
  return (upb_MessageReservedRange*)&r[i];
}

const upb_MessageReservedRange* upb_MessageDef_ReservedRange(
    const upb_MessageDef* m, int i) {
  UPB_ASSERT(0 <= i && i < m->res_range_count);
  return _upb_MessageReservedRange_At(m->res_ranges, i);
}

upb_StringView upb_MessageDef_ReservedName(const upb_MessageDef* m, int i) {
  UPB_ASSERT(0 <= i && i < m->res_name_count);
  return m->res_names[i];
}

int32_t upb_MessageReservedRange_Start(const upb_MessageReservedRange* r) {
  return r->start;
}
int32_t upb_MessageReservedRange_End(const upb_MessageReservedRange* r) {
  return r->end;
}

upb_MessageReservedRange* _upb_MessageReservedRanges_New(
    symtab_addctx* ctx, int n,
    const google_protobuf_DescriptorProto_ReservedRange* const* protos,
    const upb_MessageDef* m) {
  upb_MessageReservedRange* r =
      upb_Arena_Malloc(ctx->arena, sizeof(upb_MessageReservedRange) * n);

  for (int i = 0; i < n; i++) {
    const int32_t start = google_protobuf_DescriptorProto_ReservedRange_start(protos[i]);
    const int32_t end = google_protobuf_DescriptorProto_ReservedRange_end(protos[i]);
    const int32_t max = kUpb_MaxFieldNumber + 1;

    // A full validation would also check that each range is disjoint, and that
    // none of the fields overlap with the extension ranges, but we are just
    // sanity checking here.
    if (start < 1 || end <= start || end > max) {
      symtab_errf(ctx,
                           "Reserved range (%d, %d) is invalid, message=%s\n",
                           (int)start, (int)end, upb_MessageDef_FullName(m));
    }

    r[i].start = start;
    r[i].end = end;
  }

  return r;
}

const upb_FieldDef* upb_MessageDef_Field(const upb_MessageDef* m, int i) {
  UPB_ASSERT(0 <= i && i < m->field_count);
  return &m->fields[i];
}

const upb_OneofDef* upb_MessageDef_Oneof(const upb_MessageDef* m, int i) {
  UPB_ASSERT(0 <= i && i < m->oneof_count);
  return &m->oneofs[i];
}

const upb_MessageDef* upb_MessageDef_NestedMessage(const upb_MessageDef* m,
                                                   int i) {
  UPB_ASSERT(0 <= i && i < m->nested_msg_count);
  return &m->nested_msgs[i];
}

const upb_EnumDef* upb_MessageDef_NestedEnum(const upb_MessageDef* m, int i) {
  UPB_ASSERT(0 <= i && i < m->nested_enum_count);
  return &m->nested_enums[i];
}

const upb_FieldDef* upb_MessageDef_NestedExtension(const upb_MessageDef* m,
                                                   int i) {
  UPB_ASSERT(0 <= i && i < m->nested_ext_count);
  return &m->nested_exts[i];
}

upb_WellKnown upb_MessageDef_WellKnownType(const upb_MessageDef* m) {
  return m->well_known_type;
}

/* upb_OneofDef ***************************************************************/

const google_protobuf_OneofOptions* upb_OneofDef_Options(
    const upb_OneofDef* o) {
  return o->opts;
}

bool upb_OneofDef_HasOptions(const upb_OneofDef* o) {
  return o->opts != (void*)opt_default;
}

const char* upb_OneofDef_Name(const upb_OneofDef* o) {
  return shortdefname(o->full_name);
}

const upb_MessageDef* upb_OneofDef_ContainingType(const upb_OneofDef* o) {
  return o->parent;
}

int upb_OneofDef_FieldCount(const upb_OneofDef* o) { return o->field_count; }

const upb_FieldDef* upb_OneofDef_Field(const upb_OneofDef* o, int i) {
  UPB_ASSERT(i < o->field_count);
  return o->fields[i];
}

int upb_OneofDef_numfields(const upb_OneofDef* o) { return o->field_count; }

uint32_t upb_OneofDef_Index(const upb_OneofDef* o) {
  // Compute index in our parent's array.
  return o - o->parent->oneofs;
}

bool upb_OneofDef_IsSynthetic(const upb_OneofDef* o) { return o->synthetic; }

const upb_FieldDef* upb_OneofDef_LookupNameWithSize(const upb_OneofDef* o,
                                                    const char* name,
                                                    size_t length) {
  upb_value val;
  return upb_strtable_lookup2(&o->ntof, name, length, &val)
             ? upb_value_getptr(val)
             : NULL;
}

const upb_FieldDef* upb_OneofDef_LookupNumber(const upb_OneofDef* o,
                                              uint32_t num) {
  upb_value val;
  return upb_inttable_lookup(&o->itof, num, &val) ? upb_value_getptr(val)
                                                  : NULL;
}

/* upb_FileDef ****************************************************************/

const google_protobuf_FileOptions* upb_FileDef_Options(const upb_FileDef* f) {
  return f->opts;
}

bool upb_FileDef_HasOptions(const upb_FileDef* f) {
  return f->opts != (void*)opt_default;
}

const char* upb_FileDef_Name(const upb_FileDef* f) { return f->name; }

const char* upb_FileDef_Package(const upb_FileDef* f) {
  return f->package ? f->package : "";
}

upb_Syntax upb_FileDef_Syntax(const upb_FileDef* f) { return f->syntax; }

int upb_FileDef_TopLevelMessageCount(const upb_FileDef* f) {
  return f->top_lvl_msg_count;
}

int upb_FileDef_DependencyCount(const upb_FileDef* f) { return f->dep_count; }

int upb_FileDef_PublicDependencyCount(const upb_FileDef* f) {
  return f->public_dep_count;
}

int upb_FileDef_WeakDependencyCount(const upb_FileDef* f) {
  return f->weak_dep_count;
}

const int32_t* _upb_FileDef_PublicDependencyIndexes(const upb_FileDef* f) {
  return f->public_deps;
}

const int32_t* _upb_FileDef_WeakDependencyIndexes(const upb_FileDef* f) {
  return f->weak_deps;
}

int upb_FileDef_TopLevelEnumCount(const upb_FileDef* f) {
  return f->top_lvl_enum_count;
}

int upb_FileDef_TopLevelExtensionCount(const upb_FileDef* f) {
  return f->top_lvl_ext_count;
}

int upb_FileDef_ServiceCount(const upb_FileDef* f) { return f->service_count; }

const upb_FileDef* upb_FileDef_Dependency(const upb_FileDef* f, int i) {
  UPB_ASSERT(0 <= i && i < f->dep_count);
  return f->deps[i];
}

const upb_FileDef* upb_FileDef_PublicDependency(const upb_FileDef* f, int i) {
  UPB_ASSERT(0 <= i && i < f->public_dep_count);
  return f->deps[f->public_deps[i]];
}

const upb_FileDef* upb_FileDef_WeakDependency(const upb_FileDef* f, int i) {
  UPB_ASSERT(0 <= i && i < f->public_dep_count);
  return f->deps[f->weak_deps[i]];
}

const upb_MessageDef* upb_FileDef_TopLevelMessage(const upb_FileDef* f, int i) {
  UPB_ASSERT(0 <= i && i < f->top_lvl_msg_count);
  return &f->top_lvl_msgs[i];
}

const upb_EnumDef* upb_FileDef_TopLevelEnum(const upb_FileDef* f, int i) {
  UPB_ASSERT(0 <= i && i < f->top_lvl_enum_count);
  return &f->top_lvl_enums[i];
}

const upb_FieldDef* upb_FileDef_TopLevelExtension(const upb_FileDef* f, int i) {
  UPB_ASSERT(0 <= i && i < f->top_lvl_ext_count);
  return &f->top_lvl_exts[i];
}

const upb_ServiceDef* upb_FileDef_Service(const upb_FileDef* f, int i) {
  UPB_ASSERT(0 <= i && i < f->service_count);
  return &f->services[i];
}

const upb_DefPool* upb_FileDef_Pool(const upb_FileDef* f) { return f->symtab; }

/* upb_MethodDef **************************************************************/

const google_protobuf_MethodOptions* upb_MethodDef_Options(
    const upb_MethodDef* m) {
  return m->opts;
}

bool upb_MethodDef_HasOptions(const upb_MethodDef* m) {
  return m->opts != (void*)opt_default;
}

const char* upb_MethodDef_FullName(const upb_MethodDef* m) {
  return m->full_name;
}

int upb_MethodDef_Index(const upb_MethodDef* m) { return m->index; }

const char* upb_MethodDef_Name(const upb_MethodDef* m) {
  return shortdefname(m->full_name);
}

const upb_ServiceDef* upb_MethodDef_Service(const upb_MethodDef* m) {
  return m->service;
}

const upb_MessageDef* upb_MethodDef_InputType(const upb_MethodDef* m) {
  return m->input_type;
}

const upb_MessageDef* upb_MethodDef_OutputType(const upb_MethodDef* m) {
  return m->output_type;
}

bool upb_MethodDef_ClientStreaming(const upb_MethodDef* m) {
  return m->client_streaming;
}

bool upb_MethodDef_ServerStreaming(const upb_MethodDef* m) {
  return m->server_streaming;
}

/* upb_ServiceDef *************************************************************/

const google_protobuf_ServiceOptions* upb_ServiceDef_Options(
    const upb_ServiceDef* s) {
  return s->opts;
}

bool upb_ServiceDef_HasOptions(const upb_ServiceDef* s) {
  return s->opts != (void*)opt_default;
}

const char* upb_ServiceDef_FullName(const upb_ServiceDef* s) {
  return s->full_name;
}

const char* upb_ServiceDef_Name(const upb_ServiceDef* s) {
  return shortdefname(s->full_name);
}

int upb_ServiceDef_Index(const upb_ServiceDef* s) { return s->index; }

const upb_FileDef* upb_ServiceDef_File(const upb_ServiceDef* s) {
  return s->file;
}

int upb_ServiceDef_MethodCount(const upb_ServiceDef* s) {
  return s->method_count;
}

const upb_MethodDef* upb_ServiceDef_Method(const upb_ServiceDef* s, int i) {
  return i < 0 || i >= s->method_count ? NULL : &s->methods[i];
}

const upb_MethodDef* upb_ServiceDef_FindMethodByName(const upb_ServiceDef* s,
                                                     const char* name) {
  for (int i = 0; i < s->method_count; i++) {
    if (strcmp(name, upb_MethodDef_Name(&s->methods[i])) == 0) {
      return &s->methods[i];
    }
  }
  return NULL;
}

/* upb_DefPool ****************************************************************/

void upb_DefPool_Free(upb_DefPool* s) {
  upb_Arena_Free(s->arena);
  upb_gfree(s);
}

upb_DefPool* upb_DefPool_New(void) {
  upb_DefPool* s = upb_gmalloc(sizeof(*s));

  if (!s) {
    return NULL;
  }

  s->arena = upb_Arena_New();
  s->bytes_loaded = 0;

  if (!upb_strtable_init(&s->syms, 32, s->arena) ||
      !upb_strtable_init(&s->files, 4, s->arena) ||
      !upb_inttable_init(&s->exts, s->arena)) {
    goto err;
  }

  s->extreg = upb_ExtensionRegistry_New(s->arena);
  if (!s->extreg) goto err;
  return s;

err:
  upb_Arena_Free(s->arena);
  upb_gfree(s);
  return NULL;
}

static const void* symtab_lookup(const upb_DefPool* s, const char* sym,
                                 upb_deftype_t type) {
  upb_value v;
  return upb_strtable_lookup(&s->syms, sym, &v) ? unpack_def(v, type) : NULL;
}

static const void* symtab_lookup2(const upb_DefPool* s, const char* sym,
                                  size_t size, upb_deftype_t type) {
  upb_value v;
  return upb_strtable_lookup2(&s->syms, sym, size, &v) ? unpack_def(v, type)
                                                       : NULL;
}

const upb_MessageDef* upb_DefPool_FindMessageByName(const upb_DefPool* s,
                                                    const char* sym) {
  return symtab_lookup(s, sym, UPB_DEFTYPE_MSG);
}

const upb_MessageDef* upb_DefPool_FindMessageByNameWithSize(
    const upb_DefPool* s, const char* sym, size_t len) {
  return symtab_lookup2(s, sym, len, UPB_DEFTYPE_MSG);
}

const upb_EnumDef* upb_DefPool_FindEnumByName(const upb_DefPool* s,
                                              const char* sym) {
  return symtab_lookup(s, sym, UPB_DEFTYPE_ENUM);
}

const upb_EnumValueDef* upb_DefPool_FindEnumByNameval(const upb_DefPool* s,
                                                      const char* sym) {
  return symtab_lookup(s, sym, UPB_DEFTYPE_ENUMVAL);
}

const upb_FileDef* upb_DefPool_FindFileByName(const upb_DefPool* s,
                                              const char* name) {
  upb_value v;
  return upb_strtable_lookup(&s->files, name, &v)
             ? unpack_def(v, UPB_DEFTYPE_FILE)
             : NULL;
}

const upb_FileDef* upb_DefPool_FindFileByNameWithSize(const upb_DefPool* s,
                                                      const char* name,
                                                      size_t len) {
  upb_value v;
  return upb_strtable_lookup2(&s->files, name, len, &v)
             ? unpack_def(v, UPB_DEFTYPE_FILE)
             : NULL;
}

const upb_FieldDef* upb_DefPool_FindExtensionByNameWithSize(
    const upb_DefPool* s, const char* name, size_t size) {
  upb_value v;
  if (!upb_strtable_lookup2(&s->syms, name, size, &v)) return NULL;

  switch (deftype(v)) {
    case UPB_DEFTYPE_FIELD:
      return unpack_def(v, UPB_DEFTYPE_FIELD);
    case UPB_DEFTYPE_MSG: {
      const upb_MessageDef* m = unpack_def(v, UPB_DEFTYPE_MSG);
      return m->in_message_set ? &m->nested_exts[0] : NULL;
    }
    default:
      break;
  }

  return NULL;
}

const upb_FieldDef* upb_DefPool_FindExtensionByName(const upb_DefPool* s,
                                                    const char* sym) {
  return upb_DefPool_FindExtensionByNameWithSize(s, sym, strlen(sym));
}

const upb_ServiceDef* upb_DefPool_FindServiceByName(const upb_DefPool* s,
                                                    const char* name) {
  return symtab_lookup(s, name, UPB_DEFTYPE_SERVICE);
}

const upb_ServiceDef* upb_DefPool_FindServiceByNameWithSize(
    const upb_DefPool* s, const char* name, size_t size) {
  return symtab_lookup2(s, name, size, UPB_DEFTYPE_SERVICE);
}

const upb_FileDef* upb_DefPool_FindFileContainingSymbol(const upb_DefPool* s,
                                                        const char* name) {
  upb_value v;
  // TODO(haberman): non-extension fields and oneofs.
  if (upb_strtable_lookup(&s->syms, name, &v)) {
    switch (deftype(v)) {
      case UPB_DEFTYPE_EXT: {
        const upb_FieldDef* f = unpack_def(v, UPB_DEFTYPE_EXT);
        return upb_FieldDef_File(f);
      }
      case UPB_DEFTYPE_MSG: {
        const upb_MessageDef* m = unpack_def(v, UPB_DEFTYPE_MSG);
        return upb_MessageDef_File(m);
      }
      case UPB_DEFTYPE_ENUM: {
        const upb_EnumDef* e = unpack_def(v, UPB_DEFTYPE_ENUM);
        return upb_EnumDef_File(e);
      }
      case UPB_DEFTYPE_ENUMVAL: {
        const upb_EnumValueDef* ev = unpack_def(v, UPB_DEFTYPE_ENUMVAL);
        return upb_EnumDef_File(upb_EnumValueDef_Enum(ev));
      }
      case UPB_DEFTYPE_SERVICE: {
        const upb_ServiceDef* service = unpack_def(v, UPB_DEFTYPE_SERVICE);
        return upb_ServiceDef_File(service);
      }
      default:
        UPB_UNREACHABLE();
    }
  }

  const char* last_dot = strrchr(name, '.');
  if (last_dot) {
    const upb_MessageDef* parent =
        upb_DefPool_FindMessageByNameWithSize(s, name, last_dot - name);
    if (parent) {
      const char* shortname = last_dot + 1;
      if (upb_MessageDef_FindByNameWithSize(parent, shortname,
                                            strlen(shortname), NULL, NULL)) {
        return upb_MessageDef_File(parent);
      }
    }
  }

  return NULL;
}

/* Code to build defs from descriptor protos. *********************************/

/* There is a question of how much validation to do here.  It will be difficult
 * to perfectly match the amount of validation performed by proto2.  But since
 * this code is used to directly build defs from Ruby (for example) we do need
 * to validate important constraints like uniqueness of names and numbers. */

#define CHK_OOM(x)      \
  if (!(x)) {           \
    symtab_oomerr(ctx); \
  }

UPB_NORETURN UPB_NOINLINE static void symtab_oomerr(symtab_addctx* ctx) {
  upb_Status_setoom(ctx->status);
  UPB_LONGJMP(ctx->err, 1);
}

void* symtab_alloc(symtab_addctx* ctx, size_t bytes) {
  if (bytes == 0) return NULL;
  void* ret = upb_Arena_Malloc(ctx->arena, bytes);
  if (!ret) symtab_oomerr(ctx);
  return ret;
}

// We want to copy the options verbatim into the destination options proto.
// We use serialize+parse as our deep copy.
#define SET_OPTIONS(target, desc_type, options_type, proto)                   \
  if (google_protobuf_##desc_type##_has_options(proto)) {                     \
    size_t size;                                                              \
    char* pb = google_protobuf_##options_type##_serialize(                    \
        google_protobuf_##desc_type##_options(proto), ctx->tmp_arena, &size); \
    CHK_OOM(pb);                                                              \
    target = google_protobuf_##options_type##_parse(pb, size, ctx->arena);    \
    CHK_OOM(target);                                                          \
  } else {                                                                    \
    target = (const google_protobuf_##options_type*)opt_default;              \
  }

static void check_ident(symtab_addctx* ctx, upb_StringView name, bool full) {
  const char* str = name.data;
  size_t len = name.size;
  bool start = true;
  size_t i;
  for (i = 0; i < len; i++) {
    char c = str[i];
    if (c == '.') {
      if (start || !full) {
        symtab_errf(ctx, "invalid name: unexpected '.' (%.*s)", (int)len, str);
      }
      start = true;
    } else if (start) {
      if (!upb_isletter(c)) {
        symtab_errf(
            ctx,
            "invalid name: path components must start with a letter (%.*s)",
            (int)len, str);
      }
      start = false;
    } else {
      if (!upb_isalphanum(c)) {
        symtab_errf(ctx, "invalid name: non-alphanumeric character (%.*s)",
                    (int)len, str);
      }
    }
  }
  if (start) {
    symtab_errf(ctx, "invalid name: empty part (%.*s)", (int)len, str);
  }
}

static size_t div_round_up(size_t n, size_t d) { return (n + d - 1) / d; }

static size_t upb_MessageValue_sizeof(upb_CType type) {
  switch (type) {
    case kUpb_CType_Double:
    case kUpb_CType_Int64:
    case kUpb_CType_UInt64:
      return 8;
    case kUpb_CType_Enum:
    case kUpb_CType_Int32:
    case kUpb_CType_UInt32:
    case kUpb_CType_Float:
      return 4;
    case kUpb_CType_Bool:
      return 1;
    case kUpb_CType_Message:
      return sizeof(void*);
    case kUpb_CType_Bytes:
    case kUpb_CType_String:
      return sizeof(upb_StringView);
  }
  UPB_UNREACHABLE();
}

static uint8_t upb_msg_fielddefsize(const upb_FieldDef* f) {
  if (upb_MessageDef_IsMapEntry(upb_FieldDef_ContainingType(f))) {
    upb_MapEntry ent;
    UPB_ASSERT(sizeof(ent.k) == sizeof(ent.v));
    return sizeof(ent.k);
  } else if (upb_FieldDef_IsRepeated(f)) {
    return sizeof(void*);
  } else {
    return upb_MessageValue_sizeof(upb_FieldDef_CType(f));
  }
}

static uint32_t upb_MiniTable_place(symtab_addctx* ctx, upb_MiniTable* l,
                                    size_t size, const upb_MessageDef* m) {
  size_t ofs = UPB_ALIGN_UP(l->size, size);
  size_t next = ofs + size;

  if (next > UINT16_MAX) {
    symtab_errf(ctx, "size of message %s exceeded max size of %zu bytes",
                upb_MessageDef_FullName(m), (size_t)UINT16_MAX);
  }

  l->size = next;
  return ofs;
}

static int field_number_cmp(const void* p1, const void* p2) {
  const upb_MiniTable_Field* f1 = p1;
  const upb_MiniTable_Field* f2 = p2;
  return f1->number - f2->number;
}

static void assign_layout_indices(const upb_MessageDef* m, upb_MiniTable* l,
                                  upb_MiniTable_Field* fields) {
  int i;
  int n = upb_MessageDef_numfields(m);
  int dense_below = 0;
  for (i = 0; i < n; i++) {
    upb_FieldDef* f =
        (upb_FieldDef*)upb_MessageDef_FindFieldByNumber(m, fields[i].number);
    UPB_ASSERT(f);
    f->layout_index = i;
    if (i < UINT8_MAX && fields[i].number == i + 1 &&
        (i == 0 || fields[i - 1].number == i)) {
      dense_below = i + 1;
    }
  }
  l->dense_below = dense_below;
}

static uint8_t map_descriptortype(const upb_FieldDef* f) {
  uint8_t type = upb_FieldDef_Type(f);
  /* See TableDescriptorType() in upbc/generator.cc for details and
   * rationale of these exceptions. */
  if (type == kUpb_FieldType_String && f->file->syntax == kUpb_Syntax_Proto2) {
    return kUpb_FieldType_Bytes;
  } else if (type == kUpb_FieldType_Enum &&
             (f->sub.enumdef->file->syntax == kUpb_Syntax_Proto3 ||
              UPB_TREAT_PROTO2_ENUMS_LIKE_PROTO3 ||
              // TODO(https://github.com/protocolbuffers/upb/issues/541):
              // fix map enum values to check for unknown enum values and put
              // them in the unknown field set.
              upb_MessageDef_IsMapEntry(upb_FieldDef_ContainingType(f)))) {
    return kUpb_FieldType_Int32;
  }
  return type;
}

static void fill_fieldlayout(upb_MiniTable_Field* field,
                             const upb_FieldDef* f) {
  field->number = upb_FieldDef_Number(f);
  field->descriptortype = map_descriptortype(f);

  if (upb_FieldDef_IsMap(f)) {
    field->mode =
        kUpb_FieldMode_Map | (kUpb_FieldRep_Pointer << kUpb_FieldRep_Shift);
  } else if (upb_FieldDef_IsRepeated(f)) {
    field->mode =
        kUpb_FieldMode_Array | (kUpb_FieldRep_Pointer << kUpb_FieldRep_Shift);
  } else {
    /* Maps descriptor type -> elem_size_lg2.  */
    static const uint8_t sizes[] = {
        -1,                       /* invalid descriptor type */
        kUpb_FieldRep_8Byte,      /* DOUBLE */
        kUpb_FieldRep_4Byte,      /* FLOAT */
        kUpb_FieldRep_8Byte,      /* INT64 */
        kUpb_FieldRep_8Byte,      /* UINT64 */
        kUpb_FieldRep_4Byte,      /* INT32 */
        kUpb_FieldRep_8Byte,      /* FIXED64 */
        kUpb_FieldRep_4Byte,      /* FIXED32 */
        kUpb_FieldRep_1Byte,      /* BOOL */
        kUpb_FieldRep_StringView, /* STRING */
        kUpb_FieldRep_Pointer,    /* GROUP */
        kUpb_FieldRep_Pointer,    /* MESSAGE */
        kUpb_FieldRep_StringView, /* BYTES */
        kUpb_FieldRep_4Byte,      /* UINT32 */
        kUpb_FieldRep_4Byte,      /* ENUM */
        kUpb_FieldRep_4Byte,      /* SFIXED32 */
        kUpb_FieldRep_8Byte,      /* SFIXED64 */
        kUpb_FieldRep_4Byte,      /* SINT32 */
        kUpb_FieldRep_8Byte,      /* SINT64 */
    };
    field->mode = kUpb_FieldMode_Scalar |
                  (sizes[field->descriptortype] << kUpb_FieldRep_Shift);
  }

  if (upb_FieldDef_IsPacked(f)) {
    field->mode |= kUpb_LabelFlags_IsPacked;
  }

  if (upb_FieldDef_IsExtension(f)) {
    field->mode |= kUpb_LabelFlags_IsExtension;
  }
}

/* This function is the dynamic equivalent of message_layout.{cc,h} in upbc.
 * It computes a dynamic layout for all of the fields in |m|. */
static void make_layout(symtab_addctx* ctx, const upb_MessageDef* m) {
  upb_MiniTable* l = (upb_MiniTable*)m->layout;
  size_t field_count = upb_MessageDef_numfields(m);
  size_t sublayout_count = 0;
  upb_MiniTable_Sub* subs;
  upb_MiniTable_Field* fields;

  memset(l, 0, sizeof(*l) + sizeof(_upb_FastTable_Entry));

  /* Count sub-messages. */
  for (size_t i = 0; i < field_count; i++) {
    const upb_FieldDef* f = &m->fields[i];
    if (upb_FieldDef_IsSubMessage(f)) {
      sublayout_count++;
    }
    if (upb_FieldDef_CType(f) == kUpb_CType_Enum &&
        f->sub.enumdef->file->syntax == kUpb_Syntax_Proto2) {
      sublayout_count++;
    }
  }

  fields = symtab_alloc(ctx, field_count * sizeof(*fields));
  subs = symtab_alloc(ctx, sublayout_count * sizeof(*subs));

  l->field_count = upb_MessageDef_numfields(m);
  l->fields = fields;
  l->subs = subs;
  l->table_mask = 0;
  l->required_count = 0;

  if (upb_MessageDef_ExtensionRangeCount(m) > 0) {
    if (google_protobuf_MessageOptions_message_set_wire_format(m->opts)) {
      l->ext = kUpb_ExtMode_IsMessageSet;
    } else {
      l->ext = kUpb_ExtMode_Extendable;
    }
  } else {
    l->ext = kUpb_ExtMode_NonExtendable;
  }

  /* TODO(haberman): initialize fast tables so that reflection-based parsing
   * can get the same speeds as linked-in types. */
  l->fasttable[0].field_parser = &fastdecode_generic;
  l->fasttable[0].field_data = 0;

  if (upb_MessageDef_IsMapEntry(m)) {
    /* TODO(haberman): refactor this method so this special case is more
     * elegant. */
    const upb_FieldDef* key = upb_MessageDef_FindFieldByNumber(m, 1);
    const upb_FieldDef* val = upb_MessageDef_FindFieldByNumber(m, 2);
    if (key == NULL || val == NULL) {
      symtab_errf(ctx, "Malformed map entry from message: %s",
                  upb_MessageDef_FullName(m));
    }
    fields[0].number = 1;
    fields[1].number = 2;
    fields[0].mode = kUpb_FieldMode_Scalar;
    fields[1].mode = kUpb_FieldMode_Scalar;
    fields[0].presence = 0;
    fields[1].presence = 0;
    fields[0].descriptortype = map_descriptortype(key);
    fields[1].descriptortype = map_descriptortype(val);
    fields[0].offset = 0;
    fields[1].offset = sizeof(upb_StringView);
    fields[1].submsg_index = 0;

    if (upb_FieldDef_CType(val) == kUpb_CType_Message) {
      subs[0].submsg = upb_FieldDef_MessageSubDef(val)->layout;
    }

    upb_FieldDef* fielddefs = (upb_FieldDef*)&m->fields[0];
    UPB_ASSERT(fielddefs[0].number_ == 1);
    UPB_ASSERT(fielddefs[1].number_ == 2);
    fielddefs[0].layout_index = 0;
    fielddefs[1].layout_index = 1;

    l->field_count = 2;
    l->size = 2 * sizeof(upb_StringView);
    l->size = UPB_ALIGN_UP(l->size, 8);
    l->dense_below = 2;
    return;
  }

  /* Allocate data offsets in three stages:
   *
   * 1. hasbits.
   * 2. regular fields.
   * 3. oneof fields.
   *
   * OPT: There is a lot of room for optimization here to minimize the size.
   */

  /* Assign hasbits for required fields first. */
  size_t hasbit = 0;

  for (int i = 0; i < m->field_count; i++) {
    const upb_FieldDef* f = &m->fields[i];
    upb_MiniTable_Field* field = &fields[upb_FieldDef_Index(f)];
    if (upb_FieldDef_Label(f) == kUpb_Label_Required) {
      field->presence = ++hasbit;
      if (hasbit >= 63) {
        symtab_errf(ctx, "Message with >=63 required fields: %s",
                    upb_MessageDef_FullName(m));
      }
      l->required_count++;
    }
  }

  /* Allocate hasbits and set basic field attributes. */
  sublayout_count = 0;
  for (int i = 0; i < m->field_count; i++) {
    const upb_FieldDef* f = &m->fields[i];
    upb_MiniTable_Field* field = &fields[upb_FieldDef_Index(f)];

    fill_fieldlayout(field, f);

    if (field->descriptortype == kUpb_FieldType_Message ||
        field->descriptortype == kUpb_FieldType_Group) {
      field->submsg_index = sublayout_count++;
      subs[field->submsg_index].submsg = upb_FieldDef_MessageSubDef(f)->layout;
    } else if (field->descriptortype == kUpb_FieldType_Enum) {
      field->submsg_index = sublayout_count++;
      subs[field->submsg_index].subenum = upb_FieldDef_EnumSubDef(f)->layout;
      UPB_ASSERT(subs[field->submsg_index].subenum);
    }

    if (upb_FieldDef_Label(f) == kUpb_Label_Required) {
      /* Hasbit was already assigned. */
    } else if (upb_FieldDef_HasPresence(f) &&
               !upb_FieldDef_RealContainingOneof(f)) {
      /* We don't use hasbit 0, so that 0 can indicate "no presence" in the
       * table. This wastes one hasbit, but we don't worry about it for now. */
      field->presence = ++hasbit;
    } else {
      field->presence = 0;
    }
  }

  /* Account for space used by hasbits. */
  l->size = hasbit ? div_round_up(hasbit + 1, 8) : 0;

  /* Allocate non-oneof fields. */
  for (int i = 0; i < m->field_count; i++) {
    const upb_FieldDef* f = &m->fields[i];
    size_t field_size = upb_msg_fielddefsize(f);
    size_t index = upb_FieldDef_Index(f);

    if (upb_FieldDef_RealContainingOneof(f)) {
      /* Oneofs are handled separately below. */
      continue;
    }

    fields[index].offset = upb_MiniTable_place(ctx, l, field_size, m);
  }

  /* Allocate oneof fields.  Each oneof field consists of a uint32 for the case
   * and space for the actual data. */
  for (int i = 0; i < m->oneof_count; i++) {
    const upb_OneofDef* o = &m->oneofs[i];
    size_t case_size = sizeof(uint32_t); /* Could potentially optimize this. */
    size_t field_size = 0;
    uint32_t case_offset;
    uint32_t data_offset;

    if (upb_OneofDef_IsSynthetic(o)) continue;

    if (o->field_count == 0) {
      symtab_errf(ctx, "Oneof must have at least one field (%s)", o->full_name);
    }

    /* Calculate field size: the max of all field sizes. */
    for (int j = 0; j < o->field_count; j++) {
      const upb_FieldDef* f = o->fields[j];
      field_size = UPB_MAX(field_size, upb_msg_fielddefsize(f));
    }

    /* Align and allocate case offset. */
    case_offset = upb_MiniTable_place(ctx, l, case_size, m);
    data_offset = upb_MiniTable_place(ctx, l, field_size, m);

    for (int i = 0; i < o->field_count; i++) {
      const upb_FieldDef* f = o->fields[i];
      fields[upb_FieldDef_Index(f)].offset = data_offset;
      fields[upb_FieldDef_Index(f)].presence = ~case_offset;
    }
  }

  /* Size of the entire structure should be a multiple of its greatest
   * alignment.  TODO: track overall alignment for real? */
  l->size = UPB_ALIGN_UP(l->size, 8);

  /* Sort fields by number. */
  if (fields) {
    qsort(fields, upb_MessageDef_numfields(m), sizeof(*fields),
          field_number_cmp);
  }
  assign_layout_indices(m, l, fields);
}

static char* strviewdup(symtab_addctx* ctx, upb_StringView view) {
  char* ret = upb_strdup2(view.data, view.size, ctx->arena);
  CHK_OOM(ret);
  return ret;
}

static bool streql2(const char* a, size_t n, const char* b) {
  return n == strlen(b) && memcmp(a, b, n) == 0;
}

static bool streql_view(upb_StringView view, const char* b) {
  return streql2(view.data, view.size, b);
}

static const char* makefullname(symtab_addctx* ctx, const char* prefix,
                                upb_StringView name) {
  if (prefix) {
    /* ret = prefix + '.' + name; */
    size_t n = strlen(prefix);
    char* ret = symtab_alloc(ctx, n + name.size + 2);
    strcpy(ret, prefix);
    ret[n] = '.';
    memcpy(&ret[n + 1], name.data, name.size);
    ret[n + 1 + name.size] = '\0';
    return ret;
  } else {
    return strviewdup(ctx, name);
  }
}

static void finalize_oneofs(symtab_addctx* ctx, upb_MessageDef* m) {
  int i;
  int synthetic_count = 0;
  upb_OneofDef* mutable_oneofs = (upb_OneofDef*)m->oneofs;

  for (i = 0; i < m->oneof_count; i++) {
    upb_OneofDef* o = &mutable_oneofs[i];

    if (o->synthetic && o->field_count != 1) {
      symtab_errf(ctx, "Synthetic oneofs must have one field, not %d: %s",
                  o->field_count, upb_OneofDef_Name(o));
    }

    if (o->synthetic) {
      synthetic_count++;
    } else if (synthetic_count != 0) {
      symtab_errf(ctx, "Synthetic oneofs must be after all other oneofs: %s",
                  upb_OneofDef_Name(o));
    }

    o->fields = symtab_alloc(ctx, sizeof(upb_FieldDef*) * o->field_count);
    o->field_count = 0;
  }

  for (i = 0; i < m->field_count; i++) {
    const upb_FieldDef* f = &m->fields[i];
    upb_OneofDef* o = (upb_OneofDef*)upb_FieldDef_ContainingOneof(f);
    if (o) {
      o->fields[o->field_count++] = f;
    }
  }

  m->real_oneof_count = m->oneof_count - synthetic_count;
}

size_t getjsonname(const char* name, char* buf, size_t len) {
  size_t src, dst = 0;
  bool ucase_next = false;

#define WRITE(byte)      \
  ++dst;                 \
  if (dst < len)         \
    buf[dst - 1] = byte; \
  else if (dst == len)   \
  buf[dst - 1] = '\0'

  if (!name) {
    WRITE('\0');
    return 0;
  }

  /* Implement the transformation as described in the spec:
   *   1. upper case all letters after an underscore.
   *   2. remove all underscores.
   */
  for (src = 0; name[src]; src++) {
    if (name[src] == '_') {
      ucase_next = true;
      continue;
    }

    if (ucase_next) {
      WRITE(toupper(name[src]));
      ucase_next = false;
    } else {
      WRITE(name[src]);
    }
  }

  WRITE('\0');
  return dst;

#undef WRITE
}

static char* makejsonname(symtab_addctx* ctx, const char* name) {
  size_t size = getjsonname(name, NULL, 0);
  char* json_name = symtab_alloc(ctx, size);
  getjsonname(name, json_name, size);
  return json_name;
}

/* Adds a symbol |v| to the symtab, which must be a def pointer previously
 * packed with pack_def().  The def's pointer to upb_FileDef* must be set before
 * adding, so we know which entries to remove if building this file fails. */
static void symtab_add(symtab_addctx* ctx, const char* name, upb_value v) {
  // TODO: table should support an operation "tryinsert" to avoid the double
  // lookup.
  if (upb_strtable_lookup(&ctx->symtab->syms, name, NULL)) {
    symtab_errf(ctx, "duplicate symbol '%s'", name);
  }
  size_t len = strlen(name);
  CHK_OOM(upb_strtable_insert(&ctx->symtab->syms, name, len, v,
                              ctx->symtab->arena));
}

static bool remove_component(char* base, size_t* len) {
  if (*len == 0) return false;

  for (size_t i = *len - 1; i > 0; i--) {
    if (base[i] == '.') {
      *len = i;
      return true;
    }
  }

  *len = 0;
  return true;
}

/* Given a symbol and the base symbol inside which it is defined, find the
 * symbol's definition in t. */
static const void* symtab_resolveany(symtab_addctx* ctx,
                                     const char* from_name_dbg,
                                     const char* base, upb_StringView sym,
                                     upb_deftype_t* type) {
  const upb_strtable* t = &ctx->symtab->syms;
  if (sym.size == 0) goto notfound;
  upb_value v;
  if (sym.data[0] == '.') {
    /* Symbols starting with '.' are absolute, so we do a single lookup.
     * Slice to omit the leading '.' */
    if (!upb_strtable_lookup2(t, sym.data + 1, sym.size - 1, &v)) {
      goto notfound;
    }
  } else {
    /* Remove components from base until we find an entry or run out. */
    size_t baselen = base ? strlen(base) : 0;
    char* tmp = malloc(sym.size + baselen + 1);
    while (1) {
      char* p = tmp;
      if (baselen) {
        memcpy(p, base, baselen);
        p[baselen] = '.';
        p += baselen + 1;
      }
      memcpy(p, sym.data, sym.size);
      p += sym.size;
      if (upb_strtable_lookup2(t, tmp, p - tmp, &v)) {
        break;
      }
      if (!remove_component(tmp, &baselen)) {
        free(tmp);
        goto notfound;
      }
    }
    free(tmp);
  }

  *type = deftype(v);
  return unpack_def(v, *type);

notfound:
  symtab_errf(ctx, "couldn't resolve name '" UPB_STRINGVIEW_FORMAT "'",
              UPB_STRINGVIEW_ARGS(sym));
}

static const void* symtab_resolve(symtab_addctx* ctx, const char* from_name_dbg,
                                  const char* base, upb_StringView sym,
                                  upb_deftype_t type) {
  upb_deftype_t found_type;
  const void* ret =
      symtab_resolveany(ctx, from_name_dbg, base, sym, &found_type);
  if (ret && found_type != type) {
    symtab_errf(ctx,
                "type mismatch when resolving %s: couldn't find "
                "name " UPB_STRINGVIEW_FORMAT " with type=%d",
                from_name_dbg, UPB_STRINGVIEW_ARGS(sym), (int)type);
  }
  return ret;
}

static void create_oneofdef(
    symtab_addctx* ctx, upb_MessageDef* m,
    const google_protobuf_OneofDescriptorProto* oneof_proto,
    const upb_OneofDef* _o) {
  upb_OneofDef* o = (upb_OneofDef*)_o;
  upb_StringView name = google_protobuf_OneofDescriptorProto_name(oneof_proto);
  upb_value v;

  o->parent = m;
  o->full_name = makefullname(ctx, m->full_name, name);
  o->field_count = 0;
  o->synthetic = false;

  SET_OPTIONS(o->opts, OneofDescriptorProto, OneofOptions, oneof_proto);

  upb_value existing_v;
  if (upb_strtable_lookup2(&m->ntof, name.data, name.size, &existing_v)) {
    symtab_errf(ctx, "duplicate oneof name (%s)", o->full_name);
  }

  v = pack_def(o, UPB_DEFTYPE_ONEOF);
  CHK_OOM(upb_strtable_insert(&m->ntof, name.data, name.size, v, ctx->arena));

  CHK_OOM(upb_inttable_init(&o->itof, ctx->arena));
  CHK_OOM(upb_strtable_init(&o->ntof, 4, ctx->arena));
}

static str_t* newstr(symtab_addctx* ctx, const char* data, size_t len) {
  str_t* ret = symtab_alloc(ctx, sizeof(*ret) + len);
  CHK_OOM(ret);
  ret->len = len;
  if (len) memcpy(ret->str, data, len);
  ret->str[len] = '\0';
  return ret;
}

static bool upb_DefPool_TryGetChar(const char** src, const char* end,
                                   char* ch) {
  if (*src == end) return false;
  *ch = **src;
  *src += 1;
  return true;
}

static char upb_DefPool_TryGetHexDigit(symtab_addctx* ctx,
                                       const upb_FieldDef* f, const char** src,
                                       const char* end) {
  char ch;
  if (!upb_DefPool_TryGetChar(src, end, &ch)) return -1;
  if ('0' <= ch && ch <= '9') {
    return ch - '0';
  }
  ch = upb_ascii_lower(ch);
  if ('a' <= ch && ch <= 'f') {
    return ch - 'a' + 0xa;
  }
  *src -= 1;  // Char wasn't actually a hex digit.
  return -1;
}

static char upb_DefPool_ParseHexEscape(symtab_addctx* ctx,
                                       const upb_FieldDef* f, const char** src,
                                       const char* end) {
  char hex_digit = upb_DefPool_TryGetHexDigit(ctx, f, src, end);
  if (hex_digit < 0) {
    symtab_errf(ctx,
                "\\x cannot be followed by non-hex digit in field '%s' default",
                upb_FieldDef_FullName(f));
    return 0;
  }
  unsigned int ret = hex_digit;
  while ((hex_digit = upb_DefPool_TryGetHexDigit(ctx, f, src, end)) >= 0) {
    ret = (ret << 4) | hex_digit;
  }
  if (ret > 0xff) {
    symtab_errf(ctx, "Value of hex escape in field %s exceeds 8 bits",
                upb_FieldDef_FullName(f));
    return 0;
  }
  return ret;
}

char upb_DefPool_TryGetOctalDigit(const char** src, const char* end) {
  char ch;
  if (!upb_DefPool_TryGetChar(src, end, &ch)) return -1;
  if ('0' <= ch && ch <= '7') {
    return ch - '0';
  }
  *src -= 1;  // Char wasn't actually an octal digit.
  return -1;
}

static char upb_DefPool_ParseOctalEscape(symtab_addctx* ctx,
                                         const upb_FieldDef* f,
                                         const char** src, const char* end) {
  char ch = 0;
  for (int i = 0; i < 3; i++) {
    char digit;
    if ((digit = upb_DefPool_TryGetOctalDigit(src, end)) >= 0) {
      ch = (ch << 3) | digit;
    }
  }
  return ch;
}

static char upb_DefPool_ParseEscape(symtab_addctx* ctx, const upb_FieldDef* f,
                                    const char** src, const char* end) {
  char ch;
  if (!upb_DefPool_TryGetChar(src, end, &ch)) {
    symtab_errf(ctx, "unterminated escape sequence in field %s",
                upb_FieldDef_FullName(f));
    return 0;
  }
  switch (ch) {
    case 'a':
      return '\a';
    case 'b':
      return '\b';
    case 'f':
      return '\f';
    case 'n':
      return '\n';
    case 'r':
      return '\r';
    case 't':
      return '\t';
    case 'v':
      return '\v';
    case '\\':
      return '\\';
    case '\'':
      return '\'';
    case '\"':
      return '\"';
    case '?':
      return '\?';
    case 'x':
    case 'X':
      return upb_DefPool_ParseHexEscape(ctx, f, src, end);
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
      *src -= 1;
      return upb_DefPool_ParseOctalEscape(ctx, f, src, end);
  }
  symtab_errf(ctx, "Unknown escape sequence: \\%c", ch);
}

static str_t* unescape(symtab_addctx* ctx, const upb_FieldDef* f,
                       const char* data, size_t len) {
  // Size here is an upper bound; escape sequences could ultimately shrink it.
  str_t* ret = symtab_alloc(ctx, sizeof(*ret) + len);
  char* dst = &ret->str[0];
  const char* src = data;
  const char* end = data + len;

  while (src < end) {
    if (*src == '\\') {
      src++;
      *dst++ = upb_DefPool_ParseEscape(ctx, f, &src, end);
    } else {
      *dst++ = *src++;
    }
  }

  ret->len = dst - &ret->str[0];
  return ret;
}

static void parse_default(symtab_addctx* ctx, const char* str, size_t len,
                          upb_FieldDef* f) {
  char* end;
  char nullz[64];
  errno = 0;

  switch (upb_FieldDef_CType(f)) {
    case kUpb_CType_Int32:
    case kUpb_CType_Int64:
    case kUpb_CType_UInt32:
    case kUpb_CType_UInt64:
    case kUpb_CType_Double:
    case kUpb_CType_Float:
      /* Standard C number parsing functions expect null-terminated strings. */
      if (len >= sizeof(nullz) - 1) {
        symtab_errf(ctx, "Default too long: %.*s", (int)len, str);
      }
      memcpy(nullz, str, len);
      nullz[len] = '\0';
      str = nullz;
      break;
    default:
      break;
  }

  switch (upb_FieldDef_CType(f)) {
    case kUpb_CType_Int32: {
      long val = strtol(str, &end, 0);
      if (val > INT32_MAX || val < INT32_MIN || errno == ERANGE || *end) {
        goto invalid;
      }
      f->defaultval.sint = val;
      break;
    }
    case kUpb_CType_Enum: {
      const upb_EnumDef* e = f->sub.enumdef;
      const upb_EnumValueDef* ev =
          upb_EnumDef_FindValueByNameWithSize(e, str, len);
      if (!ev) {
        goto invalid;
      }
      f->defaultval.sint = ev->number;
      break;
    }
    case kUpb_CType_Int64: {
      long long val = strtoll(str, &end, 0);
      if (val > INT64_MAX || val < INT64_MIN || errno == ERANGE || *end) {
        goto invalid;
      }
      f->defaultval.sint = val;
      break;
    }
    case kUpb_CType_UInt32: {
      unsigned long val = strtoul(str, &end, 0);
      if (val > UINT32_MAX || errno == ERANGE || *end) {
        goto invalid;
      }
      f->defaultval.uint = val;
      break;
    }
    case kUpb_CType_UInt64: {
      unsigned long long val = strtoull(str, &end, 0);
      if (val > UINT64_MAX || errno == ERANGE || *end) {
        goto invalid;
      }
      f->defaultval.uint = val;
      break;
    }
    case kUpb_CType_Double: {
      double val = strtod(str, &end);
      if (errno == ERANGE || *end) {
        goto invalid;
      }
      f->defaultval.dbl = val;
      break;
    }
    case kUpb_CType_Float: {
      float val = strtof(str, &end);
      if (errno == ERANGE || *end) {
        goto invalid;
      }
      f->defaultval.flt = val;
      break;
    }
    case kUpb_CType_Bool: {
      if (streql2(str, len, "false")) {
        f->defaultval.boolean = false;
      } else if (streql2(str, len, "true")) {
        f->defaultval.boolean = true;
      } else {
        goto invalid;
      }
      break;
    }
    case kUpb_CType_String:
      f->defaultval.str = newstr(ctx, str, len);
      break;
    case kUpb_CType_Bytes:
      f->defaultval.str = unescape(ctx, f, str, len);
      break;
    case kUpb_CType_Message:
      /* Should not have a default value. */
      symtab_errf(ctx, "Message should not have a default (%s)",
                  upb_FieldDef_FullName(f));
  }

  return;

invalid:
  symtab_errf(ctx, "Invalid default '%.*s' for field %s of type %d", (int)len,
              str, upb_FieldDef_FullName(f), (int)upb_FieldDef_Type(f));
}

static void set_default_default(symtab_addctx* ctx, upb_FieldDef* f) {
  switch (upb_FieldDef_CType(f)) {
    case kUpb_CType_Int32:
    case kUpb_CType_Int64:
      f->defaultval.sint = 0;
      break;
    case kUpb_CType_UInt64:
    case kUpb_CType_UInt32:
      f->defaultval.uint = 0;
      break;
    case kUpb_CType_Double:
    case kUpb_CType_Float:
      f->defaultval.dbl = 0;
      break;
    case kUpb_CType_String:
    case kUpb_CType_Bytes:
      f->defaultval.str = newstr(ctx, NULL, 0);
      break;
    case kUpb_CType_Bool:
      f->defaultval.boolean = false;
      break;
    case kUpb_CType_Enum:
      f->defaultval.sint = f->sub.enumdef->values[0].number;
    case kUpb_CType_Message:
      break;
  }
}

static void create_fielddef(
    symtab_addctx* ctx, const char* prefix, upb_MessageDef* m,
    const google_protobuf_FieldDescriptorProto* field_proto,
    const upb_FieldDef* _f, bool is_extension) {
  upb_FieldDef* f = (upb_FieldDef*)_f;
  upb_StringView name;
  const char* full_name;
  const char* json_name;
  const char* shortname;
  int32_t field_number;

  f->file = ctx->file; /* Must happen prior to symtab_add(). */

  if (!google_protobuf_FieldDescriptorProto_has_name(field_proto)) {
    symtab_errf(ctx, "field has no name");
  }

  name = google_protobuf_FieldDescriptorProto_name(field_proto);
  check_ident(ctx, name, false);
  full_name = makefullname(ctx, prefix, name);
  shortname = shortdefname(full_name);

  if (google_protobuf_FieldDescriptorProto_has_json_name(field_proto)) {
    json_name = strviewdup(
        ctx, google_protobuf_FieldDescriptorProto_json_name(field_proto));
    f->has_json_name_ = true;
  } else {
    json_name = makejsonname(ctx, shortname);
    f->has_json_name_ = false;
  }

  field_number = google_protobuf_FieldDescriptorProto_number(field_proto);

  f->full_name = full_name;
  f->json_name = json_name;
  f->label_ = (int)google_protobuf_FieldDescriptorProto_label(field_proto);
  f->number_ = field_number;
  f->scope.oneof = NULL;
  f->proto3_optional_ =
      google_protobuf_FieldDescriptorProto_proto3_optional(field_proto);

  bool has_type = google_protobuf_FieldDescriptorProto_has_type(field_proto);
  bool has_type_name =
      google_protobuf_FieldDescriptorProto_has_type_name(field_proto);

  f->type_ = (int)google_protobuf_FieldDescriptorProto_type(field_proto);

  if (has_type) {
    switch (f->type_) {
      case kUpb_FieldType_Message:
      case kUpb_FieldType_Group:
      case kUpb_FieldType_Enum:
        if (!has_type_name) {
          symtab_errf(ctx, "field of type %d requires type name (%s)",
                      (int)f->type_, full_name);
        }
        break;
      default:
        if (has_type_name) {
          symtab_errf(ctx, "invalid type for field with type_name set (%s, %d)",
                      full_name, (int)f->type_);
        }
    }
  } else if (has_type_name) {
    f->type_ =
        FIELD_TYPE_UNSPECIFIED;  // We'll fill this in in resolve_fielddef().
  }

  if (!is_extension) {
    /* direct message field. */
    upb_value v, field_v, json_v, existing_v;
    size_t json_size;

    if (field_number <= 0 || field_number > kUpb_MaxFieldNumber) {
      symtab_errf(ctx, "invalid field number (%u)", field_number);
    }

    f->index_ = f - m->fields;
    f->msgdef = m;
    f->is_extension_ = false;

    field_v = pack_def(f, UPB_DEFTYPE_FIELD);
    json_v = pack_def(f, UPB_DEFTYPE_FIELD_JSONNAME);
    v = upb_value_constptr(f);
    json_size = strlen(json_name);

    if (upb_strtable_lookup(&m->ntof, shortname, &existing_v)) {
      symtab_errf(ctx, "duplicate field name (%s)", shortname);
    }

    CHK_OOM(upb_strtable_insert(&m->ntof, name.data, name.size, field_v,
                                ctx->arena));

    if (strcmp(shortname, json_name) != 0) {
      if (upb_strtable_lookup(&m->ntof, json_name, &v)) {
        symtab_errf(ctx, "duplicate json_name (%s)", json_name);
      } else {
        CHK_OOM(upb_strtable_insert(&m->ntof, json_name, json_size, json_v,
                                    ctx->arena));
      }
    }

    if (upb_inttable_lookup(&m->itof, field_number, NULL)) {
      symtab_errf(ctx, "duplicate field number (%u)", field_number);
    }

    CHK_OOM(upb_inttable_insert(&m->itof, field_number, v, ctx->arena));

    if (ctx->layout) {
      const upb_MiniTable_Field* fields = m->layout->fields;
      int count = m->layout->field_count;
      bool found = false;
      for (int i = 0; i < count; i++) {
        if (fields[i].number == field_number) {
          f->layout_index = i;
          found = true;
          break;
        }
      }
      UPB_ASSERT(found);
    }
  } else {
    /* extension field. */
    f->is_extension_ = true;
    f->scope.extension_scope = m;
    symtab_add(ctx, full_name, pack_def(f, UPB_DEFTYPE_EXT));
    f->layout_index = ctx->ext_count++;
    if (ctx->layout) {
      UPB_ASSERT(ctx->file->ext_layouts[f->layout_index]->field.number ==
                 field_number);
    }
  }

  if (f->type_ < kUpb_FieldType_Double || f->type_ > kUpb_FieldType_SInt64) {
    symtab_errf(ctx, "invalid type for field %s (%d)", f->full_name, f->type_);
  }

  if (f->label_ < kUpb_Label_Optional || f->label_ > kUpb_Label_Repeated) {
    symtab_errf(ctx, "invalid label for field %s (%d)", f->full_name,
                f->label_);
  }

  /* We can't resolve the subdef or (in the case of extensions) the containing
   * message yet, because it may not have been defined yet.  We stash a pointer
   * to the field_proto until later when we can properly resolve it. */
  f->sub.unresolved = field_proto;

  if (f->label_ == kUpb_Label_Required &&
      f->file->syntax == kUpb_Syntax_Proto3) {
    symtab_errf(ctx, "proto3 fields cannot be required (%s)", f->full_name);
  }

  if (google_protobuf_FieldDescriptorProto_has_oneof_index(field_proto)) {
    uint32_t oneof_index = google_protobuf_FieldDescriptorProto_oneof_index(field_proto);
    upb_OneofDef* oneof;
    upb_value v = upb_value_constptr(f);

    if (upb_FieldDef_Label(f) != kUpb_Label_Optional) {
      symtab_errf(ctx, "fields in oneof must have OPTIONAL label (%s)",
                  f->full_name);
    }

    if (!m) {
      symtab_errf(ctx, "oneof_index provided for extension field (%s)",
                  f->full_name);
    }

    if (oneof_index >= m->oneof_count) {
      symtab_errf(ctx, "oneof_index out of range (%s)", f->full_name);
    }

    oneof = (upb_OneofDef*)&m->oneofs[oneof_index];
    f->scope.oneof = oneof;

    oneof->field_count++;
    if (f->proto3_optional_) {
      oneof->synthetic = true;
    }
    CHK_OOM(upb_inttable_insert(&oneof->itof, f->number_, v, ctx->arena));
    CHK_OOM(
        upb_strtable_insert(&oneof->ntof, name.data, name.size, v, ctx->arena));
  } else {
    if (f->proto3_optional_ && !is_extension) {
      symtab_errf(ctx, "field with proto3_optional was not in a oneof (%s)",
                  f->full_name);
    }
  }

  SET_OPTIONS(f->opts, FieldDescriptorProto, FieldOptions, field_proto);

  if (google_protobuf_FieldOptions_has_packed(f->opts)) {
    f->packed_ = google_protobuf_FieldOptions_packed(f->opts);
  } else {
    /* Repeated fields default to packed for proto3 only. */
    f->packed_ = upb_FieldDef_IsPrimitive(f) &&
                 f->label_ == kUpb_Label_Repeated &&
                 f->file->syntax == kUpb_Syntax_Proto3;
  }
}

static void create_service(
    symtab_addctx* ctx, const google_protobuf_ServiceDescriptorProto* svc_proto,
    const upb_ServiceDef* _s) {
  upb_ServiceDef* s = (upb_ServiceDef*)_s;
  upb_StringView name;
  const google_protobuf_MethodDescriptorProto* const* methods;
  size_t i, n;

  s->file = ctx->file; /* Must happen prior to symtab_add. */

  name = google_protobuf_ServiceDescriptorProto_name(svc_proto);
  check_ident(ctx, name, false);
  s->full_name = makefullname(ctx, ctx->file->package, name);
  symtab_add(ctx, s->full_name, pack_def(s, UPB_DEFTYPE_SERVICE));

  methods = google_protobuf_ServiceDescriptorProto_method(svc_proto, &n);

  s->method_count = n;
  s->methods = symtab_alloc(ctx, sizeof(*s->methods) * n);

  SET_OPTIONS(s->opts, ServiceDescriptorProto, ServiceOptions, svc_proto);

  for (i = 0; i < n; i++) {
    const google_protobuf_MethodDescriptorProto* method_proto = methods[i];
    upb_MethodDef* m = (upb_MethodDef*)&s->methods[i];
    upb_StringView name =
        google_protobuf_MethodDescriptorProto_name(method_proto);

    m->service = s;
    m->full_name = makefullname(ctx, s->full_name, name);
    m->index = i;
    m->client_streaming =
        google_protobuf_MethodDescriptorProto_client_streaming(method_proto);
    m->server_streaming =
        google_protobuf_MethodDescriptorProto_server_streaming(method_proto);
    m->input_type = symtab_resolve(
        ctx, m->full_name, m->full_name,
        google_protobuf_MethodDescriptorProto_input_type(method_proto),
        UPB_DEFTYPE_MSG);
    m->output_type = symtab_resolve(
        ctx, m->full_name, m->full_name,
        google_protobuf_MethodDescriptorProto_output_type(method_proto),
        UPB_DEFTYPE_MSG);

    SET_OPTIONS(m->opts, MethodDescriptorProto, MethodOptions, method_proto);
  }
}

static int count_bits_debug(uint64_t x) {
  // For assertions only, speed does not matter.
  int n = 0;
  while (x) {
    if (x & 1) n++;
    x >>= 1;
  }
  return n;
}

static int compare_int32(const void* a_ptr, const void* b_ptr) {
  int32_t a = *(int32_t*)a_ptr;
  int32_t b = *(int32_t*)b_ptr;
  return a < b ? -1 : (a == b ? 0 : 1);
}

upb_MiniTable_Enum* create_enumlayout(symtab_addctx* ctx,
                                      const upb_EnumDef* e) {
  int n = 0;
  uint64_t mask = 0;

  for (int i = 0; i < e->value_count; i++) {
    uint32_t val = (uint32_t)e->values[i].number;
    if (val < 64) {
      mask |= 1ULL << val;
    } else {
      n++;
    }
  }

  int32_t* values = symtab_alloc(ctx, sizeof(*values) * n);

  if (n) {
    int32_t* p = values;

    // Add values outside the bitmask range to the list, as described in the
    // comments for upb_MiniTable_Enum.
    for (int i = 0; i < e->value_count; i++) {
      int32_t val = e->values[i].number;
      if ((uint32_t)val >= 64) {
        *p++ = val;
      }
    }
    UPB_ASSERT(p == values + n);
  }

  // Enums can have duplicate values; we must sort+uniq them.
  if (values) qsort(values, n, sizeof(*values), &compare_int32);

  int dst = 0;
  for (int i = 0; i < n; dst++) {
    int32_t val = values[i];
    while (i < n && values[i] == val) i++;  // Skip duplicates.
    values[dst] = val;
  }
  n = dst;

  UPB_ASSERT(upb_inttable_count(&e->iton) == n + count_bits_debug(mask));

  upb_MiniTable_Enum* layout = symtab_alloc(ctx, sizeof(*layout));
  layout->value_count = n;
  layout->mask = mask;
  layout->values = values;

  return layout;
}

static void create_enumvaldef(
    symtab_addctx* ctx, const char* prefix,
    const google_protobuf_EnumValueDescriptorProto* val_proto, upb_EnumDef* e,
    int i) {
  upb_EnumValueDef* val = (upb_EnumValueDef*)&e->values[i];
  upb_StringView name =
      google_protobuf_EnumValueDescriptorProto_name(val_proto);
  upb_value v = upb_value_constptr(val);

  val->parent = e; /* Must happen prior to symtab_add(). */
  val->full_name = makefullname(ctx, prefix, name);
  val->number = google_protobuf_EnumValueDescriptorProto_number(val_proto);
  symtab_add(ctx, val->full_name, pack_def(val, UPB_DEFTYPE_ENUMVAL));

  SET_OPTIONS(val->opts, EnumValueDescriptorProto, EnumValueOptions, val_proto);

  if (i == 0 && e->file->syntax == kUpb_Syntax_Proto3 && val->number != 0) {
    symtab_errf(ctx, "for proto3, the first enum value must be zero (%s)",
                e->full_name);
  }

  CHK_OOM(upb_strtable_insert(&e->ntoi, name.data, name.size, v, ctx->arena));

  // Multiple enumerators can have the same number, first one wins.
  if (!upb_inttable_lookup(&e->iton, val->number, NULL)) {
    CHK_OOM(upb_inttable_insert(&e->iton, val->number, v, ctx->arena));
  }
}

static upb_StringView* _upb_EnumReservedNames_New(
    symtab_addctx* ctx, int n, const upb_StringView* protos) {
  upb_StringView* sv =
      upb_Arena_Malloc(ctx->arena, sizeof(upb_StringView) * n);
  for (size_t i = 0; i < n; i++) {
    sv[i].data =
        upb_strdup2(protos[i].data, protos[i].size, ctx->arena);
    sv[i].size = protos[i].size;
  }
  return sv;
}

static void create_enumdef(
    symtab_addctx* ctx, const char* prefix,
    const google_protobuf_EnumDescriptorProto* enum_proto,
    const upb_MessageDef* containing_type, const upb_EnumDef* _e) {
  upb_EnumDef* e = (upb_EnumDef*)_e;
  ;
  const google_protobuf_EnumValueDescriptorProto* const* values;
  const google_protobuf_EnumDescriptorProto_EnumReservedRange* const* res_ranges;
  const upb_StringView* res_names;
  upb_StringView name;
  size_t i, n, n_res_range, n_res_name;

  e->file = ctx->file; /* Must happen prior to symtab_add() */
  e->containing_type = containing_type;

  name = google_protobuf_EnumDescriptorProto_name(enum_proto);
  check_ident(ctx, name, false);

  e->full_name = makefullname(ctx, prefix, name);
  symtab_add(ctx, e->full_name, pack_def(e, UPB_DEFTYPE_ENUM));

  values = google_protobuf_EnumDescriptorProto_value(enum_proto, &n);
  CHK_OOM(upb_strtable_init(&e->ntoi, n, ctx->arena));
  CHK_OOM(upb_inttable_init(&e->iton, ctx->arena));

  e->defaultval = 0;
  e->value_count = n;
  e->values = symtab_alloc(ctx, sizeof(*e->values) * n);

  if (n == 0) {
    symtab_errf(ctx, "enums must contain at least one value (%s)",
                e->full_name);
  }

  res_ranges =
      google_protobuf_EnumDescriptorProto_reserved_range(enum_proto, &n_res_range);
  e->res_range_count = n_res_range;
  e->res_ranges = _upb_EnumReservedRanges_New(ctx, n_res_range, res_ranges, e);

  res_names = google_protobuf_EnumDescriptorProto_reserved_name(enum_proto, &n_res_name);
  e->res_name_count = n_res_name;
  e->res_names = _upb_EnumReservedNames_New(ctx, n_res_name, res_names);

  SET_OPTIONS(e->opts, EnumDescriptorProto, EnumOptions, enum_proto);

  for (i = 0; i < n; i++) {
    create_enumvaldef(ctx, prefix, values[i], e, i);
  }

  upb_inttable_compact(&e->iton, ctx->arena);

  if (e->file->syntax == kUpb_Syntax_Proto2) {
    if (ctx->layout) {
      UPB_ASSERT(ctx->enum_count < ctx->layout->enum_count);
      e->layout = ctx->layout->enums[ctx->enum_count++];
      UPB_ASSERT(upb_inttable_count(&e->iton) ==
                 e->layout->value_count + count_bits_debug(e->layout->mask));
    } else {
      e->layout = create_enumlayout(ctx, e);
    }
  } else {
    e->layout = NULL;
  }
}

static void msgdef_create_nested(
    symtab_addctx* ctx, const google_protobuf_DescriptorProto* msg_proto,
    upb_MessageDef* m);

static upb_StringView* _upb_ReservedNames_New(symtab_addctx* ctx, int n,
                                              const upb_StringView* protos) {
  upb_StringView* sv = upb_Arena_Malloc(ctx->arena, sizeof(upb_StringView) * n);
  for (size_t i = 0; i < n; i++) {
    sv[i].data =
        upb_strdup2(protos[i].data, protos[i].size, ctx->arena);
    sv[i].size = protos[i].size;
  }
  return sv;
}

static void create_msgdef(symtab_addctx* ctx, const char* prefix,
                          const google_protobuf_DescriptorProto* msg_proto,
                          const upb_MessageDef* containing_type,
                          const upb_MessageDef* _m) {
  upb_MessageDef* m = (upb_MessageDef*)_m;
  const google_protobuf_OneofDescriptorProto* const* oneofs;
  const google_protobuf_FieldDescriptorProto* const* fields;
  const google_protobuf_DescriptorProto_ExtensionRange* const* ext_ranges;

  const google_protobuf_DescriptorProto_ReservedRange* const* res_ranges;
  const upb_StringView* res_names;
  size_t i, n_oneof, n_field, n_ext_range;
  size_t n_res_range, n_res_name;
  upb_StringView name;

  m->file = ctx->file; /* Must happen prior to symtab_add(). */
  m->containing_type = containing_type;

  name = google_protobuf_DescriptorProto_name(msg_proto);
  check_ident(ctx, name, false);

  m->full_name = makefullname(ctx, prefix, name);
  symtab_add(ctx, m->full_name, pack_def(m, UPB_DEFTYPE_MSG));

  oneofs = google_protobuf_DescriptorProto_oneof_decl(msg_proto, &n_oneof);
  fields = google_protobuf_DescriptorProto_field(msg_proto, &n_field);
  ext_ranges =
      google_protobuf_DescriptorProto_extension_range(msg_proto, &n_ext_range);
  res_ranges = google_protobuf_DescriptorProto_reserved_range(msg_proto, &n_res_range);
  res_names = google_protobuf_DescriptorProto_reserved_name(msg_proto, &n_res_name);

  CHK_OOM(upb_inttable_init(&m->itof, ctx->arena));
  CHK_OOM(upb_strtable_init(&m->ntof, n_oneof + n_field, ctx->arena));

  if (ctx->layout) {
    /* create_fielddef() below depends on this being set. */
    UPB_ASSERT(ctx->msg_count < ctx->layout->msg_count);
    m->layout = ctx->layout->msgs[ctx->msg_count++];
    UPB_ASSERT(n_field == m->layout->field_count);
  } else {
    /* Allocate now (to allow cross-linking), populate later. */
    m->layout =
        symtab_alloc(ctx, sizeof(*m->layout) + sizeof(_upb_FastTable_Entry));
  }

  SET_OPTIONS(m->opts, DescriptorProto, MessageOptions, msg_proto);

  m->oneof_count = n_oneof;
  m->oneofs = symtab_alloc(ctx, sizeof(*m->oneofs) * n_oneof);
  for (i = 0; i < n_oneof; i++) {
    create_oneofdef(ctx, m, oneofs[i], &m->oneofs[i]);
  }

  m->field_count = n_field;
  m->fields = symtab_alloc(ctx, sizeof(*m->fields) * n_field);
  for (i = 0; i < n_field; i++) {
    create_fielddef(ctx, m->full_name, m, fields[i], &m->fields[i],
                    /* is_extension= */ false);
  }

  m->ext_range_count = n_ext_range;
  m->ext_ranges = symtab_alloc(ctx, sizeof(*m->ext_ranges) * n_ext_range);
  for (i = 0; i < n_ext_range; i++) {
    const google_protobuf_DescriptorProto_ExtensionRange* r = ext_ranges[i];
    upb_ExtensionRange* r_def = (upb_ExtensionRange*)&m->ext_ranges[i];
    int32_t start = google_protobuf_DescriptorProto_ExtensionRange_start(r);
    int32_t end = google_protobuf_DescriptorProto_ExtensionRange_end(r);
    int32_t max =
        google_protobuf_MessageOptions_message_set_wire_format(m->opts)
            ? INT32_MAX
            : kUpb_MaxFieldNumber + 1;

    // A full validation would also check that each range is disjoint, and that
    // none of the fields overlap with the extension ranges, but we are just
    // sanity checking here.
    if (start < 1 || end <= start || end > max) {
      symtab_errf(ctx, "Extension range (%d, %d) is invalid, message=%s\n",
                  (int)start, (int)end, m->full_name);
    }

    r_def->start = start;
    r_def->end = end;
    SET_OPTIONS(r_def->opts, DescriptorProto_ExtensionRange,
                ExtensionRangeOptions, r);
  }

  m->res_range_count = n_res_range;
  m->res_ranges =
      _upb_MessageReservedRanges_New(ctx, n_res_range, res_ranges, m);

  m->res_name_count = n_res_name;
  m->res_names = _upb_ReservedNames_New(ctx, n_res_name, res_names);

  finalize_oneofs(ctx, m);
  assign_msg_wellknowntype(m);
  upb_inttable_compact(&m->itof, ctx->arena);
  msgdef_create_nested(ctx, msg_proto, m);
}

static void msgdef_create_nested(
    symtab_addctx* ctx, const google_protobuf_DescriptorProto* msg_proto,
    upb_MessageDef* m) {
  size_t n;

  const google_protobuf_EnumDescriptorProto* const* enums =
      google_protobuf_DescriptorProto_enum_type(msg_proto, &n);
  m->nested_enum_count = n;
  m->nested_enums = symtab_alloc(ctx, sizeof(*m->nested_enums) * n);
  for (size_t i = 0; i < n; i++) {
    m->nested_enum_count = i + 1;
    create_enumdef(ctx, m->full_name, enums[i], m, &m->nested_enums[i]);
  }

  const google_protobuf_FieldDescriptorProto* const* exts =
      google_protobuf_DescriptorProto_extension(msg_proto, &n);
  m->nested_ext_count = n;
  m->nested_exts = symtab_alloc(ctx, sizeof(*m->nested_exts) * n);
  for (size_t i = 0; i < n; i++) {
    create_fielddef(ctx, m->full_name, m, exts[i], &m->nested_exts[i],
                    /* is_extension= */ true);
    ((upb_FieldDef*)&m->nested_exts[i])->index_ = i;
  }

  const google_protobuf_DescriptorProto* const* msgs =
      google_protobuf_DescriptorProto_nested_type(msg_proto, &n);
  m->nested_msg_count = n;
  m->nested_msgs = symtab_alloc(ctx, sizeof(*m->nested_msgs) * n);
  for (size_t i = 0; i < n; i++) {
    create_msgdef(ctx, m->full_name, msgs[i], m, &m->nested_msgs[i]);
  }
}

static void resolve_subdef(symtab_addctx* ctx, const char* prefix,
                           upb_FieldDef* f) {
  const google_protobuf_FieldDescriptorProto* field_proto = f->sub.unresolved;
  upb_StringView name =
      google_protobuf_FieldDescriptorProto_type_name(field_proto);
  bool has_name =
      google_protobuf_FieldDescriptorProto_has_type_name(field_proto);
  switch ((int)f->type_) {
    case FIELD_TYPE_UNSPECIFIED: {
      // Type was not specified and must be inferred.
      UPB_ASSERT(has_name);
      upb_deftype_t type;
      const void* def =
          symtab_resolveany(ctx, f->full_name, prefix, name, &type);
      switch (type) {
        case UPB_DEFTYPE_ENUM:
          f->sub.enumdef = def;
          f->type_ = kUpb_FieldType_Enum;
          break;
        case UPB_DEFTYPE_MSG:
          f->sub.msgdef = def;
          f->type_ = kUpb_FieldType_Message;  // It appears there is no way of
                                              // this being a group.
          break;
        default:
          symtab_errf(ctx, "Couldn't resolve type name for field %s",
                      f->full_name);
      }
    }
    case kUpb_FieldType_Message:
    case kUpb_FieldType_Group:
      UPB_ASSERT(has_name);
      f->sub.msgdef =
          symtab_resolve(ctx, f->full_name, prefix, name, UPB_DEFTYPE_MSG);
      break;
    case kUpb_FieldType_Enum:
      UPB_ASSERT(has_name);
      f->sub.enumdef =
          symtab_resolve(ctx, f->full_name, prefix, name, UPB_DEFTYPE_ENUM);
      break;
    default:
      // No resolution necessary.
      break;
  }
}

static void resolve_extension(
    symtab_addctx* ctx, const char* prefix, upb_FieldDef* f,
    const google_protobuf_FieldDescriptorProto* field_proto) {
  if (!google_protobuf_FieldDescriptorProto_has_extendee(field_proto)) {
    symtab_errf(ctx, "extension for field '%s' had no extendee", f->full_name);
  }

  upb_StringView name =
      google_protobuf_FieldDescriptorProto_extendee(field_proto);
  const upb_MessageDef* m =
      symtab_resolve(ctx, f->full_name, prefix, name, UPB_DEFTYPE_MSG);
  f->msgdef = m;

  bool found = false;

  for (int i = 0, n = m->ext_range_count; i < n; i++) {
    const upb_ExtensionRange* r = &m->ext_ranges[i];
    if (r->start <= f->number_ && f->number_ < r->end) {
      found = true;
      break;
    }
  }

  if (!found) {
    symtab_errf(ctx,
                "field number %u in extension %s has no extension range in "
                "message %s",
                (unsigned)f->number_, f->full_name, f->msgdef->full_name);
  }

  const upb_MiniTable_Extension* ext = ctx->file->ext_layouts[f->layout_index];
  if (ctx->layout) {
    UPB_ASSERT(upb_FieldDef_Number(f) == ext->field.number);
  } else {
    upb_MiniTable_Extension* mut_ext = (upb_MiniTable_Extension*)ext;
    fill_fieldlayout(&mut_ext->field, f);
    mut_ext->field.presence = 0;
    mut_ext->field.offset = 0;
    mut_ext->field.submsg_index = 0;
    mut_ext->extendee = f->msgdef->layout;
    mut_ext->sub.submsg = f->sub.msgdef->layout;
  }

  CHK_OOM(upb_inttable_insert(&ctx->symtab->exts, (uintptr_t)ext,
                              upb_value_constptr(f), ctx->arena));
}

static void resolve_default(
    symtab_addctx* ctx, upb_FieldDef* f,
    const google_protobuf_FieldDescriptorProto* field_proto) {
  // Have to delay resolving of the default value until now because of the enum
  // case, since enum defaults are specified with a label.
  if (google_protobuf_FieldDescriptorProto_has_default_value(field_proto)) {
    upb_StringView defaultval =
        google_protobuf_FieldDescriptorProto_default_value(field_proto);

    if (f->file->syntax == kUpb_Syntax_Proto3) {
      symtab_errf(ctx, "proto3 fields cannot have explicit defaults (%s)",
                  f->full_name);
    }

    if (upb_FieldDef_IsSubMessage(f)) {
      symtab_errf(ctx, "message fields cannot have explicit defaults (%s)",
                  f->full_name);
    }

    parse_default(ctx, defaultval.data, defaultval.size, f);
    f->has_default = true;
  } else {
    set_default_default(ctx, f);
    f->has_default = false;
  }
}

static void resolve_fielddef(symtab_addctx* ctx, const char* prefix,
                             upb_FieldDef* f) {
  // We have to stash this away since resolve_subdef() may overwrite it.
  const google_protobuf_FieldDescriptorProto* field_proto = f->sub.unresolved;

  resolve_subdef(ctx, prefix, f);
  resolve_default(ctx, f, field_proto);

  if (f->is_extension_) {
    resolve_extension(ctx, prefix, f, field_proto);
  }
}

static void resolve_msgdef(symtab_addctx* ctx, upb_MessageDef* m) {
  for (int i = 0; i < m->field_count; i++) {
    resolve_fielddef(ctx, m->full_name, (upb_FieldDef*)&m->fields[i]);
  }

  m->in_message_set = false;
  for (int i = 0; i < m->nested_ext_count; i++) {
    upb_FieldDef* ext = (upb_FieldDef*)&m->nested_exts[i];
    resolve_fielddef(ctx, m->full_name, ext);
    if (ext->type_ == kUpb_FieldType_Message &&
        ext->label_ == kUpb_Label_Optional && ext->sub.msgdef == m &&
        google_protobuf_MessageOptions_message_set_wire_format(
            ext->msgdef->opts)) {
      m->in_message_set = true;
    }
  }

  if (!ctx->layout) make_layout(ctx, m);

  for (int i = 0; i < m->nested_msg_count; i++) {
    resolve_msgdef(ctx, (upb_MessageDef*)&m->nested_msgs[i]);
  }
}

static int count_exts_in_msg(const google_protobuf_DescriptorProto* msg_proto) {
  size_t n;
  google_protobuf_DescriptorProto_extension(msg_proto, &n);
  int ext_count = n;

  const google_protobuf_DescriptorProto* const* nested_msgs =
      google_protobuf_DescriptorProto_nested_type(msg_proto, &n);
  for (size_t i = 0; i < n; i++) {
    ext_count += count_exts_in_msg(nested_msgs[i]);
  }

  return ext_count;
}

static void build_filedef(
    symtab_addctx* ctx, upb_FileDef* file,
    const google_protobuf_FileDescriptorProto* file_proto) {
  const google_protobuf_DescriptorProto* const* msgs;
  const google_protobuf_EnumDescriptorProto* const* enums;
  const google_protobuf_FieldDescriptorProto* const* exts;
  const google_protobuf_ServiceDescriptorProto* const* services;
  const upb_StringView* strs;
  const int32_t* public_deps;
  const int32_t* weak_deps;
  size_t i, n;

  file->symtab = ctx->symtab;

  /* Count all extensions in the file, to build a flat array of layouts. */
  google_protobuf_FileDescriptorProto_extension(file_proto, &n);
  int ext_count = n;
  msgs = google_protobuf_FileDescriptorProto_message_type(file_proto, &n);
  for (int i = 0; i < n; i++) {
    ext_count += count_exts_in_msg(msgs[i]);
  }
  file->ext_count = ext_count;

  if (ctx->layout) {
    /* We are using the ext layouts that were passed in. */
    file->ext_layouts = ctx->layout->exts;
    if (ctx->layout->ext_count != file->ext_count) {
      symtab_errf(ctx, "Extension count did not match layout (%d vs %d)",
                  ctx->layout->ext_count, file->ext_count);
    }
  } else {
    /* We are building ext layouts from scratch. */
    file->ext_layouts =
        symtab_alloc(ctx, sizeof(*file->ext_layouts) * file->ext_count);
    upb_MiniTable_Extension* ext =
        symtab_alloc(ctx, sizeof(*ext) * file->ext_count);
    for (int i = 0; i < file->ext_count; i++) {
      file->ext_layouts[i] = &ext[i];
    }
  }

  if (!google_protobuf_FileDescriptorProto_has_name(file_proto)) {
    symtab_errf(ctx, "File has no name");
  }

  file->name =
      strviewdup(ctx, google_protobuf_FileDescriptorProto_name(file_proto));

  upb_StringView package = google_protobuf_FileDescriptorProto_package(file_proto);
  if (package.size) {
    check_ident(ctx, package, true);
    file->package = strviewdup(ctx, package);
  } else {
    file->package = NULL;
  }

  if (google_protobuf_FileDescriptorProto_has_syntax(file_proto)) {
    upb_StringView syntax =
        google_protobuf_FileDescriptorProto_syntax(file_proto);

    if (streql_view(syntax, "proto2")) {
      file->syntax = kUpb_Syntax_Proto2;
    } else if (streql_view(syntax, "proto3")) {
      file->syntax = kUpb_Syntax_Proto3;
    } else {
      symtab_errf(ctx, "Invalid syntax '" UPB_STRINGVIEW_FORMAT "'",
                  UPB_STRINGVIEW_ARGS(syntax));
    }
  } else {
    file->syntax = kUpb_Syntax_Proto2;
  }

  /* Read options. */
  SET_OPTIONS(file->opts, FileDescriptorProto, FileOptions, file_proto);

  /* Verify dependencies. */
  strs = google_protobuf_FileDescriptorProto_dependency(file_proto, &n);
  file->dep_count = n;
  file->deps = symtab_alloc(ctx, sizeof(*file->deps) * n);

  for (i = 0; i < n; i++) {
    upb_StringView str = strs[i];
    file->deps[i] =
        upb_DefPool_FindFileByNameWithSize(ctx->symtab, str.data, str.size);
    if (!file->deps[i]) {
      symtab_errf(ctx,
                  "Depends on file '" UPB_STRINGVIEW_FORMAT
                  "', but it has not been loaded",
                  UPB_STRINGVIEW_ARGS(str));
    }
  }

  public_deps =
      google_protobuf_FileDescriptorProto_public_dependency(file_proto, &n);
  file->public_dep_count = n;
  file->public_deps = symtab_alloc(ctx, sizeof(*file->public_deps) * n);
  int32_t* mutable_public_deps = (int32_t*)file->public_deps;
  for (i = 0; i < n; i++) {
    if (public_deps[i] >= file->dep_count) {
      symtab_errf(ctx, "public_dep %d is out of range", (int)public_deps[i]);
    }
    mutable_public_deps[i] = public_deps[i];
  }

  weak_deps =
      google_protobuf_FileDescriptorProto_weak_dependency(file_proto, &n);
  file->weak_dep_count = n;
  file->weak_deps = symtab_alloc(ctx, sizeof(*file->weak_deps) * n);
  int32_t* mutable_weak_deps = (int32_t*)file->weak_deps;
  for (i = 0; i < n; i++) {
    if (weak_deps[i] >= file->dep_count) {
      symtab_errf(ctx, "weak_dep %d is out of range", (int)weak_deps[i]);
    }
    mutable_weak_deps[i] = weak_deps[i];
  }

  /* Create enums. */
  enums = google_protobuf_FileDescriptorProto_enum_type(file_proto, &n);
  file->top_lvl_enum_count = n;
  file->top_lvl_enums = symtab_alloc(ctx, sizeof(*file->top_lvl_enums) * n);
  for (i = 0; i < n; i++) {
    create_enumdef(ctx, file->package, enums[i], NULL, &file->top_lvl_enums[i]);
  }

  /* Create extensions. */
  exts = google_protobuf_FileDescriptorProto_extension(file_proto, &n);
  file->top_lvl_ext_count = n;
  file->top_lvl_exts = symtab_alloc(ctx, sizeof(*file->top_lvl_exts) * n);
  for (i = 0; i < n; i++) {
    create_fielddef(ctx, file->package, NULL, exts[i], &file->top_lvl_exts[i],
                    /* is_extension= */ true);
    ((upb_FieldDef*)&file->top_lvl_exts[i])->index_ = i;
  }

  /* Create messages. */
  msgs = google_protobuf_FileDescriptorProto_message_type(file_proto, &n);
  file->top_lvl_msg_count = n;
  file->top_lvl_msgs = symtab_alloc(ctx, sizeof(*file->top_lvl_msgs) * n);
  for (i = 0; i < n; i++) {
    create_msgdef(ctx, file->package, msgs[i], NULL, &file->top_lvl_msgs[i]);
  }

  /* Create services. */
  services = google_protobuf_FileDescriptorProto_service(file_proto, &n);
  file->service_count = n;
  file->services = symtab_alloc(ctx, sizeof(*file->services) * n);
  for (i = 0; i < n; i++) {
    create_service(ctx, services[i], &file->services[i]);
    ((upb_ServiceDef*)&file->services[i])->index = i;
  }

  /* Now that all names are in the table, build layouts and resolve refs. */
  for (i = 0; i < (size_t)file->top_lvl_ext_count; i++) {
    resolve_fielddef(ctx, file->package, (upb_FieldDef*)&file->top_lvl_exts[i]);
  }

  for (i = 0; i < (size_t)file->top_lvl_msg_count; i++) {
    resolve_msgdef(ctx, (upb_MessageDef*)&file->top_lvl_msgs[i]);
  }

  if (file->ext_count) {
    CHK_OOM(_upb_extreg_add(ctx->symtab->extreg, file->ext_layouts,
                            file->ext_count));
  }
}

static void remove_filedef(upb_DefPool* s, upb_FileDef* file) {
  intptr_t iter = UPB_INTTABLE_BEGIN;
  upb_StringView key;
  upb_value val;
  while (upb_strtable_next2(&s->syms, &key, &val, &iter)) {
    const upb_FileDef* f;
    switch (deftype(val)) {
      case UPB_DEFTYPE_EXT:
        f = upb_FieldDef_File(unpack_def(val, UPB_DEFTYPE_EXT));
        break;
      case UPB_DEFTYPE_MSG:
        f = upb_MessageDef_File(unpack_def(val, UPB_DEFTYPE_MSG));
        break;
      case UPB_DEFTYPE_ENUM:
        f = upb_EnumDef_File(unpack_def(val, UPB_DEFTYPE_ENUM));
        break;
      case UPB_DEFTYPE_ENUMVAL:
        f = upb_EnumDef_File(
            upb_EnumValueDef_Enum(unpack_def(val, UPB_DEFTYPE_ENUMVAL)));
        break;
      case UPB_DEFTYPE_SERVICE:
        f = upb_ServiceDef_File(unpack_def(val, UPB_DEFTYPE_SERVICE));
        break;
      default:
        UPB_UNREACHABLE();
    }

    if (f == file) upb_strtable_removeiter(&s->syms, &iter);
  }
}

static const upb_FileDef* _upb_DefPool_AddFile(
    upb_DefPool* s, const google_protobuf_FileDescriptorProto* file_proto,
    const upb_MiniTable_File* layout, upb_Status* status) {
  symtab_addctx ctx;
  upb_StringView name = google_protobuf_FileDescriptorProto_name(file_proto);
  upb_value v;

  if (upb_strtable_lookup2(&s->files, name.data, name.size, &v)) {
    if (unpack_def(v, UPB_DEFTYPE_FILE)) {
      upb_Status_SetErrorFormat(status, "duplicate file name (%.*s)",
                                UPB_STRINGVIEW_ARGS(name));
      return NULL;
    }
    const upb_MiniTable_File* registered = unpack_def(v, UPB_DEFTYPE_LAYOUT);
    UPB_ASSERT(registered);
    if (layout && layout != registered) {
      upb_Status_SetErrorFormat(
          status, "tried to build with a different layout (filename=%.*s)",
          UPB_STRINGVIEW_ARGS(name));
      return NULL;
    }
    layout = registered;
  }

  ctx.symtab = s;
  ctx.layout = layout;
  ctx.msg_count = 0;
  ctx.enum_count = 0;
  ctx.ext_count = 0;
  ctx.status = status;
  ctx.file = NULL;
  ctx.arena = upb_Arena_New();
  ctx.tmp_arena = upb_Arena_New();

  if (!ctx.arena || !ctx.tmp_arena) {
    if (ctx.arena) upb_Arena_Free(ctx.arena);
    if (ctx.tmp_arena) upb_Arena_Free(ctx.tmp_arena);
    upb_Status_setoom(status);
    return NULL;
  }

  if (UPB_UNLIKELY(UPB_SETJMP(ctx.err))) {
    UPB_ASSERT(!upb_Status_IsOk(status));
    if (ctx.file) {
      remove_filedef(s, ctx.file);
      ctx.file = NULL;
    }
  } else {
    ctx.file = symtab_alloc(&ctx, sizeof(*ctx.file));
    build_filedef(&ctx, ctx.file, file_proto);
    upb_strtable_insert(&s->files, name.data, name.size,
                        pack_def(ctx.file, UPB_DEFTYPE_FILE), ctx.arena);
    UPB_ASSERT(upb_Status_IsOk(status));
    upb_Arena_Fuse(s->arena, ctx.arena);
  }

  upb_Arena_Free(ctx.arena);
  upb_Arena_Free(ctx.tmp_arena);
  return ctx.file;
}

const upb_FileDef* upb_DefPool_AddFile(
    upb_DefPool* s, const google_protobuf_FileDescriptorProto* file_proto,
    upb_Status* status) {
  return _upb_DefPool_AddFile(s, file_proto, NULL, status);
}

/* Include here since we want most of this file to be stdio-free. */
#include <stdio.h>

bool _upb_DefPool_LoadDefInitEx(upb_DefPool* s, const _upb_DefPool_Init* init,
                                bool rebuild_minitable) {
  /* Since this function should never fail (it would indicate a bug in upb) we
   * print errors to stderr instead of returning error status to the user. */
  _upb_DefPool_Init** deps = init->deps;
  google_protobuf_FileDescriptorProto* file;
  upb_Arena* arena;
  upb_Status status;

  upb_Status_Clear(&status);

  if (upb_DefPool_FindFileByName(s, init->filename)) {
    return true;
  }

  arena = upb_Arena_New();

  for (; *deps; deps++) {
    if (!_upb_DefPool_LoadDefInitEx(s, *deps, rebuild_minitable)) goto err;
  }

  file = google_protobuf_FileDescriptorProto_parse_ex(
      init->descriptor.data, init->descriptor.size, NULL,
      kUpb_DecodeOption_AliasString, arena);
  s->bytes_loaded += init->descriptor.size;

  if (!file) {
    upb_Status_SetErrorFormat(
        &status,
        "Failed to parse compiled-in descriptor for file '%s'. This should "
        "never happen.",
        init->filename);
    goto err;
  }

  const upb_MiniTable_File* mt = rebuild_minitable ? NULL : init->layout;
  if (!_upb_DefPool_AddFile(s, file, mt, &status)) {
    goto err;
  }

  upb_Arena_Free(arena);
  return true;

err:
  fprintf(stderr,
          "Error loading compiled-in descriptor for file '%s' (this should "
          "never happen): %s\n",
          init->filename, upb_Status_ErrorMessage(&status));
  upb_Arena_Free(arena);
  return false;
}

size_t _upb_DefPool_BytesLoaded(const upb_DefPool* s) {
  return s->bytes_loaded;
}

upb_Arena* _upb_DefPool_Arena(const upb_DefPool* s) { return s->arena; }

const upb_FieldDef* _upb_DefPool_FindExtensionByMiniTable(
    const upb_DefPool* s, const upb_MiniTable_Extension* ext) {
  upb_value v;
  bool ok = upb_inttable_lookup(&s->exts, (uintptr_t)ext, &v);
  UPB_ASSERT(ok);
  return upb_value_getconstptr(v);
}

const upb_FieldDef* upb_DefPool_FindExtensionByNumber(const upb_DefPool* s,
                                                      const upb_MessageDef* m,
                                                      int32_t fieldnum) {
  const upb_MiniTable* l = upb_MessageDef_MiniTable(m);
  const upb_MiniTable_Extension* ext = _upb_extreg_get(s->extreg, l, fieldnum);
  return ext ? _upb_DefPool_FindExtensionByMiniTable(s, ext) : NULL;
}

bool _upb_DefPool_registerlayout(upb_DefPool* s, const char* filename,
                                 const upb_MiniTable_File* file) {
  if (upb_DefPool_FindFileByName(s, filename)) return false;
  upb_value v = pack_def(file, UPB_DEFTYPE_LAYOUT);
  return upb_strtable_insert(&s->files, filename, strlen(filename), v,
                             s->arena);
}

const upb_ExtensionRegistry* upb_DefPool_ExtensionRegistry(
    const upb_DefPool* s) {
  return s->extreg;
}

const upb_FieldDef** upb_DefPool_GetAllExtensions(const upb_DefPool* s,
                                                  const upb_MessageDef* m,
                                                  size_t* count) {
  size_t n = 0;
  intptr_t iter = UPB_INTTABLE_BEGIN;
  uintptr_t key;
  upb_value val;
  // This is O(all exts) instead of O(exts for m).  If we need this to be
  // efficient we may need to make extreg into a two-level table, or have a
  // second per-message index.
  while (upb_inttable_next2(&s->exts, &key, &val, &iter)) {
    const upb_FieldDef* f = upb_value_getconstptr(val);
    if (upb_FieldDef_ContainingType(f) == m) n++;
  }
  const upb_FieldDef** exts = malloc(n * sizeof(*exts));
  iter = UPB_INTTABLE_BEGIN;
  size_t i = 0;
  while (upb_inttable_next2(&s->exts, &key, &val, &iter)) {
    const upb_FieldDef* f = upb_value_getconstptr(val);
    if (upb_FieldDef_ContainingType(f) == m) exts[i++] = f;
  }
  *count = n;
  return exts;
}

#undef CHK_OOM

/** upb/reflection.c ************************************************************/

#include <string.h>


static size_t get_field_size(const upb_MiniTable_Field* f) {
  static unsigned char sizes[] = {
      0,                      /* 0 */
      8,                      /* kUpb_FieldType_Double */
      4,                      /* kUpb_FieldType_Float */
      8,                      /* kUpb_FieldType_Int64 */
      8,                      /* kUpb_FieldType_UInt64 */
      4,                      /* kUpb_FieldType_Int32 */
      8,                      /* kUpb_FieldType_Fixed64 */
      4,                      /* kUpb_FieldType_Fixed32 */
      1,                      /* kUpb_FieldType_Bool */
      sizeof(upb_StringView), /* kUpb_FieldType_String */
      sizeof(void*),          /* kUpb_FieldType_Group */
      sizeof(void*),          /* kUpb_FieldType_Message */
      sizeof(upb_StringView), /* kUpb_FieldType_Bytes */
      4,                      /* kUpb_FieldType_UInt32 */
      4,                      /* kUpb_FieldType_Enum */
      4,                      /* kUpb_FieldType_SFixed32 */
      8,                      /* kUpb_FieldType_SFixed64 */
      4,                      /* kUpb_FieldType_SInt32 */
      8,                      /* kUpb_FieldType_SInt64 */
  };
  return upb_IsRepeatedOrMap(f) ? sizeof(void*) : sizes[f->descriptortype];
}

/** upb_Message
 * *******************************************************************/

upb_Message* upb_Message_New(const upb_MessageDef* m, upb_Arena* a) {
  return _upb_Message_New(upb_MessageDef_MiniTable(m), a);
}

static bool in_oneof(const upb_MiniTable_Field* field) {
  return field->presence < 0;
}

static upb_MessageValue _upb_Message_Getraw(const upb_Message* msg,
                                            const upb_FieldDef* f) {
  const upb_MiniTable_Field* field = upb_FieldDef_MiniTable(f);
  const char* mem = UPB_PTR_AT(msg, field->offset, char);
  upb_MessageValue val = {0};
  memcpy(&val, mem, get_field_size(field));
  return val;
}

bool upb_Message_Has(const upb_Message* msg, const upb_FieldDef* f) {
  assert(upb_FieldDef_HasPresence(f));
  if (upb_FieldDef_IsExtension(f)) {
    const upb_MiniTable_Extension* ext = _upb_FieldDef_ExtensionMiniTable(f);
    return _upb_Message_Getext(msg, ext) != NULL;
  } else {
    const upb_MiniTable_Field* field = upb_FieldDef_MiniTable(f);
    if (in_oneof(field)) {
      return _upb_getoneofcase_field(msg, field) == field->number;
    } else if (field->presence > 0) {
      return _upb_hasbit_field(msg, field);
    } else {
      UPB_ASSERT(field->descriptortype == kUpb_FieldType_Message ||
                 field->descriptortype == kUpb_FieldType_Group);
      return _upb_Message_Getraw(msg, f).msg_val != NULL;
    }
  }
}

const upb_FieldDef* upb_Message_WhichOneof(const upb_Message* msg,
                                           const upb_OneofDef* o) {
  const upb_FieldDef* f = upb_OneofDef_Field(o, 0);
  if (upb_OneofDef_IsSynthetic(o)) {
    UPB_ASSERT(upb_OneofDef_FieldCount(o) == 1);
    return upb_Message_Has(msg, f) ? f : NULL;
  } else {
    const upb_MiniTable_Field* field = upb_FieldDef_MiniTable(f);
    uint32_t oneof_case = _upb_getoneofcase_field(msg, field);
    f = oneof_case ? upb_OneofDef_LookupNumber(o, oneof_case) : NULL;
    UPB_ASSERT((f != NULL) == (oneof_case != 0));
    return f;
  }
}

upb_MessageValue upb_Message_Get(const upb_Message* msg,
                                 const upb_FieldDef* f) {
  if (upb_FieldDef_IsExtension(f)) {
    const upb_Message_Extension* ext =
        _upb_Message_Getext(msg, _upb_FieldDef_ExtensionMiniTable(f));
    if (ext) {
      upb_MessageValue val;
      memcpy(&val, &ext->data, sizeof(val));
      return val;
    } else if (upb_FieldDef_IsRepeated(f)) {
      return (upb_MessageValue){.array_val = NULL};
    }
  } else if (!upb_FieldDef_HasPresence(f) || upb_Message_Has(msg, f)) {
    return _upb_Message_Getraw(msg, f);
  }
  return upb_FieldDef_Default(f);
}

upb_MutableMessageValue upb_Message_Mutable(upb_Message* msg,
                                            const upb_FieldDef* f,
                                            upb_Arena* a) {
  UPB_ASSERT(upb_FieldDef_IsSubMessage(f) || upb_FieldDef_IsRepeated(f));
  if (upb_FieldDef_HasPresence(f) && !upb_Message_Has(msg, f)) {
    // We need to skip the upb_Message_Get() call in this case.
    goto make;
  }

  upb_MessageValue val = upb_Message_Get(msg, f);
  if (val.array_val) {
    return (upb_MutableMessageValue){.array = (upb_Array*)val.array_val};
  }

  upb_MutableMessageValue ret;
make:
  if (!a) return (upb_MutableMessageValue){.array = NULL};
  if (upb_FieldDef_IsMap(f)) {
    const upb_MessageDef* entry = upb_FieldDef_MessageSubDef(f);
    const upb_FieldDef* key =
        upb_MessageDef_FindFieldByNumber(entry, kUpb_MapEntry_KeyFieldNumber);
    const upb_FieldDef* value =
        upb_MessageDef_FindFieldByNumber(entry, kUpb_MapEntry_ValueFieldNumber);
    ret.map =
        upb_Map_New(a, upb_FieldDef_CType(key), upb_FieldDef_CType(value));
  } else if (upb_FieldDef_IsRepeated(f)) {
    ret.array = upb_Array_New(a, upb_FieldDef_CType(f));
  } else {
    UPB_ASSERT(upb_FieldDef_IsSubMessage(f));
    ret.msg = upb_Message_New(upb_FieldDef_MessageSubDef(f), a);
  }

  val.array_val = ret.array;
  upb_Message_Set(msg, f, val, a);

  return ret;
}

bool upb_Message_Set(upb_Message* msg, const upb_FieldDef* f,
                     upb_MessageValue val, upb_Arena* a) {
  if (upb_FieldDef_IsExtension(f)) {
    upb_Message_Extension* ext = _upb_Message_GetOrCreateExtension(
        msg, _upb_FieldDef_ExtensionMiniTable(f), a);
    if (!ext) return false;
    memcpy(&ext->data, &val, sizeof(val));
  } else {
    const upb_MiniTable_Field* field = upb_FieldDef_MiniTable(f);
    char* mem = UPB_PTR_AT(msg, field->offset, char);
    memcpy(mem, &val, get_field_size(field));
    if (field->presence > 0) {
      _upb_sethas_field(msg, field);
    } else if (in_oneof(field)) {
      *_upb_oneofcase_field(msg, field) = field->number;
    }
  }
  return true;
}

void upb_Message_ClearField(upb_Message* msg, const upb_FieldDef* f) {
  if (upb_FieldDef_IsExtension(f)) {
    _upb_Message_Clearext(msg, _upb_FieldDef_ExtensionMiniTable(f));
  } else {
    const upb_MiniTable_Field* field = upb_FieldDef_MiniTable(f);
    char* mem = UPB_PTR_AT(msg, field->offset, char);

    if (field->presence > 0) {
      _upb_clearhas_field(msg, field);
    } else if (in_oneof(field)) {
      uint32_t* oneof_case = _upb_oneofcase_field(msg, field);
      if (*oneof_case != field->number) return;
      *oneof_case = 0;
    }

    memset(mem, 0, get_field_size(field));
  }
}

void upb_Message_Clear(upb_Message* msg, const upb_MessageDef* m) {
  _upb_Message_Clear(msg, upb_MessageDef_MiniTable(m));
}

bool upb_Message_Next(const upb_Message* msg, const upb_MessageDef* m,
                      const upb_DefPool* ext_pool, const upb_FieldDef** out_f,
                      upb_MessageValue* out_val, size_t* iter) {
  size_t i = *iter;
  size_t n = upb_MessageDef_FieldCount(m);
  const upb_MessageValue zero = {0};
  UPB_UNUSED(ext_pool);

  /* Iterate over normal fields, returning the first one that is set. */
  while (++i < n) {
    const upb_FieldDef* f = upb_MessageDef_Field(m, i);
    upb_MessageValue val = _upb_Message_Getraw(msg, f);

    /* Skip field if unset or empty. */
    if (upb_FieldDef_HasPresence(f)) {
      if (!upb_Message_Has(msg, f)) continue;
    } else {
      upb_MessageValue test = val;
      if (upb_FieldDef_IsString(f) && !upb_FieldDef_IsRepeated(f)) {
        /* Clear string pointer, only size matters (ptr could be non-NULL). */
        test.str_val.data = NULL;
      }
      /* Continue if NULL or 0. */
      if (memcmp(&test, &zero, sizeof(test)) == 0) continue;

      /* Continue on empty array or map. */
      if (upb_FieldDef_IsMap(f)) {
        if (upb_Map_Size(test.map_val) == 0) continue;
      } else if (upb_FieldDef_IsRepeated(f)) {
        if (upb_Array_Size(test.array_val) == 0) continue;
      }
    }

    *out_val = val;
    *out_f = f;
    *iter = i;
    return true;
  }

  if (ext_pool) {
    /* Return any extensions that are set. */
    size_t count;
    const upb_Message_Extension* ext = _upb_Message_Getexts(msg, &count);
    if (i - n < count) {
      ext += count - 1 - (i - n);
      memcpy(out_val, &ext->data, sizeof(*out_val));
      *out_f = _upb_DefPool_FindExtensionByMiniTable(ext_pool, ext->ext);
      *iter = i;
      return true;
    }
  }

  *iter = i;
  return false;
}

bool _upb_Message_DiscardUnknown(upb_Message* msg, const upb_MessageDef* m,
                                 int depth) {
  size_t iter = kUpb_Message_Begin;
  const upb_FieldDef* f;
  upb_MessageValue val;
  bool ret = true;

  if (--depth == 0) return false;

  _upb_Message_DiscardUnknown_shallow(msg);

  while (upb_Message_Next(msg, m, NULL /*ext_pool*/, &f, &val, &iter)) {
    const upb_MessageDef* subm = upb_FieldDef_MessageSubDef(f);
    if (!subm) continue;
    if (upb_FieldDef_IsMap(f)) {
      const upb_FieldDef* val_f = upb_MessageDef_FindFieldByNumber(subm, 2);
      const upb_MessageDef* val_m = upb_FieldDef_MessageSubDef(val_f);
      upb_Map* map = (upb_Map*)val.map_val;
      size_t iter = kUpb_Map_Begin;

      if (!val_m) continue;

      while (upb_MapIterator_Next(map, &iter)) {
        upb_MessageValue map_val = upb_MapIterator_Value(map, iter);
        if (!_upb_Message_DiscardUnknown((upb_Message*)map_val.msg_val, val_m,
                                         depth)) {
          ret = false;
        }
      }
    } else if (upb_FieldDef_IsRepeated(f)) {
      const upb_Array* arr = val.array_val;
      size_t i, n = upb_Array_Size(arr);
      for (i = 0; i < n; i++) {
        upb_MessageValue elem = upb_Array_Get(arr, i);
        if (!_upb_Message_DiscardUnknown((upb_Message*)elem.msg_val, subm,
                                         depth)) {
          ret = false;
        }
      }
    } else {
      if (!_upb_Message_DiscardUnknown((upb_Message*)val.msg_val, subm,
                                       depth)) {
        ret = false;
      }
    }
  }

  return ret;
}

bool upb_Message_DiscardUnknown(upb_Message* msg, const upb_MessageDef* m,
                                int maxdepth) {
  return _upb_Message_DiscardUnknown(msg, m, maxdepth);
}

/** upb/decode.c ************************************************************/

#include <setjmp.h>
#include <string.h>


/* Must be last. */

/* Maps descriptor type -> elem_size_lg2.  */
static const uint8_t desctype_to_elem_size_lg2[] = {
    -1,             /* invalid descriptor type */
    3,              /* DOUBLE */
    2,              /* FLOAT */
    3,              /* INT64 */
    3,              /* UINT64 */
    2,              /* INT32 */
    3,              /* FIXED64 */
    2,              /* FIXED32 */
    0,              /* BOOL */
    UPB_SIZE(3, 4), /* STRING */
    UPB_SIZE(2, 3), /* GROUP */
    UPB_SIZE(2, 3), /* MESSAGE */
    UPB_SIZE(3, 4), /* BYTES */
    2,              /* UINT32 */
    2,              /* ENUM */
    2,              /* SFIXED32 */
    3,              /* SFIXED64 */
    2,              /* SINT32 */
    3,              /* SINT64 */
};

/* Maps descriptor type -> upb map size.  */
static const uint8_t desctype_to_mapsize[] = {
    -1,                 /* invalid descriptor type */
    8,                  /* DOUBLE */
    4,                  /* FLOAT */
    8,                  /* INT64 */
    8,                  /* UINT64 */
    4,                  /* INT32 */
    8,                  /* FIXED64 */
    4,                  /* FIXED32 */
    1,                  /* BOOL */
    UPB_MAPTYPE_STRING, /* STRING */
    sizeof(void*),      /* GROUP */
    sizeof(void*),      /* MESSAGE */
    UPB_MAPTYPE_STRING, /* BYTES */
    4,                  /* UINT32 */
    4,                  /* ENUM */
    4,                  /* SFIXED32 */
    8,                  /* SFIXED64 */
    4,                  /* SINT32 */
    8,                  /* SINT64 */
};

static const unsigned FIXED32_OK_MASK = (1 << kUpb_FieldType_Float) |
                                        (1 << kUpb_FieldType_Fixed32) |
                                        (1 << kUpb_FieldType_SFixed32);

static const unsigned FIXED64_OK_MASK = (1 << kUpb_FieldType_Double) |
                                        (1 << kUpb_FieldType_Fixed64) |
                                        (1 << kUpb_FieldType_SFixed64);

/* Three fake field types for MessageSet. */
#define TYPE_MSGSET_ITEM 19
#define TYPE_COUNT 19

/* Op: an action to be performed for a wire-type/field-type combination. */
#define OP_UNKNOWN -1 /* Unknown field. */
#define OP_MSGSET_ITEM -2
#define OP_SCALAR_LG2(n) (n) /* n in [0, 2, 3] => op in [0, 2, 3] */
#define OP_ENUM 1
#define OP_STRING 4
#define OP_BYTES 5
#define OP_SUBMSG 6
/* Scalar fields use only ops above. Repeated fields can use any op.  */
#define OP_FIXPCK_LG2(n) (n + 5) /* n in [2, 3] => op in [7, 8] */
#define OP_VARPCK_LG2(n) (n + 9) /* n in [0, 2, 3] => op in [9, 11, 12] */
#define OP_PACKED_ENUM 13

static const int8_t varint_ops[] = {
    OP_UNKNOWN,       /* field not found */
    OP_UNKNOWN,       /* DOUBLE */
    OP_UNKNOWN,       /* FLOAT */
    OP_SCALAR_LG2(3), /* INT64 */
    OP_SCALAR_LG2(3), /* UINT64 */
    OP_SCALAR_LG2(2), /* INT32 */
    OP_UNKNOWN,       /* FIXED64 */
    OP_UNKNOWN,       /* FIXED32 */
    OP_SCALAR_LG2(0), /* BOOL */
    OP_UNKNOWN,       /* STRING */
    OP_UNKNOWN,       /* GROUP */
    OP_UNKNOWN,       /* MESSAGE */
    OP_UNKNOWN,       /* BYTES */
    OP_SCALAR_LG2(2), /* UINT32 */
    OP_ENUM,          /* ENUM */
    OP_UNKNOWN,       /* SFIXED32 */
    OP_UNKNOWN,       /* SFIXED64 */
    OP_SCALAR_LG2(2), /* SINT32 */
    OP_SCALAR_LG2(3), /* SINT64 */
    OP_UNKNOWN,       /* MSGSET_ITEM */
};

static const int8_t delim_ops[] = {
    /* For non-repeated field type. */
    OP_UNKNOWN, /* field not found */
    OP_UNKNOWN, /* DOUBLE */
    OP_UNKNOWN, /* FLOAT */
    OP_UNKNOWN, /* INT64 */
    OP_UNKNOWN, /* UINT64 */
    OP_UNKNOWN, /* INT32 */
    OP_UNKNOWN, /* FIXED64 */
    OP_UNKNOWN, /* FIXED32 */
    OP_UNKNOWN, /* BOOL */
    OP_STRING,  /* STRING */
    OP_UNKNOWN, /* GROUP */
    OP_SUBMSG,  /* MESSAGE */
    OP_BYTES,   /* BYTES */
    OP_UNKNOWN, /* UINT32 */
    OP_UNKNOWN, /* ENUM */
    OP_UNKNOWN, /* SFIXED32 */
    OP_UNKNOWN, /* SFIXED64 */
    OP_UNKNOWN, /* SINT32 */
    OP_UNKNOWN, /* SINT64 */
    OP_UNKNOWN, /* MSGSET_ITEM */
    /* For repeated field type. */
    OP_FIXPCK_LG2(3), /* REPEATED DOUBLE */
    OP_FIXPCK_LG2(2), /* REPEATED FLOAT */
    OP_VARPCK_LG2(3), /* REPEATED INT64 */
    OP_VARPCK_LG2(3), /* REPEATED UINT64 */
    OP_VARPCK_LG2(2), /* REPEATED INT32 */
    OP_FIXPCK_LG2(3), /* REPEATED FIXED64 */
    OP_FIXPCK_LG2(2), /* REPEATED FIXED32 */
    OP_VARPCK_LG2(0), /* REPEATED BOOL */
    OP_STRING,        /* REPEATED STRING */
    OP_SUBMSG,        /* REPEATED GROUP */
    OP_SUBMSG,        /* REPEATED MESSAGE */
    OP_BYTES,         /* REPEATED BYTES */
    OP_VARPCK_LG2(2), /* REPEATED UINT32 */
    OP_PACKED_ENUM,   /* REPEATED ENUM */
    OP_FIXPCK_LG2(2), /* REPEATED SFIXED32 */
    OP_FIXPCK_LG2(3), /* REPEATED SFIXED64 */
    OP_VARPCK_LG2(2), /* REPEATED SINT32 */
    OP_VARPCK_LG2(3), /* REPEATED SINT64 */
    /* Omitting MSGSET_*, because we never emit a repeated msgset type */
};

typedef union {
  bool bool_val;
  uint32_t uint32_val;
  uint64_t uint64_val;
  uint32_t size;
} wireval;

static const char* decode_msg(upb_Decoder* d, const char* ptr, upb_Message* msg,
                              const upb_MiniTable* layout);

UPB_NORETURN static void* decode_err(upb_Decoder* d, upb_DecodeStatus status) {
  assert(status != kUpb_DecodeStatus_Ok);
  UPB_LONGJMP(d->err, status);
}

const char* fastdecode_err(upb_Decoder* d, int status) {
  assert(status != kUpb_DecodeStatus_Ok);
  UPB_LONGJMP(d->err, status);
  return NULL;
}
static void decode_verifyutf8(upb_Decoder* d, const char* buf, int len) {
  if (!decode_verifyutf8_inl(buf, len))
    decode_err(d, kUpb_DecodeStatus_BadUtf8);
}

static bool decode_reserve(upb_Decoder* d, upb_Array* arr, size_t elem) {
  bool need_realloc = arr->size - arr->len < elem;
  if (need_realloc && !_upb_array_realloc(arr, arr->len + elem, &d->arena)) {
    decode_err(d, kUpb_DecodeStatus_OutOfMemory);
  }
  return need_realloc;
}

typedef struct {
  const char* ptr;
  uint64_t val;
} decode_vret;

UPB_NOINLINE
static decode_vret decode_longvarint64(const char* ptr, uint64_t val) {
  decode_vret ret = {NULL, 0};
  uint64_t byte;
  int i;
  for (i = 1; i < 10; i++) {
    byte = (uint8_t)ptr[i];
    val += (byte - 1) << (i * 7);
    if (!(byte & 0x80)) {
      ret.ptr = ptr + i + 1;
      ret.val = val;
      return ret;
    }
  }
  return ret;
}

UPB_FORCEINLINE
static const char* decode_varint64(upb_Decoder* d, const char* ptr,
                                   uint64_t* val) {
  uint64_t byte = (uint8_t)*ptr;
  if (UPB_LIKELY((byte & 0x80) == 0)) {
    *val = byte;
    return ptr + 1;
  } else {
    decode_vret res = decode_longvarint64(ptr, byte);
    if (!res.ptr) return decode_err(d, kUpb_DecodeStatus_Malformed);
    *val = res.val;
    return res.ptr;
  }
}

UPB_FORCEINLINE
static const char* decode_tag(upb_Decoder* d, const char* ptr, uint32_t* val) {
  uint64_t byte = (uint8_t)*ptr;
  if (UPB_LIKELY((byte & 0x80) == 0)) {
    *val = byte;
    return ptr + 1;
  } else {
    const char* start = ptr;
    decode_vret res = decode_longvarint64(ptr, byte);
    if (!res.ptr || res.ptr - start > 5 || res.val > UINT32_MAX) {
      return decode_err(d, kUpb_DecodeStatus_Malformed);
    }
    *val = res.val;
    return res.ptr;
  }
}

UPB_FORCEINLINE
static const char* upb_Decoder_DecodeSize(upb_Decoder* d, const char* ptr,
                                          uint32_t* size) {
  uint64_t size64;
  ptr = decode_varint64(d, ptr, &size64);
  if (size64 >= INT32_MAX || ptr - d->end + (int)size64 > d->limit) {
    decode_err(d, kUpb_DecodeStatus_Malformed);
  }
  *size = size64;
  return ptr;
}

static void decode_munge_int32(wireval* val) {
  if (!_upb_IsLittleEndian()) {
    /* The next stage will memcpy(dst, &val, 4) */
    val->uint32_val = val->uint64_val;
  }
}

static void decode_munge(int type, wireval* val) {
  switch (type) {
    case kUpb_FieldType_Bool:
      val->bool_val = val->uint64_val != 0;
      break;
    case kUpb_FieldType_SInt32: {
      uint32_t n = val->uint64_val;
      val->uint32_val = (n >> 1) ^ -(int32_t)(n & 1);
      break;
    }
    case kUpb_FieldType_SInt64: {
      uint64_t n = val->uint64_val;
      val->uint64_val = (n >> 1) ^ -(int64_t)(n & 1);
      break;
    }
    case kUpb_FieldType_Int32:
    case kUpb_FieldType_UInt32:
    case kUpb_FieldType_Enum:
      decode_munge_int32(val);
      break;
  }
}

static upb_Message* decode_newsubmsg(upb_Decoder* d,
                                     const upb_MiniTable_Sub* subs,
                                     const upb_MiniTable_Field* field) {
  const upb_MiniTable* subl = subs[field->submsg_index].submsg;
  upb_Message* msg = _upb_Message_New_inl(subl, &d->arena);
  if (!msg) decode_err(d, kUpb_DecodeStatus_OutOfMemory);
  return msg;
}

UPB_NOINLINE
const char* decode_isdonefallback(upb_Decoder* d, const char* ptr,
                                  int overrun) {
  int status;
  ptr = decode_isdonefallback_inl(d, ptr, overrun, &status);
  if (ptr == NULL) {
    return decode_err(d, status);
  }
  return ptr;
}

static const char* decode_readstr(upb_Decoder* d, const char* ptr, int size,
                                  upb_StringView* str) {
  if (d->options & kUpb_DecodeOption_AliasString) {
    str->data = ptr;
  } else {
    char* data = upb_Arena_Malloc(&d->arena, size);
    if (!data) return decode_err(d, kUpb_DecodeStatus_OutOfMemory);
    memcpy(data, ptr, size);
    str->data = data;
  }
  str->size = size;
  return ptr + size;
}

UPB_FORCEINLINE
static const char* decode_tosubmsg2(upb_Decoder* d, const char* ptr,
                                    upb_Message* submsg,
                                    const upb_MiniTable* subl, int size) {
  int saved_delta = decode_pushlimit(d, ptr, size);
  if (--d->depth < 0) return decode_err(d, kUpb_DecodeStatus_MaxDepthExceeded);
  ptr = decode_msg(d, ptr, submsg, subl);
  if (d->end_group != DECODE_NOGROUP)
    return decode_err(d, kUpb_DecodeStatus_Malformed);
  decode_poplimit(d, ptr, saved_delta);
  d->depth++;
  return ptr;
}

UPB_FORCEINLINE
static const char* decode_tosubmsg(upb_Decoder* d, const char* ptr,
                                   upb_Message* submsg,
                                   const upb_MiniTable_Sub* subs,
                                   const upb_MiniTable_Field* field, int size) {
  return decode_tosubmsg2(d, ptr, submsg, subs[field->submsg_index].submsg,
                          size);
}

UPB_FORCEINLINE
static const char* decode_group(upb_Decoder* d, const char* ptr,
                                upb_Message* submsg, const upb_MiniTable* subl,
                                uint32_t number) {
  if (--d->depth < 0) return decode_err(d, kUpb_DecodeStatus_MaxDepthExceeded);
  if (decode_isdone(d, &ptr)) {
    return decode_err(d, kUpb_DecodeStatus_Malformed);
  }
  ptr = decode_msg(d, ptr, submsg, subl);
  if (d->end_group != number) return decode_err(d, kUpb_DecodeStatus_Malformed);
  d->end_group = DECODE_NOGROUP;
  d->depth++;
  return ptr;
}

UPB_FORCEINLINE
static const char* decode_togroup(upb_Decoder* d, const char* ptr,
                                  upb_Message* submsg,
                                  const upb_MiniTable_Sub* subs,
                                  const upb_MiniTable_Field* field) {
  const upb_MiniTable* subl = subs[field->submsg_index].submsg;
  return decode_group(d, ptr, submsg, subl, field->number);
}

static char* upb_Decoder_EncodeVarint32(uint32_t val, char* ptr) {
  do {
    uint8_t byte = val & 0x7fU;
    val >>= 7;
    if (val) byte |= 0x80U;
    *(ptr++) = byte;
  } while (val);
  return ptr;
}

static void upb_Decode_AddUnknownVarints(upb_Decoder* d, upb_Message* msg,
                                         uint32_t val1, uint32_t val2) {
  char buf[20];
  char* end = buf;
  end = upb_Decoder_EncodeVarint32(val1, end);
  end = upb_Decoder_EncodeVarint32(val2, end);

  if (!_upb_Message_AddUnknown(msg, buf, end - buf, &d->arena)) {
    decode_err(d, kUpb_DecodeStatus_OutOfMemory);
  }
}

UPB_NOINLINE
static bool decode_checkenum_slow(upb_Decoder* d, const char* ptr,
                                  upb_Message* msg, const upb_MiniTable_Enum* e,
                                  const upb_MiniTable_Field* field,
                                  uint32_t v) {
  // OPT: binary search long lists?
  int n = e->value_count;
  for (int i = 0; i < n; i++) {
    if ((uint32_t)e->values[i] == v) return true;
  }

  // Unrecognized enum goes into unknown fields.
  // For packed fields the tag could be arbitrarily far in the past, so we
  // just re-encode the tag and value here.
  uint32_t tag = ((uint32_t)field->number << 3) | kUpb_WireType_Varint;
  upb_Message* unknown_msg =
      field->mode & kUpb_LabelFlags_IsExtension ? d->unknown_msg : msg;
  upb_Decode_AddUnknownVarints(d, unknown_msg, tag, v);
  return false;
}

UPB_FORCEINLINE
static bool decode_checkenum(upb_Decoder* d, const char* ptr, upb_Message* msg,
                             const upb_MiniTable_Enum* e,
                             const upb_MiniTable_Field* field, wireval* val) {
  uint32_t v = val->uint32_val;

  if (UPB_LIKELY(v < 64) && UPB_LIKELY(((1ULL << v) & e->mask))) return true;

  return decode_checkenum_slow(d, ptr, msg, e, field, v);
}

UPB_NOINLINE
static const char* decode_enum_toarray(upb_Decoder* d, const char* ptr,
                                       upb_Message* msg, upb_Array* arr,
                                       const upb_MiniTable_Sub* subs,
                                       const upb_MiniTable_Field* field,
                                       wireval* val) {
  const upb_MiniTable_Enum* e = subs[field->submsg_index].subenum;
  if (!decode_checkenum(d, ptr, msg, e, field, val)) return ptr;
  void* mem = UPB_PTR_AT(_upb_array_ptr(arr), arr->len * 4, void);
  arr->len++;
  memcpy(mem, val, 4);
  return ptr;
}

UPB_FORCEINLINE
static const char* decode_fixed_packed(upb_Decoder* d, const char* ptr,
                                       upb_Array* arr, wireval* val,
                                       const upb_MiniTable_Field* field,
                                       int lg2) {
  int mask = (1 << lg2) - 1;
  size_t count = val->size >> lg2;
  if ((val->size & mask) != 0) {
    // Length isn't a round multiple of elem size.
    return decode_err(d, kUpb_DecodeStatus_Malformed);
  }
  decode_reserve(d, arr, count);
  void* mem = UPB_PTR_AT(_upb_array_ptr(arr), arr->len << lg2, void);
  arr->len += count;
  // Note: if/when the decoder supports multi-buffer input, we will need to
  // handle buffer seams here.
  if (_upb_IsLittleEndian()) {
    memcpy(mem, ptr, val->size);
    ptr += val->size;
  } else {
    const char* end = ptr + val->size;
    char* dst = mem;
    while (ptr < end) {
      if (lg2 == 2) {
        uint32_t val;
        memcpy(&val, ptr, sizeof(val));
        val = _upb_BigEndian_Swap32(val);
        memcpy(dst, &val, sizeof(val));
      } else {
        UPB_ASSERT(lg2 == 3);
        uint64_t val;
        memcpy(&val, ptr, sizeof(val));
        val = _upb_BigEndian_Swap64(val);
        memcpy(dst, &val, sizeof(val));
      }
      ptr += 1 << lg2;
      dst += 1 << lg2;
    }
  }

  return ptr;
}

UPB_FORCEINLINE
static const char* decode_varint_packed(upb_Decoder* d, const char* ptr,
                                        upb_Array* arr, wireval* val,
                                        const upb_MiniTable_Field* field,
                                        int lg2) {
  int scale = 1 << lg2;
  int saved_limit = decode_pushlimit(d, ptr, val->size);
  char* out = UPB_PTR_AT(_upb_array_ptr(arr), arr->len << lg2, void);
  while (!decode_isdone(d, &ptr)) {
    wireval elem;
    ptr = decode_varint64(d, ptr, &elem.uint64_val);
    decode_munge(field->descriptortype, &elem);
    if (decode_reserve(d, arr, 1)) {
      out = UPB_PTR_AT(_upb_array_ptr(arr), arr->len << lg2, void);
    }
    arr->len++;
    memcpy(out, &elem, scale);
    out += scale;
  }
  decode_poplimit(d, ptr, saved_limit);
  return ptr;
}

UPB_NOINLINE
static const char* decode_enum_packed(upb_Decoder* d, const char* ptr,
                                      upb_Message* msg, upb_Array* arr,
                                      const upb_MiniTable_Sub* subs,
                                      const upb_MiniTable_Field* field,
                                      wireval* val) {
  const upb_MiniTable_Enum* e = subs[field->submsg_index].subenum;
  int saved_limit = decode_pushlimit(d, ptr, val->size);
  char* out = UPB_PTR_AT(_upb_array_ptr(arr), arr->len * 4, void);
  while (!decode_isdone(d, &ptr)) {
    wireval elem;
    ptr = decode_varint64(d, ptr, &elem.uint64_val);
    decode_munge_int32(&elem);
    if (!decode_checkenum(d, ptr, msg, e, field, &elem)) {
      continue;
    }
    if (decode_reserve(d, arr, 1)) {
      out = UPB_PTR_AT(_upb_array_ptr(arr), arr->len * 4, void);
    }
    arr->len++;
    memcpy(out, &elem, 4);
    out += 4;
  }
  decode_poplimit(d, ptr, saved_limit);
  return ptr;
}

static const char* decode_toarray(upb_Decoder* d, const char* ptr,
                                  upb_Message* msg,
                                  const upb_MiniTable_Sub* subs,
                                  const upb_MiniTable_Field* field,
                                  wireval* val, int op) {
  upb_Array** arrp = UPB_PTR_AT(msg, field->offset, void);
  upb_Array* arr = *arrp;
  void* mem;

  if (arr) {
    decode_reserve(d, arr, 1);
  } else {
    size_t lg2 = desctype_to_elem_size_lg2[field->descriptortype];
    arr = _upb_Array_New(&d->arena, 4, lg2);
    if (!arr) return decode_err(d, kUpb_DecodeStatus_OutOfMemory);
    *arrp = arr;
  }

  switch (op) {
    case OP_SCALAR_LG2(0):
    case OP_SCALAR_LG2(2):
    case OP_SCALAR_LG2(3):
      /* Append scalar value. */
      mem = UPB_PTR_AT(_upb_array_ptr(arr), arr->len << op, void);
      arr->len++;
      memcpy(mem, val, 1 << op);
      return ptr;
    case OP_STRING:
      decode_verifyutf8(d, ptr, val->size);
      /* Fallthrough. */
    case OP_BYTES: {
      /* Append bytes. */
      upb_StringView* str = (upb_StringView*)_upb_array_ptr(arr) + arr->len;
      arr->len++;
      return decode_readstr(d, ptr, val->size, str);
    }
    case OP_SUBMSG: {
      /* Append submessage / group. */
      upb_Message* submsg = decode_newsubmsg(d, subs, field);
      *UPB_PTR_AT(_upb_array_ptr(arr), arr->len * sizeof(void*), upb_Message*) =
          submsg;
      arr->len++;
      if (UPB_UNLIKELY(field->descriptortype == kUpb_FieldType_Group)) {
        return decode_togroup(d, ptr, submsg, subs, field);
      } else {
        return decode_tosubmsg(d, ptr, submsg, subs, field, val->size);
      }
    }
    case OP_FIXPCK_LG2(2):
    case OP_FIXPCK_LG2(3):
      return decode_fixed_packed(d, ptr, arr, val, field,
                                 op - OP_FIXPCK_LG2(0));
    case OP_VARPCK_LG2(0):
    case OP_VARPCK_LG2(2):
    case OP_VARPCK_LG2(3):
      return decode_varint_packed(d, ptr, arr, val, field,
                                  op - OP_VARPCK_LG2(0));
    case OP_ENUM:
      return decode_enum_toarray(d, ptr, msg, arr, subs, field, val);
    case OP_PACKED_ENUM:
      return decode_enum_packed(d, ptr, msg, arr, subs, field, val);
    default:
      UPB_UNREACHABLE();
  }
}

static const char* decode_tomap(upb_Decoder* d, const char* ptr,
                                upb_Message* msg, const upb_MiniTable_Sub* subs,
                                const upb_MiniTable_Field* field,
                                wireval* val) {
  upb_Map** map_p = UPB_PTR_AT(msg, field->offset, upb_Map*);
  upb_Map* map = *map_p;
  upb_MapEntry ent;
  const upb_MiniTable* entry = subs[field->submsg_index].submsg;

  if (!map) {
    /* Lazily create map. */
    const upb_MiniTable_Field* key_field = &entry->fields[0];
    const upb_MiniTable_Field* val_field = &entry->fields[1];
    char key_size = desctype_to_mapsize[key_field->descriptortype];
    char val_size = desctype_to_mapsize[val_field->descriptortype];
    UPB_ASSERT(key_field->offset == 0);
    UPB_ASSERT(val_field->offset == sizeof(upb_StringView));
    map = _upb_Map_New(&d->arena, key_size, val_size);
    *map_p = map;
  }

  /* Parse map entry. */
  memset(&ent, 0, sizeof(ent));

  if (entry->fields[1].descriptortype == kUpb_FieldType_Message ||
      entry->fields[1].descriptortype == kUpb_FieldType_Group) {
    /* Create proactively to handle the case where it doesn't appear. */
    ent.v.val =
        upb_value_ptr(_upb_Message_New(entry->subs[0].submsg, &d->arena));
  }

  const char* start = ptr;
  ptr = decode_tosubmsg(d, ptr, &ent.k, subs, field, val->size);
  // check if ent had any unknown fields
  size_t size;
  upb_Message_GetUnknown(&ent.k, &size);
  if (size != 0) {
    uint32_t tag = ((uint32_t)field->number << 3) | kUpb_WireType_Delimited;
    upb_Decode_AddUnknownVarints(d, msg, tag, (uint32_t)(ptr - start));
    if (!_upb_Message_AddUnknown(msg, start, ptr - start, &d->arena)) {
      decode_err(d, kUpb_DecodeStatus_OutOfMemory);
    }
  } else {
    if (_upb_Map_Insert(map, &ent.k, map->key_size, &ent.v, map->val_size,
                        &d->arena) == _kUpb_MapInsertStatus_OutOfMemory) {
      decode_err(d, kUpb_DecodeStatus_OutOfMemory);
    }
  }
  return ptr;
}

static const char* decode_tomsg(upb_Decoder* d, const char* ptr,
                                upb_Message* msg, const upb_MiniTable_Sub* subs,
                                const upb_MiniTable_Field* field, wireval* val,
                                int op) {
  void* mem = UPB_PTR_AT(msg, field->offset, void);
  int type = field->descriptortype;

  if (UPB_UNLIKELY(op == OP_ENUM) &&
      !decode_checkenum(d, ptr, msg, subs[field->submsg_index].subenum, field,
                        val)) {
    return ptr;
  }

  /* Set presence if necessary. */
  if (field->presence > 0) {
    _upb_sethas_field(msg, field);
  } else if (field->presence < 0) {
    /* Oneof case */
    uint32_t* oneof_case = _upb_oneofcase_field(msg, field);
    if (op == OP_SUBMSG && *oneof_case != field->number) {
      memset(mem, 0, sizeof(void*));
    }
    *oneof_case = field->number;
  }

  /* Store into message. */
  switch (op) {
    case OP_SUBMSG: {
      upb_Message** submsgp = mem;
      upb_Message* submsg = *submsgp;
      if (!submsg) {
        submsg = decode_newsubmsg(d, subs, field);
        *submsgp = submsg;
      }
      if (UPB_UNLIKELY(type == kUpb_FieldType_Group)) {
        ptr = decode_togroup(d, ptr, submsg, subs, field);
      } else {
        ptr = decode_tosubmsg(d, ptr, submsg, subs, field, val->size);
      }
      break;
    }
    case OP_STRING:
      decode_verifyutf8(d, ptr, val->size);
      /* Fallthrough. */
    case OP_BYTES:
      return decode_readstr(d, ptr, val->size, mem);
    case OP_SCALAR_LG2(3):
      memcpy(mem, val, 8);
      break;
    case OP_ENUM:
    case OP_SCALAR_LG2(2):
      memcpy(mem, val, 4);
      break;
    case OP_SCALAR_LG2(0):
      memcpy(mem, val, 1);
      break;
    default:
      UPB_UNREACHABLE();
  }

  return ptr;
}

UPB_NOINLINE
const char* decode_checkrequired(upb_Decoder* d, const char* ptr,
                                 const upb_Message* msg,
                                 const upb_MiniTable* l) {
  assert(l->required_count);
  if (UPB_LIKELY((d->options & kUpb_DecodeOption_CheckRequired) == 0)) {
    return ptr;
  }
  uint64_t msg_head;
  memcpy(&msg_head, msg, 8);
  msg_head = _upb_BigEndian_Swap64(msg_head);
  if (upb_MiniTable_requiredmask(l) & ~msg_head) {
    d->missing_required = true;
  }
  return ptr;
}

UPB_FORCEINLINE
static bool decode_tryfastdispatch(upb_Decoder* d, const char** ptr,
                                   upb_Message* msg,
                                   const upb_MiniTable* layout) {
#if UPB_FASTTABLE
  if (layout && layout->table_mask != (unsigned char)-1) {
    uint16_t tag = fastdecode_loadtag(*ptr);
    intptr_t table = decode_totable(layout);
    *ptr = fastdecode_tagdispatch(d, *ptr, msg, table, 0, tag);
    return true;
  }
#endif
  return false;
}

static const char* upb_Decoder_SkipField(upb_Decoder* d, const char* ptr,
                                         uint32_t tag) {
  int field_number = tag >> 3;
  int wire_type = tag & 7;
  switch (wire_type) {
    case kUpb_WireType_Varint: {
      uint64_t val;
      return decode_varint64(d, ptr, &val);
    }
    case kUpb_WireType_64Bit:
      return ptr + 8;
    case kUpb_WireType_32Bit:
      return ptr + 4;
    case kUpb_WireType_Delimited: {
      uint32_t size;
      ptr = upb_Decoder_DecodeSize(d, ptr, &size);
      return ptr + size;
    }
    case kUpb_WireType_StartGroup:
      return decode_group(d, ptr, NULL, NULL, field_number);
    default:
      decode_err(d, kUpb_DecodeStatus_Malformed);
  }
}

enum {
  kStartItemTag = ((1 << 3) | kUpb_WireType_StartGroup),
  kEndItemTag = ((1 << 3) | kUpb_WireType_EndGroup),
  kTypeIdTag = ((2 << 3) | kUpb_WireType_Varint),
  kMessageTag = ((3 << 3) | kUpb_WireType_Delimited),
};

static void upb_Decoder_AddKnownMessageSetItem(
    upb_Decoder* d, upb_Message* msg, const upb_MiniTable_Extension* item_mt,
    const char* data, uint32_t size) {
  upb_Message_Extension* ext =
      _upb_Message_GetOrCreateExtension(msg, item_mt, &d->arena);
  if (UPB_UNLIKELY(!ext)) decode_err(d, kUpb_DecodeStatus_OutOfMemory);
  upb_Message* submsg = decode_newsubmsg(d, &ext->ext->sub, &ext->ext->field);
  upb_DecodeStatus status = upb_Decode(data, size, submsg, item_mt->sub.submsg,
                                       d->extreg, d->options, &d->arena);
  memcpy(&ext->data, &submsg, sizeof(submsg));
  if (status != kUpb_DecodeStatus_Ok) decode_err(d, status);
}

static void upb_Decoder_AddUnknownMessageSetItem(upb_Decoder* d,
                                                 upb_Message* msg,
                                                 uint32_t type_id,
                                                 const char* message_data,
                                                 uint32_t message_size) {
  char buf[60];
  char* ptr = buf;
  ptr = upb_Decoder_EncodeVarint32(kStartItemTag, ptr);
  ptr = upb_Decoder_EncodeVarint32(kTypeIdTag, ptr);
  ptr = upb_Decoder_EncodeVarint32(type_id, ptr);
  ptr = upb_Decoder_EncodeVarint32(kMessageTag, ptr);
  ptr = upb_Decoder_EncodeVarint32(message_size, ptr);
  char* split = ptr;

  ptr = upb_Decoder_EncodeVarint32(kEndItemTag, ptr);
  char* end = ptr;

  if (!_upb_Message_AddUnknown(msg, buf, split - buf, &d->arena) ||
      !_upb_Message_AddUnknown(msg, message_data, message_size, &d->arena) ||
      !_upb_Message_AddUnknown(msg, split, end - split, &d->arena)) {
    decode_err(d, kUpb_DecodeStatus_OutOfMemory);
  }
}

static void upb_Decoder_AddMessageSetItem(upb_Decoder* d, upb_Message* msg,
                                          const upb_MiniTable* layout,
                                          uint32_t type_id, const char* data,
                                          uint32_t size) {
  const upb_MiniTable_Extension* item_mt =
      _upb_extreg_get(d->extreg, layout, type_id);
  if (item_mt) {
    upb_Decoder_AddKnownMessageSetItem(d, msg, item_mt, data, size);
  } else {
    upb_Decoder_AddUnknownMessageSetItem(d, msg, type_id, data, size);
  }
}

static const char* upb_Decoder_DecodeMessageSetItem(
    upb_Decoder* d, const char* ptr, upb_Message* msg,
    const upb_MiniTable* layout) {
  uint32_t type_id = 0;
  upb_StringView preserved = {NULL, 0};
  typedef enum {
    kUpb_HaveId = 1 << 0,
    kUpb_HavePayload = 1 << 1,
  } StateMask;
  StateMask state_mask = 0;
  while (!decode_isdone(d, &ptr)) {
    uint32_t tag;
    ptr = decode_tag(d, ptr, &tag);
    switch (tag) {
      case kEndItemTag:
        return ptr;
      case kTypeIdTag: {
        uint64_t tmp;
        ptr = decode_varint64(d, ptr, &tmp);
        if (state_mask & kUpb_HaveId) break;  // Ignore dup.
        state_mask |= kUpb_HaveId;
        type_id = tmp;
        if (state_mask & kUpb_HavePayload) {
          upb_Decoder_AddMessageSetItem(d, msg, layout, type_id, preserved.data,
                                        preserved.size);
        }
        break;
      }
      case kMessageTag: {
        uint32_t size;
        ptr = upb_Decoder_DecodeSize(d, ptr, &size);
        const char* data = ptr;
        ptr += size;
        if (state_mask & kUpb_HavePayload) break;  // Ignore dup.
        state_mask |= kUpb_HavePayload;
        if (state_mask & kUpb_HaveId) {
          upb_Decoder_AddMessageSetItem(d, msg, layout, type_id, data, size);
        } else {
          // Out of order, we must preserve the payload.
          preserved.data = data;
          preserved.size = size;
        }
        break;
      }
      default:
        // We do not preserve unexpected fields inside a message set item.
        ptr = upb_Decoder_SkipField(d, ptr, tag);
        break;
    }
  }
  decode_err(d, kUpb_DecodeStatus_Malformed);
}

static const upb_MiniTable_Field* decode_findfield(upb_Decoder* d,
                                                   const upb_MiniTable* l,
                                                   uint32_t field_number,
                                                   int* last_field_index) {
  static upb_MiniTable_Field none = {0, 0, 0, 0, 0, 0};
  if (l == NULL) return &none;

  size_t idx = ((size_t)field_number) - 1;  // 0 wraps to SIZE_MAX
  if (idx < l->dense_below) {
    /* Fastest case: index into dense fields. */
    goto found;
  }

  if (l->dense_below < l->field_count) {
    /* Linear search non-dense fields. Resume scanning from last_field_index
     * since fields are usually in order. */
    int last = *last_field_index;
    for (idx = last; idx < l->field_count; idx++) {
      if (l->fields[idx].number == field_number) {
        goto found;
      }
    }

    for (idx = l->dense_below; idx < last; idx++) {
      if (l->fields[idx].number == field_number) {
        goto found;
      }
    }
  }

  if (d->extreg) {
    switch (l->ext) {
      case kUpb_ExtMode_Extendable: {
        const upb_MiniTable_Extension* ext =
            _upb_extreg_get(d->extreg, l, field_number);
        if (ext) return &ext->field;
        break;
      }
      case kUpb_ExtMode_IsMessageSet:
        if (field_number == _UPB_MSGSET_ITEM) {
          static upb_MiniTable_Field item = {0, 0, 0, 0, TYPE_MSGSET_ITEM, 0};
          return &item;
        }
        break;
    }
  }

  return &none; /* Unknown field. */

found:
  UPB_ASSERT(l->fields[idx].number == field_number);
  *last_field_index = idx;
  return &l->fields[idx];
}

UPB_FORCEINLINE
static const char* decode_wireval(upb_Decoder* d, const char* ptr,
                                  const upb_MiniTable_Field* field,
                                  int wire_type, wireval* val, int* op) {
  switch (wire_type) {
    case kUpb_WireType_Varint:
      ptr = decode_varint64(d, ptr, &val->uint64_val);
      *op = varint_ops[field->descriptortype];
      decode_munge(field->descriptortype, val);
      return ptr;
    case kUpb_WireType_32Bit:
      memcpy(&val->uint32_val, ptr, 4);
      val->uint32_val = _upb_BigEndian_Swap32(val->uint32_val);
      *op = OP_SCALAR_LG2(2);
      if (((1 << field->descriptortype) & FIXED32_OK_MASK) == 0) {
        *op = OP_UNKNOWN;
      }
      return ptr + 4;
    case kUpb_WireType_64Bit:
      memcpy(&val->uint64_val, ptr, 8);
      val->uint64_val = _upb_BigEndian_Swap64(val->uint64_val);
      *op = OP_SCALAR_LG2(3);
      if (((1 << field->descriptortype) & FIXED64_OK_MASK) == 0) {
        *op = OP_UNKNOWN;
      }
      return ptr + 8;
    case kUpb_WireType_Delimited: {
      int ndx = field->descriptortype;
      if (upb_FieldMode_Get(field) == kUpb_FieldMode_Array) ndx += TYPE_COUNT;
      ptr = upb_Decoder_DecodeSize(d, ptr, &val->size);
      *op = delim_ops[ndx];
      return ptr;
    }
    case kUpb_WireType_StartGroup:
      val->uint32_val = field->number;
      if (field->descriptortype == kUpb_FieldType_Group) {
        *op = OP_SUBMSG;
      } else if (field->descriptortype == TYPE_MSGSET_ITEM) {
        *op = OP_MSGSET_ITEM;
      } else {
        *op = OP_UNKNOWN;
      }
      return ptr;
    default:
      break;
  }
  return decode_err(d, kUpb_DecodeStatus_Malformed);
}

UPB_FORCEINLINE
static const char* decode_known(upb_Decoder* d, const char* ptr,
                                upb_Message* msg, const upb_MiniTable* layout,
                                const upb_MiniTable_Field* field, int op,
                                wireval* val) {
  const upb_MiniTable_Sub* subs = layout->subs;
  uint8_t mode = field->mode;

  if (UPB_UNLIKELY(mode & kUpb_LabelFlags_IsExtension)) {
    const upb_MiniTable_Extension* ext_layout =
        (const upb_MiniTable_Extension*)field;
    upb_Message_Extension* ext =
        _upb_Message_GetOrCreateExtension(msg, ext_layout, &d->arena);
    if (UPB_UNLIKELY(!ext)) return decode_err(d, kUpb_DecodeStatus_OutOfMemory);
    d->unknown_msg = msg;
    msg = &ext->data;
    subs = &ext->ext->sub;
  }

  switch (mode & kUpb_FieldMode_Mask) {
    case kUpb_FieldMode_Array:
      return decode_toarray(d, ptr, msg, subs, field, val, op);
    case kUpb_FieldMode_Map:
      return decode_tomap(d, ptr, msg, subs, field, val);
    case kUpb_FieldMode_Scalar:
      return decode_tomsg(d, ptr, msg, subs, field, val, op);
    default:
      UPB_UNREACHABLE();
  }
}

static const char* decode_reverse_skip_varint(const char* ptr, uint32_t val) {
  uint32_t seen = 0;
  do {
    ptr--;
    seen <<= 7;
    seen |= *ptr & 0x7f;
  } while (seen != val);
  return ptr;
}

static const char* decode_unknown(upb_Decoder* d, const char* ptr,
                                  upb_Message* msg, int field_number,
                                  int wire_type, wireval val) {
  if (field_number == 0) return decode_err(d, kUpb_DecodeStatus_Malformed);

  // Since unknown fields are the uncommon case, we do a little extra work here
  // to walk backwards through the buffer to find the field start.  This frees
  // up a register in the fast paths (when the field is known), which leads to
  // significant speedups in benchmarks.
  const char* start = ptr;

  if (wire_type == kUpb_WireType_Delimited) ptr += val.size;
  if (msg) {
    switch (wire_type) {
      case kUpb_WireType_Varint:
      case kUpb_WireType_Delimited:
        start--;
        while (start[-1] & 0x80) start--;
        break;
      case kUpb_WireType_32Bit:
        start -= 4;
        break;
      case kUpb_WireType_64Bit:
        start -= 8;
        break;
      default:
        break;
    }

    assert(start == d->debug_valstart);
    uint32_t tag = ((uint32_t)field_number << 3) | wire_type;
    start = decode_reverse_skip_varint(start, tag);
    assert(start == d->debug_tagstart);

    if (wire_type == kUpb_WireType_StartGroup) {
      d->unknown = start;
      d->unknown_msg = msg;
      ptr = decode_group(d, ptr, NULL, NULL, field_number);
      start = d->unknown;
      d->unknown = NULL;
    }
    if (!_upb_Message_AddUnknown(msg, start, ptr - start, &d->arena)) {
      return decode_err(d, kUpb_DecodeStatus_OutOfMemory);
    }
  } else if (wire_type == kUpb_WireType_StartGroup) {
    ptr = decode_group(d, ptr, NULL, NULL, field_number);
  }
  return ptr;
}

UPB_NOINLINE
static const char* decode_msg(upb_Decoder* d, const char* ptr, upb_Message* msg,
                              const upb_MiniTable* layout) {
  int last_field_index = 0;

#if UPB_FASTTABLE
  // The first time we want to skip fast dispatch, because we may have just been
  // invoked by the fast parser to handle a case that it bailed on.
  if (!decode_isdone(d, &ptr)) goto nofast;
#endif

  while (!decode_isdone(d, &ptr)) {
    uint32_t tag;
    const upb_MiniTable_Field* field;
    int field_number;
    int wire_type;
    wireval val;
    int op;

    if (decode_tryfastdispatch(d, &ptr, msg, layout)) break;

#if UPB_FASTTABLE
  nofast:
#endif

#ifndef NDEBUG
    d->debug_tagstart = ptr;
#endif

    UPB_ASSERT(ptr < d->limit_ptr);
    ptr = decode_tag(d, ptr, &tag);
    field_number = tag >> 3;
    wire_type = tag & 7;

#ifndef NDEBUG
    d->debug_valstart = ptr;
#endif

    if (wire_type == kUpb_WireType_EndGroup) {
      d->end_group = field_number;
      return ptr;
    }

    field = decode_findfield(d, layout, field_number, &last_field_index);
    ptr = decode_wireval(d, ptr, field, wire_type, &val, &op);

    if (op >= 0) {
      ptr = decode_known(d, ptr, msg, layout, field, op, &val);
    } else {
      switch (op) {
        case OP_UNKNOWN:
          ptr = decode_unknown(d, ptr, msg, field_number, wire_type, val);
          break;
        case OP_MSGSET_ITEM:
          ptr = upb_Decoder_DecodeMessageSetItem(d, ptr, msg, layout);
          break;
      }
    }
  }

  return UPB_UNLIKELY(layout && layout->required_count)
             ? decode_checkrequired(d, ptr, msg, layout)
             : ptr;
}

const char* fastdecode_generic(struct upb_Decoder* d, const char* ptr,
                               upb_Message* msg, intptr_t table,
                               uint64_t hasbits, uint64_t data) {
  (void)data;
  *(uint32_t*)msg |= hasbits;
  return decode_msg(d, ptr, msg, decode_totablep(table));
}

static upb_DecodeStatus decode_top(struct upb_Decoder* d, const char* buf,
                                   void* msg, const upb_MiniTable* l) {
  if (!decode_tryfastdispatch(d, &buf, msg, l)) {
    decode_msg(d, buf, msg, l);
  }
  if (d->end_group != DECODE_NOGROUP) return kUpb_DecodeStatus_Malformed;
  if (d->missing_required) return kUpb_DecodeStatus_MissingRequired;
  return kUpb_DecodeStatus_Ok;
}

upb_DecodeStatus upb_Decode(const char* buf, size_t size, void* msg,
                            const upb_MiniTable* l,
                            const upb_ExtensionRegistry* extreg, int options,
                            upb_Arena* arena) {
  upb_Decoder state;
  unsigned depth = (unsigned)options >> 16;

  if (size <= 16) {
    memset(&state.patch, 0, 32);
    if (size) memcpy(&state.patch, buf, size);
    buf = state.patch;
    state.end = buf + size;
    state.limit = 0;
    options &= ~kUpb_DecodeOption_AliasString;  // Can't alias patch buf.
  } else {
    state.end = buf + size - 16;
    state.limit = 16;
  }

  state.extreg = extreg;
  state.limit_ptr = state.end;
  state.unknown = NULL;
  state.depth = depth ? depth : 64;
  state.end_group = DECODE_NOGROUP;
  state.options = (uint16_t)options;
  state.missing_required = false;
  state.arena.head = arena->head;
  state.arena.last_size = arena->last_size;
  state.arena.cleanup_metadata = arena->cleanup_metadata;
  state.arena.parent = arena;

  upb_DecodeStatus status = UPB_SETJMP(state.err);
  if (UPB_LIKELY(status == kUpb_DecodeStatus_Ok)) {
    status = decode_top(&state, buf, msg, l);
  }

  arena->head.ptr = state.arena.head.ptr;
  arena->head.end = state.arena.head.end;
  arena->cleanup_metadata = state.arena.cleanup_metadata;
  return status;
}

#undef OP_UNKNOWN
#undef OP_SKIP
#undef OP_SCALAR_LG2
#undef OP_FIXPCK_LG2
#undef OP_VARPCK_LG2
#undef OP_STRING
#undef OP_BYTES
#undef OP_SUBMSG

/** upb/encode.c ************************************************************/
/* We encode backwards, to avoid pre-computing lengths (one-pass encode). */


#include <setjmp.h>
#include <string.h>


/* Must be last. */

#define UPB_PB_VARINT_MAX_LEN 10

UPB_NOINLINE
static size_t encode_varint64(uint64_t val, char* buf) {
  size_t i = 0;
  do {
    uint8_t byte = val & 0x7fU;
    val >>= 7;
    if (val) byte |= 0x80U;
    buf[i++] = byte;
  } while (val);
  return i;
}

static uint32_t encode_zz32(int32_t n) {
  return ((uint32_t)n << 1) ^ (n >> 31);
}
static uint64_t encode_zz64(int64_t n) {
  return ((uint64_t)n << 1) ^ (n >> 63);
}

typedef struct {
  jmp_buf err;
  upb_alloc* alloc;
  char *buf, *ptr, *limit;
  int options;
  int depth;
  _upb_mapsorter sorter;
} upb_encstate;

static size_t upb_roundup_pow2(size_t bytes) {
  size_t ret = 128;
  while (ret < bytes) {
    ret *= 2;
  }
  return ret;
}

UPB_NORETURN static void encode_err(upb_encstate* e) { UPB_LONGJMP(e->err, 1); }

UPB_NOINLINE
static void encode_growbuffer(upb_encstate* e, size_t bytes) {
  size_t old_size = e->limit - e->buf;
  size_t new_size = upb_roundup_pow2(bytes + (e->limit - e->ptr));
  char* new_buf = upb_realloc(e->alloc, e->buf, old_size, new_size);

  if (!new_buf) encode_err(e);

  /* We want previous data at the end, realloc() put it at the beginning. */
  if (old_size > 0) {
    memmove(new_buf + new_size - old_size, e->buf, old_size);
  }

  e->ptr = new_buf + new_size - (e->limit - e->ptr);
  e->limit = new_buf + new_size;
  e->buf = new_buf;

  e->ptr -= bytes;
}

/* Call to ensure that at least "bytes" bytes are available for writing at
 * e->ptr.  Returns false if the bytes could not be allocated. */
UPB_FORCEINLINE
static void encode_reserve(upb_encstate* e, size_t bytes) {
  if ((size_t)(e->ptr - e->buf) < bytes) {
    encode_growbuffer(e, bytes);
    return;
  }

  e->ptr -= bytes;
}

/* Writes the given bytes to the buffer, handling reserve/advance. */
static void encode_bytes(upb_encstate* e, const void* data, size_t len) {
  if (len == 0) return; /* memcpy() with zero size is UB */
  encode_reserve(e, len);
  memcpy(e->ptr, data, len);
}

static void encode_fixed64(upb_encstate* e, uint64_t val) {
  val = _upb_BigEndian_Swap64(val);
  encode_bytes(e, &val, sizeof(uint64_t));
}

static void encode_fixed32(upb_encstate* e, uint32_t val) {
  val = _upb_BigEndian_Swap32(val);
  encode_bytes(e, &val, sizeof(uint32_t));
}

UPB_NOINLINE
static void encode_longvarint(upb_encstate* e, uint64_t val) {
  size_t len;
  char* start;

  encode_reserve(e, UPB_PB_VARINT_MAX_LEN);
  len = encode_varint64(val, e->ptr);
  start = e->ptr + UPB_PB_VARINT_MAX_LEN - len;
  memmove(start, e->ptr, len);
  e->ptr = start;
}

UPB_FORCEINLINE
static void encode_varint(upb_encstate* e, uint64_t val) {
  if (val < 128 && e->ptr != e->buf) {
    --e->ptr;
    *e->ptr = val;
  } else {
    encode_longvarint(e, val);
  }
}

static void encode_double(upb_encstate* e, double d) {
  uint64_t u64;
  UPB_ASSERT(sizeof(double) == sizeof(uint64_t));
  memcpy(&u64, &d, sizeof(uint64_t));
  encode_fixed64(e, u64);
}

static void encode_float(upb_encstate* e, float d) {
  uint32_t u32;
  UPB_ASSERT(sizeof(float) == sizeof(uint32_t));
  memcpy(&u32, &d, sizeof(uint32_t));
  encode_fixed32(e, u32);
}

static void encode_tag(upb_encstate* e, uint32_t field_number,
                       uint8_t wire_type) {
  encode_varint(e, (field_number << 3) | wire_type);
}

static void encode_fixedarray(upb_encstate* e, const upb_Array* arr,
                              size_t elem_size, uint32_t tag) {
  size_t bytes = arr->len * elem_size;
  const char* data = _upb_array_constptr(arr);
  const char* ptr = data + bytes - elem_size;

  if (tag || !_upb_IsLittleEndian()) {
    while (true) {
      if (elem_size == 4) {
        uint32_t val;
        memcpy(&val, ptr, sizeof(val));
        val = _upb_BigEndian_Swap32(val);
        encode_bytes(e, &val, elem_size);
      } else {
        UPB_ASSERT(elem_size == 8);
        uint64_t val;
        memcpy(&val, ptr, sizeof(val));
        val = _upb_BigEndian_Swap64(val);
        encode_bytes(e, &val, elem_size);
      }

      if (tag) encode_varint(e, tag);
      if (ptr == data) break;
      ptr -= elem_size;
    }
  } else {
    encode_bytes(e, data, bytes);
  }
}

static void encode_message(upb_encstate* e, const upb_Message* msg,
                           const upb_MiniTable* m, size_t* size);

static void encode_scalar(upb_encstate* e, const void* _field_mem,
                          const upb_MiniTable_Sub* subs,
                          const upb_MiniTable_Field* f) {
  const char* field_mem = _field_mem;
  int wire_type;

#define CASE(ctype, type, wtype, encodeval) \
  {                                         \
    ctype val = *(ctype*)field_mem;         \
    encode_##type(e, encodeval);            \
    wire_type = wtype;                      \
    break;                                  \
  }

  switch (f->descriptortype) {
    case kUpb_FieldType_Double:
      CASE(double, double, kUpb_WireType_64Bit, val);
    case kUpb_FieldType_Float:
      CASE(float, float, kUpb_WireType_32Bit, val);
    case kUpb_FieldType_Int64:
    case kUpb_FieldType_UInt64:
      CASE(uint64_t, varint, kUpb_WireType_Varint, val);
    case kUpb_FieldType_UInt32:
      CASE(uint32_t, varint, kUpb_WireType_Varint, val);
    case kUpb_FieldType_Int32:
    case kUpb_FieldType_Enum:
      CASE(int32_t, varint, kUpb_WireType_Varint, (int64_t)val);
    case kUpb_FieldType_SFixed64:
    case kUpb_FieldType_Fixed64:
      CASE(uint64_t, fixed64, kUpb_WireType_64Bit, val);
    case kUpb_FieldType_Fixed32:
    case kUpb_FieldType_SFixed32:
      CASE(uint32_t, fixed32, kUpb_WireType_32Bit, val);
    case kUpb_FieldType_Bool:
      CASE(bool, varint, kUpb_WireType_Varint, val);
    case kUpb_FieldType_SInt32:
      CASE(int32_t, varint, kUpb_WireType_Varint, encode_zz32(val));
    case kUpb_FieldType_SInt64:
      CASE(int64_t, varint, kUpb_WireType_Varint, encode_zz64(val));
    case kUpb_FieldType_String:
    case kUpb_FieldType_Bytes: {
      upb_StringView view = *(upb_StringView*)field_mem;
      encode_bytes(e, view.data, view.size);
      encode_varint(e, view.size);
      wire_type = kUpb_WireType_Delimited;
      break;
    }
    case kUpb_FieldType_Group: {
      size_t size;
      void* submsg = *(void**)field_mem;
      const upb_MiniTable* subm = subs[f->submsg_index].submsg;
      if (submsg == NULL) {
        return;
      }
      if (--e->depth == 0) encode_err(e);
      encode_tag(e, f->number, kUpb_WireType_EndGroup);
      encode_message(e, submsg, subm, &size);
      wire_type = kUpb_WireType_StartGroup;
      e->depth++;
      break;
    }
    case kUpb_FieldType_Message: {
      size_t size;
      void* submsg = *(void**)field_mem;
      const upb_MiniTable* subm = subs[f->submsg_index].submsg;
      if (submsg == NULL) {
        return;
      }
      if (--e->depth == 0) encode_err(e);
      encode_message(e, submsg, subm, &size);
      encode_varint(e, size);
      wire_type = kUpb_WireType_Delimited;
      e->depth++;
      break;
    }
    default:
      UPB_UNREACHABLE();
  }
#undef CASE

  encode_tag(e, f->number, wire_type);
}

static void encode_array(upb_encstate* e, const upb_Message* msg,
                         const upb_MiniTable_Sub* subs,
                         const upb_MiniTable_Field* f) {
  const upb_Array* arr = *UPB_PTR_AT(msg, f->offset, upb_Array*);
  bool packed = f->mode & kUpb_LabelFlags_IsPacked;
  size_t pre_len = e->limit - e->ptr;

  if (arr == NULL || arr->len == 0) {
    return;
  }

#define VARINT_CASE(ctype, encode)                                       \
  {                                                                      \
    const ctype* start = _upb_array_constptr(arr);                       \
    const ctype* ptr = start + arr->len;                                 \
    uint32_t tag = packed ? 0 : (f->number << 3) | kUpb_WireType_Varint; \
    do {                                                                 \
      ptr--;                                                             \
      encode_varint(e, encode);                                          \
      if (tag) encode_varint(e, tag);                                    \
    } while (ptr != start);                                              \
  }                                                                      \
  break;

#define TAG(wire_type) (packed ? 0 : (f->number << 3 | wire_type))

  switch (f->descriptortype) {
    case kUpb_FieldType_Double:
      encode_fixedarray(e, arr, sizeof(double), TAG(kUpb_WireType_64Bit));
      break;
    case kUpb_FieldType_Float:
      encode_fixedarray(e, arr, sizeof(float), TAG(kUpb_WireType_32Bit));
      break;
    case kUpb_FieldType_SFixed64:
    case kUpb_FieldType_Fixed64:
      encode_fixedarray(e, arr, sizeof(uint64_t), TAG(kUpb_WireType_64Bit));
      break;
    case kUpb_FieldType_Fixed32:
    case kUpb_FieldType_SFixed32:
      encode_fixedarray(e, arr, sizeof(uint32_t), TAG(kUpb_WireType_32Bit));
      break;
    case kUpb_FieldType_Int64:
    case kUpb_FieldType_UInt64:
      VARINT_CASE(uint64_t, *ptr);
    case kUpb_FieldType_UInt32:
      VARINT_CASE(uint32_t, *ptr);
    case kUpb_FieldType_Int32:
    case kUpb_FieldType_Enum:
      VARINT_CASE(int32_t, (int64_t)*ptr);
    case kUpb_FieldType_Bool:
      VARINT_CASE(bool, *ptr);
    case kUpb_FieldType_SInt32:
      VARINT_CASE(int32_t, encode_zz32(*ptr));
    case kUpb_FieldType_SInt64:
      VARINT_CASE(int64_t, encode_zz64(*ptr));
    case kUpb_FieldType_String:
    case kUpb_FieldType_Bytes: {
      const upb_StringView* start = _upb_array_constptr(arr);
      const upb_StringView* ptr = start + arr->len;
      do {
        ptr--;
        encode_bytes(e, ptr->data, ptr->size);
        encode_varint(e, ptr->size);
        encode_tag(e, f->number, kUpb_WireType_Delimited);
      } while (ptr != start);
      return;
    }
    case kUpb_FieldType_Group: {
      const void* const* start = _upb_array_constptr(arr);
      const void* const* ptr = start + arr->len;
      const upb_MiniTable* subm = subs[f->submsg_index].submsg;
      if (--e->depth == 0) encode_err(e);
      do {
        size_t size;
        ptr--;
        encode_tag(e, f->number, kUpb_WireType_EndGroup);
        encode_message(e, *ptr, subm, &size);
        encode_tag(e, f->number, kUpb_WireType_StartGroup);
      } while (ptr != start);
      e->depth++;
      return;
    }
    case kUpb_FieldType_Message: {
      const void* const* start = _upb_array_constptr(arr);
      const void* const* ptr = start + arr->len;
      const upb_MiniTable* subm = subs[f->submsg_index].submsg;
      if (--e->depth == 0) encode_err(e);
      do {
        size_t size;
        ptr--;
        encode_message(e, *ptr, subm, &size);
        encode_varint(e, size);
        encode_tag(e, f->number, kUpb_WireType_Delimited);
      } while (ptr != start);
      e->depth++;
      return;
    }
  }
#undef VARINT_CASE

  if (packed) {
    encode_varint(e, e->limit - e->ptr - pre_len);
    encode_tag(e, f->number, kUpb_WireType_Delimited);
  }
}

static void encode_mapentry(upb_encstate* e, uint32_t number,
                            const upb_MiniTable* layout,
                            const upb_MapEntry* ent) {
  const upb_MiniTable_Field* key_field = &layout->fields[0];
  const upb_MiniTable_Field* val_field = &layout->fields[1];
  size_t pre_len = e->limit - e->ptr;
  size_t size;
  encode_scalar(e, &ent->v, layout->subs, val_field);
  encode_scalar(e, &ent->k, layout->subs, key_field);
  size = (e->limit - e->ptr) - pre_len;
  encode_varint(e, size);
  encode_tag(e, number, kUpb_WireType_Delimited);
}

static void encode_map(upb_encstate* e, const upb_Message* msg,
                       const upb_MiniTable_Sub* subs,
                       const upb_MiniTable_Field* f) {
  const upb_Map* map = *UPB_PTR_AT(msg, f->offset, const upb_Map*);
  const upb_MiniTable* layout = subs[f->submsg_index].submsg;
  UPB_ASSERT(layout->field_count == 2);

  if (map == NULL) return;

  if (e->options & kUpb_Encode_Deterministic) {
    _upb_sortedmap sorted;
    _upb_mapsorter_pushmap(&e->sorter, layout->fields[0].descriptortype, map,
                           &sorted);
    upb_MapEntry ent;
    while (_upb_sortedmap_next(&e->sorter, map, &sorted, &ent)) {
      encode_mapentry(e, f->number, layout, &ent);
    }
    _upb_mapsorter_popmap(&e->sorter, &sorted);
  } else {
    upb_strtable_iter i;
    upb_strtable_begin(&i, &map->table);
    for (; !upb_strtable_done(&i); upb_strtable_next(&i)) {
      upb_StringView key = upb_strtable_iter_key(&i);
      const upb_value val = upb_strtable_iter_value(&i);
      upb_MapEntry ent;
      _upb_map_fromkey(key, &ent.k, map->key_size);
      _upb_map_fromvalue(val, &ent.v, map->val_size);
      encode_mapentry(e, f->number, layout, &ent);
    }
  }
}

static bool encode_shouldencode(upb_encstate* e, const upb_Message* msg,
                                const upb_MiniTable_Sub* subs,
                                const upb_MiniTable_Field* f) {
  if (f->presence == 0) {
    /* Proto3 presence or map/array. */
    const void* mem = UPB_PTR_AT(msg, f->offset, void);
    switch (f->mode >> kUpb_FieldRep_Shift) {
      case kUpb_FieldRep_1Byte: {
        char ch;
        memcpy(&ch, mem, 1);
        return ch != 0;
      }
#if UINTPTR_MAX == 0xffffffff
      case kUpb_FieldRep_Pointer:
#endif
      case kUpb_FieldRep_4Byte: {
        uint32_t u32;
        memcpy(&u32, mem, 4);
        return u32 != 0;
      }
#if UINTPTR_MAX != 0xffffffff
      case kUpb_FieldRep_Pointer:
#endif
      case kUpb_FieldRep_8Byte: {
        uint64_t u64;
        memcpy(&u64, mem, 8);
        return u64 != 0;
      }
      case kUpb_FieldRep_StringView: {
        const upb_StringView* str = (const upb_StringView*)mem;
        return str->size != 0;
      }
      default:
        UPB_UNREACHABLE();
    }
  } else if (f->presence > 0) {
    /* Proto2 presence: hasbit. */
    return _upb_hasbit_field(msg, f);
  } else {
    /* Field is in a oneof. */
    return _upb_getoneofcase_field(msg, f) == f->number;
  }
}

static void encode_field(upb_encstate* e, const upb_Message* msg,
                         const upb_MiniTable_Sub* subs,
                         const upb_MiniTable_Field* field) {
  switch (upb_FieldMode_Get(field)) {
    case kUpb_FieldMode_Array:
      encode_array(e, msg, subs, field);
      break;
    case kUpb_FieldMode_Map:
      encode_map(e, msg, subs, field);
      break;
    case kUpb_FieldMode_Scalar:
      encode_scalar(e, UPB_PTR_AT(msg, field->offset, void), subs, field);
      break;
    default:
      UPB_UNREACHABLE();
  }
}

/* message MessageSet {
 *   repeated group Item = 1 {
 *     required int32 type_id = 2;
 *     required string message = 3;
 *   }
 * } */
static void encode_msgset_item(upb_encstate* e,
                               const upb_Message_Extension* ext) {
  size_t size;
  encode_tag(e, 1, kUpb_WireType_EndGroup);
  encode_message(e, ext->data.ptr, ext->ext->sub.submsg, &size);
  encode_varint(e, size);
  encode_tag(e, 3, kUpb_WireType_Delimited);
  encode_varint(e, ext->ext->field.number);
  encode_tag(e, 2, kUpb_WireType_Varint);
  encode_tag(e, 1, kUpb_WireType_StartGroup);
}

static void encode_message(upb_encstate* e, const upb_Message* msg,
                           const upb_MiniTable* m, size_t* size) {
  size_t pre_len = e->limit - e->ptr;

  if ((e->options & kUpb_Encode_CheckRequired) && m->required_count) {
    uint64_t msg_head;
    memcpy(&msg_head, msg, 8);
    msg_head = _upb_BigEndian_Swap64(msg_head);
    if (upb_MiniTable_requiredmask(m) & ~msg_head) {
      encode_err(e);
    }
  }

  if ((e->options & kUpb_Encode_SkipUnknown) == 0) {
    size_t unknown_size;
    const char* unknown = upb_Message_GetUnknown(msg, &unknown_size);

    if (unknown) {
      encode_bytes(e, unknown, unknown_size);
    }
  }

  if (m->ext != kUpb_ExtMode_NonExtendable) {
    /* Encode all extensions together. Unlike C++, we do not attempt to keep
     * these in field number order relative to normal fields or even to each
     * other. */
    size_t ext_count;
    const upb_Message_Extension* ext = _upb_Message_Getexts(msg, &ext_count);
    if (ext_count) {
      const upb_Message_Extension* end = ext + ext_count;
      for (; ext != end; ext++) {
        if (UPB_UNLIKELY(m->ext == kUpb_ExtMode_IsMessageSet)) {
          encode_msgset_item(e, ext);
        } else {
          encode_field(e, &ext->data, &ext->ext->sub, &ext->ext->field);
        }
      }
    }
  }

  if (m->field_count) {
    const upb_MiniTable_Field* f = &m->fields[m->field_count];
    const upb_MiniTable_Field* first = &m->fields[0];
    while (f != first) {
      f--;
      if (encode_shouldencode(e, msg, m->subs, f)) {
        encode_field(e, msg, m->subs, f);
      }
    }
  }

  *size = (e->limit - e->ptr) - pre_len;
}

char* upb_Encode(const void* msg, const upb_MiniTable* l, int options,
                 upb_Arena* arena, size_t* size) {
  upb_encstate e;
  unsigned depth = (unsigned)options >> 16;

  e.alloc = upb_Arena_Alloc(arena);
  e.buf = NULL;
  e.limit = NULL;
  e.ptr = NULL;
  e.depth = depth ? depth : 64;
  e.options = options;
  _upb_mapsorter_init(&e.sorter);
  char* ret = NULL;

  if (UPB_SETJMP(e.err)) {
    *size = 0;
    ret = NULL;
  } else {
    encode_message(&e, msg, l, size);
    *size = e.limit - e.ptr;
    if (*size == 0) {
      static char ch;
      ret = &ch;
    } else {
      UPB_ASSERT(e.ptr);
      ret = e.ptr;
    }
  }

  _upb_mapsorter_destroy(&e.sorter);
  return ret;
}

/** upb/msg.c ************************************************************/


/** upb_Message ***************************************************************/

static const size_t overhead = sizeof(upb_Message_InternalData);

static const upb_Message_Internal* upb_Message_Getinternal_const(
    const upb_Message* msg) {
  ptrdiff_t size = sizeof(upb_Message_Internal);
  return (upb_Message_Internal*)((char*)msg - size);
}

upb_Message* _upb_Message_New(const upb_MiniTable* l, upb_Arena* a) {
  return _upb_Message_New_inl(l, a);
}

void _upb_Message_Clear(upb_Message* msg, const upb_MiniTable* l) {
  void* mem = UPB_PTR_AT(msg, -sizeof(upb_Message_Internal), char);
  memset(mem, 0, upb_msg_sizeof(l));
}

static bool realloc_internal(upb_Message* msg, size_t need, upb_Arena* arena) {
  upb_Message_Internal* in = upb_Message_Getinternal(msg);
  if (!in->internal) {
    /* No internal data, allocate from scratch. */
    size_t size = UPB_MAX(128, _upb_Log2CeilingSize(need + overhead));
    upb_Message_InternalData* internal = upb_Arena_Malloc(arena, size);
    if (!internal) return false;
    internal->size = size;
    internal->unknown_end = overhead;
    internal->ext_begin = size;
    in->internal = internal;
  } else if (in->internal->ext_begin - in->internal->unknown_end < need) {
    /* Internal data is too small, reallocate. */
    size_t new_size = _upb_Log2CeilingSize(in->internal->size + need);
    size_t ext_bytes = in->internal->size - in->internal->ext_begin;
    size_t new_ext_begin = new_size - ext_bytes;
    upb_Message_InternalData* internal =
        upb_Arena_Realloc(arena, in->internal, in->internal->size, new_size);
    if (!internal) return false;
    if (ext_bytes) {
      /* Need to move extension data to the end. */
      char* ptr = (char*)internal;
      memmove(ptr + new_ext_begin, ptr + internal->ext_begin, ext_bytes);
    }
    internal->ext_begin = new_ext_begin;
    internal->size = new_size;
    in->internal = internal;
  }
  UPB_ASSERT(in->internal->ext_begin - in->internal->unknown_end >= need);
  return true;
}

bool _upb_Message_AddUnknown(upb_Message* msg, const char* data, size_t len,
                             upb_Arena* arena) {
  if (!realloc_internal(msg, len, arena)) return false;
  upb_Message_Internal* in = upb_Message_Getinternal(msg);
  memcpy(UPB_PTR_AT(in->internal, in->internal->unknown_end, char), data, len);
  in->internal->unknown_end += len;
  return true;
}

void _upb_Message_DiscardUnknown_shallow(upb_Message* msg) {
  upb_Message_Internal* in = upb_Message_Getinternal(msg);
  if (in->internal) {
    in->internal->unknown_end = overhead;
  }
}

const char* upb_Message_GetUnknown(const upb_Message* msg, size_t* len) {
  const upb_Message_Internal* in = upb_Message_Getinternal_const(msg);
  if (in->internal) {
    *len = in->internal->unknown_end - overhead;
    return (char*)(in->internal + 1);
  } else {
    *len = 0;
    return NULL;
  }
}

const upb_Message_Extension* _upb_Message_Getexts(const upb_Message* msg,
                                                  size_t* count) {
  const upb_Message_Internal* in = upb_Message_Getinternal_const(msg);
  if (in->internal) {
    *count = (in->internal->size - in->internal->ext_begin) /
             sizeof(upb_Message_Extension);
    return UPB_PTR_AT(in->internal, in->internal->ext_begin, void);
  } else {
    *count = 0;
    return NULL;
  }
}

const upb_Message_Extension* _upb_Message_Getext(
    const upb_Message* msg, const upb_MiniTable_Extension* e) {
  size_t n;
  const upb_Message_Extension* ext = _upb_Message_Getexts(msg, &n);

  /* For now we use linear search exclusively to find extensions. If this
   * becomes an issue due to messages with lots of extensions, we can introduce
   * a table of some sort. */
  for (size_t i = 0; i < n; i++) {
    if (ext[i].ext == e) {
      return &ext[i];
    }
  }

  return NULL;
}

void _upb_Message_Clearext(upb_Message* msg,
                           const upb_MiniTable_Extension* ext_l) {
  upb_Message_Internal* in = upb_Message_Getinternal(msg);
  if (!in->internal) return;
  const upb_Message_Extension* base =
      UPB_PTR_AT(in->internal, in->internal->ext_begin, void);
  upb_Message_Extension* ext =
      (upb_Message_Extension*)_upb_Message_Getext(msg, ext_l);
  if (ext) {
    *ext = *base;
    in->internal->ext_begin += sizeof(upb_Message_Extension);
  }
}

upb_Message_Extension* _upb_Message_GetOrCreateExtension(
    upb_Message* msg, const upb_MiniTable_Extension* e, upb_Arena* arena) {
  upb_Message_Extension* ext =
      (upb_Message_Extension*)_upb_Message_Getext(msg, e);
  if (ext) return ext;
  if (!realloc_internal(msg, sizeof(upb_Message_Extension), arena)) return NULL;
  upb_Message_Internal* in = upb_Message_Getinternal(msg);
  in->internal->ext_begin -= sizeof(upb_Message_Extension);
  ext = UPB_PTR_AT(in->internal, in->internal->ext_begin, void);
  memset(ext, 0, sizeof(upb_Message_Extension));
  ext->ext = e;
  return ext;
}

size_t upb_Message_ExtensionCount(const upb_Message* msg) {
  size_t count;
  _upb_Message_Getexts(msg, &count);
  return count;
}

/** upb_Array *****************************************************************/

bool _upb_array_realloc(upb_Array* arr, size_t min_size, upb_Arena* arena) {
  size_t new_size = UPB_MAX(arr->size, 4);
  int elem_size_lg2 = arr->data & 7;
  size_t old_bytes = arr->size << elem_size_lg2;
  size_t new_bytes;
  void* ptr = _upb_array_ptr(arr);

  /* Log2 ceiling of size. */
  while (new_size < min_size) new_size *= 2;

  new_bytes = new_size << elem_size_lg2;
  ptr = upb_Arena_Realloc(arena, ptr, old_bytes, new_bytes);

  if (!ptr) {
    return false;
  }

  arr->data = _upb_tag_arrptr(ptr, elem_size_lg2);
  arr->size = new_size;
  return true;
}

static upb_Array* getorcreate_array(upb_Array** arr_ptr, int elem_size_lg2,
                                    upb_Arena* arena) {
  upb_Array* arr = *arr_ptr;
  if (!arr) {
    arr = _upb_Array_New(arena, 4, elem_size_lg2);
    if (!arr) return NULL;
    *arr_ptr = arr;
  }
  return arr;
}

void* _upb_Array_Resize_fallback(upb_Array** arr_ptr, size_t size,
                                 int elem_size_lg2, upb_Arena* arena) {
  upb_Array* arr = getorcreate_array(arr_ptr, elem_size_lg2, arena);
  return arr && _upb_Array_Resize(arr, size, arena) ? _upb_array_ptr(arr)
                                                    : NULL;
}

bool _upb_Array_Append_fallback(upb_Array** arr_ptr, const void* value,
                                int elem_size_lg2, upb_Arena* arena) {
  upb_Array* arr = getorcreate_array(arr_ptr, elem_size_lg2, arena);
  if (!arr) return false;

  size_t elems = arr->len;

  if (!_upb_Array_Resize(arr, elems + 1, arena)) {
    return false;
  }

  char* data = _upb_array_ptr(arr);
  memcpy(data + (elems << elem_size_lg2), value, 1 << elem_size_lg2);
  return true;
}

/** upb_Map *******************************************************************/

upb_Map* _upb_Map_New(upb_Arena* a, size_t key_size, size_t value_size) {
  upb_Map* map = upb_Arena_Malloc(a, sizeof(upb_Map));

  if (!map) {
    return NULL;
  }

  upb_strtable_init(&map->table, 4, a);
  map->key_size = key_size;
  map->val_size = value_size;

  return map;
}

static void _upb_mapsorter_getkeys(const void* _a, const void* _b, void* a_key,
                                   void* b_key, size_t size) {
  const upb_tabent* const* a = _a;
  const upb_tabent* const* b = _b;
  upb_StringView a_tabkey = upb_tabstrview((*a)->key);
  upb_StringView b_tabkey = upb_tabstrview((*b)->key);
  _upb_map_fromkey(a_tabkey, a_key, size);
  _upb_map_fromkey(b_tabkey, b_key, size);
}

#define UPB_COMPARE_INTEGERS(a, b) ((a) < (b) ? -1 : ((a) == (b) ? 0 : 1))

static int _upb_mapsorter_cmpi64(const void* _a, const void* _b) {
  int64_t a, b;
  _upb_mapsorter_getkeys(_a, _b, &a, &b, 8);
  return UPB_COMPARE_INTEGERS(a, b);
}

static int _upb_mapsorter_cmpu64(const void* _a, const void* _b) {
  uint64_t a, b;
  _upb_mapsorter_getkeys(_a, _b, &a, &b, 8);
  return UPB_COMPARE_INTEGERS(a, b);
}

static int _upb_mapsorter_cmpi32(const void* _a, const void* _b) {
  int32_t a, b;
  _upb_mapsorter_getkeys(_a, _b, &a, &b, 4);
  return UPB_COMPARE_INTEGERS(a, b);
}

static int _upb_mapsorter_cmpu32(const void* _a, const void* _b) {
  uint32_t a, b;
  _upb_mapsorter_getkeys(_a, _b, &a, &b, 4);
  return UPB_COMPARE_INTEGERS(a, b);
}

static int _upb_mapsorter_cmpbool(const void* _a, const void* _b) {
  bool a, b;
  _upb_mapsorter_getkeys(_a, _b, &a, &b, 1);
  return UPB_COMPARE_INTEGERS(a, b);
}

static int _upb_mapsorter_cmpstr(const void* _a, const void* _b) {
  upb_StringView a, b;
  _upb_mapsorter_getkeys(_a, _b, &a, &b, UPB_MAPTYPE_STRING);
  size_t common_size = UPB_MIN(a.size, b.size);
  int cmp = memcmp(a.data, b.data, common_size);
  if (cmp) return -cmp;
  return UPB_COMPARE_INTEGERS(a.size, b.size);
}

#undef UPB_COMPARE_INTEGERS

bool _upb_mapsorter_pushmap(_upb_mapsorter* s, upb_FieldType key_type,
                            const upb_Map* map, _upb_sortedmap* sorted) {
  int map_size = _upb_Map_Size(map);
  sorted->start = s->size;
  sorted->pos = sorted->start;
  sorted->end = sorted->start + map_size;

  /* Grow s->entries if necessary. */
  if (sorted->end > s->cap) {
    s->cap = _upb_Log2CeilingSize(sorted->end);
    s->entries = realloc(s->entries, s->cap * sizeof(*s->entries));
    if (!s->entries) return false;
  }

  s->size = sorted->end;

  /* Copy non-empty entries from the table to s->entries. */
  upb_tabent const** dst = &s->entries[sorted->start];
  const upb_tabent* src = map->table.t.entries;
  const upb_tabent* end = src + upb_table_size(&map->table.t);
  for (; src < end; src++) {
    if (!upb_tabent_isempty(src)) {
      *dst = src;
      dst++;
    }
  }
  UPB_ASSERT(dst == &s->entries[sorted->end]);

  /* Sort entries according to the key type. */

  int (*compar)(const void*, const void*);

  switch (key_type) {
    case kUpb_FieldType_Int64:
    case kUpb_FieldType_SFixed64:
    case kUpb_FieldType_SInt64:
      compar = _upb_mapsorter_cmpi64;
      break;
    case kUpb_FieldType_UInt64:
    case kUpb_FieldType_Fixed64:
      compar = _upb_mapsorter_cmpu64;
      break;
    case kUpb_FieldType_Int32:
    case kUpb_FieldType_SInt32:
    case kUpb_FieldType_SFixed32:
    case kUpb_FieldType_Enum:
      compar = _upb_mapsorter_cmpi32;
      break;
    case kUpb_FieldType_UInt32:
    case kUpb_FieldType_Fixed32:
      compar = _upb_mapsorter_cmpu32;
      break;
    case kUpb_FieldType_Bool:
      compar = _upb_mapsorter_cmpbool;
      break;
    case kUpb_FieldType_String:
    case kUpb_FieldType_Bytes:
      compar = _upb_mapsorter_cmpstr;
      break;
    default:
      UPB_UNREACHABLE();
  }

  qsort(&s->entries[sorted->start], map_size, sizeof(*s->entries), compar);
  return true;
}

/** upb_ExtensionRegistry *****************************************************/

struct upb_ExtensionRegistry {
  upb_Arena* arena;
  upb_strtable exts; /* Key is upb_MiniTable* concatenated with fieldnum. */
};

#define EXTREG_KEY_SIZE (sizeof(upb_MiniTable*) + sizeof(uint32_t))

static void extreg_key(char* buf, const upb_MiniTable* l, uint32_t fieldnum) {
  memcpy(buf, &l, sizeof(l));
  memcpy(buf + sizeof(l), &fieldnum, sizeof(fieldnum));
}

upb_ExtensionRegistry* upb_ExtensionRegistry_New(upb_Arena* arena) {
  upb_ExtensionRegistry* r = upb_Arena_Malloc(arena, sizeof(*r));
  if (!r) return NULL;
  r->arena = arena;
  if (!upb_strtable_init(&r->exts, 8, arena)) return NULL;
  return r;
}

bool _upb_extreg_add(upb_ExtensionRegistry* r,
                     const upb_MiniTable_Extension** e, size_t count) {
  char buf[EXTREG_KEY_SIZE];
  const upb_MiniTable_Extension** start = e;
  const upb_MiniTable_Extension** end = UPB_PTRADD(e, count);
  for (; e < end; e++) {
    const upb_MiniTable_Extension* ext = *e;
    extreg_key(buf, ext->extendee, ext->field.number);
    upb_value v;
    if (upb_strtable_lookup2(&r->exts, buf, EXTREG_KEY_SIZE, &v)) {
      goto failure;
    }
    if (!upb_strtable_insert(&r->exts, buf, EXTREG_KEY_SIZE,
                             upb_value_constptr(ext), r->arena)) {
      goto failure;
    }
  }
  return true;

failure:
  /* Back out the entries previously added. */
  for (end = e, e = start; e < end; e++) {
    const upb_MiniTable_Extension* ext = *e;
    extreg_key(buf, ext->extendee, ext->field.number);
    upb_strtable_remove2(&r->exts, buf, EXTREG_KEY_SIZE, NULL);
  }
  return false;
}

const upb_MiniTable_Extension* _upb_extreg_get(const upb_ExtensionRegistry* r,
                                               const upb_MiniTable* l,
                                               uint32_t num) {
  char buf[EXTREG_KEY_SIZE];
  upb_value v;
  extreg_key(buf, l, num);
  if (upb_strtable_lookup2(&r->exts, buf, EXTREG_KEY_SIZE, &v)) {
    return upb_value_getconstptr(v);
  } else {
    return NULL;
  }
}

/** upb/table.c ************************************************************/
/*
 * upb_table Implementation
 *
 * Implementation is heavily inspired by Lua's ltable.c.
 */

#include <string.h>


/* Must be last. */

#define UPB_MAXARRSIZE 16 /* 64k. */

/* From Chromium. */
#define ARRAY_SIZE(x) \
  ((sizeof(x) / sizeof(0 [x])) / ((size_t)(!(sizeof(x) % sizeof(0 [x])))))

static const double MAX_LOAD = 0.85;

/* The minimum utilization of the array part of a mixed hash/array table.  This
 * is a speed/memory-usage tradeoff (though it's not straightforward because of
 * cache effects).  The lower this is, the more memory we'll use. */
static const double MIN_DENSITY = 0.1;

static bool is_pow2(uint64_t v) { return v == 0 || (v & (v - 1)) == 0; }

static upb_value _upb_value_val(uint64_t val) {
  upb_value ret;
  _upb_value_setval(&ret, val);
  return ret;
}

static int log2ceil(uint64_t v) {
  int ret = 0;
  bool pow2 = is_pow2(v);
  while (v >>= 1) ret++;
  ret = pow2 ? ret : ret + 1; /* Ceiling. */
  return UPB_MIN(UPB_MAXARRSIZE, ret);
}

char* upb_strdup2(const char* s, size_t len, upb_Arena* a) {
  size_t n;
  char* p;

  /* Prevent overflow errors. */
  if (len == SIZE_MAX) return NULL;
  /* Always null-terminate, even if binary data; but don't rely on the input to
   * have a null-terminating byte since it may be a raw binary buffer. */
  n = len + 1;
  p = upb_Arena_Malloc(a, n);
  if (p) {
    memcpy(p, s, len);
    p[len] = 0;
  }
  return p;
}

/* A type to represent the lookup key of either a strtable or an inttable. */
typedef union {
  uintptr_t num;
  struct {
    const char* str;
    size_t len;
  } str;
} lookupkey_t;

static lookupkey_t strkey2(const char* str, size_t len) {
  lookupkey_t k;
  k.str.str = str;
  k.str.len = len;
  return k;
}

static lookupkey_t intkey(uintptr_t key) {
  lookupkey_t k;
  k.num = key;
  return k;
}

typedef uint32_t hashfunc_t(upb_tabkey key);
typedef bool eqlfunc_t(upb_tabkey k1, lookupkey_t k2);

/* Base table (shared code) ***************************************************/

static uint32_t upb_inthash(uintptr_t key) { return (uint32_t)key; }

static const upb_tabent* upb_getentry(const upb_table* t, uint32_t hash) {
  return t->entries + (hash & t->mask);
}

static bool upb_arrhas(upb_tabval key) { return key.val != (uint64_t)-1; }

static bool isfull(upb_table* t) { return t->count == t->max_count; }

static bool init(upb_table* t, uint8_t size_lg2, upb_Arena* a) {
  size_t bytes;

  t->count = 0;
  t->size_lg2 = size_lg2;
  t->mask = upb_table_size(t) ? upb_table_size(t) - 1 : 0;
  t->max_count = upb_table_size(t) * MAX_LOAD;
  bytes = upb_table_size(t) * sizeof(upb_tabent);
  if (bytes > 0) {
    t->entries = upb_Arena_Malloc(a, bytes);
    if (!t->entries) return false;
    memset(t->entries, 0, bytes);
  } else {
    t->entries = NULL;
  }
  return true;
}

static upb_tabent* emptyent(upb_table* t, upb_tabent* e) {
  upb_tabent* begin = t->entries;
  upb_tabent* end = begin + upb_table_size(t);
  for (e = e + 1; e < end; e++) {
    if (upb_tabent_isempty(e)) return e;
  }
  for (e = begin; e < end; e++) {
    if (upb_tabent_isempty(e)) return e;
  }
  UPB_ASSERT(false);
  return NULL;
}

static upb_tabent* getentry_mutable(upb_table* t, uint32_t hash) {
  return (upb_tabent*)upb_getentry(t, hash);
}

static const upb_tabent* findentry(const upb_table* t, lookupkey_t key,
                                   uint32_t hash, eqlfunc_t* eql) {
  const upb_tabent* e;

  if (t->size_lg2 == 0) return NULL;
  e = upb_getentry(t, hash);
  if (upb_tabent_isempty(e)) return NULL;
  while (1) {
    if (eql(e->key, key)) return e;
    if ((e = e->next) == NULL) return NULL;
  }
}

static upb_tabent* findentry_mutable(upb_table* t, lookupkey_t key,
                                     uint32_t hash, eqlfunc_t* eql) {
  return (upb_tabent*)findentry(t, key, hash, eql);
}

static bool lookup(const upb_table* t, lookupkey_t key, upb_value* v,
                   uint32_t hash, eqlfunc_t* eql) {
  const upb_tabent* e = findentry(t, key, hash, eql);
  if (e) {
    if (v) {
      _upb_value_setval(v, e->val.val);
    }
    return true;
  } else {
    return false;
  }
}

/* The given key must not already exist in the table. */
static void insert(upb_table* t, lookupkey_t key, upb_tabkey tabkey,
                   upb_value val, uint32_t hash, hashfunc_t* hashfunc,
                   eqlfunc_t* eql) {
  upb_tabent* mainpos_e;
  upb_tabent* our_e;

  UPB_ASSERT(findentry(t, key, hash, eql) == NULL);

  t->count++;
  mainpos_e = getentry_mutable(t, hash);
  our_e = mainpos_e;

  if (upb_tabent_isempty(mainpos_e)) {
    /* Our main position is empty; use it. */
    our_e->next = NULL;
  } else {
    /* Collision. */
    upb_tabent* new_e = emptyent(t, mainpos_e);
    /* Head of collider's chain. */
    upb_tabent* chain = getentry_mutable(t, hashfunc(mainpos_e->key));
    if (chain == mainpos_e) {
      /* Existing ent is in its main position (it has the same hash as us, and
       * is the head of our chain).  Insert to new ent and append to this chain.
       */
      new_e->next = mainpos_e->next;
      mainpos_e->next = new_e;
      our_e = new_e;
    } else {
      /* Existing ent is not in its main position (it is a node in some other
       * chain).  This implies that no existing ent in the table has our hash.
       * Evict it (updating its chain) and use its ent for head of our chain. */
      *new_e = *mainpos_e; /* copies next. */
      while (chain->next != mainpos_e) {
        chain = (upb_tabent*)chain->next;
        UPB_ASSERT(chain);
      }
      chain->next = new_e;
      our_e = mainpos_e;
      our_e->next = NULL;
    }
  }
  our_e->key = tabkey;
  our_e->val.val = val.val;
  UPB_ASSERT(findentry(t, key, hash, eql) == our_e);
}

static bool rm(upb_table* t, lookupkey_t key, upb_value* val,
               upb_tabkey* removed, uint32_t hash, eqlfunc_t* eql) {
  upb_tabent* chain = getentry_mutable(t, hash);
  if (upb_tabent_isempty(chain)) return false;
  if (eql(chain->key, key)) {
    /* Element to remove is at the head of its chain. */
    t->count--;
    if (val) _upb_value_setval(val, chain->val.val);
    if (removed) *removed = chain->key;
    if (chain->next) {
      upb_tabent* move = (upb_tabent*)chain->next;
      *chain = *move;
      move->key = 0; /* Make the slot empty. */
    } else {
      chain->key = 0; /* Make the slot empty. */
    }
    return true;
  } else {
    /* Element to remove is either in a non-head position or not in the
     * table. */
    while (chain->next && !eql(chain->next->key, key)) {
      chain = (upb_tabent*)chain->next;
    }
    if (chain->next) {
      /* Found element to remove. */
      upb_tabent* rm = (upb_tabent*)chain->next;
      t->count--;
      if (val) _upb_value_setval(val, chain->next->val.val);
      if (removed) *removed = rm->key;
      rm->key = 0; /* Make the slot empty. */
      chain->next = rm->next;
      return true;
    } else {
      /* Element to remove is not in the table. */
      return false;
    }
  }
}

static size_t next(const upb_table* t, size_t i) {
  do {
    if (++i >= upb_table_size(t)) return SIZE_MAX - 1; /* Distinct from -1. */
  } while (upb_tabent_isempty(&t->entries[i]));

  return i;
}

static size_t begin(const upb_table* t) { return next(t, -1); }

/* upb_strtable ***************************************************************/

/* A simple "subclass" of upb_table that only adds a hash function for strings.
 */

static upb_tabkey strcopy(lookupkey_t k2, upb_Arena* a) {
  uint32_t len = (uint32_t)k2.str.len;
  char* str = upb_Arena_Malloc(a, k2.str.len + sizeof(uint32_t) + 1);
  if (str == NULL) return 0;
  memcpy(str, &len, sizeof(uint32_t));
  if (k2.str.len) memcpy(str + sizeof(uint32_t), k2.str.str, k2.str.len);
  str[sizeof(uint32_t) + k2.str.len] = '\0';
  return (uintptr_t)str;
}

/* Adapted from ABSL's wyhash. */

static uint64_t UnalignedLoad64(const void* p) {
  uint64_t val;
  memcpy(&val, p, 8);
  return val;
}

static uint32_t UnalignedLoad32(const void* p) {
  uint32_t val;
  memcpy(&val, p, 4);
  return val;
}

#if defined(_MSC_VER) && defined(_M_X64)
#include <intrin.h>
#endif

/* Computes a * b, returning the low 64 bits of the result and storing the high
 * 64 bits in |*high|. */
static uint64_t upb_umul128(uint64_t v0, uint64_t v1, uint64_t* out_high) {
#ifdef __SIZEOF_INT128__
  __uint128_t p = v0;
  p *= v1;
  *out_high = (uint64_t)(p >> 64);
  return (uint64_t)p;
#elif defined(_MSC_VER) && defined(_M_X64)
  return _umul128(v0, v1, out_high);
#else
  uint64_t a32 = v0 >> 32;
  uint64_t a00 = v0 & 0xffffffff;
  uint64_t b32 = v1 >> 32;
  uint64_t b00 = v1 & 0xffffffff;
  uint64_t high = a32 * b32;
  uint64_t low = a00 * b00;
  uint64_t mid1 = a32 * b00;
  uint64_t mid2 = a00 * b32;
  low += (mid1 << 32) + (mid2 << 32);
  // Omit carry bit, for mixing we do not care about exact numerical precision.
  high += (mid1 >> 32) + (mid2 >> 32);
  *out_high = high;
  return low;
#endif
}

static uint64_t WyhashMix(uint64_t v0, uint64_t v1) {
  uint64_t high;
  uint64_t low = upb_umul128(v0, v1, &high);
  return low ^ high;
}

static uint64_t Wyhash(const void* data, size_t len, uint64_t seed,
                       const uint64_t salt[]) {
  const uint8_t* ptr = (const uint8_t*)data;
  uint64_t starting_length = (uint64_t)len;
  uint64_t current_state = seed ^ salt[0];

  if (len > 64) {
    // If we have more than 64 bytes, we're going to handle chunks of 64
    // bytes at a time. We're going to build up two separate hash states
    // which we will then hash together.
    uint64_t duplicated_state = current_state;

    do {
      uint64_t a = UnalignedLoad64(ptr);
      uint64_t b = UnalignedLoad64(ptr + 8);
      uint64_t c = UnalignedLoad64(ptr + 16);
      uint64_t d = UnalignedLoad64(ptr + 24);
      uint64_t e = UnalignedLoad64(ptr + 32);
      uint64_t f = UnalignedLoad64(ptr + 40);
      uint64_t g = UnalignedLoad64(ptr + 48);
      uint64_t h = UnalignedLoad64(ptr + 56);

      uint64_t cs0 = WyhashMix(a ^ salt[1], b ^ current_state);
      uint64_t cs1 = WyhashMix(c ^ salt[2], d ^ current_state);
      current_state = (cs0 ^ cs1);

      uint64_t ds0 = WyhashMix(e ^ salt[3], f ^ duplicated_state);
      uint64_t ds1 = WyhashMix(g ^ salt[4], h ^ duplicated_state);
      duplicated_state = (ds0 ^ ds1);

      ptr += 64;
      len -= 64;
    } while (len > 64);

    current_state = current_state ^ duplicated_state;
  }

  // We now have a data `ptr` with at most 64 bytes and the current state
  // of the hashing state machine stored in current_state.
  while (len > 16) {
    uint64_t a = UnalignedLoad64(ptr);
    uint64_t b = UnalignedLoad64(ptr + 8);

    current_state = WyhashMix(a ^ salt[1], b ^ current_state);

    ptr += 16;
    len -= 16;
  }

  // We now have a data `ptr` with at most 16 bytes.
  uint64_t a = 0;
  uint64_t b = 0;
  if (len > 8) {
    // When we have at least 9 and at most 16 bytes, set A to the first 64
    // bits of the input and B to the last 64 bits of the input. Yes, they will
    // overlap in the middle if we are working with less than the full 16
    // bytes.
    a = UnalignedLoad64(ptr);
    b = UnalignedLoad64(ptr + len - 8);
  } else if (len > 3) {
    // If we have at least 4 and at most 8 bytes, set A to the first 32
    // bits and B to the last 32 bits.
    a = UnalignedLoad32(ptr);
    b = UnalignedLoad32(ptr + len - 4);
  } else if (len > 0) {
    // If we have at least 1 and at most 3 bytes, read all of the provided
    // bits into A, with some adjustments.
    a = ((ptr[0] << 16) | (ptr[len >> 1] << 8) | ptr[len - 1]);
    b = 0;
  } else {
    a = 0;
    b = 0;
  }

  uint64_t w = WyhashMix(a ^ salt[1], b ^ current_state);
  uint64_t z = salt[1] ^ starting_length;
  return WyhashMix(w, z);
}

const uint64_t kWyhashSalt[5] = {
    0x243F6A8885A308D3ULL, 0x13198A2E03707344ULL, 0xA4093822299F31D0ULL,
    0x082EFA98EC4E6C89ULL, 0x452821E638D01377ULL,
};

uint32_t _upb_Hash(const void* p, size_t n, uint64_t seed) {
  return Wyhash(p, n, seed, kWyhashSalt);
}

static uint32_t _upb_Hash_NoSeed(const char* p, size_t n) {
  return _upb_Hash(p, n, 0);
}

static uint32_t strhash(upb_tabkey key) {
  uint32_t len;
  char* str = upb_tabstr(key, &len);
  return _upb_Hash_NoSeed(str, len);
}

static bool streql(upb_tabkey k1, lookupkey_t k2) {
  uint32_t len;
  char* str = upb_tabstr(k1, &len);
  return len == k2.str.len && (len == 0 || memcmp(str, k2.str.str, len) == 0);
}

bool upb_strtable_init(upb_strtable* t, size_t expected_size, upb_Arena* a) {
  // Multiply by approximate reciprocal of MAX_LOAD (0.85), with pow2
  // denominator.
  size_t need_entries = (expected_size + 1) * 1204 / 1024;
  UPB_ASSERT(need_entries >= expected_size * 0.85);
  int size_lg2 = _upb_Log2Ceiling(need_entries);
  return init(&t->t, size_lg2, a);
}

void upb_strtable_clear(upb_strtable* t) {
  size_t bytes = upb_table_size(&t->t) * sizeof(upb_tabent);
  t->t.count = 0;
  memset((char*)t->t.entries, 0, bytes);
}

bool upb_strtable_resize(upb_strtable* t, size_t size_lg2, upb_Arena* a) {
  upb_strtable new_table;
  upb_strtable_iter i;

  if (!init(&new_table.t, size_lg2, a)) return false;
  upb_strtable_begin(&i, t);
  for (; !upb_strtable_done(&i); upb_strtable_next(&i)) {
    upb_StringView key = upb_strtable_iter_key(&i);
    upb_strtable_insert(&new_table, key.data, key.size,
                        upb_strtable_iter_value(&i), a);
  }
  *t = new_table;
  return true;
}

bool upb_strtable_insert(upb_strtable* t, const char* k, size_t len,
                         upb_value v, upb_Arena* a) {
  lookupkey_t key;
  upb_tabkey tabkey;
  uint32_t hash;

  if (isfull(&t->t)) {
    /* Need to resize.  New table of double the size, add old elements to it. */
    if (!upb_strtable_resize(t, t->t.size_lg2 + 1, a)) {
      return false;
    }
  }

  key = strkey2(k, len);
  tabkey = strcopy(key, a);
  if (tabkey == 0) return false;

  hash = _upb_Hash_NoSeed(key.str.str, key.str.len);
  insert(&t->t, key, tabkey, v, hash, &strhash, &streql);
  return true;
}

bool upb_strtable_lookup2(const upb_strtable* t, const char* key, size_t len,
                          upb_value* v) {
  uint32_t hash = _upb_Hash_NoSeed(key, len);
  return lookup(&t->t, strkey2(key, len), v, hash, &streql);
}

bool upb_strtable_remove2(upb_strtable* t, const char* key, size_t len,
                          upb_value* val) {
  uint32_t hash = _upb_Hash_NoSeed(key, len);
  upb_tabkey tabkey;
  return rm(&t->t, strkey2(key, len), val, &tabkey, hash, &streql);
}

/* Iteration */

void upb_strtable_begin(upb_strtable_iter* i, const upb_strtable* t) {
  i->t = t;
  i->index = begin(&t->t);
}

void upb_strtable_next(upb_strtable_iter* i) {
  i->index = next(&i->t->t, i->index);
}

bool upb_strtable_done(const upb_strtable_iter* i) {
  if (!i->t) return true;
  return i->index >= upb_table_size(&i->t->t) ||
         upb_tabent_isempty(str_tabent(i));
}

upb_StringView upb_strtable_iter_key(const upb_strtable_iter* i) {
  upb_StringView key;
  uint32_t len;
  UPB_ASSERT(!upb_strtable_done(i));
  key.data = upb_tabstr(str_tabent(i)->key, &len);
  key.size = len;
  return key;
}

upb_value upb_strtable_iter_value(const upb_strtable_iter* i) {
  UPB_ASSERT(!upb_strtable_done(i));
  return _upb_value_val(str_tabent(i)->val.val);
}

void upb_strtable_iter_setdone(upb_strtable_iter* i) {
  i->t = NULL;
  i->index = SIZE_MAX;
}

bool upb_strtable_iter_isequal(const upb_strtable_iter* i1,
                               const upb_strtable_iter* i2) {
  if (upb_strtable_done(i1) && upb_strtable_done(i2)) return true;
  return i1->t == i2->t && i1->index == i2->index;
}

/* upb_inttable ***************************************************************/

/* For inttables we use a hybrid structure where small keys are kept in an
 * array and large keys are put in the hash table. */

static uint32_t inthash(upb_tabkey key) { return upb_inthash(key); }

static bool inteql(upb_tabkey k1, lookupkey_t k2) { return k1 == k2.num; }

static upb_tabval* mutable_array(upb_inttable* t) {
  return (upb_tabval*)t->array;
}

static upb_tabval* inttable_val(upb_inttable* t, uintptr_t key) {
  if (key < t->array_size) {
    return upb_arrhas(t->array[key]) ? &(mutable_array(t)[key]) : NULL;
  } else {
    upb_tabent* e =
        findentry_mutable(&t->t, intkey(key), upb_inthash(key), &inteql);
    return e ? &e->val : NULL;
  }
}

static const upb_tabval* inttable_val_const(const upb_inttable* t,
                                            uintptr_t key) {
  return inttable_val((upb_inttable*)t, key);
}

size_t upb_inttable_count(const upb_inttable* t) {
  return t->t.count + t->array_count;
}

static void check(upb_inttable* t) {
  UPB_UNUSED(t);
#if defined(UPB_DEBUG_TABLE) && !defined(NDEBUG)
  {
    /* This check is very expensive (makes inserts/deletes O(N)). */
    size_t count = 0;
    upb_inttable_iter i;
    upb_inttable_begin(&i, t);
    for (; !upb_inttable_done(&i); upb_inttable_next(&i), count++) {
      UPB_ASSERT(upb_inttable_lookup(t, upb_inttable_iter_key(&i), NULL));
    }
    UPB_ASSERT(count == upb_inttable_count(t));
  }
#endif
}

bool upb_inttable_sizedinit(upb_inttable* t, size_t asize, int hsize_lg2,
                            upb_Arena* a) {
  size_t array_bytes;

  if (!init(&t->t, hsize_lg2, a)) return false;
  /* Always make the array part at least 1 long, so that we know key 0
   * won't be in the hash part, which simplifies things. */
  t->array_size = UPB_MAX(1, asize);
  t->array_count = 0;
  array_bytes = t->array_size * sizeof(upb_value);
  t->array = upb_Arena_Malloc(a, array_bytes);
  if (!t->array) {
    return false;
  }
  memset(mutable_array(t), 0xff, array_bytes);
  check(t);
  return true;
}

bool upb_inttable_init(upb_inttable* t, upb_Arena* a) {
  return upb_inttable_sizedinit(t, 0, 4, a);
}

bool upb_inttable_insert(upb_inttable* t, uintptr_t key, upb_value val,
                         upb_Arena* a) {
  upb_tabval tabval;
  tabval.val = val.val;
  UPB_ASSERT(
      upb_arrhas(tabval)); /* This will reject (uint64_t)-1.  Fix this. */

  if (key < t->array_size) {
    UPB_ASSERT(!upb_arrhas(t->array[key]));
    t->array_count++;
    mutable_array(t)[key].val = val.val;
  } else {
    if (isfull(&t->t)) {
      /* Need to resize the hash part, but we re-use the array part. */
      size_t i;
      upb_table new_table;

      if (!init(&new_table, t->t.size_lg2 + 1, a)) {
        return false;
      }

      for (i = begin(&t->t); i < upb_table_size(&t->t); i = next(&t->t, i)) {
        const upb_tabent* e = &t->t.entries[i];
        uint32_t hash;
        upb_value v;

        _upb_value_setval(&v, e->val.val);
        hash = upb_inthash(e->key);
        insert(&new_table, intkey(e->key), e->key, v, hash, &inthash, &inteql);
      }

      UPB_ASSERT(t->t.count == new_table.count);

      t->t = new_table;
    }
    insert(&t->t, intkey(key), key, val, upb_inthash(key), &inthash, &inteql);
  }
  check(t);
  return true;
}

bool upb_inttable_lookup(const upb_inttable* t, uintptr_t key, upb_value* v) {
  const upb_tabval* table_v = inttable_val_const(t, key);
  if (!table_v) return false;
  if (v) _upb_value_setval(v, table_v->val);
  return true;
}

bool upb_inttable_replace(upb_inttable* t, uintptr_t key, upb_value val) {
  upb_tabval* table_v = inttable_val(t, key);
  if (!table_v) return false;
  table_v->val = val.val;
  return true;
}

bool upb_inttable_remove(upb_inttable* t, uintptr_t key, upb_value* val) {
  bool success;
  if (key < t->array_size) {
    if (upb_arrhas(t->array[key])) {
      upb_tabval empty = UPB_TABVALUE_EMPTY_INIT;
      t->array_count--;
      if (val) {
        _upb_value_setval(val, t->array[key].val);
      }
      mutable_array(t)[key] = empty;
      success = true;
    } else {
      success = false;
    }
  } else {
    success = rm(&t->t, intkey(key), val, NULL, upb_inthash(key), &inteql);
  }
  check(t);
  return success;
}

void upb_inttable_compact(upb_inttable* t, upb_Arena* a) {
  /* A power-of-two histogram of the table keys. */
  size_t counts[UPB_MAXARRSIZE + 1] = {0};

  /* The max key in each bucket. */
  uintptr_t max[UPB_MAXARRSIZE + 1] = {0};

  upb_inttable_iter i;
  size_t arr_count;
  int size_lg2;
  upb_inttable new_t;

  upb_inttable_begin(&i, t);
  for (; !upb_inttable_done(&i); upb_inttable_next(&i)) {
    uintptr_t key = upb_inttable_iter_key(&i);
    int bucket = log2ceil(key);
    max[bucket] = UPB_MAX(max[bucket], key);
    counts[bucket]++;
  }

  /* Find the largest power of two that satisfies the MIN_DENSITY
   * definition (while actually having some keys). */
  arr_count = upb_inttable_count(t);

  for (size_lg2 = ARRAY_SIZE(counts) - 1; size_lg2 > 0; size_lg2--) {
    if (counts[size_lg2] == 0) {
      /* We can halve again without losing any entries. */
      continue;
    } else if (arr_count >= (1 << size_lg2) * MIN_DENSITY) {
      break;
    }

    arr_count -= counts[size_lg2];
  }

  UPB_ASSERT(arr_count <= upb_inttable_count(t));

  {
    /* Insert all elements into new, perfectly-sized table. */
    size_t arr_size = max[size_lg2] + 1; /* +1 so arr[max] will fit. */
    size_t hash_count = upb_inttable_count(t) - arr_count;
    size_t hash_size = hash_count ? (hash_count / MAX_LOAD) + 1 : 0;
    int hashsize_lg2 = log2ceil(hash_size);

    upb_inttable_sizedinit(&new_t, arr_size, hashsize_lg2, a);
    upb_inttable_begin(&i, t);
    for (; !upb_inttable_done(&i); upb_inttable_next(&i)) {
      uintptr_t k = upb_inttable_iter_key(&i);
      upb_inttable_insert(&new_t, k, upb_inttable_iter_value(&i), a);
    }
    UPB_ASSERT(new_t.array_size == arr_size);
    UPB_ASSERT(new_t.t.size_lg2 == hashsize_lg2);
  }
  *t = new_t;
}

/* Iteration. */

static const upb_tabent* int_tabent(const upb_inttable_iter* i) {
  UPB_ASSERT(!i->array_part);
  return &i->t->t.entries[i->index];
}

static upb_tabval int_arrent(const upb_inttable_iter* i) {
  UPB_ASSERT(i->array_part);
  return i->t->array[i->index];
}

void upb_inttable_begin(upb_inttable_iter* i, const upb_inttable* t) {
  i->t = t;
  i->index = -1;
  i->array_part = true;
  upb_inttable_next(i);
}

void upb_inttable_next(upb_inttable_iter* iter) {
  const upb_inttable* t = iter->t;
  if (iter->array_part) {
    while (++iter->index < t->array_size) {
      if (upb_arrhas(int_arrent(iter))) {
        return;
      }
    }
    iter->array_part = false;
    iter->index = begin(&t->t);
  } else {
    iter->index = next(&t->t, iter->index);
  }
}

bool upb_inttable_next2(const upb_inttable* t, uintptr_t* key, upb_value* val,
                        intptr_t* iter) {
  intptr_t i = *iter;
  if (i < t->array_size) {
    while (++i < t->array_size) {
      upb_tabval ent = t->array[i];
      if (upb_arrhas(ent)) {
        *key = i;
        *val = _upb_value_val(ent.val);
        *iter = i;
        return true;
      }
    }
  }

  size_t tab_idx = next(&t->t, i == -1 ? -1 : i - t->array_size);
  if (tab_idx < upb_table_size(&t->t)) {
    upb_tabent* ent = &t->t.entries[tab_idx];
    *key = ent->key;
    *val = _upb_value_val(ent->val.val);
    *iter = tab_idx + t->array_size;
    return true;
  }

  return false;
}

void upb_inttable_removeiter(upb_inttable* t, intptr_t* iter) {
  intptr_t i = *iter;
  if (i < t->array_size) {
    t->array_count--;
    mutable_array(t)[i].val = -1;
  } else {
    upb_tabent* ent = &t->t.entries[i - t->array_size];
    upb_tabent* prev = NULL;

    // Linear search, not great.
    upb_tabent* end = &t->t.entries[upb_table_size(&t->t)];
    for (upb_tabent* e = t->t.entries; e != end; e++) {
      if (e->next == ent) {
        prev = e;
        break;
      }
    }

    if (prev) {
      prev->next = ent->next;
    }

    t->t.count--;
    ent->key = 0;
    ent->next = NULL;
  }
}

bool upb_strtable_next2(const upb_strtable* t, upb_StringView* key,
                        upb_value* val, intptr_t* iter) {
  size_t tab_idx = next(&t->t, *iter);
  if (tab_idx < upb_table_size(&t->t)) {
    upb_tabent* ent = &t->t.entries[tab_idx];
    uint32_t len;
    key->data = upb_tabstr(ent->key, &len);
    key->size = len;
    *val = _upb_value_val(ent->val.val);
    *iter = tab_idx;
    return true;
  }

  return false;
}

void upb_strtable_removeiter(upb_strtable* t, intptr_t* iter) {
  intptr_t i = *iter;
  upb_tabent* ent = &t->t.entries[i];
  upb_tabent* prev = NULL;

  // Linear search, not great.
  upb_tabent* end = &t->t.entries[upb_table_size(&t->t)];
  for (upb_tabent* e = t->t.entries; e != end; e++) {
    if (e->next == ent) {
      prev = e;
      break;
    }
  }

  if (prev) {
    prev->next = ent->next;
  }

  t->t.count--;
  ent->key = 0;
  ent->next = NULL;
}

bool upb_inttable_done(const upb_inttable_iter* i) {
  if (!i->t) return true;
  if (i->array_part) {
    return i->index >= i->t->array_size || !upb_arrhas(int_arrent(i));
  } else {
    return i->index >= upb_table_size(&i->t->t) ||
           upb_tabent_isempty(int_tabent(i));
  }
}

uintptr_t upb_inttable_iter_key(const upb_inttable_iter* i) {
  UPB_ASSERT(!upb_inttable_done(i));
  return i->array_part ? i->index : int_tabent(i)->key;
}

upb_value upb_inttable_iter_value(const upb_inttable_iter* i) {
  UPB_ASSERT(!upb_inttable_done(i));
  return _upb_value_val(i->array_part ? i->t->array[i->index].val
                                      : int_tabent(i)->val.val);
}

void upb_inttable_iter_setdone(upb_inttable_iter* i) {
  i->t = NULL;
  i->index = SIZE_MAX;
  i->array_part = false;
}

bool upb_inttable_iter_isequal(const upb_inttable_iter* i1,
                               const upb_inttable_iter* i2) {
  if (upb_inttable_done(i1) && upb_inttable_done(i2)) return true;
  return i1->t == i2->t && i1->index == i2->index &&
         i1->array_part == i2->array_part;
}

/** upb/upb.c ************************************************************/
#include <errno.h>
#include <float.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// Must be last.

/* upb_Status *****************************************************************/

void upb_Status_Clear(upb_Status* status) {
  if (!status) return;
  status->ok = true;
  status->msg[0] = '\0';
}

bool upb_Status_IsOk(const upb_Status* status) { return status->ok; }

const char* upb_Status_ErrorMessage(const upb_Status* status) {
  return status->msg;
}

void upb_Status_SetErrorMessage(upb_Status* status, const char* msg) {
  if (!status) return;
  status->ok = false;
  strncpy(status->msg, msg, _kUpb_Status_MaxMessage - 1);
  status->msg[_kUpb_Status_MaxMessage - 1] = '\0';
}

void upb_Status_SetErrorFormat(upb_Status* status, const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  upb_Status_VSetErrorFormat(status, fmt, args);
  va_end(args);
}

void upb_Status_VSetErrorFormat(upb_Status* status, const char* fmt,
                                va_list args) {
  if (!status) return;
  status->ok = false;
  vsnprintf(status->msg, sizeof(status->msg), fmt, args);
  status->msg[_kUpb_Status_MaxMessage - 1] = '\0';
}

void upb_Status_VAppendErrorFormat(upb_Status* status, const char* fmt,
                                   va_list args) {
  size_t len;
  if (!status) return;
  status->ok = false;
  len = strlen(status->msg);
  vsnprintf(status->msg + len, sizeof(status->msg) - len, fmt, args);
  status->msg[_kUpb_Status_MaxMessage - 1] = '\0';
}

/* upb_alloc ******************************************************************/

static void* upb_global_allocfunc(upb_alloc* alloc, void* ptr, size_t oldsize,
                                  size_t size) {
  UPB_UNUSED(alloc);
  UPB_UNUSED(oldsize);
  if (size == 0) {
    free(ptr);
    return NULL;
  } else {
    return realloc(ptr, size);
  }
}

static uint32_t* upb_cleanup_pointer(uintptr_t cleanup_metadata) {
  return (uint32_t*)(cleanup_metadata & ~0x1);
}

static bool upb_cleanup_has_initial_block(uintptr_t cleanup_metadata) {
  return cleanup_metadata & 0x1;
}

static uintptr_t upb_cleanup_metadata(uint32_t* cleanup,
                                      bool has_initial_block) {
  return (uintptr_t)cleanup | has_initial_block;
}

upb_alloc upb_alloc_global = {&upb_global_allocfunc};

/* upb_Arena ******************************************************************/

struct mem_block {
  struct mem_block* next;
  uint32_t size;
  uint32_t cleanups;
  /* Data follows. */
};

typedef struct cleanup_ent {
  upb_CleanupFunc* cleanup;
  void* ud;
} cleanup_ent;

static const size_t memblock_reserve =
    UPB_ALIGN_UP(sizeof(mem_block), UPB_MALLOC_ALIGN);

static upb_Arena* arena_findroot(upb_Arena* a) {
  /* Path splitting keeps time complexity down, see:
   *   https://en.wikipedia.org/wiki/Disjoint-set_data_structure */
  while (a->parent != a) {
    upb_Arena* next = a->parent;
    a->parent = next->parent;
    a = next;
  }
  return a;
}

static void upb_Arena_addblock(upb_Arena* a, upb_Arena* root, void* ptr,
                               size_t size) {
  mem_block* block = ptr;

  /* The block is for arena |a|, but should appear in the freelist of |root|. */
  block->next = root->freelist;
  block->size = (uint32_t)size;
  block->cleanups = 0;
  root->freelist = block;
  a->last_size = block->size;
  if (!root->freelist_tail) root->freelist_tail = block;

  a->head.ptr = UPB_PTR_AT(block, memblock_reserve, char);
  a->head.end = UPB_PTR_AT(block, size, char);
  a->cleanup_metadata = upb_cleanup_metadata(
      &block->cleanups, upb_cleanup_has_initial_block(a->cleanup_metadata));

  UPB_POISON_MEMORY_REGION(a->head.ptr, a->head.end - a->head.ptr);
}

static bool upb_Arena_Allocblock(upb_Arena* a, size_t size) {
  upb_Arena* root = arena_findroot(a);
  size_t block_size = UPB_MAX(size, a->last_size * 2) + memblock_reserve;
  mem_block* block = upb_malloc(root->block_alloc, block_size);

  if (!block) return false;
  upb_Arena_addblock(a, root, block, block_size);
  return true;
}

void* _upb_Arena_SlowMalloc(upb_Arena* a, size_t size) {
  if (!upb_Arena_Allocblock(a, size)) return NULL; /* Out of memory. */
  UPB_ASSERT(_upb_ArenaHas(a) >= size);
  return upb_Arena_Malloc(a, size);
}

static void* upb_Arena_doalloc(upb_alloc* alloc, void* ptr, size_t oldsize,
                               size_t size) {
  upb_Arena* a = (upb_Arena*)alloc; /* upb_alloc is initial member. */
  return upb_Arena_Realloc(a, ptr, oldsize, size);
}

/* Public Arena API ***********************************************************/

upb_Arena* arena_initslow(void* mem, size_t n, upb_alloc* alloc) {
  const size_t first_block_overhead = sizeof(upb_Arena) + memblock_reserve;
  upb_Arena* a;

  /* We need to malloc the initial block. */
  n = first_block_overhead + 256;
  if (!alloc || !(mem = upb_malloc(alloc, n))) {
    return NULL;
  }

  a = UPB_PTR_AT(mem, n - sizeof(*a), upb_Arena);
  n -= sizeof(*a);

  a->head.alloc.func = &upb_Arena_doalloc;
  a->block_alloc = alloc;
  a->parent = a;
  a->refcount = 1;
  a->freelist = NULL;
  a->freelist_tail = NULL;
  a->cleanup_metadata = upb_cleanup_metadata(NULL, false);

  upb_Arena_addblock(a, a, mem, n);

  return a;
}

upb_Arena* upb_Arena_Init(void* mem, size_t n, upb_alloc* alloc) {
  upb_Arena* a;

  if (n) {
    /* Align initial pointer up so that we return properly-aligned pointers. */
    void* aligned = (void*)UPB_ALIGN_UP((uintptr_t)mem, UPB_MALLOC_ALIGN);
    size_t delta = (uintptr_t)aligned - (uintptr_t)mem;
    n = delta <= n ? n - delta : 0;
    mem = aligned;
  }

  /* Round block size down to alignof(*a) since we will allocate the arena
   * itself at the end. */
  n = UPB_ALIGN_DOWN(n, UPB_ALIGN_OF(upb_Arena));

  if (UPB_UNLIKELY(n < sizeof(upb_Arena))) {
    return arena_initslow(mem, n, alloc);
  }

  a = UPB_PTR_AT(mem, n - sizeof(*a), upb_Arena);

  a->head.alloc.func = &upb_Arena_doalloc;
  a->block_alloc = alloc;
  a->parent = a;
  a->refcount = 1;
  a->last_size = UPB_MAX(128, n);
  a->head.ptr = mem;
  a->head.end = UPB_PTR_AT(mem, n - sizeof(*a), char);
  a->freelist = NULL;
  a->cleanup_metadata = upb_cleanup_metadata(NULL, true);

  return a;
}

static void arena_dofree(upb_Arena* a) {
  mem_block* block = a->freelist;
  UPB_ASSERT(a->parent == a);
  UPB_ASSERT(a->refcount == 0);

  while (block) {
    /* Load first since we are deleting block. */
    mem_block* next = block->next;

    if (block->cleanups > 0) {
      cleanup_ent* end = UPB_PTR_AT(block, block->size, void);
      cleanup_ent* ptr = end - block->cleanups;

      for (; ptr < end; ptr++) {
        ptr->cleanup(ptr->ud);
      }
    }

    upb_free(a->block_alloc, block);
    block = next;
  }
}

void upb_Arena_Free(upb_Arena* a) {
  a = arena_findroot(a);
  if (--a->refcount == 0) arena_dofree(a);
}

bool upb_Arena_AddCleanup(upb_Arena* a, void* ud, upb_CleanupFunc* func) {
  cleanup_ent* ent;
  uint32_t* cleanups = upb_cleanup_pointer(a->cleanup_metadata);

  if (!cleanups || _upb_ArenaHas(a) < sizeof(cleanup_ent)) {
    if (!upb_Arena_Allocblock(a, 128)) return false; /* Out of memory. */
    UPB_ASSERT(_upb_ArenaHas(a) >= sizeof(cleanup_ent));
    cleanups = upb_cleanup_pointer(a->cleanup_metadata);
  }

  a->head.end -= sizeof(cleanup_ent);
  ent = (cleanup_ent*)a->head.end;
  (*cleanups)++;
  UPB_UNPOISON_MEMORY_REGION(ent, sizeof(cleanup_ent));

  ent->cleanup = func;
  ent->ud = ud;

  return true;
}

bool upb_Arena_Fuse(upb_Arena* a1, upb_Arena* a2) {
  upb_Arena* r1 = arena_findroot(a1);
  upb_Arena* r2 = arena_findroot(a2);

  if (r1 == r2) return true; /* Already fused. */

  /* Do not fuse initial blocks since we cannot lifetime extend them. */
  if (upb_cleanup_has_initial_block(r1->cleanup_metadata)) return false;
  if (upb_cleanup_has_initial_block(r2->cleanup_metadata)) return false;

  /* Only allow fuse with a common allocator */
  if (r1->block_alloc != r2->block_alloc) return false;

  /* We want to join the smaller tree to the larger tree.
   * So swap first if they are backwards. */
  if (r1->refcount < r2->refcount) {
    upb_Arena* tmp = r1;
    r1 = r2;
    r2 = tmp;
  }

  /* r1 takes over r2's freelist and refcount. */
  r1->refcount += r2->refcount;
  if (r2->freelist_tail) {
    UPB_ASSERT(r2->freelist_tail->next == NULL);
    r2->freelist_tail->next = r1->freelist;
    r1->freelist = r2->freelist;
  }
  r2->parent = r1;
  return true;
}

/* Miscellaneous utilities ****************************************************/

static void upb_FixLocale(char* p) {
  /* printf() is dependent on locales; sadly there is no easy and portable way
   * to avoid this. This little post-processing step will translate 1,2 -> 1.2
   * since JSON needs the latter. Arguably a hack, but it is simple and the
   * alternatives are far more complicated, platform-dependent, and/or larger
   * in code size. */
  for (; *p; p++) {
    if (*p == ',') *p = '.';
  }
}

void _upb_EncodeRoundTripDouble(double val, char* buf, size_t size) {
  assert(size >= kUpb_RoundTripBufferSize);
  snprintf(buf, size, "%.*g", DBL_DIG, val);
  if (strtod(buf, NULL) != val) {
    snprintf(buf, size, "%.*g", DBL_DIG + 2, val);
    assert(strtod(buf, NULL) == val);
  }
  upb_FixLocale(buf);
}

void _upb_EncodeRoundTripFloat(float val, char* buf, size_t size) {
  assert(size >= kUpb_RoundTripBufferSize);
  snprintf(buf, size, "%.*g", FLT_DIG, val);
  if (strtof(buf, NULL) != val) {
    snprintf(buf, size, "%.*g", FLT_DIG + 3, val);
    assert(strtof(buf, NULL) == val);
  }
  upb_FixLocale(buf);
}

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
#undef UPB_FASTTABLE
#undef UPB_FASTTABLE_INIT
#undef UPB_POISON_MEMORY_REGION
#undef UPB_UNPOISON_MEMORY_REGION
#undef UPB_ASAN
#undef UPB_TREAT_PROTO2_ENUMS_LIKE_PROTO3
