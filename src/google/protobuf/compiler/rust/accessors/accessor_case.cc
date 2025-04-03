#include "google/protobuf/compiler/rust/accessors/accessor_case.h"

#include <string_view>

namespace google {
namespace protobuf {
namespace compiler {
namespace rust {

std::string_view ViewReceiver(AccessorCase accessor_case) {
  switch (accessor_case) {
    case AccessorCase::VIEW:
      return "self";
    case AccessorCase::OWNED:
    case AccessorCase::MUT:
      return "&self";
  }
  return "";
}

std::string_view ViewLifetime(AccessorCase accessor_case) {
  switch (accessor_case) {
    case AccessorCase::VIEW:
      return "'msg";
    case AccessorCase::OWNED:
    case AccessorCase::MUT:
      return "'_";
  }
  return "";
}

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
