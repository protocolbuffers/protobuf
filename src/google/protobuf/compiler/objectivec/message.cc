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

#include "google/protobuf/compiler/objectivec/message.h"

#include <algorithm>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/log/absl_log.h"
#include "absl/strings/escaping.h"
#include "absl/strings/str_cat.h"
#include "google/protobuf/compiler/objectivec/extension.h"
#include "google/protobuf/compiler/objectivec/helpers.h"
#include "google/protobuf/compiler/objectivec/names.h"
#include "google/protobuf/compiler/objectivec/oneof.h"
#include "google/protobuf/compiler/objectivec/text_format_decode_data.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/io/printer.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace objectivec {

namespace {

bool IsMapEntryMessage(const Descriptor* descriptor) {
  return descriptor->options().map_entry();
}

struct FieldOrderingByNumber {
  inline bool operator()(const FieldDescriptor* a,
                         const FieldDescriptor* b) const {
    return a->number() < b->number();
  }
};

int OrderGroupForFieldDescriptor(const FieldDescriptor* descriptor) {
  // The first item in the object structure is our uint32[] for has bits.
  // We then want to order things to make the instances as small as
  // possible. So we follow the has bits with:
  //   1. Anything always 4 bytes - float, *32, enums
  //   2. Anything that is always a pointer (they will be 8 bytes on 64 bit
  //      builds and 4 bytes on 32bit builds.
  //   3. Anything always 8 bytes - double, *64
  //
  // NOTE: Bools aren't listed, they were stored in the has bits.
  //
  // Why? Using 64bit builds as an example, this means worse case, we have
  // enough bools that we overflow 1 byte from 4 byte alignment, so 3 bytes
  // are wasted before the 4 byte values. Then if we have an odd number of
  // those 4 byte values, the 8 byte values will be pushed down by 32bits to
  // keep them aligned. But the structure will end 8 byte aligned, so no
  // waste on the end. If you did the reverse order, you could waste 4 bytes
  // before the first 8 byte value (after the has array), then a single
  // bool on the end would need 7 bytes of padding to make the overall
  // structure 8 byte aligned; so 11 bytes, wasted total.

  // Anything repeated is a GPB*Array/NSArray, so pointer.
  if (descriptor->is_repeated()) {
    return 3;
  }

  switch (descriptor->type()) {
    // All always 8 bytes.
    case FieldDescriptor::TYPE_DOUBLE:
    case FieldDescriptor::TYPE_INT64:
    case FieldDescriptor::TYPE_SINT64:
    case FieldDescriptor::TYPE_UINT64:
    case FieldDescriptor::TYPE_SFIXED64:
    case FieldDescriptor::TYPE_FIXED64:
      return 4;

    // Pointers (string and bytes are NSString and NSData); 8 or 4 bytes
    // depending on the build architecture.
    case FieldDescriptor::TYPE_GROUP:
    case FieldDescriptor::TYPE_MESSAGE:
    case FieldDescriptor::TYPE_STRING:
    case FieldDescriptor::TYPE_BYTES:
      return 3;

    // All always 4 bytes (enums are int32s).
    case FieldDescriptor::TYPE_FLOAT:
    case FieldDescriptor::TYPE_INT32:
    case FieldDescriptor::TYPE_SINT32:
    case FieldDescriptor::TYPE_UINT32:
    case FieldDescriptor::TYPE_SFIXED32:
    case FieldDescriptor::TYPE_FIXED32:
    case FieldDescriptor::TYPE_ENUM:
      return 2;

    // 0 bytes. Stored in the has bits.
    case FieldDescriptor::TYPE_BOOL:
      return 99;  // End of the list (doesn't really matter).
  }

  // Some compilers report reaching end of function even though all cases of
  // the enum are handed in the switch.
  ABSL_LOG(FATAL) << "Can't get here.";
  return 0;
}

struct FieldOrderingByStorageSize {
  inline bool operator()(const FieldDescriptor* a,
                         const FieldDescriptor* b) const {
    // Order by grouping.
    const int order_group_a = OrderGroupForFieldDescriptor(a);
    const int order_group_b = OrderGroupForFieldDescriptor(b);
    if (order_group_a != order_group_b) {
      return order_group_a < order_group_b;
    }
    // Within the group, order by field number (provides stable ordering).
    return a->number() < b->number();
  }
};

struct ExtensionRangeOrdering {
  bool operator()(const Descriptor::ExtensionRange* a,
                  const Descriptor::ExtensionRange* b) const {
    return a->start < b->start;
  }
};

// This is a reduced case of Descriptor::ExtensionRange with just start and end.
struct SimpleExtensionRange {
  SimpleExtensionRange(int start, int end) : start(start), end(end){};
  int start;  // inclusive
  int end;    // exclusive

  // Descriptors expose extension ranges in the order they were defined in the
  // file, but this reorders and merges the ranges that are contiguous (i.e. -
  // [(21,30),(10,20)] -> [(10,30)])
  static std::vector<SimpleExtensionRange> Normalize(
      const Descriptor* descriptor) {
    std::vector<const Descriptor::ExtensionRange*> sorted_extensions;
    sorted_extensions.reserve(descriptor->extension_range_count());
    for (int i = 0; i < descriptor->extension_range_count(); ++i) {
      sorted_extensions.push_back(descriptor->extension_range(i));
    }

    std::sort(sorted_extensions.begin(), sorted_extensions.end(),
              ExtensionRangeOrdering());

    std::vector<SimpleExtensionRange> result;
    result.reserve(sorted_extensions.size());
    for (const auto ext : sorted_extensions) {
      if (!result.empty() && result.back().end == ext->start) {
        result.back().end = ext->end;
      } else {
        result.emplace_back(ext->start, ext->end);
      }
    }
    return result;
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
  std::sort(fields, fields + descriptor->field_count(),
            FieldOrderingByNumber());
  return fields;
}

// Sort the fields of the given Descriptor by storage size into a new[]'d
// array and return it.
const FieldDescriptor** SortFieldsByStorageSize(const Descriptor* descriptor) {
  const FieldDescriptor** fields =
      new const FieldDescriptor*[descriptor->field_count()];
  for (int i = 0; i < descriptor->field_count(); i++) {
    fields[i] = descriptor->field(i);
  }
  std::sort(fields, fields + descriptor->field_count(),
            FieldOrderingByStorageSize());
  return fields;
}

}  // namespace

MessageGenerator::MessageGenerator(const std::string& file_description_name,
                                   const Descriptor* descriptor)
    : file_description_name_(file_description_name),
      descriptor_(descriptor),
      field_generators_(descriptor),
      class_name_(ClassName(descriptor_)),
      deprecated_attribute_(GetOptionalDeprecatedAttribute(
          descriptor, descriptor->file(), false, true)) {
  for (int i = 0; i < descriptor_->real_oneof_decl_count(); i++) {
    oneof_generators_.push_back(
        std::make_unique<OneofGenerator>(descriptor_->oneof_decl(i)));
  }

  // Assign has bits:
  // 1. FieldGeneratorMap::CalculateHasBits() loops through the fields seeing
  //    who needs has bits and assigning them.
  // 2. FieldGenerator::SetOneofIndexBase() overrides has_bit with a negative
  //    index that groups all the elements in the oneof.
  size_t num_has_bits = field_generators_.CalculateHasBits();
  size_t sizeof_has_storage = (num_has_bits + 31) / 32;
  if (sizeof_has_storage == 0) {
    // In the case where no field needs has bits, don't let the _has_storage_
    // end up as zero length (zero length arrays are sort of a grey area
    // since it has to be at the start of the struct). This also ensures a
    // field with only oneofs keeps the required negative indices they need.
    sizeof_has_storage = 1;
  }
  // Tell all the fields the oneof base.
  for (const auto& generator : oneof_generators_) {
    generator->SetOneofIndexBase(sizeof_has_storage);
  }
  field_generators_.SetOneofIndexBase(sizeof_has_storage);
  // sizeof_has_storage needs enough bits for the single fields that aren't in
  // any oneof, and then one int32 for each oneof (to store the field number).
  sizeof_has_storage += oneof_generators_.size();

  sizeof_has_storage_ = sizeof_has_storage;
}

void MessageGenerator::AddExtensionGenerators(
    std::vector<std::unique_ptr<ExtensionGenerator>>* extension_generators) {
  for (int i = 0; i < descriptor_->extension_count(); i++) {
    extension_generators->push_back(std::make_unique<ExtensionGenerator>(
        class_name_, descriptor_->extension(i)));
    extension_generators_.push_back(extension_generators->back().get());
  }
}

void MessageGenerator::DetermineForwardDeclarations(
    absl::btree_set<std::string>* fwd_decls,
    bool include_external_types) const {
  if (IsMapEntryMessage(descriptor_)) {
    return;
  }
  for (int i = 0; i < descriptor_->field_count(); i++) {
    const FieldDescriptor* fieldDescriptor = descriptor_->field(i);
    field_generators_.get(fieldDescriptor)
        .DetermineForwardDeclarations(fwd_decls, include_external_types);
  }
}

void MessageGenerator::DetermineObjectiveCClassDefinitions(
    absl::btree_set<std::string>* fwd_decls) const {
  if (!IsMapEntryMessage(descriptor_)) {
    // Forward declare this class, as a linker symbol, so the symbol can be used
    // to reference the class instead of calling +class later.
    fwd_decls->insert(ObjCClassDeclaration(class_name_));

    for (int i = 0; i < descriptor_->field_count(); i++) {
      const FieldDescriptor* fieldDescriptor = descriptor_->field(i);
      field_generators_.get(fieldDescriptor)
          .DetermineObjectiveCClassDefinitions(fwd_decls);
    }
  }

  const Descriptor* containing_descriptor = descriptor_->containing_type();
  if (containing_descriptor != nullptr) {
    std::string containing_class = ClassName(containing_descriptor);
    fwd_decls->insert(ObjCClassDeclaration(containing_class));
  }
}

void MessageGenerator::GenerateMessageHeader(io::Printer* printer) const {
  // This a a map entry message, just recurse and do nothing directly.
  if (IsMapEntryMessage(descriptor_)) {
    return;
  }

  printer->Print(
      // clang-format off
      "#pragma mark - $classname$\n"
      "\n",
      // clang-format on
      "classname", class_name_);

  if (descriptor_->field_count()) {
    std::unique_ptr<const FieldDescriptor*[]> sorted_fields(
        SortFieldsByNumber(descriptor_));

    printer->Print("typedef GPB_ENUM($classname$_FieldNumber) {\n", "classname",
                   class_name_);
    printer->Indent();

    for (int i = 0; i < descriptor_->field_count(); i++) {
      field_generators_.get(sorted_fields[i])
          .GenerateFieldNumberConstant(printer);
    }

    printer->Outdent();
    printer->Print("};\n\n");
  }

  for (const auto& generator : oneof_generators_) {
    generator->GenerateCaseEnum(printer);
  }

  std::string message_comments;
  SourceLocation location;
  if (descriptor_->GetSourceLocation(&location)) {
    message_comments = BuildCommentsString(location, false);
  } else {
    message_comments = "";
  }

  printer->Print(
      // clang-format off
      "$comments$$deprecated_attribute$GPB_FINAL @interface $classname$ : GPBMessage\n\n",
      // clang-format on
      "classname", class_name_, "deprecated_attribute", deprecated_attribute_,
      "comments", message_comments);

  std::vector<char> seen_oneofs(oneof_generators_.size(), 0);
  for (int i = 0; i < descriptor_->field_count(); i++) {
    const FieldDescriptor* field = descriptor_->field(i);
    const OneofDescriptor* oneof = field->real_containing_oneof();
    if (oneof) {
      const int oneof_index = oneof->index();
      if (!seen_oneofs[oneof_index]) {
        seen_oneofs[oneof_index] = 1;
        oneof_generators_[oneof_index]->GeneratePublicCasePropertyDeclaration(
            printer);
      }
    }
    field_generators_.get(field).GeneratePropertyDeclaration(printer);
  }

  printer->Print("@end\n\n");

  for (int i = 0; i < descriptor_->field_count(); i++) {
    field_generators_.get(descriptor_->field(i))
        .GenerateCFunctionDeclarations(printer);
  }

  if (!oneof_generators_.empty()) {
    for (const auto& generator : oneof_generators_) {
      generator->GenerateClearFunctionDeclaration(printer);
    }
    printer->Print("\n");
  }

  if (descriptor_->extension_count() > 0) {
    printer->Print("@interface $classname$ (DynamicMethods)\n\n", "classname",
                   class_name_);
    for (const auto generator : extension_generators_) {
      generator->GenerateMembersHeader(printer);
    }
    printer->Print("@end\n\n");
  }
}

void MessageGenerator::GenerateSource(io::Printer* printer) const {
  if (IsMapEntryMessage(descriptor_)) {
    return;
  }
  printer->Print(
      // clang-format off
      "#pragma mark - $classname$\n"
      "\n",
      // clang-format on
      "classname", class_name_);

  if (!deprecated_attribute_.empty()) {
    // No warnings when compiling the impl of this deprecated class.
    // clang-format off
    printer->Print(
        "#pragma clang diagnostic push\n"
        "#pragma clang diagnostic ignored \"-Wdeprecated-implementations\"\n"
        "\n");
    // clang-format on
  }

  printer->Print("@implementation $classname$\n\n", "classname", class_name_);

  for (const auto& generator : oneof_generators_) {
    generator->GeneratePropertyImplementation(printer);
  }

  for (int i = 0; i < descriptor_->field_count(); i++) {
    field_generators_.get(descriptor_->field(i))
        .GeneratePropertyImplementation(printer);
  }

  std::unique_ptr<const FieldDescriptor*[]> sorted_fields(
      SortFieldsByNumber(descriptor_));
  std::unique_ptr<const FieldDescriptor*[]> size_order_fields(
      SortFieldsByStorageSize(descriptor_));

  std::vector<SimpleExtensionRange> sorted_extensions(
      SimpleExtensionRange::Normalize(descriptor_));

  printer->Print(
      // clang-format off
      "\n"
      "typedef struct $classname$__storage_ {\n"
      "  uint32_t _has_storage_[$sizeof_has_storage$];\n",
      // clang-format on
      "classname", class_name_, "sizeof_has_storage",
      absl::StrCat(sizeof_has_storage_));
  printer->Indent();
  for (int i = 0; i < descriptor_->field_count(); i++) {
    field_generators_.get(size_order_fields[i])
        .GenerateFieldStorageDeclaration(printer);
  }
  printer->Outdent();

  printer->Print("} $classname$__storage_;\n\n", "classname", class_name_);

  // clang-format off
  printer->Print(
      "// This method is threadsafe because it is initially called\n"
      "// in +initialize for each subclass.\n"
      "+ (GPBDescriptor *)descriptor {\n"
      "  static GPBDescriptor *descriptor = nil;\n"
      "  if (!descriptor) {\n");
  // clang-format on

  // If the message scopes extensions, trigger the root class
  // +initialize/+extensionRegistry as that is where the runtime support for
  // extensions lives.
  if (descriptor_->extension_count() > 0) {
    // clang-format off
    printer->Print(
        "    // Start up the root class to support the scoped extensions.\n"
        "    __unused Class rootStartup = [$root_class_name$ class];\n",
        "root_class_name", FileClassName(descriptor_->file()));
    // clang-format on
  } else {
    // The Root class has a debug runtime check, so if not starting that
    // up, add the check.
    printer->Print("    GPB_DEBUG_CHECK_RUNTIME_VERSIONS();\n");
  }

  TextFormatDecodeData text_format_decode_data;
  bool has_fields = descriptor_->field_count() > 0;
  bool need_defaults = field_generators_.DoesAnyFieldHaveNonZeroDefault();
  std::string field_description_type;
  if (need_defaults) {
    field_description_type = "GPBMessageFieldDescriptionWithDefault";
  } else {
    field_description_type = "GPBMessageFieldDescription";
  }
  if (has_fields) {
    printer->Indent();
    printer->Indent();
    printer->Print("static $field_description_type$ fields[] = {\n",
                   "field_description_type", field_description_type);
    printer->Indent();
    for (int i = 0; i < descriptor_->field_count(); ++i) {
      const FieldGenerator& field_generator =
          field_generators_.get(sorted_fields[i]);
      field_generator.GenerateFieldDescription(printer, need_defaults);
      if (field_generator.needs_textformat_name_support()) {
        text_format_decode_data.AddString(sorted_fields[i]->number(),
                                          field_generator.generated_objc_name(),
                                          field_generator.raw_field_name());
      }
    }
    printer->Outdent();
    printer->Print("};\n");
    printer->Outdent();
    printer->Outdent();
  }

  absl::flat_hash_map<absl::string_view, std::string> vars;
  vars["classname"] = class_name_;
  vars["message_name"] = descriptor_->name();
  vars["class_reference"] = ObjCClass(class_name_);
  vars["file_description_name"] = file_description_name_;
  vars["fields"] = has_fields ? "fields" : "NULL";
  if (has_fields) {
    vars["fields_count"] = absl::StrCat("(uint32_t)(sizeof(fields) / sizeof(",
                                        field_description_type, "))");
  } else {
    vars["fields_count"] = "0";
  }

  std::vector<std::string> init_flags;
  init_flags.push_back("GPBDescriptorInitializationFlag_UsesClassRefs");
  init_flags.push_back("GPBDescriptorInitializationFlag_Proto3OptionalKnown");
  init_flags.push_back(
      "GPBDescriptorInitializationFlag_ClosedEnumSupportKnown");
  if (need_defaults) {
    init_flags.push_back("GPBDescriptorInitializationFlag_FieldsWithDefault");
  }
  if (descriptor_->options().message_set_wire_format()) {
    init_flags.push_back("GPBDescriptorInitializationFlag_WireFormat");
  }
  vars["init_flags"] =
      BuildFlagsString(FLAGTYPE_DESCRIPTOR_INITIALIZATION, init_flags);

  // clang-format off
  printer->Print(
      vars,
      "    GPBDescriptor *localDescriptor =\n"
      "        [GPBDescriptor allocDescriptorForClass:$class_reference$\n"
      "                                   messageName:@\"$message_name$\"\n"
      "                               fileDescription:&$file_description_name$\n"
      "                                        fields:$fields$\n"
      "                                    fieldCount:$fields_count$\n"
      "                                   storageSize:sizeof($classname$__storage_)\n"
      "                                         flags:$init_flags$];\n");
  // clang-format on
  if (!oneof_generators_.empty()) {
    printer->Print("    static const char *oneofs[] = {\n");
    for (const auto& generator : oneof_generators_) {
      printer->Print("      \"$name$\",\n", "name",
                     generator->DescriptorName());
    }
    printer->Print(
        // clang-format off
        "    };\n"
        "    [localDescriptor setupOneofs:oneofs\n"
        "                           count:(uint32_t)(sizeof(oneofs) / sizeof(char*))\n"
        "                   firstHasIndex:$first_has_index$];\n",
        // clang-format on
        "first_has_index", oneof_generators_[0]->HasIndexAsString());
  }
  if (text_format_decode_data.num_entries() != 0) {
    const std::string text_format_data_str(text_format_decode_data.Data());
    // clang-format off
    printer->Print(
        "#if !GPBOBJC_SKIP_MESSAGE_TEXTFORMAT_EXTRAS\n"
        "    static const char *extraTextFormatInfo =");
    // clang-format on
    static const int kBytesPerLine = 40;  // allow for escaping
    for (int i = 0; i < text_format_data_str.size(); i += kBytesPerLine) {
      printer->Print("\n        \"$data$\"", "data",
                     EscapeTrigraphs(absl::CEscape(
                         text_format_data_str.substr(i, kBytesPerLine))));
    }
    // clang-format off
    printer->Print(
        ";\n"
        "    [localDescriptor setupExtraTextInfo:extraTextFormatInfo];\n"
        "#endif  // !GPBOBJC_SKIP_MESSAGE_TEXTFORMAT_EXTRAS\n");
    // clang-format on
  }
  if (!sorted_extensions.empty()) {
    printer->Print("    static const GPBExtensionRange ranges[] = {\n");
    for (const auto& extension_range : sorted_extensions) {
      printer->Print("      { .start = $start$, .end = $end$ },\n", "start",
                     absl::StrCat(extension_range.start), "end",
                     absl::StrCat(extension_range.end));
    }
    // clang-format off
    printer->Print(
        "    };\n"
        "    [localDescriptor setupExtensionRanges:ranges\n"
        "                                    count:(uint32_t)(sizeof(ranges) / sizeof(GPBExtensionRange))];\n");
    // clang-format on
  }
  if (descriptor_->containing_type() != nullptr) {
    std::string containing_class = ClassName(descriptor_->containing_type());
    std::string parent_class_ref = ObjCClass(containing_class);
    printer->Print(
        // clang-format off
        "    [localDescriptor setupContainingMessageClass:$parent_class_ref$];\n",
        // clang-format on
        "parent_class_ref", parent_class_ref);
  }
  // clang-format off
  printer->Print(
      "    #if defined(DEBUG) && DEBUG\n"
      "      NSAssert(descriptor == nil, @\"Startup recursed!\");\n"
      "    #endif  // DEBUG\n"
      "    descriptor = localDescriptor;\n"
      "  }\n"
      "  return descriptor;\n"
      "}\n\n"
      "@end\n\n");
  // clang-format on

  if (!deprecated_attribute_.empty()) {
    // clang-format off
    printer->Print(
        "#pragma clang diagnostic pop\n"
        "\n");
    // clang-format on
  }

  for (int i = 0; i < descriptor_->field_count(); i++) {
    field_generators_.get(descriptor_->field(i))
        .GenerateCFunctionImplementations(printer);
  }

  for (const auto& generator : oneof_generators_) {
    generator->GenerateClearFunctionImplementation(printer);
  }
}

}  // namespace objectivec
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
