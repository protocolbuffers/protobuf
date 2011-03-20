/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009-2010 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * Implements a set of upb_handlers that write protobuf data to the binary wire
 * format.
 *
 * For messages that have any submessages, the encoder needs a buffer
 * containing the submessage sizes, so they can be properly written at the
 * front of each message.  Note that groups do *not* have this requirement.
 */

#ifndef UPB_ENCODER_H_
#define UPB_ENCODER_H_

#include "upb.h"
#include "upb_stream.h"

#ifdef __cplusplus
extern "C" {
#endif

/* upb_encoder ****************************************************************/

// A upb_encoder is a upb_sink that emits data to a upb_bytesink in the protocol
// buffer binary wire format.
struct upb_encoder;
typedef struct upb_encoder upb_encoder;

upb_encoder *upb_encoder_new(upb_msgdef *md);
void upb_encoder_free(upb_encoder *e);

// Resets the given upb_encoder such that is is ready to begin encoding,
// outputting data to "bytesink" (which must live until the encoder is
// reset or destroyed).
void upb_encoder_reset(upb_encoder *e, upb_bytesink *bytesink);

// Returns the upb_sink to which data can be written.  The sink is invalidated
// when the encoder is reset or destroyed.  Note that if the client wants to
// encode any length-delimited submessages it must first call
// upb_encoder_buildsizes() below.
upb_sink *upb_encoder_sink(upb_encoder *e);

// Call prior to pushing any data with embedded submessages.  "src" must yield
// exactly the same data as what will next be encoded, but in reverse order.
// The encoder iterates over this data in order to determine the sizes of the
// submessages.  If any errors are returned by the upb_src, the status will
// be saved in *status.  If the client is sure that the upb_src will not throw
// any errors, "status" may be NULL.
void upb_encoder_buildsizes(upb_encoder *e, upb_src *src, upb_status *status);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_ENCODER_H_ */
