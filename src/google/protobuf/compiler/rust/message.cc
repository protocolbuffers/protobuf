// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/rust/message.h"

#include <string>

#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/cpp/names.h"
#include "google/protobuf/compiler/rust/accessors/accessor_case.h"
#include "google/protobuf/compiler/rust/accessors/accessors.h"
#include "google/protobuf/compiler/rust/context.h"
#include "google/protobuf/compiler/rust/enum.h"
#include "google/protobuf/compiler/rust/naming.h"
#include "google/protobuf/compiler/rust/oneof.h"
#include "google/protobuf/descriptor.h"
#include "upb_generator/mangle.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace rust {
namespace {

std::string UpbMinitableName(const Descriptor& msg) {
  return upb::generator::MessageInit(msg.full_name());
}

void MessageNew(Context& ctx, const Descriptor& msg) {
  switch (ctx.opts().kernel) {
    case Kernel::kCpp:
      ctx.Emit({{"new_thunk", ThunkName(ctx, msg, "new")}}, R"rs(
        Self { inner: $pbr$::MessageInner { msg: unsafe { $new_thunk$() } } }
      )rs");
      return;

    case Kernel::kUpb:
      ctx.Emit({{"new_thunk", ThunkName(ctx, msg, "new")}}, R"rs(
        let arena = $pbr$::Arena::new();
        Self {
          inner: $pbr$::MessageInner {
            msg: unsafe { $new_thunk$(arena.raw()) },
            arena,
          }
        }
      )rs");
      return;
  }

  ABSL_LOG(FATAL) << "unreachable";
}

void MessageSerialize(Context& ctx, const Descriptor& msg) {
  switch (ctx.opts().kernel) {
    case Kernel::kCpp:
      ctx.Emit({{"serialize_thunk", ThunkName(ctx, msg, "serialize")}}, R"rs(
        unsafe { $serialize_thunk$(self.raw_msg()) }
      )rs");
      return;

    case Kernel::kUpb:
      ctx.Emit({{"minitable", UpbMinitableName(msg)}},
               R"rs(
        // SAFETY: $minitable$ is a static of a const object.
        let mini_table = unsafe { $std$::ptr::addr_of!($minitable$) };
        // SAFETY: $minitable$ is the one associated with raw_msg().
        let encoded = unsafe {
          $pbr$::wire::encode(self.raw_msg(), mini_table)
        };

        //~ TODO: Currently serialize() on the Rust API is an
        //~ infallible fn, so if upb signals an error here we can only panic.
        let serialized = encoded.expect("serialize is not allowed to fail");
        serialized
      )rs");
      return;
  }

  ABSL_LOG(FATAL) << "unreachable";
}

void MessageClearAndParse(Context& ctx, const Descriptor& msg) {
  switch (ctx.opts().kernel) {
    case Kernel::kCpp:
      ctx.Emit(
          {
              {"parse_thunk", ThunkName(ctx, msg, "parse")},
          },
          R"rs(
          let success = unsafe {
            // SAFETY: `data.as_ptr()` is valid to read for `data.len()`.
            let data = $pbr$::SerializedData::from_raw_parts(
              $NonNull$::new(data.as_ptr() as *mut _).unwrap(),
              data.len(),
            );

            $parse_thunk$(self.raw_msg(), data)
          };
          success.then_some(()).ok_or($pb$::ParseError)
        )rs");
      return;

    case Kernel::kUpb:
      ctx.Emit({{"minitable", UpbMinitableName(msg)}},
               R"rs(
        let mut msg = Self::new();
        // SAFETY: $minitable$ is a static of a const object.
        let mini_table = unsafe { $std$::ptr::addr_of!($minitable$) };

        // SAFETY:
        // - `data.as_ptr()` is valid to read for `data.len()`
        // - `mini_table` is the one used to construct `msg.raw_msg()`
        // - `msg.arena().raw()` is held for the same lifetime as `msg`.
        let status = unsafe {
          $pbr$::wire::decode(
              data, msg.raw_msg(),
              mini_table, msg.arena())
        };
        match status {
          Ok(_) => {
            //~ This swap causes the old self.inner.arena to be moved into `msg`
            //~ which we immediately drop, which will release any previous
            //~ message that was held here.
            std::mem::swap(self, &mut msg);
            Ok(())
          }
          Err(_) => Err($pb$::ParseError)
        }
      )rs");
      return;
  }

  ABSL_LOG(FATAL) << "unreachable";
}

void MessageDebug(Context& ctx, const Descriptor& msg) {
  switch (ctx.opts().kernel) {
    case Kernel::kCpp:
      ctx.Emit({},
               R"rs(
        $pbr$::debug_string($pbi$::Private, self.raw_msg(), f)
      )rs");
      return;

    case Kernel::kUpb:
      ctx.Emit({},
               R"rs(
        f.debug_struct(std::any::type_name::<Self>())
          .field("raw_msg", &self.raw_msg())
          .finish()
      )rs");
      return;
  }

  ABSL_LOG(FATAL) << "unreachable";
}

void MessageExterns(Context& ctx, const Descriptor& msg) {
  switch (ctx.opts().kernel) {
    case Kernel::kCpp:
      ctx.Emit(
          {
              {"new_thunk", ThunkName(ctx, msg, "new")},
              {"delete_thunk", ThunkName(ctx, msg, "delete")},
              {"serialize_thunk", ThunkName(ctx, msg, "serialize")},
              {"parse_thunk", ThunkName(ctx, msg, "parse")},
              {"copy_from_thunk", ThunkName(ctx, msg, "copy_from")},
              {"repeated_len_thunk", ThunkName(ctx, msg, "repeated_len")},
              {"repeated_get_thunk", ThunkName(ctx, msg, "repeated_get")},
              {"repeated_get_mut_thunk",
               ThunkName(ctx, msg, "repeated_get_mut")},
              {"repeated_add_thunk", ThunkName(ctx, msg, "repeated_add")},
              {"repeated_clear_thunk", ThunkName(ctx, msg, "repeated_clear")},
              {"repeated_copy_from_thunk",
               ThunkName(ctx, msg, "repeated_copy_from")},
          },
          R"rs(
          fn $new_thunk$() -> $pbr$::RawMessage;
          fn $delete_thunk$(raw_msg: $pbr$::RawMessage);
          fn $serialize_thunk$(raw_msg: $pbr$::RawMessage) -> $pbr$::SerializedData;
          fn $parse_thunk$(raw_msg: $pbr$::RawMessage, data: $pbr$::SerializedData) -> bool;
          fn $copy_from_thunk$(dst: $pbr$::RawMessage, src: $pbr$::RawMessage);
          fn $repeated_len_thunk$(raw: $pbr$::RawRepeatedField) -> usize;
          fn $repeated_add_thunk$(raw: $pbr$::RawRepeatedField) -> $pbr$::RawMessage;
          fn $repeated_get_thunk$(raw: $pbr$::RawRepeatedField, index: usize) -> $pbr$::RawMessage;
          fn $repeated_get_mut_thunk$(raw: $pbr$::RawRepeatedField, index: usize) -> $pbr$::RawMessage;
          fn $repeated_clear_thunk$(raw: $pbr$::RawRepeatedField);
          fn $repeated_copy_from_thunk$(dst: $pbr$::RawRepeatedField, src: $pbr$::RawRepeatedField);
        )rs");
      return;

    case Kernel::kUpb:
      ctx.Emit(
          {
              {"new_thunk", ThunkName(ctx, msg, "new")},
              {"minitable", UpbMinitableName(msg)},
          },
          R"rs(
          fn $new_thunk$(arena: $pbr$::RawArena) -> $pbr$::RawMessage;
          /// Opaque wrapper for this message's MiniTable. The only valid way to
          /// reference this static is with `std::ptr::addr_of!(..)`.
          static $minitable$: $pbr$::upb_MiniTable;
      )rs");
      return;
  }

  ABSL_LOG(FATAL) << "unreachable";
}

void MessageDrop(Context& ctx, const Descriptor& msg) {
  if (ctx.is_upb()) {
    // Nothing to do here; drop glue (which will run drop(self.arena)
    // automatically) is sufficient.
    return;
  }

  ctx.Emit({{"delete_thunk", ThunkName(ctx, msg, "delete")}}, R"rs(
    unsafe { $delete_thunk$(self.raw_msg()); }
  )rs");
}

void IntoProxiedForMessage(Context& ctx, const Descriptor& msg) {
  switch (ctx.opts().kernel) {
    case Kernel::kCpp:
      ctx.Emit({{"copy_from_thunk", ThunkName(ctx, msg, "copy_from")}}, R"rs(
        impl<'msg> $pb$::IntoProxied<$Msg$> for $Msg$View<'msg> {
          fn into(self, _private: $pbi$::Private) -> $Msg$ {
            let dst = $Msg$::new();
            unsafe { $copy_from_thunk$(dst.inner.msg, self.msg) };
            dst
          }
        }

        impl<'msg> $pb$::IntoProxied<$Msg$> for $Msg$Mut<'msg> {
          fn into(self, _private: $pbi$::Private) -> $Msg$ {
            $pb$::IntoProxied::into($pb$::ViewProxy::into_view(self), _private)
          }
        }

        impl $pb$::IntoProxied<$Msg$> for $Msg$ {
          fn into(self, _private: $pbi$::Private) -> $Msg$ {
            self
          }
        }
      )rs");
      return;

    case Kernel::kUpb:
      ctx.Emit({{"minitable", UpbMinitableName(msg)}}, R"rs(
        impl<'msg> $pb$::IntoProxied<$Msg$> for $Msg$View<'msg> {
          fn into(self, _private: $pbi$::Private) -> $Msg$ {
            let dst = $Msg$::new();
            unsafe { $pbr$::upb_Message_DeepCopy(
              dst.inner.msg,
              self.msg,
              $std$::ptr::addr_of!($minitable$),
              dst.inner.arena.raw(),
            ) };
            dst
          }
        }

        impl<'msg> $pb$::IntoProxied<$Msg$> for $Msg$Mut<'msg> {
          fn into(self, _private: $pbi$::Private) -> $Msg$ {
            $pb$::IntoProxied::into($pb$::ViewProxy::into_view(self), _private)
          }
        }

        impl $pb$::IntoProxied<$Msg$> for $Msg$ {
          fn into(self, _private: $pbi$::Private) -> $Msg$ {
            self
          }
        }
      )rs");
      return;
  }

  ABSL_LOG(FATAL) << "unreachable";
}

void MessageGetMinitable(Context& ctx, const Descriptor& msg) {
  if (ctx.opts().kernel == Kernel::kUpb) {
    ctx.Emit({{"minitable", UpbMinitableName(msg)}}, R"rs(
      pub fn raw_minitable(_private: $pbi$::Private) -> *const $pbr$::upb_MiniTable {
        unsafe { $std$::ptr::addr_of!($minitable$) }
      }
    )rs");
  }
}

void MessageProxiedInRepeated(Context& ctx, const Descriptor& msg) {
  switch (ctx.opts().kernel) {
    case Kernel::kCpp:
      ctx.Emit(
          {
              {"Msg", RsSafeName(msg.name())},
              {"copy_from_thunk", ThunkName(ctx, msg, "copy_from")},
              {"repeated_len_thunk", ThunkName(ctx, msg, "repeated_len")},
              {"repeated_get_thunk", ThunkName(ctx, msg, "repeated_get")},
              {"repeated_get_mut_thunk",
               ThunkName(ctx, msg, "repeated_get_mut")},
              {"repeated_add_thunk", ThunkName(ctx, msg, "repeated_add")},
              {"repeated_clear_thunk", ThunkName(ctx, msg, "repeated_clear")},
              {"repeated_copy_from_thunk",
               ThunkName(ctx, msg, "repeated_copy_from")},
          },
          R"rs(
        unsafe impl $pb$::ProxiedInRepeated for $Msg$ {
          fn repeated_len(f: $pb$::View<$pb$::Repeated<Self>>) -> usize {
            // SAFETY: `f.as_raw()` is a valid `RepeatedPtrField*`.
            unsafe { $repeated_len_thunk$(f.as_raw($pbi$::Private)) }
          }

          unsafe fn repeated_set_unchecked(
            mut f: $pb$::Mut<$pb$::Repeated<Self>>,
            i: usize,
            v: $pb$::View<Self>,
          ) {
            // SAFETY:
            // - `f.as_raw()` is a valid `RepeatedPtrField*`.
            // - `i < len(f)` is promised by caller.
            // - `v.raw_msg()` is a valid `const Message&`.
            unsafe {
              $copy_from_thunk$(
                $repeated_get_mut_thunk$(f.as_raw($pbi$::Private), i),
                v.raw_msg(),
              );
            }
          }

          unsafe fn repeated_get_unchecked(
            f: $pb$::View<$pb$::Repeated<Self>>,
            i: usize,
          ) -> $pb$::View<Self> {
            // SAFETY:
            // - `f.as_raw()` is a valid `const RepeatedPtrField&`.
            // - `i < len(f)` is promised by caller.
            let msg = unsafe { $repeated_get_thunk$(f.as_raw($pbi$::Private), i) };
            $pb$::View::<Self>::new($pbi$::Private, msg)
          }
          fn repeated_clear(mut f: $pb$::Mut<$pb$::Repeated<Self>>) {
            // SAFETY:
            // - `f.as_raw()` is a valid `RepeatedPtrField*`.
            unsafe { $repeated_clear_thunk$(f.as_raw($pbi$::Private)) };
          }

          fn repeated_push(mut f: $pb$::Mut<$pb$::Repeated<Self>>, v: $pb$::View<Self>) {
            // SAFETY:
            // - `f.as_raw()` is a valid `RepeatedPtrField*`.
            // - `v.raw_msg()` is a valid `const Message&`.
            unsafe {
              let new_elem = $repeated_add_thunk$(f.as_raw($pbi$::Private));
              $copy_from_thunk$(new_elem, v.raw_msg());
            }
          }

          fn repeated_copy_from(
            src: $pb$::View<$pb$::Repeated<Self>>,
            mut dest: $pb$::Mut<$pb$::Repeated<Self>>,
          ) {
            // SAFETY:
            // - `dest.as_raw()` is a valid `RepeatedPtrField*`.
            // - `src.as_raw()` is a valid `const RepeatedPtrField&`.
            unsafe {
              $repeated_copy_from_thunk$(dest.as_raw($pbi$::Private), src.as_raw($pbi$::Private));
            }
          }
        }
      )rs");
      return;
    case Kernel::kUpb:
      ctx.Emit(
          {
              {"minitable", UpbMinitableName(msg)},
              {"new_thunk", ThunkName(ctx, msg, "new")},
          },
          R"rs(
        unsafe impl $pb$::ProxiedInRepeated for $Msg$ {
          fn repeated_len(f: $pb$::View<$pb$::Repeated<Self>>) -> usize {
            // SAFETY: `f.as_raw()` is a valid `upb_Array*`.
            unsafe { $pbr$::upb_Array_Size(f.as_raw($pbi$::Private)) }
          }
          unsafe fn repeated_set_unchecked(
            mut f: $pb$::Mut<$pb$::Repeated<Self>>,
            i: usize,
            v: $pb$::View<Self>,
          ) {
            // SAFETY:
            // - `f.as_raw()` is a valid `upb_Array*`.
            // - `i < len(f)` is promised by the caller.
            let dest_msg = unsafe {
              $pbr$::upb_Array_GetMutable(f.as_raw($pbi$::Private), i).msg
            }.expect("upb_Array* element should not be NULL");

            // SAFETY:
            // - `dest_msg` is a valid `upb_Message*`.
            // - `v.raw_msg()` and `dest_msg` both have message minitable `$minitable$`.
            unsafe {
              $pbr$::upb_Message_DeepCopy(
                dest_msg,
                v.raw_msg(),
                $std$::ptr::addr_of!($minitable$),
                f.raw_arena($pbi$::Private),
              )
            };
          }

          unsafe fn repeated_get_unchecked(
            f: $pb$::View<$pb$::Repeated<Self>>,
            i: usize,
          ) -> $pb$::View<Self> {
            // SAFETY:
            // - `f.as_raw()` is a valid `const upb_Array*`.
            // - `i < len(f)` is promised by the caller.
            let msg_ptr = unsafe { $pbr$::upb_Array_Get(f.as_raw($pbi$::Private), i).msg_val }
              .expect("upb_Array* element should not be NULL.");
            $pb$::View::<Self>::new($pbi$::Private, msg_ptr)
          }

          fn repeated_clear(mut f: $pb$::Mut<$pb$::Repeated<Self>>) {
            // SAFETY:
            // - `f.as_raw()` is a valid `upb_Array*`.
            unsafe {
              $pbr$::upb_Array_Resize(f.as_raw($pbi$::Private), 0, f.raw_arena($pbi$::Private))
            };
          }
          fn repeated_push(mut f: $pb$::Mut<$pb$::Repeated<Self>>, v: $pb$::View<Self>) {
            // SAFETY:
            // - `v.raw_msg()` is a valid `const upb_Message*` with minitable `$minitable$`.
            let msg_ptr = unsafe {
              $pbr$::upb_Message_DeepClone(
                v.raw_msg(),
                std::ptr::addr_of!($minitable$),
                f.raw_arena($pbi$::Private),
              )
            }.expect("upb_Message_DeepClone failed.");

            // Append new default message to array.
            // SAFETY:
            // - `f.as_raw()` is a valid `upb_Array*`.
            // - `msg_ptr` is a valid `upb_Message*`.
            unsafe {
              $pbr$::upb_Array_Append(
                f.as_raw($pbi$::Private),
                $pbr$::upb_MessageValue{msg_val: Some(msg_ptr)},
                f.raw_arena($pbi$::Private),
              );
            };
          }

          fn repeated_copy_from(
            src: $pb$::View<$pb$::Repeated<Self>>,
            dest: $pb$::Mut<$pb$::Repeated<Self>>,
          ) {
              // SAFETY:
              // - Elements of `src` and `dest` have message minitable `$minitable$`.
              unsafe {
                $pbr$::repeated_message_copy_from(src, dest, $std$::ptr::addr_of!($minitable$));
              }
          }
        }
      )rs");
      return;
  }
  ABSL_LOG(FATAL) << "unreachable";
}

void MessageProxiedInMapValue(Context& ctx, const Descriptor& msg) {
  switch (ctx.opts().kernel) {
    case Kernel::kCpp:
      for (const auto& t : kMapKeyTypes) {
        ctx.Emit(
            {{"map_new_thunk", RawMapThunk(ctx, msg, t.thunk_ident, "new")},
             {"map_free_thunk", RawMapThunk(ctx, msg, t.thunk_ident, "free")},
             {"map_clear_thunk", RawMapThunk(ctx, msg, t.thunk_ident, "clear")},
             {"map_size_thunk", RawMapThunk(ctx, msg, t.thunk_ident, "size")},
             {"map_insert_thunk",
              RawMapThunk(ctx, msg, t.thunk_ident, "insert")},
             {"map_get_thunk", RawMapThunk(ctx, msg, t.thunk_ident, "get")},
             {"map_remove_thunk",
              RawMapThunk(ctx, msg, t.thunk_ident, "remove")},
             {"map_iter_thunk", RawMapThunk(ctx, msg, t.thunk_ident, "iter")},
             {"map_iter_get_thunk",
              RawMapThunk(ctx, msg, t.thunk_ident, "iter_get")},
             {"key_expr", t.rs_to_ffi_key_expr},
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
                fn $map_insert_thunk$(m: $pbr$::RawMap, key: $ffi_key_t$, value: $pbr$::RawMessage) -> bool;
                fn $map_get_thunk$(m: $pbr$::RawMap, key: $ffi_key_t$, value: *mut $pbr$::RawMessage) -> bool;
                fn $map_remove_thunk$(m: $pbr$::RawMap, key: $ffi_key_t$, value: *mut $pbr$::RawMessage) -> bool;
                fn $map_iter_thunk$(m: $pbr$::RawMap) -> $pbr$::UntypedMapIterator;
                fn $map_iter_get_thunk$(iter: &mut $pbr$::UntypedMapIterator, key: *mut $ffi_key_t$, value: *mut $pbr$::RawMessage);
            }
            impl $pb$::ProxiedInMapValue<$key_t$> for $Msg$ {
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
                    unsafe { $map_insert_thunk$(map.as_raw($pbi$::Private), $key_expr$, value.raw_msg()) }
                }

                fn map_get<'a>(map: $pb$::View<'a, $pb$::Map<$key_t$, Self>>, key: $pb$::View<'_, $key_t$>) -> Option<$pb$::View<'a, Self>> {
                    let key = $key_expr$;
                    let mut value = $std$::mem::MaybeUninit::uninit();
                    let found = unsafe { $map_get_thunk$(map.as_raw($pbi$::Private), key, value.as_mut_ptr()) };
                    if !found {
                        return None;
                    }
                    Some($Msg$View::new($pbi$::Private, unsafe { value.assume_init() }))
                }

                fn map_remove(mut map: $pb$::Mut<'_, $pb$::Map<$key_t$, Self>>, key: $pb$::View<'_, $key_t$>) -> bool {
                    let mut value = $std$::mem::MaybeUninit::uninit();
                    unsafe { $map_remove_thunk$(map.as_raw($pbi$::Private), $key_expr$, value.as_mut_ptr()) }
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
                            |raw_msg| $Msg$View::new($pbi$::Private, raw_msg)
                        )
                    }
                }
            }
      )rs");
      }
      return;
    case Kernel::kUpb:
      ctx.Emit(
          {
              {"minitable", UpbMinitableName(msg)},
              {"new_thunk", ThunkName(ctx, msg, "new")},
          },
          R"rs(
            impl $pbr$::UpbTypeConversions for $Msg$ {
                fn upb_type() -> $pbr$::CType {
                    $pbr$::CType::Message
                }

                fn to_message_value(
                    val: $pb$::View<'_, Self>) -> $pbr$::upb_MessageValue {
                    $pbr$::upb_MessageValue { msg_val: Some(val.raw_msg()) }
                }

                unsafe fn to_message_value_copy_if_required(
                  arena: $pbr$::RawArena,
                  val: $pb$::View<'_, Self>) -> $pbr$::upb_MessageValue {
                  // Self::to_message_value(val)
                  // SAFETY: The arena memory is not freed due to `ManuallyDrop`.
                  let cloned_msg = $pbr$::upb_Message_DeepClone(
                      val.raw_msg(), $std$::ptr::addr_of!($minitable$), arena)
                      .expect("upb_Message_DeepClone failed.");
                  Self::to_message_value(
                      $Msg$View::new($pbi$::Private, cloned_msg))
                  }

                unsafe fn from_message_value<'msg>(msg: $pbr$::upb_MessageValue)
                    -> $pb$::View<'msg, Self> {
                    $Msg$View::new(
                        $pbi$::Private,
                        unsafe { msg.msg_val }
                            .expect("expected present message value in map"))
                }
            }
            )rs");
      for (const auto& t : kMapKeyTypes) {
        ctx.Emit({io::Printer::Sub("key_t", [&] { ctx.Emit(t.rs_key_t); })
                      .WithSuffix("")},
                 R"rs(
            impl $pb$::ProxiedInMapValue<$key_t$> for $Msg$ {
                fn map_new(_private: $pbi$::Private) -> $pb$::Map<$key_t$, Self> {
                    let arena = $pbr$::Arena::new();
                    let raw = unsafe {
                      $pbr$::upb_Map_New(
                        arena.raw(),
                        <$key_t$ as $pbr$::UpbTypeConversions>::upb_type(),
                        <Self as $pbr$::UpbTypeConversions>::upb_type())
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
                            <Self as $pbr$::UpbTypeConversions>::to_message_value_copy_if_required(arena, value),
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
                    Some(unsafe { <Self as $pbr$::UpbTypeConversions>::from_message_value(val.assume_init()) })
                }

                fn map_remove(mut map: $pb$::Mut<'_, $pb$::Map<$key_t$, Self>>, key: $pb$::View<'_, $key_t$>) -> bool {
                    unsafe {
                        $pbr$::upb_Map_Delete(
                            map.as_raw($pbi$::Private),
                            <$key_t$ as $pbr$::UpbTypeConversions>::to_message_value(key),
                            $std$::ptr::null_mut())
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
                            <Self as $pbr$::UpbTypeConversions>::from_message_value(v),
                        )})
                }
            }
      )rs");
      }
  }
}

}  // namespace

void GenerateRs(Context& ctx, const Descriptor& msg) {
  if (msg.map_key() != nullptr) {
    // Don't generate code for synthetic MapEntry messages.
    return;
  }
  ctx.Emit(
      {{"Msg", RsSafeName(msg.name())},
       {"Msg::new", [&] { MessageNew(ctx, msg); }},
       {"Msg::serialize", [&] { MessageSerialize(ctx, msg); }},
       {"Msg::clear_and_parse", [&] { MessageClearAndParse(ctx, msg); }},
       {"Msg::drop", [&] { MessageDrop(ctx, msg); }},
       {"Msg::debug", [&] { MessageDebug(ctx, msg); }},
       {"Msg_externs", [&] { MessageExterns(ctx, msg); }},
       {"accessor_fns",
        [&] {
          for (int i = 0; i < msg.field_count(); ++i) {
            GenerateAccessorMsgImpl(ctx, *msg.field(i), AccessorCase::OWNED);
          }
          for (int i = 0; i < msg.real_oneof_decl_count(); ++i) {
            GenerateOneofAccessors(ctx, *msg.real_oneof_decl(i),
                                   AccessorCase::OWNED);
          }
        }},
       {"accessor_externs",
        [&] {
          for (int i = 0; i < msg.field_count(); ++i) {
            GenerateAccessorExternC(ctx, *msg.field(i));
          }
        }},
       {"oneof_externs",
        [&] {
          for (int i = 0; i < msg.real_oneof_decl_count(); ++i) {
            GenerateOneofExternC(ctx, *msg.real_oneof_decl(i));
          }
        }},
       {"nested_in_msg",
        [&] {
          // If we have no nested types, enums, or oneofs, bail out without
          // emitting an empty mod some_msg.
          if (msg.nested_type_count() == 0 && msg.enum_type_count() == 0 &&
              msg.real_oneof_decl_count() == 0) {
            return;
          }
          ctx.Emit({{"mod_name", RsSafeName(CamelToSnakeCase(msg.name()))},
                    {"nested_msgs",
                     [&] {
                       for (int i = 0; i < msg.nested_type_count(); ++i) {
                         GenerateRs(ctx, *msg.nested_type(i));
                       }
                     }},
                    {"nested_enums",
                     [&] {
                       for (int i = 0; i < msg.enum_type_count(); ++i) {
                         GenerateEnumDefinition(ctx, *msg.enum_type(i));
                       }
                     }},
                    {"oneofs",
                     [&] {
                       for (int i = 0; i < msg.real_oneof_decl_count(); ++i) {
                         GenerateOneofDefinition(ctx, *msg.real_oneof_decl(i));
                       }
                     }}},
                   R"rs(
                 pub mod $mod_name$ {
                   $nested_msgs$
                   $nested_enums$

                   $oneofs$
                 }  // mod $mod_name$
                )rs");
        }},
       {"raw_arena_getter_for_message",
        [&] {
          if (ctx.is_upb()) {
            ctx.Emit({}, R"rs(
                  fn arena(&self) -> &$pbr$::Arena {
                    &self.inner.arena
                  }
                  )rs");
          }
        }},
       {"raw_arena_getter_for_msgmut",
        [&] {
          if (ctx.is_upb()) {
            ctx.Emit({}, R"rs(
                  fn arena(&self) -> &$pbr$::Arena {
                    self.inner.arena($pbi$::Private)
                  }
                  )rs");
          }
        }},
       {"accessor_fns_for_views",
        [&] {
          for (int i = 0; i < msg.field_count(); ++i) {
            GenerateAccessorMsgImpl(ctx, *msg.field(i), AccessorCase::VIEW);
          }
          for (int i = 0; i < msg.real_oneof_decl_count(); ++i) {
            GenerateOneofAccessors(ctx, *msg.real_oneof_decl(i),
                                   AccessorCase::VIEW);
          }
        }},
       {"accessor_fns_for_muts",
        [&] {
          for (int i = 0; i < msg.field_count(); ++i) {
            GenerateAccessorMsgImpl(ctx, *msg.field(i), AccessorCase::MUT);
          }
          for (int i = 0; i < msg.real_oneof_decl_count(); ++i) {
            GenerateOneofAccessors(ctx, *msg.real_oneof_decl(i),
                                   AccessorCase::MUT);
          }
        }},
       {"into_proxied_impl", [&] { IntoProxiedForMessage(ctx, msg); }},
       {"get_upb_minitable", [&] { MessageGetMinitable(ctx, msg); }},
       {"repeated_impl", [&] { MessageProxiedInRepeated(ctx, msg); }},
       {"map_value_impl", [&] { MessageProxiedInMapValue(ctx, msg); }},
       {"unwrap_upb",
        [&] {
          if (ctx.is_upb()) {
            ctx.Emit(
                ".unwrap_or_else(||$pbr$::ScratchSpace::zeroed_block($pbi$::"
                "Private))");
          }
        }},
       {"upb_arena",
        [&] {
          if (ctx.is_upb()) {
            ctx.Emit(", inner.msg_ref().arena($pbi$::Private).raw()");
          }
        }}},
      R"rs(
        #[allow(non_camel_case_types)]
        pub struct $Msg$ {
          inner: $pbr$::MessageInner
        }

        impl std::fmt::Debug for $Msg$ {
          fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
            $Msg::debug$
          }
        }

        impl std::default::Default for $Msg$ {
          fn default() -> Self {
            Self::new()
          }
        }

        // SAFETY:
        // - `$Msg$` is `Sync` because it does not implement interior mutability.
        //    Neither does `$Msg$Mut`.
        unsafe impl Sync for $Msg$ {}

        // SAFETY:
        // - `$Msg$` is `Send` because it uniquely owns its arena and does
        //   not use thread-local data.
        unsafe impl Send for $Msg$ {}

        impl $pb$::Proxied for $Msg$ {
          type View<'msg> = $Msg$View<'msg>;
        }

        impl $pb$::MutProxied for $Msg$ {
          type Mut<'msg> = $Msg$Mut<'msg>;
        }

        #[derive(Copy, Clone)]
        #[allow(dead_code)]
        pub struct $Msg$View<'msg> {
          msg: $pbr$::RawMessage,
          _phantom: $Phantom$<&'msg ()>,
        }

        impl std::fmt::Debug for $Msg$View<'_> {
          fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
            $Msg::debug$
          }
        }

        #[allow(dead_code)]
        impl<'msg> $Msg$View<'msg> {
          #[doc(hidden)]
          pub fn new(_private: $pbi$::Private, msg: $pbr$::RawMessage) -> Self {
            Self { msg, _phantom: $std$::marker::PhantomData }
          }

          fn raw_msg(&self) -> $pbr$::RawMessage {
            self.msg
          }

          pub fn serialize(&self) -> $pbr$::SerializedData {
            $Msg::serialize$
          }

          $accessor_fns_for_views$
        }

        // SAFETY:
        // - `$Msg$View` is `Sync` because it does not support mutation.
        unsafe impl Sync for $Msg$View<'_> {}

        // SAFETY:
        // - `$Msg$View` is `Send` because while its alive a `$Msg$Mut` cannot.
        // - `$Msg$View` does not use thread-local data.
        unsafe impl Send for $Msg$View<'_> {}

        impl<'msg> $pb$::ViewProxy<'msg> for $Msg$View<'msg> {
          type Proxied = $Msg$;

          fn as_view(&self) -> $pb$::View<'msg, $Msg$> {
            *self
          }
          fn into_view<'shorter>(self) -> $pb$::View<'shorter, $Msg$> where 'msg: 'shorter {
            self
          }
        }

        $into_proxied_impl$

        $repeated_impl$
        $map_value_impl$

        #[allow(dead_code)]
        #[allow(non_camel_case_types)]
        pub struct $Msg$Mut<'msg> {
          inner: $pbr$::MutatorMessageRef<'msg>,
        }

        impl std::fmt::Debug for $Msg$Mut<'_> {
          fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
            $Msg::debug$
          }
        }

        #[allow(dead_code)]
        impl<'msg> $Msg$Mut<'msg> {
          #[doc(hidden)]
          pub fn from_parent(
                     _private: $pbi$::Private,
                     parent: $pbr$::MutatorMessageRef<'msg>,
                     msg: $pbr$::RawMessage)
            -> Self {
            Self {
              inner: $pbr$::MutatorMessageRef::from_parent(
                       $pbi$::Private, parent, msg)
            }
          }

          #[doc(hidden)]
          pub fn new(_private: $pbi$::Private, msg: &'msg mut $pbr$::MessageInner) -> Self {
            Self{ inner: $pbr$::MutatorMessageRef::new(_private, msg) }
          }

          #[deprecated = "This .or_default() is a no-op, usages can be safely removed"]
          pub fn or_default(self) -> Self { self }

          fn raw_msg(&self) -> $pbr$::RawMessage {
            self.inner.msg()
          }

          pub fn as_mutator_message_ref(&mut self, _private: $pbi$::Private)
            -> $pbr$::MutatorMessageRef<'msg> {
            self.inner
          }

          pub fn serialize(&self) -> $pbr$::SerializedData {
            $pb$::ViewProxy::as_view(self).serialize()
          }

          $get_upb_minitable$

          $raw_arena_getter_for_msgmut$

          $accessor_fns_for_muts$
        }

        // SAFETY:
        // - `$Msg$Mut` does not perform any shared mutation.
        // - `$Msg$Mut` is not `Send`, and so even in the presence of mutator
        //   splitting, synchronous access of an arena is impossible.
        unsafe impl Sync for $Msg$Mut<'_> {}

        impl<'msg> $pb$::MutProxy<'msg> for $Msg$Mut<'msg> {
          fn as_mut(&mut self) -> $pb$::Mut<'_, $Msg$> {
            $Msg$Mut { inner: self.inner }
          }
          fn into_mut<'shorter>(self) -> $pb$::Mut<'shorter, $Msg$> where 'msg : 'shorter { self }
        }

        impl<'msg> $pb$::ViewProxy<'msg> for $Msg$Mut<'msg> {
          type Proxied = $Msg$;
          fn as_view(&self) -> $pb$::View<'_, $Msg$> {
            $Msg$View { msg: self.raw_msg(), _phantom: $std$::marker::PhantomData }
          }
          fn into_view<'shorter>(self) -> $pb$::View<'shorter, $Msg$> where 'msg: 'shorter {
            $Msg$View { msg: self.raw_msg(), _phantom: $std$::marker::PhantomData }
          }
        }

        #[allow(dead_code)]
        impl $Msg$ {
          pub fn new() -> Self {
            $Msg::new$
          }

          fn raw_msg(&self) -> $pbr$::RawMessage {
            self.inner.msg
          }

          pub fn as_mutator_message_ref(&mut self, _private: $pbi$::Private) -> $pbr$::MutatorMessageRef {
            $pbr$::MutatorMessageRef::new($pbi$::Private, &mut self.inner)
          }

          $raw_arena_getter_for_message$

          pub fn serialize(&self) -> $pbr$::SerializedData {
            self.as_view().serialize()
          }
          #[deprecated = "Prefer Msg::parse(), or use the new name 'clear_and_parse' to parse into a pre-existing message."]
          pub fn deserialize(&mut self, data: &[u8]) -> Result<(), $pb$::ParseError> {
            self.clear_and_parse(data)
          }
          pub fn clear_and_parse(&mut self, data: &[u8]) -> Result<(), $pb$::ParseError> {
            $Msg::clear_and_parse$
          }
          pub fn parse(data: &[u8]) -> Result<Self, $pb$::ParseError> {
            let mut msg = Self::new();
            msg.clear_and_parse(data).map(|_| msg)
          }

          pub fn as_view(&self) -> $Msg$View {
            $Msg$View::new($pbi$::Private, self.inner.msg)
          }

          pub fn as_mut(&mut self) -> $Msg$Mut {
            $Msg$Mut::new($pbi$::Private, &mut self.inner)
          }

          $get_upb_minitable$

          $accessor_fns$
        }  // impl $Msg$

        //~ We implement drop unconditionally, so that `$Msg$: Drop` regardless
        //~ of kernel.
        impl $std$::ops::Drop for $Msg$ {
          fn drop(&mut self) {
            $Msg::drop$
          }
        }

        extern "C" {
          $Msg_externs$

          $accessor_externs$

          $oneof_externs$
        }  // extern "C" for $Msg$

        $nested_in_msg$
      )rs");

  if (ctx.is_cpp()) {
    ctx.printer().PrintRaw("\n");
    ctx.Emit({{"Msg", RsSafeName(msg.name())}}, R"rs(
      impl $Msg$ {
        pub fn __unstable_wrap_cpp_grant_permission_to_break(msg: $pbr$::RawMessage) -> Self {
          Self { inner: $pbr$::MessageInner { msg } }
        }
        pub fn __unstable_leak_cpp_repr_grant_permission_to_break(self) -> $pbr$::RawMessage {
          let s = std::mem::ManuallyDrop::new(self);
          s.raw_msg()
        }
      }

      impl<'a> $Msg$Mut<'a> {
        //~ msg is a &mut so that the borrow checker enforces exclusivity to
        //~ prevent constructing multiple Muts/Views from the same RawMessage.
        pub fn __unstable_wrap_cpp_grant_permission_to_break(
            msg: &'a mut $pbr$::RawMessage) -> Self {
          Self {
            inner: $pbr$::MutatorMessageRef::from_raw_msg($pbi$::Private, msg)
          }
        }
        pub fn __unstable_cpp_repr_grant_permission_to_break(self) -> $pbr$::RawMessage {
          self.raw_msg()
        }
      }

      impl<'a> $Msg$View<'a> {
        //~ msg is a & so that the caller can claim the message is live for the
        //~ corresponding lifetime.
        pub fn __unstable_wrap_cpp_grant_permission_to_break(
          msg: &'a $pbr$::RawMessage) -> Self {
          Self::new($pbi$::Private, *msg)
        }
        pub fn __unstable_cpp_repr_grant_permission_to_break(self) -> $pbr$::RawMessage {
          self.msg
        }
      }
    )rs");
  }
}

// Generates code for a particular message in `.pb.thunk.cc`.
void GenerateThunksCc(Context& ctx, const Descriptor& msg) {
  ABSL_CHECK(ctx.is_cpp());
  if (msg.map_key() != nullptr) {
    // Don't generate code for synthetic MapEntry messages.
    return;
  }

  ctx.Emit(
      {{"abi", "\"C\""},  // Workaround for syntax highlight bug in VSCode.
       {"Msg", RsSafeName(msg.name())},
       {"QualifiedMsg", cpp::QualifiedClassName(&msg)},
       {"new_thunk", ThunkName(ctx, msg, "new")},
       {"delete_thunk", ThunkName(ctx, msg, "delete")},
       {"serialize_thunk", ThunkName(ctx, msg, "serialize")},
       {"parse_thunk", ThunkName(ctx, msg, "parse")},
       {"copy_from_thunk", ThunkName(ctx, msg, "copy_from")},
       {"repeated_len_thunk", ThunkName(ctx, msg, "repeated_len")},
       {"repeated_get_thunk", ThunkName(ctx, msg, "repeated_get")},
       {"repeated_get_mut_thunk", ThunkName(ctx, msg, "repeated_get_mut")},
       {"repeated_add_thunk", ThunkName(ctx, msg, "repeated_add")},
       {"repeated_clear_thunk", ThunkName(ctx, msg, "repeated_clear")},
       {"repeated_copy_from_thunk", ThunkName(ctx, msg, "repeated_copy_from")},
       {"nested_msg_thunks",
        [&] {
          for (int i = 0; i < msg.nested_type_count(); ++i) {
            GenerateThunksCc(ctx, *msg.nested_type(i));
          }
          for (int i = 0; i < msg.enum_type_count(); ++i) {
            GenerateEnumThunksCc(ctx, *msg.enum_type(i));
          }
        }},
       {"accessor_thunks",
        [&] {
          for (int i = 0; i < msg.field_count(); ++i) {
            GenerateAccessorThunkCc(ctx, *msg.field(i));
          }
        }},
       {"oneof_thunks",
        [&] {
          for (int i = 0; i < msg.real_oneof_decl_count(); ++i) {
            GenerateOneofThunkCc(ctx, *msg.real_oneof_decl(i));
          }
        }}},
      R"cc(
        //~ $abi$ is a workaround for a syntax highlight bug in VSCode. However,
        //~ that confuses clang-format (it refuses to keep the newline after
        //~ `$abi${`). Disabling clang-format for the block.
        // clang-format off
        extern $abi$ {
        void* $new_thunk$() { return new $QualifiedMsg$(); }
        void $delete_thunk$(void* ptr) { delete static_cast<$QualifiedMsg$*>(ptr); }
        google::protobuf::rust_internal::SerializedData $serialize_thunk$($QualifiedMsg$* msg) {
          return google::protobuf::rust_internal::SerializeMsg(msg);
        }
        bool $parse_thunk$($QualifiedMsg$* msg,
                                 google::protobuf::rust_internal::SerializedData data) {
          return msg->ParseFromArray(data.data, data.len);
        }

        void $copy_from_thunk$($QualifiedMsg$* dst, const $QualifiedMsg$* src) {
          dst->CopyFrom(*src);
        }

        size_t $repeated_len_thunk$(google::protobuf::RepeatedPtrField<$QualifiedMsg$>* field) {
          return field->size();
        }
        const $QualifiedMsg$& $repeated_get_thunk$(
          google::protobuf::RepeatedPtrField<$QualifiedMsg$>* field,
          size_t index) {
          return field->Get(index);
        }
        $QualifiedMsg$* $repeated_get_mut_thunk$(
          google::protobuf::RepeatedPtrField<$QualifiedMsg$>* field,
          size_t index) {
          return field->Mutable(index);
        }
        $QualifiedMsg$* $repeated_add_thunk$(google::protobuf::RepeatedPtrField<$QualifiedMsg$>* field) {
          return field->Add();
        }
        void $repeated_clear_thunk$(google::protobuf::RepeatedPtrField<$QualifiedMsg$>* field) {
          field->Clear();
        }
        void $repeated_copy_from_thunk$(
          google::protobuf::RepeatedPtrField<$QualifiedMsg$>& dst,
          const google::protobuf::RepeatedPtrField<$QualifiedMsg$>& src) {
          dst = src;
        }

        $accessor_thunks$

        $oneof_thunks$
        }  // extern $abi$
        // clang-format on

        $nested_msg_thunks$
      )cc");
  for (const auto& t : kMapKeyTypes) {
    ctx.Emit(
        {
            {"map_new_thunk", RawMapThunk(ctx, msg, t.thunk_ident, "new")},
            {"map_free_thunk", RawMapThunk(ctx, msg, t.thunk_ident, "free")},
            {"map_clear_thunk", RawMapThunk(ctx, msg, t.thunk_ident, "clear")},
            {"map_size_thunk", RawMapThunk(ctx, msg, t.thunk_ident, "size")},
            {"map_insert_thunk",
             RawMapThunk(ctx, msg, t.thunk_ident, "insert")},
            {"map_get_thunk", RawMapThunk(ctx, msg, t.thunk_ident, "get")},
            {"map_remove_thunk",
             RawMapThunk(ctx, msg, t.thunk_ident, "remove")},
            {"map_iter_thunk", RawMapThunk(ctx, msg, t.thunk_ident, "iter")},
            {"map_iter_get_thunk",
             RawMapThunk(ctx, msg, t.thunk_ident, "iter_get")},
            {"key_t", t.cc_key_t},
            {"ffi_key_t", t.cc_ffi_key_t},
            {"key_expr", t.cc_from_ffi_key_expr},
            {"to_ffi_key_expr", t.cc_to_ffi_key_expr},
            {"pkg::Msg", cpp::QualifiedClassName(&msg)},
            {"abi", "\"C\""},  // Workaround for syntax highlight bug in VSCode.
        },
        R"cc(
          extern $abi$ {
            const google::protobuf::Map<$key_t$, $pkg::Msg$>* $map_new_thunk$() {
              return new google::protobuf::Map<$key_t$, $pkg::Msg$>();
            }
            void $map_free_thunk$(const google::protobuf::Map<$key_t$, $pkg::Msg$>* m) { delete m; }
            void $map_clear_thunk$(google::protobuf::Map<$key_t$, $pkg::Msg$> * m) { m->clear(); }
            size_t $map_size_thunk$(const google::protobuf::Map<$key_t$, $pkg::Msg$>* m) {
              return m->size();
            }
            bool $map_insert_thunk$(google::protobuf::Map<$key_t$, $pkg::Msg$> * m,
                                    $ffi_key_t$ key, $pkg::Msg$ value) {
              auto k = $key_expr$;
              auto it = m->find(k);
              if (it != m->end()) {
                return false;
              }
              (*m)[k] = value;
              return true;
            }
            bool $map_get_thunk$(const google::protobuf::Map<$key_t$, $pkg::Msg$>* m,
                                 $ffi_key_t$ key, const $pkg::Msg$** value) {
              auto it = m->find($key_expr$);
              if (it == m->end()) {
                return false;
              }
              const $pkg::Msg$* cpp_value = &it->second;
              *value = cpp_value;
              return true;
            }
            bool $map_remove_thunk$(google::protobuf::Map<$key_t$, $pkg::Msg$> * m,
                                    $ffi_key_t$ key, $pkg::Msg$ * value) {
              auto num_removed = m->erase($key_expr$);
              return num_removed > 0;
            }
            google::protobuf::internal::UntypedMapIterator $map_iter_thunk$(
                const google::protobuf::Map<$key_t$, $pkg::Msg$>* m) {
              return google::protobuf::internal::UntypedMapIterator::FromTyped(m->cbegin());
            }
            void $map_iter_get_thunk$(
                const google::protobuf::internal::UntypedMapIterator* iter,
                $ffi_key_t$* key, const $pkg::Msg$** value) {
              auto typed_iter = iter->ToTyped<
                  google::protobuf::Map<$key_t$, $pkg::Msg$>::const_iterator>();
              const auto& cpp_key = typed_iter->first;
              const auto& cpp_value = typed_iter->second;
              *key = $to_ffi_key_expr$;
              *value = &cpp_value;
            }
          }
        )cc");
  }
}

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
