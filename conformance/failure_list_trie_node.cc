// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance/failure_list_trie_node.h"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "absl/status/status.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"

namespace google {
namespace protobuf {
namespace {

constexpr absl::string_view kDelimiters = "./";

// The first section of `name`, the delimiter that follows it ('\0' if it is
// the last section) and what comes after that delimiter.
struct Split {
  absl::string_view section;
  char delimiter;
  absl::string_view rest;
};

Split SplitFirstSection(absl::string_view name) {
  size_t pos = name.find_first_of(kDelimiters);
  if (pos == absl::string_view::npos) return {name, '\0', ""};
  return {name.substr(0, pos), name[pos], name.substr(pos + 1)};
}

// Whether some string matches both `a` and `b`, in each of which '*' matches
// any run of characters.  When neither contains a wildcard this is equality;
// when one does, whether it matches the other; when both do, whether the two
// wildcards can agree on a string (so "*_INT32" and "Proto2_*" match, since
// "Proto2_INT32" satisfies both, while "*_INT32" and "*_INT64" don't).
bool SectionsMatch(absl::string_view a, absl::string_view b) {
  if (a.find('*') == absl::string_view::npos &&
      b.find('*') == absl::string_view::npos) {
    return a == b;
  }
  // matches[i][j]: whether a.substr(i) and b.substr(j) have a common string.
  // Filled from the ends: a '*' either matches nothing (skip it) or absorbs
  // the other side's next character (advance the other side).
  std::vector<std::vector<bool>> matches(
      a.size() + 1, std::vector<bool>(b.size() + 1, false));
  matches[a.size()][b.size()] = true;
  for (size_t i = a.size() + 1; i-- > 0;) {
    for (size_t j = b.size() + 1; j-- > 0;) {
      if (i == a.size() && j == b.size()) continue;
      bool a_star = i < a.size() && a[i] == '*';
      bool b_star = j < b.size() && b[j] == '*';
      bool result = false;
      if (a_star) {
        result = matches[i + 1][j] || (j < b.size() && matches[i][j + 1]);
      }
      if (!result && b_star) {
        result = matches[i][j + 1] || (i < a.size() && matches[i + 1][j]);
      }
      if (!result && !a_star && !b_star && i < a.size() && j < b.size() &&
          a[i] == b[j]) {
        result = matches[i + 1][j + 1];
      }
      matches[i][j] = result;
    }
  }
  return matches[0][0];
}

}  // namespace

absl::Status FailureListTrieNode::Insert(absl::string_view test_name) {
  auto result = WalkDownMatch(test_name);
  if (result.has_value()) {
    return absl::AlreadyExistsError(
        absl::StrFormat("Test name  %s  already exists in the trie  FROM  %s",
                        test_name, result.value()));
  }
  InsertImpl(test_name, '\0', test_name);
  return absl::OkStatus();
}

void FailureListTrieNode::InsertImpl(absl::string_view rest, char separator,
                                     absl::string_view full_name) {
  Split split = SplitFirstSection(rest);
  FailureListTrieNode* child = nullptr;
  for (auto& candidate : children_) {
    if (candidate->separator_ == separator &&
        candidate->data_ == split.section) {
      child = candidate.get();
      break;
    }
  }
  if (child == nullptr) {
    children_.push_back(std::unique_ptr<FailureListTrieNode>(
        new FailureListTrieNode(separator, split.section)));
    child = children_.back().get();
  }
  if (split.delimiter == '\0') {
    child->test_name_ = std::string(full_name);
  } else {
    child->InsertImpl(split.rest, split.delimiter, full_name);
  }
}

absl::optional<std::string> FailureListTrieNode::WalkDownMatch(
    absl::string_view test_name) const {
  return WalkDownMatchImpl(test_name, '\0');
}

absl::optional<std::string> FailureListTrieNode::WalkDownMatchImpl(
    absl::string_view rest, char separator) const {
  Split split = SplitFirstSection(rest);
  for (const auto& child : children_) {
    if (child->separator_ != separator ||
        !SectionsMatch(child->data_, split.section)) {
      continue;
    }
    if (split.delimiter == '\0') {
      if (child->test_name_.has_value()) return child->test_name_;
    } else {
      auto result = child->WalkDownMatchImpl(split.rest, split.delimiter);
      if (result.has_value()) return result;
    }
  }
  // No match
  return absl::nullopt;
}
}  // namespace protobuf
}  // namespace google
