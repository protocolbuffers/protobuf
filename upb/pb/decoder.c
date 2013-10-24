/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2008-2013 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 */

#include <inttypes.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include "upb/bytestream.h"
#include "upb/pb/decoder.int.h"
#include "upb/pb/varint.int.h"

#ifdef UPB_DUMP_BYTECODE
#include <stdio.h>
#endif

#define CHECK_SUSPEND(x) if (!(x)) return upb_pbdecoder_suspend(d);
#define CHECK_RETURN(x) { int32_t ret = x; if (ret >= 0) return ret; }

// Error messages that are shared between the bytecode and JIT decoders.
const char *kPbDecoderStackOverflow = "Nesting too deep.";

// Error messages shared within this file.
static const char *kUnterminatedVarint = "Unterminated varint.";

/* upb_pbdecoder **************************************************************/

static opcode halt = OP_HALT;

// Whether an op consumes any of the input buffer.
static bool consumes_input(opcode op) {
  switch (op) {
    case OP_SETDISPATCH:
    case OP_STARTMSG:
    case OP_ENDMSG:
    case OP_STARTSEQ:
    case OP_ENDSEQ:
    case OP_STARTSUBMSG:
    case OP_ENDSUBMSG:
    case OP_STARTSTR:
    case OP_ENDSTR:
    case OP_PUSHTAGDELIM:
    case OP_POP:
    case OP_SETDELIM:
    case OP_SETGROUPNUM:
    case OP_SETBIGGROUPNUM:
    case OP_CHECKDELIM:
    case OP_CALL:
    case OP_BRANCH:
      return false;
    default:
      return true;
  }
}

static bool in_residual_buf(upb_pbdecoder *d, const char *p);

// It's unfortunate that we have to micro-manage the compiler this way,
// especially since this tuning is necessarily specific to one hardware
// configuration.  But emperically on a Core i7, performance increases 30-50%
// with these annotations.  Every instance where these appear, gcc 4.2.1 made
// the wrong decision and degraded performance in benchmarks.
#define FORCEINLINE static inline __attribute__((always_inline))
#define NOINLINE __attribute__((noinline))

static void seterr(upb_pbdecoder *d, const char *msg) {
  // TODO(haberman): encapsulate this access to pipeline->status, but not sure
  // exactly what that interface should look like.
  upb_status_seterrliteral(&d->sink->pipeline_->status_, msg);
}

void upb_pbdecoder_seterr(upb_pbdecoder *d, const char *msg) {
  seterr(d, msg);
}


/* Buffering ******************************************************************/

// We operate on one buffer at a time, which is either the user's buffer passed
// to our "decode" callback or some residual bytes from the previous buffer.

// How many bytes can be safely read from d->ptr without reading past end-of-buf
// or past the current delimited end.
static size_t curbufleft(upb_pbdecoder *d) {
  assert(d->data_end >= d->ptr);
  return d->data_end - d->ptr;
}

static const char *ptr(upb_pbdecoder *d) {
  return d->ptr;
}

// Overall offset of d->ptr.
uint64_t offset(upb_pbdecoder *d) {
  return d->bufstart_ofs + (ptr(d) - d->buf);
}

// Advances d->ptr.
static void advance(upb_pbdecoder *d, size_t len) {
  assert(curbufleft(d) >= len);
  d->ptr += len;
}

static bool in_buf(const char *p, const char *buf, const char *end) {
  return p >= buf && p <= end;
}

static bool in_residual_buf(upb_pbdecoder *d, const char *p) {
  return in_buf(p, d->residual, d->residual_end);
}

// Calculates the delim_end value, which represents a combination of the
// current buffer and the stack, so must be called whenever either is updated.
static void set_delim_end(upb_pbdecoder *d) {
  size_t delim_ofs = d->top->end_ofs - d->bufstart_ofs;
  if (delim_ofs <= (d->end - d->buf)) {
    d->delim_end = d->buf + delim_ofs;
    d->data_end = d->delim_end;
  } else {
    d->data_end = d->end;
    d->delim_end = NULL;
  }
}

static void switchtobuf(upb_pbdecoder *d, const char *buf, const char *end) {
  d->ptr = buf;
  d->buf = buf;
  d->end = end;
  set_delim_end(d);
}

static void advancetobuf(upb_pbdecoder *d, const char *buf, size_t len) {
  assert(curbufleft(d) == 0);
  d->bufstart_ofs += (d->end - d->buf);
  switchtobuf(d, buf, buf + len);
}

static void checkpoint(upb_pbdecoder *d) {
  // The assertion here is in the interests of efficiency, not correctness.
  // We are trying to ensure that we don't checkpoint() more often than
  // necessary.
  assert(d->checkpoint != ptr(d));
  d->checkpoint = ptr(d);
}

// Resumes the decoder from an initial state or from a previous suspend.
void *upb_pbdecoder_resume(upb_pbdecoder *d, void *p, const char *buf,
                           size_t size) {
  UPB_UNUSED(p);  // Useless; just for the benefit of the JIT.
  d->buf_param = buf;
  d->size_param = size;
  d->skip = 0;
  if (d->residual_end > d->residual) {
    // We have residual bytes from the last buffer.
    assert(ptr(d) == d->residual);
  } else {
    switchtobuf(d, buf, buf + size);
  }
  d->checkpoint = ptr(d);
  return d;  // For the JIT.
}

// Suspends the decoder at the last checkpoint, without saving any residual
// bytes.  If there are any unconsumed bytes, returns a short byte count.
size_t upb_pbdecoder_suspend(upb_pbdecoder *d) {
  d->pc = d->last;
  if (d->checkpoint == d->residual) {
    // Checkpoint was in residual buf; no user bytes were consumed.
    d->ptr = d->residual;
    return 0;
  } else {
    assert(!in_residual_buf(d, d->checkpoint));
    assert(d->buf == d->buf_param);
    size_t consumed = d->checkpoint - d->buf;
    d->bufstart_ofs += consumed + d->skip;
    d->residual_end = d->residual;
    switchtobuf(d, d->residual, d->residual_end);
    return consumed + d->skip;
  }
}

// Suspends the decoder at the last checkpoint, and saves any unconsumed
// bytes in our residual buffer.  This is necessary if we need more user
// bytes to form a complete value, which might not be contiguous in the
// user's buffers.  Always consumes all user bytes.
static size_t suspend_save(upb_pbdecoder *d) {
  // We hit end-of-buffer before we could parse a full value.
  // Save any unconsumed bytes (if any) to the residual buffer.
  d->pc = d->last;

  if (d->checkpoint == d->residual) {
    // Checkpoint was in residual buf; append user byte(s) to residual buf.
    assert((d->residual_end - d->residual) + d->size_param <=
           sizeof(d->residual));
    if (!in_residual_buf(d, ptr(d))) {
      d->bufstart_ofs -= (d->residual_end - d->residual);
    }
    memcpy(d->residual_end, d->buf_param, d->size_param);
    d->residual_end += d->size_param;
  } else {
    // Checkpoint was in user buf; old residual bytes not needed.
    assert(!in_residual_buf(d, d->checkpoint));
    d->ptr = d->checkpoint;
    size_t save = curbufleft(d);
    assert(save <= sizeof(d->residual));
    memcpy(d->residual, ptr(d), save);
    d->residual_end = d->residual + save;
    d->bufstart_ofs = offset(d) + d->skip;
  }

  switchtobuf(d, d->residual, d->residual_end);
  return d->size_param + d->skip;
}

static int32_t skip(upb_pbdecoder *d, size_t bytes) {
  assert(!in_residual_buf(d, ptr(d)) || d->size_param == 0);
  if (curbufleft(d) >= bytes) {
    // Skipped data is all in current buffer.
    advance(d, bytes);
  } else {
    // Skipped data extends beyond currently available buffers.
    d->skip = bytes - curbufleft(d);
    advance(d, curbufleft(d));
  }
  return DECODE_OK;
}

FORCEINLINE void consumebytes(upb_pbdecoder *d, void *buf, size_t bytes) {
  assert(bytes <= curbufleft(d));
  memcpy(buf, ptr(d), bytes);
  advance(d, bytes);
}

static NOINLINE int32_t getbytes_slow(upb_pbdecoder *d, void *buf,
                                      size_t bytes) {
  const size_t avail = curbufleft(d);
  consumebytes(d, buf, avail);
  bytes -= avail;
  assert(bytes > 0);
  if (in_residual_buf(d, ptr(d))) {
    advancetobuf(d, d->buf_param, d->size_param);
  }
  if (curbufleft(d) >= bytes) {
    consumebytes(d, buf + avail, bytes);
    return DECODE_OK;
  } else if (d->data_end - d->buf == d->top->end_ofs - d->bufstart_ofs) {
    seterr(d, "Submessage ended in the middle of a value");
    return upb_pbdecoder_suspend(d);
  } else {
    return suspend_save(d);
  }
}

FORCEINLINE int32_t getbytes(upb_pbdecoder *d, void *buf, size_t bytes) {
  if (curbufleft(d) >= bytes) {
    // Buffer has enough data to satisfy.
    consumebytes(d, buf, bytes);
    return DECODE_OK;
  } else {
    return getbytes_slow(d, buf, bytes);
  }
}

static NOINLINE size_t peekbytes_slow(upb_pbdecoder *d, void *buf,
                                      size_t bytes) {
  size_t ret = curbufleft(d);
  memcpy(buf, ptr(d), ret);
  if (in_residual_buf(d, ptr(d))) {
    size_t copy = UPB_MIN(bytes - ret, d->size_param);
    memcpy(buf + ret, d->buf_param, copy);
    ret += copy;
  }
  return ret;
}

FORCEINLINE size_t peekbytes(upb_pbdecoder *d, void *buf, size_t bytes) {
  if (curbufleft(d) >= bytes) {
    memcpy(buf, ptr(d), bytes);
    return bytes;
  } else {
    return peekbytes_slow(d, buf, bytes);
  }
}


/* Decoding of wire types *****************************************************/

NOINLINE int32_t upb_pbdecoder_decode_varint_slow(upb_pbdecoder *d,
                                                  uint64_t *u64) {
  *u64 = 0;
  uint8_t byte = 0x80;
  int bitpos;
  for(bitpos = 0; bitpos < 70 && (byte & 0x80); bitpos += 7) {
    int32_t ret = getbytes(d, &byte, 1);
    if (ret >= 0) return ret;
    *u64 |= (uint64_t)(byte & 0x7F) << bitpos;
  }
  if(bitpos == 70 && (byte & 0x80)) {
    seterr(d, kUnterminatedVarint);
    return upb_pbdecoder_suspend(d);
  }
  return DECODE_OK;
}

FORCEINLINE int32_t decode_varint(upb_pbdecoder *d, uint64_t *u64) {
  if (curbufleft(d) > 0 && !(*ptr(d) & 0x80)) {
    *u64 = *ptr(d);
    advance(d, 1);
    return DECODE_OK;
  } else if (curbufleft(d) >= 10) {
    // Fast case.
    upb_decoderet r = upb_vdecode_fast(ptr(d));
    if (r.p == NULL) {
      seterr(d, kUnterminatedVarint);
      return upb_pbdecoder_suspend(d);
    }
    advance(d, r.p - ptr(d));
    *u64 = r.val;
    return DECODE_OK;
  } else {
    // Slow case -- varint spans buffer seam.
    return upb_pbdecoder_decode_varint_slow(d, u64);
  }
}

FORCEINLINE int32_t decode_v32(upb_pbdecoder *d, uint32_t *u32) {
  uint64_t u64;
  int32_t ret = decode_varint(d, &u64);
  if (ret >= 0) return ret;
  if (u64 > UINT32_MAX) {
    seterr(d, "Unterminated 32-bit varint");
    // TODO(haberman) guarantee that this function return is >= 0 somehow,
    // so we know this path will always be treated as error by our caller.
    // Right now the size_t -> int32_t can overflow and produce negative values.
    *u32 = 0;
    return upb_pbdecoder_suspend(d);
  }
  *u32 = u64;
  return DECODE_OK;
}

// TODO: proper byte swapping for big-endian machines.
FORCEINLINE int32_t decode_fixed32(upb_pbdecoder *d, uint32_t *u32) {
  return getbytes(d, u32, 4);
}

// TODO: proper byte swapping for big-endian machines.
FORCEINLINE int32_t decode_fixed64(upb_pbdecoder *d, uint64_t *u64) {
  return getbytes(d, u64, 8);
}

int32_t upb_pbdecoder_decode_f32(upb_pbdecoder *d, uint32_t *u32) {
  return decode_fixed32(d, u32);
}

int32_t upb_pbdecoder_decode_f64(upb_pbdecoder *d, uint64_t *u64) {
  return decode_fixed64(d, u64);
}

static double as_double(uint64_t n) { double d; memcpy(&d, &n, 8); return d; }
static float  as_float(uint32_t n)  { float  f; memcpy(&f, &n, 4); return f; }

static bool push(upb_pbdecoder *d, uint64_t end) {
  upb_pbdecoder_frame *fr = d->top;

  if (end > fr->end_ofs) {
    seterr(d, "Submessage end extends past enclosing submessage.");
    return false;
  } else if ((fr + 1) == d->limit) {
    seterr(d, kPbDecoderStackOverflow);
    return false;
  }

  fr++;
  fr->end_ofs = end;
  fr->u.dispatch = NULL;
  fr->groupnum = -1;
  d->top = fr;
  return true;
}

NOINLINE int32_t upb_pbdecoder_checktag_slow(upb_pbdecoder *d,
                                             uint64_t expected) {
  uint64_t data = 0;
  size_t bytes = upb_value_size(expected);
  size_t read = peekbytes(d, &data, bytes);
  if (read == bytes && data == expected) {
    // Advance past matched bytes.
    int32_t ok = getbytes(d, &data, read);
    UPB_ASSERT_VAR(ok, ok < 0);
    return DECODE_OK;
  } else if (read < bytes && memcmp(&data, &expected, read) == 0) {
    return suspend_save(d);
  } else {
    return DECODE_MISMATCH;
  }
}

int32_t upb_pbdecoder_skipunknown(upb_pbdecoder *d, uint32_t fieldnum,
                                  uint8_t wire_type) {
  if (fieldnum == 0 || fieldnum > UPB_MAX_FIELDNUMBER) {
    seterr(d, "Invalid field number");
    return upb_pbdecoder_suspend(d);
  }

  if (wire_type == UPB_WIRE_TYPE_END_GROUP) {
    if (fieldnum != d->top->groupnum) {
      seterr(d, "Unmatched ENDGROUP tag.");
      return upb_pbdecoder_suspend(d);
    }
    return DECODE_ENDGROUP;
  }

  // TODO: deliver to unknown field callback.
  switch (wire_type) {
    case UPB_WIRE_TYPE_VARINT: {
      uint64_t u64;
      return decode_varint(d, &u64);
    }
    case UPB_WIRE_TYPE_32BIT:
      return skip(d, 4);
    case UPB_WIRE_TYPE_64BIT:
      return skip(d, 8);
    case UPB_WIRE_TYPE_DELIMITED: {
      uint32_t len;
      CHECK_RETURN(decode_v32(d, &len));
      return skip(d, len);
    }
    case UPB_WIRE_TYPE_START_GROUP:
      seterr(d, "Can't handle unknown groups yet");
      return upb_pbdecoder_suspend(d);
    case UPB_WIRE_TYPE_END_GROUP:
    default:
      seterr(d, "Invalid wire type");
      return upb_pbdecoder_suspend(d);
  }
}

static int32_t dispatch(upb_pbdecoder *d) {
  upb_inttable *dispatch = d->top->u.dispatch;

  // Decode tag.
  uint32_t tag;
  CHECK_RETURN(decode_v32(d, &tag));
  uint8_t wire_type = tag & 0x7;
  uint32_t fieldnum = tag >> 3;

  // Lookup tag.  Because of packed/non-packed compatibility, we have to
  // check the wire type against two possibilities.
  upb_value val;
  if (upb_inttable_lookup32(dispatch, fieldnum, &val)) {
    uint64_t v = upb_value_getuint64(val);
    if (wire_type == (v & 0xff)) {
      d->pc = d->top->base + (v >> 16);
      return DECODE_OK;
    } else if (wire_type == ((v >> 8) & 0xff)) {
      bool found =
          upb_inttable_lookup(dispatch, fieldnum + UPB_MAX_FIELDNUMBER, &val);
      UPB_ASSERT_VAR(found, found);
      d->pc = d->top->base + upb_value_getuint64(val);
      return DECODE_OK;
    }
  }

  // Unknown field or ENDGROUP.
  int32_t ret = upb_pbdecoder_skipunknown(d, fieldnum, wire_type);

  if (ret == DECODE_ENDGROUP) {
    d->pc = d->top->base - 1;  // Back to OP_ENDMSG.
    return DECODE_OK;
  } else {
    d->pc = d->last - 1;  // Rewind to CHECKDELIM.
    return ret;
  }
}


/* The main decoding loop *****************************************************/

size_t upb_pbdecoder_decode(void *closure, const void *hd, const char *buf,
                            size_t size) {
  upb_pbdecoder *d = closure;
  const upb_pbdecoderplan *p = hd;
  assert(buf);
  upb_pbdecoder_resume(d, NULL, buf, size);
  UPB_UNUSED(p);

#define VMCASE(op, code) \
  case op: { code; if (consumes_input(op)) checkpoint(d); break; }
#define PRIMITIVE_OP(type, wt, name, convfunc, ctype) \
  VMCASE(OP_PARSE_ ## type, { \
    ctype val; \
    CHECK_RETURN(decode_ ## wt(d, &val)); \
    upb_sink_put ## name(d->sink, arg, (convfunc)(val)); \
  })

  while(1) {
    d->last = d->pc;
    int32_t instruction = *d->pc++;
    opcode op = getop(instruction);
    uint32_t arg = instruction >> 8;
    int32_t longofs = arg;
    assert(ptr(d) != d->residual_end);
#ifdef UPB_DUMP_BYTECODE
    fprintf(stderr, "s_ofs=%d buf_ofs=%d data_rem=%d buf_rem=%d delim_rem=%d "
                    "%x %s (%d)\n",
            (int)offset(d),
            (int)(ptr(d) - d->buf),
            (int)(d->data_end - ptr(d)),
            (int)(d->end - ptr(d)),
            (int)((d->top->end_ofs - d->bufstart_ofs) - (ptr(d) - d->buf)),
            (int)(d->pc - 1 - upb_pbdecoderplan_codebase(p)),
            upb_pbdecoder_getopname(op),
            arg);
#endif
    switch (op) {
      // Technically, we are losing data if we see a 32-bit varint that is not
      // properly sign-extended.  We could detect this and error about the data
      // loss, but proto2 does not do this, so we pass.
      PRIMITIVE_OP(INT32,    varint,  int32,  int32_t,      uint64_t)
      PRIMITIVE_OP(INT64,    varint,  int64,  int64_t,      uint64_t)
      PRIMITIVE_OP(UINT32,   varint,  uint32, uint32_t,     uint64_t)
      PRIMITIVE_OP(UINT64,   varint,  uint64, uint64_t,     uint64_t)
      PRIMITIVE_OP(FIXED32,  fixed32, uint32, uint32_t,     uint32_t)
      PRIMITIVE_OP(FIXED64,  fixed64, uint64, uint64_t,     uint64_t)
      PRIMITIVE_OP(SFIXED32, fixed32, int32,  int32_t,      uint32_t)
      PRIMITIVE_OP(SFIXED64, fixed64, int64,  int64_t,      uint64_t)
      PRIMITIVE_OP(BOOL,     varint,  bool,   bool,         uint64_t)
      PRIMITIVE_OP(DOUBLE,   fixed64, double, as_double,    uint64_t)
      PRIMITIVE_OP(FLOAT,    fixed32, float,  as_float,     uint32_t)
      PRIMITIVE_OP(SINT32,   varint,  int32,  upb_zzdec_32, uint64_t)
      PRIMITIVE_OP(SINT64,   varint,  int64,  upb_zzdec_64, uint64_t)

      VMCASE(OP_SETDISPATCH,
        d->top->base = d->pc - 1;
        memcpy(&d->top->u.dispatch, d->pc, sizeof(void*));
        d->pc += sizeof(void*) / sizeof(uint32_t);
      )
      VMCASE(OP_STARTMSG,
        CHECK_SUSPEND(upb_sink_startmsg(d->sink));
      )
      VMCASE(OP_ENDMSG,
        CHECK_SUSPEND(upb_sink_endmsg(d->sink));
        assert(d->call_len > 0);
        d->pc = d->callstack[--d->call_len];
      )
      VMCASE(OP_STARTSEQ,
        CHECK_SUSPEND(upb_sink_startseq(d->sink, arg));
      )
      VMCASE(OP_ENDSEQ,
        CHECK_SUSPEND(upb_sink_endseq(d->sink, arg));
      )
      VMCASE(OP_STARTSUBMSG,
        CHECK_SUSPEND(upb_sink_startsubmsg(d->sink, arg));
      )
      VMCASE(OP_ENDSUBMSG,
        CHECK_SUSPEND(upb_sink_endsubmsg(d->sink, arg));
      )
      VMCASE(OP_STARTSTR,
        uint32_t len = d->top->end_ofs - offset(d);
        CHECK_SUSPEND(upb_sink_startstr(d->sink, arg, len));
        if (len == 0) {
          d->pc++;  // Skip OP_STRING.
        }
      )
      VMCASE(OP_STRING,
        uint32_t len = curbufleft(d);
        CHECK_SUSPEND(upb_sink_putstring(d->sink, arg, ptr(d), len));
        advance(d, len);
        if (d->delim_end == NULL) {  // String extends beyond this buf?
          d->pc--;
          d->bufstart_ofs += size;
          d->residual_end = d->residual;
          return size;
        }
      )
      VMCASE(OP_ENDSTR,
        CHECK_SUSPEND(upb_sink_endstr(d->sink, arg));
      )
      VMCASE(OP_PUSHTAGDELIM,
        CHECK_SUSPEND(push(d, d->top->end_ofs));
      )
      VMCASE(OP_POP,
        assert(d->top > d->stack);
        d->top--;
      )
      VMCASE(OP_PUSHLENDELIM,
        uint32_t len;
        CHECK_RETURN(decode_v32(d, &len));
        CHECK_SUSPEND(push(d, offset(d) + len));
        set_delim_end(d);
      )
      VMCASE(OP_SETDELIM,
        set_delim_end(d);
      )
      VMCASE(OP_SETGROUPNUM,
        d->top->groupnum = arg;
      )
      VMCASE(OP_SETBIGGROUPNUM,
        d->top->groupnum = *d->pc++;
      )
      VMCASE(OP_CHECKDELIM,
        assert(!(d->delim_end && ptr(d) > d->delim_end));
        if (ptr(d) == d->delim_end)
          d->pc += longofs;
      )
      VMCASE(OP_CALL,
        d->callstack[d->call_len++] = d->pc;
        d->pc += longofs;
      )
      VMCASE(OP_BRANCH,
        d->pc += longofs;
      )
      VMCASE(OP_TAG1,
        CHECK_SUSPEND(curbufleft(d) > 0);
        uint8_t expected = (arg >> 8) & 0xff;
        if (*ptr(d) == expected) {
          advance(d, 1);
        } else {
          int8_t shortofs;
         badtag:
          shortofs = arg;
          if (shortofs == LABEL_DISPATCH) {
            CHECK_RETURN(dispatch(d));
          } else {
            d->pc += shortofs;
            break; // Avoid checkpoint().
          }
        }
      )
      VMCASE(OP_TAG2,
        CHECK_SUSPEND(curbufleft(d) > 0);
        uint16_t expected = (arg >> 8) & 0xffff;
        if (curbufleft(d) >= 2) {
          uint16_t actual;
          memcpy(&actual, ptr(d), 2);
          if (expected == actual) {
            advance(d, 2);
          } else {
            goto badtag;
          }
        } else {
          int32_t result = upb_pbdecoder_checktag_slow(d, expected);
          if (result == DECODE_MISMATCH) goto badtag;
          if (result >= 0) return result;
        }
      )
      VMCASE(OP_TAGN, {
        uint64_t expected;
        memcpy(&expected, d->pc, 8);
        d->pc += 2;
        int32_t result = upb_pbdecoder_checktag_slow(d, expected);
        if (result == DECODE_MISMATCH) goto badtag;
        if (result >= 0) return result;
      })
      VMCASE(OP_HALT, {
        return size;
      })
    }
  }
}

void *upb_pbdecoder_start(void *closure, const void *handler_data,
                          size_t size_hint) {
  UPB_UNUSED(size_hint);
  upb_pbdecoder *d = closure;
  const upb_pbdecoderplan *plan = handler_data;
  UPB_UNUSED(plan);
  if (upb_pbdecoderplan_hasjitcode(plan)) {
    d->top->u.closure = d->sink->top->closure;
    d->call_len = 0;
  } else {
    d->call_len = 1;
    d->pc = upb_pbdecoderplan_codebase(plan);
  }
  assert(d);
  assert(d->sink);
  if (plan->topmethod->dest_handlers) {
    assert(d->sink->top->h == plan->topmethod->dest_handlers);
  }
  d->status = &d->sink->pipeline_->status_;
  return d;
}

bool upb_pbdecoder_end(void *closure, const void *handler_data) {
  upb_pbdecoder *d = closure;
  const upb_pbdecoderplan *plan = handler_data;

  if (d->residual_end > d->residual) {
    seterr(d, "Unexpected EOF");
    return false;
  }

  // Message ends here.
  uint64_t end = offset(d);
  d->top->end_ofs = end;
  char dummy;
  if (upb_pbdecoderplan_hasjitcode(plan)) {
#ifdef UPB_USE_JIT_X64
    if (d->top != d->stack)
      d->stack->end_ofs = 0;
    upb_pbdecoderplan_jitcode(plan)(closure, handler_data, &dummy, 0);
#endif
  } else {
    d->stack->end_ofs = end;
    uint32_t *p = d->pc - 1;
    if (getop(*p) == OP_CHECKDELIM) {
      // Rewind from OP_TAG* to OP_CHECKDELIM.
      assert(getop(*d->pc) == OP_TAG1 ||
             getop(*d->pc) == OP_TAG2 ||
             getop(*d->pc) == OP_TAGN);
      d->pc = p;
    }
    upb_pbdecoder_decode(closure, handler_data, &dummy, 0);
  }

  if (d->call_len != 0) {
    seterr(d, "Unexpected EOF");
    return false;
  }

  return upb_ok(&d->sink->pipeline_->status_);
}

void init(void *_d, upb_pipeline *p) {
  UPB_UNUSED(p);
  upb_pbdecoder *d = _d;
  d->limit = &d->stack[UPB_DECODER_MAX_NESTING];
  d->sink = NULL;
  d->callstack[0] = &halt;
  // reset() must be called before decoding; this is guaranteed by assert() in
  // start().
}

void reset(void *_d) {
  upb_pbdecoder *d = _d;
  d->top = d->stack;
  d->top->end_ofs = UINT64_MAX;
  d->bufstart_ofs = 0;
  d->ptr = d->residual;
  d->buf = d->residual;
  d->end = d->residual;
  d->residual_end = d->residual;
  d->call_len = 1;
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
