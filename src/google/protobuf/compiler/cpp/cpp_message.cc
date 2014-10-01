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

#include <algorithm>
#include <google/protobuf/stubs/hash.h>
#include <map>
#include <memory>
#include <set>
#include <utility>
#include <vector>
#include <google/protobuf/compiler/cpp/cpp_message.h>
#include <google/protobuf/compiler/cpp/cpp_field.h>
#include <google/protobuf/compiler/cpp/cpp_enum.h>
#include <google/protobuf/compiler/cpp/cpp_extension.h>
#include <google/protobuf/compiler/cpp/cpp_helpers.h>
#include <google/protobuf/stubs/strutil.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/wire_format.h>
#include <google/protobuf/descriptor.pb.h>


namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

using internal::WireFormat;
using internal::WireFormatLite;

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

// Returns true if the "required" restriction check should be ignored for the
// given field.
inline static bool ShouldIgnoreRequiredFieldCheck(
    const FieldDescriptor* field) {
  return false;
}

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
    if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE &&
        !ShouldIgnoreRequiredFieldCheck(field)) {
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

// This returns an estimate of the compiler's alignment for the field.  This
// can't guarantee to be correct because the generated code could be compiled on
// different systems with different alignment rules.  The estimates below assume
// 64-bit pointers.
int EstimateAlignmentSize(const FieldDescriptor* field) {
  if (field == NULL) return 0;
  if (field->is_repeated()) return 8;
  switch (field->cpp_type()) {
    case FieldDescriptor::CPPTYPE_BOOL:
      return 1;

    case FieldDescriptor::CPPTYPE_INT32:
    case FieldDescriptor::CPPTYPE_UINT32:
    case FieldDescriptor::CPPTYPE_ENUM:
    case FieldDescriptor::CPPTYPE_FLOAT:
      return 4;

    case FieldDescriptor::CPPTYPE_INT64:
    case FieldDescriptor::CPPTYPE_UINT64:
    case FieldDescriptor::CPPTYPE_DOUBLE:
    case FieldDescriptor::CPPTYPE_STRING:
    case FieldDescriptor::CPPTYPE_MESSAGE:
      return 8;
  }
  GOOGLE_LOG(FATAL) << "Can't get here.";
  return -1;  // Make compiler happy.
}

// FieldGroup is just a helper for OptimizePadding below.  It holds a vector of
// fields that are grouped together because they have compatible alignment, and
// a preferred location in the final field ordering.
class FieldGroup {
 public:
  FieldGroup()
      : preferred_location_(0) {}

  // A group with a single field.
  FieldGroup(float preferred_location, const FieldDescriptor* field)
      : preferred_location_(preferred_location),
        fields_(1, field) {}

  // Append the fields in 'other' to this group.
  void Append(const FieldGroup& other) {
    if (other.fields_.empty()) {
      return;
    }
    // Preferred location is the average among all the fields, so we weight by
    // the number of fields on each FieldGroup object.
    preferred_location_ =
        (preferred_location_ * fields_.size() +
         (other.preferred_location_ * other.fields_.size())) /
        (fields_.size() + other.fields_.size());
    fields_.insert(fields_.end(), other.fields_.begin(), other.fields_.end());
  }

  void SetPreferredLocation(float location) { preferred_location_ = location; }
  const vector<const FieldDescriptor*>& fields() const { return fields_; }

  // FieldGroup objects sort by their preferred location.
  bool operator<(const FieldGroup& other) const {
    return preferred_location_ < other.preferred_location_;
  }

 private:
  // "preferred_location_" is an estimate of where this group should go in the
  // final list of fields.  We compute this by taking the average index of each
  // field in this group in the original ordering of fields.  This is very
  // approximate, but should put this group close to where its member fields
  // originally went.
  float preferred_location_;
  vector<const FieldDescriptor*> fields_;
  // We rely on the default copy constructor and operator= so this type can be
  // used in a vector.
};

// Reorder 'fields' so that if the fields are output into a c++ class in the new
// order, the alignment padding is minimized.  We try to do this while keeping
// each field as close as possible to its original position so that we don't
// reduce cache locality much for function that access each field in order.
void OptimizePadding(vector<const FieldDescriptor*>* fields) {
  // First divide fields into those that align to 1 byte, 4 bytes or 8 bytes.
  vector<FieldGroup> aligned_to_1, aligned_to_4, aligned_to_8;
  for (int i = 0; i < fields->size(); ++i) {
    switch (EstimateAlignmentSize((*fields)[i])) {
      case 1: aligned_to_1.push_back(FieldGroup(i, (*fields)[i])); break;
      case 4: aligned_to_4.push_back(FieldGroup(i, (*fields)[i])); break;
      case 8: aligned_to_8.push_back(FieldGroup(i, (*fields)[i])); break;
      default:
        GOOGLE_LOG(FATAL) << "Unknown alignment size.";
    }
  }

  // Now group fields aligned to 1 byte into sets of 4, and treat those like a
  // single field aligned to 4 bytes.
  for (int i = 0; i < aligned_to_1.size(); i += 4) {
    FieldGroup field_group;
    for (int j = i; j < aligned_to_1.size() && j < i + 4; ++j) {
      field_group.Append(aligned_to_1[j]);
    }
    aligned_to_4.push_back(field_group);
  }
  // Sort by preferred location to keep fields as close to their original
  // location as possible.  Using stable_sort ensures that the output is
  // consistent across runs.
  stable_sort(aligned_to_4.begin(), aligned_to_4.end());

  // Now group fields aligned to 4 bytes (or the 4-field groups created above)
  // into pairs, and treat those like a single field aligned to 8 bytes.
  for (int i = 0; i < aligned_to_4.size(); i += 2) {
    FieldGroup field_group;
    for (int j = i; j < aligned_to_4.size() && j < i + 2; ++j) {
      field_group.Append(aligned_to_4[j]);
    }
    if (i == aligned_to_4.size() - 1) {
      // Move incomplete 4-byte block to the end.
      field_group.SetPreferredLocation(fields->size() + 1);
    }
    aligned_to_8.push_back(field_group);
  }
  // Sort by preferred location.
  stable_sort(aligned_to_8.begin(), aligned_to_8.end());

  // Now pull out all the FieldDescriptors in order.
  fields->clear();
  for (int i = 0; i < aligned_to_8.size(); ++i) {
    fields->insert(fields->end(),
                   aligned_to_8[i].fields().begin(),
                   aligned_to_8[i].fields().end());
  }
}

string MessageTypeProtoName(const FieldDescriptor* field) {
  return field->message_type()->full_name();
}

}

// ===================================================================

MessageGenerator::MessageGenerator(const Descriptor* descriptor,
                                   const Options& options)
    : descriptor_(descriptor),
      classname_(ClassName(descriptor, false)),
      options_(options),
      field_generators_(descriptor, options),
      nested_generators_(new scoped_ptr<
          MessageGenerator>[descriptor->nested_type_count()]),
      enum_generators_(
          new scoped_ptr<EnumGenerator>[descriptor->enum_type_count()]),
      extension_generators_(new scoped_ptr<
          ExtensionGenerator>[descriptor->extension_count()]) {

  for (int i = 0; i < descriptor->nested_type_count(); i++) {
    nested_generators_[i].reset(
      new MessageGenerator(descriptor->nested_type(i), options));
  }

  for (int i = 0; i < descriptor->enum_type_count(); i++) {
    enum_generators_[i].reset(
      new EnumGenerator(descriptor->enum_type(i), options));
  }

  for (int i = 0; i < descriptor->extension_count(); i++) {
    extension_generators_[i].reset(
      new ExtensionGenerator(descriptor->extension(i), options));
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
GenerateGetEnumDescriptorSpecializations(io::Printer* printer) {
  for (int i = 0; i < descriptor_->nested_type_count(); i++) {
    nested_generators_[i]->GenerateGetEnumDescriptorSpecializations(printer);
  }
  for (int i = 0; i < descriptor_->enum_type_count(); i++) {
    enum_generators_[i]->GenerateGetEnumDescriptorSpecializations(printer);
  }
}

void MessageGenerator::
GenerateFieldAccessorDeclarations(io::Printer* printer) {
  for (int i = 0; i < descriptor_->field_count(); i++) {
    const FieldDescriptor* field = descriptor_->field(i);

    PrintFieldComment(printer, field);

    map<string, string> vars;
    SetCommonFieldVariables(field, &vars, options_);
    vars["constant_name"] = FieldConstantName(field);

    if (field->is_repeated()) {
      printer->Print(vars, "inline int $name$_size() const$deprecation$;\n");
    } else {
      printer->Print(vars, "inline bool has_$name$() const$deprecation$;\n");
    }

    printer->Print(vars, "inline void clear_$name$()$deprecation$;\n");
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

  for (int i = 0; i < descriptor_->oneof_decl_count(); i++) {
    printer->Print(
        "inline $camel_oneof_name$Case $oneof_name$_case() const;\n",
        "camel_oneof_name",
        UnderscoresToCamelCase(descriptor_->oneof_decl(i)->name(), true),
        "oneof_name", descriptor_->oneof_decl(i)->name());
  }
}

void MessageGenerator::
GenerateFieldAccessorDefinitions(io::Printer* printer) {
  printer->Print("// $classname$\n\n", "classname", classname_);

  for (int i = 0; i < descriptor_->field_count(); i++) {
    const FieldDescriptor* field = descriptor_->field(i);

    PrintFieldComment(printer, field);

    map<string, string> vars;
    SetCommonFieldVariables(field, &vars, options_);

    // Generate has_$name$() or $name$_size().
    if (field->is_repeated()) {
      printer->Print(vars,
        "inline int $classname$::$name$_size() const {\n"
        "  return $name$_.size();\n"
        "}\n");
    } else if (field->containing_oneof()) {
      // Singular field in a oneof
      vars["field_name"] = UnderscoresToCamelCase(field->name(), true);
      vars["oneof_name"] = field->containing_oneof()->name();
      vars["oneof_index"] = SimpleItoa(field->containing_oneof()->index());
      printer->Print(vars,
        "inline bool $classname$::has_$name$() const {\n"
        "  return $oneof_name$_case() == k$field_name$;\n"
        "}\n"
        "inline void $classname$::set_has_$name$() {\n"
        "  _oneof_case_[$oneof_index$] = k$field_name$;\n"
        "}\n");
    } else {
      // Singular field.
      char buffer[kFastToBufferSize];
      vars["has_array_index"] = SimpleItoa(field->index() / 32);
      vars["has_mask"] = FastHex32ToBuffer(1u << (field->index() % 32), buffer);
      printer->Print(vars,
        "inline bool $classname$::has_$name$() const {\n"
        "  return (_has_bits_[$has_array_index$] & 0x$has_mask$u) != 0;\n"
        "}\n"
        "inline void $classname$::set_has_$name$() {\n"
        "  _has_bits_[$has_array_index$] |= 0x$has_mask$u;\n"
        "}\n"
        "inline void $classname$::clear_has_$name$() {\n"
        "  _has_bits_[$has_array_index$] &= ~0x$has_mask$u;\n"
        "}\n"
        );
    }

    // Generate clear_$name$()
    printer->Print(vars,
      "inline void $classname$::clear_$name$() {\n");

    printer->Indent();

    if (field->containing_oneof()) {
      // Clear this field only if it is the active field in this oneof,
      // otherwise ignore
      printer->Print(vars,
        "if (has_$name$()) {\n");
      printer->Indent();
      field_generators_.get(field).GenerateClearingCode(printer);
      printer->Print(vars,
        "clear_has_$oneof_name$();\n");
      printer->Outdent();
      printer->Print("}\n");
    } else {
      field_generators_.get(field).GenerateClearingCode(printer);
      if (!field->is_repeated()) {
        printer->Print(vars,
                       "clear_has_$name$();\n");
      }
    }

    printer->Outdent();
    printer->Print("}\n");

    // Generate type-specific accessors.
    field_generators_.get(field).GenerateInlineAccessorDefinitions(printer);

    printer->Print("\n");
  }

  // Generate has_$name$() and clear_has_$name$() functions for oneofs
  for (int i = 0; i < descriptor_->oneof_decl_count(); i++) {
    map<string, string> vars;
    vars["oneof_name"] = descriptor_->oneof_decl(i)->name();
    vars["oneof_index"] = SimpleItoa(descriptor_->oneof_decl(i)->index());
    vars["cap_oneof_name"] =
        ToUpper(descriptor_->oneof_decl(i)->name());
    vars["classname"] = classname_;
    printer->Print(
      vars,
      "inline bool $classname$::has_$oneof_name$() {\n"
      "  return $oneof_name$_case() != $cap_oneof_name$_NOT_SET;\n"
      "}\n"
      "inline void $classname$::clear_has_$oneof_name$() {\n"
      "  _oneof_case_[$oneof_index$] = $cap_oneof_name$_NOT_SET;\n"
      "}\n");
  }
}

// Helper for the code that emits the Clear() method.
static bool CanClearByZeroing(const FieldDescriptor* field) {
  if (field->is_repeated() || field->is_extension()) return false;
  switch (field->cpp_type()) {
    case internal::WireFormatLite::CPPTYPE_ENUM:
      return field->default_value_enum()->number() == 0;
    case internal::WireFormatLite::CPPTYPE_INT32:
      return field->default_value_int32() == 0;
    case internal::WireFormatLite::CPPTYPE_INT64:
      return field->default_value_int64() == 0;
    case internal::WireFormatLite::CPPTYPE_UINT32:
      return field->default_value_uint32() == 0;
    case internal::WireFormatLite::CPPTYPE_UINT64:
      return field->default_value_uint64() == 0;
    case internal::WireFormatLite::CPPTYPE_FLOAT:
      return field->default_value_float() == 0;
    case internal::WireFormatLite::CPPTYPE_DOUBLE:
      return field->default_value_double() == 0;
    case internal::WireFormatLite::CPPTYPE_BOOL:
      return field->default_value_bool() == false;
    default:
      return false;
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
  vars["oneof_decl_count"] = SimpleItoa(descriptor_->oneof_decl_count());
  if (options_.dllexport_decl.empty()) {
    vars["dllexport"] = "";
  } else {
    vars["dllexport"] = options_.dllexport_decl + " ";
  }
  vars["superclass"] = SuperClassName(descriptor_);

  printer->Print(vars,
    "class $dllexport$$classname$ : public $superclass$ {\n"
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
    "\n");

  if (UseUnknownFieldSet(descriptor_->file())) {
    printer->Print(
      "inline const ::google::protobuf::UnknownFieldSet& unknown_fields() const {\n"
      "  return _unknown_fields_;\n"
      "}\n"
      "\n"
      "inline ::google::protobuf::UnknownFieldSet* mutable_unknown_fields() {\n"
      "  return &_unknown_fields_;\n"
      "}\n"
      "\n");
  } else {
    printer->Print(
      "inline const ::std::string& unknown_fields() const {\n"
      "  return _unknown_fields_;\n"
      "}\n"
      "\n"
      "inline ::std::string* mutable_unknown_fields() {\n"
      "  return &_unknown_fields_;\n"
      "}\n"
      "\n");
  }

  // Only generate this member if it's not disabled.
  if (HasDescriptorMethods(descriptor_->file()) &&
      !descriptor_->options().no_standard_descriptor_accessor()) {
    printer->Print(vars,
      "static const ::google::protobuf::Descriptor* descriptor();\n");
  }

  printer->Print(vars,
    "static const $classname$& default_instance();\n"
    "\n");

  // Generate enum values for every field in oneofs. One list is generated for
  // each oneof with an additional *_NOT_SET value.
  for (int i = 0; i < descriptor_->oneof_decl_count(); i++) {
    printer->Print(
        "enum $camel_oneof_name$Case {\n",
        "camel_oneof_name",
        UnderscoresToCamelCase(descriptor_->oneof_decl(i)->name(), true));
    printer->Indent();
    for (int j = 0; j < descriptor_->oneof_decl(i)->field_count(); j++) {
      printer->Print(
          "k$field_name$ = $field_number$,\n",
          "field_name",
          UnderscoresToCamelCase(
              descriptor_->oneof_decl(i)->field(j)->name(), true),
          "field_number",
          SimpleItoa(descriptor_->oneof_decl(i)->field(j)->number()));
    }
    printer->Print(
        "$cap_oneof_name$_NOT_SET = 0,\n",
        "cap_oneof_name",
        ToUpper(descriptor_->oneof_decl(i)->name()));
    printer->Outdent();
    printer->Print(
        "};\n"
        "\n");
  }

  if (!StaticInitializersForced(descriptor_->file())) {
    printer->Print(vars,
      "#ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER\n"
      "// Returns the internal default instance pointer. This function can\n"
      "// return NULL thus should not be used by the user. This is intended\n"
      "// for Protobuf internal code. Please use default_instance() declared\n"
      "// above instead.\n"
      "static inline const $classname$* internal_default_instance() {\n"
      "  return default_instance_;\n"
      "}\n"
      "#endif\n"
      "\n");
  }


  printer->Print(vars,
    "void Swap($classname$* other);\n"
    "\n"
    "// implements Message ----------------------------------------------\n"
    "\n"
    "$classname$* New() const;\n");

  if (HasGeneratedMethods(descriptor_->file())) {
    if (HasDescriptorMethods(descriptor_->file())) {
      printer->Print(vars,
        "void CopyFrom(const ::google::protobuf::Message& from);\n"
        "void MergeFrom(const ::google::protobuf::Message& from);\n");
    } else {
      printer->Print(vars,
        "void CheckTypeAndMergeFrom(const ::google::protobuf::MessageLite& from);\n");
    }

    printer->Print(vars,
      "void CopyFrom(const $classname$& from);\n"
      "void MergeFrom(const $classname$& from);\n"
      "void Clear();\n"
      "bool IsInitialized() const;\n"
      "\n"
      "int ByteSize() const;\n"
      "bool MergePartialFromCodedStream(\n"
      "    ::google::protobuf::io::CodedInputStream* input);\n"
      "void SerializeWithCachedSizes(\n"
      "    ::google::protobuf::io::CodedOutputStream* output) const;\n");
    // DiscardUnknownFields() is implemented in message.cc using reflections. We
    // need to implement this function in generated code for messages.
    if (!UseUnknownFieldSet(descriptor_->file())) {
      printer->Print(
        "void DiscardUnknownFields();\n");
    }
    if (HasFastArraySerialization(descriptor_->file())) {
      printer->Print(
        "::google::protobuf::uint8* SerializeWithCachedSizesToArray(::google::protobuf::uint8* output) const;\n");
    }
  }

  // Check all FieldDescriptors including those in oneofs to estimate
  // whether ::std::string is likely to be used, and depending on that
  // estimate, set uses_string_ to true or false.  That contols
  // whether to force initialization of empty_string_ in SharedCtor().
  // It's often advantageous to do so to keep "is empty_string_
  // inited?" code from appearing all over the place.
  vector<const FieldDescriptor*> descriptors;
  for (int i = 0; i < descriptor_->field_count(); i++) {
    descriptors.push_back(descriptor_->field(i));
  }
  for (int i = 0; i < descriptor_->oneof_decl_count(); i++) {
    for (int j = 0; j < descriptor_->oneof_decl(i)->field_count(); j++) {
      descriptors.push_back(descriptor_->oneof_decl(i)->field(j));
    }
  }
  uses_string_ = false;
  for (int i = 0; i < descriptors.size(); i++) {
    const FieldDescriptor* field = descriptors[i];
    if (field->cpp_type() == FieldDescriptor::CPPTYPE_STRING) {
      switch (field->options().ctype()) {
        default: uses_string_ = true; break;
      }
    }
  }

  printer->Print(
    "int GetCachedSize() const { return _cached_size_; }\n"
    "private:\n"
    "void SharedCtor();\n"
    "void SharedDtor();\n"
    "void SetCachedSize(int size) const;\n"
    "public:\n");

  if (HasDescriptorMethods(descriptor_->file())) {
    printer->Print(
      "::google::protobuf::Metadata GetMetadata() const;\n"
      "\n");
  } else {
    printer->Print(
      "::std::string GetTypeName() const;\n"
      "\n");
  }

  printer->Print(
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


  printer->Print(
    "// @@protoc_insertion_point(class_scope:$full_name$)\n",
    "full_name", descriptor_->full_name());

  // Generate private members.
  printer->Outdent();
  printer->Print(" private:\n");
  printer->Indent();


  for (int i = 0; i < descriptor_->field_count(); i++) {
    if (!descriptor_->field(i)->is_repeated()) {
      printer->Print(
        "inline void set_has_$name$();\n",
        "name", FieldName(descriptor_->field(i)));
      if (!descriptor_->field(i)->containing_oneof()) {
        printer->Print(
          "inline void clear_has_$name$();\n",
          "name", FieldName(descriptor_->field(i)));
      }
    }
  }
  printer->Print("\n");

  // Generate oneof function declarations
  for (int i = 0; i < descriptor_->oneof_decl_count(); i++) {
    printer->Print(
        "inline bool has_$oneof_name$();\n"
        "void clear_$oneof_name$();\n"
        "inline void clear_has_$oneof_name$();\n\n",
        "oneof_name", descriptor_->oneof_decl(i)->name());
  }

  // Prepare decls for _cached_size_ and _has_bits_.  Their position in the
  // output will be determined later.

  bool need_to_emit_cached_size = true;
  // TODO(kenton):  Make _cached_size_ an atomic<int> when C++ supports it.
  const string cached_size_decl = "mutable int _cached_size_;\n";

  // TODO(jieluo) - Optimize _has_bits_ for repeated and oneof fields.
  size_t sizeof_has_bits = (descriptor_->field_count() + 31) / 32 * 4;
  if (descriptor_->field_count() == 0) {
    // Zero-size arrays aren't technically allowed, and MSVC in particular
    // doesn't like them.  We still need to declare these arrays to make
    // other code compile.  Since this is an uncommon case, we'll just declare
    // them with size 1 and waste some space.  Oh well.
    sizeof_has_bits = 4;
  }
  const string has_bits_decl = sizeof_has_bits == 0 ? "" :
      "::google::protobuf::uint32 _has_bits_[" + SimpleItoa(sizeof_has_bits / 4) + "];\n";


  // To minimize padding, data members are divided into three sections:
  // (1) members assumed to align to 8 bytes
  // (2) members corresponding to message fields, re-ordered to optimize
  //     alignment.
  // (3) members assumed to align to 4 bytes.

  // Members assumed to align to 8 bytes:

  if (descriptor_->extension_range_count() > 0) {
    printer->Print(
      "::google::protobuf::internal::ExtensionSet _extensions_;\n"
      "\n");
  }

  if (UseUnknownFieldSet(descriptor_->file())) {
    printer->Print(
      "::google::protobuf::UnknownFieldSet _unknown_fields_;\n"
      "\n");
  } else {
    printer->Print(
      "::std::string _unknown_fields_;\n"
      "\n");
  }

  // _has_bits_ is frequently accessed, so to reduce code size and improve
  // speed, it should be close to the start of the object.  But, try not to
  // waste space:_has_bits_ by itself always makes sense if its size is a
  // multiple of 8, but, otherwise, maybe _has_bits_ and cached_size_ together
  // will work well.
  printer->Print(has_bits_decl.c_str());
  if ((sizeof_has_bits % 8) != 0) {
    printer->Print(cached_size_decl.c_str());
    need_to_emit_cached_size = false;
  }

  // Field members:

  // List fields which doesn't belong to any oneof
  vector<const FieldDescriptor*> fields;
  hash_map<string, int> fieldname_to_chunk;
  for (int i = 0; i < descriptor_->field_count(); i++) {
    if (!descriptor_->field(i)->containing_oneof()) {
      const FieldDescriptor* field = descriptor_->field(i);
      fields.push_back(field);
      fieldname_to_chunk[FieldName(field)] = i / 8;
    }
  }
  OptimizePadding(&fields);
  // Emit some private and static members
  runs_of_fields_ = vector< vector<string> >(1);
  for (int i = 0; i < fields.size(); ++i) {
    const FieldDescriptor* field = fields[i];
    const FieldGenerator& generator = field_generators_.get(field);
    generator.GenerateStaticMembers(printer);
    generator.GeneratePrivateMembers(printer);
    if (CanClearByZeroing(field)) {
      const string& fieldname = FieldName(field);
      if (!runs_of_fields_.back().empty() &&
          (fieldname_to_chunk[runs_of_fields_.back().back()] !=
           fieldname_to_chunk[fieldname])) {
        runs_of_fields_.push_back(vector<string>());
      }
      runs_of_fields_.back().push_back(fieldname);
    } else if (!runs_of_fields_.back().empty()) {
      runs_of_fields_.push_back(vector<string>());
    }
  }

  // For each oneof generate a union
  for (int i = 0; i < descriptor_->oneof_decl_count(); i++) {
    printer->Print(
        "union $camel_oneof_name$Union {\n",
        "camel_oneof_name",
        UnderscoresToCamelCase(descriptor_->oneof_decl(i)->name(), true));
    printer->Indent();
    for (int j = 0; j < descriptor_->oneof_decl(i)->field_count(); j++) {
      field_generators_.get(descriptor_->oneof_decl(i)->
                            field(j)).GeneratePrivateMembers(printer);
    }
    printer->Outdent();
    printer->Print(
        "} $oneof_name$_;\n",
        "oneof_name", descriptor_->oneof_decl(i)->name());
    for (int j = 0; j < descriptor_->oneof_decl(i)->field_count(); j++) {
      field_generators_.get(descriptor_->oneof_decl(i)->
                            field(j)).GenerateStaticMembers(printer);
    }
  }

  // Members assumed to align to 4 bytes:

  if (need_to_emit_cached_size) {
    printer->Print(cached_size_decl.c_str());
    need_to_emit_cached_size = false;
  }

  // Generate _oneof_case_.
  if (descriptor_->oneof_decl_count() > 0) {
    printer->Print(vars,
      "::google::protobuf::uint32 _oneof_case_[$oneof_decl_count$];\n"
      "\n");
  }

  // Declare AddDescriptors(), BuildDescriptors(), and ShutdownFile() as
  // friends so that they can access private static variables like
  // default_instance_ and reflection_.
  PrintHandlingOptionalStaticInitializers(
    descriptor_->file(), printer,
    // With static initializers.
    "friend void $dllexport_decl$ $adddescriptorsname$();\n",
    // Without.
    "friend void $dllexport_decl$ $adddescriptorsname$_impl();\n",
    // Vars.
    "dllexport_decl", options_.dllexport_decl,
    "adddescriptorsname",
    GlobalAddDescriptorsName(descriptor_->file()->name()));

  printer->Print(
    "friend void $assigndescriptorsname$();\n"
    "friend void $shutdownfilename$();\n"
    "\n",
    "assigndescriptorsname",
      GlobalAssignDescriptorsName(descriptor_->file()->name()),
    "shutdownfilename", GlobalShutdownFileName(descriptor_->file()->name()));

  printer->Print(
    "void InitAsDefaultInstance();\n"
    "static $classname$* default_instance_;\n",
    "classname", classname_);

  printer->Outdent();
  printer->Print(vars, "};");
  GOOGLE_DCHECK(!need_to_emit_cached_size);
}

void MessageGenerator::
GenerateInlineMethods(io::Printer* printer) {
  for (int i = 0; i < descriptor_->nested_type_count(); i++) {
    nested_generators_[i]->GenerateInlineMethods(printer);
    printer->Print(kThinSeparator);
    printer->Print("\n");
  }

  GenerateFieldAccessorDefinitions(printer);

  // Generate oneof_case() functions.
  for (int i = 0; i < descriptor_->oneof_decl_count(); i++) {
    map<string, string> vars;
    vars["class_name"] = classname_;
    vars["camel_oneof_name"] = UnderscoresToCamelCase(
        descriptor_->oneof_decl(i)->name(), true);
    vars["oneof_name"] = descriptor_->oneof_decl(i)->name();
    vars["oneof_index"] = SimpleItoa(descriptor_->oneof_decl(i)->index());
    printer->Print(
        vars,
        "inline $class_name$::$camel_oneof_name$Case $class_name$::"
        "$oneof_name$_case() const {\n"
        "  return $class_name$::$camel_oneof_name$Case("
        "_oneof_case_[$oneof_index$]);\n"
        "}\n");
  }
}

void MessageGenerator::
GenerateDescriptorDeclarations(io::Printer* printer) {
  printer->Print(
    "const ::google::protobuf::Descriptor* $name$_descriptor_ = NULL;\n"
    "const ::google::protobuf::internal::GeneratedMessageReflection*\n"
    "  $name$_reflection_ = NULL;\n",
    "name", classname_);

  // Generate oneof default instance for reflection usage.
  if (descriptor_->oneof_decl_count() > 0) {
    printer->Print("struct $name$OneofInstance {\n",
                   "name", classname_);
    for (int i = 0; i < descriptor_->oneof_decl_count(); i++) {
      for (int j = 0; j < descriptor_->oneof_decl(i)->field_count(); j++) {
        const FieldDescriptor* field = descriptor_->oneof_decl(i)->field(j);
        printer->Print("  ");
        if (IsStringOrMessage(field)) {
          printer->Print("const ");
        }
        field_generators_.get(field).GeneratePrivateMembers(printer);
      }
    }

    printer->Print("}* $name$_default_oneof_instance_ = NULL;\n",
                   "name", classname_);
  }

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

  if (descriptor_->oneof_decl_count() > 0) {
    printer->Print(vars,
    "    $classname$_default_oneof_instance_,\n"
    "    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET("
      "$classname$, _oneof_case_[0]),\n");
  }

  printer->Print(
    "    ::google::protobuf::DescriptorPool::generated_pool(),\n");
  printer->Print(vars,
    "    ::google::protobuf::MessageFactory::generated_factory(),\n");
  printer->Print(vars,
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
  // Construct the default instances of all fields, as they will be used
  // when creating the default instance of the entire message.
  for (int i = 0; i < descriptor_->field_count(); i++) {
    field_generators_.get(descriptor_->field(i))
                     .GenerateDefaultInstanceAllocator(printer);
  }

  // Construct the default instance.  We can't call InitAsDefaultInstance() yet
  // because we need to make sure all default instances that this one might
  // depend on are constructed first.
  printer->Print(
    "$classname$::default_instance_ = new $classname$();\n",
    "classname", classname_);

  if ((descriptor_->oneof_decl_count() > 0) &&
      HasDescriptorMethods(descriptor_->file())) {
    printer->Print(
    "$classname$_default_oneof_instance_ = new $classname$OneofInstance;\n",
    "classname", classname_);
  }

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
GenerateShutdownCode(io::Printer* printer) {
  printer->Print(
    "delete $classname$::default_instance_;\n",
    "classname", classname_);

  if (HasDescriptorMethods(descriptor_->file())) {
    if (descriptor_->oneof_decl_count() > 0) {
      printer->Print(
        "delete $classname$_default_oneof_instance_;\n",
        "classname", classname_);
    }
    printer->Print(
      "delete $classname$_reflection_;\n",
      "classname", classname_);
  }

  // Handle default instances of fields.
  for (int i = 0; i < descriptor_->field_count(); i++) {
    field_generators_.get(descriptor_->field(i))
                     .GenerateShutdownCode(printer);
  }

  // Handle nested types.
  for (int i = 0; i < descriptor_->nested_type_count(); i++) {
    nested_generators_[i]->GenerateShutdownCode(printer);
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

  if (descriptor_->oneof_decl_count() > 0) {
    GenerateOneofClear(printer);
    printer->Print("\n");
  }

  if (HasGeneratedMethods(descriptor_->file())) {
    GenerateClear(printer);
    printer->Print("\n");

    GenerateMergeFromCodedStream(printer);
    printer->Print("\n");

    GenerateSerializeWithCachedSizes(printer);
    printer->Print("\n");

    if (HasFastArraySerialization(descriptor_->file())) {
      GenerateSerializeWithCachedSizesToArray(printer);
      printer->Print("\n");
    }

    GenerateByteSize(printer);
    printer->Print("\n");

    GenerateMergeFrom(printer);
    printer->Print("\n");

    GenerateCopyFrom(printer);
    printer->Print("\n");

    GenerateIsInitialized(printer);
    printer->Print("\n");
  }

  GenerateSwap(printer);
  printer->Print("\n");

  if (HasDescriptorMethods(descriptor_->file())) {
    printer->Print(
      "::google::protobuf::Metadata $classname$::GetMetadata() const {\n"
      "  protobuf_AssignDescriptorsOnce();\n"
      "  ::google::protobuf::Metadata metadata;\n"
      "  metadata.descriptor = $classname$_descriptor_;\n"
      "  metadata.reflection = $classname$_reflection_;\n"
      "  return metadata;\n"
      "}\n"
      "\n",
      "classname", classname_);
  } else {
    printer->Print(
      "::std::string $classname$::GetTypeName() const {\n"
      "  return \"$type_name$\";\n"
      "}\n"
      "\n",
      "classname", classname_,
      "type_name", descriptor_->full_name());
  }

}

void MessageGenerator::
GenerateOffsets(io::Printer* printer) {
  printer->Print(
    "static const int $classname$_offsets_[$field_count$] = {\n",
    "classname", classname_,
    "field_count", SimpleItoa(max(
        1, descriptor_->field_count() + descriptor_->oneof_decl_count())));
  printer->Indent();

  for (int i = 0; i < descriptor_->field_count(); i++) {
    const FieldDescriptor* field = descriptor_->field(i);
    if (field->containing_oneof()) {
      printer->Print(
          "PROTO2_GENERATED_DEFAULT_ONEOF_FIELD_OFFSET("
          "$classname$_default_oneof_instance_, $name$_),\n",
          "classname", classname_,
          "name", FieldName(field));
    } else {
      printer->Print(
          "GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET($classname$, "
                                                 "$name$_),\n",
          "classname", classname_,
          "name", FieldName(field));
    }
  }

  for (int i = 0; i < descriptor_->oneof_decl_count(); i++) {
    const OneofDescriptor* oneof = descriptor_->oneof_decl(i);
    printer->Print(
      "GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET($classname$, $name$_),\n",
      "classname", classname_,
      "name", oneof->name());
  }

  printer->Outdent();
  printer->Print("};\n");
}

void MessageGenerator::
GenerateSharedConstructorCode(io::Printer* printer) {
  printer->Print(
    "void $classname$::SharedCtor() {\n",
    "classname", classname_);
  printer->Indent();

  printer->Print(StrCat(
      uses_string_ ? "::google::protobuf::internal::GetEmptyString();\n" : "",
      "_cached_size_ = 0;\n").c_str());

  for (int i = 0; i < descriptor_->field_count(); i++) {
    if (!descriptor_->field(i)->containing_oneof()) {
      field_generators_.get(descriptor_->field(i))
          .GenerateConstructorCode(printer);
    }
  }

  printer->Print(
    "::memset(_has_bits_, 0, sizeof(_has_bits_));\n");

  for (int i = 0; i < descriptor_->oneof_decl_count(); i++) {
    printer->Print(
        "clear_has_$oneof_name$();\n",
        "oneof_name", descriptor_->oneof_decl(i)->name());
  }

  printer->Outdent();
  printer->Print("}\n\n");
}

void MessageGenerator::
GenerateSharedDestructorCode(io::Printer* printer) {
  printer->Print(
    "void $classname$::SharedDtor() {\n",
    "classname", classname_);
  printer->Indent();
  // Write the destructors for each field except oneof members.
  for (int i = 0; i < descriptor_->field_count(); i++) {
    if (!descriptor_->field(i)->containing_oneof()) {
      field_generators_.get(descriptor_->field(i))
                       .GenerateDestructorCode(printer);
    }
  }

  // Generate code to destruct oneofs. Clearing should do the work.
  for (int i = 0; i < descriptor_->oneof_decl_count(); i++) {
    printer->Print(
        "if (has_$oneof_name$()) {\n"
        "  clear_$oneof_name$();\n"
        "}\n",
        "oneof_name", descriptor_->oneof_decl(i)->name());
  }

  PrintHandlingOptionalStaticInitializers(
    descriptor_->file(), printer,
    // With static initializers.
    "if (this != default_instance_) {\n",
    // Without.
    "if (this != &default_instance()) {\n");

  // We need to delete all embedded messages.
  // TODO(kenton):  If we make unset messages point at default instances
  //   instead of NULL, then it would make sense to move this code into
  //   MessageFieldGenerator::GenerateDestructorCode().
  for (int i = 0; i < descriptor_->field_count(); i++) {
    const FieldDescriptor* field = descriptor_->field(i);

    if (!field->is_repeated() &&
        field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      // Skip oneof members
      if (!field->containing_oneof()) {
        printer->Print(
            "  delete $name$_;\n",
            "name", FieldName(field));
      }
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
  string superclass = SuperClassName(descriptor_);

  // Generate the default constructor.
  printer->Print(
    "$classname$::$classname$()\n"
    "  : $superclass$() {\n"
    "  SharedCtor();\n"
    "  // @@protoc_insertion_point(constructor:$full_name$)\n"
    "}\n",
    "classname", classname_,
    "superclass", superclass,
    "full_name", descriptor_->full_name());

  printer->Print(
    "\n"
    "void $classname$::InitAsDefaultInstance() {\n",
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
        field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE &&
        (field->containing_oneof() == NULL ||
         HasDescriptorMethods(descriptor_->file()))) {
      string name;
      if (field->containing_oneof()) {
        name = classname_ + "_default_oneof_instance_->";
      }
      name += FieldName(field);
      PrintHandlingOptionalStaticInitializers(
        descriptor_->file(), printer,
        // With static initializers.
        "  $name$_ = const_cast< $type$*>(&$type$::default_instance());\n",
        // Without.
        "  $name$_ = const_cast< $type$*>(\n"
        "      $type$::internal_default_instance());\n",
        // Vars.
        "name", name,
        "type", FieldMessageTypeName(field));
    } else if (field->containing_oneof() &&
               HasDescriptorMethods(descriptor_->file())) {
      field_generators_.get(descriptor_->field(i))
          .GenerateConstructorCode(printer);
    }
  }
  printer->Print(
    "}\n"
    "\n");

  // Generate the copy constructor.
  printer->Print(
    "$classname$::$classname$(const $classname$& from)\n"
    "  : $superclass$() {\n"
    "  SharedCtor();\n"
    "  MergeFrom(from);\n"
    "  // @@protoc_insertion_point(copy_constructor:$full_name$)\n"
    "}\n"
    "\n",
    "classname", classname_,
    "superclass", superclass,
    "full_name", descriptor_->full_name());

  // Generate the shared constructor code.
  GenerateSharedConstructorCode(printer);

  // Generate the destructor.
  printer->Print(
    "$classname$::~$classname$() {\n"
    "  // @@protoc_insertion_point(destructor:$full_name$)\n"
    "  SharedDtor();\n"
    "}\n"
    "\n",
    "classname", classname_,
    "full_name", descriptor_->full_name());

  // Generate the shared destructor code.
  GenerateSharedDestructorCode(printer);

  // Generate SetCachedSize.
  printer->Print(
    "void $classname$::SetCachedSize(int size) const {\n"
    "  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();\n"
    "  _cached_size_ = size;\n"
    "  GOOGLE_SAFE_CONCURRENT_WRITES_END();\n"
    "}\n",
    "classname", classname_);

  // Only generate this member if it's not disabled.
  if (HasDescriptorMethods(descriptor_->file()) &&
      !descriptor_->options().no_standard_descriptor_accessor()) {
    printer->Print(
      "const ::google::protobuf::Descriptor* $classname$::descriptor() {\n"
      "  protobuf_AssignDescriptorsOnce();\n"
      "  return $classname$_descriptor_;\n"
      "}\n"
      "\n",
      "classname", classname_,
      "adddescriptorsname",
      GlobalAddDescriptorsName(descriptor_->file()->name()));
  }

  printer->Print(
    "const $classname$& $classname$::default_instance() {\n",
    "classname", classname_);

  PrintHandlingOptionalStaticInitializers(
    descriptor_->file(), printer,
    // With static initializers.
    "  if (default_instance_ == NULL) $adddescriptorsname$();\n",
    // Without.
    "  $adddescriptorsname$();\n",
    // Vars.
    "adddescriptorsname",
    GlobalAddDescriptorsName(descriptor_->file()->name()));

  printer->Print(
    "  return *default_instance_;\n"
    "}\n"
    "\n"
    "$classname$* $classname$::default_instance_ = NULL;\n"
    "\n",
    "classname", classname_);

  printer->Print(
    "$classname$* $classname$::New() const {\n"
    "  return new $classname$;\n"
    "}\n",
    "classname", classname_);

}

// Return the number of bits set in n, a non-negative integer.
static int popcnt(uint32 n) {
  int result = 0;
  while (n != 0) {
    result += (n & 1);
    n = n / 2;
  }
  return result;
}

void MessageGenerator::
GenerateClear(io::Printer* printer) {
  printer->Print("void $classname$::Clear() {\n",
                 "classname", classname_);
  printer->Indent();

  // Step 1: Extensions
  if (descriptor_->extension_range_count() > 0) {
    printer->Print("_extensions_.Clear();\n");
  }

  // Step 2: Everything but extensions, repeateds, unions.
  // These are handled in chunks of 8.  The first chunk is
  // the non-extensions-non-repeateds-non-unions in
  //  descriptor_->field(0), descriptor_->field(1), ... descriptor_->field(7),
  // and the second chunk is the same for
  //  descriptor_->field(8), descriptor_->field(9), ... descriptor_->field(15),
  // etc.
  set<int> step2_indices;
  hash_map<string, int> fieldname_to_chunk;
  hash_map<int, string> memsets_for_chunk;
  hash_map<int, int> memset_field_count_for_chunk;
  hash_set<string> handled;  // fields that appear anywhere in memsets_for_chunk
  hash_map<int, uint32> fields_mask_for_chunk;
  for (int i = 0; i < descriptor_->field_count(); i++) {
    const FieldDescriptor* field = descriptor_->field(i);
    if (!field->is_repeated() && !field->containing_oneof()) {
      step2_indices.insert(i);
      int chunk = i / 8;
      fieldname_to_chunk[FieldName(field)] = chunk;
      fields_mask_for_chunk[chunk] |= static_cast<uint32>(1) << (i % 32);
    }
  }

  // Step 2a: Greedily seek runs of fields that can be cleared by memset-to-0.
  // The generated code uses two macros to help it clear runs of fields:
  // OFFSET_OF_FIELD_ computes the offset (in bytes) of a field in the Message.
  // ZR_ zeroes a non-empty range of fields via memset.
  const char* macros =
      "#define OFFSET_OF_FIELD_(f) (reinterpret_cast<char*>(      \\\n"
      "  &reinterpret_cast<$classname$*>(16)->f) - \\\n"
      "   reinterpret_cast<char*>(16))\n\n"
      "#define ZR_(first, last) do {                              \\\n"
      "    size_t f = OFFSET_OF_FIELD_(first);                    \\\n"
      "    size_t n = OFFSET_OF_FIELD_(last) - f + sizeof(last);  \\\n"
      "    ::memset(&first, 0, n);                                \\\n"
      "  } while (0)\n\n";
  for (int i = 0; i < runs_of_fields_.size(); i++) {
    const vector<string>& run = runs_of_fields_[i];
    if (run.size() < 2) continue;
    const string& first_field_name = run[0];
    const string& last_field_name = run.back();
    int chunk = fieldname_to_chunk[run[0]];
    memsets_for_chunk[chunk].append(
      "ZR_(" + first_field_name + "_, " + last_field_name + "_);\n");
    for (int j = 0; j < run.size(); j++) {
      GOOGLE_DCHECK_EQ(chunk, fieldname_to_chunk[run[j]]);
      handled.insert(run[j]);
    }
    memset_field_count_for_chunk[chunk] += run.size();
  }
  const bool macros_are_needed = handled.size() > 0;
  if (macros_are_needed) {
    printer->Outdent();
    printer->Print(macros,
                   "classname", classname_);
    printer->Indent();
  }
  // Step 2b: Finish step 2, ignoring fields handled in step 2a.
  int last_index = -1;
  bool chunk_block_in_progress = false;
  for (int i = 0; i < descriptor_->field_count(); i++) {
    if (step2_indices.count(i) == 0) continue;
    const FieldDescriptor* field = descriptor_->field(i);
    const string fieldname = FieldName(field);
    if (i / 8 != last_index / 8 || last_index < 0) {
      // End previous chunk, if there was one.
      if (chunk_block_in_progress) {
        printer->Outdent();
        printer->Print("}\n");
        chunk_block_in_progress = false;
      }
      // Start chunk.
      const string& memsets = memsets_for_chunk[i / 8];
      uint32 mask = fields_mask_for_chunk[i / 8];
      int count = popcnt(mask);
      if (count == 1 ||
          (count <= 4 && count == memset_field_count_for_chunk[i / 8])) {
        // No "if" here because the chunk is trivial.
      } else {
        printer->Print(
          "if (_has_bits_[$index$ / 32] & $mask$) {\n",
          "index", SimpleItoa(i / 8 * 8),
          "mask", SimpleItoa(mask));
        printer->Indent();
        chunk_block_in_progress = true;
      }
      printer->Print(memsets.c_str());
    }
    last_index = i;
    if (handled.count(fieldname) > 0) continue;

    // It's faster to just overwrite primitive types, but we should
    // only clear strings and messages if they were set.
    // TODO(kenton):  Let the CppFieldGenerator decide this somehow.
    bool should_check_bit =
      field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE ||
      field->cpp_type() == FieldDescriptor::CPPTYPE_STRING;

    if (should_check_bit) {
      printer->Print("if (has_$name$()) {\n", "name", fieldname);
      printer->Indent();
    }

    field_generators_.get(field).GenerateClearingCode(printer);

    if (should_check_bit) {
      printer->Outdent();
      printer->Print("}\n");
    }
  }

  if (chunk_block_in_progress) {
    printer->Outdent();
    printer->Print("}\n");
  }
  if (macros_are_needed) {
    printer->Outdent();
    printer->Print("\n#undef OFFSET_OF_FIELD_\n#undef ZR_\n\n");
    printer->Indent();
  }

  // Step 3: Repeated fields don't use _has_bits_; emit code to clear them here.
  for (int i = 0; i < descriptor_->field_count(); i++) {
    const FieldDescriptor* field = descriptor_->field(i);

    if (field->is_repeated()) {
      field_generators_.get(field).GenerateClearingCode(printer);
    }
  }

  // Step 4: Unions.
  for (int i = 0; i < descriptor_->oneof_decl_count(); i++) {
    printer->Print(
        "clear_$oneof_name$();\n",
        "oneof_name", descriptor_->oneof_decl(i)->name());
  }

  // Step 5: Everything else.
  printer->Print(
    "::memset(_has_bits_, 0, sizeof(_has_bits_));\n");

  if (UseUnknownFieldSet(descriptor_->file())) {
    printer->Print(
      "mutable_unknown_fields()->Clear();\n");
  } else {
    printer->Print(
      "mutable_unknown_fields()->clear();\n");
  }

  printer->Outdent();
  printer->Print("}\n");
}

void MessageGenerator::
GenerateOneofClear(io::Printer* printer) {
  // Generated function clears the active field and union case (e.g. foo_case_).
  for (int i = 0; i < descriptor_->oneof_decl_count(); i++) {
    printer->Print(
        "void $classname$::clear_$oneofname$() {\n",
        "classname", classname_,
        "oneofname", descriptor_->oneof_decl(i)->name());
    printer->Indent();
    printer->Print(
        "switch($oneofname$_case()) {\n",
        "oneofname", descriptor_->oneof_decl(i)->name());
    printer->Indent();
    for (int j = 0; j < descriptor_->oneof_decl(i)->field_count(); j++) {
      const FieldDescriptor* field = descriptor_->oneof_decl(i)->field(j);
      printer->Print(
          "case k$field_name$: {\n",
          "field_name", UnderscoresToCamelCase(field->name(), true));
      printer->Indent();
      // We clear only allocated objects in oneofs
      if (!IsStringOrMessage(field)) {
        printer->Print(
            "// No need to clear\n");
      } else {
        field_generators_.get(field).GenerateClearingCode(printer);
      }
      printer->Print(
          "break;\n");
      printer->Outdent();
      printer->Print(
          "}\n");
    }
    printer->Print(
        "case $cap_oneof_name$_NOT_SET: {\n"
        "  break;\n"
        "}\n",
        "cap_oneof_name",
        ToUpper(descriptor_->oneof_decl(i)->name()));
    printer->Outdent();
    printer->Print(
        "}\n"
        "_oneof_case_[$oneof_index$] = $cap_oneof_name$_NOT_SET;\n",
        "oneof_index", SimpleItoa(i),
        "cap_oneof_name",
        ToUpper(descriptor_->oneof_decl(i)->name()));
    printer->Outdent();
    printer->Print(
        "}\n"
        "\n");
  }
}

void MessageGenerator::
GenerateSwap(io::Printer* printer) {
  // Generate the Swap member function.
  printer->Print("void $classname$::Swap($classname$* other) {\n",
                 "classname", classname_);
  printer->Indent();
  printer->Print("if (other != this) {\n");
  printer->Indent();

  if (HasGeneratedMethods(descriptor_->file())) {
    for (int i = 0; i < descriptor_->field_count(); i++) {
      const FieldDescriptor* field = descriptor_->field(i);
      field_generators_.get(field).GenerateSwappingCode(printer);
    }

    for (int i = 0; i < descriptor_->oneof_decl_count(); i++) {
      printer->Print(
        "std::swap($oneof_name$_, other->$oneof_name$_);\n"
        "std::swap(_oneof_case_[$i$], other->_oneof_case_[$i$]);\n",
        "oneof_name", descriptor_->oneof_decl(i)->name(),
        "i", SimpleItoa(i));
    }

    for (int i = 0; i < (descriptor_->field_count() + 31) / 32; ++i) {
      printer->Print("std::swap(_has_bits_[$i$], other->_has_bits_[$i$]);\n",
                     "i", SimpleItoa(i));
    }

    if (UseUnknownFieldSet(descriptor_->file())) {
      printer->Print("_unknown_fields_.Swap(&other->_unknown_fields_);\n");
    } else {
      printer->Print("_unknown_fields_.swap(other->_unknown_fields_);\n");
    }
    printer->Print("std::swap(_cached_size_, other->_cached_size_);\n");
    if (descriptor_->extension_range_count() > 0) {
      printer->Print("_extensions_.Swap(&other->_extensions_);\n");
    }
  } else {
    printer->Print("GetReflection()->Swap(this, other);");
  }

  printer->Outdent();
  printer->Print("}\n");
  printer->Outdent();
  printer->Print("}\n");
}

void MessageGenerator::
GenerateMergeFrom(io::Printer* printer) {
  if (HasDescriptorMethods(descriptor_->file())) {
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
  } else {
    // Generate CheckTypeAndMergeFrom().
    printer->Print(
      "void $classname$::CheckTypeAndMergeFrom(\n"
      "    const ::google::protobuf::MessageLite& from) {\n"
      "  MergeFrom(*::google::protobuf::down_cast<const $classname$*>(&from));\n"
      "}\n"
      "\n",
      "classname", classname_);
  }

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

  // Merge oneof fields. Oneof field requires oneof case check.
  for (int i = 0; i < descriptor_->oneof_decl_count(); ++i) {
    printer->Print(
        "switch (from.$oneofname$_case()) {\n",
        "oneofname", descriptor_->oneof_decl(i)->name());
    printer->Indent();
    for (int j = 0; j < descriptor_->oneof_decl(i)->field_count(); j++) {
      const FieldDescriptor* field = descriptor_->oneof_decl(i)->field(j);
      printer->Print(
          "case k$field_name$: {\n",
          "field_name", UnderscoresToCamelCase(field->name(), true));
      printer->Indent();
      field_generators_.get(field).GenerateMergingCode(printer);
      printer->Print(
          "break;\n");
      printer->Outdent();
      printer->Print(
          "}\n");
    }
    printer->Print(
        "case $cap_oneof_name$_NOT_SET: {\n"
        "  break;\n"
        "}\n",
        "cap_oneof_name",
        ToUpper(descriptor_->oneof_decl(i)->name()));
    printer->Outdent();
    printer->Print(
        "}\n");
  }

  // Merge Optional and Required fields (after a _has_bit check).
  int last_index = -1;

  for (int i = 0; i < descriptor_->field_count(); ++i) {
    const FieldDescriptor* field = descriptor_->field(i);

    if (!field->is_repeated() && !field->containing_oneof()) {
      // See above in GenerateClear for an explanation of this.
      if (i / 8 != last_index / 8 || last_index < 0) {
        if (last_index >= 0) {
          printer->Outdent();
          printer->Print("}\n");
        }
        printer->Print(
          "if (from._has_bits_[$index$ / 32] & (0xffu << ($index$ % 32))) {\n",
          "index", SimpleItoa(field->index()));
        printer->Indent();
      }

      last_index = i;

      printer->Print(
        "if (from.has_$name$()) {\n",
        "name", FieldName(field));
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

  if (UseUnknownFieldSet(descriptor_->file())) {
    printer->Print(
      "mutable_unknown_fields()->MergeFrom(from.unknown_fields());\n");
  } else {
    printer->Print(
      "mutable_unknown_fields()->append(from.unknown_fields());\n");
  }

  printer->Outdent();
  printer->Print("}\n");
}

void MessageGenerator::
GenerateCopyFrom(io::Printer* printer) {
  if (HasDescriptorMethods(descriptor_->file())) {
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
  }

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
  if (descriptor_->options().message_set_wire_format()) {
    // Special-case MessageSet.
    printer->Print(
      "bool $classname$::MergePartialFromCodedStream(\n"
      "    ::google::protobuf::io::CodedInputStream* input) {\n",
      "classname", classname_);

    PrintHandlingOptionalStaticInitializers(
      descriptor_->file(), printer,
      // With static initializers.
      "  return _extensions_.ParseMessageSet(input, default_instance_,\n"
      "                                      mutable_unknown_fields());\n",
      // Without.
      "  return _extensions_.ParseMessageSet(input, &default_instance(),\n"
      "                                      mutable_unknown_fields());\n",
      // Vars.
      "classname", classname_);

    printer->Print(
      "}\n");
    return;
  }

  printer->Print(
    "bool $classname$::MergePartialFromCodedStream(\n"
    "    ::google::protobuf::io::CodedInputStream* input) {\n"
    "#define DO_(EXPRESSION) if (!(EXPRESSION)) goto failure\n"
    "  ::google::protobuf::uint32 tag;\n",
    "classname", classname_);

  if (!UseUnknownFieldSet(descriptor_->file())) {
    printer->Print(
      "  ::google::protobuf::io::StringOutputStream unknown_fields_string(\n"
      "      mutable_unknown_fields());\n"
      "  ::google::protobuf::io::CodedOutputStream unknown_fields_stream(\n"
      "      &unknown_fields_string);\n");
  }

  printer->Print(
    "  // @@protoc_insertion_point(parse_start:$full_name$)\n",
    "full_name", descriptor_->full_name());

  printer->Indent();
  printer->Print("for (;;) {\n");
  printer->Indent();

  scoped_array<const FieldDescriptor*> ordered_fields(
      SortFieldsByNumber(descriptor_));
  uint32 maxtag = descriptor_->field_count() == 0 ? 0 :
      WireFormat::MakeTag(ordered_fields[descriptor_->field_count() - 1]);
  const int kCutoff0 = 127;               // fits in 1-byte varint
  const int kCutoff1 = (127 << 7) + 127;  // fits in 2-byte varint
  printer->Print("::std::pair< ::google::protobuf::uint32, bool> p = "
                 "input->ReadTagWithCutoff($max$);\n"
                 "tag = p.first;\n"
                 "if (!p.second) goto handle_unusual;\n",
                 "max", SimpleItoa(maxtag <= kCutoff0 ? kCutoff0 :
                                   (maxtag <= kCutoff1 ? kCutoff1 :
                                    maxtag)));
  if (descriptor_->field_count() > 0) {
    // We don't even want to print the switch() if we have no fields because
    // MSVC dislikes switch() statements that contain only a default value.

    // Note:  If we just switched on the tag rather than the field number, we
    // could avoid the need for the if() to check the wire type at the beginning
    // of each case.  However, this is actually a bit slower in practice as it
    // creates a jump table that is 8x larger and sparser, and meanwhile the
    // if()s are highly predictable.
    printer->Print("switch (::google::protobuf::internal::WireFormatLite::"
                   "GetTagFieldNumber(tag)) {\n");

    printer->Indent();

    for (int i = 0; i < descriptor_->field_count(); i++) {
      const FieldDescriptor* field = ordered_fields[i];

      PrintFieldComment(printer, field);

      printer->Print(
        "case $number$: {\n",
        "number", SimpleItoa(field->number()));
      printer->Indent();
      const FieldGenerator& field_generator = field_generators_.get(field);

      // Emit code to parse the common, expected case.
      printer->Print("if (tag == $commontag$) {\n",
                     "commontag", SimpleItoa(WireFormat::MakeTag(field)));

      if (i > 0 || (field->is_repeated() && !field->options().packed())) {
        printer->Print(
          " parse_$name$:\n",
          "name", field->name());
      }

      printer->Indent();
      if (field->options().packed()) {
        field_generator.GenerateMergeFromCodedStreamWithPacking(printer);
      } else {
        field_generator.GenerateMergeFromCodedStream(printer);
      }
      printer->Outdent();

      // Emit code to parse unexpectedly packed or unpacked values.
      if (field->is_packable() && field->options().packed()) {
        internal::WireFormatLite::WireType wiretype =
            WireFormat::WireTypeForFieldType(field->type());
        printer->Print("} else if (tag == $uncommontag$) {\n",
                       "uncommontag", SimpleItoa(
                           internal::WireFormatLite::MakeTag(
                               field->number(), wiretype)));
        printer->Indent();
        field_generator.GenerateMergeFromCodedStream(printer);
        printer->Outdent();
      } else if (field->is_packable() && !field->options().packed()) {
        internal::WireFormatLite::WireType wiretype =
            internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED;
        printer->Print("} else if (tag == $uncommontag$) {\n",
                       "uncommontag", SimpleItoa(
                           internal::WireFormatLite::MakeTag(
                               field->number(), wiretype)));
        printer->Indent();
        field_generator.GenerateMergeFromCodedStreamWithPacking(printer);
        printer->Outdent();
      }

      printer->Print(
        "} else {\n"
        "  goto handle_unusual;\n"
        "}\n");

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
          "if (input->ExpectAtEnd()) goto success;\n");
      }

      printer->Print(
        "break;\n");

      printer->Outdent();
      printer->Print("}\n\n");
    }

    printer->Print("default: {\n");
    printer->Indent();
  }

  printer->Outdent();
  printer->Print("handle_unusual:\n");
  printer->Indent();
  // If tag is 0 or an end-group tag then this must be the end of the message.
  printer->Print(
    "if (tag == 0 ||\n"
    "    ::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==\n"
    "    ::google::protobuf::internal::WireFormatLite::WIRETYPE_END_GROUP) {\n"
    "  goto success;\n"
    "}\n");

  // Handle extension ranges.
  if (descriptor_->extension_range_count() > 0) {
    printer->Print(
      "if (");
    for (int i = 0; i < descriptor_->extension_range_count(); i++) {
      const Descriptor::ExtensionRange* range =
        descriptor_->extension_range(i);
      if (i > 0) printer->Print(" ||\n    ");

      uint32 start_tag = WireFormatLite::MakeTag(
        range->start, static_cast<WireFormatLite::WireType>(0));
      uint32 end_tag = WireFormatLite::MakeTag(
        range->end, static_cast<WireFormatLite::WireType>(0));

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
    printer->Print(") {\n");
    if (UseUnknownFieldSet(descriptor_->file())) {
      PrintHandlingOptionalStaticInitializers(
        descriptor_->file(), printer,
        // With static initializers.
        "  DO_(_extensions_.ParseField(tag, input, default_instance_,\n"
        "                              mutable_unknown_fields()));\n",
        // Without.
        "  DO_(_extensions_.ParseField(tag, input, &default_instance(),\n"
        "                              mutable_unknown_fields()));\n");
    } else {
      PrintHandlingOptionalStaticInitializers(
        descriptor_->file(), printer,
        // With static initializers.
        "  DO_(_extensions_.ParseField(tag, input, default_instance_,\n"
        "                              &unknown_fields_stream));\n",
        // Without.
        "  DO_(_extensions_.ParseField(tag, input, &default_instance(),\n"
        "                              &unknown_fields_stream));\n");
    }
    printer->Print(
      "  continue;\n"
      "}\n");
  }

  // We really don't recognize this tag.  Skip it.
  if (UseUnknownFieldSet(descriptor_->file())) {
    printer->Print(
      "DO_(::google::protobuf::internal::WireFormat::SkipField(\n"
      "      input, tag, mutable_unknown_fields()));\n");
  } else {
    printer->Print(
      "DO_(::google::protobuf::internal::WireFormatLite::SkipField(\n"
      "    input, tag, &unknown_fields_stream));\n");
  }

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
    "  }\n"                   // for (;;)
    "success:\n"
    "  // @@protoc_insertion_point(parse_success:$full_name$)\n"
    "  return true;\n"
    "failure:\n"
    "  // @@protoc_insertion_point(parse_failure:$full_name$)\n"
    "  return false;\n"
    "#undef DO_\n"
    "}\n", "full_name", descriptor_->full_name());
}

void MessageGenerator::GenerateSerializeOneField(
    io::Printer* printer, const FieldDescriptor* field, bool to_array) {
  PrintFieldComment(printer, field);

  if (!field->is_repeated()) {
    printer->Print(
      "if (has_$name$()) {\n",
      "name", FieldName(field));
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
  if (descriptor_->options().message_set_wire_format()) {
    // Special-case MessageSet.
    printer->Print(
      "void $classname$::SerializeWithCachedSizes(\n"
      "    ::google::protobuf::io::CodedOutputStream* output) const {\n"
      "  _extensions_.SerializeMessageSetWithCachedSizes(output);\n",
      "classname", classname_);
    GOOGLE_CHECK(UseUnknownFieldSet(descriptor_->file()));
    printer->Print(
      "  ::google::protobuf::internal::WireFormat::SerializeUnknownMessageSetItems(\n"
      "      unknown_fields(), output);\n");
    printer->Print(
      "}\n");
    return;
  }

  printer->Print(
    "void $classname$::SerializeWithCachedSizes(\n"
    "    ::google::protobuf::io::CodedOutputStream* output) const {\n",
    "classname", classname_);
  printer->Indent();

  printer->Print(
    "// @@protoc_insertion_point(serialize_start:$full_name$)\n",
    "full_name", descriptor_->full_name());

  GenerateSerializeWithCachedSizesBody(printer, false);

  printer->Print(
    "// @@protoc_insertion_point(serialize_end:$full_name$)\n",
    "full_name", descriptor_->full_name());

  printer->Outdent();
  printer->Print(
    "}\n");
}

void MessageGenerator::
GenerateSerializeWithCachedSizesToArray(io::Printer* printer) {
  if (descriptor_->options().message_set_wire_format()) {
    // Special-case MessageSet.
    printer->Print(
      "::google::protobuf::uint8* $classname$::SerializeWithCachedSizesToArray(\n"
      "    ::google::protobuf::uint8* target) const {\n"
      "  target =\n"
      "      _extensions_.SerializeMessageSetWithCachedSizesToArray(target);\n",
      "classname", classname_);
    GOOGLE_CHECK(UseUnknownFieldSet(descriptor_->file()));
    printer->Print(
      "  target = ::google::protobuf::internal::WireFormat::\n"
      "             SerializeUnknownMessageSetItemsToArray(\n"
      "               unknown_fields(), target);\n");
    printer->Print(
      "  return target;\n"
      "}\n");
    return;
  }

  printer->Print(
    "::google::protobuf::uint8* $classname$::SerializeWithCachedSizesToArray(\n"
    "    ::google::protobuf::uint8* target) const {\n",
    "classname", classname_);
  printer->Indent();

  printer->Print(
    "// @@protoc_insertion_point(serialize_to_array_start:$full_name$)\n",
    "full_name", descriptor_->full_name());

  GenerateSerializeWithCachedSizesBody(printer, true);

  printer->Print(
    "// @@protoc_insertion_point(serialize_to_array_end:$full_name$)\n",
    "full_name", descriptor_->full_name());

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

  if (UseUnknownFieldSet(descriptor_->file())) {
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
  } else {
    printer->Print(
      "output->WriteRaw(unknown_fields().data(),\n"
      "                 unknown_fields().size());\n");
  }
}

void MessageGenerator::
GenerateByteSize(io::Printer* printer) {
  if (descriptor_->options().message_set_wire_format()) {
    // Special-case MessageSet.
    printer->Print(
      "int $classname$::ByteSize() const {\n"
      "  int total_size = _extensions_.MessageSetByteSize();\n",
      "classname", classname_);
    GOOGLE_CHECK(UseUnknownFieldSet(descriptor_->file()));
    printer->Print(
      "  total_size += ::google::protobuf::internal::WireFormat::\n"
      "      ComputeUnknownMessageSetItemsSize(unknown_fields());\n");
    printer->Print(
      "  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();\n"
      "  _cached_size_ = total_size;\n"
      "  GOOGLE_SAFE_CONCURRENT_WRITES_END();\n"
      "  return total_size;\n"
      "}\n");
    return;
  }

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

    if (!field->is_repeated() && !field->containing_oneof()) {
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

  // Fields inside a oneof don't use _has_bits_ so we count them in a separate
  // pass.
  for (int i = 0; i < descriptor_->oneof_decl_count(); i++) {
    printer->Print(
        "switch ($oneofname$_case()) {\n",
        "oneofname", descriptor_->oneof_decl(i)->name());
    printer->Indent();
    for (int j = 0; j < descriptor_->oneof_decl(i)->field_count(); j++) {
      const FieldDescriptor* field = descriptor_->oneof_decl(i)->field(j);
      PrintFieldComment(printer, field);
      printer->Print(
          "case k$field_name$: {\n",
          "field_name", UnderscoresToCamelCase(field->name(), true));
      printer->Indent();
      field_generators_.get(field).GenerateByteSize(printer);
      printer->Print(
          "break;\n");
      printer->Outdent();
      printer->Print(
          "}\n");
    }
    printer->Print(
        "case $cap_oneof_name$_NOT_SET: {\n"
        "  break;\n"
        "}\n",
        "cap_oneof_name",
        ToUpper(descriptor_->oneof_decl(i)->name()));
    printer->Outdent();
    printer->Print(
        "}\n");
  }

  if (descriptor_->extension_range_count() > 0) {
    printer->Print(
      "total_size += _extensions_.ByteSize();\n"
      "\n");
  }

  if (UseUnknownFieldSet(descriptor_->file())) {
    printer->Print("if (!unknown_fields().empty()) {\n");
    printer->Indent();
    printer->Print(
      "total_size +=\n"
      "  ::google::protobuf::internal::WireFormat::ComputeUnknownFieldsSize(\n"
      "    unknown_fields());\n");
    printer->Outdent();
    printer->Print("}\n");
  } else {
    printer->Print(
      "total_size += unknown_fields().size();\n"
      "\n");
  }

  // We update _cached_size_ even though this is a const method.  In theory,
  // this is not thread-compatible, because concurrent writes have undefined
  // results.  In practice, since any concurrent writes will be writing the
  // exact same value, it works on all common processors.  In a future version
  // of C++, _cached_size_ should be made into an atomic<int>.
  printer->Print(
    "GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();\n"
    "_cached_size_ = total_size;\n"
    "GOOGLE_SAFE_CONCURRENT_WRITES_END();\n"
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
        !ShouldIgnoreRequiredFieldCheck(field) &&
        HasRequiredFields(field->message_type())) {
      if (field->is_repeated()) {
        printer->Print(
          "if (!::google::protobuf::internal::AllAreInitialized(this->$name$()))"
          " return false;\n",
          "name", FieldName(field));
      } else {
        if (field->options().weak()) {
          // For weak fields, use the data member (google::protobuf::Message*) instead
          // of the getter to avoid a link dependency on the weak message type
          // which is only forward declared.
          printer->Print(
            "if (has_$name$()) {\n"
            "  if (!this->$name$_->IsInitialized()) return false;\n"
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
