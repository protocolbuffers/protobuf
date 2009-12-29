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

struct upb_type_info upb_type_info[] = {
  TYPE_INFO(DOUBLE,   UPB_WIRE_TYPE_64BIT,       double)
  TYPE_INFO(FLOAT,    UPB_WIRE_TYPE_32BIT,       float)
  TYPE_INFO(INT64,    UPB_WIRE_TYPE_VARINT,      int64_t)
  TYPE_INFO(UINT64,   UPB_WIRE_TYPE_VARINT,      uint64_t)
  TYPE_INFO(INT32,    UPB_WIRE_TYPE_VARINT,      int32_t)
  TYPE_INFO(FIXED64,  UPB_WIRE_TYPE_64BIT,       uint64_t)
  TYPE_INFO(FIXED32,  UPB_WIRE_TYPE_32BIT,       uint32_t)
  TYPE_INFO(BOOL,     UPB_WIRE_TYPE_VARINT,      bool)
  TYPE_INFO(MESSAGE,  UPB_WIRE_TYPE_DELIMITED,   void*)
  TYPE_INFO(GROUP,    UPB_WIRE_TYPE_START_GROUP, void*)
  TYPE_INFO(UINT32,   UPB_WIRE_TYPE_VARINT,      uint32_t)
  TYPE_INFO(ENUM,     UPB_WIRE_TYPE_VARINT,      uint32_t)
  TYPE_INFO(SFIXED32, UPB_WIRE_TYPE_32BIT,       int32_t)
  TYPE_INFO(SFIXED64, UPB_WIRE_TYPE_64BIT,       int64_t)
  TYPE_INFO(SINT32,   UPB_WIRE_TYPE_VARINT,      int32_t)
  TYPE_INFO(SINT64,   UPB_WIRE_TYPE_VARINT,      int64_t)
  TYPE_INFO(STRING,   UPB_WIRE_TYPE_DELIMITED,   union upb_string*)
  TYPE_INFO(BYTES,    UPB_WIRE_TYPE_DELIMITED,   union upb_string*)
};

void upb_seterr(struct upb_status *status, enum upb_status_code code,
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
