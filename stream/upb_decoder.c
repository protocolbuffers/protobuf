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

/* Pure Decoding **************************************************************/

// The key fast-path varint-decoding routine.  Here we can assume we have at
// least UPB_MAX_ENCODED_SIZE bytes available.  There are a lot of
// possibilities for optimization/experimentation here.
INLINE bool upb_decode_varint_fast(uint8_t **ptr, uint64_t &val,
                                   upb_status *status) {
  *high = 0;
  uint32_t b;
  uint8_t *ptr = p->ptr;
  b = *(*ptr++); *low   = (b & 0x7f)      ; if(!(b & 0x80)) goto done;
  b = *(*ptr++); *low  |= (b & 0x7f) <<  7; if(!(b & 0x80)) goto done;
  b = *(*ptr++); *low  |= (b & 0x7f) << 14; if(!(b & 0x80)) goto done;
  b = *(*ptr++); *low  |= (b & 0x7f) << 21; if(!(b & 0x80)) goto done;
  b = *(*ptr++); *low  |= (b & 0x7f) << 28;
                 *high  = (b & 0x7f) >>  3; if(!(b & 0x80)) goto done;
  b = *(*ptr++); *high |= (b & 0x7f) <<  4; if(!(b & 0x80)) goto done;
  b = *(*ptr++); *high |= (b & 0x7f) << 11; if(!(b & 0x80)) goto done;
  b = *(*ptr++); *high |= (b & 0x7f) << 18; if(!(b & 0x80)) goto done;
  b = *(*ptr++); *high |= (b & 0x7f) << 25; if(!(b & 0x80)) goto done;

  upb_seterr(status, UPB_ERROR, "Unterminated varint");
  return false;
done:
  return true;
}


/* Decoding/Buffering of individual values ************************************/

// Performs zig-zag decoding, which is used by sint32 and sint64.
INLINE int32_t upb_zzdec_32(uint32_t n) { return (n >> 1) ^ -(int32_t)(n & 1); }
INLINE int64_t upb_zzdec_64(uint64_t n) { return (n >> 1) ^ -(int64_t)(n & 1); }

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
  upb_bytesrc *bytesrc;
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
};

// Called only from the slow path, this function copies the next "len" bytes
// from the stream to "data", adjusting "buf" and "len" appropriately.
static bool upb_getbuf(upb_decoder *d, void *data, size_t bytes_wanted,
                       uint8_t **ptr, size_t *len) {
  while (1) {
    memcpy(data, *ptr, *len);
    bytes_wanted -= *len;
    *ptr += *len;
    if (bytes_wanted == 0) return true;

    // Did "len" indicate end-of-submessage or end-of-buffer?
    size_t buf_offset = d->buf ? (*ptr - upb_string_getrobuf(d->buf)) : 0;
    if (d->top->end_offset > 0 &&
        d->top->end_offset == d->buf_stream_offset + buf_offset) {
      // End-of-submessage.
      if (bytes_wanted > 0) {
        upb_seterr(d->status, UPB_ERROR, "Bad submessage end.")
        return false;
      }
      if (upb_pop(d) != UPB_CONTINUE) return false;
    } else {
      // End-of-buffer.
      if (d->buf) d->buf_stream_offset += upb_string_len(d->buf);
      if (!upb_bytesrc_getstr(d->bytesrc, d->buf, d->status)) return false;
      *ptr = upb_string_getrobuf(d->buf);
    }

    // Wait for end-of-submessage or end-of-buffer, whichever comes first.
    size_t offset_in_buf = *ptr - upb_string_getrobuf(d->buf);
    size_t buf_remaining = upb_string_getbufend(d->buf) - *ptr;
    size_t submsg_remaining =
        d->top->end_offset - d->buf_stream_offset - offset_in_buf;
    if (d->top->end_offset == UPB_GROUP_END_OFFSET ||
        buf_remaining > submsg_remaining) {
      *len = buf_remaining;
    } else {
      // Check that non of our subtraction overflowed.
      assert(d->top->end_offset > d->buf_stream_offset);
      assert(d->top->end_offset - d->buf_stream_offset > offset_in_buf);
      *len = submsg_remaining;
    }
  }
}

// We use this path when we don't have UPB_MAX_ENCODED_SIZE contiguous bytes
// available in our current buffer.  We don't inline this because we accept
// that it will be slow and we don't want to pay for two copies of it.
static bool upb_decode_varint_slow(upb_decoder *d) {
  uint8_t buf[UPB_MAX_ENCODED_SIZE];
  uint8_t *p = buf, *end = buf + sizeof(buf);
  for(int bitpos = 0; p < end && getbyte(d, p) && (last & 0x80); p++, bitpos += 7)
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
}

INLINE bool upb_decode_tag(upb_decoder *d, const uint8_t **_ptr,
                           const uint8_t **len, upb_tag *tag) {
  const uint8_t *ptr = *_ptr, *len = *_end;
  uint32_t tag_int;
  // Nearly all tag varints will be either 1 byte (1-16) or 2 bytes (17-2048).
  if (len - ptr < 2) goto slow;  // unlikely.
  tag_int = *ptr & 0x7f;
  if ((*(ptr++) & 0x80) == 0) goto done;  // predictable if fields are in order
  tag_int |= (*ptr & 0x7f) << 7;
  if ((*(ptr++) & 0x80) != 0) goto slow;  // unlikely.
slow:
  if (!upb_decode_varint_slow(d, _ptr, _end)) return false;
  ptr = *_ptr;  // Trick the next line into not overwriting us.
done:
  *_ptr = ptr;
  tag->wire_type = (upb_wire_type_t)(tag_int & 0x07);
  tag->field_number = tag_int >> 3;
  return true;
}

INLINE bool upb_decode_varint(upb_decoder *d, ptrs *p,
                              uint32_t *low, uint32_t *high) {
  if (p->len - p->ptr >= UPB_MAX_VARINT_ENCODED_SIZE)
    return upb_decode_varint_fast(d);
  else
    return upb_decode_varint_slow(d);
}

INLINE bool upb_decode_fixed(upb_decoder *d, upb_wire_type_t wt,
                             uint8_t **ptr, uint8_t **len, upb_value *val) {
  static const char table = {0, 8, 0, 0, 0, 4};
  size_t bytes = table[wt];
  if (*len - *ptr >= bytes) {
    // Common (fast) case.
    memcpy(&val, *ptr, bytes);
    *ptr += bytes;
  } else {
    if (!upb_getptr(d, &val, bytes, ptr, len)) return false;
  }
  return true;
}

// "val" initially holds the length of the string, this is replaced by the
// contents of the string.
INLINE bool upb_decode_string(upb_decoder *d, upb_value *val, upb_string **str) {
  upb_string_recycle(str);
  upb_strlen_t len = upb_valu_getint32(*val);
  if (*len - *ptr >= len) {
    // Common (fast) case.
    upb_string_substr(*str, d->buf, *ptr - upb_string_getrobuf(d->buf), len);
    *ptr += len;
  } else {
    if (!upb_getbuf(d, upb_string_getrwbuf(*str, len), len, ptr, len))
      return false;
  }
  return true;
}


/* The main decoding loop *****************************************************/

extern upb_wire_type_t upb_expected_wire_types[];
// Returns true if wt is the correct on-the-wire type for ft.
INLINE bool upb_check_type(upb_wire_type_t wt, upb_field_type_t ft) {
  // This doesn't currently support packed arrays.
  return upb_types[ft].expected_wire_type == wt;
}

static upb_flow_t upb_push(upb_decoder *d, upb_fielddef *f,
                           upb_strlen_t submsg_len, upb_field_type_t type) {
  d->top->field = f;
  d->top++;
  if(d->top >= d->limit) {
    upb_seterr(status, UPB_ERROR, "Nesting too deep.");
    return UPB_ERROR;
  }
  d->top->end_offset = type == UPB_TYPE(GROUP) ?
      UPB_GROUP_END_OFFSET : d->completed_offset + submsg_len;
  d->top->msgdef = upb_downcast_msgdef(f->def);
  return upb_dispatch_startsubmsg(&d->dispatcher, f);
}

static upb_flow_t upb_pop(upb_decoder *d) {
  d->top--;
  return upb_dispatch_endsubmsg(&d->dispatcher);
}

void upb_decoder_run(upb_src *src, upb_status *status) {
  // We use stack variables for our frequently used vars so the compiler knows
  // they can't be changed by external code (like when we dispatch a callback).

  // Our current position in the data buffer.
  uint8_t *ptr = NULL;
  // Number of bytes available at ptr, until either end-of-buf or
  // end-of-submessage (whichever is smaller).
  size_t len = 0;

  upb_string *str = NULL;

// TODO: handle UPB_SKIPSUBMSG
#define CHECK_FLOW(expr) if ((expr) != UPB_CONTINUE) goto err
#define CHECK(expr) if (!expr) goto err;

  CHECK_FLOW(upb_dispatch_startmsg(&d->dispatcher));

  // Main loop: executed once per tag/field pair.
  while(1) {
    // Parse/handle tag.
    upb_tag tag;
    CHECK(upb_decode_tag(d, &ptr, &len, &tag));

    // Decode wire data.  Hopefully this branch will predict pretty well
    // since most types will read a varint here.
    upb_value val;
    switch (tag.wire_type) {
      case UPB_WIRE_TYPE_END_GROUP:
        if(d->top->end_offset != UPB_GROUP_END_OFFSET)
          upb_seterr(status, UPB_ERROR, "Unexpected END_GROUP tag.");
          goto err;
        }
        CHECK_FLOW(upb_pop(d));
        continue;  // We have no value to dispatch.
      case UPB_WIRE_TYPE_VARINT:
      case UPB_WIRE_TYPE_DELIMITED:
        // For the delimited case we are parsing the length.
        CHECK(upb_decode_varint(d, &ptr, &len, &val));
        break;
      case UPB_WIRE_TYPE_32BIT:
      case UPB_WIRE_TYPE_64BIT:
        CHECK(upb_decode_fixed(d, tag.wire_type, &ptr, &len, &val));
        break;
    }

    // Look up field by tag number.
    upb_fielddef *f = upb_msg_itof(d->top->msgdef, tag.field_number);

    if (!f) {
      if (tag.wire_type == UPB_WIRE_TYPE_DELIMITED)
        CHECK(upb_decode_string(d, &val, &str));
      CHECK_FLOW(upb_dispatch_unknownval(d, tag.field_number, val));
    } else if (!upb_check_type(tag.wire_type, f->type)) {
      // TODO: put more details in this error msg.
      upb_seterr(status, UPB_ERROR, "Field had incorrect type.");
      goto err;
    }

    // Perform any further massaging of the data now that we have the fielddef.
    // Now we can distinguish strings from submessages, and we know about
    // zig-zag-encoded types.
    // TODO: handle packed encoding.
    // TODO: if we were being paranoid, we could check for 32-bit-varint types
    // that the top 32 bits all match the highest bit of the low 32 bits.
    // If this is not true we are losing data.  But the main protobuf library
    // doesn't check this, and it would slow us down, so pass for now.
    switch (f->type) {
      case UPB_TYPE(MESSAGE):
      case UPB_TYPE(GROUP):
        CHECK_FLOW(upb_push(d, start, upb_value_getint32(val), f, status, &msgdef));
        continue;  // We have no value to dispatch.
      case UPB_TYPE(STRING):
      case UPB_TYPE(BYTES):
        CHECK(upb_decode_string(d, &val, &str));
        break;
      case UPB_TYPE(SINT32):
        upb_value_setint32(&val, upb_zzdec_32(upb_value_getint32(val)));
        break;
      case UPB_TYPE(SINT64):
        upb_value_setint64(&val, upb_zzdec_64(upb_value_getint64(val)));
        break;
      default:
        break;  // Other types need no further processing at this point.
    }
    CHECK_FLOW(upb_dispatch_value(d->sink, f, val, status));
  }

  CHECK_FLOW(upb_dispatch_endmsg(&d->dispatcher));
  return;

err:
  if (upb_ok(status)) {
    upb_seterr(status, UPB_ERROR, "Callback returned UPB_BREAK");
  }
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
