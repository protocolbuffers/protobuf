/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009-2010 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * Implements a set of upb_handlers that write protobuf data to the binary wire
 * format.
 *
 * This encoder implementation does not have any access to any out-of-band or
 * precomputed lengths for submessages, so it must buffer submessages internally
 * before it can emit the first byte.
 */

#ifndef UPB_ENCODER_H_
#define UPB_ENCODER_H_

#include "upb/sink.h"

#ifdef __cplusplus
namespace upb {
namespace pb {
class Encoder;
}  // namespace pb
}  // namespace upb
#endif

UPB_DECLARE_TYPE(upb::pb::Encoder, upb_pb_encoder);

#define UPB_PBENCODER_MAX_NESTING 100

/* upb::pb::Encoder ***********************************************************/

// The output buffer is divided into segments; a segment is a string of data
// that is "ready to go" -- it does not need any varint lengths inserted into
// the middle.  The seams between segments are where varints will be inserted
// once they are known.
//
// We also use the concept of a "run", which is a range of encoded bytes that
// occur at a single submessage level.  Every segment contains one or more runs.
//
// A segment can span messages.  Consider:
//
//                  .--Submessage lengths---------.
//                  |       |                     |
//                  |       V                     V
//                  V      | |---------------    | |-----------------
// Submessages:    | |-----------------------------------------------
// Top-level msg: ------------------------------------------------------------
//
// Segments:          -----   -------------------   -----------------
// Runs:              *----   *--------------*---   *----------------
// (* marks the start)
//
// Note that the top-level menssage is not in any segment because it does not
// have any length preceding it.
//
// A segment is only interrupted when another length needs to be inserted.  So
// observe how the second segment spans both the inner submessage and part of
// the next enclosing message.
typedef struct {
 UPB_PRIVATE_FOR_CPP
  uint32_t msglen;  // The length to varint-encode before this segment.
  uint32_t seglen;  // Length of the segment.
} upb_pb_encoder_segment;

UPB_DEFINE_CLASS0(upb::pb::Encoder,
 public:
  Encoder(const upb::Handlers* handlers);
  ~Encoder();

  static reffed_ptr<const Handlers> NewHandlers(const upb::MessageDef* msg);

  // Resets the state of the printer, so that it will expect to begin a new
  // document.
  void Reset();

  // Resets the output pointer which will serve as our closure.
  void ResetOutput(BytesSink* output);

  // The input to the encoder.
  Sink* input();

 private:
  UPB_DISALLOW_COPY_AND_ASSIGN(Encoder);
,
UPB_DEFINE_STRUCT0(upb_pb_encoder, UPB_QUOTE(
  // Our input and output.
  upb_sink input_;
  upb_bytessink *output_;

  // The "subclosure" -- used as the inner closure as part of the bytessink
  // protocol.
  void *subc;

  // The output buffer and limit, and our current write position.  "buf"
  // initially points to "initbuf", but is dynamically allocated if we need to
  // grow beyond the initial size.
  char *buf, *ptr, *limit;

  // The beginning of the current run, or undefined if we are at the top level.
  char *runbegin;

  // The list of segments we are accumulating.
  upb_pb_encoder_segment *segbuf, *segptr, *seglimit;

  // The stack of enclosing submessages.  Each entry in the stack points to the
  // segment where this submessage's length is being accumulated.
  int stack[UPB_PBENCODER_MAX_NESTING], *top, *stacklimit;

  // Depth of startmsg/endmsg calls.
  int depth;

  // Initial buffers for the output buffer and segment buffer.  If we outgrow
  // these we will dynamically allocate bigger ones.
  char initbuf[256];
  upb_pb_encoder_segment seginitbuf[32];
)));

UPB_BEGIN_EXTERN_C

const upb_handlers *upb_pb_encoder_newhandlers(const upb_msgdef *m,
                                               const void *owner);
void upb_pb_encoder_reset(upb_pb_encoder *e);
upb_sink *upb_pb_encoder_input(upb_pb_encoder *p);
void upb_pb_encoder_init(upb_pb_encoder *e, const upb_handlers *h);
void upb_pb_encoder_resetoutput(upb_pb_encoder *e, upb_bytessink *output);
void upb_pb_encoder_uninit(upb_pb_encoder *e);

UPB_END_EXTERN_C

#ifdef __cplusplus

namespace upb {
namespace pb {
inline Encoder::Encoder(const upb::Handlers* handlers) {
  upb_pb_encoder_init(this, handlers);
}
inline Encoder::~Encoder() {
  upb_pb_encoder_uninit(this);
}
inline void Encoder::Reset() {
  upb_pb_encoder_reset(this);
}
inline void Encoder::ResetOutput(BytesSink* output) {
  upb_pb_encoder_resetoutput(this, output);
}
inline Sink* Encoder::input() {
  return upb_pb_encoder_input(this);
}
inline reffed_ptr<const Handlers> Encoder::NewHandlers(
    const upb::MessageDef *md) {
  const Handlers* h = upb_pb_encoder_newhandlers(md, &h);
  return reffed_ptr<const Handlers>(h, &h);
}
}  // namespace pb
}  // namespace upb

#endif

#endif  /* UPB_ENCODER_H_ */
