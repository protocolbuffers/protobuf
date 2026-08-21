#[cfg(not(bzl))]
mod protos;
#[cfg(not(bzl))]
use protos::*;

use googletest::prelude::*;
use protobuf::prelude::*;

use protobuf::{AsMut, AsView, IntoMut, IntoView, Message};
use unittest_rust_proto::TestAllTypes;

fn f<M: Message, T: AsView<Proxied = M>>(msg_view: T) -> usize {
    msg_view.as_view().serialize().unwrap().len()
}

fn msg_encoded_len<T: Message>(msg: T) -> usize {
    f(msg.as_view())
        + f(T::View::default())
        + f(T::View::default().into_view())
        + f(T::View::default().as_view())
        + f(msg)
}

#[gtest]
fn msg_used_generically_for_views_test() {
    let msg = TestAllTypes::new();
    expect_that!(msg_encoded_len(msg), eq(0));
}

fn as_mut_clear<M: Message, T: AsMut<MutProxied = M>>(mut msg_mut: T) {
    msg_mut.as_mut().clear();
}

fn msg_clear<T: Message>(mut msg: T) {
    msg.clear();
    as_mut_clear(msg);
}

#[gtest]
fn msg_used_generically_for_muts_test() {
    let mut msg = TestAllTypes::new();
    msg.set_optional_int32(123);
    expect_that!(msg.has_optional_int32(), eq(true));
    as_mut_clear(msg.as_mut());
    expect_that!(msg.has_optional_int32(), eq(false));

    msg_clear(msg);
}

pub struct ProtoGenericallyStruct<'a, V: Message>(V::View<'a>);

impl<'a, V: Message> ProtoGenericallyStruct<'a, V> {
    pub fn from_view(provider: &'a impl AsView<Proxied = V>) -> Self {
        Self(provider.as_view())
    }

    fn encode(&self) -> Vec<u8> {
        self.0.serialize().unwrap()
    }

    fn _ptr_for(&self, id: std::any::TypeId) -> Option<*const ()> {
        if id != std::any::TypeId::of::<V::View<'static>>() {
            return None;
        }
        Some(&self.0 as *const _ as *const ())
    }
}

#[gtest]
fn msg_used_generically_struct_test() {
    let msg = TestAllTypes::new();
    let proto_send_msg = ProtoGenericallyStruct::from_view(&msg);
    let empty: &[u8] = &[];
    expect_that!(proto_send_msg.encode(), eq(empty));
}

#[gtest]
fn as_view_serialize_test() {
    fn as_view_serialize<M: Message, T: AsView<Proxied = M>>(msg_view: T) -> usize {
        msg_view.as_view().serialize().unwrap().len()
    }
    let mut msg = TestAllTypes::new();
    expect_that!(as_view_serialize(msg.as_view()), eq(0));
    expect_that!(as_view_serialize(&msg), eq(0));
    expect_that!(as_view_serialize(&mut msg), eq(0));
    expect_that!(as_view_serialize(msg), eq(0));
}

#[gtest]
fn into_view_serialize_test() {
    fn into_view_serialize<'a, M: Message, T: IntoView<'a, Proxied = M>>(msg_view: T) -> usize {
        msg_view.into_view().serialize().unwrap().len()
    }
    let mut msg = TestAllTypes::new();
    expect_that!(into_view_serialize(msg.as_view()), eq(0));
    expect_that!(into_view_serialize(&msg), eq(0));
    expect_that!(into_view_serialize(&mut msg), eq(0));
}

#[gtest]
fn as_mut_clear_test() {
    fn as_mut_clear<M: Message, T: AsMut<MutProxied = M>>(mut msg_mut: T) {
        msg_mut.as_mut().clear();
    }

    {
        let mut msg = TestAllTypes::new();
        msg.set_optional_int32(123);
        expect_that!(msg.has_optional_int32(), eq(true));
        as_mut_clear(msg.as_mut());
        expect_that!(msg.has_optional_int32(), eq(false));
    }
    {
        let mut msg = TestAllTypes::new();
        msg.set_optional_int32(123);
        expect_that!(msg.has_optional_int32(), eq(true));
        as_mut_clear(&mut msg);
        expect_that!(msg.has_optional_int32(), eq(false));
    }
    {
        let mut msg = TestAllTypes::new();
        msg.set_optional_int32(123);
        expect_that!(msg.has_optional_int32(), eq(true));
        as_mut_clear(msg); // msg itself is AsMut but this consumes the msg.
    }
}

#[gtest]
fn into_mut_clear_test() {
    fn into_mut_clear<'a, M: Message, T: IntoMut<'a, MutProxied = M>>(msg_mut: T) {
        msg_mut.into_mut().clear();
    }

    {
        let mut msg = TestAllTypes::new();
        msg.set_optional_int32(123);
        expect_that!(msg.has_optional_int32(), eq(true));
        into_mut_clear(msg.as_mut());
        expect_that!(msg.has_optional_int32(), eq(false));
    }
    {
        let mut msg = TestAllTypes::new();
        msg.set_optional_int32(123);
        expect_that!(msg.has_optional_int32(), eq(true));
        into_mut_clear(&mut msg);
        expect_that!(msg.has_optional_int32(), eq(false));
    }
}
