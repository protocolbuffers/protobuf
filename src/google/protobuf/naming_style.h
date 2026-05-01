// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_NAMING_STYLE_H__
#define GOOGLE_PROTOBUF_NAMING_STYLE_H__

#include <string>

#include "absl/strings/string_view.h"

namespace google {
namespace protobuf {

bool ContainsBadUnderscores(absl::string_view name);
bool IsValidTitleCaseName(absl::string_view name, std::string* error);
bool IsValidLowerSnakeCaseName(absl::string_view name, std::string* error);
bool IsValidUpperSnakeCaseName(absl::string_view name, std::string* error);

}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_NAMING_STYLE_H__
