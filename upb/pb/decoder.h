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
#include <stdbool.h>
#include <stdint.h>
#include "upb/handlers.h"

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

// TODO: add parameter for a list of other decoder plans that we can share
// generated code with.
upb_decoderplan *upb_decoderplan_new(upb_handlers *h, bool allowjit);
void upb_decoderplan_unref(upb_decoderplan *p);

// Returns true if the plan contains JIT-ted code.  This may not be the same as
// the "allowjit" parameter to the constructor if support for JIT-ting was not
// compiled in.
bool upb_decoderplan_hasjitcode(upb_decoderplan *p);


/* upb_decoder ****************************************************************/

struct dasm_State;

typedef struct _upb_decoder {
  upb_decoderplan *plan;
  int             msg_offset;      // Which message from the plan is top-level.
  upb_byteregion  *input;          // Input data (serialized), not owned.
  upb_dispatcher  dispatcher;      // Dispatcher to which we push parsed data.
  upb_status      status;          // Where we store errors that occur.
  upb_byteregion  str_byteregion;  // For passing string data to callbacks.

  upb_inttable    *dispatch_table;

  // Current input buffer and its stream offset.
  const char *buf, *ptr, *end;
  uint64_t bufstart_ofs;

  // End of the delimited region, relative to ptr, or NULL if not in this buf.
  const char *delim_end;
  // True if the top stack frame represents a packed field.
  bool top_is_packed;

#ifdef UPB_USE_JIT_X64
  // For JIT, which doesn't do bounds checks in the middle of parsing a field.
  const char *jit_end, *effective_end;  // == MIN(jit_end, submsg_end)
#endif

  // For exiting the decoder on error.
  sigjmp_buf exitjmp;
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
void upb_decoder_resetplan(upb_decoder *d, upb_decoderplan *p, int msg_offset);

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
  upb_handlers *handlers;  // owns reference.

#ifdef UPB_USE_JIT_X64
  // JIT-generated machine code (else NULL).
  char *jit_code;
  size_t jit_size;
  char *debug_info;

  // This pointer is allocated by dasm_init() and freed by dasm_free().
  struct dasm_State *dynasm;
#endif
};

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_DECODER_H_ */
