// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use protobuf_cpp::__internal::PtrAndLen;
use protobuf_cpp::__internal::RawMessage;
use unittest_proto::proto2_unittest::TestAllExtensions;
use unittest_proto::proto2_unittest::TestAllTypes;

macro_rules! proto_assert_eq {
    ($lhs:expr, $rhs:expr) => {{
        let lhs = &$lhs;
        let rhs = &$rhs;

        assert_eq!(lhs.optional_int64(), rhs.optional_int64());
        assert_eq!(lhs.optional_bytes(), rhs.optional_bytes());
        assert_eq!(lhs.optional_bool(), rhs.optional_bool());
    }};
}

// Helper functions invoking C++ Protobuf APIs directly in C++.
// Defined in `test_utils.cc`.
extern "C" {
    fn DeserializeTestAllTypes(data: *const u8, len: usize) -> RawMessage;
    fn MutateTestAllTypes(msg: RawMessage);
    fn SerializeTestAllTypes(msg: RawMessage) -> protobuf_cpp::__runtime::SerializedData;

    fn NewWithExtension() -> RawMessage;
    fn GetBytesExtension(msg: RawMessage) -> PtrAndLen;
}

#[test]
fn mutate_message_in_cpp() {
    let mut msg1 = TestAllTypes::new();
    unsafe {
        MutateTestAllTypes(msg1.__unstable_cpp_repr_grant_permission_to_break());
    }

    let mut msg2 = TestAllTypes::new();
    msg2.optional_int64_set(Some(42));
    msg2.optional_bytes_mut().set(b"something mysterious");
    msg2.optional_bool_set(Some(false));

    proto_assert_eq!(msg1, msg2);
}

#[test]
fn deserialize_in_rust() {
    let mut msg1 = TestAllTypes::new();
    msg1.optional_int64_set(Some(-1));
    msg1.optional_bytes_mut().set(b"some cool data I guess");
    let serialized =
        unsafe { SerializeTestAllTypes(msg1.__unstable_cpp_repr_grant_permission_to_break()) };

    let mut msg2 = TestAllTypes::new();
    msg2.deserialize(&serialized).unwrap();
    proto_assert_eq!(msg1, msg2);
}

#[test]
fn deserialize_in_cpp() {
    let mut msg1 = TestAllTypes::new();
    msg1.optional_int64_set(Some(-1));
    msg1.optional_bytes_mut().set(b"some cool data I guess");
    let data = msg1.serialize();

    let msg2 = unsafe {
        TestAllTypes::__unstable_wrap_cpp_grant_permission_to_break(DeserializeTestAllTypes(
            (*data).as_ptr(),
            data.len(),
        ))
    };

    proto_assert_eq!(msg1, msg2);
}

// This test ensures that random fields we (Rust) don't know about don't
// accidentally get destroyed by Rust.
#[test]
fn smuggle_extension() {
    let msg1 = unsafe {
        TestAllExtensions::__unstable_wrap_cpp_grant_permission_to_break(NewWithExtension())
    };
    let data = msg1.serialize();

    let mut msg2 = TestAllExtensions::new();
    msg2.deserialize(&data).unwrap();
    let bytes =
        unsafe { GetBytesExtension(msg2.__unstable_cpp_repr_grant_permission_to_break()).as_ref() };
    assert_eq!(&*bytes, b"smuggled");
}
