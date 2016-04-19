
#include "tests/test_util.h"
#include "tests/upb_test.h"
#include "upb/bindings/stdc++/string.h"
#include "upb/descriptor/descriptor.upbdefs.h"
#include "upb/pb/decoder.h"
#include "upb/pb/encoder.h"
#include "upb/pb/glue.h"

std::string read_string(const char *filename) {
  size_t len;
  char *str = upb_readfile(filename, &len);
  ASSERT(str);
  if (!str) { return std::string(); }
  std::string ret = std::string(str, len);
  free(str);
  return ret;
}

void test_pb_roundtrip() {
  upb::reffed_ptr<const upb::MessageDef> md(
      upbdefs::google::protobuf::FileDescriptorSet::get());
  upb::reffed_ptr<const upb::Handlers> encoder_handlers(
      upb::pb::Encoder::NewHandlers(md.get()));
  upb::reffed_ptr<const upb::pb::DecoderMethod> method(
      upb::pb::DecoderMethod::New(
          upb::pb::DecoderMethodOptions(encoder_handlers.get())));

  upb::InlinedEnvironment<512> env;
  std::string input = read_string("upb/descriptor/descriptor.pb");
  std::string output;
  upb::StringSink string_sink(&output);
  upb::pb::Encoder* encoder =
      upb::pb::Encoder::Create(&env, encoder_handlers.get(),
                               string_sink.input());
  upb::pb::Decoder* decoder =
      upb::pb::Decoder::Create(&env, method.get(), encoder->input());
  bool ok = upb::BufferSource::PutBuffer(input, decoder->input());
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
