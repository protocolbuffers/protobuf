/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Implements a upb_sink that writes protobuf data to the binary wire format.
 *
 * For messages that have any submessages, the encoder needs a buffer
 * containing the submessage sizes, so they can be properly written at the
 * front of each message.  Note that groups do *not* have this requirement.
 *
 * Copyright (c) 2009-2010 Joshua Haberman.  See LICENSE for details.
 */

#ifndef UPB_ENCODER_H_
#define UPB_ENCODER_H_

#include "upb.h"
#include "upb_sink.h"

#ifdef __cplusplus
extern "C" {
#endif

/* upb_sizebuilder ************************************************************/

// A upb_sizebuilder performs a pre-pass on data to be serialized that gathers
// the sizes of submessages.  This size data is required for serialization,
// because we have to know at the beginning of a submessage how many encoded
// bytes the submessage will represent.
struct upb_sizebuilder;
typedef struct upb_sizebuilder upb_sizebuilder;

upb_sizebuilder *upb_sizebuilder_new();
void upb_sizebuilder_free(upb_sizebuilder *sb);

// Returns a sink that must be used to perform the pre-pass.  Note that the
// pre-pass *must* occur in the opposite order from the actual encode that
// follows, and the data *must* be identical both times (except for the
// reversed order.
upb_sink *upb_sizebuilder_sink(upb_sizebuilder *sb);


/* upb_encoder ****************************************************************/

// A upb_encoder is a upb_sink that emits data to a upb_bytesink in the protocol
// buffer binary wire format.
struct upb_encoder;
typedef struct upb_encoder upb_encoder;

upb_encoder *upb_encoder_new();
void upb_encoder_free(upb_encoder *s);

// Resets the given upb_encoder such that is is ready to begin encoding.  The
// upb_sizebuilder "sb" is used to determine submessage sizes; it must have
// previously been initialized by feeding it the same data in reverse order.
// "sb" may be null if and only if the data contains no submessages; groups
// are ok and do not require sizes to be precalculated.  The upb_bytesink
// "out" is where the encoded output data will be sent.
//
// Both "sb" and "out" must live until the encoder is either reset or freed.
void upb_encoder_reset(upb_encoder *s, upb_sizebuilder *sb, upb_bytesink *out);

// The upb_sink to which data can be sent to be encoded.  Note that this data
// must be identical to the data that was previously given to the sizebuilder
// (if any).
upb_sink *upb_encoder_sink(upb_encoder *s);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_ENCODER_H_ */
