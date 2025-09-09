// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/compiler/java/internal_helpers.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

#include "absl/log/absl_log.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/java/context.h"
#include "google/protobuf/compiler/java/doc_comment.h"
#include "google/protobuf/compiler/java/helpers.h"
#include "google/protobuf/compiler/java/name_resolver.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"

// Must be last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace compiler {
namespace java {
namespace {

int GetExperimentalJavaFieldTypeForSingular(const FieldDescriptor* field) {
  // j/c/g/protobuf/FieldType.java lists field types in a slightly different
  // order from FieldDescriptor::Type so we can't do a simple cast.
  //
  // TODO: Make j/c/g/protobuf/FieldType.java follow the same order.
  int result = field->type();
  if (result == FieldDescriptor::TYPE_GROUP) {
    return 17;
  } else if (result < FieldDescriptor::TYPE_GROUP) {
    return result - 1;
  } else {
    return result - 2;
  }
}

int GetExperimentalJavaFieldTypeForRepeated(const FieldDescriptor* field) {
  if (field->type() == FieldDescriptor::TYPE_GROUP) {
    return 49;
  } else {
    return GetExperimentalJavaFieldTypeForSingular(field) + 18;
  }
}

int GetExperimentalJavaFieldTypeForPacked(const FieldDescriptor* field) {
  int result = field->type();
  if (result < FieldDescriptor::TYPE_STRING) {
    return result + 34;
  } else if (result > FieldDescriptor::TYPE_BYTES) {
    return result + 30;
  } else {
    ABSL_LOG(FATAL) << field->full_name() << " can't be packed.";
    return 0;
  }
}
}  // namespace

int GetExperimentalJavaFieldType(const FieldDescriptor* field) {
  static const int kMapFieldType = 50;
  static const int kOneofFieldTypeOffset = 51;

  static const int kRequiredBit = 0x100;
  static const int kUtf8CheckBit = 0x200;
  static const int kCheckInitialized = 0x400;
  static const int kLegacyEnumIsClosedBit = 0x800;
  static const int kHasHasBit = 0x1000;
  int extra_bits = field->is_required() ? kRequiredBit : 0;
  if (field->type() == FieldDescriptor::TYPE_STRING && CheckUtf8(field)) {
    extra_bits |= kUtf8CheckBit;
  }
  if (field->is_required() || (GetJavaType(field) == JAVATYPE_MESSAGE &&
                               HasRequiredFields(field->message_type()))) {
    extra_bits |= kCheckInitialized;
  }
  if (HasHasbit(field)) {
    extra_bits |= kHasHasBit;
  }
  if (GetJavaType(field) == JAVATYPE_ENUM && !SupportUnknownEnumValue(field)) {
    extra_bits |= kLegacyEnumIsClosedBit;
  }

  if (field->is_map()) {
    if (!SupportUnknownEnumValue(MapValueField(field))) {
      const FieldDescriptor* value = field->message_type()->map_value();
      if (GetJavaType(value) == JAVATYPE_ENUM) {
        extra_bits |= kLegacyEnumIsClosedBit;
      }
    }
    return kMapFieldType | extra_bits;
  } else if (field->is_packed()) {
    return GetExperimentalJavaFieldTypeForPacked(field) | extra_bits;
  } else if (field->is_repeated()) {
    return GetExperimentalJavaFieldTypeForRepeated(field) | extra_bits;
  } else if (IsRealOneof(field)) {
    return (GetExperimentalJavaFieldTypeForSingular(field) +
            kOneofFieldTypeOffset) |
           extra_bits;
  } else {
    return GetExperimentalJavaFieldTypeForSingular(field) | extra_bits;
  }
}

void GenerateLarge(
    io::Printer* printer, const EnumDescriptor* descriptor,
    std::vector<const EnumValueDescriptor*> canonical_values,
    std::vector<
        std::pair<const EnumValueDescriptor*, const EnumValueDescriptor*>>
        aliases,
    bool immutable_api, Context* context, ClassNameResolver* name_resolver) {
  // Max number of constants in a generated Java class.
  constexpr int kMaxEnums = 1000;
  int interface_count = ceil((double)canonical_values.size() / kMaxEnums);

  printer->Emit(
      {{"classname", descriptor->name()},
       {"static", IsOwnFile(descriptor, immutable_api) ? " " : " static "},
       {"deprecation",
        descriptor->options().deprecated() ? "@java.lang.Deprecated" : ""},
       {"canonical_values_size", absl::StrCat(canonical_values.size())},
       {"proto_enum_class", context->EnforceLite()
                                ? "com.google.protobuf.Internal.EnumLite"
                                : "com.google.protobuf.ProtocolMessageEnum"},
       {"proto_non_null_annotation",
        [&] {
          if (!context->options().opensource_runtime) {
            printer->Emit(R"(
              @com.google.protobuf.Internal.ProtoNonnullApi
            )");
          }
        }},
       {"method_return_null_annotation",
        [&] {
          if (!context->options().opensource_runtime) {
            printer->Emit(R"(
              @com.google.protobuf.Internal.ProtoMethodMayReturnNull
            )");
          }
        }},
       {"interface_names",
        [&] {
          std::vector<std::string> interface_names;
          interface_names.reserve(interface_count);
          for (int count = 0; count < interface_count; count++) {
            interface_names.push_back(
                absl::StrFormat("%s%d", descriptor->name(), count));
          }
          printer->Emit(
              {{"interface_names", absl::StrJoin(interface_names, ", ")}},
              R"($interface_names$)");
        }},
       {"gen_code_version_validator",
        [&] {
          if (!context->EnforceLite()) {
            PrintGencodeVersionValidator(printer,
                                         context->options().opensource_runtime,
                                         descriptor->name());
          }
        }},
       {"aliases",
        [&] {
          for (int i = 0; i < aliases.size(); i++) {
            WriteEnumValueDocComment(printer, aliases[i].first,
                                     context->options());
            printer->Emit({{"name", aliases[i].first->name()},
                           {"canonical_name", aliases[i].second->name()}},
                          R"(
                  public static final $classname$ $name$ = $canonical_name$;
                )");
            printer->Annotate("name", canonical_values[i]);
          }
        }},
       {"get_number_func",
        [&] {
          if (!descriptor->is_closed()) {
            printer->Emit(R"(
                if (this == UNRECOGNIZED) {
                  throw new java.lang.IllegalArgumentException(
                    "Can't get the number of an unknown enum value.");
                }
              )");
          }
          printer->Emit(R"(
            return value;
          )");
        }},
       {"deprecated_value_of_func",
        [&] {
          if (context->options().opensource_runtime) {
            printer->Emit(R"(
              /**
               * @param value The numeric wire value of the corresponding enum entry.
               * @return The enum associated with the given numeric wire value.
               * @deprecated Use {@link #forNumber(int)} instead.
               */
              @java.lang.Deprecated
              public static $classname$ valueOf(int value) {
                return forNumber(value);
              }
          )");
          }
        }},
       {"for_number_func",
        [&] {
          for (int count = 0; count < interface_count; count++) {
            printer->Emit({{"count", absl::StrCat(count)}}, R"(
                if ($classname$$count$.forNumber$count$(value) != null) {
                  return $classname$$count$.forNumber$count$(value);
                }
                )");
          }
          printer->Emit(R"(
                return null;
                )");
        }},
       {"value_of_func",
        [&] {
          for (int count = 0; count < interface_count; count++) {
            printer->Emit({{"count", absl::StrCat(count)}}, R"(
              if ($classname$$count$.valueOf$count$(name) != null) {
                return $classname$$count$.valueOf$count$(name);
              }
            )");
          }
          printer->Emit(R"(
              throw new java.lang.IllegalArgumentException(
                "No enum constant $classname$." + name);
          )");
        }},
       {"values_func",
        [&] {
          printer->Emit({{"size", absl::StrCat(descriptor->value_count() + 1)}},
                        R"(
              int ordinal = 0;
              $classname$[] values = new $classname$[$size$];
          )");

          for (int count = 0; count < interface_count; count++) {
            printer->Emit({{"count", absl::StrCat(count)}}, R"(
              $classname$[] values$count$ = $classname$$count$.values$count$();
              System.arraycopy(values$count$, 0, values, ordinal, values$count$.length);
              ordinal += values$count$.length;
            )");
          }
          printer->Emit(
              {{"last_index", absl::StrCat(descriptor->value_count())}}, R"(
              values[$last_index$] = UNRECOGNIZED;
              return values;
          )");
        }},
       {"enum_verifier_func",
        [&] {
          if (context->EnforceLite()) {
            printer->Emit(R"(
                public static com.google.protobuf.Internal.EnumVerifier
                    internalGetVerifier() {
                  return $classname$Verifier.INSTANCE;
                }

                private static final class $classname$Verifier implements
                     com.google.protobuf.Internal.EnumVerifier {
                        static final com.google.protobuf.Internal.EnumVerifier
                          INSTANCE = new $classname$Verifier();
                        @java.lang.Override
                        public boolean isInRange(int number) {
                          return $classname$.forNumber(number) != null;
                        }
                      };
            )");
          }
        }},
       {"descriptor_methods",
        [&] {
          // -----------------------------------------------------------------
          // Reflection

          if (HasDescriptorMethods(descriptor, context->EnforceLite())) {
            printer->Print(
                "public final "
                "com.google.protobuf.Descriptors.EnumValueDescriptor\n"
                "    getValueDescriptor() {\n");
            if (!descriptor->is_closed()) {
              printer->Print(
                  "  if (this == UNRECOGNIZED) {\n"
                  "    throw new java.lang.IllegalStateException(\n"
                  "        \"Can't get the descriptor of an unrecognized enum "
                  "value.\");\n"
                  "  }\n");
            }
            printer->Print(
                "  return getDescriptor().getValues().get(index());\n"
                "}\n"
                "public final com.google.protobuf.Descriptors.EnumDescriptor\n"
                "    getDescriptorForType() {\n"
                "  return getDescriptor();\n"
                "}\n"
                "public static final "
                "com.google.protobuf.Descriptors.EnumDescriptor\n"
                "    getDescriptor() {\n");

            // TODO:  Cache statically?  Note that we can't access
            // descriptors
            //   at module init time because it wouldn't work with
            //   descriptor.proto, but we can cache the value the first time
            //   getDescriptor() is called.
            if (descriptor->containing_type() == nullptr) {
              // The class generated for the File fully populates the descriptor
              // with extensions in both the mutable and immutable cases. (In
              // the mutable api this is accomplished by attempting to load the
              // immutable outer class).
              printer->Print(
                  "  return "
                  "$file$.getDescriptor().getEnumTypes().get($index$);\n",
                  "file",
                  name_resolver->GetClassName(descriptor->file(),
                                              immutable_api),
                  "index", absl::StrCat(descriptor->index()));
            } else {
              printer->Print(
                  "  return "
                  "$parent$.$descriptor$.getEnumTypes().get($index$);\n",
                  "parent",
                  name_resolver->GetClassName(descriptor->containing_type(),
                                              immutable_api),
                  "descriptor",
                  descriptor->containing_type()
                          ->options()
                          .no_standard_descriptor_accessor()
                      ? "getDefaultInstance().getDescriptorForType()"
                      : "getDescriptor()",
                  "index", absl::StrCat(descriptor->index()));
            }
            printer->Print(
                "}\n"
                "\n");

            printer->Print(
                "\n"
                "public static $classname$ valueOf(\n"
                "    com.google.protobuf.Descriptors.EnumValueDescriptor desc) "
                "{\n"
                "  if (desc.getType() != getDescriptor()) {\n"
                "    throw new java.lang.IllegalArgumentException(\n"
                "      \"EnumValueDescriptor is not for this type.\");\n"
                "  }\n",
                "classname", descriptor->name());
            if (!descriptor->is_closed()) {
              printer->Print(
                  "  if (desc.getIndex() == -1) {\n"
                  "    return UNRECOGNIZED;\n"
                  "  }\n");
            }
            printer->Print(
                "  return values()[desc.getIndex()];\n"
                "}\n"
                "\n");
          }
        }}},
      R"(
        $proto_non_null_annotation$
        $deprecation$
        public$static$final class $classname$
          implements $proto_enum_class$, java.io.Serializable, $interface_names$ {
          static {
            $gen_code_version_validator$
          }

          public static final $classname$ UNRECOGNIZED = new $classname$(-1, $canonical_values_size$, "UNRECOGNIZED");

          $aliases$

          $deprecated_value_of_func$

          public final int getNumber() {
            $get_number_func$
          }

          /**
           * @param value The numeric wire value of the corresponding enum entry.
           * @return The enum associated with the given numeric wire value.
           */
          $method_return_null_annotation$
          public static $classname$ forNumber(int value) {
            $for_number_func$
          }

          /**
           * @param name The string name of the corresponding enum entry.
           * @return The enum associated with the given string name.
           */
          public static $classname$ valueOf(String name) {
            $value_of_func$
          }

          public static $classname$[] values() {
            $values_func$
          }

          private final int value;
          private final String name;
          private final int index;

          $classname$(int v, int i, String n) {
            this.value = v;
            this.index = i;
            this.name = n;
          }

          public int index() {
            return index;
          }

          public int value() {
            return value;
          }

          public String name() {
            return name;
          }

          // For Kotlin code.
          public String getName() {
            return name;
          }

          @java.lang.Override
          public String toString() {
            return name;
          }

          public static com.google.protobuf.Internal.EnumLiteMap<$classname$> internalGetValueMap() {
            return internalValueMap;
          }

          private static final com.google.protobuf.Internal.EnumLiteMap<
            $classname$> internalValueMap =
              new com.google.protobuf.Internal.EnumLiteMap<$classname$>() {
                public $classname$ findValueByNumber(int number) {
                  return $classname$.forNumber(number);
                }
              };

          $enum_verifier_func$

          $descriptor_methods$
        }

        )");

  for (int count = 0; count < interface_count; count++) {
    int start = count * kMaxEnums;
    printer->Emit(
        {{"classname", descriptor->name()},
         {"count", absl::StrCat(count)},
         {"method_return_null_annotation",
          [&] {
            if (!context->options().opensource_runtime) {
              printer->Emit(R"(
                          @com.google.protobuf.Internal.ProtoMethodMayReturnNull
                        )");
            }
          }},
         {"enums",
          [&] {
            for (int i = start;
                 i < std::min(start + kMaxEnums, (int)canonical_values.size());
                 i++) {
              WriteEnumValueDocComment(printer, canonical_values[i],
                                       context->options());
              printer->Emit(
                  {
                      {"name", canonical_values[i]->name()},
                      {"number", absl::StrCat(canonical_values[i]->number())},
                      {"index", absl::StrCat(canonical_values[i]->index())},
                      {"deprecation",
                       canonical_values[i]->options().deprecated()
                           ? "@java.lang.Deprecated"
                           : ""},
                  },
                  R"(
                    $deprecation$
                    public static final $classname$ $name$ = new $classname$($number$, $index$, "$name$");

                    $deprecation$
                    public static final int $name$_VALUE = $number$;
                  )");
            }
          }},
         {"value_of_func",
          [&] {
            printer->Emit({{"cases",
                            [&] {
                              for (int i = start;
                                   i < std::min(start + kMaxEnums,
                                                (int)canonical_values.size());
                                   i++) {
                                printer->Emit(
                                    {{"name", canonical_values[i]->name()}},
                                    R"(
                                    case "$name$": return $name$;
                                    )");
                              }
                            }}},
                          R"(
                          switch (name) {
                            $cases$
                            default: return null;
                          }
                          )");
          }},
         {"for_number_func",
          [&] {
            printer->Emit(
                {{"cases",
                  [&] {
                    for (int i = start;
                         i < std::min(start + kMaxEnums,
                                      (int)canonical_values.size());
                         i++) {
                      printer->Emit(
                          {{"name", canonical_values[i]->name()},
                           {"number",
                            absl::StrCat(canonical_values[i]->number())}},
                          R"(
                            case $number$: return $name$;
                          )");
                    }
                  }}},
                R"(
                  switch (value) {
                    $cases$
                    default: return null;
                  }
                )");
          }},
         {"values_func",
          [&] {
            printer->Emit({{"values",
                            [&] {
                              std::vector<absl::string_view> values;
                              for (int i = start;
                                   i < std::min(start + kMaxEnums,
                                                (int)canonical_values.size());
                                   i++) {
                                values.push_back(canonical_values[i]->name());
                              }
                              printer->Emit(
                                  {{"values", absl::StrJoin(values, ", ")}},
                                  R"(
                                  $values$
                                  )");
                            }}},
                          R"(
                          return new $classname$[] {
                            $values$
                          };
                          )");
          }}},
        R"(
          interface $classname$$count$ {

            $enums$

            /**
             * @param value The numeric wire value of the corresponding enum entry.
             * @return The enum associated with the given numeric wire value.
             */
            $method_return_null_annotation$
            public static $classname$ forNumber$count$(int value) {
              $for_number_func$
            }

            /**
             * @param name The string name of the corresponding enum entry.
             * @return The enum associated with the given string name.
             */
            $method_return_null_annotation$
            public static $classname$ valueOf$count$(String name) {
              $value_of_func$
            }

            public static $classname$[] values$count$() {
              $values_func$
            }
          }
        )");
  }
}

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
