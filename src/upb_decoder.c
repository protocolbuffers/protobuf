/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2008-2011 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 */

#include <inttypes.h>
#include <stddef.h>
#include <stdlib.h>
#include "upb_bytestream.h"
#include "upb_decoder.h"
#include "upb_varint.h"

// Used for frames that have no specific end offset: groups, repeated primitive
// fields inside groups, and the top-level message.
#define UPB_NONDELIMITED UINT32_MAX

#ifdef UPB_USE_JIT_X64
#define Dst_DECL upb_decoder *d
#define Dst_REF (d->dynasm)
#define Dst (d)
#include "dynasm/dasm_proto.h"
#include "upb_decoder_x86.h"
#endif

// It's unfortunate that we have to micro-manage the compiler this way,
// especially since this tuning is necessarily specific to one hardware
// configuration.  But emperically on a Core i7, performance increases 30-50%
// with these annotations.  Every instance where these appear, gcc 4.2.1 made
// the wrong decision and degraded performance in benchmarks.
#define FORCEINLINE static __attribute__((always_inline))
#define NOINLINE static __attribute__((noinline))

static void upb_decoder_exit(upb_decoder *d) { siglongjmp(d->exitjmp, 1); }
static void upb_decoder_exit2(void *_d) {
  upb_decoder *d = _d;
  upb_decoder_exit(d);
}

/* Decoding/Buffering of wire types *******************************************/

#define UPB_MAX_VARINT_ENCODED_SIZE 10

static void upb_decoder_advance(upb_decoder *d, size_t len) { d->ptr += len; }
static size_t upb_decoder_bufleft(upb_decoder *d) { return d->end - d->ptr; }

size_t upb_decoder_offset(upb_decoder *d) {
  size_t offset = d->buf_stream_offset;
  if (d->buf) offset += (d->ptr - d->buf);
  return offset;
}

static void upb_decoder_setmsgend(upb_decoder *d) {
  uint32_t end = d->dispatcher.top->end_offset;
  d->submsg_end = (end == UPB_NONDELIMITED) ? (void*)UINTPTR_MAX : d->buf + end;
}

// Pulls the next buffer from the bytesrc.  Should be called only when the
// current buffer is completely empty.
static void upb_pullbuf(upb_decoder *d, bool need) {
  assert(upb_decoder_bufleft(d) == 0);
  int32_t last_buf_len = d->buf ? upb_string_len(d->bufstr) : -1;
  upb_string_recycle(&d->bufstr);
  if (!upb_bytesrc_getstr(d->bytesrc, d->bufstr, d->status)) {
    d->buf = NULL;
    d->end = NULL;
    if (need) upb_seterr(d->status, UPB_ERROR, "Unexpected EOF.");
    upb_decoder_exit(d);
  }
  if (last_buf_len != -1) {
    d->buf_stream_offset += last_buf_len;
    for (upb_dispatcher_frame *f = d->dispatcher.stack; f <= d->dispatcher.top; ++f)
      if (f->end_offset != UPB_NONDELIMITED)
        f->end_offset -= last_buf_len;
  }
  d->buf = upb_string_getrobuf(d->bufstr);
  d->ptr = upb_string_getrobuf(d->bufstr);
  d->end = d->buf + upb_string_len(d->bufstr);
  d->jit_end = d->end - 20;
  upb_string_recycle(&d->tmp);
  upb_string_substr(d->tmp, d->bufstr, 0, 0);
  upb_decoder_setmsgend(d);
}

// Called only from the slow path, this function copies the next "len" bytes
// from the stream to "data", adjusting the decoder state appropriately.
NOINLINE void upb_getbuf(upb_decoder *d, void *data, size_t bytes, bool need) {
  while (1) {
    size_t to_copy = UPB_MIN(bytes, upb_decoder_bufleft(d));
    memcpy(data, d->ptr, to_copy);
    upb_decoder_advance(d, to_copy);
    bytes -= to_copy;
    if (bytes == 0) return;
    upb_pullbuf(d, need);
  }
}

NOINLINE uint64_t upb_decode_varint_slow(upb_decoder *d, bool need) {
  uint8_t byte = 0x80;
  uint64_t u64 = 0;
  int bitpos;
  for(bitpos = 0; bitpos < 70 && (byte & 0x80); bitpos += 7) {
    upb_getbuf(d, &byte, 1, need);
    u64 |= ((uint64_t)byte & 0x7F) << bitpos;
  }

  if(bitpos == 70 && (byte & 0x80)) {
    upb_seterr(d->status, UPB_ERROR, "Unterminated varint.\n");
    upb_decoder_exit(d);
  }
  return u64;
}

// For tags and delimited lengths, which must be <=32bit and are usually small.
FORCEINLINE uint32_t upb_decode_varint32(upb_decoder *d, bool need) {
  const char *p = d->ptr;
  uint32_t ret;
  uint64_t u64;
  // Nearly all will be either 1 byte (1-16) or 2 bytes (17-2048).
  if (upb_decoder_bufleft(d) < 2) goto slow;  // unlikely.
  ret = *p & 0x7f;
  if ((*(p++) & 0x80) == 0) goto done;  // predictable if fields are in order
  ret |= (*p & 0x7f) << 7;
  if ((*(p++) & 0x80) == 0) goto done;  // likely
slow:
  u64 = upb_decode_varint_slow(d, need);
  if (u64 > 0xffffffff) {
    upb_seterr(d->status, UPB_ERROR, "Unterminated 32-bit varint.\n");
    upb_decoder_exit(d);
  }
  ret = (uint32_t)u64;
  p = d->ptr;  // Turn the next line into a nop.
done:
  upb_decoder_advance(d, p - d->ptr);
  return ret;
}

FORCEINLINE uint64_t upb_decode_varint(upb_decoder *d) {
  if (upb_decoder_bufleft(d) >= 16) {
    // Common (fast) case.
    upb_decoderet r = upb_vdecode_fast(d->ptr);
    if (r.p == NULL) {
      upb_seterr(d->status, UPB_ERROR, "Unterminated varint.\n");
      upb_decoder_exit(d);
    }
    upb_decoder_advance(d, r.p - d->ptr);
    return r.val;
  } else {
    return upb_decode_varint_slow(d, true);
  }
}

FORCEINLINE void upb_decode_fixed(upb_decoder *d, void *val, size_t bytes) {
  if (upb_decoder_bufleft(d) >= bytes) {
    // Common (fast) case.
    memcpy(val, d->ptr, bytes);
    upb_decoder_advance(d, bytes);
  } else {
    upb_getbuf(d, val, bytes, true);
  }
}

FORCEINLINE uint32_t upb_decode_fixed32(upb_decoder *d) {
  uint32_t u32;
  upb_decode_fixed(d, &u32, sizeof(uint32_t));
  return u32;
}
FORCEINLINE uint64_t upb_decode_fixed64(upb_decoder *d) {
  uint64_t u64;
  upb_decode_fixed(d, &u64, sizeof(uint64_t));
  return u64;
}

INLINE upb_string *upb_decode_string(upb_decoder *d) {
  upb_string_recycle(&d->tmp);
  uint32_t strlen = upb_decode_varint32(d, true);
  if (upb_decoder_bufleft(d) >= strlen) {
    // Common (fast) case.
    upb_string_substr(d->tmp, d->bufstr, d->ptr - d->buf, strlen);
    upb_decoder_advance(d, strlen);
  } else {
    upb_getbuf(d, upb_string_getrwbuf(d->tmp, strlen), strlen, true);
  }
  return d->tmp;
}

INLINE void upb_push(upb_decoder *d, upb_fhandlers *f, uint32_t end) {
  upb_dispatch_startsubmsg(&d->dispatcher, f)->end_offset = end;
  upb_decoder_setmsgend(d);
}


/* Decoding of .proto types ***************************************************/

// Technically, we are losing data if we see a 32-bit varint that is not
// properly sign-extended.  We could detect this and error about the data loss,
// but proto2 does not do this, so we pass.

#define T(type, wt, valtype, convfunc) \
  INLINE void upb_decode_ ## type(upb_decoder *d, upb_fhandlers *f) { \
    upb_value val; \
    upb_value_set ## valtype(&val, (convfunc)(upb_decode_ ## wt(d))); \
    upb_dispatch_value(&d->dispatcher, f, val); \
  } \

static double  upb_asdouble(uint64_t n) { double d; memcpy(&d, &n, 8); return d; }
static float   upb_asfloat(uint32_t n)  { float  f; memcpy(&f, &n, 4); return f; }
static int32_t upb_zzdec_32(uint32_t n) { return (n >> 1) ^ -(int32_t)(n & 1); }
static int64_t upb_zzdec_64(uint64_t n) { return (n >> 1) ^ -(int64_t)(n & 1); }

T(INT32,    varint,  int32,  int32_t)
T(INT64,    varint,  int64,  int64_t)
T(UINT32,   varint,  uint32, uint32_t)
T(UINT64,   varint,  uint64, uint64_t)
T(FIXED32,  fixed32, uint32, uint32_t)
T(FIXED64,  fixed64, uint64, uint64_t)
T(SFIXED32, fixed32, int32,  int32_t)
T(SFIXED64, fixed64, int64,  int64_t)
T(BOOL,     varint,  bool,   bool)
T(ENUM,     varint,  int32,  int32_t)
T(DOUBLE,   fixed64, double, upb_asdouble)
T(FLOAT,    fixed32, float,  upb_asfloat)
T(SINT32,   varint,  int32,  upb_zzdec_32)
T(SINT64,   varint,  int64,  upb_zzdec_64)
T(STRING,   string,  str,    upb_string*)

static void upb_decode_GROUP(upb_decoder *d, upb_fhandlers *f) {
  upb_push(d, f, UPB_NONDELIMITED);
}
static void upb_endgroup(upb_decoder *d, upb_fhandlers *f) {
  (void)f;
  upb_dispatch_endsubmsg(&d->dispatcher);
  upb_decoder_setmsgend(d);
}
static void upb_decode_MESSAGE(upb_decoder *d, upb_fhandlers *f) {
  upb_push(d, f, upb_decode_varint32(d, true) + (d->ptr - d->buf));
}


/* The main decoding loop *****************************************************/

// Called when a user callback returns something other than UPB_CONTINUE.
// This should unwind one or more stack frames, skipping the corresponding
// data in the input.

static void upb_delimend(upb_decoder *d) {
  if (d->ptr > d->submsg_end) {
    upb_seterr(d->status, UPB_ERROR, "Bad submessage end.");
    upb_decoder_exit(d);
  }

  if (d->dispatcher.top->is_sequence) {
    upb_dispatch_endseq(&d->dispatcher);
  } else {
    upb_dispatch_endsubmsg(&d->dispatcher);
  }
  upb_decoder_setmsgend(d);
}

static void upb_decoder_enterjit(upb_decoder *d) {
  (void)d;
#ifdef UPB_USE_JIT_X64
  if (d->jit_code && d->dispatcher.top == d->dispatcher.stack && d->ptr < d->jit_end) {
    // Decodes as many fields as possible, updating d->ptr appropriately,
    // before falling through to the slow(er) path.
    void (*upb_jit_decode)(upb_decoder *d) = (void*)d->jit_code;
    upb_jit_decode(d);
  }
#endif
}

INLINE upb_fhandlers *upb_decode_tag(upb_decoder *d) {
  while (1) {
    uint32_t tag = upb_decode_varint32(d, false);
    upb_fhandlers *f = upb_dispatcher_lookup(&d->dispatcher, tag);

    // There are no explicit "startseq" or "endseq" markers in protobuf
    // streams, so we have to infer them by noticing when a repeated field
    // starts or ends.
    if (d->dispatcher.top->is_sequence && d->dispatcher.top->f != f) {
      upb_dispatch_endseq(&d->dispatcher);
      upb_decoder_setmsgend(d);
    }
    if (f && f->repeated && d->dispatcher.top->f != f) {
      // TODO: support packed.
      assert(upb_issubmsgtype(f->type) || upb_isstringtype(f->type) ||
             (tag & 0x7) != UPB_WIRE_TYPE_DELIMITED);
      uint32_t end = d->dispatcher.top->end_offset;
      upb_dispatch_startseq(&d->dispatcher, f)->end_offset = end;
      upb_decoder_setmsgend(d);
    }
    if (f) return f;

    // Unknown field.
    switch (tag & 0x7) {
      case UPB_WIRE_TYPE_VARINT:    upb_decode_varint(d); break;
      case UPB_WIRE_TYPE_32BIT:     upb_decoder_advance(d, 4); break;
      case UPB_WIRE_TYPE_64BIT:     upb_decoder_advance(d, 8); break;
      case UPB_WIRE_TYPE_DELIMITED:
        upb_decoder_advance(d, upb_decode_varint32(d, true));
        break;
    }
    // TODO: deliver to unknown field callback.
    while (d->ptr >= d->submsg_end) upb_delimend(d);
  }
}

void upb_decoder_onexit(upb_decoder *d) {
  if (d->dispatcher.top->is_sequence) upb_dispatch_endseq(&d->dispatcher);
  if (d->status->code == UPB_EOF && upb_dispatcher_stackempty(&d->dispatcher)) {
    // Normal end-of-file.
    upb_clearerr(d->status);
    upb_dispatch_endmsg(&d->dispatcher, d->status);
  } else {
    if (d->status->code == UPB_EOF)
      upb_seterr(d->status, UPB_ERROR, "Input ended mid-submessage.");
  }
}

void upb_decoder_decode(upb_decoder *d, upb_status *status) {
  if (sigsetjmp(d->exitjmp, 0)) {
    upb_decoder_onexit(d);
    return;
  }
  d->status = status;
  upb_pullbuf(d, true);
  upb_dispatch_startmsg(&d->dispatcher);
  while(1) { // Main loop: executed once per tag/field pair.
    while (d->ptr >= d->submsg_end) upb_delimend(d);
    upb_decoder_enterjit(d);
    // if (!d->dispatcher.top->is_packed)
    upb_fhandlers *f = upb_decode_tag(d);
    f->decode(d, f);
  }
}

static void upb_decoder_skip(void *_d, upb_dispatcher_frame *top,
                             upb_dispatcher_frame *bottom) {
  (void)top;
  upb_decoder *d = _d;
  if (bottom->end_offset == UPB_NONDELIMITED) {
    // TODO: support skipping groups.
    abort();
  }
  d->ptr = d->buf + bottom->end_offset;
}

void upb_decoder_init(upb_decoder *d, upb_handlers *handlers) {
  upb_dispatcher_init(
      &d->dispatcher, handlers, upb_decoder_skip, upb_decoder_exit2, d);
#ifdef UPB_USE_JIT_X64
  d->jit_code = NULL;
  if (d->dispatcher.handlers->should_jit) upb_decoder_makejit(d);
#endif
  d->bufstr = NULL;
  d->tmp = NULL;
  upb_string_recycle(&d->tmp);

  // Set function pointers for each field's decode function.
  for (int i = 0; i < handlers->msgs_len; i++) {
    upb_mhandlers *m = handlers->msgs[i];
    for(upb_inttable_iter i = upb_inttable_begin(&m->fieldtab); !upb_inttable_done(i);
        i = upb_inttable_next(&m->fieldtab, i)) {
      upb_fhandlers *f = upb_inttable_iter_value(i);
      switch (f->type) {
        case UPB_TYPE(INT32):    f->decode = &upb_decode_INT32;    break;
        case UPB_TYPE(INT64):    f->decode = &upb_decode_INT64;    break;
        case UPB_TYPE(UINT32):   f->decode = &upb_decode_UINT32;   break;
        case UPB_TYPE(UINT64):   f->decode = &upb_decode_UINT64;   break;
        case UPB_TYPE(FIXED32):  f->decode = &upb_decode_FIXED32;  break;
        case UPB_TYPE(FIXED64):  f->decode = &upb_decode_FIXED64;  break;
        case UPB_TYPE(SFIXED32): f->decode = &upb_decode_SFIXED32; break;
        case UPB_TYPE(SFIXED64): f->decode = &upb_decode_SFIXED64; break;
        case UPB_TYPE(BOOL):     f->decode = &upb_decode_BOOL;     break;
        case UPB_TYPE(ENUM):     f->decode = &upb_decode_ENUM;     break;
        case UPB_TYPE(DOUBLE):   f->decode = &upb_decode_DOUBLE;   break;
        case UPB_TYPE(FLOAT):    f->decode = &upb_decode_FLOAT;    break;
        case UPB_TYPE(SINT32):   f->decode = &upb_decode_SINT32;   break;
        case UPB_TYPE(SINT64):   f->decode = &upb_decode_SINT64;   break;
        case UPB_TYPE(STRING):   f->decode = &upb_decode_STRING;   break;
        case UPB_TYPE(BYTES):    f->decode = &upb_decode_STRING;   break;
        case UPB_TYPE(GROUP):    f->decode = &upb_decode_GROUP;    break;
        case UPB_TYPE(MESSAGE):  f->decode = &upb_decode_MESSAGE;  break;
        case UPB_TYPE_ENDGROUP:  f->decode = &upb_endgroup;        break;
      }
    }
  }
}

void upb_decoder_reset(upb_decoder *d, upb_bytesrc *bytesrc, void *closure) {
  upb_dispatcher_reset(&d->dispatcher, closure)->end_offset = UPB_NONDELIMITED;
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
