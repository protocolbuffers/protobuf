
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

// Method group; represents a set of decoder methods that had their code
// emitted together, and must therefore be freed together.  Immutable once
// created.  It is possible we may want to expose this to users at some point.
//
// Overall ownership of Decoder objects looks like this:
//
//                +----------+
//                |          | <---> DecoderMethod
//                | method   |
// CodeCache ---> |  group   | <---> DecoderMethod
//                |          |
//                | (mgroup) | <---> DecoderMethod
//                +----------+
typedef struct {
  upb_refcounted base;

  // Maps upb_msgdef/upb_handlers -> upb_pbdecodermethod.  We own refs on the
  // methods.
  upb_inttable methods;

  // When we add the ability to link to previously existing mgroups, we'll
  // need an array of mgroups we reference here, and own refs on them.

  // The bytecode for our methods, if any exists.  Owned by us.
  uint32_t *bytecode;
  uint32_t *bytecode_end;

#ifdef UPB_USE_JIT_X64
  // JIT-generated machine code, if any.
  upb_string_handlerfunc *jit_code;
  // The size of the jit_code (required to munmap()).
  size_t jit_size;
  char *debug_info;
  void *dl;
#endif
} mgroup;

// Decoder entry points; used as handlers.
void *upb_pbdecoder_startbc(void *closure, const void *pc, size_t size_hint);
void *upb_pbdecoder_startjit(void *closure, const void *hd, size_t size_hint);
size_t upb_pbdecoder_decode(void *closure, const void *hd, const char *buf,
                            size_t size, const upb_bufhandle *handle);
bool upb_pbdecoder_end(void *closure, const void *handler_data);

// Decoder-internal functions that the JIT calls to handle fallback paths.
void *upb_pbdecoder_resume(upb_pbdecoder *d, void *p, const char *buf,
                           size_t size, const upb_bufhandle *handle);
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

// Access to decoderplan members needed by the decoder.
const char *upb_pbdecoder_getopname(unsigned int op);

// JIT codegen entry point.
void upb_pbdecoder_jit(mgroup *group);
void upb_pbdecoder_freejit(mgroup *group);

// A special label that means "do field dispatch for this message and branch to
// wherever that takes you."
#define LABEL_DISPATCH 0

#define DECODE_OK -1
#define DECODE_MISMATCH -2  // Used only from checktag_slow().
#define DECODE_ENDGROUP -2  // Used only from checkunknown().

#endif  // UPB_DECODER_INT_H_
