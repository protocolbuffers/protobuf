#include "google/protobuf/compiler/rust/accessors/accessor_case.h"

#include "absl/strings/string_view.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace rust {

absl::string_view ViewReceiver(AccessorCase accessor_case) {
  switch (accessor_case) {
    case AccessorCase::VIEW:
      return "self";
    case AccessorCase::OWNED:
    case AccessorCase::MUT:
      return "&self";
  }
  return "";
}

absl::string_view ViewLifetime(AccessorCase accessor_case) {
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
