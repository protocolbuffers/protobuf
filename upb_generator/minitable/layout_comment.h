// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_GENERATOR_MINITABLE_LAYOUT_COMMENT_H_
#define UPB_GENERATOR_MINITABLE_LAYOUT_COMMENT_H_

#include <string>

#include "upb/reflection/def.hpp"
#include "upb_generator/file_layout.h"

namespace upb {
namespace generator {

// Returns a C block comment (ending in a newline) that renders the 64-bit and
// 32-bit in-memory layouts of `message` as ASCII tables: the offset and size
// of every field, the hasbits, oneof case slots, and padding.
//
// upb never emits a C struct for a message; the layout is computed when the
// MiniTable is built. This comment is purely informational.
std::string MessageLayoutComment(const DefPoolPair& pools,
                                 upb::MessageDefPtr message);

}  // namespace generator
}  // namespace upb

#endif  // UPB_GENERATOR_MINITABLE_LAYOUT_COMMENT_H_
