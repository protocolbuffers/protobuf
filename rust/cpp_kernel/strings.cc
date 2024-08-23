#include "rust/cpp_kernel/strings.h"

#include <cstring>
#include <string>

#include "rust/cpp_kernel/rust_alloc_for_cpp_api.h"

namespace google {
namespace protobuf {
namespace rust {

RustStringRawParts::RustStringRawParts(std::string src) {
  if (src.empty()) {
    data = nullptr;
    len = 0;
  } else {
    void* d = proto2_rust_alloc(src.length(), 1);
    std::memcpy(d, src.data(), src.length());
    data = static_cast<char*>(d);
    len = src.length();
  }
}

}  // namespace rust
}  // namespace protobuf
}  // namespace google

extern "C" {
std::string* proto2_rust_cpp_new_string(google::protobuf::rust::PtrAndLen src) {
  return new std::string(src.ptr, src.len);
}

void proto2_rust_cpp_delete_string(std::string* str) { delete str; }

google::protobuf::rust::PtrAndLen proto2_rust_cpp_string_to_view(std::string* str) {
  return google::protobuf::rust::PtrAndLen{str->data(), str->length()};
}
}
