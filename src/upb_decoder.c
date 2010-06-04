/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2008-2009 Joshua Haberman.  See LICENSE for details.
 */

#include "upb_decoder.h"

#include <inttypes.h>
#include <stddef.h>
#include <stdlib.h>
#include "upb_def.h"

/* Functions to read wire values. *********************************************/

// These functions are internal to the decode, but might be moved into an
// internal header file if we at some point in the future opt to do code
// generation, because the generated code would want to inline these functions.
// The same applies to the functions to read .proto values below.

const int8_t upb_get_v_uint64_t_full(const uint8_t *buf, uint64_t *val);

// Gets a varint (wire type: UPB_WIRE_TYPE_VARINT).  Caller promises that >=10
// bytes are available at buf.  Returns the number of bytes consumed, or <0 if
// the varint was unterminated after 10 bytes.
INLINE int8_t upb_get_v_uint64_t(const uint8_t *buf, uint64_t *val)
{
  // We inline this common case (1-byte varints), if that fails we dispatch to
  // the full (non-inlined) version.
  int8_t ret = 1;
  *val = *buf & 0x7f;
  if(*buf & 0x80) {
    // Varint is >1 byte.
    ret += upb_get_v_uint64_t_full(buf + 1, val);
  }
  return ret;
}

// Gets a varint -- called when we only need 32 bits of it.  Note that a 32-bit
// varint is not a true wire type.
INLINE int8_t upb_get_v_uint32_t(const uint8_t *buf, uint32_t *val)
{
  uint64_t val64;
  int8_t ret = upb_get_v_uint64_t(buf, end, &val64, status);
  *val = (uint32_t)val64;  // Discard the high bits.
  return ret;
}

// Gets a fixed-length 32-bit integer (wire type: UPB_WIRE_TYPE_32BIT).  Caller
// promises that 4 bytes are available at buf.
INLINE void upb_get_f_uint32_t(const uint8_t *buf, uint32_t *val)
{
#if UPB_UNALIGNED_READS_OK
  *val = *(uint32_t*)buf;
#else
#define SHL(val, bits) ((uint32_t)val << bits)
  *val = SHL(buf[0], 0) | SHL(buf[1], 8) | SHL(buf[2], 16) | SHL(buf[3], 24);
#undef SHL
#endif
}

// Gets a fixed-length 64-bit integer (wire type: UPB_WIRE_TYPE_64BIT).
INLINE void upb_get_f_uint64_t(const uint8_t *buf uint64_t *val)
{
#if UPB_UNALIGNED_READS_OK
  *val = *(uint64_t*)buf;
#else
#define SHL(val, bits) ((uint64_t)val << bits)
  *val = SHL(buf[0],  0) | SHL(buf[1],  8) | SHL(buf[2], 16) | SHL(buf[3], 24) |
         SHL(buf[4], 32) | SHL(buf[5], 40) | SHL(buf[6], 48) | SHL(buf[7], 56);
#undef SHL
#endif
}

INLINE const uint8_t *upb_skip_v_uint64_t(const uint8_t *buf,
                                          const uint8_t *end,
                                          upb_status *status)
{
  const uint8_t *const maxend = buf + 10;
  uint8_t last = 0x80;
  for(; buf < (uint8_t*)end && (last & 0x80); buf++)
    last = *buf;

  if(buf > maxend) return -1;
  return buf;
}

/* Functions to read .proto values. *******************************************/

// Performs zig-zag decoding, which is used by sint32 and sint64.
INLINE int32_t upb_zzdec_32(uint32_t n) { return (n >> 1) ^ -(int32_t)(n & 1); }
INLINE int64_t upb_zzdec_64(uint64_t n) { return (n >> 1) ^ -(int64_t)(n & 1); }

// Use macros to define a set of two functions for each .proto type:
//
//  // Reads and converts a .proto value from buf, placing it in d.  At least
//  // 10 bytes must be available at "buf".  On success, the number of bytes
//  // consumed is returned, otherwise <0.
//  const int8_t upb_get_INT32(const uint8_t *buf, int32_t *d);
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
  INLINE const uint8_t *upb_get_ ## type(const uint8_t *buf, val_t *d) { \
    wire_t tmp = 0; \
    const int8_t ret = upb_get_ ## v_or_f ## _ ## wire_t(buf, &tmp); \
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
  upb_value v;
  v.uint64 = s;
  return v._double;
}
T(FLOAT,    f, uint32_t, float,    _float)  {
  upb_value v;
  v.uint32 = s;
  return v._float;
}

#undef WVTOV
#undef GET
#undef T

// Parses a 64-bit varint that is known to be >= 2 bytes (the inline version
// handles 1 and 2 byte varints).
const int8_t upb_get_v_uint64_t_full(const uint8_t *buf uint64_t *val)
{
  const uint8_t *const maxend = buf + 9;
  uint8_t last = 0x80;
  int bitpos;

  for(bitpos = 0; buf < (uint8_t*)maxend && (last & 0x80); buf++, bitpos += 7)
    *val |= ((uint64_t)((last = *buf) & 0x7F)) << bitpos;

  if(buf >= maxend) {
    return -11;
  }
  return buf;
}

static const uint8_t *upb_decode_value(const uint8_t *buf, const uint8_t *end,
                                       upb_field_type_t ft, upb_valueptr v,
                                       upb_status *status)
{
#define CASE(t, member_name) \
  case UPB_TYPE(t): return upb_get_ ## t(buf, end, v.member_name, status);

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

// The decoder keeps a stack with one entry per level of recursion.
// upb_decoder_frame is one frame of that stack.
typedef struct {
  upb_msgdef *msgdef;
  upb_fielddef *field;
  int32_t end_offset;  // For groups, -1.
} upb_decoder_frame;

struct upb_decoder {
  // Immutable state of the decoder.
  upb_msgdef *toplevel_msgdef;
  upb_bytesrc *bytesrc;

  // State pertaining to a particular decode (resettable).
  // Stack entries store the offset where the submsg ends (for groups, 0).
  upb_decoder_frame stack[UPB_MAX_NESTING], *top, *limit;

  // The current buffer.
  upb_string *buf;

  // The overflow buffer.  Used when fewer than UPB_MAX_ENCODED_SIZE bytes
  // are left in a buffer, the remaining bytes are copied here along with
  // the bytes from the next buffer (or 0x80 if the byte stream is EOF).
  uint8_t overflow_buf[UPB_MAX_ENCODED_SIZE];

  // The number of bytes we have yet to consume from this buffer.
  uint32_t buf_bytes_remaining;

  // The overall stream offset of the beginning of this buffer.
  uint32_t buf_stream_offset;

  // Indicates that we are in the middle of skipping bytes or groups (or both).
  // If both are set, the byte-skipping needs to happen first.
  uint8_t skip_groups;
  uint32_t skip_bytes;

  bool eof;
};

/* upb_decoder construction/destruction. **************************************/

upb_decoder *upb_decoder_new(upb_msgdef *msgdef)
{
  upb_decoder *d = malloc(sizeof(*d));
  d->toplevel_msgdef = msgdef;
  d->limit = &d->stack[UPB_MAX_NESTING];
  return d;
}

void upb_decoder_free(upb_decoder *d)
{
  free(d);
}

void upb_decoder_reset(upb_decoder *d, upb_sink *sink)
{
  d->top = d->stack;
  d->completed_offset = 0;
  d->sink = sink;
  d->top->msgdef = d->toplevel_msgdef;
  // The top-level message is not delimited (we can keep receiving data for it
  // indefinitely), so we treat it like a group.
  d->top->end_offset = 0;
}

/* upb_decoder buffering. *****************************************************/

bool upb_decoder_get_v_uint32_t(upb_decoder *d, uint32_t *val) {}

static const void *get_msgend(upb_decoder *d, const uint8_t *start)
{
  if(d->top->end_offset > 0)
    return start + (d->top->end_offset - d->completed_offset);
  else
    return (void*)UINTPTR_MAX;  // group.
}

static bool isgroup(const void *submsg_end)
{
  return submsg_end == (void*)UINTPTR_MAX;
}

upb_fielddef *upb_decoder_getdef(upb_decoder *d)
{
  // Detect end-of-submessage.
  if(offset >= d->top->end_offset) {
    d->eof = true;
    return NULL;
  }

  // Handles the packed field case.
  if(d->field) return d->field;
  if(d->eof) return NULL;

again:
  uint32_t key;
  if(!upb_decoder_get_v_uint32_t(d, &key)) return NULL;
  if(upb_wiretype_from_key(key) == UPB_WIRE_TYPE_END_GROUP) {
    if(isgroup(d->top->submsg_end)) {
      d->eof = true;
      d->status->code = UPB_STATUS_EOF;
    } else {
      upb_seterr(d->status, UPB_STATUS_ERROR, "End group seen but current "
                 "message is not a group, byte offset: %zd",
                 upb_decoder_offset(d));
    }
    return NULL;
  }

  // For delimited wire values we parse the length now, since we need it in all
  // cases.
  if(d->key.wire_type == UPB_WIRE_TYPE_DELIMITED) {
    if(!upb_decoder_get_v_uint32_t(d, &d->delim_len)) return NULL;
  }

  // Look up field by tag number.
  upb_fielddef *f = upb_msg_itof(d->top->msgdef, upb_fieldnum_from_key(key));

  if (!f || !upb_check_type(upb_wiretype_from_key(key), f->type)) {
    // Unknown field or incorrect wire type.  In the future these cases may be
    // separated, like if we want to give the client unknown fields but not
    // incorrect fields.
    upb_decoder_skipval(d);
    goto again;
  }
  return f;
}

bool upb_decoder_getval(upb_decoder *d, upb_valueptr val)
{
  if(upb_isstringtype(d->f->type)) {
    d->str = upb_string_tryrecycle(d->str);
    if (d->delimited_len <= d->bytes_left) {
      upb_string_substr(d->str, d->buf, upb_string_len(d->buf) - d->bytes_left, d->delimited_len);
    }
  } else {
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    buf = upb_decode_value(buf, end, f->type, val, &d->status);
  }
}

bool upb_decoder_skipval(upb_decoder *d) {
  switch(d->key.wire_type) {
    case UPB_WIRE_TYPE_VARINT:
      return upb_skip_v_uint64_t(buf, end, status);
    case UPB_WIRE_TYPE_64BIT:
      return upb_skip_bytes(8);
    case UPB_WIRE_TYPE_32BIT:
      return upb_skip_bytes(4);
    case UPB_WIRE_TYPE_START_GROUP:
      return upb_skip_groups(1);
    case UPB_WIRE_TYPE_DELIMITED:
      return upb_skip_bytes(d->delimited_len);
    default:
      // Including UPB_WIRE_TYPE_END_GROUP.
      assert(false);
      upb_seterr(&d->status, UPB_STATUS_ERROR, "Tried to skip an end group");
      return false;
  }
}

bool upb_decoder_startmsg(upb_src *src) {
    } else if(f->type == UPB_TYPE(MESSAGE)) {
      submsg_end = push(d, start, delim_end - start, f, status);
      msgdef = d->top->msgdef;
    } else if (f->type == UPB_TYPE(GROUP)) {
      submsg_end = push(d, start, 0, f, status);
      msgdef = d->top->msgdef;
  d->top->field = f;
  d->top++;
  if(d->top >= d->limit) {
    upb_seterr(status, UPB_ERROR_MAX_NESTING_EXCEEDED,
               "Nesting exceeded maximum (%d levels)\n",
               UPB_MAX_NESTING);
    return NULL;
  }
  upb_decoder_frame *frame = d->top;
  frame->end_offset = d->completed_offset + submsg_len;
  frame->msgdef = upb_downcast_msgdef(f->def);

  return get_msgend(d, start);
}

bool upb_decoder_endmsg(upb_decoder *src) {
  d->top--;
}

upb_status *upb_decoder_status(upb_decoder *d) { return &d->status; }

