// Tests speed of upb parsing into proto2 generated classes.

#define __STDC_LIMIT_MACROS 1
#include "main.c"

#include <stdint.h>
#include "upb/bytestream.h"
#include "upb/def.h"
#include "upb/pb/decoder.h"
#include "upb/pb/glue.h"
#include "upb/bindings/google/bridge.h"
#include MESSAGE_HFILE

const char *str;
size_t len;
MESSAGE_CIDENT msg[NUM_MESSAGES];
upb::SeededPipeline<8192> pipeline(upb_realloc, NULL);
upb::Sink *decoder_sink;
upb::Sink *proto2_sink;

static bool initialize()
{
  // Read the message data itself.
  str = upb_readfile(MESSAGE_FILE, &len);
  if(str == NULL) {
    fprintf(stderr, "Error reading " MESSAGE_FILE "\n");
    return false;
  }

  const upb::Handlers* h = upb::google::NewWriteHandlers(MESSAGE_CIDENT(), &h);
  const upb::Handlers* h2 = upb::pb::GetDecoderHandlers(h, JIT, &h2);

  proto2_sink = pipeline.NewSink(h);
  decoder_sink = pipeline.NewSink(h2);
  pipeline.DonateRef(h, &h);
  pipeline.DonateRef(h2, &h2);

  upb::pb::Decoder* d = decoder_sink->GetObject<upb::pb::Decoder>();
  upb::pb::ResetDecoderSink(d, proto2_sink);

  return true;
}

static void cleanup() {
}

static size_t run(int i) {
  pipeline.Reset();
  proto2_sink->Reset(&msg[i % NUM_MESSAGES]);
  msg[i % NUM_MESSAGES].Clear();

  if (!upb::PutStringToBytestream(decoder_sink, str, len)) {
    fprintf(stderr, "Decode error: %s", pipeline.status().GetString());
    return 0;
  }
  return len;
}
