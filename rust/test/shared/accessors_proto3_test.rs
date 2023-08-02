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

/// Tests covering accessors for singular bool, int32, int64, and bytes fields
/// on proto3.
use protobuf::Optional;
use unittest_proto3::proto3_unittest::TestAllTypes;
use unittest_proto3_optional::proto2_unittest::TestProto3Optional;

#[test]
fn test_fixed32_accessors() {
    let mut msg = TestAllTypes::new();
    assert_eq!(msg.optional_fixed32(), 0);

    msg.optional_fixed32_set(Some(99));
    assert_eq!(msg.optional_fixed32(), 99);

    msg.optional_fixed32_set(None);
    assert_eq!(msg.optional_fixed32(), 0);
}

#[test]
fn test_bytes_accessors() {
    let mut msg = TestAllTypes::new();
    // Note: even though its named 'optional_bytes' the field is actually not proto3
    // optional, so it does not support presence.
    assert_eq!(msg.optional_bytes(), b"");
    assert_eq!(msg.optional_bytes_mut().get(), b"");

    msg.optional_bytes_mut().set(b"accessors_test");
    assert_eq!(msg.optional_bytes(), b"accessors_test");
    assert_eq!(msg.optional_bytes_mut().get(), b"accessors_test");

    {
        let s = Vec::from(&b"hello world"[..]);
        msg.optional_bytes_mut().set(&s[..]);
    }
    assert_eq!(msg.optional_bytes(), b"hello world");
    assert_eq!(msg.optional_bytes_mut().get(), b"hello world");

    msg.optional_bytes_mut().clear();
    assert_eq!(msg.optional_bytes(), b"");
    assert_eq!(msg.optional_bytes_mut().get(), b"");

    msg.optional_bytes_mut().set(b"");
    assert_eq!(msg.optional_bytes(), b"");
    assert_eq!(msg.optional_bytes_mut().get(), b"");
}

#[test]
fn test_optional_bytes_accessors() {
    let mut msg = TestProto3Optional::new();
    assert_eq!(msg.optional_bytes(), b"");
    assert_eq!(msg.optional_bytes_opt(), Optional::Unset(&b""[..]));
    assert_eq!(msg.optional_bytes_mut().get(), b"");
    assert!(msg.optional_bytes_mut().is_unset());

    {
        let s = Vec::from(&b"hello world"[..]);
        msg.optional_bytes_mut().set(&s[..]);
    }
    assert_eq!(msg.optional_bytes(), b"hello world");
    assert_eq!(msg.optional_bytes_opt(), Optional::Set(&b"hello world"[..]));
    assert!(msg.optional_bytes_mut().is_set());
    assert_eq!(msg.optional_bytes_mut().get(), b"hello world");

    msg.optional_bytes_mut().or_default().set(b"accessors_test");
    assert_eq!(msg.optional_bytes(), b"accessors_test");
    assert_eq!(msg.optional_bytes_opt(), Optional::Set(&b"accessors_test"[..]));
    assert!(msg.optional_bytes_mut().is_set());
    assert_eq!(msg.optional_bytes_mut().get(), b"accessors_test");
    assert_eq!(msg.optional_bytes_mut().or_default().get(), b"accessors_test");

    msg.optional_bytes_mut().clear();
    assert_eq!(msg.optional_bytes(), b"");
    assert_eq!(msg.optional_bytes_opt(), Optional::Unset(&b""[..]));
    assert!(msg.optional_bytes_mut().is_unset());

    msg.optional_bytes_mut().set(b"");
    assert_eq!(msg.optional_bytes(), b"");
    assert_eq!(msg.optional_bytes_opt(), Optional::Set(&b""[..]));

    msg.optional_bytes_mut().clear();
    msg.optional_bytes_mut().or_default();
    assert_eq!(msg.optional_bytes(), b"");
    assert_eq!(msg.optional_bytes_opt(), Optional::Set(&b""[..]));

    msg.optional_bytes_mut().or_default().set(b"\xffbinary\x85non-utf8");
    assert_eq!(msg.optional_bytes(), b"\xffbinary\x85non-utf8");
    assert_eq!(msg.optional_bytes_opt(), Optional::Set(&b"\xffbinary\x85non-utf8"[..]));
    assert!(msg.optional_bytes_mut().is_set());
    assert_eq!(msg.optional_bytes_mut().get(), b"\xffbinary\x85non-utf8");
    assert_eq!(msg.optional_bytes_mut().or_default().get(), b"\xffbinary\x85non-utf8");
}
