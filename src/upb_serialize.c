/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 */

#include "upb_serialize.h"
#include "descriptor.h"

upb_status_t upb_serialize_value(uint8_t *buf, uint8_t *end, upb_field_type_t ft,
                                 union upb_value_ptr v, uint8_t **outbuf)
{
#define CASE(t, member_name) \
  case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_ ## t: \
    return upb_put_ ## t(buf, end, *v.member_name, outbuf);
  switch(ft) {
    CASE(DOUBLE,   _double)
    CASE(FLOAT,    _float)
    CASE(INT32,    int32)
    CASE(INT64,    int64)
    CASE(UINT32,   uint32)
    CASE(UINT64,   uint64)
    CASE(SINT32,   int32)
    CASE(SINT64,   int64)
    CASE(FIXED32,  uint32)
    CASE(FIXED64,  uint64)
    CASE(SFIXED32, int32)
    CASE(SFIXED64, int64)
    CASE(BOOL,     _bool)
    CASE(ENUM,     int32)
    default: return UPB_ERROR_ILLEGAL;
  }
#undef CASE
}
