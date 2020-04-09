#include <cstdint>

#include "google/protobuf/descriptor.upb.h"
#include "upb/upb.hpp"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  upb::Arena arena;
  google_protobuf_FileDescriptorProto_parse(reinterpret_cast<const char*>(data),
                                            size, arena.ptr());
  return 0;
}

#ifndef HAVE_FUZZER
int main() {}
#endif
