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
#define TYPE_INFO(proto_type, wire_type, ctype) [proto_type] = {alignof(ctype), sizeof(ctype), wire_type, UPB_STRLIT(#ctype)},
struct upb_type_info upb_type_info[] = {
  TYPE_INFO(GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_DOUBLE,   UPB_WIRE_TYPE_64BIT,       double)
  TYPE_INFO(GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_FLOAT,    UPB_WIRE_TYPE_32BIT,       float)
  TYPE_INFO(GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_INT64,    UPB_WIRE_TYPE_VARINT,      int64_t)
  TYPE_INFO(GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_UINT64,   UPB_WIRE_TYPE_VARINT,      uint64_t)
  TYPE_INFO(GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_INT32,    UPB_WIRE_TYPE_VARINT,      int32_t)
  TYPE_INFO(GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_FIXED64,  UPB_WIRE_TYPE_64BIT,       uint64_t)
  TYPE_INFO(GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_FIXED32,  UPB_WIRE_TYPE_32BIT,       uint32_t)
  TYPE_INFO(GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_BOOL,     UPB_WIRE_TYPE_VARINT,      bool)
  TYPE_INFO(GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_MESSAGE,  UPB_WIRE_TYPE_DELIMITED,   void*)
  TYPE_INFO(GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_GROUP,    UPB_WIRE_TYPE_START_GROUP, void*)
  TYPE_INFO(GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_UINT32,   UPB_WIRE_TYPE_VARINT,      uint32_t)
  TYPE_INFO(GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_ENUM,     UPB_WIRE_TYPE_VARINT,      uint32_t)
  TYPE_INFO(GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_SFIXED32, UPB_WIRE_TYPE_32BIT,       int32_t)
  TYPE_INFO(GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_SFIXED64, UPB_WIRE_TYPE_64BIT,       int64_t)
  TYPE_INFO(GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_SINT32,   UPB_WIRE_TYPE_VARINT,      int32_t)
  TYPE_INFO(GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_SINT64,   UPB_WIRE_TYPE_VARINT,      int64_t)
  TYPE_INFO(GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_STRING,   UPB_WIRE_TYPE_DELIMITED,   struct upb_string*)
  TYPE_INFO(GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_BYTES,    UPB_WIRE_TYPE_DELIMITED,   struct upb_string*)
};

/* This is called by the inline version of the function if the varint turns out
 * to be >= 2 bytes. */
upb_status_t upb_get_v_uint64_t_full(uint8_t *restrict buf, uint8_t *end,
                                     uint64_t *restrict val,
                                     uint8_t **outbuf)
{
  uint8_t last = 0x80;
  *val = 0;
  for(int bitpos = 0; buf < (uint8_t*)end && (last & 0x80); buf++, bitpos += 7)
    *val |= ((uint64_t)((last = *buf) & 0x7F)) << bitpos;
  if(last & 0x80) return UPB_STATUS_NEED_MORE_DATA;
  *outbuf = buf;
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

upb_status_t upb_parse_wire_value(uint8_t *buf, uint8_t *end, upb_wire_type_t wt,
                                  union upb_wire_value *wv, uint8_t **outbuf)
{
  switch(wt) {
    case UPB_WIRE_TYPE_VARINT: return upb_get_v_uint64_t(buf, end, &wv->varint, outbuf);
    case UPB_WIRE_TYPE_64BIT:  return upb_get_f_uint64_t(buf, end, &wv->_64bit, outbuf);
    case UPB_WIRE_TYPE_32BIT:  return upb_get_f_uint32_t(buf, end, &wv->_32bit, outbuf);
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
    return upb_get_ ## t(buf, end, v.member_name, outbuf);
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
  uint8_t *end = buf + len;
  uint8_t *submsg_end = buf + (*s->top > 0 ? *s->top : 0);
  upb_status_t status = UPB_STATUS_OK;

  /* Make local copies so optimizer knows they won't change. */
  upb_tag_cb tag_cb = s->tag_cb;
  upb_str_cb str_cb = s->str_cb;
  upb_value_cb value_cb = s->value_cb;
  void *udata = s->udata;

  /* Main loop: parse a tag, then handle the value. */
  while(buf < end) {
    struct upb_tag tag;
    UPB_CHECK(parse_tag(buf, end, &tag, &buf));
    if(tag.wire_type == UPB_WIRE_TYPE_END_GROUP) {
      submsg_end = pop_stack_frame(s, start);
      completed = buf;
      continue;
    }

    void *udesc;
    upb_field_type_t ft = tag_cb(udata, &tag, &udesc);
    if(tag.wire_type == UPB_WIRE_TYPE_DELIMITED) {
      int32_t delim_len;
      UPB_CHECK(upb_get_INT32(buf, end, &delim_len, &buf));
      uint8_t *delim_end = buf + delim_len;
      if(ft == GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_MESSAGE) {
          UPB_CHECK(push_stack_frame(
              s, start, delim_end - start, udesc, &submsg_end));
      } else {
        if(upb_isstringtype(ft))
          str_cb(udata, buf, UPB_MIN(delim_end, end) - buf, delim_end - buf, udesc);
        else
          ;/* Set a marker for packed arrays. */
        buf = delim_end;  /* Note that this could be greater than end. */
      }
    } else {  /* Scalar (non-delimited) value. */
      switch(ft) {
        case 0:  /* Client elected to skip. */
          UPB_CHECK(skip_wire_value(buf, end, tag.wire_type, &buf));
          break;
        case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_GROUP:
          UPB_CHECK(push_stack_frame(s, start, 0, udesc, &submsg_end));
          break;
        default:
          UPB_CHECK(value_cb(udata, buf, end, udesc, &buf));
          break;
      }
    }

    while(buf == submsg_end) submsg_end = pop_stack_frame(s, start);
    //while(buf < s->packed_end)  /* packed arrays. */
    //  UPB_CHECK(value_cb(udata, buf, end, udesc, &buf));
    completed = buf;
  }

  *read = (char*)completed - (char*)start;
  s->completed_offset += *read;
  return status;
}
