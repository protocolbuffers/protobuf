// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use super::*;
use crate::__internal::entity_tag::*;
use crate::__internal::runtime::_opaque_pointees::RawRepeatedFieldData;
use crate::__internal::{EntityType, Enum, MatcherEq, Private, Singular};
use crate::extension::{ExtAccess, ExtClear, ExtGetMut, ExtHas};
use crate::{ExtensionId, IntoMut, IntoView, MessageViewInterop, Proxied};

unsafe extern "C" {
    pub fn proto2_rust_Message_clear_extension(m: RawMessage, number: i32);
    pub fn proto2_rust_Message_has_extension(m: RawMessage, number: i32) -> bool;
    pub fn proto2_rust_Message_set_extension_bool(
        m: RawMessage,
        number: i32,
        field_type: i32,
        value: bool,
    );
    pub fn proto2_rust_Message_get_extension_bool(
        m: RawMessage,
        number: i32,
        default_value: bool,
    ) -> bool;
    pub fn proto2_rust_Message_set_extension_float(
        m: RawMessage,
        number: i32,
        field_type: i32,
        value: f32,
    );
    pub fn proto2_rust_Message_get_extension_float(
        m: RawMessage,
        number: i32,
        default_value: f32,
    ) -> f32;
    pub fn proto2_rust_Message_set_extension_double(
        m: RawMessage,
        number: i32,
        field_type: i32,
        value: f64,
    );
    pub fn proto2_rust_Message_get_extension_double(
        m: RawMessage,
        number: i32,
        default_value: f64,
    ) -> f64;
    pub fn proto2_rust_Message_set_extension_int32(
        m: RawMessage,
        number: i32,
        field_type: i32,
        value: i32,
    );
    pub fn proto2_rust_Message_get_extension_int32(
        m: RawMessage,
        number: i32,
        default_value: i32,
    ) -> i32;
    pub fn proto2_rust_Message_set_extension_int64(
        m: RawMessage,
        number: i32,
        field_type: i32,
        value: i64,
    );
    pub fn proto2_rust_Message_get_extension_int64(
        m: RawMessage,
        number: i32,
        default_value: i64,
    ) -> i64;
    pub fn proto2_rust_Message_set_extension_uint32(
        m: RawMessage,
        number: i32,
        field_type: i32,
        value: u32,
    );
    pub fn proto2_rust_Message_get_extension_uint32(
        m: RawMessage,
        number: i32,
        default_value: u32,
    ) -> u32;
    pub fn proto2_rust_Message_set_extension_uint64(
        m: RawMessage,
        number: i32,
        field_type: i32,
        value: u64,
    );
    pub fn proto2_rust_Message_get_extension_uint64(
        m: RawMessage,
        number: i32,
        default_value: u64,
    ) -> u64;
    pub fn proto2_rust_Message_set_extension_string(
        m: RawMessage,
        number: i32,
        field_type: i32,
        value: CppStdString,
    );
    pub fn proto2_rust_Message_get_extension_string(
        m: RawMessage,
        number: i32,
        default_value: PtrAndLen,
    ) -> PtrAndLen;
    pub fn proto2_rust_Message_get_extension_message(
        m: RawMessage,
        number: i32,
        default_instance: RawMessage,
    ) -> RawMessage;
    pub fn proto2_rust_Message_mutable_extension_message(
        m: RawMessage,
        number: i32,
        field_type: i32,
        default_instance: RawMessage,
    ) -> RawMessage;

}

fn pin_extension<Extendee: Message, T: Proxied>(extension: &ExtensionId<Extendee, T>) {
    unsafe {
        std::ptr::read_volatile(extension.inner.cpp_extension_id_thunk() as *const u8);
        // We could try to optimize this to the following at some point, but it's less portable.
        // std::arch::asm!("/* {0} */", in(reg) ($ptr as *const ()));
    }
}

#[doc(hidden)]
pub struct InnerExtensionId {
    field_type: i32,
    cpp_extension_id_thunk: unsafe extern "C" fn() -> *const c_void,
    cpp_get_extension_thunk: Option<unsafe extern "C" fn(*const c_void) -> *const c_void>,
    cpp_mutable_extension_thunk: Option<unsafe extern "C" fn(*mut c_void) -> *mut c_void>,
}

impl InnerExtensionId {
    pub const fn new(
        field_type: i32,
        cpp_extension_id_thunk: unsafe extern "C" fn() -> *const c_void,
    ) -> Self {
        Self {
            field_type,
            cpp_extension_id_thunk,
            cpp_get_extension_thunk: None,
            cpp_mutable_extension_thunk: None,
        }
    }

    pub const fn new_repeated(
        field_type: i32,
        cpp_extension_id_thunk: unsafe extern "C" fn() -> *const c_void,
        cpp_get_extension_thunk: unsafe extern "C" fn(*const c_void) -> *const c_void,
        cpp_mutable_extension_thunk: unsafe extern "C" fn(*mut c_void) -> *mut c_void,
    ) -> Self {
        Self {
            field_type,
            cpp_extension_id_thunk,
            cpp_get_extension_thunk: Some(cpp_get_extension_thunk),
            cpp_mutable_extension_thunk: Some(cpp_mutable_extension_thunk),
        }
    }

    pub fn cpp_extension_id_thunk(&self) -> unsafe extern "C" fn() -> *const c_void {
        self.cpp_extension_id_thunk
    }

    pub fn cpp_get_extension_thunk(
        &self,
    ) -> Option<unsafe extern "C" fn(*const c_void) -> *const c_void> {
        self.cpp_get_extension_thunk
    }

    pub fn cpp_mutable_extension_thunk(
        &self,
    ) -> Option<unsafe extern "C" fn(*mut c_void) -> *mut c_void> {
        self.cpp_mutable_extension_thunk
    }
}

impl<Msg: Message, T: Singular> ExtHas<Msg> for ExtensionId<Msg, T> {
    fn has(&self, _private: Private, msg: impl AsView<Proxied = Msg>) -> bool {
        pin_extension(self);
        unsafe {
            proto2_rust_Message_has_extension(
                msg.as_view().get_raw_message(Private),
                self.number() as i32,
            )
        }
    }
}

macro_rules! impl_scalar_extension {
    ($($t:ty => [$get:ident, $set:ident]),* $(,)?) => {
        $(
            impl<Extendee: Message> ExtAccess<Extendee, $t, PrimitiveTag>
            for ExtensionId<Extendee, $t>
            {
                fn get<'msg>(&self, _private: Private, msg: impl IntoView<'msg, Proxied = Extendee>) -> View<'msg, $t> {
                    let msg = msg.into_view();
                    pin_extension(self);
                    unsafe {
                        $get(
                            msg.get_raw_message(Private),
                            self.number() as i32,
                            self.default.expect("scalar extensions must have a default value"),
                        )
                    }
                }

                fn set(&self, _private: Private, mut msg: impl AsMut<MutProxied = Extendee>, value: impl IntoProxied<$t>) {
                    pin_extension(self);
                    unsafe {
                        $set(
                            msg.as_mut().get_raw_message_mut(Private),
                            self.number() as i32,
                            self.inner.field_type,
                            value.into_proxied(Private),
                        );
                    }
                }
            }
        )*
    };
}

impl_scalar_extension!(
    bool => [proto2_rust_Message_get_extension_bool, proto2_rust_Message_set_extension_bool],
    f32 => [proto2_rust_Message_get_extension_float, proto2_rust_Message_set_extension_float],
    f64 => [proto2_rust_Message_get_extension_double, proto2_rust_Message_set_extension_double],
    i32 => [proto2_rust_Message_get_extension_int32, proto2_rust_Message_set_extension_int32],
    i64 => [proto2_rust_Message_get_extension_int64, proto2_rust_Message_set_extension_int64],
    u32 => [proto2_rust_Message_get_extension_uint32, proto2_rust_Message_set_extension_uint32],
    u64 => [proto2_rust_Message_get_extension_uint64, proto2_rust_Message_set_extension_uint64],
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
        pin_extension(self);
        let ptr_len = unsafe {
            proto2_rust_Message_get_extension_string(
                msg.get_raw_message(Private),
                self.number() as i32,
                self.default.expect("string extensions must have a default value").into(),
            )
        };
        unsafe { ProtoStr::from_utf8_unchecked(ptr_len.as_ref()) }
    }

    fn set(
        &self,
        _private: Private,
        mut msg: impl AsMut<MutProxied = Extendee>,
        value: impl IntoProxied<ProtoString>,
    ) {
        pin_extension(self);
        unsafe {
            proto2_rust_Message_set_extension_string(
                msg.as_mut().get_raw_message_mut(Private),
                self.number() as i32,
                self.inner.field_type,
                <ProtoString as CppTypeConversions>::into_insertelem(value.into_proxied(Private)),
            );
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
        pin_extension(self);
        let ptr_len = unsafe {
            proto2_rust_Message_get_extension_string(
                msg.get_raw_message(Private),
                self.number() as i32,
                self.default.expect("bytes extensions must have a default value").into(),
            )
        };
        unsafe { ptr_len.as_ref() }
    }

    fn set(
        &self,
        _private: Private,
        mut msg: impl AsMut<MutProxied = Extendee>,
        value: impl IntoProxied<ProtoBytes>,
    ) {
        pin_extension(self);
        unsafe {
            proto2_rust_Message_set_extension_string(
                msg.as_mut().get_raw_message_mut(Private),
                self.number() as i32,
                self.inner.field_type,
                <ProtoBytes as CppTypeConversions>::into_insertelem(value.into_proxied(Private)),
            );
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
        pin_extension(self);
        let default_instance = <V as Proxied>::View::default();
        let raw_msg = unsafe {
            proto2_rust_Message_get_extension_message(
                msg.get_raw_message(Private),
                self.number() as i32,
                default_instance.get_raw_message(Private),
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
        let value = value.into_proxied(Private);
        let mut ext_mut = self.get_mut(msg.as_mut());
        ext_mut.take_from(value);
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
        pin_extension(self);
        let default_instance = <V as Proxied>::View::default();
        let raw_msg = unsafe {
            proto2_rust_Message_mutable_extension_message(
                msg.get_raw_message_mut(Private),
                self.number() as i32,
                self.inner.field_type,
                default_instance.get_raw_message(Private),
            )
        };
        unsafe {
            <Mut<'msg, V>>::__unstable_wrap_raw_message_mut_unchecked_lifetime(
                raw_msg.as_ptr() as *mut std::ffi::c_void
            )
        }
    }
}

impl<Msg: Message, T: Proxied> ExtClear<Msg> for ExtensionId<Msg, T> {
    fn clear(&self, _private: Private, mut msg: impl AsMut<MutProxied = Msg>) {
        pin_extension(self);
        unsafe {
            proto2_rust_Message_clear_extension(
                msg.as_mut().get_raw_message_mut(Private),
                self.number() as i32,
            )
        }
    }
}

impl<Msg: Message, V: Singular> ExtAccess<Msg, Repeated<V>, RepeatedTag>
    for ExtensionId<Msg, Repeated<V>>
{
    fn get<'msg>(
        &self,
        _private: Private,
        msg: impl IntoView<'msg, Proxied = Msg>,
    ) -> View<'msg, Repeated<V>> {
        let msg = msg.into_view();
        let raw = unsafe {
            let thunk = self.inner.cpp_get_extension_thunk().unwrap();
            thunk(msg.get_raw_message(Private).as_ptr() as *const std::ffi::c_void)
                as *mut RawRepeatedFieldData
        };
        unsafe {
            IntoView::into_view(RepeatedView::from_raw(
                Private,
                RawRepeatedField::new(raw).unwrap(),
            ))
        }
    }

    fn set(
        &self,
        _private: Private,
        mut msg: impl AsMut<MutProxied = Msg>,
        value: impl IntoProxied<Repeated<V>>,
    ) {
        let value = value.into_proxied(Private);
        let mut ext_mut = self.get_mut(msg.as_mut());
        ext_mut.clear();
        ext_mut.copy_from(unsafe { RepeatedView::from_raw(Private, value.inner(Private).raw()) });
    }
}

impl<Msg: Message, V: Singular> ExtGetMut<Msg, Repeated<V>, RepeatedTag>
    for ExtensionId<Msg, Repeated<V>>
{
    fn get_mut<'msg>(
        &self,
        _private: Private,
        msg: impl IntoMut<'msg, MutProxied = Msg>,
    ) -> Mut<'msg, Repeated<V>> {
        let mut msg = msg.into_mut();
        let raw = unsafe {
            let thunk = self.inner.cpp_mutable_extension_thunk().unwrap();
            thunk(msg.get_raw_message_mut(Private).as_ptr() as *mut std::ffi::c_void)
                as *mut RawRepeatedFieldData
        };
        unsafe {
            RepeatedMut::from_inner(
                Private,
                InnerRepeatedMut::new(RawRepeatedField::new(raw).unwrap()),
            )
            .into_mut()
        }
    }
}

impl<Extendee: Message, E> ExtAccess<Extendee, E, EnumTag> for ExtensionId<Extendee, E>
where
    E: Enum + Proxied + EntityType<Tag = EnumTag> + Copy,
    for<'a> E: Proxied<View<'a> = E>,
    for<'a> Extendee::MessageView<'a>: CppGetRawMessage,
    i32: From<E>,
{
    fn get<'msg>(
        &self,
        _private: Private,
        msg: impl IntoView<'msg, Proxied = Extendee>,
    ) -> View<'msg, E> {
        let msg = msg.into_view();
        pin_extension(self);
        let default = self.default.expect("enum extensions must have a default value");
        let val = unsafe {
            proto2_rust_Message_get_extension_int32(
                msg.get_raw_message(Private),
                self.number() as i32,
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
        pin_extension(self);
        unsafe {
            proto2_rust_Message_set_extension_int32(
                msg.as_mut().get_raw_message_mut(Private),
                self.number() as i32,
                self.inner.field_type,
                value.into_proxied(Private).into(),
            );
        }
    }
}
