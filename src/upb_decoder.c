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

const int8_t upb_get_v_uint64_full(const uint8_t *buf, uint64_t *val);

// Gets a varint (wire type: UPB_WIRE_TYPE_VARINT).  Caller promises that >=10
// bytes are available at buf.  Returns the number of bytes consumed, or 11 if
// the varint was unterminated after 10 bytes.
INLINE uint8_t upb_get_v_uint64(const uint8_t *buf, uint64_t *val)
{
  // We inline this common case (1-byte varints), if that fails we dispatch to
  // the full (non-inlined) version.
  int8_t ret = 1;
  *val = *buf & 0x7f;
  if(*buf & 0x80) {
    // Varint is >1 byte.
    ret += upb_get_v_uint64_full(buf + 1, val);
  }
  return ret;
}

// Gets a varint -- called when we only need 32 bits of it.  Note that a 32-bit
// varint is not a true wire type.
INLINE uint8_t upb_get_v_uint32(const uint8_t *buf, uint32_t *val)
{
  uint64_t val64;
  int8_t ret = upb_get_v_uint64(buf, end, &val64, status);
  *val = (uint32_t)val64;  // Discard the high bits.
  return ret;
}

// Gets a fixed-length 32-bit integer (wire type: UPB_WIRE_TYPE_32BIT).  Caller
// promises that 4 bytes are available at buf.
INLINE void upb_get_f_uint32(const uint8_t *buf, uint32_t *val)
{
#if UPB_UNALIGNED_READS_OK
  *val = *(uint32_t*)buf;
#else
#define SHL(val, bits) ((uint32_t)val << bits)
  *val = SHL(buf[0], 0) | SHL(buf[1], 8) | SHL(buf[2], 16) | SHL(buf[3], 24);
#undef SHL
#endif
}

// Gets a fixed-length 64-bit integer (wire type: UPB_WIRE_TYPE_64BIT).  Caller
// promises that 8 bytes are available at buf.
INLINE void upb_get_f_uint64(const uint8_t *buf uint64_t *val)
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

// Skips a varint (wire type: UPB_WIRE_TYPE_VARINT).  Caller promises that 10
// bytes are available at "buf".  Returns the number of bytes that were
// skipped.
INLINE const uint8_t *upb_skip_v_uint64(const uint8_t *buf)
{
  const uint8_t *const maxend = buf + 10;
  uint8_t last = 0x80;
  for(; buf < (uint8_t*)end && (last & 0x80); buf++)
    last = *buf;

  if(buf > maxend) return -1;
  return buf;
}

// Parses a 64-bit varint that is known to be >= 2 bytes (the inline version
// handles 1 and 2 byte varints).
const uint8_t upb_get_v_uint64_full(const uint8_t *buf uint64_t *val)
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

// Performs zig-zag decoding, which is used by sint32 and sint64.
INLINE int32_t upb_zzdec_32(uint32_t n) { return (n >> 1) ^ -(int32_t)(n & 1); }
INLINE int64_t upb_zzdec_64(uint64_t n) { return (n >> 1) ^ -(int64_t)(n & 1); }


/* upb_decoder ****************************************************************/

// The decoder keeps a stack with one entry per level of recursion.
// upb_decoder_frame is one frame of that stack.
typedef struct {
  upb_msgdef *msgdef;
  upb_fielddef *field;
  int32_t end_offset;  // For groups, -1.
} upb_decoder_frame;

struct upb_decoder {
  upb_src src;  // upb_decoder is a upb_src.

  upb_msgdef *toplevel_msgdef;
  upb_bytesrc *bytesrc;

  // We keep a stack of messages we have recursed into.
  upb_decoder_frame stack[UPB_MAX_NESTING], *top, *limit;

  // The buffers of input data.  See buffering code below for details.
  upb_string *buf;
  upb_string *nextbuf;
  uint8_t tmpbuf[UPB_MAX_ENCODED_SIZE];  // Used to bridge buf and nextbuf.

  // The number of bytes we have yet to consume from "buf".  This can be
  // negative if we have skipped more bytes than are in the buffer, or if we
  // have started to consume bytes from "nextbuf".
  int32_t buf_bytesleft;

  // The overall stream offset of the end of "buf".  If "buf" is NULL, it is as
  // if "buf" was the empty string.
  uint32_t buf_stream_offset;
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

// Discards the current buffer if we are done with it, make the next buffer
// current if there is one.
static void upb_decoder_advancebuf(upb_decoder *d)
{
  if(d->buf_bytes_remaining <= 0) {
    if(d->buf) upb_bytesrc_recycle(d->bytesrc, d->buf);
    d->buf = d->nextbuf;
    d->nextbuf = NULL;
    if(d->buf) d->buf_bytes_remaining += upb_string_len(d->buf);
  }
}

static void upb_decoder_pullnextbuf(upb_decoder *d)
{
  if(!d->nextbuf) {
    d->nextbuf = upb_bytesrc_get(d->bytesrc);
    if(!d->nextbuf && !upb_bytesrc_eof(d->bytesrc)) {
      // There was an error in the byte stream, halt the decoder.
      upb_copyerr(&d->status, upb_bytesrc_status(d->bytesrc));
      return;
    }
  }
}

static void upb_decoder_skipbytes(upb_decoder *d, int32_t bytes)
{
  d->buf_bytes_remaining -= bytes;
  while(d->buf_bytes_remaining <= 0) {
    upb_decoder_pullnextbuf(d);
    upb_decoder_advancebuf(d);
  }
}

static void upb_decoder_skipgroup(upb_decoder *d)
{
  // This will be mututally recursive if the group has sub-groups.  If we
  // wanted to handle EAGAIN in the future, this approach would not work;
  // we would need to track the group depth explicitly.
  while(upb_decoder_getdef(d)) upb_decoder_skipval(d);
}

static const uint8_t *upb_decoder_getbuf_full(upb_decoder *d, int32_t *bytes)
{
  upb_decoder_pullnextbuf(d);
  upb_decoder_advancebuf(d);
  if(d->buf_bytes_remaining >= UPB_MAX_ENCODED_SIZE) {
    return upb_string_getrobuf(d->buf) + upb_string_len(d->buf) -
        d->buf_bytes_remaining;
  } else {
    upb_strlen_t total = 0;
    if(d->buf) total += upb_decoder_append(d->buf, total);
    if(d->nextbuf) total += upb_decoder_append(d->nextbuf, total);
    memset(d->overflow_buf + total, 0x80, UPB_MAX_ENCODED_SIZE - total);
  }
}

// Returns a pointer to a buffer of data that is at least UPB_MAX_ENCODED_SIZE
// bytes long.  This buffer contains the next bytes in the stream (even if
// those bytes span multiple buffers).  *bytes is set to the number of actual
// stream bytes that are available in the returned buffer.  If
// *bytes < UPB_MAX_ENCODED_SIZE, the buffer is padded with 0x80 bytes.
INLINE static const uint8_t *upb_decoder_getbuf(upb_decoder *d, int32_t *bytes)
{
  if(d->buf_bytes_remaining >= UPB_MAX_ENCODED_SIZE) {
    *bytes = d->buf_bytes_remaining;
    return upb_string_getrobuf(d->buf) + upb_string_len(d->buf) -
        d->buf_bytes_remaining;
  } else {
    return upb_decoder_getbuf_full(d, bytes);
  }
}

/* upb_src implementation for upb_decoder. ************************************/

upb_fielddef *upb_decoder_getdef(upb_decoder *d)
{
  // Detect end-of-submessage.
  if(upb_decoder_offset(d) >= d->top->end_offset) {
    d->eof = true;
    return NULL;
  }

  // Handles the packed field case.
  if(d->field) return d->field;

again:
  uint32_t key;
  if(!upb_decoder_get_v_uint32(d, &key)) {
    return NULL;

  if(d->key.wire_type == UPB_WIRE_TYPE_DELIMITED) {
    // For delimited wire values we parse the length now, since we need it in
    // all cases.
    if(!upb_decoder_get_v_uint32(d, &d->delim_len)) return NULL;
  } else if(upb_wiretype_from_key(key) == UPB_WIRE_TYPE_END_GROUP) {
    if(isgroup(d->top->submsg_end)) {
      d->eof = true;
    } else {
      upb_seterr(d->status, UPB_STATUS_ERROR, "End group seen but current "
                 "message is not a group, byte offset: %zd",
                 upb_decoder_offset(d));
    }
    return NULL;
  }

  // Look up field by tag number.
  upb_fielddef *f = upb_msg_itof(d->top->msgdef, upb_fieldnum_from_key(key));

  if (!f) {
    // Unknown field.  If/when the upb_src interface supports reporting
    // unknown fields we will implement that here.
    upb_decoder_skipval(d);
    goto again;
  } else if (!upb_check_type(upb_wiretype_from_key(key), f->type)) {
    // This is a recoverable error condition.  We skip the value but also
    // return NULL and report the error.
    upb_decoder_skipval(d);
    // TODO: better error message.
    upb_seterr(&d->status, UPB_STATUS_ERROR, "Incorrect wire type.\n");
    return NULL;
  }
  d->field = f;
  return f;
}

bool upb_decoder_getval(upb_decoder *d, upb_valueptr val)
{
  if(expected_type_for_field == UPB_DELIMITED) {
    // A string, bytes, or a length-delimited submessage.  The latter isn't
    // technically a string, but can be gotten as one to perform lazy parsing.
    d->str = upb_string_tryrecycle(d->str);
    const upb_strlen_t total_len = d->delimited_len;
    if (total_len <= d->buf_bytes_remaining) {
      // The entire string is inside our current buffer, so we can just
      // return a substring of the buffer without copying.
      upb_string_substr(d->str, d->buf,
                        upb_string_len(d->buf) - d->buf_bytes_remaining,
                        total_len);
      d->buf_bytes_remaining -= total_len
      *val.str = d->str;
    } else {
      // The string spans buffers, so we must copy from the current buffer,
      // the next buffer (if we have one), and finally from the bytesrc.
      char *str = upb_string_getrwbuf(d->str, d->);
      upb_strlen_t len = 0;
      len += upb_decoder_append(d->buf, len, total_len);
      if(!upb_decoder_advancebuf(d)) goto err;
      if(d->buf) len += upb_decoder_append(d->buf, len, total_len);
      if(len < total_len)
        if(!upb_bytesrc_append(d->bytesrc, d->str, len - bytes)) goto err;
    }
    d->field = NULL;
  } else {
    // For all of the integer types we need the bytes to be in a single
    // contiguous buffer.
    uint32_t bytes;
    const uint8_t *buf = upb_decoder_getbuf(d, &bytes)
    switch(expected_type_for_field) {
      case UPB_64BIT_VARINT:
        if(upb_get_v_uint32(buf, val.uint32) > 10) goto err;
        if(f->type == UPB_TYPE(SINT64)) *val.int64 = upb_zzdec_64(*val.int64);
        break;
      case UPB_32BIT_VARINT:
        if(upb_get_v_uint64(buf, val.uint64) > 5) goto err;
        if(f->type == UPB_TYPE(SINT32)) *val.int32 = upb_zzdec_32(*val.int32);
        break;
      case UPB_64BIT_FIXED:
        if(bytes < 8) goto err;
        upb_get_f_uint64(buf, val.uint64);
        break;
      case UPB_32BIT_FIXED:
        if(bytes < 4) goto err;
        upb_get_f_uint32(buf, val.uint32);
        break;
      default:
        // Including start/end group.
        goto err;
    }
    if(wire_type != UPB_WIRE_TYPE_DELIMITED ||
       upb_decoder_offset(d) >= d->packed_end_offset) {
      d->field = NULL;
    }
  }
  return true;
err:
}

bool upb_decoder_skipval(upb_decoder *d) {
  switch(d->key.wire_type) {
    case UPB_WIRE_TYPE_VARINT:
      return upb_skip_v_uint64(buf, end, status);
    case UPB_WIRE_TYPE_64BIT:
      return upb_skip_bytes(8);
    case UPB_WIRE_TYPE_32BIT:
      return upb_skip_bytes(4);
    case UPB_WIRE_TYPE_START_GROUP:
      return upb_skip_groups(1);
    case UPB_WIRE_TYPE_DELIMITED:
      // Works for both string/bytes *and* submessages.
      return upb_skip_bytes(d->delimited_len);
    default:
      // Including UPB_WIRE_TYPE_END_GROUP.
      assert(false);
      upb_seterr(&d->status, UPB_STATUS_ERROR, "Tried to skip an end group");
      return false;
  }
}

bool upb_decoder_startmsg(upb_src *src) {
  d->top->field = f;
  d->top++;
  if(d->top >= d->limit) {
    upb_seterr(d->status, UPB_ERROR_MAX_NESTING_EXCEEDED,
               "Nesting exceeded maximum (%d levels)\n",
               UPB_MAX_NESTING);
    return false;
  }
  upb_decoder_frame *frame = d->top;
  frame->end_offset = d->completed_offset + submsg_len;
  frame->msgdef = upb_downcast_msgdef(f->def);

  return get_msgend(d, start);
}

bool upb_decoder_endmsg(upb_decoder *src) {
  d->top--;
  if(!d->eof) {
    if(d->top->f->type == UPB_TYPE(GROUP))
      upb_skip_group();
    else
      upb_skip_bytes(foo);
  }
  d->eof = false;
}

upb_status *upb_decoder_status(upb_decoder *d) { return &d->status; }

