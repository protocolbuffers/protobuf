// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/naming_style.h"

#include <cstddef>
#include <cstdint>
#include <string>

#include "absl/algorithm/container.h"
#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/strings/ascii.h"
#include "absl/strings/charset.h"
#include "absl/strings/string_view.h"

namespace google {
namespace protobuf {
namespace internal {
size_t CamelCaseSize(const absl::string_view input) {
  return input.size() - absl::c_count(input, '_');
}

size_t JsonNameSize(const absl::string_view input) {
  return input.size() - absl::c_count(input, '_');
}

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

std::string ToCamelCase(const absl::string_view input, bool lower_first) {
  bool capitalize_next = !lower_first;
  std::string result;
  result.reserve(input.size());

  for (char character : input) {
    if (character == '_') {
      capitalize_next = true;
    } else if (capitalize_next) {
      result.push_back(absl::ascii_toupper(character));
      capitalize_next = false;
    } else {
      result.push_back(character);
    }
  }

  // Lower-case the first letter.
  if (lower_first && !result.empty()) {
    result[0] = absl::ascii_tolower(result[0]);
  }

  ABSL_DCHECK_EQ(CamelCaseSize(input), result.size());

  return result;
}

std::string ToJsonName(const absl::string_view input) {
  bool capitalize_next = false;
  std::string result;
  result.reserve(input.size());

  for (char character : input) {
    if (character == '_') {
      capitalize_next = true;
    } else if (capitalize_next) {
      result.push_back(absl::ascii_toupper(character));
      capitalize_next = false;
    } else {
      result.push_back(character);
    }
  }

  ABSL_DCHECK_EQ(JsonNameSize(input), result.size());

  return result;
}

std::string EnumValueToPascalCase(const absl::string_view input) {
  bool next_upper = true;
  std::string result;
  result.reserve(input.size());

  for (char character : input) {
    if (character == '_') {
      next_upper = true;
    } else {
      if (next_upper) {
        result.push_back(absl::ascii_toupper(character));
      } else {
        result.push_back(absl::ascii_tolower(character));
      }
      next_upper = false;
    }
  }

  return result;
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google
