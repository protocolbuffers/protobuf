// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
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
//     * Neither the name of Google LLC. nor the names of its
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

/// Tests covering accessors for singular bool, int64, and bytes fields.
use unittest_proto::proto2_unittest::TestAllTypes;

#[test]
fn test_default_accessors() {
    let msg = TestAllTypes::new();
    assert_eq!(msg.default_int32(), 41);
    assert_eq!(msg.default_int64(), 42);
    assert_eq!(msg.default_sint32(), -45);
    assert_eq!(msg.default_sint64(), 46);
    assert_eq!(msg.default_uint32(), 43);
    assert_eq!(msg.default_uint64(), 44);
    assert_eq!(msg.default_bool(), true);
}

#[test]
fn test_optional_int32_accessors() {
    let mut msg = TestAllTypes::new();
    assert_eq!(msg.optional_int32_opt(), None);
    assert_eq!(msg.optional_int32(), 0);

    msg.optional_int32_set(Some(1));
    assert_eq!(msg.optional_int32_opt(), Some(1));
    assert_eq!(msg.optional_int32(), 1);

    msg.optional_int32_set(None);
    assert_eq!(msg.optional_int32_opt(), None);

    assert_eq!(msg.optional_int32(), 0);
}

#[test]
fn test_optional_int64_accessors() {
    let mut msg = TestAllTypes::new();
    assert_eq!(msg.optional_int64_opt(), None);
    assert_eq!(msg.optional_int64(), 0);

    msg.optional_int64_set(Some(42));
    assert_eq!(msg.optional_int64_opt(), Some(42));
    assert_eq!(msg.optional_int64(), 42);

    msg.optional_int64_set(None);
    assert_eq!(msg.optional_int64_opt(), None);
    assert_eq!(msg.optional_int64(), 0);
}

#[test]
fn test_optional_sint32_accessors() {
    let mut msg = TestAllTypes::new();
    assert_eq!(msg.optional_sint32_opt(), None);
    assert_eq!(msg.optional_sint32(), 0);

    msg.optional_sint32_set(Some(-22));
    assert_eq!(msg.optional_sint32_opt(), Some(-22));
    assert_eq!(msg.optional_sint32(), -22);

    msg.optional_sint32_set(None);
    assert_eq!(msg.optional_sint32_opt(), None);
    assert_eq!(msg.optional_sint32(), 0);
}

#[test]
fn test_optional_sint64_accessors() {
    let mut msg = TestAllTypes::new();
    assert_eq!(msg.optional_sint64_opt(), None);
    assert_eq!(msg.optional_sint64(), 0);

    msg.optional_sint64_set(Some(7000));
    assert_eq!(msg.optional_sint64_opt(), Some(7000));
    assert_eq!(msg.optional_sint64(), 7000);

    msg.optional_sint64_set(None);
    assert_eq!(msg.optional_sint64_opt(), None);
    assert_eq!(msg.optional_sint64(), 0);
}

#[test]
fn test_optional_uint32_accessors() {
    let mut msg = TestAllTypes::new();
    assert_eq!(msg.optional_uint32_opt(), None);
    assert_eq!(msg.optional_uint32(), 0);

    msg.optional_uint32_set(Some(9001));
    assert_eq!(msg.optional_uint32_opt(), Some(9001));
    assert_eq!(msg.optional_uint32(), 9001);

    msg.optional_uint32_set(None);
    assert_eq!(msg.optional_uint32_opt(), None);
    assert_eq!(msg.optional_uint32(), 0);
}

#[test]
fn test_optional_uint64_accessors() {
    let mut msg = TestAllTypes::new();
    assert_eq!(msg.optional_uint64_opt(), None);
    assert_eq!(msg.optional_uint64(), 0);

    msg.optional_uint64_set(Some(42));
    assert_eq!(msg.optional_uint64_opt(), Some(42));
    assert_eq!(msg.optional_uint64(), 42);

    msg.optional_uint64_set(None);
    assert_eq!(msg.optional_uint64_opt(), None);
    assert_eq!(msg.optional_uint64(), 0);
}

#[test]
fn test_optional_bool_accessors() {
    let mut msg = TestAllTypes::new();
    assert_eq!(msg.optional_bool_opt(), None);

    msg.optional_bool_set(Some(true));
    assert_eq!(msg.optional_bool_opt(), Some(true));

    msg.optional_bool_set(None);
    assert_eq!(msg.optional_bool_opt(), None);
}

#[test]
fn test_optional_bytes_accessors() {
    let mut msg = TestAllTypes::new();
    assert_eq!(msg.optional_bytes(), None);

    msg.optional_bytes_set(Some(b"accessors_test"));
    assert_eq!(msg.optional_bytes().unwrap(), b"accessors_test");

    msg.optional_bytes_set(None);
    assert_eq!(msg.optional_bytes(), None);

    msg.optional_bytes_set(Some(b""));
    assert_eq!(msg.optional_bytes().unwrap(), b"");
}
