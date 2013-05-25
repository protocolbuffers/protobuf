/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009-2010 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * upb::Decoder implements a high performance, streaming decoder for protobuf
 * data that works by parsing input data one buffer at a time and calling into
 * a upb::Handlers.
 */

#ifndef UPB_DECODER_H_
#define UPB_DECODER_H_

#include "upb/sink.h"

#ifdef __cplusplus
namespace upb {
namespace pb {

// Frame type that encapsulates decoder state.
class Decoder;

// Resets the sink of the Decoder.  This must be called at  least once before
// the decoder can be used.  It may only be called with the decoder is in a
// state where it was just created or reset.  The given sink must be from the
// same pipeline as this decoder.
inline bool ResetDecoderSink(Decoder* d, Sink* sink);

// Gets the handlers suitable for parsing protobuf data according to the given
// destination handlers.  The protobuf schema to parse is taken from dest.
inline const upb::Handlers *GetDecoderHandlers(const upb::Handlers *dest,
                                               bool allowjit,
                                               const void *owner);

// Returns true if these handlers represent a upb::pb::Decoder.
bool IsDecoder(const upb::Handlers *h);

// Returns true if IsDecoder(h) and the given handlers have JIT code.
inline bool HasJitCode(const upb::Handlers* h);

// Returns the destination handlers if IsDecoder(h), otherwise returns NULL.
const upb::Handlers* GetDestHandlers(const upb::Handlers* h);

}  // namespace pb
}  // namespace upb

typedef upb::pb::Decoder upb_pbdecoder;

extern "C" {
#else
struct upb_pbdecoder;
typedef struct upb_pbdecoder upb_pbdecoder;
#endif

// C API.
const upb_frametype *upb_pbdecoder_getframetype();
bool upb_pbdecoder_resetsink(upb_pbdecoder *d, upb_sink *sink);
const upb_handlers *upb_pbdecoder_gethandlers(const upb_handlers *dest,
                                              bool allowjit,
                                              const void *owner);
bool upb_pbdecoder_isdecoder(const upb_handlers *h);
bool upb_pbdecoder_hasjitcode(const upb_handlers *h);
const upb_handlers *upb_pbdecoder_getdesthandlers(const upb_handlers *h);

// C++ implementation details. /////////////////////////////////////////////////

#ifdef __cplusplus
}  // extern "C"

namespace upb {

namespace pb {
inline bool ResetDecoderSink(Decoder* r, Sink* sink) {
  return upb_pbdecoder_resetsink(r, sink);
}
inline const upb::Handlers* GetDecoderHandlers(const upb::Handlers* dest,
                                               bool allowjit,
                                               const void* owner) {
  return upb_pbdecoder_gethandlers(dest, allowjit, owner);
}
inline bool IsDecoder(const upb::Handlers* h) {
  return upb_pbdecoder_isdecoder(h);
}
inline bool HasJitCode(const upb::Handlers* h) {
  return upb_pbdecoder_hasjitcode(h);
}
inline const upb::Handlers* GetDestHandlers(const upb::Handlers* h) {
  return upb_pbdecoder_getdesthandlers(h);
}
}  // namespace pb
}  // namespace upb
#endif

#endif  /* UPB_DECODER_H_ */
