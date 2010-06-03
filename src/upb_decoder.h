/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * upb_decoder implements a high performance, callback-based, stream-oriented
 * decoder (comparable to the SAX model in XML parsers).  For parsing protobufs
 * into in-memory messages (a more DOM-like model), see the routines in
 * upb_msg.h, which are layered on top of this decoder.
 *
 * TODO: the decoder currently does not support returning unknown values.  This
 * can easily be added when it is needed.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 */

#ifndef UPB_DECODER_H_
#define UPB_DECODER_H_

#include <stdbool.h>
#include <stdint.h>
#include "upb.h"
#include "descriptor.h"

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
void upb_decoder_free(upb_decoder *p);

// Resets the internal state of an already-allocated decoder.  This puts it in a
// state where it has not seen any data, and expects the next data to be from
// the beginning of a new protobuf.  Parsers must be reset before they can be
// used.  A decoder can be reset multiple times.
void upb_decoder_reset(upb_decoder *p, upb_bytesrc *bytesrc);

// Returns a upb_src pointer by which the decoder can be used.  The returned
// upb_src is invalidated by upb_decoder_reset().
upb_src *upb_decoder_getsrc(upb_decoder *p);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_DECODER_H_ */
