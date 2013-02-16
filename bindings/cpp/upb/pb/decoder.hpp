//
// upb - a minimalist implementation of protocol buffers.
//
// Copyright (c) 2011 Google Inc.  See LICENSE for details.
// Author: Josh Haberman <jhaberman@gmail.com>
//
// upb::Decoder is a high performance, streaming decoder for protobuf
// data that works by getting its input data from a ubp::ByteRegion and calling
// into a upb::Handlers.
//
// A DecoderPlan contains whatever data structures and generated (JIT-ted) code
// are necessary to decode protobuf data of a specific type to a specific set
// of handlers.  By generating the plan ahead of time, we avoid having to
// redo this work every time we decode.
//
// A DecoderPlan is threadsafe, meaning that it can be used concurrently by
// different upb::Decoders in different threads.  However, the upb::Decoders are
// *not* thread-safe.

#ifndef UPB_PB_DECODER_HPP
#define UPB_PB_DECODER_HPP

#include "upb/pb/decoder.h"

#include "upb/bytestream.h"
#include "upb/upb.h"

namespace upb {

class DecoderPlan : public upb_decoderplan {
 public:
  static DecoderPlan* New(const Handlers* h, bool allow_jit) {
    return static_cast<DecoderPlan*>(upb_decoderplan_new(h, allow_jit));
  }
  void Unref() { upb_decoderplan_unref(this); }

  // Returns true if the plan contains JIT-ted code.  This may not be the same
  // as the "allowjit" parameter to the constructor if support for JIT-ting was
  // not compiled in.
  bool HasJitCode() { return upb_decoderplan_hasjitcode(this); }

 private:
  DecoderPlan() {}  // Only constructed by New
};

class Decoder : public upb_decoder {
 public:
  Decoder() { upb_decoder_init(this); }
  ~Decoder() { upb_decoder_uninit(this); }

  // Resets the plan that the decoder will parse from.  This will also reset the
  // decoder's input to be uninitialized -- ResetInput() must be called before
  // parsing can occur.  The plan must live until the decoder is destroyed or
  // reset to a different plan.
  //
  // Must be called before ResetInput() or Decode().
  void ResetPlan(DecoderPlan* plan) { upb_decoder_resetplan(this, plan); }

  // Resets the input of the decoder.  This puts it in a state where it has not
  // seen any data, and expects the next data to be from the beginning of a new
  // protobuf.
  //
  // ResetInput() must be called before Decode() but may be called more than
  // once.  "input" must live until the decoder destroyed or ResetInput is
  // called again.  "c" is the closure that will be passed to the handlers.
  void ResetInput(ByteRegion* byte_region, void* c) {
    upb_decoder_resetinput(this, byte_region, c);
  }

  // Decodes serialized data (calling Handlers as the data is parsed) until
  // error or EOF (see status() for details).
  Status::Success Decode() { return upb_decoder_decode(this); }

  const upb::Status& status() {
    return static_cast<const upb::Status&>(*upb_decoder_status(this));
  }
};

}  // namespace upb

#endif
