/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 *
 */

#include <stdarg.h>
#include <stddef.h>
#include <string.h>

#include "upb.h"
#include "upb_string.h"

#define alignof(t) offsetof(struct { char c; t x; }, x)
#define TYPE_INFO(wire_type, ctype, allows_delimited) \
    {alignof(ctype), sizeof(ctype), wire_type, \
     (1 << wire_type) | (allows_delimited << UPB_WIRE_TYPE_DELIMITED), \
     #ctype},

upb_type_info upb_types[] = {
  {0, 0, 0, 0, ""}, // There is no type 0.
  TYPE_INFO(UPB_WIRE_TYPE_64BIT,       double,    1)    // DOUBLE
  TYPE_INFO(UPB_WIRE_TYPE_32BIT,       float,     1)    // FLOAT
  TYPE_INFO(UPB_WIRE_TYPE_VARINT,      int64_t,   1)    // INT64
  TYPE_INFO(UPB_WIRE_TYPE_VARINT,      uint64_t,  1)    // UINT64
  TYPE_INFO(UPB_WIRE_TYPE_VARINT,      int32_t,   1)    // INT32
  TYPE_INFO(UPB_WIRE_TYPE_64BIT,       uint64_t,  1)    // FIXED64
  TYPE_INFO(UPB_WIRE_TYPE_32BIT,       uint32_t,  1)    // FIXED32
  TYPE_INFO(UPB_WIRE_TYPE_VARINT,      bool,      1)    // BOOL
  TYPE_INFO(UPB_WIRE_TYPE_DELIMITED,   void*,     1)    // STRING
  TYPE_INFO(UPB_WIRE_TYPE_START_GROUP, void*,     0)    // GROUP
  TYPE_INFO(UPB_WIRE_TYPE_DELIMITED,   void*,     1)    // MESSAGE
  TYPE_INFO(UPB_WIRE_TYPE_DELIMITED,   void*,     1)    // BYTES
  TYPE_INFO(UPB_WIRE_TYPE_VARINT,      uint32_t,  1)    // UINT32
  TYPE_INFO(UPB_WIRE_TYPE_VARINT,      uint32_t,  1)    // ENUM
  TYPE_INFO(UPB_WIRE_TYPE_32BIT,       int32_t,   1)    // SFIXED32
  TYPE_INFO(UPB_WIRE_TYPE_64BIT,       int64_t,   1)    // SFIXED64
  TYPE_INFO(UPB_WIRE_TYPE_VARINT,      int32_t,   1)    // SINT32
  TYPE_INFO(UPB_WIRE_TYPE_VARINT,      int64_t,   1)    // SINT64
};

void upb_seterr(upb_status *status, enum upb_status_code code,
                const char *msg, ...)
{
  if(upb_ok(status)) {  // The first error is the most interesting.
    status->code = code;
    upb_string_recycle(&status->str);
    va_list args;
    va_start(args, msg);
    upb_string_vprintf(status->str, msg, args);
    va_end(args);
  }
}

void upb_copyerr(upb_status *to, upb_status *from)
{
  to->code = from->code;
  if(from->str) to->str = upb_string_getref(from->str);
}

void upb_clearerr(upb_status *status) {
  status->code = UPB_STATUS_OK;
  upb_string_unref(status->str);
  status->str = NULL;
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
