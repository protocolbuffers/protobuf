// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

/// Tests covering accessors for singular bool, int32, int64, and bytes fields
/// on proto3.
use protobuf::Optional;
use unittest_proto3::proto3_unittest::{TestAllTypes, TestAllTypes_};
use unittest_proto3_optional::proto2_unittest::TestProto3Optional;

#[test]
fn test_fixed32_accessors() {
    let mut msg = TestAllTypes::new();
    assert_eq!(msg.optional_fixed32(), 0);
    assert_eq!(msg.optional_fixed32_mut().get(), 0);

    msg.optional_fixed32_mut().set(42);
    assert_eq!(msg.optional_fixed32_mut().get(), 42);
    assert_eq!(msg.optional_fixed32(), 42);

    msg.optional_fixed32_mut().clear();
    assert_eq!(msg.optional_fixed32(), 0);
    assert_eq!(msg.optional_fixed32_mut().get(), 0);
}

#[test]
fn test_bool_accessors() {
    let mut msg = TestAllTypes::new();
    assert!(!msg.optional_bool());
    assert!(!msg.optional_bool_mut().get());

    msg.optional_bool_mut().set(true);
    assert!(msg.optional_bool());
    assert!(msg.optional_bool_mut().get());

    msg.optional_bool_mut().clear();
    assert!(!msg.optional_bool());
    assert!(!msg.optional_bool_mut().get());
}

#[test]
fn test_bytes_accessors() {
    let mut msg = TestAllTypes::new();
    // Note: even though it's named 'optional_bytes', the field is actually not
    // proto3 optional, so it does not support presence.
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

#[test]
fn test_string_accessors() {
    let mut msg = TestAllTypes::new();
    // Note: even though it's named 'optional_string', the field is actually not
    // proto3 optional, so it does not support presence.
    assert_eq!(msg.optional_string(), "");
    assert_eq!(msg.optional_string_mut().get(), "");

    msg.optional_string_mut().set("accessors_test");
    assert_eq!(msg.optional_string(), "accessors_test");
    assert_eq!(msg.optional_string_mut().get(), "accessors_test");

    {
        let s = String::from("hello world");
        msg.optional_string_mut().set(&s[..]);
    }
    assert_eq!(msg.optional_string(), "hello world");
    assert_eq!(msg.optional_string_mut().get(), "hello world");

    msg.optional_string_mut().clear();
    assert_eq!(msg.optional_string(), "");
    assert_eq!(msg.optional_string_mut().get(), "");

    msg.optional_string_mut().set("");
    assert_eq!(msg.optional_string(), "");
    assert_eq!(msg.optional_string_mut().get(), "");
}

#[test]
fn test_optional_string_accessors() {
    let mut msg = TestProto3Optional::new();
    assert_eq!(msg.optional_string(), "");
    assert_eq!(msg.optional_string_opt(), Optional::Unset("".into()));
    assert_eq!(msg.optional_string_mut().get(), "");
    assert!(msg.optional_string_mut().is_unset());

    {
        let s = String::from("hello world");
        msg.optional_string_mut().set(&s[..]);
    }
    assert_eq!(msg.optional_string(), "hello world");
    assert_eq!(msg.optional_string_opt(), Optional::Set("hello world".into()));
    assert!(msg.optional_string_mut().is_set());
    assert_eq!(msg.optional_string_mut().get(), "hello world");

    msg.optional_string_mut().or_default().set("accessors_test");
    assert_eq!(msg.optional_string(), "accessors_test");
    assert_eq!(msg.optional_string_opt(), Optional::Set("accessors_test".into()));
    assert!(msg.optional_string_mut().is_set());
    assert_eq!(msg.optional_string_mut().get(), "accessors_test");
    assert_eq!(msg.optional_string_mut().or_default().get(), "accessors_test");

    msg.optional_string_mut().clear();
    assert_eq!(msg.optional_string(), "");
    assert_eq!(msg.optional_string_opt(), Optional::Unset("".into()));
    assert!(msg.optional_string_mut().is_unset());

    msg.optional_string_mut().set("");
    assert_eq!(msg.optional_string(), "");
    assert_eq!(msg.optional_string_opt(), Optional::Set("".into()));

    msg.optional_string_mut().clear();
    msg.optional_string_mut().or_default();
    assert_eq!(msg.optional_string(), "");
    assert_eq!(msg.optional_string_opt(), Optional::Set("".into()));
}

#[test]
fn test_oneof_accessors() {
    use TestAllTypes_::OneofField::*;

    let mut msg = TestAllTypes::new();
    assert!(matches!(msg.oneof_field(), not_set(_)));

    msg.oneof_uint32_set(Some(7));
    assert_eq!(msg.oneof_uint32_opt(), Optional::Set(7));
    assert!(matches!(msg.oneof_field(), OneofUint32(7)));

    msg.oneof_uint32_set(None);
    assert_eq!(msg.oneof_uint32_opt(), Optional::Unset(0));
    assert!(matches!(msg.oneof_field(), not_set(_)));

    msg.oneof_uint32_set(Some(7));
    msg.oneof_bytes_mut().set(b"");
    assert_eq!(msg.oneof_uint32_opt(), Optional::Unset(0));

    // This should show it set to the OneofBytes but its not supported yet.
    assert!(matches!(msg.oneof_field(), not_set(_)));
}
