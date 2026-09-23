// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_CONFORMANCE_FAILURE_LIST_TRIE_NODE_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_FAILURE_LIST_TRIE_NODE_H__

#include <memory>
#include <string>
#include <vector>

#include "absl/status/status.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"

namespace google {
namespace protobuf {

// Each node represents a section of a test name, i.e. a piece delimited by '.'
// or '/' ("P0.FooTest.Bar/Proto3_INT32.ProtobufInput" has five).  One
// can imagine them as prefixes to search for a match. Once we hit a prefix that
// doesn't match, we can stop searching.  A node also remembers which of the
// two delimiters precedes its section, so "Bar/Proto3" and "Bar.Proto3" never
// match each other.
//
// Wildcards are supported within a section: '*' in an inserted name matches
// any run of characters that contains no delimiter, so "*" alone stands for a
// whole section, "*_INT32" for every section ending in "_INT32", and
// "Bar/*" for every parameter of test Bar.  A wildcard never spans a delimiter.
//
// This is not a general trie implementation
// as pointed out by its name. It is only meant to only be used for conformance
// failure lists.
//
// Example of what the trie might look like in practice:
//
//                             (root)
//                            /      |
//                         "P3"     "P0"
//                         /          |
//                  "FooTest"        "*"
//                  /      |           |
//             "Bar"    "Baz"        "Bar"
//              /                      |
//          "/Proto3"             "/*_INT32"
//
//
class FailureListTrieNode {
 public:
  FailureListTrieNode() = default;
  explicit FailureListTrieNode(absl::string_view data) : data_(data) {}

  // Will attempt to insert a test name into the trie returning
  // absl::StatusCode::kAlreadyExists if the test name (or a wildcard it
  // matches, or one that matches it) already exists; otherwise, insertion is
  // successful.
  absl::Status Insert(absl::string_view test_name);

  // Returns what it matched to (the name as inserted, wildcards included) if
  // it matched anything, otherwise returns absl::nullopt.
  absl::optional<std::string> WalkDownMatch(absl::string_view test_name) const;

 private:
  FailureListTrieNode(char separator, absl::string_view data)
      : separator_(separator), data_(data) {}

  // `rest` is what remains of the name below this node and `separator` the
  // delimiter that preceded it; `full_name` is the whole name being inserted,
  // stored where it ends.
  void InsertImpl(absl::string_view rest, char separator,
                  absl::string_view full_name);
  absl::optional<std::string> WalkDownMatchImpl(absl::string_view rest,
                                                char separator) const;

  // The delimiter that precedes this node's section in a name: '.', '/', or
  // '\0' for a first section (and the root).
  char separator_ = '\0';
  std::string data_;
  std::vector<std::unique_ptr<FailureListTrieNode>> children_;
  // The name that ends at this node, as inserted, if any.
  absl::optional<std::string> test_name_;
};
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_FAILURE_LIST_TRIE_NODE_H__
