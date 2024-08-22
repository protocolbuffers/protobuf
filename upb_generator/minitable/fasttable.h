#ifndef THIRD_PARTY_UPB_UPB_GENERATOR_MINITABLE_FASTTABLE_H_
#define THIRD_PARTY_UPB_UPB_GENERATOR_MINITABLE_FASTTABLE_H_

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "upb/reflection/def.hpp"
#include "upb_generator/file_layout.h"

namespace upb {
namespace generator {

typedef std::pair<std::string, uint64_t> TableEntry;

std::vector<TableEntry> FastDecodeTable(upb::MessageDefPtr message,
                                        const DefPoolPair& pools);

}  // namespace generator
}  // namespace upb

#endif  // THIRD_PARTY_UPB_UPB_GENERATOR_MINITABLE_FASTTABLE_H_
