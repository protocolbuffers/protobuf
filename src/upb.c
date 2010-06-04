/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 *
 */

#include <stdarg.h>
#include <stddef.h>

#include "upb.h"

#define alignof(t) offsetof(struct { char c; t x; }, x)
#define TYPE_INFO(proto_type, wire_type, ctype) \
    [GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_ ## proto_type] = \
    {alignof(ctype), sizeof(ctype), wire_type, #ctype},

// With packed fields, any type expecting 32-bit, 64-bit or varint can instead
// receive delimited.
upb_type_info upb_types[] = {
  TYPE_INFO(DOUBLE,   (1<<UPB_WIRE_TYPE_64BIT)|(1<<UPB_WIRE_TYPE_DELIMITED),       double)
  TYPE_INFO(FLOAT,    (1<<UPB_WIRE_TYPE_32BIT|(1<<UPB_WIRE_TYPE_DELIMITED),       float)
  TYPE_INFO(INT64,    (1<<UPB_WIRE_TYPE_VARINT|(1<<UPB_WIRE_TYPE_DELIMITED),      int64_t)
  TYPE_INFO(UINT64,   (1<<UPB_WIRE_TYPE_VARINT|(1<<UPB_WIRE_TYPE_DELIMITED),      uint64_t)
  TYPE_INFO(INT32,    (1<<UPB_WIRE_TYPE_VARINT|(1<<UPB_WIRE_TYPE_DELIMITED),      int32_t)
  TYPE_INFO(FIXED64,  (1<<UPB_WIRE_TYPE_64BIT|(1<<UPB_WIRE_TYPE_DELIMITED),       uint64_t)
  TYPE_INFO(FIXED32,  (1<<UPB_WIRE_TYPE_32BIT|(1<<UPB_WIRE_TYPE_DELIMITED),       uint32_t)
  TYPE_INFO(BOOL,     (1<<UPB_WIRE_TYPE_VARINT|(1<<UPB_WIRE_TYPE_DELIMITED),      bool)
  TYPE_INFO(MESSAGE,  (1<<UPB_WIRE_TYPE_DELIMITED,   void*)
  TYPE_INFO(GROUP,    (1<<UPB_WIRE_TYPE_START_GROUP, void*)
  TYPE_INFO(UINT32,   (1<<UPB_WIRE_TYPE_VARINT)|(1<<UPB_WIRE_TYPE_DELIMITED),      uint32_t)
  TYPE_INFO(ENUM,     (1<<UPB_WIRE_TYPE_VARINT)|(1<<UPB_WIRE_TYPE_DELIMITED),      uint32_t)
  TYPE_INFO(SFIXED32, (1<<UPB_WIRE_TYPE_32BIT)|(1<<UPB_WIRE_TYPE_DELIMITED),       int32_t)
  TYPE_INFO(SFIXED64, (1<<UPB_WIRE_TYPE_64BIT)|(1<<UPB_WIRE_TYPE_DELIMITED),       int64_t)
  TYPE_INFO(SINT32,   (1<<UPB_WIRE_TYPE_VARINT)|(1<<UPB_WIRE_TYPE_DELIMITED),      int32_t)
  TYPE_INFO(SINT64,   (1<<UPB_WIRE_TYPE_VARINT)|(1<<UPB_WIRE_TYPE_DELIMITED),      int64_t)
  TYPE_INFO(STRING,   (1<<UPB_WIRE_TYPE_DELIMITED),   upb_strptr)
  TYPE_INFO(BYTES,    (1<<UPB_WIRE_TYPE_DELIMITED),   upb_strptr)
};

void upb_seterr(upb_status *status, enum upb_status_code code,
                const char *msg, ...)
{
  if(upb_ok(status)) {  // The first error is the most interesting.
    status->code = code;
    va_list args;
    va_start(args, msg);
    vsnprintf(status->msg, UPB_ERRORMSG_MAXLEN, msg, args);
    va_end(args);
  }
}
