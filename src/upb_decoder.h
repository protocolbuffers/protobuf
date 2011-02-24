/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * upb_decoder implements a high performance, streaming decoder for protobuf
 * data that works by implementing upb_src and getting its data from a
 * upb_bytesrc.
 *
 * The decoder does not currently support non-blocking I/O, in the sense that
 * if the bytesrc returns UPB_STATUS_TRYAGAIN it is not possible to resume the
 * decoder when data becomes available again.  Support for this could be added,
 * but it would add complexity and perhaps cost efficiency also.
 *
 * Copyright (c) 2009-2010 Joshua Haberman.  See LICENSE for details.
 */

#ifndef UPB_DECODER_H_
#define UPB_DECODER_H_

#include <stdbool.h>
#include <stdint.h>
#include "upb_def.h"
#include "upb_stream.h"

#ifdef __cplusplus
extern "C" {
#endif

/* upb_decoder *****************************************************************/

// The decoder keeps a stack with one entry per level of recursion.
// upb_decoder_frame is one frame of that stack.
typedef struct {
  upb_msgdef *msgdef;
  size_t end_offset;  // For groups, 0.
} upb_decoder_frame;

struct _upb_decoder {
  // Immutable state of the decoder.
  upb_src src;
  upb_dispatcher dispatcher;
  upb_bytesrc *bytesrc;

  // Mutable state of the decoder.

  // Msgdef for the current level.
  upb_msgdef *msgdef;

  // Stack entries store the offset where the submsg ends (for groups, 0).
  upb_decoder_frame *top, *limit;

  // Current input buffer.
  upb_string *buf;

  // Temporary string for passing to callbacks.
  upb_string *tmp;

  // The offset within the overall stream represented by the *beginning* of buf.
  size_t buf_stream_offset;

  // Our current position in the data buffer.
  const char *ptr;

  // End of this buffer, relative to *ptr.
  const char *end;

  // End of this submessage, relative to *ptr.
  const char *submsg_end;

  // Where we will store any errors that occur.
  upb_status *status;

  upb_decoder_frame stack[UPB_MAX_NESTING];
};

// A upb_decoder decodes the binary protocol buffer format, writing the data it
// decodes to a upb_sink.
struct _upb_decoder;
typedef struct _upb_decoder upb_decoder;

// Allocates and frees a upb_decoder, respectively.
void upb_decoder_init(upb_decoder *d, upb_msgdef *md);
void upb_decoder_uninit(upb_decoder *d);

// Resets the internal state of an already-allocated decoder.  This puts it in a
// state where it has not seen any data, and expects the next data to be from
// the beginning of a new protobuf.  Parsers must be reset before they can be
// used.  A decoder can be reset multiple times.
void upb_decoder_reset(upb_decoder *d, upb_bytesrc *bytesrc);

// Returns a upb_src pointer by which the decoder can be used.  The returned
// upb_src is invalidated by upb_decoder_reset() or upb_decoder_free().
upb_src *upb_decoder_src(upb_decoder *d);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_DECODER_H_ */
