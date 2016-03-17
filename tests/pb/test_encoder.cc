
#include "tests/upb_test.h"
#include "upb/bindings/stdc++/string.h"
#include "upb/descriptor/descriptor.upbdefs.h"
#include "upb/pb/decoder.h"
#include "upb/pb/encoder.h"
#include "upb/pb/glue.h"

static char *upb_readfile(const char *filename, size_t *len) {
  long size;
  char *buf;
  FILE *f = fopen(filename, "rb");
  if(!f) return NULL;
  if(fseek(f, 0, SEEK_END) != 0) goto error;
  size = ftell(f);
  if(size < 0) goto error;
  if(fseek(f, 0, SEEK_SET) != 0) goto error;
  buf = (char*)malloc(size + 1);
  if(size && fread(buf, size, 1, f) != 1) goto error;
  fclose(f);
  if (len) *len = size;
  return buf;

error:
  fclose(f);
  return NULL;
}

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

  char buf[512];
  upb::SeededAllocator alloc(buf, sizeof(buf));
  upb::Environment env;
  env.SetAllocator(&alloc);
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
