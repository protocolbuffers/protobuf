/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * This file contains shared definitions that are widely used across upb.
 *
 * This is a mixed C/C++ interface that offers a full API to both languages.
 * See the top-level README for more information.
 */

#ifndef UPB_H_
#define UPB_H_

#include <assert.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// inline if possible, emit standalone code if required.
#ifdef __cplusplus
#define UPB_INLINE inline
#else
#define UPB_INLINE static inline
#endif

#if __STDC_VERSION__ >= 199901L
#define UPB_C99
#endif

#if ((defined(__cplusplus) && __cplusplus >= 201103L) || \
      defined(__GXX_EXPERIMENTAL_CXX0X__)) && !defined(UPB_NO_CXX11)
#define UPB_CXX11
#endif

#ifdef UPB_CXX11
#define UPB_DISALLOW_POD_OPS(class_name) \
  class_name() = delete; \
  ~class_name() = delete; \
  class_name(const class_name&) = delete; \
  void operator=(const class_name&) = delete;
#else
#define UPB_DISALLOW_POD_OPS(class_name) \
  class_name(); \
  ~class_name(); \
  class_name(const class_name&); \
  void operator=(const class_name&);
#endif

#if defined(__clang__) && defined(LANG_CXX11) && defined(__has_warning)
#if __has_feature(cxx_attributes) && __has_warning("-Wimplicit-fallthrough")
#define UPB_FALLTHROUGH_INTENDED [[clang::fallthrough]]
#endif
#endif

#ifndef UPB_FALLTHROUGH_INTENDED
#define UPB_FALLTHROUGH_INTENDED do { } while (0)
#endif

#ifdef __GNUC__
#define UPB_NORETURN __attribute__((__noreturn__))
#else
#define UPB_NORETURN
#endif

// Type detection and typedefs for integer types.
// For platforms where there are multiple 32-bit or 64-bit types, we need to be
// able to enumerate them so we can properly create overloads for all variants.
//
// If any platform existed where there were three integer types with the same
// size, this would have to become more complicated.  For example, short, int,
// and long could all be 32-bits.  Even more diabolically, short, int, long,
// and long long could all be 64 bits and still be standard-compliant.
// However, few platforms are this strange, and it's unlikely that upb will be
// used on the strangest ones.

// Can't count on stdint.h limits like INT32_MAX, because in C++ these are
// only defined when __STDC_LIMIT_MACROS are defined before the *first* include
// of stdint.h.  We can't guarantee that someone else didn't include these first
// without defining __STDC_LIMIT_MACROS.
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

#if UPB_INT_IS_32BITS
typedef int upb_int32_t;
typedef unsigned int upb_uint32_t;
#define UPB_INT32_CTYPE i

#if UPB_LONG_IS_32BITS
#define UPB_TWO_32BIT_TYPES 1
typedef long upb_int32alt_t;
typedef unsigned long upb_uint32alt_t;
#define UPB_INT32_CTYPE2 l
#endif  // UPB_LONG_IS_32BITS

#elif UPB_LONG_IS_32BITS  // && !UPB_INT_IS_32BITS
typedef long upb_int32_t;
typedef unsigned long upb_uint32_t;
#define UPB_INT32_CTYPE l
#endif  // UPB_INT_IS_32BITS


#if UPB_LONG_IS_64BITS
typedef long upb_int64_t;
typedef unsigned long upb_uint64_t;
#define UPB_INT64_CTYPE l

#if UPB_LLONG_IS_64BITS
#define UPB_TWO_64BIT_TYPES 1
typedef long long upb_int64alt_t;
typedef unsigned long long upb_uint64alt_t;
#define UPB_INT64_CTYPE2 ll
#endif  // UPB_LLONG_IS_64BITS

#elif UPB_LLONG_IS_64BITS  // && !UPB_LONG_IS_64BITS
typedef long long upb_int64_t;
typedef unsigned long long upb_uint64_t;
#define UPB_INT64_CTYPE ll
#endif  // UPB_LONG_IS_64BITS

#define UPB_MAX(x, y) ((x) > (y) ? (x) : (y))
#define UPB_MIN(x, y) ((x) < (y) ? (x) : (y))

// For our C-based inheritance, sometimes it's necessary to upcast an object to
// its base class.  We try to minimize the need for this by replicating base
// class functions in the derived class -- the derived class functions simply
// forward to the base class implementations.  This strategy simplifies the C++
// API since we can't use real C++ inheritance.
#define upb_upcast(obj) (&(obj)->base)
#define upb_upcast2(obj) upb_upcast(upb_upcast(obj))

char *upb_strdup(const char *s);

#define UPB_UNUSED(var) (void)var

// For asserting something about a variable when the variable is not used for
// anything else.  This prevents "unused variable" warnings when compiling in
// debug mode.
#define UPB_ASSERT_VAR(var, predicate) UPB_UNUSED(var); assert(predicate)

// The maximum that any submessages can be nested.  Matches proto2's limit.
// At the moment this specifies the size of several statically-sized arrays
// and therefore setting it high will cause more memory to be used.  Will
// be replaced by a runtime-configurable limit and dynamically-resizing arrays.
// TODO: make this a runtime-settable property of upb_handlers.
#define UPB_MAX_NESTING 64

// Inherent limit of protobuf wire format and schema definition.
#define UPB_MAX_FIELDNUMBER ((1 << 29) - 1)

// Nested type names are separated by periods.
#define UPB_SYMBOL_SEPARATOR '.'

// The longest chain that mutually-recursive types are allowed to form.  For
// example, this is a type cycle of length 2:
//   message A {
//     B b = 1;
//   }
//   message B {
//     A a = 1;
//   }
#define UPB_MAX_TYPE_CYCLE_LEN 16

// The maximum depth that the type graph can have.  Note that this setting does
// not automatically constrain UPB_MAX_NESTING, because type cycles allow for
// unlimited nesting if we do not limit it.  Many algorithms in upb call
// recursive functions that traverse the type graph, so we must limit this to
// avoid blowing the C stack.
#define UPB_MAX_TYPE_DEPTH 64


/* upb::Status ****************************************************************/

#ifdef __cplusplus
namespace upb { class Status; }
typedef upb::Status upb_status;
#else
struct upb_status;
typedef struct upb_status upb_status;
#endif

typedef enum {
  UPB_OK,          // The operation completed successfully.
  UPB_SUSPENDED,   // The operation was suspended and may be resumed later.
  UPB_ERROR,       // An error occurred.
} upb_success_t;

typedef struct {
  const char *name;
  // Writes a NULL-terminated string to "buf" containing an error message for
  // the given error code, returning false if the message was too large to fit.
  bool (*code_to_string)(int code, char *buf, size_t len);
} upb_errorspace;

#ifdef __cplusplus

class upb::Status {
 public:
  typedef upb_success_t Success;

  Status();
  ~Status();

  bool ok() const;
  bool eof() const;

  const char *GetString() const;
  void SetEof();
  void SetErrorLiteral(const char* msg);
  void Clear();

 private:
#else
struct upb_status {
#endif
  bool error;
  bool eof_;

  // Specific status code defined by some error space (optional).
  int code;
  upb_errorspace *space;

  // Error message (optional).
  const char *str;  // NULL when no message is present.  NULL-terminated.
  char *buf;        // Owned by the status.
  size_t bufsize;
};

#define UPB_STATUS_INIT {UPB_OK, false, 0, NULL, NULL, NULL, 0}

void upb_status_init(upb_status *status);
void upb_status_uninit(upb_status *status);

bool upb_ok(const upb_status *status);
bool upb_eof(const upb_status *status);

// Any of the functions that write to a status object allow status to be NULL,
// to support use cases where the function's caller does not care about the
// status message.
void upb_status_clear(upb_status *status);
void upb_status_seterrliteral(upb_status *status, const char *msg);
void upb_status_seterrf(upb_status *status, const char *msg, ...);
void upb_status_setcode(upb_status *status, upb_errorspace *space, int code);
void upb_status_seteof(upb_status *status);
// The returned string is invalidated by any other call into the status.
const char *upb_status_getstr(const upb_status *status);
void upb_status_copy(upb_status *to, const upb_status *from);

// Like vasprintf (which allocates a string large enough for the result), but
// uses *buf (which can be NULL) as a starting point and reallocates it only if
// the new value will not fit.  "size" is updated to reflect the allocated size
// of the buffer.  Starts writing at the given offset into the string; bytes
// preceding this offset are unaffected.  Returns the new length of the string,
// or -1 on memory allocation failure.
int upb_vrprintf(char **buf, size_t *size, size_t ofs,
                 const char *fmt, va_list args);


/* upb::Value *****************************************************************/

// TODO(haberman): upb::Value is gross and should be retired from the public
// interface (we *may* still want to keep it for internal use).  upb::Handlers
// and upb::Def should replace their use of Value with one function for each C
// type.

// Clients should not need to access these enum values; they are used internally
// to do typechecks of upb_value accesses.
typedef enum {
  UPB_CTYPE_INT32 = 1,
  UPB_CTYPE_INT64 = 2,
  UPB_CTYPE_UINT32 = 3,
  UPB_CTYPE_UINT64 = 4,
  UPB_CTYPE_DOUBLE = 5,
  UPB_CTYPE_FLOAT = 6,
  UPB_CTYPE_BOOL = 7,
  UPB_CTYPE_CSTR = 8,
  UPB_CTYPE_PTR = 9,
  UPB_CTYPE_BYTEREGION = 10,
  UPB_CTYPE_FIELDDEF = 11,
} upb_ctype_t;

typedef union {
  uint64_t uint64;
  int32_t int32;
  int64_t int64;
  uint32_t uint32;
  double _double;
  float _float;
  bool _bool;
  char *cstr;
  void *ptr;
  const void *constptr;
} _upb_value;

// A single .proto value.  The owner must have an out-of-band way of knowing
// the type, so that it knows which union member to use.
typedef struct {
  _upb_value val;
#ifndef NDEBUG
  // In debug mode we carry the value type around also so we can check accesses
  // to be sure the right member is being read.
  upb_ctype_t type;
#endif
} upb_value;

#ifdef UPB_C99
#define UPB_VALUE_INIT(v, member) {.member = v}
#endif
// TODO(haberman): C++
//
//
#define UPB__VALUE_INIT_NONE      UPB_VALUE_INIT(NULL, ptr)

#ifdef NDEBUG
#define SET_TYPE(dest, val)      UPB_UNUSED(val)
#define UPB_VALUE_INIT_NONE      {UPB__VALUE_INIT_NONE}
#else
#define SET_TYPE(dest, val) dest = val
// Non-existent type, all reads will fail.
#define UPB_VALUE_INIT_NONE      {UPB__VALUE_INIT_NONE, -1}
#endif

#define UPB_VALUE_INIT_INT32(v)  UPB_VALUE_INIT(v, int32)
#define UPB_VALUE_INIT_INT64(v)  UPB_VALUE_INIT(v, int64)
#define UPB_VALUE_INIT_UINT32(v) UPB_VALUE_INIT(v, uint32)
#define UPB_VALUE_INIT_UINT64(v) UPB_VALUE_INIT(v, uint64)
#define UPB_VALUE_INIT_DOUBLE(v) UPB_VALUE_INIT(v, _double)
#define UPB_VALUE_INIT_FLOAT(v)  UPB_VALUE_INIT(v, _float)
#define UPB_VALUE_INIT_BOOL(v)   UPB_VALUE_INIT(v, _bool)
#define UPB_VALUE_INIT_CSTR(v)   UPB_VALUE_INIT(v, cstr)
#define UPB_VALUE_INIT_PTR(v)    UPB_VALUE_INIT(v, ptr)
#define UPB_VALUE_INIT_CONSTPTR(v) UPB_VALUE_INIT(v, constptr)

UPB_INLINE void _upb_value_setval(upb_value *v, _upb_value val,
                                  upb_ctype_t type) {
  v->val = val;
  SET_TYPE(v->type, type);
}

UPB_INLINE upb_value _upb_value_val(_upb_value val, upb_ctype_t type) {
  upb_value ret;
  _upb_value_setval(&ret, val, type);
  return ret;
}

// For each value type, define the following set of functions:
//
// // Get/set an int32 from a upb_value.
// int32_t upb_value_getint32(upb_value val);
// void upb_value_setint32(upb_value *val, int32_t cval);
//
// // Construct a new upb_value from an int32.
// upb_value upb_value_int32(int32_t val);

#define WRITERS(name, membername, ctype, proto_type) \
  UPB_INLINE void upb_value_set ## name(upb_value *val, ctype cval) { \
    val->val.uint64 = 0; \
    SET_TYPE(val->type, proto_type); \
    val->val.membername = cval; \
  } \
  UPB_INLINE upb_value upb_value_ ## name(ctype val) { \
    upb_value ret; \
    upb_value_set ## name(&ret, val); \
    return ret; \
  }

#define ALL(name, membername, ctype, proto_type) \
  /* Can't reuse WRITERS() here unfortunately because "bool" is a macro \
   * that expands to _Bool, so it ends up defining eg. upb_value_set_Bool */ \
  UPB_INLINE void upb_value_set ## name(upb_value *val, ctype cval) { \
    val->val.uint64 = 0; \
    SET_TYPE(val->type, proto_type); \
    val->val.membername = cval; \
  } \
  UPB_INLINE upb_value upb_value_ ## name(ctype val) { \
    upb_value ret; \
    upb_value_set ## name(&ret, val); \
    return ret; \
  } \
  UPB_INLINE ctype upb_value_get ## name(upb_value val) { \
    assert(val.type == proto_type); \
    return val.val.membername; \
  }

ALL(int32,  int32,   int32_t,  UPB_CTYPE_INT32);
ALL(int64,  int64,   int64_t,  UPB_CTYPE_INT64);
ALL(uint32, uint32,  uint32_t, UPB_CTYPE_UINT32);
ALL(uint64, uint64,  uint64_t, UPB_CTYPE_UINT64);
ALL(bool,   _bool,   bool,     UPB_CTYPE_BOOL);
ALL(cstr,   cstr,    char*,    UPB_CTYPE_CSTR);
ALL(ptr,    ptr,     void*,    UPB_CTYPE_PTR);

#ifdef __KERNEL__
// Linux kernel modules are compiled without SSE and therefore are incapable
// of compiling functions that return floating-point values, so we define as
// macros instead and lose the type check.
WRITERS(double, _double, double,   UPB_CTYPE_DOUBLE);
WRITERS(float,  _float,  float,    UPB_CTYPE_FLOAT);
#define upb_value_getdouble(v) (v.val._double)
#define upb_value_getfloat(v) (v.val._float)
#else
ALL(double, _double, double,   UPB_CTYPE_DOUBLE);
ALL(float,  _float,  float,    UPB_CTYPE_FLOAT);
#endif  /* __KERNEL__ */

#undef WRITERS
#undef ALL

extern upb_value UPB_NO_VALUE;

#ifdef __cplusplus
}  // extern "C"

namespace upb {

typedef upb_value Value;

template <typename T> T GetValue(Value v);
template <typename T> Value MakeValue(T v);

#define UPB_VALUE_ACCESSORS(type, ctype) \
  template <> inline ctype GetValue<ctype>(Value v) { \
    return upb_value_get ## type(v); \
  } \
  template <> inline Value MakeValue<ctype>(ctype v) { \
    return upb_value_ ## type(v); \
  }

UPB_VALUE_ACCESSORS(double, double);
UPB_VALUE_ACCESSORS(float,  float);
UPB_VALUE_ACCESSORS(int32,  int32_t);
UPB_VALUE_ACCESSORS(int64,  int64_t);
UPB_VALUE_ACCESSORS(uint32, uint32_t);
UPB_VALUE_ACCESSORS(uint64, uint64_t);
UPB_VALUE_ACCESSORS(bool,   bool);

#undef UPB_VALUE_ACCESSORS

template <typename T> inline T* GetPtrValue(Value v) {
  return static_cast<T*>(upb_value_getptr(v));
}
template <typename T> inline Value MakePtrValue(T* v) {
  return upb_value_ptr(static_cast<void*>(v));
}

// C++ Wrappers
inline Status::Status() { upb_status_init(this); }
inline Status::~Status() { upb_status_uninit(this); }
inline bool Status::ok() const { return upb_ok(this); }
inline bool Status::eof() const { return upb_eof(this); }
inline const char *Status::GetString() const { return upb_status_getstr(this); }
inline void Status::SetEof() { upb_status_seteof(this); }
inline void Status::SetErrorLiteral(const char* msg) {
  upb_status_seterrliteral(this, msg);
}
inline void Status::Clear() { upb_status_clear(this); }

}  // namespace upb

#endif

#endif  /* UPB_H_ */
