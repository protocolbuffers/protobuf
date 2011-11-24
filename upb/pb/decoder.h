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

/* upb_decoder *****************************************************************/

struct dasm_State;

typedef struct _upb_decoder {
  upb_byteregion *input;          // Input data (serialized).
  upb_dispatcher dispatcher;      // Dispatcher to which we push parsed data.
  upb_status     *status;         // Where we will store any errors that occur.
  upb_byteregion str_byteregion;  // For passing string data to callbacks.

  // Current input buffer and its stream offset.
  const char *buf, *ptr, *end;
  uint64_t bufstart_ofs;

  // End of the delimited region, relative to ptr, or NULL if not in this buf.
  const char *delim_end;
  bool top_is_packed;

#ifdef UPB_USE_JIT_X64
  // For JIT, which doesn't do bounds checks in the middle of parsing a field.
  const char *jit_end, *effective_end;  // == MIN(jit_end, submsg_end)

  // JIT-generated machine code (else NULL).
  char *jit_code;
  size_t jit_size;
  char *debug_info;

  struct dasm_State *dynasm;
#endif

  // For exiting the decoder on error.
  sigjmp_buf exitjmp;
} upb_decoder;

// Initializes/uninitializes a decoder for calling into the given handlers
// or to write into the given msgdef, given its accessors).  Takes a ref
// on the handlers.
void upb_decoder_init(upb_decoder *d, upb_handlers *h);
void upb_decoder_uninit(upb_decoder *d);

// Resets the internal state of an already-allocated decoder.  This puts it in a
// state where it has not seen any data, and expects the next data to be from
// the beginning of a new protobuf.  Decoders must be reset before they can be
// used.  A decoder can be reset multiple times.  "input" must live until the
// decoder is reset again (or destroyed).
void upb_decoder_reset(upb_decoder *d, upb_byteregion *input, void *closure);

// Decodes serialized data (calling handlers as the data is parsed) until error
// or EOF (see *status for details).
void upb_decoder_decode(upb_decoder *d, upb_status *status);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_DECODER_H_ */
