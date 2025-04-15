// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_UPB_UPB_GENERATOR_MINITABLE_FASTTABLE_H__
#define GOOGLE_UPB_UPB_GENERATOR_MINITABLE_FASTTABLE_H__

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

#endif  // GOOGLE_UPB_UPB_GENERATOR_MINITABLE_FASTTABLE_H__
