// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_COMPILER_HPB_KEYWORDS_H__
#define GOOGLE_PROTOBUF_COMPILER_HPB_KEYWORDS_H__

#include <string>

#include "absl/strings/string_view.h"

namespace google {
namespace protobuf {
namespace hpb_generator {

// Resolves proto field name conflict with C++ reserved keywords.
std::string ResolveKeywordConflict(absl::string_view name);
}  // namespace hpb_generator
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_HPB_KEYWORDS_H__
