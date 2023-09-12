// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

//! Tests covering accessors for singular bool, int32, int64, and bytes fields.

use protobuf::Optional;
use unittest_proto::proto2_unittest::{TestAllTypes, TestAllTypes_};

#[test]
fn test_default_accessors() {
    let msg = TestAllTypes::new();
    assert_eq!(msg.default_int32(), 41);
    assert_eq!(msg.default_int64(), 42);
    assert_eq!(msg.default_uint32(), 43);
    assert_eq!(msg.default_uint64(), 44);
    assert_eq!(msg.default_sint32(), -45);
    assert_eq!(msg.default_sint64(), 46);
    assert_eq!(msg.default_fixed32(), 47);
    assert_eq!(msg.default_fixed64(), 48);
    assert_eq!(msg.default_sfixed32(), 49);
    assert_eq!(msg.default_sfixed64(), -50);
    assert_eq!(msg.default_float(), 51.5);
    assert_eq!(msg.default_double(), 52000.0);
    assert_eq!(msg.default_bool(), true);
}

#[test]
fn test_optional_fixed32_accessors() {
    let mut msg = TestAllTypes::new();
    assert_eq!(msg.optional_fixed32_opt(), Optional::Unset(0));
    assert_eq!(msg.optional_fixed32(), 0);

    msg.optional_fixed32_set(Some(99));
    assert_eq!(msg.optional_fixed32_opt(), Optional::Set(99));
    assert_eq!(msg.optional_fixed32(), 99);

    msg.optional_fixed32_set(None);
    assert_eq!(msg.optional_fixed32_opt(), Optional::Unset(0));

    assert_eq!(msg.optional_fixed32(), 0);
}

#[test]
fn test_optional_fixed64_accessors() {
    let mut msg = TestAllTypes::new();
    assert_eq!(msg.optional_fixed64_opt(), Optional::Unset(0));
    assert_eq!(msg.optional_fixed64(), 0);

    msg.optional_fixed64_set(Some(2000));
    assert_eq!(msg.optional_fixed64_opt(), Optional::Set(2000));
    assert_eq!(msg.optional_fixed64(), 2000);

    msg.optional_fixed64_set(None);
    assert_eq!(msg.optional_fixed64_opt(), Optional::Unset(0));
    assert_eq!(msg.optional_fixed64(), 0);
}

#[test]
fn test_optional_int32_accessors() {
    let mut msg = TestAllTypes::new();
    assert_eq!(msg.optional_int32_opt(), Optional::Unset(0));
    assert_eq!(msg.optional_int32(), 0);

    msg.optional_int32_set(Some(1));
    assert_eq!(msg.optional_int32_opt(), Optional::Set(1));
    assert_eq!(msg.optional_int32(), 1);

    msg.optional_int32_set(None);
    assert_eq!(msg.optional_int32_opt(), Optional::Unset(0));

    assert_eq!(msg.optional_int32(), 0);
}

#[test]
fn test_optional_int64_accessors() {
    let mut msg = TestAllTypes::new();
    assert_eq!(msg.optional_int64_opt(), Optional::Unset(0));
    assert_eq!(msg.optional_int64(), 0);

    msg.optional_int64_set(Some(42));
    assert_eq!(msg.optional_int64_opt(), Optional::Set(42));
    assert_eq!(msg.optional_int64(), 42);

    msg.optional_int64_set(None);
    assert_eq!(msg.optional_int64_opt(), Optional::Unset(0));
    assert_eq!(msg.optional_int64(), 0);
}

#[test]
fn test_optional_sint32_accessors() {
    let mut msg = TestAllTypes::new();
    assert_eq!(msg.optional_sint32_opt(), Optional::Unset(0));
    assert_eq!(msg.optional_sint32(), 0);

    msg.optional_sint32_set(Some(-22));
    assert_eq!(msg.optional_sint32_opt(), Optional::Set(-22));
    assert_eq!(msg.optional_sint32(), -22);

    msg.optional_sint32_set(None);
    assert_eq!(msg.optional_sint32_opt(), Optional::Unset(0));
    assert_eq!(msg.optional_sint32(), 0);
}

#[test]
fn test_optional_sint64_accessors() {
    let mut msg = TestAllTypes::new();
    assert_eq!(msg.optional_sint64_opt(), Optional::Unset(0));
    assert_eq!(msg.optional_sint64(), 0);

    msg.optional_sint64_set(Some(7000));
    assert_eq!(msg.optional_sint64_opt(), Optional::Set(7000));
    assert_eq!(msg.optional_sint64(), 7000);

    msg.optional_sint64_set(None);
    assert_eq!(msg.optional_sint64_opt(), Optional::Unset(0));
    assert_eq!(msg.optional_sint64(), 0);
}

#[test]
fn test_optional_uint32_accessors() {
    let mut msg = TestAllTypes::new();
    assert_eq!(msg.optional_uint32_opt(), Optional::Unset(0));
    assert_eq!(msg.optional_uint32(), 0);

    msg.optional_uint32_set(Some(9001));
    assert_eq!(msg.optional_uint32_opt(), Optional::Set(9001));
    assert_eq!(msg.optional_uint32(), 9001);

    msg.optional_uint32_set(None);
    assert_eq!(msg.optional_uint32_opt(), Optional::Unset(0));
    assert_eq!(msg.optional_uint32(), 0);
}

#[test]
fn test_optional_uint64_accessors() {
    let mut msg = TestAllTypes::new();
    assert_eq!(msg.optional_uint64_opt(), Optional::Unset(0));
    assert_eq!(msg.optional_uint64(), 0);

    msg.optional_uint64_set(Some(42));
    assert_eq!(msg.optional_uint64_opt(), Optional::Set(42));
    assert_eq!(msg.optional_uint64(), 42);

    msg.optional_uint64_set(None);
    assert_eq!(msg.optional_uint64_opt(), Optional::Unset(0));
    assert_eq!(msg.optional_uint64(), 0);
}

#[test]
fn test_optional_float_accessors() {
    let mut msg = TestAllTypes::new();
    assert_eq!(msg.optional_float_opt(), Optional::Unset(0.0));
    assert_eq!(msg.optional_float(), 0.0);

    msg.optional_float_set(Some(3.14));
    assert_eq!(msg.optional_float_opt(), Optional::Set(3.14));
    assert_eq!(msg.optional_float(), 3.14);

    msg.optional_float_set(None);
    assert_eq!(msg.optional_float_opt(), Optional::Unset(0.0));
    assert_eq!(msg.optional_float(), 0.0);
}

#[test]
fn test_optional_double_accessors() {
    let mut msg = TestAllTypes::new();
    assert_eq!(msg.optional_double_opt(), Optional::Unset(0.0));
    assert_eq!(msg.optional_double(), 0.0);

    msg.optional_double_set(Some(-10.99));
    assert_eq!(msg.optional_double_opt(), Optional::Set(-10.99));
    assert_eq!(msg.optional_double(), -10.99);

    msg.optional_double_set(None);
    assert_eq!(msg.optional_double_opt(), Optional::Unset(0.0));
    assert_eq!(msg.optional_double(), 0.0);
}

#[test]
fn test_optional_bool_accessors() {
    let mut msg = TestAllTypes::new();
    assert_eq!(msg.optional_bool_opt(), Optional::Unset(false));

    msg.optional_bool_set(Some(true));
    assert_eq!(msg.optional_bool_opt(), Optional::Set(true));

    msg.optional_bool_set(None);
    assert_eq!(msg.optional_bool_opt(), Optional::Unset(false));
}

#[test]
fn test_optional_bytes_accessors() {
    let mut msg = TestAllTypes::new();
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
fn test_nonempty_default_bytes_accessors() {
    let mut msg = TestAllTypes::new();
    assert_eq!(msg.default_bytes(), b"world");
    assert_eq!(msg.default_bytes_opt(), Optional::Unset(&b"world"[..]));
    assert_eq!(msg.default_bytes_mut().get(), b"world");
    assert!(msg.default_bytes_mut().is_unset());

    {
        let s = String::from("hello world");
        msg.default_bytes_mut().set(s.as_bytes());
    }
    assert_eq!(msg.default_bytes(), b"hello world");
    assert_eq!(msg.default_bytes_opt(), Optional::Set(&b"hello world"[..]));
    assert!(msg.default_bytes_mut().is_set());
    assert_eq!(msg.default_bytes_mut().get(), b"hello world");

    msg.default_bytes_mut().or_default().set(b"accessors_test");
    assert_eq!(msg.default_bytes(), b"accessors_test");
    assert_eq!(msg.default_bytes_opt(), Optional::Set(&b"accessors_test"[..]));
    assert!(msg.default_bytes_mut().is_set());
    assert_eq!(msg.default_bytes_mut().get(), b"accessors_test");
    assert_eq!(msg.default_bytes_mut().or_default().get(), b"accessors_test");

    msg.default_bytes_mut().clear();
    assert_eq!(msg.default_bytes(), b"world");
    assert_eq!(msg.default_bytes_opt(), Optional::Unset(&b"world"[..]));
    assert!(msg.default_bytes_mut().is_unset());

    msg.default_bytes_mut().set(b"");
    assert_eq!(msg.default_bytes(), b"");
    assert_eq!(msg.default_bytes_opt(), Optional::Set(&b""[..]));

    msg.default_bytes_mut().clear();
    msg.default_bytes_mut().or_default();
    assert_eq!(msg.default_bytes(), b"world");
    assert_eq!(msg.default_bytes_opt(), Optional::Set(&b"world"[..]));

    msg.default_bytes_mut().or_default().set(b"\xffbinary\x85non-utf8");
    assert_eq!(msg.default_bytes(), b"\xffbinary\x85non-utf8");
    assert_eq!(msg.default_bytes_opt(), Optional::Set(&b"\xffbinary\x85non-utf8"[..]));
    assert!(msg.default_bytes_mut().is_set());
    assert_eq!(msg.default_bytes_mut().get(), b"\xffbinary\x85non-utf8");
    assert_eq!(msg.default_bytes_mut().or_default().get(), b"\xffbinary\x85non-utf8");
}

#[test]
fn test_optional_string_accessors() {
    let mut msg = TestAllTypes::new();
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
fn test_nonempty_default_string_accessors() {
    let mut msg = TestAllTypes::new();
    assert_eq!(msg.default_string(), "hello");
    assert_eq!(msg.default_string_opt(), Optional::Unset("hello".into()));
    assert_eq!(msg.default_string_mut().get(), "hello");
    assert!(msg.default_string_mut().is_unset());

    {
        let s = String::from("hello world");
        msg.default_string_mut().set(&s[..]);
    }
    assert_eq!(msg.default_string(), "hello world");
    assert_eq!(msg.default_string_opt(), Optional::Set("hello world".into()));
    assert!(msg.default_string_mut().is_set());
    assert_eq!(msg.default_string_mut().get(), "hello world");

    msg.default_string_mut().or_default().set("accessors_test");
    assert_eq!(msg.default_string(), "accessors_test");
    assert_eq!(msg.default_string_opt(), Optional::Set("accessors_test".into()));
    assert!(msg.default_string_mut().is_set());
    assert_eq!(msg.default_string_mut().get(), "accessors_test");
    assert_eq!(msg.default_string_mut().or_default().get(), "accessors_test");

    msg.default_string_mut().clear();
    assert_eq!(msg.default_string(), "hello");
    assert_eq!(msg.default_string_opt(), Optional::Unset("hello".into()));
    assert!(msg.default_string_mut().is_unset());

    msg.default_string_mut().set("");
    assert_eq!(msg.default_string(), "");
    assert_eq!(msg.default_string_opt(), Optional::Set("".into()));

    msg.default_string_mut().clear();
    msg.default_string_mut().or_default();
    assert_eq!(msg.default_string(), "hello");
    assert_eq!(msg.default_string_opt(), Optional::Set("hello".into()));
}

#[test]
fn test_singular_msg_field() {
    use crate::TestAllTypes_::NestedMessageView;
    let msg = TestAllTypes::new();
    // TODO("b/285309454"): fetch the inner integer `bb`
    // call should look like msg.optional_nested_message().bb()
    let _msg: NestedMessageView = msg.optional_nested_message();
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
