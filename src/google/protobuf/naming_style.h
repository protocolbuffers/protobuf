// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_NAMING_STYLE_H__
#define GOOGLE_PROTOBUF_NAMING_STYLE_H__

#include <string>

#include "absl/status/status.h"
#include "absl/strings/string_view.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

PROTOBUF_EXPORT bool ContainsBadUnderscores(absl::string_view name);

// Non-OK Statuses contain a simple message describing the error that does not
// contain the failing element's name. The caller is expected to assemble a
// fuller message if needed.
PROTOBUF_EXPORT absl::Status IsValidTitleCaseName(absl::string_view name);
PROTOBUF_EXPORT absl::Status IsValidLowerSnakeCaseName(absl::string_view name);
PROTOBUF_EXPORT absl::Status IsValidUpperSnakeCaseName(absl::string_view name);

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_NAMING_STYLE_H__
