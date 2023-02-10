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
#include <tuple>
#include <vector>

#include "google/protobuf/descriptor.h"
#include "absl/container/flat_hash_map.h"
#include "absl/log/absl_check.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/cpp/options.h"
#include "google/protobuf/io/printer.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

// Customization points for each field codegen type. See FieldGenerator to
// see how each of these functions is used.
//
// TODO(b/245791219): Make every function except the dtor in this generator
// non-pure-virtual. A generator with no implementation should be able to
// automatically not contribute any code to the message it is part of, as a
// matter of clean composability.
class FieldGeneratorBase {
 public:
  FieldGeneratorBase(const FieldDescriptor* descriptor, const Options& options)
      : descriptor_(descriptor), options_(options) {}

  FieldGeneratorBase(const FieldGeneratorBase&) = delete;
  FieldGeneratorBase& operator=(const FieldGeneratorBase&) = delete;

  virtual ~FieldGeneratorBase() = 0;

  virtual std::vector<io::Printer::Sub> MakeVars() const { return {}; }

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
    ABSL_CHECK(NeedsArenaDestructor() == ArenaDtorNeeds::kNone)
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

 protected:
  // TODO(b/245791219): Remove these members and make this a pure interface.
  const FieldDescriptor* descriptor_;
  const Options& options_;
  absl::flat_hash_map<absl::string_view, std::string> variables_;
};

inline FieldGeneratorBase::~FieldGeneratorBase() = default;

class FieldGenerator {
 private:
  // This function must be defined here so that the inline definitions below
  // can see it, which is required because it has deduced return type.
  auto PushVarsForCall(io::Printer* p) const {
    // NOTE: we use a struct here because:
    // * We want to ensure that order of evaluation below is well-defined,
    //   which {...} guarantees but (...) does not.
    // * We do not require C++17 as of writing and therefore cannot use
    //   std::tuple with CTAD.
    // * std::make_tuple uses (...), not {...}.
    struct Vars {
      decltype(p->WithVars(field_vars_)) cleanup1;
      decltype(p->WithVars(tracker_vars_)) cleanup2;
      decltype(p->WithVars(per_generator_vars_)) cleanup3;
    };

    return Vars{p->WithVars(field_vars_), p->WithVars(tracker_vars_),
                p->WithVars(per_generator_vars_)};
  }

 public:
  FieldGenerator(const FieldGenerator&) = delete;
  FieldGenerator& operator=(const FieldGenerator&) = delete;
  FieldGenerator(FieldGenerator&&) = default;
  FieldGenerator& operator=(FieldGenerator&&) = default;

  // Prints private members needed to represent this field.
  //
  // These are placed inside the class definition.
  void GeneratePrivateMembers(io::Printer* p) const {
    auto vars = PushVarsForCall(p);
    impl_->GeneratePrivateMembers(p);
  }

  // Prints static members needed to represent this field.
  //
  // These are placed inside the class definition.
  void GenerateStaticMembers(io::Printer* p) const {
    auto vars = PushVarsForCall(p);
    impl_->GenerateStaticMembers(p);
  }

  // Generates declarations for all of the accessor functions related to this
  // field.
  //
  // These are placed inside the class definition.
  void GenerateAccessorDeclarations(io::Printer* p) const {
    auto vars = PushVarsForCall(p);
    impl_->GenerateAccessorDeclarations(p);
  }

  // Generates inline definitions of accessor functions for this field.
  //
  // These are placed in namespace scope in the header after all class
  // definitions.
  void GenerateInlineAccessorDefinitions(io::Printer* p) const {
    auto vars = PushVarsForCall(p);
    impl_->GenerateInlineAccessorDefinitions(p);
  }

  // Generates definitions of accessors that aren't inlined.
  //
  // These are placed in namespace scope in the .cc file.
  void GenerateNonInlineAccessorDefinitions(io::Printer* p) const {
    auto vars = PushVarsForCall(p);
    impl_->GenerateNonInlineAccessorDefinitions(p);
  }

  // Generates declarations of accessors that are for internal purposes only.
  void GenerateInternalAccessorDefinitions(io::Printer* p) const {
    auto vars = PushVarsForCall(p);
    impl_->GenerateInternalAccessorDefinitions(p);
  }

  // Generates definitions of accessors that are for internal purposes only.
  void GenerateInternalAccessorDeclarations(io::Printer* p) const {
    auto vars = PushVarsForCall(p);
    impl_->GenerateInternalAccessorDeclarations(p);
  }

  // Generates statements which clear the field.
  //
  // This is used to define the clear_$name$() method.
  void GenerateClearingCode(io::Printer* p) const {
    auto vars = PushVarsForCall(p);
    impl_->GenerateClearingCode(p);
  }

  // Generates statements which clear the field as part of the Clear() method
  // for the whole message.
  //
  // For message types which have field presence bits,
  // MessageGenerator::GenerateClear will have already checked the presence
  // bits.
  void GenerateMessageClearingCode(io::Printer* p) const {
    auto vars = PushVarsForCall(p);
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
    auto vars = PushVarsForCall(p);
    impl_->GenerateMergingCode(p);
  }

  // Generates a copy constructor
  //
  // TODO(b/245791219): Document this properly.
  void GenerateCopyConstructorCode(io::Printer* p) const {
    auto vars = PushVarsForCall(p);
    impl_->GenerateCopyConstructorCode(p);
  }

  // Generates statements which swap this field and the corresponding field of
  // another message, which is stored in the generated code variable `other`.
  //
  // This is used to define the Swap method. Details of usage can be found in
  // message.cc under the GenerateSwap method.
  void GenerateSwappingCode(io::Printer* p) const {
    auto vars = PushVarsForCall(p);
    impl_->GenerateSwappingCode(p);
  }

  // Generates initialization code for private members declared by
  // GeneratePrivateMembers().
  //
  // These go into the message class's SharedCtor() method, invoked by each of
  // the generated constructors.
  void GenerateConstructorCode(io::Printer* p) const {
    auto vars = PushVarsForCall(p);
    impl_->GenerateConstructorCode(p);
  }

  // Generates any code that needs to go in the class's SharedDtor() method,
  // invoked by the destructor.
  void GenerateDestructorCode(io::Printer* p) const {
    auto vars = PushVarsForCall(p);
    impl_->GenerateDestructorCode(p);
  }

  // Generates a manual destructor invocation for use when the message is on an
  // arena.
  //
  // The code that this method generates will be executed inside a
  // shared-for-the-whole-message-class method registered with OwnDestructor().
  void GenerateArenaDestructorCode(io::Printer* p) const {
    auto vars = PushVarsForCall(p);
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
    auto vars = PushVarsForCall(p);
    impl_->GenerateAggregateInitializer(p);
  }

  // Generates constinit initialization code for private members declared by
  // GeneratePrivateMembers().
  //
  // These go into the constexpr constructor's aggregate initialization of the
  // _impl_ struct and must follow the syntax `/*decltype($field$)*/{}` (see
  // above). Does not include `:` or `,` separators.
  void GenerateConstexprAggregateInitializer(io::Printer* p) const {
    auto vars = PushVarsForCall(p);
    impl_->GenerateConstexprAggregateInitializer(p);
  }

  // Generates copy initialization code for private members declared by
  // GeneratePrivateMembers().
  //
  // These go into the copy constructor's aggregate initialization of the _impl_
  // struct and must follow the syntax `decltype($field$){from.$field$}` (see
  // above). Does not include `:` or `,` separators.
  void GenerateCopyAggregateInitializer(io::Printer* p) const {
    auto vars = PushVarsForCall(p);
    impl_->GenerateCopyAggregateInitializer(p);
  }

  // Generates statements to serialize this field directly to the array
  // `target`, which are placed within the message's
  // SerializeWithCachedSizesToArray() method.
  //
  // This must also advance `target` past the written bytes.
  void GenerateSerializeWithCachedSizesToArray(io::Printer* p) const {
    auto vars = PushVarsForCall(p);
    impl_->GenerateSerializeWithCachedSizesToArray(p);
  }

  // Generates statements to compute the serialized size of this field, which
  // are placed in the message's ByteSize() method.
  void GenerateByteSize(io::Printer* p) const {
    auto vars = PushVarsForCall(p);
    impl_->GenerateByteSize(p);
  }

  // Generates lines to call IsInitialized() for eligible message fields. Non
  // message fields won't need to override this function.
  void GenerateIsInitialized(io::Printer* p) const {
    auto vars = PushVarsForCall(p);
    impl_->GenerateIsInitialized(p);
  }

  // TODO(b/245791219): Document this properly.
  void GenerateIfHasField(io::Printer* p) const {
    auto vars = PushVarsForCall(p);
    impl_->GenerateIfHasField(p);
  }

  // TODO(b/245791219): Document this properly.
  bool IsInlined() const { return impl_->IsInlined(); }

  // TODO(b/245791219): Document this properly.
  ArenaDtorNeeds NeedsArenaDestructor() const {
    return impl_->NeedsArenaDestructor();
  }

 private:
  friend class FieldGeneratorTable;
  FieldGenerator(const FieldDescriptor* field, const Options& options,
                 MessageSCCAnalyzer* scc_analyzer,
                 absl::optional<uint32_t> hasbit_index,
                 absl::optional<uint32_t> inlined_string_index);

  std::unique_ptr<FieldGeneratorBase> impl_;
  std::vector<io::Printer::Sub> field_vars_;
  std::vector<io::Printer::Sub> tracker_vars_;
  std::vector<io::Printer::Sub> per_generator_vars_;
};

// Convenience class which constructs FieldGeneratorBases for a Descriptor.
class FieldGeneratorTable {
 public:
  explicit FieldGeneratorTable(const Descriptor* descriptor)
      : descriptor_(descriptor) {}

  FieldGeneratorTable(const FieldGeneratorTable&) = delete;
  FieldGeneratorTable& operator=(const FieldGeneratorTable&) = delete;

  void Build(const Options& options, MessageSCCAnalyzer* scc_analyzer,
             absl::Span<const int32_t> has_bit_indices,
             absl::Span<const int32_t> inlined_string_indices);

  const FieldGenerator& get(const FieldDescriptor* field) const {
    ABSL_CHECK_EQ(field->containing_type(), descriptor_);
    return fields_[field->index()];
  }

 private:
  const Descriptor* descriptor_;
  std::vector<FieldGenerator> fields_;
};

// Returns variables common to all fields.
//
// TODO(b/245791219): Make this function .cc-private.
std::vector<io::Printer::Sub> FieldVars(const FieldDescriptor* field,
                                        const Options& opts);
}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_CPP_FIELD_H__
