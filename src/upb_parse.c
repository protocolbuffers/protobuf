/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2008-2009 Joshua Haberman.  See LICENSE for details.
 */

#include "upb_parse.h"

#include <stddef.h>
#include <stdlib.h>

/* Functions to read wire values. *********************************************/

// These functions are internal to the parser, but might be moved into an
// internal header file if we at some point in the future opt to do code
// generation, because the generated code would want to inline these functions.
// The same applies to the functions to read .proto values below.

uint8_t *upb_get_v_uint64_t_full(uint8_t *buf, uint8_t *end, uint64_t *val,
                                 struct upb_status *status);

// Gets a varint (wire type: UPB_WIRE_TYPE_VARINT).
INLINE uint8_t *upb_get_v_uint64_t(uint8_t *buf, uint8_t *end, uint64_t *val,
                                   struct upb_status *status)
{
  // We inline this common case (1-byte varints), if that fails we dispatch to
  // the full (non-inlined) version.
  if((*buf & 0x80) == 0) {
    *val = *buf & 0x7f;
    return buf + 1;
  } else {
    return upb_get_v_uint64_t_full(buf, end, val, status);
  }
}

// Gets a varint -- called when we only need 32 bits of it.
INLINE uint8_t *upb_get_v_uint32_t(uint8_t *buf, uint8_t *end,
                                   uint32_t *val, struct upb_status *status)
{
  uint64_t val64;
  uint8_t *ret = upb_get_v_uint64_t(buf, end, &val64, status);
  *val = (uint32_t)val64;  // Discard the high bits.
  return ret;
}

// Gets a fixed-length 32-bit integer (wire type: UPB_WIRE_TYPE_32BIT).
INLINE uint8_t *upb_get_f_uint32_t(uint8_t *buf, uint8_t *end,
                                   uint32_t *val, struct upb_status *status)
{
  uint8_t *uint32_end = buf + sizeof(uint32_t);
  if(uint32_end > end) {
    status->code = UPB_STATUS_NEED_MORE_DATA;
    return end;
  }
#if UPB_UNALIGNED_READS_OK
  *val = *(uint32_t*)buf;
#else
#define SHL(val, bits) ((uint32_t)val << bits)
  *val = SHL(buf[0], 0) | SHL(buf[1], 8) | SHL(buf[2], 16) | SHL(buf[3], 24);
#undef SHL
#endif
  return uint32_end;
}

// Gets a fixed-length 64-bit integer (wire type: UPB_WIRE_TYPE_64BIT).
INLINE uint8_t *upb_get_f_uint64_t(uint8_t *buf, uint8_t *end,
                                   uint64_t *val, struct upb_status *status)
{
  uint8_t *uint64_end = buf + sizeof(uint64_t);
  if(uint64_end > end) {
    status->code = UPB_STATUS_NEED_MORE_DATA;
    return end;
  }
#if UPB_UNALIGNED_READS_OK
  *val = *(uint64_t*)buf;
#else
#define SHL(val, bits) ((uint64_t)val << bits)
  *val = SHL(buf[0],  0) | SHL(buf[1],  8) | SHL(buf[2], 16) | SHL(buf[3], 24) |
         SHL(buf[4], 32) | SHL(buf[5], 40) | SHL(buf[6], 48) | SHL(buf[7], 56);
#undef SHL
#endif
  return uint64_end;
}

INLINE uint8_t *upb_skip_v_uint64_t(uint8_t *buf, uint8_t *end,
                                    struct upb_status *status)
{
  uint8_t *const maxend = buf + 10;
  uint8_t last = 0x80;
  for(; buf < (uint8_t*)end && (last & 0x80); buf++)
    last = *buf;

  if(buf >= end && buf <= maxend && (last & 0x80)) {
    status->code = UPB_STATUS_NEED_MORE_DATA;
    buf = end;
  } else if(buf > maxend) {
    status->code = UPB_ERROR_UNTERMINATED_VARINT;
    buf = end;
  }
  return buf;
}

INLINE uint8_t *upb_skip_f_uint32_t(uint8_t *buf, uint8_t *end,
                                    struct upb_status *status)
{
  uint8_t *uint32_end = buf + sizeof(uint32_t);
  if(uint32_end > end) {
    status->code = UPB_STATUS_NEED_MORE_DATA;
    return end;
  }
  return uint32_end;
}

INLINE uint8_t *upb_skip_f_uint64_t(uint8_t *buf, uint8_t *end,
                                    struct upb_status *status)
{
  uint8_t *uint64_end = buf + sizeof(uint64_t);
  if(uint64_end > end) {
    status->code = UPB_STATUS_NEED_MORE_DATA;
    return end;
  }
  return uint64_end;
}

/* Functions to read .proto values. *******************************************/

// Performs zig-zag decoding, which is used by sint32 and sint64.
INLINE int32_t upb_zzdec_32(uint32_t n) { return (n >> 1) ^ -(int32_t)(n & 1); }
INLINE int64_t upb_zzdec_64(uint64_t n) { return (n >> 1) ^ -(int64_t)(n & 1); }

// Use macros to define a set of two functions for each .proto type:
//
//  // Reads and converts a .proto value from buf, placing it in d.
//  // "end" indicates the end of the current buffer (if the buffer does
//  // not contain the entire value UPB_STATUS_NEED_MORE_DATA is returned).
//  // On success, a pointer will be returned to the first byte that was
//  // not consumed.
//  uint8_t *upb_get_INT32(uint8_t *buf, uint8_t *end, int32_t *d,
//                         struct upb_status *status);
//
//  // Given an already read wire value s (source), convert it to a .proto
//  // value and return it.
//  int32_t upb_wvtov_INT32(uint32_t s);
//
// These are the most efficient functions to call if you want to decode a value
// for a known type.

#define WVTOV(type, wire_t, val_t) \
  INLINE val_t upb_wvtov_ ## type(wire_t s)

#define GET(type, v_or_f, wire_t, val_t, member_name) \
  INLINE uint8_t *upb_get_ ## type(uint8_t *buf, uint8_t *end, val_t *d, \
                                   struct upb_status *status) { \
    wire_t tmp = 0; \
    uint8_t *ret = upb_get_ ## v_or_f ## _ ## wire_t(buf, end, &tmp, status); \
    *d = upb_wvtov_ ## type(tmp); \
    return ret; \
  }

#define T(type, v_or_f, wire_t, val_t, member_name) \
  WVTOV(type, wire_t, val_t);  /* prototype for GET below */ \
  GET(type, v_or_f, wire_t, val_t, member_name) \
  WVTOV(type, wire_t, val_t)

T(INT32,    v, uint32_t, int32_t,  int32)   { return (int32_t)s;      }
T(INT64,    v, uint64_t, int64_t,  int64)   { return (int64_t)s;      }
T(UINT32,   v, uint32_t, uint32_t, uint32)  { return s;               }
T(UINT64,   v, uint64_t, uint64_t, uint64)  { return s;               }
T(SINT32,   v, uint32_t, int32_t,  int32)   { return upb_zzdec_32(s); }
T(SINT64,   v, uint64_t, int64_t,  int64)   { return upb_zzdec_64(s); }
T(FIXED32,  f, uint32_t, uint32_t, uint32)  { return s;               }
T(FIXED64,  f, uint64_t, uint64_t, uint64)  { return s;               }
T(SFIXED32, f, uint32_t, int32_t,  int32)   { return (int32_t)s;      }
T(SFIXED64, f, uint64_t, int64_t,  int64)   { return (int64_t)s;      }
T(BOOL,     v, uint32_t, bool,     _bool)   { return (bool)s;         }
T(ENUM,     v, uint32_t, int32_t,  int32)   { return (int32_t)s;      }
T(DOUBLE,   f, uint64_t, double,   _double) {
  union upb_value v;
  v.uint64 = s;
  return v._double;
}
T(FLOAT,    f, uint32_t, float,    _float)  {
  union upb_value v;
  v.uint32 = s;
  return v._float;
}

#undef WVTOV
#undef GET
#undef T

// Parses a tag, places the result in *tag.
INLINE uint8_t *parse_tag(uint8_t *buf, uint8_t *end, struct upb_tag *tag,
                          struct upb_status *status)
{
  uint32_t tag_int;
  uint8_t *ret = upb_get_v_uint32_t(buf, end, &tag_int, status);
  tag->wire_type    = (upb_wire_type_t)(tag_int & 0x07);
  tag->field_number = tag_int >> 3;
  return ret;
}


/**
 * Parses a 64-bit varint that is known to be >= 2 bytes (the inline version
 * handles 1 and 2 byte varints).
 */
uint8_t *upb_get_v_uint64_t_full(uint8_t *buf, uint8_t *end, uint64_t *val,
                                 struct upb_status *status)
{
  uint8_t *const maxend = buf + 10;
  uint8_t last = 0x80;
  *val = 0;
  int bitpos;

  for(bitpos = 0; buf < (uint8_t*)end && (last & 0x80); buf++, bitpos += 7)
    *val |= ((uint64_t)((last = *buf) & 0x7F)) << bitpos;

  if(buf >= end && buf <= maxend && (last & 0x80)) {
    upb_seterr(status, UPB_STATUS_NEED_MORE_DATA,
               "Provided data ended in the middle of a varint.\n");
    buf = end;
  } else if(buf > maxend) {
    upb_seterr(status, UPB_ERROR_UNTERMINATED_VARINT,
               "Varint was unterminated after 10 bytes.\n");
    buf = end;
  }

  return buf;
}

uint8_t *upb_parse_wire_value(uint8_t *buf, uint8_t *end, upb_wire_type_t wt,
                              union upb_wire_value *wv,
                              struct upb_status *status)
{
  switch(wt) {
    case UPB_WIRE_TYPE_VARINT:
      return upb_get_v_uint64_t(buf, end, &wv->varint, status);
    case UPB_WIRE_TYPE_64BIT:
      return upb_get_f_uint64_t(buf, end, &wv->_64bit, status);
    case UPB_WIRE_TYPE_32BIT:
      return upb_get_f_uint32_t(buf, end, &wv->_32bit, status);
    default:
      status->code = UPB_STATUS_ERROR;  // Doesn't handle delimited, groups.
      return end;
  }
}

/**
 * Advances buf past the current wire value (of type wt), saving the result in
 * outbuf.
 */
static uint8_t *skip_wire_value(uint8_t *buf, uint8_t *end, upb_wire_type_t wt,
                                struct upb_status *status)
{
  switch(wt) {
    case UPB_WIRE_TYPE_VARINT:
      return upb_skip_v_uint64_t(buf, end, status);
    case UPB_WIRE_TYPE_64BIT:
      return upb_skip_f_uint64_t(buf, end, status);
    case UPB_WIRE_TYPE_32BIT:
      return upb_skip_f_uint32_t(buf, end, status);
    case UPB_WIRE_TYPE_START_GROUP:
      // TODO: skip to matching end group.
    case UPB_WIRE_TYPE_END_GROUP:
      return buf;
    default:
      status->code = UPB_STATUS_ERROR;
      return end;
  }
}

uint8_t *upb_parse_value(uint8_t *buf, uint8_t *end, upb_field_type_t ft,
                         union upb_value_ptr v, struct upb_status *status)
{
#define CASE(t, member_name) \
  case UPB_TYPENUM(t): return upb_get_ ## t(buf, end, v.member_name, status);

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
    default: return end;
  }

#undef CASE
}

struct upb_cbparser {
  // Stack entries store the offset where the submsg ends (for groups, 0).
  size_t stack[UPB_MAX_NESTING], *top, *limit;
  size_t completed_offset;
  void *udata;
  upb_tag_cb    tag_cb;
  upb_value_cb  value_cb;
  upb_str_cb    str_cb;
  upb_start_cb  start_cb;
  upb_end_cb    end_cb;
};

struct upb_cbparser *upb_cbparser_new(void)
{
  return malloc(sizeof(struct upb_cbparser));
}

void upb_cbparser_free(struct upb_cbparser *p)
{
  free(p);
}

void upb_cbparser_reset(struct upb_cbparser *p, void *udata,
                        upb_tag_cb tagcb,
                        upb_value_cb valuecb,
                        upb_str_cb strcb,
                        upb_start_cb startcb,
                        upb_end_cb endcb)
{
  p->top = p->stack;
  p->limit = &p->stack[UPB_MAX_NESTING];
  p->completed_offset = 0;
  p->udata = udata;
  p->tag_cb = tagcb;
  p->value_cb = valuecb;
  p->str_cb = strcb;
  p->start_cb = startcb;
  p->end_cb = endcb;

  // The top-level message is not delimited (we can keep receiving data for it
  // indefinitely), so we treat it like a group.
  *p->top = 0;
}

/**
 * Pushes a new stack frame for a submessage with the given len (which will
 * be zero if the submessage is a group).
 */
static uint8_t *push(struct upb_cbparser *s, uint8_t *start,
                     uint32_t submsg_len, void *user_field_desc,
                     struct upb_status *status)
{
  s->top++;
  if(s->top >= s->limit) {
    upb_seterr(status, UPB_STATUS_ERROR,
               "Nesting exceeded maximum (%d levels)\n",
               UPB_MAX_NESTING);
    return NULL;
  }
  *s->top = s->completed_offset + submsg_len;

  if(s->start_cb)
    s->start_cb(s->udata, user_field_desc);

  if(*s->top > 0)
    return start + (*s->top - s->completed_offset);
  else
    return (void*)UINTPTR_MAX;
}

/**
 * Pops a stack frame, returning a pointer for where the next submsg should
 * end (or a pointer that is out of range for a group).
 */
static void *pop(struct upb_cbparser *s, uint8_t *start)
{
  if(s->end_cb)
    s->end_cb(s->udata);

  s->top--;

  if(*s->top > 0)
    return (char*)start + (*s->top - s->completed_offset);
  else
    return (void*)UINTPTR_MAX;  // group.
}


size_t upb_cbparser_parse(struct upb_cbparser *s, void *_buf, size_t len,
                          struct upb_status *status)
{
  uint8_t *buf = _buf;
  uint8_t *completed = buf;
  uint8_t *const start = buf;  // ptr equivalent of s->completed_offset
  uint8_t *end = buf + len;
  uint8_t *submsg_end = *s->top > 0 ? buf + *s->top : (uint8_t*)UINTPTR_MAX;

  // Make local copies so optimizer knows they won't change.
  upb_tag_cb tag_cb = s->tag_cb;
  upb_str_cb str_cb = s->str_cb;
  upb_value_cb value_cb = s->value_cb;
  void *udata = s->udata;

#define CHECK_STATUS() do { if(!upb_ok(status)) goto err; } while(0)

  // Main loop: parse a tag, then handle the value.
  while(buf < end) {
    struct upb_tag tag;
    buf = parse_tag(buf, end, &tag, status);
    if(tag.wire_type == UPB_WIRE_TYPE_END_GROUP) {
      CHECK_STATUS();
      submsg_end = pop(s, start);
      completed = buf;
      continue;
    }

    void *udesc;
    upb_field_type_t ft = tag_cb(udata, &tag, &udesc);
    if(tag.wire_type == UPB_WIRE_TYPE_DELIMITED) {
      int32_t delim_len;
      buf = upb_get_INT32(buf, end, &delim_len, status);
      CHECK_STATUS();
      uint8_t *delim_end = buf + delim_len;
      if(ft == UPB_TYPENUM(MESSAGE)) {
        submsg_end = push(s, start, delim_end - start, udesc, status);
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
          buf = skip_wire_value(buf, end, tag.wire_type, status);
          break;
        case UPB_TYPENUM(GROUP):
          submsg_end = push(s, start, 0, udesc, status);
          break;
        default:
          buf = value_cb(udata, buf, end, udesc, status);
          break;
      }
    }
    CHECK_STATUS();

    while(buf >= submsg_end) {
      if(buf > submsg_end) {
        return UPB_STATUS_ERROR;  // Bad submessage end.
      }
      submsg_end = pop(s, start);
    }
    // while(buf < s->packed_end) { TODO: packed arrays }
    completed = buf;
  }

  size_t read;
err:
  read = (char*)completed - (char*)start;
  s->completed_offset += read;
  return read;
}
