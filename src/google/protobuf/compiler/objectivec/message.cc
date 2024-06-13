// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/objectivec/message.h"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "absl/container/btree_set.h"
#include "absl/container/flat_hash_set.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/strings/escaping.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/objectivec/extension.h"
#include "google/protobuf/compiler/objectivec/field.h"
#include "google/protobuf/compiler/objectivec/helpers.h"
#include "google/protobuf/compiler/objectivec/names.h"
#include "google/protobuf/compiler/objectivec/oneof.h"
#include "google/protobuf/compiler/objectivec/options.h"
#include "google/protobuf/compiler/objectivec/tf_decode_data.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/io/printer.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace objectivec {

namespace {

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
    return a->start_number() < b->start_number();
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
      if (!result.empty() && result.back().end == ext->start_number()) {
        result.back().end = ext->end_number();
      } else {
        result.emplace_back(ext->start_number(), ext->end_number());
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
                                   const Descriptor* descriptor,
                                   const GenerationOptions& generation_options)
    : file_description_name_(file_description_name),
      descriptor_(descriptor),
      generation_options_(generation_options),
      field_generators_(descriptor, generation_options),
      class_name_(ClassName(descriptor_)),
      deprecated_attribute_(
          GetOptionalDeprecatedAttribute(descriptor, descriptor->file())) {
  ABSL_DCHECK(!descriptor->options().map_entry())
      << "error: MessageGenerator create of a map<>!";
  for (int i = 0; i < descriptor_->real_oneof_decl_count(); i++) {
    oneof_generators_.push_back(std::make_unique<OneofGenerator>(
        descriptor_->real_oneof_decl(i), generation_options));
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
    const FieldDescriptor* extension = descriptor_->extension(i);
    if (!generation_options_.strip_custom_options ||
        !ExtensionIsCustomOption(extension)) {
      extension_generators->push_back(std::make_unique<ExtensionGenerator>(
          class_name_, extension, generation_options_));
      extension_generators_.push_back(extension_generators->back().get());
    }
  }
}

void MessageGenerator::DetermineForwardDeclarations(
    absl::btree_set<std::string>* fwd_decls,
    bool include_external_types) const {
  for (int i = 0; i < descriptor_->field_count(); i++) {
    const FieldDescriptor* fieldDescriptor = descriptor_->field(i);
    field_generators_.get(fieldDescriptor)
        .DetermineForwardDeclarations(fwd_decls, include_external_types);
  }
}

void MessageGenerator::DetermineNeededFiles(
    absl::flat_hash_set<const FileDescriptor*>* deps) const {
  for (int i = 0; i < descriptor_->field_count(); i++) {
    const FieldDescriptor* fieldDescriptor = descriptor_->field(i);
    field_generators_.get(fieldDescriptor).DetermineNeededFiles(deps);
  }
}

void MessageGenerator::DetermineObjectiveCClassDefinitions(
    absl::btree_set<std::string>* fwd_decls) const {
  // Forward declare this class, as a linker symbol, so the symbol can be used
  // to reference the class instead of calling +class later.
  fwd_decls->insert(ObjCClassDeclaration(class_name_));

  for (int i = 0; i < descriptor_->field_count(); i++) {
    const FieldDescriptor* fieldDescriptor = descriptor_->field(i);
    field_generators_.get(fieldDescriptor)
        .DetermineObjectiveCClassDefinitions(fwd_decls);
  }

  const Descriptor* containing_descriptor = descriptor_->containing_type();
  if (containing_descriptor != nullptr) {
    std::string containing_class = ClassName(containing_descriptor);
    fwd_decls->insert(ObjCClassDeclaration(containing_class));
  }
}

void MessageGenerator::GenerateMessageHeader(io::Printer* printer) const {
  auto vars = printer->WithVars({{"classname", class_name_}});
  printer->Emit(
      {io::Printer::Sub("deprecated_attribute", deprecated_attribute_)
           .WithSuffix(";"),
       {"message_comments",
        [&] {
          EmitCommentsString(printer, generation_options_, descriptor_,
                             kCommentStringFlags_ForceMultiline);
        }},
       {"message_fieldnum_enum",
        [&] {
          if (descriptor_->field_count() == 0) return;
          printer->Emit(R"objc(
            typedef GPB_ENUM($classname$_FieldNumber) {
              $message_fieldnum_enum_values$,
            };
          )objc");
          printer->Emit("\n");
        }},
       {"message_fieldnum_enum_values",
        [&] {
          std::unique_ptr<const FieldDescriptor*[]> sorted_fields(
              SortFieldsByNumber(descriptor_));
          for (size_t i = 0; i < (size_t)descriptor_->field_count(); i++) {
            field_generators_.get(sorted_fields[i])
                .GenerateFieldNumberConstant(printer);
          }
        }},
       {"oneof_enums",
        [&] {
          for (const auto& generator : oneof_generators_) {
            generator->GenerateCaseEnum(printer);
          }
        }},
       {"message_properties",
        [&] {
          std::vector<char> seen_oneofs(oneof_generators_.size(), 0);
          for (int i = 0; i < descriptor_->field_count(); i++) {
            const FieldDescriptor* field = descriptor_->field(i);
            const OneofDescriptor* oneof = field->real_containing_oneof();
            if (oneof) {
              const size_t oneof_index = (size_t)oneof->index();
              if (!seen_oneofs[oneof_index]) {
                seen_oneofs[oneof_index] = 1;
                oneof_generators_[oneof_index]
                    ->GeneratePublicCasePropertyDeclaration(printer);
              }
            }
            field_generators_.get(field).GeneratePropertyDeclaration(printer);
          }
        }},
       {"wkt_extra",
        [&] {
          if (!IsWKTWithObjCCategory(descriptor_)) {
            return;
          }
          printer->Emit(R"objc(
            // NOTE: There are some Objective-C specific methods/properties in
            // GPBWellKnownTypes.h that will likey be useful.
          )objc");
          printer->Emit("\n");
        }}},
      R"objc(
        #pragma mark - $classname$

        $message_fieldnum_enum$
        $oneof_enums$
        $message_comments$
        $deprecated_attribute$;
        GPB_FINAL @interface $classname$ : GPBMessage

        $message_properties$
        $wkt_extra$
        @end
      )objc");
  printer->Emit("\n");

  for (int i = 0; i < descriptor_->field_count(); i++) {
    field_generators_.get(descriptor_->field(i))
        .GenerateCFunctionDeclarations(printer);
  }

  if (!oneof_generators_.empty()) {
    for (const auto& generator : oneof_generators_) {
      generator->GenerateClearFunctionDeclaration(printer);
    }
    printer->Emit("\n");
  }

  if (!extension_generators_.empty()) {
    printer->Emit({{"extension_info",
                    [&] {
                      for (const auto* generator : extension_generators_) {
                        generator->GenerateMembersHeader(printer);
                      }
                    }}},
                  R"objc(
                    @interface $classname$ (DynamicMethods)

                    $extension_info$
                    @end
                  )objc");
    printer->Emit("\n");
  }
}

void MessageGenerator::GenerateSource(io::Printer* printer) const {
  std::unique_ptr<const FieldDescriptor*[]> sorted_fields(
      SortFieldsByNumber(descriptor_));
  std::unique_ptr<const FieldDescriptor*[]> size_order_fields(
      SortFieldsByStorageSize(descriptor_));

  std::vector<SimpleExtensionRange> sorted_extensions(
      SimpleExtensionRange::Normalize(descriptor_));

  bool has_fields = descriptor_->field_count() > 0;
  bool need_defaults = field_generators_.DoesAnyFieldHaveNonZeroDefault();

  TextFormatDecodeData text_format_decode_data;
  for (int i = 0; i < descriptor_->field_count(); ++i) {
    const FieldGenerator& field_generator =
        field_generators_.get(sorted_fields[i]);
    if (field_generator.needs_textformat_name_support()) {
      text_format_decode_data.AddString(sorted_fields[i]->number(),
                                        field_generator.generated_objc_name(),
                                        field_generator.raw_field_name());
    }
  }

  const absl::string_view field_description_type(
      need_defaults ? "GPBMessageFieldDescriptionWithDefault"
                    : "GPBMessageFieldDescription");

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

  printer->Emit(
      {{"classname", class_name_},
       {"clang_diagnostic_push",
        [&] {
          if (deprecated_attribute_.empty()) return;
          // No warnings when compiling the impl of this deprecated class.
          printer->Emit(R"objc(
            #pragma clang diagnostic push
            #pragma clang diagnostic ignored "-Wdeprecated-implementations"
          )objc");
          printer->Emit("\n");
        }},
       {"clang_diagnostic_pop",
        [&] {
          if (deprecated_attribute_.empty()) return;
          printer->Emit(R"objc(
            #pragma clang diagnostic pop
          )objc");
          printer->Emit("\n");
        }},
       {"property_implementation",
        [&] {
          for (const auto& generator : oneof_generators_) {
            generator->GeneratePropertyImplementation(printer);
          }
          for (int i = 0; i < descriptor_->field_count(); i++) {
            field_generators_.get(descriptor_->field(i))
                .GeneratePropertyImplementation(printer);
          }
        }},
       {"sizeof_has_storage", sizeof_has_storage_},
       {"storage_fields",
        [&] {
          for (int i = 0; i < descriptor_->field_count(); i++) {
            field_generators_.get(size_order_fields[i])
                .GenerateFieldStorageDeclaration(printer);
          }
        }},
       {"descriptor_startup",
        [&] {
          // If the message scopes extensions, trigger the root class
          // +initialize/+extensionRegistry as that is where the
          // runtime support for extensions lives.
          if (!extension_generators_.empty()) {
            printer->Emit(R"objc(
              // Start up the root class to support the scoped extensions.
              __unused Class rootStartup = [$root_class_name$ class];
            )objc");
          } else {
            // The Root class has a debug runtime check, so if not
            // starting that up, add the check.
            printer->Emit("GPB_DEBUG_CHECK_RUNTIME_VERSIONS();\n");
          }
        }},
       {"field_description_type", field_description_type},
       {"declare_fields_static",
        [&] {
          if (!has_fields) return;
          printer->Emit(R"objc(
            static $field_description_type$ fields[] = {
              $declare_fields_static_fields$,
            };
          )objc");
        }},
       {"declare_fields_static_fields",
        [&] {
          for (int i = 0; i < descriptor_->field_count(); ++i) {
            const FieldGenerator& field_generator =
                field_generators_.get(sorted_fields[i]);
            field_generator.GenerateFieldDescription(printer, need_defaults);
          }
        }},
       {"message_name", descriptor_->name()},
       {"class_reference", ObjCClass(class_name_)},
       {"file_description_name", file_description_name_},
       {"fields", has_fields ? "fields" : "NULL"},
       {"fields_count",
        has_fields ? absl::StrCat("(uint32_t)(sizeof(fields) / sizeof(",
                                  field_description_type, "))")
                   : "0"},
       {"init_flags",
        BuildFlagsString(FLAGTYPE_DESCRIPTOR_INITIALIZATION, init_flags)},
       {"oneof_support",
        [&] {
          if (oneof_generators_.empty()) return;
          printer->Emit(
              {{"first_has_index", oneof_generators_[0]->HasIndexAsString()}},
              R"objc(
                static const char *oneofs[] = {
                  $declare_oneof_static_oneofs$,
                };
                [localDescriptor setupOneofs:oneofs
                                       count:(uint32_t)(sizeof(oneofs) / sizeof(char*))
                               firstHasIndex:$first_has_index$];
              )objc");
        }},
       {"declare_oneof_static_oneofs",
        [&] {
          for (const auto& generator : oneof_generators_) {
            printer->Emit({{"name", generator->DescriptorName()}}, R"objc(
              "$name$",
            )objc");
          }
        }},
       {"text_format_decode_support",
        [&] {
          if (text_format_decode_data.num_entries() == 0) return;

          printer->Emit(R"objc(
            #if !GPBOBJC_SKIP_MESSAGE_TEXTFORMAT_EXTRAS
              static const char *extraTextFormatInfo =
                $text_format_decode_support_blob$
              [localDescriptor setupExtraTextInfo:extraTextFormatInfo];
            #endif  // !GPBOBJC_SKIP_MESSAGE_TEXTFORMAT_EXTRAS
          )objc");
        }},
       {"text_format_decode_support_blob",
        [&] {
          static const int kBytesPerLine = 40;  // allow for escaping
          const std::string text_format_data_str(
              text_format_decode_data.Data());
          for (size_t i = 0; i < text_format_data_str.size();
               i += kBytesPerLine) {
            printer->Emit(
                {{"data", EscapeTrigraphs(absl::CEscape(
                              text_format_data_str.substr(i, kBytesPerLine)))},
                 {"ending_semi",
                  (i + kBytesPerLine) < text_format_data_str.size() ? ""
                                                                    : ";"}},
                R"objc(
                  "$data$"$ending_semi$
                )objc");
          }
        }},
       {"extension_range_support",
        [&] {
          if (sorted_extensions.empty()) return;
          printer->Emit(
              {{"ranges",
                [&] {
                  for (const auto& extension_range : sorted_extensions) {
                    printer->Emit({{"start", extension_range.start},
                                   {"end", extension_range.end}},
                                  "{ .start = $start$, .end = $end$ },\n");
                  }
                }}},
              R"objc(
                static const GPBExtensionRange ranges[] = {
                  $ranges$,
                };
                [localDescriptor setupExtensionRanges:ranges
                                                count:(uint32_t)(sizeof(ranges) / sizeof(GPBExtensionRange))];
              )objc");
        }},
       {"containing_type_support",
        [&] {
          if (descriptor_->containing_type() == nullptr) return;
          std::string containing_class =
              ClassName(descriptor_->containing_type());
          std::string parent_class_ref = ObjCClass(containing_class);
          printer->Emit({{"parent_class_ref", parent_class_ref}}, R"objc(
            [localDescriptor setupContainingMessageClass:$parent_class_ref$];
          )objc");
        }}},
      R"objc(
        #pragma mark - $classname$

        $clang_diagnostic_push$;
        @implementation $classname$

        $property_implementation$

        typedef struct $classname$__storage_ {
          uint32_t _has_storage_[$sizeof_has_storage$];
          $storage_fields$,
        } $classname$__storage_;

        // This method is threadsafe because it is initially called
        // in +initialize for each subclass.
        + (GPBDescriptor *)descriptor {
          static GPBDescriptor *descriptor = nil;
          if (!descriptor) {
            $descriptor_startup$;
            $declare_fields_static$;
            GPBDescriptor *localDescriptor =
                [GPBDescriptor allocDescriptorForClass:$class_reference$
                                           messageName:@"$message_name$"
                                       fileDescription:&$file_description_name$
                                                fields:$fields$
                                            fieldCount:$fields_count$
                                           storageSize:sizeof($classname$__storage_)
                                                 flags:$init_flags$];
            $oneof_support$;
            $text_format_decode_support$;
            $extension_range_support$;
            $containing_type_support$;
            #if defined(DEBUG) && DEBUG
              NSAssert(descriptor == nil, @"Startup recursed!");
            #endif  // DEBUG
            descriptor = localDescriptor;
          }
          return descriptor;
        }

        @end

        $clang_diagnostic_pop$;
      )objc");

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
