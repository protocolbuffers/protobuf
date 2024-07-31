#ifndef THIRD_PARTY_UPB_UPB_GENERATOR_MANGLE_H_
#define THIRD_PARTY_UPB_UPB_GENERATOR_MANGLE_H_

#include <string>

#include "absl/strings/string_view.h"

// Must be last.
#include "google/protobuf/port_def.inc"

namespace upb {
namespace generator {

PROTOC_EXPORT std::string MessageInit(absl::string_view full_name);

}  // namespace generator
}  // namespace upb

#include "google/protobuf/port_undef.inc"

#endif  // THIRD_PARTY_UPB_UPB_GENERATOR_MANGLE_H_
