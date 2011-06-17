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
#include "upb_handlers.h"

#ifdef __cplusplus
extern "C" {
#endif

/* upb_decoder *****************************************************************/

struct dasm_State;

struct _upb_decoder {
  // Bytesrc from which we pull serialized data.
  upb_bytesrc *bytesrc;

  // String to hold our input buffer; is only active if d->buf != NULL.
  upb_string *bufstr;

  // Temporary string for passing string data to callbacks.
  upb_string *tmp;

  // The offset within the overall stream represented by the *beginning* of buf.
  size_t buf_stream_offset;

  // Pointer to the beginning of our current data buffer, or NULL if none.
  const char *buf;

  // End of this buffer, relative to *ptr.
  const char *end;
  const char *jit_end;

  // Members which may also be written by the JIT:

  // Our current position in the data buffer.
  const char *ptr;

  // End of this submessage, relative to *ptr.
  const char *submsg_end;

  // MIN(end, submsg_end)
  const char *effective_end;

  upb_fhandlers *f;

  // Where we will store any errors that occur.
  upb_status *status;

  // Dispatcher to which we push parsed data.
  upb_dispatcher dispatcher;

  // JIT-generated machine code (else NULL).
  char *jit_code;
  size_t jit_size;
  char *debug_info;

  struct dasm_State *dynasm;
  sigjmp_buf exitjmp;
};

// For use in the upb_dispatcher's stack.
typedef struct {
  // Relative to the beginning of this buffer.
  // For groups and the top-level: UINT32_MAX.
  uint32_t end_offset;
  bool is_packed;  // == !upb_issubmsg(f) && end_offset != UPB_REPATEDEND
} upb_decoder_srcdata;

// A upb_decoder decodes the binary protocol buffer format, writing the data it
// decodes to a upb_sink.
struct _upb_decoder;
typedef struct _upb_decoder upb_decoder;

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
void upb_decoder_reset(upb_decoder *d, upb_bytesrc *bytesrc, void *closure);

void upb_decoder_decode(upb_decoder *d, upb_status *status);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_DECODER_H_ */
