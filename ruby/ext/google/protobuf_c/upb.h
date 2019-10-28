/* Amalgamated source file */
#include <stdint.h>/*
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
/*
** This file contains shared definitions that are widely used across upb.
**
** This is a mixed C/C++ interface that offers a full API to both languages.
** See the top-level README for more information.
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
#include <memory>
namespace upb {
class Arena;
class Status;
template <int N> class InlinedArena;
}
#endif


/* upb_status *****************************************************************/

/* upb_status represents a success or failure status and error message.
 * It owns no resources and allocates no memory, so it should work
 * even in OOM situations. */

/* The maximum length of an error message before it will get truncated. */
#define UPB_STATUS_MAX_MESSAGE 127

typedef struct {
  bool ok;
  char msg[UPB_STATUS_MAX_MESSAGE];  /* Error message; NULL-terminated. */
} upb_status;

#ifdef __cplusplus
extern "C" {
#endif

const char *upb_status_errmsg(const upb_status *status);
bool upb_ok(const upb_status *status);

/* Any of the functions that write to a status object allow status to be NULL,
 * to support use cases where the function's caller does not care about the
 * status message. */
void upb_status_clear(upb_status *status);
void upb_status_seterrmsg(upb_status *status, const char *msg);
void upb_status_seterrf(upb_status *status, const char *fmt, ...);
void upb_status_vseterrf(upb_status *status, const char *fmt, va_list args);

UPB_INLINE void upb_status_setoom(upb_status *status) {
  upb_status_seterrmsg(status, "out of memory");
}

#ifdef __cplusplus
}  /* extern "C" */

class upb::Status {
 public:
  Status() { upb_status_clear(&status_); }

  upb_status* ptr() { return &status_; }

  /* Returns true if there is no error. */
  bool ok() const { return upb_ok(&status_); }

  /* Guaranteed to be NULL-terminated. */
  const char *error_message() const { return upb_status_errmsg(&status_); }

  /* The error message will be truncated if it is longer than
   * UPB_STATUS_MAX_MESSAGE-4. */
  void SetErrorMessage(const char *msg) { upb_status_seterrmsg(&status_, msg); }
  void SetFormattedErrorMessage(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    upb_status_vseterrf(&status_, fmt, args);
    va_end(args);
  }

  /* Resets the status to a successful state with no message. */
  void Clear() { upb_status_clear(&status_); }

 private:
  upb_status status_;
};

#endif  /* __cplusplus */

/** upb_strview ************************************************************/

typedef struct {
  const char *data;
  size_t size;
} upb_strview;

UPB_INLINE upb_strview upb_strview_make(const char *data, size_t size) {
  upb_strview ret;
  ret.data = data;
  ret.size = size;
  return ret;
}

UPB_INLINE upb_strview upb_strview_makez(const char *data) {
  return upb_strview_make(data, strlen(data));
}

UPB_INLINE bool upb_strview_eql(upb_strview a, upb_strview b) {
  return a.size == b.size && memcmp(a.data, b.data, a.size) == 0;
}

#define UPB_STRVIEW_INIT(ptr, len) {ptr, len}

#define UPB_STRVIEW_FORMAT "%.*s"
#define UPB_STRVIEW_ARGS(view) (int)(view).size, (view).data

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
typedef void *upb_alloc_func(upb_alloc *alloc, void *ptr, size_t oldsize,
                             size_t size);

struct upb_alloc {
  upb_alloc_func *func;
};

UPB_INLINE void *upb_malloc(upb_alloc *alloc, size_t size) {
  UPB_ASSERT(alloc);
  return alloc->func(alloc, NULL, 0, size);
}

UPB_INLINE void *upb_realloc(upb_alloc *alloc, void *ptr, size_t oldsize,
                             size_t size) {
  UPB_ASSERT(alloc);
  return alloc->func(alloc, ptr, oldsize, size);
}

UPB_INLINE void upb_free(upb_alloc *alloc, void *ptr) {
  assert(alloc);
  alloc->func(alloc, ptr, 0, 0);
}

/* The global allocator used by upb.  Uses the standard malloc()/free(). */

#ifdef __cplusplus
extern "C" {
#endif

extern upb_alloc upb_alloc_global;

#ifdef __cplusplus
}  /* extern "C" */
#endif

/* Functions that hard-code the global malloc.
 *
 * We still get benefit because we can put custom logic into our global
 * allocator, like injecting out-of-memory faults in debug/testing builds. */

UPB_INLINE void *upb_gmalloc(size_t size) {
  return upb_malloc(&upb_alloc_global, size);
}

UPB_INLINE void *upb_grealloc(void *ptr, size_t oldsize, size_t size) {
  return upb_realloc(&upb_alloc_global, ptr, oldsize, size);
}

UPB_INLINE void upb_gfree(void *ptr) {
  upb_free(&upb_alloc_global, ptr);
}

/* upb_arena ******************************************************************/

/* upb_arena is a specific allocator implementation that uses arena allocation.
 * The user provides an allocator that will be used to allocate the underlying
 * arena blocks.  Arenas by nature do not require the individual allocations
 * to be freed.  However the Arena does allow users to register cleanup
 * functions that will run when the arena is destroyed.
 *
 * A upb_arena is *not* thread-safe.
 *
 * You could write a thread-safe arena allocator that satisfies the
 * upb_alloc interface, but it would not be as efficient for the
 * single-threaded case. */

typedef void upb_cleanup_func(void *ud);

struct upb_arena;
typedef struct upb_arena upb_arena;

#ifdef __cplusplus
extern "C" {
#endif

/* Creates an arena from the given initial block (if any -- n may be 0).
 * Additional blocks will be allocated from |alloc|.  If |alloc| is NULL, this
 * is a fixed-size arena and cannot grow. */
upb_arena *upb_arena_init(void *mem, size_t n, upb_alloc *alloc);
void upb_arena_free(upb_arena *a);
bool upb_arena_addcleanup(upb_arena *a, void *ud, upb_cleanup_func *func);
size_t upb_arena_bytesallocated(const upb_arena *a);

UPB_INLINE upb_alloc *upb_arena_alloc(upb_arena *a) { return (upb_alloc*)a; }

/* Convenience wrappers around upb_alloc functions. */

UPB_INLINE void *upb_arena_malloc(upb_arena *a, size_t size) {
  return upb_malloc(upb_arena_alloc(a), size);
}

UPB_INLINE void *upb_arena_realloc(upb_arena *a, void *ptr, size_t oldsize,
                                   size_t size) {
  return upb_realloc(upb_arena_alloc(a), ptr, oldsize, size);
}

UPB_INLINE upb_arena *upb_arena_new(void) {
  return upb_arena_init(NULL, 0, &upb_alloc_global);
}

#ifdef __cplusplus
}  /* extern "C" */

class upb::Arena {
 public:
  /* A simple arena with no initial memory block and the default allocator. */
  Arena() : ptr_(upb_arena_new(), upb_arena_free) {}

  upb_arena* ptr() { return ptr_.get(); }

  /* Allows this arena to be used as a generic allocator.
   *
   * The arena does not need free() calls so when using Arena as an allocator
   * it is safe to skip them.  However they are no-ops so there is no harm in
   * calling free() either. */
  upb_alloc *allocator() { return upb_arena_alloc(ptr_.get()); }

  /* Add a cleanup function to run when the arena is destroyed.
   * Returns false on out-of-memory. */
  bool AddCleanup(void *ud, upb_cleanup_func* func) {
    return upb_arena_addcleanup(ptr_.get(), ud, func);
  }

  /* Total number of bytes that have been allocated.  It is undefined what
   * Realloc() does to &arena_ counter. */
  size_t BytesAllocated() const { return upb_arena_bytesallocated(ptr_.get()); }

 private:
  std::unique_ptr<upb_arena, decltype(&upb_arena_free)> ptr_;
};

#endif

/* upb::InlinedArena **********************************************************/

/* upb::InlinedArena seeds the arenas with a predefined amount of memory.  No
 * heap memory will be allocated until the initial block is exceeded.
 *
 * These types only exist in C++ */

#ifdef __cplusplus

template <int N> class upb::InlinedArena : public upb::Arena {
 public:
  InlinedArena() : ptr_(upb_arena_new(&initial_block_, N, &upb_alloc_global)) {}

  upb_arena* ptr() { return ptr_.get(); }

 private:
  InlinedArena(const InlinedArena*) = delete;
  InlinedArena& operator=(const InlinedArena*) = delete;

  std::unique_ptr<upb_arena, decltype(&upb_arena_free)> ptr_;
  char initial_block_[N];
};

#endif  /* __cplusplus */

/* Constants ******************************************************************/

/* Generic function type. */
typedef void upb_func(void);

/* A list of types as they are encoded on-the-wire. */
typedef enum {
  UPB_WIRE_TYPE_VARINT      = 0,
  UPB_WIRE_TYPE_64BIT       = 1,
  UPB_WIRE_TYPE_DELIMITED   = 2,
  UPB_WIRE_TYPE_START_GROUP = 3,
  UPB_WIRE_TYPE_END_GROUP   = 4,
  UPB_WIRE_TYPE_32BIT       = 5
} upb_wiretype_t;

/* The types a field can have.  Note that this list is not identical to the
 * types defined in descriptor.proto, which gives INT32 and SINT32 separate
 * types (we distinguish the two with the "integer encoding" enum below). */
typedef enum {
  /* Types stored in 1 byte. */
  UPB_TYPE_BOOL     = 1,
  /* Types stored in 4 bytes. */
  UPB_TYPE_FLOAT    = 2,
  UPB_TYPE_INT32    = 3,
  UPB_TYPE_UINT32   = 4,
  UPB_TYPE_ENUM     = 5,  /* Enum values are int32. */
  /* Types stored as pointers (probably 4 or 8 bytes). */
  UPB_TYPE_STRING   = 6,
  UPB_TYPE_BYTES    = 7,
  UPB_TYPE_MESSAGE  = 8,
  /* Types stored as 8 bytes. */
  UPB_TYPE_DOUBLE   = 9,
  UPB_TYPE_INT64    = 10,
  UPB_TYPE_UINT64   = 11
} upb_fieldtype_t;

/* The repeated-ness of each field; this matches descriptor.proto. */
typedef enum {
  UPB_LABEL_OPTIONAL = 1,
  UPB_LABEL_REQUIRED = 2,
  UPB_LABEL_REPEATED = 3
} upb_label_t;

/* Descriptor types, as defined in descriptor.proto. */
typedef enum {
  UPB_DESCRIPTOR_TYPE_DOUBLE   = 1,
  UPB_DESCRIPTOR_TYPE_FLOAT    = 2,
  UPB_DESCRIPTOR_TYPE_INT64    = 3,
  UPB_DESCRIPTOR_TYPE_UINT64   = 4,
  UPB_DESCRIPTOR_TYPE_INT32    = 5,
  UPB_DESCRIPTOR_TYPE_FIXED64  = 6,
  UPB_DESCRIPTOR_TYPE_FIXED32  = 7,
  UPB_DESCRIPTOR_TYPE_BOOL     = 8,
  UPB_DESCRIPTOR_TYPE_STRING   = 9,
  UPB_DESCRIPTOR_TYPE_GROUP    = 10,
  UPB_DESCRIPTOR_TYPE_MESSAGE  = 11,
  UPB_DESCRIPTOR_TYPE_BYTES    = 12,
  UPB_DESCRIPTOR_TYPE_UINT32   = 13,
  UPB_DESCRIPTOR_TYPE_ENUM     = 14,
  UPB_DESCRIPTOR_TYPE_SFIXED32 = 15,
  UPB_DESCRIPTOR_TYPE_SFIXED64 = 16,
  UPB_DESCRIPTOR_TYPE_SINT32   = 17,
  UPB_DESCRIPTOR_TYPE_SINT64   = 18
} upb_descriptortype_t;

extern const uint8_t upb_desctype_to_fieldtype[];


#endif  /* UPB_H_ */
/*
** upb_decode: parsing into a upb_msg using a upb_msglayout.
*/

#ifndef UPB_DECODE_H_
#define UPB_DECODE_H_

/*
** Data structures for message tables, used for parsing and serialization.
** This are much lighter-weight than full reflection, but they are do not
** have enough information to convert to text format, JSON, etc.
**
** The definitions in this file are internal to upb.
**/

#ifndef UPB_MSG_H_
#define UPB_MSG_H_

#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void upb_msg;

/** upb_msglayout *************************************************************/

/* upb_msglayout represents the memory layout of a given upb_msgdef.  The
 * members are public so generated code can initialize them, but users MUST NOT
 * read or write any of its members. */

typedef struct {
  uint32_t number;
  uint16_t offset;
  int16_t presence;      /* If >0, hasbit_index+1.  If <0, oneof_index+1. */
  uint16_t submsg_index;  /* undefined if descriptortype != MESSAGE or GROUP. */
  uint8_t descriptortype;
  uint8_t label;
} upb_msglayout_field;

typedef struct upb_msglayout {
  const struct upb_msglayout *const* submsgs;
  const upb_msglayout_field *fields;
  /* Must be aligned to sizeof(void*).  Doesn't include internal members like
   * unknown fields, extension dict, pointer to msglayout, etc. */
  uint16_t size;
  uint16_t field_count;
  bool extendable;
} upb_msglayout;

/** Message internal representation *******************************************/

/* Our internal representation for repeated fields. */
typedef struct {
  void *data;   /* Each element is element_size. */
  size_t len;   /* Measured in elements. */
  size_t size;  /* Measured in elements. */
} upb_array;

upb_msg *upb_msg_new(const upb_msglayout *l, upb_arena *a);
upb_msg *upb_msg_new(const upb_msglayout *l, upb_arena *a);

void upb_msg_addunknown(upb_msg *msg, const char *data, size_t len,
                        upb_arena *arena);
const char *upb_msg_getunknown(const upb_msg *msg, size_t *len);

upb_array *upb_array_new(upb_arena *a);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif /* UPB_MSG_H_ */

#ifdef __cplusplus
extern "C" {
#endif

bool upb_decode(const char *buf, size_t size, upb_msg *msg,
                const upb_msglayout *l, upb_arena *arena);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_DECODE_H_ */
/*
** upb_encode: parsing into a upb_msg using a upb_msglayout.
*/

#ifndef UPB_ENCODE_H_
#define UPB_ENCODE_H_


#ifdef __cplusplus
extern "C" {
#endif

char *upb_encode(const void *msg, const upb_msglayout *l, upb_arena *arena,
                 size_t *size);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_ENCODE_H_ */
/*
** upb_table
**
** This header is INTERNAL-ONLY!  Its interfaces are not public or stable!
** This file defines very fast int->upb_value (inttable) and string->upb_value
** (strtable) hash tables.
**
** The table uses chained scatter with Brent's variation (inspired by the Lua
** implementation of hash tables).  The hash function for strings is Austin
** Appleby's "MurmurHash."
**
** The inttable uses uintptr_t as its key, which guarantees it can be used to
** store pointers or integers of at least 32 bits (upb isn't really useful on
** systems where sizeof(void*) < 4).
**
** The table must be homogenous (all values of the same type).  In debug
** mode, we check this on insert and lookup.
*/

#ifndef UPB_TABLE_H_
#define UPB_TABLE_H_

#include <stdint.h>
#include <string.h>


#ifdef __cplusplus
extern "C" {
#endif


/* upb_value ******************************************************************/

/* A tagged union (stored untagged inside the table) so that we can check that
 * clients calling table accessors are correctly typed without having to have
 * an explosion of accessors. */
typedef enum {
  UPB_CTYPE_INT32    = 1,
  UPB_CTYPE_INT64    = 2,
  UPB_CTYPE_UINT32   = 3,
  UPB_CTYPE_UINT64   = 4,
  UPB_CTYPE_BOOL     = 5,
  UPB_CTYPE_CSTR     = 6,
  UPB_CTYPE_PTR      = 7,
  UPB_CTYPE_CONSTPTR = 8,
  UPB_CTYPE_FPTR     = 9,
  UPB_CTYPE_FLOAT    = 10,
  UPB_CTYPE_DOUBLE   = 11
} upb_ctype_t;

typedef struct {
  uint64_t val;
#ifndef NDEBUG
  /* In debug mode we carry the value type around also so we can check accesses
   * to be sure the right member is being read. */
  upb_ctype_t ctype;
#endif
} upb_value;

#ifdef NDEBUG
#define SET_TYPE(dest, val)      UPB_UNUSED(val)
#else
#define SET_TYPE(dest, val) dest = val
#endif

/* Like strdup(), which isn't always available since it's not ANSI C. */
char *upb_strdup(const char *s, upb_alloc *a);
/* Variant that works with a length-delimited rather than NULL-delimited string,
 * as supported by strtable. */
char *upb_strdup2(const char *s, size_t len, upb_alloc *a);

UPB_INLINE char *upb_gstrdup(const char *s) {
  return upb_strdup(s, &upb_alloc_global);
}

UPB_INLINE void _upb_value_setval(upb_value *v, uint64_t val,
                                  upb_ctype_t ctype) {
  v->val = val;
  SET_TYPE(v->ctype, ctype);
}

UPB_INLINE upb_value _upb_value_val(uint64_t val, upb_ctype_t ctype) {
  upb_value ret;
  _upb_value_setval(&ret, val, ctype);
  return ret;
}

/* For each value ctype, define the following set of functions:
 *
 * // Get/set an int32 from a upb_value.
 * int32_t upb_value_getint32(upb_value val);
 * void upb_value_setint32(upb_value *val, int32_t cval);
 *
 * // Construct a new upb_value from an int32.
 * upb_value upb_value_int32(int32_t val); */
#define FUNCS(name, membername, type_t, converter, proto_type) \
  UPB_INLINE void upb_value_set ## name(upb_value *val, type_t cval) { \
    val->val = (converter)cval; \
    SET_TYPE(val->ctype, proto_type); \
  } \
  UPB_INLINE upb_value upb_value_ ## name(type_t val) { \
    upb_value ret; \
    upb_value_set ## name(&ret, val); \
    return ret; \
  } \
  UPB_INLINE type_t upb_value_get ## name(upb_value val) { \
    UPB_ASSERT_DEBUGVAR(val.ctype == proto_type); \
    return (type_t)(converter)val.val; \
  }

FUNCS(int32,    int32,        int32_t,      int32_t,    UPB_CTYPE_INT32)
FUNCS(int64,    int64,        int64_t,      int64_t,    UPB_CTYPE_INT64)
FUNCS(uint32,   uint32,       uint32_t,     uint32_t,   UPB_CTYPE_UINT32)
FUNCS(uint64,   uint64,       uint64_t,     uint64_t,   UPB_CTYPE_UINT64)
FUNCS(bool,     _bool,        bool,         bool,       UPB_CTYPE_BOOL)
FUNCS(cstr,     cstr,         char*,        uintptr_t,  UPB_CTYPE_CSTR)
FUNCS(ptr,      ptr,          void*,        uintptr_t,  UPB_CTYPE_PTR)
FUNCS(constptr, constptr,     const void*,  uintptr_t,  UPB_CTYPE_CONSTPTR)
FUNCS(fptr,     fptr,         upb_func*,    uintptr_t,  UPB_CTYPE_FPTR)

#undef FUNCS

UPB_INLINE void upb_value_setfloat(upb_value *val, float cval) {
  memcpy(&val->val, &cval, sizeof(cval));
  SET_TYPE(val->ctype, UPB_CTYPE_FLOAT);
}

UPB_INLINE void upb_value_setdouble(upb_value *val, double cval) {
  memcpy(&val->val, &cval, sizeof(cval));
  SET_TYPE(val->ctype, UPB_CTYPE_DOUBLE);
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

UPB_INLINE char *upb_tabstr(upb_tabkey key, uint32_t *len) {
  char* mem = (char*)key;
  if (len) memcpy(len, mem, sizeof(*len));
  return mem + sizeof(*len);
}


/* upb_tabval *****************************************************************/

typedef struct {
  uint64_t val;
} upb_tabval;

#define UPB_TABVALUE_EMPTY_INIT  {-1}


/* upb_table ******************************************************************/

typedef struct _upb_tabent {
  upb_tabkey key;
  upb_tabval val;

  /* Internal chaining.  This is const so we can create static initializers for
   * tables.  We cast away const sometimes, but *only* when the containing
   * upb_table is known to be non-const.  This requires a bit of care, but
   * the subtlety is confined to table.c. */
  const struct _upb_tabent *next;
} upb_tabent;

typedef struct {
  size_t count;          /* Number of entries in the hash part. */
  size_t mask;           /* Mask to turn hash value -> bucket. */
  upb_ctype_t ctype;     /* Type of all values. */
  uint8_t size_lg2;      /* Size of the hashtable part is 2^size_lg2 entries. */

  /* Hash table entries.
   * Making this const isn't entirely accurate; what we really want is for it to
   * have the same const-ness as the table it's inside.  But there's no way to
   * declare that in C.  So we have to make it const so that we can statically
   * initialize const hash tables.  Then we cast away const when we have to.
   */
  const upb_tabent *entries;

#ifndef NDEBUG
  /* This table's allocator.  We make the user pass it in to every relevant
   * function and only use this to check it in debug mode.  We do this solely
   * to keep upb_table as small as possible.  This might seem slightly paranoid
   * but the plan is to use upb_table for all map fields and extension sets in
   * a forthcoming message representation, so there could be a lot of these.
   * If this turns out to be too annoying later, we can change it (since this
   * is an internal-only header file). */
  upb_alloc *alloc;
#endif
} upb_table;

typedef struct {
  upb_table t;
} upb_strtable;

typedef struct {
  upb_table t;              /* For entries that don't fit in the array part. */
  const upb_tabval *array;  /* Array part of the table. See const note above. */
  size_t array_size;        /* Array part size. */
  size_t array_count;       /* Array part number of elements. */
} upb_inttable;

#define UPB_INTTABLE_INIT(count, mask, ctype, size_lg2, ent, a, asize, acount) \
  {UPB_TABLE_INIT(count, mask, ctype, size_lg2, ent), a, asize, acount}

#define UPB_EMPTY_INTTABLE_INIT(ctype) \
  UPB_INTTABLE_INIT(0, 0, ctype, 0, NULL, NULL, 0, 0)

#define UPB_ARRAY_EMPTYENT -1

UPB_INLINE size_t upb_table_size(const upb_table *t) {
  if (t->size_lg2 == 0)
    return 0;
  else
    return 1 << t->size_lg2;
}

/* Internal-only functions, in .h file only out of necessity. */
UPB_INLINE bool upb_tabent_isempty(const upb_tabent *e) {
  return e->key == 0;
}

/* Used by some of the unit tests for generic hashing functionality. */
uint32_t upb_murmur_hash2(const void * key, size_t len, uint32_t seed);

UPB_INLINE uintptr_t upb_intkey(uintptr_t key) {
  return key;
}

UPB_INLINE uint32_t upb_inthash(uintptr_t key) {
  return (uint32_t)key;
}

static const upb_tabent *upb_getentry(const upb_table *t, uint32_t hash) {
  return t->entries + (hash & t->mask);
}

UPB_INLINE bool upb_arrhas(upb_tabval key) {
  return key.val != (uint64_t)-1;
}

/* Initialize and uninitialize a table, respectively.  If memory allocation
 * failed, false is returned that the table is uninitialized. */
bool upb_inttable_init2(upb_inttable *table, upb_ctype_t ctype, upb_alloc *a);
bool upb_strtable_init2(upb_strtable *table, upb_ctype_t ctype, upb_alloc *a);
void upb_inttable_uninit2(upb_inttable *table, upb_alloc *a);
void upb_strtable_uninit2(upb_strtable *table, upb_alloc *a);

UPB_INLINE bool upb_inttable_init(upb_inttable *table, upb_ctype_t ctype) {
  return upb_inttable_init2(table, ctype, &upb_alloc_global);
}

UPB_INLINE bool upb_strtable_init(upb_strtable *table, upb_ctype_t ctype) {
  return upb_strtable_init2(table, ctype, &upb_alloc_global);
}

UPB_INLINE void upb_inttable_uninit(upb_inttable *table) {
  upb_inttable_uninit2(table, &upb_alloc_global);
}

UPB_INLINE void upb_strtable_uninit(upb_strtable *table) {
  upb_strtable_uninit2(table, &upb_alloc_global);
}

/* Returns the number of values in the table. */
size_t upb_inttable_count(const upb_inttable *t);
UPB_INLINE size_t upb_strtable_count(const upb_strtable *t) {
  return t->t.count;
}

void upb_inttable_packedsize(const upb_inttable *t, size_t *size);
void upb_strtable_packedsize(const upb_strtable *t, size_t *size);
upb_inttable *upb_inttable_pack(const upb_inttable *t, void *p, size_t *ofs,
                                size_t size);
upb_strtable *upb_strtable_pack(const upb_strtable *t, void *p, size_t *ofs,
                                size_t size);

/* Inserts the given key into the hashtable with the given value.  The key must
 * not already exist in the hash table.  For string tables, the key must be
 * NULL-terminated, and the table will make an internal copy of the key.
 * Inttables must not insert a value of UINTPTR_MAX.
 *
 * If a table resize was required but memory allocation failed, false is
 * returned and the table is unchanged. */
bool upb_inttable_insert2(upb_inttable *t, uintptr_t key, upb_value val,
                          upb_alloc *a);
bool upb_strtable_insert3(upb_strtable *t, const char *key, size_t len,
                          upb_value val, upb_alloc *a);

UPB_INLINE bool upb_inttable_insert(upb_inttable *t, uintptr_t key,
                                    upb_value val) {
  return upb_inttable_insert2(t, key, val, &upb_alloc_global);
}

UPB_INLINE bool upb_strtable_insert2(upb_strtable *t, const char *key,
                                     size_t len, upb_value val) {
  return upb_strtable_insert3(t, key, len, val, &upb_alloc_global);
}

/* For NULL-terminated strings. */
UPB_INLINE bool upb_strtable_insert(upb_strtable *t, const char *key,
                                    upb_value val) {
  return upb_strtable_insert2(t, key, strlen(key), val);
}

/* Looks up key in this table, returning "true" if the key was found.
 * If v is non-NULL, copies the value for this key into *v. */
bool upb_inttable_lookup(const upb_inttable *t, uintptr_t key, upb_value *v);
bool upb_strtable_lookup2(const upb_strtable *t, const char *key, size_t len,
                          upb_value *v);

/* For NULL-terminated strings. */
UPB_INLINE bool upb_strtable_lookup(const upb_strtable *t, const char *key,
                                    upb_value *v) {
  return upb_strtable_lookup2(t, key, strlen(key), v);
}

/* Removes an item from the table.  Returns true if the remove was successful,
 * and stores the removed item in *val if non-NULL. */
bool upb_inttable_remove(upb_inttable *t, uintptr_t key, upb_value *val);
bool upb_strtable_remove3(upb_strtable *t, const char *key, size_t len,
                          upb_value *val, upb_alloc *alloc);

UPB_INLINE bool upb_strtable_remove2(upb_strtable *t, const char *key,
                                     size_t len, upb_value *val) {
  return upb_strtable_remove3(t, key, len, val, &upb_alloc_global);
}

/* For NULL-terminated strings. */
UPB_INLINE bool upb_strtable_remove(upb_strtable *t, const char *key,
                                    upb_value *v) {
  return upb_strtable_remove2(t, key, strlen(key), v);
}

/* Updates an existing entry in an inttable.  If the entry does not exist,
 * returns false and does nothing.  Unlike insert/remove, this does not
 * invalidate iterators. */
bool upb_inttable_replace(upb_inttable *t, uintptr_t key, upb_value val);

/* Handy routines for treating an inttable like a stack.  May not be mixed with
 * other insert/remove calls. */
bool upb_inttable_push2(upb_inttable *t, upb_value val, upb_alloc *a);
upb_value upb_inttable_pop(upb_inttable *t);

UPB_INLINE bool upb_inttable_push(upb_inttable *t, upb_value val) {
  return upb_inttable_push2(t, val, &upb_alloc_global);
}

/* Convenience routines for inttables with pointer keys. */
bool upb_inttable_insertptr2(upb_inttable *t, const void *key, upb_value val,
                             upb_alloc *a);
bool upb_inttable_removeptr(upb_inttable *t, const void *key, upb_value *val);
bool upb_inttable_lookupptr(
    const upb_inttable *t, const void *key, upb_value *val);

UPB_INLINE bool upb_inttable_insertptr(upb_inttable *t, const void *key,
                                       upb_value val) {
  return upb_inttable_insertptr2(t, key, val, &upb_alloc_global);
}

/* Optimizes the table for the current set of entries, for both memory use and
 * lookup time.  Client should call this after all entries have been inserted;
 * inserting more entries is legal, but will likely require a table resize. */
void upb_inttable_compact2(upb_inttable *t, upb_alloc *a);

UPB_INLINE void upb_inttable_compact(upb_inttable *t) {
  upb_inttable_compact2(t, &upb_alloc_global);
}

/* A special-case inlinable version of the lookup routine for 32-bit
 * integers. */
UPB_INLINE bool upb_inttable_lookup32(const upb_inttable *t, uint32_t key,
                                      upb_value *v) {
  *v = upb_value_int32(0);  /* Silence compiler warnings. */
  if (key < t->array_size) {
    upb_tabval arrval = t->array[key];
    if (upb_arrhas(arrval)) {
      _upb_value_setval(v, arrval.val, t->t.ctype);
      return true;
    } else {
      return false;
    }
  } else {
    const upb_tabent *e;
    if (t->t.entries == NULL) return false;
    for (e = upb_getentry(&t->t, upb_inthash(key)); true; e = e->next) {
      if ((uint32_t)e->key == key) {
        _upb_value_setval(v, e->val.val, t->t.ctype);
        return true;
      }
      if (e->next == NULL) return false;
    }
  }
}

/* Exposed for testing only. */
bool upb_strtable_resize(upb_strtable *t, size_t size_lg2, upb_alloc *a);

/* Iterators ******************************************************************/

/* Iterators for int and string tables.  We are subject to some kind of unusual
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
  const upb_strtable *t;
  size_t index;
} upb_strtable_iter;

void upb_strtable_begin(upb_strtable_iter *i, const upb_strtable *t);
void upb_strtable_next(upb_strtable_iter *i);
bool upb_strtable_done(const upb_strtable_iter *i);
const char *upb_strtable_iter_key(const upb_strtable_iter *i);
size_t upb_strtable_iter_keylength(const upb_strtable_iter *i);
upb_value upb_strtable_iter_value(const upb_strtable_iter *i);
void upb_strtable_iter_setdone(upb_strtable_iter *i);
bool upb_strtable_iter_isequal(const upb_strtable_iter *i1,
                               const upb_strtable_iter *i2);


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
  const upb_inttable *t;
  size_t index;
  bool array_part;
} upb_inttable_iter;

void upb_inttable_begin(upb_inttable_iter *i, const upb_inttable *t);
void upb_inttable_next(upb_inttable_iter *i);
bool upb_inttable_done(const upb_inttable_iter *i);
uintptr_t upb_inttable_iter_key(const upb_inttable_iter *i);
upb_value upb_inttable_iter_value(const upb_inttable_iter *i);
void upb_inttable_iter_setdone(upb_inttable_iter *i);
bool upb_inttable_iter_isequal(const upb_inttable_iter *i1,
                               const upb_inttable_iter *i2);


#ifdef __cplusplus
}  /* extern "C" */
#endif


#endif  /* UPB_TABLE_H_ */
/* This file was generated by upbc (the upb compiler) from the input
 * file:
 *
 *     google/protobuf/descriptor.proto
 *
 * Do not edit -- your changes will be discarded when the file is
 * regenerated. */

#ifndef GOOGLE_PROTOBUF_DESCRIPTOR_PROTO_UPB_H_
#define GOOGLE_PROTOBUF_DESCRIPTOR_PROTO_UPB_H_

/*
** Functions for use by generated code.  These are not public and users must
** not call them directly.
*/

#ifndef UPB_GENERATED_UTIL_H_
#define UPB_GENERATED_UTIL_H_

#include <stdint.h>


#define PTR_AT(msg, ofs, type) (type*)((const char*)msg + ofs)

UPB_INLINE const void *_upb_array_accessor(const void *msg, size_t ofs,
                                           size_t *size) {
  const upb_array *arr = *PTR_AT(msg, ofs, const upb_array*);
  if (arr) {
    if (size) *size = arr->len;
    return arr->data;
  } else {
    if (size) *size = 0;
    return NULL;
  }
}

UPB_INLINE void *_upb_array_mutable_accessor(void *msg, size_t ofs,
                                             size_t *size) {
  upb_array *arr = *PTR_AT(msg, ofs, upb_array*);
  if (arr) {
    if (size) *size = arr->len;
    return arr->data;
  } else {
    if (size) *size = 0;
    return NULL;
  }
}

/* TODO(haberman): this is a mess.  It will improve when upb_array no longer
 * carries reflective state (type, elem_size). */
UPB_INLINE void *_upb_array_resize_accessor(void *msg, size_t ofs, size_t size,
                                            size_t elem_size,
                                            upb_fieldtype_t type,
                                            upb_arena *arena) {
  upb_array *arr = *PTR_AT(msg, ofs, upb_array*);

  if (!arr) {
    arr = upb_array_new(arena);
    if (!arr) return NULL;
    *PTR_AT(msg, ofs, upb_array*) = arr;
  }

  if (size > arr->size) {
    size_t new_size = UPB_MAX(arr->size, 4);
    size_t old_bytes = arr->size * elem_size;
    size_t new_bytes;
    while (new_size < size) new_size *= 2;
    new_bytes = new_size * elem_size;
    arr->data = upb_arena_realloc(arena, arr->data, old_bytes, new_bytes);
    if (!arr->data) {
      return NULL;
    }
    arr->size = new_size;
  }

  arr->len = size;
  return arr->data;
}

UPB_INLINE bool _upb_array_append_accessor(void *msg, size_t ofs,
                                           size_t elem_size,
                                           upb_fieldtype_t type,
                                           const void *value,
                                           upb_arena *arena) {
  upb_array *arr = *PTR_AT(msg, ofs, upb_array*);
  size_t i = arr ? arr->len : 0;
  void *data =
      _upb_array_resize_accessor(msg, ofs, i + 1, elem_size, type, arena);
  if (!data) return false;
  memcpy(PTR_AT(data, i * elem_size, char), value, elem_size);
  return true;
}

UPB_INLINE bool _upb_has_field(const void *msg, size_t idx) {
  return (*PTR_AT(msg, idx / 8, const char) & (1 << (idx % 8))) != 0;
}

UPB_INLINE bool _upb_sethas(const void *msg, size_t idx) {
  return (*PTR_AT(msg, idx / 8, char)) |= (char)(1 << (idx % 8));
}

UPB_INLINE bool _upb_clearhas(const void *msg, size_t idx) {
  return (*PTR_AT(msg, idx / 8, char)) &= (char)(~(1 << (idx % 8)));
}

UPB_INLINE bool _upb_has_oneof_field(const void *msg, size_t case_ofs, int32_t num) {
  return *PTR_AT(msg, case_ofs, int32_t) == num;
}

#undef PTR_AT


#endif  /* UPB_GENERATED_UTIL_H_ */


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
extern const upb_msglayout google_protobuf_FileDescriptorSet_msginit;
extern const upb_msglayout google_protobuf_FileDescriptorProto_msginit;
extern const upb_msglayout google_protobuf_DescriptorProto_msginit;
extern const upb_msglayout google_protobuf_DescriptorProto_ExtensionRange_msginit;
extern const upb_msglayout google_protobuf_DescriptorProto_ReservedRange_msginit;
extern const upb_msglayout google_protobuf_ExtensionRangeOptions_msginit;
extern const upb_msglayout google_protobuf_FieldDescriptorProto_msginit;
extern const upb_msglayout google_protobuf_OneofDescriptorProto_msginit;
extern const upb_msglayout google_protobuf_EnumDescriptorProto_msginit;
extern const upb_msglayout google_protobuf_EnumDescriptorProto_EnumReservedRange_msginit;
extern const upb_msglayout google_protobuf_EnumValueDescriptorProto_msginit;
extern const upb_msglayout google_protobuf_ServiceDescriptorProto_msginit;
extern const upb_msglayout google_protobuf_MethodDescriptorProto_msginit;
extern const upb_msglayout google_protobuf_FileOptions_msginit;
extern const upb_msglayout google_protobuf_MessageOptions_msginit;
extern const upb_msglayout google_protobuf_FieldOptions_msginit;
extern const upb_msglayout google_protobuf_OneofOptions_msginit;
extern const upb_msglayout google_protobuf_EnumOptions_msginit;
extern const upb_msglayout google_protobuf_EnumValueOptions_msginit;
extern const upb_msglayout google_protobuf_ServiceOptions_msginit;
extern const upb_msglayout google_protobuf_MethodOptions_msginit;
extern const upb_msglayout google_protobuf_UninterpretedOption_msginit;
extern const upb_msglayout google_protobuf_UninterpretedOption_NamePart_msginit;
extern const upb_msglayout google_protobuf_SourceCodeInfo_msginit;
extern const upb_msglayout google_protobuf_SourceCodeInfo_Location_msginit;
extern const upb_msglayout google_protobuf_GeneratedCodeInfo_msginit;
extern const upb_msglayout google_protobuf_GeneratedCodeInfo_Annotation_msginit;

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


/* google.protobuf.FileDescriptorSet */

UPB_INLINE google_protobuf_FileDescriptorSet *google_protobuf_FileDescriptorSet_new(upb_arena *arena) {
  return (google_protobuf_FileDescriptorSet *)upb_msg_new(&google_protobuf_FileDescriptorSet_msginit, arena);
}
UPB_INLINE google_protobuf_FileDescriptorSet *google_protobuf_FileDescriptorSet_parse(const char *buf, size_t size,
                        upb_arena *arena) {
  google_protobuf_FileDescriptorSet *ret = google_protobuf_FileDescriptorSet_new(arena);
  return (ret && upb_decode(buf, size, ret, &google_protobuf_FileDescriptorSet_msginit, arena)) ? ret : NULL;
}
UPB_INLINE char *google_protobuf_FileDescriptorSet_serialize(const google_protobuf_FileDescriptorSet *msg, upb_arena *arena, size_t *len) {
  return upb_encode(msg, &google_protobuf_FileDescriptorSet_msginit, arena, len);
}

UPB_INLINE const google_protobuf_FileDescriptorProto* const* google_protobuf_FileDescriptorSet_file(const google_protobuf_FileDescriptorSet *msg, size_t *len) { return (const google_protobuf_FileDescriptorProto* const*)_upb_array_accessor(msg, UPB_SIZE(0, 0), len); }

UPB_INLINE google_protobuf_FileDescriptorProto** google_protobuf_FileDescriptorSet_mutable_file(google_protobuf_FileDescriptorSet *msg, size_t *len) {
  return (google_protobuf_FileDescriptorProto**)_upb_array_mutable_accessor(msg, UPB_SIZE(0, 0), len);
}
UPB_INLINE google_protobuf_FileDescriptorProto** google_protobuf_FileDescriptorSet_resize_file(google_protobuf_FileDescriptorSet *msg, size_t len, upb_arena *arena) {
  return (google_protobuf_FileDescriptorProto**)_upb_array_resize_accessor(msg, UPB_SIZE(0, 0), len, UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, arena);
}
UPB_INLINE struct google_protobuf_FileDescriptorProto* google_protobuf_FileDescriptorSet_add_file(google_protobuf_FileDescriptorSet *msg, upb_arena *arena) {
  struct google_protobuf_FileDescriptorProto* sub = (struct google_protobuf_FileDescriptorProto*)upb_msg_new(&google_protobuf_FileDescriptorProto_msginit, arena);
  bool ok = _upb_array_append_accessor(
      msg, UPB_SIZE(0, 0), UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, &sub, arena);
  if (!ok) return NULL;
  return sub;
}

/* google.protobuf.FileDescriptorProto */

UPB_INLINE google_protobuf_FileDescriptorProto *google_protobuf_FileDescriptorProto_new(upb_arena *arena) {
  return (google_protobuf_FileDescriptorProto *)upb_msg_new(&google_protobuf_FileDescriptorProto_msginit, arena);
}
UPB_INLINE google_protobuf_FileDescriptorProto *google_protobuf_FileDescriptorProto_parse(const char *buf, size_t size,
                        upb_arena *arena) {
  google_protobuf_FileDescriptorProto *ret = google_protobuf_FileDescriptorProto_new(arena);
  return (ret && upb_decode(buf, size, ret, &google_protobuf_FileDescriptorProto_msginit, arena)) ? ret : NULL;
}
UPB_INLINE char *google_protobuf_FileDescriptorProto_serialize(const google_protobuf_FileDescriptorProto *msg, upb_arena *arena, size_t *len) {
  return upb_encode(msg, &google_protobuf_FileDescriptorProto_msginit, arena, len);
}

UPB_INLINE bool google_protobuf_FileDescriptorProto_has_name(const google_protobuf_FileDescriptorProto *msg) { return _upb_has_field(msg, 1); }
UPB_INLINE upb_strview google_protobuf_FileDescriptorProto_name(const google_protobuf_FileDescriptorProto *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(4, 8)); }
UPB_INLINE bool google_protobuf_FileDescriptorProto_has_package(const google_protobuf_FileDescriptorProto *msg) { return _upb_has_field(msg, 2); }
UPB_INLINE upb_strview google_protobuf_FileDescriptorProto_package(const google_protobuf_FileDescriptorProto *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(12, 24)); }
UPB_INLINE upb_strview const* google_protobuf_FileDescriptorProto_dependency(const google_protobuf_FileDescriptorProto *msg, size_t *len) { return (upb_strview const*)_upb_array_accessor(msg, UPB_SIZE(36, 72), len); }
UPB_INLINE const google_protobuf_DescriptorProto* const* google_protobuf_FileDescriptorProto_message_type(const google_protobuf_FileDescriptorProto *msg, size_t *len) { return (const google_protobuf_DescriptorProto* const*)_upb_array_accessor(msg, UPB_SIZE(40, 80), len); }
UPB_INLINE const google_protobuf_EnumDescriptorProto* const* google_protobuf_FileDescriptorProto_enum_type(const google_protobuf_FileDescriptorProto *msg, size_t *len) { return (const google_protobuf_EnumDescriptorProto* const*)_upb_array_accessor(msg, UPB_SIZE(44, 88), len); }
UPB_INLINE const google_protobuf_ServiceDescriptorProto* const* google_protobuf_FileDescriptorProto_service(const google_protobuf_FileDescriptorProto *msg, size_t *len) { return (const google_protobuf_ServiceDescriptorProto* const*)_upb_array_accessor(msg, UPB_SIZE(48, 96), len); }
UPB_INLINE const google_protobuf_FieldDescriptorProto* const* google_protobuf_FileDescriptorProto_extension(const google_protobuf_FileDescriptorProto *msg, size_t *len) { return (const google_protobuf_FieldDescriptorProto* const*)_upb_array_accessor(msg, UPB_SIZE(52, 104), len); }
UPB_INLINE bool google_protobuf_FileDescriptorProto_has_options(const google_protobuf_FileDescriptorProto *msg) { return _upb_has_field(msg, 4); }
UPB_INLINE const google_protobuf_FileOptions* google_protobuf_FileDescriptorProto_options(const google_protobuf_FileDescriptorProto *msg) { return UPB_FIELD_AT(msg, const google_protobuf_FileOptions*, UPB_SIZE(28, 56)); }
UPB_INLINE bool google_protobuf_FileDescriptorProto_has_source_code_info(const google_protobuf_FileDescriptorProto *msg) { return _upb_has_field(msg, 5); }
UPB_INLINE const google_protobuf_SourceCodeInfo* google_protobuf_FileDescriptorProto_source_code_info(const google_protobuf_FileDescriptorProto *msg) { return UPB_FIELD_AT(msg, const google_protobuf_SourceCodeInfo*, UPB_SIZE(32, 64)); }
UPB_INLINE int32_t const* google_protobuf_FileDescriptorProto_public_dependency(const google_protobuf_FileDescriptorProto *msg, size_t *len) { return (int32_t const*)_upb_array_accessor(msg, UPB_SIZE(56, 112), len); }
UPB_INLINE int32_t const* google_protobuf_FileDescriptorProto_weak_dependency(const google_protobuf_FileDescriptorProto *msg, size_t *len) { return (int32_t const*)_upb_array_accessor(msg, UPB_SIZE(60, 120), len); }
UPB_INLINE bool google_protobuf_FileDescriptorProto_has_syntax(const google_protobuf_FileDescriptorProto *msg) { return _upb_has_field(msg, 3); }
UPB_INLINE upb_strview google_protobuf_FileDescriptorProto_syntax(const google_protobuf_FileDescriptorProto *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(20, 40)); }

UPB_INLINE void google_protobuf_FileDescriptorProto_set_name(google_protobuf_FileDescriptorProto *msg, upb_strview value) {
  _upb_sethas(msg, 1);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(4, 8)) = value;
}
UPB_INLINE void google_protobuf_FileDescriptorProto_set_package(google_protobuf_FileDescriptorProto *msg, upb_strview value) {
  _upb_sethas(msg, 2);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(12, 24)) = value;
}
UPB_INLINE upb_strview* google_protobuf_FileDescriptorProto_mutable_dependency(google_protobuf_FileDescriptorProto *msg, size_t *len) {
  return (upb_strview*)_upb_array_mutable_accessor(msg, UPB_SIZE(36, 72), len);
}
UPB_INLINE upb_strview* google_protobuf_FileDescriptorProto_resize_dependency(google_protobuf_FileDescriptorProto *msg, size_t len, upb_arena *arena) {
  return (upb_strview*)_upb_array_resize_accessor(msg, UPB_SIZE(36, 72), len, UPB_SIZE(8, 16), UPB_TYPE_STRING, arena);
}
UPB_INLINE bool google_protobuf_FileDescriptorProto_add_dependency(google_protobuf_FileDescriptorProto *msg, upb_strview val, upb_arena *arena) {
  return _upb_array_append_accessor(
      msg, UPB_SIZE(36, 72), UPB_SIZE(8, 16), UPB_TYPE_STRING, &val, arena);
}
UPB_INLINE google_protobuf_DescriptorProto** google_protobuf_FileDescriptorProto_mutable_message_type(google_protobuf_FileDescriptorProto *msg, size_t *len) {
  return (google_protobuf_DescriptorProto**)_upb_array_mutable_accessor(msg, UPB_SIZE(40, 80), len);
}
UPB_INLINE google_protobuf_DescriptorProto** google_protobuf_FileDescriptorProto_resize_message_type(google_protobuf_FileDescriptorProto *msg, size_t len, upb_arena *arena) {
  return (google_protobuf_DescriptorProto**)_upb_array_resize_accessor(msg, UPB_SIZE(40, 80), len, UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, arena);
}
UPB_INLINE struct google_protobuf_DescriptorProto* google_protobuf_FileDescriptorProto_add_message_type(google_protobuf_FileDescriptorProto *msg, upb_arena *arena) {
  struct google_protobuf_DescriptorProto* sub = (struct google_protobuf_DescriptorProto*)upb_msg_new(&google_protobuf_DescriptorProto_msginit, arena);
  bool ok = _upb_array_append_accessor(
      msg, UPB_SIZE(40, 80), UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, &sub, arena);
  if (!ok) return NULL;
  return sub;
}
UPB_INLINE google_protobuf_EnumDescriptorProto** google_protobuf_FileDescriptorProto_mutable_enum_type(google_protobuf_FileDescriptorProto *msg, size_t *len) {
  return (google_protobuf_EnumDescriptorProto**)_upb_array_mutable_accessor(msg, UPB_SIZE(44, 88), len);
}
UPB_INLINE google_protobuf_EnumDescriptorProto** google_protobuf_FileDescriptorProto_resize_enum_type(google_protobuf_FileDescriptorProto *msg, size_t len, upb_arena *arena) {
  return (google_protobuf_EnumDescriptorProto**)_upb_array_resize_accessor(msg, UPB_SIZE(44, 88), len, UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, arena);
}
UPB_INLINE struct google_protobuf_EnumDescriptorProto* google_protobuf_FileDescriptorProto_add_enum_type(google_protobuf_FileDescriptorProto *msg, upb_arena *arena) {
  struct google_protobuf_EnumDescriptorProto* sub = (struct google_protobuf_EnumDescriptorProto*)upb_msg_new(&google_protobuf_EnumDescriptorProto_msginit, arena);
  bool ok = _upb_array_append_accessor(
      msg, UPB_SIZE(44, 88), UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, &sub, arena);
  if (!ok) return NULL;
  return sub;
}
UPB_INLINE google_protobuf_ServiceDescriptorProto** google_protobuf_FileDescriptorProto_mutable_service(google_protobuf_FileDescriptorProto *msg, size_t *len) {
  return (google_protobuf_ServiceDescriptorProto**)_upb_array_mutable_accessor(msg, UPB_SIZE(48, 96), len);
}
UPB_INLINE google_protobuf_ServiceDescriptorProto** google_protobuf_FileDescriptorProto_resize_service(google_protobuf_FileDescriptorProto *msg, size_t len, upb_arena *arena) {
  return (google_protobuf_ServiceDescriptorProto**)_upb_array_resize_accessor(msg, UPB_SIZE(48, 96), len, UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, arena);
}
UPB_INLINE struct google_protobuf_ServiceDescriptorProto* google_protobuf_FileDescriptorProto_add_service(google_protobuf_FileDescriptorProto *msg, upb_arena *arena) {
  struct google_protobuf_ServiceDescriptorProto* sub = (struct google_protobuf_ServiceDescriptorProto*)upb_msg_new(&google_protobuf_ServiceDescriptorProto_msginit, arena);
  bool ok = _upb_array_append_accessor(
      msg, UPB_SIZE(48, 96), UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, &sub, arena);
  if (!ok) return NULL;
  return sub;
}
UPB_INLINE google_protobuf_FieldDescriptorProto** google_protobuf_FileDescriptorProto_mutable_extension(google_protobuf_FileDescriptorProto *msg, size_t *len) {
  return (google_protobuf_FieldDescriptorProto**)_upb_array_mutable_accessor(msg, UPB_SIZE(52, 104), len);
}
UPB_INLINE google_protobuf_FieldDescriptorProto** google_protobuf_FileDescriptorProto_resize_extension(google_protobuf_FileDescriptorProto *msg, size_t len, upb_arena *arena) {
  return (google_protobuf_FieldDescriptorProto**)_upb_array_resize_accessor(msg, UPB_SIZE(52, 104), len, UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, arena);
}
UPB_INLINE struct google_protobuf_FieldDescriptorProto* google_protobuf_FileDescriptorProto_add_extension(google_protobuf_FileDescriptorProto *msg, upb_arena *arena) {
  struct google_protobuf_FieldDescriptorProto* sub = (struct google_protobuf_FieldDescriptorProto*)upb_msg_new(&google_protobuf_FieldDescriptorProto_msginit, arena);
  bool ok = _upb_array_append_accessor(
      msg, UPB_SIZE(52, 104), UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, &sub, arena);
  if (!ok) return NULL;
  return sub;
}
UPB_INLINE void google_protobuf_FileDescriptorProto_set_options(google_protobuf_FileDescriptorProto *msg, google_protobuf_FileOptions* value) {
  _upb_sethas(msg, 4);
  UPB_FIELD_AT(msg, google_protobuf_FileOptions*, UPB_SIZE(28, 56)) = value;
}
UPB_INLINE struct google_protobuf_FileOptions* google_protobuf_FileDescriptorProto_mutable_options(google_protobuf_FileDescriptorProto *msg, upb_arena *arena) {
  struct google_protobuf_FileOptions* sub = (struct google_protobuf_FileOptions*)google_protobuf_FileDescriptorProto_options(msg);
  if (sub == NULL) {
    sub = (struct google_protobuf_FileOptions*)upb_msg_new(&google_protobuf_FileOptions_msginit, arena);
    if (!sub) return NULL;
    google_protobuf_FileDescriptorProto_set_options(msg, sub);
  }
  return sub;
}
UPB_INLINE void google_protobuf_FileDescriptorProto_set_source_code_info(google_protobuf_FileDescriptorProto *msg, google_protobuf_SourceCodeInfo* value) {
  _upb_sethas(msg, 5);
  UPB_FIELD_AT(msg, google_protobuf_SourceCodeInfo*, UPB_SIZE(32, 64)) = value;
}
UPB_INLINE struct google_protobuf_SourceCodeInfo* google_protobuf_FileDescriptorProto_mutable_source_code_info(google_protobuf_FileDescriptorProto *msg, upb_arena *arena) {
  struct google_protobuf_SourceCodeInfo* sub = (struct google_protobuf_SourceCodeInfo*)google_protobuf_FileDescriptorProto_source_code_info(msg);
  if (sub == NULL) {
    sub = (struct google_protobuf_SourceCodeInfo*)upb_msg_new(&google_protobuf_SourceCodeInfo_msginit, arena);
    if (!sub) return NULL;
    google_protobuf_FileDescriptorProto_set_source_code_info(msg, sub);
  }
  return sub;
}
UPB_INLINE int32_t* google_protobuf_FileDescriptorProto_mutable_public_dependency(google_protobuf_FileDescriptorProto *msg, size_t *len) {
  return (int32_t*)_upb_array_mutable_accessor(msg, UPB_SIZE(56, 112), len);
}
UPB_INLINE int32_t* google_protobuf_FileDescriptorProto_resize_public_dependency(google_protobuf_FileDescriptorProto *msg, size_t len, upb_arena *arena) {
  return (int32_t*)_upb_array_resize_accessor(msg, UPB_SIZE(56, 112), len, UPB_SIZE(4, 4), UPB_TYPE_INT32, arena);
}
UPB_INLINE bool google_protobuf_FileDescriptorProto_add_public_dependency(google_protobuf_FileDescriptorProto *msg, int32_t val, upb_arena *arena) {
  return _upb_array_append_accessor(
      msg, UPB_SIZE(56, 112), UPB_SIZE(4, 4), UPB_TYPE_INT32, &val, arena);
}
UPB_INLINE int32_t* google_protobuf_FileDescriptorProto_mutable_weak_dependency(google_protobuf_FileDescriptorProto *msg, size_t *len) {
  return (int32_t*)_upb_array_mutable_accessor(msg, UPB_SIZE(60, 120), len);
}
UPB_INLINE int32_t* google_protobuf_FileDescriptorProto_resize_weak_dependency(google_protobuf_FileDescriptorProto *msg, size_t len, upb_arena *arena) {
  return (int32_t*)_upb_array_resize_accessor(msg, UPB_SIZE(60, 120), len, UPB_SIZE(4, 4), UPB_TYPE_INT32, arena);
}
UPB_INLINE bool google_protobuf_FileDescriptorProto_add_weak_dependency(google_protobuf_FileDescriptorProto *msg, int32_t val, upb_arena *arena) {
  return _upb_array_append_accessor(
      msg, UPB_SIZE(60, 120), UPB_SIZE(4, 4), UPB_TYPE_INT32, &val, arena);
}
UPB_INLINE void google_protobuf_FileDescriptorProto_set_syntax(google_protobuf_FileDescriptorProto *msg, upb_strview value) {
  _upb_sethas(msg, 3);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(20, 40)) = value;
}

/* google.protobuf.DescriptorProto */

UPB_INLINE google_protobuf_DescriptorProto *google_protobuf_DescriptorProto_new(upb_arena *arena) {
  return (google_protobuf_DescriptorProto *)upb_msg_new(&google_protobuf_DescriptorProto_msginit, arena);
}
UPB_INLINE google_protobuf_DescriptorProto *google_protobuf_DescriptorProto_parse(const char *buf, size_t size,
                        upb_arena *arena) {
  google_protobuf_DescriptorProto *ret = google_protobuf_DescriptorProto_new(arena);
  return (ret && upb_decode(buf, size, ret, &google_protobuf_DescriptorProto_msginit, arena)) ? ret : NULL;
}
UPB_INLINE char *google_protobuf_DescriptorProto_serialize(const google_protobuf_DescriptorProto *msg, upb_arena *arena, size_t *len) {
  return upb_encode(msg, &google_protobuf_DescriptorProto_msginit, arena, len);
}

UPB_INLINE bool google_protobuf_DescriptorProto_has_name(const google_protobuf_DescriptorProto *msg) { return _upb_has_field(msg, 1); }
UPB_INLINE upb_strview google_protobuf_DescriptorProto_name(const google_protobuf_DescriptorProto *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(4, 8)); }
UPB_INLINE const google_protobuf_FieldDescriptorProto* const* google_protobuf_DescriptorProto_field(const google_protobuf_DescriptorProto *msg, size_t *len) { return (const google_protobuf_FieldDescriptorProto* const*)_upb_array_accessor(msg, UPB_SIZE(16, 32), len); }
UPB_INLINE const google_protobuf_DescriptorProto* const* google_protobuf_DescriptorProto_nested_type(const google_protobuf_DescriptorProto *msg, size_t *len) { return (const google_protobuf_DescriptorProto* const*)_upb_array_accessor(msg, UPB_SIZE(20, 40), len); }
UPB_INLINE const google_protobuf_EnumDescriptorProto* const* google_protobuf_DescriptorProto_enum_type(const google_protobuf_DescriptorProto *msg, size_t *len) { return (const google_protobuf_EnumDescriptorProto* const*)_upb_array_accessor(msg, UPB_SIZE(24, 48), len); }
UPB_INLINE const google_protobuf_DescriptorProto_ExtensionRange* const* google_protobuf_DescriptorProto_extension_range(const google_protobuf_DescriptorProto *msg, size_t *len) { return (const google_protobuf_DescriptorProto_ExtensionRange* const*)_upb_array_accessor(msg, UPB_SIZE(28, 56), len); }
UPB_INLINE const google_protobuf_FieldDescriptorProto* const* google_protobuf_DescriptorProto_extension(const google_protobuf_DescriptorProto *msg, size_t *len) { return (const google_protobuf_FieldDescriptorProto* const*)_upb_array_accessor(msg, UPB_SIZE(32, 64), len); }
UPB_INLINE bool google_protobuf_DescriptorProto_has_options(const google_protobuf_DescriptorProto *msg) { return _upb_has_field(msg, 2); }
UPB_INLINE const google_protobuf_MessageOptions* google_protobuf_DescriptorProto_options(const google_protobuf_DescriptorProto *msg) { return UPB_FIELD_AT(msg, const google_protobuf_MessageOptions*, UPB_SIZE(12, 24)); }
UPB_INLINE const google_protobuf_OneofDescriptorProto* const* google_protobuf_DescriptorProto_oneof_decl(const google_protobuf_DescriptorProto *msg, size_t *len) { return (const google_protobuf_OneofDescriptorProto* const*)_upb_array_accessor(msg, UPB_SIZE(36, 72), len); }
UPB_INLINE const google_protobuf_DescriptorProto_ReservedRange* const* google_protobuf_DescriptorProto_reserved_range(const google_protobuf_DescriptorProto *msg, size_t *len) { return (const google_protobuf_DescriptorProto_ReservedRange* const*)_upb_array_accessor(msg, UPB_SIZE(40, 80), len); }
UPB_INLINE upb_strview const* google_protobuf_DescriptorProto_reserved_name(const google_protobuf_DescriptorProto *msg, size_t *len) { return (upb_strview const*)_upb_array_accessor(msg, UPB_SIZE(44, 88), len); }

UPB_INLINE void google_protobuf_DescriptorProto_set_name(google_protobuf_DescriptorProto *msg, upb_strview value) {
  _upb_sethas(msg, 1);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(4, 8)) = value;
}
UPB_INLINE google_protobuf_FieldDescriptorProto** google_protobuf_DescriptorProto_mutable_field(google_protobuf_DescriptorProto *msg, size_t *len) {
  return (google_protobuf_FieldDescriptorProto**)_upb_array_mutable_accessor(msg, UPB_SIZE(16, 32), len);
}
UPB_INLINE google_protobuf_FieldDescriptorProto** google_protobuf_DescriptorProto_resize_field(google_protobuf_DescriptorProto *msg, size_t len, upb_arena *arena) {
  return (google_protobuf_FieldDescriptorProto**)_upb_array_resize_accessor(msg, UPB_SIZE(16, 32), len, UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, arena);
}
UPB_INLINE struct google_protobuf_FieldDescriptorProto* google_protobuf_DescriptorProto_add_field(google_protobuf_DescriptorProto *msg, upb_arena *arena) {
  struct google_protobuf_FieldDescriptorProto* sub = (struct google_protobuf_FieldDescriptorProto*)upb_msg_new(&google_protobuf_FieldDescriptorProto_msginit, arena);
  bool ok = _upb_array_append_accessor(
      msg, UPB_SIZE(16, 32), UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, &sub, arena);
  if (!ok) return NULL;
  return sub;
}
UPB_INLINE google_protobuf_DescriptorProto** google_protobuf_DescriptorProto_mutable_nested_type(google_protobuf_DescriptorProto *msg, size_t *len) {
  return (google_protobuf_DescriptorProto**)_upb_array_mutable_accessor(msg, UPB_SIZE(20, 40), len);
}
UPB_INLINE google_protobuf_DescriptorProto** google_protobuf_DescriptorProto_resize_nested_type(google_protobuf_DescriptorProto *msg, size_t len, upb_arena *arena) {
  return (google_protobuf_DescriptorProto**)_upb_array_resize_accessor(msg, UPB_SIZE(20, 40), len, UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, arena);
}
UPB_INLINE struct google_protobuf_DescriptorProto* google_protobuf_DescriptorProto_add_nested_type(google_protobuf_DescriptorProto *msg, upb_arena *arena) {
  struct google_protobuf_DescriptorProto* sub = (struct google_protobuf_DescriptorProto*)upb_msg_new(&google_protobuf_DescriptorProto_msginit, arena);
  bool ok = _upb_array_append_accessor(
      msg, UPB_SIZE(20, 40), UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, &sub, arena);
  if (!ok) return NULL;
  return sub;
}
UPB_INLINE google_protobuf_EnumDescriptorProto** google_protobuf_DescriptorProto_mutable_enum_type(google_protobuf_DescriptorProto *msg, size_t *len) {
  return (google_protobuf_EnumDescriptorProto**)_upb_array_mutable_accessor(msg, UPB_SIZE(24, 48), len);
}
UPB_INLINE google_protobuf_EnumDescriptorProto** google_protobuf_DescriptorProto_resize_enum_type(google_protobuf_DescriptorProto *msg, size_t len, upb_arena *arena) {
  return (google_protobuf_EnumDescriptorProto**)_upb_array_resize_accessor(msg, UPB_SIZE(24, 48), len, UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, arena);
}
UPB_INLINE struct google_protobuf_EnumDescriptorProto* google_protobuf_DescriptorProto_add_enum_type(google_protobuf_DescriptorProto *msg, upb_arena *arena) {
  struct google_protobuf_EnumDescriptorProto* sub = (struct google_protobuf_EnumDescriptorProto*)upb_msg_new(&google_protobuf_EnumDescriptorProto_msginit, arena);
  bool ok = _upb_array_append_accessor(
      msg, UPB_SIZE(24, 48), UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, &sub, arena);
  if (!ok) return NULL;
  return sub;
}
UPB_INLINE google_protobuf_DescriptorProto_ExtensionRange** google_protobuf_DescriptorProto_mutable_extension_range(google_protobuf_DescriptorProto *msg, size_t *len) {
  return (google_protobuf_DescriptorProto_ExtensionRange**)_upb_array_mutable_accessor(msg, UPB_SIZE(28, 56), len);
}
UPB_INLINE google_protobuf_DescriptorProto_ExtensionRange** google_protobuf_DescriptorProto_resize_extension_range(google_protobuf_DescriptorProto *msg, size_t len, upb_arena *arena) {
  return (google_protobuf_DescriptorProto_ExtensionRange**)_upb_array_resize_accessor(msg, UPB_SIZE(28, 56), len, UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, arena);
}
UPB_INLINE struct google_protobuf_DescriptorProto_ExtensionRange* google_protobuf_DescriptorProto_add_extension_range(google_protobuf_DescriptorProto *msg, upb_arena *arena) {
  struct google_protobuf_DescriptorProto_ExtensionRange* sub = (struct google_protobuf_DescriptorProto_ExtensionRange*)upb_msg_new(&google_protobuf_DescriptorProto_ExtensionRange_msginit, arena);
  bool ok = _upb_array_append_accessor(
      msg, UPB_SIZE(28, 56), UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, &sub, arena);
  if (!ok) return NULL;
  return sub;
}
UPB_INLINE google_protobuf_FieldDescriptorProto** google_protobuf_DescriptorProto_mutable_extension(google_protobuf_DescriptorProto *msg, size_t *len) {
  return (google_protobuf_FieldDescriptorProto**)_upb_array_mutable_accessor(msg, UPB_SIZE(32, 64), len);
}
UPB_INLINE google_protobuf_FieldDescriptorProto** google_protobuf_DescriptorProto_resize_extension(google_protobuf_DescriptorProto *msg, size_t len, upb_arena *arena) {
  return (google_protobuf_FieldDescriptorProto**)_upb_array_resize_accessor(msg, UPB_SIZE(32, 64), len, UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, arena);
}
UPB_INLINE struct google_protobuf_FieldDescriptorProto* google_protobuf_DescriptorProto_add_extension(google_protobuf_DescriptorProto *msg, upb_arena *arena) {
  struct google_protobuf_FieldDescriptorProto* sub = (struct google_protobuf_FieldDescriptorProto*)upb_msg_new(&google_protobuf_FieldDescriptorProto_msginit, arena);
  bool ok = _upb_array_append_accessor(
      msg, UPB_SIZE(32, 64), UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, &sub, arena);
  if (!ok) return NULL;
  return sub;
}
UPB_INLINE void google_protobuf_DescriptorProto_set_options(google_protobuf_DescriptorProto *msg, google_protobuf_MessageOptions* value) {
  _upb_sethas(msg, 2);
  UPB_FIELD_AT(msg, google_protobuf_MessageOptions*, UPB_SIZE(12, 24)) = value;
}
UPB_INLINE struct google_protobuf_MessageOptions* google_protobuf_DescriptorProto_mutable_options(google_protobuf_DescriptorProto *msg, upb_arena *arena) {
  struct google_protobuf_MessageOptions* sub = (struct google_protobuf_MessageOptions*)google_protobuf_DescriptorProto_options(msg);
  if (sub == NULL) {
    sub = (struct google_protobuf_MessageOptions*)upb_msg_new(&google_protobuf_MessageOptions_msginit, arena);
    if (!sub) return NULL;
    google_protobuf_DescriptorProto_set_options(msg, sub);
  }
  return sub;
}
UPB_INLINE google_protobuf_OneofDescriptorProto** google_protobuf_DescriptorProto_mutable_oneof_decl(google_protobuf_DescriptorProto *msg, size_t *len) {
  return (google_protobuf_OneofDescriptorProto**)_upb_array_mutable_accessor(msg, UPB_SIZE(36, 72), len);
}
UPB_INLINE google_protobuf_OneofDescriptorProto** google_protobuf_DescriptorProto_resize_oneof_decl(google_protobuf_DescriptorProto *msg, size_t len, upb_arena *arena) {
  return (google_protobuf_OneofDescriptorProto**)_upb_array_resize_accessor(msg, UPB_SIZE(36, 72), len, UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, arena);
}
UPB_INLINE struct google_protobuf_OneofDescriptorProto* google_protobuf_DescriptorProto_add_oneof_decl(google_protobuf_DescriptorProto *msg, upb_arena *arena) {
  struct google_protobuf_OneofDescriptorProto* sub = (struct google_protobuf_OneofDescriptorProto*)upb_msg_new(&google_protobuf_OneofDescriptorProto_msginit, arena);
  bool ok = _upb_array_append_accessor(
      msg, UPB_SIZE(36, 72), UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, &sub, arena);
  if (!ok) return NULL;
  return sub;
}
UPB_INLINE google_protobuf_DescriptorProto_ReservedRange** google_protobuf_DescriptorProto_mutable_reserved_range(google_protobuf_DescriptorProto *msg, size_t *len) {
  return (google_protobuf_DescriptorProto_ReservedRange**)_upb_array_mutable_accessor(msg, UPB_SIZE(40, 80), len);
}
UPB_INLINE google_protobuf_DescriptorProto_ReservedRange** google_protobuf_DescriptorProto_resize_reserved_range(google_protobuf_DescriptorProto *msg, size_t len, upb_arena *arena) {
  return (google_protobuf_DescriptorProto_ReservedRange**)_upb_array_resize_accessor(msg, UPB_SIZE(40, 80), len, UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, arena);
}
UPB_INLINE struct google_protobuf_DescriptorProto_ReservedRange* google_protobuf_DescriptorProto_add_reserved_range(google_protobuf_DescriptorProto *msg, upb_arena *arena) {
  struct google_protobuf_DescriptorProto_ReservedRange* sub = (struct google_protobuf_DescriptorProto_ReservedRange*)upb_msg_new(&google_protobuf_DescriptorProto_ReservedRange_msginit, arena);
  bool ok = _upb_array_append_accessor(
      msg, UPB_SIZE(40, 80), UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, &sub, arena);
  if (!ok) return NULL;
  return sub;
}
UPB_INLINE upb_strview* google_protobuf_DescriptorProto_mutable_reserved_name(google_protobuf_DescriptorProto *msg, size_t *len) {
  return (upb_strview*)_upb_array_mutable_accessor(msg, UPB_SIZE(44, 88), len);
}
UPB_INLINE upb_strview* google_protobuf_DescriptorProto_resize_reserved_name(google_protobuf_DescriptorProto *msg, size_t len, upb_arena *arena) {
  return (upb_strview*)_upb_array_resize_accessor(msg, UPB_SIZE(44, 88), len, UPB_SIZE(8, 16), UPB_TYPE_STRING, arena);
}
UPB_INLINE bool google_protobuf_DescriptorProto_add_reserved_name(google_protobuf_DescriptorProto *msg, upb_strview val, upb_arena *arena) {
  return _upb_array_append_accessor(
      msg, UPB_SIZE(44, 88), UPB_SIZE(8, 16), UPB_TYPE_STRING, &val, arena);
}

/* google.protobuf.DescriptorProto.ExtensionRange */

UPB_INLINE google_protobuf_DescriptorProto_ExtensionRange *google_protobuf_DescriptorProto_ExtensionRange_new(upb_arena *arena) {
  return (google_protobuf_DescriptorProto_ExtensionRange *)upb_msg_new(&google_protobuf_DescriptorProto_ExtensionRange_msginit, arena);
}
UPB_INLINE google_protobuf_DescriptorProto_ExtensionRange *google_protobuf_DescriptorProto_ExtensionRange_parse(const char *buf, size_t size,
                        upb_arena *arena) {
  google_protobuf_DescriptorProto_ExtensionRange *ret = google_protobuf_DescriptorProto_ExtensionRange_new(arena);
  return (ret && upb_decode(buf, size, ret, &google_protobuf_DescriptorProto_ExtensionRange_msginit, arena)) ? ret : NULL;
}
UPB_INLINE char *google_protobuf_DescriptorProto_ExtensionRange_serialize(const google_protobuf_DescriptorProto_ExtensionRange *msg, upb_arena *arena, size_t *len) {
  return upb_encode(msg, &google_protobuf_DescriptorProto_ExtensionRange_msginit, arena, len);
}

UPB_INLINE bool google_protobuf_DescriptorProto_ExtensionRange_has_start(const google_protobuf_DescriptorProto_ExtensionRange *msg) { return _upb_has_field(msg, 1); }
UPB_INLINE int32_t google_protobuf_DescriptorProto_ExtensionRange_start(const google_protobuf_DescriptorProto_ExtensionRange *msg) { return UPB_FIELD_AT(msg, int32_t, UPB_SIZE(4, 4)); }
UPB_INLINE bool google_protobuf_DescriptorProto_ExtensionRange_has_end(const google_protobuf_DescriptorProto_ExtensionRange *msg) { return _upb_has_field(msg, 2); }
UPB_INLINE int32_t google_protobuf_DescriptorProto_ExtensionRange_end(const google_protobuf_DescriptorProto_ExtensionRange *msg) { return UPB_FIELD_AT(msg, int32_t, UPB_SIZE(8, 8)); }
UPB_INLINE bool google_protobuf_DescriptorProto_ExtensionRange_has_options(const google_protobuf_DescriptorProto_ExtensionRange *msg) { return _upb_has_field(msg, 3); }
UPB_INLINE const google_protobuf_ExtensionRangeOptions* google_protobuf_DescriptorProto_ExtensionRange_options(const google_protobuf_DescriptorProto_ExtensionRange *msg) { return UPB_FIELD_AT(msg, const google_protobuf_ExtensionRangeOptions*, UPB_SIZE(12, 16)); }

UPB_INLINE void google_protobuf_DescriptorProto_ExtensionRange_set_start(google_protobuf_DescriptorProto_ExtensionRange *msg, int32_t value) {
  _upb_sethas(msg, 1);
  UPB_FIELD_AT(msg, int32_t, UPB_SIZE(4, 4)) = value;
}
UPB_INLINE void google_protobuf_DescriptorProto_ExtensionRange_set_end(google_protobuf_DescriptorProto_ExtensionRange *msg, int32_t value) {
  _upb_sethas(msg, 2);
  UPB_FIELD_AT(msg, int32_t, UPB_SIZE(8, 8)) = value;
}
UPB_INLINE void google_protobuf_DescriptorProto_ExtensionRange_set_options(google_protobuf_DescriptorProto_ExtensionRange *msg, google_protobuf_ExtensionRangeOptions* value) {
  _upb_sethas(msg, 3);
  UPB_FIELD_AT(msg, google_protobuf_ExtensionRangeOptions*, UPB_SIZE(12, 16)) = value;
}
UPB_INLINE struct google_protobuf_ExtensionRangeOptions* google_protobuf_DescriptorProto_ExtensionRange_mutable_options(google_protobuf_DescriptorProto_ExtensionRange *msg, upb_arena *arena) {
  struct google_protobuf_ExtensionRangeOptions* sub = (struct google_protobuf_ExtensionRangeOptions*)google_protobuf_DescriptorProto_ExtensionRange_options(msg);
  if (sub == NULL) {
    sub = (struct google_protobuf_ExtensionRangeOptions*)upb_msg_new(&google_protobuf_ExtensionRangeOptions_msginit, arena);
    if (!sub) return NULL;
    google_protobuf_DescriptorProto_ExtensionRange_set_options(msg, sub);
  }
  return sub;
}

/* google.protobuf.DescriptorProto.ReservedRange */

UPB_INLINE google_protobuf_DescriptorProto_ReservedRange *google_protobuf_DescriptorProto_ReservedRange_new(upb_arena *arena) {
  return (google_protobuf_DescriptorProto_ReservedRange *)upb_msg_new(&google_protobuf_DescriptorProto_ReservedRange_msginit, arena);
}
UPB_INLINE google_protobuf_DescriptorProto_ReservedRange *google_protobuf_DescriptorProto_ReservedRange_parse(const char *buf, size_t size,
                        upb_arena *arena) {
  google_protobuf_DescriptorProto_ReservedRange *ret = google_protobuf_DescriptorProto_ReservedRange_new(arena);
  return (ret && upb_decode(buf, size, ret, &google_protobuf_DescriptorProto_ReservedRange_msginit, arena)) ? ret : NULL;
}
UPB_INLINE char *google_protobuf_DescriptorProto_ReservedRange_serialize(const google_protobuf_DescriptorProto_ReservedRange *msg, upb_arena *arena, size_t *len) {
  return upb_encode(msg, &google_protobuf_DescriptorProto_ReservedRange_msginit, arena, len);
}

UPB_INLINE bool google_protobuf_DescriptorProto_ReservedRange_has_start(const google_protobuf_DescriptorProto_ReservedRange *msg) { return _upb_has_field(msg, 1); }
UPB_INLINE int32_t google_protobuf_DescriptorProto_ReservedRange_start(const google_protobuf_DescriptorProto_ReservedRange *msg) { return UPB_FIELD_AT(msg, int32_t, UPB_SIZE(4, 4)); }
UPB_INLINE bool google_protobuf_DescriptorProto_ReservedRange_has_end(const google_protobuf_DescriptorProto_ReservedRange *msg) { return _upb_has_field(msg, 2); }
UPB_INLINE int32_t google_protobuf_DescriptorProto_ReservedRange_end(const google_protobuf_DescriptorProto_ReservedRange *msg) { return UPB_FIELD_AT(msg, int32_t, UPB_SIZE(8, 8)); }

UPB_INLINE void google_protobuf_DescriptorProto_ReservedRange_set_start(google_protobuf_DescriptorProto_ReservedRange *msg, int32_t value) {
  _upb_sethas(msg, 1);
  UPB_FIELD_AT(msg, int32_t, UPB_SIZE(4, 4)) = value;
}
UPB_INLINE void google_protobuf_DescriptorProto_ReservedRange_set_end(google_protobuf_DescriptorProto_ReservedRange *msg, int32_t value) {
  _upb_sethas(msg, 2);
  UPB_FIELD_AT(msg, int32_t, UPB_SIZE(8, 8)) = value;
}

/* google.protobuf.ExtensionRangeOptions */

UPB_INLINE google_protobuf_ExtensionRangeOptions *google_protobuf_ExtensionRangeOptions_new(upb_arena *arena) {
  return (google_protobuf_ExtensionRangeOptions *)upb_msg_new(&google_protobuf_ExtensionRangeOptions_msginit, arena);
}
UPB_INLINE google_protobuf_ExtensionRangeOptions *google_protobuf_ExtensionRangeOptions_parse(const char *buf, size_t size,
                        upb_arena *arena) {
  google_protobuf_ExtensionRangeOptions *ret = google_protobuf_ExtensionRangeOptions_new(arena);
  return (ret && upb_decode(buf, size, ret, &google_protobuf_ExtensionRangeOptions_msginit, arena)) ? ret : NULL;
}
UPB_INLINE char *google_protobuf_ExtensionRangeOptions_serialize(const google_protobuf_ExtensionRangeOptions *msg, upb_arena *arena, size_t *len) {
  return upb_encode(msg, &google_protobuf_ExtensionRangeOptions_msginit, arena, len);
}

UPB_INLINE const google_protobuf_UninterpretedOption* const* google_protobuf_ExtensionRangeOptions_uninterpreted_option(const google_protobuf_ExtensionRangeOptions *msg, size_t *len) { return (const google_protobuf_UninterpretedOption* const*)_upb_array_accessor(msg, UPB_SIZE(0, 0), len); }

UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_ExtensionRangeOptions_mutable_uninterpreted_option(google_protobuf_ExtensionRangeOptions *msg, size_t *len) {
  return (google_protobuf_UninterpretedOption**)_upb_array_mutable_accessor(msg, UPB_SIZE(0, 0), len);
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_ExtensionRangeOptions_resize_uninterpreted_option(google_protobuf_ExtensionRangeOptions *msg, size_t len, upb_arena *arena) {
  return (google_protobuf_UninterpretedOption**)_upb_array_resize_accessor(msg, UPB_SIZE(0, 0), len, UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, arena);
}
UPB_INLINE struct google_protobuf_UninterpretedOption* google_protobuf_ExtensionRangeOptions_add_uninterpreted_option(google_protobuf_ExtensionRangeOptions *msg, upb_arena *arena) {
  struct google_protobuf_UninterpretedOption* sub = (struct google_protobuf_UninterpretedOption*)upb_msg_new(&google_protobuf_UninterpretedOption_msginit, arena);
  bool ok = _upb_array_append_accessor(
      msg, UPB_SIZE(0, 0), UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, &sub, arena);
  if (!ok) return NULL;
  return sub;
}

/* google.protobuf.FieldDescriptorProto */

UPB_INLINE google_protobuf_FieldDescriptorProto *google_protobuf_FieldDescriptorProto_new(upb_arena *arena) {
  return (google_protobuf_FieldDescriptorProto *)upb_msg_new(&google_protobuf_FieldDescriptorProto_msginit, arena);
}
UPB_INLINE google_protobuf_FieldDescriptorProto *google_protobuf_FieldDescriptorProto_parse(const char *buf, size_t size,
                        upb_arena *arena) {
  google_protobuf_FieldDescriptorProto *ret = google_protobuf_FieldDescriptorProto_new(arena);
  return (ret && upb_decode(buf, size, ret, &google_protobuf_FieldDescriptorProto_msginit, arena)) ? ret : NULL;
}
UPB_INLINE char *google_protobuf_FieldDescriptorProto_serialize(const google_protobuf_FieldDescriptorProto *msg, upb_arena *arena, size_t *len) {
  return upb_encode(msg, &google_protobuf_FieldDescriptorProto_msginit, arena, len);
}

UPB_INLINE bool google_protobuf_FieldDescriptorProto_has_name(const google_protobuf_FieldDescriptorProto *msg) { return _upb_has_field(msg, 5); }
UPB_INLINE upb_strview google_protobuf_FieldDescriptorProto_name(const google_protobuf_FieldDescriptorProto *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(32, 32)); }
UPB_INLINE bool google_protobuf_FieldDescriptorProto_has_extendee(const google_protobuf_FieldDescriptorProto *msg) { return _upb_has_field(msg, 6); }
UPB_INLINE upb_strview google_protobuf_FieldDescriptorProto_extendee(const google_protobuf_FieldDescriptorProto *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(40, 48)); }
UPB_INLINE bool google_protobuf_FieldDescriptorProto_has_number(const google_protobuf_FieldDescriptorProto *msg) { return _upb_has_field(msg, 3); }
UPB_INLINE int32_t google_protobuf_FieldDescriptorProto_number(const google_protobuf_FieldDescriptorProto *msg) { return UPB_FIELD_AT(msg, int32_t, UPB_SIZE(24, 24)); }
UPB_INLINE bool google_protobuf_FieldDescriptorProto_has_label(const google_protobuf_FieldDescriptorProto *msg) { return _upb_has_field(msg, 1); }
UPB_INLINE int32_t google_protobuf_FieldDescriptorProto_label(const google_protobuf_FieldDescriptorProto *msg) { return UPB_FIELD_AT(msg, int32_t, UPB_SIZE(8, 8)); }
UPB_INLINE bool google_protobuf_FieldDescriptorProto_has_type(const google_protobuf_FieldDescriptorProto *msg) { return _upb_has_field(msg, 2); }
UPB_INLINE int32_t google_protobuf_FieldDescriptorProto_type(const google_protobuf_FieldDescriptorProto *msg) { return UPB_FIELD_AT(msg, int32_t, UPB_SIZE(16, 16)); }
UPB_INLINE bool google_protobuf_FieldDescriptorProto_has_type_name(const google_protobuf_FieldDescriptorProto *msg) { return _upb_has_field(msg, 7); }
UPB_INLINE upb_strview google_protobuf_FieldDescriptorProto_type_name(const google_protobuf_FieldDescriptorProto *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(48, 64)); }
UPB_INLINE bool google_protobuf_FieldDescriptorProto_has_default_value(const google_protobuf_FieldDescriptorProto *msg) { return _upb_has_field(msg, 8); }
UPB_INLINE upb_strview google_protobuf_FieldDescriptorProto_default_value(const google_protobuf_FieldDescriptorProto *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(56, 80)); }
UPB_INLINE bool google_protobuf_FieldDescriptorProto_has_options(const google_protobuf_FieldDescriptorProto *msg) { return _upb_has_field(msg, 10); }
UPB_INLINE const google_protobuf_FieldOptions* google_protobuf_FieldDescriptorProto_options(const google_protobuf_FieldDescriptorProto *msg) { return UPB_FIELD_AT(msg, const google_protobuf_FieldOptions*, UPB_SIZE(72, 112)); }
UPB_INLINE bool google_protobuf_FieldDescriptorProto_has_oneof_index(const google_protobuf_FieldDescriptorProto *msg) { return _upb_has_field(msg, 4); }
UPB_INLINE int32_t google_protobuf_FieldDescriptorProto_oneof_index(const google_protobuf_FieldDescriptorProto *msg) { return UPB_FIELD_AT(msg, int32_t, UPB_SIZE(28, 28)); }
UPB_INLINE bool google_protobuf_FieldDescriptorProto_has_json_name(const google_protobuf_FieldDescriptorProto *msg) { return _upb_has_field(msg, 9); }
UPB_INLINE upb_strview google_protobuf_FieldDescriptorProto_json_name(const google_protobuf_FieldDescriptorProto *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(64, 96)); }

UPB_INLINE void google_protobuf_FieldDescriptorProto_set_name(google_protobuf_FieldDescriptorProto *msg, upb_strview value) {
  _upb_sethas(msg, 5);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(32, 32)) = value;
}
UPB_INLINE void google_protobuf_FieldDescriptorProto_set_extendee(google_protobuf_FieldDescriptorProto *msg, upb_strview value) {
  _upb_sethas(msg, 6);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(40, 48)) = value;
}
UPB_INLINE void google_protobuf_FieldDescriptorProto_set_number(google_protobuf_FieldDescriptorProto *msg, int32_t value) {
  _upb_sethas(msg, 3);
  UPB_FIELD_AT(msg, int32_t, UPB_SIZE(24, 24)) = value;
}
UPB_INLINE void google_protobuf_FieldDescriptorProto_set_label(google_protobuf_FieldDescriptorProto *msg, int32_t value) {
  _upb_sethas(msg, 1);
  UPB_FIELD_AT(msg, int32_t, UPB_SIZE(8, 8)) = value;
}
UPB_INLINE void google_protobuf_FieldDescriptorProto_set_type(google_protobuf_FieldDescriptorProto *msg, int32_t value) {
  _upb_sethas(msg, 2);
  UPB_FIELD_AT(msg, int32_t, UPB_SIZE(16, 16)) = value;
}
UPB_INLINE void google_protobuf_FieldDescriptorProto_set_type_name(google_protobuf_FieldDescriptorProto *msg, upb_strview value) {
  _upb_sethas(msg, 7);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(48, 64)) = value;
}
UPB_INLINE void google_protobuf_FieldDescriptorProto_set_default_value(google_protobuf_FieldDescriptorProto *msg, upb_strview value) {
  _upb_sethas(msg, 8);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(56, 80)) = value;
}
UPB_INLINE void google_protobuf_FieldDescriptorProto_set_options(google_protobuf_FieldDescriptorProto *msg, google_protobuf_FieldOptions* value) {
  _upb_sethas(msg, 10);
  UPB_FIELD_AT(msg, google_protobuf_FieldOptions*, UPB_SIZE(72, 112)) = value;
}
UPB_INLINE struct google_protobuf_FieldOptions* google_protobuf_FieldDescriptorProto_mutable_options(google_protobuf_FieldDescriptorProto *msg, upb_arena *arena) {
  struct google_protobuf_FieldOptions* sub = (struct google_protobuf_FieldOptions*)google_protobuf_FieldDescriptorProto_options(msg);
  if (sub == NULL) {
    sub = (struct google_protobuf_FieldOptions*)upb_msg_new(&google_protobuf_FieldOptions_msginit, arena);
    if (!sub) return NULL;
    google_protobuf_FieldDescriptorProto_set_options(msg, sub);
  }
  return sub;
}
UPB_INLINE void google_protobuf_FieldDescriptorProto_set_oneof_index(google_protobuf_FieldDescriptorProto *msg, int32_t value) {
  _upb_sethas(msg, 4);
  UPB_FIELD_AT(msg, int32_t, UPB_SIZE(28, 28)) = value;
}
UPB_INLINE void google_protobuf_FieldDescriptorProto_set_json_name(google_protobuf_FieldDescriptorProto *msg, upb_strview value) {
  _upb_sethas(msg, 9);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(64, 96)) = value;
}

/* google.protobuf.OneofDescriptorProto */

UPB_INLINE google_protobuf_OneofDescriptorProto *google_protobuf_OneofDescriptorProto_new(upb_arena *arena) {
  return (google_protobuf_OneofDescriptorProto *)upb_msg_new(&google_protobuf_OneofDescriptorProto_msginit, arena);
}
UPB_INLINE google_protobuf_OneofDescriptorProto *google_protobuf_OneofDescriptorProto_parse(const char *buf, size_t size,
                        upb_arena *arena) {
  google_protobuf_OneofDescriptorProto *ret = google_protobuf_OneofDescriptorProto_new(arena);
  return (ret && upb_decode(buf, size, ret, &google_protobuf_OneofDescriptorProto_msginit, arena)) ? ret : NULL;
}
UPB_INLINE char *google_protobuf_OneofDescriptorProto_serialize(const google_protobuf_OneofDescriptorProto *msg, upb_arena *arena, size_t *len) {
  return upb_encode(msg, &google_protobuf_OneofDescriptorProto_msginit, arena, len);
}

UPB_INLINE bool google_protobuf_OneofDescriptorProto_has_name(const google_protobuf_OneofDescriptorProto *msg) { return _upb_has_field(msg, 1); }
UPB_INLINE upb_strview google_protobuf_OneofDescriptorProto_name(const google_protobuf_OneofDescriptorProto *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(4, 8)); }
UPB_INLINE bool google_protobuf_OneofDescriptorProto_has_options(const google_protobuf_OneofDescriptorProto *msg) { return _upb_has_field(msg, 2); }
UPB_INLINE const google_protobuf_OneofOptions* google_protobuf_OneofDescriptorProto_options(const google_protobuf_OneofDescriptorProto *msg) { return UPB_FIELD_AT(msg, const google_protobuf_OneofOptions*, UPB_SIZE(12, 24)); }

UPB_INLINE void google_protobuf_OneofDescriptorProto_set_name(google_protobuf_OneofDescriptorProto *msg, upb_strview value) {
  _upb_sethas(msg, 1);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(4, 8)) = value;
}
UPB_INLINE void google_protobuf_OneofDescriptorProto_set_options(google_protobuf_OneofDescriptorProto *msg, google_protobuf_OneofOptions* value) {
  _upb_sethas(msg, 2);
  UPB_FIELD_AT(msg, google_protobuf_OneofOptions*, UPB_SIZE(12, 24)) = value;
}
UPB_INLINE struct google_protobuf_OneofOptions* google_protobuf_OneofDescriptorProto_mutable_options(google_protobuf_OneofDescriptorProto *msg, upb_arena *arena) {
  struct google_protobuf_OneofOptions* sub = (struct google_protobuf_OneofOptions*)google_protobuf_OneofDescriptorProto_options(msg);
  if (sub == NULL) {
    sub = (struct google_protobuf_OneofOptions*)upb_msg_new(&google_protobuf_OneofOptions_msginit, arena);
    if (!sub) return NULL;
    google_protobuf_OneofDescriptorProto_set_options(msg, sub);
  }
  return sub;
}

/* google.protobuf.EnumDescriptorProto */

UPB_INLINE google_protobuf_EnumDescriptorProto *google_protobuf_EnumDescriptorProto_new(upb_arena *arena) {
  return (google_protobuf_EnumDescriptorProto *)upb_msg_new(&google_protobuf_EnumDescriptorProto_msginit, arena);
}
UPB_INLINE google_protobuf_EnumDescriptorProto *google_protobuf_EnumDescriptorProto_parse(const char *buf, size_t size,
                        upb_arena *arena) {
  google_protobuf_EnumDescriptorProto *ret = google_protobuf_EnumDescriptorProto_new(arena);
  return (ret && upb_decode(buf, size, ret, &google_protobuf_EnumDescriptorProto_msginit, arena)) ? ret : NULL;
}
UPB_INLINE char *google_protobuf_EnumDescriptorProto_serialize(const google_protobuf_EnumDescriptorProto *msg, upb_arena *arena, size_t *len) {
  return upb_encode(msg, &google_protobuf_EnumDescriptorProto_msginit, arena, len);
}

UPB_INLINE bool google_protobuf_EnumDescriptorProto_has_name(const google_protobuf_EnumDescriptorProto *msg) { return _upb_has_field(msg, 1); }
UPB_INLINE upb_strview google_protobuf_EnumDescriptorProto_name(const google_protobuf_EnumDescriptorProto *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(4, 8)); }
UPB_INLINE const google_protobuf_EnumValueDescriptorProto* const* google_protobuf_EnumDescriptorProto_value(const google_protobuf_EnumDescriptorProto *msg, size_t *len) { return (const google_protobuf_EnumValueDescriptorProto* const*)_upb_array_accessor(msg, UPB_SIZE(16, 32), len); }
UPB_INLINE bool google_protobuf_EnumDescriptorProto_has_options(const google_protobuf_EnumDescriptorProto *msg) { return _upb_has_field(msg, 2); }
UPB_INLINE const google_protobuf_EnumOptions* google_protobuf_EnumDescriptorProto_options(const google_protobuf_EnumDescriptorProto *msg) { return UPB_FIELD_AT(msg, const google_protobuf_EnumOptions*, UPB_SIZE(12, 24)); }
UPB_INLINE const google_protobuf_EnumDescriptorProto_EnumReservedRange* const* google_protobuf_EnumDescriptorProto_reserved_range(const google_protobuf_EnumDescriptorProto *msg, size_t *len) { return (const google_protobuf_EnumDescriptorProto_EnumReservedRange* const*)_upb_array_accessor(msg, UPB_SIZE(20, 40), len); }
UPB_INLINE upb_strview const* google_protobuf_EnumDescriptorProto_reserved_name(const google_protobuf_EnumDescriptorProto *msg, size_t *len) { return (upb_strview const*)_upb_array_accessor(msg, UPB_SIZE(24, 48), len); }

UPB_INLINE void google_protobuf_EnumDescriptorProto_set_name(google_protobuf_EnumDescriptorProto *msg, upb_strview value) {
  _upb_sethas(msg, 1);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(4, 8)) = value;
}
UPB_INLINE google_protobuf_EnumValueDescriptorProto** google_protobuf_EnumDescriptorProto_mutable_value(google_protobuf_EnumDescriptorProto *msg, size_t *len) {
  return (google_protobuf_EnumValueDescriptorProto**)_upb_array_mutable_accessor(msg, UPB_SIZE(16, 32), len);
}
UPB_INLINE google_protobuf_EnumValueDescriptorProto** google_protobuf_EnumDescriptorProto_resize_value(google_protobuf_EnumDescriptorProto *msg, size_t len, upb_arena *arena) {
  return (google_protobuf_EnumValueDescriptorProto**)_upb_array_resize_accessor(msg, UPB_SIZE(16, 32), len, UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, arena);
}
UPB_INLINE struct google_protobuf_EnumValueDescriptorProto* google_protobuf_EnumDescriptorProto_add_value(google_protobuf_EnumDescriptorProto *msg, upb_arena *arena) {
  struct google_protobuf_EnumValueDescriptorProto* sub = (struct google_protobuf_EnumValueDescriptorProto*)upb_msg_new(&google_protobuf_EnumValueDescriptorProto_msginit, arena);
  bool ok = _upb_array_append_accessor(
      msg, UPB_SIZE(16, 32), UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, &sub, arena);
  if (!ok) return NULL;
  return sub;
}
UPB_INLINE void google_protobuf_EnumDescriptorProto_set_options(google_protobuf_EnumDescriptorProto *msg, google_protobuf_EnumOptions* value) {
  _upb_sethas(msg, 2);
  UPB_FIELD_AT(msg, google_protobuf_EnumOptions*, UPB_SIZE(12, 24)) = value;
}
UPB_INLINE struct google_protobuf_EnumOptions* google_protobuf_EnumDescriptorProto_mutable_options(google_protobuf_EnumDescriptorProto *msg, upb_arena *arena) {
  struct google_protobuf_EnumOptions* sub = (struct google_protobuf_EnumOptions*)google_protobuf_EnumDescriptorProto_options(msg);
  if (sub == NULL) {
    sub = (struct google_protobuf_EnumOptions*)upb_msg_new(&google_protobuf_EnumOptions_msginit, arena);
    if (!sub) return NULL;
    google_protobuf_EnumDescriptorProto_set_options(msg, sub);
  }
  return sub;
}
UPB_INLINE google_protobuf_EnumDescriptorProto_EnumReservedRange** google_protobuf_EnumDescriptorProto_mutable_reserved_range(google_protobuf_EnumDescriptorProto *msg, size_t *len) {
  return (google_protobuf_EnumDescriptorProto_EnumReservedRange**)_upb_array_mutable_accessor(msg, UPB_SIZE(20, 40), len);
}
UPB_INLINE google_protobuf_EnumDescriptorProto_EnumReservedRange** google_protobuf_EnumDescriptorProto_resize_reserved_range(google_protobuf_EnumDescriptorProto *msg, size_t len, upb_arena *arena) {
  return (google_protobuf_EnumDescriptorProto_EnumReservedRange**)_upb_array_resize_accessor(msg, UPB_SIZE(20, 40), len, UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, arena);
}
UPB_INLINE struct google_protobuf_EnumDescriptorProto_EnumReservedRange* google_protobuf_EnumDescriptorProto_add_reserved_range(google_protobuf_EnumDescriptorProto *msg, upb_arena *arena) {
  struct google_protobuf_EnumDescriptorProto_EnumReservedRange* sub = (struct google_protobuf_EnumDescriptorProto_EnumReservedRange*)upb_msg_new(&google_protobuf_EnumDescriptorProto_EnumReservedRange_msginit, arena);
  bool ok = _upb_array_append_accessor(
      msg, UPB_SIZE(20, 40), UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, &sub, arena);
  if (!ok) return NULL;
  return sub;
}
UPB_INLINE upb_strview* google_protobuf_EnumDescriptorProto_mutable_reserved_name(google_protobuf_EnumDescriptorProto *msg, size_t *len) {
  return (upb_strview*)_upb_array_mutable_accessor(msg, UPB_SIZE(24, 48), len);
}
UPB_INLINE upb_strview* google_protobuf_EnumDescriptorProto_resize_reserved_name(google_protobuf_EnumDescriptorProto *msg, size_t len, upb_arena *arena) {
  return (upb_strview*)_upb_array_resize_accessor(msg, UPB_SIZE(24, 48), len, UPB_SIZE(8, 16), UPB_TYPE_STRING, arena);
}
UPB_INLINE bool google_protobuf_EnumDescriptorProto_add_reserved_name(google_protobuf_EnumDescriptorProto *msg, upb_strview val, upb_arena *arena) {
  return _upb_array_append_accessor(
      msg, UPB_SIZE(24, 48), UPB_SIZE(8, 16), UPB_TYPE_STRING, &val, arena);
}

/* google.protobuf.EnumDescriptorProto.EnumReservedRange */

UPB_INLINE google_protobuf_EnumDescriptorProto_EnumReservedRange *google_protobuf_EnumDescriptorProto_EnumReservedRange_new(upb_arena *arena) {
  return (google_protobuf_EnumDescriptorProto_EnumReservedRange *)upb_msg_new(&google_protobuf_EnumDescriptorProto_EnumReservedRange_msginit, arena);
}
UPB_INLINE google_protobuf_EnumDescriptorProto_EnumReservedRange *google_protobuf_EnumDescriptorProto_EnumReservedRange_parse(const char *buf, size_t size,
                        upb_arena *arena) {
  google_protobuf_EnumDescriptorProto_EnumReservedRange *ret = google_protobuf_EnumDescriptorProto_EnumReservedRange_new(arena);
  return (ret && upb_decode(buf, size, ret, &google_protobuf_EnumDescriptorProto_EnumReservedRange_msginit, arena)) ? ret : NULL;
}
UPB_INLINE char *google_protobuf_EnumDescriptorProto_EnumReservedRange_serialize(const google_protobuf_EnumDescriptorProto_EnumReservedRange *msg, upb_arena *arena, size_t *len) {
  return upb_encode(msg, &google_protobuf_EnumDescriptorProto_EnumReservedRange_msginit, arena, len);
}

UPB_INLINE bool google_protobuf_EnumDescriptorProto_EnumReservedRange_has_start(const google_protobuf_EnumDescriptorProto_EnumReservedRange *msg) { return _upb_has_field(msg, 1); }
UPB_INLINE int32_t google_protobuf_EnumDescriptorProto_EnumReservedRange_start(const google_protobuf_EnumDescriptorProto_EnumReservedRange *msg) { return UPB_FIELD_AT(msg, int32_t, UPB_SIZE(4, 4)); }
UPB_INLINE bool google_protobuf_EnumDescriptorProto_EnumReservedRange_has_end(const google_protobuf_EnumDescriptorProto_EnumReservedRange *msg) { return _upb_has_field(msg, 2); }
UPB_INLINE int32_t google_protobuf_EnumDescriptorProto_EnumReservedRange_end(const google_protobuf_EnumDescriptorProto_EnumReservedRange *msg) { return UPB_FIELD_AT(msg, int32_t, UPB_SIZE(8, 8)); }

UPB_INLINE void google_protobuf_EnumDescriptorProto_EnumReservedRange_set_start(google_protobuf_EnumDescriptorProto_EnumReservedRange *msg, int32_t value) {
  _upb_sethas(msg, 1);
  UPB_FIELD_AT(msg, int32_t, UPB_SIZE(4, 4)) = value;
}
UPB_INLINE void google_protobuf_EnumDescriptorProto_EnumReservedRange_set_end(google_protobuf_EnumDescriptorProto_EnumReservedRange *msg, int32_t value) {
  _upb_sethas(msg, 2);
  UPB_FIELD_AT(msg, int32_t, UPB_SIZE(8, 8)) = value;
}

/* google.protobuf.EnumValueDescriptorProto */

UPB_INLINE google_protobuf_EnumValueDescriptorProto *google_protobuf_EnumValueDescriptorProto_new(upb_arena *arena) {
  return (google_protobuf_EnumValueDescriptorProto *)upb_msg_new(&google_protobuf_EnumValueDescriptorProto_msginit, arena);
}
UPB_INLINE google_protobuf_EnumValueDescriptorProto *google_protobuf_EnumValueDescriptorProto_parse(const char *buf, size_t size,
                        upb_arena *arena) {
  google_protobuf_EnumValueDescriptorProto *ret = google_protobuf_EnumValueDescriptorProto_new(arena);
  return (ret && upb_decode(buf, size, ret, &google_protobuf_EnumValueDescriptorProto_msginit, arena)) ? ret : NULL;
}
UPB_INLINE char *google_protobuf_EnumValueDescriptorProto_serialize(const google_protobuf_EnumValueDescriptorProto *msg, upb_arena *arena, size_t *len) {
  return upb_encode(msg, &google_protobuf_EnumValueDescriptorProto_msginit, arena, len);
}

UPB_INLINE bool google_protobuf_EnumValueDescriptorProto_has_name(const google_protobuf_EnumValueDescriptorProto *msg) { return _upb_has_field(msg, 2); }
UPB_INLINE upb_strview google_protobuf_EnumValueDescriptorProto_name(const google_protobuf_EnumValueDescriptorProto *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(8, 8)); }
UPB_INLINE bool google_protobuf_EnumValueDescriptorProto_has_number(const google_protobuf_EnumValueDescriptorProto *msg) { return _upb_has_field(msg, 1); }
UPB_INLINE int32_t google_protobuf_EnumValueDescriptorProto_number(const google_protobuf_EnumValueDescriptorProto *msg) { return UPB_FIELD_AT(msg, int32_t, UPB_SIZE(4, 4)); }
UPB_INLINE bool google_protobuf_EnumValueDescriptorProto_has_options(const google_protobuf_EnumValueDescriptorProto *msg) { return _upb_has_field(msg, 3); }
UPB_INLINE const google_protobuf_EnumValueOptions* google_protobuf_EnumValueDescriptorProto_options(const google_protobuf_EnumValueDescriptorProto *msg) { return UPB_FIELD_AT(msg, const google_protobuf_EnumValueOptions*, UPB_SIZE(16, 24)); }

UPB_INLINE void google_protobuf_EnumValueDescriptorProto_set_name(google_protobuf_EnumValueDescriptorProto *msg, upb_strview value) {
  _upb_sethas(msg, 2);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(8, 8)) = value;
}
UPB_INLINE void google_protobuf_EnumValueDescriptorProto_set_number(google_protobuf_EnumValueDescriptorProto *msg, int32_t value) {
  _upb_sethas(msg, 1);
  UPB_FIELD_AT(msg, int32_t, UPB_SIZE(4, 4)) = value;
}
UPB_INLINE void google_protobuf_EnumValueDescriptorProto_set_options(google_protobuf_EnumValueDescriptorProto *msg, google_protobuf_EnumValueOptions* value) {
  _upb_sethas(msg, 3);
  UPB_FIELD_AT(msg, google_protobuf_EnumValueOptions*, UPB_SIZE(16, 24)) = value;
}
UPB_INLINE struct google_protobuf_EnumValueOptions* google_protobuf_EnumValueDescriptorProto_mutable_options(google_protobuf_EnumValueDescriptorProto *msg, upb_arena *arena) {
  struct google_protobuf_EnumValueOptions* sub = (struct google_protobuf_EnumValueOptions*)google_protobuf_EnumValueDescriptorProto_options(msg);
  if (sub == NULL) {
    sub = (struct google_protobuf_EnumValueOptions*)upb_msg_new(&google_protobuf_EnumValueOptions_msginit, arena);
    if (!sub) return NULL;
    google_protobuf_EnumValueDescriptorProto_set_options(msg, sub);
  }
  return sub;
}

/* google.protobuf.ServiceDescriptorProto */

UPB_INLINE google_protobuf_ServiceDescriptorProto *google_protobuf_ServiceDescriptorProto_new(upb_arena *arena) {
  return (google_protobuf_ServiceDescriptorProto *)upb_msg_new(&google_protobuf_ServiceDescriptorProto_msginit, arena);
}
UPB_INLINE google_protobuf_ServiceDescriptorProto *google_protobuf_ServiceDescriptorProto_parse(const char *buf, size_t size,
                        upb_arena *arena) {
  google_protobuf_ServiceDescriptorProto *ret = google_protobuf_ServiceDescriptorProto_new(arena);
  return (ret && upb_decode(buf, size, ret, &google_protobuf_ServiceDescriptorProto_msginit, arena)) ? ret : NULL;
}
UPB_INLINE char *google_protobuf_ServiceDescriptorProto_serialize(const google_protobuf_ServiceDescriptorProto *msg, upb_arena *arena, size_t *len) {
  return upb_encode(msg, &google_protobuf_ServiceDescriptorProto_msginit, arena, len);
}

UPB_INLINE bool google_protobuf_ServiceDescriptorProto_has_name(const google_protobuf_ServiceDescriptorProto *msg) { return _upb_has_field(msg, 1); }
UPB_INLINE upb_strview google_protobuf_ServiceDescriptorProto_name(const google_protobuf_ServiceDescriptorProto *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(4, 8)); }
UPB_INLINE const google_protobuf_MethodDescriptorProto* const* google_protobuf_ServiceDescriptorProto_method(const google_protobuf_ServiceDescriptorProto *msg, size_t *len) { return (const google_protobuf_MethodDescriptorProto* const*)_upb_array_accessor(msg, UPB_SIZE(16, 32), len); }
UPB_INLINE bool google_protobuf_ServiceDescriptorProto_has_options(const google_protobuf_ServiceDescriptorProto *msg) { return _upb_has_field(msg, 2); }
UPB_INLINE const google_protobuf_ServiceOptions* google_protobuf_ServiceDescriptorProto_options(const google_protobuf_ServiceDescriptorProto *msg) { return UPB_FIELD_AT(msg, const google_protobuf_ServiceOptions*, UPB_SIZE(12, 24)); }

UPB_INLINE void google_protobuf_ServiceDescriptorProto_set_name(google_protobuf_ServiceDescriptorProto *msg, upb_strview value) {
  _upb_sethas(msg, 1);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(4, 8)) = value;
}
UPB_INLINE google_protobuf_MethodDescriptorProto** google_protobuf_ServiceDescriptorProto_mutable_method(google_protobuf_ServiceDescriptorProto *msg, size_t *len) {
  return (google_protobuf_MethodDescriptorProto**)_upb_array_mutable_accessor(msg, UPB_SIZE(16, 32), len);
}
UPB_INLINE google_protobuf_MethodDescriptorProto** google_protobuf_ServiceDescriptorProto_resize_method(google_protobuf_ServiceDescriptorProto *msg, size_t len, upb_arena *arena) {
  return (google_protobuf_MethodDescriptorProto**)_upb_array_resize_accessor(msg, UPB_SIZE(16, 32), len, UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, arena);
}
UPB_INLINE struct google_protobuf_MethodDescriptorProto* google_protobuf_ServiceDescriptorProto_add_method(google_protobuf_ServiceDescriptorProto *msg, upb_arena *arena) {
  struct google_protobuf_MethodDescriptorProto* sub = (struct google_protobuf_MethodDescriptorProto*)upb_msg_new(&google_protobuf_MethodDescriptorProto_msginit, arena);
  bool ok = _upb_array_append_accessor(
      msg, UPB_SIZE(16, 32), UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, &sub, arena);
  if (!ok) return NULL;
  return sub;
}
UPB_INLINE void google_protobuf_ServiceDescriptorProto_set_options(google_protobuf_ServiceDescriptorProto *msg, google_protobuf_ServiceOptions* value) {
  _upb_sethas(msg, 2);
  UPB_FIELD_AT(msg, google_protobuf_ServiceOptions*, UPB_SIZE(12, 24)) = value;
}
UPB_INLINE struct google_protobuf_ServiceOptions* google_protobuf_ServiceDescriptorProto_mutable_options(google_protobuf_ServiceDescriptorProto *msg, upb_arena *arena) {
  struct google_protobuf_ServiceOptions* sub = (struct google_protobuf_ServiceOptions*)google_protobuf_ServiceDescriptorProto_options(msg);
  if (sub == NULL) {
    sub = (struct google_protobuf_ServiceOptions*)upb_msg_new(&google_protobuf_ServiceOptions_msginit, arena);
    if (!sub) return NULL;
    google_protobuf_ServiceDescriptorProto_set_options(msg, sub);
  }
  return sub;
}

/* google.protobuf.MethodDescriptorProto */

UPB_INLINE google_protobuf_MethodDescriptorProto *google_protobuf_MethodDescriptorProto_new(upb_arena *arena) {
  return (google_protobuf_MethodDescriptorProto *)upb_msg_new(&google_protobuf_MethodDescriptorProto_msginit, arena);
}
UPB_INLINE google_protobuf_MethodDescriptorProto *google_protobuf_MethodDescriptorProto_parse(const char *buf, size_t size,
                        upb_arena *arena) {
  google_protobuf_MethodDescriptorProto *ret = google_protobuf_MethodDescriptorProto_new(arena);
  return (ret && upb_decode(buf, size, ret, &google_protobuf_MethodDescriptorProto_msginit, arena)) ? ret : NULL;
}
UPB_INLINE char *google_protobuf_MethodDescriptorProto_serialize(const google_protobuf_MethodDescriptorProto *msg, upb_arena *arena, size_t *len) {
  return upb_encode(msg, &google_protobuf_MethodDescriptorProto_msginit, arena, len);
}

UPB_INLINE bool google_protobuf_MethodDescriptorProto_has_name(const google_protobuf_MethodDescriptorProto *msg) { return _upb_has_field(msg, 3); }
UPB_INLINE upb_strview google_protobuf_MethodDescriptorProto_name(const google_protobuf_MethodDescriptorProto *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(4, 8)); }
UPB_INLINE bool google_protobuf_MethodDescriptorProto_has_input_type(const google_protobuf_MethodDescriptorProto *msg) { return _upb_has_field(msg, 4); }
UPB_INLINE upb_strview google_protobuf_MethodDescriptorProto_input_type(const google_protobuf_MethodDescriptorProto *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(12, 24)); }
UPB_INLINE bool google_protobuf_MethodDescriptorProto_has_output_type(const google_protobuf_MethodDescriptorProto *msg) { return _upb_has_field(msg, 5); }
UPB_INLINE upb_strview google_protobuf_MethodDescriptorProto_output_type(const google_protobuf_MethodDescriptorProto *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(20, 40)); }
UPB_INLINE bool google_protobuf_MethodDescriptorProto_has_options(const google_protobuf_MethodDescriptorProto *msg) { return _upb_has_field(msg, 6); }
UPB_INLINE const google_protobuf_MethodOptions* google_protobuf_MethodDescriptorProto_options(const google_protobuf_MethodDescriptorProto *msg) { return UPB_FIELD_AT(msg, const google_protobuf_MethodOptions*, UPB_SIZE(28, 56)); }
UPB_INLINE bool google_protobuf_MethodDescriptorProto_has_client_streaming(const google_protobuf_MethodDescriptorProto *msg) { return _upb_has_field(msg, 1); }
UPB_INLINE bool google_protobuf_MethodDescriptorProto_client_streaming(const google_protobuf_MethodDescriptorProto *msg) { return UPB_FIELD_AT(msg, bool, UPB_SIZE(1, 1)); }
UPB_INLINE bool google_protobuf_MethodDescriptorProto_has_server_streaming(const google_protobuf_MethodDescriptorProto *msg) { return _upb_has_field(msg, 2); }
UPB_INLINE bool google_protobuf_MethodDescriptorProto_server_streaming(const google_protobuf_MethodDescriptorProto *msg) { return UPB_FIELD_AT(msg, bool, UPB_SIZE(2, 2)); }

UPB_INLINE void google_protobuf_MethodDescriptorProto_set_name(google_protobuf_MethodDescriptorProto *msg, upb_strview value) {
  _upb_sethas(msg, 3);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(4, 8)) = value;
}
UPB_INLINE void google_protobuf_MethodDescriptorProto_set_input_type(google_protobuf_MethodDescriptorProto *msg, upb_strview value) {
  _upb_sethas(msg, 4);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(12, 24)) = value;
}
UPB_INLINE void google_protobuf_MethodDescriptorProto_set_output_type(google_protobuf_MethodDescriptorProto *msg, upb_strview value) {
  _upb_sethas(msg, 5);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(20, 40)) = value;
}
UPB_INLINE void google_protobuf_MethodDescriptorProto_set_options(google_protobuf_MethodDescriptorProto *msg, google_protobuf_MethodOptions* value) {
  _upb_sethas(msg, 6);
  UPB_FIELD_AT(msg, google_protobuf_MethodOptions*, UPB_SIZE(28, 56)) = value;
}
UPB_INLINE struct google_protobuf_MethodOptions* google_protobuf_MethodDescriptorProto_mutable_options(google_protobuf_MethodDescriptorProto *msg, upb_arena *arena) {
  struct google_protobuf_MethodOptions* sub = (struct google_protobuf_MethodOptions*)google_protobuf_MethodDescriptorProto_options(msg);
  if (sub == NULL) {
    sub = (struct google_protobuf_MethodOptions*)upb_msg_new(&google_protobuf_MethodOptions_msginit, arena);
    if (!sub) return NULL;
    google_protobuf_MethodDescriptorProto_set_options(msg, sub);
  }
  return sub;
}
UPB_INLINE void google_protobuf_MethodDescriptorProto_set_client_streaming(google_protobuf_MethodDescriptorProto *msg, bool value) {
  _upb_sethas(msg, 1);
  UPB_FIELD_AT(msg, bool, UPB_SIZE(1, 1)) = value;
}
UPB_INLINE void google_protobuf_MethodDescriptorProto_set_server_streaming(google_protobuf_MethodDescriptorProto *msg, bool value) {
  _upb_sethas(msg, 2);
  UPB_FIELD_AT(msg, bool, UPB_SIZE(2, 2)) = value;
}

/* google.protobuf.FileOptions */

UPB_INLINE google_protobuf_FileOptions *google_protobuf_FileOptions_new(upb_arena *arena) {
  return (google_protobuf_FileOptions *)upb_msg_new(&google_protobuf_FileOptions_msginit, arena);
}
UPB_INLINE google_protobuf_FileOptions *google_protobuf_FileOptions_parse(const char *buf, size_t size,
                        upb_arena *arena) {
  google_protobuf_FileOptions *ret = google_protobuf_FileOptions_new(arena);
  return (ret && upb_decode(buf, size, ret, &google_protobuf_FileOptions_msginit, arena)) ? ret : NULL;
}
UPB_INLINE char *google_protobuf_FileOptions_serialize(const google_protobuf_FileOptions *msg, upb_arena *arena, size_t *len) {
  return upb_encode(msg, &google_protobuf_FileOptions_msginit, arena, len);
}

UPB_INLINE bool google_protobuf_FileOptions_has_java_package(const google_protobuf_FileOptions *msg) { return _upb_has_field(msg, 11); }
UPB_INLINE upb_strview google_protobuf_FileOptions_java_package(const google_protobuf_FileOptions *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(28, 32)); }
UPB_INLINE bool google_protobuf_FileOptions_has_java_outer_classname(const google_protobuf_FileOptions *msg) { return _upb_has_field(msg, 12); }
UPB_INLINE upb_strview google_protobuf_FileOptions_java_outer_classname(const google_protobuf_FileOptions *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(36, 48)); }
UPB_INLINE bool google_protobuf_FileOptions_has_optimize_for(const google_protobuf_FileOptions *msg) { return _upb_has_field(msg, 1); }
UPB_INLINE int32_t google_protobuf_FileOptions_optimize_for(const google_protobuf_FileOptions *msg) { return UPB_FIELD_AT(msg, int32_t, UPB_SIZE(8, 8)); }
UPB_INLINE bool google_protobuf_FileOptions_has_java_multiple_files(const google_protobuf_FileOptions *msg) { return _upb_has_field(msg, 2); }
UPB_INLINE bool google_protobuf_FileOptions_java_multiple_files(const google_protobuf_FileOptions *msg) { return UPB_FIELD_AT(msg, bool, UPB_SIZE(16, 16)); }
UPB_INLINE bool google_protobuf_FileOptions_has_go_package(const google_protobuf_FileOptions *msg) { return _upb_has_field(msg, 13); }
UPB_INLINE upb_strview google_protobuf_FileOptions_go_package(const google_protobuf_FileOptions *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(44, 64)); }
UPB_INLINE bool google_protobuf_FileOptions_has_cc_generic_services(const google_protobuf_FileOptions *msg) { return _upb_has_field(msg, 3); }
UPB_INLINE bool google_protobuf_FileOptions_cc_generic_services(const google_protobuf_FileOptions *msg) { return UPB_FIELD_AT(msg, bool, UPB_SIZE(17, 17)); }
UPB_INLINE bool google_protobuf_FileOptions_has_java_generic_services(const google_protobuf_FileOptions *msg) { return _upb_has_field(msg, 4); }
UPB_INLINE bool google_protobuf_FileOptions_java_generic_services(const google_protobuf_FileOptions *msg) { return UPB_FIELD_AT(msg, bool, UPB_SIZE(18, 18)); }
UPB_INLINE bool google_protobuf_FileOptions_has_py_generic_services(const google_protobuf_FileOptions *msg) { return _upb_has_field(msg, 5); }
UPB_INLINE bool google_protobuf_FileOptions_py_generic_services(const google_protobuf_FileOptions *msg) { return UPB_FIELD_AT(msg, bool, UPB_SIZE(19, 19)); }
UPB_INLINE bool google_protobuf_FileOptions_has_java_generate_equals_and_hash(const google_protobuf_FileOptions *msg) { return _upb_has_field(msg, 6); }
UPB_INLINE bool google_protobuf_FileOptions_java_generate_equals_and_hash(const google_protobuf_FileOptions *msg) { return UPB_FIELD_AT(msg, bool, UPB_SIZE(20, 20)); }
UPB_INLINE bool google_protobuf_FileOptions_has_deprecated(const google_protobuf_FileOptions *msg) { return _upb_has_field(msg, 7); }
UPB_INLINE bool google_protobuf_FileOptions_deprecated(const google_protobuf_FileOptions *msg) { return UPB_FIELD_AT(msg, bool, UPB_SIZE(21, 21)); }
UPB_INLINE bool google_protobuf_FileOptions_has_java_string_check_utf8(const google_protobuf_FileOptions *msg) { return _upb_has_field(msg, 8); }
UPB_INLINE bool google_protobuf_FileOptions_java_string_check_utf8(const google_protobuf_FileOptions *msg) { return UPB_FIELD_AT(msg, bool, UPB_SIZE(22, 22)); }
UPB_INLINE bool google_protobuf_FileOptions_has_cc_enable_arenas(const google_protobuf_FileOptions *msg) { return _upb_has_field(msg, 9); }
UPB_INLINE bool google_protobuf_FileOptions_cc_enable_arenas(const google_protobuf_FileOptions *msg) { return UPB_FIELD_AT(msg, bool, UPB_SIZE(23, 23)); }
UPB_INLINE bool google_protobuf_FileOptions_has_objc_class_prefix(const google_protobuf_FileOptions *msg) { return _upb_has_field(msg, 14); }
UPB_INLINE upb_strview google_protobuf_FileOptions_objc_class_prefix(const google_protobuf_FileOptions *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(52, 80)); }
UPB_INLINE bool google_protobuf_FileOptions_has_csharp_namespace(const google_protobuf_FileOptions *msg) { return _upb_has_field(msg, 15); }
UPB_INLINE upb_strview google_protobuf_FileOptions_csharp_namespace(const google_protobuf_FileOptions *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(60, 96)); }
UPB_INLINE bool google_protobuf_FileOptions_has_swift_prefix(const google_protobuf_FileOptions *msg) { return _upb_has_field(msg, 16); }
UPB_INLINE upb_strview google_protobuf_FileOptions_swift_prefix(const google_protobuf_FileOptions *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(68, 112)); }
UPB_INLINE bool google_protobuf_FileOptions_has_php_class_prefix(const google_protobuf_FileOptions *msg) { return _upb_has_field(msg, 17); }
UPB_INLINE upb_strview google_protobuf_FileOptions_php_class_prefix(const google_protobuf_FileOptions *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(76, 128)); }
UPB_INLINE bool google_protobuf_FileOptions_has_php_namespace(const google_protobuf_FileOptions *msg) { return _upb_has_field(msg, 18); }
UPB_INLINE upb_strview google_protobuf_FileOptions_php_namespace(const google_protobuf_FileOptions *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(84, 144)); }
UPB_INLINE bool google_protobuf_FileOptions_has_php_generic_services(const google_protobuf_FileOptions *msg) { return _upb_has_field(msg, 10); }
UPB_INLINE bool google_protobuf_FileOptions_php_generic_services(const google_protobuf_FileOptions *msg) { return UPB_FIELD_AT(msg, bool, UPB_SIZE(24, 24)); }
UPB_INLINE bool google_protobuf_FileOptions_has_php_metadata_namespace(const google_protobuf_FileOptions *msg) { return _upb_has_field(msg, 19); }
UPB_INLINE upb_strview google_protobuf_FileOptions_php_metadata_namespace(const google_protobuf_FileOptions *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(92, 160)); }
UPB_INLINE bool google_protobuf_FileOptions_has_ruby_package(const google_protobuf_FileOptions *msg) { return _upb_has_field(msg, 20); }
UPB_INLINE upb_strview google_protobuf_FileOptions_ruby_package(const google_protobuf_FileOptions *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(100, 176)); }
UPB_INLINE const google_protobuf_UninterpretedOption* const* google_protobuf_FileOptions_uninterpreted_option(const google_protobuf_FileOptions *msg, size_t *len) { return (const google_protobuf_UninterpretedOption* const*)_upb_array_accessor(msg, UPB_SIZE(108, 192), len); }

UPB_INLINE void google_protobuf_FileOptions_set_java_package(google_protobuf_FileOptions *msg, upb_strview value) {
  _upb_sethas(msg, 11);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(28, 32)) = value;
}
UPB_INLINE void google_protobuf_FileOptions_set_java_outer_classname(google_protobuf_FileOptions *msg, upb_strview value) {
  _upb_sethas(msg, 12);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(36, 48)) = value;
}
UPB_INLINE void google_protobuf_FileOptions_set_optimize_for(google_protobuf_FileOptions *msg, int32_t value) {
  _upb_sethas(msg, 1);
  UPB_FIELD_AT(msg, int32_t, UPB_SIZE(8, 8)) = value;
}
UPB_INLINE void google_protobuf_FileOptions_set_java_multiple_files(google_protobuf_FileOptions *msg, bool value) {
  _upb_sethas(msg, 2);
  UPB_FIELD_AT(msg, bool, UPB_SIZE(16, 16)) = value;
}
UPB_INLINE void google_protobuf_FileOptions_set_go_package(google_protobuf_FileOptions *msg, upb_strview value) {
  _upb_sethas(msg, 13);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(44, 64)) = value;
}
UPB_INLINE void google_protobuf_FileOptions_set_cc_generic_services(google_protobuf_FileOptions *msg, bool value) {
  _upb_sethas(msg, 3);
  UPB_FIELD_AT(msg, bool, UPB_SIZE(17, 17)) = value;
}
UPB_INLINE void google_protobuf_FileOptions_set_java_generic_services(google_protobuf_FileOptions *msg, bool value) {
  _upb_sethas(msg, 4);
  UPB_FIELD_AT(msg, bool, UPB_SIZE(18, 18)) = value;
}
UPB_INLINE void google_protobuf_FileOptions_set_py_generic_services(google_protobuf_FileOptions *msg, bool value) {
  _upb_sethas(msg, 5);
  UPB_FIELD_AT(msg, bool, UPB_SIZE(19, 19)) = value;
}
UPB_INLINE void google_protobuf_FileOptions_set_java_generate_equals_and_hash(google_protobuf_FileOptions *msg, bool value) {
  _upb_sethas(msg, 6);
  UPB_FIELD_AT(msg, bool, UPB_SIZE(20, 20)) = value;
}
UPB_INLINE void google_protobuf_FileOptions_set_deprecated(google_protobuf_FileOptions *msg, bool value) {
  _upb_sethas(msg, 7);
  UPB_FIELD_AT(msg, bool, UPB_SIZE(21, 21)) = value;
}
UPB_INLINE void google_protobuf_FileOptions_set_java_string_check_utf8(google_protobuf_FileOptions *msg, bool value) {
  _upb_sethas(msg, 8);
  UPB_FIELD_AT(msg, bool, UPB_SIZE(22, 22)) = value;
}
UPB_INLINE void google_protobuf_FileOptions_set_cc_enable_arenas(google_protobuf_FileOptions *msg, bool value) {
  _upb_sethas(msg, 9);
  UPB_FIELD_AT(msg, bool, UPB_SIZE(23, 23)) = value;
}
UPB_INLINE void google_protobuf_FileOptions_set_objc_class_prefix(google_protobuf_FileOptions *msg, upb_strview value) {
  _upb_sethas(msg, 14);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(52, 80)) = value;
}
UPB_INLINE void google_protobuf_FileOptions_set_csharp_namespace(google_protobuf_FileOptions *msg, upb_strview value) {
  _upb_sethas(msg, 15);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(60, 96)) = value;
}
UPB_INLINE void google_protobuf_FileOptions_set_swift_prefix(google_protobuf_FileOptions *msg, upb_strview value) {
  _upb_sethas(msg, 16);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(68, 112)) = value;
}
UPB_INLINE void google_protobuf_FileOptions_set_php_class_prefix(google_protobuf_FileOptions *msg, upb_strview value) {
  _upb_sethas(msg, 17);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(76, 128)) = value;
}
UPB_INLINE void google_protobuf_FileOptions_set_php_namespace(google_protobuf_FileOptions *msg, upb_strview value) {
  _upb_sethas(msg, 18);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(84, 144)) = value;
}
UPB_INLINE void google_protobuf_FileOptions_set_php_generic_services(google_protobuf_FileOptions *msg, bool value) {
  _upb_sethas(msg, 10);
  UPB_FIELD_AT(msg, bool, UPB_SIZE(24, 24)) = value;
}
UPB_INLINE void google_protobuf_FileOptions_set_php_metadata_namespace(google_protobuf_FileOptions *msg, upb_strview value) {
  _upb_sethas(msg, 19);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(92, 160)) = value;
}
UPB_INLINE void google_protobuf_FileOptions_set_ruby_package(google_protobuf_FileOptions *msg, upb_strview value) {
  _upb_sethas(msg, 20);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(100, 176)) = value;
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_FileOptions_mutable_uninterpreted_option(google_protobuf_FileOptions *msg, size_t *len) {
  return (google_protobuf_UninterpretedOption**)_upb_array_mutable_accessor(msg, UPB_SIZE(108, 192), len);
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_FileOptions_resize_uninterpreted_option(google_protobuf_FileOptions *msg, size_t len, upb_arena *arena) {
  return (google_protobuf_UninterpretedOption**)_upb_array_resize_accessor(msg, UPB_SIZE(108, 192), len, UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, arena);
}
UPB_INLINE struct google_protobuf_UninterpretedOption* google_protobuf_FileOptions_add_uninterpreted_option(google_protobuf_FileOptions *msg, upb_arena *arena) {
  struct google_protobuf_UninterpretedOption* sub = (struct google_protobuf_UninterpretedOption*)upb_msg_new(&google_protobuf_UninterpretedOption_msginit, arena);
  bool ok = _upb_array_append_accessor(
      msg, UPB_SIZE(108, 192), UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, &sub, arena);
  if (!ok) return NULL;
  return sub;
}

/* google.protobuf.MessageOptions */

UPB_INLINE google_protobuf_MessageOptions *google_protobuf_MessageOptions_new(upb_arena *arena) {
  return (google_protobuf_MessageOptions *)upb_msg_new(&google_protobuf_MessageOptions_msginit, arena);
}
UPB_INLINE google_protobuf_MessageOptions *google_protobuf_MessageOptions_parse(const char *buf, size_t size,
                        upb_arena *arena) {
  google_protobuf_MessageOptions *ret = google_protobuf_MessageOptions_new(arena);
  return (ret && upb_decode(buf, size, ret, &google_protobuf_MessageOptions_msginit, arena)) ? ret : NULL;
}
UPB_INLINE char *google_protobuf_MessageOptions_serialize(const google_protobuf_MessageOptions *msg, upb_arena *arena, size_t *len) {
  return upb_encode(msg, &google_protobuf_MessageOptions_msginit, arena, len);
}

UPB_INLINE bool google_protobuf_MessageOptions_has_message_set_wire_format(const google_protobuf_MessageOptions *msg) { return _upb_has_field(msg, 1); }
UPB_INLINE bool google_protobuf_MessageOptions_message_set_wire_format(const google_protobuf_MessageOptions *msg) { return UPB_FIELD_AT(msg, bool, UPB_SIZE(1, 1)); }
UPB_INLINE bool google_protobuf_MessageOptions_has_no_standard_descriptor_accessor(const google_protobuf_MessageOptions *msg) { return _upb_has_field(msg, 2); }
UPB_INLINE bool google_protobuf_MessageOptions_no_standard_descriptor_accessor(const google_protobuf_MessageOptions *msg) { return UPB_FIELD_AT(msg, bool, UPB_SIZE(2, 2)); }
UPB_INLINE bool google_protobuf_MessageOptions_has_deprecated(const google_protobuf_MessageOptions *msg) { return _upb_has_field(msg, 3); }
UPB_INLINE bool google_protobuf_MessageOptions_deprecated(const google_protobuf_MessageOptions *msg) { return UPB_FIELD_AT(msg, bool, UPB_SIZE(3, 3)); }
UPB_INLINE bool google_protobuf_MessageOptions_has_map_entry(const google_protobuf_MessageOptions *msg) { return _upb_has_field(msg, 4); }
UPB_INLINE bool google_protobuf_MessageOptions_map_entry(const google_protobuf_MessageOptions *msg) { return UPB_FIELD_AT(msg, bool, UPB_SIZE(4, 4)); }
UPB_INLINE const google_protobuf_UninterpretedOption* const* google_protobuf_MessageOptions_uninterpreted_option(const google_protobuf_MessageOptions *msg, size_t *len) { return (const google_protobuf_UninterpretedOption* const*)_upb_array_accessor(msg, UPB_SIZE(8, 8), len); }

UPB_INLINE void google_protobuf_MessageOptions_set_message_set_wire_format(google_protobuf_MessageOptions *msg, bool value) {
  _upb_sethas(msg, 1);
  UPB_FIELD_AT(msg, bool, UPB_SIZE(1, 1)) = value;
}
UPB_INLINE void google_protobuf_MessageOptions_set_no_standard_descriptor_accessor(google_protobuf_MessageOptions *msg, bool value) {
  _upb_sethas(msg, 2);
  UPB_FIELD_AT(msg, bool, UPB_SIZE(2, 2)) = value;
}
UPB_INLINE void google_protobuf_MessageOptions_set_deprecated(google_protobuf_MessageOptions *msg, bool value) {
  _upb_sethas(msg, 3);
  UPB_FIELD_AT(msg, bool, UPB_SIZE(3, 3)) = value;
}
UPB_INLINE void google_protobuf_MessageOptions_set_map_entry(google_protobuf_MessageOptions *msg, bool value) {
  _upb_sethas(msg, 4);
  UPB_FIELD_AT(msg, bool, UPB_SIZE(4, 4)) = value;
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_MessageOptions_mutable_uninterpreted_option(google_protobuf_MessageOptions *msg, size_t *len) {
  return (google_protobuf_UninterpretedOption**)_upb_array_mutable_accessor(msg, UPB_SIZE(8, 8), len);
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_MessageOptions_resize_uninterpreted_option(google_protobuf_MessageOptions *msg, size_t len, upb_arena *arena) {
  return (google_protobuf_UninterpretedOption**)_upb_array_resize_accessor(msg, UPB_SIZE(8, 8), len, UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, arena);
}
UPB_INLINE struct google_protobuf_UninterpretedOption* google_protobuf_MessageOptions_add_uninterpreted_option(google_protobuf_MessageOptions *msg, upb_arena *arena) {
  struct google_protobuf_UninterpretedOption* sub = (struct google_protobuf_UninterpretedOption*)upb_msg_new(&google_protobuf_UninterpretedOption_msginit, arena);
  bool ok = _upb_array_append_accessor(
      msg, UPB_SIZE(8, 8), UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, &sub, arena);
  if (!ok) return NULL;
  return sub;
}

/* google.protobuf.FieldOptions */

UPB_INLINE google_protobuf_FieldOptions *google_protobuf_FieldOptions_new(upb_arena *arena) {
  return (google_protobuf_FieldOptions *)upb_msg_new(&google_protobuf_FieldOptions_msginit, arena);
}
UPB_INLINE google_protobuf_FieldOptions *google_protobuf_FieldOptions_parse(const char *buf, size_t size,
                        upb_arena *arena) {
  google_protobuf_FieldOptions *ret = google_protobuf_FieldOptions_new(arena);
  return (ret && upb_decode(buf, size, ret, &google_protobuf_FieldOptions_msginit, arena)) ? ret : NULL;
}
UPB_INLINE char *google_protobuf_FieldOptions_serialize(const google_protobuf_FieldOptions *msg, upb_arena *arena, size_t *len) {
  return upb_encode(msg, &google_protobuf_FieldOptions_msginit, arena, len);
}

UPB_INLINE bool google_protobuf_FieldOptions_has_ctype(const google_protobuf_FieldOptions *msg) { return _upb_has_field(msg, 1); }
UPB_INLINE int32_t google_protobuf_FieldOptions_ctype(const google_protobuf_FieldOptions *msg) { return UPB_FIELD_AT(msg, int32_t, UPB_SIZE(8, 8)); }
UPB_INLINE bool google_protobuf_FieldOptions_has_packed(const google_protobuf_FieldOptions *msg) { return _upb_has_field(msg, 3); }
UPB_INLINE bool google_protobuf_FieldOptions_packed(const google_protobuf_FieldOptions *msg) { return UPB_FIELD_AT(msg, bool, UPB_SIZE(24, 24)); }
UPB_INLINE bool google_protobuf_FieldOptions_has_deprecated(const google_protobuf_FieldOptions *msg) { return _upb_has_field(msg, 4); }
UPB_INLINE bool google_protobuf_FieldOptions_deprecated(const google_protobuf_FieldOptions *msg) { return UPB_FIELD_AT(msg, bool, UPB_SIZE(25, 25)); }
UPB_INLINE bool google_protobuf_FieldOptions_has_lazy(const google_protobuf_FieldOptions *msg) { return _upb_has_field(msg, 5); }
UPB_INLINE bool google_protobuf_FieldOptions_lazy(const google_protobuf_FieldOptions *msg) { return UPB_FIELD_AT(msg, bool, UPB_SIZE(26, 26)); }
UPB_INLINE bool google_protobuf_FieldOptions_has_jstype(const google_protobuf_FieldOptions *msg) { return _upb_has_field(msg, 2); }
UPB_INLINE int32_t google_protobuf_FieldOptions_jstype(const google_protobuf_FieldOptions *msg) { return UPB_FIELD_AT(msg, int32_t, UPB_SIZE(16, 16)); }
UPB_INLINE bool google_protobuf_FieldOptions_has_weak(const google_protobuf_FieldOptions *msg) { return _upb_has_field(msg, 6); }
UPB_INLINE bool google_protobuf_FieldOptions_weak(const google_protobuf_FieldOptions *msg) { return UPB_FIELD_AT(msg, bool, UPB_SIZE(27, 27)); }
UPB_INLINE const google_protobuf_UninterpretedOption* const* google_protobuf_FieldOptions_uninterpreted_option(const google_protobuf_FieldOptions *msg, size_t *len) { return (const google_protobuf_UninterpretedOption* const*)_upb_array_accessor(msg, UPB_SIZE(28, 32), len); }

UPB_INLINE void google_protobuf_FieldOptions_set_ctype(google_protobuf_FieldOptions *msg, int32_t value) {
  _upb_sethas(msg, 1);
  UPB_FIELD_AT(msg, int32_t, UPB_SIZE(8, 8)) = value;
}
UPB_INLINE void google_protobuf_FieldOptions_set_packed(google_protobuf_FieldOptions *msg, bool value) {
  _upb_sethas(msg, 3);
  UPB_FIELD_AT(msg, bool, UPB_SIZE(24, 24)) = value;
}
UPB_INLINE void google_protobuf_FieldOptions_set_deprecated(google_protobuf_FieldOptions *msg, bool value) {
  _upb_sethas(msg, 4);
  UPB_FIELD_AT(msg, bool, UPB_SIZE(25, 25)) = value;
}
UPB_INLINE void google_protobuf_FieldOptions_set_lazy(google_protobuf_FieldOptions *msg, bool value) {
  _upb_sethas(msg, 5);
  UPB_FIELD_AT(msg, bool, UPB_SIZE(26, 26)) = value;
}
UPB_INLINE void google_protobuf_FieldOptions_set_jstype(google_protobuf_FieldOptions *msg, int32_t value) {
  _upb_sethas(msg, 2);
  UPB_FIELD_AT(msg, int32_t, UPB_SIZE(16, 16)) = value;
}
UPB_INLINE void google_protobuf_FieldOptions_set_weak(google_protobuf_FieldOptions *msg, bool value) {
  _upb_sethas(msg, 6);
  UPB_FIELD_AT(msg, bool, UPB_SIZE(27, 27)) = value;
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_FieldOptions_mutable_uninterpreted_option(google_protobuf_FieldOptions *msg, size_t *len) {
  return (google_protobuf_UninterpretedOption**)_upb_array_mutable_accessor(msg, UPB_SIZE(28, 32), len);
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_FieldOptions_resize_uninterpreted_option(google_protobuf_FieldOptions *msg, size_t len, upb_arena *arena) {
  return (google_protobuf_UninterpretedOption**)_upb_array_resize_accessor(msg, UPB_SIZE(28, 32), len, UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, arena);
}
UPB_INLINE struct google_protobuf_UninterpretedOption* google_protobuf_FieldOptions_add_uninterpreted_option(google_protobuf_FieldOptions *msg, upb_arena *arena) {
  struct google_protobuf_UninterpretedOption* sub = (struct google_protobuf_UninterpretedOption*)upb_msg_new(&google_protobuf_UninterpretedOption_msginit, arena);
  bool ok = _upb_array_append_accessor(
      msg, UPB_SIZE(28, 32), UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, &sub, arena);
  if (!ok) return NULL;
  return sub;
}

/* google.protobuf.OneofOptions */

UPB_INLINE google_protobuf_OneofOptions *google_protobuf_OneofOptions_new(upb_arena *arena) {
  return (google_protobuf_OneofOptions *)upb_msg_new(&google_protobuf_OneofOptions_msginit, arena);
}
UPB_INLINE google_protobuf_OneofOptions *google_protobuf_OneofOptions_parse(const char *buf, size_t size,
                        upb_arena *arena) {
  google_protobuf_OneofOptions *ret = google_protobuf_OneofOptions_new(arena);
  return (ret && upb_decode(buf, size, ret, &google_protobuf_OneofOptions_msginit, arena)) ? ret : NULL;
}
UPB_INLINE char *google_protobuf_OneofOptions_serialize(const google_protobuf_OneofOptions *msg, upb_arena *arena, size_t *len) {
  return upb_encode(msg, &google_protobuf_OneofOptions_msginit, arena, len);
}

UPB_INLINE const google_protobuf_UninterpretedOption* const* google_protobuf_OneofOptions_uninterpreted_option(const google_protobuf_OneofOptions *msg, size_t *len) { return (const google_protobuf_UninterpretedOption* const*)_upb_array_accessor(msg, UPB_SIZE(0, 0), len); }

UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_OneofOptions_mutable_uninterpreted_option(google_protobuf_OneofOptions *msg, size_t *len) {
  return (google_protobuf_UninterpretedOption**)_upb_array_mutable_accessor(msg, UPB_SIZE(0, 0), len);
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_OneofOptions_resize_uninterpreted_option(google_protobuf_OneofOptions *msg, size_t len, upb_arena *arena) {
  return (google_protobuf_UninterpretedOption**)_upb_array_resize_accessor(msg, UPB_SIZE(0, 0), len, UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, arena);
}
UPB_INLINE struct google_protobuf_UninterpretedOption* google_protobuf_OneofOptions_add_uninterpreted_option(google_protobuf_OneofOptions *msg, upb_arena *arena) {
  struct google_protobuf_UninterpretedOption* sub = (struct google_protobuf_UninterpretedOption*)upb_msg_new(&google_protobuf_UninterpretedOption_msginit, arena);
  bool ok = _upb_array_append_accessor(
      msg, UPB_SIZE(0, 0), UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, &sub, arena);
  if (!ok) return NULL;
  return sub;
}

/* google.protobuf.EnumOptions */

UPB_INLINE google_protobuf_EnumOptions *google_protobuf_EnumOptions_new(upb_arena *arena) {
  return (google_protobuf_EnumOptions *)upb_msg_new(&google_protobuf_EnumOptions_msginit, arena);
}
UPB_INLINE google_protobuf_EnumOptions *google_protobuf_EnumOptions_parse(const char *buf, size_t size,
                        upb_arena *arena) {
  google_protobuf_EnumOptions *ret = google_protobuf_EnumOptions_new(arena);
  return (ret && upb_decode(buf, size, ret, &google_protobuf_EnumOptions_msginit, arena)) ? ret : NULL;
}
UPB_INLINE char *google_protobuf_EnumOptions_serialize(const google_protobuf_EnumOptions *msg, upb_arena *arena, size_t *len) {
  return upb_encode(msg, &google_protobuf_EnumOptions_msginit, arena, len);
}

UPB_INLINE bool google_protobuf_EnumOptions_has_allow_alias(const google_protobuf_EnumOptions *msg) { return _upb_has_field(msg, 1); }
UPB_INLINE bool google_protobuf_EnumOptions_allow_alias(const google_protobuf_EnumOptions *msg) { return UPB_FIELD_AT(msg, bool, UPB_SIZE(1, 1)); }
UPB_INLINE bool google_protobuf_EnumOptions_has_deprecated(const google_protobuf_EnumOptions *msg) { return _upb_has_field(msg, 2); }
UPB_INLINE bool google_protobuf_EnumOptions_deprecated(const google_protobuf_EnumOptions *msg) { return UPB_FIELD_AT(msg, bool, UPB_SIZE(2, 2)); }
UPB_INLINE const google_protobuf_UninterpretedOption* const* google_protobuf_EnumOptions_uninterpreted_option(const google_protobuf_EnumOptions *msg, size_t *len) { return (const google_protobuf_UninterpretedOption* const*)_upb_array_accessor(msg, UPB_SIZE(4, 8), len); }

UPB_INLINE void google_protobuf_EnumOptions_set_allow_alias(google_protobuf_EnumOptions *msg, bool value) {
  _upb_sethas(msg, 1);
  UPB_FIELD_AT(msg, bool, UPB_SIZE(1, 1)) = value;
}
UPB_INLINE void google_protobuf_EnumOptions_set_deprecated(google_protobuf_EnumOptions *msg, bool value) {
  _upb_sethas(msg, 2);
  UPB_FIELD_AT(msg, bool, UPB_SIZE(2, 2)) = value;
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_EnumOptions_mutable_uninterpreted_option(google_protobuf_EnumOptions *msg, size_t *len) {
  return (google_protobuf_UninterpretedOption**)_upb_array_mutable_accessor(msg, UPB_SIZE(4, 8), len);
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_EnumOptions_resize_uninterpreted_option(google_protobuf_EnumOptions *msg, size_t len, upb_arena *arena) {
  return (google_protobuf_UninterpretedOption**)_upb_array_resize_accessor(msg, UPB_SIZE(4, 8), len, UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, arena);
}
UPB_INLINE struct google_protobuf_UninterpretedOption* google_protobuf_EnumOptions_add_uninterpreted_option(google_protobuf_EnumOptions *msg, upb_arena *arena) {
  struct google_protobuf_UninterpretedOption* sub = (struct google_protobuf_UninterpretedOption*)upb_msg_new(&google_protobuf_UninterpretedOption_msginit, arena);
  bool ok = _upb_array_append_accessor(
      msg, UPB_SIZE(4, 8), UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, &sub, arena);
  if (!ok) return NULL;
  return sub;
}

/* google.protobuf.EnumValueOptions */

UPB_INLINE google_protobuf_EnumValueOptions *google_protobuf_EnumValueOptions_new(upb_arena *arena) {
  return (google_protobuf_EnumValueOptions *)upb_msg_new(&google_protobuf_EnumValueOptions_msginit, arena);
}
UPB_INLINE google_protobuf_EnumValueOptions *google_protobuf_EnumValueOptions_parse(const char *buf, size_t size,
                        upb_arena *arena) {
  google_protobuf_EnumValueOptions *ret = google_protobuf_EnumValueOptions_new(arena);
  return (ret && upb_decode(buf, size, ret, &google_protobuf_EnumValueOptions_msginit, arena)) ? ret : NULL;
}
UPB_INLINE char *google_protobuf_EnumValueOptions_serialize(const google_protobuf_EnumValueOptions *msg, upb_arena *arena, size_t *len) {
  return upb_encode(msg, &google_protobuf_EnumValueOptions_msginit, arena, len);
}

UPB_INLINE bool google_protobuf_EnumValueOptions_has_deprecated(const google_protobuf_EnumValueOptions *msg) { return _upb_has_field(msg, 1); }
UPB_INLINE bool google_protobuf_EnumValueOptions_deprecated(const google_protobuf_EnumValueOptions *msg) { return UPB_FIELD_AT(msg, bool, UPB_SIZE(1, 1)); }
UPB_INLINE const google_protobuf_UninterpretedOption* const* google_protobuf_EnumValueOptions_uninterpreted_option(const google_protobuf_EnumValueOptions *msg, size_t *len) { return (const google_protobuf_UninterpretedOption* const*)_upb_array_accessor(msg, UPB_SIZE(4, 8), len); }

UPB_INLINE void google_protobuf_EnumValueOptions_set_deprecated(google_protobuf_EnumValueOptions *msg, bool value) {
  _upb_sethas(msg, 1);
  UPB_FIELD_AT(msg, bool, UPB_SIZE(1, 1)) = value;
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_EnumValueOptions_mutable_uninterpreted_option(google_protobuf_EnumValueOptions *msg, size_t *len) {
  return (google_protobuf_UninterpretedOption**)_upb_array_mutable_accessor(msg, UPB_SIZE(4, 8), len);
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_EnumValueOptions_resize_uninterpreted_option(google_protobuf_EnumValueOptions *msg, size_t len, upb_arena *arena) {
  return (google_protobuf_UninterpretedOption**)_upb_array_resize_accessor(msg, UPB_SIZE(4, 8), len, UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, arena);
}
UPB_INLINE struct google_protobuf_UninterpretedOption* google_protobuf_EnumValueOptions_add_uninterpreted_option(google_protobuf_EnumValueOptions *msg, upb_arena *arena) {
  struct google_protobuf_UninterpretedOption* sub = (struct google_protobuf_UninterpretedOption*)upb_msg_new(&google_protobuf_UninterpretedOption_msginit, arena);
  bool ok = _upb_array_append_accessor(
      msg, UPB_SIZE(4, 8), UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, &sub, arena);
  if (!ok) return NULL;
  return sub;
}

/* google.protobuf.ServiceOptions */

UPB_INLINE google_protobuf_ServiceOptions *google_protobuf_ServiceOptions_new(upb_arena *arena) {
  return (google_protobuf_ServiceOptions *)upb_msg_new(&google_protobuf_ServiceOptions_msginit, arena);
}
UPB_INLINE google_protobuf_ServiceOptions *google_protobuf_ServiceOptions_parse(const char *buf, size_t size,
                        upb_arena *arena) {
  google_protobuf_ServiceOptions *ret = google_protobuf_ServiceOptions_new(arena);
  return (ret && upb_decode(buf, size, ret, &google_protobuf_ServiceOptions_msginit, arena)) ? ret : NULL;
}
UPB_INLINE char *google_protobuf_ServiceOptions_serialize(const google_protobuf_ServiceOptions *msg, upb_arena *arena, size_t *len) {
  return upb_encode(msg, &google_protobuf_ServiceOptions_msginit, arena, len);
}

UPB_INLINE bool google_protobuf_ServiceOptions_has_deprecated(const google_protobuf_ServiceOptions *msg) { return _upb_has_field(msg, 1); }
UPB_INLINE bool google_protobuf_ServiceOptions_deprecated(const google_protobuf_ServiceOptions *msg) { return UPB_FIELD_AT(msg, bool, UPB_SIZE(1, 1)); }
UPB_INLINE const google_protobuf_UninterpretedOption* const* google_protobuf_ServiceOptions_uninterpreted_option(const google_protobuf_ServiceOptions *msg, size_t *len) { return (const google_protobuf_UninterpretedOption* const*)_upb_array_accessor(msg, UPB_SIZE(4, 8), len); }

UPB_INLINE void google_protobuf_ServiceOptions_set_deprecated(google_protobuf_ServiceOptions *msg, bool value) {
  _upb_sethas(msg, 1);
  UPB_FIELD_AT(msg, bool, UPB_SIZE(1, 1)) = value;
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_ServiceOptions_mutable_uninterpreted_option(google_protobuf_ServiceOptions *msg, size_t *len) {
  return (google_protobuf_UninterpretedOption**)_upb_array_mutable_accessor(msg, UPB_SIZE(4, 8), len);
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_ServiceOptions_resize_uninterpreted_option(google_protobuf_ServiceOptions *msg, size_t len, upb_arena *arena) {
  return (google_protobuf_UninterpretedOption**)_upb_array_resize_accessor(msg, UPB_SIZE(4, 8), len, UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, arena);
}
UPB_INLINE struct google_protobuf_UninterpretedOption* google_protobuf_ServiceOptions_add_uninterpreted_option(google_protobuf_ServiceOptions *msg, upb_arena *arena) {
  struct google_protobuf_UninterpretedOption* sub = (struct google_protobuf_UninterpretedOption*)upb_msg_new(&google_protobuf_UninterpretedOption_msginit, arena);
  bool ok = _upb_array_append_accessor(
      msg, UPB_SIZE(4, 8), UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, &sub, arena);
  if (!ok) return NULL;
  return sub;
}

/* google.protobuf.MethodOptions */

UPB_INLINE google_protobuf_MethodOptions *google_protobuf_MethodOptions_new(upb_arena *arena) {
  return (google_protobuf_MethodOptions *)upb_msg_new(&google_protobuf_MethodOptions_msginit, arena);
}
UPB_INLINE google_protobuf_MethodOptions *google_protobuf_MethodOptions_parse(const char *buf, size_t size,
                        upb_arena *arena) {
  google_protobuf_MethodOptions *ret = google_protobuf_MethodOptions_new(arena);
  return (ret && upb_decode(buf, size, ret, &google_protobuf_MethodOptions_msginit, arena)) ? ret : NULL;
}
UPB_INLINE char *google_protobuf_MethodOptions_serialize(const google_protobuf_MethodOptions *msg, upb_arena *arena, size_t *len) {
  return upb_encode(msg, &google_protobuf_MethodOptions_msginit, arena, len);
}

UPB_INLINE bool google_protobuf_MethodOptions_has_deprecated(const google_protobuf_MethodOptions *msg) { return _upb_has_field(msg, 2); }
UPB_INLINE bool google_protobuf_MethodOptions_deprecated(const google_protobuf_MethodOptions *msg) { return UPB_FIELD_AT(msg, bool, UPB_SIZE(16, 16)); }
UPB_INLINE bool google_protobuf_MethodOptions_has_idempotency_level(const google_protobuf_MethodOptions *msg) { return _upb_has_field(msg, 1); }
UPB_INLINE int32_t google_protobuf_MethodOptions_idempotency_level(const google_protobuf_MethodOptions *msg) { return UPB_FIELD_AT(msg, int32_t, UPB_SIZE(8, 8)); }
UPB_INLINE const google_protobuf_UninterpretedOption* const* google_protobuf_MethodOptions_uninterpreted_option(const google_protobuf_MethodOptions *msg, size_t *len) { return (const google_protobuf_UninterpretedOption* const*)_upb_array_accessor(msg, UPB_SIZE(20, 24), len); }

UPB_INLINE void google_protobuf_MethodOptions_set_deprecated(google_protobuf_MethodOptions *msg, bool value) {
  _upb_sethas(msg, 2);
  UPB_FIELD_AT(msg, bool, UPB_SIZE(16, 16)) = value;
}
UPB_INLINE void google_protobuf_MethodOptions_set_idempotency_level(google_protobuf_MethodOptions *msg, int32_t value) {
  _upb_sethas(msg, 1);
  UPB_FIELD_AT(msg, int32_t, UPB_SIZE(8, 8)) = value;
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_MethodOptions_mutable_uninterpreted_option(google_protobuf_MethodOptions *msg, size_t *len) {
  return (google_protobuf_UninterpretedOption**)_upb_array_mutable_accessor(msg, UPB_SIZE(20, 24), len);
}
UPB_INLINE google_protobuf_UninterpretedOption** google_protobuf_MethodOptions_resize_uninterpreted_option(google_protobuf_MethodOptions *msg, size_t len, upb_arena *arena) {
  return (google_protobuf_UninterpretedOption**)_upb_array_resize_accessor(msg, UPB_SIZE(20, 24), len, UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, arena);
}
UPB_INLINE struct google_protobuf_UninterpretedOption* google_protobuf_MethodOptions_add_uninterpreted_option(google_protobuf_MethodOptions *msg, upb_arena *arena) {
  struct google_protobuf_UninterpretedOption* sub = (struct google_protobuf_UninterpretedOption*)upb_msg_new(&google_protobuf_UninterpretedOption_msginit, arena);
  bool ok = _upb_array_append_accessor(
      msg, UPB_SIZE(20, 24), UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, &sub, arena);
  if (!ok) return NULL;
  return sub;
}

/* google.protobuf.UninterpretedOption */

UPB_INLINE google_protobuf_UninterpretedOption *google_protobuf_UninterpretedOption_new(upb_arena *arena) {
  return (google_protobuf_UninterpretedOption *)upb_msg_new(&google_protobuf_UninterpretedOption_msginit, arena);
}
UPB_INLINE google_protobuf_UninterpretedOption *google_protobuf_UninterpretedOption_parse(const char *buf, size_t size,
                        upb_arena *arena) {
  google_protobuf_UninterpretedOption *ret = google_protobuf_UninterpretedOption_new(arena);
  return (ret && upb_decode(buf, size, ret, &google_protobuf_UninterpretedOption_msginit, arena)) ? ret : NULL;
}
UPB_INLINE char *google_protobuf_UninterpretedOption_serialize(const google_protobuf_UninterpretedOption *msg, upb_arena *arena, size_t *len) {
  return upb_encode(msg, &google_protobuf_UninterpretedOption_msginit, arena, len);
}

UPB_INLINE const google_protobuf_UninterpretedOption_NamePart* const* google_protobuf_UninterpretedOption_name(const google_protobuf_UninterpretedOption *msg, size_t *len) { return (const google_protobuf_UninterpretedOption_NamePart* const*)_upb_array_accessor(msg, UPB_SIZE(56, 80), len); }
UPB_INLINE bool google_protobuf_UninterpretedOption_has_identifier_value(const google_protobuf_UninterpretedOption *msg) { return _upb_has_field(msg, 4); }
UPB_INLINE upb_strview google_protobuf_UninterpretedOption_identifier_value(const google_protobuf_UninterpretedOption *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(32, 32)); }
UPB_INLINE bool google_protobuf_UninterpretedOption_has_positive_int_value(const google_protobuf_UninterpretedOption *msg) { return _upb_has_field(msg, 1); }
UPB_INLINE uint64_t google_protobuf_UninterpretedOption_positive_int_value(const google_protobuf_UninterpretedOption *msg) { return UPB_FIELD_AT(msg, uint64_t, UPB_SIZE(8, 8)); }
UPB_INLINE bool google_protobuf_UninterpretedOption_has_negative_int_value(const google_protobuf_UninterpretedOption *msg) { return _upb_has_field(msg, 2); }
UPB_INLINE int64_t google_protobuf_UninterpretedOption_negative_int_value(const google_protobuf_UninterpretedOption *msg) { return UPB_FIELD_AT(msg, int64_t, UPB_SIZE(16, 16)); }
UPB_INLINE bool google_protobuf_UninterpretedOption_has_double_value(const google_protobuf_UninterpretedOption *msg) { return _upb_has_field(msg, 3); }
UPB_INLINE double google_protobuf_UninterpretedOption_double_value(const google_protobuf_UninterpretedOption *msg) { return UPB_FIELD_AT(msg, double, UPB_SIZE(24, 24)); }
UPB_INLINE bool google_protobuf_UninterpretedOption_has_string_value(const google_protobuf_UninterpretedOption *msg) { return _upb_has_field(msg, 5); }
UPB_INLINE upb_strview google_protobuf_UninterpretedOption_string_value(const google_protobuf_UninterpretedOption *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(40, 48)); }
UPB_INLINE bool google_protobuf_UninterpretedOption_has_aggregate_value(const google_protobuf_UninterpretedOption *msg) { return _upb_has_field(msg, 6); }
UPB_INLINE upb_strview google_protobuf_UninterpretedOption_aggregate_value(const google_protobuf_UninterpretedOption *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(48, 64)); }

UPB_INLINE google_protobuf_UninterpretedOption_NamePart** google_protobuf_UninterpretedOption_mutable_name(google_protobuf_UninterpretedOption *msg, size_t *len) {
  return (google_protobuf_UninterpretedOption_NamePart**)_upb_array_mutable_accessor(msg, UPB_SIZE(56, 80), len);
}
UPB_INLINE google_protobuf_UninterpretedOption_NamePart** google_protobuf_UninterpretedOption_resize_name(google_protobuf_UninterpretedOption *msg, size_t len, upb_arena *arena) {
  return (google_protobuf_UninterpretedOption_NamePart**)_upb_array_resize_accessor(msg, UPB_SIZE(56, 80), len, UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, arena);
}
UPB_INLINE struct google_protobuf_UninterpretedOption_NamePart* google_protobuf_UninterpretedOption_add_name(google_protobuf_UninterpretedOption *msg, upb_arena *arena) {
  struct google_protobuf_UninterpretedOption_NamePart* sub = (struct google_protobuf_UninterpretedOption_NamePart*)upb_msg_new(&google_protobuf_UninterpretedOption_NamePart_msginit, arena);
  bool ok = _upb_array_append_accessor(
      msg, UPB_SIZE(56, 80), UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, &sub, arena);
  if (!ok) return NULL;
  return sub;
}
UPB_INLINE void google_protobuf_UninterpretedOption_set_identifier_value(google_protobuf_UninterpretedOption *msg, upb_strview value) {
  _upb_sethas(msg, 4);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(32, 32)) = value;
}
UPB_INLINE void google_protobuf_UninterpretedOption_set_positive_int_value(google_protobuf_UninterpretedOption *msg, uint64_t value) {
  _upb_sethas(msg, 1);
  UPB_FIELD_AT(msg, uint64_t, UPB_SIZE(8, 8)) = value;
}
UPB_INLINE void google_protobuf_UninterpretedOption_set_negative_int_value(google_protobuf_UninterpretedOption *msg, int64_t value) {
  _upb_sethas(msg, 2);
  UPB_FIELD_AT(msg, int64_t, UPB_SIZE(16, 16)) = value;
}
UPB_INLINE void google_protobuf_UninterpretedOption_set_double_value(google_protobuf_UninterpretedOption *msg, double value) {
  _upb_sethas(msg, 3);
  UPB_FIELD_AT(msg, double, UPB_SIZE(24, 24)) = value;
}
UPB_INLINE void google_protobuf_UninterpretedOption_set_string_value(google_protobuf_UninterpretedOption *msg, upb_strview value) {
  _upb_sethas(msg, 5);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(40, 48)) = value;
}
UPB_INLINE void google_protobuf_UninterpretedOption_set_aggregate_value(google_protobuf_UninterpretedOption *msg, upb_strview value) {
  _upb_sethas(msg, 6);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(48, 64)) = value;
}

/* google.protobuf.UninterpretedOption.NamePart */

UPB_INLINE google_protobuf_UninterpretedOption_NamePart *google_protobuf_UninterpretedOption_NamePart_new(upb_arena *arena) {
  return (google_protobuf_UninterpretedOption_NamePart *)upb_msg_new(&google_protobuf_UninterpretedOption_NamePart_msginit, arena);
}
UPB_INLINE google_protobuf_UninterpretedOption_NamePart *google_protobuf_UninterpretedOption_NamePart_parse(const char *buf, size_t size,
                        upb_arena *arena) {
  google_protobuf_UninterpretedOption_NamePart *ret = google_protobuf_UninterpretedOption_NamePart_new(arena);
  return (ret && upb_decode(buf, size, ret, &google_protobuf_UninterpretedOption_NamePart_msginit, arena)) ? ret : NULL;
}
UPB_INLINE char *google_protobuf_UninterpretedOption_NamePart_serialize(const google_protobuf_UninterpretedOption_NamePart *msg, upb_arena *arena, size_t *len) {
  return upb_encode(msg, &google_protobuf_UninterpretedOption_NamePart_msginit, arena, len);
}

UPB_INLINE bool google_protobuf_UninterpretedOption_NamePart_has_name_part(const google_protobuf_UninterpretedOption_NamePart *msg) { return _upb_has_field(msg, 2); }
UPB_INLINE upb_strview google_protobuf_UninterpretedOption_NamePart_name_part(const google_protobuf_UninterpretedOption_NamePart *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(4, 8)); }
UPB_INLINE bool google_protobuf_UninterpretedOption_NamePart_has_is_extension(const google_protobuf_UninterpretedOption_NamePart *msg) { return _upb_has_field(msg, 1); }
UPB_INLINE bool google_protobuf_UninterpretedOption_NamePart_is_extension(const google_protobuf_UninterpretedOption_NamePart *msg) { return UPB_FIELD_AT(msg, bool, UPB_SIZE(1, 1)); }

UPB_INLINE void google_protobuf_UninterpretedOption_NamePart_set_name_part(google_protobuf_UninterpretedOption_NamePart *msg, upb_strview value) {
  _upb_sethas(msg, 2);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(4, 8)) = value;
}
UPB_INLINE void google_protobuf_UninterpretedOption_NamePart_set_is_extension(google_protobuf_UninterpretedOption_NamePart *msg, bool value) {
  _upb_sethas(msg, 1);
  UPB_FIELD_AT(msg, bool, UPB_SIZE(1, 1)) = value;
}

/* google.protobuf.SourceCodeInfo */

UPB_INLINE google_protobuf_SourceCodeInfo *google_protobuf_SourceCodeInfo_new(upb_arena *arena) {
  return (google_protobuf_SourceCodeInfo *)upb_msg_new(&google_protobuf_SourceCodeInfo_msginit, arena);
}
UPB_INLINE google_protobuf_SourceCodeInfo *google_protobuf_SourceCodeInfo_parse(const char *buf, size_t size,
                        upb_arena *arena) {
  google_protobuf_SourceCodeInfo *ret = google_protobuf_SourceCodeInfo_new(arena);
  return (ret && upb_decode(buf, size, ret, &google_protobuf_SourceCodeInfo_msginit, arena)) ? ret : NULL;
}
UPB_INLINE char *google_protobuf_SourceCodeInfo_serialize(const google_protobuf_SourceCodeInfo *msg, upb_arena *arena, size_t *len) {
  return upb_encode(msg, &google_protobuf_SourceCodeInfo_msginit, arena, len);
}

UPB_INLINE const google_protobuf_SourceCodeInfo_Location* const* google_protobuf_SourceCodeInfo_location(const google_protobuf_SourceCodeInfo *msg, size_t *len) { return (const google_protobuf_SourceCodeInfo_Location* const*)_upb_array_accessor(msg, UPB_SIZE(0, 0), len); }

UPB_INLINE google_protobuf_SourceCodeInfo_Location** google_protobuf_SourceCodeInfo_mutable_location(google_protobuf_SourceCodeInfo *msg, size_t *len) {
  return (google_protobuf_SourceCodeInfo_Location**)_upb_array_mutable_accessor(msg, UPB_SIZE(0, 0), len);
}
UPB_INLINE google_protobuf_SourceCodeInfo_Location** google_protobuf_SourceCodeInfo_resize_location(google_protobuf_SourceCodeInfo *msg, size_t len, upb_arena *arena) {
  return (google_protobuf_SourceCodeInfo_Location**)_upb_array_resize_accessor(msg, UPB_SIZE(0, 0), len, UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, arena);
}
UPB_INLINE struct google_protobuf_SourceCodeInfo_Location* google_protobuf_SourceCodeInfo_add_location(google_protobuf_SourceCodeInfo *msg, upb_arena *arena) {
  struct google_protobuf_SourceCodeInfo_Location* sub = (struct google_protobuf_SourceCodeInfo_Location*)upb_msg_new(&google_protobuf_SourceCodeInfo_Location_msginit, arena);
  bool ok = _upb_array_append_accessor(
      msg, UPB_SIZE(0, 0), UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, &sub, arena);
  if (!ok) return NULL;
  return sub;
}

/* google.protobuf.SourceCodeInfo.Location */

UPB_INLINE google_protobuf_SourceCodeInfo_Location *google_protobuf_SourceCodeInfo_Location_new(upb_arena *arena) {
  return (google_protobuf_SourceCodeInfo_Location *)upb_msg_new(&google_protobuf_SourceCodeInfo_Location_msginit, arena);
}
UPB_INLINE google_protobuf_SourceCodeInfo_Location *google_protobuf_SourceCodeInfo_Location_parse(const char *buf, size_t size,
                        upb_arena *arena) {
  google_protobuf_SourceCodeInfo_Location *ret = google_protobuf_SourceCodeInfo_Location_new(arena);
  return (ret && upb_decode(buf, size, ret, &google_protobuf_SourceCodeInfo_Location_msginit, arena)) ? ret : NULL;
}
UPB_INLINE char *google_protobuf_SourceCodeInfo_Location_serialize(const google_protobuf_SourceCodeInfo_Location *msg, upb_arena *arena, size_t *len) {
  return upb_encode(msg, &google_protobuf_SourceCodeInfo_Location_msginit, arena, len);
}

UPB_INLINE int32_t const* google_protobuf_SourceCodeInfo_Location_path(const google_protobuf_SourceCodeInfo_Location *msg, size_t *len) { return (int32_t const*)_upb_array_accessor(msg, UPB_SIZE(20, 40), len); }
UPB_INLINE int32_t const* google_protobuf_SourceCodeInfo_Location_span(const google_protobuf_SourceCodeInfo_Location *msg, size_t *len) { return (int32_t const*)_upb_array_accessor(msg, UPB_SIZE(24, 48), len); }
UPB_INLINE bool google_protobuf_SourceCodeInfo_Location_has_leading_comments(const google_protobuf_SourceCodeInfo_Location *msg) { return _upb_has_field(msg, 1); }
UPB_INLINE upb_strview google_protobuf_SourceCodeInfo_Location_leading_comments(const google_protobuf_SourceCodeInfo_Location *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(4, 8)); }
UPB_INLINE bool google_protobuf_SourceCodeInfo_Location_has_trailing_comments(const google_protobuf_SourceCodeInfo_Location *msg) { return _upb_has_field(msg, 2); }
UPB_INLINE upb_strview google_protobuf_SourceCodeInfo_Location_trailing_comments(const google_protobuf_SourceCodeInfo_Location *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(12, 24)); }
UPB_INLINE upb_strview const* google_protobuf_SourceCodeInfo_Location_leading_detached_comments(const google_protobuf_SourceCodeInfo_Location *msg, size_t *len) { return (upb_strview const*)_upb_array_accessor(msg, UPB_SIZE(28, 56), len); }

UPB_INLINE int32_t* google_protobuf_SourceCodeInfo_Location_mutable_path(google_protobuf_SourceCodeInfo_Location *msg, size_t *len) {
  return (int32_t*)_upb_array_mutable_accessor(msg, UPB_SIZE(20, 40), len);
}
UPB_INLINE int32_t* google_protobuf_SourceCodeInfo_Location_resize_path(google_protobuf_SourceCodeInfo_Location *msg, size_t len, upb_arena *arena) {
  return (int32_t*)_upb_array_resize_accessor(msg, UPB_SIZE(20, 40), len, UPB_SIZE(4, 4), UPB_TYPE_INT32, arena);
}
UPB_INLINE bool google_protobuf_SourceCodeInfo_Location_add_path(google_protobuf_SourceCodeInfo_Location *msg, int32_t val, upb_arena *arena) {
  return _upb_array_append_accessor(
      msg, UPB_SIZE(20, 40), UPB_SIZE(4, 4), UPB_TYPE_INT32, &val, arena);
}
UPB_INLINE int32_t* google_protobuf_SourceCodeInfo_Location_mutable_span(google_protobuf_SourceCodeInfo_Location *msg, size_t *len) {
  return (int32_t*)_upb_array_mutable_accessor(msg, UPB_SIZE(24, 48), len);
}
UPB_INLINE int32_t* google_protobuf_SourceCodeInfo_Location_resize_span(google_protobuf_SourceCodeInfo_Location *msg, size_t len, upb_arena *arena) {
  return (int32_t*)_upb_array_resize_accessor(msg, UPB_SIZE(24, 48), len, UPB_SIZE(4, 4), UPB_TYPE_INT32, arena);
}
UPB_INLINE bool google_protobuf_SourceCodeInfo_Location_add_span(google_protobuf_SourceCodeInfo_Location *msg, int32_t val, upb_arena *arena) {
  return _upb_array_append_accessor(
      msg, UPB_SIZE(24, 48), UPB_SIZE(4, 4), UPB_TYPE_INT32, &val, arena);
}
UPB_INLINE void google_protobuf_SourceCodeInfo_Location_set_leading_comments(google_protobuf_SourceCodeInfo_Location *msg, upb_strview value) {
  _upb_sethas(msg, 1);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(4, 8)) = value;
}
UPB_INLINE void google_protobuf_SourceCodeInfo_Location_set_trailing_comments(google_protobuf_SourceCodeInfo_Location *msg, upb_strview value) {
  _upb_sethas(msg, 2);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(12, 24)) = value;
}
UPB_INLINE upb_strview* google_protobuf_SourceCodeInfo_Location_mutable_leading_detached_comments(google_protobuf_SourceCodeInfo_Location *msg, size_t *len) {
  return (upb_strview*)_upb_array_mutable_accessor(msg, UPB_SIZE(28, 56), len);
}
UPB_INLINE upb_strview* google_protobuf_SourceCodeInfo_Location_resize_leading_detached_comments(google_protobuf_SourceCodeInfo_Location *msg, size_t len, upb_arena *arena) {
  return (upb_strview*)_upb_array_resize_accessor(msg, UPB_SIZE(28, 56), len, UPB_SIZE(8, 16), UPB_TYPE_STRING, arena);
}
UPB_INLINE bool google_protobuf_SourceCodeInfo_Location_add_leading_detached_comments(google_protobuf_SourceCodeInfo_Location *msg, upb_strview val, upb_arena *arena) {
  return _upb_array_append_accessor(
      msg, UPB_SIZE(28, 56), UPB_SIZE(8, 16), UPB_TYPE_STRING, &val, arena);
}

/* google.protobuf.GeneratedCodeInfo */

UPB_INLINE google_protobuf_GeneratedCodeInfo *google_protobuf_GeneratedCodeInfo_new(upb_arena *arena) {
  return (google_protobuf_GeneratedCodeInfo *)upb_msg_new(&google_protobuf_GeneratedCodeInfo_msginit, arena);
}
UPB_INLINE google_protobuf_GeneratedCodeInfo *google_protobuf_GeneratedCodeInfo_parse(const char *buf, size_t size,
                        upb_arena *arena) {
  google_protobuf_GeneratedCodeInfo *ret = google_protobuf_GeneratedCodeInfo_new(arena);
  return (ret && upb_decode(buf, size, ret, &google_protobuf_GeneratedCodeInfo_msginit, arena)) ? ret : NULL;
}
UPB_INLINE char *google_protobuf_GeneratedCodeInfo_serialize(const google_protobuf_GeneratedCodeInfo *msg, upb_arena *arena, size_t *len) {
  return upb_encode(msg, &google_protobuf_GeneratedCodeInfo_msginit, arena, len);
}

UPB_INLINE const google_protobuf_GeneratedCodeInfo_Annotation* const* google_protobuf_GeneratedCodeInfo_annotation(const google_protobuf_GeneratedCodeInfo *msg, size_t *len) { return (const google_protobuf_GeneratedCodeInfo_Annotation* const*)_upb_array_accessor(msg, UPB_SIZE(0, 0), len); }

UPB_INLINE google_protobuf_GeneratedCodeInfo_Annotation** google_protobuf_GeneratedCodeInfo_mutable_annotation(google_protobuf_GeneratedCodeInfo *msg, size_t *len) {
  return (google_protobuf_GeneratedCodeInfo_Annotation**)_upb_array_mutable_accessor(msg, UPB_SIZE(0, 0), len);
}
UPB_INLINE google_protobuf_GeneratedCodeInfo_Annotation** google_protobuf_GeneratedCodeInfo_resize_annotation(google_protobuf_GeneratedCodeInfo *msg, size_t len, upb_arena *arena) {
  return (google_protobuf_GeneratedCodeInfo_Annotation**)_upb_array_resize_accessor(msg, UPB_SIZE(0, 0), len, UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, arena);
}
UPB_INLINE struct google_protobuf_GeneratedCodeInfo_Annotation* google_protobuf_GeneratedCodeInfo_add_annotation(google_protobuf_GeneratedCodeInfo *msg, upb_arena *arena) {
  struct google_protobuf_GeneratedCodeInfo_Annotation* sub = (struct google_protobuf_GeneratedCodeInfo_Annotation*)upb_msg_new(&google_protobuf_GeneratedCodeInfo_Annotation_msginit, arena);
  bool ok = _upb_array_append_accessor(
      msg, UPB_SIZE(0, 0), UPB_SIZE(4, 8), UPB_TYPE_MESSAGE, &sub, arena);
  if (!ok) return NULL;
  return sub;
}

/* google.protobuf.GeneratedCodeInfo.Annotation */

UPB_INLINE google_protobuf_GeneratedCodeInfo_Annotation *google_protobuf_GeneratedCodeInfo_Annotation_new(upb_arena *arena) {
  return (google_protobuf_GeneratedCodeInfo_Annotation *)upb_msg_new(&google_protobuf_GeneratedCodeInfo_Annotation_msginit, arena);
}
UPB_INLINE google_protobuf_GeneratedCodeInfo_Annotation *google_protobuf_GeneratedCodeInfo_Annotation_parse(const char *buf, size_t size,
                        upb_arena *arena) {
  google_protobuf_GeneratedCodeInfo_Annotation *ret = google_protobuf_GeneratedCodeInfo_Annotation_new(arena);
  return (ret && upb_decode(buf, size, ret, &google_protobuf_GeneratedCodeInfo_Annotation_msginit, arena)) ? ret : NULL;
}
UPB_INLINE char *google_protobuf_GeneratedCodeInfo_Annotation_serialize(const google_protobuf_GeneratedCodeInfo_Annotation *msg, upb_arena *arena, size_t *len) {
  return upb_encode(msg, &google_protobuf_GeneratedCodeInfo_Annotation_msginit, arena, len);
}

UPB_INLINE int32_t const* google_protobuf_GeneratedCodeInfo_Annotation_path(const google_protobuf_GeneratedCodeInfo_Annotation *msg, size_t *len) { return (int32_t const*)_upb_array_accessor(msg, UPB_SIZE(20, 32), len); }
UPB_INLINE bool google_protobuf_GeneratedCodeInfo_Annotation_has_source_file(const google_protobuf_GeneratedCodeInfo_Annotation *msg) { return _upb_has_field(msg, 3); }
UPB_INLINE upb_strview google_protobuf_GeneratedCodeInfo_Annotation_source_file(const google_protobuf_GeneratedCodeInfo_Annotation *msg) { return UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(12, 16)); }
UPB_INLINE bool google_protobuf_GeneratedCodeInfo_Annotation_has_begin(const google_protobuf_GeneratedCodeInfo_Annotation *msg) { return _upb_has_field(msg, 1); }
UPB_INLINE int32_t google_protobuf_GeneratedCodeInfo_Annotation_begin(const google_protobuf_GeneratedCodeInfo_Annotation *msg) { return UPB_FIELD_AT(msg, int32_t, UPB_SIZE(4, 4)); }
UPB_INLINE bool google_protobuf_GeneratedCodeInfo_Annotation_has_end(const google_protobuf_GeneratedCodeInfo_Annotation *msg) { return _upb_has_field(msg, 2); }
UPB_INLINE int32_t google_protobuf_GeneratedCodeInfo_Annotation_end(const google_protobuf_GeneratedCodeInfo_Annotation *msg) { return UPB_FIELD_AT(msg, int32_t, UPB_SIZE(8, 8)); }

UPB_INLINE int32_t* google_protobuf_GeneratedCodeInfo_Annotation_mutable_path(google_protobuf_GeneratedCodeInfo_Annotation *msg, size_t *len) {
  return (int32_t*)_upb_array_mutable_accessor(msg, UPB_SIZE(20, 32), len);
}
UPB_INLINE int32_t* google_protobuf_GeneratedCodeInfo_Annotation_resize_path(google_protobuf_GeneratedCodeInfo_Annotation *msg, size_t len, upb_arena *arena) {
  return (int32_t*)_upb_array_resize_accessor(msg, UPB_SIZE(20, 32), len, UPB_SIZE(4, 4), UPB_TYPE_INT32, arena);
}
UPB_INLINE bool google_protobuf_GeneratedCodeInfo_Annotation_add_path(google_protobuf_GeneratedCodeInfo_Annotation *msg, int32_t val, upb_arena *arena) {
  return _upb_array_append_accessor(
      msg, UPB_SIZE(20, 32), UPB_SIZE(4, 4), UPB_TYPE_INT32, &val, arena);
}
UPB_INLINE void google_protobuf_GeneratedCodeInfo_Annotation_set_source_file(google_protobuf_GeneratedCodeInfo_Annotation *msg, upb_strview value) {
  _upb_sethas(msg, 3);
  UPB_FIELD_AT(msg, upb_strview, UPB_SIZE(12, 16)) = value;
}
UPB_INLINE void google_protobuf_GeneratedCodeInfo_Annotation_set_begin(google_protobuf_GeneratedCodeInfo_Annotation *msg, int32_t value) {
  _upb_sethas(msg, 1);
  UPB_FIELD_AT(msg, int32_t, UPB_SIZE(4, 4)) = value;
}
UPB_INLINE void google_protobuf_GeneratedCodeInfo_Annotation_set_end(google_protobuf_GeneratedCodeInfo_Annotation *msg, int32_t value) {
  _upb_sethas(msg, 2);
  UPB_FIELD_AT(msg, int32_t, UPB_SIZE(8, 8)) = value;
}

#ifdef __cplusplus
}  /* extern "C" */
#endif


#endif  /* GOOGLE_PROTOBUF_DESCRIPTOR_PROTO_UPB_H_ */
/*
** Defs are upb's internal representation of the constructs that can appear
** in a .proto file:
**
** - upb::MessageDefPtr (upb_msgdef): describes a "message" construct.
** - upb::FieldDefPtr (upb_fielddef): describes a message field.
** - upb::FileDefPtr (upb_filedef): describes a .proto file and its defs.
** - upb::EnumDefPtr (upb_enumdef): describes an enum.
** - upb::OneofDefPtr (upb_oneofdef): describes a oneof.
**
** TODO: definitions of services.
**
** This is a mixed C/C++ interface that offers a full API to both languages.
** See the top-level README for more information.
*/

#ifndef UPB_DEF_H_
#define UPB_DEF_H_


#ifdef __cplusplus
#include <cstring>
#include <memory>
#include <string>
#include <vector>

namespace upb {
class EnumDefPtr;
class FieldDefPtr;
class FileDefPtr;
class MessageDefPtr;
class OneofDefPtr;
class SymbolTable;
}
#endif


struct upb_enumdef;
typedef struct upb_enumdef upb_enumdef;
struct upb_fielddef;
typedef struct upb_fielddef upb_fielddef;
struct upb_filedef;
typedef struct upb_filedef upb_filedef;
struct upb_msgdef;
typedef struct upb_msgdef upb_msgdef;
struct upb_oneofdef;
typedef struct upb_oneofdef upb_oneofdef;
struct upb_symtab;
typedef struct upb_symtab upb_symtab;

typedef enum {
  UPB_SYNTAX_PROTO2 = 2,
  UPB_SYNTAX_PROTO3 = 3
} upb_syntax_t;

/* All the different kind of well known type messages. For simplicity of check,
 * number wrappers and string wrappers are grouped together. Make sure the
 * order and merber of these groups are not changed.
 */
typedef enum {
  UPB_WELLKNOWN_UNSPECIFIED,
  UPB_WELLKNOWN_ANY,
  UPB_WELLKNOWN_FIELDMASK,
  UPB_WELLKNOWN_DURATION,
  UPB_WELLKNOWN_TIMESTAMP,
  /* number wrappers */
  UPB_WELLKNOWN_DOUBLEVALUE,
  UPB_WELLKNOWN_FLOATVALUE,
  UPB_WELLKNOWN_INT64VALUE,
  UPB_WELLKNOWN_UINT64VALUE,
  UPB_WELLKNOWN_INT32VALUE,
  UPB_WELLKNOWN_UINT32VALUE,
  /* string wrappers */
  UPB_WELLKNOWN_STRINGVALUE,
  UPB_WELLKNOWN_BYTESVALUE,
  UPB_WELLKNOWN_BOOLVALUE,
  UPB_WELLKNOWN_VALUE,
  UPB_WELLKNOWN_LISTVALUE,
  UPB_WELLKNOWN_STRUCT
} upb_wellknowntype_t;

/* upb_fielddef ***************************************************************/

/* Maximum field number allowed for FieldDefs.  This is an inherent limit of the
 * protobuf wire format. */
#define UPB_MAX_FIELDNUMBER ((1 << 29) - 1)

#ifdef __cplusplus
extern "C" {
#endif

const char *upb_fielddef_fullname(const upb_fielddef *f);
upb_fieldtype_t upb_fielddef_type(const upb_fielddef *f);
upb_descriptortype_t upb_fielddef_descriptortype(const upb_fielddef *f);
upb_label_t upb_fielddef_label(const upb_fielddef *f);
uint32_t upb_fielddef_number(const upb_fielddef *f);
const char *upb_fielddef_name(const upb_fielddef *f);
bool upb_fielddef_isextension(const upb_fielddef *f);
bool upb_fielddef_lazy(const upb_fielddef *f);
bool upb_fielddef_packed(const upb_fielddef *f);
size_t upb_fielddef_getjsonname(const upb_fielddef *f, char *buf, size_t len);
const upb_msgdef *upb_fielddef_containingtype(const upb_fielddef *f);
const upb_oneofdef *upb_fielddef_containingoneof(const upb_fielddef *f);
uint32_t upb_fielddef_index(const upb_fielddef *f);
bool upb_fielddef_issubmsg(const upb_fielddef *f);
bool upb_fielddef_isstring(const upb_fielddef *f);
bool upb_fielddef_isseq(const upb_fielddef *f);
bool upb_fielddef_isprimitive(const upb_fielddef *f);
bool upb_fielddef_ismap(const upb_fielddef *f);
int64_t upb_fielddef_defaultint64(const upb_fielddef *f);
int32_t upb_fielddef_defaultint32(const upb_fielddef *f);
uint64_t upb_fielddef_defaultuint64(const upb_fielddef *f);
uint32_t upb_fielddef_defaultuint32(const upb_fielddef *f);
bool upb_fielddef_defaultbool(const upb_fielddef *f);
float upb_fielddef_defaultfloat(const upb_fielddef *f);
double upb_fielddef_defaultdouble(const upb_fielddef *f);
const char *upb_fielddef_defaultstr(const upb_fielddef *f, size_t *len);
bool upb_fielddef_hassubdef(const upb_fielddef *f);
bool upb_fielddef_haspresence(const upb_fielddef *f);
const upb_msgdef *upb_fielddef_msgsubdef(const upb_fielddef *f);
const upb_enumdef *upb_fielddef_enumsubdef(const upb_fielddef *f);

/* Internal only. */
uint32_t upb_fielddef_selectorbase(const upb_fielddef *f);

#ifdef __cplusplus
}  /* extern "C" */

/* A upb_fielddef describes a single field in a message.  It is most often
 * found as a part of a upb_msgdef, but can also stand alone to represent
 * an extension. */
class upb::FieldDefPtr {
 public:
  FieldDefPtr() : ptr_(nullptr) {}
  explicit FieldDefPtr(const upb_fielddef *ptr) : ptr_(ptr) {}

  const upb_fielddef* ptr() const { return ptr_; }
  explicit operator bool() const { return ptr_ != nullptr; }

  typedef upb_fieldtype_t Type;
  typedef upb_label_t Label;
  typedef upb_descriptortype_t DescriptorType;

  const char* full_name() const { return upb_fielddef_fullname(ptr_); }

  Type type() const { return upb_fielddef_type(ptr_); }
  Label label() const { return upb_fielddef_label(ptr_); }
  const char* name() const { return upb_fielddef_name(ptr_); }
  uint32_t number() const { return upb_fielddef_number(ptr_); }
  bool is_extension() const { return upb_fielddef_isextension(ptr_); }

  /* Copies the JSON name for this field into the given buffer.  Returns the
   * actual size of the JSON name, including the NULL terminator.  If the
   * return value is 0, the JSON name is unset.  If the return value is
   * greater than len, the JSON name was truncated.  The buffer is always
   * NULL-terminated if len > 0.
   *
   * The JSON name always defaults to a camelCased version of the regular
   * name.  However if the regular name is unset, the JSON name will be unset
   * also.
   */
  size_t GetJsonName(char *buf, size_t len) const {
    return upb_fielddef_getjsonname(ptr_, buf, len);
  }

  /* Convenience version of the above function which copies the JSON name
   * into the given string, returning false if the name is not set. */
  template <class T>
  bool GetJsonName(T* str) {
    str->resize(GetJsonName(NULL, 0));
    GetJsonName(&(*str)[0], str->size());
    return str->size() > 0;
  }

  /* For UPB_TYPE_MESSAGE fields only where is_tag_delimited() == false,
   * indicates whether this field should have lazy parsing handlers that yield
   * the unparsed string for the submessage.
   *
   * TODO(haberman): I think we want to move this into a FieldOptions container
   * when we add support for custom options (the FieldOptions struct will
   * contain both regular FieldOptions like "lazy" *and* custom options). */
  bool lazy() const { return upb_fielddef_lazy(ptr_); }

  /* For non-string, non-submessage fields, this indicates whether binary
   * protobufs are encoded in packed or non-packed format.
   *
   * TODO(haberman): see note above about putting options like this into a
   * FieldOptions container. */
  bool packed() const { return upb_fielddef_packed(ptr_); }

  /* An integer that can be used as an index into an array of fields for
   * whatever message this field belongs to.  Guaranteed to be less than
   * f->containing_type()->field_count().  May only be accessed once the def has
   * been finalized. */
  uint32_t index() const { return upb_fielddef_index(ptr_); }

  /* The MessageDef to which this field belongs.
   *
   * If this field has been added to a MessageDef, that message can be retrieved
   * directly (this is always the case for frozen FieldDefs).
   *
   * If the field has not yet been added to a MessageDef, you can set the name
   * of the containing type symbolically instead.  This is mostly useful for
   * extensions, where the extension is declared separately from the message. */
  MessageDefPtr containing_type() const;

  /* The OneofDef to which this field belongs, or NULL if this field is not part
   * of a oneof. */
  OneofDefPtr containing_oneof() const;

  /* The field's type according to the enum in descriptor.proto.  This is not
   * the same as UPB_TYPE_*, because it distinguishes between (for example)
   * INT32 and SINT32, whereas our "type" enum does not.  This return of
   * descriptor_type() is a function of type(), integer_format(), and
   * is_tag_delimited().  */
  DescriptorType descriptor_type() const {
    return upb_fielddef_descriptortype(ptr_);
  }

  /* Convenient field type tests. */
  bool IsSubMessage() const { return upb_fielddef_issubmsg(ptr_); }
  bool IsString() const { return upb_fielddef_isstring(ptr_); }
  bool IsSequence() const { return upb_fielddef_isseq(ptr_); }
  bool IsPrimitive() const { return upb_fielddef_isprimitive(ptr_); }
  bool IsMap() const { return upb_fielddef_ismap(ptr_); }

  /* Returns the non-string default value for this fielddef, which may either
   * be something the client set explicitly or the "default default" (0 for
   * numbers, empty for strings).  The field's type indicates the type of the
   * returned value, except for enum fields that are still mutable.
   *
   * Requires that the given function matches the field's current type. */
  int64_t default_int64() const { return upb_fielddef_defaultint64(ptr_); }
  int32_t default_int32() const { return upb_fielddef_defaultint32(ptr_); }
  uint64_t default_uint64() const { return upb_fielddef_defaultuint64(ptr_); }
  uint32_t default_uint32() const { return upb_fielddef_defaultuint32(ptr_); }
  bool default_bool() const { return upb_fielddef_defaultbool(ptr_); }
  float default_float() const { return upb_fielddef_defaultfloat(ptr_); }
  double default_double() const { return upb_fielddef_defaultdouble(ptr_); }

  /* The resulting string is always NULL-terminated.  If non-NULL, the length
   * will be stored in *len. */
  const char *default_string(size_t * len) const {
    return upb_fielddef_defaultstr(ptr_, len);
  }

  /* Returns the enum or submessage def for this field, if any.  The field's
   * type must match (ie. you may only call enum_subdef() for fields where
   * type() == UPB_TYPE_ENUM). */
  EnumDefPtr enum_subdef() const;
  MessageDefPtr message_subdef() const;

 private:
  const upb_fielddef *ptr_;
};

#endif  /* __cplusplus */

/* upb_oneofdef ***************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

typedef upb_inttable_iter upb_oneof_iter;

const char *upb_oneofdef_name(const upb_oneofdef *o);
const upb_msgdef *upb_oneofdef_containingtype(const upb_oneofdef *o);
int upb_oneofdef_numfields(const upb_oneofdef *o);
uint32_t upb_oneofdef_index(const upb_oneofdef *o);

/* Oneof lookups:
 * - ntof:  look up a field by name.
 * - ntofz: look up a field by name (as a null-terminated string).
 * - itof:  look up a field by number. */
const upb_fielddef *upb_oneofdef_ntof(const upb_oneofdef *o,
                                      const char *name, size_t length);
UPB_INLINE const upb_fielddef *upb_oneofdef_ntofz(const upb_oneofdef *o,
                                                  const char *name) {
  return upb_oneofdef_ntof(o, name, strlen(name));
}
const upb_fielddef *upb_oneofdef_itof(const upb_oneofdef *o, uint32_t num);

/*  upb_oneof_iter i;
 *  for(upb_oneof_begin(&i, e); !upb_oneof_done(&i); upb_oneof_next(&i)) {
 *    // ...
 *  }
 */
void upb_oneof_begin(upb_oneof_iter *iter, const upb_oneofdef *o);
void upb_oneof_next(upb_oneof_iter *iter);
bool upb_oneof_done(upb_oneof_iter *iter);
upb_fielddef *upb_oneof_iter_field(const upb_oneof_iter *iter);
void upb_oneof_iter_setdone(upb_oneof_iter *iter);
bool upb_oneof_iter_isequal(const upb_oneof_iter *iter1,
                            const upb_oneof_iter *iter2);

#ifdef __cplusplus
}  /* extern "C" */

/* Class that represents a oneof. */
class upb::OneofDefPtr {
 public:
  OneofDefPtr() : ptr_(nullptr) {}
  explicit OneofDefPtr(const upb_oneofdef *ptr) : ptr_(ptr) {}

  const upb_oneofdef* ptr() const { return ptr_; }
  explicit operator bool() { return ptr_ != nullptr; }

  /* Returns the MessageDef that owns this OneofDef. */
  MessageDefPtr containing_type() const;

  /* Returns the name of this oneof. This is the name used to look up the oneof
   * by name once added to a message def. */
  const char* name() const { return upb_oneofdef_name(ptr_); }

  /* Returns the number of fields currently defined in the oneof. */
  int field_count() const { return upb_oneofdef_numfields(ptr_); }

  /* Looks up by name. */
  FieldDefPtr FindFieldByName(const char *name, size_t len) const {
    return FieldDefPtr(upb_oneofdef_ntof(ptr_, name, len));
  }
  FieldDefPtr FindFieldByName(const char* name) const {
    return FieldDefPtr(upb_oneofdef_ntofz(ptr_, name));
  }

  template <class T>
  FieldDefPtr FindFieldByName(const T& str) const {
    return FindFieldByName(str.c_str(), str.size());
  }

  /* Looks up by tag number. */
  FieldDefPtr FindFieldByNumber(uint32_t num) const {
    return FieldDefPtr(upb_oneofdef_itof(ptr_, num));
  }

  class const_iterator
      : public std::iterator<std::forward_iterator_tag, FieldDefPtr> {
   public:
    void operator++() { upb_oneof_next(&iter_); }

    FieldDefPtr operator*() const {
      return FieldDefPtr(upb_oneof_iter_field(&iter_));
    }

    bool operator!=(const const_iterator& other) const {
      return !upb_oneof_iter_isequal(&iter_, &other.iter_);
    }

    bool operator==(const const_iterator& other) const {
      return upb_oneof_iter_isequal(&iter_, &other.iter_);
    }

   private:
    friend class OneofDefPtr;

    const_iterator() {}
    explicit const_iterator(OneofDefPtr o) {
      upb_oneof_begin(&iter_, o.ptr());
    }
    static const_iterator end() {
      const_iterator iter;
      upb_oneof_iter_setdone(&iter.iter_);
      return iter;
    }

    upb_oneof_iter iter_;
  };

  const_iterator begin() const { return const_iterator(*this); }
  const_iterator end() const { return const_iterator::end(); }

 private:
  const upb_oneofdef *ptr_;
};

inline upb::OneofDefPtr upb::FieldDefPtr::containing_oneof() const {
  return OneofDefPtr(upb_fielddef_containingoneof(ptr_));
}

#endif  /* __cplusplus */

/* upb_msgdef *****************************************************************/

typedef upb_inttable_iter upb_msg_field_iter;
typedef upb_strtable_iter upb_msg_oneof_iter;

/* Well-known field tag numbers for map-entry messages. */
#define UPB_MAPENTRY_KEY   1
#define UPB_MAPENTRY_VALUE 2

/* Well-known field tag numbers for Any messages. */
#define UPB_ANY_TYPE 1
#define UPB_ANY_VALUE 2

/* Well-known field tag numbers for timestamp messages. */
#define UPB_DURATION_SECONDS 1
#define UPB_DURATION_NANOS 2

/* Well-known field tag numbers for duration messages. */
#define UPB_TIMESTAMP_SECONDS 1
#define UPB_TIMESTAMP_NANOS 2

#ifdef __cplusplus
extern "C" {
#endif

const char *upb_msgdef_fullname(const upb_msgdef *m);
const upb_filedef *upb_msgdef_file(const upb_msgdef *m);
const char *upb_msgdef_name(const upb_msgdef *m);
int upb_msgdef_numoneofs(const upb_msgdef *m);
upb_syntax_t upb_msgdef_syntax(const upb_msgdef *m);
bool upb_msgdef_mapentry(const upb_msgdef *m);
upb_wellknowntype_t upb_msgdef_wellknowntype(const upb_msgdef *m);
bool upb_msgdef_isnumberwrapper(const upb_msgdef *m);
bool upb_msgdef_setsyntax(upb_msgdef *m, upb_syntax_t syntax);
const upb_fielddef *upb_msgdef_itof(const upb_msgdef *m, uint32_t i);
const upb_fielddef *upb_msgdef_ntof(const upb_msgdef *m, const char *name,
                                    size_t len);
const upb_oneofdef *upb_msgdef_ntoo(const upb_msgdef *m, const char *name,
                                    size_t len);
int upb_msgdef_numfields(const upb_msgdef *m);
int upb_msgdef_numoneofs(const upb_msgdef *m);

UPB_INLINE const upb_oneofdef *upb_msgdef_ntooz(const upb_msgdef *m,
                                               const char *name) {
  return upb_msgdef_ntoo(m, name, strlen(name));
}

UPB_INLINE const upb_fielddef *upb_msgdef_ntofz(const upb_msgdef *m,
                                                const char *name) {
  return upb_msgdef_ntof(m, name, strlen(name));
}

/* Internal-only. */
size_t upb_msgdef_selectorcount(const upb_msgdef *m);
uint32_t upb_msgdef_submsgfieldcount(const upb_msgdef *m);

/* Lookup of either field or oneof by name.  Returns whether either was found.
 * If the return is true, then the found def will be set, and the non-found
 * one set to NULL. */
bool upb_msgdef_lookupname(const upb_msgdef *m, const char *name, size_t len,
                           const upb_fielddef **f, const upb_oneofdef **o);

UPB_INLINE bool upb_msgdef_lookupnamez(const upb_msgdef *m, const char *name,
                                       const upb_fielddef **f,
                                       const upb_oneofdef **o) {
  return upb_msgdef_lookupname(m, name, strlen(name), f, o);
}

/* Iteration over fields and oneofs.  For example:
 *
 * upb_msg_field_iter i;
 * for(upb_msg_field_begin(&i, m);
 *     !upb_msg_field_done(&i);
 *     upb_msg_field_next(&i)) {
 *   upb_fielddef *f = upb_msg_iter_field(&i);
 *   // ...
 * }
 *
 * For C we don't have separate iterators for const and non-const.
 * It is the caller's responsibility to cast the upb_fielddef* to
 * const if the upb_msgdef* is const. */
void upb_msg_field_begin(upb_msg_field_iter *iter, const upb_msgdef *m);
void upb_msg_field_next(upb_msg_field_iter *iter);
bool upb_msg_field_done(const upb_msg_field_iter *iter);
upb_fielddef *upb_msg_iter_field(const upb_msg_field_iter *iter);
void upb_msg_field_iter_setdone(upb_msg_field_iter *iter);
bool upb_msg_field_iter_isequal(const upb_msg_field_iter * iter1,
                                const upb_msg_field_iter * iter2);

/* Similar to above, we also support iterating through the oneofs in a
 * msgdef. */
void upb_msg_oneof_begin(upb_msg_oneof_iter * iter, const upb_msgdef *m);
void upb_msg_oneof_next(upb_msg_oneof_iter * iter);
bool upb_msg_oneof_done(const upb_msg_oneof_iter *iter);
const upb_oneofdef *upb_msg_iter_oneof(const upb_msg_oneof_iter *iter);
void upb_msg_oneof_iter_setdone(upb_msg_oneof_iter * iter);
bool upb_msg_oneof_iter_isequal(const upb_msg_oneof_iter *iter1,
                                const upb_msg_oneof_iter *iter2);

#ifdef __cplusplus
}  /* extern "C" */

/* Structure that describes a single .proto message type. */
class upb::MessageDefPtr {
 public:
  MessageDefPtr() : ptr_(nullptr) {}
  explicit MessageDefPtr(const upb_msgdef *ptr) : ptr_(ptr) {}

  const upb_msgdef *ptr() const { return ptr_; }
  explicit operator bool() const { return ptr_ != nullptr; }

  const char* full_name() const { return upb_msgdef_fullname(ptr_); }
  const char* name() const { return upb_msgdef_name(ptr_); }

  /* The number of fields that belong to the MessageDef. */
  int field_count() const { return upb_msgdef_numfields(ptr_); }

  /* The number of oneofs that belong to the MessageDef. */
  int oneof_count() const { return upb_msgdef_numoneofs(ptr_); }

  upb_syntax_t syntax() const { return upb_msgdef_syntax(ptr_); }

  /* These return null pointers if the field is not found. */
  FieldDefPtr FindFieldByNumber(uint32_t number) const {
    return FieldDefPtr(upb_msgdef_itof(ptr_, number));
  }
  FieldDefPtr FindFieldByName(const char* name, size_t len) const {
    return FieldDefPtr(upb_msgdef_ntof(ptr_, name, len));
  }
  FieldDefPtr FindFieldByName(const char *name) const {
    return FieldDefPtr(upb_msgdef_ntofz(ptr_, name));
  }

  template <class T>
  FieldDefPtr FindFieldByName(const T& str) const {
    return FindFieldByName(str.c_str(), str.size());
  }

  OneofDefPtr FindOneofByName(const char* name, size_t len) const {
    return OneofDefPtr(upb_msgdef_ntoo(ptr_, name, len));
  }

  OneofDefPtr FindOneofByName(const char *name) const {
    return OneofDefPtr(upb_msgdef_ntooz(ptr_, name));
  }

  template <class T>
  OneofDefPtr FindOneofByName(const T &str) const {
    return FindOneofByName(str.c_str(), str.size());
  }

  /* Is this message a map entry? */
  bool mapentry() const { return upb_msgdef_mapentry(ptr_); }

  /* Return the type of well known type message. UPB_WELLKNOWN_UNSPECIFIED for
   * non-well-known message. */
  upb_wellknowntype_t wellknowntype() const {
    return upb_msgdef_wellknowntype(ptr_);
  }

  /* Whether is a number wrapper. */
  bool isnumberwrapper() const { return upb_msgdef_isnumberwrapper(ptr_); }

  /* Iteration over fields.  The order is undefined. */
  class const_field_iterator
      : public std::iterator<std::forward_iterator_tag, FieldDefPtr> {
   public:
    void operator++() { upb_msg_field_next(&iter_); }

    FieldDefPtr operator*() const {
      return FieldDefPtr(upb_msg_iter_field(&iter_));
    }

    bool operator!=(const const_field_iterator &other) const {
      return !upb_msg_field_iter_isequal(&iter_, &other.iter_);
    }

    bool operator==(const const_field_iterator &other) const {
      return upb_msg_field_iter_isequal(&iter_, &other.iter_);
    }

   private:
    friend class MessageDefPtr;

    explicit const_field_iterator() {}

    explicit const_field_iterator(MessageDefPtr msg) {
      upb_msg_field_begin(&iter_, msg.ptr());
    }

    static const_field_iterator end() {
      const_field_iterator iter;
      upb_msg_field_iter_setdone(&iter.iter_);
      return iter;
    }

    upb_msg_field_iter iter_;
  };

  /* Iteration over oneofs. The order is undefined. */
  class const_oneof_iterator
      : public std::iterator<std::forward_iterator_tag, OneofDefPtr> {
   public:

    void operator++() { upb_msg_oneof_next(&iter_); }

    OneofDefPtr operator*() const {
      return OneofDefPtr(upb_msg_iter_oneof(&iter_));
    }

    bool operator!=(const const_oneof_iterator& other) const {
      return !upb_msg_oneof_iter_isequal(&iter_, &other.iter_);
    }

    bool operator==(const const_oneof_iterator &other) const {
      return upb_msg_oneof_iter_isequal(&iter_, &other.iter_);
    }

   private:
    friend class MessageDefPtr;

    const_oneof_iterator() {}

    explicit const_oneof_iterator(MessageDefPtr msg) {
      upb_msg_oneof_begin(&iter_, msg.ptr());
    }

    static const_oneof_iterator end() {
      const_oneof_iterator iter;
      upb_msg_oneof_iter_setdone(&iter.iter_);
      return iter;
    }

    upb_msg_oneof_iter iter_;
  };

  class ConstFieldAccessor {
   public:
    explicit ConstFieldAccessor(const upb_msgdef* md) : md_(md) {}
    const_field_iterator begin() { return MessageDefPtr(md_).field_begin(); }
    const_field_iterator end() { return MessageDefPtr(md_).field_end(); }
   private:
    const upb_msgdef* md_;
  };

  class ConstOneofAccessor {
   public:
    explicit ConstOneofAccessor(const upb_msgdef* md) : md_(md) {}
    const_oneof_iterator begin() { return MessageDefPtr(md_).oneof_begin(); }
    const_oneof_iterator end() { return MessageDefPtr(md_).oneof_end(); }
   private:
    const upb_msgdef* md_;
  };

  const_field_iterator field_begin() const {
    return const_field_iterator(*this);
  }

  const_field_iterator field_end() const { return const_field_iterator::end(); }

  const_oneof_iterator oneof_begin() const {
    return const_oneof_iterator(*this);
  }

  const_oneof_iterator oneof_end() const { return const_oneof_iterator::end(); }

  ConstFieldAccessor fields() const { return ConstFieldAccessor(ptr()); }
  ConstOneofAccessor oneofs() const { return ConstOneofAccessor(ptr()); }

 private:
  const upb_msgdef* ptr_;
};

inline upb::MessageDefPtr upb::FieldDefPtr::message_subdef() const {
  return MessageDefPtr(upb_fielddef_msgsubdef(ptr_));
}

inline upb::MessageDefPtr upb::FieldDefPtr::containing_type() const {
  return MessageDefPtr(upb_fielddef_containingtype(ptr_));
}

inline upb::MessageDefPtr upb::OneofDefPtr::containing_type() const {
  return MessageDefPtr(upb_oneofdef_containingtype(ptr_));
}

#endif  /* __cplusplus */

/* upb_enumdef ****************************************************************/

typedef upb_strtable_iter upb_enum_iter;

const char *upb_enumdef_fullname(const upb_enumdef *e);
const char *upb_enumdef_name(const upb_enumdef *e);
const upb_filedef *upb_enumdef_file(const upb_enumdef *e);
int32_t upb_enumdef_default(const upb_enumdef *e);
int upb_enumdef_numvals(const upb_enumdef *e);

/* Enum lookups:
 * - ntoi:  look up a name with specified length.
 * - ntoiz: look up a name provided as a null-terminated string.
 * - iton:  look up an integer, returning the name as a null-terminated
 *          string. */
bool upb_enumdef_ntoi(const upb_enumdef *e, const char *name, size_t len,
                      int32_t *num);
UPB_INLINE bool upb_enumdef_ntoiz(const upb_enumdef *e,
                                  const char *name, int32_t *num) {
  return upb_enumdef_ntoi(e, name, strlen(name), num);
}
const char *upb_enumdef_iton(const upb_enumdef *e, int32_t num);

/*  upb_enum_iter i;
 *  for(upb_enum_begin(&i, e); !upb_enum_done(&i); upb_enum_next(&i)) {
 *    // ...
 *  }
 */
void upb_enum_begin(upb_enum_iter *iter, const upb_enumdef *e);
void upb_enum_next(upb_enum_iter *iter);
bool upb_enum_done(upb_enum_iter *iter);
const char *upb_enum_iter_name(upb_enum_iter *iter);
int32_t upb_enum_iter_number(upb_enum_iter *iter);

#ifdef __cplusplus

class upb::EnumDefPtr {
 public:
  EnumDefPtr() : ptr_(nullptr) {}
  explicit EnumDefPtr(const upb_enumdef* ptr) : ptr_(ptr) {}

  const upb_enumdef* ptr() const { return ptr_; }
  explicit operator bool() const { return ptr_ != nullptr; }

  const char* full_name() const { return upb_enumdef_fullname(ptr_); }
  const char* name() const { return upb_enumdef_name(ptr_); }

  /* The value that is used as the default when no field default is specified.
   * If not set explicitly, the first value that was added will be used.
   * The default value must be a member of the enum.
   * Requires that value_count() > 0. */
  int32_t default_value() const { return upb_enumdef_default(ptr_); }

  /* Returns the number of values currently defined in the enum.  Note that
   * multiple names can refer to the same number, so this may be greater than
   * the total number of unique numbers. */
  int value_count() const { return upb_enumdef_numvals(ptr_); }

  /* Lookups from name to integer, returning true if found. */
  bool FindValueByName(const char *name, int32_t *num) const {
    return upb_enumdef_ntoiz(ptr_, name, num);
  }

  /* Finds the name corresponding to the given number, or NULL if none was
   * found.  If more than one name corresponds to this number, returns the
   * first one that was added. */
  const char *FindValueByNumber(int32_t num) const {
    return upb_enumdef_iton(ptr_, num);
  }

  /* Iteration over name/value pairs.  The order is undefined.
   * Adding an enum val invalidates any iterators.
   *
   * TODO: make compatible with range-for, with elements as pairs? */
  class Iterator {
   public:
    explicit Iterator(EnumDefPtr e) { upb_enum_begin(&iter_, e.ptr()); }

    int32_t number() { return upb_enum_iter_number(&iter_); }
    const char *name() { return upb_enum_iter_name(&iter_); }
    bool Done() { return upb_enum_done(&iter_); }
    void Next() { return upb_enum_next(&iter_); }

   private:
    upb_enum_iter iter_;
  };

 private:
  const upb_enumdef *ptr_;
};

inline upb::EnumDefPtr upb::FieldDefPtr::enum_subdef() const {
  return EnumDefPtr(upb_fielddef_enumsubdef(ptr_));
}

#endif  /* __cplusplus */

/* upb_filedef ****************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

const char *upb_filedef_name(const upb_filedef *f);
const char *upb_filedef_package(const upb_filedef *f);
const char *upb_filedef_phpprefix(const upb_filedef *f);
const char *upb_filedef_phpnamespace(const upb_filedef *f);
upb_syntax_t upb_filedef_syntax(const upb_filedef *f);
int upb_filedef_depcount(const upb_filedef *f);
int upb_filedef_msgcount(const upb_filedef *f);
int upb_filedef_enumcount(const upb_filedef *f);
const upb_filedef *upb_filedef_dep(const upb_filedef *f, int i);
const upb_msgdef *upb_filedef_msg(const upb_filedef *f, int i);
const upb_enumdef *upb_filedef_enum(const upb_filedef *f, int i);

#ifdef __cplusplus
}  /* extern "C" */

/* Class that represents a .proto file with some things defined in it.
 *
 * Many users won't care about FileDefs, but they are necessary if you want to
 * read the values of file-level options. */
class upb::FileDefPtr {
 public:
  explicit FileDefPtr(const upb_filedef *ptr) : ptr_(ptr) {}

  const upb_filedef* ptr() const { return ptr_; }
  explicit operator bool() const { return ptr_ != nullptr; }

  /* Get/set name of the file (eg. "foo/bar.proto"). */
  const char* name() const { return upb_filedef_name(ptr_); }

  /* Package name for definitions inside the file (eg. "foo.bar"). */
  const char* package() const { return upb_filedef_package(ptr_); }

  /* Sets the php class prefix which is prepended to all php generated classes
   * from this .proto. Default is empty. */
  const char* phpprefix() const { return upb_filedef_phpprefix(ptr_); }

  /* Use this option to change the namespace of php generated classes. Default
   * is empty. When this option is empty, the package name will be used for
   * determining the namespace. */
  const char* phpnamespace() const { return upb_filedef_phpnamespace(ptr_); }

  /* Syntax for the file.  Defaults to proto2. */
  upb_syntax_t syntax() const { return upb_filedef_syntax(ptr_); }

  /* Get the list of dependencies from the file.  These are returned in the
   * order that they were added to the FileDefPtr. */
  int dependency_count() const { return upb_filedef_depcount(ptr_); }
  const FileDefPtr dependency(int index) const {
    return FileDefPtr(upb_filedef_dep(ptr_, index));
  }

 private:
  const upb_filedef* ptr_;
};

#endif  /* __cplusplus */

/* upb_symtab *****************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

upb_symtab *upb_symtab_new(void);
void upb_symtab_free(upb_symtab* s);
const upb_msgdef *upb_symtab_lookupmsg(const upb_symtab *s, const char *sym);
const upb_msgdef *upb_symtab_lookupmsg2(
    const upb_symtab *s, const char *sym, size_t len);
const upb_enumdef *upb_symtab_lookupenum(const upb_symtab *s, const char *sym);
const upb_filedef *upb_symtab_lookupfile(const upb_symtab *s, const char *name);
int upb_symtab_filecount(const upb_symtab *s);
const upb_filedef *upb_symtab_addfile(
    upb_symtab *s, const google_protobuf_FileDescriptorProto *file,
    upb_status *status);

/* For generated code only: loads a generated descriptor. */
typedef struct upb_def_init {
  struct upb_def_init **deps;
  const char *filename;
  upb_strview descriptor;
} upb_def_init;

bool _upb_symtab_loaddefinit(upb_symtab *s, const upb_def_init *init);

#ifdef __cplusplus
}  /* extern "C" */

/* Non-const methods in upb::SymbolTable are NOT thread-safe. */
class upb::SymbolTable {
 public:
  SymbolTable() : ptr_(upb_symtab_new(), upb_symtab_free) {}
  explicit SymbolTable(upb_symtab* s) : ptr_(s, upb_symtab_free) {}

  const upb_symtab* ptr() const { return ptr_.get(); }
  upb_symtab* ptr() { return ptr_.get(); }

  /* Finds an entry in the symbol table with this exact name.  If not found,
   * returns NULL. */
  MessageDefPtr LookupMessage(const char *sym) const {
    return MessageDefPtr(upb_symtab_lookupmsg(ptr_.get(), sym));
  }

  EnumDefPtr LookupEnum(const char *sym) const {
    return EnumDefPtr(upb_symtab_lookupenum(ptr_.get(), sym));
  }

  FileDefPtr LookupFile(const char *name) const {
    return FileDefPtr(upb_symtab_lookupfile(ptr_.get(), name));
  }

  /* TODO: iteration? */

  /* Adds the given serialized FileDescriptorProto to the pool. */
  FileDefPtr AddFile(const google_protobuf_FileDescriptorProto *file_proto,
                     Status *status) {
    return FileDefPtr(
        upb_symtab_addfile(ptr_.get(), file_proto, status->ptr()));
  }

 private:
  std::unique_ptr<upb_symtab, decltype(&upb_symtab_free)> ptr_;
};

UPB_INLINE const char* upb_safecstr(const std::string& str) {
  UPB_ASSERT(str.size() == std::strlen(str.c_str()));
  return str.c_str();
}

#endif  /* __cplusplus */


#endif /* UPB_DEF_H_ */


#ifndef UPB_MSGFACTORY_H_
#define UPB_MSGFACTORY_H_

/** upb_msgfactory ************************************************************/

struct upb_msgfactory;
typedef struct upb_msgfactory upb_msgfactory;

#ifdef __cplusplus
extern "C" {
#endif

/* A upb_msgfactory contains a cache of upb_msglayout, upb_handlers, and
 * upb_visitorplan objects.  These are the objects necessary to represent,
 * populate, and and visit upb_msg objects.
 *
 * These caches are all populated by upb_msgdef, and lazily created on demand.
 */

/* Creates and destroys a msgfactory, respectively.  The messages for this
 * msgfactory must come from |symtab| (which should outlive the msgfactory). */
upb_msgfactory *upb_msgfactory_new(const upb_symtab *symtab);
void upb_msgfactory_free(upb_msgfactory *f);

const upb_symtab *upb_msgfactory_symtab(const upb_msgfactory *f);

/* The functions to get cached objects, lazily creating them on demand.  These
 * all require:
 *
 * - m is in upb_msgfactory_symtab(f)
 * - upb_msgdef_mapentry(m) == false (since map messages can't have layouts).
 *
 * The returned objects will live for as long as the msgfactory does.
 *
 * TODO(haberman): consider making this thread-safe and take a const
 * upb_msgfactory. */
const upb_msglayout *upb_msgfactory_getlayout(upb_msgfactory *f,
                                              const upb_msgdef *m);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif /* UPB_MSGFACTORY_H_ */
/*
** upb::Handlers (upb_handlers)
**
** A upb_handlers is like a virtual table for a upb_msgdef.  Each field of the
** message can have associated functions that will be called when we are
** parsing or visiting a stream of data.  This is similar to how handlers work
** in SAX (the Simple API for XML).
**
** The handlers have no idea where the data is coming from, so a single set of
** handlers could be used with two completely different data sources (for
** example, a parser and a visitor over in-memory objects).  This decoupling is
** the most important feature of upb, because it allows parsers and serializers
** to be highly reusable.
**
** This is a mixed C/C++ interface that offers a full API to both languages.
** See the top-level README for more information.
*/

#ifndef UPB_HANDLERS_H
#define UPB_HANDLERS_H



#ifdef __cplusplus
namespace upb {
class HandlersPtr;
class HandlerCache;
template <class T> class Handler;
template <class T> struct CanonicalType;
}  /* namespace upb */
#endif


/* The maximum depth that the handler graph can have.  This is a resource limit
 * for the C stack since we sometimes need to recursively traverse the graph.
 * Cycles are ok; the traversal will stop when it detects a cycle, but we must
 * hit the cycle before the maximum depth is reached.
 *
 * If having a single static limit is too inflexible, we can add another variant
 * of Handlers::Freeze that allows specifying this as a parameter. */
#define UPB_MAX_HANDLER_DEPTH 64

/* All the different types of handlers that can be registered.
 * Only needed for the advanced functions in upb::Handlers. */
typedef enum {
  UPB_HANDLER_INT32,
  UPB_HANDLER_INT64,
  UPB_HANDLER_UINT32,
  UPB_HANDLER_UINT64,
  UPB_HANDLER_FLOAT,
  UPB_HANDLER_DOUBLE,
  UPB_HANDLER_BOOL,
  UPB_HANDLER_STARTSTR,
  UPB_HANDLER_STRING,
  UPB_HANDLER_ENDSTR,
  UPB_HANDLER_STARTSUBMSG,
  UPB_HANDLER_ENDSUBMSG,
  UPB_HANDLER_STARTSEQ,
  UPB_HANDLER_ENDSEQ
} upb_handlertype_t;

#define UPB_HANDLER_MAX (UPB_HANDLER_ENDSEQ+1)

#define UPB_BREAK NULL

/* A convenient definition for when no closure is needed. */
extern char _upb_noclosure;
#define UPB_NO_CLOSURE &_upb_noclosure

/* A selector refers to a specific field handler in the Handlers object
 * (for example: the STARTSUBMSG handler for field "field15"). */
typedef int32_t upb_selector_t;

/* Static selectors for upb::Handlers. */
#define UPB_STARTMSG_SELECTOR 0
#define UPB_ENDMSG_SELECTOR 1
#define UPB_UNKNOWN_SELECTOR 2
#define UPB_STATIC_SELECTOR_COUNT 3  /* Warning: also in upb/def.c. */

/* Static selectors for upb::BytesHandler. */
#define UPB_STARTSTR_SELECTOR 0
#define UPB_STRING_SELECTOR 1
#define UPB_ENDSTR_SELECTOR 2

#ifdef __cplusplus
template<class T> const void *UniquePtrForType() {
  static const char ch = 0;
  return &ch;
}
#endif

/* upb_handlers ************************************************************/

/* Handler attributes, to be registered with the handler itself. */
typedef struct {
  const void *handler_data;
  const void *closure_type;
  const void *return_closure_type;
  bool alwaysok;
} upb_handlerattr;

#define UPB_HANDLERATTR_INIT {NULL, NULL, NULL, false}

/* Bufhandle, data passed along with a buffer to indicate its provenance. */
typedef struct {
  /* The beginning of the buffer.  This may be different than the pointer
   * passed to a StringBuf handler because the handler may receive data
   * that is from the middle or end of a larger buffer. */
  const char *buf;

  /* The offset within the attached object where this buffer begins.  Only
   * meaningful if there is an attached object. */
  size_t objofs;

  /* The attached object (if any) and a pointer representing its type. */
  const void *obj;
  const void *objtype;

#ifdef __cplusplus
  template <class T>
  void SetAttachedObject(const T* _obj) {
    obj = _obj;
    objtype = UniquePtrForType<T>();
  }

  template <class T>
  const T *GetAttachedObject() const {
    return objtype == UniquePtrForType<T>() ? static_cast<const T *>(obj)
                                            : NULL;
  }
#endif
} upb_bufhandle;

#define UPB_BUFHANDLE_INIT {NULL, 0, NULL, NULL}

/* Handler function typedefs. */
typedef void upb_handlerfree(void *d);
typedef bool upb_unknown_handlerfunc(void *c, const void *hd, const char *buf,
                                     size_t n);
typedef bool upb_startmsg_handlerfunc(void *c, const void*);
typedef bool upb_endmsg_handlerfunc(void *c, const void *, upb_status *status);
typedef void* upb_startfield_handlerfunc(void *c, const void *hd);
typedef bool upb_endfield_handlerfunc(void *c, const void *hd);
typedef bool upb_int32_handlerfunc(void *c, const void *hd, int32_t val);
typedef bool upb_int64_handlerfunc(void *c, const void *hd, int64_t val);
typedef bool upb_uint32_handlerfunc(void *c, const void *hd, uint32_t val);
typedef bool upb_uint64_handlerfunc(void *c, const void *hd, uint64_t val);
typedef bool upb_float_handlerfunc(void *c, const void *hd, float val);
typedef bool upb_double_handlerfunc(void *c, const void *hd, double val);
typedef bool upb_bool_handlerfunc(void *c, const void *hd, bool val);
typedef void *upb_startstr_handlerfunc(void *c, const void *hd,
                                       size_t size_hint);
typedef size_t upb_string_handlerfunc(void *c, const void *hd, const char *buf,
                                      size_t n, const upb_bufhandle* handle);

struct upb_handlers;
typedef struct upb_handlers upb_handlers;

#ifdef __cplusplus
extern "C" {
#endif

/* Mutating accessors. */
const upb_status *upb_handlers_status(upb_handlers *h);
void upb_handlers_clearerr(upb_handlers *h);
const upb_msgdef *upb_handlers_msgdef(const upb_handlers *h);
bool upb_handlers_addcleanup(upb_handlers *h, void *p, upb_handlerfree *hfree);
bool upb_handlers_setunknown(upb_handlers *h, upb_unknown_handlerfunc *func,
                             const upb_handlerattr *attr);
bool upb_handlers_setstartmsg(upb_handlers *h, upb_startmsg_handlerfunc *func,
                              const upb_handlerattr *attr);
bool upb_handlers_setendmsg(upb_handlers *h, upb_endmsg_handlerfunc *func,
                            const upb_handlerattr *attr);
bool upb_handlers_setint32(upb_handlers *h, const upb_fielddef *f,
                           upb_int32_handlerfunc *func,
                           const upb_handlerattr *attr);
bool upb_handlers_setint64(upb_handlers *h, const upb_fielddef *f,
                           upb_int64_handlerfunc *func,
                           const upb_handlerattr *attr);
bool upb_handlers_setuint32(upb_handlers *h, const upb_fielddef *f,
                            upb_uint32_handlerfunc *func,
                            const upb_handlerattr *attr);
bool upb_handlers_setuint64(upb_handlers *h, const upb_fielddef *f,
                            upb_uint64_handlerfunc *func,
                            const upb_handlerattr *attr);
bool upb_handlers_setfloat(upb_handlers *h, const upb_fielddef *f,
                           upb_float_handlerfunc *func,
                           const upb_handlerattr *attr);
bool upb_handlers_setdouble(upb_handlers *h, const upb_fielddef *f,
                            upb_double_handlerfunc *func,
                            const upb_handlerattr *attr);
bool upb_handlers_setbool(upb_handlers *h, const upb_fielddef *f,
                          upb_bool_handlerfunc *func,
                          const upb_handlerattr *attr);
bool upb_handlers_setstartstr(upb_handlers *h, const upb_fielddef *f,
                              upb_startstr_handlerfunc *func,
                              const upb_handlerattr *attr);
bool upb_handlers_setstring(upb_handlers *h, const upb_fielddef *f,
                            upb_string_handlerfunc *func,
                            const upb_handlerattr *attr);
bool upb_handlers_setendstr(upb_handlers *h, const upb_fielddef *f,
                            upb_endfield_handlerfunc *func,
                            const upb_handlerattr *attr);
bool upb_handlers_setstartseq(upb_handlers *h, const upb_fielddef *f,
                              upb_startfield_handlerfunc *func,
                              const upb_handlerattr *attr);
bool upb_handlers_setstartsubmsg(upb_handlers *h, const upb_fielddef *f,
                                 upb_startfield_handlerfunc *func,
                                 const upb_handlerattr *attr);
bool upb_handlers_setendsubmsg(upb_handlers *h, const upb_fielddef *f,
                               upb_endfield_handlerfunc *func,
                               const upb_handlerattr *attr);
bool upb_handlers_setendseq(upb_handlers *h, const upb_fielddef *f,
                            upb_endfield_handlerfunc *func,
                            const upb_handlerattr *attr);

/* Read-only accessors. */
const upb_handlers *upb_handlers_getsubhandlers(const upb_handlers *h,
                                                const upb_fielddef *f);
const upb_handlers *upb_handlers_getsubhandlers_sel(const upb_handlers *h,
                                                    upb_selector_t sel);
upb_func *upb_handlers_gethandler(const upb_handlers *h, upb_selector_t s,
                                  const void **handler_data);
bool upb_handlers_getattr(const upb_handlers *h, upb_selector_t s,
                          upb_handlerattr *attr);

/* "Static" methods */
upb_handlertype_t upb_handlers_getprimitivehandlertype(const upb_fielddef *f);
bool upb_handlers_getselector(const upb_fielddef *f, upb_handlertype_t type,
                              upb_selector_t *s);
UPB_INLINE upb_selector_t upb_handlers_getendselector(upb_selector_t start) {
  return start + 1;
}

#ifdef __cplusplus
}  /* extern "C" */

namespace upb {
typedef upb_handlers Handlers;
}

/* Convenience macros for creating a Handler object that is wrapped with a
 * type-safe wrapper function that converts the "void*" parameters/returns
 * of the underlying C API into nice C++ function.
 *
 * Sample usage:
 *   void OnValue1(MyClosure* c, const MyHandlerData* d, int32_t val) {
 *     // do stuff ...
 *   }
 *
 *   // Handler that doesn't need any data bound to it.
 *   void OnValue2(MyClosure* c, int32_t val) {
 *     // do stuff ...
 *   }
 *
 *   // Handler that returns bool so it can return failure if necessary.
 *   bool OnValue3(MyClosure* c, int32_t val) {
 *     // do stuff ...
 *     return ok;
 *   }
 *
 *   // Member function handler.
 *   class MyClosure {
 *    public:
 *     void OnValue(int32_t val) {
 *       // do stuff ...
 *     }
 *   };
 *
 *   // Takes ownership of the MyHandlerData.
 *   handlers->SetInt32Handler(f1, UpbBind(OnValue1, new MyHandlerData(...)));
 *   handlers->SetInt32Handler(f2, UpbMakeHandler(OnValue2));
 *   handlers->SetInt32Handler(f1, UpbMakeHandler(OnValue3));
 *   handlers->SetInt32Handler(f2, UpbMakeHandler(&MyClosure::OnValue));
 */

/* In C++11, the "template" disambiguator can appear even outside templates,
 * so all calls can safely use this pair of macros. */

#define UpbMakeHandler(f) upb::MatchFunc(f).template GetFunc<f>()

/* We have to be careful to only evaluate "d" once. */
#define UpbBind(f, d) upb::MatchFunc(f).template GetFunc<f>((d))

/* Handler: a struct that contains the (handler, data, deleter) tuple that is
 * used to register all handlers.  Users can Make() these directly but it's
 * more convenient to use the UpbMakeHandler/UpbBind macros above. */
template <class T> class upb::Handler {
 public:
  /* The underlying, handler function signature that upb uses internally. */
  typedef T FuncPtr;

  /* Intentionally implicit. */
  template <class F> Handler(F func);
  ~Handler() { UPB_ASSERT(registered_); }

  void AddCleanup(upb_handlers* h) const;
  FuncPtr handler() const { return handler_; }
  const upb_handlerattr& attr() const { return attr_; }

 private:
  Handler(const Handler&) = delete;
  Handler& operator=(const Handler&) = delete;

  FuncPtr handler_;
  mutable upb_handlerattr attr_;
  mutable bool registered_;
  void *cleanup_data_;
  upb_handlerfree *cleanup_func_;
};

/* A upb::Handlers object represents the set of handlers associated with a
 * message in the graph of messages.  You can think of it as a big virtual
 * table with functions corresponding to all the events that can fire while
 * parsing or visiting a message of a specific type.
 *
 * Any handlers that are not set behave as if they had successfully consumed
 * the value.  Any unset Start* handlers will propagate their closure to the
 * inner frame.
 *
 * The easiest way to create the *Handler objects needed by the Set* methods is
 * with the UpbBind() and UpbMakeHandler() macros; see below. */
class upb::HandlersPtr {
 public:
  HandlersPtr(upb_handlers* ptr) : ptr_(ptr) {}

  upb_handlers* ptr() const { return ptr_; }

  typedef upb_selector_t Selector;
  typedef upb_handlertype_t Type;

  typedef Handler<void *(*)(void *, const void *)> StartFieldHandler;
  typedef Handler<bool (*)(void *, const void *)> EndFieldHandler;
  typedef Handler<bool (*)(void *, const void *)> StartMessageHandler;
  typedef Handler<bool (*)(void *, const void *, upb_status *)>
      EndMessageHandler;
  typedef Handler<void *(*)(void *, const void *, size_t)> StartStringHandler;
  typedef Handler<size_t (*)(void *, const void *, const char *, size_t,
                             const upb_bufhandle *)>
      StringHandler;

  template <class T> struct ValueHandler {
    typedef Handler<bool(*)(void *, const void *, T)> H;
  };

  typedef ValueHandler<int32_t>::H     Int32Handler;
  typedef ValueHandler<int64_t>::H     Int64Handler;
  typedef ValueHandler<uint32_t>::H    UInt32Handler;
  typedef ValueHandler<uint64_t>::H    UInt64Handler;
  typedef ValueHandler<float>::H       FloatHandler;
  typedef ValueHandler<double>::H      DoubleHandler;
  typedef ValueHandler<bool>::H        BoolHandler;

  /* Any function pointer can be converted to this and converted back to its
   * correct type. */
  typedef void GenericFunction();

  typedef void HandlersCallback(const void *closure, upb_handlers *h);

  /* Returns the msgdef associated with this handlers object. */
  MessageDefPtr message_def() const {
    return MessageDefPtr(upb_handlers_msgdef(ptr()));
  }

  /* Adds the given pointer and function to the list of cleanup functions that
   * will be run when these handlers are freed.  If this pointer has previously
   * been registered, the function returns false and does nothing. */
  bool AddCleanup(void *ptr, upb_handlerfree *cleanup) {
    return upb_handlers_addcleanup(ptr_, ptr, cleanup);
  }

  /* Sets the startmsg handler for the message, which is defined as follows:
   *
   *   bool startmsg(MyType* closure) {
   *     // Called when the message begins.  Returns true if processing should
   *     // continue.
   *     return true;
   *   }
   */
  bool SetStartMessageHandler(const StartMessageHandler &h) {
    h.AddCleanup(ptr());
    return upb_handlers_setstartmsg(ptr(), h.handler(), &h.attr());
  }

  /* Sets the endmsg handler for the message, which is defined as follows:
   *
   *   bool endmsg(MyType* closure, upb_status *status) {
   *     // Called when processing of this message ends, whether in success or
   *     // failure.  "status" indicates the final status of processing, and
   *     // can also be modified in-place to update the final status.
   *   }
   */
  bool SetEndMessageHandler(const EndMessageHandler& h) {
    h.AddCleanup(ptr());
    return upb_handlers_setendmsg(ptr(), h.handler(), &h.attr());
  }

  /* Sets the value handler for the given field, which is defined as follows
   * (this is for an int32 field; other field types will pass their native
   * C/C++ type for "val"):
   *
   *   bool OnValue(MyClosure* c, const MyHandlerData* d, int32_t val) {
   *     // Called when the field's value is encountered.  "d" contains
   *     // whatever data was bound to this field when it was registered.
   *     // Returns true if processing should continue.
   *     return true;
   *   }
   *
   *   handers->SetInt32Handler(f, UpbBind(OnValue, new MyHandlerData(...)));
   *
   * The value type must exactly match f->type().
   * For example, a handler that takes an int32_t parameter may only be used for
   * fields of type UPB_TYPE_INT32 and UPB_TYPE_ENUM.
   *
   * Returns false if the handler failed to register; in this case the cleanup
   * handler (if any) will be called immediately.
   */
  bool SetInt32Handler(FieldDefPtr f, const Int32Handler &h) {
    h.AddCleanup(ptr());
    return upb_handlers_setint32(ptr(), f.ptr(), h.handler(), &h.attr());
  }

  bool SetInt64Handler (FieldDefPtr f,  const Int64Handler& h) {
    h.AddCleanup(ptr());
    return upb_handlers_setint64(ptr(), f.ptr(), h.handler(), &h.attr());
  }

  bool SetUInt32Handler(FieldDefPtr f, const UInt32Handler& h) {
    h.AddCleanup(ptr());
    return upb_handlers_setuint32(ptr(), f.ptr(), h.handler(), &h.attr());
  }

  bool SetUInt64Handler(FieldDefPtr f, const UInt64Handler& h) {
    h.AddCleanup(ptr());
    return upb_handlers_setuint64(ptr(), f.ptr(), h.handler(), &h.attr());
  }

  bool SetFloatHandler (FieldDefPtr f,  const FloatHandler& h) {
    h.AddCleanup(ptr());
    return upb_handlers_setfloat(ptr(), f.ptr(), h.handler(), &h.attr());
  }

  bool SetDoubleHandler(FieldDefPtr f, const DoubleHandler& h) {
    h.AddCleanup(ptr());
    return upb_handlers_setdouble(ptr(), f.ptr(), h.handler(), &h.attr());
  }

  bool SetBoolHandler(FieldDefPtr f, const BoolHandler &h) {
    h.AddCleanup(ptr());
    return upb_handlers_setbool(ptr(), f.ptr(), h.handler(), &h.attr());
  }

  /* Like the previous, but templated on the type on the value (ie. int32).
   * This is mostly useful to call from other templates.  To call this you must
   * specify the template parameter explicitly, ie:
   *   h->SetValueHandler<T>(f, UpbBind(MyHandler<T>, MyData)); */
  template <class T>
  bool SetValueHandler(
      FieldDefPtr f,
      const typename ValueHandler<typename CanonicalType<T>::Type>::H &handler);

  /* Sets handlers for a string field, which are defined as follows:
   *
   *   MySubClosure* startstr(MyClosure* c, const MyHandlerData* d,
   *                          size_t size_hint) {
   *     // Called when a string value begins.  The return value indicates the
   *     // closure for the string.  "size_hint" indicates the size of the
   *     // string if it is known, however if the string is length-delimited
   *     // and the end-of-string is not available size_hint will be zero.
   *     // This case is indistinguishable from the case where the size is
   *     // known to be zero.
   *     //
   *     // TODO(haberman): is it important to distinguish these cases?
   *     // If we had ssize_t as a type we could make -1 "unknown", but
   *     // ssize_t is POSIX (not ANSI) and therefore less portable.
   *     // In practice I suspect it won't be important to distinguish.
   *     return closure;
   *   }
   *
   *   size_t str(MyClosure* closure, const MyHandlerData* d,
   *              const char *str, size_t len) {
   *     // Called for each buffer of string data; the multiple physical buffers
   *     // are all part of the same logical string.  The return value indicates
   *     // how many bytes were consumed.  If this number is less than "len",
   *     // this will also indicate that processing should be halted for now,
   *     // like returning false or UPB_BREAK from any other callback.  If
   *     // number is greater than "len", the excess bytes will be skipped over
   *     // and not passed to the callback.
   *     return len;
   *   }
   *
   *   bool endstr(MyClosure* c, const MyHandlerData* d) {
   *     // Called when a string value ends.  Return value indicates whether
   *     // processing should continue.
   *     return true;
   *   }
   */
  bool SetStartStringHandler(FieldDefPtr f, const StartStringHandler &h) {
    h.AddCleanup(ptr());
    return upb_handlers_setstartstr(ptr(), f.ptr(), h.handler(), &h.attr());
  }

  bool SetStringHandler(FieldDefPtr f, const StringHandler& h) {
    h.AddCleanup(ptr());
    return upb_handlers_setstring(ptr(), f.ptr(), h.handler(), &h.attr());
  }

  bool SetEndStringHandler(FieldDefPtr f, const EndFieldHandler& h) {
    h.AddCleanup(ptr());
    return upb_handlers_setendstr(ptr(), f.ptr(), h.handler(), &h.attr());
  }

  /* Sets the startseq handler, which is defined as follows:
   *
   *   MySubClosure *startseq(MyClosure* c, const MyHandlerData* d) {
   *     // Called when a sequence (repeated field) begins.  The returned
   *     // pointer indicates the closure for the sequence (or UPB_BREAK
   *     // to interrupt processing).
   *     return closure;
   *   }
   *
   *   h->SetStartSequenceHandler(f, UpbBind(startseq, new MyHandlerData(...)));
   *
   * Returns "false" if "f" does not belong to this message or is not a
   * repeated field.
   */
  bool SetStartSequenceHandler(FieldDefPtr f, const StartFieldHandler &h) {
    h.AddCleanup(ptr());
    return upb_handlers_setstartseq(ptr(), f.ptr(), h.handler(), &h.attr());
  }

  /* Sets the startsubmsg handler for the given field, which is defined as
   * follows:
   *
   *   MySubClosure* startsubmsg(MyClosure* c, const MyHandlerData* d) {
   *     // Called when a submessage begins.  The returned pointer indicates the
   *     // closure for the sequence (or UPB_BREAK to interrupt processing).
   *     return closure;
   *   }
   *
   *   h->SetStartSubMessageHandler(f, UpbBind(startsubmsg,
   *                                           new MyHandlerData(...)));
   *
   * Returns "false" if "f" does not belong to this message or is not a
   * submessage/group field.
   */
  bool SetStartSubMessageHandler(FieldDefPtr f, const StartFieldHandler& h) {
    h.AddCleanup(ptr());
    return upb_handlers_setstartsubmsg(ptr(), f.ptr(), h.handler(), &h.attr());
  }

  /* Sets the endsubmsg handler for the given field, which is defined as
   * follows:
   *
   *   bool endsubmsg(MyClosure* c, const MyHandlerData* d) {
   *     // Called when a submessage ends.  Returns true to continue processing.
   *     return true;
   *   }
   *
   * Returns "false" if "f" does not belong to this message or is not a
   * submessage/group field.
   */
  bool SetEndSubMessageHandler(FieldDefPtr f, const EndFieldHandler &h) {
    h.AddCleanup(ptr());
    return upb_handlers_setendsubmsg(ptr(), f.ptr(), h.handler(), &h.attr());
  }

  /* Starts the endsubseq handler for the given field, which is defined as
   * follows:
   *
   *   bool endseq(MyClosure* c, const MyHandlerData* d) {
   *     // Called when a sequence ends.  Returns true continue processing.
   *     return true;
   *   }
   *
   * Returns "false" if "f" does not belong to this message or is not a
   * repeated field.
   */
  bool SetEndSequenceHandler(FieldDefPtr f, const EndFieldHandler &h) {
    h.AddCleanup(ptr());
    return upb_handlers_setendseq(ptr(), f.ptr(), h.handler(), &h.attr());
  }

 private:
  upb_handlers* ptr_;
};

#endif  /* __cplusplus */

/* upb_handlercache ***********************************************************/

/* A upb_handlercache lazily builds and caches upb_handlers.  You pass it a
 * function (with optional closure) that can build handlers for a given
 * message on-demand, and the cache maintains a map of msgdef->handlers. */

#ifdef __cplusplus
extern "C" {
#endif

struct upb_handlercache;
typedef struct upb_handlercache upb_handlercache;

typedef void upb_handlers_callback(const void *closure, upb_handlers *h);

upb_handlercache *upb_handlercache_new(upb_handlers_callback *callback,
                                       const void *closure);
void upb_handlercache_free(upb_handlercache *cache);
const upb_handlers *upb_handlercache_get(upb_handlercache *cache,
                                         const upb_msgdef *md);
bool upb_handlercache_addcleanup(upb_handlercache *h, void *p,
                                 upb_handlerfree *hfree);

#ifdef __cplusplus
}  /* extern "C" */

class upb::HandlerCache {
 public:
  HandlerCache(upb_handlers_callback *callback, const void *closure)
      : ptr_(upb_handlercache_new(callback, closure), upb_handlercache_free) {}
  HandlerCache(HandlerCache&&) = default;
  HandlerCache& operator=(HandlerCache&&) = default;
  HandlerCache(upb_handlercache* c) : ptr_(c, upb_handlercache_free) {}

  upb_handlercache* ptr() { return ptr_.get(); }

  const upb_handlers *Get(MessageDefPtr md) {
    return upb_handlercache_get(ptr_.get(), md.ptr());
  }

 private:
  std::unique_ptr<upb_handlercache, decltype(&upb_handlercache_free)> ptr_;
};

#endif  /* __cplusplus */

/* upb_byteshandler ***********************************************************/

typedef struct {
  upb_func *func;

  /* It is wasteful to include the entire attributes here:
   *
   * * Some of the information is redundant (like storing the closure type
   *   separately for each handler that must match).
   * * Some of the info is only needed prior to freeze() (like closure types).
   * * alignment padding wastes a lot of space for alwaysok_.
   *
   * If/when the size and locality of handlers is an issue, we can optimize this
   * not to store the entire attr like this.  We do not expose the table's
   * layout to allow this optimization in the future. */
  upb_handlerattr attr;
} upb_handlers_tabent;

#define UPB_TABENT_INIT {NULL, UPB_HANDLERATTR_INIT}

typedef struct {
  upb_handlers_tabent table[3];
} upb_byteshandler;

#define UPB_BYTESHANDLER_INIT                             \
  {                                                       \
    { UPB_TABENT_INIT, UPB_TABENT_INIT, UPB_TABENT_INIT } \
  }

UPB_INLINE void upb_byteshandler_init(upb_byteshandler *handler) {
  upb_byteshandler init = UPB_BYTESHANDLER_INIT;
  *handler = init;
}

#ifdef __cplusplus
extern "C" {
#endif

/* Caller must ensure that "d" outlives the handlers. */
bool upb_byteshandler_setstartstr(upb_byteshandler *h,
                                  upb_startstr_handlerfunc *func, void *d);
bool upb_byteshandler_setstring(upb_byteshandler *h,
                                upb_string_handlerfunc *func, void *d);
bool upb_byteshandler_setendstr(upb_byteshandler *h,
                                upb_endfield_handlerfunc *func, void *d);

#ifdef __cplusplus
}  /* extern "C" */

namespace upb {
typedef upb_byteshandler BytesHandler;
}
#endif

/** Message handlers ******************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/* These are the handlers used internally by upb_msgfactory_getmergehandlers().
 * They write scalar data to a known offset from the message pointer.
 *
 * These would be trivial for anyone to implement themselves, but it's better
 * to use these because some JITs will recognize and specialize these instead
 * of actually calling the function. */

/* Sets a handler for the given primitive field that will write the data at the
 * given offset.  If hasbit > 0, also sets a hasbit at the given bit offset
 * (addressing each byte low to high). */
bool upb_msg_setscalarhandler(upb_handlers *h,
                              const upb_fielddef *f,
                              size_t offset,
                              int32_t hasbit);

/* If the given handler is a msghandlers_primitive field, returns true and sets
 * *type, *offset and *hasbit.  Otherwise returns false. */
bool upb_msg_getscalarhandlerdata(const upb_handlers *h,
                                  upb_selector_t s,
                                  upb_fieldtype_t *type,
                                  size_t *offset,
                                  int32_t *hasbit);



#ifdef __cplusplus
}  /* extern "C" */
#endif


/*
** Inline definitions for handlers.h, which are particularly long and a bit
** tricky.
*/

#ifndef UPB_HANDLERS_INL_H_
#define UPB_HANDLERS_INL_H_

#include <limits.h>
#include <stddef.h>


#ifdef __cplusplus

/* Type detection and typedefs for integer types.
 * For platforms where there are multiple 32-bit or 64-bit types, we need to be
 * able to enumerate them so we can properly create overloads for all variants.
 *
 * If any platform existed where there were three integer types with the same
 * size, this would have to become more complicated.  For example, short, int,
 * and long could all be 32-bits.  Even more diabolically, short, int, long,
 * and long long could all be 64 bits and still be standard-compliant.
 * However, few platforms are this strange, and it's unlikely that upb will be
 * used on the strangest ones. */

/* Can't count on stdint.h limits like INT32_MAX, because in C++ these are
 * only defined when __STDC_LIMIT_MACROS are defined before the *first* include
 * of stdint.h.  We can't guarantee that someone else didn't include these first
 * without defining __STDC_LIMIT_MACROS. */
#define UPB_INT32_MAX 0x7fffffffLL
#define UPB_INT32_MIN (-UPB_INT32_MAX - 1)
#define UPB_INT64_MAX 0x7fffffffffffffffLL
#define UPB_INT64_MIN (-UPB_INT64_MAX - 1)

#if INT_MAX == UPB_INT32_MAX && INT_MIN == UPB_INT32_MIN
#define UPB_INT_IS_32BITS 1
#endif

#if LONG_MAX == UPB_INT32_MAX && LONG_MIN == UPB_INT32_MIN
#define UPB_LONG_IS_32BITS 1
#endif

#if LONG_MAX == UPB_INT64_MAX && LONG_MIN == UPB_INT64_MIN
#define UPB_LONG_IS_64BITS 1
#endif

#if LLONG_MAX == UPB_INT64_MAX && LLONG_MIN == UPB_INT64_MIN
#define UPB_LLONG_IS_64BITS 1
#endif

/* We use macros instead of typedefs so we can undefine them later and avoid
 * leaking them outside this header file. */
#if UPB_INT_IS_32BITS
#define UPB_INT32_T int
#define UPB_UINT32_T unsigned int

#if UPB_LONG_IS_32BITS
#define UPB_TWO_32BIT_TYPES 1
#define UPB_INT32ALT_T long
#define UPB_UINT32ALT_T unsigned long
#endif  /* UPB_LONG_IS_32BITS */

#elif UPB_LONG_IS_32BITS  /* && !UPB_INT_IS_32BITS */
#define UPB_INT32_T long
#define UPB_UINT32_T unsigned long
#endif  /* UPB_INT_IS_32BITS */


#if UPB_LONG_IS_64BITS
#define UPB_INT64_T long
#define UPB_UINT64_T unsigned long

#if UPB_LLONG_IS_64BITS
#define UPB_TWO_64BIT_TYPES 1
#define UPB_INT64ALT_T long long
#define UPB_UINT64ALT_T unsigned long long
#endif  /* UPB_LLONG_IS_64BITS */

#elif UPB_LLONG_IS_64BITS  /* && !UPB_LONG_IS_64BITS */
#define UPB_INT64_T long long
#define UPB_UINT64_T unsigned long long
#endif  /* UPB_LONG_IS_64BITS */

#undef UPB_INT32_MAX
#undef UPB_INT32_MIN
#undef UPB_INT64_MAX
#undef UPB_INT64_MIN
#undef UPB_INT_IS_32BITS
#undef UPB_LONG_IS_32BITS
#undef UPB_LONG_IS_64BITS
#undef UPB_LLONG_IS_64BITS


namespace upb {

typedef void CleanupFunc(void *ptr);

/* Template to remove "const" from "const T*" and just return "T*".
 *
 * We define a nonsense default because otherwise it will fail to instantiate as
 * a function parameter type even in cases where we don't expect any caller to
 * actually match the overload. */
class CouldntRemoveConst {};
template <class T> struct remove_constptr { typedef CouldntRemoveConst type; };
template <class T> struct remove_constptr<const T *> { typedef T *type; };

/* Template that we use below to remove a template specialization from
 * consideration if it matches a specific type. */
template <class T, class U> struct disable_if_same { typedef void Type; };
template <class T> struct disable_if_same<T, T> {};

template <class T> void DeletePointer(void *p) { delete static_cast<T>(p); }

template <class T1, class T2>
struct FirstUnlessVoidOrBool {
  typedef T1 value;
};

template <class T2>
struct FirstUnlessVoidOrBool<void, T2> {
  typedef T2 value;
};

template <class T2>
struct FirstUnlessVoidOrBool<bool, T2> {
  typedef T2 value;
};

template<class T, class U>
struct is_same {
  static bool value;
};

template<class T>
struct is_same<T, T> {
  static bool value;
};

template<class T, class U>
bool is_same<T, U>::value = false;

template<class T>
bool is_same<T, T>::value = true;

/* FuncInfo *******************************************************************/

/* Info about the user's original, pre-wrapped function. */
template <class C, class R = void>
struct FuncInfo {
  /* The type of the closure that the function takes (its first param). */
  typedef C Closure;

  /* The return type. */
  typedef R Return;
};

/* Func ***********************************************************************/

/* Func1, Func2, Func3: Template classes representing a function and its
 * signature.
 *
 * Since the function is a template parameter, calling the function can be
 * inlined at compile-time and does not require a function pointer at runtime.
 * These functions are not bound to a handler data so have no data or cleanup
 * handler. */
struct UnboundFunc {
  CleanupFunc *GetCleanup() { return nullptr; }
  void *GetData() { return nullptr; }
};

template <class R, class P1, R F(P1), class I>
struct Func1 : public UnboundFunc {
  typedef R Return;
  typedef I FuncInfo;
  static R Call(P1 p1) { return F(p1); }
};

template <class R, class P1, class P2, R F(P1, P2), class I>
struct Func2 : public UnboundFunc {
  typedef R Return;
  typedef I FuncInfo;
  static R Call(P1 p1, P2 p2) { return F(p1, p2); }
};

template <class R, class P1, class P2, class P3, R F(P1, P2, P3), class I>
struct Func3 : public UnboundFunc {
  typedef R Return;
  typedef I FuncInfo;
  static R Call(P1 p1, P2 p2, P3 p3) { return F(p1, p2, p3); }
};

template <class R, class P1, class P2, class P3, class P4, R F(P1, P2, P3, P4),
          class I>
struct Func4 : public UnboundFunc {
  typedef R Return;
  typedef I FuncInfo;
  static R Call(P1 p1, P2 p2, P3 p3, P4 p4) { return F(p1, p2, p3, p4); }
};

template <class R, class P1, class P2, class P3, class P4, class P5,
          R F(P1, P2, P3, P4, P5), class I>
struct Func5 : public UnboundFunc {
  typedef R Return;
  typedef I FuncInfo;
  static R Call(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5) {
    return F(p1, p2, p3, p4, p5);
  }
};

/* BoundFunc ******************************************************************/

/* BoundFunc2, BoundFunc3: Like Func2/Func3 except also contains a value that
 * shall be bound to the function's second parameter.
 * 
 * Note that the second parameter is a const pointer, but our stored bound value
 * is non-const so we can free it when the handlers are destroyed. */
template <class T>
struct BoundFunc {
  typedef typename remove_constptr<T>::type MutableP2;
  explicit BoundFunc(MutableP2 data_) : data(data_) {}
  CleanupFunc *GetCleanup() { return &DeletePointer<MutableP2>; }
  MutableP2 GetData() { return data; }
  MutableP2 data;
};

template <class R, class P1, class P2, R F(P1, P2), class I>
struct BoundFunc2 : public BoundFunc<P2> {
  typedef BoundFunc<P2> Base;
  typedef I FuncInfo;
  explicit BoundFunc2(typename Base::MutableP2 arg) : Base(arg) {}
};

template <class R, class P1, class P2, class P3, R F(P1, P2, P3), class I>
struct BoundFunc3 : public BoundFunc<P2> {
  typedef BoundFunc<P2> Base;
  typedef I FuncInfo;
  explicit BoundFunc3(typename Base::MutableP2 arg) : Base(arg) {}
};

template <class R, class P1, class P2, class P3, class P4, R F(P1, P2, P3, P4),
          class I>
struct BoundFunc4 : public BoundFunc<P2> {
  typedef BoundFunc<P2> Base;
  typedef I FuncInfo;
  explicit BoundFunc4(typename Base::MutableP2 arg) : Base(arg) {}
};

template <class R, class P1, class P2, class P3, class P4, class P5,
          R F(P1, P2, P3, P4, P5), class I>
struct BoundFunc5 : public BoundFunc<P2> {
  typedef BoundFunc<P2> Base;
  typedef I FuncInfo;
  explicit BoundFunc5(typename Base::MutableP2 arg) : Base(arg) {}
};

/* FuncSig ********************************************************************/

/* FuncSig1, FuncSig2, FuncSig3: template classes reflecting a function
 * *signature*, but without a specific function attached.
 *
 * These classes contain member functions that can be invoked with a
 * specific function to return a Func/BoundFunc class. */
template <class R, class P1>
struct FuncSig1 {
  template <R F(P1)>
  Func1<R, P1, F, FuncInfo<P1, R> > GetFunc() {
    return Func1<R, P1, F, FuncInfo<P1, R> >();
  }
};

template <class R, class P1, class P2>
struct FuncSig2 {
  template <R F(P1, P2)>
  Func2<R, P1, P2, F, FuncInfo<P1, R> > GetFunc() {
    return Func2<R, P1, P2, F, FuncInfo<P1, R> >();
  }

  template <R F(P1, P2)>
  BoundFunc2<R, P1, P2, F, FuncInfo<P1, R> > GetFunc(
      typename remove_constptr<P2>::type param2) {
    return BoundFunc2<R, P1, P2, F, FuncInfo<P1, R> >(param2);
  }
};

template <class R, class P1, class P2, class P3>
struct FuncSig3 {
  template <R F(P1, P2, P3)>
  Func3<R, P1, P2, P3, F, FuncInfo<P1, R> > GetFunc() {
    return Func3<R, P1, P2, P3, F, FuncInfo<P1, R> >();
  }

  template <R F(P1, P2, P3)>
  BoundFunc3<R, P1, P2, P3, F, FuncInfo<P1, R> > GetFunc(
      typename remove_constptr<P2>::type param2) {
    return BoundFunc3<R, P1, P2, P3, F, FuncInfo<P1, R> >(param2);
  }
};

template <class R, class P1, class P2, class P3, class P4>
struct FuncSig4 {
  template <R F(P1, P2, P3, P4)>
  Func4<R, P1, P2, P3, P4, F, FuncInfo<P1, R> > GetFunc() {
    return Func4<R, P1, P2, P3, P4, F, FuncInfo<P1, R> >();
  }

  template <R F(P1, P2, P3, P4)>
  BoundFunc4<R, P1, P2, P3, P4, F, FuncInfo<P1, R> > GetFunc(
      typename remove_constptr<P2>::type param2) {
    return BoundFunc4<R, P1, P2, P3, P4, F, FuncInfo<P1, R> >(param2);
  }
};

template <class R, class P1, class P2, class P3, class P4, class P5>
struct FuncSig5 {
  template <R F(P1, P2, P3, P4, P5)>
  Func5<R, P1, P2, P3, P4, P5, F, FuncInfo<P1, R> > GetFunc() {
    return Func5<R, P1, P2, P3, P4, P5, F, FuncInfo<P1, R> >();
  }

  template <R F(P1, P2, P3, P4, P5)>
  BoundFunc5<R, P1, P2, P3, P4, P5, F, FuncInfo<P1, R> > GetFunc(
      typename remove_constptr<P2>::type param2) {
    return BoundFunc5<R, P1, P2, P3, P4, P5, F, FuncInfo<P1, R> >(param2);
  }
};

/* Overloaded template function that can construct the appropriate FuncSig*
 * class given a function pointer by deducing the template parameters. */
template <class R, class P1>
inline FuncSig1<R, P1> MatchFunc(R (*f)(P1)) {
  UPB_UNUSED(f);  /* Only used for template parameter deduction. */
  return FuncSig1<R, P1>();
}

template <class R, class P1, class P2>
inline FuncSig2<R, P1, P2> MatchFunc(R (*f)(P1, P2)) {
  UPB_UNUSED(f);  /* Only used for template parameter deduction. */
  return FuncSig2<R, P1, P2>();
}

template <class R, class P1, class P2, class P3>
inline FuncSig3<R, P1, P2, P3> MatchFunc(R (*f)(P1, P2, P3)) {
  UPB_UNUSED(f);  /* Only used for template parameter deduction. */
  return FuncSig3<R, P1, P2, P3>();
}

template <class R, class P1, class P2, class P3, class P4>
inline FuncSig4<R, P1, P2, P3, P4> MatchFunc(R (*f)(P1, P2, P3, P4)) {
  UPB_UNUSED(f);  /* Only used for template parameter deduction. */
  return FuncSig4<R, P1, P2, P3, P4>();
}

template <class R, class P1, class P2, class P3, class P4, class P5>
inline FuncSig5<R, P1, P2, P3, P4, P5> MatchFunc(R (*f)(P1, P2, P3, P4, P5)) {
  UPB_UNUSED(f);  /* Only used for template parameter deduction. */
  return FuncSig5<R, P1, P2, P3, P4, P5>();
}

/* MethodSig ******************************************************************/

/* CallMethod*: a function template that calls a given method. */
template <class R, class C, R (C::*F)()>
R CallMethod0(C *obj) {
  return ((*obj).*F)();
}

template <class R, class C, class P1, R (C::*F)(P1)>
R CallMethod1(C *obj, P1 arg1) {
  return ((*obj).*F)(arg1);
}

template <class R, class C, class P1, class P2, R (C::*F)(P1, P2)>
R CallMethod2(C *obj, P1 arg1, P2 arg2) {
  return ((*obj).*F)(arg1, arg2);
}

template <class R, class C, class P1, class P2, class P3, R (C::*F)(P1, P2, P3)>
R CallMethod3(C *obj, P1 arg1, P2 arg2, P3 arg3) {
  return ((*obj).*F)(arg1, arg2, arg3);
}

template <class R, class C, class P1, class P2, class P3, class P4,
          R (C::*F)(P1, P2, P3, P4)>
R CallMethod4(C *obj, P1 arg1, P2 arg2, P3 arg3, P4 arg4) {
  return ((*obj).*F)(arg1, arg2, arg3, arg4);
}

/* MethodSig: like FuncSig, but for member functions.
 *
 * GetFunc() returns a normal FuncN object, so after calling GetFunc() no
 * more logic is required to special-case methods. */
template <class R, class C>
struct MethodSig0 {
  template <R (C::*F)()>
  Func1<R, C *, CallMethod0<R, C, F>, FuncInfo<C *, R> > GetFunc() {
    return Func1<R, C *, CallMethod0<R, C, F>, FuncInfo<C *, R> >();
  }
};

template <class R, class C, class P1>
struct MethodSig1 {
  template <R (C::*F)(P1)>
  Func2<R, C *, P1, CallMethod1<R, C, P1, F>, FuncInfo<C *, R> > GetFunc() {
    return Func2<R, C *, P1, CallMethod1<R, C, P1, F>, FuncInfo<C *, R> >();
  }

  template <R (C::*F)(P1)>
  BoundFunc2<R, C *, P1, CallMethod1<R, C, P1, F>, FuncInfo<C *, R> > GetFunc(
      typename remove_constptr<P1>::type param1) {
    return BoundFunc2<R, C *, P1, CallMethod1<R, C, P1, F>, FuncInfo<C *, R> >(
        param1);
  }
};

template <class R, class C, class P1, class P2>
struct MethodSig2 {
  template <R (C::*F)(P1, P2)>
  Func3<R, C *, P1, P2, CallMethod2<R, C, P1, P2, F>, FuncInfo<C *, R> >
  GetFunc() {
    return Func3<R, C *, P1, P2, CallMethod2<R, C, P1, P2, F>,
                 FuncInfo<C *, R> >();
  }

  template <R (C::*F)(P1, P2)>
  BoundFunc3<R, C *, P1, P2, CallMethod2<R, C, P1, P2, F>, FuncInfo<C *, R> >
  GetFunc(typename remove_constptr<P1>::type param1) {
    return BoundFunc3<R, C *, P1, P2, CallMethod2<R, C, P1, P2, F>,
                      FuncInfo<C *, R> >(param1);
  }
};

template <class R, class C, class P1, class P2, class P3>
struct MethodSig3 {
  template <R (C::*F)(P1, P2, P3)>
  Func4<R, C *, P1, P2, P3, CallMethod3<R, C, P1, P2, P3, F>, FuncInfo<C *, R> >
  GetFunc() {
    return Func4<R, C *, P1, P2, P3, CallMethod3<R, C, P1, P2, P3, F>,
                 FuncInfo<C *, R> >();
  }

  template <R (C::*F)(P1, P2, P3)>
  BoundFunc4<R, C *, P1, P2, P3, CallMethod3<R, C, P1, P2, P3, F>,
             FuncInfo<C *, R> >
  GetFunc(typename remove_constptr<P1>::type param1) {
    return BoundFunc4<R, C *, P1, P2, P3, CallMethod3<R, C, P1, P2, P3, F>,
                      FuncInfo<C *, R> >(param1);
  }
};

template <class R, class C, class P1, class P2, class P3, class P4>
struct MethodSig4 {
  template <R (C::*F)(P1, P2, P3, P4)>
  Func5<R, C *, P1, P2, P3, P4, CallMethod4<R, C, P1, P2, P3, P4, F>,
        FuncInfo<C *, R> >
  GetFunc() {
    return Func5<R, C *, P1, P2, P3, P4, CallMethod4<R, C, P1, P2, P3, P4, F>,
                 FuncInfo<C *, R> >();
  }

  template <R (C::*F)(P1, P2, P3, P4)>
  BoundFunc5<R, C *, P1, P2, P3, P4, CallMethod4<R, C, P1, P2, P3, P4, F>,
             FuncInfo<C *, R> >
  GetFunc(typename remove_constptr<P1>::type param1) {
    return BoundFunc5<R, C *, P1, P2, P3, P4,
                      CallMethod4<R, C, P1, P2, P3, P4, F>, FuncInfo<C *, R> >(
        param1);
  }
};

template <class R, class C>
inline MethodSig0<R, C> MatchFunc(R (C::*f)()) {
  UPB_UNUSED(f);  /* Only used for template parameter deduction. */
  return MethodSig0<R, C>();
}

template <class R, class C, class P1>
inline MethodSig1<R, C, P1> MatchFunc(R (C::*f)(P1)) {
  UPB_UNUSED(f);  /* Only used for template parameter deduction. */
  return MethodSig1<R, C, P1>();
}

template <class R, class C, class P1, class P2>
inline MethodSig2<R, C, P1, P2> MatchFunc(R (C::*f)(P1, P2)) {
  UPB_UNUSED(f);  /* Only used for template parameter deduction. */
  return MethodSig2<R, C, P1, P2>();
}

template <class R, class C, class P1, class P2, class P3>
inline MethodSig3<R, C, P1, P2, P3> MatchFunc(R (C::*f)(P1, P2, P3)) {
  UPB_UNUSED(f);  /* Only used for template parameter deduction. */
  return MethodSig3<R, C, P1, P2, P3>();
}

template <class R, class C, class P1, class P2, class P3, class P4>
inline MethodSig4<R, C, P1, P2, P3, P4> MatchFunc(R (C::*f)(P1, P2, P3, P4)) {
  UPB_UNUSED(f);  /* Only used for template parameter deduction. */
  return MethodSig4<R, C, P1, P2, P3, P4>();
}

/* MaybeWrapReturn ************************************************************/

/* Template class that attempts to wrap the return value of the function so it
 * matches the expected type.  There are two main adjustments it may make:
 *
 *   1. If the function returns void, make it return the expected type and with
 *      a value that always indicates success.
 *   2. If the function returns bool, make it return the expected type with a
 *      value that indicates success or failure.
 *
 * The "expected type" for return is:
 *   1. void* for start handlers.  If the closure parameter has a different type
 *      we will cast it to void* for the return in the success case.
 *   2. size_t for string buffer handlers.
 *   3. bool for everything else. */

/* Template parameters are FuncN type and desired return type. */
template <class F, class R, class Enable = void>
struct MaybeWrapReturn;

/* If the return type matches, return the given function unwrapped. */
template <class F>
struct MaybeWrapReturn<F, typename F::Return> {
  typedef F Func;
};

/* Function wrapper that munges the return value from void to (bool)true. */
template <class P1, class P2, void F(P1, P2)>
bool ReturnTrue2(P1 p1, P2 p2) {
  F(p1, p2);
  return true;
}

template <class P1, class P2, class P3, void F(P1, P2, P3)>
bool ReturnTrue3(P1 p1, P2 p2, P3 p3) {
  F(p1, p2, p3);
  return true;
}

/* Function wrapper that munges the return value from void to (void*)arg1  */
template <class P1, class P2, void F(P1, P2)>
void *ReturnClosure2(P1 p1, P2 p2) {
  F(p1, p2);
  return p1;
}

template <class P1, class P2, class P3, void F(P1, P2, P3)>
void *ReturnClosure3(P1 p1, P2 p2, P3 p3) {
  F(p1, p2, p3);
  return p1;
}

/* Function wrapper that munges the return value from R to void*. */
template <class R, class P1, class P2, R F(P1, P2)>
void *CastReturnToVoidPtr2(P1 p1, P2 p2) {
  return F(p1, p2);
}

template <class R, class P1, class P2, class P3, R F(P1, P2, P3)>
void *CastReturnToVoidPtr3(P1 p1, P2 p2, P3 p3) {
  return F(p1, p2, p3);
}

/* Function wrapper that munges the return value from bool to void*. */
template <class P1, class P2, bool F(P1, P2)>
void *ReturnClosureOrBreak2(P1 p1, P2 p2) {
  return F(p1, p2) ? p1 : UPB_BREAK;
}

template <class P1, class P2, class P3, bool F(P1, P2, P3)>
void *ReturnClosureOrBreak3(P1 p1, P2 p2, P3 p3) {
  return F(p1, p2, p3) ? p1 : UPB_BREAK;
}

/* For the string callback, which takes five params, returns the size param. */
template <class P1, class P2,
          void F(P1, P2, const char *, size_t, const upb_bufhandle *)>
size_t ReturnStringLen(P1 p1, P2 p2, const char *p3, size_t p4,
                       const upb_bufhandle *p5) {
  F(p1, p2, p3, p4, p5);
  return p4;
}

/* For the string callback, which takes five params, returns the size param or
 * zero. */
template <class P1, class P2,
          bool F(P1, P2, const char *, size_t, const upb_bufhandle *)>
size_t ReturnNOr0(P1 p1, P2 p2, const char *p3, size_t p4,
                  const upb_bufhandle *p5) {
  return F(p1, p2, p3, p4, p5) ? p4 : 0;
}

/* If we have a function returning void but want a function returning bool, wrap
 * it in a function that returns true. */
template <class P1, class P2, void F(P1, P2), class I>
struct MaybeWrapReturn<Func2<void, P1, P2, F, I>, bool> {
  typedef Func2<bool, P1, P2, ReturnTrue2<P1, P2, F>, I> Func;
};

template <class P1, class P2, class P3, void F(P1, P2, P3), class I>
struct MaybeWrapReturn<Func3<void, P1, P2, P3, F, I>, bool> {
  typedef Func3<bool, P1, P2, P3, ReturnTrue3<P1, P2, P3, F>, I> Func;
};

/* If our function returns void but we want one returning void*, wrap it in a
 * function that returns the first argument. */
template <class P1, class P2, void F(P1, P2), class I>
struct MaybeWrapReturn<Func2<void, P1, P2, F, I>, void *> {
  typedef Func2<void *, P1, P2, ReturnClosure2<P1, P2, F>, I> Func;
};

template <class P1, class P2, class P3, void F(P1, P2, P3), class I>
struct MaybeWrapReturn<Func3<void, P1, P2, P3, F, I>, void *> {
  typedef Func3<void *, P1, P2, P3, ReturnClosure3<P1, P2, P3, F>, I> Func;
};

/* If our function returns R* but we want one returning void*, wrap it in a
 * function that casts to void*. */
template <class R, class P1, class P2, R *F(P1, P2), class I>
struct MaybeWrapReturn<Func2<R *, P1, P2, F, I>, void *,
                       typename disable_if_same<R *, void *>::Type> {
  typedef Func2<void *, P1, P2, CastReturnToVoidPtr2<R *, P1, P2, F>, I> Func;
};

template <class R, class P1, class P2, class P3, R *F(P1, P2, P3), class I>
struct MaybeWrapReturn<Func3<R *, P1, P2, P3, F, I>, void *,
                       typename disable_if_same<R *, void *>::Type> {
  typedef Func3<void *, P1, P2, P3, CastReturnToVoidPtr3<R *, P1, P2, P3, F>, I>
      Func;
};

/* If our function returns bool but we want one returning void*, wrap it in a
 * function that returns either the first param or UPB_BREAK. */
template <class P1, class P2, bool F(P1, P2), class I>
struct MaybeWrapReturn<Func2<bool, P1, P2, F, I>, void *> {
  typedef Func2<void *, P1, P2, ReturnClosureOrBreak2<P1, P2, F>, I> Func;
};

template <class P1, class P2, class P3, bool F(P1, P2, P3), class I>
struct MaybeWrapReturn<Func3<bool, P1, P2, P3, F, I>, void *> {
  typedef Func3<void *, P1, P2, P3, ReturnClosureOrBreak3<P1, P2, P3, F>, I>
      Func;
};

/* If our function returns void but we want one returning size_t, wrap it in a
 * function that returns the size argument. */
template <class P1, class P2,
          void F(P1, P2, const char *, size_t, const upb_bufhandle *), class I>
struct MaybeWrapReturn<
    Func5<void, P1, P2, const char *, size_t, const upb_bufhandle *, F, I>,
          size_t> {
  typedef Func5<size_t, P1, P2, const char *, size_t, const upb_bufhandle *,
                ReturnStringLen<P1, P2, F>, I> Func;
};

/* If our function returns bool but we want one returning size_t, wrap it in a
 * function that returns either 0 or the buf size. */
template <class P1, class P2,
          bool F(P1, P2, const char *, size_t, const upb_bufhandle *), class I>
struct MaybeWrapReturn<
    Func5<bool, P1, P2, const char *, size_t, const upb_bufhandle *, F, I>,
    size_t> {
  typedef Func5<size_t, P1, P2, const char *, size_t, const upb_bufhandle *,
                ReturnNOr0<P1, P2, F>, I> Func;
};

/* ConvertParams **************************************************************/

/* Template class that converts the function parameters if necessary, and
 * ignores the HandlerData parameter if appropriate.
 *
 * Template parameter is the are FuncN function type. */
template <class F, class T>
struct ConvertParams;

/* Function that discards the handler data parameter. */
template <class R, class P1, R F(P1)>
R IgnoreHandlerData2(void *p1, const void *hd) {
  UPB_UNUSED(hd);
  return F(static_cast<P1>(p1));
}

template <class R, class P1, class P2Wrapper, class P2Wrapped,
          R F(P1, P2Wrapped)>
R IgnoreHandlerData3(void *p1, const void *hd, P2Wrapper p2) {
  UPB_UNUSED(hd);
  return F(static_cast<P1>(p1), p2);
}

template <class R, class P1, class P2, class P3, R F(P1, P2, P3)>
R IgnoreHandlerData4(void *p1, const void *hd, P2 p2, P3 p3) {
  UPB_UNUSED(hd);
  return F(static_cast<P1>(p1), p2, p3);
}

template <class R, class P1, class P2, class P3, class P4, R F(P1, P2, P3, P4)>
R IgnoreHandlerData5(void *p1, const void *hd, P2 p2, P3 p3, P4 p4) {
  UPB_UNUSED(hd);
  return F(static_cast<P1>(p1), p2, p3, p4);
}

template <class R, class P1, R F(P1, const char*, size_t)>
R IgnoreHandlerDataIgnoreHandle(void *p1, const void *hd, const char *p2,
                                size_t p3, const upb_bufhandle *handle) {
  UPB_UNUSED(hd);
  UPB_UNUSED(handle);
  return F(static_cast<P1>(p1), p2, p3);
}

/* Function that casts the handler data parameter. */
template <class R, class P1, class P2, R F(P1, P2)>
R CastHandlerData2(void *c, const void *hd) {
  return F(static_cast<P1>(c), static_cast<P2>(hd));
}

template <class R, class P1, class P2, class P3Wrapper, class P3Wrapped,
          R F(P1, P2, P3Wrapped)>
R CastHandlerData3(void *c, const void *hd, P3Wrapper p3) {
  return F(static_cast<P1>(c), static_cast<P2>(hd), p3);
}

template <class R, class P1, class P2, class P3, class P4, class P5,
          R F(P1, P2, P3, P4, P5)>
R CastHandlerData5(void *c, const void *hd, P3 p3, P4 p4, P5 p5) {
  return F(static_cast<P1>(c), static_cast<P2>(hd), p3, p4, p5);
}

template <class R, class P1, class P2, R F(P1, P2, const char *, size_t)>
R CastHandlerDataIgnoreHandle(void *c, const void *hd, const char *p3,
                              size_t p4, const upb_bufhandle *handle) {
  UPB_UNUSED(handle);
  return F(static_cast<P1>(c), static_cast<P2>(hd), p3, p4);
}

/* For unbound functions, ignore the handler data. */
template <class R, class P1, R F(P1), class I, class T>
struct ConvertParams<Func1<R, P1, F, I>, T> {
  typedef Func2<R, void *, const void *, IgnoreHandlerData2<R, P1, F>, I> Func;
};

template <class R, class P1, class P2, R F(P1, P2), class I,
          class R2, class P1_2, class P2_2, class P3_2>
struct ConvertParams<Func2<R, P1, P2, F, I>,
                     R2 (*)(P1_2, P2_2, P3_2)> {
  typedef Func3<R, void *, const void *, P3_2,
                IgnoreHandlerData3<R, P1, P3_2, P2, F>, I> Func;
};

/* For StringBuffer only; this ignores both the handler data and the
 * upb_bufhandle. */
template <class R, class P1, R F(P1, const char *, size_t), class I, class T>
struct ConvertParams<Func3<R, P1, const char *, size_t, F, I>, T> {
  typedef Func5<R, void *, const void *, const char *, size_t,
                const upb_bufhandle *, IgnoreHandlerDataIgnoreHandle<R, P1, F>,
                I> Func;
};

template <class R, class P1, class P2, class P3, class P4, R F(P1, P2, P3, P4),
          class I, class T>
struct ConvertParams<Func4<R, P1, P2, P3, P4, F, I>, T> {
  typedef Func5<R, void *, const void *, P2, P3, P4,
                IgnoreHandlerData5<R, P1, P2, P3, P4, F>, I> Func;
};

/* For bound functions, cast the handler data. */
template <class R, class P1, class P2, R F(P1, P2), class I, class T>
struct ConvertParams<BoundFunc2<R, P1, P2, F, I>, T> {
  typedef Func2<R, void *, const void *, CastHandlerData2<R, P1, P2, F>, I>
      Func;
};

template <class R, class P1, class P2, class P3, R F(P1, P2, P3), class I,
          class R2, class P1_2, class P2_2, class P3_2>
struct ConvertParams<BoundFunc3<R, P1, P2, P3, F, I>,
                     R2 (*)(P1_2, P2_2, P3_2)> {
  typedef Func3<R, void *, const void *, P3_2,
                CastHandlerData3<R, P1, P2, P3_2, P3, F>, I> Func;
};

/* For StringBuffer only; this ignores the upb_bufhandle. */
template <class R, class P1, class P2, R F(P1, P2, const char *, size_t),
          class I, class T>
struct ConvertParams<BoundFunc4<R, P1, P2, const char *, size_t, F, I>, T> {
  typedef Func5<R, void *, const void *, const char *, size_t,
                const upb_bufhandle *,
                CastHandlerDataIgnoreHandle<R, P1, P2, F>, I>
      Func;
};

template <class R, class P1, class P2, class P3, class P4, class P5,
          R F(P1, P2, P3, P4, P5), class I, class T>
struct ConvertParams<BoundFunc5<R, P1, P2, P3, P4, P5, F, I>, T> {
  typedef Func5<R, void *, const void *, P3, P4, P5,
                CastHandlerData5<R, P1, P2, P3, P4, P5, F>, I> Func;
};

/* utype/ltype are upper/lower-case, ctype is canonical C type, vtype is
 * variant C type. */
#define TYPE_METHODS(utype, ltype, ctype, vtype)                      \
  template <>                                                         \
  struct CanonicalType<vtype> {                                       \
    typedef ctype Type;                                               \
  };                                                                  \
  template <>                                                         \
  inline bool HandlersPtr::SetValueHandler<vtype>(                    \
      FieldDefPtr f, const HandlersPtr::utype##Handler &handler) {    \
    handler.AddCleanup(ptr());                                        \
    return upb_handlers_set##ltype(ptr(), f.ptr(), handler.handler(), \
                                   &handler.attr());                  \
  }

TYPE_METHODS(Double, double, double,   double)
TYPE_METHODS(Float,  float,  float,    float)
TYPE_METHODS(UInt64, uint64, uint64_t, UPB_UINT64_T)
TYPE_METHODS(UInt32, uint32, uint32_t, UPB_UINT32_T)
TYPE_METHODS(Int64,  int64,  int64_t,  UPB_INT64_T)
TYPE_METHODS(Int32,  int32,  int32_t,  UPB_INT32_T)
TYPE_METHODS(Bool,   bool,   bool,     bool)

#ifdef UPB_TWO_32BIT_TYPES
TYPE_METHODS(Int32,  int32,  int32_t,  UPB_INT32ALT_T)
TYPE_METHODS(UInt32, uint32, uint32_t, UPB_UINT32ALT_T)
#endif

#ifdef UPB_TWO_64BIT_TYPES
TYPE_METHODS(Int64,  int64,  int64_t,  UPB_INT64ALT_T)
TYPE_METHODS(UInt64, uint64, uint64_t, UPB_UINT64ALT_T)
#endif
#undef TYPE_METHODS

template <> struct CanonicalType<Status*> {
  typedef Status* Type;
};

template <class F> struct ReturnOf;

template <class R, class P1, class P2>
struct ReturnOf<R (*)(P1, P2)> {
  typedef R Return;
};

template <class R, class P1, class P2, class P3>
struct ReturnOf<R (*)(P1, P2, P3)> {
  typedef R Return;
};

template <class R, class P1, class P2, class P3, class P4>
struct ReturnOf<R (*)(P1, P2, P3, P4)> {
  typedef R Return;
};

template <class R, class P1, class P2, class P3, class P4, class P5>
struct ReturnOf<R (*)(P1, P2, P3, P4, P5)> {
  typedef R Return;
};


template <class T>
template <class F>
inline Handler<T>::Handler(F func)
    : registered_(false),
      cleanup_data_(func.GetData()),
      cleanup_func_(func.GetCleanup()) {
  attr_.handler_data = func.GetData();
  typedef typename ReturnOf<T>::Return Return;
  typedef typename ConvertParams<F, T>::Func ConvertedParamsFunc;
  typedef typename MaybeWrapReturn<ConvertedParamsFunc, Return>::Func
      ReturnWrappedFunc;
  handler_ = ReturnWrappedFunc().Call;

  /* Set attributes based on what templates can statically tell us about the
   * user's function. */

  /* If the original function returns void, then we know that we wrapped it to
   * always return ok. */
  bool always_ok = is_same<typename F::FuncInfo::Return, void>::value;
  attr_.alwaysok = always_ok;

  /* Closure parameter and return type. */
  attr_.closure_type = UniquePtrForType<typename F::FuncInfo::Closure>();

  /* We use the closure type (from the first parameter) if the return type is
   * void or bool, since these are the two cases we wrap to return the closure's
   * type anyway.
   *
   * This is all nonsense for non START* handlers, but it doesn't matter because
   * in that case the value will be ignored. */
  typedef typename FirstUnlessVoidOrBool<typename F::FuncInfo::Return,
                                         typename F::FuncInfo::Closure>::value
      EffectiveReturn;
  attr_.return_closure_type = UniquePtrForType<EffectiveReturn>();
}

template <class T>
inline void Handler<T>::AddCleanup(upb_handlers* h) const {
  UPB_ASSERT(!registered_);
  registered_ = true;
  if (cleanup_func_) {
    bool ok = upb_handlers_addcleanup(h, cleanup_data_, cleanup_func_);
    UPB_ASSERT(ok);
  }
}

}  /* namespace upb */

#endif  /* __cplusplus */


#undef UPB_TWO_32BIT_TYPES
#undef UPB_TWO_64BIT_TYPES
#undef UPB_INT32_T
#undef UPB_UINT32_T
#undef UPB_INT32ALT_T
#undef UPB_UINT32ALT_T
#undef UPB_INT64_T
#undef UPB_UINT64_T
#undef UPB_INT64ALT_T
#undef UPB_UINT64ALT_T


#endif  /* UPB_HANDLERS_INL_H_ */

#endif  /* UPB_HANDLERS_H */
/*
** upb::Sink (upb_sink)
** upb::BytesSink (upb_bytessink)
**
** A upb_sink is an object that binds a upb_handlers object to some runtime
** state.  It is the object that can actually receive data via the upb_handlers
** interface.
**
** Unlike upb_def and upb_handlers, upb_sink is never frozen, immutable, or
** thread-safe.  You can create as many of them as you want, but each one may
** only be used in a single thread at a time.
**
** If we compare with class-based OOP, a you can think of a upb_def as an
** abstract base class, a upb_handlers as a concrete derived class, and a
** upb_sink as an object (class instance).
*/

#ifndef UPB_SINK_H
#define UPB_SINK_H



#ifdef __cplusplus
namespace upb {
class BytesSink;
class Sink;
}
#endif

/* upb_sink *******************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  const upb_handlers *handlers;
  void *closure;
} upb_sink;

#define PUTVAL(type, ctype)                                           \
  UPB_INLINE bool upb_sink_put##type(upb_sink s, upb_selector_t sel,  \
                                     ctype val) {                     \
    typedef upb_##type##_handlerfunc functype;                        \
    functype *func;                                                   \
    const void *hd;                                                   \
    if (!s.handlers) return true;                                     \
    func = (functype *)upb_handlers_gethandler(s.handlers, sel, &hd); \
    if (!func) return true;                                           \
    return func(s.closure, hd, val);                                  \
  }

PUTVAL(int32,  int32_t)
PUTVAL(int64,  int64_t)
PUTVAL(uint32, uint32_t)
PUTVAL(uint64, uint64_t)
PUTVAL(float,  float)
PUTVAL(double, double)
PUTVAL(bool,   bool)
#undef PUTVAL

UPB_INLINE void upb_sink_reset(upb_sink *s, const upb_handlers *h, void *c) {
  s->handlers = h;
  s->closure = c;
}

UPB_INLINE size_t upb_sink_putstring(upb_sink s, upb_selector_t sel,
                                     const char *buf, size_t n,
                                     const upb_bufhandle *handle) {
  typedef upb_string_handlerfunc func;
  func *handler;
  const void *hd;
  if (!s.handlers) return n;
  handler = (func *)upb_handlers_gethandler(s.handlers, sel, &hd);

  if (!handler) return n;
  return handler(s.closure, hd, buf, n, handle);
}

UPB_INLINE bool upb_sink_putunknown(upb_sink s, const char *buf, size_t n) {
  typedef upb_unknown_handlerfunc func;
  func *handler;
  const void *hd;
  if (!s.handlers) return true;
  handler =
      (func *)upb_handlers_gethandler(s.handlers, UPB_UNKNOWN_SELECTOR, &hd);

  if (!handler) return n;
  return handler(s.closure, hd, buf, n);
}

UPB_INLINE bool upb_sink_startmsg(upb_sink s) {
  typedef upb_startmsg_handlerfunc func;
  func *startmsg;
  const void *hd;
  if (!s.handlers) return true;
  startmsg =
      (func *)upb_handlers_gethandler(s.handlers, UPB_STARTMSG_SELECTOR, &hd);

  if (!startmsg) return true;
  return startmsg(s.closure, hd);
}

UPB_INLINE bool upb_sink_endmsg(upb_sink s, upb_status *status) {
  typedef upb_endmsg_handlerfunc func;
  func *endmsg;
  const void *hd;
  if (!s.handlers) return true;
  endmsg =
      (func *)upb_handlers_gethandler(s.handlers, UPB_ENDMSG_SELECTOR, &hd);

  if (!endmsg) return true;
  return endmsg(s.closure, hd, status);
}

UPB_INLINE bool upb_sink_startseq(upb_sink s, upb_selector_t sel,
                                  upb_sink *sub) {
  typedef upb_startfield_handlerfunc func;
  func *startseq;
  const void *hd;
  sub->closure = s.closure;
  sub->handlers = s.handlers;
  if (!s.handlers) return true;
  startseq = (func*)upb_handlers_gethandler(s.handlers, sel, &hd);

  if (!startseq) return true;
  sub->closure = startseq(s.closure, hd);
  return sub->closure ? true : false;
}

UPB_INLINE bool upb_sink_endseq(upb_sink s, upb_selector_t sel) {
  typedef upb_endfield_handlerfunc func;
  func *endseq;
  const void *hd;
  if (!s.handlers) return true;
  endseq = (func*)upb_handlers_gethandler(s.handlers, sel, &hd);

  if (!endseq) return true;
  return endseq(s.closure, hd);
}

UPB_INLINE bool upb_sink_startstr(upb_sink s, upb_selector_t sel,
                                  size_t size_hint, upb_sink *sub) {
  typedef upb_startstr_handlerfunc func;
  func *startstr;
  const void *hd;
  sub->closure = s.closure;
  sub->handlers = s.handlers;
  if (!s.handlers) return true;
  startstr = (func*)upb_handlers_gethandler(s.handlers, sel, &hd);

  if (!startstr) return true;
  sub->closure = startstr(s.closure, hd, size_hint);
  return sub->closure ? true : false;
}

UPB_INLINE bool upb_sink_endstr(upb_sink s, upb_selector_t sel) {
  typedef upb_endfield_handlerfunc func;
  func *endstr;
  const void *hd;
  if (!s.handlers) return true;
  endstr = (func*)upb_handlers_gethandler(s.handlers, sel, &hd);

  if (!endstr) return true;
  return endstr(s.closure, hd);
}

UPB_INLINE bool upb_sink_startsubmsg(upb_sink s, upb_selector_t sel,
                                     upb_sink *sub) {
  typedef upb_startfield_handlerfunc func;
  func *startsubmsg;
  const void *hd;
  sub->closure = s.closure;
  if (!s.handlers) {
    sub->handlers = NULL;
    return true;
  }
  sub->handlers = upb_handlers_getsubhandlers_sel(s.handlers, sel);
  startsubmsg = (func*)upb_handlers_gethandler(s.handlers, sel, &hd);

  if (!startsubmsg) return true;
  sub->closure = startsubmsg(s.closure, hd);
  return sub->closure ? true : false;
}

UPB_INLINE bool upb_sink_endsubmsg(upb_sink s, upb_selector_t sel) {
  typedef upb_endfield_handlerfunc func;
  func *endsubmsg;
  const void *hd;
  if (!s.handlers) return true;
  endsubmsg = (func*)upb_handlers_gethandler(s.handlers, sel, &hd);

  if (!endsubmsg) return s.closure;
  return endsubmsg(s.closure, hd);
}

#ifdef __cplusplus
}  /* extern "C" */

/* A upb::Sink is an object that binds a upb::Handlers object to some runtime
 * state.  It represents an endpoint to which data can be sent.
 *
 * TODO(haberman): right now all of these functions take selectors.  Should they
 * take selectorbase instead?
 *
 * ie. instead of calling:
 *   sink->StartString(FOO_FIELD_START_STRING, ...)
 * a selector base would let you say:
 *   sink->StartString(FOO_FIELD, ...)
 *
 * This would make call sites a little nicer and require emitting fewer selector
 * definitions in .h files.
 *
 * But the current scheme has the benefit that you can retrieve a function
 * pointer for any handler with handlers->GetHandler(selector), without having
 * to have a separate GetHandler() function for each handler type.  The JIT
 * compiler uses this.  To accommodate we'd have to expose a separate
 * GetHandler() for every handler type.
 *
 * Also to ponder: selectors right now are independent of a specific Handlers
 * instance.  In other words, they allocate a number to every possible handler
 * that *could* be registered, without knowing anything about what handlers
 * *are* registered.  That means that using selectors as table offsets prohibits
 * us from compacting the handler table at Freeze() time.  If the table is very
 * sparse, this could be wasteful.
 *
 * Having another selector-like thing that is specific to a Handlers instance
 * would allow this compacting, but then it would be impossible to write code
 * ahead-of-time that can be bound to any Handlers instance at runtime.  For
 * example, a .proto file parser written as straight C will not know what
 * Handlers it will be bound to, so when it calls sink->StartString() what
 * selector will it pass?  It needs a selector like we have today, that is
 * independent of any particular upb::Handlers.
 *
 * Is there a way then to allow Handlers table compaction? */
class upb::Sink {
 public:
  /* Constructor with no initialization; must be Reset() before use. */
  Sink() {}

  Sink(const Sink&) = default;
  Sink& operator=(const Sink&) = default;

  Sink(const upb_sink& sink) : sink_(sink) {}
  Sink &operator=(const upb_sink &sink) {
    sink_ = sink;
    return *this;
  }

  upb_sink sink() { return sink_; }

  /* Constructs a new sink for the given frozen handlers and closure.
   *
   * TODO: once the Handlers know the expected closure type, verify that T
   * matches it. */
  template <class T> Sink(const upb_handlers* handlers, T* closure) {
    Reset(handlers, closure);
  }

  upb_sink* ptr() { return &sink_; }

  /* Resets the value of the sink. */
  template <class T> void Reset(const upb_handlers* handlers, T* closure) {
    upb_sink_reset(&sink_, handlers, closure);
  }

  /* Returns the top-level object that is bound to this sink.
   *
   * TODO: once the Handlers know the expected closure type, verify that T
   * matches it. */
  template <class T> T* GetObject() const {
    return static_cast<T*>(sink_.closure);
  }

  /* Functions for pushing data into the sink.
   *
   * These return false if processing should stop (either due to error or just
   * to suspend).
   *
   * These may not be called from within one of the same sink's handlers (in
   * other words, handlers are not re-entrant). */

  /* Should be called at the start and end of every message; both the top-level
   * message and submessages.  This means that submessages should use the
   * following sequence:
   *   sink->StartSubMessage(startsubmsg_selector);
   *   sink->StartMessage();
   *   // ...
   *   sink->EndMessage(&status);
   *   sink->EndSubMessage(endsubmsg_selector); */
  bool StartMessage() { return upb_sink_startmsg(sink_); }
  bool EndMessage(upb_status *status) {
    return upb_sink_endmsg(sink_, status);
  }

  /* Putting of individual values.  These work for both repeated and
   * non-repeated fields, but for repeated fields you must wrap them in
   * calls to StartSequence()/EndSequence(). */
  bool PutInt32(HandlersPtr::Selector s, int32_t val) {
    return upb_sink_putint32(sink_, s, val);
  }

  bool PutInt64(HandlersPtr::Selector s, int64_t val) {
    return upb_sink_putint64(sink_, s, val);
  }

  bool PutUInt32(HandlersPtr::Selector s, uint32_t val) {
    return upb_sink_putuint32(sink_, s, val);
  }

  bool PutUInt64(HandlersPtr::Selector s, uint64_t val) {
    return upb_sink_putuint64(sink_, s, val);
  }

  bool PutFloat(HandlersPtr::Selector s, float val) {
    return upb_sink_putfloat(sink_, s, val);
  }

  bool PutDouble(HandlersPtr::Selector s, double val) {
    return upb_sink_putdouble(sink_, s, val);
  }

  bool PutBool(HandlersPtr::Selector s, bool val) {
    return upb_sink_putbool(sink_, s, val);
  }

  /* Putting of string/bytes values.  Each string can consist of zero or more
   * non-contiguous buffers of data.
   *
   * For StartString(), the function will write a sink for the string to "sub."
   * The sub-sink must be used for any/all PutStringBuffer() calls. */
  bool StartString(HandlersPtr::Selector s, size_t size_hint, Sink* sub) {
    upb_sink sub_c;
    bool ret = upb_sink_startstr(sink_, s, size_hint, &sub_c);
    *sub = sub_c;
    return ret;
  }

  size_t PutStringBuffer(HandlersPtr::Selector s, const char *buf, size_t len,
                         const upb_bufhandle *handle) {
    return upb_sink_putstring(sink_, s, buf, len, handle);
  }

  bool EndString(HandlersPtr::Selector s) {
    return upb_sink_endstr(sink_, s);
  }

  /* For submessage fields.
   *
   * For StartSubMessage(), the function will write a sink for the string to
   * "sub." The sub-sink must be used for any/all handlers called within the
   * submessage. */
  bool StartSubMessage(HandlersPtr::Selector s, Sink* sub) {
    upb_sink sub_c;
    bool ret = upb_sink_startsubmsg(sink_, s, &sub_c);
    *sub = sub_c;
    return ret;
  }

  bool EndSubMessage(HandlersPtr::Selector s) {
    return upb_sink_endsubmsg(sink_, s);
  }

  /* For repeated fields of any type, the sequence of values must be wrapped in
   * these calls.
   *
   * For StartSequence(), the function will write a sink for the string to
   * "sub." The sub-sink must be used for any/all handlers called within the
   * sequence. */
  bool StartSequence(HandlersPtr::Selector s, Sink* sub) {
    upb_sink sub_c;
    bool ret = upb_sink_startseq(sink_, s, &sub_c);
    *sub = sub_c;
    return ret;
  }

  bool EndSequence(HandlersPtr::Selector s) {
    return upb_sink_endseq(sink_, s);
  }

  /* Copy and assign specifically allowed.
   * We don't even bother making these members private because so many
   * functions need them and this is mainly just a dumb data container anyway.
   */

 private:
  upb_sink sink_;
};

#endif  /* __cplusplus */

/* upb_bytessink **************************************************************/

typedef struct {
  const upb_byteshandler *handler;
  void *closure;
} upb_bytessink ;

UPB_INLINE void upb_bytessink_reset(upb_bytessink* s, const upb_byteshandler *h,
                                    void *closure) {
  s->handler = h;
  s->closure = closure;
}

UPB_INLINE bool upb_bytessink_start(upb_bytessink s, size_t size_hint,
                                    void **subc) {
  typedef upb_startstr_handlerfunc func;
  func *start;
  *subc = s.closure;
  if (!s.handler) return true;
  start = (func *)s.handler->table[UPB_STARTSTR_SELECTOR].func;

  if (!start) return true;
  *subc = start(s.closure,
                s.handler->table[UPB_STARTSTR_SELECTOR].attr.handler_data,
                size_hint);
  return *subc != NULL;
}

UPB_INLINE size_t upb_bytessink_putbuf(upb_bytessink s, void *subc,
                                       const char *buf, size_t size,
                                       const upb_bufhandle* handle) {
  typedef upb_string_handlerfunc func;
  func *putbuf;
  if (!s.handler) return true;
  putbuf = (func *)s.handler->table[UPB_STRING_SELECTOR].func;

  if (!putbuf) return true;
  return putbuf(subc, s.handler->table[UPB_STRING_SELECTOR].attr.handler_data,
                buf, size, handle);
}

UPB_INLINE bool upb_bytessink_end(upb_bytessink s) {
  typedef upb_endfield_handlerfunc func;
  func *end;
  if (!s.handler) return true;
  end = (func *)s.handler->table[UPB_ENDSTR_SELECTOR].func;

  if (!end) return true;
  return end(s.closure,
             s.handler->table[UPB_ENDSTR_SELECTOR].attr.handler_data);
}

#ifdef __cplusplus

class upb::BytesSink {
 public:
  BytesSink() {}

  BytesSink(const BytesSink&) = default;
  BytesSink& operator=(const BytesSink&) = default;

  BytesSink(const upb_bytessink& sink) : sink_(sink) {}
  BytesSink &operator=(const upb_bytessink &sink) {
    sink_ = sink;
    return *this;
  }

  upb_bytessink sink() { return sink_; }

  /* Constructs a new sink for the given frozen handlers and closure.
   *
   * TODO(haberman): once the Handlers know the expected closure type, verify
   * that T matches it. */
  template <class T> BytesSink(const upb_byteshandler* handler, T* closure) {
    upb_bytessink_reset(sink_, handler, closure);
  }

  /* Resets the value of the sink. */
  template <class T> void Reset(const upb_byteshandler* handler, T* closure) {
    upb_bytessink_reset(&sink_, handler, closure);
  }

  bool Start(size_t size_hint, void **subc) {
    return upb_bytessink_start(sink_, size_hint, subc);
  }

  size_t PutBuffer(void *subc, const char *buf, size_t len,
                   const upb_bufhandle *handle) {
    return upb_bytessink_putbuf(sink_, subc, buf, len, handle);
  }

  bool End() {
    return upb_bytessink_end(sink_);
  }

 private:
  upb_bytessink sink_;
};

#endif  /* __cplusplus */

/* upb_bufsrc *****************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

bool upb_bufsrc_putbuf(const char *buf, size_t len, upb_bytessink sink);

#ifdef __cplusplus
}  /* extern "C" */

namespace upb {
template <class T> bool PutBuffer(const T& str, BytesSink sink) {
  return upb_bufsrc_putbuf(str.data(), str.size(), sink.sink());
}
}

#endif  /* __cplusplus */


#endif
/*
** Internal-only definitions for the decoder.
*/

#ifndef UPB_DECODER_INT_H_
#define UPB_DECODER_INT_H_

/*
** upb::pb::Decoder
**
** A high performance, streaming, resumable decoder for the binary protobuf
** format.
**
** This interface works the same regardless of what decoder backend is being
** used.  A client of this class does not need to know whether decoding is using
** a JITted decoder (DynASM, LLVM, etc) or an interpreted decoder.  By default,
** it will always use the fastest available decoder.  However, you can call
** set_allow_jit(false) to disable any JIT decoder that might be available.
** This is primarily useful for testing purposes.
*/

#ifndef UPB_DECODER_H_
#define UPB_DECODER_H_


#ifdef __cplusplus
namespace upb {
namespace pb {
class CodeCache;
class DecoderPtr;
class DecoderMethodPtr;
class DecoderMethodOptions;
}  /* namespace pb */
}  /* namespace upb */
#endif

/* The maximum number of bytes we are required to buffer internally between
 * calls to the decoder.  The value is 14: a 5 byte unknown tag plus ten-byte
 * varint, less one because we are buffering an incomplete value.
 *
 * Should only be used by unit tests. */
#define UPB_DECODER_MAX_RESIDUAL_BYTES 14

/* upb_pbdecodermethod ********************************************************/

struct upb_pbdecodermethod;
typedef struct upb_pbdecodermethod upb_pbdecodermethod;

#ifdef __cplusplus
extern "C" {
#endif

const upb_handlers *upb_pbdecodermethod_desthandlers(
    const upb_pbdecodermethod *m);
const upb_byteshandler *upb_pbdecodermethod_inputhandler(
    const upb_pbdecodermethod *m);
bool upb_pbdecodermethod_isnative(const upb_pbdecodermethod *m);

#ifdef __cplusplus
}  /* extern "C" */

/* Represents the code to parse a protobuf according to a destination
 * Handlers. */
class upb::pb::DecoderMethodPtr {
 public:
  DecoderMethodPtr() : ptr_(nullptr) {}
  DecoderMethodPtr(const upb_pbdecodermethod* ptr) : ptr_(ptr) {}

  const upb_pbdecodermethod* ptr() { return ptr_; }

  /* The destination handlers that are statically bound to this method.
   * This method is only capable of outputting to a sink that uses these
   * handlers. */
  const Handlers *dest_handlers() const {
    return upb_pbdecodermethod_desthandlers(ptr_);
  }

  /* The input handlers for this decoder method. */
  const BytesHandler* input_handler() const {
    return upb_pbdecodermethod_inputhandler(ptr_);
  }

  /* Whether this method is native. */
  bool is_native() const {
    return upb_pbdecodermethod_isnative(ptr_);
  }

 private:
  const upb_pbdecodermethod* ptr_;
};

#endif

/* upb_pbdecoder **************************************************************/

/* Preallocation hint: decoder won't allocate more bytes than this when first
 * constructed.  This hint may be an overestimate for some build configurations.
 * But if the decoder library is upgraded without recompiling the application,
 * it may be an underestimate. */
#define UPB_PB_DECODER_SIZE 4416

struct upb_pbdecoder;
typedef struct upb_pbdecoder upb_pbdecoder;

#ifdef __cplusplus
extern "C" {
#endif

upb_pbdecoder *upb_pbdecoder_create(upb_arena *arena,
                                    const upb_pbdecodermethod *method,
                                    upb_sink output, upb_status *status);
const upb_pbdecodermethod *upb_pbdecoder_method(const upb_pbdecoder *d);
upb_bytessink upb_pbdecoder_input(upb_pbdecoder *d);
uint64_t upb_pbdecoder_bytesparsed(const upb_pbdecoder *d);
size_t upb_pbdecoder_maxnesting(const upb_pbdecoder *d);
bool upb_pbdecoder_setmaxnesting(upb_pbdecoder *d, size_t max);
void upb_pbdecoder_reset(upb_pbdecoder *d);

#ifdef __cplusplus
}  /* extern "C" */

/* A Decoder receives binary protobuf data on its input sink and pushes the
 * decoded data to its output sink. */
class upb::pb::DecoderPtr {
 public:
  DecoderPtr() : ptr_(nullptr) {}
  DecoderPtr(upb_pbdecoder* ptr) : ptr_(ptr) {}

  upb_pbdecoder* ptr() { return ptr_; }

  /* Constructs a decoder instance for the given method, which must outlive this
   * decoder.  Any errors during parsing will be set on the given status, which
   * must also outlive this decoder.
   *
   * The sink must match the given method. */
  static DecoderPtr Create(Arena *arena, DecoderMethodPtr method,
                           upb::Sink output, Status *status) {
    return DecoderPtr(upb_pbdecoder_create(arena->ptr(), method.ptr(),
                                           output.sink(), status->ptr()));
  }

  /* Returns the DecoderMethod this decoder is parsing from. */
  const DecoderMethodPtr method() const {
    return DecoderMethodPtr(upb_pbdecoder_method(ptr_));
  }

  /* The sink on which this decoder receives input. */
  BytesSink input() { return BytesSink(upb_pbdecoder_input(ptr())); }

  /* Returns number of bytes successfully parsed.
   *
   * This can be useful for determining the stream position where an error
   * occurred.
   *
   * This value may not be up-to-date when called from inside a parsing
   * callback. */
  uint64_t BytesParsed() { return upb_pbdecoder_bytesparsed(ptr()); }

  /* Gets/sets the parsing nexting limit.  If the total number of nested
   * submessages and repeated fields hits this limit, parsing will fail.  This
   * is a resource limit that controls the amount of memory used by the parsing
   * stack.
   *
   * Setting the limit will fail if the parser is currently suspended at a depth
   * greater than this, or if memory allocation of the stack fails. */
  size_t max_nesting() { return upb_pbdecoder_maxnesting(ptr()); }
  bool set_max_nesting(size_t max) { return upb_pbdecoder_maxnesting(ptr()); }

  void Reset() { upb_pbdecoder_reset(ptr()); }

  static const size_t kSize = UPB_PB_DECODER_SIZE;

 private:
  upb_pbdecoder *ptr_;
};

#endif  /* __cplusplus */

/* upb_pbcodecache ************************************************************/

/* Lazily builds and caches decoder methods that will push data to the given
 * handlers.  The destination handlercache must outlive this object. */

struct upb_pbcodecache;
typedef struct upb_pbcodecache upb_pbcodecache;

#ifdef __cplusplus
extern "C" {
#endif

upb_pbcodecache *upb_pbcodecache_new(upb_handlercache *dest);
void upb_pbcodecache_free(upb_pbcodecache *c);
bool upb_pbcodecache_allowjit(const upb_pbcodecache *c);
void upb_pbcodecache_setallowjit(upb_pbcodecache *c, bool allow);
void upb_pbcodecache_setlazy(upb_pbcodecache *c, bool lazy);
const upb_pbdecodermethod *upb_pbcodecache_get(upb_pbcodecache *c,
                                               const upb_msgdef *md);

#ifdef __cplusplus
}  /* extern "C" */

/* A class for caching protobuf processing code, whether bytecode for the
 * interpreted decoder or machine code for the JIT.
 *
 * This class is not thread-safe. */
class upb::pb::CodeCache {
 public:
  CodeCache(upb::HandlerCache *dest)
      : ptr_(upb_pbcodecache_new(dest->ptr()), upb_pbcodecache_free) {}
  CodeCache(CodeCache&&) = default;
  CodeCache& operator=(CodeCache&&) = default;

  upb_pbcodecache* ptr() { return ptr_.get(); }
  const upb_pbcodecache* ptr() const { return ptr_.get(); }

  /* Whether the cache is allowed to generate machine code.  Defaults to true.
   * There is no real reason to turn it off except for testing or if you are
   * having a specific problem with the JIT.
   *
   * Note that allow_jit = true does not *guarantee* that the code will be JIT
   * compiled.  If this platform is not supported or the JIT was not compiled
   * in, the code may still be interpreted. */
  bool allow_jit() const { return upb_pbcodecache_allowjit(ptr()); }

  /* This may only be called when the object is first constructed, and prior to
   * any code generation. */
  void set_allow_jit(bool allow) { upb_pbcodecache_setallowjit(ptr(), allow); }

  /* Should the decoder push submessages to lazy handlers for fields that have
   * them?  The caller should set this iff the lazy handlers expect data that is
   * in protobuf binary format and the caller wishes to lazy parse it. */
  void set_lazy(bool lazy) { upb_pbcodecache_setlazy(ptr(), lazy); }

  /* Returns a DecoderMethod that can push data to the given handlers.
   * If a suitable method already exists, it will be returned from the cache. */
  const DecoderMethodPtr Get(MessageDefPtr md) {
    return DecoderMethodPtr(upb_pbcodecache_get(ptr(), md.ptr()));
  }

 private:
  std::unique_ptr<upb_pbcodecache, decltype(&upb_pbcodecache_free)> ptr_;
};

#endif  /* __cplusplus */

#endif  /* UPB_DECODER_H_ */


/* Opcode definitions.  The canonical meaning of each opcode is its
 * implementation in the interpreter (the JIT is written to match this).
 *
 * All instructions have the opcode in the low byte.
 * Instruction format for most instructions is:
 *
 * +-------------------+--------+
 * |     arg (24)      | op (8) |
 * +-------------------+--------+
 *
 * Exceptions are indicated below.  A few opcodes are multi-word. */
typedef enum {
  /* Opcodes 1-8, 13, 15-18 parse their respective descriptor types.
   * Arg for all of these is the upb selector for this field. */
#define T(type) OP_PARSE_ ## type = UPB_DESCRIPTOR_TYPE_ ## type
  T(DOUBLE), T(FLOAT), T(INT64), T(UINT64), T(INT32), T(FIXED64), T(FIXED32),
  T(BOOL), T(UINT32), T(SFIXED32), T(SFIXED64), T(SINT32), T(SINT64),
#undef T
  OP_STARTMSG       = 9,   /* No arg. */
  OP_ENDMSG         = 10,  /* No arg. */
  OP_STARTSEQ       = 11,
  OP_ENDSEQ         = 12,
  OP_STARTSUBMSG    = 14,
  OP_ENDSUBMSG      = 19,
  OP_STARTSTR       = 20,
  OP_STRING         = 21,
  OP_ENDSTR         = 22,

  OP_PUSHTAGDELIM   = 23,  /* No arg. */
  OP_PUSHLENDELIM   = 24,  /* No arg. */
  OP_POP            = 25,  /* No arg. */
  OP_SETDELIM       = 26,  /* No arg. */
  OP_SETBIGGROUPNUM = 27,  /* two words:
                            *   | unused (24)     | opc (8) |
                            *   |        groupnum (32)      | */
  OP_CHECKDELIM     = 28,
  OP_CALL           = 29,
  OP_RET            = 30,
  OP_BRANCH         = 31,

  /* Different opcodes depending on how many bytes expected. */
  OP_TAG1           = 32,  /* | match tag (16) | jump target (8) | opc (8) | */
  OP_TAG2           = 33,  /* | match tag (16) | jump target (8) | opc (8) | */
  OP_TAGN           = 34,  /* three words: */
                           /*   | unused (16) | jump target(8) | opc (8) | */
                           /*   |           match tag 1 (32)             | */
                           /*   |           match tag 2 (32)             | */

  OP_SETDISPATCH    = 35,  /* N words: */
                           /*   | unused (24)         | opc | */
                           /*   | upb_inttable* (32 or 64)  | */

  OP_DISPATCH       = 36,  /* No arg. */

  OP_HALT           = 37   /* No arg. */
} opcode;

#define OP_MAX OP_HALT

UPB_INLINE opcode getop(uint32_t instr) { return (opcode)(instr & 0xff); }

struct upb_pbcodecache {
  upb_arena *arena;
  upb_handlercache *dest;
  bool allow_jit;
  bool lazy;

  /* Map of upb_msgdef -> mgroup. */
  upb_inttable groups;
};

/* Method group; represents a set of decoder methods that had their code
 * emitted together.  Immutable once created.  */
typedef struct {
  /* Maps upb_msgdef/upb_handlers -> upb_pbdecodermethod.  Owned by us.
   *
   * Ideally this would be on pbcodecache (if we were actually caching code).
   * Right now we don't actually cache anything, which is wasteful. */
  upb_inttable methods;

  /* The bytecode for our methods, if any exists.  Owned by us. */
  uint32_t *bytecode;
  uint32_t *bytecode_end;
} mgroup;

/* The maximum that any submessages can be nested.  Matches proto2's limit.
 * This specifies the size of the decoder's statically-sized array and therefore
 * setting it high will cause the upb::pb::Decoder object to be larger.
 *
 * If necessary we can add a runtime-settable property to Decoder that allow
 * this to be larger than the compile-time setting, but this would add
 * complexity, particularly since we would have to decide how/if to give users
 * the ability to set a custom memory allocation function. */
#define UPB_DECODER_MAX_NESTING 64

/* Internal-only struct used by the decoder. */
typedef struct {
  /* Space optimization note: we store two pointers here that the JIT
   * doesn't need at all; the upb_handlers* inside the sink and
   * the dispatch table pointer.  We can optimze so that the JIT uses
   * smaller stack frames than the interpreter.  The only thing we need
   * to guarantee is that the fallback routines can find end_ofs. */
  upb_sink sink;

  /* The absolute stream offset of the end-of-frame delimiter.
   * Non-delimited frames (groups and non-packed repeated fields) reuse the
   * delimiter of their parent, even though the frame may not end there.
   *
   * NOTE: the JIT stores a slightly different value here for non-top frames.
   * It stores the value relative to the end of the enclosed message.  But the
   * top frame is still stored the same way, which is important for ensuring
   * that calls from the JIT into C work correctly. */
  uint64_t end_ofs;
  const uint32_t *base;

  /* 0 indicates a length-delimited field.
   * A positive number indicates a known group.
   * A negative number indicates an unknown group. */
  int32_t groupnum;
  upb_inttable *dispatch;  /* Not used by the JIT. */
} upb_pbdecoder_frame;

struct upb_pbdecodermethod {
  /* While compiling, the base is relative in "ofs", after compiling it is
   * absolute in "ptr". */
  union {
    uint32_t ofs;     /* PC offset of method. */
    void *ptr;        /* Pointer to bytecode or machine code for this method. */
  } code_base;

  /* The decoder method group to which this method belongs. */
  const mgroup *group;

  /* Whether this method is native code or bytecode. */
  bool is_native_;

  /* The handler one calls to invoke this method. */
  upb_byteshandler input_handler_;

  /* The destination handlers this method is bound to.  We own a ref. */
  const upb_handlers *dest_handlers_;

  /* Dispatch table -- used by both bytecode decoder and JIT when encountering a
   * field number that wasn't the one we were expecting to see.  See
   * decoder.int.h for the layout of this table. */
  upb_inttable dispatch;
};

struct upb_pbdecoder {
  upb_arena *arena;

  /* Our input sink. */
  upb_bytessink input_;

  /* The decoder method we are parsing with (owned). */
  const upb_pbdecodermethod *method_;

  size_t call_len;
  const uint32_t *pc, *last;

  /* Current input buffer and its stream offset. */
  const char *buf, *ptr, *end, *checkpoint;

  /* End of the delimited region, relative to ptr, NULL if not in this buf. */
  const char *delim_end;

  /* End of the delimited region, relative to ptr, end if not in this buf. */
  const char *data_end;

  /* Overall stream offset of "buf." */
  uint64_t bufstart_ofs;

  /* Buffer for residual bytes not parsed from the previous buffer. */
  char residual[UPB_DECODER_MAX_RESIDUAL_BYTES];
  char *residual_end;

  /* Bytes of data that should be discarded from the input beore we start
   * parsing again.  We set this when we internally determine that we can
   * safely skip the next N bytes, but this region extends past the current
   * user buffer. */
  size_t skip;

  /* Stores the user buffer passed to our decode function. */
  const char *buf_param;
  size_t size_param;
  const upb_bufhandle *handle;

  /* Our internal stack. */
  upb_pbdecoder_frame *stack, *top, *limit;
  const uint32_t **callstack;
  size_t stack_size;

  upb_status *status;
};

/* Decoder entry points; used as handlers. */
void *upb_pbdecoder_startbc(void *closure, const void *pc, size_t size_hint);
size_t upb_pbdecoder_decode(void *closure, const void *hd, const char *buf,
                            size_t size, const upb_bufhandle *handle);
bool upb_pbdecoder_end(void *closure, const void *handler_data);

/* Decoder-internal functions that the JIT calls to handle fallback paths. */
int32_t upb_pbdecoder_resume(upb_pbdecoder *d, void *p, const char *buf,
                             size_t size, const upb_bufhandle *handle);
size_t upb_pbdecoder_suspend(upb_pbdecoder *d);
int32_t upb_pbdecoder_skipunknown(upb_pbdecoder *d, int32_t fieldnum,
                                  uint8_t wire_type);
int32_t upb_pbdecoder_checktag_slow(upb_pbdecoder *d, uint64_t expected);
int32_t upb_pbdecoder_decode_varint_slow(upb_pbdecoder *d, uint64_t *u64);
int32_t upb_pbdecoder_decode_f32(upb_pbdecoder *d, uint32_t *u32);
int32_t upb_pbdecoder_decode_f64(upb_pbdecoder *d, uint64_t *u64);
void upb_pbdecoder_seterr(upb_pbdecoder *d, const char *msg);

/* Error messages that are shared between the bytecode and JIT decoders. */
extern const char *kPbDecoderStackOverflow;
extern const char *kPbDecoderSubmessageTooLong;

/* Access to decoderplan members needed by the decoder. */
const char *upb_pbdecoder_getopname(unsigned int op);

/* A special label that means "do field dispatch for this message and branch to
 * wherever that takes you." */
#define LABEL_DISPATCH 0

/* A special slot in the dispatch table that stores the epilogue (ENDMSG and/or
 * RET) for branching to when we find an appropriate ENDGROUP tag. */
#define DISPATCH_ENDMSG 0

/* It's important to use this invalid wire type instead of 0 (which is a valid
 * wire type). */
#define NO_WIRE_TYPE 0xff

/* The dispatch table layout is:
 *   [field number] -> [ 48-bit offset ][ 8-bit wt2 ][ 8-bit wt1 ]
 *
 * If wt1 matches, jump to the 48-bit offset.  If wt2 matches, lookup
 * (UPB_MAX_FIELDNUMBER + fieldnum) and jump there.
 *
 * We need two wire types because of packed/non-packed compatibility.  A
 * primitive repeated field can use either wire type and be valid.  While we
 * could key the table on fieldnum+wiretype, the table would be 8x sparser.
 *
 * Storing two wire types in the primary value allows us to quickly rule out
 * the second wire type without needing to do a separate lookup (this case is
 * less common than an unknown field). */
UPB_INLINE uint64_t upb_pbdecoder_packdispatch(uint64_t ofs, uint8_t wt1,
                                               uint8_t wt2) {
  return (ofs << 16) | (wt2 << 8) | wt1;
}

UPB_INLINE void upb_pbdecoder_unpackdispatch(uint64_t dispatch, uint64_t *ofs,
                                             uint8_t *wt1, uint8_t *wt2) {
  *wt1 = (uint8_t)dispatch;
  *wt2 = (uint8_t)(dispatch >> 8);
  *ofs = dispatch >> 16;
}

/* All of the functions in decoder.c that return int32_t return values according
 * to the following scheme:
 *   1. negative values indicate a return code from the following list.
 *   2. positive values indicate that error or end of buffer was hit, and
 *      that the decode function should immediately return the given value
 *      (the decoder state has already been suspended and is ready to be
 *      resumed). */
#define DECODE_OK -1
#define DECODE_MISMATCH -2  /* Used only from checktag_slow(). */
#define DECODE_ENDGROUP -3  /* Used only from checkunknown(). */

#define CHECK_RETURN(x) { int32_t ret = x; if (ret >= 0) return ret; }


#endif  /* UPB_DECODER_INT_H_ */
/*
** A number of routines for varint manipulation (we keep them all around to
** have multiple approaches available for benchmarking).
*/

#ifndef UPB_VARINT_DECODER_H_
#define UPB_VARINT_DECODER_H_

#include <assert.h>
#include <stdint.h>
#include <string.h>


#ifdef __cplusplus
extern "C" {
#endif

#define UPB_MAX_WIRE_TYPE 5

/* The maximum number of bytes that it takes to encode a 64-bit varint. */
#define UPB_PB_VARINT_MAX_LEN 10

/* Array of the "native" (ie. non-packed-repeated) wire type for the given a
 * descriptor type (upb_descriptortype_t). */
extern const uint8_t upb_pb_native_wire_types[];

UPB_INLINE uint64_t byteswap64(uint64_t val)
{
  return ((((val) & 0xff00000000000000ull) >> 56)
    | (((val) & 0x00ff000000000000ull) >> 40)
    | (((val) & 0x0000ff0000000000ull) >> 24)
    | (((val) & 0x000000ff00000000ull) >> 8)
    | (((val) & 0x00000000ff000000ull) << 8)
    | (((val) & 0x0000000000ff0000ull) << 24)
    | (((val) & 0x000000000000ff00ull) << 40)
    | (((val) & 0x00000000000000ffull) << 56));
}

/* Zig-zag encoding/decoding **************************************************/

UPB_INLINE int32_t upb_zzdec_32(uint32_t n) {
  return (n >> 1) ^ -(int32_t)(n & 1);
}
UPB_INLINE int64_t upb_zzdec_64(uint64_t n) {
  return (n >> 1) ^ -(int64_t)(n & 1);
}
UPB_INLINE uint32_t upb_zzenc_32(int32_t n) {
  return ((uint32_t)n << 1) ^ (n >> 31);
}
UPB_INLINE uint64_t upb_zzenc_64(int64_t n) {
  return ((uint64_t)n << 1) ^ (n >> 63);
}

/* Decoding *******************************************************************/

/* All decoding functions return this struct by value. */
typedef struct {
  const char *p;  /* NULL if the varint was unterminated. */
  uint64_t val;
} upb_decoderet;

UPB_INLINE upb_decoderet upb_decoderet_make(const char *p, uint64_t val) {
  upb_decoderet ret;
  ret.p = p;
  ret.val = val;
  return ret;
}

upb_decoderet upb_vdecode_max8_branch32(upb_decoderet r);
upb_decoderet upb_vdecode_max8_branch64(upb_decoderet r);

/* Template for a function that checks the first two bytes with branching
 * and dispatches 2-10 bytes with a separate function.  Note that this may read
 * up to 10 bytes, so it must not be used unless there are at least ten bytes
 * left in the buffer! */
#define UPB_VARINT_DECODER_CHECK2(name, decode_max8_function)                  \
UPB_INLINE upb_decoderet upb_vdecode_check2_ ## name(const char *_p) {         \
  uint8_t *p = (uint8_t*)_p;                                                   \
  upb_decoderet r;                                                             \
  if ((*p & 0x80) == 0) {                                                      \
  /* Common case: one-byte varint. */                                          \
    return upb_decoderet_make(_p + 1, *p & 0x7fU);                             \
  }                                                                            \
  r = upb_decoderet_make(_p + 2, (*p & 0x7fU) | ((*(p + 1) & 0x7fU) << 7));    \
  if ((*(p + 1) & 0x80) == 0) {                                                \
    /* Two-byte varint. */                                                     \
    return r;                                                                  \
  }                                                                            \
  /* Longer varint, fallback to out-of-line function. */                       \
  return decode_max8_function(r);                                              \
}

UPB_VARINT_DECODER_CHECK2(branch32, upb_vdecode_max8_branch32)
UPB_VARINT_DECODER_CHECK2(branch64, upb_vdecode_max8_branch64)
#undef UPB_VARINT_DECODER_CHECK2

/* Our canonical functions for decoding varints, based on the currently
 * favored best-performing implementations. */
UPB_INLINE upb_decoderet upb_vdecode_fast(const char *p) {
  if (sizeof(long) == 8)
    return upb_vdecode_check2_branch64(p);
  else
    return upb_vdecode_check2_branch32(p);
}


/* Encoding *******************************************************************/

UPB_INLINE int upb_value_size(uint64_t val) {
#ifdef __GNUC__
  int high_bit = 63 - __builtin_clzll(val);  /* 0-based, undef if val == 0. */
#else
  int high_bit = 0;
  uint64_t tmp = val;
  while(tmp >>= 1) high_bit++;
#endif
  return val == 0 ? 1 : high_bit / 8 + 1;
}

/* Encodes a 64-bit varint into buf (which must be >=UPB_PB_VARINT_MAX_LEN
 * bytes long), returning how many bytes were used.
 *
 * TODO: benchmark and optimize if necessary. */
UPB_INLINE size_t upb_vencode64(uint64_t val, char *buf) {
  size_t i;
  if (val == 0) { buf[0] = 0; return 1; }
  i = 0;
  while (val) {
    uint8_t byte = val & 0x7fU;
    val >>= 7;
    if (val) byte |= 0x80U;
    buf[i++] = byte;
  }
  return i;
}

UPB_INLINE size_t upb_varint_size(uint64_t val) {
  char buf[UPB_PB_VARINT_MAX_LEN];
  return upb_vencode64(val, buf);
}

/* Encodes a 32-bit varint, *not* sign-extended. */
UPB_INLINE uint64_t upb_vencode32(uint32_t val) {
  char buf[UPB_PB_VARINT_MAX_LEN];
  size_t bytes = upb_vencode64(val, buf);
  uint64_t ret = 0;
  UPB_ASSERT(bytes <= 5);
  memcpy(&ret, buf, bytes);
#ifdef UPB_BIG_ENDIAN
  ret = byteswap64(ret);
#endif
  UPB_ASSERT(ret <= 0xffffffffffU);
  return ret;
}

#ifdef __cplusplus
}  /* extern "C" */
#endif


#endif  /* UPB_VARINT_DECODER_H_ */
/*
** upb::pb::Encoder (upb_pb_encoder)
**
** Implements a set of upb_handlers that write protobuf data to the binary wire
** format.
**
** This encoder implementation does not have any access to any out-of-band or
** precomputed lengths for submessages, so it must buffer submessages internally
** before it can emit the first byte.
*/

#ifndef UPB_ENCODER_H_
#define UPB_ENCODER_H_


#ifdef __cplusplus
namespace upb {
namespace pb {
class EncoderPtr;
}  /* namespace pb */
}  /* namespace upb */
#endif

#define UPB_PBENCODER_MAX_NESTING 100

/* upb_pb_encoder *************************************************************/

/* Preallocation hint: decoder won't allocate more bytes than this when first
 * constructed.  This hint may be an overestimate for some build configurations.
 * But if the decoder library is upgraded without recompiling the application,
 * it may be an underestimate. */
#define UPB_PB_ENCODER_SIZE 784

struct upb_pb_encoder;
typedef struct upb_pb_encoder upb_pb_encoder;

#ifdef __cplusplus
extern "C" {
#endif

upb_sink upb_pb_encoder_input(upb_pb_encoder *p);
upb_pb_encoder* upb_pb_encoder_create(upb_arena* a, const upb_handlers* h,
                                      upb_bytessink output);

/* Lazily builds and caches handlers that will push encoded data to a bytessink.
 * Any msgdef objects used with this object must outlive it. */
upb_handlercache *upb_pb_encoder_newcache(void);

#ifdef __cplusplus
}  /* extern "C" { */

class upb::pb::EncoderPtr {
 public:
  EncoderPtr(upb_pb_encoder* ptr) : ptr_(ptr) {}

  upb_pb_encoder* ptr() { return ptr_; }

  /* Creates a new encoder in the given environment.  The Handlers must have
   * come from NewHandlers() below. */
  static EncoderPtr Create(Arena* arena, const Handlers* handlers,
                           BytesSink output) {
    return EncoderPtr(
        upb_pb_encoder_create(arena->ptr(), handlers, output.sink()));
  }

  /* The input to the encoder. */
  upb::Sink input() { return upb_pb_encoder_input(ptr()); }

  /* Creates a new set of handlers for this MessageDef. */
  static HandlerCache NewCache() {
    return HandlerCache(upb_pb_encoder_newcache());
  }

  static const size_t kSize = UPB_PB_ENCODER_SIZE;

 private:
  upb_pb_encoder* ptr_;
};

#endif  /* __cplusplus */

#endif  /* UPB_ENCODER_H_ */
/*
** upb::pb::TextPrinter (upb_textprinter)
**
** Handlers for writing to protobuf text format.
*/

#ifndef UPB_TEXT_H_
#define UPB_TEXT_H_


#ifdef __cplusplus
namespace upb {
namespace pb {
class TextPrinterPtr;
}  /* namespace pb */
}  /* namespace upb */
#endif

/* upb_textprinter ************************************************************/

struct upb_textprinter;
typedef struct upb_textprinter upb_textprinter;

#ifdef __cplusplus
extern "C" {
#endif

/* C API. */
upb_textprinter *upb_textprinter_create(upb_arena *arena, const upb_handlers *h,
                                        upb_bytessink output);
void upb_textprinter_setsingleline(upb_textprinter *p, bool single_line);
upb_sink upb_textprinter_input(upb_textprinter *p);
upb_handlercache *upb_textprinter_newcache(void);

#ifdef __cplusplus
}  /* extern "C" */

class upb::pb::TextPrinterPtr {
 public:
  TextPrinterPtr(upb_textprinter* ptr) : ptr_(ptr) {}

  /* The given handlers must have come from NewHandlers().  It must outlive the
   * TextPrinter. */
  static TextPrinterPtr Create(Arena *arena, upb::HandlersPtr *handlers,
                               BytesSink output) {
    return TextPrinterPtr(
        upb_textprinter_create(arena->ptr(), handlers->ptr(), output.sink()));
  }

  void SetSingleLineMode(bool single_line) {
    upb_textprinter_setsingleline(ptr_, single_line);
  }

  Sink input() { return upb_textprinter_input(ptr_); }

  /* If handler caching becomes a requirement we can add a code cache as in
   * decoder.h */
  static HandlerCache NewCache() {
    return HandlerCache(upb_textprinter_newcache());
  }

 private:
  upb_textprinter* ptr_;
};

#endif

#endif  /* UPB_TEXT_H_ */
/*
** upb::json::Parser (upb_json_parser)
**
** Parses JSON according to a specific schema.
** Support for parsing arbitrary JSON (schema-less) will be added later.
*/

#ifndef UPB_JSON_PARSER_H_
#define UPB_JSON_PARSER_H_


#ifdef __cplusplus
namespace upb {
namespace json {
class CodeCache;
class ParserPtr;
class ParserMethodPtr;
}  /* namespace json */
}  /* namespace upb */
#endif

/* upb_json_parsermethod ******************************************************/

struct upb_json_parsermethod;
typedef struct upb_json_parsermethod upb_json_parsermethod;

#ifdef __cplusplus
extern "C" {
#endif

const upb_byteshandler* upb_json_parsermethod_inputhandler(
    const upb_json_parsermethod* m);

#ifdef __cplusplus
}  /* extern "C" */

class upb::json::ParserMethodPtr {
 public:
  ParserMethodPtr() : ptr_(nullptr) {}
  ParserMethodPtr(const upb_json_parsermethod* ptr) : ptr_(ptr) {}

  const upb_json_parsermethod* ptr() const { return ptr_; }

  const BytesHandler* input_handler() const {
    return upb_json_parsermethod_inputhandler(ptr());
  }

 private:
  const upb_json_parsermethod* ptr_;
};

#endif  /* __cplusplus */

/* upb_json_parser ************************************************************/

/* Preallocation hint: parser won't allocate more bytes than this when first
 * constructed.  This hint may be an overestimate for some build configurations.
 * But if the parser library is upgraded without recompiling the application,
 * it may be an underestimate. */
#define UPB_JSON_PARSER_SIZE 5712

struct upb_json_parser;
typedef struct upb_json_parser upb_json_parser;

#ifdef __cplusplus
extern "C" {
#endif

upb_json_parser* upb_json_parser_create(upb_arena* a,
                                        const upb_json_parsermethod* m,
                                        const upb_symtab* symtab,
                                        upb_sink output,
                                        upb_status *status,
                                        bool ignore_json_unknown);
upb_bytessink upb_json_parser_input(upb_json_parser* p);

#ifdef __cplusplus
}  /* extern "C" */

/* Parses an incoming BytesStream, pushing the results to the destination
 * sink. */
class upb::json::ParserPtr {
 public:
  ParserPtr(upb_json_parser* ptr) : ptr_(ptr) {}

  static ParserPtr Create(Arena* arena, ParserMethodPtr method,
                          SymbolTable* symtab, Sink output, Status* status,
                          bool ignore_json_unknown) {
    upb_symtab* symtab_ptr = symtab ? symtab->ptr() : nullptr;
    return ParserPtr(upb_json_parser_create(
        arena->ptr(), method.ptr(), symtab_ptr, output.sink(), status->ptr(),
        ignore_json_unknown));
  }

  BytesSink input() { return upb_json_parser_input(ptr_); }

 private:
  upb_json_parser* ptr_;
};

#endif  /* __cplusplus */

/* upb_json_codecache *********************************************************/

/* Lazily builds and caches decoder methods that will push data to the given
 * handlers.  The upb_symtab object(s) must outlive this object. */

struct upb_json_codecache;
typedef struct upb_json_codecache upb_json_codecache;

#ifdef __cplusplus
extern "C" {
#endif

upb_json_codecache *upb_json_codecache_new(void);
void upb_json_codecache_free(upb_json_codecache *cache);
const upb_json_parsermethod* upb_json_codecache_get(upb_json_codecache* cache,
                                                    const upb_msgdef* md);

#ifdef __cplusplus
}  /* extern "C" */

class upb::json::CodeCache {
 public:
  CodeCache() : ptr_(upb_json_codecache_new(), upb_json_codecache_free) {}

  /* Returns a DecoderMethod that can push data to the given handlers.
   * If a suitable method already exists, it will be returned from the cache. */
  ParserMethodPtr Get(MessageDefPtr md) {
    return upb_json_codecache_get(ptr_.get(), md.ptr());
  }

 private:
  std::unique_ptr<upb_json_codecache, decltype(&upb_json_codecache_free)> ptr_;
};

#endif

#endif  /* UPB_JSON_PARSER_H_ */
/*
** upb::json::Printer
**
** Handlers that emit JSON according to a specific protobuf schema.
*/

#ifndef UPB_JSON_TYPED_PRINTER_H_
#define UPB_JSON_TYPED_PRINTER_H_


#ifdef __cplusplus
namespace upb {
namespace json {
class PrinterPtr;
}  /* namespace json */
}  /* namespace upb */
#endif

/* upb_json_printer ***********************************************************/

#define UPB_JSON_PRINTER_SIZE 192

struct upb_json_printer;
typedef struct upb_json_printer upb_json_printer;

#ifdef __cplusplus
extern "C" {
#endif

/* Native C API. */
upb_json_printer *upb_json_printer_create(upb_arena *a, const upb_handlers *h,
                                          upb_bytessink output);
upb_sink upb_json_printer_input(upb_json_printer *p);
const upb_handlers *upb_json_printer_newhandlers(const upb_msgdef *md,
                                                 bool preserve_fieldnames,
                                                 const void *owner);

/* Lazily builds and caches handlers that will push encoded data to a bytessink.
 * Any msgdef objects used with this object must outlive it. */
upb_handlercache *upb_json_printer_newcache(bool preserve_proto_fieldnames);

#ifdef __cplusplus
}  /* extern "C" */

/* Prints an incoming stream of data to a BytesSink in JSON format. */
class upb::json::PrinterPtr {
 public:
  PrinterPtr(upb_json_printer* ptr) : ptr_(ptr) {}

  static PrinterPtr Create(Arena *arena, const upb::Handlers *handlers,
                           BytesSink output) {
    return PrinterPtr(
        upb_json_printer_create(arena->ptr(), handlers, output.sink()));
  }

  /* The input to the printer. */
  Sink input() { return upb_json_printer_input(ptr_); }

  static const size_t kSize = UPB_JSON_PRINTER_SIZE;

  static HandlerCache NewCache(bool preserve_proto_fieldnames) {
    return upb_json_printer_newcache(preserve_proto_fieldnames);
  }

 private:
  upb_json_printer* ptr_;
};

#endif  /* __cplusplus */

#endif  /* UPB_JSON_TYPED_PRINTER_H_ */
/* See port_def.inc.  This should #undef all macros #defined there. */

#undef UPB_SIZE
#undef UPB_FIELD_AT
#undef UPB_READ_ONEOF
#undef UPB_WRITE_ONEOF
#undef UPB_INLINE
#undef UPB_FORCEINLINE
#undef UPB_NOINLINE
#undef UPB_NORETURN
#undef UPB_MAX
#undef UPB_MIN
#undef UPB_UNUSED
#undef UPB_ASSERT
#undef UPB_ASSERT_DEBUGVAR
#undef UPB_UNREACHABLE
#undef UPB_INFINITY
#undef UPB_MSVC_VSNPRINTF
#undef _upb_snprintf
#undef _upb_vsnprintf
#undef _upb_va_copy
