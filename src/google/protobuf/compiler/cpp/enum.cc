// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/compiler/cpp/enum.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "absl/algorithm/container.h"
#include "absl/container/btree_map.h"
#include "absl/container/btree_set.h"
#include "absl/container/flat_hash_map.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/cpp/generator.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/cpp/names.h"
#include "google/protobuf/compiler/cpp/options.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/generated_enum_util.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {
namespace {
using Sub = ::google::protobuf::io::Printer::Sub;

absl::flat_hash_map<absl::string_view, std::string> EnumVars(
    const EnumDescriptor* enum_, const Options& options,
    const EnumValueDescriptor* min, const EnumValueDescriptor* max) {
  auto classname = ClassName(enum_, false);
  return {
      {"Enum", std::string(enum_->name())},
      {"Enum_", ResolveKnownNameCollisions(enum_->name(),
                                           enum_->containing_type() != nullptr
                                               ? NameContext::kMessage
                                               : NameContext::kFile,
                                           NameKind::kType)},
      {"Msg_Enum", classname},
      {"::Msg_Enum", QualifiedClassName(enum_, options)},
      {"Msg_Enum_",
       enum_->containing_type() == nullptr ? "" : absl::StrCat(classname, "_")},
      {"kMin", absl::StrCat(min->number())},
      {"kMax", absl::StrCat(max->number())},
      {"return_type", CppGenerator::GetResolvedSourceFeatures(*enum_)
                              .GetExtension(::pb::cpp)
                              .enum_name_uses_string_view()
                          ? "::absl::string_view"
                          : "const ::std::string&"},
  };
}

// The ARRAYSIZE constant is the max enum value plus 1. If the max enum value
// is kint32max, ARRAYSIZE will overflow. In such cases we should omit the
// generation of the ARRAYSIZE constant.
bool ShouldGenerateArraySize(const EnumDescriptor* descriptor) {
  int32_t max_value = descriptor->value(0)->number();
  for (int i = 0; i < descriptor->value_count(); i++) {
    if (descriptor->value(i)->number() > max_value) {
      max_value = descriptor->value(i)->number();
    }
  }
  return max_value != std::numeric_limits<int32_t>::max();
}
}  // namespace
EnumGenerator::ValueLimits EnumGenerator::ValueLimits::FromEnum(
    const EnumDescriptor* descriptor) {
  const EnumValueDescriptor* min_desc = descriptor->value(0);
  const EnumValueDescriptor* max_desc = descriptor->value(0);

  for (int i = 1; i < descriptor->value_count(); ++i) {
    if (descriptor->value(i)->number() < min_desc->number()) {
      min_desc = descriptor->value(i);
    }
    if (descriptor->value(i)->number() > max_desc->number()) {
      max_desc = descriptor->value(i);
    }
  }

  return EnumGenerator::ValueLimits{min_desc, max_desc};
}

EnumGenerator::EnumGenerator(const EnumDescriptor* descriptor,
                             const Options& options)
    : enum_(descriptor),
      options_(options),
      generate_array_size_(ShouldGenerateArraySize(descriptor)),
      has_reflection_(HasDescriptorMethods(enum_->file(), options_)),
      limits_(ValueLimits::FromEnum(enum_)) {
  // The conditions here for what is "sparse" are not rigorously
  // chosen.
  size_t values_range = static_cast<size_t>(limits_.max->number()) -
                        static_cast<size_t>(limits_.min->number());
  size_t total_values = static_cast<size_t>(enum_->value_count());
  should_cache_ = has_reflection_ &&
                  (values_range < 16u || values_range < total_values * 2u);

  sorted_unique_values_.reserve(enum_->value_count());
  for (int i = 0; i < enum_->value_count(); ++i) {
    sorted_unique_values_.push_back(enum_->value(i)->number());
  }
  // Sort and deduplicate
  absl::c_sort(sorted_unique_values_);
  sorted_unique_values_.erase(
      std::unique(sorted_unique_values_.begin(), sorted_unique_values_.end()),
      sorted_unique_values_.end());
}

void EnumGenerator::GenerateDefinition(io::Printer* p) {
  auto v1 = p->WithVars(EnumVars(enum_, options_, limits_.min, limits_.max));

  auto v2 = p->WithVars({
      Sub("Msg_Enum_Enum_MIN",
          absl::StrCat(p->LookupVar("Msg_Enum_"), enum_->name(), "_MIN"))
          .AnnotatedAs(enum_),
      Sub("Msg_Enum_Enum_MAX",
          absl::StrCat(p->LookupVar("Msg_Enum_"), enum_->name(), "_MAX"))
          .AnnotatedAs(enum_),
  });
  p->Emit(
      {
          {"values",
           [&] {
             for (int i = 0; i < enum_->value_count(); ++i) {
               const auto* value = enum_->value(i);
               p->Emit(
                   {
                       Sub("Msg_Enum_VALUE",
                           absl::StrCat(p->LookupVar("Msg_Enum_"),
                                        EnumValueName(value)))
                           .AnnotatedAs(value),
                       {"kNumber", Int32ToString(value->number())},
                       {"DEPRECATED",
                        value->options().deprecated() ? "[[deprecated]]" : ""},
                   },
                   R"cc(
                     $Msg_Enum_VALUE$$ DEPRECATED$ = $kNumber$,
                   )cc");
             }
           }},
          // Only emit annotations for the $Msg_Enum$ used in the `enum`
          // definition.
          Sub("Msg_Enum_annotated", p->LookupVar("Msg_Enum"))
              .AnnotatedAs(enum_),
          {"open_enum_sentinels",
           [&] {
             if (enum_->is_closed()) {
               return;
             }

             // For open enum semantics: generate min and max sentinel values
             // equal to INT32_MIN and INT32_MAX
             p->Emit({{"Msg_Enum_Msg_Enum_",
                       absl::StrCat(p->LookupVar("Msg_Enum"), "_",
                                    p->LookupVar("Msg_Enum_"))}},
                     R"cc(
                       $Msg_Enum_Msg_Enum_$INT_MIN_SENTINEL_DO_NOT_USE_ =
                           ::std::numeric_limits<::int32_t>::min(),
                       $Msg_Enum_Msg_Enum_$INT_MAX_SENTINEL_DO_NOT_USE_ =
                           ::std::numeric_limits<::int32_t>::max(),
                     )cc");
           }},
      },
      R"cc(
        enum $Msg_Enum_annotated$ : int {
          $values$,
          $open_enum_sentinels$,
        };

        $dllexport_decl $extern const uint32_t $Msg_Enum$_internal_data_[];
        inline constexpr $Msg_Enum$ $Msg_Enum_Enum_MIN$ =
            static_cast<$Msg_Enum$>($kMin$);
        inline constexpr $Msg_Enum$ $Msg_Enum_Enum_MAX$ =
            static_cast<$Msg_Enum$>($kMax$);
      )cc");

  // Generate the inline `_IsValid` function choosing the best implementation
  // for the values.
  GenerateIsValid(p);

  if (generate_array_size_) {
    p->Emit({Sub("Msg_Enum_Enum_ARRAYSIZE",
                 absl::StrCat(p->LookupVar("Msg_Enum_"), enum_->name(),
                              "_ARRAYSIZE"))
                 .AnnotatedAs(enum_)},
            R"cc(
              inline constexpr int $Msg_Enum_Enum_ARRAYSIZE$ = $kMax$ + 1;
            )cc");
  }

  if (has_reflection_) {
    p->Emit(R"(
      $dllexport_decl $const $pb$::EnumDescriptor* $nonnull$ $Msg_Enum$_descriptor();
    )");
  } else {
    p->Emit(R"cc(
      $return_type$ $Msg_Enum$_Name($Msg_Enum$ value);
    )cc");
  }

  // There are three possible implementations of $Enum$_Name() and
  // $Msg_Enum$_Parse(), depending on whether we are using a dense enum name
  // cache or not, and whether or not we have reflection. Very little code is
  // shared between the three, so it is split into three Emit() calls.

  // Can't use WithVars here, since callbacks can only be passed to Emit()
  // directly. Because this includes $Enum$, it must be a callback.
  auto write_assert = [&] {
    p->Emit(R"cc(
      static_assert(::std::is_same<T, $Msg_Enum$>::value ||
                        ::std::is_integral<T>::value,
                    "Incorrect type passed to $Enum$_Name().");
    )cc");
  };

  if (should_cache_ || !has_reflection_) {
    p->Emit({{"static_assert", write_assert}}, R"cc(
      template <typename T>
      $return_type$ $Msg_Enum$_Name(T value) {
        $static_assert$;
        return $Msg_Enum$_Name(static_cast<$Msg_Enum$>(value));
      }
    )cc");
    if (should_cache_) {
      // Using the NameOfEnum routine can be slow, so we create a small
      // cache of pointers to the std::string objects that reflection
      // stores internally.  This cache is a simple contiguous array of
      // pointers, so if the enum values are sparse, it's not worth it.
      p->Emit(R"cc(
        template <>
        inline $return_type$ $Msg_Enum$_Name($Msg_Enum$ value) {
          return $pbi$::NameOfDenseEnum<$Msg_Enum$_descriptor, $kMin$, $kMax$>(
              static_cast<int>(value));
        }
      )cc");
    }
  } else {
    p->Emit({{"static_assert", write_assert}}, R"cc(
      template <typename T>
      $return_type$ $Msg_Enum$_Name(T value) {
        $static_assert$;
        return $pbi$::NameOfEnum($Msg_Enum$_descriptor(), value);
      }
    )cc");
  }

  if (has_reflection_) {
    p->Emit(R"cc(
      inline bool $Msg_Enum$_Parse(
          //~
          ::absl::string_view name, $Msg_Enum$* $nonnull$ value) {
        return $pbi$::ParseNamedEnum<$Msg_Enum$>($Msg_Enum$_descriptor(), name,
                                                 value);
      }
    )cc");
  } else {
    p->Emit(R"cc(
      bool $Msg_Enum$_Parse(
          //~
          ::absl::string_view name, $Msg_Enum$* $nonnull$ value);
    )cc");
  }
}

void EnumGenerator::GenerateGetEnumDescriptorSpecializations(io::Printer* p) {
  auto v = p->WithVars(EnumVars(enum_, options_, limits_.min, limits_.max));

  p->Emit(R"cc(
    template <>
    struct is_proto_enum<$::Msg_Enum$> : std::true_type {};
  )cc");
  if (!has_reflection_) {
    return;
  }
  p->Emit(R"cc(
    template <>
    inline const EnumDescriptor* $nonnull$ GetEnumDescriptor<$::Msg_Enum$>() {
      return $::Msg_Enum$_descriptor();
    }
  )cc");
}


void EnumGenerator::GenerateSymbolImports(io::Printer* p) const {
  auto v = p->WithVars(EnumVars(enum_, options_, limits_.min, limits_.max));

  p->Emit({Sub("Enum_", p->LookupVar("Enum_")).AnnotatedAs(enum_)}, R"cc(
    using $Enum_$ = $Msg_Enum$;
  )cc");

  for (int j = 0; j < enum_->value_count(); ++j) {
    const auto* value = enum_->value(j);
    p->Emit(
        {
            Sub("VALUE", EnumValueName(enum_->value(j))).AnnotatedAs(value),
            {"DEPRECATED",
             value->options().deprecated() ? "[[deprecated]]" : ""},
        },
        R"cc(
          $DEPRECATED $static constexpr $Enum_$ $VALUE$ = $Msg_Enum$_$VALUE$;
        )cc");
  }

  p->Emit(
      {
          Sub("Enum_MIN", absl::StrCat(enum_->name(), "_MIN"))
              .AnnotatedAs(enum_),
          Sub("Enum_MAX", absl::StrCat(enum_->name(), "_MAX"))
              .AnnotatedAs(enum_),
      },
      R"cc(
        static inline bool $Enum$_IsValid(int value) {
          return $Msg_Enum$_IsValid(value);
        }
        static constexpr $Enum_$ $Enum_MIN$ = $Msg_Enum$_$Enum$_MIN;
        static constexpr $Enum_$ $Enum_MAX$ = $Msg_Enum$_$Enum$_MAX;
      )cc");

  if (generate_array_size_) {
    p->Emit(
        {
            Sub("Enum_ARRAYSIZE", absl::StrCat(enum_->name(), "_ARRAYSIZE"))
                .AnnotatedAs(enum_),
        },
        R"cc(
          static constexpr int $Enum_ARRAYSIZE$ = $Msg_Enum$_$Enum$_ARRAYSIZE;
        )cc");
  }

  if (has_reflection_) {
    p->Emit(R"(
      static inline const $pb$::EnumDescriptor* $nonnull$ $Enum$_descriptor() {
        return $Msg_Enum$_descriptor();
      }
    )");
  }

  p->Emit(R"cc(
    template <typename T>
    static inline $return_type$ $Enum$_Name(T value) {
      return $Msg_Enum$_Name(value);
    }
    static inline bool $Enum$_Parse(
        //~
        ::absl::string_view name, $Enum_$* $nonnull$ value) {
      return $Msg_Enum$_Parse(name, value);
    }
  )cc");
}

void EnumGenerator::GenerateIsValid(io::Printer* p) const {
  auto v = p->WithVars(EnumVars(enum_, options_, limits_.min, limits_.max));

  // For simple enums we skip the generic ValidateEnum call and use better
  // codegen. It matches the speed of the previous switch-based codegen.
  // For more complex enums we use the new algorithm with the encoded data.

  if (sorted_unique_values_.front() +
          static_cast<int64_t>(sorted_unique_values_.size()) - 1 ==
      sorted_unique_values_.back()) {
    // They are sequential. Do a simple range check.
    p->Emit({{"min", sorted_unique_values_.front()},
             {"max", sorted_unique_values_.back()}},
            R"cc(
              inline bool $Msg_Enum$_IsValid(int value) {
                return $min$ <= value && value <= $max$;
              }
            )cc");
  } else if (sorted_unique_values_.front() >= 0 &&
             sorted_unique_values_.back() < 64) {
    // Not sequential, but they fit in a 64-bit bitmap.
    uint64_t bitmap = 0;
    for (int n : sorted_unique_values_) {
      bitmap |= uint64_t{1} << n;
    }
    p->Emit({{"bitmap", bitmap}, {"max", sorted_unique_values_.back()}},
            R"cc(
              inline bool $Msg_Enum$_IsValid(int value) {
                return 0 <= value && value <= $max$ && (($bitmap$u >> value) & 1) != 0;
              }
            )cc");
  } else {
    // More complex struct. Use enum data structure for lookup.
    p->Emit(
        R"cc(
          inline bool $Msg_Enum$_IsValid(int value) {
            return $pbi$::ValidateEnum(value, $Msg_Enum$_internal_data_);
          }
        )cc");
  }
}

void EnumGenerator::GenerateMethods(int idx, io::Printer* p) {
  auto v = p->WithVars(EnumVars(enum_, options_, limits_.min, limits_.max));

  if (has_reflection_) {
    p->Emit({{"idx", idx}}, R"cc(
      const $pb$::EnumDescriptor* $nonnull$ $Msg_Enum$_descriptor() {
        $pbi$::AssignDescriptors(&$desc_table$);
        return $file_level_enum_descriptors$[$idx$];
      }
    )cc");
  }

  // Always generate the data array, even on the simple cases because someone
  // might be using it for TDP entries. If it is not used in the end, the linker
  // will drop it.
  p->Emit({{"encoded",
            [&] {
              for (uint32_t n :
                   google::protobuf::internal::GenerateEnumData(sorted_unique_values_)) {
                p->Emit({{"n", n}}, "$n$u, ");
              }
            }}},
          R"cc(
            PROTOBUF_CONSTINIT const uint32_t $Msg_Enum$_internal_data_[] = {
                $encoded$};
          )cc");

  if (!has_reflection_) {
    // In lite mode (where descriptors are unavailable), we generate separate
    // tables for mapping between enum names and numbers. The _entries table
    // contains the bulk of the data and is sorted by name, while
    // _entries_by_number is sorted by number and just contains pointers into
    // _entries. The two tables allow mapping from name to number and number to
    // name, both in time logarithmic in the number of enum entries. This could
    // probably be made faster, but for now the tables are intended to be simple
    // and compact.
    //
    // Enums with allow_alias = true support multiple entries with the same
    // numerical value. In cases where there are multiple names for the same
    // number, we treat the first name appearing in the .proto file as the
    // canonical one.

    absl::btree_map<std::string, int> name_to_number;
    absl::flat_hash_map<int, std::string> number_to_canonical_name;
    for (int i = 0; i < enum_->value_count(); ++i) {
      const auto* value = enum_->value(i);
      name_to_number.emplace(value->name(), value->number());

      // The same number may appear with multiple names, so we use emplace() to
      // let the first name win.
      number_to_canonical_name.emplace(value->number(), value->name());
    }

    // Build the offset table for the strings table.
    struct Offset {
      int number;
      size_t index, byte_offset, len;
    };
    std::vector<Offset> offsets;
    size_t index = 0;
    size_t offset = 0;
    for (const auto& e : name_to_number) {
      offsets.push_back(Offset{e.second, index, offset, e.first.size()});
      ++index;
      offset += e.first.size();
    }
    absl::c_stable_sort(offsets, [](const auto& a, const auto& b) {
      return a.byte_offset < b.byte_offset;
    });

    std::vector<Offset> offsets_by_number = offsets;
    absl::c_stable_sort(offsets_by_number, [](const auto& a, const auto& b) {
      return a.number < b.number;
    });

    offsets_by_number.erase(
        std::unique(
            offsets_by_number.begin(), offsets_by_number.end(),
            [](const auto& a, const auto& b) { return a.number == b.number; }),
        offsets_by_number.end());

    p->Emit(
        {
            {"num_unique", number_to_canonical_name.size()},
            {"num_declared", enum_->value_count()},
            {"names",
             // We concatenate all the names for a given enum into one big
             // string literal. If instead we store an array of string
             // literals, the linker seems to put all enum strings for a given
             // .proto file in the same section, which hinders its ability to
             // strip out unused strings.
             [&] {
               for (const auto& e : name_to_number) {
                 p->Emit({{"name", e.first}}, R"cc(
                   "$name$"
                 )cc");
               }
             }},
            {"entries",
             [&] {
               for (const auto& offset : offsets) {
                 p->Emit({{"number", offset.number},
                          {"offset", offset.byte_offset},
                          {"len", offset.len}},
                         R"cc(
                           {{&$Msg_Enum$_names[$offset$], $len$}, $number$},
                         )cc");
               }
             }},
            {"entries_by_number",
             [&] {
               for (const auto& offset : offsets_by_number) {
                 p->Emit({{"number", offset.number},
                          {"index", offset.index},
                          {"name", number_to_canonical_name[offset.number]}},
                         R"cc(
                           $index$,  // $number$ -> $name$
                         )cc");
               }
             }},
        },
        R"cc(
          static $pbi$::ExplicitlyConstructed<::std::string>
              $Msg_Enum$_strings[$num_unique$] = {};

          static const char $Msg_Enum$_names[] = {
              $names$,
          };

          static const $pbi$::EnumEntry $Msg_Enum$_entries[] = {
              $entries$,
          };

          static const int $Msg_Enum$_entries_by_number[] = {
              $entries_by_number$,
          };

          $return_type$ $Msg_Enum$_Name($Msg_Enum$ value) {
            static const bool kDummy = $pbi$::InitializeEnumStrings(
                $Msg_Enum$_entries, $Msg_Enum$_entries_by_number, $num_unique$,
                $Msg_Enum$_strings);
            (void)kDummy;

            int idx = $pbi$::LookUpEnumName($Msg_Enum$_entries,
                                            $Msg_Enum$_entries_by_number,
                                            $num_unique$, value);
            return idx == -1 ? $pbi$::GetEmptyString() : $Msg_Enum$_strings[idx].get();
          }

          bool $Msg_Enum$_Parse(::absl::string_view name, $Msg_Enum$* $nonnull$ value) {
            int int_value;
            bool success = $pbi$::LookUpEnumValue(
                $Msg_Enum$_entries, $num_declared$, name, &int_value);
            if (success) {
              *value = static_cast<$Msg_Enum$>(int_value);
            }
            return success;
          }
        )cc");
  }
}
}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
