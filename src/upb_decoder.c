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

// Performs zig-zag decoding, which is used by sint32 and sint64.
static int32_t upb_zzdec_32(uint32_t n) { return (n >> 1) ^ -(int32_t)(n & 1); }
static int64_t upb_zzdec_64(uint64_t n) { return (n >> 1) ^ -(int64_t)(n & 1); }


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

  // The buffer of input data.  NULL is equivalent to the empty string.
  upb_string *buf;

  // Holds residual bytes when fewer than UPB_MAX_ENCODED_SIZE bytes remain.
  uint8_t tmpbuf[UPB_MAX_ENCODED_SIZE];

  // The number of bytes we have yet to consume from "buf".  This can be
  // negative if we have skipped more bytes than are in the buffer, or if we
  // have started to consume bytes from "nextbuf".
  int32_t buf_bytesleft;
  int32_t buf_offset;

  // The overall stream offset of the end of "buf".  If "buf" is NULL, it is as
  // if "buf" was the empty string.
  uint32_t buf_stream_offset;

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


/* upb_decoder buffering. *****************************************************/

static upb_strlen_t upb_decoder_offset(upb_decoder *d)
{
  return d->buf_stream_offset - d->buf_offset;
}

static bool upb_decoder_nextbuf(upb_decoder *d)
{
  assert(d->buf_bytesleft < UPB_MAX_ENCODED_SIZE);

  // Copy residual bytes to temporary buffer.
  if(d->buf_bytesleft > 0) {
    memcpy(d->tmpbuf, upb_string_getrobuf(d->buf) + d->buf_offset,
           d->buf_bytesleft);
  }

  // Recycle old buffer, pull new one.
  if(d->buf) {
    upb_bytesrc_recycle(d->bytesrc, d->buf);
    d->buf_offset -= upb_string_len(d->buf);
    d->buf_stream_offset += upb_string_len(d->buf);
  }
  d->buf = upb_bytesrc_get(d->bytesrc, UPB_MAX_ENCODED_SIZE);

  // Handle cases arising from error or EOF.
  if(d->buf) {
    d->buf_bytesleft += upb_string_len(d->buf);
  } else {
    if(!upb_bytesrc_eof(d->bytesrc)) {
      // Error from bytesrc.
      upb_copyerr(&d->src.status, upb_bytesrc_status(d->bytesrc));
      return false;
    } else if(d->buf_bytesleft == 0) {
      // EOF from bytesrc and we don't have any residual bytes left.
      d->src.eof = true;
      return false;
    }
  }
  return true;
}

static bool upb_decoder_skipbytes(upb_decoder *d, int32_t bytes)
{
  // TODO.
  return true;
}

static upb_strlen_t upb_decoder_append(uint8_t *buf, upb_string *frombuf,
                                       upb_strlen_t len, upb_strlen_t total_len)
{
  upb_strlen_t copy = UPB_MIN(total_len - len, upb_string_len(frombuf));
  //memcpy(buf, upb_string_getrobuf(frombuf) )
  return 0;
}

static const uint8_t *upb_decoder_getbuf_full(upb_decoder *d, uint32_t *bytes)
{
  if(d->buf_bytesleft < UPB_MAX_ENCODED_SIZE)
    if(!upb_decoder_nextbuf(d)) return NULL;

  assert(d->buf_bytesleft >= UPB_MAX_ENCODED_SIZE);

  if(d->buf_offset >= 0) {
    // Common case: the main buffer contains at least UPB_MAX_ENCODED_SIZE
    // contiguous bytes, so we can read directly out of it.
    *bytes = d->buf_bytesleft;
    return (uint8_t*)upb_string_getrobuf(d->buf) + d->buf_offset;
  } else {
    upb_strlen_t residual_bytes = -d->buf_offset;
    if(d->buf) {
      memcpy(d->tmpbuf + residual_bytes, upb_string_getrobuf(d->buf),
             UPB_MAX_ENCODED_SIZE - residual_bytes);
      *bytes = 10;
    } else {
      // All we have are residual bytes; pad them with 0x80.
      memset(d->tmpbuf + residual_bytes, 0x80,
             UPB_MAX_ENCODED_SIZE - residual_bytes);
      *bytes = residual_bytes;
    }
    return d->tmpbuf;
  }
}

// Returns a pointer to a buffer of data that is at least UPB_MAX_ENCODED_SIZE
// bytes long.  This buffer contains the next bytes in the stream (even if
// those bytes span multiple buffers).  *bytes is set to the number of actual
// stream bytes that are available in the returned buffer.  If
// *bytes < UPB_MAX_ENCODED_SIZE, the buffer is padded with 0x80 bytes.
//
// After the data has been read, upb_decoder_consume() should be called to
// indicate how many bytes were consumed.
static const uint8_t *upb_decoder_getbuf(upb_decoder *d, uint32_t *bytes)
{
  if(d->buf_bytesleft >= UPB_MAX_ENCODED_SIZE && d->buf_offset >= 0) {
    // Common case: the main buffer contains at least UPB_MAX_ENCODED_SIZE
    // contiguous bytes, so we can read directly out of it.
    *bytes = d->buf_bytesleft;
    return (uint8_t*)upb_string_getrobuf(d->buf) + d->buf_offset;
  } else {
    return upb_decoder_getbuf_full(d, bytes);
  }
}

static bool upb_decoder_consume(upb_decoder *d, uint32_t bytes)
{
  assert(bytes <= UPB_MAX_ENCODED_SIZE);
  d->buf_offset += bytes;
  d->buf_bytesleft -= bytes;
  if(d->buf_offset < 0) {
    // We still have residual bytes we have not consumed.
    memmove(d->tmpbuf, d->tmpbuf + bytes, -d->buf_offset);
  }
  return true;
}


/* Functions to read wire values. *********************************************/

// Parses remining bytes of a 64-bit varint that has already had its first byte
// parsed.
INLINE bool upb_decoder_readv64(upb_decoder *d, uint32_t *low, uint32_t *high)
{
  upb_strlen_t bytes_available;
  const uint8_t *buf = upb_decoder_getbuf(d, &bytes_available);
  const uint8_t *start = buf;
  if(!buf) return false;

  *high = 0;
  uint32_t b;
  b = *(buf++); *low   = (b & 0x7f)      ; if(!(b & 0x80)) goto done;
  b = *(buf++); *low  |= (b & 0x7f) <<  7; if(!(b & 0x80)) goto done;
  b = *(buf++); *low  |= (b & 0x7f) << 14; if(!(b & 0x80)) goto done;
  b = *(buf++); *low  |= (b & 0x7f) << 21; if(!(b & 0x80)) goto done;
  b = *(buf++); *low  |= (b & 0x7f) << 28;
                *high  = (b & 0x7f) >>  3; if(!(b & 0x80)) goto done;
  b = *(buf++); *high |= (b & 0x7f) <<  4; if(!(b & 0x80)) goto done;
  b = *(buf++); *high |= (b & 0x7f) << 11; if(!(b & 0x80)) goto done;
  b = *(buf++); *high |= (b & 0x7f) << 18; if(!(b & 0x80)) goto done;
  b = *(buf++); *high |= (b & 0x7f) << 25; if(!(b & 0x80)) goto done;

  if(bytes_available >= 10) {
    upb_seterr(&d->src.status, UPB_STATUS_ERROR, "Varint was unterminated "
               "after 10 bytes, stream offset: %u", upb_decoder_offset(d));
  } else {
    upb_seterr(&d->src.status, UPB_STATUS_ERROR, "Stream ended in the middle "
               "of a varint, stream offset: %u", upb_decoder_offset(d));
  }
  return false;

done:
  return upb_decoder_consume(d, buf - start);
}

// Gets a varint -- called when we only need 32 bits of it.  Note that a 32-bit
// varint is not a true wire type.
static bool upb_decoder_readv32(upb_decoder *d, uint32_t *val)
{
  uint32_t high;
  if(!upb_decoder_readv64(d, val, &high)) return false;

  // We expect the high bits to be zero, except that signed 32-bit values are
  // first sign-extended to be wire-compatible with 64 bits, in which case we
  // expect the high bits to be all one.
  //
  // We could perform a slightly more sophisticated check by having the caller
  // indicate whether a signed or unsigned value is being read.  We could check
  // that the high bits are all zeros for unsigned, and properly sign-extended
  // for signed.
  if(high != 0 && ~high != 0) {
    upb_seterr(&d->src.status, UPB_STATUS_ERROR, "Read a 32-bit varint, but "
               "the high bits contained data we should not truncate: "
               "%ux, stream offset: %u", high, upb_decoder_offset(d));
    return false;
  }
  return true;
}

// Gets a fixed-length 32-bit integer (wire type: UPB_WIRE_TYPE_32BIT).  Caller
// promises that 4 bytes are available at buf.
static bool upb_decoder_readf32(upb_decoder *d, uint32_t *val)
{
  upb_strlen_t bytes_available;
  const uint8_t *buf = upb_decoder_getbuf(d, &bytes_available);
  if(!buf) return false;
  if(bytes_available < 4) {
    upb_seterr(&d->src.status, UPB_STATUS_ERROR,
               "Stream ended in the middle of a 32-bit value");
    return false;
  }
  memcpy(val, buf, 4);
  // TODO: byte swap if big-endian.
  return upb_decoder_consume(d, 4);
}

// Gets a fixed-length 64-bit integer (wire type: UPB_WIRE_TYPE_64BIT).  Caller
// promises that 8 bytes are available at buf.
static bool upb_decoder_readf64(upb_decoder *d, uint64_t *val)
{
  upb_strlen_t bytes_available;
  const uint8_t *buf = upb_decoder_getbuf(d, &bytes_available);
  if(!buf) return false;
  if(bytes_available < 8) {
    upb_seterr(&d->src.status, UPB_STATUS_ERROR,
               "Stream ended in the middle of a 64-bit value");
    return false;
  }
  memcpy(val, buf, 8);
  // TODO: byte swap if big-endian.
  return upb_decoder_consume(d, 8);
}

// Returns the length of a varint (wire type: UPB_WIRE_TYPE_VARINT), allowing
// it to be easily skipped.  Caller promises that 10 bytes are available at
// "buf".  The function will return a maximum of 11 bytes before quitting.
static uint8_t upb_decoder_skipv64(upb_decoder *d)
{
  uint32_t bytes_available;
  const uint8_t *buf = upb_decoder_getbuf(d, &bytes_available);
  if(!buf) return false;
  uint8_t i;
  for(i = 0; i < 10 && buf[i] & 0x80; i++)
    ;  // empty loop body.
  if(i > 10) {
    upb_seterr(&d->src.status, UPB_STATUS_ERROR, "Unterminated varint.");
    return false;
  }
  return upb_decoder_consume(d, i);
}


/* upb_src implementation for upb_decoder. ************************************/

bool upb_decoder_skipval(upb_decoder *d);

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
  if(!upb_decoder_readv32(d, &key)) return NULL;
  wire_type = key & 0x7;

  if(wire_type == UPB_WIRE_TYPE_DELIMITED) {
    // For delimited wire values we parse the length now, since we need it in
    // all cases.
    if(!upb_decoder_readv32(d, &d->delimited_len)) return NULL;
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
      //// The string spans buffers, so we must copy from the current buffer,
      //// the next buffer (if we have one), and finally from the bytesrc.
      //uint8_t *str = (uint8_t*)upb_string_getrwbuf(d->str, total_len);
      //upb_strlen_t len = 0;
      //len += upb_decoder_append(str, d->buf, len, total_len);
      //upb_decoder_advancebuf(d);
      //if(d->buf) len += upb_decoder_append(str, d->buf, len, total_len);
      //upb_string_getrwbuf(d->str, len);  // Cheap resize.
      //if(len < total_len) {
      //  if(!upb_bytesrc_append(d->bytesrc, d->str, total_len - len)) {
      //    upb_copyerr(&d->src.status, upb_bytesrc_status(d->bytesrc));
      //    return false;
      //  }
      //}
    }
    d->field = NULL;
  } else {
    switch(native_wire_type) {
      case UPB_WIRE_TYPE_VARINT: {
        uint32_t low, high;
        if(!upb_decoder_readv64(d, &low, &high)) return false;
        uint64_t u64 = ((uint64_t)high << 32) | low;
        if(d->field->type == UPB_TYPE(SINT64))
          *val.int64 = upb_zzdec_64(u64);
        else
          *val.uint64 = u64;
        break;
      }
      case UPB_WIRE_TYPE_32BIT_VARINT: {
        uint32_t u32;
        if(!upb_decoder_readv32(d, &u32)) return false;
        if(d->field->type == UPB_TYPE(SINT32))
          *val.int32 = upb_zzdec_32(u32);
        else
          *val.uint32 = u32;
        break;
      }
      case UPB_WIRE_TYPE_64BIT:
        if(!upb_decoder_readf64(d, val.uint64)) return false;
        break;
      case UPB_WIRE_TYPE_32BIT:
        if(!upb_decoder_readf32(d, val.uint32)) return false;
        break;
      default:
        upb_seterr(&d->src.status, UPB_STATUS_ERROR,
                   "Attempted to call getval on a group.");
        return false;
    }
    // For a packed field where we have not reached the end, we leave the field
    // in the decoder so we will return it again without parsing a key.
    if(d->wire_type != UPB_WIRE_TYPE_DELIMITED ||
       upb_decoder_offset(d) >= d->packed_end_offset) {
      d->field = NULL;
    }
  }
  return true;
}

static bool upb_decoder_skipgroup(upb_decoder *d);

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

bool upb_decoder_skipval(upb_decoder *d) {
  upb_strlen_t bytes_to_skip;
  switch(d->wire_type) {
    case UPB_WIRE_TYPE_VARINT: {
      return upb_decoder_skipv64(d);
    }
    case UPB_WIRE_TYPE_START_GROUP:
      if(!upb_decoder_startmsg(d)) return false;
      if(!upb_decoder_skipgroup(d)) return false;
      if(!upb_decoder_endmsg(d)) return false;
      return true;
    default:
      // Including UPB_WIRE_TYPE_END_GROUP.
      assert(false);
      upb_seterr(&d->src.status, UPB_STATUS_ERROR, "Tried to skip an end group");
      return false;
    case UPB_WIRE_TYPE_64BIT:
      bytes_to_skip = 8;
      break;
    case UPB_WIRE_TYPE_32BIT:
      bytes_to_skip = 4;
      break;
    case UPB_WIRE_TYPE_DELIMITED:
      // Works for both string/bytes *and* submessages.
      bytes_to_skip = d->delimited_len;
      break;
  }
  return upb_decoder_skipbytes(d, bytes_to_skip);
}

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
  if(!&d->src.eof) return false;
  return true;
}

upb_src_vtable upb_decoder_src_vtbl = {
  (upb_src_getdef_fptr)&upb_decoder_getdef,
  (upb_src_getval_fptr)&upb_decoder_getval,
  (upb_src_skipval_fptr)&upb_decoder_skipval,
  (upb_src_startmsg_fptr)&upb_decoder_startmsg,
  (upb_src_endmsg_fptr)&upb_decoder_endmsg,
};


/* upb_decoder construction/destruction. **************************************/

upb_decoder *upb_decoder_new(upb_msgdef *msgdef)
{
  upb_decoder *d = malloc(sizeof(*d));
  d->toplevel_msgdef = msgdef;
  d->limit = &d->stack[UPB_MAX_NESTING];
  d->buf = NULL;
  d->str = upb_string_new();
  upb_src_init(&d->src, &upb_decoder_src_vtbl);
  return d;
}

void upb_decoder_free(upb_decoder *d)
{
  free(d);
}

void upb_decoder_reset(upb_decoder *d, upb_bytesrc *bytesrc)
{
  if(d->buf) upb_bytesrc_recycle(d->bytesrc, d->buf);
  d->top = d->stack;
  d->top->msgdef = d->toplevel_msgdef;
  // The top-level message is not delimited (we can keep receiving data for it
  // indefinitely), so we set the end offset as high as possible, but not equal
  // to UINT32_MAX so it doesn't equal UPB_GROUP_END_OFFSET.
  d->top->end_offset = UINT32_MAX - 1;
  d->bytesrc = bytesrc;
  d->buf = NULL;
  d->buf_bytesleft = 0;
  d->buf_stream_offset = 0;
  d->buf_offset = 0;
}
