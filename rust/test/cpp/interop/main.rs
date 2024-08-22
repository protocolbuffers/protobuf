// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use googletest::prelude::*;
use protobuf_cpp::prelude::*;

use protobuf_cpp::__runtime::PtrAndLen;
use protobuf_cpp::{MessageMutInterop, MessageViewInterop, OwnedMessageInterop};
use std::ffi::c_void;
use unittest_rust_proto::{TestAllExtensions, TestAllTypes, TestAllTypesMut, TestAllTypesView};

macro_rules! proto_assert_eq {
    ($lhs:expr, $rhs:expr) => {{
        let lhs = &$lhs;
        let rhs = &$rhs;

        assert_that!(lhs.optional_int64(), eq(rhs.optional_int64()));
        assert_that!(lhs.optional_bytes(), eq(rhs.optional_bytes()));
        assert_that!(lhs.optional_bool(), eq(rhs.optional_bool()));
    }};
}

// Helper functions invoking C++ Protobuf APIs directly in C++.
// Defined in `test_utils.cc`.
extern "C" {
    fn TakeOwnershipAndGetOptionalInt32(msg: *mut c_void) -> i32;
    fn DeserializeTestAllTypes(data: *const u8, len: usize) -> *mut c_void;
    fn MutateTestAllTypes(msg: *mut c_void);
    fn SerializeTestAllTypes(msg: *const c_void) -> protobuf_cpp::__runtime::SerializedData;
    fn DeleteTestAllTypes(msg: *mut c_void);

    fn NewWithExtension() -> *mut c_void;
    fn GetBytesExtension(msg: *const c_void) -> PtrAndLen;

    fn GetConstStaticTestAllTypes() -> *const c_void;
}

#[gtest]
fn send_to_cpp() {
    let mut msg1 = TestAllTypes::new();
    msg1.set_optional_int32(7);
    let i = unsafe { TakeOwnershipAndGetOptionalInt32(msg1.__unstable_leak_raw_message()) };
    assert_eq!(i, 7);
}

#[gtest]
fn mutate_message_mut_in_cpp() {
    let mut msg1 = TestAllTypes::new();
    unsafe {
        MutateTestAllTypes(msg1.as_mut().__unstable_as_raw_message_mut());
    }

    let mut msg2 = TestAllTypes::new();
    msg2.set_optional_int64(42);
    msg2.set_optional_bytes(b"something mysterious");
    msg2.set_optional_bool(false);

    proto_assert_eq!(msg1, msg2);
}

#[gtest]
fn deserialize_in_rust() {
    let mut msg1 = TestAllTypes::new();
    msg1.set_optional_int64(-1);
    msg1.set_optional_bytes(b"some cool data I guess");
    let serialized = unsafe { SerializeTestAllTypes(msg1.as_view().__unstable_as_raw_message()) };

    let msg2 = TestAllTypes::parse(&serialized).unwrap();
    proto_assert_eq!(msg1, msg2);
}

#[gtest]
fn deserialize_in_cpp() {
    let mut msg1 = TestAllTypes::new();
    msg1.set_optional_int64(-1);
    msg1.set_optional_bytes(b"some cool data I guess");
    let data = msg1.serialize().unwrap();

    let msg2 = unsafe {
        TestAllTypes::__unstable_take_ownership_of_raw_message(DeserializeTestAllTypes(
            (*data).as_ptr(),
            data.len(),
        ))
    };

    proto_assert_eq!(msg1, msg2);
}

#[gtest]
fn deserialize_in_cpp_into_mut() {
    let mut msg1 = TestAllTypes::new();
    msg1.set_optional_int64(-1);
    msg1.set_optional_bytes(b"some cool data I guess");
    let data = msg1.serialize().unwrap();

    let mut raw_msg = unsafe { DeserializeTestAllTypes((*data).as_ptr(), data.len()) };
    let msg2 = unsafe { TestAllTypesMut::__unstable_wrap_raw_message_mut(&mut raw_msg) };

    proto_assert_eq!(msg1, msg2);

    // The C++ still owns the message here and needs to delete it.
    unsafe {
        DeleteTestAllTypes(raw_msg);
    }
}

#[gtest]
fn deserialize_in_cpp_into_view() {
    let mut msg1 = TestAllTypes::new();
    msg1.set_optional_int64(-1);
    msg1.set_optional_bytes(b"some cool data I guess");
    let data = msg1.serialize().unwrap();

    let raw_msg = unsafe { DeserializeTestAllTypes((*data).as_ptr(), data.len()) };
    let const_msg = raw_msg as *const _;
    let msg2 = unsafe { TestAllTypesView::__unstable_wrap_raw_message(&const_msg) };

    proto_assert_eq!(msg1, msg2);

    // The C++ still owns the message here and needs to delete it.
    unsafe {
        DeleteTestAllTypes(raw_msg);
    }
}

// This test ensures that random fields we (Rust) don't know about don't
// accidentally get destroyed by Rust.
#[gtest]
fn smuggle_extension() {
    let msg1 =
        unsafe { TestAllExtensions::__unstable_take_ownership_of_raw_message(NewWithExtension()) };
    let data = msg1.serialize().unwrap();

    let mut msg2 = TestAllExtensions::parse(&data).unwrap();
    let bytes =
        unsafe { GetBytesExtension(msg2.as_mut().__unstable_as_raw_message_mut()).as_ref() };
    assert_eq!(bytes, b"smuggled");
}

#[gtest]
fn view_of_const_static() {
    let view: TestAllTypesView<'static> = unsafe {
        TestAllTypesView::__unstable_wrap_raw_message_unchecked_lifetime(
            GetConstStaticTestAllTypes(),
        )
    };
    assert_eq!(view.optional_int64(), 0);
    assert_eq!(view.default_int32(), 41);
}
