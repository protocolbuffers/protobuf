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
