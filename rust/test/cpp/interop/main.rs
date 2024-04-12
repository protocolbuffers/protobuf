// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use googletest::prelude::*;
use protobuf_cpp::__runtime::{PtrAndLen, RawMessage};
use unittest_proto::{TestAllExtensions, TestAllTypes, TestAllTypesMut, TestAllTypesView};

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
    fn TakeOwnershipAndGetOptionalInt32(msg: RawMessage) -> i32;
    fn DeserializeTestAllTypes(data: *const u8, len: usize) -> RawMessage;
    fn MutateTestAllTypes(msg: RawMessage);
    fn SerializeTestAllTypes(msg: RawMessage) -> protobuf_cpp::__runtime::SerializedData;
    fn DeleteTestAllTypes(msg: RawMessage);

    fn NewWithExtension() -> RawMessage;
    fn GetBytesExtension(msg: RawMessage) -> PtrAndLen;
}

#[test]
fn send_to_cpp() {
    let mut msg1 = TestAllTypes::new();
    msg1.set_optional_int32(7);
    let i = unsafe {
        TakeOwnershipAndGetOptionalInt32(msg1.__unstable_leak_cpp_repr_grant_permission_to_break())
    };
    assert_eq!(i, 7);
}

#[test]
fn mutate_message_mut_in_cpp() {
    let mut msg1 = TestAllTypes::new();
    unsafe {
        MutateTestAllTypes(msg1.as_mut().__unstable_cpp_repr_grant_permission_to_break());
    }

    let mut msg2 = TestAllTypes::new();
    msg2.set_optional_int64(42);
    msg2.set_optional_bytes(b"something mysterious");
    msg2.set_optional_bool(false);

    proto_assert_eq!(msg1, msg2);
}

#[test]
fn deserialize_in_rust() {
    let mut msg1 = TestAllTypes::new();
    msg1.set_optional_int64(-1);
    msg1.set_optional_bytes(b"some cool data I guess");
    let serialized = unsafe {
        SerializeTestAllTypes(msg1.as_view().__unstable_cpp_repr_grant_permission_to_break())
    };

    let msg2 = TestAllTypes::parse(&serialized).unwrap();
    proto_assert_eq!(msg1, msg2);
}

#[test]
fn deserialize_in_cpp() {
    let mut msg1 = TestAllTypes::new();
    msg1.set_optional_int64(-1);
    msg1.set_optional_bytes(b"some cool data I guess");
    let data = msg1.serialize();

    let msg2 = unsafe {
        TestAllTypes::__unstable_wrap_cpp_grant_permission_to_break(DeserializeTestAllTypes(
            (*data).as_ptr(),
            data.len(),
        ))
    };

    proto_assert_eq!(msg1, msg2);
}

#[test]
fn deserialize_in_cpp_into_mut() {
    let mut msg1 = TestAllTypes::new();
    msg1.set_optional_int64(-1);
    msg1.set_optional_bytes(b"some cool data I guess");
    let data = msg1.serialize();

    let mut raw_msg = unsafe { DeserializeTestAllTypes((*data).as_ptr(), data.len()) };
    let msg2 = TestAllTypesMut::__unstable_wrap_cpp_grant_permission_to_break(&mut raw_msg);

    proto_assert_eq!(msg1, msg2);

    // The C++ still owns the message here and needs to delete it.
    unsafe {
        DeleteTestAllTypes(raw_msg);
    }
}

#[test]
fn deserialize_in_cpp_into_view() {
    let mut msg1 = TestAllTypes::new();
    msg1.set_optional_int64(-1);
    msg1.set_optional_bytes(b"some cool data I guess");
    let data = msg1.serialize();

    let raw_msg = unsafe { DeserializeTestAllTypes((*data).as_ptr(), data.len()) };
    let msg2 = TestAllTypesView::__unstable_wrap_cpp_grant_permission_to_break(&raw_msg);

    proto_assert_eq!(msg1, msg2);

    // The C++ still owns the message here and needs to delete it.
    unsafe {
        DeleteTestAllTypes(raw_msg);
    }
}

// This test ensures that random fields we (Rust) don't know about don't
// accidentally get destroyed by Rust.
#[test]
fn smuggle_extension() {
    let msg1 = unsafe {
        TestAllExtensions::__unstable_wrap_cpp_grant_permission_to_break(NewWithExtension())
    };
    let data = msg1.serialize();

    let mut msg2 = TestAllExtensions::parse(&data).unwrap();
    let bytes = unsafe {
        GetBytesExtension(msg2.as_mut().__unstable_cpp_repr_grant_permission_to_break()).as_ref()
    };
    assert_eq!(bytes, b"smuggled");
}
