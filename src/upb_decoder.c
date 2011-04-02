/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2008-2011 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 */

#include <inttypes.h>
#include <stddef.h>
#include <stdlib.h>
#include "upb_decoder.h"
#include "upb_varint_decoder.h"

#ifdef UPB_USE_JIT_X64
#define Dst_DECL upb_decoder *d
#define Dst_REF (d->dynasm)
#define Dst (d)
#include "dynasm/dasm_proto.h"
#include "upb_decoder_x86.h"
#endif

/* Decoding/Buffering of individual values ************************************/

// Performs zig-zag decoding, which is used by sint32 and sint64.
INLINE int32_t upb_zzdec_32(uint32_t n) { return (n >> 1) ^ -(int32_t)(n & 1); }
INLINE int64_t upb_zzdec_64(uint64_t n) { return (n >> 1) ^ -(int64_t)(n & 1); }

#define UPB_MAX_VARINT_ENCODED_SIZE 10

INLINE void upb_decoder_advance(upb_decoder *d, size_t len) {
  d->ptr += len;
}

INLINE size_t upb_decoder_offset(upb_decoder *d) {
  size_t offset = d->buf_stream_offset;
  if (d->buf) offset += (d->ptr - d->buf);
  return offset;
}

INLINE size_t upb_decoder_bufleft(upb_decoder *d) {
  return d->end - d->ptr;
}

INLINE void upb_dstate_setmsgend(upb_decoder *d) {
  uint32_t end_offset = d->dispatcher.top->end_offset;
  d->submsg_end = (end_offset == UINT32_MAX) ?
      (void*)UINTPTR_MAX : d->buf + end_offset;
}

// Pulls the next buffer from the bytesrc.  Should be called only when the
// current buffer is completely empty.
static bool upb_pullbuf(upb_decoder *d) {
  assert(upb_decoder_bufleft(d) == 0);
  int32_t last_buf_len = d->buf ? upb_string_len(d->bufstr) : -1;
  upb_string_recycle(&d->bufstr);
  if (!upb_bytesrc_getstr(d->bytesrc, d->bufstr, d->status)) {
    d->buf = NULL;
    d->end = NULL;
    return false;
  }
  if (last_buf_len != -1) {
    d->buf_stream_offset += last_buf_len;
    for (upb_dispatcher_frame *f = d->dispatcher.stack; f <= d->dispatcher.top; ++f)
      if (f->end_offset != UINT32_MAX)
        f->end_offset -= last_buf_len;
  }
  d->buf = upb_string_getrobuf(d->bufstr);
  d->ptr = upb_string_getrobuf(d->bufstr);
  d->end = d->buf + upb_string_len(d->bufstr);
  d->jit_end = d->end - 20;
  upb_string_substr(d->tmp, d->bufstr, 0, 0);
  upb_dstate_setmsgend(d);
  return true;
}

// Called only from the slow path, this function copies the next "len" bytes
// from the stream to "data", adjusting the dstate appropriately.
static bool upb_getbuf(upb_decoder *d, void *data, size_t bytes_wanted) {
  while (1) {
    size_t to_copy = UPB_MIN(bytes_wanted, upb_decoder_bufleft(d));
    memcpy(data, d->ptr, to_copy);
    upb_decoder_advance(d, to_copy);
    bytes_wanted -= to_copy;
    if (bytes_wanted == 0) return true;
    if (!upb_pullbuf(d)) return false;
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

INLINE bool upb_decode_tag(upb_decoder *d, uint32_t *tag) {
  const char *p = d->ptr;
  upb_value val;
  // Nearly all tag varints will be either 1 byte (1-16) or 2 bytes (17-2048).
  if (upb_decoder_bufleft(d) < 2) goto slow;  // unlikely.
  *tag = *p & 0x7f;
  if ((*(p++) & 0x80) == 0) goto done;  // predictable if fields are in order
  *tag |= (*p & 0x7f) << 7;
  if ((*(p++) & 0x80) == 0) goto done;  // likely
slow:
  // Decode a full varint starting over from ptr.
  if (!upb_decode_varint_slow(d, &val)) return false;
  *tag = upb_value_getint64(val);
  p = d->ptr;  // Trick the next line into not overwriting us.
done:
  upb_decoder_advance(d, p - d->ptr);
  return true;
}

INLINE bool upb_decode_varint(upb_decoder *d, upb_value *val) {
  if (upb_decoder_bufleft(d) >= 16) {
    // Common (fast) case.
    upb_decoderet r = upb_vdecode_fast(d->ptr);
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
    upb_string_substr(*str, d->bufstr, d->ptr - d->buf, strlen);
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

static upb_flow_t upb_pop(upb_decoder *d) {
  upb_flow_t ret = upb_dispatch_endsubmsg(&d->dispatcher);
  upb_dstate_setmsgend(d);
  return ret;
}

static upb_flow_t upb_decoder_skipsubmsg(upb_decoder *d) {
  if (d->dispatcher.top->f->type == UPB_TYPE(GROUP)) {
    fprintf(stderr, "upb_decoder: Can't skip groups yet.\n");
    abort();
  }
  upb_decoder_advance(d, d->dispatcher.top->end_offset - (d->ptr - d->buf));
  upb_pop(d);
  return UPB_CONTINUE;
}

static upb_flow_t upb_push(upb_decoder *d, upb_handlers_fieldent *f,
                           uint32_t end_offset) {
  upb_flow_t flow = upb_dispatch_startsubmsg(&d->dispatcher, f, end_offset);
  upb_dstate_setmsgend(d);
  return flow;
}

void upb_decoder_decode(upb_decoder *d, upb_status *status) {
  d->status = status;

#define CHECK_FLOW(expr) \
    switch (expr) { \
      case UPB_BREAK: goto callback_err; \
      case UPB_SKIPSUBMSG: upb_decoder_skipsubmsg(d); continue; \
      default: break; /* continue normally. */ \
    }
#define CHECK(expr) if (!expr) { assert(!upb_ok(status)); goto err; }

  CHECK(upb_pullbuf(d));
  if (upb_dispatch_startmsg(&d->dispatcher) != UPB_CONTINUE) goto err;

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
#ifdef UPB_USE_JIT_X64
    void (*upb_jit_decode)(upb_decoder *d) = (void*)d->jit_code;
    if (d->jit_code && d->dispatcher.top == d->dispatcher.stack && d->ptr < d->jit_end) {
      const char *before = d->ptr;
      //fprintf(stderr, "Entering JIT, JIT bytes left: %zd\n", d->jit_end - d->ptr);
      upb_jit_decode(d);
      //fprintf(stderr, "Exiting JIT, parsed %zd bytes\n", d->ptr - before);
      //fprintf(stderr, "ptr: %p, effective_end: %p, jit_end: %p, effective_end-ptr=%d\n",
      //        d->ptr, d->effective_end, d->jit_end, d->effective_end - d->ptr);
    }
#endif

    // Parse/handle tag.
    uint32_t tag;
    if (!upb_decode_tag(d, &tag)) {
      if (status->code == UPB_EOF && upb_dispatcher_stackempty(&d->dispatcher)) {
        // Normal end-of-file.
        upb_clearerr(status);
        upb_dispatch_endmsg(&d->dispatcher, status);
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
    uint8_t wire_type = tag & 0x7;
    switch (wire_type) {
      case UPB_WIRE_TYPE_START_GROUP:
        break;  // Nothing to do now, below we will push appropriately.
      case UPB_WIRE_TYPE_END_GROUP:
        // Strictly speaking we should also check the field number here.
        if(d->dispatcher.top->f->type != UPB_TYPE(GROUP)) {
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
    upb_dispatcher_field *f = upb_dispatcher_lookup(&d->dispatcher, tag);

    if (!f) {
      if (wire_type == UPB_WIRE_TYPE_DELIMITED)
        CHECK(upb_decode_string(d, &val, &d->tmp));
      // TODO.
      CHECK_FLOW(upb_dispatch_unknownval(&d->dispatcher, 0, UPB_NO_VALUE));
      continue;
    }

    // Perform any further massaging of the data now that we have the field's
    // type.  Now we can distinguish strings from submessages, and we know
    // about zig-zag-encoded types.
    // TODO: handle packed encoding.
    // TODO: if we were being paranoid, we could check for 32-bit-varint types
    // that the top 32 bits all match the highest bit of the low 32 bits.
    // If this is not true we are losing data.  But the main protobuf library
    // doesn't check this, and it would slow us down, so pass for now.
    switch (f->type) {
      case UPB_TYPE(GROUP):
        CHECK_FLOW(upb_push(d, f, UINT32_MAX));
        continue;  // We have no value to dispatch.
      case UPB_TYPE(MESSAGE):
        CHECK_FLOW(upb_push(d, f, upb_value_getuint32(val) + (d->ptr - d->buf)));
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
  if (upb_ok(status)) {
    upb_seterr(status, UPB_ERROR, "Callback returned UPB_BREAK");
  }
err:
  assert(!upb_ok(status));
}

void upb_decoder_init(upb_decoder *d, upb_handlers *handlers) {
  upb_dispatcher_init(&d->dispatcher, handlers);
#ifdef UPB_USE_JIT_X64
  d->jit_code = NULL;
  if (d->dispatcher.handlers->should_jit) upb_decoder_makejit(d);
#endif
  d->bufstr = NULL;
  d->buf = NULL;
  d->tmp = NULL;
  upb_string_recycle(&d->tmp);
}

void upb_decoder_reset(upb_decoder *d, upb_bytesrc *bytesrc, void *closure) {
  upb_dispatcher_reset(&d->dispatcher, closure, UINT32_MAX);
  d->bytesrc = bytesrc;
  d->buf = NULL;
  d->ptr = NULL;
  d->end = NULL;  // Force a buffer pull.
  d->submsg_end = (void*)0x1;  // But don't let end-of-message get triggered.
  d->buf_stream_offset = 0;
}

void upb_decoder_uninit(upb_decoder *d) {
  upb_dispatcher_uninit(&d->dispatcher);
  upb_string_unref(d->bufstr);
  upb_string_unref(d->tmp);
#ifdef UPB_USE_JIT_X64
  if (d->dispatcher.handlers->should_jit) upb_decoder_freejit(d);
#endif
}
