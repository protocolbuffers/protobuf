#ifndef THIRD_PARTY_UPB_UPB_GENERATOR_MANGLE_H_
#define THIRD_PARTY_UPB_UPB_GENERATOR_MANGLE_H_

#include <string>

#include "absl/strings/string_view.h"

namespace upb {
namespace generator {

std::string MessageInit(absl::string_view full_name);

}  // namespace generator
}  // namespace upb

#endif  // THIRD_PARTY_UPB_UPB_GENERATOR_MANGLE_H_
