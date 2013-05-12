/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2008-2011 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 */

#include <inttypes.h>
#include <setjmp.h>
#include <stddef.h>
#include <stdlib.h>
#include "upb/bytestream.h"
#include "upb/pb/decoder.h"
#include "upb/pb/varint.h"

#define UPB_NONDELIMITED (0xffffffffffffffffULL)

/* upb_pbdecoder ****************************************************************/

struct dasm_State;

typedef struct {
  const upb_fielddef *f;
  uint64_t end_ofs;
  uint32_t group_fieldnum;  // UINT32_MAX for non-groups.
  bool is_sequence;   // frame represents seq or submsg/str? (f might be both).
  bool is_packed;     // true for packed primitive sequences.
} frame;

struct upb_pbdecoder {
  // Where we push parsed data (not owned).
  upb_sink *sink;

  // Current input buffer and its stream offset.
  const char *buf, *ptr, *end, *checkpoint;
  uint64_t bufstart_ofs;

  // Buffer for residual bytes not parsed from the previous buffer.
  char residual[16];
  char *residual_end;

  // Stores the user buffer passed to our decode function.
  const char *buf_param;
  size_t size_param;

  // Equal to size_param while we are in the residual buf, 0 otherwise.
  size_t userbuf_remaining;

  // Used to temporarily store the return value before calling longjmp().
  size_t ret;

  // End of the delimited region, relative to ptr, or NULL if not in this buf.
  const char *delim_end;

#ifdef UPB_USE_JIT_X64
  // For JIT, which doesn't do bounds checks in the middle of parsing a field.
  const char *jit_end, *effective_end;  // == MIN(jit_end, delim_end)

  // Used momentarily by the generated code to store a value while a user
  // function is called.
  uint32_t tmp_len;

  const void *saved_rbp;
#endif

  // Our internal stack.
  frame *top, *limit;
  frame stack[UPB_MAX_NESTING];

  // For exiting the decoder on error.
  jmp_buf exitjmp;
};

typedef struct {
  // The top-level handlers that this plan calls into.  We own a ref.
  const upb_handlers *dest_handlers;

#ifdef UPB_USE_JIT_X64
  // JIT-generated machine code (else NULL).
  char *jit_code;
  size_t jit_size;
  char *debug_info;

  // For storing upb_jitmsginfo, which contains per-msg runtime data needed
  // by the JIT.
  // Maps upb_handlers* -> upb_jitmsginfo.
  upb_inttable msginfo;

  // The following members are used only while the JIT is being built.

  // This pointer is allocated by dasm_init() and freed by dasm_free().
  struct dasm_State *dynasm;

  // For storing pclabel bases while we are building the JIT.
  // Maps (upb_handlers* or upb_fielddef*) -> int32 pclabel_base
  upb_inttable pclabels;

  // This is not the same as len(pclabels) because the table only contains base
  // offsets for each def, but each def can have many pclabels.
  uint32_t pclabel_count;
#endif
} decoderplan;

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

static upb_selector_t getselector(const upb_fielddef *f,
                                  upb_handlertype_t type) {
  upb_selector_t selector;
  bool ok = upb_getselector(f, type, &selector);
  UPB_ASSERT_VAR(ok, ok);
  return selector;
}


/* decoderplan ****************************************************************/

#ifdef UPB_USE_JIT_X64
// These defines are necessary for DynASM codegen.
// See dynasm/dasm_proto.h for more info.
#define Dst_DECL decoderplan *plan
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

void freeplan(void *_p) {
  decoderplan *p = _p;
  upb_handlers_unref(p->dest_handlers, p);
#ifdef UPB_USE_JIT_X64
  if (p->jit_code) upb_decoderplan_freejit(p);
#endif
  free(p);
}

static decoderplan *getdecoderplan(const upb_handlers *h) {
  if (upb_handlers_frametype(h) != upb_pbdecoder_getframetype())
    return NULL;
  upb_selector_t sel;
  if (!upb_getselector(UPB_BYTESTREAM_BYTES, UPB_HANDLER_STRING, &sel))
    return NULL;
  return upb_handlers_gethandlerdata(h, sel);
}

bool upb_pbdecoder_isdecoder(const upb_handlers *h) {
  return getdecoderplan(h) != NULL;
}

bool upb_pbdecoder_hasjitcode(const upb_handlers *h) {
#ifdef UPB_USE_JIT_X64
  decoderplan *p = getdecoderplan(h);
  if (!p) return false;
  return p->jit_code != NULL;
#else
  UPB_UNUSED(h);
  return false;
#endif
}

const upb_handlers *upb_pbdecoder_getdesthandlers(const upb_handlers *h) {
  decoderplan *p = getdecoderplan(h);
  if (!p) return NULL;
  return p->dest_handlers;
}


/* upb_pbdecoder ****************************************************************/

static bool in_residual_buf(const upb_pbdecoder *d, const char *p);

// It's unfortunate that we have to micro-manage the compiler this way,
// especially since this tuning is necessarily specific to one hardware
// configuration.  But emperically on a Core i7, performance increases 30-50%
// with these annotations.  Every instance where these appear, gcc 4.2.1 made
// the wrong decision and degraded performance in benchmarks.
#define FORCEINLINE static inline __attribute__((always_inline))
#define NOINLINE static __attribute__((noinline))

static upb_status *decoder_status(upb_pbdecoder *d) {
  // TODO(haberman): encapsulate this access to pipeline->status, but not sure
  // exactly what that interface should look like.
  return &d->sink->pipeline_->status_;
}

UPB_NORETURN static void exitjmp(upb_pbdecoder *d) {
  _longjmp(d->exitjmp, 1);
}

UPB_NORETURN static void abortjmp(upb_pbdecoder *d, const char *msg) {
  d->ret = in_residual_buf(d, d->checkpoint) ? 0 : (d->checkpoint - d->buf);
  upb_status_seterrliteral(decoder_status(d), msg);
  exitjmp(d);
}

/* Buffering ******************************************************************/

// We operate on one buffer at a time, which is either the user's buffer passed
// to our "decode" callback or some residual bytes from the previous buffer.

// How many bytes can be safely read from d->ptr.
static size_t bufleft(upb_pbdecoder *d) {
  assert(d->end >= d->ptr);
  return d->end - d->ptr;
}

// Overall offset of d->ptr.
uint64_t offset(const upb_pbdecoder *d) {
  return d->bufstart_ofs + (d->ptr - d->buf);
}

// Advances d->ptr.
static void advance(upb_pbdecoder *d, size_t len) {
  assert(bufleft(d) >= len);
  d->ptr += len;
}

// Commits d->ptr progress; should be called when an entire atomic value
// (ie tag+value) has been successfully consumed.
static void checkpoint(upb_pbdecoder *d) {
  d->checkpoint = d->ptr;
}

static bool in_buf(const char *p, const char *buf, const char *end) {
  return p >= buf && p <= end;
}

static bool in_residual_buf(const upb_pbdecoder *d, const char *p) {
  return in_buf(p, d->residual, d->residual_end);
}

// Calculates the delim_end value, which represents a combination of the
// current buffer and the stack, so must be called whenever either is updated.
static void set_delim_end(upb_pbdecoder *d) {
  frame *f = d->top;
  size_t delimlen = f->end_ofs - d->bufstart_ofs;
  size_t buflen = d->end - d->buf;
  d->delim_end = (f->end_ofs != UPB_NONDELIMITED && delimlen <= buflen) ?
      d->buf + delimlen : NULL;  // NULL if not in this buf.
}

static void switchtobuf(upb_pbdecoder *d, const char *buf, const char *end) {
  d->ptr = buf;
  d->buf = buf;
  d->end = end;
  set_delim_end(d);
#ifdef UPB_USE_JIT_X64
  // If we start parsing a value, we can parse up to 20 bytes without
  // having to bounds-check anything (2 10-byte varints).  Since the
  // JIT bounds-checks only *between* values (and for strings), the
  // JIT bails if there are not 20 bytes available.
  d->jit_end = d->end - 20;
#endif
}

static void suspendjmp(upb_pbdecoder *d) {
  switchtobuf(d, d->residual, d->residual_end);
  exitjmp(d);
}

static void advancetobuf(upb_pbdecoder *d, const char *buf, size_t len) {
  assert(d->ptr == d->end);
  d->bufstart_ofs += (d->ptr - d->buf);
  switchtobuf(d, buf, buf + len);
}

static void skip(upb_pbdecoder *d, size_t bytes) {
  size_t avail = bufleft(d);
  size_t total_avail = avail + d->userbuf_remaining;
  if (avail >= bytes) {
    // Skipped data is all in current buffer.
    advance(d, bytes);
  } else if (total_avail >= bytes) {
    // Skipped data is all in residual buf and param buffer.
    assert(in_residual_buf(d, d->ptr));
    advance(d, avail);
    advancetobuf(d, d->buf_param, d->size_param);
    d->userbuf_remaining = 0;
    advance(d, bytes - avail);
  } else {
    // Skipped data extends beyond currently available buffers.
    // TODO: we need to do a checkdelim() equivalent that pops any frames that
    // we just skipped past.
    d->bufstart_ofs = offset(d) + bytes;
    d->residual_end = d->residual;
    d->ret += bytes - total_avail;
    suspendjmp(d);
  }
}

static void consumebytes(upb_pbdecoder *d, void *buf, size_t bytes) {
  assert(bytes <= bufleft(d));
  memcpy(buf, d->ptr, bytes);
  advance(d, bytes);
}

NOINLINE void getbytes_slow(upb_pbdecoder *d, void *buf, size_t bytes) {
  const size_t avail = bufleft(d);
  if (avail + d->userbuf_remaining >= bytes) {
    // Remaining residual buffer and param buffer together can satisfy.
    // (We are only called from getbytes() which has already verified that
    // the current buffer alone cannot satisfy).
    assert(in_residual_buf(d, d->ptr));
    consumebytes(d, buf, avail);
    advancetobuf(d, d->buf_param, d->size_param);
    consumebytes(d, buf + avail, bytes - avail);
    d->userbuf_remaining = 0;
  } else {
    // There is not enough remaining data, save residual bytes (if any)
    // starting at the last committed checkpoint and exit.
    if (in_buf(d->checkpoint, d->buf_param, d->buf_param + d->size_param)) {
      // Checkpoint was in user buf; old residual bytes not needed.
      d->ptr = d->checkpoint;
      size_t save = bufleft(d);
      assert(save <= sizeof(d->residual));
      memcpy(d->residual, d->ptr, save);
      d->residual_end = d->residual + save;
      d->bufstart_ofs = offset(d);
    } else {
      // Checkpoint was in residual buf; append user byte(s) to residual buf.
      assert(d->checkpoint == d->residual);
      assert((d->residual_end - d->residual) + d->size_param <=
             sizeof(d->residual));
      if (!in_residual_buf(d, d->ptr)) {
        d->bufstart_ofs -= (d->residual_end - d->residual);
      }
      memcpy(d->residual_end, d->buf_param, d->size_param);
      d->residual_end += d->size_param;
    }
    suspendjmp(d);
  }
}

FORCEINLINE void getbytes(upb_pbdecoder *d, void *buf, size_t bytes) {
  if (bufleft(d) >= bytes) {
    // Buffer has enough data to satisfy.
    consumebytes(d, buf, bytes);
  } else {
    getbytes_slow(d, buf, bytes);
  }
}

FORCEINLINE uint8_t getbyte(upb_pbdecoder *d) {
  uint8_t byte;
  getbytes(d, &byte, 1);
  return byte;
}


/* Decoding of wire types *****************************************************/

NOINLINE uint64_t decode_varint_slow(upb_pbdecoder *d) {
  uint8_t byte = 0x80;
  uint64_t u64 = 0;
  int bitpos;
  for(bitpos = 0; bitpos < 70 && (byte & 0x80); bitpos += 7) {
    u64 |= ((uint64_t)((byte = getbyte(d)) & 0x7F)) << bitpos;
  }
  if(bitpos == 70 && (byte & 0x80))
    abortjmp(d, "Unterminated varint");
  return u64;
}

NOINLINE uint32_t decode_v32_slow(upb_pbdecoder *d) {
  uint64_t u64 = decode_varint_slow(d);
  if (u64 > UINT32_MAX) abortjmp(d, "Unterminated 32-bit varint");
  return (uint32_t)u64;
}

// For tags and delimited lengths, which must be <=32bit and are usually small.
FORCEINLINE uint32_t decode_v32(upb_pbdecoder *d) {
  // Nearly all will be either 1 byte (1-16) or 2 bytes (17-2048).
  if (bufleft(d) >= 2) {
    uint32_t ret = d->ptr[0] & 0x7f;
    if ((d->ptr[0] & 0x80) == 0) {
      advance(d, 1);
      return ret;
    }
    ret |= (d->ptr[1] & 0x7f) << 7;
    if ((d->ptr[1] & 0x80) == 0) {
      advance(d, 2);
      return ret;
    }
  }
  return decode_v32_slow(d);
}

FORCEINLINE uint64_t decode_varint(upb_pbdecoder *d) {
  if (bufleft(d) >= 10) {
    // Fast case.
    upb_decoderet r = upb_vdecode_fast(d->ptr);
    if (r.p == NULL) abortjmp(d, "Unterminated varint");
    advance(d, r.p - d->ptr);
    return r.val;
  } else {
    // Slow case -- varint spans buffer seam.
    return decode_varint_slow(d);
  }
}

FORCEINLINE uint32_t decode_fixed32(upb_pbdecoder *d) {
  uint32_t u32;
  getbytes(d, &u32, 4);
  return u32;  // TODO: proper byte swapping for big-endian machines.
}

FORCEINLINE uint64_t decode_fixed64(upb_pbdecoder *d) {
  uint64_t u64;
  getbytes(d, &u64, 8);
  return u64;  // TODO: proper byte swapping for big-endian machines.
}

static void push(upb_pbdecoder *d, const upb_fielddef *f, bool is_sequence,
                 bool is_packed, int32_t group_fieldnum, uint64_t end) {
  frame *fr = d->top + 1;
  if (fr >= d->limit) abortjmp(d, "Nesting too deep.");
  fr->f = f;
  fr->is_sequence = is_sequence;
  fr->is_packed = is_packed;
  fr->end_ofs = end;
  fr->group_fieldnum = group_fieldnum;
  d->top = fr;
  set_delim_end(d);
}

static void push_msg(upb_pbdecoder *d, const upb_fielddef *f, uint64_t end) {
  if (!upb_sink_startsubmsg(d->sink, getselector(f, UPB_HANDLER_STARTSUBMSG)))
    abortjmp(d, "startsubmsg failed.");
  int32_t group_fieldnum = (end == UPB_NONDELIMITED) ?
      (int32_t)upb_fielddef_number(f) : -1;
  push(d, f, false, false, group_fieldnum, end);
}

static void push_seq(upb_pbdecoder *d, const upb_fielddef *f, bool packed,
                     uint64_t end_ofs) {
  if (!upb_sink_startseq(d->sink, getselector(f, UPB_HANDLER_STARTSEQ)))
    abortjmp(d, "startseq failed.");
  push(d, f, true, packed, -1, end_ofs);
}

static void push_str(upb_pbdecoder *d, const upb_fielddef *f, size_t len,
                     uint64_t end) {
  if (!upb_sink_startstr(d->sink, getselector(f, UPB_HANDLER_STARTSTR), len))
    abortjmp(d, "startseq failed.");
  push(d, f, false, false, -1, end);
}

static void pop_submsg(upb_pbdecoder *d) {
  upb_sink_endsubmsg(d->sink, getselector(d->top->f, UPB_HANDLER_ENDSUBMSG));
  d->top--;
  set_delim_end(d);
}

static void pop_seq(upb_pbdecoder *d) {
  upb_sink_endseq(d->sink, getselector(d->top->f, UPB_HANDLER_ENDSEQ));
  d->top--;
  set_delim_end(d);
}

static void pop_string(upb_pbdecoder *d) {
  upb_sink_endstr(d->sink, getselector(d->top->f, UPB_HANDLER_ENDSTR));
  d->top--;
  set_delim_end(d);
}

static void checkdelim(upb_pbdecoder *d) {
  while (d->delim_end && d->ptr >= d->delim_end) {
    // TODO(haberman): not sure what to do about this; if we detect this error
    // we can possibly violate the promise that errors are always signaled by a
    // short "parsed byte" count (because all bytes might have been successfully
    // parsed prior to detecting this error).
    // if (d->ptr > d->delim_end) abortjmp(d, "Bad submessage end");
    if (d->top->is_sequence) {
      pop_seq(d);
    } else {
      pop_submsg(d);
    }
  }
}


/* Decoding of .proto types ***************************************************/

// Technically, we are losing data if we see a 32-bit varint that is not
// properly sign-extended.  We could detect this and error about the data loss,
// but proto2 does not do this, so we pass.

#define T(type, sel, wt, name, convfunc) \
  static void decode_ ## type(upb_pbdecoder *d, const upb_fielddef *f) { \
    upb_sink_put ## name(d->sink, getselector(f, UPB_HANDLER_ ## sel), \
                         (convfunc)(decode_ ## wt(d))); \
  } \

static double  upb_asdouble(uint64_t n) { double d; memcpy(&d, &n, 8); return d; }
static float   upb_asfloat(uint32_t n)  { float  f; memcpy(&f, &n, 4); return f; }

T(INT32,    INT32,  varint,  int32,  int32_t)
T(INT64,    INT64,  varint,  int64,  int64_t)
T(UINT32,   UINT32, varint,  uint32, uint32_t)
T(UINT64,   UINT64, varint,  uint64, uint64_t)
T(FIXED32,  UINT32, fixed32, uint32, uint32_t)
T(FIXED64,  UINT64, fixed64, uint64, uint64_t)
T(SFIXED32, INT32,  fixed32, int32,  int32_t)
T(SFIXED64, INT64,  fixed64, int64,  int64_t)
T(BOOL,     BOOL,   varint,  bool,   bool)
T(ENUM,     INT32,  varint,  int32,  int32_t)
T(DOUBLE,   DOUBLE, fixed64, double, upb_asdouble)
T(FLOAT,    FLOAT,  fixed32, float,  upb_asfloat)
T(SINT32,   INT32,  varint,  int32,  upb_zzdec_32)
T(SINT64,   INT64,  varint,  int64,  upb_zzdec_64)
#undef T

static void decode_GROUP(upb_pbdecoder *d, const upb_fielddef *f) {
  push_msg(d, f, UPB_NONDELIMITED);
}

static void decode_MESSAGE(upb_pbdecoder *d, const upb_fielddef *f) {
  uint32_t len = decode_v32(d);
  push_msg(d, f, offset(d) + len);
}

static void decode_STRING(upb_pbdecoder *d, const upb_fielddef *f) {
  uint32_t strlen = decode_v32(d);
  if (strlen <= bufleft(d)) {
    upb_sink_startstr(d->sink, getselector(f, UPB_HANDLER_STARTSTR), strlen);
    if (strlen)
      upb_sink_putstring(d->sink, getselector(f, UPB_HANDLER_STRING),
                         d->ptr, strlen);
    upb_sink_endstr(d->sink, getselector(f, UPB_HANDLER_ENDSTR));
    advance(d, strlen);
  } else {
    // Buffer ends in the middle of the string; need to push a decoder frame
    // for it.
    push_str(d, f, strlen, offset(d) + strlen);
    if (bufleft(d)) {
      upb_sink_putstring(d->sink, getselector(f, UPB_HANDLER_STRING),
                         d->ptr, bufleft(d));
      advance(d, bufleft(d));
    }
    d->bufstart_ofs = offset(d);
    d->residual_end = d->residual;
    suspendjmp(d);
  }
}


/* The main decoding loop *****************************************************/

static const upb_fielddef *decode_tag(upb_pbdecoder *d) {
  while (1) {
    uint32_t tag = decode_v32(d);
    uint8_t wire_type = tag & 0x7;
    uint32_t fieldnum = tag >> 3; const upb_fielddef *f = NULL;
    const upb_handlers *h = upb_sinkframe_handlers(upb_sink_top(d->sink));
    f = upb_msgdef_itof(upb_handlers_msgdef(h), fieldnum);
    bool packed = false;

    if (f) {
      // Wire type check.
      upb_descriptortype_t type = upb_fielddef_descriptortype(f);
      if (wire_type == upb_decoder_types[type].native_wire_type) {
        // Wire type is ok.
      } else if ((wire_type == UPB_WIRE_TYPE_DELIMITED &&
                 upb_decoder_types[type].is_numeric)) {
        // Wire type is ok (and packed).
        packed = true;
      } else {
        f = NULL;
      }
    }

    // There are no explicit "startseq" or "endseq" markers in protobuf
    // streams, so we have to infer them by noticing when a repeated field
    // starts or ends.
    frame *fr = d->top;
    if (fr->is_sequence && fr->f != f) {
      pop_seq(d);
      fr = d->top;
    }

    if (f && upb_fielddef_isseq(f) && !fr->is_sequence) {
      if (packed) {
        uint32_t len = decode_v32(d);
        push_seq(d, f, true, offset(d) + len);
        checkpoint(d);
      } else {
        push_seq(d, f, false, fr->end_ofs);
      }
    }

    if (f) return f;

    // Unknown field or ENDGROUP.
    if (fieldnum == 0 || fieldnum > UPB_MAX_FIELDNUMBER)
      abortjmp(d, "Invalid field number");
    switch (wire_type) {
      case UPB_WIRE_TYPE_VARINT:    decode_varint(d); break;
      case UPB_WIRE_TYPE_32BIT:     skip(d, 4); break;
      case UPB_WIRE_TYPE_64BIT:     skip(d, 8); break;
      case UPB_WIRE_TYPE_DELIMITED: skip(d, decode_v32(d)); break;
      case UPB_WIRE_TYPE_START_GROUP:
        abortjmp(d, "Can't handle unknown groups yet");
      case UPB_WIRE_TYPE_END_GROUP:
        if (fieldnum != fr->group_fieldnum)
          abortjmp(d, "Unmatched ENDGROUP tag");
        pop_submsg(d);
        break;
      default:
        abortjmp(d, "Invalid wire type");
    }
    // TODO: deliver to unknown field callback.
    checkpoint(d);
    checkdelim(d);
  }
}

void *start(const upb_sinkframe *fr, size_t size_hint) {
  UPB_UNUSED(size_hint);
  upb_pbdecoder *d = upb_sinkframe_userdata(fr);
  assert(d);
  assert(d->sink);
  upb_sink_startmsg(d->sink);
  return d;
}

bool end(const upb_sinkframe *fr) {
  upb_pbdecoder *d = upb_sinkframe_userdata(fr);

  if (d->residual_end > d->residual) {
    // We have preserved bytes.
    upb_status_seterrliteral(decoder_status(d), "Unexpected EOF");
    return false;
  }

  // We may need to dispatch a top-level implicit frame.
  if (d->top == d->stack + 1 &&
      d->top->is_sequence &&
      !d->top->is_packed) {
    assert(upb_sinkframe_depth(upb_sink_top(d->sink)) == 1);
    pop_seq(d);
  }
  if (d->top != d->stack) {
    upb_status_seterrliteral(
        decoder_status(d), "Ended inside delimited field.");
    return false;
  }
  upb_sink_endmsg(d->sink);
  return true;
}

size_t decode(const upb_sinkframe *fr, const char *buf, size_t size) {
  upb_pbdecoder *d = upb_sinkframe_userdata(fr);
  decoderplan *plan = upb_sinkframe_handlerdata(fr);
  UPB_UNUSED(plan);
  assert(upb_sinkframe_handlers(upb_sink_top(d->sink)) == plan->dest_handlers);

  if (size == 0) return 0;
  // Assume we'll consume the whole buffer unless this is overwritten.
  d->ret = size;

  if (_setjmp(d->exitjmp)) {
    // Hit end-of-buffer or error.
    return d->ret;
  }

  d->buf_param = buf;
  d->size_param = size;
  if (d->residual_end > d->residual) {
    // We have residual bytes from the last buffer.
    d->userbuf_remaining = size;
  } else {
    d->userbuf_remaining = 0;
    advancetobuf(d, buf, size);

    if (d->top != d->stack &&
        upb_fielddef_isstring(d->top->f) &&
        !d->top->is_sequence) {
      // Last buffer ended in the middle of a string; deliver more of it.
      size_t len = d->top->end_ofs - offset(d);
      if (size >= len) {
        upb_sink_putstring(d->sink, getselector(d->top->f, UPB_HANDLER_STRING),
                           d->ptr, len);
        advance(d, len);
        pop_string(d);
      } else {
        upb_sink_putstring(d->sink, getselector(d->top->f, UPB_HANDLER_STRING),
                           d->ptr, size);
        advance(d, size);
        d->residual_end = d->residual;
        advancetobuf(d, d->residual, 0);
        return size;
      }
    }
  }
  checkpoint(d);

  const upb_fielddef *f = d->top->f;
  while(1) {
#ifdef UPB_USE_JIT_X64
    upb_decoder_enterjit(d, plan);
    checkpoint(d);
    set_delim_end(d);  // JIT doesn't keep this current.
#endif
    checkdelim(d);
    if (!d->top->is_packed) {
      f = decode_tag(d);
    }

    switch (upb_fielddef_descriptortype(f)) {
      case UPB_DESCRIPTOR_TYPE_DOUBLE:   decode_DOUBLE(d, f);   break;
      case UPB_DESCRIPTOR_TYPE_FLOAT:    decode_FLOAT(d, f);    break;
      case UPB_DESCRIPTOR_TYPE_INT64:    decode_INT64(d, f);    break;
      case UPB_DESCRIPTOR_TYPE_UINT64:   decode_UINT64(d, f);   break;
      case UPB_DESCRIPTOR_TYPE_INT32:    decode_INT32(d, f);    break;
      case UPB_DESCRIPTOR_TYPE_FIXED64:  decode_FIXED64(d, f);  break;
      case UPB_DESCRIPTOR_TYPE_FIXED32:  decode_FIXED32(d, f);  break;
      case UPB_DESCRIPTOR_TYPE_BOOL:     decode_BOOL(d, f);     break;
      case UPB_DESCRIPTOR_TYPE_STRING:   UPB_FALLTHROUGH_INTENDED;
      case UPB_DESCRIPTOR_TYPE_BYTES:    decode_STRING(d, f);   break;
      case UPB_DESCRIPTOR_TYPE_GROUP:    decode_GROUP(d, f);    break;
      case UPB_DESCRIPTOR_TYPE_MESSAGE:  decode_MESSAGE(d, f);  break;
      case UPB_DESCRIPTOR_TYPE_UINT32:   decode_UINT32(d, f);   break;
      case UPB_DESCRIPTOR_TYPE_ENUM:     decode_ENUM(d, f);     break;
      case UPB_DESCRIPTOR_TYPE_SFIXED32: decode_SFIXED32(d, f); break;
      case UPB_DESCRIPTOR_TYPE_SFIXED64: decode_SFIXED64(d, f); break;
      case UPB_DESCRIPTOR_TYPE_SINT32:   decode_SINT32(d, f);   break;
      case UPB_DESCRIPTOR_TYPE_SINT64:   decode_SINT64(d, f);   break;
    }
    checkpoint(d);
  }
}

void init(void *_d) {
  upb_pbdecoder *d = _d;
  d->limit = &d->stack[UPB_MAX_NESTING];
  d->sink = NULL;
  // reset() must be called before decoding; this is guaranteed by assert() in
  // start().
}

void reset(void *_d) {
  upb_pbdecoder *d = _d;
  d->top = d->stack;
  d->top->is_sequence = false;
  d->top->is_packed = false;
  d->top->group_fieldnum = UINT32_MAX;
  d->top->end_ofs = UPB_NONDELIMITED;
  d->bufstart_ofs = 0;
  d->ptr = d->residual;
  d->buf = d->residual;
  d->end = d->residual;
  d->residual_end = d->residual;
}

bool upb_pbdecoder_resetsink(upb_pbdecoder *d, upb_sink* sink) {
  // TODO(haberman): typecheck the sink, and test whether the decoder is in the
  // middle of decoding.  Return false if either assumption is violated.
  d->sink = sink;
  reset(d);
  return true;
}

const upb_frametype upb_pbdecoder_frametype = {
  sizeof(upb_pbdecoder),
  init,
  NULL,
  reset,
};

const upb_frametype *upb_pbdecoder_getframetype() {
  return &upb_pbdecoder_frametype;
}

const upb_handlers *upb_pbdecoder_gethandlers(const upb_handlers *dest,
                                              bool allowjit,
                                              const void *owner) {
  UPB_UNUSED(allowjit);
  decoderplan *p = malloc(sizeof(*p));
  assert(upb_handlers_isfrozen(dest));
  p->dest_handlers = dest;
  upb_handlers_ref(dest, p);
#ifdef UPB_USE_JIT_X64
  p->jit_code = NULL;
  if (allowjit) upb_decoderplan_makejit(p);
#endif

  upb_handlers *h = upb_handlers_new(
      UPB_BYTESTREAM, &upb_pbdecoder_frametype, owner);
  upb_handlers_setstartstr(h, UPB_BYTESTREAM_BYTES, start, NULL, NULL);
  upb_handlers_setstring(h, UPB_BYTESTREAM_BYTES, decode, p, freeplan);
  upb_handlers_setendstr(h, UPB_BYTESTREAM_BYTES, end, NULL, NULL);
  return h;
}
