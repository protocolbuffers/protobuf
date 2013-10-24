
#ifndef UPB_DECODER_INT_H_
#define UPB_DECODER_INT_H_

#include <stdlib.h>
#include "upb/def.h"
#include "upb/handlers.h"
#include "upb/sink.h"
#include "upb/pb/decoder.h"

// Opcode definitions.  The canonical meaning of each opcode is its
// implementation in the interpreter (the JIT is written to match this).
//
// All instructions have the opcode in the low byte.
// Instruction format for most instructions is:
//
// +-------------------+--------+
// |     arg (24)      | op (8) |
// +-------------------+--------+
//
// Exceptions are indicated below.  A few opcodes are multi-word.
typedef enum {
  // Opcodes 1-8, 13, 15-18 parse their respective descriptor types.
  // Arg for all of these is the upb selector for this field.
#define T(type) OP_PARSE_ ## type = UPB_DESCRIPTOR_TYPE_ ## type
  T(DOUBLE), T(FLOAT), T(INT64), T(UINT64), T(INT32), T(FIXED64), T(FIXED32),
  T(BOOL), T(UINT32), T(SFIXED32), T(SFIXED64), T(SINT32), T(SINT64),
#undef T
  OP_STARTMSG       = 9,   // No arg.
  OP_ENDMSG         = 10,  // No arg.
  OP_STARTSEQ       = 11,
  OP_ENDSEQ         = 12,
  OP_STARTSUBMSG    = 14,
  OP_ENDSUBMSG      = 19,
  OP_STARTSTR       = 20,
  OP_STRING         = 21,
  OP_ENDSTR         = 22,

  OP_PUSHTAGDELIM   = 23,  // No arg.
  OP_PUSHLENDELIM   = 24,  // No arg.
  OP_POP            = 25,  // No arg.
  OP_SETDELIM       = 26,  // No arg.
  OP_SETGROUPNUM    = 27,
  OP_SETBIGGROUPNUM = 28,  // two words: | unused (24) | opc || groupnum (32) |

  // The arg for these opcodes is a local label reference.
  OP_CHECKDELIM     = 29,
  OP_CALL           = 30,
  OP_BRANCH         = 31,

  // Different opcodes depending on how many bytes expected.
  OP_TAG1           = 32,  // | expected tag (16) | jump target (8) | opc (8) |
  OP_TAG2           = 33,  // | expected tag (16) | jump target (8) | opc (8) |
  OP_TAGN           = 34,  // three words:
                           //   | unused (16) | jump target(8) | opc (8) |
                           //   |           expected tag 1 (32)          |
                           //   |           expected tag 2 (32)          |

  OP_SETDISPATCH    = 35,  // N words:
                           //   | unused (24)         | opc |
                           //   | upb_inttable* (32 or 64)  |

  OP_HALT           = 36,  // No arg.
} opcode;

#define OP_MAX OP_HALT

UPB_INLINE opcode getop(uint32_t instr) { return instr & 0xff; }

const upb_frametype upb_pbdecoder_frametype;

// Decoder entry points; used as handlers.
void *upb_pbdecoder_start(void *closure, const void *handler_data,
                          size_t size_hint);
size_t upb_pbdecoder_decode(void *closure, const void *hd, const char *buf,
                            size_t size);
bool upb_pbdecoder_end(void *closure, const void *handler_data);

// Decoder-internal functions that the JIT calls to handle fallback paths.
void *upb_pbdecoder_resume(upb_pbdecoder *d, void *p, const char *buf,
                           size_t size);
size_t upb_pbdecoder_suspend(upb_pbdecoder *d);
int32_t upb_pbdecoder_skipunknown(upb_pbdecoder *d, uint32_t fieldnum,
                                  uint8_t wire_type);
int32_t upb_pbdecoder_checktag_slow(upb_pbdecoder *d, uint64_t expected);
int32_t upb_pbdecoder_decode_varint_slow(upb_pbdecoder *d, uint64_t *u64);
int32_t upb_pbdecoder_decode_f32(upb_pbdecoder *d, uint32_t *u32);
int32_t upb_pbdecoder_decode_f64(upb_pbdecoder *d, uint64_t *u64);
void upb_pbdecoder_seterr(upb_pbdecoder *d, const char *msg);

// Error messages that are shared between the bytecode and JIT decoders.
extern const char *kPbDecoderStackOverflow;

typedef struct _upb_pbdecoderplan upb_pbdecoderplan;

// Access to decoderplan members needed by the decoder.
bool upb_pbdecoderplan_hasjitcode(const upb_pbdecoderplan *p);
uint32_t *upb_pbdecoderplan_codebase(const upb_pbdecoderplan *p);
const char *upb_pbdecoder_getopname(unsigned int op);
upb_string_handler *upb_pbdecoderplan_jitcode(const upb_pbdecoderplan *p);

// JIT entry point.
void upb_pbdecoder_jit(upb_pbdecoderplan *plan);
void upb_pbdecoder_freejit(upb_pbdecoderplan *plan);


// A special label that means "do field dispatch for this message and branch to
// wherever that takes you."
#define LABEL_DISPATCH 0

#define DECODE_OK -1
#define DECODE_MISMATCH -2  // Used only from checktag_slow().
#define DECODE_ENDGROUP -2  // Used only from checkunknown().

typedef struct {
  // The absolute stream offset of the end-of-frame delimiter.
  // Non-delimited frames (groups and non-packed repeated fields) reuse the
  // delimiter of their parent, even though the frame may not end there.
  //
  // NOTE: the JIT stores a slightly different value here for non-top frames.
  // It stores the value relative to the end of the enclosed message.  But the
  // innermost frame is still stored the same way, which is important for
  // ensuring that calls from the JIT into C work correctly.
  uint64_t end_ofs;
  uint32_t *base;
  uint32_t groupnum;
  union {
    upb_inttable *dispatch;  // Not used by the JIT.
    void         *closure;   // Only used by the JIT.
  } u;
} upb_pbdecoder_frame;

struct upb_pbdecoder {
  // Where we push parsed data (not owned).
  upb_sink *sink;

  size_t call_len;
  uint32_t *pc, *last;

  // Current input buffer and its stream offset.
  const char *buf, *ptr, *end, *checkpoint;

  // End of the delimited region, relative to ptr, or NULL if not in this buf.
  const char *delim_end;

  // End of the delimited region, relative to ptr, or end if not in this buf.
  const char *data_end;

  // Overall stream offset of "buf."
  uint64_t bufstart_ofs;

  // How many bytes past the end of the user buffer we want to skip.
  size_t skip;

  // Buffer for residual bytes not parsed from the previous buffer.
  // The maximum number of residual bytes we require is 12; a five-byte
  // unknown tag plus an eight-byte value, less one because the value
  // is only a partial value.
  char residual[12];
  char *residual_end;

  // Stores the user buffer passed to our decode function.
  const char *buf_param;
  size_t size_param;

#ifdef UPB_USE_JIT_X64
  // Used momentarily by the generated code to store a value while a user
  // function is called.
  uint32_t tmp_len;

  const void *saved_rsp;
#endif

  upb_status *status;

  // Our internal stack.
  upb_pbdecoder_frame *top, *limit;
  upb_pbdecoder_frame stack[UPB_DECODER_MAX_NESTING];
  uint32_t *callstack[UPB_DECODER_MAX_NESTING * 2];
};

// Data pertaining to a single decoding method/function.
// Each method contains code to parse a single message type.
// If may or may not be bound to a destination handlers object.
typedef struct {
  // While compiling, the base is relative in "ofs", after compiling it is
  // absolute in "ptr".
  union {
    uint32_t ofs;     // PC offset of method.
    const void *ptr;  // Pointer to bytecode or machine code for this method.
  } base;

  // Whether this method is native code or bytecode.
  bool native_code;

  // The message type that this method is parsing.
  const upb_msgdef *msg;

  // The destination handlers this method is bound to, or NULL if this method
  // can be bound to a destination handlers instance at runtime.
  //
  // If non-NULL, we own a ref.
  const upb_handlers *dest_handlers;

  // The dispatch table layout is:
  //   [field number] -> [ 48-bit offset ][ 8-bit wt2 ][ 8-bit wt1 ]
  //
  // If wt1 matches, jump to the 48-bit offset.  If wt2 matches, lookup
  // (UPB_MAX_FIELDNUMBER + fieldnum) and jump there.
  //
  // We need two wire types because of packed/non-packed compatibility.  A
  // primitive repeated field can use either wire type and be valid.  While we
  // could key the table on fieldnum+wiretype, the table would be 8x sparser.
  //
  // Storing two wire types in the primary value allows us to quickly rule out
  // the second wire type without needing to do a separate lookup (this case is
  // less common than an unknown field).
  upb_inttable dispatch;
} upb_pbdecodermethod;

struct _upb_pbdecoderplan {
  // Pointer to bytecode.
  uint32_t *code, *code_end;

  // Maps upb_msgdef*/upb_handlers* -> upb_pbdecodermethod
  upb_inttable methods;

  // The method that starts parsing when we first call into the plan.
  // Ideally we will remove the idea that any of the methods in the plan
  // are special like this, so that any method can be the top-level one.
  upb_pbdecodermethod *topmethod;

#ifdef UPB_USE_JIT_X64
  // JIT-generated machine code (else NULL).
  upb_string_handler *jit_code;
  size_t jit_size;
  char *debug_info;
  void *dl;
#endif
};

#endif  // UPB_DECODER_INT_H_
