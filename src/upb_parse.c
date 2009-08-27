/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2008-2009 Joshua Haberman.  See LICENSE for details.
 */

#include "upb_parse.h"

#include <stddef.h>
#include <stdlib.h>

/**
 * Parses a 64-bit varint that is known to be >= 2 bytes (the inline version
 * handles 1 and 2 byte varints).
 */
upb_status_t upb_get_v_uint64_t_full(uint8_t *buf, uint8_t *end, uint64_t *val,
                                     uint8_t **outbuf)
{
  uint8_t *const maxend = buf + 10;
  uint8_t last = 0x80;
  *val = 0;
  int bitpos;

  for(bitpos = 0; buf < (uint8_t*)end && (last & 0x80); buf++, bitpos += 7)
    *val |= ((uint64_t)((last = *buf) & 0x7F)) << bitpos;

  if(buf >= end && buf <= maxend && (last & 0x80))
    return UPB_STATUS_NEED_MORE_DATA;
  if(buf > maxend)
    return UPB_ERROR_UNTERMINATED_VARINT;

  *outbuf = buf;
  return UPB_STATUS_OK;
}

upb_status_t upb_parse_wire_value(uint8_t *buf, uint8_t *end, upb_wire_type_t wt,
                                  union upb_wire_value *wv, uint8_t **outbuf)
{
  switch(wt) {
    case UPB_WIRE_TYPE_VARINT:
      return upb_get_v_uint64_t(buf, end, &wv->varint, outbuf);
    case UPB_WIRE_TYPE_64BIT:
      return upb_get_f_uint64_t(buf, end, &wv->_64bit, outbuf);
    case UPB_WIRE_TYPE_32BIT:
      return upb_get_f_uint32_t(buf, end, &wv->_32bit, outbuf);
    default:
      return UPB_ERROR_ILLEGAL;  // Doesn't handle delimited, groups.
  }
}

/**
 * Advances buf past the current wire value (of type wt), saving the result in
 * outbuf.
 */
static upb_status_t skip_wire_value(uint8_t *buf, uint8_t *end, upb_wire_type_t wt,
                                    uint8_t **outbuf)
{
  switch(wt) {
    case UPB_WIRE_TYPE_VARINT:
      return upb_skip_v_uint64_t(buf, end, outbuf);
    case UPB_WIRE_TYPE_64BIT:
      return upb_skip_f_uint64_t(buf, end, outbuf);
    case UPB_WIRE_TYPE_32BIT:
      return upb_skip_f_uint32_t(buf, end, outbuf);
    case UPB_WIRE_TYPE_START_GROUP:
      // TODO: skip to matching end group.
    case UPB_WIRE_TYPE_END_GROUP:
      return UPB_STATUS_OK;
    default:
      return UPB_ERROR_ILLEGAL;
  }
}

upb_status_t upb_parse_value(uint8_t *buf, uint8_t *end, upb_field_type_t ft,
                             union upb_value_ptr v, uint8_t **outbuf)
{
#define CASE(t, member_name) \
  case UPB_TYPENUM(t): return upb_get_ ## t(buf, end, v.member_name, outbuf);

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

void upb_stream_parser_reset(struct upb_stream_parser *state, void *udata)
{
  state->top = state->stack;
  state->limit = &state->stack[UPB_MAX_NESTING];
  state->completed_offset = 0;
  state->udata = udata;

  // The top-level message is not delimited (we can keep receiving data for it
  // indefinitely), so we treat it like a group.
  *state->top = 0;
}

/**
 * Pushes a new stack frame for a submessage with the given len (which will
 * be zero if the submessage is a group).
 */
static upb_status_t push(struct upb_stream_parser *s, uint8_t *start,
                         uint32_t submsg_len, void *user_field_desc,
                         uint8_t **submsg_end)
{
  s->top++;
  if(s->top >= s->limit)
    return UPB_ERROR_STACK_OVERFLOW;
  *s->top = s->completed_offset + submsg_len;

  if(s->submsg_start_cb)
    s->submsg_start_cb(s->udata, user_field_desc);

  *submsg_end = start + (*s->top > 0 ? (*s->top - s->completed_offset) : 0);
  return UPB_STATUS_OK;
}

/**
 * Pops a stack frame, returning a pointer for where the next submsg should
 * end (or a pointer that is out of range for a group).
 */
static void *pop(struct upb_stream_parser *s, uint8_t *start)
{
  if(s->submsg_end_cb)
    s->submsg_end_cb(s->udata);

  s->top--;

  if(*s->top > 0)
    return (char*)start + (*s->top - s->completed_offset);
  else
    return (char*)start;  // group.
}


upb_status_t upb_stream_parser_parse(struct upb_stream_parser *s,
                                     void *_buf, size_t len, size_t *read)
{
  uint8_t *buf = _buf;
  uint8_t *completed = buf;
  uint8_t *const start = buf;  // ptr equivalent of s->completed_offset
  uint8_t *end = buf + len;
  uint8_t *submsg_end = buf + (*s->top > 0 ? *s->top : 0);
  upb_status_t status = UPB_STATUS_OK;

  // Make local copies so optimizer knows they won't change.
  upb_tag_cb tag_cb = s->tag_cb;
  upb_str_cb str_cb = s->str_cb;
  upb_value_cb value_cb = s->value_cb;
  void *udata = s->udata;

#define CHECK(exp) do { if((status = exp) != UPB_STATUS_OK) goto err; } while(0)

  // Main loop: parse a tag, then handle the value.
  while(buf < end) {
    struct upb_tag tag;
    CHECK(parse_tag(buf, end, &tag, &buf));
    if(tag.wire_type == UPB_WIRE_TYPE_END_GROUP) {
      submsg_end = pop(s, start);
      completed = buf;
      continue;
    }

    void *udesc;
    upb_field_type_t ft = tag_cb(udata, &tag, &udesc);
    if(tag.wire_type == UPB_WIRE_TYPE_DELIMITED) {
      int32_t delim_len;
      CHECK(upb_get_INT32(buf, end, &delim_len, &buf));
      uint8_t *delim_end = buf + delim_len;
      if(ft == UPB_TYPENUM(MESSAGE)) {
        CHECK(push(s, start, delim_end - start, udesc, &submsg_end));
      } else {
        if(upb_isstringtype(ft)) {
          size_t avail_len = UPB_MIN(delim_end, end) - buf;
          str_cb(udata, buf, avail_len, delim_end - buf, udesc);
        } // else { TODO: packed arrays }
        buf = delim_end;  // Could be >end.
      }
    } else {
      // Scalar (non-delimited) value.
      switch(ft) {
        case 0:  // Client elected to skip.
          CHECK(skip_wire_value(buf, end, tag.wire_type, &buf));
          break;
        case UPB_TYPENUM(GROUP):
          CHECK(push(s, start, 0, udesc, &submsg_end));
          break;
        default:
          CHECK(value_cb(udata, buf, end, udesc, &buf));
          break;
      }
    }

    while(buf == submsg_end)
      submsg_end = pop(s, start);
    // while(buf < s->packed_end) { TODO: packed arrays }
    completed = buf;
  }

err:
  *read = (char*)completed - (char*)start;
  s->completed_offset += *read;
  return status;
}
