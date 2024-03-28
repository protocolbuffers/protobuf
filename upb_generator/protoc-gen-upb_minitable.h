// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <string.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "upb_generator/common.h"
#include "upb_generator/file_layout.h"
#include "upb_generator/plugin.h"

namespace upb {
namespace generator {

void WriteMiniTableSource(const DefPoolPair& pools, upb::FileDefPtr file,
                          Output& output);
void WriteMiniTableHeader(const DefPoolPair& pools, upb::FileDefPtr file,
                          Output& output);

}  // namespace generator
}  // namespace upb
