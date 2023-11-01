// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/objectivec/enum.h"

#include <cstddef>
#include <limits>
#include <string>

#include "absl/container/flat_hash_set.h"
#include "absl/strings/escaping.h"
#include "absl/strings/str_cat.h"
#include "google/protobuf/compiler/objectivec/helpers.h"
#include "google/protobuf/compiler/objectivec/names.h"
#include "google/protobuf/compiler/objectivec/options.h"
#include "google/protobuf/compiler/objectivec/text_format_decode_data.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/io/printer.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace objectivec {
namespace {
std::string SafelyPrintIntToCode(int v) {
  if (v == std::numeric_limits<int>::min()) {
    // Some compilers try to parse -2147483648 as two tokens and then get spicy
    // about the fact that +2147483648 cannot be represented as an int.
    return absl::StrCat(v + 1, " - 1");
  } else {
    return absl::StrCat(v);
  }
}
}  // namespace

EnumGenerator::EnumGenerator(const EnumDescriptor* descriptor,
                             const GenerationOptions& generation_options)
    : descriptor_(descriptor), name_(EnumName(descriptor_)) {
  // Track the names for the enum values, and if an alias overlaps a base
  // value, skip making a name for it. Likewise if two alias overlap, the
  // first one wins.
  // The one gap in this logic is if two base values overlap, but for that
  // to happen you have to have "Foo" and "FOO" or "FOO_BAR" and "FooBar",
  // and if an enum has that, it is already going to be confusing and a
  // compile error is just fine.
  // The values are still tracked to support the reflection apis and
  // TextFormat handing since they are different there.
  absl::flat_hash_set<std::string> value_names;

  for (int i = 0; i < descriptor_->value_count(); i++) {
    const EnumValueDescriptor* value = descriptor_->value(i);
    const EnumValueDescriptor* canonical_value =
        descriptor_->FindValueByNumber(value->number());

    if (value == canonical_value) {
      base_values_.push_back(value);
      value_names.insert(EnumValueName(value));
    } else {
      if (!value_names.insert(EnumValueName(value)).second) {
        alias_values_to_skip_.insert(value);
      }
    }
    all_values_.push_back(value);
  }
}

void EnumGenerator::GenerateHeader(io::Printer* printer) const {
  // Swift 5 included SE0192 "Handling Future Enum Cases"
  //   https://github.com/apple/swift-evolution/blob/master/proposals/0192-non-exhaustive-enums.md
  // Since a .proto file can get new values added to an enum at any time, they
  // are effectively "non-frozen". Even with an EnumType::Open there is support
  // for the unknown value, an edit to the file can always add a new value
  // moving something from unknown to known. Since Swift is ABI stable, it also
  // means a binary could contain Swift compiled against one version of the
  // .pbobjc.h file, but finally linked against an enum with more cases. So the
  // Swift code will always have to treat ObjC Proto Enums as "non-frozen". The
  // default behavior in SE0192 is for all objc enums to be "non-frozen" unless
  // marked as otherwise, so this means this generation doesn't have to bother
  // with the `enum_extensibility` clang attribute, as the default will be what
  // is needed.

  printer->Emit(
      {
          {"enum_name", name_},
          {"enum_comments", [&] { EmitCommentsString(printer, descriptor_); }},
          {"enum_deprecated_attribute",
           GetOptionalDeprecatedAttribute(descriptor_, descriptor_->file())},
          {"maybe_unknown_value",
           [&] {
             if (descriptor_->is_closed()) return;

             // Include the unknown value.
             printer->Emit(R"objc(
               /**
                * Value used if any message's field encounters a value that is not defined
                * by this enum. The message will also have C functions to get/set the rawValue
                * of the field.
                **/
               $enum_name$_GPBUnrecognizedEnumeratorValue = kGPBUnrecognizedEnumeratorValue,
             )objc");
           }},
          {"enum_values",
           [&] {
             CommentStringFlags comment_flags = kCommentStringFlags_None;
             for (const auto* v : all_values_) {
               if (alias_values_to_skip_.contains(v)) continue;
               printer->Emit(
                   {
                       {"name", EnumValueName(v)},
                       {"comments",
                        [&] { EmitCommentsString(printer, v, comment_flags); }},
                       {"deprecated_attribute",
                        GetOptionalDeprecatedAttribute(v)},
                       {"value", SafelyPrintIntToCode(v->number())},

                   },
                   R"objc(
                     $comments$
                     $name$$ deprecated_attribute$ = $value$,
                   )objc");
               comment_flags = kCommentStringFlags_AddLeadingNewline;
             }
           }},
      },
      R"objc(
        #pragma mark - Enum $enum_name$

        $enum_comments$
        typedef$ enum_deprecated_attribute$ GPB_ENUM($enum_name$) {
          $maybe_unknown_value$
          $enum_values$
        };

        GPBEnumDescriptor *$enum_name$_EnumDescriptor(void);

        /**
         * Checks to see if the given value is defined by the enum or was not known at
         * the time this source was generated.
         **/
        BOOL $enum_name$_IsValidValue(int32_t value);
      )objc");
  printer->Emit("\n");
}

void EnumGenerator::GenerateSource(io::Printer* printer) const {
  // Note: For the TextFormat decode info, we can't use the enum value as
  // the key because protocol buffer enums have 'allow_alias', which lets
  // a value be used more than once. Instead, the index into the list of
  // enum value descriptions is used. Note: start with -1 so the first one
  // will be zero.
  TextFormatDecodeData text_format_decode_data;
  int enum_value_description_key = -1;
  std::string text_blob;

  for (const auto* v : all_values_) {
    ++enum_value_description_key;
    std::string short_name(EnumValueShortName(v));
    text_blob += short_name + '\0';
    if (UnCamelCaseEnumShortName(short_name) != v->name()) {
      text_format_decode_data.AddString(enum_value_description_key, short_name,
                                        v->name());
    }
  }

  printer->Emit(
      {{"name", name_},
       {"values_name_blob",
        [&] {
          static const int kBytesPerLine = 40;  // allow for escaping
          for (size_t i = 0; i < text_blob.size(); i += kBytesPerLine) {
            printer->Emit({{"data", EscapeTrigraphs(absl::CEscape(
                                        text_blob.substr(i, kBytesPerLine)))},
                           {"ending_semi",
                            (i + kBytesPerLine) < text_blob.size() ? "" : ";"}},
                          R"objc(
                            "$data$"$ending_semi$
                          )objc");
          }
        }},
       {"values",
        [&] {
          for (const auto* v : all_values_) {
            printer->Emit({{"value_name", EnumValueName(v)}},
                          R"objc(
                            $value_name$,
                          )objc");
          }
        }},
       {"maybe_extra_text_format_decl",
        [&] {
          if (text_format_decode_data.num_entries()) {
            printer->Emit({{"extraTextFormatInfo",
                            absl::CEscape(text_format_decode_data.Data())}},
                          R"objc(
                            static const char *extraTextFormatInfo = "$extraTextFormatInfo$";
                          )objc");
          }
        }},
       {"maybe_extraTextFormatInfo",
        // Could not find a better way to get this extra line inserted and
        // correctly formatted.
        (text_format_decode_data.num_entries() == 0
             ? ""
             : "\n                              "
               "extraTextFormatInfo:extraTextFormatInfo")},
       {"enum_flags", descriptor_->is_closed()
                          ? "GPBEnumDescriptorInitializationFlag_IsClosed"
                          : "GPBEnumDescriptorInitializationFlag_None"},
       {"enum_cases",
        [&] {
          for (const auto* v : base_values_) {
            printer->Emit({{"case_name", EnumValueName(v)}},
                          R"objc(
                            case $case_name$:
                          )objc");
          }
        }}},
      R"objc(
        #pragma mark - Enum $name$

        GPBEnumDescriptor *$name$_EnumDescriptor(void) {
          static _Atomic(GPBEnumDescriptor*) descriptor = nil;
          if (!descriptor) {
            GPB_DEBUG_CHECK_RUNTIME_VERSIONS();
            static const char *valueNames =
                $values_name_blob$
            static const int32_t values[] = {
                $values$
            };
            $maybe_extra_text_format_decl$
            GPBEnumDescriptor *worker =
                [GPBEnumDescriptor allocDescriptorForName:GPBNSStringifySymbol($name$)
                                               valueNames:valueNames
                                                   values:values
                                                    count:(uint32_t)(sizeof(values) / sizeof(int32_t))
                                             enumVerifier:$name$_IsValidValue
                                                    flags:$enum_flags$$maybe_extraTextFormatInfo$];
            GPBEnumDescriptor *expected = nil;
            if (!atomic_compare_exchange_strong(&descriptor, &expected, worker)) {
              [worker release];
            }
          }
          return descriptor;
        }

        BOOL $name$_IsValidValue(int32_t value__) {
          switch (value__) {
            $enum_cases$
              return YES;
            default:
              return NO;
          }
        }
      )objc");
  printer->Emit("\n");
}

}  // namespace objectivec
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
