/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009-2010 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * upb_decoder implements a high performance, streaming decoder for protobuf
 * data that works by getting its input data from a upb_byteregion and calling
 * into a upb_handlers.
 */

#ifndef UPB_DECODER_H_
#define UPB_DECODER_H_

#include <setjmp.h>
#include "upb/bytestream.h"
#include "upb/sink.h"

#ifdef __cplusplus
extern "C" {
#endif

/* upb_decoderplan ************************************************************/

// A decoderplan contains whatever data structures and generated (JIT-ted) code
// are necessary to decode protobuf data of a specific type to a specific set
// of handlers.  By generating the plan ahead of time, we avoid having to
// redo this work every time we decode.
//
// A decoderplan is threadsafe, meaning that it can be used concurrently by
// different upb_decoders in different threads.  However, the upb_decoders are
// *not* thread-safe.
struct _upb_decoderplan;
typedef struct _upb_decoderplan upb_decoderplan;

// TODO(haberman):
// - add support for letting any message in the plan be at the top level.
// - make this object a handlers instead (when bytesrc/bytesink are merged
//   into handlers).
// - add support for sharing code with previously-built plans/handlers.
upb_decoderplan *upb_decoderplan_new(const upb_handlers *h, bool allowjit);
void upb_decoderplan_unref(upb_decoderplan *p);

// Returns true if the plan contains JIT-ted code.  This may not be the same as
// the "allowjit" parameter to the constructor if support for JIT-ting was not
// compiled in.
bool upb_decoderplan_hasjitcode(upb_decoderplan *p);


/* upb_decoder ****************************************************************/

struct dasm_State;

typedef struct {
  const upb_fielddef *f;
  uint64_t end_ofs;
  uint32_t group_fieldnum;  // UINT32_MAX for non-groups.
  bool is_sequence;   // frame represents seq or submsg? (f might be both).
  bool is_packed;     // !upb_issubmsg(f) && end_ofs != UINT64_MAX
                      // (strings aren't pushed).
} upb_decoder_frame;

typedef struct _upb_decoder {
  upb_decoderplan *plan;
  upb_byteregion  *input;          // Input data (serialized), not owned.
  upb_status      status;          // Where we store errors that occur.

  // Where we push parsed data.
  // TODO(haberman): make this a pointer and make upb_decoder_resetinput() take
  // one of these instead of a void*.
  upb_sink        sink;

  // Our internal stack.
  upb_decoder_frame *top, *limit;
  upb_decoder_frame stack[UPB_MAX_NESTING];

  // Current input buffer and its stream offset.
  const char *buf, *ptr, *end;
  uint64_t bufstart_ofs;

  // End of the delimited region, relative to ptr, or NULL if not in this buf.
  const char *delim_end;
  // True if the top stack frame represents a packed field.
  bool top_is_packed;

#ifdef UPB_USE_JIT_X64
  // For JIT, which doesn't do bounds checks in the middle of parsing a field.
  const char *jit_end, *effective_end;  // == MIN(jit_end, delim_end)

  // Used momentarily by the generated code to store a value while a user
  // function is called.
  uint32_t tmp_len;
#endif

  // For exiting the decoder on error.
  jmp_buf exitjmp;
} upb_decoder;

void upb_decoder_init(upb_decoder *d);
void upb_decoder_uninit(upb_decoder *d);

// Resets the plan that the decoder will parse from.  "msg_offset" indicates
// which message from the plan will be used as the top-level message.
//
// This will also reset the decoder's input to be uninitialized --
// upb_decoder_resetinput() must be called before parsing can occur.  The plan
// must live until the decoder is destroyed or reset to a different plan.
//
// Must be called before upb_decoder_resetinput() or upb_decoder_decode().
void upb_decoder_resetplan(upb_decoder *d, upb_decoderplan *p);

// Resets the input of an already-allocated decoder.  This puts it in a state
// where it has not seen any data, and expects the next data to be from the
// beginning of a new protobuf.  Decoders must have their input reset before
// they can be used.  A decoder can have its input reset multiple times.
// "input" must live until the decoder is destroyed or has it input reset
// again. "c" is the closure that will be passed to the handlers.
//
// Must be called before upb_decoder_decode().
void upb_decoder_resetinput(upb_decoder *d, upb_byteregion *input, void *c);

// Decodes serialized data (calling handlers as the data is parsed), returning
// the success of the operation (call upb_decoder_status() for details).
upb_success_t upb_decoder_decode(upb_decoder *d);

INLINE const upb_status *upb_decoder_status(upb_decoder *d) {
  return &d->status;
}

// Implementation details

struct _upb_decoderplan {
  // The top-level handlers that this plan calls into.  We own a ref.
  const upb_handlers *handlers;

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
};

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_DECODER_H_ */
