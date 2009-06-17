/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2008-2009 Joshua Haberman.  See LICENSE for details.
 */

#define INLINE
#include "upb_parse.h"

#include <assert.h>
#include <string.h>
#include "descriptor.h"

/* Branch prediction hints for GCC. */
#ifdef __GNUC__
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)
#else
#define likely(x)       (x)
#define unlikely(x)     (x)
#endif

#define CHECK(func) do { \
  upb_status_t status = func; \
  if(status != UPB_STATUS_OK) return status; \
  } while (0)

/* Lowest-level functions -- these read integers from the input buffer.
 * To avoid branches, none of these do bounds checking.  So we force clients
 * to overallocate their buffers by >=9 bytes. */

static upb_status_t get_v_uint64_t(uint8_t *restrict *buf,
                                   uint64_t *restrict val)
{
  uint8_t *ptr = *buf, b;
  uint32_t part0 = 0, part1 = 0, part2 = 0;

  /* From the original proto2 implementation. */
  b = *(ptr++); part0  = (b & 0x7F)      ; if (!(b & 0x80)) goto done;
  b = *(ptr++); part0 |= (b & 0x7F) <<  7; if (!(b & 0x80)) goto done;
  b = *(ptr++); part0 |= (b & 0x7F) << 14; if (!(b & 0x80)) goto done;
  b = *(ptr++); part0 |= (b & 0x7F) << 21; if (!(b & 0x80)) goto done;
  b = *(ptr++); part1  = (b & 0x7F)      ; if (!(b & 0x80)) goto done;
  b = *(ptr++); part1 |= (b & 0x7F) <<  7; if (!(b & 0x80)) goto done;
  b = *(ptr++); part1 |= (b & 0x7F) << 14; if (!(b & 0x80)) goto done;
  b = *(ptr++); part1 |= (b & 0x7F) << 21; if (!(b & 0x80)) goto done;
  b = *(ptr++); part2  = (b & 0x7F)      ; if (!(b & 0x80)) goto done;
  b = *(ptr++); part2 |= (b & 0x7F) <<  7; if (!(b & 0x80)) goto done;
  return UPB_ERROR_UNTERMINATED_VARINT;

done:
  *buf = ptr;
  *val = (uint64_t)part0 | ((uint64_t)part1 << 28) | ((uint64_t)part2 << 56);
  return UPB_STATUS_OK;
}

/* Alternate implementations of get_v_uint64_t -- one performs no branching,
 * another performs only a single branch.  TODO: test performance. */

#if 0
/* The no-branching version. */
static upb_status_t get_v_uint64_t(uint8_t *restrict *buf,
                                   uint64_t *restrict val)
{
  uint8_t *b = *buf;
  /* Endian-specific! */
  uint64_t b0 = *(uint64_t*)b;
  uint16_t b10 = *(uint16_t*)(b+8);

  /* Put the 10 continuation bits in the bottom 10 bits of cont. */
  uint32_t cont = (b0 & 0x8080808080808080ULL) * 0x2040810204081ULL >> 56;
  cont |= (*(b+8) & 0x80) << 1 | (*(b+9) & 0x80) << 2;
  if(cont == 0x3FF) return UPB_ERROR_UNTERMINATED_VARINT;

  int num_bytes = __builtin_ctzll(~cont) + 1;
  *buf += num_bytes;

  uint64_t more_than_8_mask = -(num_bytes > 8);
  uint64_t low10_mask = (0x7F7F7F7F7F7F7F7FULL >> (8-num_bytes)*8);
  low10_mask ^= (low10_mask ^ 0x7F7F7F7F7F7F7F7FULL) & more_than_8_mask;
  uint64_t valb0 = b0 & low10_mask;
  uint16_t valb10 = b10 & (0x7F7FU >> (10-num_bytes)*8) & more_than_8_mask;

  *val = ((valb0 & 0xFF00000000000000) >> 7) |
         ((valb0 & 0x00FF000000000000) >> 6) |
         ((valb0 & 0x0000FF0000000000) >> 5) |
         ((valb0 & 0x000000FF00000000) >> 4) |
         ((valb0 & 0x00000000FF000000) >> 3) |
         ((valb0 & 0x0000000000FF0000) >> 2) |
         ((valb0 & 0x000000000000FF00) >> 1) |
         ((valb0 & 0x00000000000000FF)) |
         (((uint64_t)valb10 & 0xFF00) << 55) |
         (((uint64_t)valb10 & 0xFF) << 56);
  return UPB_STATUS_OK;
}

/* The single-branch version. */
static upb_status_t get_v_uint64_t(uint8_t *restrict *buf,
                                   uint64_t *restrict val)
{
  /* Endian-specific! */
  uint64_t b0 = *(uint64_t*)*buf;

  /* Put the 10 continuation bits in the bottom 10 bits of cont. */
  uint32_t cont = (b0 & 0x8080808080808080ULL) * 0x2040810204081ULL >> 56;
  cont |= (*(*buf+8) & 0x80) << 1 | (*(*buf+9) & 0x80) << 2;

  int num_bytes = __builtin_ctzll(~cont) + 1;
  uint32_t part0 = 0, part1 = 0, part2 = 0;

  switch(num_bytes) {
    default: return UPB_ERROR_UNTERMINATED_VARINT;
    case 10: part2 |= ((*buf)[9] & 0x7F) <<  7;
    case 9:  part2 |= ((*buf)[8] & 0x7F)      ;
    case 8:  part1 |= ((*buf)[7] & 0x7F) << 21;
    case 7:  part1 |= ((*buf)[6] & 0x7F) << 14;
    case 6:  part1 |= ((*buf)[5] & 0x7F) <<  7;
    case 5:  part1 |= ((*buf)[4] & 0x7F)      ;
    case 4:  part0 |= ((*buf)[3] & 0x7F) << 21;
    case 3:  part0 |= ((*buf)[2] & 0x7F) << 14;
    case 2:  part0 |= ((*buf)[1] & 0x7F) <<  7;
    case 1:  part0 |= ((*buf)[0] & 0x7F)      ;
  }
  *buf += num_bytes;
  *val = (uint64_t)part0 | ((uint64_t)part1 << 28) | ((uint64_t)part2 << 56);
  return UPB_STATUS_OK;
}
#endif

static upb_status_t skip_v_uint64_t(uint8_t **buf)
{
  uint8_t *ptr = *buf, b;
  b = *(ptr++); if (!(b & 0x80)) goto done;
  b = *(ptr++); if (!(b & 0x80)) goto done;
  b = *(ptr++); if (!(b & 0x80)) goto done;
  b = *(ptr++); if (!(b & 0x80)) goto done;
  b = *(ptr++); if (!(b & 0x80)) goto done;
  b = *(ptr++); if (!(b & 0x80)) goto done;
  b = *(ptr++); if (!(b & 0x80)) goto done;
  b = *(ptr++); if (!(b & 0x80)) goto done;
  b = *(ptr++); if (!(b & 0x80)) goto done;
  b = *(ptr++); if (!(b & 0x80)) goto done;
  return UPB_ERROR_UNTERMINATED_VARINT;

done:
  *buf = (uint8_t*)ptr;
  return UPB_STATUS_OK;
}

static upb_status_t get_v_uint32_t(uint8_t *restrict *buf,
                                   uint32_t *restrict val)
{
  uint8_t *ptr = *buf, b;
  uint32_t result;

  /* From the original proto2 implementation. */
  b = *(ptr++); result  = (b & 0x7F)      ; if (!(b & 0x80)) goto done;
  b = *(ptr++); result |= (b & 0x7F) <<  7; if (!(b & 0x80)) goto done;
  b = *(ptr++); result |= (b & 0x7F) << 14; if (!(b & 0x80)) goto done;
  b = *(ptr++); result |= (b & 0x7F) << 21; if (!(b & 0x80)) goto done;
  b = *(ptr++); result  = (b & 0x7F) << 28; if (!(b & 0x80)) goto done;
  return UPB_ERROR_UNTERMINATED_VARINT;

done:
  *buf = ptr;
  *val = result;
  return UPB_STATUS_OK;
}

static upb_status_t get_f_uint32_t(uint8_t *restrict *buf,
                                   uint32_t *restrict val)
{
  uint8_t *b = *buf;
#define SHL(val, bits) ((uint32_t)val << bits)
  *val = SHL(b[0], 0) | SHL(b[1], 8) | SHL(b[2], 16) | SHL(b[3], 24);
#undef SHL
  *buf += sizeof(uint32_t);
  return UPB_STATUS_OK;
}

static upb_status_t skip_f_uint32_t(uint8_t **buf)
{
  *buf += sizeof(uint32_t);
  return UPB_STATUS_OK;
}

static upb_status_t get_f_uint64_t(uint8_t *restrict *buf,
                                   uint64_t *restrict val)
{
  uint8_t *b = *buf;
  /* TODO: is this worth 32/64 specializing? */
#define SHL(val, bits) ((uint64_t)val << bits)
  *val = SHL(b[0], 0)  | SHL(b[1], 8)  | SHL(b[2], 16) | SHL(b[3], 24) |
         SHL(b[4], 32) | SHL(b[5], 40) | SHL(b[6], 48) | SHL(b[7], 56);
#undef SHL
  *buf += sizeof(uint64_t);
  return UPB_STATUS_OK;
}

static upb_status_t skip_f_uint64_t(uint8_t **buf)
{
  *buf += sizeof(uint64_t);
  return UPB_STATUS_OK;
}

static int32_t zz_decode_32(uint32_t n) { return (n >> 1) ^ -(int32_t)(n & 1); }
static int64_t zz_decode_64(uint64_t n) { return (n >> 1) ^ -(int64_t)(n & 1); }

/* Functions for reading wire values and converting them to values.  These
 * are generated with macros because they follow a higly consistent pattern. */

#define WVTOV(type, wire_t, val_t) \
  static void wvtov_ ## type(wire_t s, val_t *d)

#define GET(type, v_or_f, wire_t, val_t, member_name) \
  static upb_status_t get_ ## type(uint8_t **buf, union upb_value *d) { \
    wire_t tmp; \
    CHECK(get_ ## v_or_f ## _ ## wire_t(buf, &tmp)); \
    wvtov_ ## type(tmp, &d->member_name); \
    return UPB_STATUS_OK; \
  }

#define T(type, v_or_f, wire_t, val_t, member_name) \
  WVTOV(type, wire_t, val_t);  /* prototype for GET below */ \
  GET(type, v_or_f, wire_t, val_t, member_name) \
  WVTOV(type, wire_t, val_t)

T(DOUBLE,   f, uint64_t, double,   _double) { memcpy(d, &s, sizeof(double)); }
T(FLOAT,    f, uint32_t, float,    _float)  { memcpy(d, &s, sizeof(float));  }
T(INT32,    v, uint32_t, int32_t,  int32)   { *d = (int32_t)s;               }
T(INT64,    v, uint64_t, int64_t,  int64)   { *d = (int64_t)s;               }
T(UINT32,   v, uint32_t, uint32_t, uint32)  { *d = s;                        }
T(UINT64,   v, uint64_t, uint64_t, uint64)  { *d = s;                        }
T(SINT32,   v, uint32_t, int32_t,  int32)   { *d = zz_decode_32(s);          }
T(SINT64,   v, uint64_t, int64_t,  int64)   { *d = zz_decode_64(s);          }
T(FIXED32,  f, uint32_t, uint32_t, uint32)  { *d = s;                        }
T(FIXED64,  f, uint64_t, uint64_t, uint64)  { *d = s;                        }
T(SFIXED32, f, uint32_t, int32_t,  int32)   { *d = (int32_t)s;               }
T(SFIXED64, f, uint64_t, int64_t,  int64)   { *d = (int64_t)s;               }
T(BOOL,     v, uint32_t, bool,     _bool)   { *d = (bool)s;                  }
T(ENUM,     v, uint32_t, int32_t,  int32)   { *d = (int32_t)s;               }
#undef WVTOV
#undef GET
#undef T

upb_wire_type_t upb_expected_wire_types[] = {
  [GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_DOUBLE] = UPB_WIRE_TYPE_64BIT,
  [GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_FLOAT] = UPB_WIRE_TYPE_32BIT,
  [GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_INT64] = UPB_WIRE_TYPE_VARINT,
  [GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_UINT64] = UPB_WIRE_TYPE_VARINT,
  [GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_INT32] = UPB_WIRE_TYPE_VARINT,
  [GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_FIXED64] = UPB_WIRE_TYPE_64BIT,
  [GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_FIXED32] = UPB_WIRE_TYPE_32BIT,
  [GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_BOOL] = UPB_WIRE_TYPE_VARINT,
  [GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_STRING] = UPB_WIRE_TYPE_DELIMITED,
  [GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_BYTES] = UPB_WIRE_TYPE_DELIMITED,
  [GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_GROUP] = -1,  /* TODO */
  [GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_MESSAGE] = UPB_WIRE_TYPE_DELIMITED,
  [GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_UINT32] = UPB_WIRE_TYPE_VARINT,
  [GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_ENUM] = UPB_WIRE_TYPE_VARINT,
  [GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_SFIXED32] = UPB_WIRE_TYPE_32BIT,
  [GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_SFIXED64] = UPB_WIRE_TYPE_64BIT,
  [GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_SINT32] = UPB_WIRE_TYPE_VARINT,
  [GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_SINT64] = UPB_WIRE_TYPE_VARINT,
};

upb_status_t parse_tag(uint8_t **buf, struct upb_tag *tag)
{
  uint32_t tag_int;
  CHECK(get_v_uint32_t(buf, &tag_int));
  tag->wire_type    = (upb_wire_type_t)(tag_int & 0x07);
  tag->field_number = tag_int >> 3;
  return UPB_STATUS_OK;
}

upb_status_t parse_wire_value(uint8_t *buf, size_t *offset,
                              upb_wire_type_t wt,
                              union upb_wire_value *wv)
{
#define READ(expr) CHECK(expr); *offset += (b-buf)
  uint8_t *b = buf;
  switch(wt) {
    case UPB_WIRE_TYPE_VARINT: READ(get_v_uint64_t(&b, &wv->varint)); break;
    case UPB_WIRE_TYPE_64BIT: READ(get_f_uint64_t(&b, &wv->_64bit)); break;
    case UPB_WIRE_TYPE_32BIT: READ(get_f_uint32_t(&b, &wv->_32bit)); break;
    case UPB_WIRE_TYPE_DELIMITED:
      READ(get_v_uint32_t(&b, &wv->_32bit));
      size_t new_offset = *offset + wv->_32bit;
      if (new_offset < *offset) return UPB_ERROR_OVERFLOW;
      *offset += new_offset;
      break;
    case UPB_WIRE_TYPE_START_GROUP:
    case UPB_WIRE_TYPE_END_GROUP: return UPB_ERROR_GROUP;  /* TODO */
  }
  return UPB_STATUS_OK;
}

upb_status_t skip_wire_value(uint8_t *buf, size_t *offset,
                             upb_wire_type_t wt)
{
  uint8_t *b = buf;
  switch(wt) {
    case UPB_WIRE_TYPE_VARINT: READ(skip_v_uint64_t(&b)); break;
    case UPB_WIRE_TYPE_64BIT: READ(skip_f_uint64_t(&b)); break;
    case UPB_WIRE_TYPE_32BIT: READ(skip_f_uint32_t(&b)); break;
    case UPB_WIRE_TYPE_DELIMITED: {
      /* Have to get (not skip) the length to skip the bytes. */
      uint32_t len;
      READ(get_v_uint32_t(&b, &len));
      size_t new_offset = *offset + len;
      if (new_offset < *offset) return UPB_ERROR_OVERFLOW;
      *offset += new_offset;
      break;
    }
    case UPB_WIRE_TYPE_START_GROUP:
    case UPB_WIRE_TYPE_END_GROUP: return UPB_ERROR_GROUP;  /* TODO */
  }
  return UPB_STATUS_OK;
#undef READ
}

upb_status_t upb_parse_value(uint8_t **b, upb_field_type_t ft,
                             union upb_value *v)
{
#define CASE(t) \
  case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_ ## t: return get_ ## t(b, v);
  switch(ft) {
    CASE(DOUBLE) CASE(FLOAT) CASE(INT64) CASE(UINT64) CASE(INT32) CASE(FIXED64)
    CASE(FIXED32) CASE(BOOL) CASE(UINT32) CASE(ENUM) CASE(SFIXED32)
    CASE(SFIXED64) CASE(SINT32) CASE(SINT64)
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_BYTES:
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_STRING:
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_MESSAGE:
      return get_UINT32(b, v);
    default: return UPB_ERROR;  /* Including GROUP. */
  }
#undef CASE
}
