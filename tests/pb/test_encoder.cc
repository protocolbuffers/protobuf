
#include <iostream>

#include "google/protobuf/descriptor.upb.h"
#include "google/protobuf/descriptor.upbdefs.h"
#include "tests/test_util.h"
#include "tests/upb_test.h"
#include "upb/pb/decoder.h"
#include "upb/pb/encoder.h"
#include "upb/port_def.inc"
#include "upb/upb.hpp"

template <class T>
class FillStringHandler {
 public:
  static void SetHandler(upb_byteshandler* handler) {
    upb_byteshandler_setstartstr(handler, &FillStringHandler::StartString,
                                 NULL);
    upb_byteshandler_setstring(handler, &FillStringHandler::StringBuf, NULL);
  }

 private:
  // TODO(haberman): add UpbBind/UpbMakeHandler support to BytesHandler so these
  // can be prettier callbacks.
  static void* StartString(void *c, const void *hd, size_t size) {
    UPB_UNUSED(hd);
    UPB_UNUSED(size);

    T* str = static_cast<T*>(c);
    str->clear();
    return c;
  }

  static size_t StringBuf(void* c, const void* hd, const char* buf, size_t n,
                          const upb_bufhandle* h) {
    UPB_UNUSED(hd);
    UPB_UNUSED(h);

    T* str = static_cast<T*>(c);
    try {
      str->append(buf, n);
      return n;
    } catch (const std::exception&) {
      return 0;
    }
  }
};

class StringSink {
 public:
  template <class T>
  explicit StringSink(T* target) {
    // TODO(haberman): we need to avoid rebuilding a new handler every time,
    // but with class globals disallowed for google3 C++ this is tricky.
    upb_byteshandler_init(&handler_);
    FillStringHandler<T>::SetHandler(&handler_);
    input_.Reset(&handler_, target);
  }

  upb::BytesSink input() { return input_; }

 private:
  upb_byteshandler handler_;
  upb::BytesSink input_;
};

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
  StringSink string_sink(&output);
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
