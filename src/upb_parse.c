/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2008-2009 Joshua Haberman.  See LICENSE for details.
 */

#include "upb_parse.h"

#include <stddef.h>
#include <stdlib.h>

/* May want to move this to upb.c if enough other things warrant it. */
#define alignof(t) offsetof(struct { char c; t x; }, x)
struct upb_type_info upb_type_info[] = {
  [GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_DOUBLE]  = {alignof(double),   sizeof(double),   UPB_WIRE_TYPE_64BIT},
  [GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_FLOAT]   = {alignof(float),    sizeof(float),    UPB_WIRE_TYPE_32BIT},
  [GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_INT64]   = {alignof(int64_t),  sizeof(int64_t),  UPB_WIRE_TYPE_VARINT},
  [GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_UINT64]  = {alignof(uint64_t), sizeof(uint64_t), UPB_WIRE_TYPE_VARINT},
  [GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_INT32]   = {alignof(int32_t),  sizeof(int32_t),  UPB_WIRE_TYPE_VARINT},
  [GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_FIXED64] = {alignof(uint64_t), sizeof(uint64_t), UPB_WIRE_TYPE_64BIT},
  [GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_FIXED32] = {alignof(uint32_t), sizeof(uint32_t), UPB_WIRE_TYPE_32BIT},
  [GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_BOOL]    = {alignof(bool),     sizeof(bool),     UPB_WIRE_TYPE_VARINT},
  [GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_MESSAGE] = {alignof(void*),    sizeof(void*),    UPB_WIRE_TYPE_DELIMITED},
  [GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_GROUP]   = {alignof(void*),    sizeof(void*),    UPB_WIRE_TYPE_START_GROUP},
  [GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_UINT32]  = {alignof(uint32_t), sizeof(uint32_t), UPB_WIRE_TYPE_VARINT},
  [GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_ENUM]    = {alignof(uint32_t), sizeof(uint32_t), UPB_WIRE_TYPE_VARINT},
  [GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_SFIXED32]= {alignof(int32_t),  sizeof(int32_t),  UPB_WIRE_TYPE_32BIT},
  [GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_SFIXED64]= {alignof(int64_t),  sizeof(int64_t),  UPB_WIRE_TYPE_64BIT},
  [GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_SINT32]  = {alignof(int32_t),  sizeof(int32_t),  UPB_WIRE_TYPE_VARINT},
  [GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_SINT64]  = {alignof(int64_t),  sizeof(int64_t),  UPB_WIRE_TYPE_VARINT},
  [GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_STRING]  = {alignof(struct upb_string*), sizeof(struct upb_string*), UPB_WIRE_TYPE_DELIMITED},
  [GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_BYTES]   = {alignof(struct upb_string*), sizeof(struct upb_string*), UPB_WIRE_TYPE_DELIMITED},
};

/* Lowest-level functions -- these read integers from the input buffer. */

inline
static upb_status_t get_v_uint64_t(uint8_t *restrict buf, uint8_t *end,
                                   uint64_t *restrict val, uint8_t **outbuf)
{
  if((*buf & 0x80) == 0) {
    /* Single-byte varint -- very common case. */
    *val = *buf & 0x7f;
    *outbuf = buf + 1;
  } else if(buf <= end && (*(buf+1) & 0x80) == 0) {
    /* Two-byte varint. */
    *val = (buf[0] & 0x7f) | ((buf[1] & 0x7f) << 7);
    *outbuf = buf + 2;
  } else if(buf + 10 <= end) {
    /* >2-byte varint, fast path. */
    uint64_t cont = *(uint64_t*)(buf+2) | 0x7f7f7f7f7f7f7f7fULL;
    int num_bytes = __builtin_ffsll(~cont) / 8;
    uint32_t part0 = 0, part1 = 0, part2 = 0;

    switch(num_bytes) {
      default: return UPB_ERROR_UNTERMINATED_VARINT;
      case 8: part2 |= (buf[9] & 0x7F) << 7;
      case 7: part2 |= (buf[8] & 0x7F);
      case 6: part1 |= (buf[7] & 0x7F) << 21;
      case 5: part1 |= (buf[6] & 0x7F) << 14;
      case 4: part1 |= (buf[5] & 0x7F) << 7;
      case 3: part1 |= (buf[4] & 0x7F);
      case 2: part0 |= (buf[3] & 0x7F) << 21;
      case 1: part0 |= (buf[2] & 0x7F) << 14;
              part0 |= (buf[1] & 0x7F) << 7;
              part0 |= (buf[0] & 0x7F);
    }
    *val = (uint64_t)part0 | ((uint64_t)part1 << 28) | ((uint64_t)part2 << 56);
    *outbuf = buf + num_bytes + 2;
  } else {
    /* >2-byte varint, slow path. */
    uint8_t last = 0x80;
    *val = 0;
    for(int bitpos = 0; buf < (uint8_t*)end && (last & 0x80); buf++, bitpos += 7)
      *val |= ((uint64_t)((last = *buf) & 0x7F)) << bitpos;
    if(last & 0x80) return UPB_STATUS_NEED_MORE_DATA;
    *outbuf = buf;
  }
  return UPB_STATUS_OK;
}

static upb_status_t skip_v_uint64_t(uint8_t *buf, uint8_t *end, uint8_t **outbuf)
{
  /* TODO: fix and optimize. */
  uint8_t last = 0x80;
  for(; buf < end && (last & 0x80); buf++) {
    last = *buf;
  }

  if(last & 0x80) {
    return UPB_ERROR_UNTERMINATED_VARINT;
  }
  *outbuf = buf;
  return UPB_STATUS_OK;
}

static upb_status_t get_v_uint32_t(uint8_t *restrict buf, uint8_t *end,
                                   uint32_t *restrict val, uint8_t **outbuf)
{
  uint64_t val64;
  UPB_CHECK(get_v_uint64_t(buf, end, &val64, outbuf));
  /* TODO: should we throw an error if any of the high bits in val64 are set? */
  *val = (uint32_t)val64;
  return UPB_STATUS_OK;
}

static upb_status_t get_f_uint32_t(uint8_t *restrict buf, uint8_t *end,
                                   uint32_t *restrict val, uint8_t **outbuf)
{
  uint8_t *uint32_end = buf + sizeof(uint32_t);
  if(uint32_end > end) return UPB_STATUS_NEED_MORE_DATA;
#if UPB_UNALIGNED_READS_OK
  *val = *(uint32_t*)buf;
#else
#define SHL(val, bits) ((uint32_t)val << bits)
  *val = SHL(b[0], 0) | SHL(b[1], 8) | SHL(b[2], 16) | SHL(b[3], 24);
#undef SHL
#endif
  *outbuf = uint32_end;
  return UPB_STATUS_OK;
}

static upb_status_t get_f_uint64_t(uint8_t *restrict buf, uint8_t *end,
                                   uint64_t *restrict val, uint8_t **outbuf)
{
  uint8_t *uint64_end = buf + sizeof(uint64_t);
  if(uint64_end > end) return UPB_STATUS_NEED_MORE_DATA;
#if UPB_UNALIGNED_READS_OK
  *val = *(uint64_t*)buf;
#else
#define SHL(val, bits) ((uint64_t)val << bits)
  *val = SHL(b[0],  0) | SHL(b[1],  8) | SHL(b[2], 16) | SHL(b[3], 24) |
         SHL(b[4], 32) | SHL(b[5], 40) | SHL(b[6], 48) | SHL(b[7], 56) |
#undef SHL
#endif
  *outbuf = uint64_end;
  return UPB_STATUS_OK;
}

static upb_status_t skip_f_uint32_t(uint8_t *buf, uint8_t *end, uint8_t **outbuf)
{
  uint8_t *uint32_end = buf + sizeof(uint32_t);
  if(uint32_end > end) return UPB_STATUS_NEED_MORE_DATA;
  *outbuf = uint32_end;
  return UPB_STATUS_OK;
}

static upb_status_t skip_f_uint64_t(uint8_t *buf, uint8_t *end, uint8_t **outbuf)
{
  uint8_t *uint64_end = buf + sizeof(uint64_t);
  if(uint64_end > end) return UPB_STATUS_NEED_MORE_DATA;
  *outbuf = uint64_end;
  return UPB_STATUS_OK;
}

static int32_t zz_decode_32(uint32_t n) { return (n >> 1) ^ -(int32_t)(n & 1); }
static int64_t zz_decode_64(uint64_t n) { return (n >> 1) ^ -(int64_t)(n & 1); }

/* Functions for reading wire values and converting them to values.  These
 * are generated with macros because they follow a higly consistent pattern. */

#define WVTOV(type, wire_t, val_t) \
  static void wvtov_ ## type(wire_t s, val_t *d)

#define GET(type, v_or_f, wire_t, val_t, member_name) \
  static upb_status_t get_ ## type(uint8_t *buf, uint8_t *end, val_t *d, uint8_t **outbuf) { \
    wire_t tmp; \
    UPB_CHECK(get_ ## v_or_f ## _ ## wire_t(buf, end, &tmp, outbuf)); \
    wvtov_ ## type(tmp, d); \
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

static upb_status_t parse_tag(uint8_t *buf, uint8_t *end, struct upb_tag *tag, uint8_t **outbuf)
{
  uint32_t tag_int;
  UPB_CHECK(get_v_uint32_t(buf, end, &tag_int, outbuf));
  tag->wire_type    = (upb_wire_type_t)(tag_int & 0x07);
  tag->field_number = tag_int >> 3;
  return UPB_STATUS_OK;
}

upb_status_t upb_parse_wire_value(uint8_t *buf, uint8_t *end, upb_wire_type_t wt,
                                  union upb_wire_value *wv, uint8_t **outbuf)
{
  switch(wt) {
    case UPB_WIRE_TYPE_VARINT: return get_v_uint64_t(buf, end, &wv->varint, outbuf);
    case UPB_WIRE_TYPE_64BIT:  return get_f_uint64_t(buf, end, &wv->_64bit, outbuf);
    case UPB_WIRE_TYPE_32BIT:  return get_f_uint32_t(buf, end, &wv->_32bit, outbuf);
    default: return UPB_ERROR_ILLEGAL; /* Doesn't handle delimited, groups. */
  }
}

static upb_status_t skip_wire_value(uint8_t *buf, uint8_t *end, upb_wire_type_t wt,
                                    uint8_t **outbuf)
{
  switch(wt) {
    case UPB_WIRE_TYPE_VARINT: return skip_v_uint64_t(buf, end, outbuf);
    case UPB_WIRE_TYPE_64BIT:  return skip_f_uint64_t(buf, end, outbuf);
    case UPB_WIRE_TYPE_32BIT:  return skip_f_uint32_t(buf, end, outbuf);
    case UPB_WIRE_TYPE_START_GROUP: /* TODO: skip to matching end group. */
    case UPB_WIRE_TYPE_END_GROUP: return UPB_STATUS_OK;
    default: return UPB_ERROR_ILLEGAL;
  }
}

upb_status_t upb_parse_value(uint8_t *buf, uint8_t *end, upb_field_type_t ft,
                             union upb_value_ptr v, uint8_t **outbuf)
{
#define CASE(t, member_name) \
  case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_ ## t: \
    return get_ ## t(buf, end, v.member_name, outbuf);
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

void upb_parse_reset(struct upb_parse_state *state, void *udata)
{
  state->top = state->stack;
  state->limit = &state->stack[UPB_MAX_NESTING];
  /* The top-level message is not delimited (we can keep receiving data for
   * it indefinitely), so we treat it like a group. */
  *state->top = 0;
  state->completed_offset = 0;
  state->udata = udata;
}

void upb_parse_init(struct upb_parse_state *state, void *udata)
{
  memset(state, 0, sizeof(struct upb_parse_state));  /* Clear all callbacks. */
  upb_parse_reset(state, udata);
}

void upb_parse_free(struct upb_parse_state *state)
{
  (void)state;
}

static void *pop_stack_frame(struct upb_parse_state *s, uint8_t *buf)
{
  if(s->submsg_end_cb) s->submsg_end_cb(s->udata);
  s->top--;
  return (char*)buf + (*s->top > 0 ? (*s->top - s->completed_offset) : 0);
}

/* Returns the next end offset. */
static upb_status_t push_stack_frame(struct upb_parse_state *s,
                                     uint8_t *buf, uint32_t len,
                                     void *user_field_desc, uint8_t **submsg_end)
{
  s->top++;
  if(s->top > s->limit) return UPB_ERROR_STACK_OVERFLOW;
  *s->top = s->completed_offset + len;
  if(s->submsg_start_cb) s->submsg_start_cb(s->udata, user_field_desc);
  *submsg_end = buf + (*s->top > 0 ? (*s->top - s->completed_offset) : 0);
  return UPB_STATUS_OK;
}

upb_status_t upb_parse(struct upb_parse_state *s, void *_buf, size_t len,
                       size_t *read)
{
  uint8_t *buf = _buf;
  uint8_t *completed = buf;
  uint8_t *const start = buf;

  upb_status_t status = UPB_STATUS_OK;
  upb_tag_cb tag_cb = s->tag_cb;
  upb_str_cb str_cb = s->str_cb;
  upb_value_cb value_cb = s->value_cb;
  void *udata = s->udata;

  uint8_t *end = buf + len;
  uint8_t *submsg_end = buf + (*s->top > 0 ? *s->top : 0);
  while(buf < end) {
    struct upb_tag tag;
    UPB_CHECK(parse_tag(buf, end, &tag, &buf));
    if(tag.wire_type == UPB_WIRE_TYPE_END_GROUP) {
      submsg_end = pop_stack_frame(s, start);
      completed = buf;
      continue;
    }
    /* Don't handle START_GROUP here, so client can skip group via tag_cb. */
    void *user_field_desc;

    upb_field_type_t ft = tag_cb(udata, &tag, &user_field_desc);
    if(tag.wire_type == UPB_WIRE_TYPE_DELIMITED) {
      int32_t delim_len;
      UPB_CHECK(get_INT32(buf, end, &delim_len, &buf));
      uint8_t *delim_end = buf + delim_len;

      if(delim_end > end) { /* String ends beyond the data we have. */
        if(ft == GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_MESSAGE) {
          /* Streaming the body of a message is ok. */
        } else {
          /* String, bytes, and packed arrays must have all data present. */
          status = UPB_STATUS_NEED_MORE_DATA;
          goto done;
        }
      }

      if(ft == GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_MESSAGE) {
        UPB_CHECK(push_stack_frame(s, start, delim_end - start, user_field_desc, &submsg_end));
      } else {  /* Delimited data for which we require (and have) all data. */
        if(ft == 0) {
          /* Do nothing -- client has elected to skip. */
        } else if(upb_isstringtype(ft)) {
          struct upb_string str = {.ptr = (char*)buf, .byte_len = delim_len};
          str_cb(udata, &str, user_field_desc);
        } else {  /* Packed Array. */
          while(buf < delim_end)
            UPB_CHECK(value_cb(udata, buf, end, user_field_desc, &buf));
        }
        buf = delim_end;
      }
    } else {  /* Scalar (non-delimited) value. */
      if(ft == 0)  /* Client elected to skip. */
        UPB_CHECK(skip_wire_value(buf, end, tag.wire_type, &buf));
      else if(ft == GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_GROUP)
        UPB_CHECK(push_stack_frame(s, start, 0, user_field_desc, &submsg_end));
      else
        UPB_CHECK(value_cb(udata, buf, end, user_field_desc, &buf));
    }

    while(buf == submsg_end) submsg_end = pop_stack_frame(s, start);
    completed = buf;
  }

done:
  *read = (char*)completed - (char*)start;
  s->completed_offset += *read;
  return status;
}
