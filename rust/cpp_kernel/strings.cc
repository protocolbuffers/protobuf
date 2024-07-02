#include "rust/cpp_kernel/strings.h"

#include <cstring>
#include <string>

#include "rust/cpp_kernel/rust_alloc_for_cpp_api.h"

namespace google {
namespace protobuf {
namespace rust_internal {

RustStringRawParts::RustStringRawParts(std::string src) {
  if (src.empty()) {
    data = nullptr;
    len = 0;
  } else {
    void* d = __pb_rust_alloc(src.length(), 1);
    std::memcpy(d, src.data(), src.length());
    data = static_cast<char*>(d);
    len = src.length();
  }
}

}  // namespace rust_internal
}  // namespace protobuf
}  // namespace google

extern "C" {
std::string* third_party_protobuf_rust_cpp_kernel_new_string(
    google::protobuf::rust_internal::PtrAndLen src) {
  return new std::string(src.ptr, src.len);
}

void third_party_protobuf_rust_cpp_kernel_delete_string(std::string* str) {
  delete str;
}

google::protobuf::rust_internal::PtrAndLen
third_party_protobuf_rust_cpp_kernel_to_string_view(std::string* str) {
  return google::protobuf::rust_internal::PtrAndLen(str->data(), str->length());
}
}
