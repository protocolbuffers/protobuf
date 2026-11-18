// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// This file contains the SymbolChecker class which is used to enforce
// visibility rules within a FileDescriptor.
//
// The SymbolChecker is used to validate that a FileDescriptor is compatible
// with the rules of the protobuf symbol visibility specifications.  When
// enabled these rules are used to ensure FileDescriptors are narrowly scoped.

#ifndef GOOGLE_PROTOBUF_SYMBOL_CHECKER_H__
#define GOOGLE_PROTOBUF_SYMBOL_CHECKER_H__

#include <string>
#include <vector>

#include "absl/strings/string_view.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {

// Forward Declare Mesasge and Descriptor types.
class Message;
class Descriptor;
class DescriptorProto;
class FileDescriptor;
class FileDescriptorProto;
class EnumDescriptor;
class EnumDescriptorProto;

enum SymbolCheckerErrorType {
  // A nested message was marked 'export' while using STRICT visibility.
  NESTED_MESSAGE_STRICT_VIOLATION,
  // A nested enum was marked 'export' while using STRICT visibility.
  NESTED_ENUM_STRICT_VIOLATION,
};

// A single error generated from the SymbolChecker methods.
class SymbolCheckerError {
 public:
  SymbolCheckerError(absl::string_view symbol_name, const Message& descriptor,
                     SymbolCheckerErrorType type)
      : symbol_name_(symbol_name), descriptor_(descriptor), type_(type) {}

  absl::string_view symbol_name() const { return symbol_name_; }
  const Message* descriptor() const { return &descriptor_; }
  SymbolCheckerErrorType type() const { return type_; }

  std::string message() const;

 private:
  absl::string_view symbol_name_;
  const Message& descriptor_;
  SymbolCheckerErrorType type_;
};

namespace internal {
// Pair for symbol type and its proto.
template <typename DescriptorT, typename DescriptorProtoT>
struct DescriptorAndProto {
  const DescriptorT* descriptor;
  const DescriptorProtoT* proto;
};

// Internal State used for checking symbol visibility rules.
typedef DescriptorAndProto<Descriptor, DescriptorProto>
    MessageDescriptorAndProto;
typedef DescriptorAndProto<EnumDescriptor, EnumDescriptorProto>
    EnumDescriptorAndProto;

// Internal State for SymbolChecker kept out of public API.
struct SymbolCheckerState {
  std::vector<MessageDescriptorAndProto> nested_messages;
  std::vector<EnumDescriptorAndProto> nested_enums;
  // Enums that are considered 'namespaced' by IsEnumNamespaceMessage
  std::vector<EnumDescriptorAndProto> namespaced_enums;
};

}  // namespace internal

// Container for visibility checking state.  This class is NOT thread-safe,
// concurrent calls to Initialize will result in undefined behavior.
//
// This should not be considered public API and is intended for internal tooling
// and CLI access only.
class PROTOBUF_EXPORT SymbolChecker {
 public:
  SymbolChecker(const FileDescriptor* file, const FileDescriptorProto& proto);
  SymbolChecker(const SymbolChecker&) = delete;
  SymbolChecker& operator=(const SymbolChecker&) = delete;
  ~SymbolChecker() = default;

  // Returns true iff the message is a pure zero field message used only for
  // Enum namespacing.  AKA it is:
  // * top-level
  // * visibility local either explicitly or by file default
  // * has reserved range of 1 to max.
  // Example:
  // local message Foo {
  //   export enum Type {
  //     TYPE_UNSPECIFIED = 0;
  //   }
  //   reserved 1 to max;
  // }
  PROTOBUF_FUTURE_ADD_EARLY_NODISCARD static bool IsEnumNamespaceMessage(
      const Descriptor& container);

  // Returns true iff the enum is a namespaced enum.  This is an enum that is
  // nested within a message that is considered an 'EnumNamespaceMessage' as
  // defined above. IsEnumNamespaceMessage must return true for the enums
  // container message.
  PROTOBUF_FUTURE_ADD_EARLY_NODISCARD static bool IsNamespacedEnum(
      const EnumDescriptor& enm);

  // Return a list of errors identified in the given FileDescriptor/proto w.r.t
  // to SymbolChecking visibility.
  PROTOBUF_FUTURE_ADD_EARLY_NODISCARD std::vector<SymbolCheckerError>
  CheckSymbolVisibilityRules();

 private:
  void Initialize();

  bool initialized_;
  const FileDescriptor* descriptor_;
  const FileDescriptorProto& proto_;

  internal::SymbolCheckerState state_;
};

}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_SYMBOL_CHECKER_H__
