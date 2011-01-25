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
// least UPB_MAX_VARINT_ENCODED_SIZE bytes available.  There are a lot of
// possibilities for optimization/experimentation here.
INLINE bool upb_decode_varint_fast(const char **ptr, uint64_t *val,
                                   upb_status *status) {
  uint32_t low, high = 0;
  uint32_t b;
  b = *(*ptr++); low   = (b & 0x7f)      ; if(!(b & 0x80)) goto done;
  b = *(*ptr++); low  |= (b & 0x7f) <<  7; if(!(b & 0x80)) goto done;
  b = *(*ptr++); low  |= (b & 0x7f) << 14; if(!(b & 0x80)) goto done;
  b = *(*ptr++); low  |= (b & 0x7f) << 21; if(!(b & 0x80)) goto done;
  b = *(*ptr++); low  |= (b & 0x7f) << 28;
                 high  = (b & 0x7f) >>  3; if(!(b & 0x80)) goto done;
  b = *(*ptr++); high |= (b & 0x7f) <<  4; if(!(b & 0x80)) goto done;
  b = *(*ptr++); high |= (b & 0x7f) << 11; if(!(b & 0x80)) goto done;
  b = *(*ptr++); high |= (b & 0x7f) << 18; if(!(b & 0x80)) goto done;
  b = *(*ptr++); high |= (b & 0x7f) << 25; if(!(b & 0x80)) goto done;

  upb_seterr(status, UPB_ERROR, "Unterminated varint");
  return false;
done:
  *val = ((uint64_t)high << 32) | low;
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
  ssize_t end_offset;  // For groups, 0.
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

typedef struct {
  // Our current position in the data buffer.
  const char *ptr;

  // Number of bytes available at ptr, until either end-of-buf or
  // end-of-submessage (whichever is smaller).
  size_t len;

  // Msgdef for the current level.
  upb_msgdef *msgdef;
} upb_dstate;

INLINE void upb_dstate_advance(upb_dstate *s, size_t len) {
  s->ptr += len;
  s->len -= len;
}

static upb_flow_t upb_pop(upb_decoder *d);

// Constant used to signal that the submessage is a group and therefore we
// don't know its end offset.  This cannot be the offset of a real submessage
// end because it takes at least one byte to begin a submessage.
#define UPB_GROUP_END_OFFSET -1
#define UPB_MAX_VARINT_ENCODED_SIZE 10

// Called only from the slow path, this function copies the next "len" bytes
// from the stream to "data", adjusting "buf" and "len" appropriately.
static bool upb_getbuf(upb_decoder *d, void *data, size_t bytes_wanted,
                       upb_dstate *s) {
  while (1) {
    size_t to_copy = UPB_MIN(bytes_wanted, s->len);
    memcpy(data, s->ptr, to_copy);
    upb_dstate_advance(s, to_copy);
    bytes_wanted -= to_copy;
    if (bytes_wanted == 0) return true;

    // Did "len" indicate end-of-submessage or end-of-buffer?
    ssize_t buf_offset =
        d->buf ? ((const char*)s->ptr - upb_string_getrobuf(d->buf)) : 0;
    if (d->top->end_offset > 0 &&
        d->top->end_offset == d->buf_stream_offset + buf_offset) {
      // End-of-submessage.
      if (bytes_wanted > 0) {
        upb_seterr(d->status, UPB_ERROR, "Bad submessage end.");
        return false;
      }
      if (upb_pop(d) != UPB_CONTINUE) return false;
    } else {
      // End-of-buffer.
      if (d->buf) d->buf_stream_offset += upb_string_len(d->buf);
      upb_string_recycle(&d->buf);
      if (!upb_bytesrc_getstr(d->bytesrc, d->buf, d->status)) return false;
      s->ptr = upb_string_getrobuf(d->buf);
    }

    // Wait for end-of-submessage or end-of-buffer, whichever comes first.
    ssize_t offset_in_buf = s->ptr - upb_string_getrobuf(d->buf);
    ssize_t buf_remaining = upb_string_getbufend(d->buf) - s->ptr;
    ssize_t submsg_remaining =
        d->top->end_offset - d->buf_stream_offset - offset_in_buf;
    if (d->top->end_offset == UPB_GROUP_END_OFFSET ||
        buf_remaining > submsg_remaining) {
      s->len = buf_remaining;
    } else {
      // Check that non of our subtraction overflowed.
      assert(d->top->end_offset > d->buf_stream_offset);
      assert(d->top->end_offset - d->buf_stream_offset > offset_in_buf);
      s->len = submsg_remaining;
    }
  }
}

// We use this path when we don't have UPB_MAX_VARINT_ENCODED_SIZE contiguous
// bytes available in our current buffer.  We don't inline this because we
// accept that it will be slow and we don't want to pay for two copies of it.
static bool upb_decode_varint_slow(upb_decoder *d, upb_dstate *s,
                                   upb_value *val) {
  char byte = 0x80;
  uint64_t val64 = 0;
  int bitpos;
  for(bitpos = 0;
      bitpos < 70 && (byte & 0x80) && upb_getbuf(d, &byte, 1, s);
      bitpos += 7)
    val64 |= ((uint64_t)byte & 0x7F) << bitpos;

  if(bitpos == 70) {
    upb_seterr(d->status, UPB_ERROR,
               "Varint was unterminated after 10 bytes.\n");
    return false;
  } else if (d->status->code == UPB_EOF  && (byte & 0x80)) {
    upb_seterr(d->status, UPB_ERROR,
               "Provided data ended in the middle of a varint.\n");
    return false;
  } else {
    // Success.
    upb_value_setint64(val, val64);
    return true;
  }
}

typedef struct {
  upb_wire_type_t wire_type;
  upb_field_number_t field_number;
} upb_tag;

INLINE bool upb_decode_tag(upb_decoder *d, upb_dstate *s, upb_tag *tag) {
  const char *p = s->ptr;
  uint32_t tag_int;
  upb_value val;
  // Nearly all tag varints will be either 1 byte (1-16) or 2 bytes (17-2048).
  if (s->len < 2) goto slow;  // unlikely.
  tag_int = *p & 0x7f;
  if ((*(p++) & 0x80) == 0) goto done;  // predictable if fields are in order
  tag_int |= (*p & 0x7f) << 7;
  if ((*(p++) & 0x80) == 0) goto done;  // likely
slow:
  // Decode a full varint starting over from ptr.
  if (!upb_decode_varint_slow(d, s, &val)) return false;
  tag_int = upb_value_getint64(val);
  p = s->ptr;  // Trick the next line into not overwriting us.
done:
  upb_dstate_advance(s, p - s->ptr);
  tag->wire_type = (upb_wire_type_t)(tag_int & 0x07);
  tag->field_number = tag_int >> 3;
  return true;
}

INLINE bool upb_decode_varint(upb_decoder *d, upb_dstate *s, upb_value *val) {
  if (s->len >= UPB_MAX_VARINT_ENCODED_SIZE) {
    // Common (fast) case.
    uint64_t val64;
    const char *p = s->ptr;
    if (!upb_decode_varint_fast(&p, &val64, d->status)) return false;
    upb_dstate_advance(s, p - s->ptr);
    upb_value_setint64(val, val64);
    return true;
  } else {
    return upb_decode_varint_slow(d, s, val);
  }
}

INLINE bool upb_decode_fixed(upb_decoder *d, upb_wire_type_t wt,
                             upb_dstate *s, upb_value *val) {
  static const char table[] = {0, 8, 0, 0, 0, 4};
  size_t bytes = table[wt];
  if (s->len >= bytes) {
    // Common (fast) case.
    memcpy(&val, s->ptr, bytes);
    upb_dstate_advance(s, bytes);
  } else {
    if (!upb_getbuf(d, &val, bytes, s)) return false;
  }
  return true;
}

// "val" initially holds the length of the string, this is replaced by the
// contents of the string.
INLINE bool upb_decode_string(upb_decoder *d, upb_value *val, upb_string **str,
                              upb_dstate *s) {
  upb_string_recycle(str);
  uint32_t strlen = upb_value_getint32(*val);
  if (s->len >= strlen) {
    // Common (fast) case.
    upb_string_substr(*str, d->buf, s->ptr - upb_string_getrobuf(d->buf), strlen);
    upb_dstate_advance(s, strlen);
  } else {
    if (!upb_getbuf(d, upb_string_getrwbuf(*str, strlen), strlen, s))
      return false;
  }
  return true;
}


/* The main decoding loop *****************************************************/

extern upb_wire_type_t upb_expected_wire_types[];
// Returns true if wt is the correct on-the-wire type for ft.
INLINE bool upb_check_type(upb_wire_type_t wt, upb_fieldtype_t ft) {
  // This doesn't currently support packed arrays.
  return upb_types[ft].native_wire_type == wt;
}

static upb_flow_t upb_push(upb_decoder *d, upb_dstate *s, upb_fielddef *f,
                           upb_strlen_t submsg_len, upb_fieldtype_t type) {
  d->top->field = f;
  d->top++;
  if(d->top >= d->limit) {
    upb_seterr(d->status, UPB_ERROR, "Nesting too deep.");
    return UPB_ERROR;
  }
  d->top->end_offset = (type == UPB_TYPE(GROUP)) ?
      UPB_GROUP_END_OFFSET :
      d->buf_stream_offset + (s->ptr - upb_string_getrobuf(d->buf)) + submsg_len;
  d->top->msgdef = upb_downcast_msgdef(f->def);
  return upb_dispatch_startsubmsg(&d->dispatcher, f);
}

static upb_flow_t upb_pop(upb_decoder *d) {
  d->top--;
  return upb_dispatch_endsubmsg(&d->dispatcher);
}

void upb_decoder_run(upb_src *src, upb_status *status) {
  upb_decoder *d = (upb_decoder*)src;
  // We put our dstate on the stack so the compiler knows they can't be changed
  // by external code (like when we dispatch a callback).  We must be sure not
  // to let its address escape this source file.
  upb_dstate state = {NULL, 0, d->top->msgdef};
  upb_string *str = NULL;

// TODO: handle UPB_SKIPSUBMSG
#define CHECK_FLOW(expr) if ((expr) != UPB_CONTINUE) goto err
#define CHECK(expr) if (!expr) goto err;

  CHECK_FLOW(upb_dispatch_startmsg(&d->dispatcher));

  // Main loop: executed once per tag/field pair.
  while(1) {
    // Parse/handle tag.
    upb_tag tag;
    if (!upb_decode_tag(d, &state, &tag)) {
      if (status->code == UPB_EOF && d->top == d->stack) {
        // Normal end-of-file.
        CHECK_FLOW(upb_dispatch_endmsg(&d->dispatcher));
        return;
      } else {
        goto err;
      }
    }

    // Decode wire data.  Hopefully this branch will predict pretty well
    // since most types will read a varint here.
    upb_value val;
    switch (tag.wire_type) {
      case UPB_WIRE_TYPE_END_GROUP:
        if(d->top->end_offset != UPB_GROUP_END_OFFSET) {
          upb_seterr(status, UPB_ERROR, "Unexpected END_GROUP tag.");
          goto err;
        }
        CHECK_FLOW(upb_pop(d));
        continue;  // We have no value to dispatch.
      case UPB_WIRE_TYPE_VARINT:
      case UPB_WIRE_TYPE_DELIMITED:
        // For the delimited case we are parsing the length.
        CHECK(upb_decode_varint(d, &state, &val));
        break;
      case UPB_WIRE_TYPE_32BIT:
      case UPB_WIRE_TYPE_64BIT:
        CHECK(upb_decode_fixed(d, tag.wire_type, &state, &val));
        break;
    }

    // Look up field by tag number.
    upb_fielddef *f = upb_msgdef_itof(d->top->msgdef, tag.field_number);

    if (!f) {
      if (tag.wire_type == UPB_WIRE_TYPE_DELIMITED)
        CHECK(upb_decode_string(d, &val, &str, &state));
      CHECK_FLOW(upb_dispatch_unknownval(&d->dispatcher, tag.field_number, val));
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
        CHECK_FLOW(upb_push(d, &state, f, upb_value_getint32(val), f->type));
        continue;  // We have no value to dispatch.
      case UPB_TYPE(STRING):
      case UPB_TYPE(BYTES):
        CHECK(upb_decode_string(d, &val, &str, &state));
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
    CHECK_FLOW(upb_dispatch_value(&d->dispatcher, f, val));
  }

err:
  if (upb_ok(status)) {
    upb_seterr(status, UPB_ERROR, "Callback returned UPB_BREAK");
  }
}

void upb_decoder_sethandlers(upb_src *src, upb_handlers *handlers) {
  upb_decoder *d = (upb_decoder*)src;
  upb_dispatcher_reset(&d->dispatcher, handlers);
  d->top = d->stack;
  d->buf_stream_offset = 0;
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
