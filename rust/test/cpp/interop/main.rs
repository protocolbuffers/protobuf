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

macro_rules! assert_serializes_equally {
    ($msg:ident) => {{
        let mut msg = $msg;
        let serialized_cpp =
            unsafe { Serialize(msg.__unstable_cpp_repr_grant_permission_to_break()) };
        let serialized_rs = msg.serialize();

        assert_eq!(*serialized_rs, *serialized_cpp);
    }};
}

#[test]
fn mutate_message_in_cpp() {
    let mut msg = TestAllTypes::new();
    unsafe { MutateInt64Field(msg.__unstable_cpp_repr_grant_permission_to_break()) };
    assert_serializes_equally!(msg);
}

#[test]
fn mutate_message_in_rust() {
    let mut msg = TestAllTypes::new();
    msg.optional_int64_set(Some(43));
    assert_serializes_equally!(msg);
}

#[test]
fn deserialize_message_in_rust() {
    let serialized = unsafe { SerializeMutatedInstance() };
    let mut msg = TestAllTypes::new();
    msg.deserialize(&serialized).unwrap();
    assert_serializes_equally!(msg);
}

// Helper functions invoking C++ Protobuf APIs directly in C++. Defined in
// `//third_party/protobuf/rust/test/cpp/interop:test_utils`.
extern "C" {
    fn SerializeMutatedInstance() -> protobuf_cpp::SerializedData;
    fn MutateInt64Field(msg: NonNull<u8>);
    fn Serialize(msg: NonNull<u8>) -> protobuf_cpp::SerializedData;
}
