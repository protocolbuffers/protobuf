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

// A upb_decoder decodes the binary protocol buffer format, writing the data it
// decodes to a upb_sink.
struct upb_decoder;
typedef struct upb_decoder upb_decoder;

// Allocates and frees a upb_decoder, respectively.
upb_decoder *upb_decoder_new(upb_msgdef *md);
void upb_decoder_free(upb_decoder *d);

// Resets the internal state of an already-allocated decoder.  This puts it in a
// state where it has not seen any data, and expects the next data to be from
// the beginning of a new protobuf.  Parsers must be reset before they can be
// used.  A decoder can be reset multiple times.
void upb_decoder_reset(upb_decoder *d, upb_bytesrc *bytesrc);

// Returns a upb_src pointer by which the decoder can be used.  The returned
// upb_src is invalidated by upb_decoder_reset() or upb_decoder_free().
upb_src *upb_decoder_getsrc(upb_decoder *d);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_DECODER_H_ */
