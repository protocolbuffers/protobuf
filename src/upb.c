/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 */

#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include "descriptor_const.h"
#include "upb.h"
#include "upb_string.h"

#define alignof(t) offsetof(struct { char c; t x; }, x)
#define TYPE_INFO(wire_type, ctype, inmemory_type) \
    {alignof(ctype), sizeof(ctype), wire_type, UPB_TYPE(inmemory_type), #ctype},

const upb_type_info upb_types[] = {
  {0, 0, 0, 0, ""}, // There is no type 0.
  TYPE_INFO(UPB_WIRE_TYPE_64BIT,       double,    DOUBLE)    // DOUBLE
  TYPE_INFO(UPB_WIRE_TYPE_32BIT,       float,     FLOAT)     // FLOAT
  TYPE_INFO(UPB_WIRE_TYPE_VARINT,      int64_t,   INT64)     // INT64
  TYPE_INFO(UPB_WIRE_TYPE_VARINT,      uint64_t,  UINT64)    // UINT64
  TYPE_INFO(UPB_WIRE_TYPE_VARINT,      int32_t,   INT32)     // INT32
  TYPE_INFO(UPB_WIRE_TYPE_64BIT,       uint64_t,  UINT64)    // FIXED64
  TYPE_INFO(UPB_WIRE_TYPE_32BIT,       uint32_t,  UINT32)    // FIXED32
  TYPE_INFO(UPB_WIRE_TYPE_VARINT,      bool,      BOOL)      // BOOL
  TYPE_INFO(UPB_WIRE_TYPE_DELIMITED,   void*,     STRING)    // STRING
  TYPE_INFO(UPB_WIRE_TYPE_START_GROUP, void*,     MESSAGE)   // GROUP
  TYPE_INFO(UPB_WIRE_TYPE_DELIMITED,   void*,     MESSAGE)   // MESSAGE
  TYPE_INFO(UPB_WIRE_TYPE_DELIMITED,   void*,     STRING)    // BYTES
  TYPE_INFO(UPB_WIRE_TYPE_VARINT,      uint32_t,  UINT32)    // UINT32
  TYPE_INFO(UPB_WIRE_TYPE_VARINT,      uint32_t,  INT32)     // ENUM
  TYPE_INFO(UPB_WIRE_TYPE_32BIT,       int32_t,   INT32)     // SFIXED32
  TYPE_INFO(UPB_WIRE_TYPE_64BIT,       int64_t,   INT64)     // SFIXED64
  TYPE_INFO(UPB_WIRE_TYPE_VARINT,      int32_t,   INT32)     // SINT32
  TYPE_INFO(UPB_WIRE_TYPE_VARINT,      int64_t,   INT64)     // SINT64
  TYPE_INFO(UPB_WIRE_TYPE_END_GROUP,   void*,     INT64)     // SINT64
};

#ifdef NDEBUG
upb_value UPB_NO_VALUE = {{0}};
#else
upb_value UPB_NO_VALUE = {{0}, UPB_VALUETYPE_RAW};
#endif

void upb_seterr(upb_status *status, enum upb_status_code code,
                const char *msg, ...) {
  status->code = code;
  upb_string_recycle(&status->str);
  va_list args;
  va_start(args, msg);
  upb_string_vprintf(status->str, msg, args);
  va_end(args);
}

void upb_copyerr(upb_status *to, upb_status *from)
{
  to->code = from->code;
  if(from->str) to->str = upb_string_getref(from->str);
}

void upb_clearerr(upb_status *status) {
  status->code = UPB_OK;
  if (status->str) upb_string_recycle(&status->str);
}

void upb_printerr(upb_status *status) {
  if(status->str) {
    fprintf(stderr, "code: %d, msg: " UPB_STRFMT "\n",
            status->code, UPB_STRARG(status->str));
  } else {
    fprintf(stderr, "code: %d, no msg\n", status->code);
  }
}

void upb_status_uninit(upb_status *status) {
  upb_string_unref(status->str);
}
