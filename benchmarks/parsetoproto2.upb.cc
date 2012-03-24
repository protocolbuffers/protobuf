// Tests speed of upb parsing into proto2 generated classes.

#define __STDC_LIMIT_MACROS 1
#include "main.c"

#include <stdint.h>
#include "upb/bytestream.hpp"
#include "upb/def.hpp"
#include "upb/msg.hpp"
#include "upb/pb/decoder.hpp"
#include "upb/pb/glue.h"
#include "upb/proto2_bridge.hpp"
#include MESSAGE_HFILE

const char *str;
size_t len;
MESSAGE_CIDENT msg[NUM_MESSAGES];
MESSAGE_CIDENT msg2;
upb::StringSource strsrc;
upb::Decoder d;
const upb::MessageDef *def;
upb::DecoderPlan* plan;

static bool initialize()
{
  // Read the message data itself.
  str = upb_readfile(MESSAGE_FILE, &len);
  if(str == NULL) {
    fprintf(stderr, "Error reading " MESSAGE_FILE "\n");
    return false;
  }

  def = upb::proto2_bridge::NewFinalMessageDef(msg2, &def);

  msg2.ParseFromArray(str, len);

  upb::Handlers* h = upb::Handlers::New();
  upb::RegisterWriteHandlers(h, def);
  plan = upb::DecoderPlan::New(h, JIT);
  d.ResetPlan(plan, 0);
  h->Unref();

  return true;
}

static void cleanup() {
  def->Unref(&def);
  plan->Unref();
}

static size_t run(int i) {
  msg[i % NUM_MESSAGES].Clear();
  strsrc.Reset(str, len);
  d.ResetInput(strsrc.AllBytes(), &msg[i % NUM_MESSAGES]);
  if (d.Decode() != UPB_OK) goto err;
  return len;

err:
  fprintf(stderr, "Decode error: %s", d.status().GetString());
  return 0;
}
