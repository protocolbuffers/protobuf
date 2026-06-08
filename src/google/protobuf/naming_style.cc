// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/naming_style.h"

#include <cstdint>
#include <string>

#include "absl/status/status.h"
#include "absl/strings/ascii.h"
#include "absl/strings/charset.h"
#include "absl/strings/string_view.h"

namespace google {
namespace protobuf {
namespace internal {

bool ContainsBadUnderscores(absl::string_view name) {
  if (name.empty()) {
    return false;
  }
  if (name[0] == '_' || name[name.size() - 1] == '_') {
    return true;
  }
  for (size_t i = 1; i < name.size(); ++i) {
    if (name[i - 1] == '_' && !absl::ascii_isalpha(name[i])) {
      return true;
    }
  }
  return false;
}

absl::Status IsValidTitleCaseName(absl::string_view name) {
  if (name.empty()) {
    return absl::InvalidArgumentError("is empty");
  }

  for (char c : name) {
    if (!absl::ascii_isalnum(c)) {
      return absl::InvalidArgumentError("should be TitleCase");
    }
  }
  if (!absl::ascii_isupper(name[0])) {
    return absl::InvalidArgumentError("should begin with a capital letter");
  }
  return absl::OkStatus();
}

absl::Status IsValidLowerSnakeCaseName(absl::string_view name) {
  if (name.empty()) {
    return absl::InvalidArgumentError("is empty");
  }

  constexpr absl::CharSet kLowerSnakeCaseChars =
      absl::CharSet::Range('a', 'z') | absl::CharSet::Range('0', '9') |
      absl::CharSet::Char('_') | absl::CharSet::Char('.');
  for (char c : name) {
    if (!kLowerSnakeCaseChars.contains(c)) {
      return absl::InvalidArgumentError("should be lower_snake_case");
    }
  }
  if (!absl::ascii_islower(name[0])) {
    return absl::InvalidArgumentError("should begin with a lower case letter");
  }
  if (ContainsBadUnderscores(name)) {
    return absl::InvalidArgumentError("contains style violating underscores");
  }
  return absl::OkStatus();
}

absl::Status IsValidUpperSnakeCaseName(absl::string_view name) {
  if (name.empty()) {
    return absl::InvalidArgumentError("is empty");
  }

  constexpr absl::CharSet kUpperSnakeCaseChars =
      absl::CharSet::Range('A', 'Z') | absl::CharSet::Range('0', '9') |
      absl::CharSet::Char('_');
  for (char c : name) {
    if (!kUpperSnakeCaseChars.contains(c)) {
      return absl::InvalidArgumentError("should be UPPER_SNAKE_CASE");
    }
  }
  if (!absl::ascii_isupper(name[0])) {
    return absl::InvalidArgumentError("should begin with an upper case letter");
  }
  if (ContainsBadUnderscores(name)) {
    return absl::InvalidArgumentError("contains style violating underscores");
  }
  return absl::OkStatus();
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google
