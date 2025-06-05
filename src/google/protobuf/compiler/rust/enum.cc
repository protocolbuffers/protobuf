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
#include "google/protobuf/compiler/cpp/names.h"
#include "google/protobuf/compiler/rust/context.h"
#include "google/protobuf/compiler/rust/naming.h"
#include "google/protobuf/descriptor.h"
#include "upb/reflection/def.hpp"

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

void TypeConversions(Context& ctx, const EnumDescriptor& desc) {
  switch (ctx.opts().kernel) {
    case Kernel::kCpp:
      ctx.Emit(
          R"rs(
          impl $pbr$::CppMapTypeConversions for $name$ {
              fn get_prototype() -> $pbr$::MapValue {
                  Self::to_map_value(Self::default())
              }

              fn to_map_value(self) -> $pbr$::MapValue {
                  $pbr$::MapValue::make_u32(self.0 as u32)
              }

              unsafe fn from_map_value<'a>(value: $pbr$::MapValue) -> $pb$::View<'a, Self> {
                  debug_assert_eq!(value.tag, $pbr$::MapValueTag::U32);
                  $name$(unsafe { value.val.u as i32 })
              }
          }
          )rs");
      return;
    case Kernel::kUpb:
      ctx.Emit(R"rs(
            impl $pbr$::UpbTypeConversions for $name$ {
                fn upb_type() -> $pbr$::CType {
                    $pbr$::CType::Enum
                }

                fn to_message_value(
                    val: $pb$::View<'_, Self>) -> $pbr$::upb_MessageValue {
                    $pbr$::upb_MessageValue { int32_val: val.0 }
                }

                unsafe fn into_message_value_fuse_if_required(
                  _raw_parent_arena: $pbr$::RawArena,
                  val: Self) -> $pbr$::upb_MessageValue {
                    $pbr$::upb_MessageValue { int32_val: val.0 }
                }

                unsafe fn from_message_value<'msg>(val: $pbr$::upb_MessageValue)
                    -> $pb$::View<'msg, Self> {
                  $name$(unsafe { val.int32_val })
                }
            }
            )rs");
      return;
  }
}

void MiniTable(Context& ctx, const EnumDescriptor& desc,
               upb::EnumDefPtr upb_enum) {
  if (ctx.is_cpp() || !desc.is_closed()) {
    return;
  }
  std::string mini_descriptor = upb_enum.MiniDescriptorEncode();
  ctx.Emit({{"mini_descriptor", mini_descriptor},
            {"mini_descriptor_length", mini_descriptor.size()}},
           R"rs(
    unsafe impl $pbr$::AssociatedMiniTableEnum for $name$ {
      fn mini_table() -> *const $pbr$::upb_MiniTableEnum {
        static MINI_TABLE: $std$::sync::OnceLock<$pbr$::MiniTableEnumPtr> =
            $std$::sync::OnceLock::new();
        MINI_TABLE.get_or_init(|| unsafe {
          $pbr$::MiniTableEnumPtr($pbr$::upb_MiniTableEnum_Build(
              "$mini_descriptor$".as_ptr(), $mini_descriptor_length$,
              $pbr$::THREAD_LOCAL_ARENA.with(|a| a.raw()),
              $std$::ptr::null_mut()))
        }).0
      }
    }
  )rs");
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

void GenerateEnumDefinition(Context& ctx, const EnumDescriptor& desc,
                            upb::EnumDefPtr upb_enum) {
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
          {"constant_name_fn",
           [&] {
             ctx.Emit({{"name_cases",
                        [&] {
                          for (const auto& value : values) {
                            std::string number_str = absl::StrCat(value.number);
                            ctx.Emit({{"variant_name", value.name},
                                      {"number", number_str}},
                                     R"rs(
                              $number$ => "$variant_name$",
                            )rs");
                          }
                        }}},
                      R"rs(
                fn constant_name(&self) -> $Option$<&'static str> {
                  #[allow(unreachable_patterns)] // In the case of aliases, just emit them all and let the first one match.
                  Some(match self.0 {
                    $name_cases$
                    _ => return None
                  })
                }
              )rs");
           }},
          // The default value of an enum is the first listed value.
          // The compiler checks that this is equal to 0 for open enums.
          {"default_int_value", absl::StrCat(desc.value(0)->number())},
          {"known_values_pattern",
           // TODO: Check validity in UPB/C++.
           absl::StrJoin(values, "|",
                         [](std::string* o, const RustEnumValue& val) {
                           absl::StrAppend(o, val.number);
                         })},
          {"impl_from_i32",
           [&] {
             if (desc.is_closed()) {
               ctx.Emit(R"rs(
              impl $std$::convert::TryFrom<i32> for $name$ {
                type Error = $pb$::UnknownEnumValue<Self>;

                fn try_from(val: i32) -> $Result$<$name$, Self::Error> {
                  if <Self as $pbi$::Enum>::is_known(val) {
                    Ok(Self(val))
                  } else {
                    Err($pb$::UnknownEnumValue::new($pbi$::Private, val))
                  }
                }
              }
            )rs");
             } else {
               ctx.Emit(R"rs(
              impl $std$::convert::From<i32> for $name$ {
                fn from(val: i32) -> $name$ {
                  Self(val)
                }
              }
            )rs");
             }
           }},
          {"type_conversions_impl", [&] { TypeConversions(ctx, desc); }},
          {"mini_table", [&] { MiniTable(ctx, desc, upb_enum); }},
      },
      R"rs(
      #[repr(transparent)]
      #[derive(Clone, Copy, PartialEq, Eq, Hash, PartialOrd, Ord)]
      pub struct $name$(i32);

      #[allow(non_upper_case_globals)]
      impl $name$ {
        $variants$

        $constant_name_fn$
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
          if let Some(constant_name) = self.constant_name() {
            write!(f, "$name$::{}", constant_name)
          } else {
            write!(f, "$name$::from({})", self.0)
          }
        }
      }

      impl $pb$::IntoProxied<i32> for $name$ {
        fn into_proxied(self, _: $pbi$::Private) -> i32 {
          self.0
        }
      }

      impl $pbi$::SealedInternal for $name$ {}

      impl $pb$::Proxied for $name$ {
        type View<'a> = $name$;
      }

      impl $pb$::Proxy<'_> for $name$ {}
      impl $pb$::ViewProxy<'_> for $name$ {}

      impl $pb$::AsView for $name$ {
        type Proxied = $name$;

        fn as_view(&self) -> $name$ {
          *self
        }
      }

      impl<'msg> $pb$::IntoView<'msg> for $name$ {
        fn into_view<'shorter>(self) -> $name$ where 'msg: 'shorter {
          self
        }
      }

      unsafe impl $pb$::ProxiedInRepeated for $name$ {
        fn repeated_new(_private: $pbi$::Private) -> $pb$::Repeated<Self> {
          $pbr$::new_enum_repeated()
        }

        unsafe fn repeated_free(_private: $pbi$::Private, f: &mut $pb$::Repeated<Self>) {
          $pbr$::free_enum_repeated(f)
        }

        fn repeated_len(r: $pb$::View<$pb$::Repeated<Self>>) -> usize {
          $pbr$::cast_enum_repeated_view(r).len()
        }

        fn repeated_push(r: $pb$::Mut<$pb$::Repeated<Self>>, val: impl $pb$::IntoProxied<$name$>) {
          $pbr$::cast_enum_repeated_mut(r).push(val.into_proxied($pbi$::Private))
        }

        fn repeated_clear(r: $pb$::Mut<$pb$::Repeated<Self>>) {
          $pbr$::cast_enum_repeated_mut(r).clear()
        }

        unsafe fn repeated_get_unchecked(
            r: $pb$::View<$pb$::Repeated<Self>>,
            index: usize,
        ) -> $pb$::View<$name$> {
          // SAFETY: In-bounds as promised by the caller.
          unsafe {
            $pbr$::cast_enum_repeated_view(r)
              .get_unchecked(index)
              .try_into()
              .unwrap_unchecked()
          }
        }

        unsafe fn repeated_set_unchecked(
            r: $pb$::Mut<$pb$::Repeated<Self>>,
            index: usize,
            val: impl $pb$::IntoProxied<$name$>,
        ) {
          // SAFETY: In-bounds as promised by the caller.
          unsafe {
            $pbr$::cast_enum_repeated_mut(r)
              .set_unchecked(index, val.into_proxied($pbi$::Private))
          }
        }

        fn repeated_copy_from(
            src: $pb$::View<$pb$::Repeated<Self>>,
            dest: $pb$::Mut<$pb$::Repeated<Self>>,
        ) {
          $pbr$::cast_enum_repeated_mut(dest)
            .copy_from($pbr$::cast_enum_repeated_view(src))
        }

        fn repeated_reserve(
            r: $pb$::Mut<$pb$::Repeated<Self>>,
            additional: usize,
        ) {
            // SAFETY:
            // - `f.as_raw()` is valid.
            $pbr$::reserve_enum_repeated_mut(r, additional);
        }
      }

      // SAFETY: this is an enum type
      unsafe impl $pbi$::Enum for $name$ {
        const NAME: &'static str = "$name$";

        fn is_known(value: i32) -> bool {
          matches!(value, $known_values_pattern$)
        }
      }

      $type_conversions_impl$

      $mini_table$
      )rs");
}

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
