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

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>  // only for size_t.
#include <assert.h>
#include "descriptor_const.h"
#include "upb_atomic.h"

#ifdef __cplusplus
extern "C" {
#endif

// inline if possible, emit standalone code if required.
#ifndef INLINE
#define INLINE static inline
#endif

#define UPB_MAX(x, y) ((x) > (y) ? (x) : (y))
#define UPB_MIN(x, y) ((x) < (y) ? (x) : (y))
#define UPB_INDEX(base, i, m) (void*)((char*)(base) + ((i)*(m)))

INLINE void nop_printf(const char *fmt, ...) {
  (void)fmt;
}

#ifdef NDEBUG
#define DEBUGPRINTF nop_printf
#else
#define DEBUGPRINTF printf
#endif

// Rounds val up to the next multiple of align.
INLINE size_t upb_align_up(size_t val, size_t align) {
  return val % align == 0 ? val : val + align - (val % align);
}


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


/* Fundamental types and type constants. **************************************/

// A list of types as they are encoded on-the-wire.
enum upb_wire_type {
  UPB_WIRE_TYPE_VARINT      = 0,
  UPB_WIRE_TYPE_64BIT       = 1,
  UPB_WIRE_TYPE_DELIMITED   = 2,
  UPB_WIRE_TYPE_START_GROUP = 3,
  UPB_WIRE_TYPE_END_GROUP   = 4,
  UPB_WIRE_TYPE_32BIT       = 5,
};

// Type of a field as defined in a .proto file.  eg. string, int32, etc.  The
// integers that represent this are defined by descriptor.proto.  Note that
// descriptor.proto reserves "0" for errors, and we use it to represent
// exceptional circumstances.
typedef uint8_t upb_fieldtype_t;

// For referencing the type constants tersely.
#define UPB_TYPE(type) GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_ ## type
#define UPB_LABEL(type) GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_LABEL_ ## type

// Info for a given field type.
typedef struct {
  uint8_t align;
  uint8_t size;
  uint8_t native_wire_type;
  uint8_t inmemory_type;    // For example, INT32, SINT32, and SFIXED32 -> INT32
  char *ctype;
} upb_type_info;

// A static array of info about all of the field types, indexed by type number.
extern const upb_type_info upb_types[];


/* Polymorphic values of .proto types *****************************************/

struct _upb_string;
typedef struct _upb_string upb_string;
struct _upb_array;
typedef struct _upb_array upb_array;
struct _upb_msg;
typedef struct _upb_msg upb_msg;
struct _upb_bytesrc;
typedef struct _upb_bytesrc upb_bytesrc;
struct _upb_fielddef;
typedef struct _upb_fielddef upb_fielddef;

typedef int32_t upb_strlen_t;
#define UPB_STRLEN_MAX INT32_MAX

// The type of a upb_value.  This is like a upb_fieldtype_t, but adds the
// constant UPB_VALUETYPE_ARRAY to represent an array.
typedef uint8_t upb_valuetype_t;
#define UPB_TYPE_ENDGROUP 19  // Need to increase if more real types are added!
#define UPB_VALUETYPE_ARRAY 32
#define UPB_VALUETYPE_BYTESRC 32
#define UPB_VALUETYPE_RAW 33
#define UPB_VALUETYPE_FIELDDEF 34

// A single .proto value.  The owner must have an out-of-band way of knowing
// the type, so that it knows which union member to use.
typedef struct {
  union {
    uint64_t uint64;
    double _double;
    float _float;
    int32_t int32;
    int64_t int64;
    uint32_t uint32;
    bool _bool;
    upb_string *str;
    upb_bytesrc *bytesrc;
    upb_msg *msg;
    upb_array *arr;
    upb_atomic_t *refcount;
    upb_fielddef *fielddef;
    void *_void;
  } val;

  // In debug mode we carry the value type around also so we can check accesses
  // to be sure the right member is being read.
#ifndef NDEBUG
  upb_valuetype_t type;
#endif
} upb_value;

#ifdef NDEBUG
#define SET_TYPE(dest, val)
#else
#define SET_TYPE(dest, val) dest = val
#endif

#define UPB_VALUE_ACCESSORS(name, membername, ctype, proto_type) \
  INLINE ctype upb_value_get ## name(upb_value val) { \
    assert(val.type == proto_type || val.type == UPB_VALUETYPE_RAW); \
    return val.val.membername; \
  } \
  INLINE void upb_value_set ## name(upb_value *val, ctype cval) { \
    SET_TYPE(val->type, proto_type); \
    val->val.membername = cval; \
  }
UPB_VALUE_ACCESSORS(double, _double, double, UPB_TYPE(DOUBLE));
UPB_VALUE_ACCESSORS(float, _float, float, UPB_TYPE(FLOAT));
UPB_VALUE_ACCESSORS(int32, int32, int32_t, UPB_TYPE(INT32));
UPB_VALUE_ACCESSORS(int64, int64, int64_t, UPB_TYPE(INT64));
UPB_VALUE_ACCESSORS(uint32, uint32, uint32_t, UPB_TYPE(UINT32));
UPB_VALUE_ACCESSORS(uint64, uint64, uint64_t, UPB_TYPE(UINT64));
UPB_VALUE_ACCESSORS(bool, _bool, bool, UPB_TYPE(BOOL));
UPB_VALUE_ACCESSORS(str, str, upb_string*, UPB_TYPE(STRING));
UPB_VALUE_ACCESSORS(msg, msg, upb_msg*, UPB_TYPE(MESSAGE));
UPB_VALUE_ACCESSORS(arr, arr, upb_array*, UPB_VALUETYPE_ARRAY);
UPB_VALUE_ACCESSORS(bytesrc, bytesrc, upb_bytesrc*, UPB_VALUETYPE_BYTESRC);
UPB_VALUE_ACCESSORS(fielddef, fielddef, upb_fielddef*, UPB_VALUETYPE_FIELDDEF);

extern upb_value UPB_NO_VALUE;

INLINE upb_atomic_t *upb_value_getrefcount(upb_value val) {
  assert(val.type == UPB_TYPE(MESSAGE) ||
         val.type == UPB_TYPE(STRING) ||
         val.type == UPB_VALUETYPE_ARRAY);
  return val.val.refcount;
}

// Status codes used as a return value.  Codes >0 are not fatal and can be
// resumed.
enum upb_status_code {
  // The operation completed successfully.
  UPB_OK = 0,

  // The bytesrc is at EOF and all data was read successfully.
  UPB_EOF = 1,

  // A read or write from a streaming src/sink could not be completed right now.
  UPB_TRYAGAIN = 2,

  // An unrecoverable error occurred.
  UPB_ERROR = -1,

  // A recoverable error occurred (for example, data of the wrong type was
  // encountered which we can skip over).
  // UPB_STATUS_RECOVERABLE_ERROR = -2
};

// TODO: consider adding error space and code, to let ie. errno be stored
// as a proper code, or application-specific error codes.
struct _upb_status {
  char code;
  upb_string *str;
};

typedef struct _upb_status upb_status;

#define UPB_STATUS_INIT {UPB_OK, NULL}
#define UPB_ERRORMSG_MAXLEN 256

INLINE bool upb_ok(upb_status *status) {
  return status->code == UPB_OK;
}

INLINE void upb_status_init(upb_status *status) {
  status->code = UPB_OK;
  status->str = NULL;
}

void upb_status_uninit(upb_status *status);

// Caller owns a ref on the returned string.
upb_string *upb_status_tostring(upb_status *status);
void upb_printerr(upb_status *status);
void upb_clearerr(upb_status *status);
void upb_seterr(upb_status *status, enum upb_status_code code, const char *msg,
                ...);
void upb_copyerr(upb_status *to, upb_status *from);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_H_ */
