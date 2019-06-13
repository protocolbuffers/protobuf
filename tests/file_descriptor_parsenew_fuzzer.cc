#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include "google/protobuf/descriptor.upb.h"
#include "upb/def.h"
#include "upb/msg.h"
#include "upb/upb.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  upb_strview strview =
      upb_strview_make(reinterpret_cast<const char*>(data), size);
  upb::Arena arena;
  google_protobuf_FileDescriptorProto_parsenew(strview, arena.ptr());
  return 0;
}
