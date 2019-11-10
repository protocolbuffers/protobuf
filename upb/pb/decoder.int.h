/*
** Internal-only definitions for the decoder.
*/

#ifndef UPB_DECODER_INT_H_
#define UPB_DECODER_INT_H_

#include "upb/def.h"
#include "upb/handlers.h"
#include "upb/pb/decoder.h"
#include "upb/sink.h"
#include "upb/table.int.h"

#include "upb/port_def.inc"

/* Opcode definitions.  The canonical meaning of each opcode is its
 * implementation in the interpreter (the JIT is written to match this).
 *
 * All instructions have the opcode in the low byte.
 * Instruction format for most instructions is:
 *
 * +-------------------+--------+
 * |     arg (24)      | op (8) |
 * +-------------------+--------+
 *
 * Exceptions are indicated below.  A few opcodes are multi-word. */
typedef enum {
  /* Opcodes 1-8, 13, 15-18 parse their respective descriptor types.
   * Arg for all of these is the upb selector for this field. */
#define T(type) OP_PARSE_ ## type = UPB_DESCRIPTOR_TYPE_ ## type
  T(DOUBLE), T(FLOAT), T(INT64), T(UINT64), T(INT32), T(FIXED64), T(FIXED32),
  T(BOOL), T(UINT32), T(SFIXED32), T(SFIXED64), T(SINT32), T(SINT64),
#undef T
  OP_STARTMSG       = 9,   /* No arg. */
  OP_ENDMSG         = 10,  /* No arg. */
  OP_STARTSEQ       = 11,
  OP_ENDSEQ         = 12,
  OP_STARTSUBMSG    = 14,
  OP_ENDSUBMSG      = 19,
  OP_STARTSTR       = 20,
  OP_STRING         = 21,
  OP_ENDSTR         = 22,

  OP_PUSHTAGDELIM   = 23,  /* No arg. */
  OP_PUSHLENDELIM   = 24,  /* No arg. */
  OP_POP            = 25,  /* No arg. */
  OP_SETDELIM       = 26,  /* No arg. */
  OP_SETBIGGROUPNUM = 27,  /* two words:
                            *   | unused (24)     | opc (8) |
                            *   |        groupnum (32)      | */
  OP_CHECKDELIM     = 28,
  OP_CALL           = 29,
  OP_RET            = 30,
  OP_BRANCH         = 31,

  /* Different opcodes depending on how many bytes expected. */
  OP_TAG1           = 32,  /* | match tag (16) | jump target (8) | opc (8) | */
  OP_TAG2           = 33,  /* | match tag (16) | jump target (8) | opc (8) | */
  OP_TAGN           = 34,  /* three words: */
                           /*   | unused (16) | jump target(8) | opc (8) | */
                           /*   |           match tag 1 (32)             | */
                           /*   |           match tag 2 (32)             | */

  OP_SETDISPATCH    = 35,  /* N words: */
                           /*   | unused (24)         | opc | */
                           /*   | upb_inttable* (32 or 64)  | */

  OP_DISPATCH       = 36,  /* No arg. */

  OP_HALT           = 37   /* No arg. */
} opcode;

#define OP_MAX OP_HALT

UPB_INLINE opcode getop(uint32_t instr) { return (opcode)(instr & 0xff); }

struct upb_pbcodecache {
  upb_arena *arena;
  upb_handlercache *dest;
  bool allow_jit;
  bool lazy;

  /* Map of upb_msgdef -> mgroup. */
  upb_inttable groups;
};

/* Method group; represents a set of decoder methods that had their code
 * emitted together.  Immutable once created.  */
typedef struct {
  /* Maps upb_msgdef/upb_handlers -> upb_pbdecodermethod.  Owned by us.
   *
   * Ideally this would be on pbcodecache (if we were actually caching code).
   * Right now we don't actually cache anything, which is wasteful. */
  upb_inttable methods;

  /* The bytecode for our methods, if any exists.  Owned by us. */
  uint32_t *bytecode;
  uint32_t *bytecode_end;
} mgroup;

/* The maximum that any submessages can be nested.  Matches proto2's limit.
 * This specifies the size of the decoder's statically-sized array and therefore
 * setting it high will cause the upb::pb::Decoder object to be larger.
 *
 * If necessary we can add a runtime-settable property to Decoder that allow
 * this to be larger than the compile-time setting, but this would add
 * complexity, particularly since we would have to decide how/if to give users
 * the ability to set a custom memory allocation function. */
#define UPB_DECODER_MAX_NESTING 64

/* Internal-only struct used by the decoder. */
typedef struct {
  /* Space optimization note: we store two pointers here that the JIT
   * doesn't need at all; the upb_handlers* inside the sink and
   * the dispatch table pointer.  We can optimze so that the JIT uses
   * smaller stack frames than the interpreter.  The only thing we need
   * to guarantee is that the fallback routines can find end_ofs. */
  upb_sink sink;

  /* The absolute stream offset of the end-of-frame delimiter.
   * Non-delimited frames (groups and non-packed repeated fields) reuse the
   * delimiter of their parent, even though the frame may not end there.
   *
   * NOTE: the JIT stores a slightly different value here for non-top frames.
   * It stores the value relative to the end of the enclosed message.  But the
   * top frame is still stored the same way, which is important for ensuring
   * that calls from the JIT into C work correctly. */
  uint64_t end_ofs;
  const uint32_t *base;

  /* 0 indicates a length-delimited field.
   * A positive number indicates a known group.
   * A negative number indicates an unknown group. */
  int32_t groupnum;
  upb_inttable *dispatch;  /* Not used by the JIT. */
} upb_pbdecoder_frame;

struct upb_pbdecodermethod {
  /* While compiling, the base is relative in "ofs", after compiling it is
   * absolute in "ptr". */
  union {
    uint32_t ofs;     /* PC offset of method. */
    void *ptr;        /* Pointer to bytecode or machine code for this method. */
  } code_base;

  /* The decoder method group to which this method belongs. */
  const mgroup *group;

  /* Whether this method is native code or bytecode. */
  bool is_native_;

  /* The handler one calls to invoke this method. */
  upb_byteshandler input_handler_;

  /* The destination handlers this method is bound to.  We own a ref. */
  const upb_handlers *dest_handlers_;

  /* Dispatch table -- used by both bytecode decoder and JIT when encountering a
   * field number that wasn't the one we were expecting to see.  See
   * decoder.int.h for the layout of this table. */
  upb_inttable dispatch;
};

struct upb_pbdecoder {
  upb_arena *arena;

  /* Our input sink. */
  upb_bytessink input_;

  /* The decoder method we are parsing with (owned). */
  const upb_pbdecodermethod *method_;

  size_t call_len;
  const uint32_t *pc, *last;

  /* Current input buffer and its stream offset. */
  const char *buf, *ptr, *end, *checkpoint;

  /* End of the delimited region, relative to ptr, NULL if not in this buf. */
  const char *delim_end;

  /* End of the delimited region, relative to ptr, end if not in this buf. */
  const char *data_end;

  /* Overall stream offset of "buf." */
  uint64_t bufstart_ofs;

  /* Buffer for residual bytes not parsed from the previous buffer. */
  char residual[UPB_DECODER_MAX_RESIDUAL_BYTES];
  char *residual_end;

  /* Bytes of data that should be discarded from the input beore we start
   * parsing again.  We set this when we internally determine that we can
   * safely skip the next N bytes, but this region extends past the current
   * user buffer. */
  size_t skip;

  /* Stores the user buffer passed to our decode function. */
  const char *buf_param;
  size_t size_param;
  const upb_bufhandle *handle;

  /* Our internal stack. */
  upb_pbdecoder_frame *stack, *top, *limit;
  const uint32_t **callstack;
  size_t stack_size;

  upb_status *status;
};

/* Decoder entry points; used as handlers. */
void *upb_pbdecoder_startbc(void *closure, const void *pc, size_t size_hint);
size_t upb_pbdecoder_decode(void *closure, const void *hd, const char *buf,
                            size_t size, const upb_bufhandle *handle);
bool upb_pbdecoder_end(void *closure, const void *handler_data);

/* Decoder-internal functions that the JIT calls to handle fallback paths. */
int32_t upb_pbdecoder_resume(upb_pbdecoder *d, void *p, const char *buf,
                             size_t size, const upb_bufhandle *handle);
size_t upb_pbdecoder_suspend(upb_pbdecoder *d);
int32_t upb_pbdecoder_skipunknown(upb_pbdecoder *d, int32_t fieldnum,
                                  uint8_t wire_type);
int32_t upb_pbdecoder_checktag_slow(upb_pbdecoder *d, uint64_t expected);
int32_t upb_pbdecoder_decode_varint_slow(upb_pbdecoder *d, uint64_t *u64);
int32_t upb_pbdecoder_decode_f32(upb_pbdecoder *d, uint32_t *u32);
int32_t upb_pbdecoder_decode_f64(upb_pbdecoder *d, uint64_t *u64);
void upb_pbdecoder_seterr(upb_pbdecoder *d, const char *msg);

/* Error messages that are shared between the bytecode and JIT decoders. */
extern const char *kPbDecoderStackOverflow;
extern const char *kPbDecoderSubmessageTooLong;

/* Access to decoderplan members needed by the decoder. */
const char *upb_pbdecoder_getopname(unsigned int op);

/* A special label that means "do field dispatch for this message and branch to
 * wherever that takes you." */
#define LABEL_DISPATCH 0

/* A special slot in the dispatch table that stores the epilogue (ENDMSG and/or
 * RET) for branching to when we find an appropriate ENDGROUP tag. */
#define DISPATCH_ENDMSG 0

/* It's important to use this invalid wire type instead of 0 (which is a valid
 * wire type). */
#define NO_WIRE_TYPE 0xff

/* The dispatch table layout is:
 *   [field number] -> [ 48-bit offset ][ 8-bit wt2 ][ 8-bit wt1 ]
 *
 * If wt1 matches, jump to the 48-bit offset.  If wt2 matches, lookup
 * (UPB_MAX_FIELDNUMBER + fieldnum) and jump there.
 *
 * We need two wire types because of packed/non-packed compatibility.  A
 * primitive repeated field can use either wire type and be valid.  While we
 * could key the table on fieldnum+wiretype, the table would be 8x sparser.
 *
 * Storing two wire types in the primary value allows us to quickly rule out
 * the second wire type without needing to do a separate lookup (this case is
 * less common than an unknown field). */
UPB_INLINE uint64_t upb_pbdecoder_packdispatch(uint64_t ofs, uint8_t wt1,
                                               uint8_t wt2) {
  return (ofs << 16) | (wt2 << 8) | wt1;
}

UPB_INLINE void upb_pbdecoder_unpackdispatch(uint64_t dispatch, uint64_t *ofs,
                                             uint8_t *wt1, uint8_t *wt2) {
  *wt1 = (uint8_t)dispatch;
  *wt2 = (uint8_t)(dispatch >> 8);
  *ofs = dispatch >> 16;
}

/* All of the functions in decoder.c that return int32_t return values according
 * to the following scheme:
 *   1. negative values indicate a return code from the following list.
 *   2. positive values indicate that error or end of buffer was hit, and
 *      that the decode function should immediately return the given value
 *      (the decoder state has already been suspended and is ready to be
 *      resumed). */
#define DECODE_OK -1
#define DECODE_MISMATCH -2  /* Used only from checktag_slow(). */
#define DECODE_ENDGROUP -3  /* Used only from checkunknown(). */

#define CHECK_RETURN(x) { int32_t ret = x; if (ret >= 0) return ret; }

#include "upb/port_undef.inc"

#endif  /* UPB_DECODER_INT_H_ */
