// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "google/protobuf/util/field_mask_util.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "absl/container/btree_map.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/log/die_if_null.h"
#include "absl/memory/memory.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"
#include "google/protobuf/message.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace util {

using google::protobuf::FieldMask;

std::string FieldMaskUtil::ToString(const FieldMask& mask) {
  return absl::StrJoin(mask.paths(), ",");
}

void FieldMaskUtil::FromString(absl::string_view str, FieldMask* out) {
  out->Clear();
  std::vector<absl::string_view> paths = absl::StrSplit(str, ',');
  for (absl::string_view path : paths) {
    if (path.empty()) continue;
    out->add_paths(path);
  }
}

bool FieldMaskUtil::SnakeCaseToCamelCase(absl::string_view input,
                                         std::string* output) {
  output->clear();
  bool after_underscore = false;
  for (char input_char : input) {
    if (input_char >= 'A' && input_char <= 'Z') {
      // The field name must not contain uppercase letters.
      return false;
    }
    if (after_underscore) {
      if (input_char >= 'a' && input_char <= 'z') {
        output->push_back(input_char + 'A' - 'a');
        after_underscore = false;
      } else {
        // The character after a "_" must be a lowercase letter.
        return false;
      }
    } else if (input_char == '_') {
      after_underscore = true;
    } else {
      output->push_back(input_char);
    }
  }
  if (after_underscore) {
    // Trailing "_".
    return false;
  }
  return true;
}

bool FieldMaskUtil::CamelCaseToSnakeCase(absl::string_view input,
                                         std::string* output) {
  output->clear();
  for (const char c : input) {
    if (c == '_') {
      // The field name must not contain "_"s.
      return false;
    }
    if (c >= 'A' && c <= 'Z') {
      output->push_back('_');
      output->push_back(c + 'a' - 'A');
    } else {
      output->push_back(c);
    }
  }
  return true;
}

bool FieldMaskUtil::ToJsonString(const FieldMask& mask, std::string* out) {
  out->clear();
  for (int i = 0; i < mask.paths_size(); ++i) {
    absl::string_view path = mask.paths(i);
    std::string camelcase_path;
    if (!SnakeCaseToCamelCase(path, &camelcase_path)) {
      return false;
    }
    if (i > 0) {
      out->push_back(',');
    }
    out->append(camelcase_path);
  }
  return true;
}

bool FieldMaskUtil::FromJsonString(absl::string_view str, FieldMask* out) {
  out->Clear();
  std::vector<absl::string_view> paths = absl::StrSplit(str, ',');
  for (absl::string_view path : paths) {
    if (path.empty()) continue;
    std::string snakecase_path;
    if (!CamelCaseToSnakeCase(path, &snakecase_path)) {
      return false;
    }
    out->add_paths(snakecase_path);
  }
  return true;
}

bool FieldMaskUtil::GetFieldDescriptors(
    const Descriptor* descriptor, absl::string_view path,
    std::vector<const FieldDescriptor*>* field_descriptors) {
  if (field_descriptors != nullptr) {
    field_descriptors->clear();
  }
  std::vector<absl::string_view> parts = absl::StrSplit(path, '.');
  for (absl::string_view field_name : parts) {
    if (descriptor == nullptr) {
      return false;
    }
    const FieldDescriptor* field = descriptor->FindFieldByName(field_name);
    if (field == nullptr) {
      return false;
    }
    if (field_descriptors != nullptr) {
      field_descriptors->push_back(field);
    }
    if (!field->is_repeated() &&
        field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      descriptor = field->message_type();
    } else {
      descriptor = nullptr;
    }
  }
  return true;
}

void FieldMaskUtil::GetFieldMaskForAllFields(const Descriptor* descriptor,
                                             FieldMask* out) {
  for (int i = 0; i < descriptor->field_count(); ++i) {
    out->add_paths(descriptor->field(i)->name());
  }
}

namespace {
// A FieldMaskTree represents a FieldMask in a tree structure. For example,
// given a FieldMask "foo.bar,foo.baz,bar.baz", the FieldMaskTree will be:
//
//   [root] -+- foo -+- bar
//           |       |
//           |       +- baz
//           |
//           +- bar --- baz
//
// In the tree, each leaf node represents a field path.
class FieldMaskTree {
 public:
  FieldMaskTree();
  FieldMaskTree(const FieldMaskTree&) = delete;
  FieldMaskTree& operator=(const FieldMaskTree&) = delete;
  ~FieldMaskTree();

  void MergeFromFieldMask(const FieldMask& mask);
  void MergeToFieldMask(FieldMask* mask);

  // Add a field path into the tree. In a FieldMask, each field path matches
  // the specified field and also all its sub-fields. If the field path to
  // add is a sub-path of an existing field path in the tree (i.e., a leaf
  // node), it means the tree already matches the given path so nothing will
  // be added to the tree. If the path matches an existing non-leaf node in the
  // tree, that non-leaf node will be turned into a leaf node with all its
  // children removed because the path matches all the node's children.
  void AddPath(absl::string_view path);

  // Remove a path from the tree.
  // If the path is a sub-path of an existing field path in the tree, it means
  // we need remove the existing field path and add all sub-paths except
  // specified path. If the path matches an existing node in the tree, this node
  // will be moved.
  void RemovePath(absl::string_view path, const Descriptor* descriptor);

  // Calculate the intersection part of a field path with this tree and add
  // the intersection field path into out.
  void IntersectPath(absl::string_view path, FieldMaskTree* out);

  // Merge all fields specified by this tree from one message to another.
  void MergeMessage(const Message& source,
                    const FieldMaskUtil::MergeOptions& options,
                    Message* destination) {
    // Do nothing if the tree is empty.
    if (root_.children.empty()) {
      return;
    }
    MergeMessage(&root_, source, options, destination);
  }

  // Add required field path of the message to this tree based on current tree
  // structure. If a message is present in the tree, add the path of its
  // required field to the tree. This is to make sure that after trimming a
  // message with required fields are set, check IsInitialized() will not fail.
  void AddRequiredFieldPath(const Descriptor* descriptor) {
    // Do nothing if the tree is empty.
    if (root_.children.empty()) {
      return;
    }
    AddRequiredFieldPath(&root_, descriptor);
  }

  // Trims all fields not specified by this tree from the given message.
  // Returns true if the message is modified.
  bool TrimMessage(Message* message) {
    // Do nothing if the tree is empty.
    if (root_.children.empty()) {
      return false;
    }
    return TrimMessage(&root_, message);
  }

 private:
  struct Node {
    Node() = default;
    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;

    ~Node() { ClearChildren(); }

    void ClearChildren() {
      children.clear();
    }

    absl::btree_map<std::string, std::unique_ptr<Node>> children;
  };

  // Merge a sub-tree to mask. This method adds the field paths represented
  // by all leaf nodes descended from "node" to mask.
  void MergeToFieldMask(absl::string_view prefix, const Node* node,
                        FieldMask* out);

  // Merge all leaf nodes of a sub-tree to another tree.
  void MergeLeafNodesToTree(absl::string_view prefix, const Node* node,
                            FieldMaskTree* out);

  // Merge all fields specified by a sub-tree from one message to another.
  void MergeMessage(const Node* node, const Message& source,
                    const FieldMaskUtil::MergeOptions& options,
                    Message* destination);

  // Add required field path of the message to this tree based on current tree
  // structure. If a message is present in the tree, add the path of its
  // required field to the tree. This is to make sure that after trimming a
  // message with required fields are set, check IsInitialized() will not fail.
  void AddRequiredFieldPath(Node* node, const Descriptor* descriptor);

  // Trims all fields not specified by this sub-tree from the given message.
  // Returns true if the message is actually modified
  bool TrimMessage(const Node* node, Message* message);

  Node root_;
};

FieldMaskTree::FieldMaskTree() {}

FieldMaskTree::~FieldMaskTree() {}

void FieldMaskTree::MergeFromFieldMask(const FieldMask& mask) {
  for (int i = 0; i < mask.paths_size(); ++i) {
    AddPath(mask.paths(i));
  }
}

void FieldMaskTree::MergeToFieldMask(FieldMask* mask) {
  MergeToFieldMask("", &root_, mask);
}

void FieldMaskTree::MergeToFieldMask(absl::string_view prefix, const Node* node,
                                     FieldMask* out) {
  if (node->children.empty()) {
    if (prefix.empty()) {
      // This is the root node.
      return;
    }
    out->add_paths(prefix);
    return;
  }
  for (const auto& kv : node->children) {
    std::string current_path =
        prefix.empty() ? kv.first : absl::StrCat(prefix, ".", kv.first);
    MergeToFieldMask(current_path, kv.second.get(), out);
  }
}

void FieldMaskTree::AddPath(absl::string_view path) {
  std::vector<absl::string_view> parts = absl::StrSplit(path, '.');
  if (parts.empty()) {
    return;
  }
  bool new_branch = false;
  Node* node = &root_;
  for (absl::string_view node_name : parts) {
    if (!new_branch && node != &root_ && node->children.empty()) {
      // Path matches an existing leaf node. This means the path is already
      // covered by this tree (for example, adding "foo.bar.baz" to a tree
      // which already contains "foo.bar").
      return;
    }
    std::unique_ptr<Node>& child = node->children[node_name];
    if (child == nullptr) {
      new_branch = true;
      child = absl::make_unique<Node>();
    }
    node = child.get();
  }
  if (!node->children.empty()) {
    node->ClearChildren();
  }
}

void FieldMaskTree::RemovePath(absl::string_view path,
                               const Descriptor* descriptor) {
  if (root_.children.empty()) {
    // Nothing to be removed from an empty tree. We shortcut it here so an empty
    // tree won't be interpreted as a field mask containing all fields by the
    // code below.
    return;
  }
  std::vector<absl::string_view> parts = absl::StrSplit(path, '.');
  if (parts.empty()) {
    return;
  }
  std::vector<Node*> nodes(parts.size());
  Node* node = &root_;
  const Descriptor* current_descriptor = descriptor;
  Node* new_branch_node = nullptr;
  for (int i = 0; i < parts.size(); ++i) {
    nodes[i] = node;
    const FieldDescriptor* field_descriptor =
        current_descriptor->FindFieldByName(parts[i]);
    if (field_descriptor == nullptr ||
        (field_descriptor->cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE &&
         i != parts.size() - 1)) {
      // Invalid path.
      if (new_branch_node != nullptr) {
        // If add any new nodes, cleanup.
        new_branch_node->ClearChildren();
      }
      return;
    }

    if (node->children.empty()) {
      if (new_branch_node == nullptr) {
        new_branch_node = node;
      }
      for (int j = 0; j < current_descriptor->field_count(); ++j) {
        node->children[current_descriptor->field(j)->name()] =
            absl::make_unique<Node>();
      }
    }
    auto it = node->children.find(parts[i]);
    if (it == node->children.end()) return;
    node = it->second.get();
    if (field_descriptor->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      current_descriptor = field_descriptor->message_type();
    }
  }
  // Remove path.
  for (int i = parts.size() - 1; i >= 0; i--) {
    nodes[i]->children.erase(parts[i]);
    if (!nodes[i]->children.empty()) {
      break;
    }
  }
}

void FieldMaskTree::IntersectPath(absl::string_view path, FieldMaskTree* out) {
  std::vector<absl::string_view> parts = absl::StrSplit(path, '.');
  if (parts.empty()) {
    return;
  }
  const Node* node = &root_;
  for (absl::string_view node_name : parts) {
    if (node->children.empty()) {
      if (node != &root_) {
        out->AddPath(path);
      }
      return;
    }
    auto it = node->children.find(node_name);
    if (it == node->children.end()) {
      // No intersection found.
      return;
    }
    node = it->second.get();
  }
  // Now we found a matching node with the given path. Add all leaf nodes
  // to out.
  MergeLeafNodesToTree(path, node, out);
}

void FieldMaskTree::MergeLeafNodesToTree(absl::string_view prefix,
                                         const Node* node, FieldMaskTree* out) {
  if (node->children.empty()) {
    out->AddPath(prefix);
  }
  for (const auto& kv : node->children) {
    std::string current_path =
        prefix.empty() ? kv.first : absl::StrCat(prefix, ".", kv.first);
    MergeLeafNodesToTree(current_path, kv.second.get(), out);
  }
}

void FieldMaskTree::MergeMessage(const Node* node, const Message& source,
                                 const FieldMaskUtil::MergeOptions& options,
                                 Message* destination) {
  ABSL_DCHECK(!node->children.empty());
  const Reflection* source_reflection = source.GetReflection();
  const Reflection* destination_reflection = destination->GetReflection();
  const Descriptor* descriptor = source.GetDescriptor();
  for (const auto& kv : node->children) {
    absl::string_view field_name = kv.first;
    const Node* child = kv.second.get();
    const FieldDescriptor* field = descriptor->FindFieldByName(field_name);
    if (field == nullptr) {
      ABSL_LOG(ERROR) << "Cannot find field \"" << field_name
                      << "\" in message " << descriptor->full_name();
      continue;
    }
    if (!child->children.empty()) {
      // Sub-paths are only allowed for singular message fields.
      if (field->is_repeated() ||
          field->cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE) {
        ABSL_LOG(ERROR) << "Field \"" << field_name << "\" in message "
                        << descriptor->full_name()
                        << " is not a singular message field and cannot "
                        << "have sub-fields.";
        continue;
      }
      MergeMessage(child, source_reflection->GetMessage(source, field), options,
                   destination_reflection->MutableMessage(destination, field));
      continue;
    }
    if (!field->is_repeated()) {
      switch (field->cpp_type()) {
#define COPY_VALUE(TYPE, Name)                                              \
  case FieldDescriptor::CPPTYPE_##TYPE: {                                   \
    if (source_reflection->HasField(source, field)) {                       \
      destination_reflection->Set##Name(                                    \
          destination, field, source_reflection->Get##Name(source, field)); \
    } else {                                                                \
      destination_reflection->ClearField(destination, field);               \
    }                                                                       \
    break;                                                                  \
  }
        COPY_VALUE(BOOL, Bool)
        COPY_VALUE(INT32, Int32)
        COPY_VALUE(INT64, Int64)
        COPY_VALUE(UINT32, UInt32)
        COPY_VALUE(UINT64, UInt64)
        COPY_VALUE(FLOAT, Float)
        COPY_VALUE(DOUBLE, Double)
        COPY_VALUE(ENUM, Enum)
        COPY_VALUE(STRING, String)
#undef COPY_VALUE
        case FieldDescriptor::CPPTYPE_MESSAGE: {
          if (options.replace_message_fields()) {
            destination_reflection->ClearField(destination, field);
          }
          if (source_reflection->HasField(source, field)) {
            destination_reflection->MutableMessage(destination, field)
                ->MergeFrom(source_reflection->GetMessage(source, field));
          }
          break;
        }
      }
    } else {
      if (options.replace_repeated_fields()) {
        destination_reflection->ClearField(destination, field);
      }
      switch (field->cpp_type()) {
#define COPY_REPEATED_VALUE(TYPE, Name)                            \
  case FieldDescriptor::CPPTYPE_##TYPE: {                          \
    int size = source_reflection->FieldSize(source, field);        \
    for (int i = 0; i < size; ++i) {                               \
      destination_reflection->Add##Name(                           \
          destination, field,                                      \
          source_reflection->GetRepeated##Name(source, field, i)); \
    }                                                              \
    break;                                                         \
  }
        COPY_REPEATED_VALUE(BOOL, Bool)
        COPY_REPEATED_VALUE(INT32, Int32)
        COPY_REPEATED_VALUE(INT64, Int64)
        COPY_REPEATED_VALUE(UINT32, UInt32)
        COPY_REPEATED_VALUE(UINT64, UInt64)
        COPY_REPEATED_VALUE(FLOAT, Float)
        COPY_REPEATED_VALUE(DOUBLE, Double)
        COPY_REPEATED_VALUE(ENUM, Enum)
        COPY_REPEATED_VALUE(STRING, String)
#undef COPY_REPEATED_VALUE
        case FieldDescriptor::CPPTYPE_MESSAGE: {
          int size = source_reflection->FieldSize(source, field);
          for (int i = 0; i < size; ++i) {
            destination_reflection->AddMessage(destination, field)
                ->MergeFrom(
                    source_reflection->GetRepeatedMessage(source, field, i));
          }
          break;
        }
      }
    }
  }
}

void FieldMaskTree::AddRequiredFieldPath(Node* node,
                                         const Descriptor* descriptor) {
  const int32_t field_count = descriptor->field_count();
  for (int index = 0; index < field_count; ++index) {
    const FieldDescriptor* field = descriptor->field(index);
    if (field->is_required()) {
      absl::string_view node_name = field->name();
      std::unique_ptr<Node>& child = node->children[node_name];
      if (child == nullptr) {
        // Add required field path to the tree
        child = absl::make_unique<Node>();
      } else if (child->children.empty()) {
        // If the required field is in the tree and does not have any children,
        // do nothing.
        continue;
      }
      // Add required field in the children to the tree if the field is message.
      if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
        AddRequiredFieldPath(child.get(), field->message_type());
      }
    } else if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      auto it = node->children.find(field->name());
      if (it != node->children.end()) {
        // Add required fields in the children to the
        // tree if the field is a message and present in the tree.
        Node* child = it->second.get();
        if (!child->children.empty()) {
          AddRequiredFieldPath(child, field->message_type());
        }
      }
    }
  }
}

bool FieldMaskTree::TrimMessage(const Node* node, Message* message) {
  ABSL_DCHECK(!node->children.empty());
  const Reflection* reflection = message->GetReflection();
  const Descriptor* descriptor = message->GetDescriptor();
  const int32_t field_count = descriptor->field_count();
  bool modified = false;
  for (int index = 0; index < field_count; ++index) {
    const FieldDescriptor* field = descriptor->field(index);
    auto it = node->children.find(field->name());
    if (it == node->children.end()) {
      if (field->is_repeated()) {
        if (reflection->FieldSize(*message, field) != 0) {
          modified = true;
        }
      } else {
        if (reflection->HasField(*message, field)) {
          modified = true;
        }
      }
      reflection->ClearField(message, field);
    } else {
      if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
        Node* child = it->second.get();
        if (!child->children.empty() && reflection->HasField(*message, field)) {
          bool nestedMessageChanged =
              TrimMessage(child, reflection->MutableMessage(message, field));
          modified = nestedMessageChanged || modified;
        }
      }
    }
  }
  return modified;
}

}  // namespace

void FieldMaskUtil::ToCanonicalForm(const FieldMask& mask, FieldMask* out) {
  FieldMaskTree tree;
  tree.MergeFromFieldMask(mask);
  out->Clear();
  tree.MergeToFieldMask(out);
}

void FieldMaskUtil::Union(const FieldMask& mask1, const FieldMask& mask2,
                          FieldMask* out) {
  FieldMaskTree tree;
  tree.MergeFromFieldMask(mask1);
  tree.MergeFromFieldMask(mask2);
  out->Clear();
  tree.MergeToFieldMask(out);
}

void FieldMaskUtil::Intersect(const FieldMask& mask1, const FieldMask& mask2,
                              FieldMask* out) {
  FieldMaskTree tree, intersection;
  tree.MergeFromFieldMask(mask1);
  for (int i = 0; i < mask2.paths_size(); ++i) {
    tree.IntersectPath(mask2.paths(i), &intersection);
  }
  out->Clear();
  intersection.MergeToFieldMask(out);
}

void FieldMaskUtil::Subtract(const Descriptor* descriptor,
                             const FieldMask& mask1, const FieldMask& mask2,
                             FieldMask* out) {
  if (mask1.paths().empty()) {
    out->Clear();
    return;
  }
  FieldMaskTree tree;
  tree.MergeFromFieldMask(mask1);
  for (int i = 0; i < mask2.paths_size(); ++i) {
    tree.RemovePath(mask2.paths(i), descriptor);
  }
  out->Clear();
  tree.MergeToFieldMask(out);
}

bool FieldMaskUtil::IsPathInFieldMask(absl::string_view path,
                                      const FieldMask& mask) {
  for (int i = 0; i < mask.paths_size(); ++i) {
    absl::string_view current = path;
    absl::string_view mask_path = mask.paths(i);
    if (current == mask_path) {
      return true;
    }
      // Also check whether mask.paths(i) is a prefix of path.
    if (mask_path.length() < current.length() &&
        absl::ConsumePrefix(&current, mask_path) &&
        absl::ConsumePrefix(&current, ".")) {
      return true;
    }
  }
  return false;
}

void FieldMaskUtil::MergeMessageTo(const Message& source, const FieldMask& mask,
                                   const MergeOptions& options,
                                   Message* destination) {
  ABSL_CHECK(source.GetDescriptor() == destination->GetDescriptor());
  // Build a FieldMaskTree and walk through the tree to merge all specified
  // fields.
  FieldMaskTree tree;
  tree.MergeFromFieldMask(mask);
  tree.MergeMessage(source, options, destination);
}

bool FieldMaskUtil::TrimMessage(const FieldMask& mask, Message* message) {
  // Build a FieldMaskTree and walk through the tree to merge all specified
  // fields.
  FieldMaskTree tree;
  tree.MergeFromFieldMask(mask);
  return tree.TrimMessage(ABSL_DIE_IF_NULL(message));
}

bool FieldMaskUtil::TrimMessage(const FieldMask& mask, Message* message,
                                const TrimOptions& options) {
  // Build a FieldMaskTree and walk through the tree to merge all specified
  // fields.
  FieldMaskTree tree;
  tree.MergeFromFieldMask(mask);
  // If keep_required_fields is true, implicitly add required fields of
  // a message present in the tree to prevent from trimming.
  if (options.keep_required_fields()) {
    tree.AddRequiredFieldPath(ABSL_DIE_IF_NULL(message->GetDescriptor()));
  }
  return tree.TrimMessage(ABSL_DIE_IF_NULL(message));
}

}  // namespace util
}  // namespace protobuf
}  // namespace google
