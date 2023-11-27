#include "upb_generator/mangle.h"

#include <string>

#include "absl/strings/str_replace.h"
#include "absl/strings/string_view.h"

// Generate a mangled C name for a proto object.
static std::string MangleName(absl::string_view name) {
  return absl::StrReplaceAll(name, {{"_", "_0"}, {".", "__"}});
}

namespace upb {
namespace generator {

std::string MessageInit(absl::string_view full_name) {
  return MangleName(full_name) + "_msg_init";
}

}  // namespace generator
}  // namespace upb
