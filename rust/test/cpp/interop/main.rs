// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

use std::ptr::NonNull;
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
    fn DeserializeTestAllTypes(data: *const u8, len: usize) -> NonNull<u8>;
    fn MutateTestAllTypes(msg: NonNull<u8>);
    fn SerializeTestAllTypes(msg: NonNull<u8>) -> protobuf_cpp::SerializedData;
}

#[test]
fn mutate_message_in_cpp() {
    let mut msg1 = TestAllTypes::new();
    unsafe {
        MutateTestAllTypes(msg1.__unstable_cpp_repr_grant_permission_to_break());
    }

    let mut msg2 = TestAllTypes::new();
    msg2.optional_int64_set(Some(42));
    msg2.optional_bytes_set(Some(b"something mysterious"));
    msg2.optional_bool_set(Some(false));

    proto_assert_eq!(msg1, msg2);
}

#[test]
fn deserialize_in_rust() {
    let mut msg1 = TestAllTypes::new();
    msg1.optional_int64_set(Some(-1));
    msg1.optional_bytes_set(Some(b"some cool data I guess"));
    let serialized = unsafe {
        SerializeTestAllTypes(msg1.__unstable_cpp_repr_grant_permission_to_break())
    };

    let mut msg2 = TestAllTypes::new();
    msg2.deserialize(&serialized).unwrap();
    proto_assert_eq!(msg1, msg2);
}

#[test]
fn deserialize_in_cpp() {
    let mut msg1 = TestAllTypes::new();
    msg1.optional_int64_set(Some(-1));
    msg1.optional_bytes_set(Some(b"some cool data I guess"));
    let data = msg1.serialize();

    let msg2 = unsafe {
        TestAllTypes::__unstable_wrap_cpp_grant_permission_to_break(
            DeserializeTestAllTypes(data.as_ptr(), data.len()),
        )
    };

    proto_assert_eq!(msg1, msg2);
}
