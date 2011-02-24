/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2008-2011 Joshua Haberman.  See LICENSE for details.
 */

#include "upb_decoder.h"
#include "upb_varint_decoder.h"

#include <inttypes.h>
#include <stddef.h>
#include <stdlib.h>
#include "upb_def.h"

// If the return value is other than UPB_CONTINUE, that is what the last
// callback returned.
typedef struct {
  upb_flow_t flow;
  const char *ptr;
} fastdecode_ret;
extern fastdecode_ret upb_fastdecode(const char *p, const char *end,
                                     upb_value_handler_t value_cb, void *closure,
                                     void *table, int table_size);


/* Decoding/Buffering of individual values ************************************/

// Performs zig-zag decoding, which is used by sint32 and sint64.
INLINE int32_t upb_zzdec_32(uint32_t n) { return (n >> 1) ^ -(int32_t)(n & 1); }
INLINE int64_t upb_zzdec_64(uint64_t n) { return (n >> 1) ^ -(int64_t)(n & 1); }

// Constant used to signal that the submessage is a group and therefore we
// don't know its end offset.  This cannot be the offset of a real submessage
// end because it takes at least one byte to begin a submessage.
#define UPB_GROUP_END_OFFSET 0
#define UPB_MAX_VARINT_ENCODED_SIZE 10

INLINE void upb_decoder_advance(upb_decoder *d, size_t len) {
  d->ptr += len;
  //d->bytes_parsed_slow += len;
}

INLINE size_t upb_decoder_bufleft(upb_decoder *d) {
  return d->end - d->ptr;
}

INLINE void upb_dstate_setmsgend(upb_decoder *d) {
  d->submsg_end = (d->top->end_offset == UPB_GROUP_END_OFFSET) ?
      (void*)UINTPTR_MAX :
      upb_string_getrobuf(d->buf) + (d->top->end_offset - d->buf_stream_offset);
}

static upb_flow_t upb_pop(upb_decoder *d);

// Called only from the slow path, this function copies the next "len" bytes
// from the stream to "data", adjusting the dstate appropriately.
static bool upb_getbuf(upb_decoder *d, void *data, size_t bytes_wanted) {
  while (1) {
    size_t to_copy = UPB_MIN(bytes_wanted, upb_decoder_bufleft(d));
    memcpy(data, d->ptr, to_copy);
    upb_decoder_advance(d, to_copy);
    bytes_wanted -= to_copy;
    if (bytes_wanted == 0) {
      upb_dstate_setmsgend(d);
      return true;
    }

    // Get next buffer.
    if (d->buf) d->buf_stream_offset += upb_string_len(d->buf);
    upb_string_recycle(&d->buf);
    if (!upb_bytesrc_getstr(d->bytesrc, d->buf, d->status)) return false;
    d->ptr = upb_string_getrobuf(d->buf);
    d->end = d->ptr + upb_string_len(d->buf);
  }
}

// We use this path when we don't have UPB_MAX_VARINT_ENCODED_SIZE contiguous
// bytes available in our current buffer.  We don't inline this because we
// accept that it will be slow and we don't want to pay for two copies of it.
static bool upb_decode_varint_slow(upb_decoder *d, upb_value *val) {
  char byte = 0x80;
  uint64_t val64 = 0;
  int bitpos;
  for(bitpos = 0;
      bitpos < 70 && (byte & 0x80) && upb_getbuf(d, &byte, 1);
      bitpos += 7)
    val64 |= ((uint64_t)byte & 0x7F) << bitpos;

  if(bitpos == 70) {
    upb_seterr(d->status, UPB_ERROR,
               "Varint was unterminated after 10 bytes.\n");
    return false;
  } else if (d->status->code == UPB_EOF && bitpos == 0) {
    // Regular EOF.
    return false;
  } else if (d->status->code == UPB_EOF  && (byte & 0x80)) {
    upb_seterr(d->status, UPB_ERROR,
               "Provided data ended in the middle of a varint.\n");
    return false;
  } else {
    // Success.
    upb_value_setraw(val, val64);
    return true;
  }
}

typedef struct {
  upb_wire_type_t wire_type;
  upb_field_number_t field_number;
} upb_tag;

INLINE bool upb_decode_tag(upb_decoder *d, upb_tag *tag) {
  const char *p = d->ptr;
  uint32_t tag_int;
  upb_value val;
  // Nearly all tag varints will be either 1 byte (1-16) or 2 bytes (17-2048).
  if (upb_decoder_bufleft(d) < 2) goto slow;  // unlikely.
  tag_int = *p & 0x7f;
  if ((*(p++) & 0x80) == 0) goto done;  // predictable if fields are in order
  tag_int |= (*p & 0x7f) << 7;
  if ((*(p++) & 0x80) == 0) goto done;  // likely
slow:
  // Decode a full varint starting over from ptr.
  if (!upb_decode_varint_slow(d, &val)) return false;
  tag_int = upb_value_getint64(val);
  p = d->ptr;  // Trick the next line into not overwriting us.
done:
  upb_decoder_advance(d, p - d->ptr);
  tag->wire_type = (upb_wire_type_t)(tag_int & 0x07);
  tag->field_number = tag_int >> 3;
  return true;
}

INLINE bool upb_decode_varint(upb_decoder *d, upb_value *val) {
  if (upb_decoder_bufleft(d) >= 16) {
    // Common (fast) case.
    upb_decoderet r = upb_decode_varint_fast(d->ptr);
    if (r.p == NULL) {
      upb_seterr(d->status, UPB_ERROR, "Unterminated varint.\n");
      return false;
    }
    upb_value_setraw(val, r.val);
    upb_decoder_advance(d, r.p - d->ptr);
    return true;
  } else {
    return upb_decode_varint_slow(d, val);
  }
}

INLINE bool upb_decode_fixed(upb_decoder *d, size_t bytes, upb_value *val) {
  if (upb_decoder_bufleft(d) >= bytes) {
    // Common (fast) case.
    memcpy(val, d->ptr, bytes);
    upb_decoder_advance(d, bytes);
  } else {
    if (!upb_getbuf(d, val, bytes)) return false;
  }
  return true;
}

// "val" initially holds the length of the string, this is replaced by the
// contents of the string.
INLINE bool upb_decode_string(upb_decoder *d, upb_value *val,
                              upb_string **str) {
  upb_string_recycle(str);
  uint32_t strlen = upb_value_getint32(*val);
  if (upb_decoder_bufleft(d) >= strlen) {
    // Common (fast) case.
    upb_string_substr(*str, d->buf, d->ptr - upb_string_getrobuf(d->buf), strlen);
    upb_decoder_advance(d, strlen);
  } else {
    if (!upb_getbuf(d, upb_string_getrwbuf(*str, strlen), strlen))
      return false;
  }
  upb_value_setstr(val, *str);
  return true;
}


/* The main decoding loop *****************************************************/

extern upb_wire_type_t upb_expected_wire_types[];
// Returns true if wt is the correct on-the-wire type for ft.
INLINE bool upb_check_type(upb_wire_type_t wt, upb_fieldtype_t ft) {
  // This doesn't currently support packed arrays.
  return upb_types[ft].native_wire_type == wt;
}

static upb_flow_t upb_push(upb_decoder *d, upb_fielddef *f,
                           upb_value submsg_len, upb_fieldtype_t type) {
  ++d->top;
  if(d->top >= d->limit) {
    upb_seterr(d->status, UPB_ERROR, "Nesting too deep.");
    return UPB_ERROR;
  }
  d->top->end_offset = (type == UPB_TYPE(GROUP)) ?
      UPB_GROUP_END_OFFSET :
      d->buf_stream_offset + (d->ptr - upb_string_getrobuf(d->buf)) +
          upb_value_getint32(submsg_len);
  upb_msgdef *md = upb_downcast_msgdef(f->def);
  d->top->msgdef = md;
  d->msgdef = md;
  upb_dstate_setmsgend(d);
  upb_flow_t ret = upb_dispatch_startsubmsg(&d->dispatcher, f);
  if (ret == UPB_SKIPSUBMSG) {
    if (type == UPB_TYPE(GROUP)) {
      fprintf(stderr, "upb_decoder: Can't skip groups yet.\n");
      abort();
    }
    upb_decoder_advance(d, upb_value_getint32(submsg_len));
    --d->top;
    upb_dstate_setmsgend(d);
    ret = UPB_CONTINUE;
  }
  return ret;
}

static upb_flow_t upb_pop(upb_decoder *d) {
  --d->top;
  upb_dstate_setmsgend(d);
  d->msgdef = d->top->msgdef;
  return upb_dispatch_endsubmsg(&d->dispatcher);
}

void upb_decoder_run(upb_src *src, upb_status *status) {
  upb_decoder *d = (upb_decoder*)src;
  d->status = status;
  d->ptr = NULL;
  d->end = NULL;  // Force a buffer pull.
  d->submsg_end = (void*)0x1;  // But don't let end-of-message get triggered.
  d->msgdef = d->top->msgdef;

// TODO: handle UPB_SKIPSUBMSG
#define CHECK_FLOW(expr) if ((expr) == UPB_BREAK) { /*assert(!upb_ok(status));*/ goto callback_err; }
#define CHECK(expr) if (!expr) { assert(!upb_ok(status)); goto err; }

  CHECK_FLOW(upb_dispatch_startmsg(&d->dispatcher));

  // Main loop: executed once per tag/field pair.
  while(1) {
    // Check for end-of-submessage.
    while (d->ptr >= d->submsg_end) {
      if (d->ptr > d->submsg_end) {
        upb_seterr(d->status, UPB_ERROR, "Bad submessage end.");
        goto err;
      }
      CHECK_FLOW(upb_pop(d));
    }

    // Decodes as many fields as possible, updating d->ptr appropriately,
    // before falling through to the slow(er) path.
#ifdef USE_ASSEMBLY_FASTPATH
    const char *end = UPB_MIN(d->end, d->submsg_end);
    fastdecode_ret ret = upb_fastdecode(d->ptr, end,
                                        d->dispatcher.top->handlers.set->value,
                                        d->dispatcher.top->handlers.closure,
                                        d->msgdef->itof.array,
                                        d->msgdef->itof.array_size,
                                        d->tmp);
    CHECK_FLOW(ret.flow);
#endif

    // Parse/handle tag.
    upb_tag tag;
    if (!upb_decode_tag(d, &tag)) {
      if (status->code == UPB_EOF && d->top == d->stack) {
        // Normal end-of-file.
        upb_clearerr(status);
        CHECK_FLOW(upb_dispatch_endmsg(&d->dispatcher));
        return;
      } else {
        if (status->code == UPB_EOF) {
          upb_seterr(status, UPB_ERROR,
                     "Input ended in the middle of a submessage.");
        }
        goto err;
      }
    }

    // Decode wire data.  Hopefully this branch will predict pretty well
    // since most types will read a varint here.
    upb_value val;
    switch (tag.wire_type) {
      case UPB_WIRE_TYPE_START_GROUP:
        break;  // Nothing to do now, below we will push appropriately.
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
        CHECK(upb_decode_varint(d, &val));
        break;
      case UPB_WIRE_TYPE_32BIT:
        CHECK(upb_decode_fixed(d, 4, &val));
        break;
      case UPB_WIRE_TYPE_64BIT:
        CHECK(upb_decode_fixed(d, 8, &val));
        break;
    }

    // Look up field by tag number.
    upb_itof_ent *e = upb_msgdef_itofent(d->msgdef, tag.field_number);

    if (!e) {
      if (tag.wire_type == UPB_WIRE_TYPE_DELIMITED)
        CHECK(upb_decode_string(d, &val, &d->tmp));
      CHECK_FLOW(upb_dispatch_unknownval(&d->dispatcher, tag.field_number, val));
      continue;
    }

    upb_fielddef *f = e->f;
    assert(e->field_type == f->type);
    assert(e->native_wire_type == upb_types[f->type].native_wire_type);

    if (tag.wire_type != e->native_wire_type) {
      // TODO: Support packed fields.
      upb_seterr(status, UPB_ERROR, "Field had incorrect type, name: " UPB_STRFMT
                 ", field type: %d, expected wire type %d, actual wire type: %d",
                 UPB_STRARG(f->name), f->type, upb_types[f->type].native_wire_type,
                 tag.wire_type);
      upb_printerr(status);
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
    switch (e->field_type) {
      case UPB_TYPE(MESSAGE):
      case UPB_TYPE(GROUP):
        CHECK_FLOW(upb_push(d, f, val, e->field_type));
        continue;  // We have no value to dispatch.
      case UPB_TYPE(STRING):
      case UPB_TYPE(BYTES):
        CHECK(upb_decode_string(d, &val, &d->tmp));
        break;
      case UPB_TYPE(SINT32):
        upb_value_setint32(&val, upb_zzdec_32(upb_value_getint32(val)));
        break;
      case UPB_TYPE(SINT64):
        upb_value_setint64(&val, upb_zzdec_64(upb_value_getint64(val)));
        break;
      default:
#ifndef NDEBUG
        val.type = upb_types[f->type].inmemory_type;
#endif
        break;  // Other types need no further processing at this point.
    }
    CHECK_FLOW(upb_dispatch_value(&d->dispatcher, f, val));
  }

callback_err:
  upb_copyerr(status, d->dispatcher.top->handlers.status);
  if (upb_ok(status)) {
    upb_seterr(status, UPB_ERROR, "Callback returned UPB_BREAK");
  }
err:
  assert(!upb_ok(status));
}

void upb_decoder_sethandlers(upb_src *src, upb_handlers *handlers) {
  upb_decoder *d = (upb_decoder*)src;
  upb_dispatcher_reset(&d->dispatcher, handlers, true);
  d->top = d->stack;
  d->buf_stream_offset = 0;
  // The top-level message is not delimited (we can keep receiving data for it
  // indefinitely), so we treat it like a group.
  d->top->end_offset = 0;
}

void upb_decoder_init(upb_decoder *d, upb_msgdef *msgdef) {
  static upb_src_vtbl vtbl = {
    &upb_decoder_sethandlers,
    &upb_decoder_run,
  };
  upb_src_init(&d->src, &vtbl);
  upb_dispatcher_init(&d->dispatcher);
  d->stack[0].msgdef = msgdef;
  d->stack[0].end_offset = UPB_GROUP_END_OFFSET;
  d->limit = &d->stack[UPB_MAX_NESTING];
  d->buf = NULL;
  d->tmp = NULL;
}

void upb_decoder_reset(upb_decoder *d, upb_bytesrc *bytesrc) {
  d->bytesrc = bytesrc;
  d->top = &d->stack[0];
}

void upb_decoder_uninit(upb_decoder *d) {
  upb_string_unref(d->buf);
  upb_string_unref(d->tmp);
}

upb_src *upb_decoder_src(upb_decoder *d) { return &d->src; }
