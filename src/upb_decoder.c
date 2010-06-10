/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2008-2009 Joshua Haberman.  See LICENSE for details.
 */

#include "upb_decoder.h"

#include <inttypes.h>
#include <stddef.h>
#include <stdlib.h>

#define UPB_GROUP_END_OFFSET UINT32_MAX

// Returns true if the give wire type and field type combination is valid,
// taking into account both packed and non-packed encodings.
static bool upb_check_type(upb_wire_type_t wt, upb_field_type_t ft) {
  return (1 << wt) & upb_types[ft].allowed_wire_types;
}

/* Functions to read wire values. *********************************************/

static uint8_t upb_get_v_uint64_full(const uint8_t *buf, uint64_t *val);

// Gets a varint (wire type: UPB_WIRE_TYPE_VARINT).  Caller promises that >=10
// bytes are available at buf.  Returns the number of bytes consumed, or 11 if
// the varint was unterminated after 10 bytes.
INLINE uint8_t upb_get_v_uint64(const uint8_t *buf, uint64_t *val)
{
  // We inline this common case (1-byte varints), if that fails we dispatch to
  // the full (non-inlined) version.
  *val = *buf & 0x7f;
  if((*buf & 0x80) == 0) return 1;
  return upb_get_v_uint64_full(buf + 1, val);
}

// Gets a varint -- called when we only need 32 bits of it.  Note that a 32-bit
// varint is not a true wire type.
INLINE uint8_t upb_get_v_uint32(const uint8_t *buf, uint32_t *val)
{
  uint64_t val64;
  int8_t ret = upb_get_v_uint64(buf, &val64);
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
INLINE void upb_get_f_uint64(const uint8_t *buf, uint64_t *val)
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
INLINE uint8_t upb_skip_v_uint64(const uint8_t *buf)
{
  const uint8_t *const maxend = buf + 10;
  uint8_t last = 0x80;
  for(; buf < maxend && (last & 0x80); buf++)
    last = *buf;

  return 0;  // TODO
}

// Parses remining bytes of a 64-bit varint that has already had its first byte
// parsed.
static uint8_t upb_get_v_uint64_full(const uint8_t *buf, uint64_t *val)
{
  uint8_t bytes = 0;

  // bitpos starts at 7 because our caller already read one byte.
  for(int bitpos = 7; bytes < 10 && (*buf & 0x80); buf++, bitpos += 7)
    *val |= (uint64_t)(*buf & 0x7F) << bitpos;

  return bytes;
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
  upb_strlen_t end_offset;  // For groups, -1.
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
  uint32_t buf_endoffset;

  // Fielddef for the key we just read.
  upb_fielddef *field;

  // Wire type of the key we just read.
  upb_wire_type_t wire_type;

  // Delimited length of the string field we are reading.
  upb_strlen_t delimited_len;

  upb_strlen_t packed_end_offset;

  // String we return for string values.  We try to recycle it if possible.
  upb_string *str;
};


/* upb_decoder construction/destruction. **************************************/

upb_decoder *upb_decoder_new(upb_msgdef *msgdef)
{
  upb_decoder *d = malloc(sizeof(*d));
  d->toplevel_msgdef = msgdef;
  d->limit = &d->stack[UPB_MAX_NESTING];
  d->buf = NULL;
  d->nextbuf = NULL;
  d->str = upb_string_new();
  return d;
}

void upb_decoder_free(upb_decoder *d)
{
  free(d);
}

void upb_decoder_reset(upb_decoder *d, upb_bytesrc *bytesrc)
{
  if(d->buf) upb_bytesrc_recycle(d->bytesrc, d->buf);
  if(d->nextbuf) upb_bytesrc_recycle(d->bytesrc, d->nextbuf);
  d->top = d->stack;
  d->top->msgdef = d->toplevel_msgdef;
  // The top-level message is not delimited (we can keep receiving data for it
  // indefinitely), so we set the end offset as high as possible, but not equal
  // to UINT32_MAX so it doesn't equal UPB_GROUP_END_OFFSET.
  d->top->end_offset = UINT32_MAX - 1;
  d->bytesrc = bytesrc;
  d->buf = NULL;
  d->nextbuf = NULL;
  d->buf_bytesleft = 0;
  d->buf_endoffset = 0;
}


/* upb_decoder buffering. *****************************************************/

static upb_strlen_t upb_decoder_offset(upb_decoder *d)
{
  return d->buf_endoffset - d->buf_bytesleft;
}

// Discards the current buffer if we are done with it, make the next buffer
// current if there is one.
static void upb_decoder_advancebuf(upb_decoder *d)
{
  if(d->buf_bytesleft <= 0) {
    if(d->buf) upb_bytesrc_recycle(d->bytesrc, d->buf);
    d->buf = d->nextbuf;
    d->nextbuf = NULL;
    if(d->buf) d->buf_bytesleft += upb_string_len(d->buf);
  }
}

static bool upb_decoder_pullnextbuf(upb_decoder *d)
{
  if(!d->nextbuf) {
    d->nextbuf = upb_bytesrc_get(d->bytesrc, UPB_MAX_ENCODED_SIZE);
    if(!d->nextbuf && !upb_bytesrc_eof(d->bytesrc)) {
      // There was an error in the byte stream, halt the decoder.
      upb_copyerr(&d->src.status, upb_bytesrc_status(d->bytesrc));
      return false;
    }
  }
  return true;
}

static bool upb_decoder_skipbytes(upb_decoder *d, int32_t bytes)
{
  d->buf_bytesleft -= bytes;
  while(d->buf_bytesleft <= 0 && !upb_bytesrc_eof(d->bytesrc)) {
    if(!upb_decoder_pullnextbuf(d)) return false;
    upb_decoder_advancebuf(d);
  }
  return true;
}

bool upb_decoder_skipval(upb_decoder *d);
upb_fielddef *upb_decoder_getdef(upb_decoder *d);

static bool upb_decoder_skipgroup(upb_decoder *d)
{
  // This will be mututally recursive with upb_decoder_skipval() if the group
  // has sub-groups.  If we wanted to handle EAGAIN in the future, this
  // approach would not work; we would need to track the group depth
  // explicitly.
  while(upb_decoder_getdef(d)) {
    if(!upb_decoder_skipval(d)) return false;
  }
  // If we are at the end of the group like we want to be, then
  // upb_decoder_getdef() returned NULL because of eof, not error.
  return upb_ok(&d->src.status);
}

upb_strlen_t upb_decoder_append(uint8_t *buf, upb_string *frombuf,
                                upb_strlen_t len, upb_strlen_t total_len);

static const uint8_t *upb_decoder_getbuf_full(upb_decoder *d, uint32_t *bytes)
{
  upb_decoder_pullnextbuf(d);
  upb_decoder_advancebuf(d);
  if(d->buf_bytesleft >= UPB_MAX_ENCODED_SIZE) {
    *bytes = d->buf_bytesleft;
    return (uint8_t*)upb_string_getrobuf(d->buf) + upb_string_len(d->buf) -
        d->buf_bytesleft;
  } else {
    upb_strlen_t len = 0;
    if(d->buf)
      len += upb_decoder_append(d->tmpbuf, d->buf, len, UPB_MAX_ENCODED_SIZE);
    if(d->nextbuf)
      len += upb_decoder_append(d->tmpbuf, d->nextbuf, len, UPB_MAX_ENCODED_SIZE);
    *bytes = len;
    memset(d->tmpbuf + len, 0x80, UPB_MAX_ENCODED_SIZE - len);
    return d->tmpbuf;
  }
}

// Returns a pointer to a buffer of data that is at least UPB_MAX_ENCODED_SIZE
// bytes long.  This buffer contains the next bytes in the stream (even if
// those bytes span multiple buffers).  *bytes is set to the number of actual
// stream bytes that are available in the returned buffer.  If
// *bytes < UPB_MAX_ENCODED_SIZE, the buffer is padded with 0x80 bytes.
INLINE const uint8_t *upb_decoder_getbuf(upb_decoder *d, uint32_t *bytes)
{
  if(d->buf_bytesleft >= UPB_MAX_ENCODED_SIZE) {
    // The common case; only when we get to the last ten bytes of the buffer
    // do we have to do tricky things.
    *bytes = d->buf_bytesleft;
    return (uint8_t*)upb_string_getrobuf(d->buf) + upb_string_len(d->buf) -
        d->buf_bytesleft;
  } else {
    return upb_decoder_getbuf_full(d, bytes);
  }
}

/* upb_src implementation for upb_decoder. ************************************/

bool upb_decoder_get_v_uint32(upb_decoder *d, uint32_t *key);

upb_fielddef *upb_decoder_getdef(upb_decoder *d)
{
  uint32_t key;
  upb_wire_type_t wire_type;

  // Detect end-of-submessage.
  if(upb_decoder_offset(d) >= d->top->end_offset) {
    d->src.eof = true;
    return NULL;
  }

  // Handles the packed field case.
  if(d->field) return d->field;

 again:
  if(!upb_decoder_get_v_uint32(d, &key)) {
    return NULL;
  }
  wire_type = key & 0x7;

  if(wire_type == UPB_WIRE_TYPE_DELIMITED) {
    // For delimited wire values we parse the length now, since we need it in
    // all cases.
    if(!upb_decoder_get_v_uint32(d, &d->delimited_len)) return NULL;
  } else if(wire_type == UPB_WIRE_TYPE_END_GROUP) {
    if(d->top->end_offset == UPB_GROUP_END_OFFSET) {
      d->src.eof = true;
    } else {
      upb_seterr(&d->src.status, UPB_STATUS_ERROR, "End group seen but current "
                 "message is not a group, byte offset: %zd",
                 upb_decoder_offset(d));
    }
    return NULL;
  }

  // Look up field by tag number.
  upb_fielddef *f = upb_msg_itof(d->top->msgdef, key >> 3);

  if (!f) {
    // Unknown field.  If/when the upb_src interface supports reporting
    // unknown fields we will implement that here.
    upb_decoder_skipval(d);
    goto again;
  } else if (!upb_check_type(wire_type, f->type)) {
    // This is a recoverable error condition.  We skip the value but also
    // return NULL and report the error.
    upb_decoder_skipval(d);
    // TODO: better error message.
    upb_seterr(&d->src.status, UPB_STATUS_ERROR, "Incorrect wire type.\n");
    return NULL;
  }
  d->field = f;
  d->wire_type = wire_type;
  return f;
}

bool upb_decoder_getval(upb_decoder *d, upb_valueptr val)
{
  upb_wire_type_t native_wire_type = upb_types[d->field->type].native_wire_type;
  if(native_wire_type == UPB_WIRE_TYPE_DELIMITED) {
    // A string, bytes, or a length-delimited submessage.  The latter isn't
    // technically a string, but can be gotten as one to perform lazy parsing.
    d->str = upb_string_tryrecycle(d->str);
    const upb_strlen_t total_len = d->delimited_len;
    if ((int32_t)total_len <= d->buf_bytesleft) {
      // The entire string is inside our current buffer, so we can just
      // return a substring of the buffer without copying.
      upb_string_substr(d->str, d->buf,
                        upb_string_len(d->buf) - d->buf_bytesleft,
                        total_len);
      d->buf_bytesleft -= total_len;
      *val.str = d->str;
    } else {
      // The string spans buffers, so we must copy from the current buffer,
      // the next buffer (if we have one), and finally from the bytesrc.
      uint8_t *str = (uint8_t*)upb_string_getrwbuf(d->str, total_len);
      upb_strlen_t len = 0;
      len += upb_decoder_append(str, d->buf, len, total_len);
      upb_decoder_advancebuf(d);
      if(d->buf) len += upb_decoder_append(str, d->buf, len, total_len);
      upb_string_getrwbuf(d->str, len);  // Cheap resize.
      if(len < total_len) {
        if(!upb_bytesrc_append(d->bytesrc, d->str, total_len - len)) {
          upb_copyerr(&d->src.status, upb_bytesrc_status(d->bytesrc));
          return false;
        }
      }
    }
    d->field = NULL;
  } else {
    // For all of the integer types we need the bytes to be in a single
    // contiguous buffer.
    uint32_t bytes_available;
    uint32_t bytes_consumed;
    const uint8_t *buf = upb_decoder_getbuf(d, &bytes_available);
    switch(native_wire_type) {
      case UPB_WIRE_TYPE_VARINT:
        if((bytes_consumed = upb_get_v_uint32(buf, val.uint32)) > 10) goto err;
        if(d->field->type == UPB_TYPE(SINT64)) *val.int64 = upb_zzdec_64(*val.int64);
        break;
      case UPB_WIRE_TYPE_32BIT_VARINT:
        if((bytes_consumed = upb_get_v_uint64(buf, val.uint64)) > 5) goto err;
        if(d->field->type == UPB_TYPE(SINT32)) *val.int32 = upb_zzdec_32(*val.int32);
        break;
      case UPB_WIRE_TYPE_64BIT:
        bytes_consumed = 8;
        if(bytes_available < bytes_consumed) goto err;
        upb_get_f_uint64(buf, val.uint64);
        break;
      case UPB_WIRE_TYPE_32BIT:
        bytes_consumed = 4;
        if(bytes_available < bytes_consumed) goto err;
        upb_get_f_uint32(buf, val.uint32);
        break;
      default:
        // Including start/end group.
        goto err;
    }
    d->buf_bytesleft -= bytes_consumed;
    if(d->wire_type != UPB_WIRE_TYPE_DELIMITED ||
       upb_decoder_offset(d) >= d->packed_end_offset) {
      d->field = NULL;
    }
  }
  return true;
err:
  return false;
}

bool upb_decoder_skipval(upb_decoder *d) {
  switch(d->wire_type) {
    case UPB_WIRE_TYPE_VARINT: {
      uint32_t bytes_available;
      const uint8_t *buf = upb_decoder_getbuf(d, &bytes_available);
      uint8_t bytes = upb_skip_v_uint64(buf);
      if(bytes > 10) return false;
      upb_decoder_skipbytes(d,  bytes);
      return true;
    }
    case UPB_WIRE_TYPE_64BIT:
      return upb_decoder_skipbytes(d, 8);
    case UPB_WIRE_TYPE_32BIT:
      return upb_decoder_skipbytes(d, 4);
    case UPB_WIRE_TYPE_START_GROUP:
      return upb_decoder_skipgroup(d);
    case UPB_WIRE_TYPE_DELIMITED:
      // Works for both string/bytes *and* submessages.
      return upb_decoder_skipbytes(d, d->delimited_len);
    default:
      // Including UPB_WIRE_TYPE_END_GROUP.
      assert(false);
      upb_seterr(&d->src.status, UPB_STATUS_ERROR, "Tried to skip an end group");
      return false;
  }
}

bool upb_decoder_startmsg(upb_decoder *d) {
  d->top->field = d->field;
  if(++d->top >= d->limit) {
    upb_seterr(&d->src.status, UPB_ERROR_MAX_NESTING_EXCEEDED,
               "Nesting exceeded maximum (%d levels)\n",
               UPB_MAX_NESTING);
    return false;
  }
  upb_decoder_frame *frame = d->top;
  frame->msgdef = upb_downcast_msgdef(d->field->def);
  if(d->field->type == UPB_TYPE(GROUP)) {
    frame->end_offset = UPB_GROUP_END_OFFSET;
  } else {
    frame->end_offset = upb_decoder_offset(d) + d->delimited_len;
  }
  return true;
}

bool upb_decoder_endmsg(upb_decoder *d) {
  if(d->top > d->stack) {
    --d->top;
    if(!d->src.eof) {
      if(d->top->field->type == UPB_TYPE(GROUP))
        upb_decoder_skipgroup(d);
      else
        upb_decoder_skipbytes(d, d->top->end_offset - upb_decoder_offset(d));
    }
    d->src.eof = false;
    return true;
  } else {
    return false;
  }
}
