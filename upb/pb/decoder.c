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

typedef struct {
  uint8_t native_wire_type;
  bool is_numeric;
} upb_decoder_typeinfo;

static const upb_decoder_typeinfo upb_decoder_types[] = {
  {UPB_WIRE_TYPE_END_GROUP,   false},  // ENDGROUP
  {UPB_WIRE_TYPE_64BIT,       true},   // DOUBLE
  {UPB_WIRE_TYPE_32BIT,       true},   // FLOAT
  {UPB_WIRE_TYPE_VARINT,      true},   // INT64
  {UPB_WIRE_TYPE_VARINT,      true},   // UINT64
  {UPB_WIRE_TYPE_VARINT,      true},   // INT32
  {UPB_WIRE_TYPE_64BIT,       true},   // FIXED64
  {UPB_WIRE_TYPE_32BIT,       true},   // FIXED32
  {UPB_WIRE_TYPE_VARINT,      true},   // BOOL
  {UPB_WIRE_TYPE_DELIMITED,   false},  // STRING
  {UPB_WIRE_TYPE_START_GROUP, false},  // GROUP
  {UPB_WIRE_TYPE_DELIMITED,   false},  // MESSAGE
  {UPB_WIRE_TYPE_DELIMITED,   false},  // BYTES
  {UPB_WIRE_TYPE_VARINT,      true},   // UINT32
  {UPB_WIRE_TYPE_VARINT,      true},   // ENUM
  {UPB_WIRE_TYPE_32BIT,       true},   // SFIXED32
  {UPB_WIRE_TYPE_64BIT,       true},   // SFIXED64
  {UPB_WIRE_TYPE_VARINT,      true},   // SINT32
  {UPB_WIRE_TYPE_VARINT,      true},   // SINT64
};

/* upb_decoderplan ************************************************************/

#ifdef UPB_USE_JIT_X64
// These defines are necessary for DynASM codegen.
// See dynasm/dasm_proto.h for more info.
#define Dst_DECL upb_decoderplan *plan
#define Dst_REF (plan->dynasm)
#define Dst (plan)

// In debug mode, make DynASM do internal checks (must be defined before any
// dasm header is included.
#ifndef NDEBUG
#define DASM_CHECKS
#endif

#include "dynasm/dasm_proto.h"
#include "upb/pb/decoder_x64.h"
#endif

upb_decoderplan *upb_decoderplan_new(upb_handlers *h, bool allowjit) {
  upb_decoderplan *p = malloc(sizeof(*p));
  p->handlers = h;
  upb_handlers_ref(h);
  h->should_jit = allowjit;
#ifdef UPB_USE_JIT_X64
  p->jit_code = NULL;
  if (allowjit) upb_decoderplan_makejit(p);
#endif
  return p;
}

void upb_decoderplan_unref(upb_decoderplan *p) {
  // TODO: make truly refcounted.
  upb_handlers_unref(p->handlers);
#ifdef UPB_USE_JIT_X64
  if (p->jit_code) upb_decoderplan_freejit(p);
#endif
  free(p);
}

bool upb_decoderplan_hasjitcode(upb_decoderplan *p) {
#ifdef UPB_USE_JIT_X64
  return p->jit_code != NULL;
#else
  (void)p;
  return false;
#endif
}


/* upb_decoder ****************************************************************/

// It's unfortunate that we have to micro-manage the compiler this way,
// especially since this tuning is necessarily specific to one hardware
// configuration.  But emperically on a Core i7, performance increases 30-50%
// with these annotations.  Every instance where these appear, gcc 4.2.1 made
// the wrong decision and degraded performance in benchmarks.
#define FORCEINLINE static __attribute__((always_inline))
#define NOINLINE static __attribute__((noinline))

UPB_NORETURN static void upb_decoder_exitjmp(upb_decoder *d) {
  // Resumable decoder would back out to completed_ptr (and possibly get a
  // previous buffer).
  siglongjmp(d->exitjmp, 1);
}
UPB_NORETURN static void upb_decoder_exitjmp2(void *d) {
  upb_decoder_exitjmp(d);
}
UPB_NORETURN static void upb_decoder_abortjmp(upb_decoder *d, const char *msg) {
  upb_status_seterrliteral(&d->status, msg);
  upb_decoder_exitjmp(d);
}

/* Buffering ******************************************************************/

// We operate on one buffer at a time, which may be a subset of the currently
// loaded byteregion data.  When data for the buffer is completely gone we pull
// the next one.  When we've committed our progress we discard any previous
// buffers' regions.

static size_t upb_decoder_bufleft(upb_decoder *d) {
  assert(d->end >= d->ptr);
  return d->end - d->ptr;
}

static void upb_decoder_advance(upb_decoder *d, size_t len) {
  assert(upb_decoder_bufleft(d) >= len);
  d->ptr += len;
}

uint64_t upb_decoder_offset(upb_decoder *d) {
  return d->bufstart_ofs + (d->ptr - d->buf);
}

uint64_t upb_decoder_bufendofs(upb_decoder *d) {
  return d->bufstart_ofs + (d->end - d->buf);
}

static void upb_decoder_setmsgend(upb_decoder *d) {
  upb_dispatcher_frame *f = d->dispatcher.top;
  size_t delimlen = f->end_ofs - d->bufstart_ofs;
  size_t buflen = d->end - d->buf;
  d->delim_end = (f->end_ofs != UPB_NONDELIMITED && delimlen <= buflen) ?
      d->buf + delimlen : NULL;  // NULL if not in this buf.
  d->top_is_packed = f->is_packed;
  d->dispatch_table = &d->dispatcher.msgent->fieldtab;
}

static void upb_decoder_skiptonewbuf(upb_decoder *d, uint64_t ofs) {
  assert(ofs >= upb_decoder_offset(d));
  if (ofs > upb_byteregion_endofs(d->input))
    upb_decoder_abortjmp(d, "Unexpected EOF");
  d->buf = NULL;
  d->ptr = NULL;
  d->end = NULL;
  d->delim_end = NULL;
#ifdef UPB_USE_JIT_X64
  d->jit_end = NULL;
#endif
  d->bufstart_ofs = ofs;
}

static bool upb_trypullbuf(upb_decoder *d) {
  assert(upb_decoder_bufleft(d) == 0);
  upb_decoder_skiptonewbuf(d, upb_decoder_offset(d));
  if (upb_byteregion_available(d->input, d->bufstart_ofs) == 0) {
    switch (upb_byteregion_fetch(d->input)) {
      case UPB_BYTE_OK:
        assert(upb_byteregion_available(d->input, d->bufstart_ofs) > 0);
        break;
      case UPB_BYTE_EOF: return false;
      case UPB_BYTE_ERROR: upb_decoder_abortjmp(d, "I/O error in input");
      // Decoder resuming is not yet supported.
      case UPB_BYTE_WOULDBLOCK:
        upb_decoder_abortjmp(d, "Input returned WOULDBLOCK");
    }
  }
  size_t len;
  d->buf = upb_byteregion_getptr(d->input, d->bufstart_ofs, &len);
  assert(len > 0);
  d->ptr = d->buf;
  d->end = d->buf + len;
  upb_decoder_setmsgend(d);
#ifdef UPB_USE_JIT_X64
  // If we start parsing a value, we can parse up to 20 bytes without
  // having to bounds-check anything (2 10-byte varints).  Since the
  // JIT bounds-checks only *between* values (and for strings), the
  // JIT bails if there are not 20 bytes available.
  d->jit_end = d->end - 20;
#endif
  assert(upb_decoder_bufleft(d) > 0);
  return true;
}

static void upb_pullbuf(upb_decoder *d) {
  if (!upb_trypullbuf(d)) upb_decoder_abortjmp(d, "Unexpected EOF");
}

void upb_decoder_checkpoint(upb_decoder *d) {
  upb_byteregion_discard(d->input, upb_decoder_offset(d));
}

void upb_decoder_discardto(upb_decoder *d, uint64_t ofs) {
  if (ofs <= upb_decoder_bufendofs(d)) {
    upb_decoder_advance(d, ofs - upb_decoder_offset(d));
  } else {
    upb_decoder_skiptonewbuf(d, ofs);
  }
  upb_decoder_checkpoint(d);
}

void upb_decoder_discard(upb_decoder *d, size_t bytes) {
  upb_decoder_discardto(d, upb_decoder_offset(d) + bytes);
}


/* Decoding of wire types *****************************************************/

NOINLINE uint64_t upb_decode_varint_slow(upb_decoder *d) {
  uint8_t byte = 0x80;
  uint64_t u64 = 0;
  int bitpos;
  for(bitpos = 0; bitpos < 70 && (byte & 0x80); bitpos += 7) {
    if (upb_decoder_bufleft(d) == 0) upb_pullbuf(d);
    u64 |= ((uint64_t)(byte = *d->ptr) & 0x7F) << bitpos;
    upb_decoder_advance(d, 1);
  }
  if(bitpos == 70 && (byte & 0x80))
    upb_decoder_abortjmp(d, "Unterminated varint");
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
  if (u64 > UINT32_MAX) upb_decoder_abortjmp(d, "Unterminated 32-bit varint");
  ret = (uint32_t)u64;
  p = d->ptr;  // Turn the next line into a nop.
done:
  upb_decoder_advance(d, p - d->ptr);
  return ret;
}

// Returns true on success or false if we've hit a valid EOF.
FORCEINLINE bool upb_trydecode_varint32(upb_decoder *d, uint32_t *val) {
  if (upb_decoder_bufleft(d) == 0 &&
      upb_dispatcher_islegalend(&d->dispatcher) &&
      !upb_trypullbuf(d)) {
    return false;
  }
  *val = upb_decode_varint32(d);
  return true;
}

FORCEINLINE uint64_t upb_decode_varint(upb_decoder *d) {
  if (upb_decoder_bufleft(d) >= 10) {
    // Fast case.
    upb_decoderet r = upb_vdecode_fast(d->ptr);
    if (r.p == NULL) upb_decoder_abortjmp(d, "Unterminated varint");
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
    while (1) {
      size_t avail = UPB_MIN(upb_decoder_bufleft(d), bytes - read);
      memcpy(buf + read, d->ptr, avail);
      upb_decoder_advance(d, avail);
      read += avail;
      if (read == bytes) break;
      upb_pullbuf(d);
    }
  }
}

FORCEINLINE uint32_t upb_decode_fixed32(upb_decoder *d) {
  uint32_t u32;
  upb_decode_fixed(d, (char*)&u32, sizeof(uint32_t));
  return u32;  // TODO: proper byte swapping for big-endian machines.
}
FORCEINLINE uint64_t upb_decode_fixed64(upb_decoder *d) {
  uint64_t u64;
  upb_decode_fixed(d, (char*)&u64, sizeof(uint64_t));
  return u64;  // TODO: proper byte swapping for big-endian machines.
}

INLINE upb_byteregion *upb_decode_string(upb_decoder *d) {
  uint32_t strlen = upb_decode_varint32(d);
  uint64_t offset = upb_decoder_offset(d);
  if (offset + strlen > upb_byteregion_endofs(d->input))
    upb_decoder_abortjmp(d, "Unexpected EOF");
  upb_byteregion_reset(&d->str_byteregion, d->input, offset, strlen);
  // Could make it an option on the callback whether we fetchall() first or not.
  if (upb_byteregion_fetchall(&d->str_byteregion) != UPB_BYTE_OK)
    upb_decoder_abortjmp(d, "Couldn't fetchall() on string.");
  upb_decoder_discardto(d, offset + strlen);
  return &d->str_byteregion;
}

INLINE void upb_push_msg(upb_decoder *d, upb_fhandlers *f, uint64_t end) {
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
T(STRING,   string,  byteregion, upb_byteregion*)
#undef T

static void upb_decode_GROUP(upb_decoder *d, upb_fhandlers *f) {
  upb_push_msg(d, f, UPB_NONDELIMITED);
}
static void upb_endgroup(upb_decoder *d, upb_fhandlers *f) {
  (void)f;
  upb_dispatch_endsubmsg(&d->dispatcher);
  upb_decoder_setmsgend(d);
}
static void upb_decode_MESSAGE(upb_decoder *d, upb_fhandlers *f) {
  uint32_t len = upb_decode_varint32(d);
  upb_push_msg(d, f, upb_decoder_offset(d) + len);
}


/* The main decoding loop *****************************************************/

static void upb_decoder_checkdelim(upb_decoder *d) {
  // TODO: This doesn't work for the case that no buffer is currently loaded
  // (ie. d->buf == NULL) because delim_end is NULL even if we are at
  // end-of-delim.  Need to add a test that exercises this by putting a buffer
  // seam in the middle of the final delimited value in a proto that we skip
  // for some reason (like because it's unknown and we have no unknown field
  // handler).
  while (d->delim_end != NULL && d->ptr >= d->delim_end) {
    if (d->ptr > d->delim_end) upb_decoder_abortjmp(d, "Bad submessage end");
    if (d->dispatcher.top->is_sequence) {
      upb_dispatch_endseq(&d->dispatcher);
    } else {
      upb_dispatch_endsubmsg(&d->dispatcher);
    }
    upb_decoder_setmsgend(d);
  }
}

INLINE upb_fhandlers *upb_decode_tag(upb_decoder *d) {
  while (1) {
    uint32_t tag;
    if (!upb_trydecode_varint32(d, &tag)) return NULL;
    uint8_t wire_type = tag & 0x7;
    uint32_t fieldnum = tag >> 3;
    const upb_value *val = upb_inttable_lookup32(d->dispatch_table, fieldnum);
    upb_fhandlers *f = val ? upb_value_getptr(*val) : NULL;
    bool is_packed = false;

    if (f) {
      // Wire type check.
      if (wire_type == upb_decoder_types[f->type].native_wire_type) {
        // Wire type is ok.
      } else if ((wire_type == UPB_WIRE_TYPE_DELIMITED &&
                 upb_decoder_types[f->type].is_numeric)) {
        // Wire type is ok (and packed).
        is_packed = true;
      } else {
        f = NULL;
      }
    }

    // There are no explicit "startseq" or "endseq" markers in protobuf
    // streams, so we have to infer them by noticing when a repeated field
    // starts or ends.
    upb_dispatcher_frame *fr = d->dispatcher.top;
    if (fr->is_sequence && fr->f != f) {
      upb_dispatch_endseq(&d->dispatcher);
      upb_decoder_setmsgend(d);
      fr = d->dispatcher.top;
    }
    if (f && f->repeated && !fr->is_sequence) {
      upb_dispatcher_frame *fr2 = upb_dispatch_startseq(&d->dispatcher, f);
      if (is_packed) {
        // Packed primitive field.
        uint32_t len = upb_decode_varint32(d);
        fr2->end_ofs = upb_decoder_offset(d) + len;
        fr2->is_packed = true;
      } else {
        // Non-packed field -- this tag pertains to only a single message.
        fr2->end_ofs = fr->end_ofs;
      }
      upb_decoder_setmsgend(d);
    }

    if (f) return f;

    // Unknown field.
    if (fieldnum == 0 || fieldnum > UPB_MAX_FIELDNUMBER)
      upb_decoder_abortjmp(d, "Invalid field number");
    switch (wire_type) {
      case UPB_WIRE_TYPE_VARINT:    upb_decode_varint(d); break;
      case UPB_WIRE_TYPE_32BIT:     upb_decoder_discard(d, 4); break;
      case UPB_WIRE_TYPE_64BIT:     upb_decoder_discard(d, 8); break;
      case UPB_WIRE_TYPE_DELIMITED:
        upb_decoder_discard(d, upb_decode_varint32(d)); break;
      case UPB_WIRE_TYPE_START_GROUP:
        upb_decoder_abortjmp(d, "Can't handle unknown groups yet");
      case UPB_WIRE_TYPE_END_GROUP:
        upb_decoder_abortjmp(d, "Unmatched ENDGROUP tag");
      default:
        upb_decoder_abortjmp(d, "Invalid wire type");
    }
    // TODO: deliver to unknown field callback.
    upb_decoder_checkpoint(d);
    upb_decoder_checkdelim(d);
  }
}

upb_success_t upb_decoder_decode(upb_decoder *d) {
  assert(d->input);
  if (sigsetjmp(d->exitjmp, 0)) {
    assert(!upb_ok(&d->status));
    return UPB_ERROR;
  }
  upb_dispatch_startmsg(&d->dispatcher);
  // Prime the buf so we can hit the JIT immediately.
  upb_trypullbuf(d);
  upb_fhandlers *f = d->dispatcher.top->f;
  while(1) {
    upb_decoder_checkdelim(d);
#ifdef UPB_USE_JIT_X64
    upb_decoder_enterjit(d);
    upb_decoder_checkpoint(d);
#endif
    if (!d->top_is_packed) f = upb_decode_tag(d);
    if (!f) {
      // Sucessful EOF.  We may need to dispatch a top-level implicit frame.
      if (d->dispatcher.top->is_sequence) {
        assert(d->dispatcher.top == d->dispatcher.stack + 1);
        upb_dispatch_endseq(&d->dispatcher);
      }
      assert(d->dispatcher.top == d->dispatcher.stack);
      upb_dispatch_endmsg(&d->dispatcher, &d->status);
      return UPB_OK;
    }

    switch (f->type) {
      case UPB_TYPE_ENDGROUP:  upb_endgroup(d, f);        break;
      case UPB_TYPE(DOUBLE):   upb_decode_DOUBLE(d, f);   break;
      case UPB_TYPE(FLOAT):    upb_decode_FLOAT(d, f);    break;
      case UPB_TYPE(INT64):    upb_decode_INT64(d, f);    break;
      case UPB_TYPE(UINT64):   upb_decode_UINT64(d, f);   break;
      case UPB_TYPE(INT32):    upb_decode_INT32(d, f);    break;
      case UPB_TYPE(FIXED64):  upb_decode_FIXED64(d, f);  break;
      case UPB_TYPE(FIXED32):  upb_decode_FIXED32(d, f);  break;
      case UPB_TYPE(BOOL):     upb_decode_BOOL(d, f);     break;
      case UPB_TYPE(STRING):
      case UPB_TYPE(BYTES):    upb_decode_STRING(d, f);   break;
      case UPB_TYPE(GROUP):    upb_decode_GROUP(d, f);    break;
      case UPB_TYPE(MESSAGE):  upb_decode_MESSAGE(d, f);  break;
      case UPB_TYPE(UINT32):   upb_decode_UINT32(d, f);   break;
      case UPB_TYPE(ENUM):     upb_decode_ENUM(d, f);     break;
      case UPB_TYPE(SFIXED32): upb_decode_SFIXED32(d, f); break;
      case UPB_TYPE(SFIXED64): upb_decode_SFIXED64(d, f); break;
      case UPB_TYPE(SINT32):   upb_decode_SINT32(d, f);   break;
      case UPB_TYPE(SINT64):   upb_decode_SINT64(d, f);   break;
      case UPB_TYPE_NONE: assert(false); break;
    }
    upb_decoder_checkpoint(d);
  }
}

void upb_decoder_init(upb_decoder *d) {
  upb_status_init(&d->status);
  upb_dispatcher_init(&d->dispatcher, &d->status, &upb_decoder_exitjmp2, d);
  d->plan = NULL;
  d->input = NULL;
}

void upb_decoder_resetplan(upb_decoder *d, upb_decoderplan *p, int msg_offset) {
  assert(msg_offset >= 0);
  assert(msg_offset < p->handlers->msgs_len);
  d->plan = p;
  d->msg_offset = msg_offset;
  d->input = NULL;
}

void upb_decoder_resetinput(upb_decoder *d, upb_byteregion *input,
                            void *closure) {
  assert(d->plan);
  upb_dispatcher_frame *f =
      upb_dispatcher_reset(&d->dispatcher, closure, d->plan->handlers->msgs[0]);
  upb_status_clear(&d->status);
  f->end_ofs = UPB_NONDELIMITED;
  d->input = input;
  d->str_byteregion.bytesrc = input->bytesrc;

  // Protect against assert in skiptonewbuf().
  d->bufstart_ofs = 0;
  d->ptr = NULL;
  d->buf = NULL;
  upb_decoder_skiptonewbuf(d, upb_byteregion_startofs(input));
}

void upb_decoder_uninit(upb_decoder *d) {
  upb_dispatcher_uninit(&d->dispatcher);
  upb_status_uninit(&d->status);
}
