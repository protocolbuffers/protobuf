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
      ctx.Emit({{"serialize_thunk", ThunkName(ctx, msg, "serialize")}}, R"rs(
        let arena = $pbr$::Arena::new();
        let mut len = 0;
        unsafe {
          let data = $serialize_thunk$(self.raw_msg(), arena.raw(), &mut len);
          $pbr$::SerializedData::from_raw_parts(arena, data, len)
        }
      )rs");
      return;
  }

  ABSL_LOG(FATAL) << "unreachable";
}

void MessageDeserialize(Context& ctx, const Descriptor& msg) {
  switch (ctx.opts().kernel) {
    case Kernel::kCpp:
      ctx.Emit(
          {
              {"deserialize_thunk", ThunkName(ctx, msg, "deserialize")},
          },
          R"rs(
          let success = unsafe {
            let data = $pbr$::SerializedData::from_raw_parts(
              $NonNull$::new(data.as_ptr() as *mut _).unwrap(),
              data.len(),
            );

            $deserialize_thunk$(self.raw_msg(), data)
          };
          success.then_some(()).ok_or($pb$::ParseError)
        )rs");
      return;

    case Kernel::kUpb:
      ctx.Emit({{"deserialize_thunk", ThunkName(ctx, msg, "parse")}}, R"rs(
        let arena = $pbr$::Arena::new();
        let msg = unsafe {
          $deserialize_thunk$(data.as_ptr(), data.len(), arena.raw())
        };

        match msg {
          None => Err($pb$::ParseError),
          Some(msg) => {
            //~ This assignment causes self.arena to be dropped and to deallocate
            //~ any previous message pointed/owned to by self.inner.msg.
            self.inner.arena = arena;
            self.inner.msg = msg;
            Ok(())
          }
        }
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
              {"deserialize_thunk", ThunkName(ctx, msg, "deserialize")},
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
          fn $new_thunk$() -> $pbi$::RawMessage;
          fn $delete_thunk$(raw_msg: $pbi$::RawMessage);
          fn $serialize_thunk$(raw_msg: $pbi$::RawMessage) -> $pbr$::SerializedData;
          fn $deserialize_thunk$(raw_msg: $pbi$::RawMessage, data: $pbr$::SerializedData) -> bool;
          fn $copy_from_thunk$(dst: $pbi$::RawMessage, src: $pbi$::RawMessage);
          fn $repeated_len_thunk$(raw: $pbi$::RawRepeatedField) -> usize;
          fn $repeated_add_thunk$(raw: $pbi$::RawRepeatedField) -> $pbi$::RawMessage;
          fn $repeated_get_thunk$(raw: $pbi$::RawRepeatedField, index: usize) -> $pbi$::RawMessage;
          fn $repeated_get_mut_thunk$(raw: $pbi$::RawRepeatedField, index: usize) -> $pbi$::RawMessage;
          fn $repeated_clear_thunk$(raw: $pbi$::RawRepeatedField);
          fn $repeated_copy_from_thunk$(dst: $pbi$::RawRepeatedField, src: $pbi$::RawRepeatedField);
        )rs");
      return;

    case Kernel::kUpb:
      ctx.Emit(
          {
              {"new_thunk", ThunkName(ctx, msg, "new")},
              {"serialize_thunk", ThunkName(ctx, msg, "serialize")},
              {"deserialize_thunk", ThunkName(ctx, msg, "parse")},
              {"minitable", UpbMinitableName(msg)},
          },
          R"rs(
          fn $new_thunk$(arena: $pbi$::RawArena) -> $pbi$::RawMessage;
          fn $serialize_thunk$(msg: $pbi$::RawMessage, arena: $pbi$::RawArena, len: &mut usize) -> $NonNull$<u8>;
          fn $deserialize_thunk$(data: *const u8, size: usize, arena: $pbi$::RawArena) -> Option<$pbi$::RawMessage>;
          /// Opaque wrapper for this message's MiniTable. The only valid way to
          /// reference this static is with `std::ptr::addr_of!(..)`.
          static $minitable$: $pbr$::OpaqueMiniTable;
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

void MessageSettableValue(Context& ctx, const Descriptor& msg) {
  switch (ctx.opts().kernel) {
    case Kernel::kCpp:
      ctx.Emit({{"copy_from_thunk", ThunkName(ctx, msg, "copy_from")}}, R"rs(
        impl<'msg> $pb$::SettableValue<$Msg$> for $Msg$View<'msg> {
          fn set_on<'dst>(
            self, _private: $pbi$::Private, mutator: $pb$::Mut<'dst, $Msg$>)
            where $Msg$: 'dst {
            unsafe { $copy_from_thunk$(mutator.inner.msg(), self.msg) };
          }
        }
      )rs");
      return;

    case Kernel::kUpb:
      // TODO: Add owned SettableValue impl for upb messages.
      ctx.Emit({{"minitable", UpbMinitableName(msg)}}, R"rs(
        impl<'msg> $pb$::SettableValue<$Msg$> for $Msg$View<'msg> {
          fn set_on<'dst>(
            self, _private: $pbi$::Private, mutator: $pb$::Mut<'dst, $Msg$>)
            where $Msg$: 'dst {
            unsafe { $pbr$::upb_Message_DeepCopy(
              mutator.inner.msg(),
              self.msg,
              $std$::ptr::addr_of!($minitable$),
              mutator.inner.arena($pbi$::Private).raw(),
            ) };
          }
        }
      )rs");
      return;
  }

  ABSL_LOG(FATAL) << "unreachable";
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
            let mut dest_msg = unsafe {
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
            mut dest: $pb$::Mut<$pb$::Repeated<Self>>,
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

}  // namespace

void GenerateRs(Context& ctx, const Descriptor& msg) {
  if (msg.map_key() != nullptr) {
    ABSL_LOG(WARNING) << "unsupported map field: " << msg.full_name();
    return;
  }
  ctx.Emit(
      {{"Msg", RsSafeName(msg.name())},
       {"Msg::new", [&] { MessageNew(ctx, msg); }},
       {"Msg::serialize", [&] { MessageSerialize(ctx, msg); }},
       {"Msg::deserialize", [&] { MessageDeserialize(ctx, msg); }},
       {"Msg::drop", [&] { MessageDrop(ctx, msg); }},
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
          // emitting an empty mod SomeMsg_.
          if (msg.nested_type_count() == 0 && msg.enum_type_count() == 0 &&
              msg.real_oneof_decl_count() == 0) {
            return;
          }
          ctx.Emit({{"Msg", RsSafeName(msg.name())},
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
                 #[allow(non_snake_case)]
                 pub mod $Msg$_ {
                   $nested_msgs$
                   $nested_enums$

                   $oneofs$
                 }  // mod $Msg$_
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
       {"settable_impl", [&] { MessageSettableValue(ctx, msg); }},
       {"repeated_impl", [&] { MessageProxiedInRepeated(ctx, msg); }}},
      R"rs(
        #[allow(non_camel_case_types)]
        //~ TODO: Implement support for debug redaction
        #[derive(Debug)]
        pub struct $Msg$ {
          inner: $pbr$::MessageInner
        }

        // SAFETY:
        // - `$Msg$` does not provide shared mutation with its arena.
        // - `$Msg$Mut` is not `Send`, and so even in the presence of mutator
        //   splitting, synchronous access of an arena that would conflict with
        //   field access is impossible.
        unsafe impl Sync for $Msg$ {}

        impl $pb$::Proxied for $Msg$ {
          type View<'msg> = $Msg$View<'msg>;
          type Mut<'msg> = $Msg$Mut<'msg>;
        }

        #[derive(Debug, Copy, Clone)]
        #[allow(dead_code)]
        pub struct $Msg$View<'msg> {
          msg: $pbi$::RawMessage,
          _phantom: $Phantom$<&'msg ()>,
        }

        #[allow(dead_code)]
        impl<'msg> $Msg$View<'msg> {
          #[doc(hidden)]
          pub fn new(_private: $pbi$::Private, msg: $pbi$::RawMessage) -> Self {
            Self { msg, _phantom: std::marker::PhantomData }
          }

          fn raw_msg(&self) -> $pbi$::RawMessage {
            self.msg
          }

          $accessor_fns_for_views$
        }

        // SAFETY:
        // - `$Msg$View` does not perform any mutation.
        // - While a `$Msg$View` exists, a `$Msg$Mut` can't exist to mutate
        //   the arena that would conflict with field access.
        // - `$Msg$Mut` is not `Send`, and so even in the presence of mutator
        //   splitting, synchronous access of an arena is impossible.
        unsafe impl Sync for $Msg$View<'_> {}
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

        impl $pbi$::ProxiedWithRawVTable for $Msg$ {
          type VTable = $pbi$::MessageVTable;

          fn make_view(_private: $pbi$::Private,
                      mut_inner: $pbi$::RawVTableMutator<'_, Self>)
                      -> $pb$::View<'_, Self> {
            let msg = unsafe {
              (mut_inner.vtable().getter)(mut_inner.msg_ref().msg())
            };
            $Msg$View::new($pbi$::Private, msg)
          }

          fn make_mut(_private: $pbi$::Private,
                      inner: $pbi$::RawVTableMutator<'_, Self>)
                      -> $pb$::Mut<'_, Self> {
            let raw_submsg = unsafe {
              (inner.vtable().mut_getter)(inner.msg_ref().msg())
            };
            $Msg$Mut::from_parent($pbi$::Private, inner.msg_ref(), raw_submsg)
          }
        }

        impl $pbi$::ProxiedWithRawOptionalVTable for $Msg$ {
          type OptionalVTable = $pbi$::MessageVTable;

          fn upcast_vtable(_private: $pbi$::Private,
                           optional_vtable: &'static Self::OptionalVTable)
                          -> &'static Self::VTable {
            &optional_vtable
          }
        }

        impl $pb$::ProxiedWithPresence for $Msg$ {
          type PresentMutData<'a> = $pbr$::MessagePresentMutData<'a, $Msg$>;
          type AbsentMutData<'a> = $pbr$::MessageAbsentMutData<'a, $Msg$>;

          fn clear_present_field(present_mutator: Self::PresentMutData<'_>)
             -> Self::AbsentMutData<'_> {
             // SAFETY: The raw ptr msg_ref is valid
            unsafe {
              (present_mutator.optional_vtable().clearer);
              (present_mutator.msg_ref().msg());

             $pbi$::RawVTableOptionalMutatorData::new($pbi$::Private,
               present_mutator.msg_ref(),
               present_mutator.optional_vtable())
            }
          }

          fn set_absent_to_default(absent_mutator: Self::AbsentMutData<'_>)
             -> Self::PresentMutData<'_> {
           unsafe {
             $pbi$::RawVTableOptionalMutatorData::new($pbi$::Private,
               absent_mutator.msg_ref(),
               absent_mutator.optional_vtable())
           }
          }
        }

        $settable_impl$
        $repeated_impl$

        #[derive(Debug)]
        #[allow(dead_code)]
        #[allow(non_camel_case_types)]
        pub struct $Msg$Mut<'msg> {
          inner: $pbr$::MutatorMessageRef<'msg>,
        }

        #[allow(dead_code)]
        impl<'msg> $Msg$Mut<'msg> {
          #[doc(hidden)]
          pub fn from_parent(
                     _private: $pbi$::Private,
                     parent: $pbr$::MutatorMessageRef<'msg>,
                     msg: $pbi$::RawMessage)
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

          fn raw_msg(&self) -> $pbi$::RawMessage {
            self.inner.msg()
          }

          fn as_mutator_message_ref(&mut self) -> $pbr$::MutatorMessageRef<'msg> {
            self.inner
          }

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
            $Msg$View { msg: self.raw_msg(), _phantom: std::marker::PhantomData }
          }
          fn into_view<'shorter>(self) -> $pb$::View<'shorter, $Msg$> where 'msg: 'shorter {
            $Msg$View { msg: self.raw_msg(), _phantom: std::marker::PhantomData }
          }
        }

        #[allow(dead_code)]
        impl $Msg$ {
          pub fn new() -> Self {
            $Msg::new$
          }

          fn raw_msg(&self) -> $pbi$::RawMessage {
            self.inner.msg
          }

          fn as_mutator_message_ref(&mut self) -> $pbr$::MutatorMessageRef {
            $pbr$::MutatorMessageRef::new($pbi$::Private, &mut self.inner)
          }

          $raw_arena_getter_for_message$

          pub fn serialize(&self) -> $pbr$::SerializedData {
            $Msg::serialize$
          }
          pub fn deserialize(&mut self, data: &[u8]) -> Result<(), $pb$::ParseError> {
            $Msg::deserialize$
          }

          pub fn as_view(&self) -> $Msg$View {
            $Msg$View::new($pbi$::Private, self.inner.msg)
          }

          pub fn as_mut(&mut self) -> $Msg$Mut {
            $Msg$Mut::new($pbi$::Private, &mut self.inner)
          }

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
        pub fn __unstable_wrap_cpp_grant_permission_to_break(msg: $pbi$::RawMessage) -> Self {
          Self { inner: $pbr$::MessageInner { msg } }
        }
        pub fn __unstable_cpp_repr_grant_permission_to_break(&mut self) -> $pbi$::RawMessage {
          self.raw_msg()
        }
      }
    )rs");
  }
}

// Generates code for a particular message in `.pb.thunk.cc`.
void GenerateThunksCc(Context& ctx, const Descriptor& msg) {
  ABSL_CHECK(ctx.is_cpp());
  if (msg.map_key() != nullptr) {
    ABSL_LOG(WARNING) << "unsupported map field: " << msg.full_name();
    return;
  }

  ctx.Emit(
      {{"abi", "\"C\""},  // Workaround for syntax highlight bug in VSCode.
       {"Msg", RsSafeName(msg.name())},
       {"QualifiedMsg", cpp::QualifiedClassName(&msg)},
       {"new_thunk", ThunkName(ctx, msg, "new")},
       {"delete_thunk", ThunkName(ctx, msg, "delete")},
       {"serialize_thunk", ThunkName(ctx, msg, "serialize")},
       {"deserialize_thunk", ThunkName(ctx, msg, "deserialize")},
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
        bool $deserialize_thunk$($QualifiedMsg$* msg,
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
}

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
