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

#ifndef GOOGLE_PROTOBUF_COMPILER_CPP_MESSAGE_H__
#define GOOGLE_PROTOBUF_COMPILER_CPP_MESSAGE_H__

#include <cstdint>
#include <memory>
#include <set>
#include <string>

#include <google/protobuf/compiler/cpp/field.h>
#include <google/protobuf/compiler/cpp/helpers.h>
#include <google/protobuf/compiler/cpp/message_layout_helper.h>
#include <google/protobuf/compiler/cpp/options.h>
#include <google/protobuf/compiler/cpp/parse_function_generator.h>

namespace google {
namespace protobuf {
namespace io {
class Printer;  // printer.h
}
}  // namespace protobuf
}  // namespace google

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

class EnumGenerator;       // enum.h
class ExtensionGenerator;  // extension.h

class MessageGenerator {
 public:
  // See generator.cc for the meaning of dllexport_decl.
  MessageGenerator(const Descriptor* descriptor,
                   const std::map<std::string, std::string>& vars,
                   int index_in_file_messages, const Options& options,
                   MessageSCCAnalyzer* scc_analyzer);
  ~MessageGenerator();

  // Append the two types of nested generators to the corresponding vector.
  void AddGenerators(
      std::vector<std::unique_ptr<EnumGenerator>>* enum_generators,
      std::vector<std::unique_ptr<ExtensionGenerator>>* extension_generators);

  // Generate definitions for this class and all its nested types.
  void GenerateClassDefinition(io::Printer* printer);

  // Generate definitions of inline methods (placed at the end of the header
  // file).
  void GenerateInlineMethods(io::Printer* printer);

  // Source file stuff.

  // Generate all non-inline methods for this class.
  void GenerateClassMethods(io::Printer* printer);

  // Generate source file code that should go outside any namespace.
  void GenerateSourceInProto2Namespace(io::Printer* printer);

 private:
  // Generate declarations and definitions of accessors for fields.
  void GenerateFieldAccessorDeclarations(io::Printer* printer);
  void GenerateFieldAccessorDefinitions(io::Printer* printer);

  // Generate the field offsets array.  Returns the a pair of the total number
  // of entries generated and the index of the first has_bit entry.
  std::pair<size_t, size_t> GenerateOffsets(io::Printer* printer);
  void GenerateSchema(io::Printer* printer, int offset, int has_offset);

  // Generate constructors and destructor.
  void GenerateStructors(io::Printer* printer);

  // The compiler typically generates multiple copies of each constructor and
  // destructor: http://gcc.gnu.org/bugs.html#nonbugs_cxx
  // Placing common code in a separate method reduces the generated code size.
  //
  // Generate the shared constructor code.
  void GenerateSharedConstructorCode(io::Printer* printer);
  // Generate the shared destructor code.
  void GenerateSharedDestructorCode(io::Printer* printer);
  // Generate the arena-specific destructor code.
  void GenerateArenaDestructorCode(io::Printer* printer);

  // Generate the constexpr constructor for constant initialization of the
  // default instance.
  void GenerateConstexprConstructor(io::Printer* printer);

  void GenerateCreateSplitMessage(io::Printer* printer);
  void GenerateInitDefaultSplitInstance(io::Printer* printer);

  // Generate standard Message methods.
  void GenerateClear(io::Printer* printer);
  void GenerateOneofClear(io::Printer* printer);
  void GenerateVerify(io::Printer* printer);
  void GenerateSerializeWithCachedSizes(io::Printer* printer);
  void GenerateSerializeWithCachedSizesToArray(io::Printer* printer);
  void GenerateSerializeWithCachedSizesBody(io::Printer* printer);
  void GenerateSerializeWithCachedSizesBodyShuffled(io::Printer* printer);
  void GenerateByteSize(io::Printer* printer);
  void GenerateMergeFrom(io::Printer* printer);
  void GenerateClassSpecificMergeImpl(io::Printer* printer);
  void GenerateCopyFrom(io::Printer* printer);
  void GenerateSwap(io::Printer* printer);
  void GenerateIsInitialized(io::Printer* printer);

  // Helpers for GenerateSerializeWithCachedSizes().
  //
  // cached_has_bit_index maintains that:
  //   cached_has_bits = _has_bits_[cached_has_bit_index]
  // for cached_has_bit_index >= 0
  void GenerateSerializeOneField(io::Printer* printer,
                                 const FieldDescriptor* field,
                                 int cached_has_bits_index);
  // Generate a switch statement to serialize 2+ fields from the same oneof.
  // Or, if fields.size() == 1, just call GenerateSerializeOneField().
  void GenerateSerializeOneofFields(
      io::Printer* printer, const std::vector<const FieldDescriptor*>& fields);
  void GenerateSerializeOneExtensionRange(
      io::Printer* printer, const Descriptor::ExtensionRange* range);

  // Generates has_foo() functions and variables for singular field has-bits.
  void GenerateSingularFieldHasBits(const FieldDescriptor* field,
                                    Formatter format);
  // Generates has_foo() functions and variables for oneof field has-bits.
  void GenerateOneofHasBits(io::Printer* printer);
  // Generates has_foo_bar() functions for oneof members.
  void GenerateOneofMemberHasBits(const FieldDescriptor* field,
                                  const Formatter& format);
  // Generates the clear_foo() method for a field.
  void GenerateFieldClear(const FieldDescriptor* field, bool is_inline,
                          Formatter format);

  // Generates the body of the message's copy constructor.
  void GenerateCopyConstructorBody(io::Printer* printer) const;

  // Returns the level that this message needs ArenaDtor. If the message has
  // a field that is not arena-exclusive, it needs an ArenaDtor
  // (go/proto-destructor).
  //
  // - Returning kNone means we don't need to generate ArenaDtor.
  // - Returning kOnDemand means we need to generate ArenaDtor, but don't need
  //   to register ArenaDtor at construction. Such as when the message's
  //   ArenaDtor code is only for destructing inlined string.
  // - Returning kRequired means we meed to generate ArenaDtor and register it
  //   at construction.
  ArenaDtorNeeds NeedsArenaDestructor() const;

  size_t HasBitsSize() const;
  size_t InlinedStringDonatedSize() const;
  int HasBitIndex(const FieldDescriptor* a) const;
  int HasByteIndex(const FieldDescriptor* a) const;
  int HasWordIndex(const FieldDescriptor* a) const;
  bool SameHasByte(const FieldDescriptor* a, const FieldDescriptor* b) const;
  std::vector<uint32_t> RequiredFieldsBitMask() const;

  const Descriptor* descriptor_;
  int index_in_file_messages_;
  std::string classname_;
  Options options_;
  FieldGeneratorMap field_generators_;
  // optimized_order_ is the order we layout the message's fields in the
  // class. This is reused to initialize the fields in-order for cache
  // efficiency.
  //
  // optimized_order_ excludes oneof fields and weak fields.
  std::vector<const FieldDescriptor*> optimized_order_;
  std::vector<int> has_bit_indices_;
  int max_has_bit_index_;

  // A map from field index to inlined_string index. For non-inlined-string
  // fields, the element is -1. If there is no inlined string in the message,
  // this is empty.
  std::vector<int> inlined_string_indices_;
  // The count of inlined_string fields in the message.
  int max_inlined_string_index_;

  std::vector<const EnumGenerator*> enum_generators_;
  std::vector<const ExtensionGenerator*> extension_generators_;
  int num_required_fields_;
  int num_weak_fields_;

  std::unique_ptr<MessageLayoutHelper> message_layout_helper_;
  std::unique_ptr<ParseFunctionGenerator> parse_function_generator_;

  MessageSCCAnalyzer* scc_analyzer_;

  std::map<std::string, std::string> variables_;

  friend class FileGenerator;
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(MessageGenerator);
};

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_CPP_MESSAGE_H__
