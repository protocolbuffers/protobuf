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

// Each node represents a section of a test name (divided by '.'). One can
// imagine them as prefixes to search for a match. Once we hit a prefix that
// doesn't match, we can stop searching. Wildcards matching to any set of
// characters are supported.
//
// This is not a general trie implementation
// as pointed out by its name. It is only meant to only be used for conformance
// failure lists.
//
// Example of what the trie might look like in practice:
//
//                             (root)
//                            /      |
//                "Recommended"   "Required"
//                      /               |
//                 "Proto2"             "*"
//                 /      |               |
//          "JsonInput" "ProtobufInput"  "JsonInput"
//
//
class FailureListTrieNode {
 public:
  FailureListTrieNode() : data_("") {}
  explicit FailureListTrieNode(absl::string_view data) : data_(data) {}

  // Will attempt to insert a test name into the trie returning
  // absl::StatusCode::kAlreadyExists if the test name already exists or
  // absl::StatusCode::kInvalidArgument if the test name contains invalid
  // wildcards; otherwise, insertion is successful.
  absl::Status Insert(absl::string_view test_name);

  // Returns what it matched to if it matched anything, otherwise returns
  // absl::nullopt
  absl::optional<std::string> WalkDownMatch(absl::string_view test_name);

 private:
  absl::string_view data_;
  std::vector<std::unique_ptr<FailureListTrieNode>> children_;
  void InsertImpl(absl::string_view test_name);
};
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_FAILURE_LIST_TRIE_NODE_H__
