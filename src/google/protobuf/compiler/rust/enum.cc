// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/rust/enum.h"

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/log/absl_check.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "google/protobuf/compiler/rust/context.h"
#include "google/protobuf/compiler/rust/naming.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace rust {

namespace {
// Constructs input for `EnumValues` from an enum descriptor.
std::vector<std::pair<absl::string_view, int32_t>> EnumValuesInput(
    const EnumDescriptor& desc) {
  std::vector<std::pair<absl::string_view, int32_t>> result;
  result.reserve(static_cast<size_t>(desc.value_count()));

  for (int i = 0; i < desc.value_count(); ++i) {
    result.emplace_back(desc.value(i)->name(), desc.value(i)->number());
  }

  return result;
}

}  // namespace

std::vector<RustEnumValue> EnumValues(
    absl::string_view enum_name,
    absl::Span<const std::pair<absl::string_view, int32_t>> values) {
  MultiCasePrefixStripper stripper(enum_name);

  absl::flat_hash_set<std::string> seen_by_name;
  absl::flat_hash_map<int32_t, RustEnumValue*> seen_by_number;
  std::vector<RustEnumValue> result;
  // The below code depends on pointer stability of elements in `result`;
  // this reserve must not be too low.
  result.reserve(values.size());
  seen_by_name.reserve(values.size());
  seen_by_number.reserve(values.size());

  for (const auto& name_and_number : values) {
    int32_t number = name_and_number.second;
    std::string rust_value_name =
        EnumValueRsName(stripper, name_and_number.first);

    if (seen_by_name.contains(rust_value_name)) {
      // Don't add an alias with the same normalized name.
      continue;
    }

    auto it_and_inserted = seen_by_number.try_emplace(number);
    if (it_and_inserted.second) {
      // This is the first value with this number; this name is canonical.
      result.push_back(RustEnumValue{rust_value_name, number});
      it_and_inserted.first->second = &result.back();
    } else {
      // This number has been seen before; this name is an alias.
      it_and_inserted.first->second->aliases.push_back(rust_value_name);
    }

    seen_by_name.insert(std::move(rust_value_name));
  }
  return result;
}

void GenerateEnumDefinition(Context& ctx, const EnumDescriptor& desc) {
  std::string name = EnumRsName(desc);
  ABSL_CHECK(desc.value_count() > 0);
  std::vector<RustEnumValue> values =
      EnumValues(desc.name(), EnumValuesInput(desc));
  ABSL_CHECK(!values.empty());

  ctx.Emit(
      {
          {"name", name},
          {"variants",
           [&] {
             for (const auto& value : values) {
               std::string number_str = absl::StrCat(value.number);
               // TODO: Replace with open enum variants when stable
               ctx.Emit({{"variant_name", value.name}, {"number", number_str}},
                        R"rs(
                    pub const $variant_name$: $name$ = $name$($number$);
                    )rs");
               for (const auto& alias : value.aliases) {
                 ctx.Emit({{"alias_name", alias}, {"number", number_str}},
                          R"rs(
                            pub const $alias_name$: $name$ = $name$($number$);
                            )rs");
               }
             }
           }},
          // The default value of an enum is the first listed value.
          // The compiler checks that this is equal to 0 for open enums.
          {"default_int_value", absl::StrCat(desc.value(0)->number())},
          {"impl_from_i32",
           [&] {
             if (desc.is_closed()) {
               ctx.Emit({{"name", name},
                         {"known_values_pattern",
                          // TODO: Check validity in UPB/C++.
                          absl::StrJoin(
                              values, "|",
                              [](std::string* o, const RustEnumValue& val) {
                                absl::StrAppend(o, val.number);
                              })}},
                        R"rs(
              impl $std$::convert::TryFrom<i32> for $name$ {
                type Error = $pb$::UnknownEnumValue<Self>;

                fn try_from(val: i32) -> Result<$name$, Self::Error> {
                  if matches!(val, $known_values_pattern$) {
                    Ok(Self(val))
                  } else {
                    Err($pb$::UnknownEnumValue::new($pbi$::Private, val))
                  }
                }
              }
            )rs");
             } else {
               ctx.Emit({{"name", name}}, R"rs(
              impl $std$::convert::From<i32> for $name$ {
                fn from(val: i32) -> $name$ {
                  Self(val)
                }
              }
            )rs");
             }
           }},
      },
      R"rs(
      #[repr(transparent)]
      #[derive(Clone, Copy, PartialEq, Eq)]
      pub struct $name$(i32);

      #[allow(non_upper_case_globals)]
      impl $name$ {
        $variants$
      }

      impl $std$::convert::From<$name$> for i32 {
        fn from(val: $name$) -> i32 {
          val.0
        }
      }

      $impl_from_i32$

      impl $std$::default::Default for $name$ {
        fn default() -> Self {
          Self($default_int_value$)
        }
      }

      impl $std$::fmt::Debug for $name$ {
        fn fmt(&self, f: &mut $std$::fmt::Formatter<'_>) -> $std$::fmt::Result {
          f.debug_tuple(stringify!($name$)).field(&self.0).finish()
        }
      }

      impl $pb$::Proxied for $name$ {
        type View<'a> = $name$;
        type Mut<'a> = $pb$::PrimitiveMut<'a, $name$>;
      }

      impl $pb$::ViewProxy<'_> for $name$ {
        type Proxied = $name$;

        fn as_view(&self) -> $name$ {
          *self
        }

        fn into_view<'shorter>(self) -> $pb$::View<'shorter, $name$> {
          self
        }
      }

      impl $pb$::SettableValue<$name$> for $name$ {
        fn set_on<'msg>(
            self,
            private: $pbi$::Private,
            mut mutator: $pb$::Mut<'msg, $name$>
        ) where $name$: 'msg {
          mutator.set_primitive(private, self)
        }
      }

      impl $pb$::ProxiedWithPresence for $name$ {
        type PresentMutData<'a> = $pbi$::RawVTableOptionalMutatorData<'a, $name$>;
        type AbsentMutData<'a> = $pbi$::RawVTableOptionalMutatorData<'a, $name$>;

        fn clear_present_field(
          present_mutator: Self::PresentMutData<'_>,
        ) -> Self::AbsentMutData<'_> {
          present_mutator.clear($pbi$::Private)
        }

        fn set_absent_to_default(
          absent_mutator: Self::AbsentMutData<'_>,
        ) -> Self::PresentMutData<'_> {
          absent_mutator.set_absent_to_default($pbi$::Private)
        }
      }

      unsafe impl $pb$::ProxiedInRepeated for $name$ {
        fn repeated_len(r: $pb$::View<$pb$::Repeated<Self>>) -> usize {
          $pbr$::cast_enum_repeated_view($pbi$::Private, r).len()
        }

        fn repeated_push(r: $pb$::Mut<$pb$::Repeated<Self>>, val: $name$) {
          $pbr$::cast_enum_repeated_mut($pbi$::Private, r).push(val.into())
        }

        fn repeated_clear(r: $pb$::Mut<$pb$::Repeated<Self>>) {
          $pbr$::cast_enum_repeated_mut($pbi$::Private, r).clear()
        }

        unsafe fn repeated_get_unchecked(
            r: $pb$::View<$pb$::Repeated<Self>>,
            index: usize,
        ) -> $pb$::View<$name$> {
          // SAFETY: In-bounds as promised by the caller.
          unsafe {
            $pbr$::cast_enum_repeated_view($pbi$::Private, r)
              .get_unchecked(index)
              .try_into()
              .unwrap_unchecked()
          }
        }

        unsafe fn repeated_set_unchecked(
            r: $pb$::Mut<$pb$::Repeated<Self>>,
            index: usize,
            val: $name$,
        ) {
          // SAFETY: In-bounds as promised by the caller.
          unsafe {
            $pbr$::cast_enum_repeated_mut($pbi$::Private, r)
              .set_unchecked(index, val.into())
          }
        }

        fn repeated_copy_from(
            src: $pb$::View<$pb$::Repeated<Self>>,
            dest: $pb$::Mut<$pb$::Repeated<Self>>,
        ) {
          $pbr$::cast_enum_repeated_mut($pbi$::Private, dest)
            .copy_from($pbr$::cast_enum_repeated_view($pbi$::Private, src))
        }
      }

      impl $pbi$::PrimitiveWithRawVTable for $name$ {}

      // SAFETY: this is an enum type
      unsafe impl $pbi$::Enum for $name$ {
        const NAME: &'static str = "$name$";
      }
      )rs");
}

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
