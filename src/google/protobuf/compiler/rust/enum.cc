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

void EnumProxiedInMapValue(Context& ctx, const EnumDescriptor& desc) {
  switch (ctx.opts().kernel) {
    case Kernel::kCpp:
      for (const auto& t : kMapKeyTypes) {
        ctx.Emit(
            {{"map_new_thunk", RawMapThunk(ctx, desc, t.thunk_ident, "new")},
             {"map_free_thunk", RawMapThunk(ctx, desc, t.thunk_ident, "free")},
             {"map_clear_thunk",
              RawMapThunk(ctx, desc, t.thunk_ident, "clear")},
             {"map_size_thunk", RawMapThunk(ctx, desc, t.thunk_ident, "size")},
             {"map_insert_thunk",
              RawMapThunk(ctx, desc, t.thunk_ident, "insert")},
             {"map_get_thunk", RawMapThunk(ctx, desc, t.thunk_ident, "get")},
             {"map_remove_thunk",
              RawMapThunk(ctx, desc, t.thunk_ident, "remove")},
             {"map_iter_thunk", RawMapThunk(ctx, desc, t.thunk_ident, "iter")},
             {"map_iter_get_thunk",
              RawMapThunk(ctx, desc, t.thunk_ident, "iter_get")},
             {"to_ffi_key_expr", t.rs_to_ffi_key_expr},
             io::Printer::Sub("ffi_key_t", [&] { ctx.Emit(t.rs_ffi_key_t); })
                 .WithSuffix(""),
             io::Printer::Sub("key_t", [&] { ctx.Emit(t.rs_key_t); })
                 .WithSuffix(""),
             io::Printer::Sub("from_ffi_key_expr",
                              [&] { ctx.Emit(t.rs_from_ffi_key_expr); })
                 .WithSuffix("")},
            R"rs(
      extern "C" {
        fn $map_new_thunk$() -> $pbr$::RawMap;
        fn $map_free_thunk$(m: $pbr$::RawMap);
        fn $map_clear_thunk$(m: $pbr$::RawMap);
        fn $map_size_thunk$(m: $pbr$::RawMap) -> usize;
        fn $map_insert_thunk$(m: $pbr$::RawMap, key: $ffi_key_t$, value: $name$) -> bool;
        fn $map_get_thunk$(m: $pbr$::RawMap, key: $ffi_key_t$, value: *mut $name$) -> bool;
        fn $map_remove_thunk$(m: $pbr$::RawMap, key: $ffi_key_t$, value: *mut $name$) -> bool;
        fn $map_iter_thunk$(m: $pbr$::RawMap) -> $pbr$::UntypedMapIterator;
        fn $map_iter_get_thunk$(iter: &mut $pbr$::UntypedMapIterator, key: *mut $ffi_key_t$, value: *mut $name$);
      }
      impl $pb$::ProxiedInMapValue<$key_t$> for $name$ {
        fn map_new(_private: $pbi$::Private) -> $pb$::Map<$key_t$, Self> {
            unsafe {
                $pb$::Map::from_inner(
                    $pbi$::Private,
                    $pbr$::InnerMap::new($pbi$::Private, $map_new_thunk$())
                )
            }
        }

        unsafe fn map_free(_private: $pbi$::Private, map: &mut $pb$::Map<$key_t$, Self>) {
            unsafe { $map_free_thunk$(map.as_raw($pbi$::Private)); }
        }

        fn map_clear(mut map: $pb$::Mut<'_, $pb$::Map<$key_t$, Self>>) {
            unsafe { $map_clear_thunk$(map.as_raw($pbi$::Private)); }
        }

        fn map_len(map: $pb$::View<'_, $pb$::Map<$key_t$, Self>>) -> usize {
            unsafe { $map_size_thunk$(map.as_raw($pbi$::Private)) }
        }

        fn map_insert(mut map: $pb$::Mut<'_, $pb$::Map<$key_t$, Self>>, key: $pb$::View<'_, $key_t$>, value: $pb$::View<'_, Self>) -> bool {
            unsafe { $map_insert_thunk$(map.as_raw($pbi$::Private), $to_ffi_key_expr$, value) }
        }

        fn map_get<'a>(map: $pb$::View<'a, $pb$::Map<$key_t$, Self>>, key: $pb$::View<'_, $key_t$>) -> Option<$pb$::View<'a, Self>> {
            let key = $to_ffi_key_expr$;
            let mut value = $std$::mem::MaybeUninit::uninit();
            let found = unsafe { $map_get_thunk$(map.as_raw($pbi$::Private), key, value.as_mut_ptr()) };
            if !found {
                return None;
            }
            Some(unsafe { value.assume_init() })
        }

        fn map_remove(mut map: $pb$::Mut<'_, $pb$::Map<$key_t$, Self>>, key: $pb$::View<'_, $key_t$>) -> bool {
            let mut value = $std$::mem::MaybeUninit::uninit();
            unsafe { $map_remove_thunk$(map.as_raw($pbi$::Private), $to_ffi_key_expr$, value.as_mut_ptr()) }
        }

        fn map_iter(map: $pb$::View<'_, $pb$::Map<$key_t$, Self>>) -> $pb$::MapIter<'_, $key_t$, Self> {
            // SAFETY:
            // - The backing map for `map.as_raw` is valid for at least '_.
            // - A View that is live for '_ guarantees the backing map is unmodified for '_.
            // - The `iter` function produces an iterator that is valid for the key
            //   and value types, and live for at least '_.
            unsafe {
                $pb$::MapIter::from_raw(
                    $pbi$::Private,
                    $map_iter_thunk$(map.as_raw($pbi$::Private))
                )
            }
        }

        fn map_iter_next<'a>(iter: &mut $pb$::MapIter<'a, $key_t$, Self>) -> Option<($pb$::View<'a, $key_t$>, $pb$::View<'a, Self>)> {
            // SAFETY:
            // - The `MapIter` API forbids the backing map from being mutated for 'a,
            //   and guarantees that it's the correct key and value types.
            // - The thunk is safe to call as long as the iterator isn't at the end.
            // - The thunk always writes to key and value fields and does not read.
            // - The thunk does not increment the iterator.
            unsafe {
                iter.as_raw_mut($pbi$::Private).next_unchecked::<$key_t$, Self, _, _>(
                    $pbi$::Private,
                    $map_iter_get_thunk$,
                    |ffi_key| $from_ffi_key_expr$,
                    $std$::convert::identity,
                )
            }
        }
      }
      )rs");
      }
      return;
    case Kernel::kUpb:
      for (const auto& t : kMapKeyTypes) {
        ctx.Emit({io::Printer::Sub("key_t", [&] { ctx.Emit(t.rs_key_t); })
                      .WithSuffix("")},
                 R"rs(
      impl $pb$::ProxiedInMapValue<$key_t$> for $name$ {
          fn map_new(_private: $pbi$::Private) -> $pb$::Map<$key_t$, Self> {
              let arena = $pbr$::Arena::new();
              let raw = unsafe {
                  $pbr$::upb_Map_New(
                      arena.raw(),
                      <$key_t$ as $pbr$::UpbTypeConversions>::upb_type(),
                      $pbr$::CType::Enum)
              };
              $pb$::Map::from_inner(
                  $pbi$::Private,
                  $pbr$::InnerMap::new($pbi$::Private, raw, arena))
          }

          unsafe fn map_free(_private: $pbi$::Private, _map: &mut $pb$::Map<$key_t$, Self>) {
              // No-op: the memory will be dropped by the arena.
          }

          fn map_clear(mut map: $pb$::Mut<'_, $pb$::Map<$key_t$, Self>>) {
              unsafe {
                  $pbr$::upb_Map_Clear(map.as_raw($pbi$::Private));
              }
          }

          fn map_len(map: $pb$::View<'_, $pb$::Map<$key_t$, Self>>) -> usize {
              unsafe {
                  $pbr$::upb_Map_Size(map.as_raw($pbi$::Private))
              }
          }

          fn map_insert(mut map: $pb$::Mut<'_, $pb$::Map<$key_t$, Self>>, key: $pb$::View<'_, $key_t$>, value: $pb$::View<'_, Self>) -> bool {
              let arena = map.inner($pbi$::Private).raw_arena($pbi$::Private);
              unsafe {
                  $pbr$::upb_Map_InsertAndReturnIfInserted(
                      map.as_raw($pbi$::Private),
                      <$key_t$ as $pbr$::UpbTypeConversions>::to_message_value(key),
                      $pbr$::upb_MessageValue { int32_val: value.0 },
                      arena
                  )
              }
          }

          fn map_get<'a>(map: $pb$::View<'a, $pb$::Map<$key_t$, Self>>, key: $pb$::View<'_, $key_t$>) -> Option<$pb$::View<'a, Self>> {
              let mut val = $std$::mem::MaybeUninit::uninit();
              let found = unsafe {
                  $pbr$::upb_Map_Get(
                      map.as_raw($pbi$::Private),
                      <$key_t$ as $pbr$::UpbTypeConversions>::to_message_value(key),
                      val.as_mut_ptr())
              };
              if !found {
                  return None;
              }
              Some($name$(unsafe { val.assume_init().int32_val }))
          }

          fn map_remove(mut map: $pb$::Mut<'_, $pb$::Map<$key_t$, Self>>, key: $pb$::View<'_, $key_t$>) -> bool {
              let mut val = $std$::mem::MaybeUninit::uninit();
              unsafe {
                  $pbr$::upb_Map_Delete(
                      map.as_raw($pbi$::Private),
                      <$key_t$ as $pbr$::UpbTypeConversions>::to_message_value(key),
                      val.as_mut_ptr())
              }
          }
          fn map_iter(map: $pb$::View<'_, $pb$::Map<$key_t$, Self>>) -> $pb$::MapIter<'_, $key_t$, Self> {
              // SAFETY: View<Map<'_,..>> guarantees its RawMap outlives '_.
              unsafe {
                  $pb$::MapIter::from_raw($pbi$::Private, $pbr$::RawMapIter::new($pbi$::Private, map.as_raw($pbi$::Private)))
              }
          }

          fn map_iter_next<'a>(
              iter: &mut $pb$::MapIter<'a, $key_t$, Self>
          ) -> Option<($pb$::View<'a, $key_t$>, $pb$::View<'a, Self>)> {
              // SAFETY: MapIter<'a, ..> guarantees its RawMapIter outlives 'a.
              unsafe { iter.as_raw_mut($pbi$::Private).next_unchecked($pbi$::Private) }
                  // SAFETY: MapIter<K, V> returns key and values message values
                  //         with the variants for K and V active.
                  .map(|(k, v)| unsafe {(
                      <$key_t$ as $pbr$::UpbTypeConversions>::from_message_value(k),
                      Self(v.int32_val),
                  )})
          }
      }
      )rs");
      }
      return;
  }
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

                fn try_from(val: i32) -> Result<$name$, Self::Error> {
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
          {"impl_proxied_in_map", [&] { EnumProxiedInMapValue(ctx, desc); }},
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

      // SAFETY: this is an enum type
      unsafe impl $pbi$::Enum for $name$ {
        const NAME: &'static str = "$name$";

        fn is_known(value: i32) -> bool {
          matches!(value, $known_values_pattern$)
        }
      }

      $impl_proxied_in_map$
      )rs");
}

void GenerateEnumThunksCc(Context& ctx, const EnumDescriptor& desc) {
  ctx.Emit(
      {
          {"cpp_t", cpp::QualifiedClassName(&desc)},
          {"rs_t", UnderscoreDelimitFullName(ctx, desc.full_name())},
          {"abi", "\"C\""},  // Workaround for syntax highlight bug in VSCode.
      },
      R"cc(
        extern $abi$ {
          __PB_RUST_EXPOSE_SCALAR_MAP_METHODS_FOR_VALUE_TYPE(
              $cpp_t$, $rs_t$, $cpp_t$, value, cpp_value)
        }
      )cc");
}

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
