// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance/recursion_limit_payloads.h"

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "absl/log/absl_check.h"
#include "absl/strings/str_cat.h"
#include "conformance/binary_wireformat.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

// One nesting level of such a payload, seen from outside: the bytes before the
// level below it (the level's own fields, then the tag of the field that holds
// the next level; its length prefix is added by Nested()) and the bytes after
// it (e.g. the end tag of a group).  The innermost level holds no next level.
struct Level {
  Wire before;
  Wire after;
};

// The serialization of `levels[0]`, which holds the serialization of
// `levels[1]` as a length-prefixed submessage, and so on down to
// `levels.back()`.  Two passes and no recursion: the sizes inside out, the
// bytes outside in.
Wire Nested(const std::vector<Level>& levels) {
  std::vector<size_t> sizes(levels.size());
  for (size_t i = levels.size(); i-- > 0;) {
    sizes[i] = levels[i].before.size() + levels[i].after.size();
    if (i + 1 < levels.size()) {
      sizes[i] += Varint(sizes[i + 1]).size() + sizes[i + 1];
    }
  }
  std::string bytes;
  bytes.reserve(sizes.empty() ? 0 : sizes[0]);
  for (size_t i = 0; i < levels.size(); ++i) {
    absl::StrAppend(&bytes, levels[i].before.data());
    if (i + 1 < levels.size()) {
      absl::StrAppend(&bytes, Varint(sizes[i + 1]).data());
    }
  }
  for (size_t i = levels.size(); i-- > 0;) {
    absl::StrAppend(&bytes, levels[i].after.data());
  }
  return Wire(std::move(bytes));
}

}  // namespace

Wire DeepMapPayload(int depth) {
  ABSL_DCHECK_GE(depth, 1);
  std::vector<Level> levels;
  levels.push_back({Tag(301, WireType::kLengthPrefixed)});
  for (int i = 0; i < depth; ++i) {
    // The map entry: C++ writes the key even when it is the default.
    levels.push_back(
        {Wire(VarintField(1, 0), Tag(2, WireType::kLengthPrefixed))});
    // The nested message.
    Wire fields = VarintField(1, 123);
    if (i + 1 < depth) {
      fields = Wire(fields, Tag(301, WireType::kLengthPrefixed));
    }
    levels.push_back({fields});
  }
  return Nested(levels);
}

Wire DeepMapStringKeyPayload(int depth) {
  ABSL_DCHECK_GE(depth, 1);
  std::vector<Level> levels;
  levels.push_back({Tag(71, WireType::kLengthPrefixed)});
  for (int i = 0; i < depth; ++i) {
    // The map entry: C++ writes the key even when it is empty.
    levels.push_back(
        {Wire(LengthPrefixedField(1, ""), Tag(2, WireType::kLengthPrefixed))});
    // The NestedMessage.
    levels.push_back({Tag(2, WireType::kLengthPrefixed)});
    // The nested TestAllTypesEdition2023.
    Wire fields = VarintField(1, 123);
    if (i + 1 < depth) {
      fields = Wire(fields, Tag(71, WireType::kLengthPrefixed));
    }
    levels.push_back({fields});
  }
  return Nested(levels);
}

// A message set item is a group: start tag (1), type_id (2), message (3),
// end tag.
Wire DeepMessageSetPayload(int depth) {
  ABSL_DCHECK_GE(depth, 0);
  std::vector<Level> levels;
  levels.push_back({Tag(500, WireType::kLengthPrefixed)});
  for (int i = 0; i <= depth; ++i) {
    // The MessageSetCorrect and its item.
    levels.push_back(
        {Wire(Tag(1, WireType::kStartGroup), VarintField(2, 4135312),
              Tag(3, WireType::kLengthPrefixed)),
         Tag(1, WireType::kEndGroup)});
    // The MessageSetCorrectExtension2.
    levels.push_back(
        {i < depth ? Tag(10, WireType::kLengthPrefixed) : VarintField(9, 123)});
  }
  return Nested(levels);
}

}  // namespace conformance
}  // namespace protobuf
}  // namespace google
