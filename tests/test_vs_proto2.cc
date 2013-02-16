/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2011-2012 Google Inc.  See LICENSE for details.
 *
 * A test that verifies that our results are identical to proto2 for a
 * given proto type and input protobuf.
 */

#define __STDC_LIMIT_MACROS  // So we get UINT32_MAX
#include <assert.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/wire_format_lite.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "benchmarks/google_messages.pb.h"
#include "bindings/cpp/upb/pb/decoder.hpp"
#include "upb/def.h"
#include "upb/google/bridge.h"
#include "upb/handlers.h"
#include "upb/pb/glue.h"
#include "upb/pb/varint.h"
#include "upb_test.h"

void compare_metadata(const google::protobuf::Descriptor* d,
                      const upb::MessageDef *upb_md) {
  ASSERT(d->field_count() == upb_md->field_count());
  for (upb::MessageDef::ConstIterator i(upb_md); !i.Done(); i.Next()) {
    const upb::FieldDef* upb_f = i.field();
    const google::protobuf::FieldDescriptor *proto2_f =
        d->FindFieldByNumber(upb_f->number());
    ASSERT(upb_f);
    ASSERT(proto2_f);
    ASSERT(upb_f->number() == proto2_f->number());
    ASSERT(std::string(upb_f->name()) == proto2_f->name());
    ASSERT(upb_f->type() == static_cast<upb::FieldDef::Type>(proto2_f->type()));
    ASSERT(upb_f->IsSequence() == proto2_f->is_repeated());
  }
}

void parse_and_compare(MESSAGE_CIDENT *msg1, MESSAGE_CIDENT *msg2,
                       const upb::Handlers *handlers,
                       const char *str, size_t len, bool allow_jit) {
  // Parse to both proto2 and upb.
  ASSERT(msg1->ParseFromArray(str, len));

  upb::DecoderPlan* plan = upb::DecoderPlan::New(handlers, allow_jit);
  upb::StringSource src(str, len);
  upb::Decoder decoder;
  decoder.ResetPlan(plan);
  decoder.ResetInput(src.AllBytes(), msg2);
  msg2->Clear();
  ASSERT(decoder.Decode() == UPB_OK);
  plan->Unref();

  // Would like to just compare the message objects themselves,  but
  // unfortunately MessageDifferencer is not part of the open-source release of
  // proto2, so we compare their serialized strings, which we expect will be
  // equivalent.
  std::string str1;
  std::string str2;
  msg1->SerializeToString(&str1);
  msg2->SerializeToString(&str2);
  ASSERT(str1 == str2);
  ASSERT(std::string(str, len) == str2);
}

void test_zig_zag() {
  for (uint64_t num = 5; num * 1.5 > num; num *= 1.5) {
    ASSERT(upb_zzenc_64(num) ==
           google::protobuf::internal::WireFormatLite::ZigZagEncode64(num));
    if (num < UINT32_MAX) {
      ASSERT(upb_zzenc_32(num) ==
             google::protobuf::internal::WireFormatLite::ZigZagEncode32(num));
    }
  }

}

extern "C" {

int run_tests(int argc, char *argv[])
{
  if (argc < 2) {
    fprintf(stderr, "Usage: test_vs_proto2 <message file>\n");
    return 1;
  }
  const char *message_file = argv[1];

  // Read the message data itself.
  size_t len;
  const char *str = upb_readfile(message_file, &len);
  if(str == NULL) {
    fprintf(stderr, "Error reading %s\n", message_file);
    return 1;
  }

  MESSAGE_CIDENT msg1;
  MESSAGE_CIDENT msg2;

  const upb::Handlers* h = upb::google::NewWriteHandlers(msg1, &h);

  compare_metadata(msg1.GetDescriptor(), h->message_def());

  // Run twice to test proper object reuse.
  parse_and_compare(&msg1, &msg2, h, str, len, false);
  parse_and_compare(&msg1, &msg2, h, str, len, true);
  parse_and_compare(&msg1, &msg2, h, str, len, false);
  parse_and_compare(&msg1, &msg2, h, str, len, true);
  printf("All tests passed, %d assertions.\n", num_assertions);

  h->Unref(&h);
  free((void*)str);

  test_zig_zag();

  google::protobuf::ShutdownProtobufLibrary();
  return 0;
}

}
