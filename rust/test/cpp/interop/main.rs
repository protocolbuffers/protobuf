// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use googletest::prelude::*;
use protobuf_cpp::prelude::*;

use protobuf_cpp::__internal::runtime::PtrAndLen;
use protobuf_cpp::{
    MessageDescriptorInterop, MessageMutInterop, MessageViewInterop, OwnedMessageInterop,
};
use std::ffi::c_void;

use interop_test_rust_proto::{InteropTestMessage, InteropTestMessageMut, InteropTestMessageView};

macro_rules! proto_assert_eq {
    ($lhs:expr, $rhs:expr) => {{
        let lhs = &$lhs;
        let rhs = &$rhs;

        assert_that!(lhs.i64(), eq(rhs.i64()));
        assert_that!(lhs.bytes(), eq(rhs.bytes()));
        assert_that!(lhs.b(), eq(rhs.b()));
    }};
}

// Helper functions invoking C++ Protobuf APIs directly in C++.
// Defined in `test_utils.cc`.
unsafe extern "C" {
    fn TakeOwnershipAndGetOptionalInt64(msg: *mut c_void) -> i64;
    fn DeserializeInteropTestMessage(data: *const u8, len: usize) -> *mut c_void;
    fn MutateInteropTestMessage(msg: *mut c_void);
    fn SerializeInteropTestMessage(
        msg: *const c_void,
    ) -> protobuf_cpp::__internal::runtime::SerializedData;
    fn DeleteInteropTestMessage(msg: *mut c_void);

    fn NewWithExtension() -> *mut c_void;
    fn GetBytesExtension(msg: *const c_void) -> PtrAndLen;

    fn GetConstStaticInteropTestMessage() -> *const c_void;

    fn IsExpectedDescriptor(provided: *const c_void) -> bool;

    fn NewArenaInteropTestMessage(out_arena: *mut *mut c_void) -> *mut c_void;
    fn VerifyAndDeleteArena(arena: *mut c_void, msg: *mut c_void);
}

#[gtest]
fn send_to_cpp() {
    let mut msg1 = InteropTestMessage::new();
    msg1.set_i64(7);
    let i = unsafe { TakeOwnershipAndGetOptionalInt64(msg1.__unstable_leak_raw_message()) };
    assert_eq!(i, 7);
}

#[gtest]
fn mutate_message_mut_in_cpp() {
    let mut msg1 = InteropTestMessage::new();
    unsafe {
        MutateInteropTestMessage(msg1.as_mut().__unstable_as_raw_message_mut());
    }

    let mut msg2 = InteropTestMessage::new();
    msg2.set_i64(42);
    msg2.set_bytes(b"something mysterious");
    msg2.set_b(false);

    proto_assert_eq!(msg1, msg2);
}

#[gtest]
fn deserialize_in_rust() {
    let mut msg1 = InteropTestMessage::new();
    msg1.set_i64(-1);
    msg1.set_bytes(b"some cool data I guess");
    let serialized =
        unsafe { SerializeInteropTestMessage(msg1.as_view().__unstable_as_raw_message()) };

    let msg2 = InteropTestMessage::parse(&serialized).unwrap();
    proto_assert_eq!(msg1, msg2);
}

#[gtest]
fn deserialize_in_cpp() {
    let mut msg1 = InteropTestMessage::new();
    msg1.set_i64(-1);
    msg1.set_bytes(b"some cool data I guess");
    let data = msg1.serialize().unwrap();

    let msg2 = unsafe {
        InteropTestMessage::__unstable_take_ownership_of_raw_message(DeserializeInteropTestMessage(
            (*data).as_ptr(),
            data.len(),
        ))
    };

    proto_assert_eq!(msg1, msg2);
}

#[gtest]
fn deserialize_in_cpp_into_mut() {
    let mut msg1 = InteropTestMessage::new();
    msg1.set_i64(-1);
    msg1.set_bytes(b"some cool data I guess");
    let data = msg1.serialize().unwrap();

    let mut raw_msg = unsafe { DeserializeInteropTestMessage((*data).as_ptr(), data.len()) };
    let msg2 = unsafe { InteropTestMessageMut::__unstable_wrap_raw_message_mut(&mut raw_msg) };

    proto_assert_eq!(msg1, msg2);

    // The C++ still owns the message here and needs to delete it.
    unsafe {
        DeleteInteropTestMessage(raw_msg);
    }
}

#[gtest]
fn deserialize_in_cpp_into_view() {
    let mut msg1 = InteropTestMessage::new();
    msg1.set_i64(-1);
    msg1.set_bytes(b"some cool data I guess");
    let data = msg1.serialize().unwrap();

    let raw_msg = unsafe { DeserializeInteropTestMessage((*data).as_ptr(), data.len()) };
    let const_msg = raw_msg as *const _;
    let msg2 = unsafe { InteropTestMessageView::__unstable_wrap_raw_message(&const_msg) };

    proto_assert_eq!(msg1, msg2);

    // The C++ still owns the message here and needs to delete it.
    unsafe {
        DeleteInteropTestMessage(raw_msg);
    }
}

// This test ensures that random fields we (Rust) don't know about don't
// accidentally get destroyed by Rust.
#[gtest]
fn smuggle_extension() {
    let msg1 =
        unsafe { InteropTestMessage::__unstable_take_ownership_of_raw_message(NewWithExtension()) };
    let data = msg1.serialize().unwrap();

    let mut msg2 = InteropTestMessage::parse(&data).unwrap();
    let bytes =
        unsafe { GetBytesExtension(msg2.as_mut().__unstable_as_raw_message_mut()).as_ref() };
    assert_eq!(bytes, b"smuggled");
}

#[gtest]
fn view_of_const_static() {
    let view: InteropTestMessageView<'static> = unsafe {
        InteropTestMessageView::__unstable_wrap_raw_message_unchecked_lifetime(
            GetConstStaticInteropTestMessage(),
        )
    };
    assert_eq!(view.i64(), 0);
    assert_eq!(view.default_int32(), 41);
}

#[gtest]
fn descriptor_interop() {
    let descriptor = InteropTestMessage::__unstable_get_descriptor();
    let result = unsafe { IsExpectedDescriptor(descriptor) };
    assert!(result);
}

#[gtest]
fn arena_backed_map_insert() {
    let mut arena = std::ptr::null_mut();
    let mut raw_msg = unsafe { NewArenaInteropTestMessage(&mut arena) };
    let mut msg = unsafe { InteropTestMessageMut::__unstable_wrap_raw_message_mut(&mut raw_msg) };

    let mut sub = InteropTestMessage::new();
    sub.set_bytes(b"non_sso_submessage_bytes_exceeding_32_bytes");
    msg.map_string_msg_mut().insert("non_sso_map_key_exceeding_32_bytes_1", sub);
    let mut m = msg.map_string_bytes_mut();
    m.insert("non_sso_map_key_exceeding_32_bytes_2", b"initial_non_sso_value_exceeding_32_bytes");
    m.insert("non_sso_map_key_exceeding_32_bytes_2", b"non_sso_bytes_value_exceeding_32_bytes");
    m.insert("non_sso_map_key_exceeding_32_bytes_3", b"transient_non_sso_bytes_exceeding_32_bytes");
    assert!(m.remove("non_sso_map_key_exceeding_32_bytes_3"));

    unsafe { VerifyAndDeleteArena(arena, raw_msg) };
}
