// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/rust/message.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/strings/ascii.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/cpp/names.h"
#include "google/protobuf/compiler/rust/accessors/accessor_case.h"
#include "google/protobuf/compiler/rust/accessors/accessors.h"
#include "google/protobuf/compiler/rust/context.h"
#include "google/protobuf/compiler/rust/enum.h"
#include "google/protobuf/compiler/rust/naming.h"
#include "google/protobuf/compiler/rust/oneof.h"
#include "google/protobuf/compiler/rust/upb_helpers.h"
#include "google/protobuf/compiler/scc.h"
#include "google/protobuf/descriptor.h"
#include "upb/mini_descriptor/link.h"
#include "upb/mini_table/field.h"
#include "upb/reflection/def.hpp"

namespace google {
namespace protobuf {
namespace compiler {
namespace rust {
namespace {

void MessageNew(Context& ctx, const Descriptor& msg) {
  switch (ctx.opts().kernel) {
    case Kernel::kCpp:
      ctx.Emit({{"new_thunk", ThunkName(ctx, msg, "new")}}, R"rs(
        let raw = unsafe { $new_thunk$() };
        let inner = unsafe { $pbr$::OwnedMessageInner::<Self>::wrap_raw(raw) };
        Self { inner }
      )rs");
      return;

    case Kernel::kUpb:
      ctx.Emit(R"rs(
        Self { inner: $pbr$::OwnedMessageInner::<Self>::new() }
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
        $pbr$::debug_string(self.raw_msg(), f)
      )rs");
      return;

    case Kernel::kUpb:
      ctx.Emit(
          R"rs(
        write!(f, "{}", $pbr$::debug_string(self))
      )rs");
      return;
  }

  ABSL_LOG(FATAL) << "unreachable";
}

void CppMessageExterns(Context& ctx, const Descriptor& msg) {
  ABSL_CHECK(ctx.is_cpp());

  ctx.Emit(
      {{"new_thunk", ThunkName(ctx, msg, "new")},
       {"default_instance_thunk", ThunkName(ctx, msg, "default_instance")}},
      R"rs(
      fn $new_thunk$() -> $pbr$::RawMessage;
      fn $default_instance_thunk$() -> $pbr$::RawMessage;
    )rs");
}

void MessageDrop(Context& ctx, const Descriptor& msg) {
  if (ctx.is_upb()) {
    // Nothing to do here; drop glue (which will run drop(self.arena)
    // automatically) is sufficient.
    return;
  }

  ctx.Emit(R"rs(
    unsafe { $pbr$::proto2_rust_Message_delete(self.raw_msg()); }
  )rs");
}

void IntoProxiedForMessage(Context& ctx, const Descriptor& msg) {
  switch (ctx.opts().kernel) {
    case Kernel::kCpp:
      ctx.Emit(R"rs(
        impl<'msg> $pb$::IntoProxied<$Msg$> for $Msg$View<'msg> {
          fn into_proxied(self, _private: $pbi$::Private) -> $Msg$ {
            let dst = $Msg$::new();
            unsafe { $pbr$::proto2_rust_Message_copy_from(dst.inner.raw(), self.inner.raw()) };
            dst
          }
        }

        impl<'msg> $pb$::IntoProxied<$Msg$> for $Msg$Mut<'msg> {
          fn into_proxied(self, _private: $pbi$::Private) -> $Msg$ {
            $pb$::IntoProxied::into_proxied($pb$::IntoView::into_view(self), _private)
          }
        }
      )rs");
      return;

    case Kernel::kUpb:
      ctx.Emit(R"rs(
        impl<'msg> $pb$::IntoProxied<$Msg$> for $Msg$View<'msg> {
          fn into_proxied(self, _private: $pbi$::Private) -> $Msg$ {
            let mut dst = $Msg$::new();
            assert!(unsafe {
              dst.inner.ptr_mut().deep_copy(self.inner.ptr(), dst.inner.arena())
            });
            dst
          }
        }

        impl<'msg> $pb$::IntoProxied<$Msg$> for $Msg$Mut<'msg> {
          fn into_proxied(self, _private: $pbi$::Private) -> $Msg$ {
            $pb$::IntoProxied::into_proxied($pb$::IntoView::into_view(self), _private)
          }
        }
      )rs");
      return;
  }

  ABSL_LOG(FATAL) << "unreachable";
}

// Generates code for linking the message's minitable to its dependencies.
void UpbMiniTableLinking(Context& ctx, const Descriptor& msg,
                         upb::MessageDefPtr upb_msg, const SCC& scc) {
  std::vector<const upb_MiniTableField*> subs;
  subs.resize(static_cast<size_t>(msg.field_count()));
  uint32_t counts = upb_MiniTable_GetSubList(upb_msg.mini_table(), subs.data());
  size_t message_count = counts >> 16;
  size_t enum_count = static_cast<uint16_t>(counts);
  ctx.Emit(
      {{"submessages",
        [&] {
          for (size_t i = 0; i < message_count; ++i) {
            const Descriptor& m =
                *msg.FindFieldByNumber(
                        static_cast<int>(upb_MiniTableField_Number(subs[i])))
                     ->message_type();
            if (scc.Contains(m)) {
              ctx.Emit({{"minitable_symbol_name",
                         QualifiedUpbMiniTableName(ctx, m)}},
                       "$minitable_symbol_name$.0,\n");
            } else {
              ctx.Emit({{"name", RsTypePath(ctx, m)}},
                       "<$name$ as "
                       "$pbr$::AssociatedMiniTable>::mini_table(),\n");
            }
          }
        }},
       {"subenums",
        [&] {
          for (size_t i = message_count; i < message_count + enum_count; ++i) {
            const EnumDescriptor* m =
                msg.FindFieldByNumber(
                       static_cast<int>(upb_MiniTableField_Number(subs[i])))
                    ->enum_type();
            ctx.Emit({{"name", RsTypePath(ctx, *m)}},
                     "<$name$ as "
                     "$pbr$::AssociatedMiniTableEnum>::"
                     "mini_table(),\n");
          }
        }},
       {"minitable_symbol_name", QualifiedUpbMiniTableName(ctx, msg)}},
      R"rs(
      $pbr$::link_mini_table(
          $minitable_symbol_name$.0, &[$submessages$], &[$subenums$]);
  )rs");
}

void CppGeneratedMessageTraitImpls(Context& ctx, const Descriptor& msg) {
  ABSL_CHECK(ctx.is_cpp());
  ctx.Emit(R"rs(
    unsafe impl $pbr$::CppGetRawMessageMut for $Msg$Mut<'_> {
      fn get_raw_message_mut(&mut self, _private: $pbi$::Private) -> $pbr$::RawMessage {
        self.inner.raw()
      }
    }

    unsafe impl $pbr$::CppGetRawMessage for $Msg$View<'_> {
      fn get_raw_message(&self, _private: $pbi$::Private) -> $pbr$::RawMessage {
        self.inner.raw()
      }
    }
  )rs");
}

void UpbGeneratedMessageTraitImpls(Context& ctx, const Descriptor& msg,
                                   const upb::DefPool& pool) {
  ABSL_CHECK(ctx.is_upb());
  ctx.Emit(
      {{"name", RsSafeName(msg.name())},
       {"mini_table_impl",
        [&] {
          const SCC& scc = ctx.GetSCC(msg);
          if (scc.GetRepresentative() == &msg) {
            for (const Descriptor* d : scc.descriptors) {
              std::string mini_descriptor =
                  pool.FindMessageByName(d->full_name().data())
                      .MiniDescriptorEncode();
              ctx.Emit({{"name", RsTypePath(ctx, *d)},
                        {"minitable_symbol_name",
                         QualifiedUpbMiniTableName(ctx, *d)},
                        {"mini_descriptor", mini_descriptor},
                        {"mini_descriptor_length", mini_descriptor.size()}},
                       R"rs(
                       $minitable_symbol_name$.0 =
                           $pbr$::build_mini_table("$mini_descriptor$");
              )rs");
            }
            for (const Descriptor* d : scc.descriptors) {
              UpbMiniTableLinking(
                  ctx, *d, pool.FindMessageByName(d->full_name().data()), scc);
            }
          } else {
            ctx.Emit(
                {{"representative", RsTypePath(ctx, *scc.GetRepresentative())}},
                "<$representative$ as "
                "$pbr$::AssociatedMiniTable>::mini_table();\n");
          }
        }},
       {"minitable_symbol_name", QualifiedUpbMiniTableName(ctx, msg)}},
      // We currently synchronize the minitable initialization using a
      // OnceLock, but we could instead use CAS if we ever want to make this
      // lock-free.
      R"rs(
      unsafe impl $pbr$::AssociatedMiniTable for $name$ {
        fn mini_table() -> $pbr$::MiniTablePtr {
          static ONCE_LOCK: $std$::sync::OnceLock<$pbr$::MiniTableInitPtr> =
              $std$::sync::OnceLock::new();
          unsafe {
            ONCE_LOCK.get_or_init(|| {
              $mini_table_impl$
              $pbr$::MiniTableInitPtr($minitable_symbol_name$.0)
            }).0
          }
        }
      }
    )rs");

  if (msg.options().map_entry()) {
    return;
  }
  ctx.Emit(R"rs(
      unsafe impl $pbr$::UpbGetArena for $Msg$ {
        fn get_arena(&mut self, _private: $pbi$::Private) -> &$pbr$::Arena {
          self.inner.arena()
        }
      }

      unsafe impl $pbr$::UpbGetMessagePtrMut for $Msg$ {
        type Msg = $Msg$;
        fn get_ptr_mut(&mut self, _private: $pbi$::Private) -> $pbr$::MessagePtr<$Msg$> {
          self.inner.ptr_mut()
        }
      }
      unsafe impl $pbr$::UpbGetMessagePtr for $Msg$ {
        type Msg = $Msg$;
        fn get_ptr(&self, _private: $pbi$::Private) -> $pbr$::MessagePtr<$Msg$> {
          self.inner.ptr()
        }
      }
      unsafe impl $pbr$::UpbGetMessagePtrMut for $Msg$Mut<'_> {
        type Msg = $Msg$;
        fn get_ptr_mut(&mut self, _private: $pbi$::Private) -> $pbr$::MessagePtr<$Msg$> {
          self.inner.ptr_mut()
        }
      }
      unsafe impl $pbr$::UpbGetMessagePtr for $Msg$Mut<'_> {
        type Msg = $Msg$;
        fn get_ptr(&self, _private: $pbi$::Private) -> $pbr$::MessagePtr<$Msg$> {
          self.inner.ptr()
        }
      }
      unsafe impl $pbr$::UpbGetMessagePtr for $Msg$View<'_> {
        type Msg = $Msg$;
        fn get_ptr(&self, _private: $pbi$::Private) -> $pbr$::MessagePtr<$Msg$> {
          self.inner.ptr()
        }
      }

      unsafe impl $pbr$::UpbGetArena for $Msg$Mut<'_> {
        fn get_arena(&mut self, _private: $pbi$::Private) -> &$pbr$::Arena {
          self.inner.arena()
        }
      }
    )rs");
}

void TypeConversions(Context& ctx, const Descriptor& msg) {
  switch (ctx.opts().kernel) {
    case Kernel::kCpp:
      ctx.Emit(
          R"rs(
          impl $pbr$::CppMapTypeConversions for $Msg$ {
              fn get_prototype() -> $pbr$::FfiMapValue {
                  $pbr$::FfiMapValue::make_message(<$Msg$View as $std$::default::Default>::default().raw_msg())
              }

              fn to_map_value(self) -> $pbr$::FfiMapValue {
                  $pbr$::FfiMapValue::make_message(std::mem::ManuallyDrop::new(self).raw_msg())
              }

              unsafe fn from_map_value<'b>(value: $pbr$::FfiMapValue) -> $Msg$View<'b> {
                  debug_assert_eq!(value.tag, $pbr$::FfiMapValueTag::Message);
                  unsafe { $pbr$::MessageViewInner::wrap_raw(value.val.m).into() }
              }

              unsafe fn mut_from_map_value<'b>(value: $pbr$::FfiMapValue) -> $Msg$Mut<'b> {
                  debug_assert_eq!(value.tag, $pbr$::FfiMapValueTag::Message);
                  let inner = unsafe { $pbr$::MessageMutInner::wrap_raw(value.val.m) };
                  $Msg$Mut { inner }
              }
          }
          )rs");
      return;
    case Kernel::kUpb:
      ctx.Emit(
          {
              {"new_thunk", ThunkName(ctx, msg, "new")},
          },
          R"rs(
            impl $pbr$::EntityType for $Msg$ {
                type Tag = $pbr$::MessageTag;
            }

            impl<'msg> $pbr$::EntityType for $Msg$View<'msg> {
                type Tag = $pbr$::ViewProxyTag;
            }

            impl<'msg> $pbr$::EntityType for $Msg$Mut<'msg> {
                type Tag = $pbr$::MutProxyTag;
            }
            )rs");
  }
}

void GenerateDefaultInstanceImpl(Context& ctx, const Descriptor& msg) {
  if (ctx.is_upb()) {
    ctx.Emit("$pbr$::MessageViewInner::default()");
  } else {
    ctx.Emit(
        {{"default_instance_thunk", ThunkName(ctx, msg, "default_instance")}},
        R"rs(
        unsafe {
          $pbr$::MessageViewInner::wrap_raw($default_instance_thunk$())
        }
        )rs");
  }
}

}  // namespace

void GenerateRs(Context& ctx, const Descriptor& msg, const upb::DefPool& pool) {
  if (ctx.is_upb()) {
    ctx.Emit({{"minitable_symbol_name", UpbMiniTableName(msg)}},
             R"rs(
        // This variable must not be referenced except by protobuf generated
        // code.
        pub(crate) static mut $minitable_symbol_name$: $pbr$::MiniTableInitPtr =
            $pbr$::MiniTableInitPtr($pbr$::MiniTablePtr::dangling());
    )rs");
  }

  if (msg.options().map_entry()) {
    if (ctx.is_upb()) {
      // Map entry messages are an implementation detail, so we restrict their
      // visibility. The only reason we generate anything for them at all is
      // that it is useful to have map entries implement the
      // AssociatedMiniTable trait.
      ctx.Emit({{"Msg", RsSafeName(msg.name())},
                {"upb_generated_message_trait_impls",
                 [&] { UpbGeneratedMessageTraitImpls(ctx, msg, pool); }}},
               R"rs(
          #[allow(dead_code)]
          pub(super) struct $Msg$;

          $upb_generated_message_trait_impls$
      )rs");
    }
    return;
  }

  upb::MessageDefPtr upb_msg = pool.FindMessageByName(msg.full_name().data());
  ctx.Emit(
      {
          {"Msg", RsSafeName(msg.name())},
          {"Msg::new", [&] { MessageNew(ctx, msg); }},
          {"Msg::drop", [&] { MessageDrop(ctx, msg); }},
          {"Msg::debug", [&] { MessageDebug(ctx, msg); }},
          {"default_instance_impl",
           [&] { GenerateDefaultInstanceImpl(ctx, msg); }},
          {"raw_msg",
           [&] {
             // The raw_msg() method is emitted for the C++ kernel only,
             // because we do not need it for upb.
             if (ctx.is_cpp()) {
               ctx.Emit(R"rs(
                 fn raw_msg(&self) -> $pbr$::RawMessage {
                   self.inner.raw()
                 }
               )rs");
             }
           }},
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
          {"nested_in_msg",
           [&] {
             // If we have no nested types, enums, or oneofs, bail out without
             // emitting an empty mod some_msg.
             if (msg.nested_type_count() == 0 && msg.enum_type_count() == 0 &&
                 msg.real_oneof_decl_count() == 0) {
               return;
             }
             ctx.PushModule(RsSafeName(CamelToSnakeCase(msg.name())));
             ctx.Emit(
                 {{"nested_msgs",
                   [&] {
                     for (int i = 0; i < msg.nested_type_count(); ++i) {
                       GenerateRs(ctx, *msg.nested_type(i), pool);
                     }
                   }},
                  {"nested_enums",
                   [&] {
                     for (int i = 0; i < msg.enum_type_count(); ++i) {
                       GenerateEnumDefinition(ctx, *msg.enum_type(i),
                                              upb_msg.enum_type(i));
                     }
                   }},
                  {"oneofs",
                   [&] {
                     for (int i = 0; i < msg.real_oneof_decl_count(); ++i) {
                       GenerateOneofDefinition(ctx, *msg.real_oneof_decl(i));
                     }
                   }}},
                 R"rs(
                   $nested_msgs$
                   $nested_enums$

                   $oneofs$
                )rs");
             ctx.PopModule();
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
          {"generated_message_trait_impls",
           [&] {
             if (ctx.is_upb()) {
               UpbGeneratedMessageTraitImpls(ctx, msg, pool);
             } else {
               CppGeneratedMessageTraitImpls(ctx, msg);
             }
           }},
          {"type_conversions_impl", [&] { TypeConversions(ctx, msg); }},
          {"unwrap_upb",
           [&] {
             if (ctx.is_upb()) {
               ctx.Emit(
                   ".unwrap_or_else(||$pbr$::ScratchSpace::zeroed_block())");
             }
           }},
          {"upb_arena",
           [&] {
             if (ctx.is_upb()) {
               ctx.Emit(", inner.msg_ref().arena().raw()");
             }
           }},
      },
      R"rs(
        #[allow(non_camel_case_types)]
        pub struct $Msg$ {
          inner: $pbr$::OwnedMessageInner<$Msg$>
        }

        impl $pb$::Message for $Msg$ {
          type MessageView<'msg> = $Msg$View<'msg>;
          type MessageMut<'msg> = $Msg$Mut<'msg>;
        }

        impl $std$::default::Default for $Msg$ {
          fn default() -> Self {
            Self::new()
          }
        }

        impl $std$::fmt::Debug for $Msg$ {
          fn fmt(&self, f: &mut $std$::fmt::Formatter<'_>) -> $std$::fmt::Result {
            $Msg::debug$
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

        impl $pbi$::SealedInternal for $Msg$ {}

        impl $pb$::MutProxied for $Msg$ {
          type Mut<'msg> = $Msg$Mut<'msg>;
        }

        #[derive(Copy, Clone)]
        #[allow(dead_code)]
        pub struct $Msg$View<'msg> {
          inner: $pbr$::MessageViewInner<'msg, $Msg$>,
        }

        impl<'msg> $pbi$::SealedInternal for $Msg$View<'msg> {}

        impl<'msg> $pb$::MessageView<'msg> for $Msg$View<'msg> {
          type Message = $Msg$;
        }

        impl $std$::fmt::Debug for $Msg$View<'_> {
          fn fmt(&self, f: &mut $std$::fmt::Formatter<'_>) -> $std$::fmt::Result {
            $Msg::debug$
          }
        }

        impl $std$::default::Default for $Msg$View<'_> {
          fn default() -> $Msg$View<'static> {
            $default_instance_impl$.into()
          }
        }

        impl<'msg> From<$pbr$::MessageViewInner<'msg, $Msg$>> for $Msg$View<'msg> {
          fn from(inner: $pbr$::MessageViewInner<'msg, $Msg$>) -> Self {
            Self { inner }
          }
        }

        #[allow(dead_code)]
        impl<'msg> $Msg$View<'msg> {
          $raw_msg$

          pub fn to_owned(&self) -> $Msg$ {
            $pb$::IntoProxied::into_proxied(*self, $pbi$::Private)
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

        impl<'msg> $pb$::AsView for $Msg$View<'msg> {
          type Proxied = $Msg$;
          fn as_view(&self) -> $pb$::View<'msg, $Msg$> {
            *self
          }
        }

        impl<'msg> $pb$::IntoView<'msg> for $Msg$View<'msg> {
          fn into_view<'shorter>(self) -> $Msg$View<'shorter>
          where
              'msg: 'shorter {
            self
          }
        }

        $into_proxied_impl$

        $type_conversions_impl$

        #[allow(dead_code)]
        #[allow(non_camel_case_types)]
        pub struct $Msg$Mut<'msg> {
          inner: $pbr$::MessageMutInner<'msg, $Msg$>,
        }

        impl<'msg> $pbi$::SealedInternal for $Msg$Mut<'msg> {}

        impl<'msg> $pb$::MessageMut<'msg> for $Msg$Mut<'msg> {
          type Message = $Msg$;
        }

        impl $std$::fmt::Debug for $Msg$Mut<'_> {
          fn fmt(&self, f: &mut $std$::fmt::Formatter<'_>) -> $std$::fmt::Result {
            $Msg::debug$
          }
        }

        impl<'msg> From<$pbr$::MessageMutInner<'msg, $Msg$>> for $Msg$Mut<'msg> {
          fn from(inner: $pbr$::MessageMutInner<'msg, $Msg$>) -> Self {
            Self { inner }
          }
        }

        #[allow(dead_code)]
        impl<'msg> $Msg$Mut<'msg> {
          $raw_msg$

          #[doc(hidden)]
          pub fn as_message_mut_inner(&mut self, _private: $pbi$::Private)
            -> $pbr$::MessageMutInner<'msg, $Msg$> {
            self.inner
          }

          pub fn to_owned(&self) -> $Msg$ {
            $pb$::AsView::as_view(self).to_owned()
          }

          $accessor_fns_for_muts$
        }

        //~ Note that upb Arenas are not threadsafe but we mark `$Msg$Mut` as
        //~ both Send and Sync.
        //~ We currently ensure safety by designing the API to ensure that no two
        //~ threads can hold a reference to MsgMuts with the same arena.
        // SAFETY:
        // - `$Msg$Mut` does not perform any shared mutation.
        unsafe impl Send for $Msg$Mut<'_> {}

        // SAFETY:
        // - `$Msg$Mut` does not perform any shared mutation.
        unsafe impl Sync for $Msg$Mut<'_> {}

        impl<'msg> $pb$::AsView for $Msg$Mut<'msg> {
          type Proxied = $Msg$;
          fn as_view(&self) -> $pb$::View<'_, $Msg$> {
            $Msg$View {
              inner: $pbr$::MessageViewInner::view_of_mut(self.inner)
            }
          }
        }

        impl<'msg> $pb$::IntoView<'msg> for $Msg$Mut<'msg> {
          fn into_view<'shorter>(self) -> $pb$::View<'shorter, $Msg$>
          where
              'msg: 'shorter {
            $Msg$View {
              inner: $pbr$::MessageViewInner::view_of_mut(self.inner)
            }
          }
        }

        impl<'msg> $pb$::AsMut for $Msg$Mut<'msg> {
          type MutProxied = $Msg$;
          fn as_mut(&mut self) -> $Msg$Mut<'msg> {
            $Msg$Mut { inner: self.inner }
          }
        }

        impl<'msg> $pb$::IntoMut<'msg> for $Msg$Mut<'msg> {
          fn into_mut<'shorter>(self) -> $Msg$Mut<'shorter>
          where
              'msg: 'shorter {
            self
          }
        }

        #[allow(dead_code)]
        impl $Msg$ {
          pub fn new() -> Self {
            $Msg::new$
          }

          $raw_msg$

          #[doc(hidden)]
          pub fn as_message_mut_inner(&mut self, _private: $pbi$::Private) -> $pbr$::MessageMutInner<'_, $Msg$> {
            $pbr$::MessageMutInner::mut_of_owned(&mut self.inner)
          }

          pub fn as_view(&self) -> $Msg$View<'_> {
            $pbr$::MessageViewInner::view_of_owned(&self.inner).into()
          }

          pub fn as_mut(&mut self) -> $Msg$Mut<'_> {
            $pbr$::MessageMutInner::mut_of_owned(&mut self.inner).into()
          }

          $accessor_fns$
        }  // impl $Msg$

        //~ We implement drop unconditionally, so that `$Msg$: Drop` regardless
        //~ of kernel.
        impl $std$::ops::Drop for $Msg$ {
          #[inline]
          fn drop(&mut self) {
            $Msg::drop$
          }
        }

        impl $std$::clone::Clone for $Msg$ {
          fn clone(&self) -> Self {
            self.as_view().to_owned()
          }
        }

        impl $pb$::AsView for $Msg$ {
          type Proxied = Self;
          fn as_view(&self) -> $Msg$View<'_> {
            self.as_view()
          }
        }

        impl $pb$::AsMut for $Msg$ {
          type MutProxied = Self;
          fn as_mut(&mut self) -> $Msg$Mut<'_> {
            self.as_mut()
          }
        }

        $generated_message_trait_impls$

        $nested_in_msg$
      )rs");

  if (ctx.is_cpp()) {
    ctx.Emit(
        {
            {"message_externs", [&] { CppMessageExterns(ctx, msg); }},
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
        },
        R"rs(
        unsafe extern "C" {
          $message_externs$
          $accessor_externs$
          $oneof_externs$
        }
    )rs");
  }

  ctx.printer().PrintRaw("\n");
  if (ctx.is_cpp()) {

    ctx.Emit({{"Msg", RsSafeName(msg.name())}},
             R"rs(
      impl<'a> $Msg$Mut<'a> {
        pub unsafe fn __unstable_wrap_cpp_grant_permission_to_break(
            msg: &'a mut *mut $std$::ffi::c_void) -> Self {
          let raw = $pbr$::RawMessage::new(*msg as *mut _).unwrap();
          let inner = unsafe { $pbr$::MessageMutInner::wrap_raw(raw) };
          Self { inner }
        }
        pub fn __unstable_cpp_repr_grant_permission_to_break(self) -> *mut $std$::ffi::c_void {
          self.raw_msg().as_ptr() as *mut _
        }
      }

      impl<'a> $Msg$View<'a> {
        pub fn __unstable_wrap_cpp_grant_permission_to_break(
          msg: &'a *const $std$::ffi::c_void) -> Self {
          let raw = $pbr$::RawMessage::new(*msg as *mut _).unwrap();
          let inner = unsafe { $pbr$::MessageViewInner::wrap_raw(raw) };
          inner.into()
        }
        pub fn __unstable_cpp_repr_grant_permission_to_break(self) -> *const $std$::ffi::c_void {
          self.inner.raw().as_ptr() as *const _
        }
      }

      impl $pb$::OwnedMessageInterop for $Msg$ {
        unsafe fn __unstable_take_ownership_of_raw_message(msg: *mut $std$::ffi::c_void) -> Self {
          let raw = $pbr$::RawMessage::new(msg as *mut _).unwrap();
          let inner = unsafe { $pbr$::OwnedMessageInner::<$Msg$>::wrap_raw(raw) };
          Self { inner }
        }

        fn __unstable_leak_raw_message(self) -> *mut $std$::ffi::c_void {
          let s = $std$::mem::ManuallyDrop::new(self);
          s.raw_msg().as_ptr() as *mut _
        }
      }

      impl<'a> $pb$::MessageViewInterop<'a> for $Msg$View<'a> {
        unsafe fn __unstable_wrap_raw_message(
          msg: &'a *const $std$::ffi::c_void) -> Self {
          let raw = $pbr$::RawMessage::new(*msg as *mut _).unwrap();
          let inner = unsafe { $pbr$::MessageViewInner::wrap_raw(raw) };
          inner.into()
        }
        unsafe fn __unstable_wrap_raw_message_unchecked_lifetime(
          msg: *const $std$::ffi::c_void) -> Self {
          let raw = $pbr$::RawMessage::new(msg as *mut _).unwrap();
          let inner = unsafe { $pbr$::MessageViewInner::wrap_raw(raw) };
          inner.into()
        }
        fn __unstable_as_raw_message(&self) -> *const $std$::ffi::c_void {
          self.inner.raw().as_ptr() as *const _
        }
      }
    )rs");

    if (!ctx.opts().force_lite_runtime &&
        msg.file()->options().optimize_for() != FileOptions::LITE_RUNTIME) {
      ctx.Emit({{"Msg", RsSafeName(msg.name())}},
               R"rs(
              impl $pb$::MessageDescriptorInterop for $Msg$ {
                fn __unstable_get_descriptor() -> *const $std$::ffi::c_void {
                  unsafe { $pbr$::proto2_rust_Message_get_descriptor(<$Msg$View as Default>::default().raw_msg()) }
                }
              }
            )rs");
    }
  }
}  // NOLINT(readability/fn_size)

// Generates code for a particular message in `.pb.thunk.cc`.
void GenerateThunksCc(Context& ctx, const Descriptor& msg) {
  ABSL_CHECK(ctx.is_cpp());
  if (msg.map_key() != nullptr) {
    // Don't generate code for synthetic MapEntry messages.
    return;
  }

  // Approaches to put the extern "C" in any R"cc()cc" badly confuse either
  // clang-format or VSCode highlighting. Emit this as a vanilla raw string to
  // avoid any issues.
  ctx.Emit(R"(extern "C" {
  )");

  ctx.Emit(
      {{"QualifiedMsg", cpp::QualifiedClassName(&msg)},
       {"new_thunk", ThunkName(ctx, msg, "new")},
       {"default_instance_thunk", ThunkName(ctx, msg, "default_instance")}},
      R"cc(
        void* $new_thunk$() { return new $QualifiedMsg$(); }

        const google::protobuf::MessageLite* $default_instance_thunk$() {
          return &$QualifiedMsg$::default_instance();
        }
      )cc");

  for (int i = 0; i < msg.field_count(); ++i) {
    GenerateAccessorThunkCc(ctx, *msg.field(i));
  }

  for (int i = 0; i < msg.real_oneof_decl_count(); ++i) {
    GenerateOneofThunkCc(ctx, *msg.real_oneof_decl(i));
  }

  ctx.Emit(R"(}  //extern "C"
  )");

  // Recursively generate the thunks for any nested messages.
  for (int i = 0; i < msg.nested_type_count(); ++i) {
    GenerateThunksCc(ctx, *msg.nested_type(i));
  }
}

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
