/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Implements routines for serializing protobufs.  For serializing an entire
 * upb_msg, see the serialization routines in upb_msg.h (which are layered on
 * top of this).
 *
 * By default this interface does not "check your work."  It doesn't pay
 * attention to whether the lengths you give for submessages are correct, or
 * whether your groups are properly balanced, or whether you give each value a
 * tag of the appropriate type.  In other words, it is quite possible (easy,
 * even) to use this interface to emit invalid protobufs.  We don't want to
 * pay for the runtime checks.
 *
 * The best way to test that you're using the API correctly is to try to parse
 * your output.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 */

#ifndef UPB_SERIALIZE_H_
#define UPB_SERIALIZE_H_

#include "upb.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Functions to write wire values. ********************************************/

/* Puts a varint (wire type: UPB_WIRE_TYPE_VARINT). */
INLINE upb_status_t upb_put_v_uint64_t(uint8_t *buf, uint8_t *end, uint64_t val,
                                       uint8_t **outbuf)
{
  do {
    uint8_t byte = val & 0x7f;
    val >>= 7;
    if(val) byte |= 0x80;
    if(buf >= end) return UPB_STATUS_NEED_MORE_DATA;
    *buf++ = byte;
  } while(val);
  *outbuf = buf;
  return UPB_STATUS_OK;
}

/* Puts a varint -- called when we only have 32 bits of data. */
INLINE upb_status_t upb_put_v_uint32_t(uint8_t *buf, uint8_t *end,
                                       uint32_t val, uint8_t **outbuf)
{
  return UPB_STATUS_OK;
}

/* Puts a fixed-length 32-bit integer (wire type: UPB_WIRE_TYPE_32BIT). */
INLINE upb_status_t upb_put_f_uint32_t(uint8_t *buf, uint8_t *end,
                                       uint32_t val, uint8_t **outbuf)
{
  uint8_t *uint32_end = buf + sizeof(uint32_t);
  if(uint32_end > end) return UPB_STATUS_NEED_MORE_DATA;
  *(uint32_t*)buf = val;
  *outbuf = uint32_end;
  return UPB_STATUS_OK;
}

/* Puts a fixed-length 64-bit integer (wire type: UPB_WIRE_TYPE_64BIT). */
INLINE upb_status_t upb_put_f_uint64_t(uint8_t *buf, uint8_t *end,
                                       uint64_t val, uint8_t **outbuf)
{
  uint8_t *uint64_end = buf + sizeof(uint64_t);
  if(uint64_end > end) return UPB_STATUS_NEED_MORE_DATA;
  *(uint64_t*)buf = val;
  *outbuf = uint64_end;
  return UPB_STATUS_OK;
}

/* Functions to write .proto values. ******************************************/

/* Performs zig-zag encoding, which is used by sint32 and sint64. */
INLINE uint32_t upb_zzenc_32(int32_t n) { return (n << 1) ^ (n >> 31); }
INLINE uint64_t upb_zzenc_64(int64_t n) { return (n << 1) ^ (n >> 63); }

/* Use macros to define a set of two functions for each .proto type:
 *
 *  // Converts and writes a .proto value into buf.  "end" indicates the end
 *  // of the current available buffer (if the buffer does not contain enough
 *  // space UPB_STATUS_NEED_MORE_DATA is returned).  On success, *outbuf will
 *  // point one past the data that was written.
 *  upb_status_t upb_put_INT32(uint8_t *buf, uint8_t *end, int32_t val,
 *                             uint8_t **outbuf);
 *
 *  // Given a .proto value s (source) convert it to a wire value.
 *  uint32_t upb_vtowv_INT32(int32_t s);
 */

#define VTOWV(type, wire_t, val_t) \
  INLINE wire_t upb_vtowv_ ## type(val_t s)

#define PUT(type, v_or_f, wire_t, val_t, member_name) \
  INLINE upb_status_t upb_put_ ## type(uint8_t *buf, uint8_t *end, val_t val, \
                                       uint8_t **outbuf) { \
    wire_t tmp = upb_vtowv_ ## type(val); \
    UPB_CHECK(upb_put_ ## v_or_f ## _ ## wire_t(buf, end, tmp, outbuf)); \
    return UPB_STATUS_OK; \
  }

#define T(type, v_or_f, wire_t, val_t, member_name) \
  VTOWV(type, wire_t, val_t);  /* prototype for PUT below */ \
  PUT(type, v_or_f, wire_t, val_t, member_name) \
  VTOWV(type, wire_t, val_t)

T(INT32,    v, uint32_t, int32_t,  int32)   { return (uint32_t)s;              }
T(INT64,    v, uint64_t, int64_t,  int64)   { return (uint64_t)s;              }
T(UINT32,   v, uint32_t, uint32_t, uint32)  { return s;                        }
T(UINT64,   v, uint64_t, uint64_t, uint64)  { return s;                        }
T(SINT32,   v, uint32_t, int32_t,  int32)   { return upb_zzenc_32(s);          }
T(SINT64,   v, uint64_t, int64_t,  int64)   { return upb_zzdec_64(s);          }
T(FIXED32,  f, uint32_t, uint32_t, uint32)  { return s;                        }
T(FIXED64,  f, uint64_t, uint64_t, uint64)  { return s;                        }
T(SFIXED32, f, uint32_t, int32_t,  int32)   { return (uint32_t)s;              }
T(SFIXED64, f, uint64_t, int64_t,  int64)   { return (uint64_t)s;              }
T(BOOL,     v, uint32_t, bool,     _bool)   { return (uint32_t)s;              }
T(ENUM,     v, uint32_t, int32_t,  int32)   { return (uint32_t)s;              }
T(DOUBLE,   f, uint64_t, double,   _double) {
  union upb_value v;
  v._double = s;
  return v.uint64;
}
T(FLOAT,    f, uint32_t, float,    _float)  {
  union upb_value v;
  v._float = s;
  return v.uint32;
}
#undef VTOWV
#undef PUT
#undef T

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_SERIALIZE_H_ */
