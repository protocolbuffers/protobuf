/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2008-2009 Joshua Haberman.  See LICENSE for details.
 */

#include "upb_parse.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "descriptor.h"

/* Lowest-level functions -- these read integers from the input buffer. */

static void *check_end(uint8_t *buf, void *end, size_t maxlen,
                       upb_status_t *bound_error)
{
  void *maxend = buf + maxlen;
  if(end < maxend) {
    *bound_error = UPB_STATUS_NEED_MORE_DATA;
    return end;
  } else {
    *bound_error = UPB_ERROR_UNTERMINATED_VARINT;
    return maxend;
  }
}

static upb_status_t get_v_uint64_t(void *restrict *buf, void *end,
                                   uint64_t *restrict val)
{
  uint8_t *b = *buf;
  upb_status_t bound_error;
  end = check_end(b, end, 10, &bound_error);  /* 2**64 is a 10-byte varint. */
  uint8_t last = 0x80;
  *val = 0;
  for(int bitpos = 0; b < (uint8_t*)end && (last & 0x80); b++, bitpos += 7)
    *val |= ((uint64_t)((last = *b) & 0x7F)) << bitpos;

  if(last & 0x80) return bound_error;
  *buf = b;
  return UPB_STATUS_OK;
}

static upb_status_t skip_v_uint64_t(void **buf, void *end)
{
  uint8_t *b = *buf;
  upb_status_t bound_error;
  end = check_end(b, end, 10, &bound_error);  /* 2**64 is a 10-byte varint. */
  uint8_t last = 0x80;
  for(; b < (uint8_t*)end && (last & 0x80); b++)
    last = *b;

  if(last & 0x80) return bound_error;
  *buf = b;
  return UPB_STATUS_OK;
}

static upb_status_t get_v_uint32_t(void *restrict *buf, void *end,
                                   uint32_t *restrict val)
{
  uint8_t *b = *buf, *dend;
  upb_status_t bound_error;
  dend = check_end(b, end, 5, &bound_error);  /* 2**32 is a 5-byte varint. */
  end = check_end(b, end, 10, &bound_error);  /* May have to discard bytes. */
  uint8_t last = 0x80;
  *val = 0;
  for(int bitpos = 0; b < dend && (last & 0x80); b++, bitpos += 7)
    *val |= ((uint32_t)((last = *b) & 0x7F)) << bitpos;

  /* Discard high bytes until varint ends. */
  for(; b < (uint8_t*)end && (last & 0x80); b++)
    last = *b;

  if(last & 0x80)
    return bound_error;
  *buf = b;
  return UPB_STATUS_OK;
}

static upb_status_t get_f_uint32_t(void *restrict *buf, void *end,
                                   uint32_t *restrict val)
{
  uint8_t *b = *buf;
  void *uint32_end = (uint8_t*)*buf + sizeof(uint32_t);
  if(uint32_end > end) return UPB_STATUS_NEED_MORE_DATA;
#if UPB_UNALIGNED_READS_OK
  *val = *(uint32_t*)b;
#else
#define SHL(val, bits) ((uint32_t)val << bits)
  *val = SHL(b[0], 0) | SHL(b[1], 8) | SHL(b[2], 16) | SHL(b[3], 24);
#undef SHL
#endif
  *buf = uint32_end;
  return UPB_STATUS_OK;
}

static upb_status_t get_f_uint64_t(void *restrict *buf, void *end,
                                   uint64_t *restrict val)
{
  void *uint64_end = (uint8_t*)*buf + sizeof(uint64_t);
  if(uint64_end > end) return UPB_STATUS_NEED_MORE_DATA;
#if UPB_UNALIGNED_READS_OK
  *val = *(uint64_t*)*buf;
  *buf = uint64_end;
#else
  uint32_t lo32, hi32;
  get_f_uint32_t(buf, &lo32, end);
  get_f_uint32_t(buf, &hi32, end);
  *val = lo32 | ((uint64_t)hi32 << 32);
#endif
  return UPB_STATUS_OK;
}

static upb_status_t skip_f_uint32_t(void **buf, void *end)
{
  void *uint32_end = (uint8_t*)*buf + sizeof(uint32_t);
  if(uint32_end > end) return UPB_STATUS_NEED_MORE_DATA;
  *buf = uint32_end;
  return UPB_STATUS_OK;
}

static upb_status_t skip_f_uint64_t(void **buf, void *end)
{
  void *uint64_end = (uint8_t*)*buf + sizeof(uint64_t);
  if(uint64_end > end) return UPB_STATUS_NEED_MORE_DATA;
  *buf = uint64_end;
  return UPB_STATUS_OK;
}

static int32_t zz_decode_32(uint32_t n) { return (n >> 1) ^ -(int32_t)(n & 1); }
static int64_t zz_decode_64(uint64_t n) { return (n >> 1) ^ -(int64_t)(n & 1); }

/* Functions for reading wire values and converting them to values.  These
 * are generated with macros because they follow a higly consistent pattern. */

#define WVTOV(type, wire_t, val_t) \
  static void wvtov_ ## type(wire_t s, val_t *d)

#define GET(type, v_or_f, wire_t, val_t, member_name) \
  static upb_status_t get_ ## type(void **buf, void *end, val_t *d) { \
    wire_t tmp; \
    UPB_CHECK(get_ ## v_or_f ## _ ## wire_t(buf, end, &tmp)); \
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

#define alignof(t) offsetof(struct { char c; t x; }, x)

/* May want to move this to upb.c if enough other things warrant it. */
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

static upb_status_t parse_tag(void **buf, void *end, struct upb_tag *tag)
{
  uint32_t tag_int;
  UPB_CHECK(get_v_uint32_t(buf, end, &tag_int));
  tag->wire_type    = (upb_wire_type_t)(tag_int & 0x07);
  tag->field_number = tag_int >> 3;
  return UPB_STATUS_OK;
}

upb_status_t upb_parse_wire_value(void **buf, void *end, upb_wire_type_t wt,
                                  union upb_wire_value *wv)
{
  switch(wt) {
    case UPB_WIRE_TYPE_VARINT: UPB_CHECK(get_v_uint64_t(buf, end, &wv->varint)); break;
    case UPB_WIRE_TYPE_64BIT:  UPB_CHECK(get_f_uint64_t(buf, end, &wv->_64bit)); break;
    case UPB_WIRE_TYPE_32BIT:  UPB_CHECK(get_f_uint32_t(buf, end, &wv->_32bit)); break;
    default: return UPB_ERROR_ILLEGAL; /* Doesn't handle delimited, groups. */
  }
  return UPB_STATUS_OK;
}

static upb_status_t skip_wire_value(void **buf, void *end, upb_wire_type_t wt)
{
  switch(wt) {
    case UPB_WIRE_TYPE_VARINT: UPB_CHECK(skip_v_uint64_t(buf, end)); break;
    case UPB_WIRE_TYPE_64BIT:  UPB_CHECK(skip_f_uint64_t(buf, end)); break;
    case UPB_WIRE_TYPE_32BIT:  UPB_CHECK(skip_f_uint32_t(buf, end)); break;
    case UPB_WIRE_TYPE_START_GROUP: /* TODO: skip to matching end group. */
    case UPB_WIRE_TYPE_END_GROUP: break;
    default: return UPB_ERROR_ILLEGAL;
  }
  return UPB_STATUS_OK;
}

upb_status_t upb_parse_value(void **buf, void *end, upb_field_type_t ft,
                             union upb_value_ptr v)
{
#define CASE(t, member_name) \
  case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_ ## t: \
    return get_ ## t(buf, end, v.member_name);
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

void upb_parse_reset(struct upb_parse_state *state)
{
  state->offset = 0;
  state->top = state->stack;
  /* The top-level message is not delimited (we can keep receiving data for
   * it indefinitely). */
  state->top->end_offset = SIZE_MAX;
}

void upb_parse_init(struct upb_parse_state *state, size_t udata_size)
{
  memset(state, 0, sizeof(struct upb_parse_state));  /* Clear all callbacks. */
  size_t stack_bytes = (sizeof(*state->stack) + udata_size) * UPB_MAX_NESTING;
  state->stack = malloc(stack_bytes);
  state->limit = (struct upb_parse_stack_frame*)((char*)state->stack + stack_bytes);
  state->udata_size = udata_size;
  upb_parse_reset(state);
}

void upb_parse_free(struct upb_parse_state *state)
{
  free(state->stack);
}

static void pop_stack_frame(struct upb_parse_state *s)
{
  if(s->submsg_end_cb) s->submsg_end_cb(s);
  s->top--;
  s->top = (struct upb_parse_stack_frame*)((char*)s->top - s->udata_size);
}

static upb_status_t push_stack_frame(struct upb_parse_state *s, size_t end,
                                     void *user_field_desc)
{
  s->top++;
  s->top = (struct upb_parse_stack_frame*)((char*)s->top + s->udata_size);
  if(s->top > s->limit) return UPB_ERROR_STACK_OVERFLOW;
  s->top->end_offset = end;
  if(s->submsg_start_cb) s->submsg_start_cb(s, user_field_desc);
  return UPB_STATUS_OK;
}

static upb_status_t parse_delimited(struct upb_parse_state *s,
                                    struct upb_tag *tag,
                                    void **buf, void *end,
                                    size_t base_offset)
{
  int32_t delim_len;
  void *user_field_desc;
  void *bufstart = *buf;

  /* Whether we are parsing or skipping the field, we always need to parse
   * the length. */
  UPB_CHECK(get_INT32(buf, end, &delim_len));
  upb_field_type_t ft = s->tag_cb(s, tag, &user_field_desc);
  if(*buf < bufstart) return UPB_ERROR_OVERFLOW;
  if(*buf > end && ft != GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_MESSAGE) {
    /* Streaming submessages is ok, but for other delimited types (string,
     * bytes, and packed arrays) we require that all the delimited data is
     * available.  This could be relaxed if desired. */
    return UPB_STATUS_NEED_MORE_DATA;
  }

  if(ft == GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_MESSAGE) {
    base_offset += ((char*)*buf - (char*)bufstart);
    UPB_CHECK(push_stack_frame(s, base_offset + delim_len, user_field_desc));
  } else {
    void *delim_end = (char*)*buf + delim_len;
    if(ft == GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_STRING ||
       ft == GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_BYTES) {
      struct upb_string str = {.ptr = *buf, .byte_len = delim_len};
      s->str_cb(s, &str, user_field_desc);
      *buf = delim_end;
    } else {
      /* Packed Array. */
      while(*buf < delim_end)
        UPB_CHECK(s->value_cb(s, buf, end, user_field_desc));
    }
  }
  return UPB_STATUS_OK;
}

static upb_status_t parse_nondelimited(struct upb_parse_state *s,
                                       struct upb_tag *tag,
                                       void **buf, void *end)
{
  /* Simple value or begin group. */
  void *user_field_desc;
  upb_field_type_t ft = s->tag_cb(s, tag, &user_field_desc);
  if(ft == 0) {
    UPB_CHECK(skip_wire_value(buf, end, tag->wire_type));
  } else if(ft == GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_GROUP) {
    /* No length specified, an "end group" tag will mark the end. */
    UPB_CHECK(push_stack_frame(s, UINT32_MAX, user_field_desc));
  } else {
    UPB_CHECK(s->value_cb(s, buf, end, user_field_desc));
  }
  return UPB_STATUS_OK;
}

upb_status_t upb_parse(struct upb_parse_state *restrict s, void *buf, size_t len,
                       size_t *restrict read)
{
  void *end = (char*)buf + len;
  *read = 0;
  while(buf < end) {
    while(s->offset >= s->top->end_offset) {
      if(s->offset != s->top->end_offset) return UPB_ERROR_BAD_SUBMESSAGE_END;
      pop_stack_frame(s);
    }

    struct upb_tag tag;
    void *bufstart = buf;
    UPB_CHECK(parse_tag(&buf, end, &tag));
    if(tag.wire_type == UPB_WIRE_TYPE_END_GROUP) {
      if(s->top->end_offset != UINT32_MAX)
        return UPB_ERROR_SPURIOUS_END_GROUP;
      pop_stack_frame(s);
    } else if(tag.wire_type == UPB_WIRE_TYPE_DELIMITED) {
      parse_delimited(s, &tag, &buf, end, s->offset + (char*)buf - (char*)bufstart);
    } else {
      parse_nondelimited(s, &tag, &buf, end);
    }
    size_t bytes_read = ((char*)buf - (char*)bufstart);
    *read += bytes_read;
    s->offset += bytes_read;
  }
  return UPB_STATUS_OK;
}
