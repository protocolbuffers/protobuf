/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * This file contains shared definitions that are widely used across upb.
 */

#ifndef UPB_H_
#define UPB_H_

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// inline if possible, emit standalone code if required.
#ifndef INLINE
#define INLINE static inline
#endif

#ifdef __GNUC__
#define UPB_NORETURN __attribute__((__noreturn__))
#else
#define UPB_NORETURN
#endif

#define UPB_MAX(x, y) ((x) > (y) ? (x) : (y))
#define UPB_MIN(x, y) ((x) < (y) ? (x) : (y))

// The maximum that any submessages can be nested.  Matches proto2's limit.
// At the moment this specifies the size of several statically-sized arrays
// and therefore setting it high will cause more memory to be used.  Will
// be replaced by a runtime-configurable limit and dynamically-resizing arrays.
// TODO: make this a runtime-settable property of upb_handlers.
#define UPB_MAX_NESTING 64

// The maximum number of fields that any one .proto type can have.  Note that
// this is very different than the max field number.  It is hard to imagine a
// scenario where more than 2k fields (each with its own name and field number)
// makes sense.  The .proto file to describe it would be 2000 lines long and
// contain 2000 unique names.
//
// With this limit we can store a has-bit offset in 8 bits (2**8 * 8 = 2048)
// and we can store a value offset in 16 bits, since the maximum message
// size is 16,640 bytes (2**8 has-bits + 2048 * 8-byte value).  Note that
// strings and arrays are not counted in this, only the *pointer* to them is.
// An individual string or array is unaffected by this 16k byte limit.
#define UPB_MAX_FIELDS (2048)

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


/* upb_value ******************************************************************/

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
  UPB_CTYPE_PTR = 8,
  UPB_CTYPE_BYTEREGION = 9,
  UPB_CTYPE_FIELDDEF = 10,
} upb_ctype_t;

struct _upb_byteregion;
struct _upb_fielddef;

// A single .proto value.  The owner must have an out-of-band way of knowing
// the type, so that it knows which union member to use.
typedef struct {
  union {
    uint64_t uint64;
    int32_t int32;
    int64_t int64;
    uint32_t uint32;
    double _double;
    float _float;
    bool _bool;
    void *_void;
    struct _upb_byteregion *byteregion;
    const struct _upb_fielddef *fielddef;
  } val;

#ifndef NDEBUG
  // In debug mode we carry the value type around also so we can check accesses
  // to be sure the right member is being read.
  upb_ctype_t type;
#endif
} upb_value;

#ifdef NDEBUG
#define SET_TYPE(dest, val)
#else
#define SET_TYPE(dest, val) dest = val
#endif

// For each value type, define the following set of functions:
//
// // Get/set an int32 from a upb_value.
// int32_t upb_value_getint32(upb_value val);
// void upb_value_setint32(upb_value *val, int32_t cval);
//
// // Construct a new upb_value from an int32.
// upb_value upb_value_int32(int32_t val);

#define UPB_VALUE_ACCESSORS(name, membername, ctype, proto_type) \
  INLINE ctype upb_value_get ## name(upb_value val) { \
    assert(val.type == proto_type); \
    return val.val.membername; \
  } \
  INLINE void upb_value_set ## name(upb_value *val, ctype cval) { \
    val->val.uint64 = 0; \
    SET_TYPE(val->type, proto_type); \
    val->val.membername = cval; \
  } \
  INLINE upb_value upb_value_ ## name(ctype val) { \
    upb_value ret; \
    upb_value_set ## name(&ret, val); \
    return ret; \
  }

UPB_VALUE_ACCESSORS(int32,  int32,   int32_t,  UPB_CTYPE_INT32);
UPB_VALUE_ACCESSORS(int64,  int64,   int64_t,  UPB_CTYPE_INT64);
UPB_VALUE_ACCESSORS(uint32, uint32,  uint32_t, UPB_CTYPE_UINT32);
UPB_VALUE_ACCESSORS(uint64, uint64,  uint64_t, UPB_CTYPE_UINT64);
UPB_VALUE_ACCESSORS(double, _double, double,   UPB_CTYPE_DOUBLE);
UPB_VALUE_ACCESSORS(float,  _float,  float,    UPB_CTYPE_FLOAT);
UPB_VALUE_ACCESSORS(bool,   _bool,   bool,     UPB_CTYPE_BOOL);
UPB_VALUE_ACCESSORS(ptr,    _void,   void*,    UPB_CTYPE_PTR);
UPB_VALUE_ACCESSORS(byteregion, byteregion, struct _upb_byteregion*,
                    UPB_CTYPE_BYTEREGION);

// upb_fielddef should never be modified from a callback
// (ie. when they're getting passed through a upb_value).
UPB_VALUE_ACCESSORS(fielddef, fielddef, const struct _upb_fielddef*,
                    UPB_CTYPE_FIELDDEF);

#undef UPB_VALUE_ACCESSORS

extern upb_value UPB_NO_VALUE;


/* upb_status *****************************************************************/

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

typedef struct {
  bool error;
  bool eof;

  // Specific status code defined by some error space (optional).
  int code;
  upb_errorspace *space;

  // Error message (optional).
  const char *str;  // NULL when no message is present.  NULL-terminated.
  char *buf;        // Owned by the status.
  size_t bufsize;
} upb_status;

#define UPB_STATUS_INIT {UPB_OK, false, 0, NULL, NULL, NULL, 0}

void upb_status_init(upb_status *status);
void upb_status_uninit(upb_status *status);

INLINE bool upb_ok(const upb_status *status) { return !status->error; }
INLINE bool upb_eof(const upb_status *status) { return status->eof; }

void upb_status_clear(upb_status *status);
void upb_status_seterrliteral(upb_status *status, const char *msg);
void upb_status_seterrf(upb_status *s, const char *msg, ...);
void upb_status_setcode(upb_status *s, upb_errorspace *space, int code);
INLINE void upb_status_seteof(upb_status *s) { s->eof = true; }
// The returned string is invalidated by any other call into the status.
const char *upb_status_getstr(const upb_status *s);
void upb_status_copy(upb_status *to, const upb_status *from);

extern upb_errorspace upb_posix_errorspace;
void upb_status_fromerrno(upb_status *status);
bool upb_errno_is_wouldblock();

// Like vasprintf (which allocates a string large enough for the result), but
// uses *buf (which can be NULL) as a starting point and reallocates it only if
// the new value will not fit.  "size" is updated to reflect the allocated size
// of the buffer.  Starts writing at the given offset into the string; bytes
// preceding this offset are unaffected.  Returns the new length of the string,
// or -1 on memory allocation failure.
int upb_vrprintf(char **buf, size_t *size, size_t ofs,
                 const char *fmt, va_list args);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_H_ */
