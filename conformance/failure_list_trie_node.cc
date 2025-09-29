// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "failure_list_trie_node.h"

#include <memory>
#include <optional>
#include <string>

#include "absl/status/status.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"

namespace google {
namespace protobuf {

absl::Status FailureListTrieNode::Insert(absl::string_view test_name) {
  auto result = WalkDownMatch(test_name);
  if (result.has_value()) {
    return absl::AlreadyExistsError(
        absl::StrFormat("Test name  %s  already exists in the trie  FROM  %s",
                        test_name, result.value()));
  }

  auto sections = absl::StrSplit(test_name, '.');
  for (auto section : sections) {
    if (absl::StrContains(section, '*') && section.length() > 1) {
      return absl::InvalidArgumentError(absl::StrFormat(
          "Test name %s contains invalid wildcard(s) (wildcards "
          "must span the whole of a section)",
          test_name));
    }
  }
  InsertImpl(test_name);
  return absl::OkStatus();
}

void FailureListTrieNode::InsertImpl(absl::string_view test_name) {
  absl::string_view section = test_name.substr(0, test_name.find('.'));

  bool is_last_section = test_name == section;

  // test_name cannot be overwritten
  absl::string_view test_name_rest =
      is_last_section ? "" : test_name.substr(section.length() + 1);
  for (auto& child : children_) {
    if (child->data_ == section) {
      if (is_last_section) {
        // Extracted last section -> no more '.' -> test_name will be equal to
        // section
        child->is_test_name_ = true;
      } else {
        child->InsertImpl(test_name_rest);
      }
      return;
    }
  }

  // No match
  children_.push_back(std::make_unique<FailureListTrieNode>(section));
  if (is_last_section) {
    children_.back()->is_test_name_ = true;
    return;
  }
  children_.back()->InsertImpl(test_name_rest);
}

std::optional<std::string> FailureListTrieNode::WalkDownMatch(
    absl::string_view test_name) {
  absl::string_view section = test_name.substr(0, test_name.find('.'));
  // test_name cannot be overwritten
  absl::string_view to_match;
  if (section != test_name) {
    to_match = test_name.substr(section.length() + 1);
  }

  for (auto& child : children_) {
    if (child->data_ == section || child->data_ == "*" || section == "*") {
      absl::string_view appended = child->data_;
      // Extracted last section -> no more '.' -> test_name will be
      // equal to section
      if (test_name == section) {
        if (child->is_test_name_) {
          return std::string(appended);
        }
      } else {
        auto result = child->WalkDownMatch(to_match);
        if (result.has_value()) {
          return absl::StrCat(appended, ".", result.value());
        }
      }
    }
  }
  // No match
  return std::nullopt;
}
}  // namespace protobuf
}  // namespace google
