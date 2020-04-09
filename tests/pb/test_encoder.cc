
#include <iostream>

#include "google/protobuf/descriptor.upb.h"
#include "google/protobuf/descriptor.upbdefs.h"
#include "tests/test_util.h"
#include "tests/upb_test.h"
#include "upb/bindings/stdc++/string.h"
#include "upb/pb/decoder.h"
#include "upb/pb/encoder.h"
#include "upb/port_def.inc"
#include "upb/upb.hpp"

void test_pb_roundtrip() {
  std::string input(
      google_protobuf_descriptor_proto_upbdefinit.descriptor.data,
      google_protobuf_descriptor_proto_upbdefinit.descriptor.size);
  std::cout << input.size() << "\n";
  upb::SymbolTable symtab;
  upb::HandlerCache encoder_cache(upb::pb::EncoderPtr::NewCache());
  upb::pb::CodeCache decoder_cache(&encoder_cache);
  upb::Arena arena;
  upb::Status status;
  upb::MessageDefPtr md(
      google_protobuf_FileDescriptorProto_getmsgdef(symtab.ptr()));
  ASSERT(md);
  const upb::Handlers *encoder_handlers = encoder_cache.Get(md);
  ASSERT(encoder_handlers);
  const upb::pb::DecoderMethodPtr method = decoder_cache.Get(md);

  std::string output;
  upb::StringSink string_sink(&output);
  upb::pb::EncoderPtr encoder =
      upb::pb::EncoderPtr::Create(&arena, encoder_handlers, string_sink.input());
  upb::pb::DecoderPtr decoder =
      upb::pb::DecoderPtr::Create(&arena, method, encoder.input(), &status);
  bool ok = upb::PutBuffer(input, decoder.input());
  ASSERT(ok);
  ASSERT(input == output);
}

extern "C" {
int run_tests(int argc, char *argv[]) {
  UPB_UNUSED(argc);
  UPB_UNUSED(argv);
  test_pb_roundtrip();
  return 0;
}
}
