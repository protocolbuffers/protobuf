#ifndef THIRD_PARTY_UPB_UPB_GENERATOR_REFLECTION_HEADER_H_
#define THIRD_PARTY_UPB_UPB_GENERATOR_REFLECTION_HEADER_H_

#include <string>

#include "upb/reflection/def.hpp"
#include "upb_generator/plugin.h"
#include "upb_generator/reflection/context.h"

namespace upb {
namespace generator {
namespace reflection {

std::string DefHeaderFilename(upb::FileDefPtr file);

void GenerateReflectionHeader(upb::FileDefPtr file, const Options& options,
                              Plugin* plugin);

}  // namespace reflection
}  // namespace generator
}  // namespace upb

#endif  // THIRD_PARTY_UPB_UPB_GENERATOR_REFLECTION_HEADER_H_
