/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009-2010 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * upb_decoder implements a high performance, streaming decoder for protobuf
 * data that works by getting its input data from a upb_bytesrc and calling
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
  upb_bytesrc *bytesrc;       // Source of our serialized data.
  upb_dispatcher dispatcher;  // Dispatcher to which we push parsed data.
  upb_status *status;         // Where we will store any errors that occur.
  upb_strref strref;          // For passing string data to callbacks.

  // Offsets for the bytesrc region we currently have ref'd.
  uint64_t refstart_ofs, refend_ofs;

  // Current input buffer and its stream offset.
  const char *buf, *ptr, *end;
  uint64_t bufstart_ofs, bufend_ofs;

  // Stream offset for the end of the top-level message, if any.
  uint64_t end_ofs;

  // Buf offset as of which we've delivered calbacks; needed for rollback if
  // a callback returns UPB_BREAK.
  const char *completed_ptr;

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

// Used for frames that have no specific end offset: groups, repeated primitive
// fields inside groups, and the top-level message.
#define UPB_NONDELIMITED UINT64_MAX

// Initializes/uninitializes a decoder for calling into the given handlers
// or to write into the given msgdef, given its accessors).  Takes a ref
// on the handlers.
void upb_decoder_init(upb_decoder *d, upb_handlers *h);
void upb_decoder_uninit(upb_decoder *d);

// Resets the internal state of an already-allocated decoder.  This puts it in a
// state where it has not seen any data, and expects the next data to be from
// the beginning of a new protobuf.  Parsers must be reset before they can be
// used.  A decoder can be reset multiple times.
//
// Pass UINT64_MAX for end_ofs to indicate a non-delimited top-level message.
void upb_decoder_reset(upb_decoder *d, upb_bytesrc *src, uint64_t start_ofs,
                       uint64_t end_ofs, void *closure);

void upb_decoder_decode(upb_decoder *d, upb_status *status);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_DECODER_H_ */
