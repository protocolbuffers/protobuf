#[cfg(not(bzl))]
mod protos;
#[cfg(not(bzl))]
use protos::*;

use googletest::prelude::*;
use protobuf::prelude::*;

use protobuf::{AsView, Message, MessageView};
use unittest_rust_proto::TestAllTypes;

fn msg_view_encoded_len<'a, T: MessageView<'a>>(msg_view: T) -> usize {
    msg_view.serialize().unwrap().len()
}

fn msg_encoded_len<T: Message>(msg: T) -> usize {
    msg_view_encoded_len(msg.as_view()) + msg_view_encoded_len(T::View::default())
}

#[gtest]
fn msg_used_generically_fn_test() {
    let msg = TestAllTypes::new();
    expect_that!(msg_encoded_len(msg), eq(0));
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
    expect_that!(proto_send_msg.encode(), eq(&[]));
}
