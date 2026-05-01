// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use super::*;
use crate::__internal::entity_tag::*;
use crate::__internal::{EntityType, Enum, Private, Singular};
use crate::extension::{ExtAccess, ExtClear, ExtGetMut, ExtHas};
use crate::{ExtensionId, IntoMut, IntoView, MessageViewInterop, Proxied};
use std::sync::LazyLock;

pub struct InnerExtensionId {
    mini_table: &'static LazyLock<MiniTableExtensionInitPtr>,
}

impl InnerExtensionId {
    pub const fn new(mini_table: &'static LazyLock<MiniTableExtensionInitPtr>) -> Self {
        Self { mini_table }
    }

    pub fn mini_table(&self) -> RawMiniTableExtension {
        self.mini_table.0
    }
}

#[linkme::distributed_slice]
pub static EXTENSIONS: [LazyLock<MiniTableExtensionInitPtr>];

#[cfg(all(target_family = "unix", not(target_os = "macos")))]
core::arch::global_asm!(
    r#"
    .section linkme_EXTENSIONS,"awR",@progbits
    .globl LINKME_SENTINEL_EXTENSIONS
    LINKME_SENTINEL_EXTENSIONS:
    .size   LINKME_SENTINEL_EXTENSIONS, 0

    .section linkm2_EXTENSIONS,"awR",@progbits
    .globl LINKM2_SENTINEL_EXTENSIONS
    LINKM2_SENTINEL_EXTENSIONS:
    .size   LINKM2_SENTINEL_EXTENSIONS, 0
    "#
);

pub fn generated_extension_registry() -> RawExtensionRegistry {
    static EXTENSIONS_REGISTRY: LazyLock<ExtensionRegistryInitPtr> = LazyLock::new(|| unsafe {
        let registry = upb_ExtensionRegistry_New(THREAD_LOCAL_ARENA.with(|a| a.raw()));
        for extension in EXTENSIONS {
            upb_ExtensionRegistry_Add(registry, extension.0);
        }
        ExtensionRegistryInitPtr(registry)
    });
    EXTENSIONS_REGISTRY.0
}

impl<Msg: Message, T: Singular> ExtHas<Msg> for ExtensionId<Msg, T> {
    fn has(&self, _private: Private, msg: impl AsView<Proxied = Msg>) -> bool {
        // SAFETY:
        // - `msg` is a valid message.
        // - `mini_table` is the one associated with `self`.
        unsafe {
            upb_Message_HasExtension(
                msg.as_view().get_ptr(Private).raw(),
                self.inner.mini_table().as_ptr(),
            )
        }
    }
}

impl<Extendee: Message, V: Message> ExtAccess<Extendee, V, MessageTag>
    for ExtensionId<Extendee, V>
{
    fn get<'msg>(
        &self,
        _private: Private,
        msg: impl IntoView<'msg, Proxied = Extendee>,
    ) -> View<'msg, V> {
        let msg = msg.into_view();
        let default_instance = <V as Proxied>::View::default();
        let raw_msg = unsafe {
            upb_Message_GetExtensionMessage(
                msg.get_ptr(Private).raw(),
                self.inner.mini_table().as_ptr(),
                default_instance.get_ptr(Private).raw(),
            )
        };
        unsafe {
            <View<'msg, V>>::__unstable_wrap_raw_message_unchecked_lifetime(
                raw_msg.as_ptr() as *const std::ffi::c_void
            )
        }
    }

    fn set(
        &self,
        _private: Private,
        mut msg: impl AsMut<MutProxied = Extendee>,
        value: impl IntoProxied<V>,
    ) {
        let mut msg_mut = msg.as_mut();

        let mut child = value.into_proxied(Private);
        let child_ptr = child.get_ptr_mut(Private);

        // Ensure child arena is fused into parent message
        msg_mut.get_arena(Private).fuse(child.get_arena(Private));

        unsafe {
            assert!(upb_Message_SetExtensionMessage(
                msg_mut.get_ptr_mut(Private).raw(),
                self.inner.mini_table().as_ptr(),
                child_ptr.raw(),
                msg_mut.get_arena(Private).raw(),
            ));
        }
    }
}

impl<Extendee: Message, V: Message> ExtGetMut<Extendee, V, MessageTag>
    for ExtensionId<Extendee, V>
{
    fn get_mut<'msg>(
        &self,
        _private: Private,
        msg: impl IntoMut<'msg, MutProxied = Extendee>,
    ) -> Mut<'msg, V> {
        let mut msg = msg.into_mut();
        unsafe {
            // SAFETY: The arena associated with a `Mut<'msg, _>` proxy lives for at least `'msg`.
            // `UpbGetArena::get_arena` returns an `&Arena` tied to the local borrow of `msg`,
            // but we can safely extend it back to `'msg` here because `msg` holds an `&'msg Arena`.
            let arena_ref: &'msg Arena = std::mem::transmute(msg.get_arena(Private));

            // TODO: upb should have a GetOrCreateExtension operation instead of this dance.
            let raw_msg = match upb_Message_HasExtension(
                msg.get_ptr_mut(Private).raw(),
                self.inner.mini_table().as_ptr(),
            ) {
                true => {
                    upb_Message_GetExtensionMessage(
                        msg.get_ptr_mut(Private).raw(),
                        self.inner.mini_table().as_ptr(),
                        NonNull::dangling(), // Not used.
                    )
                }
                false => {
                    let raw_msg =
                        MessagePtr::<V>::new(arena_ref).expect("alloc should never fail").raw();
                    upb_Message_SetExtensionMessage(
                        msg.get_ptr_mut(Private).raw(),
                        self.inner.mini_table().as_ptr(),
                        raw_msg,
                        arena_ref.raw(),
                    );
                    raw_msg
                }
            };
            V::from_message_mut(raw_msg, arena_ref)
        }
    }
}

impl<Extendee: Message, V: Singular> ExtAccess<Extendee, Repeated<V>, RepeatedTag>
    for ExtensionId<Extendee, Repeated<V>>
where
    V: EntityType + UpbTypeConversions<V::Tag>,
{
    fn get<'msg>(
        &self,
        _private: Private,
        msg: impl IntoView<'msg, Proxied = Extendee>,
    ) -> View<'msg, Repeated<V>> {
        let msg = msg.into_view();
        let raw_array = unsafe {
            upb_Message_GetExtensionArray(
                msg.get_ptr(Private).raw(),
                self.inner.mini_table().as_ptr(),
            )
        };
        unsafe {
            IntoView::into_view(
                raw_array.map_or_else(empty_array::<V>, |raw| RepeatedView::from_raw(Private, raw)),
            )
        }
    }

    fn set(
        &self,
        _private: Private,
        mut msg: impl AsMut<MutProxied = Extendee>,
        value: impl IntoProxied<Repeated<V>>,
    ) {
        let value = value.into_proxied(Private);
        let mut ext_mut = self.get_mut(msg.as_mut());
        ext_mut.clear();
        ext_mut.copy_from(unsafe { RepeatedView::from_raw(Private, value.inner(Private).raw()) });
    }
}

impl<Extendee: Message, V> ExtGetMut<Extendee, Repeated<V>, RepeatedTag>
    for ExtensionId<Extendee, Repeated<V>>
where
    V: Singular + EntityType + UpbTypeConversions<V::Tag>,
{
    fn get_mut<'msg>(
        &self,
        _private: Private,
        msg: impl IntoMut<'msg, MutProxied = Extendee>,
    ) -> Mut<'msg, Repeated<V>> {
        let mut msg = msg.into_mut();

        let arena_ref: &'msg Arena = unsafe { std::mem::transmute(msg.get_arena(Private)) };
        let raw_array = unsafe {
            upb_Message_GetExtensionMutableArray(
                msg.get_ptr_mut(Private).raw(),
                self.inner.mini_table().as_ptr(),
            )
        };
        let raw_array = raw_array.unwrap_or_else(|| unsafe {
            let new_arr = upb_Array_New(arena_ref.raw(), V::upb_type());
            let mut ptr = new_arr.as_ptr();
            upb_Message_SetExtension(
                msg.get_ptr_mut(Private).raw(),
                self.inner.mini_table().as_ptr(),
                &mut ptr as *mut _ as *const core::ffi::c_void,
                arena_ref.raw(),
            );
            new_arr
        });
        unsafe {
            RepeatedMut::from_inner(Private, InnerRepeatedMut::new(raw_array, arena_ref)).into_mut()
        }
    }
}

macro_rules! impl_upb_scalar_extension {
    ($($t:ty => [$get:ident, $set:ident]),* $(,)?) => {
        $(
            impl<Extendee: Message> ExtAccess<Extendee, $t, PrimitiveTag> for ExtensionId<Extendee, $t> {
                fn get<'msg>(&self, _private: Private, msg: impl IntoView<'msg, Proxied = Extendee>) -> View<'msg, $t> {
                    let msg = msg.into_view();
                    unsafe {
                        $get(
                            msg.get_ptr(Private).raw(),
                            self.inner.mini_table().as_ptr(),
                            self.default.expect("scalar extensions must have a default value"),
                        )
                    }
                }

                fn set(&self, _private: Private, mut msg: impl AsMut<MutProxied = Extendee>, value: impl IntoProxied<$t>) {
                    let mut msg_mut = msg.as_mut();
                    unsafe {
                        assert!($set(
                            msg_mut.get_ptr_mut(Private).raw(),
                            self.inner.mini_table().as_ptr(),
                            value.into_proxied(Private),
                            msg_mut.get_arena(Private).raw(),
                        ));
                    }
                }
            }
        )*
    };
}

impl_upb_scalar_extension!(
    bool => [upb_Message_GetExtensionBool, upb_Message_SetExtensionBool],
    f32 => [upb_Message_GetExtensionFloat, upb_Message_SetExtensionFloat],
    f64 => [upb_Message_GetExtensionDouble, upb_Message_SetExtensionDouble],
    i32 => [upb_Message_GetExtensionInt32, upb_Message_SetExtensionInt32],
    i64 => [upb_Message_GetExtensionInt64, upb_Message_SetExtensionInt64],
    u32 => [upb_Message_GetExtensionUInt32, upb_Message_SetExtensionUInt32],
    u64 => [upb_Message_GetExtensionUInt64, upb_Message_SetExtensionUInt64],
);

impl<Extendee: Message> ExtAccess<Extendee, ProtoString, PrimitiveTag>
    for ExtensionId<Extendee, ProtoString>
{
    fn get<'msg>(
        &self,
        _private: Private,
        msg: impl IntoView<'msg, Proxied = Extendee>,
    ) -> View<'msg, ProtoString> {
        let msg = msg.into_view();
        let upb_str_view = unsafe {
            upb_Message_GetExtensionString(
                msg.get_ptr(Private).raw(),
                self.inner.mini_table().as_ptr(),
                self.default.expect("string extensions must have a default value").into(),
            )
        };
        unsafe { ProtoStr::from_utf8_unchecked(upb_str_view.as_ref()) }
    }

    fn set(
        &self,
        _private: Private,
        mut msg: impl AsMut<MutProxied = Extendee>,
        value: impl IntoProxied<ProtoString>,
    ) {
        let s = value.into_proxied(Private);
        let (view, arena) = s.into_inner(Private).into_raw_parts();
        let mut msg_mut = msg.as_mut();
        msg_mut.get_arena(Private).fuse(&arena);
        unsafe {
            assert!(upb_Message_SetExtensionString(
                msg_mut.get_ptr_mut(Private).raw(),
                self.inner.mini_table().as_ptr(),
                view,
                msg_mut.get_arena(Private).raw(),
            ));
        }
    }
}

impl<Extendee: Message> ExtAccess<Extendee, ProtoBytes, PrimitiveTag>
    for ExtensionId<Extendee, ProtoBytes>
{
    fn get<'msg>(
        &self,
        _private: Private,
        msg: impl IntoView<'msg, Proxied = Extendee>,
    ) -> View<'msg, ProtoBytes> {
        let msg = msg.into_view();
        let upb_str_view = unsafe {
            upb_Message_GetExtensionString(
                msg.get_ptr(Private).raw(),
                self.inner.mini_table().as_ptr(),
                self.default.expect("bytes extensions must have a default value").into(),
            )
        };
        unsafe { upb_str_view.as_ref() }
    }

    fn set(
        &self,
        _private: Private,
        mut msg: impl AsMut<MutProxied = Extendee>,
        value: impl IntoProxied<ProtoBytes>,
    ) {
        let s = value.into_proxied(Private);
        let (view, arena) = s.into_inner(Private).into_raw_parts();
        let mut msg_mut = msg.as_mut();
        msg_mut.get_arena(Private).fuse(&arena);
        unsafe {
            assert!(upb_Message_SetExtensionString(
                msg_mut.get_ptr_mut(Private).raw(),
                self.inner.mini_table().as_ptr(),
                view,
                msg_mut.get_arena(Private).raw(),
            ));
        }
    }
}

impl<Msg: Message, T: Proxied> ExtClear<Msg> for ExtensionId<Msg, T> {
    fn clear(&self, _private: Private, mut msg: impl AsMut<MutProxied = Msg>) {
        // SAFETY:
        // - `msg` is a valid message.
        // - `mini_table` is the one associated with `self`.
        unsafe {
            upb_Message_ClearExtension(
                msg.as_mut().get_ptr_mut(Private).raw(),
                self.inner.mini_table().as_ptr(),
            )
        }
    }
}

impl<Extendee: Message, E> ExtAccess<Extendee, E, EnumTag> for ExtensionId<Extendee, E>
where
    E: Enum + Proxied + EntityType<Tag = EnumTag> + Copy,
    for<'a> E: Proxied<View<'a> = E>,
    for<'a> Extendee::MessageView<'a>: UpbGetMessagePtr,
    i32: From<E>,
{
    fn get<'msg>(
        &self,
        _private: Private,
        msg: impl IntoView<'msg, Proxied = Extendee>,
    ) -> View<'msg, E> {
        let msg = msg.into_view();
        let default = self.default.expect("enum extensions must have a default value");
        let val = unsafe {
            upb_Message_GetExtensionInt32(
                msg.get_ptr(Private).raw(),
                self.inner.mini_table().as_ptr(),
                i32::from(default),
            )
        };
        E::try_from(val).unwrap_or(default)
    }

    fn set(
        &self,
        _private: Private,
        mut msg: impl AsMut<MutProxied = Extendee>,
        value: impl IntoProxied<E>,
    ) {
        let mut msg_mut = msg.as_mut();
        unsafe {
            assert!(upb_Message_SetExtensionInt32(
                msg_mut.get_ptr_mut(Private).raw(),
                self.inner.mini_table().as_ptr(),
                value.into_proxied(Private).into(),
                msg_mut.get_arena(Private).raw(),
            ));
        }
    }
}
