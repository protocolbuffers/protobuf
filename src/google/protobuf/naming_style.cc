// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/naming_style.h"

#include <cstdint>
#include <string>

#include "absl/strings/ascii.h"
#include "absl/strings/charset.h"
#include "absl/strings/string_view.h"

namespace google {
namespace protobuf {

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

bool IsValidTitleCaseName(absl::string_view name, std::string* error) {
  if (name.empty()) {
    *error = "is empty";
    return false;
  }

  for (char c : name) {
    if (!absl::ascii_isalnum(c)) {
      *error = "should be TitleCase";
      return false;
    }
  }
  if (!absl::ascii_isupper(name[0])) {
    *error = "should begin with a capital letter";
    return false;
  }
  return true;
}

bool IsValidLowerSnakeCaseName(absl::string_view name, std::string* error) {
  if (name.empty()) {
    *error = "is empty";
    return false;
  }

  constexpr absl::CharSet kLowerSnakeCaseChars =
      absl::CharSet::Range('a', 'z') | absl::CharSet::Range('0', '9') |
      absl::CharSet::Char('_') | absl::CharSet::Char('.');
  for (char c : name) {
    if (!kLowerSnakeCaseChars.contains(c)) {
      *error = "should be lower_snake_case";
      return false;
    }
  }
  if (!absl::ascii_islower(name[0])) {
    *error = "should begin with a lower case letter";
    return false;
  }
  if (ContainsBadUnderscores(name)) {
    *error = "contains style violating underscores";
    return false;
  }
  return true;
}

bool IsValidUpperSnakeCaseName(absl::string_view name, std::string* error) {
  if (name.empty()) {
    *error = "is empty";
    return false;
  }

  constexpr absl::CharSet kUpperSnakeCaseChars =
      absl::CharSet::Range('A', 'Z') | absl::CharSet::Range('0', '9') |
      absl::CharSet::Char('_');
  for (char c : name) {
    if (!kUpperSnakeCaseChars.contains(c)) {
      *error = "should be UPPER_SNAKE_CASE";
      return false;
    }
  }
  if (!absl::ascii_isupper(name[0])) {
    *error = "should begin with an upper case letter";
    return false;
  }
  if (ContainsBadUnderscores(name)) {
    *error = "contains style violating underscores";
    return false;
  }
  return true;
}

}  // namespace protobuf
}  // namespace google
