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

const uint8_t *upb_get_v_uint64_t_full(const uint8_t *buf, const uint8_t *end,
                                       uint64_t *val, upb_status *status);

// Gets a varint (wire type: UPB_WIRE_TYPE_VARINT).
INLINE const uint8_t *upb_get_v_uint64_t(const uint8_t *buf, const uint8_t *end,
                                         uint64_t *val, upb_status *status)
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

// Gets a varint -- called when we only need 32 bits of it.  Note that a 32-bit
// varint is not a true wire type.
INLINE const uint8_t *upb_get_v_uint32_t(const uint8_t *buf, const uint8_t *end,
                                         uint32_t *val, upb_status *status)
{
  uint64_t val64;
  const uint8_t *ret = upb_get_v_uint64_t(buf, end, &val64, status);
  *val = (uint32_t)val64;  // Discard the high bits.
  return ret;
}

// Gets a fixed-length 32-bit integer (wire type: UPB_WIRE_TYPE_32BIT).
INLINE const uint8_t *upb_get_f_uint32_t(const uint8_t *buf, const uint8_t *end,
                                         uint32_t *val, upb_status *status)
{
  const uint8_t *uint32_end = buf + sizeof(uint32_t);
  if(uint32_end > end) {
    status->code = UPB_STATUS_NEED_MORE_DATA;
    return end;
  }
  memcpy(val, buf, sizeof(uint32_t));
  return uint32_end;
}

// Gets a fixed-length 64-bit integer (wire type: UPB_WIRE_TYPE_64BIT).
INLINE const uint8_t *upb_get_f_uint64_t(const uint8_t *buf, const uint8_t *end,
                                         uint64_t *val, upb_status *status)
{
  const uint8_t *uint64_end = buf + sizeof(uint64_t);
  if(uint64_end > end) {
    status->code = UPB_STATUS_NEED_MORE_DATA;
    return end;
  }
  memcpy(val, buf, sizeof(uint64_t));
  return uint64_end;
}

INLINE const uint8_t *upb_skip_v_uint64_t(const uint8_t *buf,
                                          const uint8_t *end,
                                          upb_status *status)
{
  const uint8_t *const maxend = buf + 10;
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

INLINE const uint8_t *upb_skip_f_uint32_t(const uint8_t *buf,
                                          const uint8_t *end,
                                          upb_status *status)
{
  const uint8_t *uint32_end = buf + sizeof(uint32_t);
  if(uint32_end > end) {
    status->code = UPB_STATUS_NEED_MORE_DATA;
    return end;
  }
  return uint32_end;
}

INLINE const uint8_t *upb_skip_f_uint64_t(const uint8_t *buf,
                                          const uint8_t *end,
                                          upb_status *status)
{
  const uint8_t *uint64_end = buf + sizeof(uint64_t);
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

// Parses a tag, places the result in *tag.
INLINE const uint8_t *decode_tag(const uint8_t *buf, const uint8_t *end,
                                 upb_tag *tag, upb_status *status)
{
  uint32_t tag_int;
  const uint8_t *ret = upb_get_v_uint32_t(buf, end, &tag_int, status);
  tag->wire_type    = (upb_wire_type_t)(tag_int & 0x07);
  tag->field_number = tag_int >> 3;
  return ret;
}

// The decoder keeps a stack with one entry per level of recursion.
// upb_decoder_frame is one frame of that stack.
typedef struct {
  upb_msgdef *msgdef;
  upb_fielddef *field;
  size_t end_offset;  // For groups, 0.
} upb_decoder_frame;

struct upb_decoder {
  // Immutable state of the decoder.
  upb_src src;
  upb_dispatcher dispatcher;
  upb_msgdef *toplevel_msgdef;
  upb_decoder_frame stack[UPB_MAX_NESTING];

  // Mutable state of the decoder.

  // Where we will store any errors that occur.
  upb_status *status;

  // Stack entries store the offset where the submsg ends (for groups, 0).
  upb_decoder_frame *top, *limit;

  // Current input buffer.
  upb_string *buf;

  // The offset within the overall stream represented by the *beginning* of buf.
  upb_strlen_t buf_stream_offset;

  // Our current offset *within* buf.  Will be negative if we are buffering
  // from previous buffers in tmpbuf.
  upb_strlen_t buf_offset;

  // Holds any bytes we have from previous buffers.  The number of bytes we
  // have encoded here is -buf_offset, if buf_offset<0, 0 otherwise.
  uint8_t tmpbuf[UPB_MAX_ENCODED_SIZE];
};

upb_flow_t upb_decode_varint(upb_decoder *d, ptrs *p,
                             uint32_t *low, uint32_t *high) {
  if (p->end - p->ptr > UPB_MAX_ENCODED_SIZE) {
    // Fast path; we know we have a complete varint in our existing buffer.
    *high = 0;
    uint32_t b;
    uint8_t *ptr = p->ptr;
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
      return false;
    }

  done:
    p->ptr = ptr;
  } else {
    // Slow path: we may have to combine one or more buffers to get a whole
    // varint worth of data.
    uint8_t buf[UPB_MAX_ENCODED_SIZE];
    uint8_t *p = buf, *end = buf + sizeof(buf);
    for(ing bitpos = 0; p < end && getbyte(d, p) && (last & 0x80); p++, bitpos += 7)
      *val |= ((uint64_t)((last = *p) & 0x7F)) << bitpos;

    if(d->status->code == UPB_EOF  && (last & 0x80)) {
      upb_seterr(status, UPB_ERROR,
                 "Provided data ended in the middle of a varint.\n");
    } else if(buf == maxend) {
      upb_seterr(status, UPB_ERROR,
                 "Varint was unterminated after 10 bytes.\n");
    } else {
      // Success.
      return;
    }
    ungetbytes(d, buf, p - buf);
  }
}

static const void *get_msgend(upb_decoder *d)
{
  if(d->top->end_offset > 0)
    return upb_string_getrobuf(d->buf) + (d->top->end_offset - d->buf_stream_offset);
  else
    return (void*)UINTPTR_MAX;  // group.
}

static bool isgroup(const void *submsg_end)
{
  return submsg_end == (void*)UINTPTR_MAX;
}

extern upb_wire_type_t upb_expected_wire_types[];
// Returns true if wt is the correct on-the-wire type for ft.
INLINE bool upb_check_type(upb_wire_type_t wt, upb_field_type_t ft) {
  // This doesn't currently support packed arrays.
  return upb_types[ft].expected_wire_type == wt;
}


// Pushes a new stack frame for a submessage with the given len (which will
// be zero if the submessage is a group).
static const uint8_t *push(upb_decoder *d, const uint8_t *start,
                           uint32_t submsg_len, upb_fielddef *f,
                           upb_status *status)
{
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

  upb_dispatch_startsubmsg(&d->dispatcher, f);
  return get_msgend(d);
}

// Pops a stack frame, returning a pointer for where the next submsg should
// end (or a pointer that is out of range for a group).
static const void *pop(upb_decoder *d, const uint8_t *start, upb_status *status)
{
  d->top--;
  upb_dispatch_endsubmsg(&d->dispatcher);
  return get_msgend(d);
}

void upb_decoder_run(upb_src *src, upb_status *status) {
  // buf is our current offset, moves from start to end.
  const uint8_t *buf = (uint8_t*)upb_string_getrobuf(str) + d->buf_offset;
  const uint8_t *end = (uint8_t*)upb_string_getrobuf(str) + upb_string_len(str);
  const uint8_t *submsg_end = get_msgend(d, start);
  upb_msgdef *msgdef = d->top->msgdef;
  upb_string *str = NULL;

  // Main loop: executed once per tag/field pair.
  while(1) {
    // Parse/handle tag.
    upb_tag tag;
    CHECK(decode_tag(d, &buf, &end, &tag));

    // Decode wire data.  Hopefully this branch will predict pretty well
    // since most types will read a varint here.
    upb_value val;
    switch (tag.wire_type) {
      case UPB_WIRE_TYPE_END_GROUP:
        if(!isgroup(submsg_end)) {
          upb_seterr(status, UPB_STATUS_ERROR, "End group seen but current "
                     "message is not a group, byte offset: %zd",
                     d->completed_offset + (completed - start));
          goto err;
        }
        submsg_end = pop(d, start, status, &msgdef);
        completed = buf;
        goto check_msgend;
      case UPB_WIRE_TYPE_VARINT:
      case UPB_WIRE_TYPE_DELIMITED:
        // For the delimited case we are parsing the length.
        CHECK(upb_decode_varint(d, &buf, &end, &val));
        break;
      case UPB_WIRE_TYPE_32BIT:
        CHECK(upb_decode_32bit(d, &buf, &end, &val));
        break;
      case UPB_WIRE_TYPE_64BIT:
        CHECK(upb_decode_64bit(d, &buf, &end, &val));
        break;
    }

    // Look up field by tag number.
    upb_fielddef *f = upb_msg_itof(msgdef, tag.field_number);

    if (!f) {
      // Unknown field.
    } else if (!upb_check_type(tag.wire_type, f->type)) {
      // Field has incorrect type.
    }

    // Perform any further massaging of the data now that we have the fielddef.
    // Now we can distinguish strings from submessages, and we know about
    // zig-zag-encoded types.
    // TODO: handle packed encoding.
    switch (f->type) {
      case UPB_TYPE(MESSAGE):
      case UPB_TYPE(GROUP):
        CHECK(push(d, start, upb_value_getint32(val), f, status, &msgdef));
        goto check_msgend;
      case UPB_TYPE(STRING):
      case UPB_TYPE(BYTES):
        CHECK(upb_decode_string(d, str, upb_value_getint32(val)));
        upb_value_setstr(&val, str);
        break;
      case UPB_TYPE(SINT32):
        upb_value_setint32(&val, upb_zzdec_32(upb_value_getint32(val)));
        break;
      case UPB_TYPE(SINT64):
        upb_value_setint64(&val, upb_zzdec_64(upb_value_getint64(val)));
        break;
      default:
        // Other types need no further processing at this point.
    }
    CHECK(upb_dispatch_value(d->sink, f, val, status));

check_msgend:
    while(buf >= submsg_end) {
      if(buf > submsg_end) {
        upb_seterr(status, UPB_ERROR, "Expected submsg end offset "
                   "did not lie on a tag/value boundary.");
        goto err;
      }
      submsg_end = pop(d, start, status, &msgdef);
    }
    completed = buf;
  }

err:
  read = (char*)completed - (char*)start;
  d->completed_offset += read;
  return read;
}

void upb_decoder_sethandlers(upb_src *src, upb_handlers *handlers) {
  upb_decoder *d = (upb_decoder*)src;
  upb_dispatcher_reset(&d->dispatcher, handlers);
  d->top = d->stack;
  d->completed_offset = 0;
  d->top->msgdef = d->toplevel_msgdef;
  // The top-level message is not delimited (we can keep receiving data for it
  // indefinitely), so we treat it like a group.
  d->top->end_offset = 0;
}

upb_decoder *upb_decoder_new(upb_msgdef *msgdef) {
  static upb_src_vtbl vtbl = {
    &upb_decoder_sethandlers,
    &upb_decoder_run,
  };
  upb_decoder *d = malloc(sizeof(*d));
  upb_src_init(&d->src, &vtbl);
  upb_dispatcher_init(&d->dispatcher);
  d->toplevel_msgdef = msgdef;
  d->limit = &d->stack[UPB_MAX_NESTING];
  return d;
}

void upb_decoder_free(upb_decoder *d) {
  free(d);
}
