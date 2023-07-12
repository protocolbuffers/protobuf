/* Amalgamated source file */
#include "ruby-upb.h"

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


#include <errno.h>
#include <float.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

// Must be last.

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

#include <string.h>


// Must be last.

const char _upb_Array_CTypeSizeLg2Table[] = {
    [kUpb_CType_Bool] = 0,
    [kUpb_CType_Float] = 2,
    [kUpb_CType_Int32] = 2,
    [kUpb_CType_UInt32] = 2,
    [kUpb_CType_Enum] = 2,
    [kUpb_CType_Message] = UPB_SIZE(2, 3),
    [kUpb_CType_Double] = 3,
    [kUpb_CType_Int64] = 3,
    [kUpb_CType_UInt64] = 3,
    [kUpb_CType_String] = UPB_SIZE(3, 4),
    [kUpb_CType_Bytes] = UPB_SIZE(3, 4),
};

upb_Array* upb_Array_New(upb_Arena* a, upb_CType type) {
  return _upb_Array_New(a, 4, _upb_Array_CTypeSizeLg2(type));
}

const void* upb_Array_DataPtr(const upb_Array* arr) {
  return _upb_array_ptr((upb_Array*)arr);
}

void* upb_Array_MutableDataPtr(upb_Array* arr) { return _upb_array_ptr(arr); }

size_t upb_Array_Size(const upb_Array* arr) { return arr->size; }

upb_MessageValue upb_Array_Get(const upb_Array* arr, size_t i) {
  upb_MessageValue ret;
  const char* data = _upb_array_constptr(arr);
  int lg2 = arr->data & 7;
  UPB_ASSERT(i < arr->size);
  memcpy(&ret, data + (i << lg2), 1 << lg2);
  return ret;
}

void upb_Array_Set(upb_Array* arr, size_t i, upb_MessageValue val) {
  char* data = _upb_array_ptr(arr);
  int lg2 = arr->data & 7;
  UPB_ASSERT(i < arr->size);
  memcpy(data + (i << lg2), &val, 1 << lg2);
}

bool upb_Array_Append(upb_Array* arr, upb_MessageValue val, upb_Arena* arena) {
  UPB_ASSERT(arena);
  if (!upb_Array_Resize(arr, arr->size + 1, arena)) {
    return false;
  }
  upb_Array_Set(arr, arr->size - 1, val);
  return true;
}

void upb_Array_Move(upb_Array* arr, size_t dst_idx, size_t src_idx,
                    size_t count) {
  const int lg2 = arr->data & 7;
  char* data = _upb_array_ptr(arr);
  memmove(&data[dst_idx << lg2], &data[src_idx << lg2], count << lg2);
}

bool upb_Array_Insert(upb_Array* arr, size_t i, size_t count,
                      upb_Arena* arena) {
  UPB_ASSERT(arena);
  UPB_ASSERT(i <= arr->size);
  UPB_ASSERT(count + arr->size >= count);
  const size_t oldsize = arr->size;
  if (!upb_Array_Resize(arr, arr->size + count, arena)) {
    return false;
  }
  upb_Array_Move(arr, i + count, i, oldsize - i);
  return true;
}

/*
 *              i        end      arr->size
 * |------------|XXXXXXXX|--------|
 */
void upb_Array_Delete(upb_Array* arr, size_t i, size_t count) {
  const size_t end = i + count;
  UPB_ASSERT(i <= end);
  UPB_ASSERT(end <= arr->size);
  upb_Array_Move(arr, i, end, arr->size - end);
  arr->size -= count;
}

bool upb_Array_Resize(upb_Array* arr, size_t size, upb_Arena* arena) {
  const size_t oldsize = arr->size;
  if (UPB_UNLIKELY(!_upb_Array_ResizeUninitialized(arr, size, arena))) {
    return false;
  }
  const size_t newsize = arr->size;
  if (newsize > oldsize) {
    const int lg2 = arr->data & 7;
    char* data = _upb_array_ptr(arr);
    memset(data + (oldsize << lg2), 0, (newsize - oldsize) << lg2);
  }
  return true;
}

// EVERYTHING BELOW THIS LINE IS INTERNAL - DO NOT USE /////////////////////////

bool _upb_array_realloc(upb_Array* arr, size_t min_capacity, upb_Arena* arena) {
  size_t new_capacity = UPB_MAX(arr->capacity, 4);
  int elem_size_lg2 = arr->data & 7;
  size_t old_bytes = arr->capacity << elem_size_lg2;
  size_t new_bytes;
  void* ptr = _upb_array_ptr(arr);

  // Log2 ceiling of size.
  while (new_capacity < min_capacity) new_capacity *= 2;

  new_bytes = new_capacity << elem_size_lg2;
  ptr = upb_Arena_Realloc(arena, ptr, old_bytes, new_bytes);
  if (!ptr) return false;

  arr->data = _upb_tag_arrptr(ptr, elem_size_lg2);
  arr->capacity = new_capacity;
  return true;
}


#include <string.h>


// Must be last.

// Strings/bytes are special-cased in maps.
char _upb_Map_CTypeSizeTable[12] = {
    [kUpb_CType_Bool] = 1,
    [kUpb_CType_Float] = 4,
    [kUpb_CType_Int32] = 4,
    [kUpb_CType_UInt32] = 4,
    [kUpb_CType_Enum] = 4,
    [kUpb_CType_Message] = sizeof(void*),
    [kUpb_CType_Double] = 8,
    [kUpb_CType_Int64] = 8,
    [kUpb_CType_UInt64] = 8,
    [kUpb_CType_String] = UPB_MAPTYPE_STRING,
    [kUpb_CType_Bytes] = UPB_MAPTYPE_STRING,
};

upb_Map* upb_Map_New(upb_Arena* a, upb_CType key_type, upb_CType value_type) {
  return _upb_Map_New(a, _upb_Map_CTypeSize(key_type),
                      _upb_Map_CTypeSize(value_type));
}

size_t upb_Map_Size(const upb_Map* map) { return _upb_Map_Size(map); }

bool upb_Map_Get(const upb_Map* map, upb_MessageValue key,
                 upb_MessageValue* val) {
  return _upb_Map_Get(map, &key, map->key_size, val, map->val_size);
}

void upb_Map_Clear(upb_Map* map) { _upb_Map_Clear(map); }

upb_MapInsertStatus upb_Map_Insert(upb_Map* map, upb_MessageValue key,
                                   upb_MessageValue val, upb_Arena* arena) {
  UPB_ASSERT(arena);
  return (upb_MapInsertStatus)_upb_Map_Insert(map, &key, map->key_size, &val,
                                              map->val_size, arena);
}

bool upb_Map_Delete(upb_Map* map, upb_MessageValue key, upb_MessageValue* val) {
  upb_value v;
  const bool removed = _upb_Map_Delete(map, &key, map->key_size, &v);
  if (val) _upb_map_fromvalue(v, val, map->val_size);
  return removed;
}

bool upb_Map_Next(const upb_Map* map, upb_MessageValue* key,
                  upb_MessageValue* val, size_t* iter) {
  upb_StringView k;
  upb_value v;
  const bool ok = upb_strtable_next2(&map->table, &k, &v, (intptr_t*)iter);
  if (ok) {
    _upb_map_fromkey(k, key, map->key_size);
    _upb_map_fromvalue(v, val, map->val_size);
  }
  return ok;
}

UPB_API void upb_Map_SetEntryValue(upb_Map* map, size_t iter,
                                   upb_MessageValue val) {
  upb_value v;
  _upb_map_tovalue(&val, map->val_size, &v, NULL);
  upb_strtable_setentryvalue(&map->table, iter, v);
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

// Returns the key and value for this entry of the map.
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

// EVERYTHING BELOW THIS LINE IS INTERNAL - DO NOT USE /////////////////////////

upb_Map* _upb_Map_New(upb_Arena* a, size_t key_size, size_t value_size) {
  upb_Map* map = upb_Arena_Malloc(a, sizeof(upb_Map));
  if (!map) return NULL;

  upb_strtable_init(&map->table, 4, a);
  map->key_size = key_size;
  map->val_size = value_size;

  return map;
}


// Must be last.

static void _upb_mapsorter_getkeys(const void* _a, const void* _b, void* a_key,
                                   void* b_key, size_t size) {
  const upb_tabent* const* a = _a;
  const upb_tabent* const* b = _b;
  upb_StringView a_tabkey = upb_tabstrview((*a)->key);
  upb_StringView b_tabkey = upb_tabstrview((*b)->key);
  _upb_map_fromkey(a_tabkey, a_key, size);
  _upb_map_fromkey(b_tabkey, b_key, size);
}

static int _upb_mapsorter_cmpi64(const void* _a, const void* _b) {
  int64_t a, b;
  _upb_mapsorter_getkeys(_a, _b, &a, &b, 8);
  return a < b ? -1 : a > b;
}

static int _upb_mapsorter_cmpu64(const void* _a, const void* _b) {
  uint64_t a, b;
  _upb_mapsorter_getkeys(_a, _b, &a, &b, 8);
  return a < b ? -1 : a > b;
}

static int _upb_mapsorter_cmpi32(const void* _a, const void* _b) {
  int32_t a, b;
  _upb_mapsorter_getkeys(_a, _b, &a, &b, 4);
  return a < b ? -1 : a > b;
}

static int _upb_mapsorter_cmpu32(const void* _a, const void* _b) {
  uint32_t a, b;
  _upb_mapsorter_getkeys(_a, _b, &a, &b, 4);
  return a < b ? -1 : a > b;
}

static int _upb_mapsorter_cmpbool(const void* _a, const void* _b) {
  bool a, b;
  _upb_mapsorter_getkeys(_a, _b, &a, &b, 1);
  return a < b ? -1 : a > b;
}

static int _upb_mapsorter_cmpstr(const void* _a, const void* _b) {
  upb_StringView a, b;
  _upb_mapsorter_getkeys(_a, _b, &a, &b, UPB_MAPTYPE_STRING);
  size_t common_size = UPB_MIN(a.size, b.size);
  int cmp = memcmp(a.data, b.data, common_size);
  if (cmp) return -cmp;
  return a.size < b.size ? -1 : a.size > b.size;
}

static int (*const compar[kUpb_FieldType_SizeOf])(const void*, const void*) = {
    [kUpb_FieldType_Int64] = _upb_mapsorter_cmpi64,
    [kUpb_FieldType_SFixed64] = _upb_mapsorter_cmpi64,
    [kUpb_FieldType_SInt64] = _upb_mapsorter_cmpi64,

    [kUpb_FieldType_UInt64] = _upb_mapsorter_cmpu64,
    [kUpb_FieldType_Fixed64] = _upb_mapsorter_cmpu64,

    [kUpb_FieldType_Int32] = _upb_mapsorter_cmpi32,
    [kUpb_FieldType_SInt32] = _upb_mapsorter_cmpi32,
    [kUpb_FieldType_SFixed32] = _upb_mapsorter_cmpi32,
    [kUpb_FieldType_Enum] = _upb_mapsorter_cmpi32,

    [kUpb_FieldType_UInt32] = _upb_mapsorter_cmpu32,
    [kUpb_FieldType_Fixed32] = _upb_mapsorter_cmpu32,

    [kUpb_FieldType_Bool] = _upb_mapsorter_cmpbool,

    [kUpb_FieldType_String] = _upb_mapsorter_cmpstr,
    [kUpb_FieldType_Bytes] = _upb_mapsorter_cmpstr,
};

static bool _upb_mapsorter_resize(_upb_mapsorter* s, _upb_sortedmap* sorted,
                                  int size) {
  sorted->start = s->size;
  sorted->pos = sorted->start;
  sorted->end = sorted->start + size;

  if (sorted->end > s->cap) {
    s->cap = upb_Log2CeilingSize(sorted->end);
    s->entries = realloc(s->entries, s->cap * sizeof(*s->entries));
    if (!s->entries) return false;
  }

  s->size = sorted->end;
  return true;
}

bool _upb_mapsorter_pushmap(_upb_mapsorter* s, upb_FieldType key_type,
                            const upb_Map* map, _upb_sortedmap* sorted) {
  int map_size = _upb_Map_Size(map);

  if (!_upb_mapsorter_resize(s, sorted, map_size)) return false;

  // Copy non-empty entries from the table to s->entries.
  const void** dst = &s->entries[sorted->start];
  const upb_tabent* src = map->table.t.entries;
  const upb_tabent* end = src + upb_table_size(&map->table.t);
  for (; src < end; src++) {
    if (!upb_tabent_isempty(src)) {
      *dst = src;
      dst++;
    }
  }
  UPB_ASSERT(dst == &s->entries[sorted->end]);

  // Sort entries according to the key type.
  qsort(&s->entries[sorted->start], map_size, sizeof(*s->entries),
        compar[key_type]);
  return true;
}

static int _upb_mapsorter_cmpext(const void* _a, const void* _b) {
  const upb_Message_Extension* const* a = _a;
  const upb_Message_Extension* const* b = _b;
  uint32_t a_num = (*a)->ext->field.number;
  uint32_t b_num = (*b)->ext->field.number;
  assert(a_num != b_num);
  return a_num < b_num ? -1 : 1;
}

bool _upb_mapsorter_pushexts(_upb_mapsorter* s,
                             const upb_Message_Extension* exts, size_t count,
                             _upb_sortedmap* sorted) {
  if (!_upb_mapsorter_resize(s, sorted, count)) return false;

  for (size_t i = 0; i < count; i++) {
    s->entries[sorted->start + i] = &exts[i];
  }

  qsort(&s->entries[sorted->start], count, sizeof(*s->entries),
        _upb_mapsorter_cmpext);
  return true;
}
/* This file was generated by upbc (the upb compiler) from the input
 * file:
 *
 *     google/protobuf/descriptor.proto
 *
 * Do not edit -- your changes will be discarded when the file is
 * regenerated. */

#include <stddef.h>

// Must be last.

static const upb_MiniTableSub google_protobuf_FileDescriptorSet_submsgs[1] = {
  {.submsg = &google_protobuf_FileDescriptorProto_msg_init},
};

static const upb_MiniTableField google_protobuf_FileDescriptorSet__fields[1] = {
  {1, 0, 0, 0, 11, (int)kUpb_FieldMode_Array | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_FileDescriptorSet_msg_init = {
  &google_protobuf_FileDescriptorSet_submsgs[0],
  &google_protobuf_FileDescriptorSet__fields[0],
  8, 1, kUpb_ExtMode_NonExtendable, 1, UPB_FASTTABLE_MASK(8), 0,
  UPB_FASTTABLE_INIT({
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x000000003f00000a, &upb_prm_1bt_max192b},
  })
};

static const upb_MiniTableSub google_protobuf_FileDescriptorProto_submsgs[6] = {
  {.submsg = &google_protobuf_DescriptorProto_msg_init},
  {.submsg = &google_protobuf_EnumDescriptorProto_msg_init},
  {.submsg = &google_protobuf_ServiceDescriptorProto_msg_init},
  {.submsg = &google_protobuf_FieldDescriptorProto_msg_init},
  {.submsg = &google_protobuf_FileOptions_msg_init},
  {.submsg = &google_protobuf_SourceCodeInfo_msg_init},
};

static const upb_MiniTableField google_protobuf_FileDescriptorProto__fields[13] = {
  {1, UPB_SIZE(40, 8), 1, kUpb_NoSub, 12, (int)kUpb_FieldMode_Scalar | (int)kUpb_LabelFlags_IsAlternate | ((int)kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {2, UPB_SIZE(48, 24), 2, kUpb_NoSub, 12, (int)kUpb_FieldMode_Scalar | (int)kUpb_LabelFlags_IsAlternate | ((int)kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {3, UPB_SIZE(4, 40), 0, kUpb_NoSub, 12, (int)kUpb_FieldMode_Array | (int)kUpb_LabelFlags_IsAlternate | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {4, UPB_SIZE(8, 48), 0, 0, 11, (int)kUpb_FieldMode_Array | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {5, UPB_SIZE(12, 56), 0, 1, 11, (int)kUpb_FieldMode_Array | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {6, UPB_SIZE(16, 64), 0, 2, 11, (int)kUpb_FieldMode_Array | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {7, UPB_SIZE(20, 72), 0, 3, 11, (int)kUpb_FieldMode_Array | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {8, UPB_SIZE(24, 80), 3, 4, 11, (int)kUpb_FieldMode_Scalar | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {9, UPB_SIZE(28, 88), 4, 5, 11, (int)kUpb_FieldMode_Scalar | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {10, UPB_SIZE(32, 96), 0, kUpb_NoSub, 5, (int)kUpb_FieldMode_Array | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {11, UPB_SIZE(36, 104), 0, kUpb_NoSub, 5, (int)kUpb_FieldMode_Array | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {12, UPB_SIZE(56, 112), 5, kUpb_NoSub, 12, (int)kUpb_FieldMode_Scalar | (int)kUpb_LabelFlags_IsAlternate | ((int)kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {13, UPB_SIZE(64, 128), 6, kUpb_NoSub, 12, (int)kUpb_FieldMode_Scalar | (int)kUpb_LabelFlags_IsAlternate | ((int)kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_FileDescriptorProto_msg_init = {
  &google_protobuf_FileDescriptorProto_submsgs[0],
  &google_protobuf_FileDescriptorProto__fields[0],
  UPB_SIZE(72, 144), 13, kUpb_ExtMode_NonExtendable, 13, UPB_FASTTABLE_MASK(120), 0,
  UPB_FASTTABLE_INIT({
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x000800000100000a, &upb_pss_1bt},
    {0x0018000002000012, &upb_pss_1bt},
    {0x002800003f00001a, &upb_prs_1bt},
    {0x003000003f000022, &upb_prm_1bt_max128b},
    {0x003800003f01002a, &upb_prm_1bt_max64b},
    {0x004000003f020032, &upb_prm_1bt_max64b},
    {0x004800003f03003a, &upb_prm_1bt_max128b},
    {0x0050000003040042, &upb_psm_1bt_max256b},
    {0x005800000405004a, &upb_psm_1bt_max64b},
    {0x006000003f000050, &upb_prv4_1bt},
    {0x006800003f000058, &upb_prv4_1bt},
    {0x0070000005000062, &upb_pss_1bt},
    {0x008000000600006a, &upb_pss_1bt},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
  })
};

static const upb_MiniTableSub google_protobuf_DescriptorProto_submsgs[8] = {
  {.submsg = &google_protobuf_FieldDescriptorProto_msg_init},
  {.submsg = &google_protobuf_DescriptorProto_msg_init},
  {.submsg = &google_protobuf_EnumDescriptorProto_msg_init},
  {.submsg = &google_protobuf_DescriptorProto_ExtensionRange_msg_init},
  {.submsg = &google_protobuf_FieldDescriptorProto_msg_init},
  {.submsg = &google_protobuf_MessageOptions_msg_init},
  {.submsg = &google_protobuf_OneofDescriptorProto_msg_init},
  {.submsg = &google_protobuf_DescriptorProto_ReservedRange_msg_init},
};

static const upb_MiniTableField google_protobuf_DescriptorProto__fields[10] = {
  {1, UPB_SIZE(40, 8), 1, kUpb_NoSub, 12, (int)kUpb_FieldMode_Scalar | (int)kUpb_LabelFlags_IsAlternate | ((int)kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {2, UPB_SIZE(4, 24), 0, 0, 11, (int)kUpb_FieldMode_Array | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {3, UPB_SIZE(8, 32), 0, 1, 11, (int)kUpb_FieldMode_Array | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {4, UPB_SIZE(12, 40), 0, 2, 11, (int)kUpb_FieldMode_Array | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {5, UPB_SIZE(16, 48), 0, 3, 11, (int)kUpb_FieldMode_Array | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {6, UPB_SIZE(20, 56), 0, 4, 11, (int)kUpb_FieldMode_Array | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {7, UPB_SIZE(24, 64), 2, 5, 11, (int)kUpb_FieldMode_Scalar | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {8, UPB_SIZE(28, 72), 0, 6, 11, (int)kUpb_FieldMode_Array | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {9, UPB_SIZE(32, 80), 0, 7, 11, (int)kUpb_FieldMode_Array | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {10, UPB_SIZE(36, 88), 0, kUpb_NoSub, 12, (int)kUpb_FieldMode_Array | (int)kUpb_LabelFlags_IsAlternate | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_DescriptorProto_msg_init = {
  &google_protobuf_DescriptorProto_submsgs[0],
  &google_protobuf_DescriptorProto__fields[0],
  UPB_SIZE(48, 96), 10, kUpb_ExtMode_NonExtendable, 10, UPB_FASTTABLE_MASK(120), 0,
  UPB_FASTTABLE_INIT({
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x000800000100000a, &upb_pss_1bt},
    {0x001800003f000012, &upb_prm_1bt_max128b},
    {0x002000003f01001a, &upb_prm_1bt_max128b},
    {0x002800003f020022, &upb_prm_1bt_max64b},
    {0x003000003f03002a, &upb_prm_1bt_max64b},
    {0x003800003f040032, &upb_prm_1bt_max128b},
    {0x004000000205003a, &upb_psm_1bt_max64b},
    {0x004800003f060042, &upb_prm_1bt_max64b},
    {0x005000003f07004a, &upb_prm_1bt_max64b},
    {0x005800003f000052, &upb_prs_1bt},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
  })
};

static const upb_MiniTableSub google_protobuf_DescriptorProto_ExtensionRange_submsgs[1] = {
  {.submsg = &google_protobuf_ExtensionRangeOptions_msg_init},
};

static const upb_MiniTableField google_protobuf_DescriptorProto_ExtensionRange__fields[3] = {
  {1, 4, 1, kUpb_NoSub, 5, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)},
  {2, 8, 2, kUpb_NoSub, 5, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)},
  {3, UPB_SIZE(12, 16), 3, 0, 11, (int)kUpb_FieldMode_Scalar | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_DescriptorProto_ExtensionRange_msg_init = {
  &google_protobuf_DescriptorProto_ExtensionRange_submsgs[0],
  &google_protobuf_DescriptorProto_ExtensionRange__fields[0],
  UPB_SIZE(16, 24), 3, kUpb_ExtMode_NonExtendable, 3, UPB_FASTTABLE_MASK(24), 0,
  UPB_FASTTABLE_INIT({
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0004000001000008, &upb_psv4_1bt},
    {0x0008000002000010, &upb_psv4_1bt},
    {0x001000000300001a, &upb_psm_1bt_max64b},
  })
};

static const upb_MiniTableField google_protobuf_DescriptorProto_ReservedRange__fields[2] = {
  {1, 4, 1, kUpb_NoSub, 5, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)},
  {2, 8, 2, kUpb_NoSub, 5, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_DescriptorProto_ReservedRange_msg_init = {
  NULL,
  &google_protobuf_DescriptorProto_ReservedRange__fields[0],
  16, 2, kUpb_ExtMode_NonExtendable, 2, UPB_FASTTABLE_MASK(24), 0,
  UPB_FASTTABLE_INIT({
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0004000001000008, &upb_psv4_1bt},
    {0x0008000002000010, &upb_psv4_1bt},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
  })
};

static const upb_MiniTableSub google_protobuf_ExtensionRangeOptions_submsgs[4] = {
  {.submsg = &google_protobuf_ExtensionRangeOptions_Declaration_msg_init},
  {.submsg = &google_protobuf_FeatureSet_msg_init},
  {.submsg = &google_protobuf_UninterpretedOption_msg_init},
  {.subenum = &google_protobuf_ExtensionRangeOptions_VerificationState_enum_init},
};

static const upb_MiniTableField google_protobuf_ExtensionRangeOptions__fields[4] = {
  {2, UPB_SIZE(4, 8), 0, 0, 11, (int)kUpb_FieldMode_Array | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {3, UPB_SIZE(8, 4), 1, 3, 14, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)},
  {50, UPB_SIZE(12, 16), 2, 1, 11, (int)kUpb_FieldMode_Scalar | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {999, UPB_SIZE(16, 24), 0, 2, 11, (int)kUpb_FieldMode_Array | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_ExtensionRangeOptions_msg_init = {
  &google_protobuf_ExtensionRangeOptions_submsgs[0],
  &google_protobuf_ExtensionRangeOptions__fields[0],
  UPB_SIZE(24, 32), 4, kUpb_ExtMode_Extendable, 0, UPB_FASTTABLE_MASK(248), 0,
  UPB_FASTTABLE_INIT({
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x000800003f000012, &upb_prm_1bt_max64b},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0010000002010392, &upb_psm_2bt_max64b},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x001800003f023eba, &upb_prm_2bt_max128b},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
  })
};

static const upb_MiniTableField google_protobuf_ExtensionRangeOptions_Declaration__fields[5] = {
  {1, 4, 1, kUpb_NoSub, 5, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)},
  {2, UPB_SIZE(12, 16), 2, kUpb_NoSub, 12, (int)kUpb_FieldMode_Scalar | (int)kUpb_LabelFlags_IsAlternate | ((int)kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {3, UPB_SIZE(20, 32), 3, kUpb_NoSub, 12, (int)kUpb_FieldMode_Scalar | (int)kUpb_LabelFlags_IsAlternate | ((int)kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {5, 8, 4, kUpb_NoSub, 8, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
  {6, 9, 5, kUpb_NoSub, 8, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_ExtensionRangeOptions_Declaration_msg_init = {
  NULL,
  &google_protobuf_ExtensionRangeOptions_Declaration__fields[0],
  UPB_SIZE(32, 48), 5, kUpb_ExtMode_NonExtendable, 3, UPB_FASTTABLE_MASK(56), 0,
  UPB_FASTTABLE_INIT({
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0004000001000008, &upb_psv4_1bt},
    {0x0010000002000012, &upb_pss_1bt},
    {0x002000000300001a, &upb_pss_1bt},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0008000004000028, &upb_psb1_1bt},
    {0x0009000005000030, &upb_psb1_1bt},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
  })
};

static const upb_MiniTableSub google_protobuf_FieldDescriptorProto_submsgs[3] = {
  {.submsg = &google_protobuf_FieldOptions_msg_init},
  {.subenum = &google_protobuf_FieldDescriptorProto_Label_enum_init},
  {.subenum = &google_protobuf_FieldDescriptorProto_Type_enum_init},
};

static const upb_MiniTableField google_protobuf_FieldDescriptorProto__fields[11] = {
  {1, UPB_SIZE(28, 24), 1, kUpb_NoSub, 12, (int)kUpb_FieldMode_Scalar | (int)kUpb_LabelFlags_IsAlternate | ((int)kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {2, UPB_SIZE(36, 40), 2, kUpb_NoSub, 12, (int)kUpb_FieldMode_Scalar | (int)kUpb_LabelFlags_IsAlternate | ((int)kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {3, 4, 3, kUpb_NoSub, 5, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)},
  {4, 8, 4, 1, 14, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)},
  {5, 12, 5, 2, 14, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)},
  {6, UPB_SIZE(44, 56), 6, kUpb_NoSub, 12, (int)kUpb_FieldMode_Scalar | (int)kUpb_LabelFlags_IsAlternate | ((int)kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {7, UPB_SIZE(52, 72), 7, kUpb_NoSub, 12, (int)kUpb_FieldMode_Scalar | (int)kUpb_LabelFlags_IsAlternate | ((int)kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {8, UPB_SIZE(16, 88), 8, 0, 11, (int)kUpb_FieldMode_Scalar | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {9, UPB_SIZE(20, 16), 9, kUpb_NoSub, 5, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)},
  {10, UPB_SIZE(60, 96), 10, kUpb_NoSub, 12, (int)kUpb_FieldMode_Scalar | (int)kUpb_LabelFlags_IsAlternate | ((int)kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {17, UPB_SIZE(24, 20), 11, kUpb_NoSub, 8, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_FieldDescriptorProto_msg_init = {
  &google_protobuf_FieldDescriptorProto_submsgs[0],
  &google_protobuf_FieldDescriptorProto__fields[0],
  UPB_SIZE(72, 112), 11, kUpb_ExtMode_NonExtendable, 10, UPB_FASTTABLE_MASK(248), 0,
  UPB_FASTTABLE_INIT({
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x001800000100000a, &upb_pss_1bt},
    {0x0028000002000012, &upb_pss_1bt},
    {0x0004000003000018, &upb_psv4_1bt},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0038000006000032, &upb_pss_1bt},
    {0x004800000700003a, &upb_pss_1bt},
    {0x0058000008000042, &upb_psm_1bt_max64b},
    {0x0010000009000048, &upb_psv4_1bt},
    {0x006000000a000052, &upb_pss_1bt},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x001400000b000188, &upb_psb1_2bt},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
  })
};

static const upb_MiniTableSub google_protobuf_OneofDescriptorProto_submsgs[1] = {
  {.submsg = &google_protobuf_OneofOptions_msg_init},
};

static const upb_MiniTableField google_protobuf_OneofDescriptorProto__fields[2] = {
  {1, 8, 1, kUpb_NoSub, 12, (int)kUpb_FieldMode_Scalar | (int)kUpb_LabelFlags_IsAlternate | ((int)kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {2, UPB_SIZE(4, 24), 2, 0, 11, (int)kUpb_FieldMode_Scalar | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_OneofDescriptorProto_msg_init = {
  &google_protobuf_OneofDescriptorProto_submsgs[0],
  &google_protobuf_OneofDescriptorProto__fields[0],
  UPB_SIZE(16, 32), 2, kUpb_ExtMode_NonExtendable, 2, UPB_FASTTABLE_MASK(24), 0,
  UPB_FASTTABLE_INIT({
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x000800000100000a, &upb_pss_1bt},
    {0x0018000002000012, &upb_psm_1bt_max64b},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
  })
};

static const upb_MiniTableSub google_protobuf_EnumDescriptorProto_submsgs[3] = {
  {.submsg = &google_protobuf_EnumValueDescriptorProto_msg_init},
  {.submsg = &google_protobuf_EnumOptions_msg_init},
  {.submsg = &google_protobuf_EnumDescriptorProto_EnumReservedRange_msg_init},
};

static const upb_MiniTableField google_protobuf_EnumDescriptorProto__fields[5] = {
  {1, UPB_SIZE(20, 8), 1, kUpb_NoSub, 12, (int)kUpb_FieldMode_Scalar | (int)kUpb_LabelFlags_IsAlternate | ((int)kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {2, UPB_SIZE(4, 24), 0, 0, 11, (int)kUpb_FieldMode_Array | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {3, UPB_SIZE(8, 32), 2, 1, 11, (int)kUpb_FieldMode_Scalar | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {4, UPB_SIZE(12, 40), 0, 2, 11, (int)kUpb_FieldMode_Array | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {5, UPB_SIZE(16, 48), 0, kUpb_NoSub, 12, (int)kUpb_FieldMode_Array | (int)kUpb_LabelFlags_IsAlternate | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_EnumDescriptorProto_msg_init = {
  &google_protobuf_EnumDescriptorProto_submsgs[0],
  &google_protobuf_EnumDescriptorProto__fields[0],
  UPB_SIZE(32, 56), 5, kUpb_ExtMode_NonExtendable, 5, UPB_FASTTABLE_MASK(56), 0,
  UPB_FASTTABLE_INIT({
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x000800000100000a, &upb_pss_1bt},
    {0x001800003f000012, &upb_prm_1bt_max64b},
    {0x002000000201001a, &upb_psm_1bt_max64b},
    {0x002800003f020022, &upb_prm_1bt_max64b},
    {0x003000003f00002a, &upb_prs_1bt},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
  })
};

static const upb_MiniTableField google_protobuf_EnumDescriptorProto_EnumReservedRange__fields[2] = {
  {1, 4, 1, kUpb_NoSub, 5, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)},
  {2, 8, 2, kUpb_NoSub, 5, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_EnumDescriptorProto_EnumReservedRange_msg_init = {
  NULL,
  &google_protobuf_EnumDescriptorProto_EnumReservedRange__fields[0],
  16, 2, kUpb_ExtMode_NonExtendable, 2, UPB_FASTTABLE_MASK(24), 0,
  UPB_FASTTABLE_INIT({
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0004000001000008, &upb_psv4_1bt},
    {0x0008000002000010, &upb_psv4_1bt},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
  })
};

static const upb_MiniTableSub google_protobuf_EnumValueDescriptorProto_submsgs[1] = {
  {.submsg = &google_protobuf_EnumValueOptions_msg_init},
};

static const upb_MiniTableField google_protobuf_EnumValueDescriptorProto__fields[3] = {
  {1, UPB_SIZE(12, 8), 1, kUpb_NoSub, 12, (int)kUpb_FieldMode_Scalar | (int)kUpb_LabelFlags_IsAlternate | ((int)kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {2, 4, 2, kUpb_NoSub, 5, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)},
  {3, UPB_SIZE(8, 24), 3, 0, 11, (int)kUpb_FieldMode_Scalar | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_EnumValueDescriptorProto_msg_init = {
  &google_protobuf_EnumValueDescriptorProto_submsgs[0],
  &google_protobuf_EnumValueDescriptorProto__fields[0],
  UPB_SIZE(24, 32), 3, kUpb_ExtMode_NonExtendable, 3, UPB_FASTTABLE_MASK(24), 0,
  UPB_FASTTABLE_INIT({
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x000800000100000a, &upb_pss_1bt},
    {0x0004000002000010, &upb_psv4_1bt},
    {0x001800000300001a, &upb_psm_1bt_max64b},
  })
};

static const upb_MiniTableSub google_protobuf_ServiceDescriptorProto_submsgs[2] = {
  {.submsg = &google_protobuf_MethodDescriptorProto_msg_init},
  {.submsg = &google_protobuf_ServiceOptions_msg_init},
};

static const upb_MiniTableField google_protobuf_ServiceDescriptorProto__fields[3] = {
  {1, UPB_SIZE(12, 8), 1, kUpb_NoSub, 12, (int)kUpb_FieldMode_Scalar | (int)kUpb_LabelFlags_IsAlternate | ((int)kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {2, UPB_SIZE(4, 24), 0, 0, 11, (int)kUpb_FieldMode_Array | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {3, UPB_SIZE(8, 32), 2, 1, 11, (int)kUpb_FieldMode_Scalar | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_ServiceDescriptorProto_msg_init = {
  &google_protobuf_ServiceDescriptorProto_submsgs[0],
  &google_protobuf_ServiceDescriptorProto__fields[0],
  UPB_SIZE(24, 40), 3, kUpb_ExtMode_NonExtendable, 3, UPB_FASTTABLE_MASK(24), 0,
  UPB_FASTTABLE_INIT({
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x000800000100000a, &upb_pss_1bt},
    {0x001800003f000012, &upb_prm_1bt_max128b},
    {0x002000000201001a, &upb_psm_1bt_max64b},
  })
};

static const upb_MiniTableSub google_protobuf_MethodDescriptorProto_submsgs[1] = {
  {.submsg = &google_protobuf_MethodOptions_msg_init},
};

static const upb_MiniTableField google_protobuf_MethodDescriptorProto__fields[6] = {
  {1, UPB_SIZE(12, 8), 1, kUpb_NoSub, 12, (int)kUpb_FieldMode_Scalar | (int)kUpb_LabelFlags_IsAlternate | ((int)kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {2, UPB_SIZE(20, 24), 2, kUpb_NoSub, 12, (int)kUpb_FieldMode_Scalar | (int)kUpb_LabelFlags_IsAlternate | ((int)kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {3, UPB_SIZE(28, 40), 3, kUpb_NoSub, 12, (int)kUpb_FieldMode_Scalar | (int)kUpb_LabelFlags_IsAlternate | ((int)kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {4, UPB_SIZE(4, 56), 4, 0, 11, (int)kUpb_FieldMode_Scalar | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {5, UPB_SIZE(8, 1), 5, kUpb_NoSub, 8, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
  {6, UPB_SIZE(9, 2), 6, kUpb_NoSub, 8, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_MethodDescriptorProto_msg_init = {
  &google_protobuf_MethodDescriptorProto_submsgs[0],
  &google_protobuf_MethodDescriptorProto__fields[0],
  UPB_SIZE(40, 64), 6, kUpb_ExtMode_NonExtendable, 6, UPB_FASTTABLE_MASK(56), 0,
  UPB_FASTTABLE_INIT({
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x000800000100000a, &upb_pss_1bt},
    {0x0018000002000012, &upb_pss_1bt},
    {0x002800000300001a, &upb_pss_1bt},
    {0x0038000004000022, &upb_psm_1bt_max64b},
    {0x0001000005000028, &upb_psb1_1bt},
    {0x0002000006000030, &upb_psb1_1bt},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
  })
};

static const upb_MiniTableSub google_protobuf_FileOptions_submsgs[3] = {
  {.submsg = &google_protobuf_FeatureSet_msg_init},
  {.submsg = &google_protobuf_UninterpretedOption_msg_init},
  {.subenum = &google_protobuf_FileOptions_OptimizeMode_enum_init},
};

static const upb_MiniTableField google_protobuf_FileOptions__fields[22] = {
  {1, UPB_SIZE(28, 24), 1, kUpb_NoSub, 12, (int)kUpb_FieldMode_Scalar | (int)kUpb_LabelFlags_IsAlternate | ((int)kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {8, UPB_SIZE(36, 40), 2, kUpb_NoSub, 12, (int)kUpb_FieldMode_Scalar | (int)kUpb_LabelFlags_IsAlternate | ((int)kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {9, 4, 3, 2, 14, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)},
  {10, 8, 4, kUpb_NoSub, 8, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
  {11, UPB_SIZE(44, 56), 5, kUpb_NoSub, 12, (int)kUpb_FieldMode_Scalar | (int)kUpb_LabelFlags_IsAlternate | ((int)kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {16, 9, 6, kUpb_NoSub, 8, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
  {17, 10, 7, kUpb_NoSub, 8, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
  {18, 11, 8, kUpb_NoSub, 8, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
  {20, 12, 9, kUpb_NoSub, 8, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
  {23, 13, 10, kUpb_NoSub, 8, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
  {27, 14, 11, kUpb_NoSub, 8, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
  {31, 15, 12, kUpb_NoSub, 8, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
  {36, UPB_SIZE(52, 72), 13, kUpb_NoSub, 12, (int)kUpb_FieldMode_Scalar | (int)kUpb_LabelFlags_IsAlternate | ((int)kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {37, UPB_SIZE(60, 88), 14, kUpb_NoSub, 12, (int)kUpb_FieldMode_Scalar | (int)kUpb_LabelFlags_IsAlternate | ((int)kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {39, UPB_SIZE(68, 104), 15, kUpb_NoSub, 12, (int)kUpb_FieldMode_Scalar | (int)kUpb_LabelFlags_IsAlternate | ((int)kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {40, UPB_SIZE(76, 120), 16, kUpb_NoSub, 12, (int)kUpb_FieldMode_Scalar | (int)kUpb_LabelFlags_IsAlternate | ((int)kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {41, UPB_SIZE(84, 136), 17, kUpb_NoSub, 12, (int)kUpb_FieldMode_Scalar | (int)kUpb_LabelFlags_IsAlternate | ((int)kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {42, 16, 18, kUpb_NoSub, 8, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
  {44, UPB_SIZE(92, 152), 19, kUpb_NoSub, 12, (int)kUpb_FieldMode_Scalar | (int)kUpb_LabelFlags_IsAlternate | ((int)kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {45, UPB_SIZE(100, 168), 20, kUpb_NoSub, 12, (int)kUpb_FieldMode_Scalar | (int)kUpb_LabelFlags_IsAlternate | ((int)kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {50, UPB_SIZE(20, 184), 21, 0, 11, (int)kUpb_FieldMode_Scalar | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {999, UPB_SIZE(24, 192), 0, 1, 11, (int)kUpb_FieldMode_Array | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_FileOptions_msg_init = {
  &google_protobuf_FileOptions_submsgs[0],
  &google_protobuf_FileOptions__fields[0],
  UPB_SIZE(112, 200), 22, kUpb_ExtMode_Extendable, 1, UPB_FASTTABLE_MASK(248), 0,
  UPB_FASTTABLE_INIT({
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x001800000100000a, &upb_pss_1bt},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0028000002000042, &upb_pss_1bt},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0008000004000050, &upb_psb1_1bt},
    {0x003800000500005a, &upb_pss_1bt},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0009000006000180, &upb_psb1_2bt},
    {0x000a000007000188, &upb_psb1_2bt},
    {0x000b000008000190, &upb_psb1_2bt},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x000c0000090001a0, &upb_psb1_2bt},
    {0x005800000e0002aa, &upb_pss_2bt},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x000d00000a0001b8, &upb_psb1_2bt},
    {0x00780000100002c2, &upb_pss_2bt},
    {0x00880000110002ca, &upb_pss_2bt},
    {0x00100000120002d0, &upb_psb1_2bt},
    {0x000e00000b0001d8, &upb_psb1_2bt},
    {0x00980000130002e2, &upb_pss_2bt},
    {0x00a80000140002ea, &upb_pss_2bt},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x000f00000c0001f8, &upb_psb1_2bt},
  })
};

static const upb_MiniTableSub google_protobuf_MessageOptions_submsgs[2] = {
  {.submsg = &google_protobuf_FeatureSet_msg_init},
  {.submsg = &google_protobuf_UninterpretedOption_msg_init},
};

static const upb_MiniTableField google_protobuf_MessageOptions__fields[7] = {
  {1, 1, 1, kUpb_NoSub, 8, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
  {2, 2, 2, kUpb_NoSub, 8, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
  {3, 3, 3, kUpb_NoSub, 8, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
  {7, 4, 4, kUpb_NoSub, 8, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
  {11, 5, 5, kUpb_NoSub, 8, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
  {12, 8, 6, 0, 11, (int)kUpb_FieldMode_Scalar | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {999, UPB_SIZE(12, 16), 0, 1, 11, (int)kUpb_FieldMode_Array | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_MessageOptions_msg_init = {
  &google_protobuf_MessageOptions_submsgs[0],
  &google_protobuf_MessageOptions__fields[0],
  UPB_SIZE(16, 24), 7, kUpb_ExtMode_Extendable, 3, UPB_FASTTABLE_MASK(248), 0,
  UPB_FASTTABLE_INIT({
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0001000001000008, &upb_psb1_1bt},
    {0x0002000002000010, &upb_psb1_1bt},
    {0x0003000003000018, &upb_psb1_1bt},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0004000004000038, &upb_psb1_1bt},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0005000005000058, &upb_psb1_1bt},
    {0x0008000006000062, &upb_psm_1bt_max64b},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x001000003f013eba, &upb_prm_2bt_max128b},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
  })
};

static const upb_MiniTableSub google_protobuf_FieldOptions_submsgs[7] = {
  {.submsg = &google_protobuf_FieldOptions_EditionDefault_msg_init},
  {.submsg = &google_protobuf_FeatureSet_msg_init},
  {.submsg = &google_protobuf_UninterpretedOption_msg_init},
  {.subenum = &google_protobuf_FieldOptions_CType_enum_init},
  {.subenum = &google_protobuf_FieldOptions_JSType_enum_init},
  {.subenum = &google_protobuf_FieldOptions_OptionRetention_enum_init},
  {.subenum = &google_protobuf_FieldOptions_OptionTargetType_enum_init},
};

static const upb_MiniTableField google_protobuf_FieldOptions__fields[13] = {
  {1, 4, 1, 3, 14, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)},
  {2, 8, 2, kUpb_NoSub, 8, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
  {3, 9, 3, kUpb_NoSub, 8, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
  {5, 10, 4, kUpb_NoSub, 8, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
  {6, 12, 5, 4, 14, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)},
  {10, 16, 6, kUpb_NoSub, 8, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
  {15, 17, 7, kUpb_NoSub, 8, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
  {16, 18, 8, kUpb_NoSub, 8, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
  {17, 20, 9, 5, 14, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)},
  {19, 24, 0, 6, 14, (int)kUpb_FieldMode_Array | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {20, UPB_SIZE(28, 32), 0, 0, 11, (int)kUpb_FieldMode_Array | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {21, UPB_SIZE(32, 40), 10, 1, 11, (int)kUpb_FieldMode_Scalar | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {999, UPB_SIZE(36, 48), 0, 2, 11, (int)kUpb_FieldMode_Array | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_FieldOptions_msg_init = {
  &google_protobuf_FieldOptions_submsgs[0],
  &google_protobuf_FieldOptions__fields[0],
  UPB_SIZE(40, 56), 13, kUpb_ExtMode_Extendable, 3, UPB_FASTTABLE_MASK(248), 0,
  UPB_FASTTABLE_INIT({
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0008000002000010, &upb_psb1_1bt},
    {0x0009000003000018, &upb_psb1_1bt},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x000a000004000028, &upb_psb1_1bt},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0010000006000050, &upb_psb1_1bt},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0011000007000078, &upb_psb1_1bt},
    {0x0012000008000180, &upb_psb1_2bt},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x002000003f0001a2, &upb_prm_2bt_max64b},
    {0x002800000a0101aa, &upb_psm_2bt_max64b},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x003000003f023eba, &upb_prm_2bt_max128b},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
  })
};

static const upb_MiniTableField google_protobuf_FieldOptions_EditionDefault__fields[2] = {
  {1, UPB_SIZE(4, 8), 1, kUpb_NoSub, 12, (int)kUpb_FieldMode_Scalar | (int)kUpb_LabelFlags_IsAlternate | ((int)kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {2, UPB_SIZE(12, 24), 2, kUpb_NoSub, 12, (int)kUpb_FieldMode_Scalar | (int)kUpb_LabelFlags_IsAlternate | ((int)kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_FieldOptions_EditionDefault_msg_init = {
  NULL,
  &google_protobuf_FieldOptions_EditionDefault__fields[0],
  UPB_SIZE(24, 40), 2, kUpb_ExtMode_NonExtendable, 2, UPB_FASTTABLE_MASK(24), 0,
  UPB_FASTTABLE_INIT({
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x000800000100000a, &upb_pss_1bt},
    {0x0018000002000012, &upb_pss_1bt},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
  })
};

static const upb_MiniTableSub google_protobuf_OneofOptions_submsgs[2] = {
  {.submsg = &google_protobuf_FeatureSet_msg_init},
  {.submsg = &google_protobuf_UninterpretedOption_msg_init},
};

static const upb_MiniTableField google_protobuf_OneofOptions__fields[2] = {
  {1, UPB_SIZE(4, 8), 1, 0, 11, (int)kUpb_FieldMode_Scalar | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {999, UPB_SIZE(8, 16), 0, 1, 11, (int)kUpb_FieldMode_Array | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_OneofOptions_msg_init = {
  &google_protobuf_OneofOptions_submsgs[0],
  &google_protobuf_OneofOptions__fields[0],
  UPB_SIZE(16, 24), 2, kUpb_ExtMode_Extendable, 1, UPB_FASTTABLE_MASK(248), 0,
  UPB_FASTTABLE_INIT({
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x000800000100000a, &upb_psm_1bt_max64b},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x001000003f013eba, &upb_prm_2bt_max128b},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
  })
};

static const upb_MiniTableSub google_protobuf_EnumOptions_submsgs[2] = {
  {.submsg = &google_protobuf_FeatureSet_msg_init},
  {.submsg = &google_protobuf_UninterpretedOption_msg_init},
};

static const upb_MiniTableField google_protobuf_EnumOptions__fields[5] = {
  {2, 1, 1, kUpb_NoSub, 8, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
  {3, 2, 2, kUpb_NoSub, 8, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
  {6, 3, 3, kUpb_NoSub, 8, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
  {7, UPB_SIZE(4, 8), 4, 0, 11, (int)kUpb_FieldMode_Scalar | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {999, UPB_SIZE(8, 16), 0, 1, 11, (int)kUpb_FieldMode_Array | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_EnumOptions_msg_init = {
  &google_protobuf_EnumOptions_submsgs[0],
  &google_protobuf_EnumOptions__fields[0],
  UPB_SIZE(16, 24), 5, kUpb_ExtMode_Extendable, 0, UPB_FASTTABLE_MASK(248), 0,
  UPB_FASTTABLE_INIT({
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0001000001000010, &upb_psb1_1bt},
    {0x0002000002000018, &upb_psb1_1bt},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0003000003000030, &upb_psb1_1bt},
    {0x000800000400003a, &upb_psm_1bt_max64b},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x001000003f013eba, &upb_prm_2bt_max128b},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
  })
};

static const upb_MiniTableSub google_protobuf_EnumValueOptions_submsgs[2] = {
  {.submsg = &google_protobuf_FeatureSet_msg_init},
  {.submsg = &google_protobuf_UninterpretedOption_msg_init},
};

static const upb_MiniTableField google_protobuf_EnumValueOptions__fields[4] = {
  {1, 1, 1, kUpb_NoSub, 8, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
  {2, UPB_SIZE(4, 8), 2, 0, 11, (int)kUpb_FieldMode_Scalar | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {3, UPB_SIZE(8, 2), 3, kUpb_NoSub, 8, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
  {999, UPB_SIZE(12, 16), 0, 1, 11, (int)kUpb_FieldMode_Array | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_EnumValueOptions_msg_init = {
  &google_protobuf_EnumValueOptions_submsgs[0],
  &google_protobuf_EnumValueOptions__fields[0],
  UPB_SIZE(16, 24), 4, kUpb_ExtMode_Extendable, 3, UPB_FASTTABLE_MASK(248), 0,
  UPB_FASTTABLE_INIT({
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0001000001000008, &upb_psb1_1bt},
    {0x0008000002000012, &upb_psm_1bt_max64b},
    {0x0002000003000018, &upb_psb1_1bt},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x001000003f013eba, &upb_prm_2bt_max128b},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
  })
};

static const upb_MiniTableSub google_protobuf_ServiceOptions_submsgs[2] = {
  {.submsg = &google_protobuf_FeatureSet_msg_init},
  {.submsg = &google_protobuf_UninterpretedOption_msg_init},
};

static const upb_MiniTableField google_protobuf_ServiceOptions__fields[3] = {
  {33, 1, 1, kUpb_NoSub, 8, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
  {34, UPB_SIZE(4, 8), 2, 0, 11, (int)kUpb_FieldMode_Scalar | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {999, UPB_SIZE(8, 16), 0, 1, 11, (int)kUpb_FieldMode_Array | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_ServiceOptions_msg_init = {
  &google_protobuf_ServiceOptions_submsgs[0],
  &google_protobuf_ServiceOptions__fields[0],
  UPB_SIZE(16, 24), 3, kUpb_ExtMode_Extendable, 0, UPB_FASTTABLE_MASK(248), 0,
  UPB_FASTTABLE_INIT({
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0001000001000288, &upb_psb1_2bt},
    {0x0008000002000292, &upb_psm_2bt_max64b},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x001000003f013eba, &upb_prm_2bt_max128b},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
  })
};

static const upb_MiniTableSub google_protobuf_MethodOptions_submsgs[3] = {
  {.submsg = &google_protobuf_FeatureSet_msg_init},
  {.submsg = &google_protobuf_UninterpretedOption_msg_init},
  {.subenum = &google_protobuf_MethodOptions_IdempotencyLevel_enum_init},
};

static const upb_MiniTableField google_protobuf_MethodOptions__fields[4] = {
  {33, 1, 1, kUpb_NoSub, 8, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
  {34, 4, 2, 2, 14, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)},
  {35, 8, 3, 0, 11, (int)kUpb_FieldMode_Scalar | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {999, UPB_SIZE(12, 16), 0, 1, 11, (int)kUpb_FieldMode_Array | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_MethodOptions_msg_init = {
  &google_protobuf_MethodOptions_submsgs[0],
  &google_protobuf_MethodOptions__fields[0],
  UPB_SIZE(16, 24), 4, kUpb_ExtMode_Extendable, 0, UPB_FASTTABLE_MASK(248), 0,
  UPB_FASTTABLE_INIT({
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0001000001000288, &upb_psb1_2bt},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x000800000300029a, &upb_psm_2bt_max64b},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x001000003f013eba, &upb_prm_2bt_max128b},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
  })
};

static const upb_MiniTableSub google_protobuf_UninterpretedOption_submsgs[1] = {
  {.submsg = &google_protobuf_UninterpretedOption_NamePart_msg_init},
};

static const upb_MiniTableField google_protobuf_UninterpretedOption__fields[7] = {
  {2, UPB_SIZE(4, 8), 0, 0, 11, (int)kUpb_FieldMode_Array | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {3, UPB_SIZE(8, 16), 1, kUpb_NoSub, 12, (int)kUpb_FieldMode_Scalar | (int)kUpb_LabelFlags_IsAlternate | ((int)kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {4, UPB_SIZE(16, 32), 2, kUpb_NoSub, 4, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_8Byte << kUpb_FieldRep_Shift)},
  {5, UPB_SIZE(24, 40), 3, kUpb_NoSub, 3, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_8Byte << kUpb_FieldRep_Shift)},
  {6, UPB_SIZE(32, 48), 4, kUpb_NoSub, 1, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_8Byte << kUpb_FieldRep_Shift)},
  {7, UPB_SIZE(40, 56), 5, kUpb_NoSub, 12, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {8, UPB_SIZE(48, 72), 6, kUpb_NoSub, 12, (int)kUpb_FieldMode_Scalar | (int)kUpb_LabelFlags_IsAlternate | ((int)kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_UninterpretedOption_msg_init = {
  &google_protobuf_UninterpretedOption_submsgs[0],
  &google_protobuf_UninterpretedOption__fields[0],
  UPB_SIZE(56, 88), 7, kUpb_ExtMode_NonExtendable, 0, UPB_FASTTABLE_MASK(120), 0,
  UPB_FASTTABLE_INIT({
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x000800003f000012, &upb_prm_1bt_max64b},
    {0x001000000100001a, &upb_pss_1bt},
    {0x0020000002000020, &upb_psv8_1bt},
    {0x0028000003000028, &upb_psv8_1bt},
    {0x0030000004000031, &upb_psf8_1bt},
    {0x003800000500003a, &upb_psb_1bt},
    {0x0048000006000042, &upb_pss_1bt},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
  })
};

static const upb_MiniTableField google_protobuf_UninterpretedOption_NamePart__fields[2] = {
  {1, UPB_SIZE(4, 8), 1, kUpb_NoSub, 12, (int)kUpb_FieldMode_Scalar | (int)kUpb_LabelFlags_IsAlternate | ((int)kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {2, 1, 2, kUpb_NoSub, 8, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_1Byte << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_UninterpretedOption_NamePart_msg_init = {
  NULL,
  &google_protobuf_UninterpretedOption_NamePart__fields[0],
  UPB_SIZE(16, 24), 2, kUpb_ExtMode_NonExtendable, 2, UPB_FASTTABLE_MASK(24), 2,
  UPB_FASTTABLE_INIT({
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x000800000100000a, &upb_pss_1bt},
    {0x0001000002000010, &upb_psb1_1bt},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
  })
};

static const upb_MiniTableSub google_protobuf_FeatureSet_submsgs[7] = {
  {.submsg = &google_protobuf_FeatureSet_msg_init},
  {.subenum = &google_protobuf_FeatureSet_FieldPresence_enum_init},
  {.subenum = &google_protobuf_FeatureSet_EnumType_enum_init},
  {.subenum = &google_protobuf_FeatureSet_RepeatedFieldEncoding_enum_init},
  {.subenum = &google_protobuf_FeatureSet_StringFieldValidation_enum_init},
  {.subenum = &google_protobuf_FeatureSet_MessageEncoding_enum_init},
  {.subenum = &google_protobuf_FeatureSet_JsonFormat_enum_init},
};

static const upb_MiniTableField google_protobuf_FeatureSet__fields[7] = {
  {1, 4, 1, 1, 14, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)},
  {2, 8, 2, 2, 14, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)},
  {3, 12, 3, 3, 14, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)},
  {4, 16, 4, 4, 14, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)},
  {5, 20, 5, 5, 14, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)},
  {6, 24, 6, 6, 14, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)},
  {999, UPB_SIZE(28, 32), 7, 0, 11, (int)kUpb_FieldMode_Scalar | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_FeatureSet_msg_init = {
  &google_protobuf_FeatureSet_submsgs[0],
  &google_protobuf_FeatureSet__fields[0],
  UPB_SIZE(32, 40), 7, kUpb_ExtMode_Extendable, 6, UPB_FASTTABLE_MASK(248), 0,
  UPB_FASTTABLE_INIT({
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0020000007003eba, &upb_psm_2bt_max64b},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
  })
};

static const upb_MiniTableSub google_protobuf_SourceCodeInfo_submsgs[1] = {
  {.submsg = &google_protobuf_SourceCodeInfo_Location_msg_init},
};

static const upb_MiniTableField google_protobuf_SourceCodeInfo__fields[1] = {
  {1, 0, 0, 0, 11, (int)kUpb_FieldMode_Array | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_SourceCodeInfo_msg_init = {
  &google_protobuf_SourceCodeInfo_submsgs[0],
  &google_protobuf_SourceCodeInfo__fields[0],
  8, 1, kUpb_ExtMode_NonExtendable, 1, UPB_FASTTABLE_MASK(8), 0,
  UPB_FASTTABLE_INIT({
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x000000003f00000a, &upb_prm_1bt_max128b},
  })
};

static const upb_MiniTableField google_protobuf_SourceCodeInfo_Location__fields[5] = {
  {1, UPB_SIZE(4, 8), 0, kUpb_NoSub, 5, (int)kUpb_FieldMode_Array | (int)kUpb_LabelFlags_IsPacked | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {2, UPB_SIZE(8, 16), 0, kUpb_NoSub, 5, (int)kUpb_FieldMode_Array | (int)kUpb_LabelFlags_IsPacked | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {3, UPB_SIZE(16, 24), 1, kUpb_NoSub, 12, (int)kUpb_FieldMode_Scalar | (int)kUpb_LabelFlags_IsAlternate | ((int)kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {4, UPB_SIZE(24, 40), 2, kUpb_NoSub, 12, (int)kUpb_FieldMode_Scalar | (int)kUpb_LabelFlags_IsAlternate | ((int)kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {6, UPB_SIZE(12, 56), 0, kUpb_NoSub, 12, (int)kUpb_FieldMode_Array | (int)kUpb_LabelFlags_IsAlternate | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_SourceCodeInfo_Location_msg_init = {
  NULL,
  &google_protobuf_SourceCodeInfo_Location__fields[0],
  UPB_SIZE(32, 64), 5, kUpb_ExtMode_NonExtendable, 4, UPB_FASTTABLE_MASK(56), 0,
  UPB_FASTTABLE_INIT({
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x000800003f00000a, &upb_ppv4_1bt},
    {0x001000003f000012, &upb_ppv4_1bt},
    {0x001800000100001a, &upb_pss_1bt},
    {0x0028000002000022, &upb_pss_1bt},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x003800003f000032, &upb_prs_1bt},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
  })
};

static const upb_MiniTableSub google_protobuf_GeneratedCodeInfo_submsgs[1] = {
  {.submsg = &google_protobuf_GeneratedCodeInfo_Annotation_msg_init},
};

static const upb_MiniTableField google_protobuf_GeneratedCodeInfo__fields[1] = {
  {1, 0, 0, 0, 11, (int)kUpb_FieldMode_Array | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_GeneratedCodeInfo_msg_init = {
  &google_protobuf_GeneratedCodeInfo_submsgs[0],
  &google_protobuf_GeneratedCodeInfo__fields[0],
  8, 1, kUpb_ExtMode_NonExtendable, 1, UPB_FASTTABLE_MASK(8), 0,
  UPB_FASTTABLE_INIT({
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x000000003f00000a, &upb_prm_1bt_max64b},
  })
};

static const upb_MiniTableSub google_protobuf_GeneratedCodeInfo_Annotation_submsgs[1] = {
  {.subenum = &google_protobuf_GeneratedCodeInfo_Annotation_Semantic_enum_init},
};

static const upb_MiniTableField google_protobuf_GeneratedCodeInfo_Annotation__fields[5] = {
  {1, UPB_SIZE(4, 16), 0, kUpb_NoSub, 5, (int)kUpb_FieldMode_Array | (int)kUpb_LabelFlags_IsPacked | ((int)UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte) << kUpb_FieldRep_Shift)},
  {2, UPB_SIZE(20, 24), 1, kUpb_NoSub, 12, (int)kUpb_FieldMode_Scalar | (int)kUpb_LabelFlags_IsAlternate | ((int)kUpb_FieldRep_StringView << kUpb_FieldRep_Shift)},
  {3, UPB_SIZE(8, 4), 2, kUpb_NoSub, 5, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)},
  {4, UPB_SIZE(12, 8), 3, kUpb_NoSub, 5, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)},
  {5, UPB_SIZE(16, 12), 4, 0, 14, (int)kUpb_FieldMode_Scalar | ((int)kUpb_FieldRep_4Byte << kUpb_FieldRep_Shift)},
};

const upb_MiniTable google_protobuf_GeneratedCodeInfo_Annotation_msg_init = {
  &google_protobuf_GeneratedCodeInfo_Annotation_submsgs[0],
  &google_protobuf_GeneratedCodeInfo_Annotation__fields[0],
  UPB_SIZE(32, 40), 5, kUpb_ExtMode_NonExtendable, 5, UPB_FASTTABLE_MASK(56), 0,
  UPB_FASTTABLE_INIT({
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x001000003f00000a, &upb_ppv4_1bt},
    {0x0018000001000012, &upb_pss_1bt},
    {0x0004000002000018, &upb_psv4_1bt},
    {0x0008000003000020, &upb_psv4_1bt},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
    {0x0000000000000000, &_upb_FastDecoder_DecodeGeneric},
  })
};

static const upb_MiniTable *messages_layout[30] = {
  &google_protobuf_FileDescriptorSet_msg_init,
  &google_protobuf_FileDescriptorProto_msg_init,
  &google_protobuf_DescriptorProto_msg_init,
  &google_protobuf_DescriptorProto_ExtensionRange_msg_init,
  &google_protobuf_DescriptorProto_ReservedRange_msg_init,
  &google_protobuf_ExtensionRangeOptions_msg_init,
  &google_protobuf_ExtensionRangeOptions_Declaration_msg_init,
  &google_protobuf_FieldDescriptorProto_msg_init,
  &google_protobuf_OneofDescriptorProto_msg_init,
  &google_protobuf_EnumDescriptorProto_msg_init,
  &google_protobuf_EnumDescriptorProto_EnumReservedRange_msg_init,
  &google_protobuf_EnumValueDescriptorProto_msg_init,
  &google_protobuf_ServiceDescriptorProto_msg_init,
  &google_protobuf_MethodDescriptorProto_msg_init,
  &google_protobuf_FileOptions_msg_init,
  &google_protobuf_MessageOptions_msg_init,
  &google_protobuf_FieldOptions_msg_init,
  &google_protobuf_FieldOptions_EditionDefault_msg_init,
  &google_protobuf_OneofOptions_msg_init,
  &google_protobuf_EnumOptions_msg_init,
  &google_protobuf_EnumValueOptions_msg_init,
  &google_protobuf_ServiceOptions_msg_init,
  &google_protobuf_MethodOptions_msg_init,
  &google_protobuf_UninterpretedOption_msg_init,
  &google_protobuf_UninterpretedOption_NamePart_msg_init,
  &google_protobuf_FeatureSet_msg_init,
  &google_protobuf_SourceCodeInfo_msg_init,
  &google_protobuf_SourceCodeInfo_Location_msg_init,
  &google_protobuf_GeneratedCodeInfo_msg_init,
  &google_protobuf_GeneratedCodeInfo_Annotation_msg_init,
};

const upb_MiniTableEnum google_protobuf_ExtensionRangeOptions_VerificationState_enum_init = {
    64,
    0,
    {
        0x3,
        0x0,
    },
};

const upb_MiniTableEnum google_protobuf_FeatureSet_EnumType_enum_init = {
    64,
    0,
    {
        0x7,
        0x0,
    },
};

const upb_MiniTableEnum google_protobuf_FeatureSet_FieldPresence_enum_init = {
    64,
    0,
    {
        0xf,
        0x0,
    },
};

const upb_MiniTableEnum google_protobuf_FeatureSet_JsonFormat_enum_init = {
    64,
    0,
    {
        0x7,
        0x0,
    },
};

const upb_MiniTableEnum google_protobuf_FeatureSet_MessageEncoding_enum_init = {
    64,
    0,
    {
        0x7,
        0x0,
    },
};

const upb_MiniTableEnum google_protobuf_FeatureSet_RepeatedFieldEncoding_enum_init = {
    64,
    0,
    {
        0x7,
        0x0,
    },
};

const upb_MiniTableEnum google_protobuf_FeatureSet_StringFieldValidation_enum_init = {
    64,
    0,
    {
        0xf,
        0x0,
    },
};

const upb_MiniTableEnum google_protobuf_FieldDescriptorProto_Label_enum_init = {
    64,
    0,
    {
        0xe,
        0x0,
    },
};

const upb_MiniTableEnum google_protobuf_FieldDescriptorProto_Type_enum_init = {
    64,
    0,
    {
        0x7fffe,
        0x0,
    },
};

const upb_MiniTableEnum google_protobuf_FieldOptions_CType_enum_init = {
    64,
    0,
    {
        0x7,
        0x0,
    },
};

const upb_MiniTableEnum google_protobuf_FieldOptions_JSType_enum_init = {
    64,
    0,
    {
        0x7,
        0x0,
    },
};

const upb_MiniTableEnum google_protobuf_FieldOptions_OptionRetention_enum_init = {
    64,
    0,
    {
        0x7,
        0x0,
    },
};

const upb_MiniTableEnum google_protobuf_FieldOptions_OptionTargetType_enum_init = {
    64,
    0,
    {
        0x3ff,
        0x0,
    },
};

const upb_MiniTableEnum google_protobuf_FileOptions_OptimizeMode_enum_init = {
    64,
    0,
    {
        0xe,
        0x0,
    },
};

const upb_MiniTableEnum google_protobuf_GeneratedCodeInfo_Annotation_Semantic_enum_init = {
    64,
    0,
    {
        0x7,
        0x0,
    },
};

const upb_MiniTableEnum google_protobuf_MethodOptions_IdempotencyLevel_enum_init = {
    64,
    0,
    {
        0x7,
        0x0,
    },
};

static const upb_MiniTableEnum *enums_layout[16] = {
  &google_protobuf_ExtensionRangeOptions_VerificationState_enum_init,
  &google_protobuf_FeatureSet_EnumType_enum_init,
  &google_protobuf_FeatureSet_FieldPresence_enum_init,
  &google_protobuf_FeatureSet_JsonFormat_enum_init,
  &google_protobuf_FeatureSet_MessageEncoding_enum_init,
  &google_protobuf_FeatureSet_RepeatedFieldEncoding_enum_init,
  &google_protobuf_FeatureSet_StringFieldValidation_enum_init,
  &google_protobuf_FieldDescriptorProto_Label_enum_init,
  &google_protobuf_FieldDescriptorProto_Type_enum_init,
  &google_protobuf_FieldOptions_CType_enum_init,
  &google_protobuf_FieldOptions_JSType_enum_init,
  &google_protobuf_FieldOptions_OptionRetention_enum_init,
  &google_protobuf_FieldOptions_OptionTargetType_enum_init,
  &google_protobuf_FileOptions_OptimizeMode_enum_init,
  &google_protobuf_GeneratedCodeInfo_Annotation_Semantic_enum_init,
  &google_protobuf_MethodOptions_IdempotencyLevel_enum_init,
};

const upb_MiniTableFile google_protobuf_descriptor_proto_upb_file_layout = {
  messages_layout,
  enums_layout,
  NULL,
  30,
  16,
  0,
};




static const char* _upb_EpsCopyInputStream_NoOpCallback(
    upb_EpsCopyInputStream* e, const char* old_end, const char* new_start) {
  return new_start;
}

const char* _upb_EpsCopyInputStream_IsDoneFallbackNoCallback(
    upb_EpsCopyInputStream* e, const char* ptr, int overrun) {
  return _upb_EpsCopyInputStream_IsDoneFallbackInline(
      e, ptr, overrun, _upb_EpsCopyInputStream_NoOpCallback);
}

/*
 * upb_table Implementation
 *
 * Implementation is heavily inspired by Lua's ltable.c.
 */

#include <string.h>


// Must be last.

#define UPB_MAXARRSIZE 16  // 2**16 = 64k.

// From Chromium.
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
  ret = pow2 ? ret : ret + 1;  // Ceiling.
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
    if (len != 0) memcpy(p, s, len);
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
  int size_lg2 = upb_Log2Ceiling(need_entries);
  return init(&t->t, size_lg2, a);
}

void upb_strtable_clear(upb_strtable* t) {
  size_t bytes = upb_table_size(&t->t) * sizeof(upb_tabent);
  t->t.count = 0;
  memset((char*)t->t.entries, 0, bytes);
}

bool upb_strtable_resize(upb_strtable* t, size_t size_lg2, upb_Arena* a) {
  upb_strtable new_table;
  if (!init(&new_table.t, size_lg2, a)) return false;

  intptr_t iter = UPB_STRTABLE_BEGIN;
  upb_StringView key;
  upb_value val;
  while (upb_strtable_next2(t, &key, &val, &iter)) {
    upb_strtable_insert(&new_table, key.data, key.size, val, a);
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
    // This check is very expensive (makes inserts/deletes O(N)).
    size_t count = 0;
    intptr_t iter = UPB_INTTABLE_BEGIN;
    uintptr_t key;
    upb_value val;
    while (upb_inttable_next(t, &key, &val, &iter)) {
      UPB_ASSERT(upb_inttable_lookup(t, key, NULL));
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

  {
    intptr_t iter = UPB_INTTABLE_BEGIN;
    uintptr_t key;
    upb_value val;
    while (upb_inttable_next(t, &key, &val, &iter)) {
      int bucket = log2ceil(key);
      max[bucket] = UPB_MAX(max[bucket], key);
      counts[bucket]++;
    }
  }

  /* Find the largest power of two that satisfies the MIN_DENSITY
   * definition (while actually having some keys). */
  size_t arr_count = upb_inttable_count(t);
  int size_lg2;
  upb_inttable new_t;

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

    {
      intptr_t iter = UPB_INTTABLE_BEGIN;
      uintptr_t key;
      upb_value val;
      while (upb_inttable_next(t, &key, &val, &iter)) {
        upb_inttable_insert(&new_t, key, val, a);
      }
    }

    UPB_ASSERT(new_t.array_size == arr_size);
    UPB_ASSERT(new_t.t.size_lg2 == hashsize_lg2);
  }
  *t = new_t;
}

// Iteration.

bool upb_inttable_next(const upb_inttable* t, uintptr_t* key, upb_value* val,
                       intptr_t* iter) {
  intptr_t i = *iter;
  if ((size_t)(i + 1) <= t->array_size) {
    while ((size_t)++i < t->array_size) {
      upb_tabval ent = t->array[i];
      if (upb_arrhas(ent)) {
        *key = i;
        *val = _upb_value_val(ent.val);
        *iter = i;
        return true;
      }
    }
    i--;  // Back up to exactly one position before the start of the table.
  }

  size_t tab_idx = next(&t->t, i - t->array_size);
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
  if ((size_t)i < t->array_size) {
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

void upb_strtable_setentryvalue(upb_strtable* t, intptr_t iter, upb_value v) {
  upb_tabent* ent = &t->t.entries[iter];
  ent->val.val = v.val;
}


#include <errno.h>
#include <float.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>


// Must be last.

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
  if (upb_Unicode_IsHigh(cp)) {
    /* Surrogate pair: two 16-bit codepoints become a 32-bit codepoint. */
    jsondec_parselit(d, "\\u");
    uint32_t low = jsondec_codepoint(d);
    if (!upb_Unicode_IsLow(low)) jsondec_err(d, "Invalid low surrogate");
    cp = upb_Unicode_FromPair(cp, low);
  } else if (upb_Unicode_IsLow(cp)) {
    jsondec_err(d, "Unpaired low surrogate");
  }

  /* Write to UTF-8 */
  int bytes = upb_Unicode_ToUTF8(cp, out);
  if (bytes == 0) jsondec_err(d, "Invalid codepoint");
  return bytes;
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
            /* Allow space for maximum-sized codepoint (4 bytes). */
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

static const char* jsondec_buftouint64(jsondec* d, const char* ptr,
                                       const char* end, uint64_t* val) {
  const char* out = upb_BufToUint64(ptr, end, val);
  if (!out) jsondec_err(d, "Integer overflow");
  return out;
}

static const char* jsondec_buftoint64(jsondec* d, const char* ptr,
                                      const char* end, int64_t* val,
                                      bool* is_neg) {
  const char* out = upb_BufToInt64(ptr, end, val, is_neg);
  if (!out) jsondec_err(d, "Integer overflow");
  return out;
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
  if (jsondec_buftoint64(d, str.data, end, &ret, NULL) != end) {
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
    float f = val.double_val;
    if (val.double_val != INFINITY && val.double_val != -INFINITY) {
      if (f == INFINITY || f == -INFINITY) jsondec_err(d, "Float out of range");
    }
    val.float_val = f;
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
  const upb_MiniTable* layout = upb_MessageDef_MiniTable(m);
  upb_Message* msg = upb_Message_New(layout, d->arena);
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
    upb_Message_SetFieldByDef(msg, f, val, d->arena);
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

  upb_Message_SetFieldByDef(msg, upb_MessageDef_FindFieldByNumber(m, 1),
                            seconds, d->arena);
  upb_Message_SetFieldByDef(msg, upb_MessageDef_FindFieldByNumber(m, 2), nanos,
                            d->arena);
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
  bool neg = false;

  /* "3.000000001s", "3s", etc. */
  ptr = jsondec_buftoint64(d, ptr, end, &seconds.int64_val, &neg);
  nanos.int32_val = jsondec_nanos(d, &ptr, end);

  if (end - ptr != 1 || *ptr != 's') {
    jsondec_err(d, "Malformed duration");
  }

  if (seconds.int64_val < -max || seconds.int64_val > max) {
    jsondec_err(d, "Duration out of range");
  }

  if (neg) {
    nanos.int32_val = -nanos.int32_val;
  }

  upb_Message_SetFieldByDef(msg, upb_MessageDef_FindFieldByNumber(m, 1),
                            seconds, d->arena);
  upb_Message_SetFieldByDef(msg, upb_MessageDef_FindFieldByNumber(m, 2), nanos,
                            d->arena);
}

static void jsondec_listvalue(jsondec* d, upb_Message* msg,
                              const upb_MessageDef* m) {
  const upb_FieldDef* values_f = upb_MessageDef_FindFieldByNumber(m, 1);
  const upb_MessageDef* value_m = upb_FieldDef_MessageSubDef(values_f);
  const upb_MiniTable* value_layout = upb_MessageDef_MiniTable(value_m);
  upb_Array* values = upb_Message_Mutable(msg, values_f, d->arena).array;

  jsondec_arrstart(d);
  while (jsondec_arrnext(d)) {
    upb_Message* value_msg = upb_Message_New(value_layout, d->arena);
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
  const upb_MiniTable* value_layout = upb_MessageDef_MiniTable(value_m);
  upb_Map* fields = upb_Message_Mutable(msg, fields_f, d->arena).map;

  jsondec_objstart(d);
  while (jsondec_objnext(d)) {
    upb_MessageValue key, value;
    upb_Message* value_msg = upb_Message_New(value_layout, d->arena);
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

  upb_Message_SetFieldByDef(msg, f, val, d->arena);
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
  upb_Message_SetFieldByDef(msg, type_url_f, val, d->arena);

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

  const upb_MiniTable* any_layout = upb_MessageDef_MiniTable(any_m);
  any_msg = upb_Message_New(any_layout, d->arena);

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

  upb_EncodeStatus status =
      upb_Encode(any_msg, upb_MessageDef_MiniTable(any_m), 0, d->arena,
                 (char**)&encoded.str_val.data, &encoded.str_val.size);
  // TODO(b/235839510): We should fail gracefully here on a bad return status.
  UPB_ASSERT(status == kUpb_EncodeStatus_Ok);
  upb_Message_SetFieldByDef(msg, value_f, encoded, d->arena);
}

static void jsondec_wrapper(jsondec* d, upb_Message* msg,
                            const upb_MessageDef* m) {
  const upb_FieldDef* value_f = upb_MessageDef_FindFieldByNumber(m, 1);
  upb_MessageValue val = jsondec_value(d, value_f);
  upb_Message_SetFieldByDef(msg, value_f, val, d->arena);
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

static bool upb_JsonDecoder_Decode(jsondec* const d, upb_Message* const msg,
                                   const upb_MessageDef* const m) {
  if (UPB_SETJMP(d->err)) return false;

  jsondec_tomsg(d, msg, m);
  return true;
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

  return upb_JsonDecoder_Decode(&d, msg, m);
}


#include <ctype.h>
#include <float.h>
#include <inttypes.h>
#include <math.h>
#include <stdarg.h>
#include <string.h>


// Must be last.

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
  int64_t seconds = upb_Message_GetFieldByDef(msg, seconds_f).int64_val;
  int32_t nanos = upb_Message_GetFieldByDef(msg, nanos_f).int32_val;
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
  int64_t seconds = upb_Message_GetFieldByDef(msg, seconds_f).int64_val;
  int32_t nanos = upb_Message_GetFieldByDef(msg, nanos_f).int32_val;
  bool negative = false;

  if (seconds > 315576000000 || seconds < -315576000000 ||
      (seconds != 0 && nanos != 0 && (seconds < 0) != (nanos < 0))) {
    jsonenc_err(e, "bad duration");
  }

  if (seconds < 0) {
    negative = true;
    seconds = -seconds;
  }
  if (nanos < 0) {
    negative = true;
    nanos = -nanos;
  }

  jsonenc_putstr(e, "\"");
  if (negative) {
    jsonenc_putstr(e, "-");
  }
  jsonenc_printf(e, "%" PRId64, seconds);
  jsonenc_nanos(e, nanos);
  jsonenc_putstr(e, "s\"");
}

static void jsonenc_enum(int32_t val, const upb_FieldDef* f, jsonenc* e) {
  const upb_EnumDef* e_def = upb_FieldDef_EnumSubDef(f);

  if (strcmp(upb_EnumDef_FullName(e_def), "google.protobuf.NullValue") == 0) {
    jsonenc_putstr(e, "null");
  } else {
    const upb_EnumValueDef* ev =
        (e->options & upb_JsonEncode_FormatEnumsAsIntegers)
            ? NULL
            : upb_EnumDef_FindValueByNumber(e_def, val);

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
  upb_MessageValue val = upb_Message_GetFieldByDef(msg, val_f);
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
  upb_StringView type_url = upb_Message_GetFieldByDef(msg, type_url_f).str_val;
  upb_StringView value = upb_Message_GetFieldByDef(msg, value_f).str_val;
  const upb_MessageDef* any_m = jsonenc_getanymsg(e, type_url);
  const upb_MiniTable* any_layout = upb_MessageDef_MiniTable(any_m);
  upb_Arena* arena = jsonenc_arena(e);
  upb_Message* any = upb_Message_New(any_layout, arena);

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
  const upb_Array* paths = upb_Message_GetFieldByDef(msg, paths_f).array_val;
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
  jsonenc_putstr(e, "{");

  const upb_FieldDef* fields_f = upb_MessageDef_FindFieldByNumber(m, 1);
  const upb_Map* fields = upb_Message_GetFieldByDef(msg, fields_f).map_val;

  if (fields) {
    const upb_MessageDef* entry_m = upb_FieldDef_MessageSubDef(fields_f);
    const upb_FieldDef* value_f = upb_MessageDef_FindFieldByNumber(entry_m, 2);

    size_t iter = kUpb_Map_Begin;
    bool first = true;

    upb_MessageValue key, val;
    while (upb_Map_Next(fields, &key, &val, &iter)) {
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
  const upb_Array* values = upb_Message_GetFieldByDef(msg, values_f).array_val;
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
      if (upb_JsonEncode_HandleSpecialDoubles(e, val.double_val)) {
        jsonenc_err(
            e,
            "google.protobuf.Value cannot encode double values for "
            "infinity or nan, because they would be parsed as a string");
      }
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
  jsonenc_putstr(e, "{");

  const upb_MessageDef* entry = upb_FieldDef_MessageSubDef(f);
  const upb_FieldDef* key_f = upb_MessageDef_FindFieldByNumber(entry, 1);
  const upb_FieldDef* val_f = upb_MessageDef_FindFieldByNumber(entry, 2);

  if (map) {
    size_t iter = kUpb_Map_Begin;
    bool first = true;

    upb_MessageValue key, val;
    while (upb_Map_Next(map, &key, &val, &iter)) {
      jsonenc_putsep(e, ",", &first);
      jsonenc_mapkey(e, key, key_f);
      jsonenc_scalar(e, val, val_f);
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
      if (!upb_FieldDef_HasPresence(f) || upb_Message_HasFieldByDef(msg, f)) {
        jsonenc_fieldval(e, f, upb_Message_GetFieldByDef(msg, f), &first);
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

static size_t upb_JsonEncoder_Encode(jsonenc* const e,
                                     const upb_Message* const msg,
                                     const upb_MessageDef* const m,
                                     const size_t size) {
  if (UPB_SETJMP(e->err) != 0) return -1;

  jsonenc_msgfield(e, msg, m);
  if (e->arena) upb_Arena_Free(e->arena);
  return jsonenc_nullz(e, size);
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

  return upb_JsonEncoder_Encode(&e, msg, m, size);
}


// Must be last.

const char* upb_BufToUint64(const char* ptr, const char* end, uint64_t* val) {
  uint64_t u64 = 0;
  while (ptr < end) {
    unsigned ch = *ptr - '0';
    if (ch >= 10) break;
    if (u64 > UINT64_MAX / 10 || u64 * 10 > UINT64_MAX - ch) {
      return NULL;  // integer overflow
    }
    u64 *= 10;
    u64 += ch;
    ptr++;
  }

  *val = u64;
  return ptr;
}

const char* upb_BufToInt64(const char* ptr, const char* end, int64_t* val,
                           bool* is_neg) {
  bool neg = false;
  uint64_t u64;

  if (ptr != end && *ptr == '-') {
    ptr++;
    neg = true;
  }

  ptr = upb_BufToUint64(ptr, end, &u64);
  if (!ptr || u64 > (uint64_t)INT64_MAX + neg) {
    return NULL;  // integer overflow
  }

  *val = neg ? -u64 : u64;
  if (is_neg) *is_neg = neg;
  return ptr;
}


#include <float.h>
#include <stdlib.h>

// Must be last.

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


#include <stdlib.h>
#include <string.h>

// Must be last.

// Determine the locale-specific radix character by calling sprintf() to print
// the number 1.5, then stripping off the digits.  As far as I can tell, this
// is the only portable, thread-safe way to get the C library to divulge the
// locale's radix character.  No, localeconv() is NOT thread-safe.

static int GetLocaleRadix(char *data, size_t capacity) {
  char temp[16];
  const int size = snprintf(temp, sizeof(temp), "%.1f", 1.5);
  UPB_ASSERT(temp[0] == '1');
  UPB_ASSERT(temp[size - 1] == '5');
  UPB_ASSERT(size < capacity);
  temp[size - 1] = '\0';
  strcpy(data, temp + 1);
  return size - 2;
}

// Populates a string identical to *input except that the character pointed to
// by pos (which should be '.') is replaced with the locale-specific radix.

static void LocalizeRadix(const char *input, const char *pos, char *output) {
  const int len1 = pos - input;

  char radix[8];
  const int len2 = GetLocaleRadix(radix, sizeof(radix));

  memcpy(output, input, len1);
  memcpy(output + len1, radix, len2);
  strcpy(output + len1 + len2, input + len1 + 1);
}

double _upb_NoLocaleStrtod(const char *str, char **endptr) {
  // We cannot simply set the locale to "C" temporarily with setlocale()
  // as this is not thread-safe.  Instead, we try to parse in the current
  // locale first.  If parsing stops at a '.' character, then this is a
  // pretty good hint that we're actually in some other locale in which
  // '.' is not the radix character.

  char *temp_endptr;
  double result = strtod(str, &temp_endptr);
  if (endptr != NULL) *endptr = temp_endptr;
  if (*temp_endptr != '.') return result;

  // Parsing halted on a '.'.  Perhaps we're in a different locale?  Let's
  // try to replace the '.' with a locale-specific radix character and
  // try again.

  char localized[80];
  LocalizeRadix(str, temp_endptr, localized);
  char *localized_endptr;
  result = strtod(localized, &localized_endptr);
  if ((localized_endptr - &localized[0]) > (temp_endptr - str)) {
    // This attempt got further, so replacing the decimal must have helped.
    // Update endptr to point at the right location.
    if (endptr != NULL) {
      // size_diff is non-zero if the localized radix has multiple bytes.
      int size_diff = strlen(localized) - strlen(str);
      *endptr = (char *)str + (localized_endptr - &localized[0] - size_diff);
    }
  }

  return result;
}


// Must be last.

int upb_Unicode_ToUTF8(uint32_t cp, char* out) {
  if (cp <= 0x7f) {
    out[0] = cp;
    return 1;
  }
  if (cp <= 0x07ff) {
    out[0] = (cp >> 6) | 0xc0;
    out[1] = (cp & 0x3f) | 0x80;
    return 2;
  }
  if (cp <= 0xffff) {
    out[0] = (cp >> 12) | 0xe0;
    out[1] = ((cp >> 6) & 0x3f) | 0x80;
    out[2] = (cp & 0x3f) | 0x80;
    return 3;
  }
  if (cp <= 0x10ffff) {
    out[0] = (cp >> 18) | 0xf0;
    out[1] = ((cp >> 12) & 0x3f) | 0x80;
    out[2] = ((cp >> 6) & 0x3f) | 0x80;
    out[3] = (cp & 0x3f) | 0x80;
    return 4;
  }
  return 0;
}


#include <stdlib.h>

// Must be last.

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

upb_alloc upb_alloc_global = {&upb_global_allocfunc};


// Must be last.

struct _upb_MemBlock {
  // Atomic only for the benefit of SpaceAllocated().
  UPB_ATOMIC(_upb_MemBlock*) next;
  uint32_t size;
  // Data follows.
};

static const size_t memblock_reserve =
    UPB_ALIGN_UP(sizeof(_upb_MemBlock), UPB_MALLOC_ALIGN);

typedef struct _upb_ArenaRoot {
  upb_Arena* root;
  uintptr_t tagged_count;
} _upb_ArenaRoot;

static _upb_ArenaRoot _upb_Arena_FindRoot(upb_Arena* a) {
  uintptr_t poc = upb_Atomic_Load(&a->parent_or_count, memory_order_acquire);
  while (_upb_Arena_IsTaggedPointer(poc)) {
    upb_Arena* next = _upb_Arena_PointerFromTagged(poc);
    UPB_ASSERT(a != next);
    uintptr_t next_poc =
        upb_Atomic_Load(&next->parent_or_count, memory_order_acquire);

    if (_upb_Arena_IsTaggedPointer(next_poc)) {
      // To keep complexity down, we lazily collapse levels of the tree.  This
      // keeps it flat in the final case, but doesn't cost much incrementally.
      //
      // Path splitting keeps time complexity down, see:
      //   https://en.wikipedia.org/wiki/Disjoint-set_data_structure
      //
      // We can safely use a relaxed atomic here because all threads doing this
      // will converge on the same value and we don't need memory orderings to
      // be visible.
      //
      // This is true because:
      // - If no fuses occur, this will eventually become the root.
      // - If fuses are actively occuring, the root may change, but the
      //   invariant is that `parent_or_count` merely points to *a* parent.
      //
      // In other words, it is moving towards "the" root, and that root may move
      // further away over time, but the path towards that root will continue to
      // be valid and the creation of the path carries all the memory orderings
      // required.
      UPB_ASSERT(a != _upb_Arena_PointerFromTagged(next_poc));
      upb_Atomic_Store(&a->parent_or_count, next_poc, memory_order_relaxed);
    }
    a = next;
    poc = next_poc;
  }
  return (_upb_ArenaRoot){.root = a, .tagged_count = poc};
}

size_t upb_Arena_SpaceAllocated(upb_Arena* arena) {
  arena = _upb_Arena_FindRoot(arena).root;
  size_t memsize = 0;

  while (arena != NULL) {
    _upb_MemBlock* block =
        upb_Atomic_Load(&arena->blocks, memory_order_relaxed);
    while (block != NULL) {
      memsize += sizeof(_upb_MemBlock) + block->size;
      block = upb_Atomic_Load(&block->next, memory_order_relaxed);
    }
    arena = upb_Atomic_Load(&arena->next, memory_order_relaxed);
  }

  return memsize;
}

uint32_t upb_Arena_DebugRefCount(upb_Arena* a) {
  // These loads could probably be relaxed, but given that this is debug-only,
  // it's not worth introducing a new variant for it.
  uintptr_t poc = upb_Atomic_Load(&a->parent_or_count, memory_order_acquire);
  while (_upb_Arena_IsTaggedPointer(poc)) {
    a = _upb_Arena_PointerFromTagged(poc);
    poc = upb_Atomic_Load(&a->parent_or_count, memory_order_acquire);
  }
  return _upb_Arena_RefCountFromTagged(poc);
}

static void upb_Arena_AddBlock(upb_Arena* a, void* ptr, size_t size) {
  _upb_MemBlock* block = ptr;

  // Insert into linked list.
  block->size = (uint32_t)size;
  upb_Atomic_Init(&block->next, a->blocks);
  upb_Atomic_Store(&a->blocks, block, memory_order_release);

  a->head.ptr = UPB_PTR_AT(block, memblock_reserve, char);
  a->head.end = UPB_PTR_AT(block, size, char);

  UPB_POISON_MEMORY_REGION(a->head.ptr, a->head.end - a->head.ptr);
}

static bool upb_Arena_AllocBlock(upb_Arena* a, size_t size) {
  if (!a->block_alloc) return false;
  _upb_MemBlock* last_block = upb_Atomic_Load(&a->blocks, memory_order_acquire);
  size_t last_size = last_block != NULL ? last_block->size : 128;
  size_t block_size = UPB_MAX(size, last_size * 2) + memblock_reserve;
  _upb_MemBlock* block = upb_malloc(upb_Arena_BlockAlloc(a), block_size);

  if (!block) return false;
  upb_Arena_AddBlock(a, block, block_size);
  return true;
}

void* _upb_Arena_SlowMalloc(upb_Arena* a, size_t size) {
  if (!upb_Arena_AllocBlock(a, size)) return NULL; /* Out of memory. */
  UPB_ASSERT(_upb_ArenaHas(a) >= size);
  return upb_Arena_Malloc(a, size);
}

/* Public Arena API ***********************************************************/

static upb_Arena* upb_Arena_InitSlow(upb_alloc* alloc) {
  const size_t first_block_overhead = sizeof(upb_Arena) + memblock_reserve;
  upb_Arena* a;

  /* We need to malloc the initial block. */
  char* mem;
  size_t n = first_block_overhead + 256;
  if (!alloc || !(mem = upb_malloc(alloc, n))) {
    return NULL;
  }

  a = UPB_PTR_AT(mem, n - sizeof(*a), upb_Arena);
  n -= sizeof(*a);

  a->block_alloc = upb_Arena_MakeBlockAlloc(alloc, 0);
  upb_Atomic_Init(&a->parent_or_count, _upb_Arena_TaggedFromRefcount(1));
  upb_Atomic_Init(&a->next, NULL);
  upb_Atomic_Init(&a->tail, a);
  upb_Atomic_Init(&a->blocks, NULL);

  upb_Arena_AddBlock(a, mem, n);

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
    return upb_Arena_InitSlow(alloc);
  }

  a = UPB_PTR_AT(mem, n - sizeof(*a), upb_Arena);

  upb_Atomic_Init(&a->parent_or_count, _upb_Arena_TaggedFromRefcount(1));
  upb_Atomic_Init(&a->next, NULL);
  upb_Atomic_Init(&a->tail, a);
  upb_Atomic_Init(&a->blocks, NULL);
  a->block_alloc = upb_Arena_MakeBlockAlloc(alloc, 1);
  a->head.ptr = mem;
  a->head.end = UPB_PTR_AT(mem, n - sizeof(*a), char);

  return a;
}

static void arena_dofree(upb_Arena* a) {
  UPB_ASSERT(_upb_Arena_RefCountFromTagged(a->parent_or_count) == 1);

  while (a != NULL) {
    // Load first since arena itself is likely from one of its blocks.
    upb_Arena* next_arena =
        (upb_Arena*)upb_Atomic_Load(&a->next, memory_order_acquire);
    upb_alloc* block_alloc = upb_Arena_BlockAlloc(a);
    _upb_MemBlock* block = upb_Atomic_Load(&a->blocks, memory_order_acquire);
    while (block != NULL) {
      // Load first since we are deleting block.
      _upb_MemBlock* next_block =
          upb_Atomic_Load(&block->next, memory_order_acquire);
      upb_free(block_alloc, block);
      block = next_block;
    }
    a = next_arena;
  }
}

void upb_Arena_Free(upb_Arena* a) {
  uintptr_t poc = upb_Atomic_Load(&a->parent_or_count, memory_order_acquire);
retry:
  while (_upb_Arena_IsTaggedPointer(poc)) {
    a = _upb_Arena_PointerFromTagged(poc);
    poc = upb_Atomic_Load(&a->parent_or_count, memory_order_acquire);
  }

  // compare_exchange or fetch_sub are RMW operations, which are more
  // expensive then direct loads.  As an optimization, we only do RMW ops
  // when we need to update things for other threads to see.
  if (poc == _upb_Arena_TaggedFromRefcount(1)) {
    arena_dofree(a);
    return;
  }

  if (upb_Atomic_CompareExchangeWeak(
          &a->parent_or_count, &poc,
          _upb_Arena_TaggedFromRefcount(_upb_Arena_RefCountFromTagged(poc) - 1),
          memory_order_release, memory_order_acquire)) {
    // We were >1 and we decremented it successfully, so we are done.
    return;
  }

  // We failed our update, so someone has done something, retry the whole
  // process, but the failed exchange reloaded `poc` for us.
  goto retry;
}

static void _upb_Arena_DoFuseArenaLists(upb_Arena* const parent,
                                        upb_Arena* child) {
  upb_Arena* parent_tail = upb_Atomic_Load(&parent->tail, memory_order_relaxed);
  do {
    // Our tail might be stale, but it will always converge to the true tail.
    upb_Arena* parent_tail_next =
        upb_Atomic_Load(&parent_tail->next, memory_order_relaxed);
    while (parent_tail_next != NULL) {
      parent_tail = parent_tail_next;
      parent_tail_next =
          upb_Atomic_Load(&parent_tail->next, memory_order_relaxed);
    }

    upb_Arena* displaced =
        upb_Atomic_Exchange(&parent_tail->next, child, memory_order_relaxed);
    parent_tail = upb_Atomic_Load(&child->tail, memory_order_relaxed);

    // If we displaced something that got installed racily, we can simply
    // reinstall it on our new tail.
    child = displaced;
  } while (child != NULL);

  upb_Atomic_Store(&parent->tail, parent_tail, memory_order_relaxed);
}

static upb_Arena* _upb_Arena_DoFuse(upb_Arena* a1, upb_Arena* a2,
                                    uintptr_t* ref_delta) {
  // `parent_or_count` has two disctint modes
  // -  parent pointer mode
  // -  refcount mode
  //
  // In parent pointer mode, it may change what pointer it refers to in the
  // tree, but it will always approach a root.  Any operation that walks the
  // tree to the root may collapse levels of the tree concurrently.
  _upb_ArenaRoot r1 = _upb_Arena_FindRoot(a1);
  _upb_ArenaRoot r2 = _upb_Arena_FindRoot(a2);

  if (r1.root == r2.root) return r1.root;  // Already fused.

  // Avoid cycles by always fusing into the root with the lower address.
  if ((uintptr_t)r1.root > (uintptr_t)r2.root) {
    _upb_ArenaRoot tmp = r1;
    r1 = r2;
    r2 = tmp;
  }

  // The moment we install `r1` as the parent for `r2` all racing frees may
  // immediately begin decrementing `r1`'s refcount (including pending
  // increments to that refcount and their frees!).  We need to add `r2`'s refs
  // now, so that `r1` can withstand any unrefs that come from r2.
  //
  // Note that while it is possible for `r2`'s refcount to increase
  // asynchronously, we will not actually do the reparenting operation below
  // unless `r2`'s refcount is unchanged from when we read it.
  //
  // Note that we may have done this previously, either to this node or a
  // different node, during a previous and failed DoFuse() attempt. But we will
  // not lose track of these refs because we always add them to our overall
  // delta.
  uintptr_t r2_untagged_count = r2.tagged_count & ~1;
  uintptr_t with_r2_refs = r1.tagged_count + r2_untagged_count;
  if (!upb_Atomic_CompareExchangeStrong(
          &r1.root->parent_or_count, &r1.tagged_count, with_r2_refs,
          memory_order_release, memory_order_acquire)) {
    return NULL;
  }

  // Perform the actual fuse by removing the refs from `r2` and swapping in the
  // parent pointer.
  if (!upb_Atomic_CompareExchangeStrong(
          &r2.root->parent_or_count, &r2.tagged_count,
          _upb_Arena_TaggedFromPointer(r1.root), memory_order_release,
          memory_order_acquire)) {
    // We'll need to remove the excess refs we added to r1 previously.
    *ref_delta += r2_untagged_count;
    return NULL;
  }

  // Now that the fuse has been performed (and can no longer fail) we need to
  // append `r2` to `r1`'s linked list.
  _upb_Arena_DoFuseArenaLists(r1.root, r2.root);
  return r1.root;
}

static bool _upb_Arena_FixupRefs(upb_Arena* new_root, uintptr_t ref_delta) {
  if (ref_delta == 0) return true;  // No fixup required.
  uintptr_t poc =
      upb_Atomic_Load(&new_root->parent_or_count, memory_order_relaxed);
  if (_upb_Arena_IsTaggedPointer(poc)) return false;
  uintptr_t with_refs = poc - ref_delta;
  UPB_ASSERT(!_upb_Arena_IsTaggedPointer(with_refs));
  return upb_Atomic_CompareExchangeStrong(&new_root->parent_or_count, &poc,
                                          with_refs, memory_order_relaxed,
                                          memory_order_relaxed);
}

bool upb_Arena_Fuse(upb_Arena* a1, upb_Arena* a2) {
  if (a1 == a2) return true;  // trivial fuse

  // Do not fuse initial blocks since we cannot lifetime extend them.
  // Any other fuse scenario is allowed.
  if (upb_Arena_HasInitialBlock(a1) || upb_Arena_HasInitialBlock(a2)) {
    return false;
  }

  // The number of refs we ultimately need to transfer to the new root.
  uintptr_t ref_delta = 0;
  while (true) {
    upb_Arena* new_root = _upb_Arena_DoFuse(a1, a2, &ref_delta);
    if (new_root != NULL && _upb_Arena_FixupRefs(new_root, ref_delta)) {
      return true;
    }
  }
}



// Must be last.

upb_MapInsertStatus upb_Message_InsertMapEntry(upb_Map* map,
                                               const upb_MiniTable* mini_table,
                                               const upb_MiniTableField* field,
                                               upb_Message* map_entry_message,
                                               upb_Arena* arena) {
  const upb_MiniTable* map_entry_mini_table =
      mini_table->subs[field->UPB_PRIVATE(submsg_index)].submsg;
  UPB_ASSERT(map_entry_mini_table);
  UPB_ASSERT(map_entry_mini_table->field_count == 2);
  const upb_MiniTableField* map_entry_key_field =
      &map_entry_mini_table->fields[0];
  const upb_MiniTableField* map_entry_value_field =
      &map_entry_mini_table->fields[1];
  // Map key/value cannot have explicit defaults,
  // hence assuming a zero default is valid.
  upb_MessageValue default_val;
  memset(&default_val, 0, sizeof(upb_MessageValue));
  upb_MessageValue map_entry_key;
  upb_MessageValue map_entry_value;
  _upb_Message_GetField(map_entry_message, map_entry_key_field, &default_val,
                        &map_entry_key);
  _upb_Message_GetField(map_entry_message, map_entry_value_field, &default_val,
                        &map_entry_value);
  return upb_Map_Insert(map, map_entry_key, map_entry_value, arena);
}

bool upb_Message_IsExactlyEqual(const upb_Message* m1, const upb_Message* m2,
                                const upb_MiniTable* layout) {
  if (m1 == m2) return true;

  int opts = kUpb_EncodeOption_SkipUnknown | kUpb_EncodeOption_Deterministic;
  upb_Arena* a = upb_Arena_New();

  // Compare deterministically serialized payloads with no unknown fields.
  size_t size1, size2;
  char *data1, *data2;
  upb_EncodeStatus status1 = upb_Encode(m1, layout, opts, a, &data1, &size1);
  upb_EncodeStatus status2 = upb_Encode(m2, layout, opts, a, &data2, &size2);

  if (status1 != kUpb_EncodeStatus_Ok || status2 != kUpb_EncodeStatus_Ok) {
    // TODO(salo): How should we fail here? (In Ruby we throw an exception.)
    upb_Arena_Free(a);
    return false;
  }

  const bool ret = (size1 == size2) && (memcmp(data1, data2, size1) == 0);
  upb_Arena_Free(a);
  return ret;
}


#include <math.h>


// Must be last.

const float kUpb_FltInfinity = INFINITY;
const double kUpb_Infinity = INFINITY;
const double kUpb_NaN = NAN;

static const size_t overhead = sizeof(upb_Message_InternalData);

upb_Message* upb_Message_New(const upb_MiniTable* mini_table,
                             upb_Arena* arena) {
  return _upb_Message_New(mini_table, arena);
}

static bool realloc_internal(upb_Message* msg, size_t need, upb_Arena* arena) {
  upb_Message_Internal* in = upb_Message_Getinternal(msg);
  if (!in->internal) {
    /* No internal data, allocate from scratch. */
    size_t size = UPB_MAX(128, upb_Log2CeilingSize(need + overhead));
    upb_Message_InternalData* internal = upb_Arena_Malloc(arena, size);
    if (!internal) return false;
    internal->size = size;
    internal->unknown_end = overhead;
    internal->ext_begin = size;
    in->internal = internal;
  } else if (in->internal->ext_begin - in->internal->unknown_end < need) {
    /* Internal data is too small, reallocate. */
    size_t new_size = upb_Log2CeilingSize(in->internal->size + need);
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
  const upb_Message_Internal* in = upb_Message_Getinternal(msg);
  if (in->internal) {
    *len = in->internal->unknown_end - overhead;
    return (char*)(in->internal + 1);
  } else {
    *len = 0;
    return NULL;
  }
}

void upb_Message_DeleteUnknown(upb_Message* msg, const char* data, size_t len) {
  upb_Message_Internal* in = upb_Message_Getinternal(msg);
  const char* internal_unknown_end =
      UPB_PTR_AT(in->internal, in->internal->unknown_end, char);
#ifndef NDEBUG
  size_t full_unknown_size;
  const char* full_unknown = upb_Message_GetUnknown(msg, &full_unknown_size);
  UPB_ASSERT((uintptr_t)data >= (uintptr_t)full_unknown);
  UPB_ASSERT((uintptr_t)data < (uintptr_t)(full_unknown + full_unknown_size));
  UPB_ASSERT((uintptr_t)(data + len) > (uintptr_t)data);
  UPB_ASSERT((uintptr_t)(data + len) <= (uintptr_t)internal_unknown_end);
#endif
  if ((data + len) != internal_unknown_end) {
    memmove((char*)data, data + len, internal_unknown_end - data - len);
  }
  in->internal->unknown_end -= len;
}

const upb_Message_Extension* _upb_Message_Getexts(const upb_Message* msg,
                                                  size_t* count) {
  const upb_Message_Internal* in = upb_Message_Getinternal(msg);
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
    const upb_Message* msg, const upb_MiniTableExtension* e) {
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

upb_Message_Extension* _upb_Message_GetOrCreateExtension(
    upb_Message* msg, const upb_MiniTableExtension* e, upb_Arena* arena) {
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

#include <string.h>


// Must be last.

/* The upb core does not generally have a concept of default instances. However
 * for descriptor options we make an exception since the max size is known and
 * modest (<200 bytes). All types can share a default instance since it is
 * initialized to zeroes.
 *
 * We have to allocate an extra pointer for upb's internal metadata. */
static const char opt_default_buf[_UPB_MAXOPT_SIZE + sizeof(void*)] = {0};
const char* kUpbDefOptDefault = &opt_default_buf[sizeof(void*)];

const char* _upb_DefBuilder_FullToShort(const char* fullname) {
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

void _upb_DefBuilder_FailJmp(upb_DefBuilder* ctx) { UPB_LONGJMP(ctx->err, 1); }

void _upb_DefBuilder_Errf(upb_DefBuilder* ctx, const char* fmt, ...) {
  va_list argp;
  va_start(argp, fmt);
  upb_Status_VSetErrorFormat(ctx->status, fmt, argp);
  va_end(argp);
  _upb_DefBuilder_FailJmp(ctx);
}

void _upb_DefBuilder_OomErr(upb_DefBuilder* ctx) {
  upb_Status_SetErrorMessage(ctx->status, "out of memory");
  _upb_DefBuilder_FailJmp(ctx);
}

// Verify a relative identifier string. The loop is branchless for speed.
static void _upb_DefBuilder_CheckIdentNotFull(upb_DefBuilder* ctx,
                                              upb_StringView name) {
  bool good = name.size > 0;

  for (size_t i = 0; i < name.size; i++) {
    const char c = name.data[i];
    const char d = c | 0x20;  // force lowercase
    const bool is_alpha = (('a' <= d) & (d <= 'z')) | (c == '_');
    const bool is_numer = ('0' <= c) & (c <= '9') & (i != 0);

    good &= is_alpha | is_numer;
  }

  if (!good) _upb_DefBuilder_CheckIdentSlow(ctx, name, false);
}

const char* _upb_DefBuilder_MakeFullName(upb_DefBuilder* ctx,
                                         const char* prefix,
                                         upb_StringView name) {
  _upb_DefBuilder_CheckIdentNotFull(ctx, name);
  if (prefix) {
    // ret = prefix + '.' + name;
    size_t n = strlen(prefix);
    char* ret = _upb_DefBuilder_Alloc(ctx, n + name.size + 2);
    strcpy(ret, prefix);
    ret[n] = '.';
    memcpy(&ret[n + 1], name.data, name.size);
    ret[n + 1 + name.size] = '\0';
    return ret;
  } else {
    char* ret = upb_strdup2(name.data, name.size, ctx->arena);
    if (!ret) _upb_DefBuilder_OomErr(ctx);
    return ret;
  }
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

const void* _upb_DefBuilder_ResolveAny(upb_DefBuilder* ctx,
                                       const char* from_name_dbg,
                                       const char* base, upb_StringView sym,
                                       upb_deftype_t* type) {
  if (sym.size == 0) goto notfound;
  upb_value v;
  if (sym.data[0] == '.') {
    /* Symbols starting with '.' are absolute, so we do a single lookup.
     * Slice to omit the leading '.' */
    if (!_upb_DefPool_LookupSym(ctx->symtab, sym.data + 1, sym.size - 1, &v)) {
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
      if (_upb_DefPool_LookupSym(ctx->symtab, tmp, p - tmp, &v)) {
        break;
      }
      if (!remove_component(tmp, &baselen)) {
        free(tmp);
        goto notfound;
      }
    }
    free(tmp);
  }

  *type = _upb_DefType_Type(v);
  return _upb_DefType_Unpack(v, *type);

notfound:
  _upb_DefBuilder_Errf(ctx, "couldn't resolve name '" UPB_STRINGVIEW_FORMAT "'",
                       UPB_STRINGVIEW_ARGS(sym));
}

const void* _upb_DefBuilder_Resolve(upb_DefBuilder* ctx,
                                    const char* from_name_dbg, const char* base,
                                    upb_StringView sym, upb_deftype_t type) {
  upb_deftype_t found_type;
  const void* ret =
      _upb_DefBuilder_ResolveAny(ctx, from_name_dbg, base, sym, &found_type);
  if (ret && found_type != type) {
    _upb_DefBuilder_Errf(ctx,
                         "type mismatch when resolving %s: couldn't find "
                         "name " UPB_STRINGVIEW_FORMAT " with type=%d",
                         from_name_dbg, UPB_STRINGVIEW_ARGS(sym), (int)type);
  }
  return ret;
}

// Per ASCII this will lower-case a letter. If the result is a letter, the
// input was definitely a letter. If the output is not a letter, this may
// have transformed the character unpredictably.
static char upb_ascii_lower(char ch) { return ch | 0x20; }

// isalpha() etc. from <ctype.h> are locale-dependent, which we don't want.
static bool upb_isbetween(uint8_t c, uint8_t low, uint8_t high) {
  return low <= c && c <= high;
}

static bool upb_isletter(char c) {
  char lower = upb_ascii_lower(c);
  return upb_isbetween(lower, 'a', 'z') || c == '_';
}

static bool upb_isalphanum(char c) {
  return upb_isletter(c) || upb_isbetween(c, '0', '9');
}

static bool TryGetChar(const char** src, const char* end, char* ch) {
  if (*src == end) return false;
  *ch = **src;
  *src += 1;
  return true;
}

static int TryGetHexDigit(const char** src, const char* end) {
  char ch;
  if (!TryGetChar(src, end, &ch)) return -1;
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

static char upb_DefBuilder_ParseHexEscape(upb_DefBuilder* ctx,
                                          const upb_FieldDef* f,
                                          const char** src, const char* end) {
  int hex_digit = TryGetHexDigit(src, end);
  if (hex_digit < 0) {
    _upb_DefBuilder_Errf(
        ctx, "\\x must be followed by at least one hex digit (field='%s')",
        upb_FieldDef_FullName(f));
    return 0;
  }
  unsigned int ret = hex_digit;
  while ((hex_digit = TryGetHexDigit(src, end)) >= 0) {
    ret = (ret << 4) | hex_digit;
  }
  if (ret > 0xff) {
    _upb_DefBuilder_Errf(ctx, "Value of hex escape in field %s exceeds 8 bits",
                         upb_FieldDef_FullName(f));
    return 0;
  }
  return ret;
}

static char TryGetOctalDigit(const char** src, const char* end) {
  char ch;
  if (!TryGetChar(src, end, &ch)) return -1;
  if ('0' <= ch && ch <= '7') {
    return ch - '0';
  }
  *src -= 1;  // Char wasn't actually an octal digit.
  return -1;
}

static char upb_DefBuilder_ParseOctalEscape(upb_DefBuilder* ctx,
                                            const upb_FieldDef* f,
                                            const char** src, const char* end) {
  char ch = 0;
  for (int i = 0; i < 3; i++) {
    char digit;
    if ((digit = TryGetOctalDigit(src, end)) >= 0) {
      ch = (ch << 3) | digit;
    }
  }
  return ch;
}

char _upb_DefBuilder_ParseEscape(upb_DefBuilder* ctx, const upb_FieldDef* f,
                                 const char** src, const char* end) {
  char ch;
  if (!TryGetChar(src, end, &ch)) {
    _upb_DefBuilder_Errf(ctx, "unterminated escape sequence in field %s",
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
      return upb_DefBuilder_ParseHexEscape(ctx, f, src, end);
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
      *src -= 1;
      return upb_DefBuilder_ParseOctalEscape(ctx, f, src, end);
  }
  _upb_DefBuilder_Errf(ctx, "Unknown escape sequence: \\%c", ch);
}

void _upb_DefBuilder_CheckIdentSlow(upb_DefBuilder* ctx, upb_StringView name,
                                    bool full) {
  const char* str = name.data;
  const size_t len = name.size;
  bool start = true;
  for (size_t i = 0; i < len; i++) {
    const char c = str[i];
    if (c == '.') {
      if (start || !full) {
        _upb_DefBuilder_Errf(
            ctx, "invalid name: unexpected '.' (" UPB_STRINGVIEW_FORMAT ")",
            UPB_STRINGVIEW_ARGS(name));
      }
      start = true;
    } else if (start) {
      if (!upb_isletter(c)) {
        _upb_DefBuilder_Errf(ctx,
                             "invalid name: path components must start with a "
                             "letter (" UPB_STRINGVIEW_FORMAT ")",
                             UPB_STRINGVIEW_ARGS(name));
      }
      start = false;
    } else if (!upb_isalphanum(c)) {
      _upb_DefBuilder_Errf(
          ctx,
          "invalid name: non-alphanumeric character (" UPB_STRINGVIEW_FORMAT
          ")",
          UPB_STRINGVIEW_ARGS(name));
    }
  }
  if (start) {
    _upb_DefBuilder_Errf(ctx,
                         "invalid name: empty part (" UPB_STRINGVIEW_FORMAT ")",
                         UPB_STRINGVIEW_ARGS(name));
  }

  // We should never reach this point.
  UPB_ASSERT(false);
}


// Must be last.

struct upb_DefPool {
  upb_Arena* arena;
  upb_strtable syms;   // full_name -> packed def ptr
  upb_strtable files;  // file_name -> (upb_FileDef*)
  upb_inttable exts;   // (upb_MiniTableExtension*) -> (upb_FieldDef*)
  upb_ExtensionRegistry* extreg;
  upb_MiniTablePlatform platform;
  void* scratch_data;
  size_t scratch_size;
  size_t bytes_loaded;
};

void upb_DefPool_Free(upb_DefPool* s) {
  upb_Arena_Free(s->arena);
  upb_gfree(s->scratch_data);
  upb_gfree(s);
}

upb_DefPool* upb_DefPool_New(void) {
  upb_DefPool* s = upb_gmalloc(sizeof(*s));
  if (!s) return NULL;

  s->arena = upb_Arena_New();
  s->bytes_loaded = 0;

  s->scratch_size = 240;
  s->scratch_data = upb_gmalloc(s->scratch_size);
  if (!s->scratch_data) goto err;

  if (!upb_strtable_init(&s->syms, 32, s->arena)) goto err;
  if (!upb_strtable_init(&s->files, 4, s->arena)) goto err;
  if (!upb_inttable_init(&s->exts, s->arena)) goto err;

  s->extreg = upb_ExtensionRegistry_New(s->arena);
  if (!s->extreg) goto err;

  s->platform = kUpb_MiniTablePlatform_Native;

  return s;

err:
  upb_DefPool_Free(s);
  return NULL;
}

bool _upb_DefPool_InsertExt(upb_DefPool* s, const upb_MiniTableExtension* ext,
                            const upb_FieldDef* f) {
  return upb_inttable_insert(&s->exts, (uintptr_t)ext, upb_value_constptr(f),
                             s->arena);
}

bool _upb_DefPool_InsertSym(upb_DefPool* s, upb_StringView sym, upb_value v,
                            upb_Status* status) {
  // TODO: table should support an operation "tryinsert" to avoid the double
  // lookup.
  if (upb_strtable_lookup2(&s->syms, sym.data, sym.size, NULL)) {
    upb_Status_SetErrorFormat(status, "duplicate symbol '%s'", sym.data);
    return false;
  }
  if (!upb_strtable_insert(&s->syms, sym.data, sym.size, v, s->arena)) {
    upb_Status_SetErrorMessage(status, "out of memory");
    return false;
  }
  return true;
}

static const void* _upb_DefPool_Unpack(const upb_DefPool* s, const char* sym,
                                       size_t size, upb_deftype_t type) {
  upb_value v;
  return upb_strtable_lookup2(&s->syms, sym, size, &v)
             ? _upb_DefType_Unpack(v, type)
             : NULL;
}

bool _upb_DefPool_LookupSym(const upb_DefPool* s, const char* sym, size_t size,
                            upb_value* v) {
  return upb_strtable_lookup2(&s->syms, sym, size, v);
}

upb_ExtensionRegistry* _upb_DefPool_ExtReg(const upb_DefPool* s) {
  return s->extreg;
}

void** _upb_DefPool_ScratchData(const upb_DefPool* s) {
  return (void**)&s->scratch_data;
}

size_t* _upb_DefPool_ScratchSize(const upb_DefPool* s) {
  return (size_t*)&s->scratch_size;
}

void _upb_DefPool_SetPlatform(upb_DefPool* s, upb_MiniTablePlatform platform) {
  assert(upb_strtable_count(&s->files) == 0);
  s->platform = platform;
}

const upb_MessageDef* upb_DefPool_FindMessageByName(const upb_DefPool* s,
                                                    const char* sym) {
  return _upb_DefPool_Unpack(s, sym, strlen(sym), UPB_DEFTYPE_MSG);
}

const upb_MessageDef* upb_DefPool_FindMessageByNameWithSize(
    const upb_DefPool* s, const char* sym, size_t len) {
  return _upb_DefPool_Unpack(s, sym, len, UPB_DEFTYPE_MSG);
}

const upb_EnumDef* upb_DefPool_FindEnumByName(const upb_DefPool* s,
                                              const char* sym) {
  return _upb_DefPool_Unpack(s, sym, strlen(sym), UPB_DEFTYPE_ENUM);
}

const upb_EnumValueDef* upb_DefPool_FindEnumByNameval(const upb_DefPool* s,
                                                      const char* sym) {
  return _upb_DefPool_Unpack(s, sym, strlen(sym), UPB_DEFTYPE_ENUMVAL);
}

const upb_FileDef* upb_DefPool_FindFileByName(const upb_DefPool* s,
                                              const char* name) {
  upb_value v;
  return upb_strtable_lookup(&s->files, name, &v) ? upb_value_getconstptr(v)
                                                  : NULL;
}

const upb_FileDef* upb_DefPool_FindFileByNameWithSize(const upb_DefPool* s,
                                                      const char* name,
                                                      size_t len) {
  upb_value v;
  return upb_strtable_lookup2(&s->files, name, len, &v)
             ? upb_value_getconstptr(v)
             : NULL;
}

const upb_FieldDef* upb_DefPool_FindExtensionByNameWithSize(
    const upb_DefPool* s, const char* name, size_t size) {
  upb_value v;
  if (!upb_strtable_lookup2(&s->syms, name, size, &v)) return NULL;

  switch (_upb_DefType_Type(v)) {
    case UPB_DEFTYPE_FIELD:
      return _upb_DefType_Unpack(v, UPB_DEFTYPE_FIELD);
    case UPB_DEFTYPE_MSG: {
      const upb_MessageDef* m = _upb_DefType_Unpack(v, UPB_DEFTYPE_MSG);
      return _upb_MessageDef_InMessageSet(m)
                 ? upb_MessageDef_NestedExtension(m, 0)
                 : NULL;
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
  return _upb_DefPool_Unpack(s, name, strlen(name), UPB_DEFTYPE_SERVICE);
}

const upb_ServiceDef* upb_DefPool_FindServiceByNameWithSize(
    const upb_DefPool* s, const char* name, size_t size) {
  return _upb_DefPool_Unpack(s, name, size, UPB_DEFTYPE_SERVICE);
}

const upb_FileDef* upb_DefPool_FindFileContainingSymbol(const upb_DefPool* s,
                                                        const char* name) {
  upb_value v;
  // TODO(haberman): non-extension fields and oneofs.
  if (upb_strtable_lookup(&s->syms, name, &v)) {
    switch (_upb_DefType_Type(v)) {
      case UPB_DEFTYPE_EXT: {
        const upb_FieldDef* f = _upb_DefType_Unpack(v, UPB_DEFTYPE_EXT);
        return upb_FieldDef_File(f);
      }
      case UPB_DEFTYPE_MSG: {
        const upb_MessageDef* m = _upb_DefType_Unpack(v, UPB_DEFTYPE_MSG);
        return upb_MessageDef_File(m);
      }
      case UPB_DEFTYPE_ENUM: {
        const upb_EnumDef* e = _upb_DefType_Unpack(v, UPB_DEFTYPE_ENUM);
        return upb_EnumDef_File(e);
      }
      case UPB_DEFTYPE_ENUMVAL: {
        const upb_EnumValueDef* ev =
            _upb_DefType_Unpack(v, UPB_DEFTYPE_ENUMVAL);
        return upb_EnumDef_File(upb_EnumValueDef_Enum(ev));
      }
      case UPB_DEFTYPE_SERVICE: {
        const upb_ServiceDef* service =
            _upb_DefType_Unpack(v, UPB_DEFTYPE_SERVICE);
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

static void remove_filedef(upb_DefPool* s, upb_FileDef* file) {
  intptr_t iter = UPB_INTTABLE_BEGIN;
  upb_StringView key;
  upb_value val;
  while (upb_strtable_next2(&s->syms, &key, &val, &iter)) {
    const upb_FileDef* f;
    switch (_upb_DefType_Type(val)) {
      case UPB_DEFTYPE_EXT:
        f = upb_FieldDef_File(_upb_DefType_Unpack(val, UPB_DEFTYPE_EXT));
        break;
      case UPB_DEFTYPE_MSG:
        f = upb_MessageDef_File(_upb_DefType_Unpack(val, UPB_DEFTYPE_MSG));
        break;
      case UPB_DEFTYPE_ENUM:
        f = upb_EnumDef_File(_upb_DefType_Unpack(val, UPB_DEFTYPE_ENUM));
        break;
      case UPB_DEFTYPE_ENUMVAL:
        f = upb_EnumDef_File(upb_EnumValueDef_Enum(
            _upb_DefType_Unpack(val, UPB_DEFTYPE_ENUMVAL)));
        break;
      case UPB_DEFTYPE_SERVICE:
        f = upb_ServiceDef_File(_upb_DefType_Unpack(val, UPB_DEFTYPE_SERVICE));
        break;
      default:
        UPB_UNREACHABLE();
    }

    if (f == file) upb_strtable_removeiter(&s->syms, &iter);
  }
}

static const upb_FileDef* upb_DefBuilder_AddFileToPool(
    upb_DefBuilder* const builder, upb_DefPool* const s,
    const UPB_DESC(FileDescriptorProto) * const file_proto,
    const upb_StringView name, upb_Status* const status) {
  if (UPB_SETJMP(builder->err) != 0) {
    UPB_ASSERT(!upb_Status_IsOk(status));
    if (builder->file) {
      remove_filedef(s, builder->file);
      builder->file = NULL;
    }
  } else if (!builder->arena || !builder->tmp_arena) {
    _upb_DefBuilder_OomErr(builder);
  } else {
    _upb_FileDef_Create(builder, file_proto);
    upb_strtable_insert(&s->files, name.data, name.size,
                        upb_value_constptr(builder->file), builder->arena);
    UPB_ASSERT(upb_Status_IsOk(status));
    upb_Arena_Fuse(s->arena, builder->arena);
  }

  if (builder->arena) upb_Arena_Free(builder->arena);
  if (builder->tmp_arena) upb_Arena_Free(builder->tmp_arena);
  return builder->file;
}

static const upb_FileDef* _upb_DefPool_AddFile(
    upb_DefPool* s, const UPB_DESC(FileDescriptorProto) * file_proto,
    const upb_MiniTableFile* layout, upb_Status* status) {
  const upb_StringView name = UPB_DESC(FileDescriptorProto_name)(file_proto);

  // Determine whether we already know about this file.
  {
    upb_value v;
    if (upb_strtable_lookup2(&s->files, name.data, name.size, &v)) {
      upb_Status_SetErrorFormat(status,
                                "duplicate file name " UPB_STRINGVIEW_FORMAT,
                                UPB_STRINGVIEW_ARGS(name));
      return NULL;
    }
  }

  upb_DefBuilder ctx = {
      .symtab = s,
      .layout = layout,
      .platform = s->platform,
      .msg_count = 0,
      .enum_count = 0,
      .ext_count = 0,
      .status = status,
      .file = NULL,
      .arena = upb_Arena_New(),
      .tmp_arena = upb_Arena_New(),
  };

  return upb_DefBuilder_AddFileToPool(&ctx, s, file_proto, name, status);
}

const upb_FileDef* upb_DefPool_AddFile(upb_DefPool* s,
                                       const UPB_DESC(FileDescriptorProto) *
                                           file_proto,
                                       upb_Status* status) {
  return _upb_DefPool_AddFile(s, file_proto, NULL, status);
}

bool _upb_DefPool_LoadDefInitEx(upb_DefPool* s, const _upb_DefPool_Init* init,
                                bool rebuild_minitable) {
  /* Since this function should never fail (it would indicate a bug in upb) we
   * print errors to stderr instead of returning error status to the user. */
  _upb_DefPool_Init** deps = init->deps;
  UPB_DESC(FileDescriptorProto) * file;
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

  file = UPB_DESC(FileDescriptorProto_parse_ex)(
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

  const upb_MiniTableFile* mt = rebuild_minitable ? NULL : init->layout;
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

const upb_FieldDef* upb_DefPool_FindExtensionByMiniTable(
    const upb_DefPool* s, const upb_MiniTableExtension* ext) {
  upb_value v;
  bool ok = upb_inttable_lookup(&s->exts, (uintptr_t)ext, &v);
  UPB_ASSERT(ok);
  return upb_value_getconstptr(v);
}

const upb_FieldDef* upb_DefPool_FindExtensionByNumber(const upb_DefPool* s,
                                                      const upb_MessageDef* m,
                                                      int32_t fieldnum) {
  const upb_MiniTable* t = upb_MessageDef_MiniTable(m);
  const upb_MiniTableExtension* ext =
      upb_ExtensionRegistry_Lookup(s->extreg, t, fieldnum);
  return ext ? upb_DefPool_FindExtensionByMiniTable(s, ext) : NULL;
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
  while (upb_inttable_next(&s->exts, &key, &val, &iter)) {
    const upb_FieldDef* f = upb_value_getconstptr(val);
    if (upb_FieldDef_ContainingType(f) == m) n++;
  }
  const upb_FieldDef** exts = malloc(n * sizeof(*exts));
  iter = UPB_INTTABLE_BEGIN;
  size_t i = 0;
  while (upb_inttable_next(&s->exts, &key, &val, &iter)) {
    const upb_FieldDef* f = upb_value_getconstptr(val);
    if (upb_FieldDef_ContainingType(f) == m) exts[i++] = f;
  }
  *count = n;
  return exts;
}

bool _upb_DefPool_LoadDefInit(upb_DefPool* s, const _upb_DefPool_Init* init) {
  return _upb_DefPool_LoadDefInitEx(s, init, false);
}


// Must be last.

upb_deftype_t _upb_DefType_Type(upb_value v) {
  const uintptr_t num = (uintptr_t)upb_value_getconstptr(v);
  return num & UPB_DEFTYPE_MASK;
}

upb_value _upb_DefType_Pack(const void* ptr, upb_deftype_t type) {
  uintptr_t num = (uintptr_t)ptr;
  UPB_ASSERT((num & UPB_DEFTYPE_MASK) == 0);
  num |= type;
  return upb_value_constptr((const void*)num);
}

const void* _upb_DefType_Unpack(upb_value v, upb_deftype_t type) {
  uintptr_t num = (uintptr_t)upb_value_getconstptr(v);
  return (num & UPB_DEFTYPE_MASK) == type
             ? (const void*)(num & ~UPB_DEFTYPE_MASK)
             : NULL;
}


// Must be last.

bool _upb_DescState_Grow(upb_DescState* d, upb_Arena* a) {
  const size_t oldbufsize = d->bufsize;
  const int used = d->ptr - d->buf;

  if (!d->buf) {
    d->buf = upb_Arena_Malloc(a, d->bufsize);
    if (!d->buf) return false;
    d->ptr = d->buf;
    d->e.end = d->buf + d->bufsize;
  }

  if (oldbufsize - used < kUpb_MtDataEncoder_MinSize) {
    d->bufsize *= 2;
    d->buf = upb_Arena_Realloc(a, d->buf, oldbufsize, d->bufsize);
    if (!d->buf) return false;
    d->ptr = d->buf + used;
    d->e.end = d->buf + d->bufsize;
  }

  return true;
}


// Must be last.

struct upb_EnumDef {
  const UPB_DESC(EnumOptions) * opts;
  const upb_MiniTableEnum* layout;  // Only for proto2.
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
  bool is_closed;
  bool is_sorted;  // Whether all of the values are defined in ascending order.
};

upb_EnumDef* _upb_EnumDef_At(const upb_EnumDef* e, int i) {
  return (upb_EnumDef*)&e[i];
}

const upb_MiniTableEnum* _upb_EnumDef_MiniTable(const upb_EnumDef* e) {
  return e->layout;
}

bool _upb_EnumDef_Insert(upb_EnumDef* e, upb_EnumValueDef* v, upb_Arena* a) {
  const char* name = upb_EnumValueDef_Name(v);
  const upb_value val = upb_value_constptr(v);
  bool ok = upb_strtable_insert(&e->ntoi, name, strlen(name), val, a);
  if (!ok) return false;

  // Multiple enumerators can have the same number, first one wins.
  const int number = upb_EnumValueDef_Number(v);
  if (!upb_inttable_lookup(&e->iton, number, NULL)) {
    return upb_inttable_insert(&e->iton, number, val, a);
  }
  return true;
}

const UPB_DESC(EnumOptions) * upb_EnumDef_Options(const upb_EnumDef* e) {
  return e->opts;
}

bool upb_EnumDef_HasOptions(const upb_EnumDef* e) {
  return e->opts != (void*)kUpbDefOptDefault;
}

const char* upb_EnumDef_FullName(const upb_EnumDef* e) { return e->full_name; }

const char* upb_EnumDef_Name(const upb_EnumDef* e) {
  return _upb_DefBuilder_FullToShort(e->full_name);
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

const upb_EnumValueDef* upb_EnumDef_FindValueByName(const upb_EnumDef* e,
                                                    const char* name) {
  return upb_EnumDef_FindValueByNameWithSize(e, name, strlen(name));
}

const upb_EnumValueDef* upb_EnumDef_FindValueByNameWithSize(
    const upb_EnumDef* e, const char* name, size_t size) {
  upb_value v;
  return upb_strtable_lookup2(&e->ntoi, name, size, &v)
             ? upb_value_getconstptr(v)
             : NULL;
}

const upb_EnumValueDef* upb_EnumDef_FindValueByNumber(const upb_EnumDef* e,
                                                      int32_t num) {
  upb_value v;
  return upb_inttable_lookup(&e->iton, num, &v) ? upb_value_getconstptr(v)
                                                : NULL;
}

bool upb_EnumDef_CheckNumber(const upb_EnumDef* e, int32_t num) {
  // We could use upb_EnumDef_FindValueByNumber(e, num) != NULL, but we expect
  // this to be faster (especially for small numbers).
  return upb_MiniTableEnum_CheckValue(e->layout, num);
}

const upb_EnumValueDef* upb_EnumDef_Value(const upb_EnumDef* e, int i) {
  UPB_ASSERT(0 <= i && i < e->value_count);
  return _upb_EnumValueDef_At(e->values, i);
}

bool upb_EnumDef_IsClosed(const upb_EnumDef* e) { return e->is_closed; }

bool upb_EnumDef_MiniDescriptorEncode(const upb_EnumDef* e, upb_Arena* a,
                                      upb_StringView* out) {
  upb_DescState s;
  _upb_DescState_Init(&s);

  const upb_EnumValueDef** sorted = NULL;
  if (!e->is_sorted) {
    sorted = _upb_EnumValueDefs_Sorted(e->values, e->value_count, a);
    if (!sorted) return false;
  }

  if (!_upb_DescState_Grow(&s, a)) return false;
  s.ptr = upb_MtDataEncoder_StartEnum(&s.e, s.ptr);

  // Duplicate values are allowed but we only encode each value once.
  uint32_t previous = 0;

  for (size_t i = 0; i < e->value_count; i++) {
    const uint32_t current =
        upb_EnumValueDef_Number(sorted ? sorted[i] : upb_EnumDef_Value(e, i));
    if (i != 0 && previous == current) continue;

    if (!_upb_DescState_Grow(&s, a)) return false;
    s.ptr = upb_MtDataEncoder_PutEnumValue(&s.e, s.ptr, current);
    previous = current;
  }

  if (!_upb_DescState_Grow(&s, a)) return false;
  s.ptr = upb_MtDataEncoder_EndEnum(&s.e, s.ptr);

  // There will always be room for this '\0' in the encoder buffer because
  // kUpb_MtDataEncoder_MinSize is overkill for upb_MtDataEncoder_EndEnum().
  UPB_ASSERT(s.ptr < s.buf + s.bufsize);
  *s.ptr = '\0';

  out->data = s.buf;
  out->size = s.ptr - s.buf;
  return true;
}

static upb_MiniTableEnum* create_enumlayout(upb_DefBuilder* ctx,
                                            const upb_EnumDef* e) {
  upb_StringView sv;
  bool ok = upb_EnumDef_MiniDescriptorEncode(e, ctx->tmp_arena, &sv);
  if (!ok) _upb_DefBuilder_Errf(ctx, "OOM while building enum MiniDescriptor");

  upb_Status status;
  upb_MiniTableEnum* layout =
      upb_MiniTableEnum_Build(sv.data, sv.size, ctx->arena, &status);
  if (!layout)
    _upb_DefBuilder_Errf(ctx, "Error building enum MiniTable: %s", status.msg);
  return layout;
}

static upb_StringView* _upb_EnumReservedNames_New(
    upb_DefBuilder* ctx, int n, const upb_StringView* protos) {
  upb_StringView* sv = _upb_DefBuilder_Alloc(ctx, sizeof(upb_StringView) * n);
  for (size_t i = 0; i < n; i++) {
    sv[i].data =
        upb_strdup2(protos[i].data, protos[i].size, _upb_DefBuilder_Arena(ctx));
    sv[i].size = protos[i].size;
  }
  return sv;
}

static void create_enumdef(upb_DefBuilder* ctx, const char* prefix,
                           const UPB_DESC(EnumDescriptorProto) * enum_proto,
                           upb_EnumDef* e) {
  const UPB_DESC(EnumValueDescriptorProto)* const* values;
  const UPB_DESC(EnumDescriptorProto_EnumReservedRange)* const* res_ranges;
  const upb_StringView* res_names;
  upb_StringView name;
  size_t n_value, n_res_range, n_res_name;

  // Must happen before _upb_DefBuilder_Add()
  e->file = _upb_DefBuilder_File(ctx);

  name = UPB_DESC(EnumDescriptorProto_name)(enum_proto);

  e->full_name = _upb_DefBuilder_MakeFullName(ctx, prefix, name);
  _upb_DefBuilder_Add(ctx, e->full_name,
                      _upb_DefType_Pack(e, UPB_DEFTYPE_ENUM));

  e->is_closed = (!UPB_TREAT_PROTO2_ENUMS_LIKE_PROTO3) &&
                 (upb_FileDef_Syntax(e->file) == kUpb_Syntax_Proto2);

  values = UPB_DESC(EnumDescriptorProto_value)(enum_proto, &n_value);

  bool ok = upb_strtable_init(&e->ntoi, n_value, ctx->arena);
  if (!ok) _upb_DefBuilder_OomErr(ctx);

  ok = upb_inttable_init(&e->iton, ctx->arena);
  if (!ok) _upb_DefBuilder_OomErr(ctx);

  e->defaultval = 0;
  e->value_count = n_value;
  e->values =
      _upb_EnumValueDefs_New(ctx, prefix, n_value, values, e, &e->is_sorted);

  if (n_value == 0) {
    _upb_DefBuilder_Errf(ctx, "enums must contain at least one value (%s)",
                         e->full_name);
  }

  res_ranges =
      UPB_DESC(EnumDescriptorProto_reserved_range)(enum_proto, &n_res_range);
  e->res_range_count = n_res_range;
  e->res_ranges = _upb_EnumReservedRanges_New(ctx, n_res_range, res_ranges, e);

  res_names =
      UPB_DESC(EnumDescriptorProto_reserved_name)(enum_proto, &n_res_name);
  e->res_name_count = n_res_name;
  e->res_names = _upb_EnumReservedNames_New(ctx, n_res_name, res_names);

  UPB_DEF_SET_OPTIONS(e->opts, EnumDescriptorProto, EnumOptions, enum_proto);

  upb_inttable_compact(&e->iton, ctx->arena);

  if (e->is_closed) {
    if (ctx->layout) {
      UPB_ASSERT(ctx->enum_count < ctx->layout->enum_count);
      e->layout = ctx->layout->enums[ctx->enum_count++];
    } else {
      e->layout = create_enumlayout(ctx, e);
    }
  } else {
    e->layout = NULL;
  }
}

upb_EnumDef* _upb_EnumDefs_New(
    upb_DefBuilder* ctx, int n,
    const UPB_DESC(EnumDescriptorProto) * const* protos,
    const upb_MessageDef* containing_type) {
  _upb_DefType_CheckPadding(sizeof(upb_EnumDef));

  // If a containing type is defined then get the full name from that.
  // Otherwise use the package name from the file def.
  const char* name = containing_type ? upb_MessageDef_FullName(containing_type)
                                     : _upb_FileDef_RawPackage(ctx->file);

  upb_EnumDef* e = _upb_DefBuilder_Alloc(ctx, sizeof(upb_EnumDef) * n);
  for (size_t i = 0; i < n; i++) {
    create_enumdef(ctx, name, protos[i], &e[i]);
    e[i].containing_type = containing_type;
  }
  return e;
}

// #include "upb/reflection/extension_range_internal.h"
// #include "upb/reflection/message_def.h"

// Must be last.

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

upb_EnumReservedRange* _upb_EnumReservedRanges_New(
    upb_DefBuilder* ctx, int n,
    const UPB_DESC(EnumDescriptorProto_EnumReservedRange) * const* protos,
    const upb_EnumDef* e) {
  upb_EnumReservedRange* r =
      _upb_DefBuilder_Alloc(ctx, sizeof(upb_EnumReservedRange) * n);

  for (int i = 0; i < n; i++) {
    const int32_t start =
        UPB_DESC(EnumDescriptorProto_EnumReservedRange_start)(protos[i]);
    const int32_t end =
        UPB_DESC(EnumDescriptorProto_EnumReservedRange_end)(protos[i]);

    // A full validation would also check that each range is disjoint, and that
    // none of the fields overlap with the extension ranges, but we are just
    // sanity checking here.

    // Note: Not a typo! Unlike extension ranges and message reserved ranges,
    // the end value of an enum reserved range is *inclusive*!
    if (end < start) {
      _upb_DefBuilder_Errf(ctx, "Reserved range (%d, %d) is invalid, enum=%s\n",
                           (int)start, (int)end, upb_EnumDef_FullName(e));
    }

    r[i].start = start;
    r[i].end = end;
  }

  return r;
}


// Must be last.

struct upb_EnumValueDef {
  const UPB_DESC(EnumValueOptions) * opts;
  const upb_EnumDef* parent;
  const char* full_name;
  int32_t number;
};

upb_EnumValueDef* _upb_EnumValueDef_At(const upb_EnumValueDef* v, int i) {
  return (upb_EnumValueDef*)&v[i];
}

static int _upb_EnumValueDef_Compare(const void* p1, const void* p2) {
  const uint32_t v1 = (*(const upb_EnumValueDef**)p1)->number;
  const uint32_t v2 = (*(const upb_EnumValueDef**)p2)->number;
  return (v1 < v2) ? -1 : (v1 > v2);
}

const upb_EnumValueDef** _upb_EnumValueDefs_Sorted(const upb_EnumValueDef* v,
                                                   int n, upb_Arena* a) {
  // TODO: Try to replace this arena alloc with a persistent scratch buffer.
  upb_EnumValueDef** out =
      (upb_EnumValueDef**)upb_Arena_Malloc(a, n * sizeof(void*));
  if (!out) return NULL;

  for (int i = 0; i < n; i++) {
    out[i] = (upb_EnumValueDef*)&v[i];
  }
  qsort(out, n, sizeof(void*), _upb_EnumValueDef_Compare);

  return (const upb_EnumValueDef**)out;
}

const UPB_DESC(EnumValueOptions) *
    upb_EnumValueDef_Options(const upb_EnumValueDef* v) {
  return v->opts;
}

bool upb_EnumValueDef_HasOptions(const upb_EnumValueDef* v) {
  return v->opts != (void*)kUpbDefOptDefault;
}

const upb_EnumDef* upb_EnumValueDef_Enum(const upb_EnumValueDef* v) {
  return v->parent;
}

const char* upb_EnumValueDef_FullName(const upb_EnumValueDef* v) {
  return v->full_name;
}

const char* upb_EnumValueDef_Name(const upb_EnumValueDef* v) {
  return _upb_DefBuilder_FullToShort(v->full_name);
}

int32_t upb_EnumValueDef_Number(const upb_EnumValueDef* v) { return v->number; }

uint32_t upb_EnumValueDef_Index(const upb_EnumValueDef* v) {
  // Compute index in our parent's array.
  return v - upb_EnumDef_Value(v->parent, 0);
}

static void create_enumvaldef(upb_DefBuilder* ctx, const char* prefix,
                              const UPB_DESC(EnumValueDescriptorProto) *
                                  val_proto,
                              upb_EnumDef* e, upb_EnumValueDef* v) {
  upb_StringView name = UPB_DESC(EnumValueDescriptorProto_name)(val_proto);

  v->parent = e;  // Must happen prior to _upb_DefBuilder_Add()
  v->full_name = _upb_DefBuilder_MakeFullName(ctx, prefix, name);
  v->number = UPB_DESC(EnumValueDescriptorProto_number)(val_proto);
  _upb_DefBuilder_Add(ctx, v->full_name,
                      _upb_DefType_Pack(v, UPB_DEFTYPE_ENUMVAL));

  UPB_DEF_SET_OPTIONS(v->opts, EnumValueDescriptorProto, EnumValueOptions,
                      val_proto);

  bool ok = _upb_EnumDef_Insert(e, v, ctx->arena);
  if (!ok) _upb_DefBuilder_OomErr(ctx);
}

// Allocate and initialize an array of |n| enum value defs owned by |e|.
upb_EnumValueDef* _upb_EnumValueDefs_New(
    upb_DefBuilder* ctx, const char* prefix, int n,
    const UPB_DESC(EnumValueDescriptorProto) * const* protos, upb_EnumDef* e,
    bool* is_sorted) {
  _upb_DefType_CheckPadding(sizeof(upb_EnumValueDef));

  upb_EnumValueDef* v =
      _upb_DefBuilder_Alloc(ctx, sizeof(upb_EnumValueDef) * n);

  *is_sorted = true;
  uint32_t previous = 0;
  for (size_t i = 0; i < n; i++) {
    create_enumvaldef(ctx, prefix, protos[i], e, &v[i]);

    const uint32_t current = v[i].number;
    if (previous > current) *is_sorted = false;
    previous = current;
  }

  if (upb_FileDef_Syntax(ctx->file) == kUpb_Syntax_Proto3 && n > 0 &&
      v[0].number != 0) {
    _upb_DefBuilder_Errf(ctx,
                         "for proto3, the first enum value must be zero (%s)",
                         upb_EnumDef_FullName(e));
  }

  return v;
}


// Must be last.

struct upb_ExtensionRange {
  const UPB_DESC(ExtensionRangeOptions) * opts;
  int32_t start;
  int32_t end;
};

upb_ExtensionRange* _upb_ExtensionRange_At(const upb_ExtensionRange* r, int i) {
  return (upb_ExtensionRange*)&r[i];
}

const UPB_DESC(ExtensionRangeOptions) *
    upb_ExtensionRange_Options(const upb_ExtensionRange* r) {
  return r->opts;
}

bool upb_ExtensionRange_HasOptions(const upb_ExtensionRange* r) {
  return r->opts != (void*)kUpbDefOptDefault;
}

int32_t upb_ExtensionRange_Start(const upb_ExtensionRange* r) {
  return r->start;
}

int32_t upb_ExtensionRange_End(const upb_ExtensionRange* r) { return r->end; }

upb_ExtensionRange* _upb_ExtensionRanges_New(
    upb_DefBuilder* ctx, int n,
    const UPB_DESC(DescriptorProto_ExtensionRange) * const* protos,
    const upb_MessageDef* m) {
  upb_ExtensionRange* r =
      _upb_DefBuilder_Alloc(ctx, sizeof(upb_ExtensionRange) * n);

  for (int i = 0; i < n; i++) {
    const int32_t start =
        UPB_DESC(DescriptorProto_ExtensionRange_start)(protos[i]);
    const int32_t end = UPB_DESC(DescriptorProto_ExtensionRange_end)(protos[i]);
    const int32_t max = UPB_DESC(MessageOptions_message_set_wire_format)(
                            upb_MessageDef_Options(m))
                            ? INT32_MAX
                            : kUpb_MaxFieldNumber + 1;

    // A full validation would also check that each range is disjoint, and that
    // none of the fields overlap with the extension ranges, but we are just
    // sanity checking here.
    if (start < 1 || end <= start || end > max) {
      _upb_DefBuilder_Errf(ctx,
                           "Extension range (%d, %d) is invalid, message=%s\n",
                           (int)start, (int)end, upb_MessageDef_FullName(m));
    }

    r[i].start = start;
    r[i].end = end;
    UPB_DEF_SET_OPTIONS(r[i].opts, DescriptorProto_ExtensionRange,
                        ExtensionRangeOptions, protos[i]);
  }

  return r;
}

#include <ctype.h>
#include <errno.h>


// Must be last.

#define UPB_FIELD_TYPE_UNSPECIFIED 0

typedef struct {
  size_t len;
  char str[1];  // Null-terminated string data follows.
} str_t;

struct upb_FieldDef {
  const UPB_DESC(FieldOptions) * opts;
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
    void* msg;  // Always NULL.
  } defaultval;
  union {
    const upb_OneofDef* oneof;
    const upb_MessageDef* extension_scope;
  } scope;
  union {
    const upb_MessageDef* msgdef;
    const upb_EnumDef* enumdef;
    const UPB_DESC(FieldDescriptorProto) * unresolved;
  } sub;
  uint32_t number_;
  uint16_t index_;
  uint16_t layout_index;  // Index into msgdef->layout->fields or file->exts
  bool has_default;
  bool has_json_name;
  bool has_presence;
  bool is_extension;
  bool is_packed;
  bool is_proto3_optional;
  upb_FieldType type_;
  upb_Label label_;
#if UINTPTR_MAX == 0xffffffff
  uint32_t padding;  // Increase size to a multiple of 8.
#endif
};

upb_FieldDef* _upb_FieldDef_At(const upb_FieldDef* f, int i) {
  return (upb_FieldDef*)&f[i];
}

const UPB_DESC(FieldOptions) * upb_FieldDef_Options(const upb_FieldDef* f) {
  return f->opts;
}

bool upb_FieldDef_HasOptions(const upb_FieldDef* f) {
  return f->opts != (void*)kUpbDefOptDefault;
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

bool upb_FieldDef_IsExtension(const upb_FieldDef* f) { return f->is_extension; }

bool upb_FieldDef_IsPacked(const upb_FieldDef* f) { return f->is_packed; }

const char* upb_FieldDef_Name(const upb_FieldDef* f) {
  return _upb_DefBuilder_FullToShort(f->full_name);
}

const char* upb_FieldDef_JsonName(const upb_FieldDef* f) {
  return f->json_name;
}

bool upb_FieldDef_HasJsonName(const upb_FieldDef* f) {
  return f->has_json_name;
}

const upb_FileDef* upb_FieldDef_File(const upb_FieldDef* f) { return f->file; }

const upb_MessageDef* upb_FieldDef_ContainingType(const upb_FieldDef* f) {
  return f->msgdef;
}

const upb_MessageDef* upb_FieldDef_ExtensionScope(const upb_FieldDef* f) {
  return f->is_extension ? f->scope.extension_scope : NULL;
}

const upb_OneofDef* upb_FieldDef_ContainingOneof(const upb_FieldDef* f) {
  return f->is_extension ? NULL : f->scope.oneof;
}

const upb_OneofDef* upb_FieldDef_RealContainingOneof(const upb_FieldDef* f) {
  const upb_OneofDef* oneof = upb_FieldDef_ContainingOneof(f);
  if (!oneof || upb_OneofDef_IsSynthetic(oneof)) return NULL;
  return oneof;
}

upb_MessageValue upb_FieldDef_Default(const upb_FieldDef* f) {
  upb_MessageValue ret;

  if (upb_FieldDef_IsRepeated(f) || upb_FieldDef_IsSubMessage(f)) {
    return (upb_MessageValue){.msg_val = NULL};
  }

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

const upb_MiniTableField* upb_FieldDef_MiniTable(const upb_FieldDef* f) {
  if (upb_FieldDef_IsExtension(f)) {
    const upb_FileDef* file = upb_FieldDef_File(f);
    return (upb_MiniTableField*)_upb_FileDef_ExtensionMiniTable(
        file, f->layout_index);
  } else {
    const upb_MiniTable* layout = upb_MessageDef_MiniTable(f->msgdef);
    return &layout->fields[f->layout_index];
  }
}

const upb_MiniTableExtension* _upb_FieldDef_ExtensionMiniTable(
    const upb_FieldDef* f) {
  UPB_ASSERT(upb_FieldDef_IsExtension(f));
  const upb_FileDef* file = upb_FieldDef_File(f);
  return _upb_FileDef_ExtensionMiniTable(file, f->layout_index);
}

bool _upb_FieldDef_IsClosedEnum(const upb_FieldDef* f) {
  if (f->type_ != kUpb_FieldType_Enum) return false;
  return upb_EnumDef_IsClosed(f->sub.enumdef);
}

bool _upb_FieldDef_IsProto3Optional(const upb_FieldDef* f) {
  return f->is_proto3_optional;
}

int _upb_FieldDef_LayoutIndex(const upb_FieldDef* f) { return f->layout_index; }

uint64_t _upb_FieldDef_Modifiers(const upb_FieldDef* f) {
  uint64_t out = f->is_packed ? kUpb_FieldModifier_IsPacked : 0;

  switch (f->label_) {
    case kUpb_Label_Optional:
      if (!upb_FieldDef_HasPresence(f)) {
        out |= kUpb_FieldModifier_IsProto3Singular;
      }
      break;
    case kUpb_Label_Repeated:
      out |= kUpb_FieldModifier_IsRepeated;
      break;
    case kUpb_Label_Required:
      out |= kUpb_FieldModifier_IsRequired;
      break;
  }

  if (_upb_FieldDef_IsClosedEnum(f)) {
    out |= kUpb_FieldModifier_IsClosedEnum;
  }
  return out;
}

bool upb_FieldDef_HasDefault(const upb_FieldDef* f) { return f->has_default; }
bool upb_FieldDef_HasPresence(const upb_FieldDef* f) { return f->has_presence; }

bool upb_FieldDef_HasSubDef(const upb_FieldDef* f) {
  return upb_FieldDef_IsSubMessage(f) ||
         upb_FieldDef_CType(f) == kUpb_CType_Enum;
}

bool upb_FieldDef_IsMap(const upb_FieldDef* f) {
  return upb_FieldDef_IsRepeated(f) && upb_FieldDef_IsSubMessage(f) &&
         upb_MessageDef_IsMapEntry(upb_FieldDef_MessageSubDef(f));
}

bool upb_FieldDef_IsOptional(const upb_FieldDef* f) {
  return upb_FieldDef_Label(f) == kUpb_Label_Optional;
}

bool upb_FieldDef_IsPrimitive(const upb_FieldDef* f) {
  return !upb_FieldDef_IsString(f) && !upb_FieldDef_IsSubMessage(f);
}

bool upb_FieldDef_IsRepeated(const upb_FieldDef* f) {
  return upb_FieldDef_Label(f) == kUpb_Label_Repeated;
}

bool upb_FieldDef_IsRequired(const upb_FieldDef* f) {
  return upb_FieldDef_Label(f) == kUpb_Label_Required;
}

bool upb_FieldDef_IsString(const upb_FieldDef* f) {
  return upb_FieldDef_CType(f) == kUpb_CType_String ||
         upb_FieldDef_CType(f) == kUpb_CType_Bytes;
}

bool upb_FieldDef_IsSubMessage(const upb_FieldDef* f) {
  return upb_FieldDef_CType(f) == kUpb_CType_Message;
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

static bool streql2(const char* a, size_t n, const char* b) {
  return n == strlen(b) && memcmp(a, b, n) == 0;
}

// Implement the transformation as described in the spec:
//   1. upper case all letters after an underscore.
//   2. remove all underscores.
static char* make_json_name(const char* name, size_t size, upb_Arena* a) {
  char* out = upb_Arena_Malloc(a, size + 1);  // +1 is to add a trailing '\0'
  if (out == NULL) return NULL;

  bool ucase_next = false;
  char* des = out;
  for (size_t i = 0; i < size; i++) {
    if (name[i] == '_') {
      ucase_next = true;
    } else {
      *des++ = ucase_next ? toupper(name[i]) : name[i];
      ucase_next = false;
    }
  }
  *des++ = '\0';
  return out;
}

static str_t* newstr(upb_DefBuilder* ctx, const char* data, size_t len) {
  str_t* ret = _upb_DefBuilder_Alloc(ctx, sizeof(*ret) + len);
  if (!ret) _upb_DefBuilder_OomErr(ctx);
  ret->len = len;
  if (len) memcpy(ret->str, data, len);
  ret->str[len] = '\0';
  return ret;
}

static str_t* unescape(upb_DefBuilder* ctx, const upb_FieldDef* f,
                       const char* data, size_t len) {
  // Size here is an upper bound; escape sequences could ultimately shrink it.
  str_t* ret = _upb_DefBuilder_Alloc(ctx, sizeof(*ret) + len);
  char* dst = &ret->str[0];
  const char* src = data;
  const char* end = data + len;

  while (src < end) {
    if (*src == '\\') {
      src++;
      *dst++ = _upb_DefBuilder_ParseEscape(ctx, f, &src, end);
    } else {
      *dst++ = *src++;
    }
  }

  ret->len = dst - &ret->str[0];
  return ret;
}

static void parse_default(upb_DefBuilder* ctx, const char* str, size_t len,
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
      // Standard C number parsing functions expect null-terminated strings.
      if (len >= sizeof(nullz) - 1) {
        _upb_DefBuilder_Errf(ctx, "Default too long: %.*s", (int)len, str);
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
      f->defaultval.sint = upb_EnumValueDef_Number(ev);
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
      _upb_DefBuilder_Errf(ctx, "Message should not have a default (%s)",
                           upb_FieldDef_FullName(f));
  }

  return;

invalid:
  _upb_DefBuilder_Errf(ctx, "Invalid default '%.*s' for field %s of type %d",
                       (int)len, str, upb_FieldDef_FullName(f),
                       (int)upb_FieldDef_Type(f));
}

static void set_default_default(upb_DefBuilder* ctx, upb_FieldDef* f) {
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
    case kUpb_CType_Enum: {
      const upb_EnumValueDef* v = upb_EnumDef_Value(f->sub.enumdef, 0);
      f->defaultval.sint = upb_EnumValueDef_Number(v);
      break;
    }
    case kUpb_CType_Message:
      break;
  }
}

static void _upb_FieldDef_Create(upb_DefBuilder* ctx, const char* prefix,
                                 const UPB_DESC(FieldDescriptorProto) *
                                     field_proto,
                                 upb_MessageDef* m, upb_FieldDef* f) {
  // Must happen before _upb_DefBuilder_Add()
  f->file = _upb_DefBuilder_File(ctx);

  if (!UPB_DESC(FieldDescriptorProto_has_name)(field_proto)) {
    _upb_DefBuilder_Errf(ctx, "field has no name");
  }

  const upb_StringView name = UPB_DESC(FieldDescriptorProto_name)(field_proto);

  f->full_name = _upb_DefBuilder_MakeFullName(ctx, prefix, name);
  f->label_ = (int)UPB_DESC(FieldDescriptorProto_label)(field_proto);
  f->number_ = UPB_DESC(FieldDescriptorProto_number)(field_proto);
  f->is_proto3_optional =
      UPB_DESC(FieldDescriptorProto_proto3_optional)(field_proto);
  f->msgdef = m;
  f->scope.oneof = NULL;

  f->has_json_name = UPB_DESC(FieldDescriptorProto_has_json_name)(field_proto);
  if (f->has_json_name) {
    const upb_StringView sv =
        UPB_DESC(FieldDescriptorProto_json_name)(field_proto);
    f->json_name = upb_strdup2(sv.data, sv.size, ctx->arena);
  } else {
    f->json_name = make_json_name(name.data, name.size, ctx->arena);
  }
  if (!f->json_name) _upb_DefBuilder_OomErr(ctx);

  const bool has_type = UPB_DESC(FieldDescriptorProto_has_type)(field_proto);
  const bool has_type_name =
      UPB_DESC(FieldDescriptorProto_has_type_name)(field_proto);

  f->type_ = (int)UPB_DESC(FieldDescriptorProto_type)(field_proto);

  if (has_type) {
    switch (f->type_) {
      case kUpb_FieldType_Message:
      case kUpb_FieldType_Group:
      case kUpb_FieldType_Enum:
        if (!has_type_name) {
          _upb_DefBuilder_Errf(ctx, "field of type %d requires type name (%s)",
                               (int)f->type_, f->full_name);
        }
        break;
      default:
        if (has_type_name) {
          _upb_DefBuilder_Errf(
              ctx, "invalid type for field with type_name set (%s, %d)",
              f->full_name, (int)f->type_);
        }
    }
  } else if (has_type_name) {
    f->type_ =
        UPB_FIELD_TYPE_UNSPECIFIED;  // We'll assign this in resolve_fielddef()
  }

  if (f->type_ < kUpb_FieldType_Double || f->type_ > kUpb_FieldType_SInt64) {
    _upb_DefBuilder_Errf(ctx, "invalid type for field %s (%d)", f->full_name,
                         f->type_);
  }

  if (f->label_ < kUpb_Label_Optional || f->label_ > kUpb_Label_Repeated) {
    _upb_DefBuilder_Errf(ctx, "invalid label for field %s (%d)", f->full_name,
                         f->label_);
  }

  /* We can't resolve the subdef or (in the case of extensions) the containing
   * message yet, because it may not have been defined yet.  We stash a pointer
   * to the field_proto until later when we can properly resolve it. */
  f->sub.unresolved = field_proto;

  if (f->label_ == kUpb_Label_Required &&
      upb_FileDef_Syntax(f->file) == kUpb_Syntax_Proto3) {
    _upb_DefBuilder_Errf(ctx, "proto3 fields cannot be required (%s)",
                         f->full_name);
  }

  if (UPB_DESC(FieldDescriptorProto_has_oneof_index)(field_proto)) {
    uint32_t oneof_index =
        UPB_DESC(FieldDescriptorProto_oneof_index)(field_proto);

    if (upb_FieldDef_Label(f) != kUpb_Label_Optional) {
      _upb_DefBuilder_Errf(ctx, "fields in oneof must have OPTIONAL label (%s)",
                           f->full_name);
    }

    if (!m) {
      _upb_DefBuilder_Errf(ctx, "oneof field (%s) has no containing msg",
                           f->full_name);
    }

    if (oneof_index >= upb_MessageDef_OneofCount(m)) {
      _upb_DefBuilder_Errf(ctx, "oneof_index out of range (%s)", f->full_name);
    }

    upb_OneofDef* oneof = (upb_OneofDef*)upb_MessageDef_Oneof(m, oneof_index);
    f->scope.oneof = oneof;

    _upb_OneofDef_Insert(ctx, oneof, f, name.data, name.size);
  }

  UPB_DEF_SET_OPTIONS(f->opts, FieldDescriptorProto, FieldOptions, field_proto);

  if (UPB_DESC(FieldOptions_has_packed)(f->opts)) {
    f->is_packed = UPB_DESC(FieldOptions_packed)(f->opts);
  } else {
    // Repeated fields default to packed for proto3 only.
    f->is_packed = upb_FieldDef_IsPrimitive(f) &&
                   f->label_ == kUpb_Label_Repeated &&
                   upb_FileDef_Syntax(f->file) == kUpb_Syntax_Proto3;
  }

  f->has_presence =
      (!upb_FieldDef_IsRepeated(f)) &&
      (upb_FieldDef_IsSubMessage(f) || upb_FieldDef_ContainingOneof(f) ||
       (upb_FileDef_Syntax(f->file) == kUpb_Syntax_Proto2));
}

static void _upb_FieldDef_CreateExt(upb_DefBuilder* ctx, const char* prefix,
                                    const UPB_DESC(FieldDescriptorProto) *
                                        field_proto,
                                    upb_MessageDef* m, upb_FieldDef* f) {
  f->is_extension = true;
  _upb_FieldDef_Create(ctx, prefix, field_proto, m, f);

  if (UPB_DESC(FieldDescriptorProto_has_oneof_index)(field_proto)) {
    _upb_DefBuilder_Errf(ctx, "oneof_index provided for extension field (%s)",
                         f->full_name);
  }

  f->scope.extension_scope = m;
  _upb_DefBuilder_Add(ctx, f->full_name, _upb_DefType_Pack(f, UPB_DEFTYPE_EXT));
  f->layout_index = ctx->ext_count++;

  if (ctx->layout) {
    UPB_ASSERT(_upb_FieldDef_ExtensionMiniTable(f)->field.number == f->number_);
  }
}

static void _upb_FieldDef_CreateNotExt(upb_DefBuilder* ctx, const char* prefix,
                                       const UPB_DESC(FieldDescriptorProto) *
                                           field_proto,
                                       upb_MessageDef* m, upb_FieldDef* f) {
  f->is_extension = false;
  _upb_FieldDef_Create(ctx, prefix, field_proto, m, f);

  if (!UPB_DESC(FieldDescriptorProto_has_oneof_index)(field_proto)) {
    if (f->is_proto3_optional) {
      _upb_DefBuilder_Errf(
          ctx,
          "non-extension field (%s) with proto3_optional was not in a oneof",
          f->full_name);
    }
  }

  _upb_MessageDef_InsertField(ctx, m, f);
}

upb_FieldDef* _upb_Extensions_New(
    upb_DefBuilder* ctx, int n,
    const UPB_DESC(FieldDescriptorProto) * const* protos, const char* prefix,
    upb_MessageDef* m) {
  _upb_DefType_CheckPadding(sizeof(upb_FieldDef));
  upb_FieldDef* defs =
      (upb_FieldDef*)_upb_DefBuilder_Alloc(ctx, sizeof(upb_FieldDef) * n);

  for (int i = 0; i < n; i++) {
    upb_FieldDef* f = &defs[i];

    _upb_FieldDef_CreateExt(ctx, prefix, protos[i], m, f);
    f->index_ = i;
  }

  return defs;
}

upb_FieldDef* _upb_FieldDefs_New(
    upb_DefBuilder* ctx, int n,
    const UPB_DESC(FieldDescriptorProto) * const* protos, const char* prefix,
    upb_MessageDef* m, bool* is_sorted) {
  _upb_DefType_CheckPadding(sizeof(upb_FieldDef));
  upb_FieldDef* defs =
      (upb_FieldDef*)_upb_DefBuilder_Alloc(ctx, sizeof(upb_FieldDef) * n);

  uint32_t previous = 0;
  for (int i = 0; i < n; i++) {
    upb_FieldDef* f = &defs[i];

    _upb_FieldDef_CreateNotExt(ctx, prefix, protos[i], m, f);
    f->index_ = i;
    if (!ctx->layout) {
      // Speculate that the def fields are sorted.  We will always sort the
      // MiniTable fields, so if defs are sorted then indices will match.
      //
      // If this is incorrect, we will overwrite later.
      f->layout_index = i;
    }

    const uint32_t current = f->number_;
    if (previous > current) *is_sorted = false;
    previous = current;
  }

  return defs;
}

static void resolve_subdef(upb_DefBuilder* ctx, const char* prefix,
                           upb_FieldDef* f) {
  const UPB_DESC(FieldDescriptorProto)* field_proto = f->sub.unresolved;
  upb_StringView name = UPB_DESC(FieldDescriptorProto_type_name)(field_proto);
  bool has_name = UPB_DESC(FieldDescriptorProto_has_type_name)(field_proto);
  switch ((int)f->type_) {
    case UPB_FIELD_TYPE_UNSPECIFIED: {
      // Type was not specified and must be inferred.
      UPB_ASSERT(has_name);
      upb_deftype_t type;
      const void* def =
          _upb_DefBuilder_ResolveAny(ctx, f->full_name, prefix, name, &type);
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
          _upb_DefBuilder_Errf(ctx, "Couldn't resolve type name for field %s",
                               f->full_name);
      }
    }
    case kUpb_FieldType_Message:
    case kUpb_FieldType_Group:
      UPB_ASSERT(has_name);
      f->sub.msgdef = _upb_DefBuilder_Resolve(ctx, f->full_name, prefix, name,
                                              UPB_DEFTYPE_MSG);
      break;
    case kUpb_FieldType_Enum:
      UPB_ASSERT(has_name);
      f->sub.enumdef = _upb_DefBuilder_Resolve(ctx, f->full_name, prefix, name,
                                               UPB_DEFTYPE_ENUM);
      break;
    default:
      // No resolution necessary.
      break;
  }
}

static int _upb_FieldDef_Compare(const void* p1, const void* p2) {
  const uint32_t v1 = (*(upb_FieldDef**)p1)->number_;
  const uint32_t v2 = (*(upb_FieldDef**)p2)->number_;
  return (v1 < v2) ? -1 : (v1 > v2);
}

// _upb_FieldDefs_Sorted() is mostly a pure function of its inputs, but has one
// critical side effect that we depend on: it sets layout_index appropriately
// for non-sorted lists of fields.
const upb_FieldDef** _upb_FieldDefs_Sorted(const upb_FieldDef* f, int n,
                                           upb_Arena* a) {
  // TODO(salo): Replace this arena alloc with a persistent scratch buffer.
  upb_FieldDef** out = (upb_FieldDef**)upb_Arena_Malloc(a, n * sizeof(void*));
  if (!out) return NULL;

  for (int i = 0; i < n; i++) {
    out[i] = (upb_FieldDef*)&f[i];
  }
  qsort(out, n, sizeof(void*), _upb_FieldDef_Compare);

  for (int i = 0; i < n; i++) {
    out[i]->layout_index = i;
  }
  return (const upb_FieldDef**)out;
}

bool upb_FieldDef_MiniDescriptorEncode(const upb_FieldDef* f, upb_Arena* a,
                                       upb_StringView* out) {
  UPB_ASSERT(f->is_extension);

  upb_DescState s;
  _upb_DescState_Init(&s);

  const int number = upb_FieldDef_Number(f);
  const uint64_t modifiers = _upb_FieldDef_Modifiers(f);

  if (!_upb_DescState_Grow(&s, a)) return false;
  s.ptr = upb_MtDataEncoder_EncodeExtension(&s.e, s.ptr, f->type_, number,
                                            modifiers);
  *s.ptr = '\0';

  out->data = s.buf;
  out->size = s.ptr - s.buf;
  return true;
}

static void resolve_extension(upb_DefBuilder* ctx, const char* prefix,
                              upb_FieldDef* f,
                              const UPB_DESC(FieldDescriptorProto) *
                                  field_proto) {
  if (!UPB_DESC(FieldDescriptorProto_has_extendee)(field_proto)) {
    _upb_DefBuilder_Errf(ctx, "extension for field '%s' had no extendee",
                         f->full_name);
  }

  upb_StringView name = UPB_DESC(FieldDescriptorProto_extendee)(field_proto);
  const upb_MessageDef* m =
      _upb_DefBuilder_Resolve(ctx, f->full_name, prefix, name, UPB_DEFTYPE_MSG);
  f->msgdef = m;

  if (!_upb_MessageDef_IsValidExtensionNumber(m, f->number_)) {
    _upb_DefBuilder_Errf(
        ctx,
        "field number %u in extension %s has no extension range in message %s",
        (unsigned)f->number_, f->full_name, upb_MessageDef_FullName(m));
  }
}

void _upb_FieldDef_BuildMiniTableExtension(upb_DefBuilder* ctx,
                                           const upb_FieldDef* f) {
  const upb_MiniTableExtension* ext = _upb_FieldDef_ExtensionMiniTable(f);

  if (ctx->layout) {
    UPB_ASSERT(upb_FieldDef_Number(f) == ext->field.number);
  } else {
    upb_StringView desc;
    if (!upb_FieldDef_MiniDescriptorEncode(f, ctx->tmp_arena, &desc)) {
      _upb_DefBuilder_OomErr(ctx);
    }

    upb_MiniTableExtension* mut_ext = (upb_MiniTableExtension*)ext;
    upb_MiniTableSub sub = {NULL};
    if (upb_FieldDef_IsSubMessage(f)) {
      sub.submsg = upb_MessageDef_MiniTable(f->sub.msgdef);
    } else if (_upb_FieldDef_IsClosedEnum(f)) {
      sub.subenum = _upb_EnumDef_MiniTable(f->sub.enumdef);
    }
    bool ok2 = upb_MiniTableExtension_Init(desc.data, desc.size, mut_ext,
                                           upb_MessageDef_MiniTable(f->msgdef),
                                           sub, ctx->status);
    if (!ok2) _upb_DefBuilder_Errf(ctx, "Could not build extension mini table");
  }

  bool ok = _upb_DefPool_InsertExt(ctx->symtab, ext, f);
  if (!ok) _upb_DefBuilder_OomErr(ctx);
}

static void resolve_default(upb_DefBuilder* ctx, upb_FieldDef* f,
                            const UPB_DESC(FieldDescriptorProto) *
                                field_proto) {
  // Have to delay resolving of the default value until now because of the enum
  // case, since enum defaults are specified with a label.
  if (UPB_DESC(FieldDescriptorProto_has_default_value)(field_proto)) {
    upb_StringView defaultval =
        UPB_DESC(FieldDescriptorProto_default_value)(field_proto);

    if (upb_FileDef_Syntax(f->file) == kUpb_Syntax_Proto3) {
      _upb_DefBuilder_Errf(ctx,
                           "proto3 fields cannot have explicit defaults (%s)",
                           f->full_name);
    }

    if (upb_FieldDef_IsSubMessage(f)) {
      _upb_DefBuilder_Errf(ctx,
                           "message fields cannot have explicit defaults (%s)",
                           f->full_name);
    }

    parse_default(ctx, defaultval.data, defaultval.size, f);
    f->has_default = true;
  } else {
    set_default_default(ctx, f);
    f->has_default = false;
  }
}

void _upb_FieldDef_Resolve(upb_DefBuilder* ctx, const char* prefix,
                           upb_FieldDef* f) {
  // We have to stash this away since resolve_subdef() may overwrite it.
  const UPB_DESC(FieldDescriptorProto)* field_proto = f->sub.unresolved;

  resolve_subdef(ctx, prefix, f);
  resolve_default(ctx, f, field_proto);

  if (f->is_extension) {
    resolve_extension(ctx, prefix, f, field_proto);
  }
}


// Must be last.

struct upb_FileDef {
  const UPB_DESC(FileOptions) * opts;
  const char* name;
  const char* package;
  const char* edition;

  const upb_FileDef** deps;
  const int32_t* public_deps;
  const int32_t* weak_deps;
  const upb_MessageDef* top_lvl_msgs;
  const upb_EnumDef* top_lvl_enums;
  const upb_FieldDef* top_lvl_exts;
  const upb_ServiceDef* services;
  const upb_MiniTableExtension** ext_layouts;
  const upb_DefPool* symtab;

  int dep_count;
  int public_dep_count;
  int weak_dep_count;
  int top_lvl_msg_count;
  int top_lvl_enum_count;
  int top_lvl_ext_count;
  int service_count;
  int ext_count;  // All exts in the file.
  upb_Syntax syntax;
};

const UPB_DESC(FileOptions) * upb_FileDef_Options(const upb_FileDef* f) {
  return f->opts;
}

bool upb_FileDef_HasOptions(const upb_FileDef* f) {
  return f->opts != (void*)kUpbDefOptDefault;
}

const char* upb_FileDef_Name(const upb_FileDef* f) { return f->name; }

const char* upb_FileDef_Package(const upb_FileDef* f) {
  return f->package ? f->package : "";
}

const char* upb_FileDef_Edition(const upb_FileDef* f) {
  return f->edition ? f->edition : "";
}

const char* _upb_FileDef_RawPackage(const upb_FileDef* f) { return f->package; }

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
  return _upb_MessageDef_At(f->top_lvl_msgs, i);
}

const upb_EnumDef* upb_FileDef_TopLevelEnum(const upb_FileDef* f, int i) {
  UPB_ASSERT(0 <= i && i < f->top_lvl_enum_count);
  return _upb_EnumDef_At(f->top_lvl_enums, i);
}

const upb_FieldDef* upb_FileDef_TopLevelExtension(const upb_FileDef* f, int i) {
  UPB_ASSERT(0 <= i && i < f->top_lvl_ext_count);
  return _upb_FieldDef_At(f->top_lvl_exts, i);
}

const upb_ServiceDef* upb_FileDef_Service(const upb_FileDef* f, int i) {
  UPB_ASSERT(0 <= i && i < f->service_count);
  return _upb_ServiceDef_At(f->services, i);
}

const upb_DefPool* upb_FileDef_Pool(const upb_FileDef* f) { return f->symtab; }

const upb_MiniTableExtension* _upb_FileDef_ExtensionMiniTable(
    const upb_FileDef* f, int i) {
  return f->ext_layouts[i];
}

static char* strviewdup(upb_DefBuilder* ctx, upb_StringView view) {
  char* ret = upb_strdup2(view.data, view.size, _upb_DefBuilder_Arena(ctx));
  if (!ret) _upb_DefBuilder_OomErr(ctx);
  return ret;
}

static bool streql_view(upb_StringView view, const char* b) {
  return view.size == strlen(b) && memcmp(view.data, b, view.size) == 0;
}

static int count_exts_in_msg(const UPB_DESC(DescriptorProto) * msg_proto) {
  size_t n;
  UPB_DESC(DescriptorProto_extension)(msg_proto, &n);
  int ext_count = n;

  const UPB_DESC(DescriptorProto)* const* nested_msgs =
      UPB_DESC(DescriptorProto_nested_type)(msg_proto, &n);
  for (size_t i = 0; i < n; i++) {
    ext_count += count_exts_in_msg(nested_msgs[i]);
  }

  return ext_count;
}

// Allocate and initialize one file def, and add it to the context object.
void _upb_FileDef_Create(upb_DefBuilder* ctx,
                         const UPB_DESC(FileDescriptorProto) * file_proto) {
  upb_FileDef* file = _upb_DefBuilder_Alloc(ctx, sizeof(upb_FileDef));
  ctx->file = file;

  const UPB_DESC(DescriptorProto)* const* msgs;
  const UPB_DESC(EnumDescriptorProto)* const* enums;
  const UPB_DESC(FieldDescriptorProto)* const* exts;
  const UPB_DESC(ServiceDescriptorProto)* const* services;
  const upb_StringView* strs;
  const int32_t* public_deps;
  const int32_t* weak_deps;
  size_t n;

  file->symtab = ctx->symtab;

  // Count all extensions in the file, to build a flat array of layouts.
  UPB_DESC(FileDescriptorProto_extension)(file_proto, &n);
  int ext_count = n;
  msgs = UPB_DESC(FileDescriptorProto_message_type)(file_proto, &n);
  for (int i = 0; i < n; i++) {
    ext_count += count_exts_in_msg(msgs[i]);
  }
  file->ext_count = ext_count;

  if (ctx->layout) {
    // We are using the ext layouts that were passed in.
    file->ext_layouts = ctx->layout->exts;
    if (ctx->layout->ext_count != file->ext_count) {
      _upb_DefBuilder_Errf(ctx,
                           "Extension count did not match layout (%d vs %d)",
                           ctx->layout->ext_count, file->ext_count);
    }
  } else {
    // We are building ext layouts from scratch.
    file->ext_layouts = _upb_DefBuilder_Alloc(
        ctx, sizeof(*file->ext_layouts) * file->ext_count);
    upb_MiniTableExtension* ext =
        _upb_DefBuilder_Alloc(ctx, sizeof(*ext) * file->ext_count);
    for (int i = 0; i < file->ext_count; i++) {
      file->ext_layouts[i] = &ext[i];
    }
  }

  upb_StringView name = UPB_DESC(FileDescriptorProto_name)(file_proto);
  file->name = strviewdup(ctx, name);
  if (strlen(file->name) != name.size) {
    _upb_DefBuilder_Errf(ctx, "File name contained embedded NULL");
  }

  upb_StringView package = UPB_DESC(FileDescriptorProto_package)(file_proto);

  if (package.size) {
    _upb_DefBuilder_CheckIdentFull(ctx, package);
    file->package = strviewdup(ctx, package);
  } else {
    file->package = NULL;
  }

  upb_StringView edition = UPB_DESC(FileDescriptorProto_edition)(file_proto);

  if (edition.size == 0) {
    file->edition = NULL;
  } else {
    // TODO(b/267770604): How should we validate this?
    file->edition = strviewdup(ctx, edition);
    if (strlen(file->edition) != edition.size) {
      _upb_DefBuilder_Errf(ctx, "Edition name contained embedded NULL");
    }
  }

  if (UPB_DESC(FileDescriptorProto_has_syntax)(file_proto)) {
    upb_StringView syntax = UPB_DESC(FileDescriptorProto_syntax)(file_proto);

    if (streql_view(syntax, "proto2")) {
      file->syntax = kUpb_Syntax_Proto2;
    } else if (streql_view(syntax, "proto3")) {
      file->syntax = kUpb_Syntax_Proto3;
    } else {
      _upb_DefBuilder_Errf(ctx, "Invalid syntax '" UPB_STRINGVIEW_FORMAT "'",
                           UPB_STRINGVIEW_ARGS(syntax));
    }
  } else {
    file->syntax = kUpb_Syntax_Proto2;
  }

  // Read options.
  UPB_DEF_SET_OPTIONS(file->opts, FileDescriptorProto, FileOptions, file_proto);

  // Verify dependencies.
  strs = UPB_DESC(FileDescriptorProto_dependency)(file_proto, &n);
  file->dep_count = n;
  file->deps = _upb_DefBuilder_Alloc(ctx, sizeof(*file->deps) * n);

  for (size_t i = 0; i < n; i++) {
    upb_StringView str = strs[i];
    file->deps[i] =
        upb_DefPool_FindFileByNameWithSize(ctx->symtab, str.data, str.size);
    if (!file->deps[i]) {
      _upb_DefBuilder_Errf(ctx,
                           "Depends on file '" UPB_STRINGVIEW_FORMAT
                           "', but it has not been loaded",
                           UPB_STRINGVIEW_ARGS(str));
    }
  }

  public_deps = UPB_DESC(FileDescriptorProto_public_dependency)(file_proto, &n);
  file->public_dep_count = n;
  file->public_deps =
      _upb_DefBuilder_Alloc(ctx, sizeof(*file->public_deps) * n);
  int32_t* mutable_public_deps = (int32_t*)file->public_deps;
  for (size_t i = 0; i < n; i++) {
    if (public_deps[i] >= file->dep_count) {
      _upb_DefBuilder_Errf(ctx, "public_dep %d is out of range",
                           (int)public_deps[i]);
    }
    mutable_public_deps[i] = public_deps[i];
  }

  weak_deps = UPB_DESC(FileDescriptorProto_weak_dependency)(file_proto, &n);
  file->weak_dep_count = n;
  file->weak_deps = _upb_DefBuilder_Alloc(ctx, sizeof(*file->weak_deps) * n);
  int32_t* mutable_weak_deps = (int32_t*)file->weak_deps;
  for (size_t i = 0; i < n; i++) {
    if (weak_deps[i] >= file->dep_count) {
      _upb_DefBuilder_Errf(ctx, "weak_dep %d is out of range",
                           (int)weak_deps[i]);
    }
    mutable_weak_deps[i] = weak_deps[i];
  }

  // Create enums.
  enums = UPB_DESC(FileDescriptorProto_enum_type)(file_proto, &n);
  file->top_lvl_enum_count = n;
  file->top_lvl_enums = _upb_EnumDefs_New(ctx, n, enums, NULL);

  // Create extensions.
  exts = UPB_DESC(FileDescriptorProto_extension)(file_proto, &n);
  file->top_lvl_ext_count = n;
  file->top_lvl_exts = _upb_Extensions_New(ctx, n, exts, file->package, NULL);

  // Create messages.
  msgs = UPB_DESC(FileDescriptorProto_message_type)(file_proto, &n);
  file->top_lvl_msg_count = n;
  file->top_lvl_msgs = _upb_MessageDefs_New(ctx, n, msgs, NULL);

  // Create services.
  services = UPB_DESC(FileDescriptorProto_service)(file_proto, &n);
  file->service_count = n;
  file->services = _upb_ServiceDefs_New(ctx, n, services);

  // Now that all names are in the table, build layouts and resolve refs.

  for (int i = 0; i < file->top_lvl_msg_count; i++) {
    upb_MessageDef* m = (upb_MessageDef*)upb_FileDef_TopLevelMessage(file, i);
    _upb_MessageDef_Resolve(ctx, m);
  }

  for (int i = 0; i < file->top_lvl_ext_count; i++) {
    upb_FieldDef* f = (upb_FieldDef*)upb_FileDef_TopLevelExtension(file, i);
    _upb_FieldDef_Resolve(ctx, file->package, f);
  }

  for (int i = 0; i < file->top_lvl_msg_count; i++) {
    upb_MessageDef* m = (upb_MessageDef*)upb_FileDef_TopLevelMessage(file, i);
    _upb_MessageDef_CreateMiniTable(ctx, (upb_MessageDef*)m);
  }

  for (int i = 0; i < file->top_lvl_ext_count; i++) {
    upb_FieldDef* f = (upb_FieldDef*)upb_FileDef_TopLevelExtension(file, i);
    _upb_FieldDef_BuildMiniTableExtension(ctx, f);
  }

  for (int i = 0; i < file->top_lvl_msg_count; i++) {
    upb_MessageDef* m = (upb_MessageDef*)upb_FileDef_TopLevelMessage(file, i);
    _upb_MessageDef_LinkMiniTable(ctx, m);
  }

  if (file->ext_count) {
    bool ok = upb_ExtensionRegistry_AddArray(
        _upb_DefPool_ExtReg(ctx->symtab), file->ext_layouts, file->ext_count);
    if (!ok) _upb_DefBuilder_OomErr(ctx);
  }
}


#include <string.h>


// Must be last.

bool upb_Message_HasFieldByDef(const upb_Message* msg, const upb_FieldDef* f) {
  UPB_ASSERT(upb_FieldDef_HasPresence(f));
  return upb_Message_HasField(msg, upb_FieldDef_MiniTable(f));
}

const upb_FieldDef* upb_Message_WhichOneof(const upb_Message* msg,
                                           const upb_OneofDef* o) {
  const upb_FieldDef* f = upb_OneofDef_Field(o, 0);
  if (upb_OneofDef_IsSynthetic(o)) {
    UPB_ASSERT(upb_OneofDef_FieldCount(o) == 1);
    return upb_Message_HasFieldByDef(msg, f) ? f : NULL;
  } else {
    const upb_MiniTableField* field = upb_FieldDef_MiniTable(f);
    uint32_t oneof_case = upb_Message_WhichOneofFieldNumber(msg, field);
    f = oneof_case ? upb_OneofDef_LookupNumber(o, oneof_case) : NULL;
    UPB_ASSERT((f != NULL) == (oneof_case != 0));
    return f;
  }
}

upb_MessageValue upb_Message_GetFieldByDef(const upb_Message* msg,
                                           const upb_FieldDef* f) {
  upb_MessageValue default_val = upb_FieldDef_Default(f);
  upb_MessageValue ret;
  _upb_Message_GetField(msg, upb_FieldDef_MiniTable(f), &default_val, &ret);
  return ret;
}

upb_MutableMessageValue upb_Message_Mutable(upb_Message* msg,
                                            const upb_FieldDef* f,
                                            upb_Arena* a) {
  UPB_ASSERT(upb_FieldDef_IsSubMessage(f) || upb_FieldDef_IsRepeated(f));
  if (upb_FieldDef_HasPresence(f) && !upb_Message_HasFieldByDef(msg, f)) {
    // We need to skip the upb_Message_GetFieldByDef() call in this case.
    goto make;
  }

  upb_MessageValue val = upb_Message_GetFieldByDef(msg, f);
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
    const upb_MessageDef* m = upb_FieldDef_MessageSubDef(f);
    ret.msg = upb_Message_New(upb_MessageDef_MiniTable(m), a);
  }

  val.array_val = ret.array;
  upb_Message_SetFieldByDef(msg, f, val, a);

  return ret;
}

bool upb_Message_SetFieldByDef(upb_Message* msg, const upb_FieldDef* f,
                               upb_MessageValue val, upb_Arena* a) {
  return _upb_Message_SetField(msg, upb_FieldDef_MiniTable(f), &val, a);
}

void upb_Message_ClearFieldByDef(upb_Message* msg, const upb_FieldDef* f) {
  upb_Message_ClearField(msg, upb_FieldDef_MiniTable(f));
}

void upb_Message_ClearByDef(upb_Message* msg, const upb_MessageDef* m) {
  upb_Message_Clear(msg, upb_MessageDef_MiniTable(m));
}

bool upb_Message_Next(const upb_Message* msg, const upb_MessageDef* m,
                      const upb_DefPool* ext_pool, const upb_FieldDef** out_f,
                      upb_MessageValue* out_val, size_t* iter) {
  size_t i = *iter;
  size_t n = upb_MessageDef_FieldCount(m);
  UPB_UNUSED(ext_pool);

  // Iterate over normal fields, returning the first one that is set.
  while (++i < n) {
    const upb_FieldDef* f = upb_MessageDef_Field(m, i);
    const upb_MiniTableField* field = upb_FieldDef_MiniTable(f);
    upb_MessageValue val = upb_Message_GetFieldByDef(msg, f);

    // Skip field if unset or empty.
    if (upb_MiniTableField_HasPresence(field)) {
      if (!upb_Message_HasFieldByDef(msg, f)) continue;
    } else {
      switch (upb_FieldMode_Get(field)) {
        case kUpb_FieldMode_Map:
          if (!val.map_val || upb_Map_Size(val.map_val) == 0) continue;
          break;
        case kUpb_FieldMode_Array:
          if (!val.array_val || upb_Array_Size(val.array_val) == 0) continue;
          break;
        case kUpb_FieldMode_Scalar:
          if (!_upb_MiniTable_ValueIsNonZero(&val, field)) continue;
          break;
      }
    }

    *out_val = val;
    *out_f = f;
    *iter = i;
    return true;
  }

  if (ext_pool) {
    // Return any extensions that are set.
    size_t count;
    const upb_Message_Extension* ext = _upb_Message_Getexts(msg, &count);
    if (i - n < count) {
      ext += count - 1 - (i - n);
      memcpy(out_val, &ext->data, sizeof(*out_val));
      *out_f = upb_DefPool_FindExtensionByMiniTable(ext_pool, ext->ext);
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

      upb_MessageValue map_key, map_val;
      while (upb_Map_Next(map, &map_key, &map_val, &iter)) {
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


// Must be last.

struct upb_MessageDef {
  const UPB_DESC(MessageOptions) * opts;
  const upb_MiniTable* layout;
  const upb_FileDef* file;
  const upb_MessageDef* containing_type;
  const char* full_name;

  // Tables for looking up fields by number and name.
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

  // TODO(salo): These counters don't need anywhere near 32 bits.
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
  bool is_sorted;
  upb_WellKnown well_known_type;
#if UINTPTR_MAX == 0xffffffff
  uint32_t padding;  // Increase size to a multiple of 8.
#endif
};

static void assign_msg_wellknowntype(upb_MessageDef* m) {
  const char* name = m->full_name;
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

upb_MessageDef* _upb_MessageDef_At(const upb_MessageDef* m, int i) {
  return (upb_MessageDef*)&m[i];
}

bool _upb_MessageDef_IsValidExtensionNumber(const upb_MessageDef* m, int n) {
  for (int i = 0; i < m->ext_range_count; i++) {
    const upb_ExtensionRange* r = upb_MessageDef_ExtensionRange(m, i);
    if (upb_ExtensionRange_Start(r) <= n && n < upb_ExtensionRange_End(r)) {
      return true;
    }
  }
  return false;
}

const UPB_DESC(MessageOptions) *
    upb_MessageDef_Options(const upb_MessageDef* m) {
  return m->opts;
}

bool upb_MessageDef_HasOptions(const upb_MessageDef* m) {
  return m->opts != (void*)kUpbDefOptDefault;
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
  return _upb_DefBuilder_FullToShort(m->full_name);
}

upb_Syntax upb_MessageDef_Syntax(const upb_MessageDef* m) {
  return upb_FileDef_Syntax(m->file);
}

const upb_FieldDef* upb_MessageDef_FindFieldByNumber(const upb_MessageDef* m,
                                                     uint32_t i) {
  upb_value val;
  return upb_inttable_lookup(&m->itof, i, &val) ? upb_value_getconstptr(val)
                                                : NULL;
}

const upb_FieldDef* upb_MessageDef_FindFieldByNameWithSize(
    const upb_MessageDef* m, const char* name, size_t size) {
  upb_value val;

  if (!upb_strtable_lookup2(&m->ntof, name, size, &val)) {
    return NULL;
  }

  return _upb_DefType_Unpack(val, UPB_DEFTYPE_FIELD);
}

const upb_OneofDef* upb_MessageDef_FindOneofByNameWithSize(
    const upb_MessageDef* m, const char* name, size_t size) {
  upb_value val;

  if (!upb_strtable_lookup2(&m->ntof, name, size, &val)) {
    return NULL;
  }

  return _upb_DefType_Unpack(val, UPB_DEFTYPE_ONEOF);
}

bool _upb_MessageDef_Insert(upb_MessageDef* m, const char* name, size_t len,
                            upb_value v, upb_Arena* a) {
  return upb_strtable_insert(&m->ntof, name, len, v, a);
}

bool upb_MessageDef_FindByNameWithSize(const upb_MessageDef* m,
                                       const char* name, size_t len,
                                       const upb_FieldDef** out_f,
                                       const upb_OneofDef** out_o) {
  upb_value val;

  if (!upb_strtable_lookup2(&m->ntof, name, len, &val)) {
    return false;
  }

  const upb_FieldDef* f = _upb_DefType_Unpack(val, UPB_DEFTYPE_FIELD);
  const upb_OneofDef* o = _upb_DefType_Unpack(val, UPB_DEFTYPE_ONEOF);
  if (out_f) *out_f = f;
  if (out_o) *out_o = o;
  return f || o; /* False if this was a JSON name. */
}

const upb_FieldDef* upb_MessageDef_FindByJsonNameWithSize(
    const upb_MessageDef* m, const char* name, size_t size) {
  upb_value val;
  const upb_FieldDef* f;

  if (!upb_strtable_lookup2(&m->ntof, name, size, &val)) {
    return NULL;
  }

  f = _upb_DefType_Unpack(val, UPB_DEFTYPE_FIELD);
  if (!f) f = _upb_DefType_Unpack(val, UPB_DEFTYPE_FIELD_JSONNAME);

  return f;
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

int upb_MessageDef_RealOneofCount(const upb_MessageDef* m) {
  return m->real_oneof_count;
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

const upb_MiniTable* upb_MessageDef_MiniTable(const upb_MessageDef* m) {
  return m->layout;
}

const upb_ExtensionRange* upb_MessageDef_ExtensionRange(const upb_MessageDef* m,
                                                        int i) {
  UPB_ASSERT(0 <= i && i < m->ext_range_count);
  return _upb_ExtensionRange_At(m->ext_ranges, i);
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

const upb_FieldDef* upb_MessageDef_Field(const upb_MessageDef* m, int i) {
  UPB_ASSERT(0 <= i && i < m->field_count);
  return _upb_FieldDef_At(m->fields, i);
}

const upb_OneofDef* upb_MessageDef_Oneof(const upb_MessageDef* m, int i) {
  UPB_ASSERT(0 <= i && i < m->oneof_count);
  return _upb_OneofDef_At(m->oneofs, i);
}

const upb_MessageDef* upb_MessageDef_NestedMessage(const upb_MessageDef* m,
                                                   int i) {
  UPB_ASSERT(0 <= i && i < m->nested_msg_count);
  return &m->nested_msgs[i];
}

const upb_EnumDef* upb_MessageDef_NestedEnum(const upb_MessageDef* m, int i) {
  UPB_ASSERT(0 <= i && i < m->nested_enum_count);
  return _upb_EnumDef_At(m->nested_enums, i);
}

const upb_FieldDef* upb_MessageDef_NestedExtension(const upb_MessageDef* m,
                                                   int i) {
  UPB_ASSERT(0 <= i && i < m->nested_ext_count);
  return _upb_FieldDef_At(m->nested_exts, i);
}

upb_WellKnown upb_MessageDef_WellKnownType(const upb_MessageDef* m) {
  return m->well_known_type;
}

bool _upb_MessageDef_InMessageSet(const upb_MessageDef* m) {
  return m->in_message_set;
}

const upb_FieldDef* upb_MessageDef_FindFieldByName(const upb_MessageDef* m,
                                                   const char* name) {
  return upb_MessageDef_FindFieldByNameWithSize(m, name, strlen(name));
}

const upb_OneofDef* upb_MessageDef_FindOneofByName(const upb_MessageDef* m,
                                                   const char* name) {
  return upb_MessageDef_FindOneofByNameWithSize(m, name, strlen(name));
}

bool upb_MessageDef_IsMapEntry(const upb_MessageDef* m) {
  return UPB_DESC(MessageOptions_map_entry)(m->opts);
}

bool upb_MessageDef_IsMessageSet(const upb_MessageDef* m) {
  return UPB_DESC(MessageOptions_message_set_wire_format)(m->opts);
}

static upb_MiniTable* _upb_MessageDef_MakeMiniTable(upb_DefBuilder* ctx,
                                                    const upb_MessageDef* m) {
  upb_StringView desc;
  // Note: this will assign layout_index for fields, so upb_FieldDef_MiniTable()
  // is safe to call only after this call.
  bool ok = upb_MessageDef_MiniDescriptorEncode(m, ctx->tmp_arena, &desc);
  if (!ok) _upb_DefBuilder_OomErr(ctx);

  void** scratch_data = _upb_DefPool_ScratchData(ctx->symtab);
  size_t* scratch_size = _upb_DefPool_ScratchSize(ctx->symtab);
  upb_MiniTable* ret = upb_MiniTable_BuildWithBuf(
      desc.data, desc.size, ctx->platform, ctx->arena, scratch_data,
      scratch_size, ctx->status);
  if (!ret) _upb_DefBuilder_FailJmp(ctx);

  return ret;
}

void _upb_MessageDef_Resolve(upb_DefBuilder* ctx, upb_MessageDef* m) {
  for (int i = 0; i < m->field_count; i++) {
    upb_FieldDef* f = (upb_FieldDef*)upb_MessageDef_Field(m, i);
    _upb_FieldDef_Resolve(ctx, m->full_name, f);
  }

  m->in_message_set = false;
  for (int i = 0; i < upb_MessageDef_NestedExtensionCount(m); i++) {
    upb_FieldDef* ext = (upb_FieldDef*)upb_MessageDef_NestedExtension(m, i);
    _upb_FieldDef_Resolve(ctx, m->full_name, ext);
    if (upb_FieldDef_Type(ext) == kUpb_FieldType_Message &&
        upb_FieldDef_Label(ext) == kUpb_Label_Optional &&
        upb_FieldDef_MessageSubDef(ext) == m &&
        UPB_DESC(MessageOptions_message_set_wire_format)(
            upb_MessageDef_Options(upb_FieldDef_ContainingType(ext)))) {
      m->in_message_set = true;
    }
  }

  for (int i = 0; i < upb_MessageDef_NestedMessageCount(m); i++) {
    upb_MessageDef* n = (upb_MessageDef*)upb_MessageDef_NestedMessage(m, i);
    _upb_MessageDef_Resolve(ctx, n);
  }
}

void _upb_MessageDef_InsertField(upb_DefBuilder* ctx, upb_MessageDef* m,
                                 const upb_FieldDef* f) {
  const int32_t field_number = upb_FieldDef_Number(f);

  if (field_number <= 0 || field_number > kUpb_MaxFieldNumber) {
    _upb_DefBuilder_Errf(ctx, "invalid field number (%u)", field_number);
  }

  const char* json_name = upb_FieldDef_JsonName(f);
  const char* shortname = upb_FieldDef_Name(f);
  const size_t shortnamelen = strlen(shortname);

  upb_value v = upb_value_constptr(f);

  upb_value existing_v;
  if (upb_strtable_lookup(&m->ntof, shortname, &existing_v)) {
    _upb_DefBuilder_Errf(ctx, "duplicate field name (%s)", shortname);
  }

  const upb_value field_v = _upb_DefType_Pack(f, UPB_DEFTYPE_FIELD);
  bool ok =
      _upb_MessageDef_Insert(m, shortname, shortnamelen, field_v, ctx->arena);
  if (!ok) _upb_DefBuilder_OomErr(ctx);

  if (strcmp(shortname, json_name) != 0) {
    if (upb_strtable_lookup(&m->ntof, json_name, &v)) {
      _upb_DefBuilder_Errf(ctx, "duplicate json_name (%s)", json_name);
    }

    const size_t json_size = strlen(json_name);
    const upb_value json_v = _upb_DefType_Pack(f, UPB_DEFTYPE_FIELD_JSONNAME);
    ok = _upb_MessageDef_Insert(m, json_name, json_size, json_v, ctx->arena);
    if (!ok) _upb_DefBuilder_OomErr(ctx);
  }

  if (upb_inttable_lookup(&m->itof, field_number, NULL)) {
    _upb_DefBuilder_Errf(ctx, "duplicate field number (%u)", field_number);
  }

  ok = upb_inttable_insert(&m->itof, field_number, v, ctx->arena);
  if (!ok) _upb_DefBuilder_OomErr(ctx);
}

void _upb_MessageDef_CreateMiniTable(upb_DefBuilder* ctx, upb_MessageDef* m) {
  if (ctx->layout == NULL) {
    m->layout = _upb_MessageDef_MakeMiniTable(ctx, m);
  } else {
    UPB_ASSERT(ctx->msg_count < ctx->layout->msg_count);
    m->layout = ctx->layout->msgs[ctx->msg_count++];
    UPB_ASSERT(m->field_count == m->layout->field_count);

    // We don't need the result of this call, but it will assign layout_index
    // for all the fields in O(n lg n) time.
    _upb_FieldDefs_Sorted(m->fields, m->field_count, ctx->tmp_arena);
  }

  for (int i = 0; i < m->nested_msg_count; i++) {
    upb_MessageDef* nested =
        (upb_MessageDef*)upb_MessageDef_NestedMessage(m, i);
    _upb_MessageDef_CreateMiniTable(ctx, nested);
  }
}

void _upb_MessageDef_LinkMiniTable(upb_DefBuilder* ctx,
                                   const upb_MessageDef* m) {
  for (int i = 0; i < upb_MessageDef_NestedExtensionCount(m); i++) {
    const upb_FieldDef* ext = upb_MessageDef_NestedExtension(m, i);
    _upb_FieldDef_BuildMiniTableExtension(ctx, ext);
  }

  for (int i = 0; i < m->nested_msg_count; i++) {
    _upb_MessageDef_LinkMiniTable(ctx, upb_MessageDef_NestedMessage(m, i));
  }

  if (ctx->layout) return;

  for (int i = 0; i < m->field_count; i++) {
    const upb_FieldDef* f = upb_MessageDef_Field(m, i);
    const upb_MessageDef* sub_m = upb_FieldDef_MessageSubDef(f);
    const upb_EnumDef* sub_e = upb_FieldDef_EnumSubDef(f);
    const int layout_index = _upb_FieldDef_LayoutIndex(f);
    upb_MiniTable* mt = (upb_MiniTable*)upb_MessageDef_MiniTable(m);

    UPB_ASSERT(layout_index < m->field_count);
    upb_MiniTableField* mt_f =
        (upb_MiniTableField*)&m->layout->fields[layout_index];
    if (sub_m) {
      if (!mt->subs) {
        _upb_DefBuilder_Errf(ctx, "unexpected submsg for (%s)", m->full_name);
      }
      UPB_ASSERT(mt_f);
      UPB_ASSERT(sub_m->layout);
      if (UPB_UNLIKELY(!upb_MiniTable_SetSubMessage(mt, mt_f, sub_m->layout))) {
        _upb_DefBuilder_Errf(ctx, "invalid submsg for (%s)", m->full_name);
      }
    } else if (_upb_FieldDef_IsClosedEnum(f)) {
      const upb_MiniTableEnum* mt_e = _upb_EnumDef_MiniTable(sub_e);
      if (UPB_UNLIKELY(!upb_MiniTable_SetSubEnum(mt, mt_f, mt_e))) {
        _upb_DefBuilder_Errf(ctx, "invalid subenum for (%s)", m->full_name);
      }
    }
  }

#ifndef NDEBUG
  for (int i = 0; i < m->field_count; i++) {
    const upb_FieldDef* f = upb_MessageDef_Field(m, i);
    const int layout_index = _upb_FieldDef_LayoutIndex(f);
    UPB_ASSERT(layout_index < m->layout->field_count);
    const upb_MiniTableField* mt_f = &m->layout->fields[layout_index];
    UPB_ASSERT(upb_FieldDef_Type(f) == upb_MiniTableField_Type(mt_f));
    UPB_ASSERT(upb_FieldDef_CType(f) == upb_MiniTableField_CType(mt_f));
    UPB_ASSERT(upb_FieldDef_HasPresence(f) ==
               upb_MiniTableField_HasPresence(mt_f));
  }
#endif
}

static uint64_t _upb_MessageDef_Modifiers(const upb_MessageDef* m) {
  uint64_t out = 0;
  if (upb_FileDef_Syntax(m->file) == kUpb_Syntax_Proto3) {
    out |= kUpb_MessageModifier_ValidateUtf8;
    out |= kUpb_MessageModifier_DefaultIsPacked;
  }
  if (m->ext_range_count) {
    out |= kUpb_MessageModifier_IsExtendable;
  }
  return out;
}

static bool _upb_MessageDef_EncodeMap(upb_DescState* s, const upb_MessageDef* m,
                                      upb_Arena* a) {
  if (m->field_count != 2) return false;

  const upb_FieldDef* key_field = upb_MessageDef_Field(m, 0);
  const upb_FieldDef* val_field = upb_MessageDef_Field(m, 1);
  if (key_field == NULL || val_field == NULL) return false;

  UPB_ASSERT(_upb_FieldDef_LayoutIndex(key_field) == 0);
  UPB_ASSERT(_upb_FieldDef_LayoutIndex(val_field) == 1);

  s->ptr = upb_MtDataEncoder_EncodeMap(
      &s->e, s->ptr, upb_FieldDef_Type(key_field), upb_FieldDef_Type(val_field),
      _upb_FieldDef_Modifiers(key_field), _upb_FieldDef_Modifiers(val_field));
  return true;
}

static bool _upb_MessageDef_EncodeMessage(upb_DescState* s,
                                          const upb_MessageDef* m,
                                          upb_Arena* a) {
  const upb_FieldDef** sorted = NULL;
  if (!m->is_sorted) {
    sorted = _upb_FieldDefs_Sorted(m->fields, m->field_count, a);
    if (!sorted) return false;
  }

  s->ptr = upb_MtDataEncoder_StartMessage(&s->e, s->ptr,
                                          _upb_MessageDef_Modifiers(m));

  for (int i = 0; i < m->field_count; i++) {
    const upb_FieldDef* f = sorted ? sorted[i] : upb_MessageDef_Field(m, i);
    const upb_FieldType type = upb_FieldDef_Type(f);
    const int number = upb_FieldDef_Number(f);
    const uint64_t modifiers = _upb_FieldDef_Modifiers(f);

    if (!_upb_DescState_Grow(s, a)) return false;
    s->ptr = upb_MtDataEncoder_PutField(&s->e, s->ptr, type, number, modifiers);
  }

  for (int i = 0; i < m->real_oneof_count; i++) {
    if (!_upb_DescState_Grow(s, a)) return false;
    s->ptr = upb_MtDataEncoder_StartOneof(&s->e, s->ptr);

    const upb_OneofDef* o = upb_MessageDef_Oneof(m, i);
    const int field_count = upb_OneofDef_FieldCount(o);
    for (int j = 0; j < field_count; j++) {
      const int number = upb_FieldDef_Number(upb_OneofDef_Field(o, j));

      if (!_upb_DescState_Grow(s, a)) return false;
      s->ptr = upb_MtDataEncoder_PutOneofField(&s->e, s->ptr, number);
    }
  }

  return true;
}

static bool _upb_MessageDef_EncodeMessageSet(upb_DescState* s,
                                             const upb_MessageDef* m,
                                             upb_Arena* a) {
  s->ptr = upb_MtDataEncoder_EncodeMessageSet(&s->e, s->ptr);

  return true;
}

bool upb_MessageDef_MiniDescriptorEncode(const upb_MessageDef* m, upb_Arena* a,
                                         upb_StringView* out) {
  upb_DescState s;
  _upb_DescState_Init(&s);

  if (!_upb_DescState_Grow(&s, a)) return false;

  if (upb_MessageDef_IsMapEntry(m)) {
    if (!_upb_MessageDef_EncodeMap(&s, m, a)) return false;
  } else if (UPB_DESC(MessageOptions_message_set_wire_format)(m->opts)) {
    if (!_upb_MessageDef_EncodeMessageSet(&s, m, a)) return false;
  } else {
    if (!_upb_MessageDef_EncodeMessage(&s, m, a)) return false;
  }

  if (!_upb_DescState_Grow(&s, a)) return false;
  *s.ptr = '\0';

  out->data = s.buf;
  out->size = s.ptr - s.buf;
  return true;
}

static upb_StringView* _upb_ReservedNames_New(upb_DefBuilder* ctx, int n,
                                              const upb_StringView* protos) {
  upb_StringView* sv = _upb_DefBuilder_Alloc(ctx, sizeof(upb_StringView) * n);
  for (size_t i = 0; i < n; i++) {
    sv[i].data =
        upb_strdup2(protos[i].data, protos[i].size, _upb_DefBuilder_Arena(ctx));
    sv[i].size = protos[i].size;
  }
  return sv;
}

static void create_msgdef(upb_DefBuilder* ctx, const char* prefix,
                          const UPB_DESC(DescriptorProto) * msg_proto,
                          const upb_MessageDef* containing_type,
                          upb_MessageDef* m) {
  const UPB_DESC(OneofDescriptorProto)* const* oneofs;
  const UPB_DESC(FieldDescriptorProto)* const* fields;
  const UPB_DESC(DescriptorProto_ExtensionRange)* const* ext_ranges;
  const UPB_DESC(DescriptorProto_ReservedRange)* const* res_ranges;
  const upb_StringView* res_names;
  size_t n_oneof, n_field, n_enum, n_ext, n_msg;
  size_t n_ext_range, n_res_range, n_res_name;
  upb_StringView name;

  // Must happen before _upb_DefBuilder_Add()
  m->file = _upb_DefBuilder_File(ctx);

  m->containing_type = containing_type;
  m->is_sorted = true;

  name = UPB_DESC(DescriptorProto_name)(msg_proto);

  m->full_name = _upb_DefBuilder_MakeFullName(ctx, prefix, name);
  _upb_DefBuilder_Add(ctx, m->full_name, _upb_DefType_Pack(m, UPB_DEFTYPE_MSG));

  oneofs = UPB_DESC(DescriptorProto_oneof_decl)(msg_proto, &n_oneof);
  fields = UPB_DESC(DescriptorProto_field)(msg_proto, &n_field);
  ext_ranges =
      UPB_DESC(DescriptorProto_extension_range)(msg_proto, &n_ext_range);
  res_ranges =
      UPB_DESC(DescriptorProto_reserved_range)(msg_proto, &n_res_range);
  res_names = UPB_DESC(DescriptorProto_reserved_name)(msg_proto, &n_res_name);

  bool ok = upb_inttable_init(&m->itof, ctx->arena);
  if (!ok) _upb_DefBuilder_OomErr(ctx);

  ok = upb_strtable_init(&m->ntof, n_oneof + n_field, ctx->arena);
  if (!ok) _upb_DefBuilder_OomErr(ctx);

  UPB_DEF_SET_OPTIONS(m->opts, DescriptorProto, MessageOptions, msg_proto);

  m->oneof_count = n_oneof;
  m->oneofs = _upb_OneofDefs_New(ctx, n_oneof, oneofs, m);

  m->field_count = n_field;
  m->fields =
      _upb_FieldDefs_New(ctx, n_field, fields, m->full_name, m, &m->is_sorted);

  // Message Sets may not contain fields.
  if (UPB_UNLIKELY(UPB_DESC(MessageOptions_message_set_wire_format)(m->opts))) {
    if (UPB_UNLIKELY(n_field > 0)) {
      _upb_DefBuilder_Errf(ctx, "invalid message set (%s)", m->full_name);
    }
  }

  m->ext_range_count = n_ext_range;
  m->ext_ranges = _upb_ExtensionRanges_New(ctx, n_ext_range, ext_ranges, m);

  m->res_range_count = n_res_range;
  m->res_ranges =
      _upb_MessageReservedRanges_New(ctx, n_res_range, res_ranges, m);

  m->res_name_count = n_res_name;
  m->res_names = _upb_ReservedNames_New(ctx, n_res_name, res_names);

  const size_t synthetic_count = _upb_OneofDefs_Finalize(ctx, m);
  m->real_oneof_count = m->oneof_count - synthetic_count;

  assign_msg_wellknowntype(m);
  upb_inttable_compact(&m->itof, ctx->arena);

  const UPB_DESC(EnumDescriptorProto)* const* enums =
      UPB_DESC(DescriptorProto_enum_type)(msg_proto, &n_enum);
  m->nested_enum_count = n_enum;
  m->nested_enums = _upb_EnumDefs_New(ctx, n_enum, enums, m);

  const UPB_DESC(FieldDescriptorProto)* const* exts =
      UPB_DESC(DescriptorProto_extension)(msg_proto, &n_ext);
  m->nested_ext_count = n_ext;
  m->nested_exts = _upb_Extensions_New(ctx, n_ext, exts, m->full_name, m);

  const UPB_DESC(DescriptorProto)* const* msgs =
      UPB_DESC(DescriptorProto_nested_type)(msg_proto, &n_msg);
  m->nested_msg_count = n_msg;
  m->nested_msgs = _upb_MessageDefs_New(ctx, n_msg, msgs, m);
}

// Allocate and initialize an array of |n| message defs.
upb_MessageDef* _upb_MessageDefs_New(
    upb_DefBuilder* ctx, int n, const UPB_DESC(DescriptorProto) * const* protos,
    const upb_MessageDef* containing_type) {
  _upb_DefType_CheckPadding(sizeof(upb_MessageDef));

  const char* name = containing_type ? containing_type->full_name
                                     : _upb_FileDef_RawPackage(ctx->file);

  upb_MessageDef* m = _upb_DefBuilder_Alloc(ctx, sizeof(upb_MessageDef) * n);
  for (int i = 0; i < n; i++) {
    create_msgdef(ctx, name, protos[i], containing_type, &m[i]);
  }
  return m;
}


// Must be last.

struct upb_MessageReservedRange {
  int32_t start;
  int32_t end;
};

upb_MessageReservedRange* _upb_MessageReservedRange_At(
    const upb_MessageReservedRange* r, int i) {
  return (upb_MessageReservedRange*)&r[i];
}

int32_t upb_MessageReservedRange_Start(const upb_MessageReservedRange* r) {
  return r->start;
}
int32_t upb_MessageReservedRange_End(const upb_MessageReservedRange* r) {
  return r->end;
}

upb_MessageReservedRange* _upb_MessageReservedRanges_New(
    upb_DefBuilder* ctx, int n,
    const UPB_DESC(DescriptorProto_ReservedRange) * const* protos,
    const upb_MessageDef* m) {
  upb_MessageReservedRange* r =
      _upb_DefBuilder_Alloc(ctx, sizeof(upb_MessageReservedRange) * n);

  for (int i = 0; i < n; i++) {
    const int32_t start =
        UPB_DESC(DescriptorProto_ReservedRange_start)(protos[i]);
    const int32_t end = UPB_DESC(DescriptorProto_ReservedRange_end)(protos[i]);
    const int32_t max = kUpb_MaxFieldNumber + 1;

    // A full validation would also check that each range is disjoint, and that
    // none of the fields overlap with the extension ranges, but we are just
    // sanity checking here.
    if (start < 1 || end <= start || end > max) {
      _upb_DefBuilder_Errf(ctx,
                           "Reserved range (%d, %d) is invalid, message=%s\n",
                           (int)start, (int)end, upb_MessageDef_FullName(m));
    }

    r[i].start = start;
    r[i].end = end;
  }

  return r;
}


// Must be last.

struct upb_MethodDef {
  const UPB_DESC(MethodOptions) * opts;
  upb_ServiceDef* service;
  const char* full_name;
  const upb_MessageDef* input_type;
  const upb_MessageDef* output_type;
  int index;
  bool client_streaming;
  bool server_streaming;
};

upb_MethodDef* _upb_MethodDef_At(const upb_MethodDef* m, int i) {
  return (upb_MethodDef*)&m[i];
}

const upb_ServiceDef* upb_MethodDef_Service(const upb_MethodDef* m) {
  return m->service;
}

const UPB_DESC(MethodOptions) * upb_MethodDef_Options(const upb_MethodDef* m) {
  return m->opts;
}

bool upb_MethodDef_HasOptions(const upb_MethodDef* m) {
  return m->opts != (void*)kUpbDefOptDefault;
}

const char* upb_MethodDef_FullName(const upb_MethodDef* m) {
  return m->full_name;
}

const char* upb_MethodDef_Name(const upb_MethodDef* m) {
  return _upb_DefBuilder_FullToShort(m->full_name);
}

int upb_MethodDef_Index(const upb_MethodDef* m) { return m->index; }

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

static void create_method(upb_DefBuilder* ctx,
                          const UPB_DESC(MethodDescriptorProto) * method_proto,
                          upb_ServiceDef* s, upb_MethodDef* m) {
  upb_StringView name = UPB_DESC(MethodDescriptorProto_name)(method_proto);

  m->service = s;
  m->full_name =
      _upb_DefBuilder_MakeFullName(ctx, upb_ServiceDef_FullName(s), name);
  m->client_streaming =
      UPB_DESC(MethodDescriptorProto_client_streaming)(method_proto);
  m->server_streaming =
      UPB_DESC(MethodDescriptorProto_server_streaming)(method_proto);
  m->input_type = _upb_DefBuilder_Resolve(
      ctx, m->full_name, m->full_name,
      UPB_DESC(MethodDescriptorProto_input_type)(method_proto),
      UPB_DEFTYPE_MSG);
  m->output_type = _upb_DefBuilder_Resolve(
      ctx, m->full_name, m->full_name,
      UPB_DESC(MethodDescriptorProto_output_type)(method_proto),
      UPB_DEFTYPE_MSG);

  UPB_DEF_SET_OPTIONS(m->opts, MethodDescriptorProto, MethodOptions,
                      method_proto);
}

// Allocate and initialize an array of |n| method defs belonging to |s|.
upb_MethodDef* _upb_MethodDefs_New(
    upb_DefBuilder* ctx, int n,
    const UPB_DESC(MethodDescriptorProto) * const* protos, upb_ServiceDef* s) {
  upb_MethodDef* m = _upb_DefBuilder_Alloc(ctx, sizeof(upb_MethodDef) * n);
  for (int i = 0; i < n; i++) {
    create_method(ctx, protos[i], s, &m[i]);
    m[i].index = i;
  }
  return m;
}

#include <ctype.h>
#include <stdlib.h>
#include <string.h>


// Must be last.

struct upb_OneofDef {
  const UPB_DESC(OneofOptions) * opts;
  const upb_MessageDef* parent;
  const char* full_name;
  int field_count;
  bool synthetic;
  const upb_FieldDef** fields;
  upb_strtable ntof;  // lookup a field by name
  upb_inttable itof;  // lookup a field by number (index)
#if UINTPTR_MAX == 0xffffffff
  uint32_t padding;  // Increase size to a multiple of 8.
#endif
};

upb_OneofDef* _upb_OneofDef_At(const upb_OneofDef* o, int i) {
  return (upb_OneofDef*)&o[i];
}

const UPB_DESC(OneofOptions) * upb_OneofDef_Options(const upb_OneofDef* o) {
  return o->opts;
}

bool upb_OneofDef_HasOptions(const upb_OneofDef* o) {
  return o->opts != (void*)kUpbDefOptDefault;
}

const char* upb_OneofDef_FullName(const upb_OneofDef* o) {
  return o->full_name;
}

const char* upb_OneofDef_Name(const upb_OneofDef* o) {
  return _upb_DefBuilder_FullToShort(o->full_name);
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
  return o - upb_MessageDef_Oneof(o->parent, 0);
}

bool upb_OneofDef_IsSynthetic(const upb_OneofDef* o) { return o->synthetic; }

const upb_FieldDef* upb_OneofDef_LookupNameWithSize(const upb_OneofDef* o,
                                                    const char* name,
                                                    size_t size) {
  upb_value val;
  return upb_strtable_lookup2(&o->ntof, name, size, &val)
             ? upb_value_getptr(val)
             : NULL;
}

const upb_FieldDef* upb_OneofDef_LookupName(const upb_OneofDef* o,
                                            const char* name) {
  return upb_OneofDef_LookupNameWithSize(o, name, strlen(name));
}

const upb_FieldDef* upb_OneofDef_LookupNumber(const upb_OneofDef* o,
                                              uint32_t num) {
  upb_value val;
  return upb_inttable_lookup(&o->itof, num, &val) ? upb_value_getptr(val)
                                                  : NULL;
}

void _upb_OneofDef_Insert(upb_DefBuilder* ctx, upb_OneofDef* o,
                          const upb_FieldDef* f, const char* name,
                          size_t size) {
  o->field_count++;
  if (_upb_FieldDef_IsProto3Optional(f)) o->synthetic = true;

  const int number = upb_FieldDef_Number(f);
  const upb_value v = upb_value_constptr(f);

  // TODO(salo): This lookup is unfortunate because we also perform it when
  // inserting into the message's table. Unfortunately that step occurs after
  // this one and moving things around could be tricky so let's leave it for
  // a future refactoring.
  const bool number_exists = upb_inttable_lookup(&o->itof, number, NULL);
  if (UPB_UNLIKELY(number_exists)) {
    _upb_DefBuilder_Errf(ctx, "oneof fields have the same number (%d)", number);
  }

  // TODO(salo): More redundant work happening here.
  const bool name_exists = upb_strtable_lookup2(&o->ntof, name, size, NULL);
  if (UPB_UNLIKELY(name_exists)) {
    _upb_DefBuilder_Errf(ctx, "oneof fields have the same name (%s)", name);
  }

  const bool ok = upb_inttable_insert(&o->itof, number, v, ctx->arena) &&
                  upb_strtable_insert(&o->ntof, name, size, v, ctx->arena);
  if (UPB_UNLIKELY(!ok)) {
    _upb_DefBuilder_OomErr(ctx);
  }
}

// Returns the synthetic count.
size_t _upb_OneofDefs_Finalize(upb_DefBuilder* ctx, upb_MessageDef* m) {
  int synthetic_count = 0;

  for (int i = 0; i < upb_MessageDef_OneofCount(m); i++) {
    upb_OneofDef* o = (upb_OneofDef*)upb_MessageDef_Oneof(m, i);

    if (o->synthetic && o->field_count != 1) {
      _upb_DefBuilder_Errf(ctx,
                           "Synthetic oneofs must have one field, not %d: %s",
                           o->field_count, upb_OneofDef_Name(o));
    }

    if (o->synthetic) {
      synthetic_count++;
    } else if (synthetic_count != 0) {
      _upb_DefBuilder_Errf(
          ctx, "Synthetic oneofs must be after all other oneofs: %s",
          upb_OneofDef_Name(o));
    }

    o->fields =
        _upb_DefBuilder_Alloc(ctx, sizeof(upb_FieldDef*) * o->field_count);
    o->field_count = 0;
  }

  for (int i = 0; i < upb_MessageDef_FieldCount(m); i++) {
    const upb_FieldDef* f = upb_MessageDef_Field(m, i);
    upb_OneofDef* o = (upb_OneofDef*)upb_FieldDef_ContainingOneof(f);
    if (o) {
      o->fields[o->field_count++] = f;
    }
  }

  return synthetic_count;
}

static void create_oneofdef(upb_DefBuilder* ctx, upb_MessageDef* m,
                            const UPB_DESC(OneofDescriptorProto) * oneof_proto,
                            const upb_OneofDef* _o) {
  upb_OneofDef* o = (upb_OneofDef*)_o;
  upb_StringView name = UPB_DESC(OneofDescriptorProto_name)(oneof_proto);

  o->parent = m;
  o->full_name =
      _upb_DefBuilder_MakeFullName(ctx, upb_MessageDef_FullName(m), name);
  o->field_count = 0;
  o->synthetic = false;

  UPB_DEF_SET_OPTIONS(o->opts, OneofDescriptorProto, OneofOptions, oneof_proto);

  if (upb_MessageDef_FindByNameWithSize(m, name.data, name.size, NULL, NULL)) {
    _upb_DefBuilder_Errf(ctx, "duplicate oneof name (%s)", o->full_name);
  }

  upb_value v = _upb_DefType_Pack(o, UPB_DEFTYPE_ONEOF);
  bool ok = _upb_MessageDef_Insert(m, name.data, name.size, v, ctx->arena);
  if (!ok) _upb_DefBuilder_OomErr(ctx);

  ok = upb_inttable_init(&o->itof, ctx->arena);
  if (!ok) _upb_DefBuilder_OomErr(ctx);

  ok = upb_strtable_init(&o->ntof, 4, ctx->arena);
  if (!ok) _upb_DefBuilder_OomErr(ctx);
}

// Allocate and initialize an array of |n| oneof defs.
upb_OneofDef* _upb_OneofDefs_New(
    upb_DefBuilder* ctx, int n,
    const UPB_DESC(OneofDescriptorProto) * const* protos, upb_MessageDef* m) {
  _upb_DefType_CheckPadding(sizeof(upb_OneofDef));

  upb_OneofDef* o = _upb_DefBuilder_Alloc(ctx, sizeof(upb_OneofDef) * n);
  for (int i = 0; i < n; i++) {
    create_oneofdef(ctx, m, protos[i], &o[i]);
  }
  return o;
}


// Must be last.

struct upb_ServiceDef {
  const UPB_DESC(ServiceOptions) * opts;
  const upb_FileDef* file;
  const char* full_name;
  upb_MethodDef* methods;
  int method_count;
  int index;
};

upb_ServiceDef* _upb_ServiceDef_At(const upb_ServiceDef* s, int index) {
  return (upb_ServiceDef*)&s[index];
}

const UPB_DESC(ServiceOptions) *
    upb_ServiceDef_Options(const upb_ServiceDef* s) {
  return s->opts;
}

bool upb_ServiceDef_HasOptions(const upb_ServiceDef* s) {
  return s->opts != (void*)kUpbDefOptDefault;
}

const char* upb_ServiceDef_FullName(const upb_ServiceDef* s) {
  return s->full_name;
}

const char* upb_ServiceDef_Name(const upb_ServiceDef* s) {
  return _upb_DefBuilder_FullToShort(s->full_name);
}

int upb_ServiceDef_Index(const upb_ServiceDef* s) { return s->index; }

const upb_FileDef* upb_ServiceDef_File(const upb_ServiceDef* s) {
  return s->file;
}

int upb_ServiceDef_MethodCount(const upb_ServiceDef* s) {
  return s->method_count;
}

const upb_MethodDef* upb_ServiceDef_Method(const upb_ServiceDef* s, int i) {
  return (i < 0 || i >= s->method_count) ? NULL
                                         : _upb_MethodDef_At(s->methods, i);
}

const upb_MethodDef* upb_ServiceDef_FindMethodByName(const upb_ServiceDef* s,
                                                     const char* name) {
  for (int i = 0; i < s->method_count; i++) {
    const upb_MethodDef* m = _upb_MethodDef_At(s->methods, i);
    if (strcmp(name, upb_MethodDef_Name(m)) == 0) {
      return m;
    }
  }
  return NULL;
}

static void create_service(upb_DefBuilder* ctx,
                           const UPB_DESC(ServiceDescriptorProto) * svc_proto,
                           upb_ServiceDef* s) {
  upb_StringView name;
  size_t n;

  // Must happen before _upb_DefBuilder_Add()
  s->file = _upb_DefBuilder_File(ctx);

  name = UPB_DESC(ServiceDescriptorProto_name)(svc_proto);
  const char* package = _upb_FileDef_RawPackage(s->file);
  s->full_name = _upb_DefBuilder_MakeFullName(ctx, package, name);
  _upb_DefBuilder_Add(ctx, s->full_name,
                      _upb_DefType_Pack(s, UPB_DEFTYPE_SERVICE));

  const UPB_DESC(MethodDescriptorProto)* const* methods =
      UPB_DESC(ServiceDescriptorProto_method)(svc_proto, &n);
  s->method_count = n;
  s->methods = _upb_MethodDefs_New(ctx, n, methods, s);

  UPB_DEF_SET_OPTIONS(s->opts, ServiceDescriptorProto, ServiceOptions,
                      svc_proto);
}

upb_ServiceDef* _upb_ServiceDefs_New(
    upb_DefBuilder* ctx, int n,
    const UPB_DESC(ServiceDescriptorProto) * const* protos) {
  _upb_DefType_CheckPadding(sizeof(upb_ServiceDef));

  upb_ServiceDef* s = _upb_DefBuilder_Alloc(ctx, sizeof(upb_ServiceDef) * n);
  for (int i = 0; i < n; i++) {
    create_service(ctx, protos[i], &s[i]);
    s[i].index = i;
  }
  return s;
}


#include <string.h>


// Must be last.

// A few fake field types for our tables.
enum {
  kUpb_FakeFieldType_FieldNotFound = 0,
  kUpb_FakeFieldType_MessageSetItem = 19,
};

// DecodeOp: an action to be performed for a wire-type/field-type combination.
enum {
  // Special ops: we don't write data to regular fields for these.
  kUpb_DecodeOp_UnknownField = -1,
  kUpb_DecodeOp_MessageSetItem = -2,

  // Scalar-only ops.
  kUpb_DecodeOp_Scalar1Byte = 0,
  kUpb_DecodeOp_Scalar4Byte = 2,
  kUpb_DecodeOp_Scalar8Byte = 3,
  kUpb_DecodeOp_Enum = 1,

  // Scalar/repeated ops.
  kUpb_DecodeOp_String = 4,
  kUpb_DecodeOp_Bytes = 5,
  kUpb_DecodeOp_SubMessage = 6,

  // Repeated-only ops (also see macros below).
  kUpb_DecodeOp_PackedEnum = 13,
};

// For packed fields it is helpful to be able to recover the lg2 of the data
// size from the op.
#define OP_FIXPCK_LG2(n) (n + 5) /* n in [2, 3] => op in [7, 8] */
#define OP_VARPCK_LG2(n) (n + 9) /* n in [0, 2, 3] => op in [9, 11, 12] */

typedef union {
  bool bool_val;
  uint32_t uint32_val;
  uint64_t uint64_val;
  uint32_t size;
} wireval;

static const char* _upb_Decoder_DecodeMessage(upb_Decoder* d, const char* ptr,
                                              upb_Message* msg,
                                              const upb_MiniTable* layout);

UPB_NORETURN static void* _upb_Decoder_ErrorJmp(upb_Decoder* d,
                                                upb_DecodeStatus status) {
  assert(status != kUpb_DecodeStatus_Ok);
  d->status = status;
  UPB_LONGJMP(d->err, 1);
}

const char* _upb_FastDecoder_ErrorJmp(upb_Decoder* d, int status) {
  assert(status != kUpb_DecodeStatus_Ok);
  d->status = status;
  UPB_LONGJMP(d->err, 1);
  return NULL;
}

static void _upb_Decoder_VerifyUtf8(upb_Decoder* d, const char* buf, int len) {
  if (!_upb_Decoder_VerifyUtf8Inline(buf, len)) {
    _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_BadUtf8);
  }
}

static bool _upb_Decoder_Reserve(upb_Decoder* d, upb_Array* arr, size_t elem) {
  bool need_realloc = arr->capacity - arr->size < elem;
  if (need_realloc && !_upb_array_realloc(arr, arr->size + elem, &d->arena)) {
    _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_OutOfMemory);
  }
  return need_realloc;
}

typedef struct {
  const char* ptr;
  uint64_t val;
} _upb_DecodeLongVarintReturn;

UPB_NOINLINE
static _upb_DecodeLongVarintReturn _upb_Decoder_DecodeLongVarint(
    const char* ptr, uint64_t val) {
  _upb_DecodeLongVarintReturn ret = {NULL, 0};
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
static const char* _upb_Decoder_DecodeVarint(upb_Decoder* d, const char* ptr,
                                             uint64_t* val) {
  uint64_t byte = (uint8_t)*ptr;
  if (UPB_LIKELY((byte & 0x80) == 0)) {
    *val = byte;
    return ptr + 1;
  } else {
    _upb_DecodeLongVarintReturn res = _upb_Decoder_DecodeLongVarint(ptr, byte);
    if (!res.ptr) _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_Malformed);
    *val = res.val;
    return res.ptr;
  }
}

UPB_FORCEINLINE
static const char* _upb_Decoder_DecodeTag(upb_Decoder* d, const char* ptr,
                                          uint32_t* val) {
  uint64_t byte = (uint8_t)*ptr;
  if (UPB_LIKELY((byte & 0x80) == 0)) {
    *val = byte;
    return ptr + 1;
  } else {
    const char* start = ptr;
    _upb_DecodeLongVarintReturn res = _upb_Decoder_DecodeLongVarint(ptr, byte);
    if (!res.ptr || res.ptr - start > 5 || res.val > UINT32_MAX) {
      _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_Malformed);
    }
    *val = res.val;
    return res.ptr;
  }
}

UPB_FORCEINLINE
static const char* upb_Decoder_DecodeSize(upb_Decoder* d, const char* ptr,
                                          uint32_t* size) {
  uint64_t size64;
  ptr = _upb_Decoder_DecodeVarint(d, ptr, &size64);
  if (size64 >= INT32_MAX ||
      !upb_EpsCopyInputStream_CheckSize(&d->input, ptr, (int)size64)) {
    _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_Malformed);
  }
  *size = size64;
  return ptr;
}

static void _upb_Decoder_MungeInt32(wireval* val) {
  if (!_upb_IsLittleEndian()) {
    /* The next stage will memcpy(dst, &val, 4) */
    val->uint32_val = val->uint64_val;
  }
}

static void _upb_Decoder_Munge(int type, wireval* val) {
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
      _upb_Decoder_MungeInt32(val);
      break;
  }
}

static upb_Message* _upb_Decoder_NewSubMessage(upb_Decoder* d,
                                               const upb_MiniTableSub* subs,
                                               const upb_MiniTableField* field,
                                               upb_TaggedMessagePtr* target) {
  const upb_MiniTable* subl = subs[field->UPB_PRIVATE(submsg_index)].submsg;
  UPB_ASSERT(subl);
  upb_Message* msg = _upb_Message_New(subl, &d->arena);
  if (!msg) _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_OutOfMemory);

  // Extensions should not be unlinked.  A message extension should not be
  // registered until its sub-message type is available to be linked.
  bool is_empty = subl == &_kUpb_MiniTable_Empty;
  bool is_extension = field->mode & kUpb_LabelFlags_IsExtension;
  UPB_ASSERT(!(is_empty && is_extension));

  if (is_empty && !(d->options & kUpb_DecodeOption_ExperimentalAllowUnlinked)) {
    _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_UnlinkedSubMessage);
  }

  upb_TaggedMessagePtr tagged = _upb_TaggedMessagePtr_Pack(msg, is_empty);
  memcpy(target, &tagged, sizeof(tagged));
  return msg;
}

static upb_Message* _upb_Decoder_ReuseSubMessage(
    upb_Decoder* d, const upb_MiniTableSub* subs,
    const upb_MiniTableField* field, upb_TaggedMessagePtr* target) {
  upb_TaggedMessagePtr tagged = *target;
  const upb_MiniTable* subl = subs[field->UPB_PRIVATE(submsg_index)].submsg;
  UPB_ASSERT(subl);
  if (!upb_TaggedMessagePtr_IsEmpty(tagged) || subl == &_kUpb_MiniTable_Empty) {
    return _upb_TaggedMessagePtr_GetMessage(tagged);
  }

  // We found an empty message from a previous parse that was performed before
  // this field was linked.  But it is linked now, so we want to allocate a new
  // message of the correct type and promote data into it before continuing.
  upb_Message* existing = _upb_TaggedMessagePtr_GetEmptyMessage(tagged);
  upb_Message* promoted = _upb_Decoder_NewSubMessage(d, subs, field, target);
  size_t size;
  const char* unknown = upb_Message_GetUnknown(existing, &size);
  upb_DecodeStatus status = upb_Decode(unknown, size, promoted, subl, d->extreg,
                                       d->options, &d->arena);
  if (status != kUpb_DecodeStatus_Ok) _upb_Decoder_ErrorJmp(d, status);
  return promoted;
}

static const char* _upb_Decoder_ReadString(upb_Decoder* d, const char* ptr,
                                           int size, upb_StringView* str) {
  const char* str_ptr = ptr;
  ptr = upb_EpsCopyInputStream_ReadString(&d->input, &str_ptr, size, &d->arena);
  if (!ptr) _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_OutOfMemory);
  str->data = str_ptr;
  str->size = size;
  return ptr;
}

UPB_FORCEINLINE
static const char* _upb_Decoder_RecurseSubMessage(upb_Decoder* d,
                                                  const char* ptr,
                                                  upb_Message* submsg,
                                                  const upb_MiniTable* subl,
                                                  uint32_t expected_end_group) {
  if (--d->depth < 0) {
    _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_MaxDepthExceeded);
  }
  ptr = _upb_Decoder_DecodeMessage(d, ptr, submsg, subl);
  d->depth++;
  if (d->end_group != expected_end_group) {
    _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_Malformed);
  }
  return ptr;
}

UPB_FORCEINLINE
static const char* _upb_Decoder_DecodeSubMessage(
    upb_Decoder* d, const char* ptr, upb_Message* submsg,
    const upb_MiniTableSub* subs, const upb_MiniTableField* field, int size) {
  int saved_delta = upb_EpsCopyInputStream_PushLimit(&d->input, ptr, size);
  const upb_MiniTable* subl = subs[field->UPB_PRIVATE(submsg_index)].submsg;
  UPB_ASSERT(subl);
  ptr = _upb_Decoder_RecurseSubMessage(d, ptr, submsg, subl, DECODE_NOGROUP);
  upb_EpsCopyInputStream_PopLimit(&d->input, ptr, saved_delta);
  return ptr;
}

UPB_FORCEINLINE
static const char* _upb_Decoder_DecodeGroup(upb_Decoder* d, const char* ptr,
                                            upb_Message* submsg,
                                            const upb_MiniTable* subl,
                                            uint32_t number) {
  if (_upb_Decoder_IsDone(d, &ptr)) {
    _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_Malformed);
  }
  ptr = _upb_Decoder_RecurseSubMessage(d, ptr, submsg, subl, number);
  d->end_group = DECODE_NOGROUP;
  return ptr;
}

UPB_FORCEINLINE
static const char* _upb_Decoder_DecodeUnknownGroup(upb_Decoder* d,
                                                   const char* ptr,
                                                   uint32_t number) {
  return _upb_Decoder_DecodeGroup(d, ptr, NULL, NULL, number);
}

UPB_FORCEINLINE
static const char* _upb_Decoder_DecodeKnownGroup(
    upb_Decoder* d, const char* ptr, upb_Message* submsg,
    const upb_MiniTableSub* subs, const upb_MiniTableField* field) {
  const upb_MiniTable* subl = subs[field->UPB_PRIVATE(submsg_index)].submsg;
  UPB_ASSERT(subl);
  return _upb_Decoder_DecodeGroup(d, ptr, submsg, subl, field->number);
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

static void _upb_Decoder_AddUnknownVarints(upb_Decoder* d, upb_Message* msg,
                                           uint32_t val1, uint32_t val2) {
  char buf[20];
  char* end = buf;
  end = upb_Decoder_EncodeVarint32(val1, end);
  end = upb_Decoder_EncodeVarint32(val2, end);

  if (!_upb_Message_AddUnknown(msg, buf, end - buf, &d->arena)) {
    _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_OutOfMemory);
  }
}

UPB_NOINLINE
static bool _upb_Decoder_CheckEnumSlow(upb_Decoder* d, const char* ptr,
                                       upb_Message* msg,
                                       const upb_MiniTableEnum* e,
                                       const upb_MiniTableField* field,
                                       uint32_t v) {
  if (_upb_MiniTable_CheckEnumValueSlow(e, v)) return true;

  // Unrecognized enum goes into unknown fields.
  // For packed fields the tag could be arbitrarily far in the past, so we
  // just re-encode the tag and value here.
  uint32_t tag = ((uint32_t)field->number << 3) | kUpb_WireType_Varint;
  upb_Message* unknown_msg =
      field->mode & kUpb_LabelFlags_IsExtension ? d->unknown_msg : msg;
  _upb_Decoder_AddUnknownVarints(d, unknown_msg, tag, v);
  return false;
}

UPB_FORCEINLINE
static bool _upb_Decoder_CheckEnum(upb_Decoder* d, const char* ptr,
                                   upb_Message* msg, const upb_MiniTableEnum* e,
                                   const upb_MiniTableField* field,
                                   wireval* val) {
  uint32_t v = val->uint32_val;

  _kUpb_FastEnumCheck_Status status = _upb_MiniTable_CheckEnumValueFast(e, v);
  if (UPB_LIKELY(status == _kUpb_FastEnumCheck_ValueIsInEnum)) return true;
  return _upb_Decoder_CheckEnumSlow(d, ptr, msg, e, field, v);
}

UPB_NOINLINE
static const char* _upb_Decoder_DecodeEnumArray(upb_Decoder* d, const char* ptr,
                                                upb_Message* msg,
                                                upb_Array* arr,
                                                const upb_MiniTableSub* subs,
                                                const upb_MiniTableField* field,
                                                wireval* val) {
  const upb_MiniTableEnum* e = subs[field->UPB_PRIVATE(submsg_index)].subenum;
  if (!_upb_Decoder_CheckEnum(d, ptr, msg, e, field, val)) return ptr;
  void* mem = UPB_PTR_AT(_upb_array_ptr(arr), arr->size * 4, void);
  arr->size++;
  memcpy(mem, val, 4);
  return ptr;
}

UPB_FORCEINLINE
static const char* _upb_Decoder_DecodeFixedPacked(
    upb_Decoder* d, const char* ptr, upb_Array* arr, wireval* val,
    const upb_MiniTableField* field, int lg2) {
  int mask = (1 << lg2) - 1;
  size_t count = val->size >> lg2;
  if ((val->size & mask) != 0) {
    // Length isn't a round multiple of elem size.
    _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_Malformed);
  }
  _upb_Decoder_Reserve(d, arr, count);
  void* mem = UPB_PTR_AT(_upb_array_ptr(arr), arr->size << lg2, void);
  arr->size += count;
  // Note: if/when the decoder supports multi-buffer input, we will need to
  // handle buffer seams here.
  if (_upb_IsLittleEndian()) {
    ptr = upb_EpsCopyInputStream_Copy(&d->input, ptr, mem, val->size);
  } else {
    int delta = upb_EpsCopyInputStream_PushLimit(&d->input, ptr, val->size);
    char* dst = mem;
    while (!_upb_Decoder_IsDone(d, &ptr)) {
      if (lg2 == 2) {
        ptr = upb_WireReader_ReadFixed32(ptr, dst);
        dst += 4;
      } else {
        UPB_ASSERT(lg2 == 3);
        ptr = upb_WireReader_ReadFixed64(ptr, dst);
        dst += 8;
      }
    }
    upb_EpsCopyInputStream_PopLimit(&d->input, ptr, delta);
  }

  return ptr;
}

UPB_FORCEINLINE
static const char* _upb_Decoder_DecodeVarintPacked(
    upb_Decoder* d, const char* ptr, upb_Array* arr, wireval* val,
    const upb_MiniTableField* field, int lg2) {
  int scale = 1 << lg2;
  int saved_limit = upb_EpsCopyInputStream_PushLimit(&d->input, ptr, val->size);
  char* out = UPB_PTR_AT(_upb_array_ptr(arr), arr->size << lg2, void);
  while (!_upb_Decoder_IsDone(d, &ptr)) {
    wireval elem;
    ptr = _upb_Decoder_DecodeVarint(d, ptr, &elem.uint64_val);
    _upb_Decoder_Munge(field->UPB_PRIVATE(descriptortype), &elem);
    if (_upb_Decoder_Reserve(d, arr, 1)) {
      out = UPB_PTR_AT(_upb_array_ptr(arr), arr->size << lg2, void);
    }
    arr->size++;
    memcpy(out, &elem, scale);
    out += scale;
  }
  upb_EpsCopyInputStream_PopLimit(&d->input, ptr, saved_limit);
  return ptr;
}

UPB_NOINLINE
static const char* _upb_Decoder_DecodeEnumPacked(
    upb_Decoder* d, const char* ptr, upb_Message* msg, upb_Array* arr,
    const upb_MiniTableSub* subs, const upb_MiniTableField* field,
    wireval* val) {
  const upb_MiniTableEnum* e = subs[field->UPB_PRIVATE(submsg_index)].subenum;
  int saved_limit = upb_EpsCopyInputStream_PushLimit(&d->input, ptr, val->size);
  char* out = UPB_PTR_AT(_upb_array_ptr(arr), arr->size * 4, void);
  while (!_upb_Decoder_IsDone(d, &ptr)) {
    wireval elem;
    ptr = _upb_Decoder_DecodeVarint(d, ptr, &elem.uint64_val);
    _upb_Decoder_MungeInt32(&elem);
    if (!_upb_Decoder_CheckEnum(d, ptr, msg, e, field, &elem)) {
      continue;
    }
    if (_upb_Decoder_Reserve(d, arr, 1)) {
      out = UPB_PTR_AT(_upb_array_ptr(arr), arr->size * 4, void);
    }
    arr->size++;
    memcpy(out, &elem, 4);
    out += 4;
  }
  upb_EpsCopyInputStream_PopLimit(&d->input, ptr, saved_limit);
  return ptr;
}

upb_Array* _upb_Decoder_CreateArray(upb_Decoder* d,
                                    const upb_MiniTableField* field) {
  /* Maps descriptor type -> elem_size_lg2.  */
  static const uint8_t kElemSizeLg2[] = {
      [0] = -1,  // invalid descriptor type
      [kUpb_FieldType_Double] = 3,
      [kUpb_FieldType_Float] = 2,
      [kUpb_FieldType_Int64] = 3,
      [kUpb_FieldType_UInt64] = 3,
      [kUpb_FieldType_Int32] = 2,
      [kUpb_FieldType_Fixed64] = 3,
      [kUpb_FieldType_Fixed32] = 2,
      [kUpb_FieldType_Bool] = 0,
      [kUpb_FieldType_String] = UPB_SIZE(3, 4),
      [kUpb_FieldType_Group] = UPB_SIZE(2, 3),
      [kUpb_FieldType_Message] = UPB_SIZE(2, 3),
      [kUpb_FieldType_Bytes] = UPB_SIZE(3, 4),
      [kUpb_FieldType_UInt32] = 2,
      [kUpb_FieldType_Enum] = 2,
      [kUpb_FieldType_SFixed32] = 2,
      [kUpb_FieldType_SFixed64] = 3,
      [kUpb_FieldType_SInt32] = 2,
      [kUpb_FieldType_SInt64] = 3,
  };

  size_t lg2 = kElemSizeLg2[field->UPB_PRIVATE(descriptortype)];
  upb_Array* ret = _upb_Array_New(&d->arena, 4, lg2);
  if (!ret) _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_OutOfMemory);
  return ret;
}

static const char* _upb_Decoder_DecodeToArray(upb_Decoder* d, const char* ptr,
                                              upb_Message* msg,
                                              const upb_MiniTableSub* subs,
                                              const upb_MiniTableField* field,
                                              wireval* val, int op) {
  upb_Array** arrp = UPB_PTR_AT(msg, field->offset, void);
  upb_Array* arr = *arrp;
  void* mem;

  if (arr) {
    _upb_Decoder_Reserve(d, arr, 1);
  } else {
    arr = _upb_Decoder_CreateArray(d, field);
    *arrp = arr;
  }

  switch (op) {
    case kUpb_DecodeOp_Scalar1Byte:
    case kUpb_DecodeOp_Scalar4Byte:
    case kUpb_DecodeOp_Scalar8Byte:
      /* Append scalar value. */
      mem = UPB_PTR_AT(_upb_array_ptr(arr), arr->size << op, void);
      arr->size++;
      memcpy(mem, val, 1 << op);
      return ptr;
    case kUpb_DecodeOp_String:
      _upb_Decoder_VerifyUtf8(d, ptr, val->size);
      /* Fallthrough. */
    case kUpb_DecodeOp_Bytes: {
      /* Append bytes. */
      upb_StringView* str = (upb_StringView*)_upb_array_ptr(arr) + arr->size;
      arr->size++;
      return _upb_Decoder_ReadString(d, ptr, val->size, str);
    }
    case kUpb_DecodeOp_SubMessage: {
      /* Append submessage / group. */
      upb_TaggedMessagePtr* target = UPB_PTR_AT(
          _upb_array_ptr(arr), arr->size * sizeof(void*), upb_TaggedMessagePtr);
      upb_Message* submsg = _upb_Decoder_NewSubMessage(d, subs, field, target);
      arr->size++;
      if (UPB_UNLIKELY(field->UPB_PRIVATE(descriptortype) ==
                       kUpb_FieldType_Group)) {
        return _upb_Decoder_DecodeKnownGroup(d, ptr, submsg, subs, field);
      } else {
        return _upb_Decoder_DecodeSubMessage(d, ptr, submsg, subs, field,
                                             val->size);
      }
    }
    case OP_FIXPCK_LG2(2):
    case OP_FIXPCK_LG2(3):
      return _upb_Decoder_DecodeFixedPacked(d, ptr, arr, val, field,
                                            op - OP_FIXPCK_LG2(0));
    case OP_VARPCK_LG2(0):
    case OP_VARPCK_LG2(2):
    case OP_VARPCK_LG2(3):
      return _upb_Decoder_DecodeVarintPacked(d, ptr, arr, val, field,
                                             op - OP_VARPCK_LG2(0));
    case kUpb_DecodeOp_Enum:
      return _upb_Decoder_DecodeEnumArray(d, ptr, msg, arr, subs, field, val);
    case kUpb_DecodeOp_PackedEnum:
      return _upb_Decoder_DecodeEnumPacked(d, ptr, msg, arr, subs, field, val);
    default:
      UPB_UNREACHABLE();
  }
}

upb_Map* _upb_Decoder_CreateMap(upb_Decoder* d, const upb_MiniTable* entry) {
  /* Maps descriptor type -> upb map size.  */
  static const uint8_t kSizeInMap[] = {
      [0] = -1,  // invalid descriptor type */
      [kUpb_FieldType_Double] = 8,
      [kUpb_FieldType_Float] = 4,
      [kUpb_FieldType_Int64] = 8,
      [kUpb_FieldType_UInt64] = 8,
      [kUpb_FieldType_Int32] = 4,
      [kUpb_FieldType_Fixed64] = 8,
      [kUpb_FieldType_Fixed32] = 4,
      [kUpb_FieldType_Bool] = 1,
      [kUpb_FieldType_String] = UPB_MAPTYPE_STRING,
      [kUpb_FieldType_Group] = sizeof(void*),
      [kUpb_FieldType_Message] = sizeof(void*),
      [kUpb_FieldType_Bytes] = UPB_MAPTYPE_STRING,
      [kUpb_FieldType_UInt32] = 4,
      [kUpb_FieldType_Enum] = 4,
      [kUpb_FieldType_SFixed32] = 4,
      [kUpb_FieldType_SFixed64] = 8,
      [kUpb_FieldType_SInt32] = 4,
      [kUpb_FieldType_SInt64] = 8,
  };

  const upb_MiniTableField* key_field = &entry->fields[0];
  const upb_MiniTableField* val_field = &entry->fields[1];
  char key_size = kSizeInMap[key_field->UPB_PRIVATE(descriptortype)];
  char val_size = kSizeInMap[val_field->UPB_PRIVATE(descriptortype)];
  UPB_ASSERT(key_field->offset == offsetof(upb_MapEntryData, k));
  UPB_ASSERT(val_field->offset == offsetof(upb_MapEntryData, v));
  upb_Map* ret = _upb_Map_New(&d->arena, key_size, val_size);
  if (!ret) _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_OutOfMemory);
  return ret;
}

static const char* _upb_Decoder_DecodeToMap(upb_Decoder* d, const char* ptr,
                                            upb_Message* msg,
                                            const upb_MiniTableSub* subs,
                                            const upb_MiniTableField* field,
                                            wireval* val) {
  upb_Map** map_p = UPB_PTR_AT(msg, field->offset, upb_Map*);
  upb_Map* map = *map_p;
  upb_MapEntry ent;
  UPB_ASSERT(upb_MiniTableField_Type(field) == kUpb_FieldType_Message);
  const upb_MiniTable* entry = subs[field->UPB_PRIVATE(submsg_index)].submsg;

  UPB_ASSERT(entry);
  UPB_ASSERT(entry->field_count == 2);
  UPB_ASSERT(!upb_IsRepeatedOrMap(&entry->fields[0]));
  UPB_ASSERT(!upb_IsRepeatedOrMap(&entry->fields[1]));

  if (!map) {
    map = _upb_Decoder_CreateMap(d, entry);
    *map_p = map;
  }

  // Parse map entry.
  memset(&ent, 0, sizeof(ent));

  if (entry->fields[1].UPB_PRIVATE(descriptortype) == kUpb_FieldType_Message ||
      entry->fields[1].UPB_PRIVATE(descriptortype) == kUpb_FieldType_Group) {
    // Create proactively to handle the case where it doesn't appear.
    upb_TaggedMessagePtr msg;
    _upb_Decoder_NewSubMessage(d, entry->subs, &entry->fields[1], &msg);
    ent.data.v.val = upb_value_uintptr(msg);
  }

  ptr =
      _upb_Decoder_DecodeSubMessage(d, ptr, &ent.data, subs, field, val->size);
  // check if ent had any unknown fields
  size_t size;
  upb_Message_GetUnknown(&ent.data, &size);
  if (size != 0) {
    char* buf;
    size_t size;
    uint32_t tag = ((uint32_t)field->number << 3) | kUpb_WireType_Delimited;
    upb_EncodeStatus status =
        upb_Encode(&ent.data, entry, 0, &d->arena, &buf, &size);
    if (status != kUpb_EncodeStatus_Ok) {
      _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_OutOfMemory);
    }
    _upb_Decoder_AddUnknownVarints(d, msg, tag, size);
    if (!_upb_Message_AddUnknown(msg, buf, size, &d->arena)) {
      _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_OutOfMemory);
    }
  } else {
    if (_upb_Map_Insert(map, &ent.data.k, map->key_size, &ent.data.v,
                        map->val_size,
                        &d->arena) == kUpb_MapInsertStatus_OutOfMemory) {
      _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_OutOfMemory);
    }
  }
  return ptr;
}

static const char* _upb_Decoder_DecodeToSubMessage(
    upb_Decoder* d, const char* ptr, upb_Message* msg,
    const upb_MiniTableSub* subs, const upb_MiniTableField* field, wireval* val,
    int op) {
  void* mem = UPB_PTR_AT(msg, field->offset, void);
  int type = field->UPB_PRIVATE(descriptortype);

  if (UPB_UNLIKELY(op == kUpb_DecodeOp_Enum) &&
      !_upb_Decoder_CheckEnum(d, ptr, msg,
                              subs[field->UPB_PRIVATE(submsg_index)].subenum,
                              field, val)) {
    return ptr;
  }

  /* Set presence if necessary. */
  if (field->presence > 0) {
    _upb_sethas_field(msg, field);
  } else if (field->presence < 0) {
    /* Oneof case */
    uint32_t* oneof_case = _upb_oneofcase_field(msg, field);
    if (op == kUpb_DecodeOp_SubMessage && *oneof_case != field->number) {
      memset(mem, 0, sizeof(void*));
    }
    *oneof_case = field->number;
  }

  /* Store into message. */
  switch (op) {
    case kUpb_DecodeOp_SubMessage: {
      upb_TaggedMessagePtr* submsgp = mem;
      upb_Message* submsg;
      if (*submsgp) {
        submsg = _upb_Decoder_ReuseSubMessage(d, subs, field, submsgp);
      } else {
        submsg = _upb_Decoder_NewSubMessage(d, subs, field, submsgp);
      }
      if (UPB_UNLIKELY(type == kUpb_FieldType_Group)) {
        ptr = _upb_Decoder_DecodeKnownGroup(d, ptr, submsg, subs, field);
      } else {
        ptr = _upb_Decoder_DecodeSubMessage(d, ptr, submsg, subs, field,
                                            val->size);
      }
      break;
    }
    case kUpb_DecodeOp_String:
      _upb_Decoder_VerifyUtf8(d, ptr, val->size);
      /* Fallthrough. */
    case kUpb_DecodeOp_Bytes:
      return _upb_Decoder_ReadString(d, ptr, val->size, mem);
    case kUpb_DecodeOp_Scalar8Byte:
      memcpy(mem, val, 8);
      break;
    case kUpb_DecodeOp_Enum:
    case kUpb_DecodeOp_Scalar4Byte:
      memcpy(mem, val, 4);
      break;
    case kUpb_DecodeOp_Scalar1Byte:
      memcpy(mem, val, 1);
      break;
    default:
      UPB_UNREACHABLE();
  }

  return ptr;
}

UPB_NOINLINE
const char* _upb_Decoder_CheckRequired(upb_Decoder* d, const char* ptr,
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
static bool _upb_Decoder_TryFastDispatch(upb_Decoder* d, const char** ptr,
                                         upb_Message* msg,
                                         const upb_MiniTable* layout) {
#if UPB_FASTTABLE
  if (layout && layout->table_mask != (unsigned char)-1) {
    uint16_t tag = _upb_FastDecoder_LoadTag(*ptr);
    intptr_t table = decode_totable(layout);
    *ptr = _upb_FastDecoder_TagDispatch(d, *ptr, msg, table, 0, tag);
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
      return _upb_Decoder_DecodeVarint(d, ptr, &val);
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
      return _upb_Decoder_DecodeUnknownGroup(d, ptr, field_number);
    default:
      _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_Malformed);
  }
}

enum {
  kStartItemTag = ((kUpb_MsgSet_Item << 3) | kUpb_WireType_StartGroup),
  kEndItemTag = ((kUpb_MsgSet_Item << 3) | kUpb_WireType_EndGroup),
  kTypeIdTag = ((kUpb_MsgSet_TypeId << 3) | kUpb_WireType_Varint),
  kMessageTag = ((kUpb_MsgSet_Message << 3) | kUpb_WireType_Delimited),
};

static void upb_Decoder_AddKnownMessageSetItem(
    upb_Decoder* d, upb_Message* msg, const upb_MiniTableExtension* item_mt,
    const char* data, uint32_t size) {
  upb_Message_Extension* ext =
      _upb_Message_GetOrCreateExtension(msg, item_mt, &d->arena);
  if (UPB_UNLIKELY(!ext)) {
    _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_OutOfMemory);
  }
  upb_Message* submsg = _upb_Decoder_NewSubMessage(
      d, &ext->ext->sub, &ext->ext->field, (upb_TaggedMessagePtr*)&ext->data);
  upb_DecodeStatus status = upb_Decode(data, size, submsg, item_mt->sub.submsg,
                                       d->extreg, d->options, &d->arena);
  if (status != kUpb_DecodeStatus_Ok) _upb_Decoder_ErrorJmp(d, status);
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
    _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_OutOfMemory);
  }
}

static void upb_Decoder_AddMessageSetItem(upb_Decoder* d, upb_Message* msg,
                                          const upb_MiniTable* t,
                                          uint32_t type_id, const char* data,
                                          uint32_t size) {
  const upb_MiniTableExtension* item_mt =
      upb_ExtensionRegistry_Lookup(d->extreg, t, type_id);
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
  while (!_upb_Decoder_IsDone(d, &ptr)) {
    uint32_t tag;
    ptr = _upb_Decoder_DecodeTag(d, ptr, &tag);
    switch (tag) {
      case kEndItemTag:
        return ptr;
      case kTypeIdTag: {
        uint64_t tmp;
        ptr = _upb_Decoder_DecodeVarint(d, ptr, &tmp);
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
  _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_Malformed);
}

static const upb_MiniTableField* _upb_Decoder_FindField(upb_Decoder* d,
                                                        const upb_MiniTable* t,
                                                        uint32_t field_number,
                                                        int* last_field_index) {
  static upb_MiniTableField none = {
      0, 0, 0, 0, kUpb_FakeFieldType_FieldNotFound, 0};
  if (t == NULL) return &none;

  size_t idx = ((size_t)field_number) - 1;  // 0 wraps to SIZE_MAX
  if (idx < t->dense_below) {
    /* Fastest case: index into dense fields. */
    goto found;
  }

  if (t->dense_below < t->field_count) {
    /* Linear search non-dense fields. Resume scanning from last_field_index
     * since fields are usually in order. */
    size_t last = *last_field_index;
    for (idx = last; idx < t->field_count; idx++) {
      if (t->fields[idx].number == field_number) {
        goto found;
      }
    }

    for (idx = t->dense_below; idx < last; idx++) {
      if (t->fields[idx].number == field_number) {
        goto found;
      }
    }
  }

  if (d->extreg) {
    switch (t->ext) {
      case kUpb_ExtMode_Extendable: {
        const upb_MiniTableExtension* ext =
            upb_ExtensionRegistry_Lookup(d->extreg, t, field_number);
        if (ext) return &ext->field;
        break;
      }
      case kUpb_ExtMode_IsMessageSet:
        if (field_number == kUpb_MsgSet_Item) {
          static upb_MiniTableField item = {
              0, 0, 0, 0, kUpb_FakeFieldType_MessageSetItem, 0};
          return &item;
        }
        break;
    }
  }

  return &none; /* Unknown field. */

found:
  UPB_ASSERT(t->fields[idx].number == field_number);
  *last_field_index = idx;
  return &t->fields[idx];
}

int _upb_Decoder_GetVarintOp(const upb_MiniTableField* field) {
  static const int8_t kVarintOps[] = {
      [kUpb_FakeFieldType_FieldNotFound] = kUpb_DecodeOp_UnknownField,
      [kUpb_FieldType_Double] = kUpb_DecodeOp_UnknownField,
      [kUpb_FieldType_Float] = kUpb_DecodeOp_UnknownField,
      [kUpb_FieldType_Int64] = kUpb_DecodeOp_Scalar8Byte,
      [kUpb_FieldType_UInt64] = kUpb_DecodeOp_Scalar8Byte,
      [kUpb_FieldType_Int32] = kUpb_DecodeOp_Scalar4Byte,
      [kUpb_FieldType_Fixed64] = kUpb_DecodeOp_UnknownField,
      [kUpb_FieldType_Fixed32] = kUpb_DecodeOp_UnknownField,
      [kUpb_FieldType_Bool] = kUpb_DecodeOp_Scalar1Byte,
      [kUpb_FieldType_String] = kUpb_DecodeOp_UnknownField,
      [kUpb_FieldType_Group] = kUpb_DecodeOp_UnknownField,
      [kUpb_FieldType_Message] = kUpb_DecodeOp_UnknownField,
      [kUpb_FieldType_Bytes] = kUpb_DecodeOp_UnknownField,
      [kUpb_FieldType_UInt32] = kUpb_DecodeOp_Scalar4Byte,
      [kUpb_FieldType_Enum] = kUpb_DecodeOp_Enum,
      [kUpb_FieldType_SFixed32] = kUpb_DecodeOp_UnknownField,
      [kUpb_FieldType_SFixed64] = kUpb_DecodeOp_UnknownField,
      [kUpb_FieldType_SInt32] = kUpb_DecodeOp_Scalar4Byte,
      [kUpb_FieldType_SInt64] = kUpb_DecodeOp_Scalar8Byte,
      [kUpb_FakeFieldType_MessageSetItem] = kUpb_DecodeOp_UnknownField,
  };

  return kVarintOps[field->UPB_PRIVATE(descriptortype)];
}

UPB_FORCEINLINE
static void _upb_Decoder_CheckUnlinked(upb_Decoder* d, const upb_MiniTable* mt,
                                       const upb_MiniTableField* field,
                                       int* op) {
  // If sub-message is not linked, treat as unknown.
  if (field->mode & kUpb_LabelFlags_IsExtension) return;
  const upb_MiniTableSub* sub = &mt->subs[field->UPB_PRIVATE(submsg_index)];
  if ((d->options & kUpb_DecodeOption_ExperimentalAllowUnlinked) ||
      sub->submsg != &_kUpb_MiniTable_Empty) {
    return;
  }
#ifndef NDEBUG
  const upb_MiniTableField* oneof = upb_MiniTable_GetOneof(mt, field);
  if (oneof) {
    // All other members of the oneof must be message fields that are also
    // unlinked.
    do {
      assert(upb_MiniTableField_CType(oneof) == kUpb_CType_Message);
      const upb_MiniTableSub* oneof_sub =
          &mt->subs[oneof->UPB_PRIVATE(submsg_index)];
      assert(!oneof_sub);
    } while (upb_MiniTable_NextOneofField(mt, &oneof));
  }
#endif  // NDEBUG
  *op = kUpb_DecodeOp_UnknownField;
}

int _upb_Decoder_GetDelimitedOp(upb_Decoder* d, const upb_MiniTable* mt,
                                const upb_MiniTableField* field) {
  enum { kRepeatedBase = 19 };

  static const int8_t kDelimitedOps[] = {
      /* For non-repeated field type. */
      [kUpb_FakeFieldType_FieldNotFound] =
          kUpb_DecodeOp_UnknownField,  // Field not found.
      [kUpb_FieldType_Double] = kUpb_DecodeOp_UnknownField,
      [kUpb_FieldType_Float] = kUpb_DecodeOp_UnknownField,
      [kUpb_FieldType_Int64] = kUpb_DecodeOp_UnknownField,
      [kUpb_FieldType_UInt64] = kUpb_DecodeOp_UnknownField,
      [kUpb_FieldType_Int32] = kUpb_DecodeOp_UnknownField,
      [kUpb_FieldType_Fixed64] = kUpb_DecodeOp_UnknownField,
      [kUpb_FieldType_Fixed32] = kUpb_DecodeOp_UnknownField,
      [kUpb_FieldType_Bool] = kUpb_DecodeOp_UnknownField,
      [kUpb_FieldType_String] = kUpb_DecodeOp_String,
      [kUpb_FieldType_Group] = kUpb_DecodeOp_UnknownField,
      [kUpb_FieldType_Message] = kUpb_DecodeOp_SubMessage,
      [kUpb_FieldType_Bytes] = kUpb_DecodeOp_Bytes,
      [kUpb_FieldType_UInt32] = kUpb_DecodeOp_UnknownField,
      [kUpb_FieldType_Enum] = kUpb_DecodeOp_UnknownField,
      [kUpb_FieldType_SFixed32] = kUpb_DecodeOp_UnknownField,
      [kUpb_FieldType_SFixed64] = kUpb_DecodeOp_UnknownField,
      [kUpb_FieldType_SInt32] = kUpb_DecodeOp_UnknownField,
      [kUpb_FieldType_SInt64] = kUpb_DecodeOp_UnknownField,
      [kUpb_FakeFieldType_MessageSetItem] = kUpb_DecodeOp_UnknownField,
      // For repeated field type. */
      [kRepeatedBase + kUpb_FieldType_Double] = OP_FIXPCK_LG2(3),
      [kRepeatedBase + kUpb_FieldType_Float] = OP_FIXPCK_LG2(2),
      [kRepeatedBase + kUpb_FieldType_Int64] = OP_VARPCK_LG2(3),
      [kRepeatedBase + kUpb_FieldType_UInt64] = OP_VARPCK_LG2(3),
      [kRepeatedBase + kUpb_FieldType_Int32] = OP_VARPCK_LG2(2),
      [kRepeatedBase + kUpb_FieldType_Fixed64] = OP_FIXPCK_LG2(3),
      [kRepeatedBase + kUpb_FieldType_Fixed32] = OP_FIXPCK_LG2(2),
      [kRepeatedBase + kUpb_FieldType_Bool] = OP_VARPCK_LG2(0),
      [kRepeatedBase + kUpb_FieldType_String] = kUpb_DecodeOp_String,
      [kRepeatedBase + kUpb_FieldType_Group] = kUpb_DecodeOp_SubMessage,
      [kRepeatedBase + kUpb_FieldType_Message] = kUpb_DecodeOp_SubMessage,
      [kRepeatedBase + kUpb_FieldType_Bytes] = kUpb_DecodeOp_Bytes,
      [kRepeatedBase + kUpb_FieldType_UInt32] = OP_VARPCK_LG2(2),
      [kRepeatedBase + kUpb_FieldType_Enum] = kUpb_DecodeOp_PackedEnum,
      [kRepeatedBase + kUpb_FieldType_SFixed32] = OP_FIXPCK_LG2(2),
      [kRepeatedBase + kUpb_FieldType_SFixed64] = OP_FIXPCK_LG2(3),
      [kRepeatedBase + kUpb_FieldType_SInt32] = OP_VARPCK_LG2(2),
      [kRepeatedBase + kUpb_FieldType_SInt64] = OP_VARPCK_LG2(3),
      // Omitting kUpb_FakeFieldType_MessageSetItem, because we never emit a
      // repeated msgset type
  };

  int ndx = field->UPB_PRIVATE(descriptortype);
  if (upb_FieldMode_Get(field) == kUpb_FieldMode_Array) ndx += kRepeatedBase;
  int op = kDelimitedOps[ndx];

  if (op == kUpb_DecodeOp_SubMessage) {
    _upb_Decoder_CheckUnlinked(d, mt, field, &op);
  }

  return op;
}

UPB_FORCEINLINE
static const char* _upb_Decoder_DecodeWireValue(upb_Decoder* d, const char* ptr,
                                                const upb_MiniTable* mt,
                                                const upb_MiniTableField* field,
                                                int wire_type, wireval* val,
                                                int* op) {
  static const unsigned kFixed32OkMask = (1 << kUpb_FieldType_Float) |
                                         (1 << kUpb_FieldType_Fixed32) |
                                         (1 << kUpb_FieldType_SFixed32);

  static const unsigned kFixed64OkMask = (1 << kUpb_FieldType_Double) |
                                         (1 << kUpb_FieldType_Fixed64) |
                                         (1 << kUpb_FieldType_SFixed64);

  switch (wire_type) {
    case kUpb_WireType_Varint:
      ptr = _upb_Decoder_DecodeVarint(d, ptr, &val->uint64_val);
      *op = _upb_Decoder_GetVarintOp(field);
      _upb_Decoder_Munge(field->UPB_PRIVATE(descriptortype), val);
      return ptr;
    case kUpb_WireType_32Bit:
      *op = kUpb_DecodeOp_Scalar4Byte;
      if (((1 << field->UPB_PRIVATE(descriptortype)) & kFixed32OkMask) == 0) {
        *op = kUpb_DecodeOp_UnknownField;
      }
      return upb_WireReader_ReadFixed32(ptr, &val->uint32_val);
    case kUpb_WireType_64Bit:
      *op = kUpb_DecodeOp_Scalar8Byte;
      if (((1 << field->UPB_PRIVATE(descriptortype)) & kFixed64OkMask) == 0) {
        *op = kUpb_DecodeOp_UnknownField;
      }
      return upb_WireReader_ReadFixed64(ptr, &val->uint64_val);
    case kUpb_WireType_Delimited:
      ptr = upb_Decoder_DecodeSize(d, ptr, &val->size);
      *op = _upb_Decoder_GetDelimitedOp(d, mt, field);
      return ptr;
    case kUpb_WireType_StartGroup:
      val->uint32_val = field->number;
      if (field->UPB_PRIVATE(descriptortype) == kUpb_FieldType_Group) {
        *op = kUpb_DecodeOp_SubMessage;
        _upb_Decoder_CheckUnlinked(d, mt, field, op);
      } else if (field->UPB_PRIVATE(descriptortype) ==
                 kUpb_FakeFieldType_MessageSetItem) {
        *op = kUpb_DecodeOp_MessageSetItem;
      } else {
        *op = kUpb_DecodeOp_UnknownField;
      }
      return ptr;
    default:
      break;
  }
  _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_Malformed);
}

UPB_FORCEINLINE
static const char* _upb_Decoder_DecodeKnownField(
    upb_Decoder* d, const char* ptr, upb_Message* msg,
    const upb_MiniTable* layout, const upb_MiniTableField* field, int op,
    wireval* val) {
  const upb_MiniTableSub* subs = layout->subs;
  uint8_t mode = field->mode;

  if (UPB_UNLIKELY(mode & kUpb_LabelFlags_IsExtension)) {
    const upb_MiniTableExtension* ext_layout =
        (const upb_MiniTableExtension*)field;
    upb_Message_Extension* ext =
        _upb_Message_GetOrCreateExtension(msg, ext_layout, &d->arena);
    if (UPB_UNLIKELY(!ext)) {
      _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_OutOfMemory);
    }
    d->unknown_msg = msg;
    msg = &ext->data;
    subs = &ext->ext->sub;
  }

  switch (mode & kUpb_FieldMode_Mask) {
    case kUpb_FieldMode_Array:
      return _upb_Decoder_DecodeToArray(d, ptr, msg, subs, field, val, op);
    case kUpb_FieldMode_Map:
      return _upb_Decoder_DecodeToMap(d, ptr, msg, subs, field, val);
    case kUpb_FieldMode_Scalar:
      return _upb_Decoder_DecodeToSubMessage(d, ptr, msg, subs, field, val, op);
    default:
      UPB_UNREACHABLE();
  }
}

static const char* _upb_Decoder_ReverseSkipVarint(const char* ptr,
                                                  uint32_t val) {
  uint32_t seen = 0;
  do {
    ptr--;
    seen <<= 7;
    seen |= *ptr & 0x7f;
  } while (seen != val);
  return ptr;
}

static const char* _upb_Decoder_DecodeUnknownField(upb_Decoder* d,
                                                   const char* ptr,
                                                   upb_Message* msg,
                                                   int field_number,
                                                   int wire_type, wireval val) {
  if (field_number == 0) _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_Malformed);

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
    start = _upb_Decoder_ReverseSkipVarint(start, tag);
    assert(start == d->debug_tagstart);

    if (wire_type == kUpb_WireType_StartGroup) {
      d->unknown = start;
      d->unknown_msg = msg;
      ptr = _upb_Decoder_DecodeUnknownGroup(d, ptr, field_number);
      start = d->unknown;
      d->unknown = NULL;
    }
    if (!_upb_Message_AddUnknown(msg, start, ptr - start, &d->arena)) {
      _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_OutOfMemory);
    }
  } else if (wire_type == kUpb_WireType_StartGroup) {
    ptr = _upb_Decoder_DecodeUnknownGroup(d, ptr, field_number);
  }
  return ptr;
}

UPB_NOINLINE
static const char* _upb_Decoder_DecodeMessage(upb_Decoder* d, const char* ptr,
                                              upb_Message* msg,
                                              const upb_MiniTable* layout) {
  int last_field_index = 0;

#if UPB_FASTTABLE
  // The first time we want to skip fast dispatch, because we may have just been
  // invoked by the fast parser to handle a case that it bailed on.
  if (!_upb_Decoder_IsDone(d, &ptr)) goto nofast;
#endif

  while (!_upb_Decoder_IsDone(d, &ptr)) {
    uint32_t tag;
    const upb_MiniTableField* field;
    int field_number;
    int wire_type;
    wireval val;
    int op;

    if (_upb_Decoder_TryFastDispatch(d, &ptr, msg, layout)) break;

#if UPB_FASTTABLE
  nofast:
#endif

#ifndef NDEBUG
    d->debug_tagstart = ptr;
#endif

    UPB_ASSERT(ptr < d->input.limit_ptr);
    ptr = _upb_Decoder_DecodeTag(d, ptr, &tag);
    field_number = tag >> 3;
    wire_type = tag & 7;

#ifndef NDEBUG
    d->debug_valstart = ptr;
#endif

    if (wire_type == kUpb_WireType_EndGroup) {
      d->end_group = field_number;
      return ptr;
    }

    field = _upb_Decoder_FindField(d, layout, field_number, &last_field_index);
    ptr = _upb_Decoder_DecodeWireValue(d, ptr, layout, field, wire_type, &val,
                                       &op);

    if (op >= 0) {
      ptr = _upb_Decoder_DecodeKnownField(d, ptr, msg, layout, field, op, &val);
    } else {
      switch (op) {
        case kUpb_DecodeOp_UnknownField:
          ptr = _upb_Decoder_DecodeUnknownField(d, ptr, msg, field_number,
                                                wire_type, val);
          break;
        case kUpb_DecodeOp_MessageSetItem:
          ptr = upb_Decoder_DecodeMessageSetItem(d, ptr, msg, layout);
          break;
      }
    }
  }

  return UPB_UNLIKELY(layout && layout->required_count)
             ? _upb_Decoder_CheckRequired(d, ptr, msg, layout)
             : ptr;
}

const char* _upb_FastDecoder_DecodeGeneric(struct upb_Decoder* d,
                                           const char* ptr, upb_Message* msg,
                                           intptr_t table, uint64_t hasbits,
                                           uint64_t data) {
  (void)data;
  *(uint32_t*)msg |= hasbits;
  return _upb_Decoder_DecodeMessage(d, ptr, msg, decode_totablep(table));
}

static upb_DecodeStatus _upb_Decoder_DecodeTop(struct upb_Decoder* d,
                                               const char* buf, void* msg,
                                               const upb_MiniTable* l) {
  if (!_upb_Decoder_TryFastDispatch(d, &buf, msg, l)) {
    _upb_Decoder_DecodeMessage(d, buf, msg, l);
  }
  if (d->end_group != DECODE_NOGROUP) return kUpb_DecodeStatus_Malformed;
  if (d->missing_required) return kUpb_DecodeStatus_MissingRequired;
  return kUpb_DecodeStatus_Ok;
}

UPB_NOINLINE
const char* _upb_Decoder_IsDoneFallback(upb_EpsCopyInputStream* e,
                                        const char* ptr, int overrun) {
  return _upb_EpsCopyInputStream_IsDoneFallbackInline(
      e, ptr, overrun, _upb_Decoder_BufferFlipCallback);
}

static upb_DecodeStatus upb_Decoder_Decode(upb_Decoder* const decoder,
                                           const char* const buf,
                                           void* const msg,
                                           const upb_MiniTable* const l,
                                           upb_Arena* const arena) {
  if (UPB_SETJMP(decoder->err) == 0) {
    decoder->status = _upb_Decoder_DecodeTop(decoder, buf, msg, l);
  } else {
    UPB_ASSERT(decoder->status != kUpb_DecodeStatus_Ok);
  }

  _upb_MemBlock* blocks =
      upb_Atomic_Load(&decoder->arena.blocks, memory_order_relaxed);
  arena->head = decoder->arena.head;
  upb_Atomic_Store(&arena->blocks, blocks, memory_order_relaxed);
  return decoder->status;
}

upb_DecodeStatus upb_Decode(const char* buf, size_t size, void* msg,
                            const upb_MiniTable* l,
                            const upb_ExtensionRegistry* extreg, int options,
                            upb_Arena* arena) {
  upb_Decoder decoder;
  unsigned depth = (unsigned)options >> 16;

  upb_EpsCopyInputStream_Init(&decoder.input, &buf, size,
                              options & kUpb_DecodeOption_AliasString);

  decoder.extreg = extreg;
  decoder.unknown = NULL;
  decoder.depth = depth ? depth : kUpb_WireFormat_DefaultDepthLimit;
  decoder.end_group = DECODE_NOGROUP;
  decoder.options = (uint16_t)options;
  decoder.missing_required = false;
  decoder.status = kUpb_DecodeStatus_Ok;

  // Violating the encapsulation of the arena for performance reasons.
  // This is a temporary arena that we swap into and swap out of when we are
  // done.  The temporary arena only needs to be able to handle allocation,
  // not fuse or free, so it does not need many of the members to be initialized
  // (particularly parent_or_count).
  _upb_MemBlock* blocks = upb_Atomic_Load(&arena->blocks, memory_order_relaxed);
  decoder.arena.head = arena->head;
  decoder.arena.block_alloc = arena->block_alloc;
  upb_Atomic_Init(&decoder.arena.blocks, blocks);

  return upb_Decoder_Decode(&decoder, buf, msg, l, arena);
}

#undef OP_FIXPCK_LG2
#undef OP_VARPCK_LG2

// Fast decoder: ~3x the speed of decode.c, but requires x86-64/ARM64.
// Also the table size grows by 2x.
//
// Could potentially be ported to other 64-bit archs that pass at least six
// arguments in registers and have 8 unused high bits in pointers.
//
// The overall design is to create specialized functions for every possible
// field type (eg. oneof boolean field with a 1 byte tag) and then dispatch
// to the specialized function as quickly as possible.



// Must be last.

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
  return _upb_FastDecoder_DecodeGeneric(d, ptr, msg, table, hasbits, 0);

typedef enum {
  CARD_s = 0, /* Singular (optional, non-repeated) */
  CARD_o = 1, /* Oneof */
  CARD_r = 2, /* Repeated */
  CARD_p = 3  /* Packed Repeated */
} upb_card;

UPB_NOINLINE
static const char* fastdecode_isdonefallback(UPB_PARSE_PARAMS) {
  int overrun = data;
  ptr = _upb_EpsCopyInputStream_IsDoneFallbackInline(
      &d->input, ptr, overrun, _upb_Decoder_BufferFlipCallback);
  data = _upb_FastDecoder_LoadTag(ptr);
  UPB_MUSTTAIL return _upb_FastDecoder_TagDispatch(UPB_PARSE_ARGS);
}

UPB_FORCEINLINE
static const char* fastdecode_dispatch(UPB_PARSE_PARAMS) {
  int overrun;
  switch (upb_EpsCopyInputStream_IsDoneStatus(&d->input, ptr, &overrun)) {
    case kUpb_IsDoneStatus_Done:
      *(uint32_t*)msg |= hasbits;  // Sync hasbits.
      const upb_MiniTable* l = decode_totablep(table);
      return UPB_UNLIKELY(l->required_count)
                 ? _upb_Decoder_CheckRequired(d, ptr, msg, l)
                 : ptr;
    case kUpb_IsDoneStatus_NotDone:
      break;
    case kUpb_IsDoneStatus_NeedFallback:
      data = overrun;
      UPB_MUSTTAIL return fastdecode_isdonefallback(UPB_PARSE_ARGS);
  }

  // Read two bytes of tag data (for a one-byte tag, the high byte is junk).
  data = _upb_FastDecoder_LoadTag(ptr);
  UPB_MUSTTAIL return _upb_FastDecoder_TagDispatch(UPB_PARSE_ARGS);
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
static const char* fastdecode_delimited(
    upb_Decoder* d, const char* ptr,
    upb_EpsCopyInputStream_ParseDelimitedFunc* func, void* ctx) {
  ptr++;

  // Sign-extend so varint greater than one byte becomes negative, causing
  // fast delimited parse to fail.
  int len = (int8_t)ptr[-1];

  if (!upb_EpsCopyInputStream_TryParseDelimitedFast(&d->input, &ptr, len, func,
                                                    ctx)) {
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
    if (!upb_EpsCopyInputStream_CheckSize(&d->input, ptr, len)) {
      // Corrupt wire format: invalid limit.
      return NULL;
    }
    int delta = upb_EpsCopyInputStream_PushLimit(&d->input, ptr, len);
    ptr = func(&d->input, ptr, ctx);
    upb_EpsCopyInputStream_PopLimit(&d->input, ptr, delta);
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
    size_t old_size = farr->arr->capacity;
    size_t old_bytes = old_size * valbytes;
    size_t new_size = old_size * 2;
    size_t new_bytes = new_size * valbytes;
    char* old_ptr = _upb_array_ptr(farr->arr);
    char* new_ptr = upb_Arena_Realloc(&d->arena, old_ptr, old_bytes, new_bytes);
    uint8_t elem_size_lg2 = __builtin_ctz(valbytes);
    farr->arr->capacity = new_size;
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
  farr->arr->size =
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

  if (UPB_LIKELY(!_upb_Decoder_IsDone(d, ptr))) {
    ret.tag = _upb_FastDecoder_LoadTag(*ptr);
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
      farr->end = begin + (farr->arr->capacity * valbytes);
      *data = _upb_FastDecoder_LoadTag(ptr);
      return begin + (farr->arr->size * valbytes);
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
  if (ptr == NULL) _upb_FastDecoder_ErrorJmp(d, kUpb_DecodeStatus_Malformed);  \
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
        UPB_MUSTTAIL return _upb_FastDecoder_TagDispatch(UPB_PARSE_ARGS);      \
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
static const char* fastdecode_topackedvarint(upb_EpsCopyInputStream* e,
                                             const char* ptr, void* ctx) {
  upb_Decoder* d = (upb_Decoder*)e;
  fastdecode_varintdata* data = ctx;
  void* dst = data->dst;
  uint64_t val;

  while (!_upb_Decoder_IsDone(d, &ptr)) {
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
    _upb_FastDecoder_ErrorJmp(d, kUpb_DecodeStatus_Malformed);               \
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
        UPB_MUSTTAIL return _upb_FastDecoder_TagDispatch(UPB_PARSE_ARGS);     \
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
  if (UPB_UNLIKELY(!upb_EpsCopyInputStream_CheckDataSizeAvailable(          \
                       &d->input, ptr, size) ||                             \
                   (size % valbytes) != 0)) {                               \
    _upb_FastDecoder_ErrorJmp(d, kUpb_DecodeStatus_Malformed);              \
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
      _upb_FastDecoder_ErrorJmp(d, kUpb_DecodeStatus_Malformed);            \
    }                                                                       \
  } else {                                                                  \
    _upb_Array_ResizeUninitialized(arr, elems, &d->arena);                  \
  }                                                                         \
                                                                            \
  char* dst = _upb_array_ptr(arr);                                          \
  memcpy(dst, ptr, size);                                                   \
  arr->size = elems;                                                        \
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
  if (!_upb_Decoder_VerifyUtf8Inline(dst->data, dst->size)) {
    _upb_FastDecoder_ErrorJmp(d, kUpb_DecodeStatus_BadUtf8);
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
  if (UPB_UNLIKELY(!upb_EpsCopyInputStream_CheckSize(&d->input, ptr, size))) { \
    dst->size = 0;                                                             \
    _upb_FastDecoder_ErrorJmp(d, kUpb_DecodeStatus_Malformed);                 \
  }                                                                            \
                                                                               \
  const char* s_ptr = ptr;                                                     \
  ptr = upb_EpsCopyInputStream_ReadString(&d->input, &s_ptr, size, &d->arena); \
  if (!ptr) _upb_FastDecoder_ErrorJmp(d, kUpb_DecodeStatus_OutOfMemory);       \
  dst->data = s_ptr;                                                           \
  dst->size = size;                                                            \
                                                                               \
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

#define FASTDECODE_COPYSTRING(d, ptr, msg, table, hasbits, data, tagbytes,     \
                              card, validate_utf8)                             \
  upb_StringView* dst;                                                         \
  fastdecode_arr farr;                                                         \
  int64_t size;                                                                \
  size_t arena_has;                                                            \
  size_t common_has;                                                           \
  char* buf;                                                                   \
                                                                               \
  UPB_ASSERT(!upb_EpsCopyInputStream_AliasingAvailable(&d->input, ptr, 0));    \
  UPB_ASSERT(fastdecode_checktag(data, tagbytes));                             \
                                                                               \
  dst = fastdecode_getfield(d, ptr, msg, &data, &hasbits, &farr,               \
                            sizeof(upb_StringView), card);                     \
                                                                               \
  again:                                                                       \
  if (card == CARD_r) {                                                        \
    dst = fastdecode_resizearr(d, dst, &farr, sizeof(upb_StringView));         \
  }                                                                            \
                                                                               \
  size = (uint8_t)ptr[tagbytes];                                               \
  ptr += tagbytes + 1;                                                         \
  dst->size = size;                                                            \
                                                                               \
  buf = d->arena.head.ptr;                                                     \
  arena_has = _upb_ArenaHas(&d->arena);                                        \
  common_has = UPB_MIN(arena_has,                                              \
                       upb_EpsCopyInputStream_BytesAvailable(&d->input, ptr)); \
                                                                               \
  if (UPB_LIKELY(size <= 15 - tagbytes)) {                                     \
    if (arena_has < 16) goto longstr;                                          \
    d->arena.head.ptr += 16;                                                   \
    memcpy(buf, ptr - tagbytes - 1, 16);                                       \
    dst->data = buf + tagbytes + 1;                                            \
  } else if (UPB_LIKELY(size <= 32)) {                                         \
    if (UPB_UNLIKELY(common_has < 32)) goto longstr;                           \
    fastdecode_docopy(d, ptr, size, 32, buf, dst);                             \
  } else if (UPB_LIKELY(size <= 64)) {                                         \
    if (UPB_UNLIKELY(common_has < 64)) goto longstr;                           \
    fastdecode_docopy(d, ptr, size, 64, buf, dst);                             \
  } else if (UPB_LIKELY(size < 128)) {                                         \
    if (UPB_UNLIKELY(common_has < 128)) goto longstr;                          \
    fastdecode_docopy(d, ptr, size, 128, buf, dst);                            \
  } else {                                                                     \
    goto longstr;                                                              \
  }                                                                            \
                                                                               \
  ptr += size;                                                                 \
                                                                               \
  if (card == CARD_r) {                                                        \
    if (validate_utf8 &&                                                       \
        !_upb_Decoder_VerifyUtf8Inline(dst->data, dst->size)) {                \
      _upb_FastDecoder_ErrorJmp(d, kUpb_DecodeStatus_BadUtf8);                 \
    }                                                                          \
    fastdecode_nextret ret = fastdecode_nextrepeated(                          \
        d, dst, &ptr, &farr, data, tagbytes, sizeof(upb_StringView));          \
    switch (ret.next) {                                                        \
      case FD_NEXT_SAMEFIELD:                                                  \
        dst = ret.dst;                                                         \
        goto again;                                                            \
      case FD_NEXT_OTHERFIELD:                                                 \
        data = ret.tag;                                                        \
        UPB_MUSTTAIL return _upb_FastDecoder_TagDispatch(UPB_PARSE_ARGS);      \
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
  UPB_MUSTTAIL return fastdecode_dispatch(UPB_PARSE_ARGS);                     \
                                                                               \
  longstr:                                                                     \
  if (card == CARD_r) {                                                        \
    fastdecode_commitarr(dst + 1, &farr, sizeof(upb_StringView));              \
  }                                                                            \
  ptr--;                                                                       \
  if (validate_utf8) {                                                         \
    UPB_MUSTTAIL return fastdecode_longstring_utf8(d, ptr, msg, table,         \
                                                   hasbits, (uint64_t)dst);    \
  } else {                                                                     \
    UPB_MUSTTAIL return fastdecode_longstring_noutf8(d, ptr, msg, table,       \
                                                     hasbits, (uint64_t)dst);  \
  }

#define FASTDECODE_STRING(d, ptr, msg, table, hasbits, data, tagbytes, card,  \
                          copyfunc, validate_utf8)                            \
  upb_StringView* dst;                                                        \
  fastdecode_arr farr;                                                        \
  int64_t size;                                                               \
                                                                              \
  if (UPB_UNLIKELY(!fastdecode_checktag(data, tagbytes))) {                   \
    RETURN_GENERIC("string field tag mismatch\n");                            \
  }                                                                           \
                                                                              \
  if (UPB_UNLIKELY(                                                           \
          !upb_EpsCopyInputStream_AliasingAvailable(&d->input, ptr, 0))) {    \
    UPB_MUSTTAIL return copyfunc(UPB_PARSE_ARGS);                             \
  }                                                                           \
                                                                              \
  dst = fastdecode_getfield(d, ptr, msg, &data, &hasbits, &farr,              \
                            sizeof(upb_StringView), card);                    \
                                                                              \
  again:                                                                      \
  if (card == CARD_r) {                                                       \
    dst = fastdecode_resizearr(d, dst, &farr, sizeof(upb_StringView));        \
  }                                                                           \
                                                                              \
  size = (int8_t)ptr[tagbytes];                                               \
  ptr += tagbytes + 1;                                                        \
                                                                              \
  if (UPB_UNLIKELY(                                                           \
          !upb_EpsCopyInputStream_AliasingAvailable(&d->input, ptr, size))) { \
    ptr--;                                                                    \
    if (validate_utf8) {                                                      \
      return fastdecode_longstring_utf8(d, ptr, msg, table, hasbits,          \
                                        (uint64_t)dst);                       \
    } else {                                                                  \
      return fastdecode_longstring_noutf8(d, ptr, msg, table, hasbits,        \
                                          (uint64_t)dst);                     \
    }                                                                         \
  }                                                                           \
                                                                              \
  dst->data = ptr;                                                            \
  dst->size = size;                                                           \
  ptr = upb_EpsCopyInputStream_ReadStringAliased(&d->input, &dst->data,       \
                                                 dst->size);                  \
                                                                              \
  if (card == CARD_r) {                                                       \
    if (validate_utf8 &&                                                      \
        !_upb_Decoder_VerifyUtf8Inline(dst->data, dst->size)) {               \
      _upb_FastDecoder_ErrorJmp(d, kUpb_DecodeStatus_BadUtf8);                \
    }                                                                         \
    fastdecode_nextret ret = fastdecode_nextrepeated(                         \
        d, dst, &ptr, &farr, data, tagbytes, sizeof(upb_StringView));         \
    switch (ret.next) {                                                       \
      case FD_NEXT_SAMEFIELD:                                                 \
        dst = ret.dst;                                                        \
        goto again;                                                           \
      case FD_NEXT_OTHERFIELD:                                                \
        data = ret.tag;                                                       \
        UPB_MUSTTAIL return _upb_FastDecoder_TagDispatch(UPB_PARSE_ARGS);     \
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
static const char* fastdecode_tosubmsg(upb_EpsCopyInputStream* e,
                                       const char* ptr, void* ctx) {
  upb_Decoder* d = (upb_Decoder*)e;
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
    _upb_FastDecoder_ErrorJmp(d, kUpb_DecodeStatus_MaxDepthExceeded);     \
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
    _upb_FastDecoder_ErrorJmp(d, kUpb_DecodeStatus_Malformed);            \
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
        UPB_MUSTTAIL return _upb_FastDecoder_TagDispatch(UPB_PARSE_ARGS); \
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

// We encode backwards, to avoid pre-computing lengths (one-pass encode).


#include <string.h>


// Must be last.

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
  upb_EncodeStatus status;
  jmp_buf err;
  upb_Arena* arena;
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

UPB_NORETURN static void encode_err(upb_encstate* e, upb_EncodeStatus s) {
  UPB_ASSERT(s != kUpb_EncodeStatus_Ok);
  e->status = s;
  UPB_LONGJMP(e->err, 1);
}

UPB_NOINLINE
static void encode_growbuffer(upb_encstate* e, size_t bytes) {
  size_t old_size = e->limit - e->buf;
  size_t new_size = upb_roundup_pow2(bytes + (e->limit - e->ptr));
  char* new_buf = upb_Arena_Realloc(e->arena, e->buf, old_size, new_size);

  if (!new_buf) encode_err(e, kUpb_EncodeStatus_OutOfMemory);

  // We want previous data at the end, realloc() put it at the beginning.
  // TODO(salo): This is somewhat inefficient since we are copying twice.
  // Maybe create a realloc() that copies to the end of the new buffer?
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
  size_t bytes = arr->size * elem_size;
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

static void encode_TaggedMessagePtr(upb_encstate* e,
                                    upb_TaggedMessagePtr tagged,
                                    const upb_MiniTable* m, size_t* size) {
  if (upb_TaggedMessagePtr_IsEmpty(tagged)) {
    m = &_kUpb_MiniTable_Empty;
  }
  encode_message(e, _upb_TaggedMessagePtr_GetMessage(tagged), m, size);
}

static void encode_scalar(upb_encstate* e, const void* _field_mem,
                          const upb_MiniTableSub* subs,
                          const upb_MiniTableField* f) {
  const char* field_mem = _field_mem;
  int wire_type;

#define CASE(ctype, type, wtype, encodeval) \
  {                                         \
    ctype val = *(ctype*)field_mem;         \
    encode_##type(e, encodeval);            \
    wire_type = wtype;                      \
    break;                                  \
  }

  switch (f->UPB_PRIVATE(descriptortype)) {
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
      upb_TaggedMessagePtr submsg = *(upb_TaggedMessagePtr*)field_mem;
      const upb_MiniTable* subm = subs[f->UPB_PRIVATE(submsg_index)].submsg;
      if (submsg == 0) {
        return;
      }
      if (--e->depth == 0) encode_err(e, kUpb_EncodeStatus_MaxDepthExceeded);
      encode_tag(e, f->number, kUpb_WireType_EndGroup);
      encode_TaggedMessagePtr(e, submsg, subm, &size);
      wire_type = kUpb_WireType_StartGroup;
      e->depth++;
      break;
    }
    case kUpb_FieldType_Message: {
      size_t size;
      upb_TaggedMessagePtr submsg = *(upb_TaggedMessagePtr*)field_mem;
      const upb_MiniTable* subm = subs[f->UPB_PRIVATE(submsg_index)].submsg;
      if (submsg == 0) {
        return;
      }
      if (--e->depth == 0) encode_err(e, kUpb_EncodeStatus_MaxDepthExceeded);
      encode_TaggedMessagePtr(e, submsg, subm, &size);
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
                         const upb_MiniTableSub* subs,
                         const upb_MiniTableField* f) {
  const upb_Array* arr = *UPB_PTR_AT(msg, f->offset, upb_Array*);
  bool packed = f->mode & kUpb_LabelFlags_IsPacked;
  size_t pre_len = e->limit - e->ptr;

  if (arr == NULL || arr->size == 0) {
    return;
  }

#define VARINT_CASE(ctype, encode)                                       \
  {                                                                      \
    const ctype* start = _upb_array_constptr(arr);                       \
    const ctype* ptr = start + arr->size;                                \
    uint32_t tag = packed ? 0 : (f->number << 3) | kUpb_WireType_Varint; \
    do {                                                                 \
      ptr--;                                                             \
      encode_varint(e, encode);                                          \
      if (tag) encode_varint(e, tag);                                    \
    } while (ptr != start);                                              \
  }                                                                      \
  break;

#define TAG(wire_type) (packed ? 0 : (f->number << 3 | wire_type))

  switch (f->UPB_PRIVATE(descriptortype)) {
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
      const upb_StringView* ptr = start + arr->size;
      do {
        ptr--;
        encode_bytes(e, ptr->data, ptr->size);
        encode_varint(e, ptr->size);
        encode_tag(e, f->number, kUpb_WireType_Delimited);
      } while (ptr != start);
      return;
    }
    case kUpb_FieldType_Group: {
      const upb_TaggedMessagePtr* start = _upb_array_constptr(arr);
      const upb_TaggedMessagePtr* ptr = start + arr->size;
      const upb_MiniTable* subm = subs[f->UPB_PRIVATE(submsg_index)].submsg;
      if (--e->depth == 0) encode_err(e, kUpb_EncodeStatus_MaxDepthExceeded);
      do {
        size_t size;
        ptr--;
        encode_tag(e, f->number, kUpb_WireType_EndGroup);
        encode_TaggedMessagePtr(e, *ptr, subm, &size);
        encode_tag(e, f->number, kUpb_WireType_StartGroup);
      } while (ptr != start);
      e->depth++;
      return;
    }
    case kUpb_FieldType_Message: {
      const upb_TaggedMessagePtr* start = _upb_array_constptr(arr);
      const upb_TaggedMessagePtr* ptr = start + arr->size;
      const upb_MiniTable* subm = subs[f->UPB_PRIVATE(submsg_index)].submsg;
      if (--e->depth == 0) encode_err(e, kUpb_EncodeStatus_MaxDepthExceeded);
      do {
        size_t size;
        ptr--;
        encode_TaggedMessagePtr(e, *ptr, subm, &size);
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
  const upb_MiniTableField* key_field = &layout->fields[0];
  const upb_MiniTableField* val_field = &layout->fields[1];
  size_t pre_len = e->limit - e->ptr;
  size_t size;
  encode_scalar(e, &ent->data.v, layout->subs, val_field);
  encode_scalar(e, &ent->data.k, layout->subs, key_field);
  size = (e->limit - e->ptr) - pre_len;
  encode_varint(e, size);
  encode_tag(e, number, kUpb_WireType_Delimited);
}

static void encode_map(upb_encstate* e, const upb_Message* msg,
                       const upb_MiniTableSub* subs,
                       const upb_MiniTableField* f) {
  const upb_Map* map = *UPB_PTR_AT(msg, f->offset, const upb_Map*);
  const upb_MiniTable* layout = subs[f->UPB_PRIVATE(submsg_index)].submsg;
  UPB_ASSERT(layout->field_count == 2);

  if (map == NULL) return;

  if (e->options & kUpb_EncodeOption_Deterministic) {
    _upb_sortedmap sorted;
    _upb_mapsorter_pushmap(&e->sorter,
                           layout->fields[0].UPB_PRIVATE(descriptortype), map,
                           &sorted);
    upb_MapEntry ent;
    while (_upb_sortedmap_next(&e->sorter, map, &sorted, &ent)) {
      encode_mapentry(e, f->number, layout, &ent);
    }
    _upb_mapsorter_popmap(&e->sorter, &sorted);
  } else {
    intptr_t iter = UPB_STRTABLE_BEGIN;
    upb_StringView key;
    upb_value val;
    while (upb_strtable_next2(&map->table, &key, &val, &iter)) {
      upb_MapEntry ent;
      _upb_map_fromkey(key, &ent.data.k, map->key_size);
      _upb_map_fromvalue(val, &ent.data.v, map->val_size);
      encode_mapentry(e, f->number, layout, &ent);
    }
  }
}

static bool encode_shouldencode(upb_encstate* e, const upb_Message* msg,
                                const upb_MiniTableSub* subs,
                                const upb_MiniTableField* f) {
  if (f->presence == 0) {
    /* Proto3 presence or map/array. */
    const void* mem = UPB_PTR_AT(msg, f->offset, void);
    switch (_upb_MiniTableField_GetRep(f)) {
      case kUpb_FieldRep_1Byte: {
        char ch;
        memcpy(&ch, mem, 1);
        return ch != 0;
      }
      case kUpb_FieldRep_4Byte: {
        uint32_t u32;
        memcpy(&u32, mem, 4);
        return u32 != 0;
      }
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
                         const upb_MiniTableSub* subs,
                         const upb_MiniTableField* field) {
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

static void encode_msgset_item(upb_encstate* e,
                               const upb_Message_Extension* ext) {
  size_t size;
  encode_tag(e, kUpb_MsgSet_Item, kUpb_WireType_EndGroup);
  encode_message(e, ext->data.ptr, ext->ext->sub.submsg, &size);
  encode_varint(e, size);
  encode_tag(e, kUpb_MsgSet_Message, kUpb_WireType_Delimited);
  encode_varint(e, ext->ext->field.number);
  encode_tag(e, kUpb_MsgSet_TypeId, kUpb_WireType_Varint);
  encode_tag(e, kUpb_MsgSet_Item, kUpb_WireType_StartGroup);
}

static void encode_ext(upb_encstate* e, const upb_Message_Extension* ext,
                       bool is_message_set) {
  if (UPB_UNLIKELY(is_message_set)) {
    encode_msgset_item(e, ext);
  } else {
    encode_field(e, &ext->data, &ext->ext->sub, &ext->ext->field);
  }
}

static void encode_message(upb_encstate* e, const upb_Message* msg,
                           const upb_MiniTable* m, size_t* size) {
  size_t pre_len = e->limit - e->ptr;

  if ((e->options & kUpb_EncodeOption_CheckRequired) && m->required_count) {
    uint64_t msg_head;
    memcpy(&msg_head, msg, 8);
    msg_head = _upb_BigEndian_Swap64(msg_head);
    if (upb_MiniTable_requiredmask(m) & ~msg_head) {
      encode_err(e, kUpb_EncodeStatus_MissingRequired);
    }
  }

  if ((e->options & kUpb_EncodeOption_SkipUnknown) == 0) {
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
      if (e->options & kUpb_EncodeOption_Deterministic) {
        _upb_sortedmap sorted;
        _upb_mapsorter_pushexts(&e->sorter, ext, ext_count, &sorted);
        while (_upb_sortedmap_nextext(&e->sorter, &sorted, &ext)) {
          encode_ext(e, ext, m->ext == kUpb_ExtMode_IsMessageSet);
        }
        _upb_mapsorter_popmap(&e->sorter, &sorted);
      } else {
        const upb_Message_Extension* end = ext + ext_count;
        for (; ext != end; ext++) {
          encode_ext(e, ext, m->ext == kUpb_ExtMode_IsMessageSet);
        }
      }
    }
  }

  if (m->field_count) {
    const upb_MiniTableField* f = &m->fields[m->field_count];
    const upb_MiniTableField* first = &m->fields[0];
    while (f != first) {
      f--;
      if (encode_shouldencode(e, msg, m->subs, f)) {
        encode_field(e, msg, m->subs, f);
      }
    }
  }

  *size = (e->limit - e->ptr) - pre_len;
}

static upb_EncodeStatus upb_Encoder_Encode(upb_encstate* const encoder,
                                           const void* const msg,
                                           const upb_MiniTable* const l,
                                           char** const buf,
                                           size_t* const size) {
  // Unfortunately we must continue to perform hackery here because there are
  // code paths which blindly copy the returned pointer without bothering to
  // check for errors until much later (b/235839510). So we still set *buf to
  // NULL on error and we still set it to non-NULL on a successful empty result.
  if (UPB_SETJMP(encoder->err) == 0) {
    encode_message(encoder, msg, l, size);
    *size = encoder->limit - encoder->ptr;
    if (*size == 0) {
      static char ch;
      *buf = &ch;
    } else {
      UPB_ASSERT(encoder->ptr);
      *buf = encoder->ptr;
    }
  } else {
    UPB_ASSERT(encoder->status != kUpb_EncodeStatus_Ok);
    *buf = NULL;
    *size = 0;
  }

  _upb_mapsorter_destroy(&encoder->sorter);
  return encoder->status;
}

upb_EncodeStatus upb_Encode(const void* msg, const upb_MiniTable* l,
                            int options, upb_Arena* arena, char** buf,
                            size_t* size) {
  upb_encstate e;
  unsigned depth = (unsigned)options >> 16;

  e.status = kUpb_EncodeStatus_Ok;
  e.arena = arena;
  e.buf = NULL;
  e.limit = NULL;
  e.ptr = NULL;
  e.depth = depth ? depth : kUpb_WireFormat_DefaultDepthLimit;
  e.options = options;
  _upb_mapsorter_init(&e.sorter);

  return upb_Encoder_Encode(&e, msg, l, buf, size);
}



// Must be last.

UPB_NOINLINE _upb_WireReader_ReadLongVarintRet
_upb_WireReader_ReadLongVarint(const char* ptr, uint64_t val) {
  _upb_WireReader_ReadLongVarintRet ret = {NULL, 0};
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

const char* _upb_WireReader_SkipGroup(const char* ptr, uint32_t tag,
                                      int depth_limit,
                                      upb_EpsCopyInputStream* stream) {
  if (--depth_limit == 0) return NULL;
  uint32_t end_group_tag = (tag & ~7ULL) | kUpb_WireType_EndGroup;
  while (!upb_EpsCopyInputStream_IsDone(stream, &ptr)) {
    uint32_t tag;
    ptr = upb_WireReader_ReadTag(ptr, &tag);
    if (!ptr) return NULL;
    if (tag == end_group_tag) return ptr;
    ptr = _upb_WireReader_SkipValue(ptr, tag, depth_limit, stream);
    if (!ptr) return NULL;
  }
  return ptr;
}



// Must be last.

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

static char* upb_MtDataEncoder_PutRaw(upb_MtDataEncoder* e, char* ptr,
                                      char ch) {
  upb_MtDataEncoderInternal* in = (upb_MtDataEncoderInternal*)e->internal;
  UPB_ASSERT(ptr - in->buf_start < kUpb_MtDataEncoder_MinSize);
  if (ptr == e->end) return NULL;
  *ptr++ = ch;
  return ptr;
}

static char* upb_MtDataEncoder_Put(upb_MtDataEncoder* e, char* ptr, char ch) {
  return upb_MtDataEncoder_PutRaw(e, ptr, _upb_ToBase92(ch));
}

static char* upb_MtDataEncoder_PutBase92Varint(upb_MtDataEncoder* e, char* ptr,
                                               uint32_t val, int min, int max) {
  int shift = upb_Log2Ceiling(_upb_FromBase92(max) - _upb_FromBase92(min) + 1);
  UPB_ASSERT(shift <= 6);
  uint32_t mask = (1 << shift) - 1;
  do {
    uint32_t bits = val & mask;
    ptr = upb_MtDataEncoder_Put(e, ptr, bits + _upb_FromBase92(min));
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

char* upb_MtDataEncoder_EncodeExtension(upb_MtDataEncoder* e, char* ptr,
                                        upb_FieldType type, uint32_t field_num,
                                        uint64_t field_mod) {
  upb_MtDataEncoderInternal* in = upb_MtDataEncoder_GetInternal(e, ptr);
  in->state.msg_state.msg_modifiers = 0;
  in->state.msg_state.last_field_num = 0;
  in->state.msg_state.oneof_state = kUpb_OneofState_NotStarted;

  ptr = upb_MtDataEncoder_PutRaw(e, ptr, kUpb_EncodedVersion_ExtensionV1);
  if (!ptr) return NULL;

  return upb_MtDataEncoder_PutField(e, ptr, type, field_num, field_mod);
}

char* upb_MtDataEncoder_EncodeMap(upb_MtDataEncoder* e, char* ptr,
                                  upb_FieldType key_type,
                                  upb_FieldType value_type, uint64_t key_mod,
                                  uint64_t value_mod) {
  upb_MtDataEncoderInternal* in = upb_MtDataEncoder_GetInternal(e, ptr);
  in->state.msg_state.msg_modifiers = 0;
  in->state.msg_state.last_field_num = 0;
  in->state.msg_state.oneof_state = kUpb_OneofState_NotStarted;

  ptr = upb_MtDataEncoder_PutRaw(e, ptr, kUpb_EncodedVersion_MapV1);
  if (!ptr) return NULL;

  ptr = upb_MtDataEncoder_PutField(e, ptr, key_type, 1, key_mod);
  if (!ptr) return NULL;

  return upb_MtDataEncoder_PutField(e, ptr, value_type, 2, value_mod);
}

char* upb_MtDataEncoder_EncodeMessageSet(upb_MtDataEncoder* e, char* ptr) {
  (void)upb_MtDataEncoder_GetInternal(e, ptr);
  return upb_MtDataEncoder_PutRaw(e, ptr, kUpb_EncodedVersion_MessageSetV1);
}

char* upb_MtDataEncoder_StartMessage(upb_MtDataEncoder* e, char* ptr,
                                     uint64_t msg_mod) {
  upb_MtDataEncoderInternal* in = upb_MtDataEncoder_GetInternal(e, ptr);
  in->state.msg_state.msg_modifiers = msg_mod;
  in->state.msg_state.last_field_num = 0;
  in->state.msg_state.oneof_state = kUpb_OneofState_NotStarted;

  ptr = upb_MtDataEncoder_PutRaw(e, ptr, kUpb_EncodedVersion_MessageV1);
  if (!ptr) return NULL;

  return upb_MtDataEncoder_PutModifier(e, ptr, msg_mod);
}

static char* _upb_MtDataEncoder_MaybePutFieldSkip(upb_MtDataEncoder* e,
                                                  char* ptr,
                                                  uint32_t field_num) {
  upb_MtDataEncoderInternal* in = (upb_MtDataEncoderInternal*)e->internal;
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
  return ptr;
}

static char* _upb_MtDataEncoder_PutFieldType(upb_MtDataEncoder* e, char* ptr,
                                             upb_FieldType type,
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
      [kUpb_FieldType_Enum] = kUpb_EncodedType_OpenEnum,
      [kUpb_FieldType_SFixed32] = kUpb_EncodedType_SFixed32,
      [kUpb_FieldType_SFixed64] = kUpb_EncodedType_SFixed64,
      [kUpb_FieldType_SInt32] = kUpb_EncodedType_SInt32,
      [kUpb_FieldType_SInt64] = kUpb_EncodedType_SInt64,
  };

  int encoded_type = kUpb_TypeToEncoded[type];

  if (field_mod & kUpb_FieldModifier_IsClosedEnum) {
    UPB_ASSERT(type == kUpb_FieldType_Enum);
    encoded_type = kUpb_EncodedType_ClosedEnum;
  }

  if (field_mod & kUpb_FieldModifier_IsRepeated) {
    // Repeated fields shift the type number up (unlike other modifiers which
    // are bit flags).
    encoded_type += kUpb_EncodedType_RepeatedBase;
  }

  return upb_MtDataEncoder_Put(e, ptr, encoded_type);
}

static char* _upb_MtDataEncoder_MaybePutModifiers(upb_MtDataEncoder* e,
                                                  char* ptr, upb_FieldType type,
                                                  uint64_t field_mod) {
  upb_MtDataEncoderInternal* in = (upb_MtDataEncoderInternal*)e->internal;
  uint32_t encoded_modifiers = 0;
  if ((field_mod & kUpb_FieldModifier_IsRepeated) &&
      upb_FieldType_IsPackable(type)) {
    bool field_is_packed = field_mod & kUpb_FieldModifier_IsPacked;
    bool default_is_packed = in->state.msg_state.msg_modifiers &
                             kUpb_MessageModifier_DefaultIsPacked;
    if (field_is_packed != default_is_packed) {
      encoded_modifiers |= kUpb_EncodedFieldModifier_FlipPacked;
    }
  }

  if (field_mod & kUpb_FieldModifier_IsProto3Singular) {
    encoded_modifiers |= kUpb_EncodedFieldModifier_IsProto3Singular;
  }

  if (field_mod & kUpb_FieldModifier_IsRequired) {
    encoded_modifiers |= kUpb_EncodedFieldModifier_IsRequired;
  }

  return upb_MtDataEncoder_PutModifier(e, ptr, encoded_modifiers);
}

char* upb_MtDataEncoder_PutField(upb_MtDataEncoder* e, char* ptr,
                                 upb_FieldType type, uint32_t field_num,
                                 uint64_t field_mod) {
  upb_MtDataEncoder_GetInternal(e, ptr);

  ptr = _upb_MtDataEncoder_MaybePutFieldSkip(e, ptr, field_num);
  if (!ptr) return NULL;

  ptr = _upb_MtDataEncoder_PutFieldType(e, ptr, type, field_mod);
  if (!ptr) return NULL;

  return _upb_MtDataEncoder_MaybePutModifiers(e, ptr, type, field_mod);
}

char* upb_MtDataEncoder_StartOneof(upb_MtDataEncoder* e, char* ptr) {
  upb_MtDataEncoderInternal* in = upb_MtDataEncoder_GetInternal(e, ptr);
  if (in->state.msg_state.oneof_state == kUpb_OneofState_NotStarted) {
    ptr = upb_MtDataEncoder_Put(e, ptr, _upb_FromBase92(kUpb_EncodedValue_End));
  } else {
    ptr = upb_MtDataEncoder_Put(
        e, ptr, _upb_FromBase92(kUpb_EncodedValue_OneofSeparator));
  }
  in->state.msg_state.oneof_state = kUpb_OneofState_StartedOneof;
  return ptr;
}

char* upb_MtDataEncoder_PutOneofField(upb_MtDataEncoder* e, char* ptr,
                                      uint32_t field_num) {
  upb_MtDataEncoderInternal* in = upb_MtDataEncoder_GetInternal(e, ptr);
  if (in->state.msg_state.oneof_state == kUpb_OneofState_EmittedOneofField) {
    ptr = upb_MtDataEncoder_Put(
        e, ptr, _upb_FromBase92(kUpb_EncodedValue_FieldSeparator));
    if (!ptr) return NULL;
  }
  ptr = upb_MtDataEncoder_PutBase92Varint(e, ptr, field_num, _upb_ToBase92(0),
                                          _upb_ToBase92(63));
  in->state.msg_state.oneof_state = kUpb_OneofState_EmittedOneofField;
  return ptr;
}

char* upb_MtDataEncoder_StartEnum(upb_MtDataEncoder* e, char* ptr) {
  upb_MtDataEncoderInternal* in = upb_MtDataEncoder_GetInternal(e, ptr);
  in->state.enum_state.present_values_mask = 0;
  in->state.enum_state.last_written_value = 0;

  return upb_MtDataEncoder_PutRaw(e, ptr, kUpb_EncodedVersion_EnumV1);
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
    if (!ptr) {
      return NULL;
    }
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


const char _kUpb_ToBase92[] = {
    ' ', '!', '#', '$', '%', '&', '(', ')', '*', '+', ',', '-', '.', '/',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', ';', '<', '=',
    '>', '?', '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K',
    'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y',
    'Z', '[', ']', '^', '_', '`', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
    'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
    'w', 'x', 'y', 'z', '{', '|', '}', '~',
};

const int8_t _kUpb_FromBase92[] = {
    0,  1,  -1, 2,  3,  4,  5,  -1, 6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
    36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54,
    55, 56, 57, -1, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72,
    73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91,
};



// Must be last.

typedef struct {
  upb_MdDecoder base;
  upb_Arena* arena;
  upb_MiniTableEnum* enum_table;
  uint32_t enum_value_count;
  uint32_t enum_data_count;
  uint32_t enum_data_capacity;
} upb_MdEnumDecoder;

static size_t upb_MiniTableEnum_Size(size_t count) {
  return sizeof(upb_MiniTableEnum) + count * sizeof(uint32_t);
}

static upb_MiniTableEnum* _upb_MiniTable_AddEnumDataMember(upb_MdEnumDecoder* d,
                                                           uint32_t val) {
  if (d->enum_data_count == d->enum_data_capacity) {
    size_t old_sz = upb_MiniTableEnum_Size(d->enum_data_capacity);
    d->enum_data_capacity = UPB_MAX(2, d->enum_data_capacity * 2);
    size_t new_sz = upb_MiniTableEnum_Size(d->enum_data_capacity);
    d->enum_table = upb_Arena_Realloc(d->arena, d->enum_table, old_sz, new_sz);
    upb_MdDecoder_CheckOutOfMemory(&d->base, d->enum_table);
  }
  d->enum_table->data[d->enum_data_count++] = val;
  return d->enum_table;
}

static void upb_MiniTableEnum_BuildValue(upb_MdEnumDecoder* d, uint32_t val) {
  upb_MiniTableEnum* table = d->enum_table;
  d->enum_value_count++;
  if (table->value_count || (val > 512 && d->enum_value_count < val / 32)) {
    if (table->value_count == 0) {
      assert(d->enum_data_count == table->mask_limit / 32);
    }
    table = _upb_MiniTable_AddEnumDataMember(d, val);
    table->value_count++;
  } else {
    uint32_t new_mask_limit = ((val / 32) + 1) * 32;
    while (table->mask_limit < new_mask_limit) {
      table = _upb_MiniTable_AddEnumDataMember(d, 0);
      table->mask_limit += 32;
    }
    table->data[val / 32] |= 1ULL << (val % 32);
  }
}

static upb_MiniTableEnum* upb_MtDecoder_DoBuildMiniTableEnum(
    upb_MdEnumDecoder* d, const char* data, size_t len) {
  // If the string is non-empty then it must begin with a version tag.
  if (len) {
    if (*data != kUpb_EncodedVersion_EnumV1) {
      upb_MdDecoder_ErrorJmp(&d->base, "Invalid enum version: %c", *data);
    }
    data++;
    len--;
  }

  upb_MdDecoder_CheckOutOfMemory(&d->base, d->enum_table);

  // Guarantee at least 64 bits of mask without checking mask size.
  d->enum_table->mask_limit = 64;
  d->enum_table = _upb_MiniTable_AddEnumDataMember(d, 0);
  d->enum_table = _upb_MiniTable_AddEnumDataMember(d, 0);

  d->enum_table->value_count = 0;

  const char* ptr = data;
  uint32_t base = 0;

  while (ptr < d->base.end) {
    char ch = *ptr++;
    if (ch <= kUpb_EncodedValue_MaxEnumMask) {
      uint32_t mask = _upb_FromBase92(ch);
      for (int i = 0; i < 5; i++, base++, mask >>= 1) {
        if (mask & 1) upb_MiniTableEnum_BuildValue(d, base);
      }
    } else if (kUpb_EncodedValue_MinSkip <= ch &&
               ch <= kUpb_EncodedValue_MaxSkip) {
      uint32_t skip;
      ptr = upb_MdDecoder_DecodeBase92Varint(&d->base, ptr, ch,
                                             kUpb_EncodedValue_MinSkip,
                                             kUpb_EncodedValue_MaxSkip, &skip);
      base += skip;
    } else {
      upb_MdDecoder_ErrorJmp(&d->base, "Unexpected character: %c", ch);
    }
  }

  return d->enum_table;
}

static upb_MiniTableEnum* upb_MtDecoder_BuildMiniTableEnum(
    upb_MdEnumDecoder* const decoder, const char* const data, size_t const len) {
  if (UPB_SETJMP(decoder->base.err) != 0) return NULL;
  return upb_MtDecoder_DoBuildMiniTableEnum(decoder, data, len);
}

upb_MiniTableEnum* upb_MiniDescriptor_BuildEnum(const char* data, size_t len,
                                                upb_Arena* arena,
                                                upb_Status* status) {
  upb_MdEnumDecoder decoder = {
      .base =
          {
              .end = UPB_PTRADD(data, len),
              .status = status,
          },
      .arena = arena,
      .enum_table = upb_Arena_Malloc(arena, upb_MiniTableEnum_Size(2)),
      .enum_value_count = 0,
      .enum_data_count = 0,
      .enum_data_capacity = 1,
  };

  return upb_MtDecoder_BuildMiniTableEnum(&decoder, data, len);
}


#include <inttypes.h>
#include <stdlib.h>


// Must be last.

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
  upb_MdDecoder base;
  upb_MiniTable* table;
  upb_MiniTableField* fields;
  upb_MiniTablePlatform platform;
  upb_LayoutItemVector vec;
  upb_Arena* arena;
} upb_MtDecoder;

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

static bool upb_MtDecoder_FieldIsPackable(upb_MiniTableField* field) {
  return (field->mode & kUpb_FieldMode_Array) &&
         upb_FieldType_IsPackable(field->UPB_PRIVATE(descriptortype));
}

typedef struct {
  uint16_t submsg_count;
  uint16_t subenum_count;
} upb_SubCounts;

static void upb_MiniTable_SetTypeAndSub(upb_MiniTableField* field,
                                        upb_FieldType type,
                                        upb_SubCounts* sub_counts,
                                        uint64_t msg_modifiers,
                                        bool is_proto3_enum) {
  if (is_proto3_enum) {
    UPB_ASSERT(type == kUpb_FieldType_Enum);
    type = kUpb_FieldType_Int32;
    field->mode |= kUpb_LabelFlags_IsAlternate;
  } else if (type == kUpb_FieldType_String &&
             !(msg_modifiers & kUpb_MessageModifier_ValidateUtf8)) {
    type = kUpb_FieldType_Bytes;
    field->mode |= kUpb_LabelFlags_IsAlternate;
  }

  field->UPB_PRIVATE(descriptortype) = type;

  if (upb_MtDecoder_FieldIsPackable(field) &&
      (msg_modifiers & kUpb_MessageModifier_DefaultIsPacked)) {
    field->mode |= kUpb_LabelFlags_IsPacked;
  }

  if (type == kUpb_FieldType_Message || type == kUpb_FieldType_Group) {
    field->UPB_PRIVATE(submsg_index) = sub_counts->submsg_count++;
  } else if (type == kUpb_FieldType_Enum) {
    // We will need to update this later once we know the total number of
    // submsg fields.
    field->UPB_PRIVATE(submsg_index) = sub_counts->subenum_count++;
  } else {
    field->UPB_PRIVATE(submsg_index) = kUpb_NoSub;
  }
}

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
    [kUpb_EncodedType_OpenEnum] = kUpb_FieldType_Enum,
    [kUpb_EncodedType_SFixed32] = kUpb_FieldType_SFixed32,
    [kUpb_EncodedType_SFixed64] = kUpb_FieldType_SFixed64,
    [kUpb_EncodedType_SInt32] = kUpb_FieldType_SInt32,
    [kUpb_EncodedType_SInt64] = kUpb_FieldType_SInt64,
    [kUpb_EncodedType_ClosedEnum] = kUpb_FieldType_Enum,
};

static void upb_MiniTable_SetField(upb_MtDecoder* d, uint8_t ch,
                                   upb_MiniTableField* field,
                                   uint64_t msg_modifiers,
                                   upb_SubCounts* sub_counts) {
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
      [kUpb_EncodedType_Bytes] = kUpb_FieldRep_StringView,
      [kUpb_EncodedType_UInt32] = kUpb_FieldRep_4Byte,
      [kUpb_EncodedType_OpenEnum] = kUpb_FieldRep_4Byte,
      [kUpb_EncodedType_SFixed32] = kUpb_FieldRep_4Byte,
      [kUpb_EncodedType_SFixed64] = kUpb_FieldRep_8Byte,
      [kUpb_EncodedType_SInt32] = kUpb_FieldRep_4Byte,
      [kUpb_EncodedType_SInt64] = kUpb_FieldRep_8Byte,
      [kUpb_EncodedType_ClosedEnum] = kUpb_FieldRep_4Byte,
  };

  char pointer_rep = d->platform == kUpb_MiniTablePlatform_32Bit
                         ? kUpb_FieldRep_4Byte
                         : kUpb_FieldRep_8Byte;

  int8_t type = _upb_FromBase92(ch);
  if (ch >= _upb_ToBase92(kUpb_EncodedType_RepeatedBase)) {
    type -= kUpb_EncodedType_RepeatedBase;
    field->mode = kUpb_FieldMode_Array;
    field->mode |= pointer_rep << kUpb_FieldRep_Shift;
    field->offset = kNoPresence;
  } else {
    field->mode = kUpb_FieldMode_Scalar;
    field->offset = kHasbitPresence;
    if (type == kUpb_EncodedType_Group || type == kUpb_EncodedType_Message) {
      field->mode |= pointer_rep << kUpb_FieldRep_Shift;
    } else if ((unsigned long)type >= sizeof(kUpb_EncodedToFieldRep)) {
      upb_MdDecoder_ErrorJmp(&d->base, "Invalid field type: %d", (int)type);
    } else {
      field->mode |= kUpb_EncodedToFieldRep[type] << kUpb_FieldRep_Shift;
    }
  }
  if ((unsigned long)type >= sizeof(kUpb_EncodedToType)) {
    upb_MdDecoder_ErrorJmp(&d->base, "Invalid field type: %d", (int)type);
  }
  upb_MiniTable_SetTypeAndSub(field, kUpb_EncodedToType[type], sub_counts,
                              msg_modifiers, type == kUpb_EncodedType_OpenEnum);
}

static void upb_MtDecoder_ModifyField(upb_MtDecoder* d,
                                      uint32_t message_modifiers,
                                      uint32_t field_modifiers,
                                      upb_MiniTableField* field) {
  if (field_modifiers & kUpb_EncodedFieldModifier_FlipPacked) {
    if (!upb_MtDecoder_FieldIsPackable(field)) {
      upb_MdDecoder_ErrorJmp(&d->base,
                             "Cannot flip packed on unpackable field %" PRIu32,
                             field->number);
    }
    field->mode ^= kUpb_LabelFlags_IsPacked;
  }

  bool singular = field_modifiers & kUpb_EncodedFieldModifier_IsProto3Singular;
  bool required = field_modifiers & kUpb_EncodedFieldModifier_IsRequired;

  // Validate.
  if ((singular || required) && field->offset != kHasbitPresence) {
    upb_MdDecoder_ErrorJmp(&d->base,
                           "Invalid modifier(s) for repeated field %" PRIu32,
                           field->number);
  }
  if (singular && required) {
    upb_MdDecoder_ErrorJmp(
        &d->base, "Field %" PRIu32 " cannot be both singular and required",
        field->number);
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
    upb_MdDecoder_CheckOutOfMemory(&d->base, d->vec.data);
    d->vec.capacity = new_cap;
  }
  d->vec.data[d->vec.size++] = item;
}

static void upb_MtDecoder_PushOneof(upb_MtDecoder* d, upb_LayoutItem item) {
  if (item.field_index == kUpb_LayoutItem_IndexSentinel) {
    upb_MdDecoder_ErrorJmp(&d->base, "Empty oneof");
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
      [kUpb_FieldRep_1Byte] = 1,
      [kUpb_FieldRep_4Byte] = 4,
      [kUpb_FieldRep_StringView] = 8,
      [kUpb_FieldRep_8Byte] = 8,
  };
  static const uint8_t kRepToSize64[] = {
      [kUpb_FieldRep_1Byte] = 1,
      [kUpb_FieldRep_4Byte] = 4,
      [kUpb_FieldRep_StringView] = 16,
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
      [kUpb_FieldRep_1Byte] = 1,
      [kUpb_FieldRep_4Byte] = 4,
      [kUpb_FieldRep_StringView] = 4,
      [kUpb_FieldRep_8Byte] = 8,
  };
  static const uint8_t kRepToAlign64[] = {
      [kUpb_FieldRep_1Byte] = 1,
      [kUpb_FieldRep_4Byte] = 4,
      [kUpb_FieldRep_StringView] = 8,
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
  ptr = upb_MdDecoder_DecodeBase92Varint(
      &d->base, ptr, first_ch, kUpb_EncodedValue_MinOneofField,
      kUpb_EncodedValue_MaxOneofField, &field_num);
  upb_MiniTableField* f =
      (void*)upb_MiniTable_FindFieldByNumber(d->table, field_num);

  if (!f) {
    upb_MdDecoder_ErrorJmp(&d->base,
                           "Couldn't add field number %" PRIu32
                           " to oneof, no such field number.",
                           field_num);
  }
  if (f->offset != kHasbitPresence) {
    upb_MdDecoder_ErrorJmp(
        &d->base,
        "Cannot add repeated, required, or singular field %" PRIu32
        " to oneof.",
        field_num);
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
  while (ptr < d->base.end) {
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
                                               upb_MiniTableField* last_field,
                                               uint64_t* msg_modifiers) {
  uint32_t mod;
  ptr = upb_MdDecoder_DecodeBase92Varint(&d->base, ptr, first_ch,
                                         kUpb_EncodedValue_MinModifier,
                                         kUpb_EncodedValue_MaxModifier, &mod);
  if (last_field) {
    upb_MtDecoder_ModifyField(d, *msg_modifiers, mod, last_field);
  } else {
    if (!d->table) {
      upb_MdDecoder_ErrorJmp(&d->base,
                             "Extensions cannot have message modifiers");
    }
    *msg_modifiers = mod;
  }

  return ptr;
}

static void upb_MtDecoder_AllocateSubs(upb_MtDecoder* d,
                                       upb_SubCounts sub_counts) {
  uint32_t total_count = sub_counts.submsg_count + sub_counts.subenum_count;
  size_t subs_bytes = sizeof(*d->table->subs) * total_count;
  upb_MiniTableSub* subs = upb_Arena_Malloc(d->arena, subs_bytes);
  upb_MdDecoder_CheckOutOfMemory(&d->base, subs);
  uint32_t i = 0;
  for (; i < sub_counts.submsg_count; i++) {
    subs[i].submsg = &_kUpb_MiniTable_Empty;
  }
  if (sub_counts.subenum_count) {
    upb_MiniTableField* f = d->fields;
    upb_MiniTableField* end_f = f + d->table->field_count;
    for (; f < end_f; f++) {
      if (f->UPB_PRIVATE(descriptortype) == kUpb_FieldType_Enum) {
        f->UPB_PRIVATE(submsg_index) += sub_counts.submsg_count;
      }
    }
    for (; i < sub_counts.submsg_count + sub_counts.subenum_count; i++) {
      subs[i].subenum = NULL;
    }
  }
  d->table->subs = subs;
}

static const char* upb_MtDecoder_Parse(upb_MtDecoder* d, const char* ptr,
                                       size_t len, void* fields,
                                       size_t field_size, uint16_t* field_count,
                                       upb_SubCounts* sub_counts) {
  uint64_t msg_modifiers = 0;
  uint32_t last_field_number = 0;
  upb_MiniTableField* last_field = NULL;
  bool need_dense_below = d->table != NULL;

  d->base.end = UPB_PTRADD(ptr, len);

  while (ptr < d->base.end) {
    char ch = *ptr++;
    if (ch <= kUpb_EncodedValue_MaxField) {
      if (!d->table && last_field) {
        // For extensions, consume only a single field and then return.
        return --ptr;
      }
      upb_MiniTableField* field = fields;
      *field_count += 1;
      fields = (char*)fields + field_size;
      field->number = ++last_field_number;
      last_field = field;
      upb_MiniTable_SetField(d, ch, field, msg_modifiers, sub_counts);
    } else if (kUpb_EncodedValue_MinModifier <= ch &&
               ch <= kUpb_EncodedValue_MaxModifier) {
      ptr = upb_MtDecoder_ParseModifier(d, ptr, ch, last_field, &msg_modifiers);
      if (msg_modifiers & kUpb_MessageModifier_IsExtendable) {
        d->table->ext |= kUpb_ExtMode_Extendable;
      }
    } else if (ch == kUpb_EncodedValue_End) {
      if (!d->table) {
        upb_MdDecoder_ErrorJmp(&d->base, "Extensions cannot have oneofs.");
      }
      ptr = upb_MtDecoder_DecodeOneofs(d, ptr);
    } else if (kUpb_EncodedValue_MinSkip <= ch &&
               ch <= kUpb_EncodedValue_MaxSkip) {
      if (need_dense_below) {
        d->table->dense_below = d->table->field_count;
        need_dense_below = false;
      }
      uint32_t skip;
      ptr = upb_MdDecoder_DecodeBase92Varint(&d->base, ptr, ch,
                                             kUpb_EncodedValue_MinSkip,
                                             kUpb_EncodedValue_MaxSkip, &skip);
      last_field_number += skip;
      last_field_number--;  // Next field seen will increment.
    } else {
      upb_MdDecoder_ErrorJmp(&d->base, "Invalid char: %c", ch);
    }
  }

  if (need_dense_below) {
    d->table->dense_below = d->table->field_count;
  }

  return ptr;
}

static void upb_MtDecoder_ParseMessage(upb_MtDecoder* d, const char* data,
                                       size_t len) {
  // Buffer length is an upper bound on the number of fields. We will return
  // what we don't use.
  d->fields = upb_Arena_Malloc(d->arena, sizeof(*d->fields) * len);
  upb_MdDecoder_CheckOutOfMemory(&d->base, d->fields);

  upb_SubCounts sub_counts = {0, 0};
  d->table->field_count = 0;
  d->table->fields = d->fields;
  upb_MtDecoder_Parse(d, data, len, d->fields, sizeof(*d->fields),
                      &d->table->field_count, &sub_counts);

  upb_Arena_ShrinkLast(d->arena, d->fields, sizeof(*d->fields) * len,
                       sizeof(*d->fields) * d->table->field_count);
  d->table->fields = d->fields;
  upb_MtDecoder_AllocateSubs(d, sub_counts);
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
  const int rep_bits = upb_Log2Ceiling(kUpb_FieldRep_Max);
  const int type_bits = upb_Log2Ceiling(kUpb_LayoutItemType_Max);
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
    upb_MiniTableField* f = &d->fields[i];
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

static void upb_MtDecoder_AssignHasbits(upb_MtDecoder* d) {
  upb_MiniTable* ret = d->table;
  int n = ret->field_count;
  int last_hasbit = 0;  // 0 cannot be used.

  // First assign required fields, which must have the lowest hasbits.
  for (int i = 0; i < n; i++) {
    upb_MiniTableField* field = (upb_MiniTableField*)&ret->fields[i];
    if (field->offset == kRequiredPresence) {
      field->presence = ++last_hasbit;
    } else if (field->offset == kNoPresence) {
      field->presence = 0;
    }
  }
  ret->required_count = last_hasbit;

  if (ret->required_count > 63) {
    upb_MdDecoder_ErrorJmp(&d->base, "Too many required fields");
  }

  // Next assign non-required hasbit fields.
  for (int i = 0; i < n; i++) {
    upb_MiniTableField* field = (upb_MiniTableField*)&ret->fields[i];
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
  static const size_t max = UINT16_MAX;
  size_t new_size = ret + size;
  if (new_size > max) {
    upb_MdDecoder_ErrorJmp(
        &d->base, "Message size exceeded maximum size of %zu bytes", max);
  }
  d->table->size = new_size;
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
    upb_MiniTableField* f = &d->fields[item->field_index];
    while (true) {
      f->presence = ~item->offset;
      if (f->offset == kUpb_LayoutItem_IndexSentinel) break;
      UPB_ASSERT(f->offset - kOneofBase < d->table->field_count);
      f = &d->fields[f->offset - kOneofBase];
    }
  }

  // Assign offsets.
  for (upb_LayoutItem* item = d->vec.data; item < end; item++) {
    upb_MiniTableField* f = &d->fields[item->field_index];
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

static void upb_MtDecoder_ValidateEntryField(upb_MtDecoder* d,
                                             const upb_MiniTableField* f,
                                             uint32_t expected_num) {
  const char* name = expected_num == 1 ? "key" : "val";
  if (f->number != expected_num) {
    upb_MdDecoder_ErrorJmp(&d->base,
                           "map %s did not have expected number (%d vs %d)",
                           name, expected_num, (int)f->number);
  }

  if (upb_IsRepeatedOrMap(f)) {
    upb_MdDecoder_ErrorJmp(
        &d->base, "map %s cannot be repeated or map, or be in oneof", name);
  }

  uint32_t not_ok_types;
  if (expected_num == 1) {
    not_ok_types = (1 << kUpb_FieldType_Float) | (1 << kUpb_FieldType_Double) |
                   (1 << kUpb_FieldType_Message) | (1 << kUpb_FieldType_Group) |
                   (1 << kUpb_FieldType_Bytes) | (1 << kUpb_FieldType_Enum);
  } else {
    not_ok_types = 1 << kUpb_FieldType_Group;
  }

  if ((1 << upb_MiniTableField_Type(f)) & not_ok_types) {
    upb_MdDecoder_ErrorJmp(&d->base, "map %s cannot have type %d", name,
                           (int)f->UPB_PRIVATE(descriptortype));
  }
}

static void upb_MtDecoder_ParseMap(upb_MtDecoder* d, const char* data,
                                   size_t len) {
  upb_MtDecoder_ParseMessage(d, data, len);
  upb_MtDecoder_AssignHasbits(d);

  if (UPB_UNLIKELY(d->table->field_count != 2)) {
    upb_MdDecoder_ErrorJmp(&d->base, "%hu fields in map",
                           d->table->field_count);
    UPB_UNREACHABLE();
  }

  upb_LayoutItem* end = UPB_PTRADD(d->vec.data, d->vec.size);
  for (upb_LayoutItem* item = d->vec.data; item < end; item++) {
    if (item->type == kUpb_LayoutItemType_OneofCase) {
      upb_MdDecoder_ErrorJmp(&d->base, "Map entry cannot have oneof");
    }
  }

  upb_MtDecoder_ValidateEntryField(d, &d->table->fields[0], 1);
  upb_MtDecoder_ValidateEntryField(d, &d->table->fields[1], 2);

  // Map entries have a pre-determined layout, regardless of types.
  // NOTE: sync with mini_table/message_internal.h.
  const size_t kv_size = d->platform == kUpb_MiniTablePlatform_32Bit ? 8 : 16;
  const size_t hasbit_size = 8;
  d->fields[0].offset = hasbit_size;
  d->fields[1].offset = hasbit_size + kv_size;
  d->table->size = UPB_ALIGN_UP(hasbit_size + kv_size + kv_size, 8);

  // Map entries have a special bit set to signal it's a map entry, used in
  // upb_MiniTable_SetSubMessage() below.
  d->table->ext |= kUpb_ExtMode_IsMapEntry;
}

static void upb_MtDecoder_ParseMessageSet(upb_MtDecoder* d, const char* data,
                                          size_t len) {
  if (len > 0) {
    upb_MdDecoder_ErrorJmp(&d->base, "Invalid message set encode length: %zu",
                           len);
  }

  upb_MiniTable* ret = d->table;
  ret->size = 0;
  ret->field_count = 0;
  ret->ext = kUpb_ExtMode_IsMessageSet;
  ret->dense_below = 0;
  ret->table_mask = -1;
  ret->required_count = 0;
}

static upb_MiniTable* upb_MtDecoder_DoBuildMiniTableWithBuf(
    upb_MtDecoder* decoder, const char* data, size_t len, void** buf,
    size_t* buf_size) {
  upb_MdDecoder_CheckOutOfMemory(&decoder->base, decoder->table);

  decoder->table->size = 0;
  decoder->table->field_count = 0;
  decoder->table->ext = kUpb_ExtMode_NonExtendable;
  decoder->table->dense_below = 0;
  decoder->table->table_mask = -1;
  decoder->table->required_count = 0;

  // Strip off and verify the version tag.
  if (!len--) goto done;
  const char vers = *data++;

  switch (vers) {
    case kUpb_EncodedVersion_MapV1:
      upb_MtDecoder_ParseMap(decoder, data, len);
      break;

    case kUpb_EncodedVersion_MessageV1:
      upb_MtDecoder_ParseMessage(decoder, data, len);
      upb_MtDecoder_AssignHasbits(decoder);
      upb_MtDecoder_SortLayoutItems(decoder);
      upb_MtDecoder_AssignOffsets(decoder);
      break;

    case kUpb_EncodedVersion_MessageSetV1:
      upb_MtDecoder_ParseMessageSet(decoder, data, len);
      break;

    default:
      upb_MdDecoder_ErrorJmp(&decoder->base, "Invalid message version: %c",
                             vers);
  }

done:
  *buf = decoder->vec.data;
  *buf_size = decoder->vec.capacity * sizeof(*decoder->vec.data);
  return decoder->table;
}

static upb_MiniTable* upb_MtDecoder_BuildMiniTableWithBuf(
    upb_MtDecoder* const decoder, const char* const data, const size_t len,
    void** const buf, size_t* const buf_size) {
  if (UPB_SETJMP(decoder->base.err) != 0) {
    *buf = decoder->vec.data;
    *buf_size = decoder->vec.capacity * sizeof(*decoder->vec.data);
    return NULL;
  }

  return upb_MtDecoder_DoBuildMiniTableWithBuf(decoder, data, len, buf,
                                               buf_size);
}

upb_MiniTable* upb_MiniTable_BuildWithBuf(const char* data, size_t len,
                                          upb_MiniTablePlatform platform,
                                          upb_Arena* arena, void** buf,
                                          size_t* buf_size,
                                          upb_Status* status) {
  upb_MtDecoder decoder = {
      .base = {.status = status},
      .platform = platform,
      .vec =
          {
              .data = *buf,
              .capacity = *buf_size / sizeof(*decoder.vec.data),
              .size = 0,
          },
      .arena = arena,
      .table = upb_Arena_Malloc(arena, sizeof(*decoder.table)),
  };

  return upb_MtDecoder_BuildMiniTableWithBuf(&decoder, data, len, buf,
                                             buf_size);
}

static const char* upb_MtDecoder_DoBuildMiniTableExtension(
    upb_MtDecoder* decoder, const char* data, size_t len,
    upb_MiniTableExtension* ext, const upb_MiniTable* extendee,
    upb_MiniTableSub sub) {
  // If the string is non-empty then it must begin with a version tag.
  if (len) {
    if (*data != kUpb_EncodedVersion_ExtensionV1) {
      upb_MdDecoder_ErrorJmp(&decoder->base, "Invalid ext version: %c", *data);
    }
    data++;
    len--;
  }

  uint16_t count = 0;
  upb_SubCounts sub_counts = {0, 0};
  const char* ret = upb_MtDecoder_Parse(decoder, data, len, ext, sizeof(*ext),
                                        &count, &sub_counts);
  if (!ret || count != 1) return NULL;

  upb_MiniTableField* f = &ext->field;

  f->mode |= kUpb_LabelFlags_IsExtension;
  f->offset = 0;
  f->presence = 0;

  if (extendee->ext & kUpb_ExtMode_IsMessageSet) {
    // Extensions of MessageSet must be messages.
    if (!upb_IsSubMessage(f)) return NULL;

    // Extensions of MessageSet must be non-repeating.
    if ((f->mode & kUpb_FieldMode_Mask) == kUpb_FieldMode_Array) return NULL;
  }

  ext->extendee = extendee;
  ext->sub = sub;

  return ret;
}

static const char* upb_MtDecoder_BuildMiniTableExtension(
    upb_MtDecoder* const decoder, const char* const data, const size_t len,
    upb_MiniTableExtension* const ext, const upb_MiniTable* const extendee,
    const upb_MiniTableSub sub) {
  if (UPB_SETJMP(decoder->base.err) != 0) return NULL;
  return upb_MtDecoder_DoBuildMiniTableExtension(decoder, data, len, ext,
                                                 extendee, sub);
}

const char* _upb_MiniTableExtension_Init(const char* data, size_t len,
                                         upb_MiniTableExtension* ext,
                                         const upb_MiniTable* extendee,
                                         upb_MiniTableSub sub,
                                         upb_MiniTablePlatform platform,
                                         upb_Status* status) {
  upb_MtDecoder decoder = {
      .base = {.status = status},
      .arena = NULL,
      .table = NULL,
      .platform = platform,
  };

  return upb_MtDecoder_BuildMiniTableExtension(&decoder, data, len, ext,
                                               extendee, sub);
}

upb_MiniTableExtension* _upb_MiniTableExtension_Build(
    const char* data, size_t len, const upb_MiniTable* extendee,
    upb_MiniTableSub sub, upb_MiniTablePlatform platform, upb_Arena* arena,
    upb_Status* status) {
  upb_MiniTableExtension* ext =
      upb_Arena_Malloc(arena, sizeof(upb_MiniTableExtension));
  if (UPB_UNLIKELY(!ext)) return NULL;

  const char* ptr = _upb_MiniTableExtension_Init(data, len, ext, extendee, sub,
                                                 platform, status);
  if (UPB_UNLIKELY(!ptr)) return NULL;

  return ext;
}

upb_MiniTable* _upb_MiniTable_Build(const char* data, size_t len,
                                    upb_MiniTablePlatform platform,
                                    upb_Arena* arena, upb_Status* status) {
  void* buf = NULL;
  size_t size = 0;
  upb_MiniTable* ret = upb_MiniTable_BuildWithBuf(data, len, platform, arena,
                                                  &buf, &size, status);
  free(buf);
  return ret;
}


// Must be last.

bool upb_MiniTable_SetSubMessage(upb_MiniTable* table,
                                 upb_MiniTableField* field,
                                 const upb_MiniTable* sub) {
  UPB_ASSERT((uintptr_t)table->fields <= (uintptr_t)field &&
             (uintptr_t)field <
                 (uintptr_t)(table->fields + table->field_count));
  UPB_ASSERT(sub);

  const bool sub_is_map = sub->ext & kUpb_ExtMode_IsMapEntry;

  switch (field->UPB_PRIVATE(descriptortype)) {
    case kUpb_FieldType_Message:
      if (sub_is_map) {
        const bool table_is_map = table->ext & kUpb_ExtMode_IsMapEntry;
        if (UPB_UNLIKELY(table_is_map)) return false;

        field->mode = (field->mode & ~kUpb_FieldMode_Mask) | kUpb_FieldMode_Map;
      }
      break;

    case kUpb_FieldType_Group:
      if (UPB_UNLIKELY(sub_is_map)) return false;
      break;

    default:
      return false;
  }

  upb_MiniTableSub* table_sub =
      (void*)&table->subs[field->UPB_PRIVATE(submsg_index)];
  // TODO(haberman): Add this assert back once YouTube is updated to not call
  // this function repeatedly.
  // UPB_ASSERT(table_sub->submsg == &_kUpb_MiniTable_Empty);
  table_sub->submsg = sub;
  return true;
}

bool upb_MiniTable_SetSubEnum(upb_MiniTable* table, upb_MiniTableField* field,
                              const upb_MiniTableEnum* sub) {
  UPB_ASSERT((uintptr_t)table->fields <= (uintptr_t)field &&
             (uintptr_t)field <
                 (uintptr_t)(table->fields + table->field_count));
  UPB_ASSERT(sub);

  upb_MiniTableSub* table_sub =
      (void*)&table->subs[field->UPB_PRIVATE(submsg_index)];
  table_sub->subenum = sub;
  return true;
}

uint32_t upb_MiniTable_GetSubList(const upb_MiniTable* mt,
                                  const upb_MiniTableField** subs) {
  uint32_t msg_count = 0;
  uint32_t enum_count = 0;

  for (int i = 0; i < mt->field_count; i++) {
    const upb_MiniTableField* f = &mt->fields[i];
    if (upb_MiniTableField_CType(f) == kUpb_CType_Message) {
      *subs = f;
      ++subs;
      msg_count++;
    }
  }

  for (int i = 0; i < mt->field_count; i++) {
    const upb_MiniTableField* f = &mt->fields[i];
    if (upb_MiniTableField_CType(f) == kUpb_CType_Enum) {
      *subs = f;
      ++subs;
      enum_count++;
    }
  }

  return (msg_count << 16) | enum_count;
}

// The list of sub_tables and sub_enums must exactly match the number and order
// of sub-message fields and sub-enum fields given by upb_MiniTable_GetSubList()
// above.
bool upb_MiniTable_Link(upb_MiniTable* mt, const upb_MiniTable** sub_tables,
                        size_t sub_table_count,
                        const upb_MiniTableEnum** sub_enums,
                        size_t sub_enum_count) {
  uint32_t msg_count = 0;
  uint32_t enum_count = 0;

  for (int i = 0; i < mt->field_count; i++) {
    upb_MiniTableField* f = (upb_MiniTableField*)&mt->fields[i];
    if (upb_MiniTableField_CType(f) == kUpb_CType_Message) {
      const upb_MiniTable* sub = sub_tables[msg_count++];
      if (msg_count > sub_table_count) return false;
      if (sub != NULL) {
        if (!upb_MiniTable_SetSubMessage(mt, f, sub)) return false;
      }
    }
  }

  for (int i = 0; i < mt->field_count; i++) {
    upb_MiniTableField* f = (upb_MiniTableField*)&mt->fields[i];
    if (upb_MiniTableField_CType(f) == kUpb_CType_Enum) {
      const upb_MiniTableEnum* sub = sub_enums[enum_count++];
      if (enum_count > sub_enum_count) return false;
      if (sub != NULL) {
        if (!upb_MiniTable_SetSubEnum(mt, f, sub)) return false;
      }
    }
  }

  return true;
}


const struct upb_MiniTable _kUpb_MiniTable_Empty = {
    .subs = NULL,
    .fields = NULL,
    .size = 0,
    .field_count = 0,
    .ext = kUpb_ExtMode_NonExtendable,
    .dense_below = 0,
    .table_mask = -1,
    .required_count = 0,
};



// Must be last.

#define EXTREG_KEY_SIZE (sizeof(upb_MiniTable*) + sizeof(uint32_t))

struct upb_ExtensionRegistry {
  upb_Arena* arena;
  upb_strtable exts;  // Key is upb_MiniTable* concatenated with fieldnum.
};

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

UPB_API bool upb_ExtensionRegistry_Add(upb_ExtensionRegistry* r,
                                       const upb_MiniTableExtension* e) {
  char buf[EXTREG_KEY_SIZE];
  extreg_key(buf, e->extendee, e->field.number);
  if (upb_strtable_lookup2(&r->exts, buf, EXTREG_KEY_SIZE, NULL)) return false;
  return upb_strtable_insert(&r->exts, buf, EXTREG_KEY_SIZE,
                             upb_value_constptr(e), r->arena);
}

bool upb_ExtensionRegistry_AddArray(upb_ExtensionRegistry* r,
                                    const upb_MiniTableExtension** e,
                                    size_t count) {
  const upb_MiniTableExtension** start = e;
  const upb_MiniTableExtension** end = UPB_PTRADD(e, count);
  for (; e < end; e++) {
    if (!upb_ExtensionRegistry_Add(r, *e)) goto failure;
  }
  return true;

failure:
  // Back out the entries previously added.
  for (end = e, e = start; e < end; e++) {
    const upb_MiniTableExtension* ext = *e;
    char buf[EXTREG_KEY_SIZE];
    extreg_key(buf, ext->extendee, ext->field.number);
    upb_strtable_remove2(&r->exts, buf, EXTREG_KEY_SIZE, NULL);
  }
  return false;
}

const upb_MiniTableExtension* upb_ExtensionRegistry_Lookup(
    const upb_ExtensionRegistry* r, const upb_MiniTable* t, uint32_t num) {
  char buf[EXTREG_KEY_SIZE];
  upb_value v;
  extreg_key(buf, t, num);
  if (upb_strtable_lookup2(&r->exts, buf, EXTREG_KEY_SIZE, &v)) {
    return upb_value_getconstptr(v);
  } else {
    return NULL;
  }
}


#include <inttypes.h>


// Must be last.

const upb_MiniTableField* upb_MiniTable_FindFieldByNumber(
    const upb_MiniTable* t, uint32_t number) {
  const size_t i = ((size_t)number) - 1;  // 0 wraps to SIZE_MAX

  // Ideal case: index into dense fields
  if (i < t->dense_below) {
    UPB_ASSERT(t->fields[i].number == number);
    return &t->fields[i];
  }

  // Slow case: binary search
  int lo = t->dense_below;
  int hi = t->field_count - 1;
  while (lo <= hi) {
    int mid = (lo + hi) / 2;
    uint32_t num = t->fields[mid].number;
    if (num < number) {
      lo = mid + 1;
      continue;
    }
    if (num > number) {
      hi = mid - 1;
      continue;
    }
    return &t->fields[mid];
  }
  return NULL;
}

static bool upb_MiniTable_Is_Oneof(const upb_MiniTableField* f) {
  return f->presence < 0;
}

const upb_MiniTableField* upb_MiniTable_GetOneof(const upb_MiniTable* m,
                                                 const upb_MiniTableField* f) {
  if (UPB_UNLIKELY(!upb_MiniTable_Is_Oneof(f))) {
    return NULL;
  }
  const upb_MiniTableField* ptr = &m->fields[0];
  const upb_MiniTableField* end = &m->fields[m->field_count];
  while (++ptr < end) {
    if (ptr->presence == (*f).presence) {
      return ptr;
    }
  }
  return NULL;
}

bool upb_MiniTable_NextOneofField(const upb_MiniTable* m,
                                  const upb_MiniTableField** f) {
  const upb_MiniTableField* ptr = *f;
  const upb_MiniTableField* end = &m->fields[m->field_count];
  while (++ptr < end) {
    if (ptr->presence == (*f)->presence) {
      *f = ptr;
      return true;
    }
  }
  return false;
}

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
