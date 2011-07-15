/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2008-2011 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 */

#include <inttypes.h>
#include <stddef.h>
#include <stdlib.h>
#include "upb/bytestream.h"
#include "upb/msg.h"
#include "upb/pb/decoder.h"
#include "upb/pb/varint.h"

// Used for frames that have no specific end offset: groups, repeated primitive
// fields inside groups, and the top-level message.
#define UPB_NONDELIMITED UINT32_MAX

#ifdef UPB_USE_JIT_X64
#define Dst_DECL upb_decoder *d
#define Dst_REF (d->dynasm)
#define Dst (d)
#include "dynasm/dasm_proto.h"
#include "upb/pb/decoder_x86.h"
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
static void upb_decoder_abort(upb_decoder *d, const char *msg) {
  upb_status_setf(d->status, UPB_ERROR, msg);
  upb_decoder_exit(d);
}

/* Decoding/Buffering of wire types *******************************************/

static size_t upb_decoder_bufleft(upb_decoder *d) { return d->end - d->ptr; }
static void upb_decoder_advance(upb_decoder *d, size_t len) {
  assert((size_t)(d->end - d->ptr) >= len);
  d->ptr += len;
}

size_t upb_decoder_offset(upb_decoder *d) {
  size_t offset = d->bufstart_ofs;
  if (d->ptr) offset += (d->ptr - d->buf);
  return offset;
}

static void upb_decoder_setmsgend(upb_decoder *d) {
  upb_dispatcher_frame *f = d->dispatcher.top;
  size_t delimlen = f->end_ofs - d->bufstart_ofs;
  size_t buflen = d->end - d->buf;
  if (f->end_ofs != UINT64_MAX && delimlen <= buflen) {
    d->delim_end = (uintptr_t)(d->buf + delimlen);
  } else {
    // Buffers must not run up against the end of memory.
    assert((uintptr_t)d->end < UINTPTR_MAX);
    d->delim_end = UINTPTR_MAX;
  }
}

// Pulls the next buffer from the bytesrc.  Should be called only when the
// current buffer is completely empty.
static bool upb_trypullbuf(upb_decoder *d) {
  assert(upb_decoder_bufleft(d) == 0);
  if (d->bufend_ofs == d->refend_ofs) {
    d->refend_ofs += upb_bytesrc_fetch(d->bytesrc, d->refend_ofs, d->status);
    if (!upb_ok(d->status)) {
      d->ptr = NULL;
      d->end = NULL;
      if (upb_iseof(d->status)) return false;
      upb_decoder_exit(d);
    }
  }
  d->bufstart_ofs = d->bufend_ofs;
  size_t len;
  d->buf = upb_bytesrc_getptr(d->bytesrc, d->bufstart_ofs, &len);
  assert(len > 0);
  d->bufend_ofs = d->bufstart_ofs + len;
  d->ptr = d->buf;
  d->end = d->buf + len;
#ifdef UPB_USE_JIT_X64
  d->jit_end = d->end - 20;
#endif
  upb_decoder_setmsgend(d);
  return true;
}

static void upb_pullbuf(upb_decoder *d) {
  if (!upb_trypullbuf(d)) upb_decoder_abort(d, "Unexpected EOF");
}

void upb_decoder_commit(upb_decoder *d) {
  d->completed_ptr = d->ptr;
  if (d->refstart_ofs < d->bufstart_ofs) {
    // Drop our ref on the previous buf's region.
    upb_bytesrc_refregion(d->bytesrc, d->bufstart_ofs, d->refend_ofs);
    upb_bytesrc_unrefregion(d->bytesrc, d->refstart_ofs, d->refend_ofs);
    d->refstart_ofs = d->bufstart_ofs;
  }
}

NOINLINE uint64_t upb_decode_varint_slow(upb_decoder *d) {
  uint8_t byte = 0x80;
  uint64_t u64 = 0;
  int bitpos;
  const char *ptr = d->ptr;
  for(bitpos = 0; bitpos < 70 && (byte & 0x80); bitpos += 7) {
    if (upb_decoder_bufleft(d) == 0) {
      upb_pullbuf(d);
      ptr = d->ptr;
    }
    u64 |= ((uint64_t)(byte = *ptr++) & 0x7F) << bitpos;
  }
  if(bitpos == 70 && (byte & 0x80)) upb_decoder_abort(d, "Unterminated varint");
  return u64;
}

// For tags and delimited lengths, which must be <=32bit and are usually small.
FORCEINLINE uint32_t upb_decode_varint32(upb_decoder *d) {
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
  u64 = upb_decode_varint_slow(d);
  if (u64 > 0xffffffff) upb_decoder_abort(d, "Unterminated 32-bit varint");
  ret = (uint32_t)u64;
  p = d->ptr;  // Turn the next line into a nop.
done:
  upb_decoder_advance(d, p - d->ptr);
  return ret;
}

FORCEINLINE bool upb_trydecode_varint32(upb_decoder *d, uint32_t *val) {
  if (upb_decoder_bufleft(d) == 0) {
    // Check for our two normal end-of-message conditions.
    if (d->bufend_ofs == d->end_ofs) return false;
    if (!upb_trypullbuf(d)) return false;
  }
  *val = upb_decode_varint32(d);
  return true;
}

FORCEINLINE uint64_t upb_decode_varint(upb_decoder *d) {
  if (upb_decoder_bufleft(d) >= 10) {
    // Fast case.
    upb_decoderet r = upb_vdecode_fast(d->ptr);
    if (r.p == NULL) upb_decoder_abort(d, "Unterminated varint");
    upb_decoder_advance(d, r.p - d->ptr);
    return r.val;
  } else if (upb_decoder_bufleft(d) > 0) {
    // Intermediate case -- worth it?
    char tmpbuf[10];
    memset(tmpbuf, 0x80, 10);
    memcpy(tmpbuf, d->ptr, upb_decoder_bufleft(d));
    upb_decoderet r = upb_vdecode_fast(tmpbuf);
    if (r.p != NULL) {
      upb_decoder_advance(d, r.p - tmpbuf);
      return r.val;
    }
  }
  // Slow case -- varint spans buffer seam.
  return upb_decode_varint_slow(d);
}

FORCEINLINE void upb_decode_fixed(upb_decoder *d, char *buf, size_t bytes) {
  if (upb_decoder_bufleft(d) >= bytes) {
    // Fast case.
    memcpy(buf, d->ptr, bytes);
    upb_decoder_advance(d, bytes);
  } else {
    // Slow case.
    size_t read = 0;
    while (read < bytes) {
      size_t avail = upb_decoder_bufleft(d);
      memcpy(buf + read, d->ptr, avail);
      upb_decoder_advance(d, avail);
      read += avail;
    }
  }
}

FORCEINLINE uint32_t upb_decode_fixed32(upb_decoder *d) {
  uint32_t u32;
  upb_decode_fixed(d, (char*)&u32, sizeof(uint32_t));
  return u32;  // TODO: proper byte swapping
}
FORCEINLINE uint64_t upb_decode_fixed64(upb_decoder *d) {
  uint64_t u64;
  upb_decode_fixed(d, (char*)&u64, sizeof(uint64_t));
  return u64;  // TODO: proper byte swapping
}

INLINE upb_strref *upb_decode_string(upb_decoder *d) {
  uint32_t strlen = upb_decode_varint32(d);
  d->strref.stream_offset = upb_decoder_offset(d);
  d->strref.len = strlen;
  if (upb_decoder_bufleft(d) == 0) upb_pullbuf(d);
  if (upb_decoder_bufleft(d) >= strlen) {
    // Fast case.
    d->strref.ptr = d->ptr;
    upb_decoder_advance(d, strlen);
  } else {
    // Slow case.
    while (1) {
      size_t consume = UPB_MIN(upb_decoder_bufleft(d), strlen);
      upb_decoder_advance(d, consume);
      strlen -= consume;
      if (strlen == 0) break;
      upb_pullbuf(d);
    }
  }
  return &d->strref;
}

INLINE void upb_push(upb_decoder *d, upb_fhandlers *f, uint32_t end) {
  upb_dispatch_startsubmsg(&d->dispatcher, f)->end_ofs = end;
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
T(STRING,   string,  strref, upb_strref*)

static void upb_decode_GROUP(upb_decoder *d, upb_fhandlers *f) {
  upb_push(d, f, UPB_NONDELIMITED);
}
static void upb_endgroup(upb_decoder *d, upb_fhandlers *f) {
  (void)f;
  upb_dispatch_endsubmsg(&d->dispatcher);
  upb_decoder_setmsgend(d);
}
static void upb_decode_MESSAGE(upb_decoder *d, upb_fhandlers *f) {
  upb_push(d, f, upb_decode_varint32(d) + (d->ptr - d->buf));
}


/* The main decoding loop *****************************************************/

static void upb_decoder_checkdelim(upb_decoder *d) {
  while ((uintptr_t)d->ptr >= d->delim_end) {
    if ((uintptr_t)d->ptr > d->delim_end)
      upb_decoder_abort(d, "Bad submessage end");

    if (d->dispatcher.top->is_sequence) {
      upb_dispatch_endseq(&d->dispatcher);
    } else {
      upb_dispatch_endsubmsg(&d->dispatcher);
    }
    upb_decoder_setmsgend(d);
  }
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
    uint32_t tag;
    if (!upb_trydecode_varint32(d, &tag)) return NULL;
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
      uint32_t end = d->dispatcher.top->end_ofs;
      upb_dispatch_startseq(&d->dispatcher, f)->end_ofs = end;
      upb_decoder_setmsgend(d);
    }
    if (f) return f;

    // Unknown field.
    switch (tag & 0x7) {
      case UPB_WIRE_TYPE_VARINT:    upb_decode_varint(d); break;
      case UPB_WIRE_TYPE_32BIT:     upb_decoder_advance(d, 4); break;
      case UPB_WIRE_TYPE_64BIT:     upb_decoder_advance(d, 8); break;
      case UPB_WIRE_TYPE_DELIMITED:
        upb_decoder_advance(d, upb_decode_varint32(d)); break;
      default:
        upb_decoder_abort(d, "Invavlid wire type");
    }
    // TODO: deliver to unknown field callback.
    upb_decoder_commit(d);
    upb_decoder_checkdelim(d);
  }
}

void upb_decoder_onexit(upb_decoder *d) {
  if (d->dispatcher.top->is_sequence) upb_dispatch_endseq(&d->dispatcher);
  if (d->status->code == UPB_EOF && upb_dispatcher_stackempty(&d->dispatcher)) {
    // Normal end-of-file.
    upb_status_clear(d->status);
    upb_dispatch_endmsg(&d->dispatcher, d->status);
  } else {
    if (d->status->code == UPB_EOF)
      upb_status_setf(d->status, UPB_ERROR, "Input ended mid-submessage.");
  }
}

void upb_decoder_decode(upb_decoder *d, upb_status *status) {
  if (sigsetjmp(d->exitjmp, 0)) {
    upb_decoder_onexit(d);
    return;
  }
  d->status = status;
  upb_dispatch_startmsg(&d->dispatcher);
  while(1) { // Main loop: executed once per tag/field pair.
    upb_decoder_checkdelim(d);
    upb_decoder_enterjit(d);
    // if (!d->dispatcher.top->is_packed)
    upb_fhandlers *f = upb_decode_tag(d);
    if (!f) upb_decoder_exit2(d);
    f->decode(d, f);
    upb_decoder_commit(d);
  }
}

static void upb_decoder_skip(void *_d, upb_dispatcher_frame *top,
                             upb_dispatcher_frame *bottom) {
  (void)top;
  (void)bottom;
  (void)_d;
#if 0
  upb_decoder *d = _d;
  // TODO
  if (bottom->end_offset == UPB_NONDELIMITED) {
    // TODO: support skipping groups.
    abort();
  }
  d->ptr = d->buf.ptr + bottom->end_offset;
#endif
}

void upb_decoder_initforhandlers(upb_decoder *d, upb_handlers *handlers) {
  upb_dispatcher_init(
      &d->dispatcher, handlers, upb_decoder_skip, upb_decoder_exit2, d);
#ifdef UPB_USE_JIT_X64
  d->jit_code = NULL;
  if (d->dispatcher.handlers->should_jit) upb_decoder_makejit(d);
#endif
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

void upb_decoder_initformsgdef(upb_decoder *d, upb_msgdef *m) {
  upb_handlers *h = upb_handlers_new();
  upb_accessors_reghandlers(h, m);
  upb_decoder_initforhandlers(d, h);
  upb_handlers_unref(h);
}

void upb_decoder_reset(upb_decoder *d, upb_bytesrc *bytesrc, uint64_t start_ofs,
                       uint64_t end_ofs, void *closure) {
  upb_dispatcher_frame *f = upb_dispatcher_reset(&d->dispatcher, closure);
  f->end_ofs = end_ofs;
  d->end_ofs = end_ofs;
  d->refstart_ofs = start_ofs;
  d->refend_ofs = start_ofs;
  d->bufstart_ofs = start_ofs;
  d->bufend_ofs = start_ofs;
  d->bytesrc = bytesrc;
  d->buf = NULL;
  d->ptr = NULL;
  d->end = NULL;  // Force a buffer pull.
#ifdef UPB_USE_JIT_X64
  d->jit_end = NULL;
#endif
  d->delim_end = UINTPTR_MAX;  // But don't let end-of-message get triggered.
  d->strref.bytesrc = bytesrc;
}

void upb_decoder_uninit(upb_decoder *d) {
#ifdef UPB_USE_JIT_X64
  if (d->dispatcher.handlers->should_jit) upb_decoder_freejit(d);
#endif
  upb_dispatcher_uninit(&d->dispatcher);
}
