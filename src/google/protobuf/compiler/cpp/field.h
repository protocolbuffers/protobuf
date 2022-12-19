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

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#ifndef GOOGLE_PROTOBUF_COMPILER_CPP_FIELD_H__
#define GOOGLE_PROTOBUF_COMPILER_CPP_FIELD_H__

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "google/protobuf/descriptor.h"
#include "absl/container/flat_hash_map.h"
#include "google/protobuf/stubs/logging.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/cpp/options.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

// Customization points for each field codegen type. See FieldGenWrapper to
// see how each of these functions is used.
class FieldGenerator {
 public:
  explicit FieldGenerator(const FieldDescriptor* descriptor,
                          const Options& options)
      : descriptor_(descriptor), options_(options) {}

  FieldGenerator(const FieldGenerator&) = delete;
  FieldGenerator& operator=(const FieldGenerator&) = delete;

  virtual ~FieldGenerator() = 0;

  virtual void GeneratePrivateMembers(io::Printer* p) const = 0;

  virtual void GenerateStaticMembers(io::Printer* p) const {}

  virtual void GenerateAccessorDeclarations(io::Printer* p) const = 0;

  virtual void GenerateInlineAccessorDefinitions(io::Printer* p) const = 0;

  virtual void GenerateNonInlineAccessorDefinitions(io::Printer* p) const {}

  virtual void GenerateInternalAccessorDefinitions(io::Printer* p) const {}

  virtual void GenerateInternalAccessorDeclarations(io::Printer* p) const {}

  virtual void GenerateClearingCode(io::Printer* p) const = 0;

  virtual void GenerateMessageClearingCode(io::Printer* p) const {
    GenerateClearingCode(p);
  }

  virtual void GenerateMergingCode(io::Printer* p) const = 0;

  virtual void GenerateCopyConstructorCode(io::Printer* p) const;

  virtual void GenerateSwappingCode(io::Printer* p) const = 0;

  virtual void GenerateConstructorCode(io::Printer* p) const = 0;

  virtual void GenerateDestructorCode(io::Printer* p) const {}

  virtual void GenerateArenaDestructorCode(io::Printer* p) const {
    GOOGLE_ABSL_CHECK(NeedsArenaDestructor() == ArenaDtorNeeds::kNone)
        << descriptor_->cpp_type_name();
  }

  virtual void GenerateAggregateInitializer(io::Printer* p) const;

  virtual void GenerateConstexprAggregateInitializer(io::Printer* p) const;

  virtual void GenerateCopyAggregateInitializer(io::Printer* p) const;

  virtual void GenerateSerializeWithCachedSizesToArray(
      io::Printer* p) const = 0;

  virtual void GenerateByteSize(io::Printer* p) const = 0;

  virtual void GenerateIsInitialized(io::Printer* p) const {}

  virtual void GenerateIfHasField(io::Printer* p) const;

  virtual bool IsInlined() const { return false; }

  virtual ArenaDtorNeeds NeedsArenaDestructor() const {
    return ArenaDtorNeeds::kNone;
  }

  void SetHasBitIndex(int32_t has_bit_index);
  void SetInlinedStringIndex(int32_t inlined_string_index);

 protected:
  // TODO(b/245791219): Remove these members and make this a pure interface.
  const FieldDescriptor* descriptor_;
  const Options& options_;
  absl::flat_hash_map<absl::string_view, std::string> variables_;
};

inline FieldGenerator::~FieldGenerator() = default;

class FieldGenWrapper {
 public:
  FieldGenWrapper(const FieldDescriptor* field, const Options& options,
                  MessageSCCAnalyzer* scc_analyzer);

  FieldGenWrapper(const FieldGenWrapper&) = delete;
  FieldGenWrapper& operator=(const FieldGenWrapper&) = delete;
  FieldGenWrapper(FieldGenWrapper&&) = default;
  FieldGenWrapper& operator=(FieldGenWrapper&&) = default;

  // Prints private members needed to represent this field.
  //
  // These are placed inside the class definition.
  void GeneratePrivateMembers(io::Printer* p) const {
    impl_->GeneratePrivateMembers(p);
  }

  // Prints static members needed to represent this field.
  //
  // These are placed inside the class definition.
  void GenerateStaticMembers(io::Printer* p) const {
    impl_->GenerateStaticMembers(p);
  }

  // Generates declarations for all of the accessor functions related to this
  // field.
  //
  // These are placed inside the class definition.
  void GenerateAccessorDeclarations(io::Printer* p) const {
    impl_->GenerateAccessorDeclarations(p);
  }

  // Generates inline definitions of accessor functions for this field.
  //
  // These are placed in namespace scope in the header after all class
  // definitions.
  void GenerateInlineAccessorDefinitions(io::Printer* p) const {
    impl_->GenerateInlineAccessorDefinitions(p);
  }

  // Generates definitions of accessors that aren't inlined.
  //
  // These are placed in namespace scope in the .cc file.
  void GenerateNonInlineAccessorDefinitions(io::Printer* p) const {
    impl_->GenerateNonInlineAccessorDefinitions(p);
  }

  // Generates declarations of accessors that are for internal purposes only.
  void GenerateInternalAccessorDefinitions(io::Printer* p) const {
    impl_->GenerateInternalAccessorDefinitions(p);
  }

  // Generates definitions of accessors that are for internal purposes only.
  void GenerateInternalAccessorDeclarations(io::Printer* p) const {
    impl_->GenerateInternalAccessorDeclarations(p);
  }

  // Generates statements which clear the field.
  //
  // This is used to define the clear_$name$() method.
  void GenerateClearingCode(io::Printer* p) const {
    impl_->GenerateClearingCode(p);
  }

  // Generates statements which clear the field as part of the Clear() method
  // for the whole message.
  //
  // For message types which have field presence bits,
  // MessageGenerator::GenerateClear will have already checked the presence
  // bits.
  void GenerateMessageClearingCode(io::Printer* p) const {
    impl_->GenerateMessageClearingCode(p);
  }

  // Generates statements which merge the contents of the field from the current
  // message to the target message, which is stored in the generated code
  // variable `from`.
  //
  // This is used to fill in the MergeFrom method for the whole message.
  //
  // Details of this usage can be found in message.cc under the
  // GenerateMergeFrom method.
  void GenerateMergingCode(io::Printer* p) const {
    impl_->GenerateMergingCode(p);
  }

  // Generates a copy constructor
  //
  // TODO(b/245791219): Document this properly.
  void GenerateCopyConstructorCode(io::Printer* p) const {
    impl_->GenerateCopyConstructorCode(p);
  }

  // Generates statements which swap this field and the corresponding field of
  // another message, which is stored in the generated code variable `other`.
  //
  // This is used to define the Swap method. Details of usage can be found in
  // message.cc under the GenerateSwap method.
  void GenerateSwappingCode(io::Printer* p) const {
    impl_->GenerateSwappingCode(p);
  }

  // Generates initialization code for private members declared by
  // GeneratePrivateMembers().
  //
  // These go into the message class's SharedCtor() method, invoked by each of
  // the generated constructors.
  void GenerateConstructorCode(io::Printer* p) const {
    impl_->GenerateConstructorCode(p);
  }

  // Generates any code that needs to go in the class's SharedDtor() method,
  // invoked by the destructor.
  void GenerateDestructorCode(io::Printer* p) const {
    impl_->GenerateDestructorCode(p);
  }

  // Generates a manual destructor invocation for use when the message is on an
  // arena.
  //
  // The code that this method generates will be executed inside a
  // shared-for-the-whole-message-class method registered with OwnDestructor().
  void GenerateArenaDestructorCode(io::Printer* p) const {
    impl_->GenerateArenaDestructorCode(p);
  }

  // Generates initialization code for private members declared by
  // GeneratePrivateMembers().
  //
  // These go into the SharedCtor's aggregate initialization of the _impl_
  // struct and must follow the syntax `decltype($field$){$default$}`.
  // Does not include `:` or `,` separators. Default values should be specified
  // here when possible.
  //
  // NOTE: We use `decltype($field$)` for both explicit construction and the
  // fact that it's self-documenting. Pre-C++17, copy elision isn't guaranteed
  // in aggregate initialization so a valid copy/move constructor must exist
  // (even though it's not used). Because of this, we need to comment out the
  // decltype and fallback to implicit construction.
  void GenerateAggregateInitializer(io::Printer* p) const {
    impl_->GenerateAggregateInitializer(p);
  }

  // Generates constinit initialization code for private members declared by
  // GeneratePrivateMembers().
  //
  // These go into the constexpr constructor's aggregate initialization of the
  // _impl_ struct and must follow the syntax `/*decltype($field$)*/{}` (see
  // above). Does not include `:` or `,` separators.
  void GenerateConstexprAggregateInitializer(io::Printer* p) const {
    impl_->GenerateConstexprAggregateInitializer(p);
  }

  // Generates copy initialization code for private members declared by
  // GeneratePrivateMembers().
  //
  // These go into the copy constructor's aggregate initialization of the _impl_
  // struct and must follow the syntax `decltype($field$){from.$field$}` (see
  // above). Does not include `:` or `,` separators.
  void GenerateCopyAggregateInitializer(io::Printer* p) const {
    impl_->GenerateCopyAggregateInitializer(p);
  }

  // Generates statements to serialize this field directly to the array
  // `target`, which are placed within the message's
  // SerializeWithCachedSizesToArray() method.
  //
  // This must also advance `target` past the written bytes.
  void GenerateSerializeWithCachedSizesToArray(io::Printer* p) const {
    impl_->GenerateSerializeWithCachedSizesToArray(p);
  }

  // Generates statements to compute the serialized size of this field, which
  // are placed in the message's ByteSize() method.
  void GenerateByteSize(io::Printer* p) const { impl_->GenerateByteSize(p); }

  // Generates lines to call IsInitialized() for eligible message fields. Non
  // message fields won't need to override this function.
  void GenerateIsInitialized(io::Printer* p) const {
    impl_->GenerateIsInitialized(p);
  }

  // TODO(b/245791219): Document this properly.
  void GenerateIfHasField(io::Printer* p) const {
    impl_->GenerateIfHasField(p);
  }

  // TODO(b/245791219): Document this properly.
  bool IsInlined() const { return impl_->IsInlined(); }

  // TODO(b/245791219): Document this properly.
  ArenaDtorNeeds NeedsArenaDestructor() const {
    return impl_->NeedsArenaDestructor();
  }

 private:
  friend class FieldGeneratorMap;
  std::unique_ptr<FieldGenerator> impl_;
};

// Convenience class which constructs FieldGenerators for a Descriptor.
class FieldGeneratorMap {
 public:
  FieldGeneratorMap(const Descriptor* descriptor, const Options& options,
                    MessageSCCAnalyzer* scc_analyzer);

  FieldGeneratorMap(const FieldGeneratorMap&) = delete;
  FieldGeneratorMap& operator=(const FieldGeneratorMap&) = delete;

  const FieldGenWrapper& get(const FieldDescriptor* field) const {
    GOOGLE_ABSL_CHECK_EQ(field->containing_type(), descriptor_);
    return fields_[field->index()];
  }

  void SetHasBitIndices(absl::Span<const int> has_bit_indices) {
    for (int i = 0; i < descriptor_->field_count(); ++i) {
      fields_[i].impl_->SetHasBitIndex(has_bit_indices[i]);
    }
  }

  void SetInlinedStringIndices(absl::Span<const int> inlined_string_indices) {
    for (int i = 0; i < descriptor_->field_count(); ++i) {
      fields_[i].impl_->SetInlinedStringIndex(inlined_string_indices[i]);
    }
  }

 private:
  const Descriptor* descriptor_;
  std::vector<FieldGenWrapper> fields_;
};

// Helper function: set variables in the map that are the same for all
// field code generators.
// ['name', 'index', 'number', 'classname', 'declared_type', 'tag_size',
// 'deprecation'].
//
// Each function comes in a legacy "write variables to an existing variable
// map" and a new version that returns a new map object for use with WithVars().

absl::flat_hash_map<absl::string_view, std::string> FieldVars(
    const FieldDescriptor* desc, const Options& opts);

absl::flat_hash_map<absl::string_view, std::string> OneofFieldVars(
    const FieldDescriptor* descriptor);

void SetCommonFieldVariables(
    const FieldDescriptor* descriptor,
    absl::flat_hash_map<absl::string_view, std::string>* variables,
    const Options& options);

void SetCommonOneofFieldVariables(
    const FieldDescriptor* descriptor,
    absl::flat_hash_map<absl::string_view, std::string>* variables);
}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_CPP_FIELD_H__
