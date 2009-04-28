// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
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

#include <algorithm>
#include <google/protobuf/stubs/hash.h>
#include <google/protobuf/compiler/cpp/cpp_message.h>
#include <google/protobuf/compiler/cpp/cpp_enum.h>
#include <google/protobuf/compiler/cpp/cpp_extension.h>
#include <google/protobuf/compiler/cpp/cpp_helpers.h>
#include <google/protobuf/stubs/strutil.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/wire_format_inl.h>
#include <google/protobuf/descriptor.pb.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

using internal::WireFormat;

namespace {

void PrintFieldComment(io::Printer* printer, const FieldDescriptor* field) {
  // Print the field's proto-syntax definition as a comment.  We don't want to
  // print group bodies so we cut off after the first line.
  string def = field->DebugString();
  printer->Print("// $def$\n",
    "def", def.substr(0, def.find_first_of('\n')));
}

struct FieldOrderingByNumber {
  inline bool operator()(const FieldDescriptor* a,
                         const FieldDescriptor* b) const {
    return a->number() < b->number();
  }
};

const char* kWireTypeNames[] = {
  "VARINT",
  "FIXED64",
  "LENGTH_DELIMITED",
  "START_GROUP",
  "END_GROUP",
  "FIXED32",
};

// Sort the fields of the given Descriptor by number into a new[]'d array
// and return it.
const FieldDescriptor** SortFieldsByNumber(const Descriptor* descriptor) {
  const FieldDescriptor** fields =
    new const FieldDescriptor*[descriptor->field_count()];
  for (int i = 0; i < descriptor->field_count(); i++) {
    fields[i] = descriptor->field(i);
  }
  sort(fields, fields + descriptor->field_count(),
       FieldOrderingByNumber());
  return fields;
}

// Functor for sorting extension ranges by their "start" field number.
struct ExtensionRangeSorter {
  bool operator()(const Descriptor::ExtensionRange* left,
                  const Descriptor::ExtensionRange* right) const {
    return left->start < right->start;
  }
};

// Returns true if the message type has any required fields.  If it doesn't,
// we can optimize out calls to its IsInitialized() method.
//
// already_seen is used to avoid checking the same type multiple times
// (and also to protect against recursion).
static bool HasRequiredFields(
    const Descriptor* type,
    hash_set<const Descriptor*>* already_seen) {
  if (already_seen->count(type) > 0) {
    // Since the first occurrence of a required field causes the whole
    // function to return true, we can assume that if the type is already
    // in the cache it didn't have any required fields.
    return false;
  }
  already_seen->insert(type);

  // If the type has extensions, an extension with message type could contain
  // required fields, so we have to be conservative and assume such an
  // extension exists.
  if (type->extension_range_count() > 0) return true;

  for (int i = 0; i < type->field_count(); i++) {
    const FieldDescriptor* field = type->field(i);
    if (field->is_required()) {
      return true;
    }
    if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      if (HasRequiredFields(field->message_type(), already_seen)) {
        return true;
      }
    }
  }

  return false;
}

static bool HasRequiredFields(const Descriptor* type) {
  hash_set<const Descriptor*> already_seen;
  return HasRequiredFields(type, &already_seen);
}

}

// ===================================================================

MessageGenerator::MessageGenerator(const Descriptor* descriptor,
                                   const string& dllexport_decl)
  : descriptor_(descriptor),
    classname_(ClassName(descriptor, false)),
    dllexport_decl_(dllexport_decl),
    field_generators_(descriptor),
    nested_generators_(new scoped_ptr<MessageGenerator>[
      descriptor->nested_type_count()]),
    enum_generators_(new scoped_ptr<EnumGenerator>[
      descriptor->enum_type_count()]),
    extension_generators_(new scoped_ptr<ExtensionGenerator>[
      descriptor->extension_count()]) {

  for (int i = 0; i < descriptor->nested_type_count(); i++) {
    nested_generators_[i].reset(
      new MessageGenerator(descriptor->nested_type(i), dllexport_decl));
  }

  for (int i = 0; i < descriptor->enum_type_count(); i++) {
    enum_generators_[i].reset(
      new EnumGenerator(descriptor->enum_type(i), dllexport_decl));
  }

  for (int i = 0; i < descriptor->extension_count(); i++) {
    extension_generators_[i].reset(
      new ExtensionGenerator(descriptor->extension(i), dllexport_decl));
  }
}

MessageGenerator::~MessageGenerator() {}

void MessageGenerator::
GenerateForwardDeclaration(io::Printer* printer) {
  printer->Print("class $classname$;\n",
                 "classname", classname_);

  for (int i = 0; i < descriptor_->nested_type_count(); i++) {
    nested_generators_[i]->GenerateForwardDeclaration(printer);
  }
}

void MessageGenerator::
GenerateEnumDefinitions(io::Printer* printer) {
  for (int i = 0; i < descriptor_->nested_type_count(); i++) {
    nested_generators_[i]->GenerateEnumDefinitions(printer);
  }

  for (int i = 0; i < descriptor_->enum_type_count(); i++) {
    enum_generators_[i]->GenerateDefinition(printer);
  }
}

void MessageGenerator::
GenerateFieldAccessorDeclarations(io::Printer* printer) {
  for (int i = 0; i < descriptor_->field_count(); i++) {
    const FieldDescriptor* field = descriptor_->field(i);

    PrintFieldComment(printer, field);

    map<string, string> vars;
    vars["name"] = FieldName(field);
    vars["constant_name"] = FieldConstantName(field);
    vars["number"] = SimpleItoa(field->number());

    if (field->is_repeated()) {
      printer->Print(vars, "inline int $name$_size() const;\n");
    } else {
      printer->Print(vars, "inline bool has_$name$() const;\n");
    }

    printer->Print(vars, "inline void clear_$name$();\n");
    printer->Print(vars, "static const int $constant_name$ = $number$;\n");

    // Generate type-specific accessor declarations.
    field_generators_.get(field).GenerateAccessorDeclarations(printer);

    printer->Print("\n");
  }

  if (descriptor_->extension_range_count() > 0) {
    // Generate accessors for extensions.  We just call a macro located in
    // extension_set.h since the accessors about 80 lines of static code.
    printer->Print(
      "GOOGLE_PROTOBUF_EXTENSION_ACCESSORS($classname$)\n",
      "classname", classname_);
  }
}

void MessageGenerator::
GenerateFieldAccessorDefinitions(io::Printer* printer) {
  printer->Print("// $classname$\n\n", "classname", classname_);

  for (int i = 0; i < descriptor_->field_count(); i++) {
    const FieldDescriptor* field = descriptor_->field(i);

    PrintFieldComment(printer, field);

    map<string, string> vars;
    vars["name"] = FieldName(field);
    vars["index"] = SimpleItoa(field->index());
    vars["classname"] = classname_;

    // Generate has_$name$() or $name$_size().
    if (field->is_repeated()) {
      printer->Print(vars,
        "inline int $classname$::$name$_size() const {\n"
        "  return $name$_.size();\n"
        "}\n");
    } else {
      // Singular field.
      printer->Print(vars,
        "inline bool $classname$::has_$name$() const {\n"
        "  return _has_bit($index$);\n"
        "}\n");
    }

    // Generate clear_$name$()
    printer->Print(vars,
      "inline void $classname$::clear_$name$() {\n");

    printer->Indent();
    field_generators_.get(field).GenerateClearingCode(printer);
    printer->Outdent();

    if (!field->is_repeated()) {
      printer->Print(vars, "  _clear_bit($index$);\n");
    }

    printer->Print("}\n");

    // Generate type-specific accessors.
    field_generators_.get(field).GenerateInlineAccessorDefinitions(printer);

    printer->Print("\n");
  }
}

void MessageGenerator::
GenerateClassDefinition(io::Printer* printer) {
  for (int i = 0; i < descriptor_->nested_type_count(); i++) {
    nested_generators_[i]->GenerateClassDefinition(printer);
    printer->Print("\n");
    printer->Print(kThinSeparator);
    printer->Print("\n");
  }

  map<string, string> vars;
  vars["classname"] = classname_;
  vars["field_count"] = SimpleItoa(descriptor_->field_count());
  if (dllexport_decl_.empty()) {
    vars["dllexport"] = "";
  } else {
    vars["dllexport"] = dllexport_decl_ + " ";
  }

  printer->Print(vars,
    "class $dllexport$$classname$ : public ::google::protobuf::Message {\n"
    " public:\n");
  printer->Indent();

  printer->Print(vars,
    "$classname$();\n"
    "virtual ~$classname$();\n"
    "\n"
    "$classname$(const $classname$& from);\n"
    "\n"
    "inline $classname$& operator=(const $classname$& from) {\n"
    "  CopyFrom(from);\n"
    "  return *this;\n"
    "}\n"
    "\n"
    "inline const ::google::protobuf::UnknownFieldSet& unknown_fields() const {\n"
    "  return _unknown_fields_;\n"
    "}\n"
    "\n"
    "inline ::google::protobuf::UnknownFieldSet* mutable_unknown_fields() {\n"
    "  return &_unknown_fields_;\n"
    "}\n"
    "\n"
    "static const ::google::protobuf::Descriptor* descriptor();\n"
    "static const $classname$& default_instance();\n"
    "void Swap($classname$* other);\n"
    "\n"
    "// implements Message ----------------------------------------------\n"
    "\n"
    "$classname$* New() const;\n");

  if (descriptor_->file()->options().optimize_for() == FileOptions::SPEED) {
    printer->Print(vars,
      "void CopyFrom(const ::google::protobuf::Message& from);\n"
      "void MergeFrom(const ::google::protobuf::Message& from);\n"
      "void CopyFrom(const $classname$& from);\n"
      "void MergeFrom(const $classname$& from);\n"
      "void Clear();\n"
      "bool IsInitialized() const;\n");

    if (!descriptor_->options().message_set_wire_format()) {
      // For message_set_wire_format, we don't generate parsing or
      // serialization code even if optimize_for = SPEED, since MessageSet
      // encoding is somewhat more complicated than normal extension encoding
      // and we'd like to avoid having to implement it in multiple places.
      // WireFormat's implementation is probably good enough.
      printer->Print(vars,
        "\n"
        "int ByteSize() const;\n"
        "bool MergePartialFromCodedStream(\n"
        "    ::google::protobuf::io::CodedInputStream* input);\n"
        "void SerializeWithCachedSizes(\n"
        "    ::google::protobuf::io::CodedOutputStream* output) const;\n"
        "::google::protobuf::uint8* SerializeWithCachedSizesToArray(::google::protobuf::uint8* output) const;\n");
    }
  }

  printer->Print(vars,
    "int GetCachedSize() const { return _cached_size_; }\n"
    "private:\n"
    "void SharedCtor();\n"
    "void SharedDtor();\n"
    "void SetCachedSize(int size) const { _cached_size_ = size; }\n"
    "public:\n"
    "\n"
    "const ::google::protobuf::Descriptor* GetDescriptor() const;\n"
    "const ::google::protobuf::Reflection* GetReflection() const;\n"
    "\n"
    "// nested types ----------------------------------------------------\n"
    "\n");

  // Import all nested message classes into this class's scope with typedefs.
  for (int i = 0; i < descriptor_->nested_type_count(); i++) {
    const Descriptor* nested_type = descriptor_->nested_type(i);
    printer->Print("typedef $nested_full_name$ $nested_name$;\n",
                   "nested_name", nested_type->name(),
                   "nested_full_name", ClassName(nested_type, false));
  }

  if (descriptor_->nested_type_count() > 0) {
    printer->Print("\n");
  }

  // Import all nested enums and their values into this class's scope with
  // typedefs and constants.
  for (int i = 0; i < descriptor_->enum_type_count(); i++) {
    enum_generators_[i]->GenerateSymbolImports(printer);
    printer->Print("\n");
  }

  printer->Print(
    "// accessors -------------------------------------------------------\n"
    "\n");

  // Generate accessor methods for all fields.
  GenerateFieldAccessorDeclarations(printer);

  // Declare extension identifiers.
  for (int i = 0; i < descriptor_->extension_count(); i++) {
    extension_generators_[i]->GenerateDeclaration(printer);
  }

  // Generate private members for fields.
  printer->Outdent();
  printer->Print(" private:\n");
  printer->Indent();

  if (descriptor_->extension_range_count() > 0) {
    printer->Print(
      "::google::protobuf::internal::ExtensionSet _extensions_;\n");
  }

  // TODO(kenton):  Make _cached_size_ an atomic<int> when C++ supports it.
  printer->Print(
    "::google::protobuf::UnknownFieldSet _unknown_fields_;\n"
    "mutable int _cached_size_;\n"
    "\n");
  for (int i = 0; i < descriptor_->field_count(); i++) {
    field_generators_.get(descriptor_->field(i))
                     .GeneratePrivateMembers(printer);
  }

  // Declare AddDescriptors() and BuildDescriptors() as friends so that they
  // can assign private static variables like default_instance_ and reflection_.
  printer->Print(
    "friend void $dllexport_decl$ $adddescriptorsname$();\n",
    "dllexport_decl", dllexport_decl_,
    "adddescriptorsname",
      GlobalAddDescriptorsName(descriptor_->file()->name()));
  printer->Print(
    "friend void $assigndescriptorsname$();\n",
    "assigndescriptorsname",
      GlobalAssignDescriptorsName(descriptor_->file()->name()));

  // Generate offsets and _has_bits_ boilerplate.
  if (descriptor_->field_count() > 0) {
    printer->Print(vars,
      "::google::protobuf::uint32 _has_bits_[($field_count$ + 31) / 32];\n");
  } else {
    // Zero-size arrays aren't technically allowed, and MSVC in particular
    // doesn't like them.  We still need to declare these arrays to make
    // other code compile.  Since this is an uncommon case, we'll just declare
    // them with size 1 and waste some space.  Oh well.
    printer->Print(
      "::google::protobuf::uint32 _has_bits_[1];\n");
  }

  printer->Print(
    "\n"
    "// WHY DOES & HAVE LOWER PRECEDENCE THAN != !?\n"
    "inline bool _has_bit(int index) const {\n"
    "  return (_has_bits_[index / 32] & (1u << (index % 32))) != 0;\n"
    "}\n"
    "inline void _set_bit(int index) {\n"
    "  _has_bits_[index / 32] |= (1u << (index % 32));\n"
    "}\n"
    "inline void _clear_bit(int index) {\n"
    "  _has_bits_[index / 32] &= ~(1u << (index % 32));\n"
    "}\n"
    "\n"
    "void InitAsDefaultInstance();\n"
    "static $classname$* default_instance_;\n",
    "classname", classname_);

  printer->Outdent();
  printer->Print(vars, "};");
}

void MessageGenerator::
GenerateInlineMethods(io::Printer* printer) {
  for (int i = 0; i < descriptor_->nested_type_count(); i++) {
    nested_generators_[i]->GenerateInlineMethods(printer);
    printer->Print(kThinSeparator);
    printer->Print("\n");
  }

  GenerateFieldAccessorDefinitions(printer);
}

void MessageGenerator::
GenerateDescriptorDeclarations(io::Printer* printer) {
  printer->Print(
    "const ::google::protobuf::Descriptor* $name$_descriptor_ = NULL;\n"
    "const ::google::protobuf::internal::GeneratedMessageReflection*\n"
    "  $name$_reflection_ = NULL;\n",
    "name", classname_);

  for (int i = 0; i < descriptor_->nested_type_count(); i++) {
    nested_generators_[i]->GenerateDescriptorDeclarations(printer);
  }

  for (int i = 0; i < descriptor_->enum_type_count(); i++) {
    printer->Print(
      "const ::google::protobuf::EnumDescriptor* $name$_descriptor_ = NULL;\n",
      "name", ClassName(descriptor_->enum_type(i), false));
  }
}

void MessageGenerator::
GenerateDescriptorInitializer(io::Printer* printer, int index) {
  // TODO(kenton):  Passing the index to this method is redundant; just use
  //   descriptor_->index() instead.
  map<string, string> vars;
  vars["classname"] = classname_;
  vars["index"] = SimpleItoa(index);

  // Obtain the descriptor from the parent's descriptor.
  if (descriptor_->containing_type() == NULL) {
    printer->Print(vars,
      "$classname$_descriptor_ = file->message_type($index$);\n");
  } else {
    vars["parent"] = ClassName(descriptor_->containing_type(), false);
    printer->Print(vars,
      "$classname$_descriptor_ = "
        "$parent$_descriptor_->nested_type($index$);\n");
  }

  // Generate the offsets.
  GenerateOffsets(printer);

  // Construct the reflection object.
  printer->Print(vars,
    "$classname$_reflection_ =\n"
    "  new ::google::protobuf::internal::GeneratedMessageReflection(\n"
    "    $classname$_descriptor_,\n"
    "    $classname$::default_instance_,\n"
    "    $classname$_offsets_,\n"
    "    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET($classname$, _has_bits_[0]),\n"
    "    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET("
      "$classname$, _unknown_fields_),\n");
  if (descriptor_->extension_range_count() > 0) {
    printer->Print(vars,
      "    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET("
        "$classname$, _extensions_),\n");
  } else {
    // No extensions.
    printer->Print(vars,
      "    -1,\n");
  }
  printer->Print(vars,
    "    ::google::protobuf::DescriptorPool::generated_pool(),\n"
    "    ::google::protobuf::MessageFactory::generated_factory(),\n"
    "    sizeof($classname$));\n");

  // Handle nested types.
  for (int i = 0; i < descriptor_->nested_type_count(); i++) {
    nested_generators_[i]->GenerateDescriptorInitializer(printer, i);
  }

  for (int i = 0; i < descriptor_->enum_type_count(); i++) {
    enum_generators_[i]->GenerateDescriptorInitializer(printer, i);
  }
}

void MessageGenerator::
GenerateTypeRegistrations(io::Printer* printer) {
  // Register this message type with the message factory.
  printer->Print(
    "::google::protobuf::MessageFactory::InternalRegisterGeneratedMessage(\n"
    "  $classname$_descriptor_, &$classname$::default_instance());\n",
    "classname", classname_);

  // Handle nested types.
  for (int i = 0; i < descriptor_->nested_type_count(); i++) {
    nested_generators_[i]->GenerateTypeRegistrations(printer);
  }
}

void MessageGenerator::
GenerateDefaultInstanceAllocator(io::Printer* printer) {
  // Construct the default instance.  We can't call InitAsDefaultInstance() yet
  // because we need to make sure all default instances that this one might
  // depend on are constructed first.
  printer->Print(
    "$classname$::default_instance_ = new $classname$();\n",
    "classname", classname_);

  // Handle nested types.
  for (int i = 0; i < descriptor_->nested_type_count(); i++) {
    nested_generators_[i]->GenerateDefaultInstanceAllocator(printer);
  }
}

void MessageGenerator::
GenerateDefaultInstanceInitializer(io::Printer* printer) {
  printer->Print(
    "$classname$::default_instance_->InitAsDefaultInstance();\n",
    "classname", classname_);

  // Register extensions.
  for (int i = 0; i < descriptor_->extension_count(); i++) {
    extension_generators_[i]->GenerateRegistration(printer);
  }

  // Handle nested types.
  for (int i = 0; i < descriptor_->nested_type_count(); i++) {
    nested_generators_[i]->GenerateDefaultInstanceInitializer(printer);
  }
}

void MessageGenerator::
GenerateClassMethods(io::Printer* printer) {
  for (int i = 0; i < descriptor_->enum_type_count(); i++) {
    enum_generators_[i]->GenerateMethods(printer);
  }

  for (int i = 0; i < descriptor_->nested_type_count(); i++) {
    nested_generators_[i]->GenerateClassMethods(printer);
    printer->Print("\n");
    printer->Print(kThinSeparator);
    printer->Print("\n");
  }

  // Generate non-inline field definitions.
  for (int i = 0; i < descriptor_->field_count(); i++) {
    field_generators_.get(descriptor_->field(i))
                     .GenerateNonInlineAccessorDefinitions(printer);
  }

  // Generate field number constants.
  printer->Print("#ifndef _MSC_VER\n");
  for (int i = 0; i < descriptor_->field_count(); i++) {
    const FieldDescriptor *field = descriptor_->field(i);
    printer->Print(
      "const int $classname$::$constant_name$;\n",
      "classname", ClassName(FieldScope(field), false),
      "constant_name", FieldConstantName(field));
  }
  printer->Print(
    "#endif  // !_MSC_VER\n"
    "\n");

  // Define extension identifiers.
  for (int i = 0; i < descriptor_->extension_count(); i++) {
    extension_generators_[i]->GenerateDefinition(printer);
  }

  GenerateStructors(printer);
  printer->Print("\n");

  if (descriptor_->file()->options().optimize_for() == FileOptions::SPEED) {
    GenerateClear(printer);
    printer->Print("\n");

    if (!descriptor_->options().message_set_wire_format()) {
      // For message_set_wire_format, we don't generate parsing or
      // serialization code even if optimize_for = SPEED, since MessageSet
      // encoding is somewhat more complicated than normal extension encoding
      // and we'd like to avoid having to implement it in multiple places.
      // WireFormat's implementation is probably good enough.
      GenerateMergeFromCodedStream(printer);
      printer->Print("\n");

      GenerateSerializeWithCachedSizes(printer);
      printer->Print("\n");

      GenerateSerializeWithCachedSizesToArray(printer);
      printer->Print("\n");

      GenerateByteSize(printer);
      printer->Print("\n");
    }

    GenerateMergeFrom(printer);
    printer->Print("\n");

    GenerateCopyFrom(printer);
    printer->Print("\n");

    GenerateSwap(printer);
    printer->Print("\n");

    GenerateIsInitialized(printer);
    printer->Print("\n");
  }

  printer->Print(
    "const ::google::protobuf::Descriptor* $classname$::GetDescriptor() const {\n"
    "  return descriptor();\n"
    "}\n"
    "\n"
    "const ::google::protobuf::Reflection* $classname$::GetReflection() const {\n"
    "  protobuf_AssignDescriptorsOnce();\n"
    "  return $classname$_reflection_;\n"
    "}\n",
    "classname", classname_);
}

void MessageGenerator::
GenerateOffsets(io::Printer* printer) {
  printer->Print(
    "static const int $classname$_offsets_[$field_count$] = {\n",
    "classname", classname_,
    "field_count", SimpleItoa(max(1, descriptor_->field_count())));
  printer->Indent();

  for (int i = 0; i < descriptor_->field_count(); i++) {
    const FieldDescriptor* field = descriptor_->field(i);
    printer->Print(
      "GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET($classname$, $name$_),\n",
      "classname", classname_,
      "name", FieldName(field));
  }

  printer->Outdent();
  printer->Print("};\n");
}

void MessageGenerator::
GenerateInitializerList(io::Printer* printer) {
  printer->Indent();
  printer->Indent();

  printer->Print(
    "::google::protobuf::Message()");

  printer->Outdent();
  printer->Outdent();
}

void MessageGenerator::
GenerateSharedConstructorCode(io::Printer* printer) {
  printer->Print(
    "void $classname$::SharedCtor() {\n",
    "classname", classname_);
  printer->Indent();

  printer->Print(
    "_cached_size_ = 0;\n");

  for (int i = 0; i < descriptor_->field_count(); i++) {
    field_generators_.get(descriptor_->field(i))
                     .GenerateConstructorCode(printer);
  }

  printer->Print(
    "::memset(_has_bits_, 0, sizeof(_has_bits_));\n");

  printer->Outdent();
  printer->Print("}\n\n");
}

void MessageGenerator::
GenerateSharedDestructorCode(io::Printer* printer) {
  printer->Print(
    "void $classname$::SharedDtor() {\n",
    "classname", classname_);
  printer->Indent();
  // Write the destructors for each field.
  for (int i = 0; i < descriptor_->field_count(); i++) {
    field_generators_.get(descriptor_->field(i))
                     .GenerateDestructorCode(printer);
  }

  printer->Print(
    "if (this != default_instance_) {\n");

  // We need to delete all embedded messages.
  // TODO(kenton):  If we make unset messages point at default instances
  //   instead of NULL, then it would make sense to move this code into
  //   MessageFieldGenerator::GenerateDestructorCode().
  for (int i = 0; i < descriptor_->field_count(); i++) {
    const FieldDescriptor* field = descriptor_->field(i);

    if (!field->is_repeated() &&
        field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      printer->Print("  delete $name$_;\n",
                     "name", FieldName(field));
    }
  }

  printer->Outdent();
  printer->Print(
    "  }\n"
    "}\n"
    "\n");
}

void MessageGenerator::
GenerateStructors(io::Printer* printer) {
  // Generate the default constructor.
  printer->Print(
      "$classname$::$classname$()\n"
      "  : ",
      "classname", classname_);
  GenerateInitializerList(printer);
  printer->Print(" {\n"
    "  SharedCtor();\n"
    "}\n");

  printer->Print(
    "\n"
    "void $classname$::InitAsDefaultInstance() {",
    "classname", classname_);

  // The default instance needs all of its embedded message pointers
  // cross-linked to other default instances.  We can't do this initialization
  // in the constructor because some other default instances may not have been
  // constructed yet at that time.
  // TODO(kenton):  Maybe all message fields (even for non-default messages)
  //   should be initialized to point at default instances rather than NULL?
  for (int i = 0; i < descriptor_->field_count(); i++) {
    const FieldDescriptor* field = descriptor_->field(i);

    if (!field->is_repeated() &&
        field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      printer->Print(
          "  $name$_ = const_cast< $type$*>(&$type$::default_instance());\n",
          "name", FieldName(field),
          "type", ClassName(field->message_type(), true));
    }
  }
  printer->Print(
    "}\n"
    "\n");

  // Generate the copy constructor.
  printer->Print(
      "$classname$::$classname$(const $classname$& from)\n"
      "  : ",
      "classname", classname_);
  GenerateInitializerList(printer);
  printer->Print(" {\n"
    "  SharedCtor();\n"
    "  MergeFrom(from);\n"
    "}\n"
    "\n");

  // Generate the shared constructor code.
  GenerateSharedConstructorCode(printer);

  // Generate the destructor.
  printer->Print(
    "$classname$::~$classname$() {\n"
    "  SharedDtor();\n"
    "}\n"
    "\n",
    "classname", classname_);

  // Generate the shared destructor code.
  GenerateSharedDestructorCode(printer);

  printer->Print(
    "const ::google::protobuf::Descriptor* $classname$::descriptor() {\n"
    "  protobuf_AssignDescriptorsOnce();\n"
    "  return $classname$_descriptor_;\n"
    "}\n"
    "\n"
    "const $classname$& $classname$::default_instance() {\n"
    "  if (default_instance_ == NULL) $adddescriptorsname$();"
    "  return *default_instance_;\n"
    "}\n"
    "\n"
    "$classname$* $classname$::default_instance_ = NULL;\n"
    "\n"
    "$classname$* $classname$::New() const {\n"
    "  return new $classname$;\n"
    "}\n",
    "classname", classname_,
    "adddescriptorsname",
    GlobalAddDescriptorsName(descriptor_->file()->name()));
}

void MessageGenerator::
GenerateClear(io::Printer* printer) {
  printer->Print("void $classname$::Clear() {\n",
                 "classname", classname_);
  printer->Indent();

  int last_index = -1;

  if (descriptor_->extension_range_count() > 0) {
    printer->Print("_extensions_.Clear();\n");
  }

  for (int i = 0; i < descriptor_->field_count(); i++) {
    const FieldDescriptor* field = descriptor_->field(i);

    if (!field->is_repeated()) {
      map<string, string> vars;
      vars["index"] = SimpleItoa(field->index());

      // We can use the fact that _has_bits_ is a giant bitfield to our
      // advantage:  We can check up to 32 bits at a time for equality to
      // zero, and skip the whole range if so.  This can improve the speed
      // of Clear() for messages which contain a very large number of
      // optional fields of which only a few are used at a time.  Here,
      // we've chosen to check 8 bits at a time rather than 32.
      if (i / 8 != last_index / 8 || last_index < 0) {
        if (last_index >= 0) {
          printer->Outdent();
          printer->Print("}\n");
        }
        printer->Print(vars,
          "if (_has_bits_[$index$ / 32] & (0xffu << ($index$ % 32))) {\n");
        printer->Indent();
      }
      last_index = i;

      // It's faster to just overwrite primitive types, but we should
      // only clear strings and messages if they were set.
      // TODO(kenton):  Let the CppFieldGenerator decide this somehow.
      bool should_check_bit =
        field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE ||
        field->cpp_type() == FieldDescriptor::CPPTYPE_STRING;

      if (should_check_bit) {
        printer->Print(vars, "if (_has_bit($index$)) {\n");
        printer->Indent();
      }

      field_generators_.get(field).GenerateClearingCode(printer);

      if (should_check_bit) {
        printer->Outdent();
        printer->Print("}\n");
      }
    }
  }

  if (last_index >= 0) {
    printer->Outdent();
    printer->Print("}\n");
  }

  // Repeated fields don't use _has_bits_ so we clear them in a separate
  // pass.
  for (int i = 0; i < descriptor_->field_count(); i++) {
    const FieldDescriptor* field = descriptor_->field(i);

    if (field->is_repeated()) {
      field_generators_.get(field).GenerateClearingCode(printer);
    }
  }

  printer->Print(
    "::memset(_has_bits_, 0, sizeof(_has_bits_));\n"
    "mutable_unknown_fields()->Clear();\n");

  printer->Outdent();
  printer->Print("}\n");
}

void MessageGenerator::
GenerateSwap(io::Printer* printer) {
  // Generate the Swap member function.
  printer->Print("void $classname$::Swap($classname$* other) {\n",
                 "classname", classname_);
  printer->Indent();
  printer->Print("if (other != this) {\n");
  printer->Indent();

  for (int i = 0; i < descriptor_->field_count(); i++) {
    const FieldDescriptor* field = descriptor_->field(i);
    field_generators_.get(field).GenerateSwappingCode(printer);
  }

  for (int i = 0; i < (descriptor_->field_count() + 31) / 32; ++i) {
    printer->Print("std::swap(_has_bits_[$i$], other->_has_bits_[$i$]);\n",
                   "i", SimpleItoa(i));
  }

  printer->Print("_unknown_fields_.Swap(&other->_unknown_fields_);\n");
  printer->Print("std::swap(_cached_size_, other->_cached_size_);\n");
  if (descriptor_->extension_range_count() > 0) {
    printer->Print("_extensions_.Swap(&other->_extensions_);\n");
  }

  printer->Outdent();
  printer->Print("}\n");
  printer->Outdent();
  printer->Print("}\n");
}

void MessageGenerator::
GenerateMergeFrom(io::Printer* printer) {
  // Generate the generalized MergeFrom (aka that which takes in the Message
  // base class as a parameter).
  printer->Print(
    "void $classname$::MergeFrom(const ::google::protobuf::Message& from) {\n"
    "  GOOGLE_CHECK_NE(&from, this);\n",
    "classname", classname_);
  printer->Indent();

  // Cast the message to the proper type. If we find that the message is
  // *not* of the proper type, we can still call Merge via the reflection
  // system, as the GOOGLE_CHECK above ensured that we have the same descriptor
  // for each message.
  printer->Print(
    "const $classname$* source =\n"
    "  ::google::protobuf::internal::dynamic_cast_if_available<const $classname$*>(\n"
    "    &from);\n"
    "if (source == NULL) {\n"
    "  ::google::protobuf::internal::ReflectionOps::Merge(from, this);\n"
    "} else {\n"
    "  MergeFrom(*source);\n"
    "}\n",
    "classname", classname_);

  printer->Outdent();
  printer->Print("}\n\n");

  // Generate the class-specific MergeFrom, which avoids the GOOGLE_CHECK and cast.
  printer->Print(
    "void $classname$::MergeFrom(const $classname$& from) {\n"
    "  GOOGLE_CHECK_NE(&from, this);\n",
    "classname", classname_);
  printer->Indent();

  // Merge Repeated fields. These fields do not require a
  // check as we can simply iterate over them.
  for (int i = 0; i < descriptor_->field_count(); ++i) {
    const FieldDescriptor* field = descriptor_->field(i);

    if (field->is_repeated()) {
      field_generators_.get(field).GenerateMergingCode(printer);
    }
  }

  // Merge Optional and Required fields (after a _has_bit check).
  int last_index = -1;

  for (int i = 0; i < descriptor_->field_count(); ++i) {
    const FieldDescriptor* field = descriptor_->field(i);

    if (!field->is_repeated()) {
      map<string, string> vars;
      vars["index"] = SimpleItoa(field->index());

      // See above in GenerateClear for an explanation of this.
      if (i / 8 != last_index / 8 || last_index < 0) {
        if (last_index >= 0) {
          printer->Outdent();
          printer->Print("}\n");
        }
        printer->Print(vars,
          "if (from._has_bits_[$index$ / 32] & (0xffu << ($index$ % 32))) {\n");
        printer->Indent();
      }

      last_index = i;

      printer->Print(vars,
        "if (from._has_bit($index$)) {\n");
      printer->Indent();

      field_generators_.get(field).GenerateMergingCode(printer);

      printer->Outdent();
      printer->Print("}\n");
    }
  }

  if (last_index >= 0) {
    printer->Outdent();
    printer->Print("}\n");
  }

  if (descriptor_->extension_range_count() > 0) {
    printer->Print("_extensions_.MergeFrom(from._extensions_);\n");
  }

  printer->Print(
    "mutable_unknown_fields()->MergeFrom(from.unknown_fields());\n");

  printer->Outdent();
  printer->Print("}\n");
}

void MessageGenerator::
GenerateCopyFrom(io::Printer* printer) {
  // Generate the generalized CopyFrom (aka that which takes in the Message
  // base class as a parameter).
  printer->Print(
    "void $classname$::CopyFrom(const ::google::protobuf::Message& from) {\n",
    "classname", classname_);
  printer->Indent();

  printer->Print(
    "if (&from == this) return;\n"
    "Clear();\n"
    "MergeFrom(from);\n");

  printer->Outdent();
  printer->Print("}\n\n");

  // Generate the class-specific CopyFrom.
  printer->Print(
    "void $classname$::CopyFrom(const $classname$& from) {\n",
    "classname", classname_);
  printer->Indent();

  printer->Print(
    "if (&from == this) return;\n"
    "Clear();\n"
    "MergeFrom(from);\n");

  printer->Outdent();
  printer->Print("}\n");
}

void MessageGenerator::
GenerateMergeFromCodedStream(io::Printer* printer) {
  printer->Print(
    "bool $classname$::MergePartialFromCodedStream(\n"
    "    ::google::protobuf::io::CodedInputStream* input) {\n"
    "#define DO_(EXPRESSION) if (!(EXPRESSION)) return false\n"
    "  ::google::protobuf::uint32 tag;\n"
    "  while ((tag = input->ReadTag()) != 0) {\n",
    "classname", classname_);

  printer->Indent();
  printer->Indent();

  if (descriptor_->field_count() > 0) {
    // We don't even want to print the switch() if we have no fields because
    // MSVC dislikes switch() statements that contain only a default value.

    // Note:  If we just switched on the tag rather than the field number, we
    // could avoid the need for the if() to check the wire type at the beginning
    // of each case.  However, this is actually a bit slower in practice as it
    // creates a jump table that is 8x larger and sparser, and meanwhile the
    // if()s are highly predictable.
    printer->Print(
      "switch (::google::protobuf::internal::WireFormat::GetTagFieldNumber(tag)) {\n");

    printer->Indent();

    scoped_array<const FieldDescriptor*> ordered_fields(
      SortFieldsByNumber(descriptor_));

    for (int i = 0; i < descriptor_->field_count(); i++) {
      const FieldDescriptor* field = ordered_fields[i];

      PrintFieldComment(printer, field);

      printer->Print(
        "case $number$: {\n"
        "  if (::google::protobuf::internal::WireFormat::GetTagWireType(tag) !=\n"
        "      ::google::protobuf::internal::WireFormat::WIRETYPE_$wiretype$) {\n"
        "    goto handle_uninterpreted;\n"
        "  }\n",
        "number", SimpleItoa(field->number()),
        "wiretype", kWireTypeNames[WireFormat::WireTypeForField(field)]);

      if (i > 0 || (field->is_repeated() && !field->options().packed())) {
        printer->Print(
          " parse_$name$:\n",
          "name", field->name());
      }

      printer->Indent();

      field_generators_.get(field).GenerateMergeFromCodedStream(printer);

      // switch() is slow since it can't be predicted well.  Insert some if()s
      // here that attempt to predict the next tag.
      if (field->is_repeated() && !field->options().packed()) {
        // Expect repeats of this field.
        printer->Print(
          "if (input->ExpectTag($tag$)) goto parse_$name$;\n",
          "tag", SimpleItoa(WireFormat::MakeTag(field)),
          "name", field->name());
      }

      if (i + 1 < descriptor_->field_count()) {
        // Expect the next field in order.
        const FieldDescriptor* next_field = ordered_fields[i + 1];
        printer->Print(
          "if (input->ExpectTag($next_tag$)) goto parse_$next_name$;\n",
          "next_tag", SimpleItoa(WireFormat::MakeTag(next_field)),
          "next_name", next_field->name());
      } else {
        // Expect EOF.
        // TODO(kenton):  Expect group end-tag?
        printer->Print(
          "if (input->ExpectAtEnd()) return true;\n");
      }

      printer->Print(
        "break;\n");

      printer->Outdent();
      printer->Print("}\n\n");
    }

    printer->Print(
      "default: {\n"
      "handle_uninterpreted:\n");
    printer->Indent();
  }

  // Is this an end-group tag?  If so, this must be the end of the message.
  printer->Print(
    "if (::google::protobuf::internal::WireFormat::GetTagWireType(tag) ==\n"
    "    ::google::protobuf::internal::WireFormat::WIRETYPE_END_GROUP) {\n"
    "  return true;\n"
    "}\n");

  // Handle extension ranges.
  if (descriptor_->extension_range_count() > 0) {
    printer->Print(
      "if (");
    for (int i = 0; i < descriptor_->extension_range_count(); i++) {
      const Descriptor::ExtensionRange* range =
        descriptor_->extension_range(i);
      if (i > 0) printer->Print(" ||\n    ");

      uint32 start_tag = WireFormat::MakeTag(
        range->start, static_cast<WireFormat::WireType>(0));
      uint32 end_tag = WireFormat::MakeTag(
        range->end, static_cast<WireFormat::WireType>(0));

      if (range->end > FieldDescriptor::kMaxNumber) {
        printer->Print(
          "($start$u <= tag)",
          "start", SimpleItoa(start_tag));
      } else {
        printer->Print(
          "($start$u <= tag && tag < $end$u)",
          "start", SimpleItoa(start_tag),
          "end", SimpleItoa(end_tag));
      }
    }
    printer->Print(") {\n"
      "  DO_(_extensions_.ParseField(tag, input, default_instance_,\n"
      "                              mutable_unknown_fields()));\n"
      "  continue;\n"
      "}\n");
  }

  // We really don't recognize this tag.  Skip it.
  printer->Print(
    "DO_(::google::protobuf::internal::WireFormat::SkipField(\n"
    "      input, tag, mutable_unknown_fields()));\n");

  if (descriptor_->field_count() > 0) {
    printer->Print("break;\n");
    printer->Outdent();
    printer->Print("}\n");    // default:
    printer->Outdent();
    printer->Print("}\n");    // switch
  }

  printer->Outdent();
  printer->Outdent();
  printer->Print(
    "  }\n"                   // while
    "  return true;\n"
    "#undef DO_\n"
    "}\n");
}

void MessageGenerator::GenerateSerializeOneField(
    io::Printer* printer, const FieldDescriptor* field, bool to_array) {
  PrintFieldComment(printer, field);

  if (!field->is_repeated()) {
    printer->Print(
      "if (_has_bit($index$)) {\n",
      "index", SimpleItoa(field->index()));
    printer->Indent();
  }

  if (to_array) {
    field_generators_.get(field).GenerateSerializeWithCachedSizesToArray(
        printer);
  } else {
    field_generators_.get(field).GenerateSerializeWithCachedSizes(printer);
  }

  if (!field->is_repeated()) {
    printer->Outdent();
    printer->Print("}\n");
  }
  printer->Print("\n");
}

void MessageGenerator::GenerateSerializeOneExtensionRange(
    io::Printer* printer, const Descriptor::ExtensionRange* range,
    bool to_array) {
  map<string, string> vars;
  vars["start"] = SimpleItoa(range->start);
  vars["end"] = SimpleItoa(range->end);
  printer->Print(vars,
    "// Extension range [$start$, $end$)\n");
  if (to_array) {
    printer->Print(vars,
      "target = _extensions_.SerializeWithCachedSizesToArray(\n"
      "    $start$, $end$, target);\n\n");
  } else {
    printer->Print(vars,
      "_extensions_.SerializeWithCachedSizes(\n"
      "    $start$, $end$, output);\n\n");
  }
}

void MessageGenerator::
GenerateSerializeWithCachedSizes(io::Printer* printer) {
  printer->Print(
    "void $classname$::SerializeWithCachedSizes(\n"
    "    ::google::protobuf::io::CodedOutputStream* output) const {\n",
    "classname", classname_);
  printer->Indent();

  printer->Print(
    "::google::protobuf::uint8* raw_buffer = "
      "output->GetDirectBufferForNBytesAndAdvance(_cached_size_);\n"
    "if (raw_buffer != NULL) {\n"
    "  $classname$::SerializeWithCachedSizesToArray(raw_buffer);\n"
    "  return;\n"
    "}\n"
    "\n",
    "classname", classname_);
  GenerateSerializeWithCachedSizesBody(printer, false);

  printer->Outdent();
  printer->Print(
    "}\n");
}

void MessageGenerator::
GenerateSerializeWithCachedSizesToArray(io::Printer* printer) {
  printer->Print(
    "::google::protobuf::uint8* $classname$::SerializeWithCachedSizesToArray(\n"
    "    ::google::protobuf::uint8* target) const {\n",
    "classname", classname_);
  printer->Indent();

  GenerateSerializeWithCachedSizesBody(printer, true);

  printer->Outdent();
  printer->Print(
    "  return target;\n"
    "}\n");
}

void MessageGenerator::
GenerateSerializeWithCachedSizesBody(io::Printer* printer, bool to_array) {
  scoped_array<const FieldDescriptor*> ordered_fields(
    SortFieldsByNumber(descriptor_));

  vector<const Descriptor::ExtensionRange*> sorted_extensions;
  for (int i = 0; i < descriptor_->extension_range_count(); ++i) {
    sorted_extensions.push_back(descriptor_->extension_range(i));
  }
  sort(sorted_extensions.begin(), sorted_extensions.end(),
       ExtensionRangeSorter());

  // Merge the fields and the extension ranges, both sorted by field number.
  int i, j;
  for (i = 0, j = 0;
       i < descriptor_->field_count() || j < sorted_extensions.size();
       ) {
    if (i == descriptor_->field_count()) {
      GenerateSerializeOneExtensionRange(printer,
                                         sorted_extensions[j++],
                                         to_array);
    } else if (j == sorted_extensions.size()) {
      GenerateSerializeOneField(printer, ordered_fields[i++], to_array);
    } else if (ordered_fields[i]->number() < sorted_extensions[j]->start) {
      GenerateSerializeOneField(printer, ordered_fields[i++], to_array);
    } else {
      GenerateSerializeOneExtensionRange(printer,
                                         sorted_extensions[j++],
                                         to_array);
    }
  }

  printer->Print("if (!unknown_fields().empty()) {\n");
  printer->Indent();
  if (to_array) {
    printer->Print(
      "target = "
          "::google::protobuf::internal::WireFormat::SerializeUnknownFieldsToArray(\n"
      "    unknown_fields(), target);\n");
  } else {
    printer->Print(
      "::google::protobuf::internal::WireFormat::SerializeUnknownFields(\n"
      "    unknown_fields(), output);\n");
  }
  printer->Outdent();

  printer->Print(
    "}\n");
}

void MessageGenerator::
GenerateByteSize(io::Printer* printer) {
  printer->Print(
    "int $classname$::ByteSize() const {\n",
    "classname", classname_);
  printer->Indent();
  printer->Print(
    "int total_size = 0;\n"
    "\n");

  int last_index = -1;

  for (int i = 0; i < descriptor_->field_count(); i++) {
    const FieldDescriptor* field = descriptor_->field(i);

    if (!field->is_repeated()) {
      // See above in GenerateClear for an explanation of this.
      // TODO(kenton):  Share code?  Unclear how to do so without
      //   over-engineering.
      if ((i / 8) != (last_index / 8) ||
          last_index < 0) {
        if (last_index >= 0) {
          printer->Outdent();
          printer->Print("}\n");
        }
        printer->Print(
          "if (_has_bits_[$index$ / 32] & (0xffu << ($index$ % 32))) {\n",
          "index", SimpleItoa(field->index()));
        printer->Indent();
      }
      last_index = i;

      PrintFieldComment(printer, field);

      printer->Print(
        "if (has_$name$()) {\n",
        "name", FieldName(field));
      printer->Indent();

      field_generators_.get(field).GenerateByteSize(printer);

      printer->Outdent();
      printer->Print(
        "}\n"
        "\n");
    }
  }

  if (last_index >= 0) {
    printer->Outdent();
    printer->Print("}\n");
  }

  // Repeated fields don't use _has_bits_ so we count them in a separate
  // pass.
  for (int i = 0; i < descriptor_->field_count(); i++) {
    const FieldDescriptor* field = descriptor_->field(i);

    if (field->is_repeated()) {
      PrintFieldComment(printer, field);
      field_generators_.get(field).GenerateByteSize(printer);
      printer->Print("\n");
    }
  }

  if (descriptor_->extension_range_count() > 0) {
    printer->Print(
      "total_size += _extensions_.ByteSize();\n"
      "\n");
  }

  printer->Print("if (!unknown_fields().empty()) {\n");
  printer->Indent();
  printer->Print(
    "total_size +=\n"
    "  ::google::protobuf::internal::WireFormat::ComputeUnknownFieldsSize(\n"
    "    unknown_fields());\n");
  printer->Outdent();
  printer->Print("}\n");

  // We update _cached_size_ even though this is a const method.  In theory,
  // this is not thread-compatible, because concurrent writes have undefined
  // results.  In practice, since any concurrent writes will be writing the
  // exact same value, it works on all common processors.  In a future version
  // of C++, _cached_size_ should be made into an atomic<int>.
  printer->Print(
    "_cached_size_ = total_size;\n"
    "return total_size;\n");

  printer->Outdent();
  printer->Print("}\n");
}

void MessageGenerator::
GenerateIsInitialized(io::Printer* printer) {
  printer->Print(
    "bool $classname$::IsInitialized() const {\n",
    "classname", classname_);
  printer->Indent();

  // Check that all required fields in this message are set.  We can do this
  // most efficiently by checking 32 "has bits" at a time.
  int has_bits_array_size = (descriptor_->field_count() + 31) / 32;
  for (int i = 0; i < has_bits_array_size; i++) {
    uint32 mask = 0;
    for (int bit = 0; bit < 32; bit++) {
      int index = i * 32 + bit;
      if (index >= descriptor_->field_count()) break;
      const FieldDescriptor* field = descriptor_->field(index);

      if (field->is_required()) {
        mask |= 1 << bit;
      }
    }

    if (mask != 0) {
      char buffer[kFastToBufferSize];
      printer->Print(
        "if ((_has_bits_[$i$] & 0x$mask$) != 0x$mask$) return false;\n",
        "i", SimpleItoa(i),
        "mask", FastHex32ToBuffer(mask, buffer));
    }
  }

  // Now check that all embedded messages are initialized.
  printer->Print("\n");
  for (int i = 0; i < descriptor_->field_count(); i++) {
    const FieldDescriptor* field = descriptor_->field(i);
    if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE &&
        HasRequiredFields(field->message_type())) {
      if (field->is_repeated()) {
        printer->Print(
          "for (int i = 0; i < $name$_size(); i++) {\n"
          "  if (!this->$name$(i).IsInitialized()) return false;\n"
          "}\n",
          "name", FieldName(field));
      } else {
        printer->Print(
          "if (has_$name$()) {\n"
          "  if (!this->$name$().IsInitialized()) return false;\n"
          "}\n",
          "name", FieldName(field));
      }
    }
  }

  if (descriptor_->extension_range_count() > 0) {
    printer->Print(
      "\n"
      "if (!_extensions_.IsInitialized()) return false;");
  }

  printer->Outdent();
  printer->Print(
    "  return true;\n"
    "}\n");
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
