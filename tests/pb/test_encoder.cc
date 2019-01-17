
#include "tests/test_util.h"
#include "tests/upb_test.h"
#include "upb/bindings/stdc++/string.h"
#include "google/protobuf/descriptor.upb.h"
#include "upb/pb/decoder.h"
#include "upb/pb/encoder.h"

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
  std::string input = read_string("google/protobuf/descriptor.pb");
  upb::SymbolTable symtab;
  upb::HandlerCache encoder_cache(upb::pb::EncoderPtr::NewCache());
  upb::pb::CodeCache decoder_cache(&encoder_cache);
  upb::Arena arena;
  google_protobuf_FileDescriptorSet *set =
      google_protobuf_FileDescriptorSet_parsenew(
          upb_strview_make(input.c_str(), input.size()), arena.ptr());
  ASSERT(set);
  size_t n;
  const google_protobuf_FileDescriptorProto *const *files =
      google_protobuf_FileDescriptorSet_file(set, &n);
  ASSERT(n == 1);
  upb::Status status;
  upb::FileDefPtr file_def = symtab.AddFile(files[0], &status);
  if (!file_def) {
    fprintf(stderr, "Error building def: %s\n", status.error_message());
    ASSERT(false);
  }
  upb::MessageDefPtr md =
      symtab.LookupMessage("google.protobuf.FileDescriptorSet");
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
