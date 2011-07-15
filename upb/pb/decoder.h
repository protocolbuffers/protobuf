/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009-2010 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * upb_decoder implements a high performance, streaming decoder for protobuf
 * data that works by implementing upb_src and getting its data from a
 * upb_bytesrc.
 *
 * The decoder does not currently support non-blocking I/O, in the sense that
 * if the bytesrc returns UPB_STATUS_TRYAGAIN it is not possible to resume the
 * decoder when data becomes available again.  Support for this could be added,
 * but it would add complexity and perhaps cost efficiency also.
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

  // Offsets for the region we currently have ref'd.
  uint64_t refstart_ofs, refend_ofs;

  // Current buffer and its stream offset.
  const char *buf, *ptr, *end;
  uint64_t bufstart_ofs, bufend_ofs;

  // Stream offset for the end of the top-level message, if any.
  uint64_t end_ofs;

  // Buf offset as of which we've delivered calbacks; needed for rollback on
  // UPB_TRYAGAIN (or in the future, UPB_SUSPEND).
  const char *completed_ptr;

  // End of the delimited region, relative to ptr, or UINTPTR_MAX if not in
  // this buf.
  uintptr_t delim_end;

#ifdef UPB_USE_JIT_X64
  // For JIT, which doesn't do bounds checks in the middle of parsing a field.
  const char *jit_end, *effective_end;  // == MIN(jit_end, submsg_end)

  // JIT-generated machine code (else NULL).
  char *jit_code;
  size_t jit_size;
  char *debug_info;

  struct dasm_State *dynasm;
#endif

  sigjmp_buf exitjmp;
} upb_decoder;

// Initializes/uninitializes a decoder for calling into the given handlers
// or to write into the given msgdef, given its accessors).  Takes a ref
// on the handlers or msgdef.
void upb_decoder_initforhandlers(upb_decoder *d, upb_handlers *h);

// Equivalent to:
//   upb_accessors_reghandlers(m, h);
//   upb_decoder_initforhandlers(d, h);
// except possibly more efficient, by using cached state in the msgdef.
void upb_decoder_initformsgdef(upb_decoder *d, upb_msgdef *m);
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
