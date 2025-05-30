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
        let arena = $pbr$::Arena::new();
        let raw = unsafe {
            $pbr$::upb_Message_New(
                <Self as $pbr$::AssociatedMiniTable>::mini_table(),
                arena.raw()).unwrap()
        };
        let inner = unsafe { $pbr$::OwnedMessageInner::<Self>::wrap_raw(raw, arena) };
        Self { inner }
      )rs");
      return;
  }

  ABSL_LOG(FATAL) << "unreachable";
}

void MessageSerialize(Context& ctx, const Descriptor& msg) {
  switch (ctx.opts().kernel) {
    case Kernel::kCpp:
      ctx.Emit({}, R"rs(
        let mut serialized_data = $pbr$::SerializedData::new();
        let success = unsafe {
          $pbr$::proto2_rust_Message_serialize(self.raw_msg(), &mut serialized_data)
        };
        if success {
          Ok(serialized_data.into_vec())
        } else {
          Err($pb$::SerializeError)
        }
      )rs");
      return;

    case Kernel::kUpb:
      ctx.Emit(R"rs(
        // SAFETY: `MINI_TABLE` is the one associated with `self.raw_msg()`.
        let encoded = unsafe {
          $pbr$::wire::encode(self.raw_msg(),
              <Self as $pbr$::AssociatedMiniTable>::mini_table())
        };
        //~ TODO: This discards the info we have about the reason
        //~ of the failure, we should try to keep it instead.
        encoded.map_err(|_| $pb$::SerializeError)
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
        let string = unsafe {
          $pbr$::debug_string(
            self.raw_msg(),
            <Self as $pbr$::AssociatedMiniTable>::mini_table()
          )
        };
        write!(f, "{}", string)
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
            unsafe { $pbr$::proto2_rust_Message_copy_from(dst.inner.msg(), self.inner.msg()) };
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
            let dst_raw = $pbr$::UpbGetRawMessageMut::get_raw_message_mut(&mut dst, $pbi$::Private);
            let dst_arena = $pbr$::UpbGetArena::get_arena(&mut dst, $pbi$::Private);
            let src_raw = $pbr$::UpbGetRawMessage::get_raw_message(&self, $pbi$::Private);

            unsafe { $pbr$::upb_Message_DeepCopy(
              dst_raw,
              src_raw,
              <Self as $pbr$::AssociatedMiniTable>::mini_table(),
              dst_arena.raw(),
            ) };
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
      let submessages = [
        $submessages$
      ];
      let subenums = [
        $subenums$
      ];
      assert!($pbr$::upb_MiniTable_Link(
          $minitable_symbol_name$.0,
          submessages.as_ptr() as *const *const $pbr$::upb_MiniTable,
          submessages.len(), subenums.as_ptr(), subenums.len()));
  )rs");
}

void CppGeneratedMessageTraitImpls(Context& ctx, const Descriptor& msg) {
  ABSL_CHECK(ctx.is_cpp());
  ctx.Emit(R"rs(
    unsafe impl $pbr$::CppGetRawMessageMut for $Msg$Mut<'_> {
      fn get_raw_message_mut(&mut self, _private: $pbi$::Private) -> $pbr$::RawMessage {
        self.inner.msg()
      }
    }

    unsafe impl $pbr$::CppGetRawMessage for $Msg$View<'_> {
      fn get_raw_message(&self, _private: $pbi$::Private) -> $pbr$::RawMessage {
        self.inner.msg()
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
                           $pbr$::upb_MiniTable_Build(
                               "$mini_descriptor$".as_ptr(),
                               $mini_descriptor_length$,
                               $pbr$::THREAD_LOCAL_ARENA.with(|a| a.raw()),
                               $std$::ptr::null_mut());
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
        fn mini_table() -> *const $pbr$::upb_MiniTable {
          static ONCE_LOCK: $std$::sync::OnceLock<$pbr$::MiniTablePtr> =
              $std$::sync::OnceLock::new();
          ONCE_LOCK.get_or_init(|| unsafe {
            $mini_table_impl$
            $pbr$::MiniTablePtr($minitable_symbol_name$.0)
          }).0
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

      unsafe impl $pbr$::AssociatedMiniTable for $Msg$View<'_> {
        #[inline(always)]
        fn mini_table() -> *const $pbr$::upb_MiniTable {
          <$Msg$ as $pbr$::AssociatedMiniTable>::mini_table()
        }
      }

      unsafe impl $pbr$::AssociatedMiniTable for $Msg$Mut<'_> {
        #[inline(always)]
        fn mini_table() -> *const $pbr$::upb_MiniTable {
          <$Msg$ as $pbr$::AssociatedMiniTable>::mini_table()
        }
      }

      unsafe impl $pbr$::UpbGetRawMessageMut for $Msg$Mut<'_> {
        fn get_raw_message_mut(&mut self, _private: $pbi$::Private) -> $pbr$::RawMessage {
          self.inner.msg()
        }
      }
      unsafe impl $pbr$::UpbGetArena for $Msg$Mut<'_> {
        fn get_arena(&mut self, _private: $pbi$::Private) -> &$pbr$::Arena {
          self.inner.arena()
        }
      }

      unsafe impl $pbr$::UpbGetRawMessage for $Msg$View<'_> {
        fn get_raw_message(&self, _private: $pbi$::Private) -> $pbr$::RawMessage {
          self.inner.msg()
        }
      }
    )rs");
}

void MessageMutTakeCopyMergeFrom(Context& ctx, const Descriptor& msg) {
  switch (ctx.opts().kernel) {
    case Kernel::kCpp:
      ctx.Emit(R"rs(
          impl $pb$::TakeFrom for $Msg$Mut<'_> {
            fn take_from(&mut self, mut src: impl $pb$::AsMut<MutProxied = $Msg$>) {
              //~ TODO: b/393559271 - Optimize this copy out.
              let mut src = src.as_mut();
              $pb$::CopyFrom::copy_from(self, $pb$::AsView::as_view(&src));
              $pb$::Clear::clear(&mut src);
            }
          }

          impl $pb$::CopyFrom for $Msg$Mut<'_> {
            fn copy_from(&mut self, src: impl $pb$::AsView<Proxied = $Msg$>) {
              unsafe { $pbr$::proto2_rust_Message_copy_from(self.raw_msg(), src.as_view().raw_msg()) };
            }
          }

          impl $pb$::MergeFrom for $Msg$Mut<'_> {
            fn merge_from(&mut self, src: impl $pb$::AsView<Proxied = $Msg$>) {
              // SAFETY: self and src are both valid `$Msg$`s.
              unsafe {
                $pbr$::proto2_rust_Message_merge_from(self.raw_msg(), src.as_view().raw_msg());
              }
            }
          }
        )rs");
      return;
    case Kernel::kUpb:
      ctx.Emit(
          R"rs(
          impl $pb$::TakeFrom for $Msg$Mut<'_> {
            fn take_from(&mut self, mut src: impl $pb$::AsMut<MutProxied = $Msg$>) {
              let mut src = src.as_mut();
              //~ TODO: b/393559271 - Optimize this copy out.
              $pb$::CopyFrom::copy_from(self, $pb$::AsView::as_view(&src));
              $pb$::Clear::clear(&mut src);
            }
          }

          impl $pb$::CopyFrom for $Msg$Mut<'_> {
            fn copy_from(&mut self, src: impl $pb$::AsView<Proxied = $Msg$>) {
              // SAFETY: self and src are both valid `$Msg$`s associated with
              // `Self::mini_table()`.
              unsafe {
                assert!(
                  $pbr$::upb_Message_DeepCopy(
                    self.raw_msg(),
                    src.as_view().raw_msg(),
                    <Self as $pbr$::AssociatedMiniTable>::mini_table(),
                    self.arena().raw())
                );
              }
            }
          }

          impl $pb$::MergeFrom for $Msg$Mut<'_> {
            fn merge_from(&mut self, src: impl $pb$::AsView<Proxied = $Msg$>) {
              // SAFETY: self and src are both valid `$Msg$`s.
              unsafe {
                assert!(
                  $pbr$::upb_Message_MergeFrom(self.raw_msg(),
                    src.as_view().raw_msg(),
                    <Self as $pbr$::AssociatedMiniTable>::mini_table(),
                    // Use a nullptr for the ExtensionRegistry.
                    $std$::ptr::null(),
                    self.arena().raw())
                );
              }
            }
          }
        )rs");
      return;
  }
}

void MessageProxiedInRepeated(Context& ctx, const Descriptor& msg) {
  switch (ctx.opts().kernel) {
    case Kernel::kCpp:
      ctx.Emit(
          {
              {"Msg", RsSafeName(msg.name())},
          },
          R"rs(
        unsafe impl $pb$::ProxiedInRepeated for $Msg$ {
          fn repeated_new(_private: $pbi$::Private) -> $pb$::Repeated<Self> {
            // SAFETY:
            // - The thunk returns an unaliased and valid `RepeatedPtrField*`
            unsafe {
              $pb$::Repeated::from_inner($pbi$::Private,
                $pbr$::InnerRepeated::from_raw($pbr$::proto2_rust_RepeatedField_Message_new())
              )
            }
          }

          unsafe fn repeated_free(_private: $pbi$::Private, f: &mut $pb$::Repeated<Self>) {
            // SAFETY
            // - `f.raw()` is a valid `RepeatedPtrField*`.
            unsafe { $pbr$::proto2_rust_RepeatedField_Message_free(f.as_view().as_raw($pbi$::Private)) }
          }

          fn repeated_len(f: $pb$::View<$pb$::Repeated<Self>>) -> usize {
            // SAFETY: `f.as_raw()` is a valid `RepeatedPtrField*`.
            unsafe { $pbr$::proto2_rust_RepeatedField_Message_size(f.as_raw($pbi$::Private)) }
          }

          unsafe fn repeated_set_unchecked(
            mut f: $pb$::Mut<$pb$::Repeated<Self>>,
            i: usize,
            v: impl $pb$::IntoProxied<Self>,
          ) {
            // SAFETY:
            // - `f.as_raw()` is a valid `RepeatedPtrField*`.
            // - `i < len(f)` is promised by caller.
            // - `v.raw_msg()` is a valid `const Message&`.
            unsafe {
              $pbr$::proto2_rust_Message_copy_from(
                $pbr$::proto2_rust_RepeatedField_Message_get_mut(f.as_raw($pbi$::Private), i),
                v.into_proxied($pbi$::Private).raw_msg(),
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
            let msg = unsafe { $pbr$::proto2_rust_RepeatedField_Message_get(f.as_raw($pbi$::Private), i) };
            let inner = unsafe { $pbr$::MessageViewInner::wrap_raw(msg) };
            $pb$::View::<Self>::new($pbi$::Private, inner)
          }

          unsafe fn repeated_get_mut_unchecked(
            mut f: $pb$::Mut<$pb$::Repeated<Self>>,
            i: usize,
          ) -> $pb$::Mut<Self> {
            // SAFETY:
            // - `f.as_raw()` is a valid `RepeatedPtrField*`.
            // - `i < len(f)` is promised by caller.
            let msg = unsafe { $pbr$::proto2_rust_RepeatedField_Message_get_mut(f.as_raw($pbi$::Private), i) };
            let inner = unsafe { $pbr$::MessageMutInner::wrap_raw(msg) };
            $pb$::Mut::<Self>::new($pbi$::Private, inner)
          }

          fn repeated_clear(mut f: $pb$::Mut<$pb$::Repeated<Self>>) {
            // SAFETY:
            // - `f.as_raw()` is a valid `RepeatedPtrField*`.
            unsafe { $pbr$::proto2_rust_RepeatedField_Message_clear(f.as_raw($pbi$::Private)) };
          }

          fn repeated_push(mut f: $pb$::Mut<$pb$::Repeated<Self>>, v: impl $pb$::IntoProxied<Self>) {
            // SAFETY:
            // - `f.as_raw()` is a valid `RepeatedPtrField*`.
            // - `v.raw_msg()` is a valid `const Message&`.
            unsafe {
              let prototype = <$Msg$View as $std$::default::Default>::default().raw_msg();
              let new_elem = $pbr$::proto2_rust_RepeatedField_Message_add(f.as_raw($pbi$::Private), prototype);
              $pbr$::proto2_rust_Message_copy_from(new_elem, v.into_proxied($pbi$::Private).raw_msg());
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
              $pbr$::proto2_rust_RepeatedField_Message_copy_from(dest.as_raw($pbi$::Private), src.as_raw($pbi$::Private));
            }
          }

          fn repeated_reserve(
            mut f: $pb$::Mut<$pb$::Repeated<Self>>,
            additional: usize,
          ) {
            // SAFETY:
            // - `f.as_raw()` is a valid `RepeatedPtrField*`.
            unsafe { $pbr$::proto2_rust_RepeatedField_Message_reserve(f.as_raw($pbi$::Private), additional) }
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
        unsafe impl $pb$::ProxiedInRepeated for $Msg$ {
          fn repeated_new(_private: $pbi$::Private) -> $pb$::Repeated<Self> {
            let arena = $pbr$::Arena::new();
            unsafe {
              $pb$::Repeated::from_inner(
                  $pbi$::Private,
                  $pbr$::InnerRepeated::from_raw_parts(
                      $pbr$::upb_Array_New(arena.raw(), $pbr$::CType::Message),
                      arena,
                  ))
            }
          }

          unsafe fn repeated_free(_private: $pbi$::Private, _f: &mut $pb$::Repeated<Self>) {
            // No-op: the memory will be dropped by the arena.
          }

          fn repeated_len(f: $pb$::View<$pb$::Repeated<Self>>) -> usize {
            // SAFETY: `f.as_raw()` is a valid `upb_Array*`.
            unsafe { $pbr$::upb_Array_Size(f.as_raw($pbi$::Private)) }
          }
          unsafe fn repeated_set_unchecked(
            mut f: $pb$::Mut<$pb$::Repeated<Self>>,
            i: usize,
            v: impl $pb$::IntoProxied<Self>,
          ) {
            unsafe {
                $pbr$::upb_Array_Set(
                    f.as_raw($pbi$::Private),
                    i,
                    <Self as $pbr$::UpbTypeConversions>::into_message_value_fuse_if_required(
                        f.raw_arena($pbi$::Private),
                        v.into_proxied($pbi$::Private),
                    ),
                )
            }
          }

          unsafe fn repeated_get_unchecked(
            f: $pb$::View<$pb$::Repeated<Self>>,
            i: usize,
          ) -> $pb$::View<Self> {
            // SAFETY:
            // - `f.as_raw()` is a valid `const upb_Array*`.
            // - `i < len(f)` is promised by the caller.
            let val = unsafe { $pbr$::upb_Array_Get(f.as_raw($pbi$::Private), i) };
            let raw_msg = unsafe { val.msg_val }.expect("upb_Array* element should not be NULL.");
            let inner = unsafe { $pbr$::MessageViewInner::wrap_raw(raw_msg) };
            $pb$::View::<Self>::new($pbi$::Private, inner)
          }

          unsafe fn repeated_get_mut_unchecked(
            mut f: $pb$::Mut<$pb$::Repeated<Self>>,
            i: usize,
          ) -> $pb$::Mut<Self> {
            // SAFETY:
            // - `f.as_raw()` is a valid `upb_Array*`.
            // - `f` is a an array of message-valued elements.
            // - `i < len(f)` is promised by the caller.
            let msg_ptr = unsafe { $pbr$::upb_Array_GetMutable(f.as_raw($pbi$::Private), i) };
            unsafe {$pb$::Mut::<Self> { inner: $pbr$::MessageMutInner::wrap_raw(msg_ptr, f.arena($pbi$::Private)) } }
          }

          fn repeated_clear(mut f: $pb$::Mut<$pb$::Repeated<Self>>) {
            // SAFETY:
            // - `f.as_raw()` is a valid `upb_Array*`.
            unsafe {
              $pbr$::upb_Array_Resize(f.as_raw($pbi$::Private), 0, f.raw_arena($pbi$::Private))
            };
          }
          fn repeated_push(mut f: $pb$::Mut<$pb$::Repeated<Self>>, v: impl $pb$::IntoProxied<Self>) {
            // SAFETY:
            // - `f.as_raw()` is a valid `upb_Array*`.
            // - `msg_ptr` is a valid `upb_Message*`.
            unsafe {
              $pbr$::upb_Array_Append(
                f.as_raw($pbi$::Private),
                <Self as $pbr$::UpbTypeConversions>::into_message_value_fuse_if_required(f.raw_arena($pbi$::Private), v.into_proxied($pbi$::Private)),
                f.raw_arena($pbi$::Private)
              );
            };
          }

          fn repeated_copy_from(
            src: $pb$::View<$pb$::Repeated<Self>>,
            dest: $pb$::Mut<$pb$::Repeated<Self>>,
          ) {
              // SAFETY:
              // - Elements of `src` and `dest` have message minitable `MINI_TABLE`.
              unsafe {
                $pbr$::repeated_message_copy_from(src, dest, <Self as $pbr$::AssociatedMiniTable>::mini_table());
              }
          }

          fn repeated_reserve(
            mut f: $pb$::Mut<$pb$::Repeated<Self>>,
            additional: usize,
          ) {
            // SAFETY:
            // - `f.as_raw()` is a valid `upb_Array*`.
            unsafe {
              let size = $pbr$::upb_Array_Size(f.as_raw($pbi$::Private));
              $pbr$::upb_Array_Reserve(f.as_raw($pbi$::Private), size + additional, f.raw_arena($pbi$::Private));
            }
          }
        }
      )rs");
      return;
  }
  ABSL_LOG(FATAL) << "unreachable";
}

void TypeConversions(Context& ctx, const Descriptor& msg) {
  switch (ctx.opts().kernel) {
    case Kernel::kCpp:
      ctx.Emit(
          R"rs(
          impl $pbr$::CppMapTypeConversions for $Msg$ {
              fn get_prototype() -> $pbr$::MapValue {
                  $pbr$::MapValue::make_message(<$Msg$View as $std$::default::Default>::default().raw_msg())
              }

              fn to_map_value(self) -> $pbr$::MapValue {
                  $pbr$::MapValue::make_message(std::mem::ManuallyDrop::new(self).raw_msg())
              }

              unsafe fn from_map_value<'b>(value: $pbr$::MapValue) -> $Msg$View<'b> {
                  debug_assert_eq!(value.tag, $pbr$::MapValueTag::Message);
                  unsafe { $Msg$View::new($pbi$::Private, $pbr$::MessageViewInner::wrap_raw(value.val.m)) }
              }

              unsafe fn mut_from_map_value<'b>(value: $pbr$::MapValue) -> $Msg$Mut<'b> {
                  debug_assert_eq!(value.tag, $pbr$::MapValueTag::Message);
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
            impl $pbr$::UpbTypeConversions for $Msg$ {
                fn upb_type() -> $pbr$::CType {
                    $pbr$::CType::Message
                }

                fn to_message_value(
                    val: $pb$::View<'_, Self>) -> $pbr$::upb_MessageValue {
                    $pbr$::upb_MessageValue { msg_val: Some(val.raw_msg()) }
                }

                unsafe fn into_message_value_fuse_if_required(
                  raw_parent_arena: $pbr$::RawArena,
                  mut val: Self) -> $pbr$::upb_MessageValue {
                  // SAFETY: The arena memory is not freed due to `ManuallyDrop`.
                  let parent_arena = $std$::mem::ManuallyDrop::new(
                      unsafe { $pbr$::Arena::from_raw(raw_parent_arena) });

                  parent_arena.fuse(val.as_message_mut_inner($pbi$::Private).arena());
                  $pbr$::upb_MessageValue { msg_val: Some(val.raw_msg()) }
                }

                unsafe fn from_message_value<'msg>(msg: $pbr$::upb_MessageValue)
                    -> $pb$::View<'msg, Self> {
                    let raw = unsafe { msg.msg_val }.expect("expected present message value in map");
                    let inner = unsafe { $pbr$::MessageViewInner::wrap_raw(raw) };
                    $Msg$View::new($pbi$::Private, inner)
                }

                unsafe fn from_message_mut<'msg>(msg: $pbr$::RawMessage, arena: &'msg $pbr$::Arena)
                    -> $Msg$Mut<'msg> {
                    let inner = unsafe { $pbr$::MessageMutInner::<'msg, $Msg$>::wrap_raw(msg, arena) };
                    $Msg$Mut::new($pbi$::Private, inner)
                }
            }
            )rs");
  }
}

void GenerateDefaultInstanceImpl(Context& ctx, const Descriptor& msg) {
  if (ctx.is_upb()) {
    ctx.Emit("$pbr$::ScratchSpace::zeroed_block()");
  } else {
    ctx.Emit(
        {{"default_instance_thunk", ThunkName(ctx, msg, "default_instance")}},
        "$default_instance_thunk$()");
  }
}

}  // namespace

void GenerateRs(Context& ctx, const Descriptor& msg, const upb::DefPool& pool) {
  if (ctx.is_upb()) {
    ctx.Emit({{"minitable_symbol_name", UpbMiniTableName(msg)}},
             R"rs(
        // This variable must not be referenced except by protobuf generated
        // code.
        pub(crate) static mut $minitable_symbol_name$: $pbr$::MiniTablePtr =
            $pbr$::MiniTablePtr($std$::ptr::null_mut());
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
          {"Msg::serialize", [&] { MessageSerialize(ctx, msg); }},
          {"Msg::drop", [&] { MessageDrop(ctx, msg); }},
          {"Msg::debug", [&] { MessageDebug(ctx, msg); }},
          {"MsgMut::take_copy_merge_from",
           [&] { MessageMutTakeCopyMergeFrom(ctx, msg); }},
          {"default_instance_impl",
           [&] { GenerateDefaultInstanceImpl(ctx, msg); }},
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
          {"raw_arena_getter_for_message",
           [&] {
             if (ctx.is_upb()) {
               ctx.Emit({}, R"rs(
                  fn arena(&mut self) -> &$pbr$::Arena {
                    self.inner.arena()
                  }
                  )rs");
             }
           }},
          {"raw_arena_getter_for_msgmut",
           [&] {
             if (ctx.is_upb()) {
               ctx.Emit({}, R"rs(
                  fn arena(&mut self) -> &$pbr$::Arena {
                    self.inner.arena()
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
          {"generated_message_trait_impls",
           [&] {
             if (ctx.is_upb()) {
               UpbGeneratedMessageTraitImpls(ctx, msg, pool);
             } else {
               CppGeneratedMessageTraitImpls(ctx, msg);
             }
           }},
          {"repeated_impl", [&] { MessageProxiedInRepeated(ctx, msg); }},
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

        impl $pb$::Message for $Msg$ {}

        impl $std$::default::Default for $Msg$ {
          fn default() -> Self {
            Self::new()
          }
        }

        impl $pb$::Parse for $Msg$ {
          fn parse(serialized: &[u8]) -> $Result$<Self, $pb$::ParseError> {
            Self::parse(serialized)
          }

          fn parse_dont_enforce_required(serialized: &[u8]) -> $Result$<Self, $pb$::ParseError> {
            Self::parse_dont_enforce_required(serialized)
          }
        }

        impl $std$::fmt::Debug for $Msg$ {
          fn fmt(&self, f: &mut $std$::fmt::Formatter<'_>) -> $std$::fmt::Result {
            $Msg::debug$
          }
        }

        impl $pb$::TakeFrom for $Msg$ {
          fn take_from(&mut self, src: impl $pb$::AsMut<MutProxied = Self>) {
            let mut m = self.as_mut();
            $pb$::TakeFrom::take_from(&mut m, src)
          }
        }

        impl $pb$::CopyFrom for $Msg$ {
          fn copy_from(&mut self, src: impl $pb$::AsView<Proxied = Self>) {
            let mut m = self.as_mut();
            $pb$::CopyFrom::copy_from(&mut m, src)
          }
        }

        impl $pb$::MergeFrom for $Msg$ {
          fn merge_from<'src>(&mut self, src: impl $pb$::AsView<Proxied = Self>) {
            let mut m = self.as_mut();
            $pb$::MergeFrom::merge_from(&mut m, src)
          }
        }

        impl $pb$::Serialize for $Msg$ {
          fn serialize(&self) -> $Result$<Vec<u8>, $pb$::SerializeError> {
            $pb$::AsView::as_view(self).serialize()
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
          _phantom: $Phantom$<&'msg ()>,
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

        impl $pb$::Serialize for $Msg$View<'_> {
          fn serialize(&self) -> $Result$<Vec<u8>, $pb$::SerializeError> {
            $Msg::serialize$
          }
        }

        impl $std$::default::Default for $Msg$View<'_> {
          fn default() -> $Msg$View<'static> {
            let inner = unsafe { $pbr$::MessageViewInner::wrap_raw($default_instance_impl$) };
            $Msg$View::new($pbi$::Private, inner)
          }
        }

        #[allow(dead_code)]
        impl<'msg> $Msg$View<'msg> {
          #[doc(hidden)]
          pub fn new(_private: $pbi$::Private, inner: $pbr$::MessageViewInner<'msg, $Msg$>) -> Self {
            Self { inner, _phantom: $std$::marker::PhantomData }
          }

          fn raw_msg(&self) -> $pbr$::RawMessage {
            self.inner.msg()
          }

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

        impl<'msg> $pb$::Proxy<'msg> for $Msg$View<'msg> {}
        impl<'msg> $pb$::ViewProxy<'msg> for $Msg$View<'msg> {}

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

        $repeated_impl$
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

        impl $pb$::Serialize for $Msg$Mut<'_> {
          fn serialize(&self) -> $Result$<Vec<u8>, $pb$::SerializeError> {
            $pb$::AsView::as_view(self).serialize()
          }
        }

        $MsgMut::take_copy_merge_from$

        #[allow(dead_code)]
        impl<'msg> $Msg$Mut<'msg> {
          #[doc(hidden)]
          pub fn from_parent<ParentT: $pb$::Message>(
                     _private: $pbi$::Private,
                     parent: $pbr$::MessageMutInner<'msg, ParentT>,
                     msg: $pbr$::RawMessage)
            -> Self {
            Self {
              inner: $pbr$::MessageMutInner::from_parent(parent, msg)
            }
          }

          #[doc(hidden)]
          pub fn new(_private: $pbi$::Private, inner: $pbr$::MessageMutInner<'msg, $Msg$>) -> Self {
            Self { inner }
          }

          fn raw_msg(&self) -> $pbr$::RawMessage {
            self.inner.msg()
          }

          #[doc(hidden)]
          pub fn as_message_mut_inner(&mut self, _private: $pbi$::Private)
            -> $pbr$::MessageMutInner<'msg, $Msg$> {
            self.inner
          }

          pub fn to_owned(&self) -> $Msg$ {
            $pb$::AsView::as_view(self).to_owned()
          }

          $raw_arena_getter_for_msgmut$

          $accessor_fns_for_muts$
        }

        // SAFETY:
        // - `$Msg$Mut` does not perform any shared mutation.
        // - `$Msg$Mut` is not `Send`, and so even in the presence of mutator
        //   splitting, synchronous access of an arena is impossible.
        unsafe impl Sync for $Msg$Mut<'_> {}

        impl<'msg> $pb$::Proxy<'msg> for $Msg$Mut<'msg> {}
        impl<'msg> $pb$::MutProxy<'msg> for $Msg$Mut<'msg> {}

        impl<'msg> $pb$::AsView for $Msg$Mut<'msg> {
          type Proxied = $Msg$;
          fn as_view(&self) -> $pb$::View<'_, $Msg$> {
            $Msg$View {
              inner: $pbr$::MessageViewInner::view_of_mut(self.inner.clone()),
              _phantom: $std$::marker::PhantomData
            }
          }
        }

        impl<'msg> $pb$::IntoView<'msg> for $Msg$Mut<'msg> {
          fn into_view<'shorter>(self) -> $pb$::View<'shorter, $Msg$>
          where
              'msg: 'shorter {
            $Msg$View {
              inner: $pbr$::MessageViewInner::view_of_mut(self.inner.clone()),
              _phantom: $std$::marker::PhantomData
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

          fn raw_msg(&self) -> $pbr$::RawMessage {
            self.inner.msg()
          }

          #[doc(hidden)]
          pub fn as_message_mut_inner(&mut self, _private: $pbi$::Private) -> $pbr$::MessageMutInner<'_, $Msg$> {
            $pbr$::MessageMutInner::mut_of_owned(&mut self.inner)
          }

          $raw_arena_getter_for_message$

          pub fn parse(data: &[u8]) -> $Result$<Self, $pb$::ParseError> {
            let mut msg = Self::new();
            $pb$::ClearAndParse::clear_and_parse(&mut msg, data).map(|_| msg)
          }

          pub fn parse_dont_enforce_required(data: &[u8]) -> $Result$<Self, $pb$::ParseError> {
            let mut msg = Self::new();
            $pb$::ClearAndParse::clear_and_parse_dont_enforce_required(&mut msg, data).map(|_| msg)
          }

          pub fn as_view(&self) -> $Msg$View {
            $Msg$View::new(
                $pbi$::Private,
                $pbr$::MessageViewInner::view_of_owned(&self.inner))
          }

          pub fn as_mut(&mut self) -> $Msg$Mut {
            let inner = $pbr$::MessageMutInner::mut_of_owned(&mut self.inner);
            $Msg$Mut::new($pbi$::Private, inner)
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

        impl $std$::clone::Clone for $Msg$ {
          fn clone(&self) -> Self {
            self.as_view().to_owned()
          }
        }

        impl $pb$::AsView for $Msg$ {
          type Proxied = Self;
          fn as_view(&self) -> $Msg$View {
            self.as_view()
          }
        }

        impl $pb$::AsMut for $Msg$ {
          type MutProxied = Self;
          fn as_mut(&mut self) -> $Msg$Mut {
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
        extern "C" {
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
          Self::new($pbi$::Private, inner)
        }
        pub fn __unstable_cpp_repr_grant_permission_to_break(self) -> *const $std$::ffi::c_void {
          self.inner.msg().as_ptr() as *const _
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

      impl<'a> $pb$::MessageMutInterop<'a> for $Msg$Mut<'a> {
        unsafe fn __unstable_wrap_raw_message_mut(
            msg: &'a mut *mut $std$::ffi::c_void) -> Self {
          let raw = $pbr$::RawMessage::new(*msg as *mut _).unwrap();
          let inner = unsafe { $pbr$::MessageMutInner::wrap_raw(raw) };
          Self { inner }
        }
        unsafe fn __unstable_wrap_raw_message_mut_unchecked_lifetime(
            msg: *mut $std$::ffi::c_void) -> Self {
          let raw = $pbr$::RawMessage::new(msg as *mut _).unwrap();
          let inner = unsafe { $pbr$::MessageMutInner::wrap_raw(raw) };
          Self { inner }
        }
        fn __unstable_as_raw_message_mut(&mut self) -> *mut $std$::ffi::c_void {
          self.raw_msg().as_ptr() as *mut _
        }
      }

      impl<'a> $pb$::MessageViewInterop<'a> for $Msg$View<'a> {
        unsafe fn __unstable_wrap_raw_message(
          msg: &'a *const $std$::ffi::c_void) -> Self {
          let raw = $pbr$::RawMessage::new(*msg as *mut _).unwrap();
          let inner = unsafe { $pbr$::MessageViewInner::wrap_raw(raw) };
          Self::new($pbi$::Private, inner)
        }
        unsafe fn __unstable_wrap_raw_message_unchecked_lifetime(
          msg: *const $std$::ffi::c_void) -> Self {
          let raw = $pbr$::RawMessage::new(msg as *mut _).unwrap();
          let inner = unsafe { $pbr$::MessageViewInner::wrap_raw(raw) };
          Self::new($pbi$::Private, inner)
        }
        fn __unstable_as_raw_message(&self) -> *const $std$::ffi::c_void {
          self.inner.msg().as_ptr() as *const _
        }
      }
    )rs");
  } else {
    ctx.Emit({{"Msg", RsSafeName(msg.name())}}, R"rs(
      // upb kernel doesn't support any owned message or message mut interop.
      impl $pb$::OwnedMessageInterop for $Msg$ {}
      impl<'a> $pb$::MessageMutInterop<'a> for $Msg$Mut<'a> {}

      impl<'a> $pb$::MessageViewInterop<'a> for $Msg$View<'a> {
        unsafe fn __unstable_wrap_raw_message(
          msg: &'a *const $std$::ffi::c_void) -> Self {
          let raw = $pbr$::RawMessage::new(*msg as *mut _).unwrap();
          let inner = unsafe { $pbr$::MessageViewInner::wrap_raw(raw) };
          Self::new($pbi$::Private, inner)
        }
        unsafe fn __unstable_wrap_raw_message_unchecked_lifetime(
          msg: *const $std$::ffi::c_void) -> Self {
          let raw = $pbr$::RawMessage::new(msg as *mut _).unwrap();
          let inner = unsafe { $pbr$::MessageViewInner::wrap_raw(raw) };
          Self::new($pbi$::Private, inner)
        }
        fn __unstable_as_raw_message(&self) -> *const $std$::ffi::c_void {
          self.inner.msg().as_ptr() as *const _
        }
      }
    )rs");
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
