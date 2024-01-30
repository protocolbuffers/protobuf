// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

//! Tests covering accessors for singular bool, int32, int64, and bytes fields.

use googletest::prelude::*;
use matchers::{is_set, is_unset};
use protobuf::Optional;
use unittest_proto::{TestAllTypes, TestAllTypes_};

#[test]
fn test_default_accessors() {
    let msg = TestAllTypes::new();
    assert_that!(
        msg,
        matches_pattern!(TestAllTypes{
            default_int32(): eq(41),
            default_int64(): eq(42),
            default_uint32(): eq(43),
            default_uint64(): eq(44),
            default_sint32(): eq(-45),
            default_sint64(): eq(46),
            default_fixed32(): eq(47),
            default_fixed64(): eq(48),
            default_sfixed32(): eq(49),
            default_sfixed64(): eq(-50),
            default_float(): eq(51.5),
            default_double(): eq(52000.0),
            default_bool(): eq(true),
        })
    );
    assert_that!(msg.default_string(), eq("hello"));
    assert_that!(msg.default_bytes(), eq("world".as_bytes()));
}

#[test]
fn test_optional_fixed32_accessors() {
    let mut msg = TestAllTypes::new();
    assert_that!(msg.optional_fixed32_opt(), eq(Optional::Unset(0)));
    assert_that!(msg.optional_fixed32(), eq(0));

    msg.optional_fixed32_mut().set(99);
    assert_that!(msg.optional_fixed32_opt(), eq(Optional::Set(99)));
    assert_that!(msg.optional_fixed32(), eq(99));

    msg.optional_fixed32_mut().clear();
    assert_that!(msg.optional_fixed32_opt(), eq(Optional::Unset(0)));
    assert_that!(msg.optional_fixed32(), eq(0));
}

#[test]
fn test_default_fixed32_accessors() {
    let mut msg = TestAllTypes::new();
    assert_that!(msg.default_fixed32(), eq(47));
    assert_that!(msg.default_fixed32_mut().get(), eq(47));
    assert_that!(msg.default_fixed32_mut().is_set(), eq(false));
    assert_that!(msg.default_fixed32_opt(), eq(Optional::Unset(47)));

    msg.default_fixed32_mut().set(999);
    assert_that!(msg.default_fixed32(), eq(999));
    assert_that!(msg.default_fixed32_mut().get(), eq(999));
    assert_that!(msg.default_fixed32_mut().is_set(), eq(true));
    assert_that!(msg.default_fixed32_opt(), eq(Optional::Set(999)));

    msg.default_fixed32_mut().clear();
    assert_that!(msg.default_fixed32(), eq(47));
    assert_that!(msg.default_fixed32_mut().get(), eq(47));
    assert_that!(msg.default_fixed32_mut().is_set(), eq(false));
    assert_that!(msg.default_fixed32_opt(), eq(Optional::Unset(47)));

    msg.default_fixed32_mut().or_default();
    assert_that!(msg.default_fixed32(), eq(47));
    assert_that!(msg.default_fixed32_mut().get(), eq(47));
    assert_that!(msg.default_fixed32_mut().is_set(), eq(true));
    assert_that!(msg.default_fixed32_opt(), eq(Optional::Set(47)));
}

#[test]
fn test_optional_fixed64_accessors() {
    let mut msg = TestAllTypes::new();
    assert_that!(msg.optional_fixed64_opt(), eq(Optional::Unset(0)));
    assert_that!(msg.optional_fixed64(), eq(0));

    msg.optional_fixed64_mut().set(2000);
    assert_that!(msg.optional_fixed64_opt(), eq(Optional::Set(2000)));
    assert_that!(msg.optional_fixed64(), eq(2000));

    msg.optional_fixed64_mut().clear();
    assert_that!(msg.optional_fixed64_opt(), eq(Optional::Unset(0)));
    assert_that!(msg.optional_fixed64(), eq(0));
}

#[test]
fn test_default_fixed64_accessors() {
    let mut msg = TestAllTypes::new();
    assert_that!(msg.default_fixed64(), eq(48));
    assert_that!(msg.default_fixed64_mut().get(), eq(48));
    assert_that!(msg.default_fixed64_mut().is_set(), eq(false));
    assert_that!(msg.default_fixed64_opt(), eq(Optional::Unset(48)));

    msg.default_fixed64_mut().set(999);
    assert_that!(msg.default_fixed64(), eq(999));
    assert_that!(msg.default_fixed64_mut().get(), eq(999));
    assert_that!(msg.default_fixed64_mut().is_set(), eq(true));
    assert_that!(msg.default_fixed64_opt(), eq(Optional::Set(999)));

    msg.default_fixed64_mut().clear();
    assert_that!(msg.default_fixed64(), eq(48));
    assert_that!(msg.default_fixed64_mut().get(), eq(48));
    assert_that!(msg.default_fixed64_mut().is_set(), eq(false));
    assert_that!(msg.default_fixed64_opt(), eq(Optional::Unset(48)));

    msg.default_fixed64_mut().or_default();
    assert_that!(msg.default_fixed64(), eq(48));
    assert_that!(msg.default_fixed64_mut().get(), eq(48));
    assert_that!(msg.default_fixed64_mut().is_set(), eq(true));
    assert_that!(msg.default_fixed64_opt(), eq(Optional::Set(48)));
}

#[test]
fn test_optional_int32_accessors() {
    let mut msg = TestAllTypes::new();
    assert_that!(msg.optional_int32_opt(), eq(Optional::Unset(0)));
    assert_that!(msg.optional_int32(), eq(0));

    msg.optional_int32_mut().set(1);
    assert_that!(msg.optional_int32_opt(), eq(Optional::Set(1)));
    assert_that!(msg.optional_int32(), eq(1));

    msg.optional_int32_mut().clear();
    assert_that!(msg.optional_int32_opt(), eq(Optional::Unset(0)));
    assert_that!(msg.optional_int32(), eq(0));
}

#[test]
fn test_default_int32_accessors() {
    let mut msg = TestAllTypes::new();
    assert_that!(msg.default_int32(), eq(41));
    assert_that!(msg.default_int32_mut().get(), eq(41));
    assert_that!(msg.default_int32_mut().is_set(), eq(false));
    assert_that!(msg.default_int32_opt(), eq(Optional::Unset(41)));

    msg.default_int32_mut().set(999);
    assert_that!(msg.default_int32(), eq(999));
    assert_that!(msg.default_int32_mut().get(), eq(999));
    assert_that!(msg.default_int32_mut().is_set(), eq(true));
    assert_that!(msg.default_int32_opt(), eq(Optional::Set(999)));

    msg.default_int32_mut().clear();
    assert_that!(msg.default_int32(), eq(41));
    assert_that!(msg.default_int32_mut().get(), eq(41));
    assert_that!(msg.default_int32_mut().is_set(), eq(false));
    assert_that!(msg.default_int32_opt(), eq(Optional::Unset(41)));

    msg.default_int32_mut().or_default();
    assert_that!(msg.default_int32(), eq(41));
    assert_that!(msg.default_int32_mut().get(), eq(41));
    assert_that!(msg.default_int32_mut().is_set(), eq(true));
    assert_that!(msg.default_int32_opt(), eq(Optional::Set(41)));
}

#[test]
fn test_optional_int64_accessors() {
    let mut msg = TestAllTypes::new();
    assert_that!(msg.optional_int64_opt(), eq(Optional::Unset(0)));
    assert_that!(msg.optional_int64(), eq(0));

    msg.optional_int64_mut().set(42);
    assert_that!(msg.optional_int64_opt(), eq(Optional::Set(42)));
    assert_that!(msg.optional_int64(), eq(42));

    msg.optional_int64_mut().clear();
    assert_that!(msg.optional_int64_opt(), eq(Optional::Unset(0)));
    assert_that!(msg.optional_int64(), eq(0));
}

#[test]
fn test_default_int64_accessors() {
    let mut msg = TestAllTypes::new();
    assert_that!(msg.default_int64(), eq(42));
    assert_that!(msg.default_int64_mut().get(), eq(42));
    assert_that!(msg.default_int64_mut().is_set(), eq(false));
    assert_that!(msg.default_int64_opt(), eq(Optional::Unset(42)));

    msg.default_int64_mut().set(999);
    assert_that!(msg.default_int64(), eq(999));
    assert_that!(msg.default_int64_mut().get(), eq(999));
    assert_that!(msg.default_int64_mut().is_set(), eq(true));
    assert_that!(msg.default_int64_opt(), eq(Optional::Set(999)));

    msg.default_int64_mut().clear();
    assert_that!(msg.default_int64(), eq(42));
    assert_that!(msg.default_int64_mut().get(), eq(42));
    assert_that!(msg.default_int64_mut().is_set(), eq(false));
    assert_that!(msg.default_int64_opt(), eq(Optional::Unset(42)));

    msg.default_int64_mut().or_default();
    assert_that!(msg.default_int64(), eq(42));
    assert_that!(msg.default_int64_mut().get(), eq(42));
    assert_that!(msg.default_int64_mut().is_set(), eq(true));
    assert_that!(msg.default_int64_opt(), eq(Optional::Set(42)));
}

#[test]
fn test_optional_sint32_accessors() {
    let mut msg = TestAllTypes::new();
    assert_that!(msg.optional_sint32_opt(), eq(Optional::Unset(0)));
    assert_that!(msg.optional_sint32(), eq(0));

    msg.optional_sint32_mut().set(-22);
    assert_that!(msg.optional_sint32_opt(), eq(Optional::Set(-22)));
    assert_that!(msg.optional_sint32(), eq(-22));

    msg.optional_sint32_mut().clear();
    assert_that!(msg.optional_sint32_opt(), eq(Optional::Unset(0)));
    assert_that!(msg.optional_sint32(), eq(0));
}

#[test]
fn test_default_sint32_accessors() {
    let mut msg = TestAllTypes::new();
    assert_that!(msg.default_sint32(), eq(-45));
    assert_that!(msg.default_sint32_mut().get(), eq(-45));
    assert_that!(msg.default_sint32_mut().is_set(), eq(false));
    assert_that!(msg.default_sint32_opt(), eq(Optional::Unset(-45)));

    msg.default_sint32_mut().set(999);
    assert_that!(msg.default_sint32(), eq(999));
    assert_that!(msg.default_sint32_mut().get(), eq(999));
    assert_that!(msg.default_sint32_mut().is_set(), eq(true));
    assert_that!(msg.default_sint32_opt(), eq(Optional::Set(999)));

    msg.default_sint32_mut().clear();
    assert_that!(msg.default_sint32(), eq(-45));
    assert_that!(msg.default_sint32_mut().get(), eq(-45));
    assert_that!(msg.default_sint32_mut().is_set(), eq(false));
    assert_that!(msg.default_sint32_opt(), eq(Optional::Unset(-45)));

    msg.default_sint32_mut().or_default();
    assert_that!(msg.default_sint32(), eq(-45));
    assert_that!(msg.default_sint32_mut().get(), eq(-45));
    assert_that!(msg.default_sint32_mut().is_set(), eq(true));
    assert_that!(msg.default_sint32_opt(), eq(Optional::Set(-45)));
}

#[test]
fn test_optional_sint64_accessors() {
    let mut msg = TestAllTypes::new();
    assert_that!(msg.optional_sint64_opt(), eq(Optional::Unset(0)));
    assert_that!(msg.optional_sint64(), eq(0));

    msg.optional_sint64_mut().set(7000);
    assert_that!(msg.optional_sint64_opt(), eq(Optional::Set(7000)));
    assert_that!(msg.optional_sint64(), eq(7000));

    msg.optional_sint64_mut().clear();
    assert_that!(msg.optional_sint64_opt(), eq(Optional::Unset(0)));
    assert_that!(msg.optional_sint64(), eq(0));
}

#[test]
fn test_default_sint64_accessors() {
    let mut msg = TestAllTypes::new();
    assert_that!(msg.default_sint64(), eq(46));
    assert_that!(msg.default_sint64_mut().get(), eq(46));
    assert_that!(msg.default_sint64_mut().is_set(), eq(false));
    assert_that!(msg.default_sint64_opt(), eq(Optional::Unset(46)));

    msg.default_sint64_mut().set(999);
    assert_that!(msg.default_sint64(), eq(999));
    assert_that!(msg.default_sint64_mut().get(), eq(999));
    assert_that!(msg.default_sint64_mut().is_set(), eq(true));
    assert_that!(msg.default_sint64_opt(), eq(Optional::Set(999)));

    msg.default_sint64_mut().clear();
    assert_that!(msg.default_sint64(), eq(46));
    assert_that!(msg.default_sint64_mut().get(), eq(46));
    assert_that!(msg.default_sint64_mut().is_set(), eq(false));
    assert_that!(msg.default_sint64_opt(), eq(Optional::Unset(46)));

    msg.default_sint64_mut().or_default();
    assert_that!(msg.default_sint64(), eq(46));
    assert_that!(msg.default_sint64_mut().get(), eq(46));
    assert_that!(msg.default_sint64_mut().is_set(), eq(true));
    assert_that!(msg.default_sint64_opt(), eq(Optional::Set(46)));
}

#[test]
fn test_optional_uint32_accessors() {
    let mut msg = TestAllTypes::new();
    assert_that!(msg.optional_uint32_opt(), eq(Optional::Unset(0)));
    assert_that!(msg.optional_uint32(), eq(0));

    msg.optional_uint32_mut().set(9001);
    assert_that!(msg.optional_uint32_opt(), eq(Optional::Set(9001)));
    assert_that!(msg.optional_uint32(), eq(9001));

    msg.optional_uint32_mut().clear();
    assert_that!(msg.optional_uint32_opt(), eq(Optional::Unset(0)));
    assert_that!(msg.optional_uint32(), eq(0));
}

#[test]
fn test_default_uint32_accessors() {
    let mut msg = TestAllTypes::new();
    assert_that!(msg.default_uint32(), eq(43));
    assert_that!(msg.default_uint32_mut().get(), eq(43));
    assert_that!(msg.default_uint32_mut().is_set(), eq(false));
    assert_that!(msg.default_uint32_opt(), eq(Optional::Unset(43)));

    msg.default_uint32_mut().set(999);
    assert_that!(msg.default_uint32(), eq(999));
    assert_that!(msg.default_uint32_mut().get(), eq(999));
    assert_that!(msg.default_uint32_mut().is_set(), eq(true));
    assert_that!(msg.default_uint32_opt(), eq(Optional::Set(999)));

    msg.default_uint32_mut().clear();
    assert_that!(msg.default_uint32(), eq(43));
    assert_that!(msg.default_uint32_mut().get(), eq(43));
    assert_that!(msg.default_uint32_mut().is_set(), eq(false));
    assert_that!(msg.default_uint32_opt(), eq(Optional::Unset(43)));

    msg.default_uint32_mut().or_default();
    assert_that!(msg.default_uint32(), eq(43));
    assert_that!(msg.default_uint32_mut().get(), eq(43));
    assert_that!(msg.default_uint32_mut().is_set(), eq(true));
    assert_that!(msg.default_uint32_opt(), eq(Optional::Set(43)));
}

#[test]
fn test_optional_uint64_accessors() {
    let mut msg = TestAllTypes::new();
    assert_that!(msg.optional_uint64_opt(), eq(Optional::Unset(0)));
    assert_that!(msg.optional_uint64(), eq(0));

    msg.optional_uint64_mut().set(42);
    assert_that!(msg.optional_uint64_opt(), eq(Optional::Set(42)));
    assert_that!(msg.optional_uint64(), eq(42));

    msg.optional_uint64_mut().clear();
    assert_that!(msg.optional_uint64_opt(), eq(Optional::Unset(0)));
    assert_that!(msg.optional_uint64(), eq(0));
}

#[test]
fn test_default_uint64_accessors() {
    let mut msg = TestAllTypes::new();
    assert_that!(msg.default_uint64(), eq(44));
    assert_that!(msg.default_uint64_mut().get(), eq(44));
    assert_that!(msg.default_uint64_mut().is_set(), eq(false));
    assert_that!(msg.default_uint64_opt(), eq(Optional::Unset(44)));

    msg.default_uint64_mut().set(999);
    assert_that!(msg.default_uint64(), eq(999));
    assert_that!(msg.default_uint64_mut().get(), eq(999));
    assert_that!(msg.default_uint64_mut().is_set(), eq(true));
    assert_that!(msg.default_uint64_opt(), eq(Optional::Set(999)));

    msg.default_uint64_mut().clear();
    assert_that!(msg.default_uint64(), eq(44));
    assert_that!(msg.default_uint64_mut().get(), eq(44));
    assert_that!(msg.default_uint64_mut().is_set(), eq(false));
    assert_that!(msg.default_uint64_opt(), eq(Optional::Unset(44)));

    msg.default_uint64_mut().or_default();
    assert_that!(msg.default_uint64(), eq(44));
    assert_that!(msg.default_uint64_mut().get(), eq(44));
    assert_that!(msg.default_uint64_mut().is_set(), eq(true));
    assert_that!(msg.default_uint64_opt(), eq(Optional::Set(44)));
}

#[test]
fn test_optional_float_accessors() {
    let mut msg = TestAllTypes::new();
    assert_that!(msg.optional_float_opt(), eq(Optional::Unset(0.0)));
    assert_that!(msg.optional_float(), eq(0.0));

    msg.optional_float_mut().set(std::f32::consts::PI);
    assert_that!(msg.optional_float_opt(), eq(Optional::Set(std::f32::consts::PI)));
    assert_that!(msg.optional_float(), eq(std::f32::consts::PI));

    msg.optional_float_mut().clear();
    assert_that!(msg.optional_float_opt(), eq(Optional::Unset(0.0)));
    assert_that!(msg.optional_float(), eq(0.0));
}

#[test]
fn test_default_float_accessors() {
    let mut msg = TestAllTypes::new();
    assert_that!(msg.default_float(), eq(51.5));
    assert_that!(msg.default_float_mut().get(), eq(51.5));
    assert_that!(msg.default_float_mut().is_set(), eq(false));
    assert_that!(msg.default_float_opt(), eq(Optional::Unset(51.5)));

    msg.default_float_mut().set(999.9);
    assert_that!(msg.default_float(), eq(999.9));
    assert_that!(msg.default_float_mut().get(), eq(999.9));
    assert_that!(msg.default_float_mut().is_set(), eq(true));
    assert_that!(msg.default_float_opt(), eq(Optional::Set(999.9)));

    msg.default_float_mut().clear();
    assert_that!(msg.default_float(), eq(51.5));
    assert_that!(msg.default_float_mut().get(), eq(51.5));
    assert_that!(msg.default_float_mut().is_set(), eq(false));
    assert_that!(msg.default_float_opt(), eq(Optional::Unset(51.5)));

    msg.default_float_mut().or_default();
    assert_that!(msg.default_float(), eq(51.5));
    assert_that!(msg.default_float_mut().get(), eq(51.5));
    assert_that!(msg.default_float_mut().is_set(), eq(true));
    assert_that!(msg.default_float_opt(), eq(Optional::Set(51.5)));
}

#[test]
fn test_optional_double_accessors() {
    let mut msg = TestAllTypes::new();
    assert_that!(msg.optional_double_opt(), eq(Optional::Unset(0.0)));
    assert_that!(msg.optional_double(), eq(0.0));

    msg.optional_double_mut().set(-10.99);
    assert_that!(msg.optional_double_opt(), eq(Optional::Set(-10.99)));
    assert_that!(msg.optional_double(), eq(-10.99));

    msg.optional_double_mut().clear();
    assert_that!(msg.optional_double_opt(), eq(Optional::Unset(0.0)));
    assert_that!(msg.optional_double(), eq(0.0));
}

#[test]
fn test_default_double_accessors() {
    let mut msg = TestAllTypes::new();
    assert_that!(msg.default_double(), eq(52e3));
    assert_that!(msg.default_double_mut().get(), eq(52e3));
    assert_that!(msg.default_double_mut().is_set(), eq(false));
    assert_that!(msg.default_double_opt(), eq(Optional::Unset(52e3)));

    msg.default_double_mut().set(999.9);
    assert_that!(msg.default_double(), eq(999.9));
    assert_that!(msg.default_double_mut().get(), eq(999.9));
    assert_that!(msg.default_double_mut().is_set(), eq(true));
    assert_that!(msg.default_double_opt(), eq(Optional::Set(999.9)));

    msg.default_double_mut().clear();
    assert_that!(msg.default_double(), eq(52e3));
    assert_that!(msg.default_double_mut().get(), eq(52e3));
    assert_that!(msg.default_double_mut().is_set(), eq(false));
    assert_that!(msg.default_double_opt(), eq(Optional::Unset(52e3)));

    msg.default_double_mut().or_default();
    assert_that!(msg.default_double(), eq(52e3));
    assert_that!(msg.default_double_mut().get(), eq(52e3));
    assert_that!(msg.default_double_mut().is_set(), eq(true));
    assert_that!(msg.default_double_opt(), eq(Optional::Set(52e3)));
}

#[test]
fn test_optional_bool_accessors() {
    let mut msg = TestAllTypes::new();
    assert_that!(msg.optional_bool_opt(), eq(Optional::Unset(false)));

    msg.optional_bool_mut().set(true);
    assert_that!(msg.optional_bool_opt(), eq(Optional::Set(true)));

    msg.optional_bool_mut().clear();
    assert_that!(msg.optional_bool_opt(), eq(Optional::Unset(false)));
}

#[test]
fn test_default_bool_accessors() {
    let mut msg = TestAllTypes::new();
    assert_that!(msg.default_bool(), eq(true));
    assert_that!(msg.default_bool_mut().get(), eq(true));
    assert_that!(msg.default_bool_mut().is_set(), eq(false));
    assert_that!(msg.default_bool_opt(), eq(Optional::Unset(true)));

    msg.default_bool_mut().set(false);
    assert_that!(msg.default_bool(), eq(false));
    assert_that!(msg.default_bool_mut().get(), eq(false));
    assert_that!(msg.default_bool_mut().is_set(), eq(true));
    assert_that!(msg.default_bool_opt(), eq(Optional::Set(false)));

    msg.default_bool_mut().clear();
    assert_that!(msg.default_bool(), eq(true));
    assert_that!(msg.default_bool_mut().get(), eq(true));
    assert_that!(msg.default_bool_mut().is_set(), eq(false));
    assert_that!(msg.default_bool_opt(), eq(Optional::Unset(true)));

    msg.default_bool_mut().or_default();
    assert_that!(msg.default_bool(), eq(true));
    assert_that!(msg.default_bool_mut().get(), eq(true));
    assert_that!(msg.default_bool_mut().is_set(), eq(true));
    assert_that!(msg.default_bool_opt(), eq(Optional::Set(true)));
}

#[test]
fn test_optional_bytes_accessors() {
    let mut msg = TestAllTypes::new();
    assert_that!(*msg.optional_bytes(), empty());
    assert_that!(msg.optional_bytes_opt(), eq(Optional::Unset(&b""[..])));
    assert_that!(*msg.optional_bytes_mut().get(), empty());
    assert_that!(msg.optional_bytes_mut(), is_unset());

    {
        let s = Vec::from(&b"hello world"[..]);
        msg.optional_bytes_mut().set(&s[..]);
    }
    assert_that!(msg.optional_bytes(), eq(b"hello world"));
    assert_that!(msg.optional_bytes_opt(), eq(Optional::Set(&b"hello world"[..])));
    assert_that!(msg.optional_bytes_mut(), is_set());
    assert_that!(msg.optional_bytes_mut().get(), eq(b"hello world"));

    msg.optional_bytes_mut().or_default().set(b"accessors_test");
    assert_that!(msg.optional_bytes(), eq(b"accessors_test"));
    assert_that!(msg.optional_bytes_opt(), eq(Optional::Set(&b"accessors_test"[..])));
    assert_that!(msg.optional_bytes_mut(), is_set());
    assert_that!(msg.optional_bytes_mut().get(), eq(b"accessors_test"));
    assert_that!(msg.optional_bytes_mut().or_default().get(), eq(b"accessors_test"));

    msg.optional_bytes_mut().clear();
    assert_that!(*msg.optional_bytes(), empty());
    assert_that!(msg.optional_bytes_opt(), eq(Optional::Unset(&b""[..])));
    assert_that!(msg.optional_bytes_mut(), is_unset());

    msg.optional_bytes_mut().set(b"");
    assert_that!(*msg.optional_bytes(), empty());
    assert_that!(msg.optional_bytes_opt(), eq(Optional::Set(&b""[..])));

    msg.optional_bytes_mut().clear();
    msg.optional_bytes_mut().or_default();
    assert_that!(*msg.optional_bytes(), empty());
    assert_that!(msg.optional_bytes_opt(), eq(Optional::Set(&b""[..])));

    msg.optional_bytes_mut().or_default().set(b"\xffbinary\x85non-utf8");
    assert_that!(msg.optional_bytes(), eq(b"\xffbinary\x85non-utf8"));
    assert_that!(msg.optional_bytes_opt(), eq(Optional::Set(&b"\xffbinary\x85non-utf8"[..])));
    assert_that!(msg.optional_bytes_mut(), is_set());
    assert_that!(msg.optional_bytes_mut().get(), eq(b"\xffbinary\x85non-utf8"));
    assert_that!(msg.optional_bytes_mut().or_default().get(), eq(b"\xffbinary\x85non-utf8"));
}

#[test]
fn test_nonempty_default_bytes_accessors() {
    let mut msg = TestAllTypes::new();
    assert_that!(msg.default_bytes(), eq(b"world"));
    assert_that!(msg.default_bytes_opt(), eq(Optional::Unset(&b"world"[..])));
    assert_that!(msg.default_bytes_mut().get(), eq(b"world"));
    assert_that!(msg.default_bytes_mut(), is_unset());

    {
        let s = String::from("hello world");
        msg.default_bytes_mut().set(s.as_bytes());
    }
    assert_that!(msg.default_bytes(), eq(b"hello world"));
    assert_that!(msg.default_bytes_opt(), eq(Optional::Set(&b"hello world"[..])));
    assert_that!(msg.default_bytes_mut(), is_set());
    assert_that!(msg.default_bytes_mut().get(), eq(b"hello world"));

    msg.default_bytes_mut().or_default().set(b"accessors_test");
    assert_that!(msg.default_bytes(), eq(b"accessors_test"));
    assert_that!(msg.default_bytes_opt(), eq(Optional::Set(&b"accessors_test"[..])));
    assert_that!(msg.default_bytes_mut(), is_set());
    assert_that!(msg.default_bytes_mut().get(), eq(b"accessors_test"));
    assert_that!(msg.default_bytes_mut().or_default().get(), eq(b"accessors_test"));

    msg.default_bytes_mut().clear();
    assert_that!(msg.default_bytes(), eq(b"world"));
    assert_that!(msg.default_bytes_opt(), eq(Optional::Unset(&b"world"[..])));
    assert_that!(msg.default_bytes_mut(), is_unset());

    msg.default_bytes_mut().set(b"");
    assert_that!(*msg.default_bytes(), empty());
    assert_that!(msg.default_bytes_opt(), eq(Optional::Set(&b""[..])));

    msg.default_bytes_mut().clear();
    msg.default_bytes_mut().or_default();
    assert_that!(msg.default_bytes(), eq(b"world"));
    assert_that!(msg.default_bytes_opt(), eq(Optional::Set(&b"world"[..])));

    msg.default_bytes_mut().or_default().set(b"\xffbinary\x85non-utf8");
    assert_that!(msg.default_bytes(), eq(b"\xffbinary\x85non-utf8"));
    assert_that!(msg.default_bytes_opt(), eq(Optional::Set(&b"\xffbinary\x85non-utf8"[..])));
    assert_that!(msg.default_bytes_mut(), is_set());
    assert_that!(msg.default_bytes_mut().get(), eq(b"\xffbinary\x85non-utf8"));
    assert_that!(msg.default_bytes_mut().or_default().get(), eq(b"\xffbinary\x85non-utf8"));
}

#[test]
fn test_optional_string_accessors() {
    let mut msg = TestAllTypes::new();
    assert_that!(msg.optional_string(), eq(""));
    assert_that!(msg.optional_string_opt(), eq(Optional::Unset("".into())));
    assert_that!(msg.optional_string_mut().get(), eq(""));
    assert_that!(msg.optional_string_mut(), is_unset());

    {
        let s = String::from("hello world");
        msg.optional_string_mut().set(&s[..]);
    }
    assert_that!(msg.optional_string(), eq("hello world"));
    assert_that!(msg.optional_string_opt(), eq(Optional::Set("hello world".into())));
    assert_that!(msg.optional_string_mut(), is_set());
    assert_that!(msg.optional_string_mut().get(), eq("hello world"));

    msg.optional_string_mut().or_default().set("accessors_test");
    assert_that!(msg.optional_string(), eq("accessors_test"));
    assert_that!(msg.optional_string_opt(), eq(Optional::Set("accessors_test".into())));
    assert_that!(msg.optional_string_mut(), is_set());
    assert_that!(msg.optional_string_mut().get(), eq("accessors_test"));
    assert_that!(msg.optional_string_mut().or_default().get(), eq("accessors_test"));

    msg.optional_string_mut().clear();
    assert_that!(msg.optional_string(), eq(""));
    assert_that!(msg.optional_string_opt(), eq(Optional::Unset("".into())));
    assert_that!(msg.optional_string_mut(), is_unset());

    msg.optional_string_mut().set("");
    assert_that!(msg.optional_string(), eq(""));
    assert_that!(msg.optional_string_opt(), eq(Optional::Set("".into())));

    msg.optional_string_mut().clear();
    msg.optional_string_mut().or_default();
    assert_that!(msg.optional_string(), eq(""));
    assert_that!(msg.optional_string_opt(), eq(Optional::Set("".into())));
}

#[test]
fn test_nonempty_default_string_accessors() {
    let mut msg = TestAllTypes::new();
    assert_that!(msg.default_string(), eq("hello"));
    assert_that!(msg.default_string_opt(), eq(Optional::Unset("hello".into())));
    assert_that!(msg.default_string_mut().get(), eq("hello"));
    assert_that!(msg.default_string_mut(), is_unset());

    {
        let s = String::from("hello world");
        msg.default_string_mut().set(&s[..]);
    }
    assert_that!(msg.default_string(), eq("hello world"));
    assert_that!(msg.default_string_opt(), eq(Optional::Set("hello world".into())));
    assert_that!(msg.default_string_mut(), is_set());
    assert_that!(msg.default_string_mut().get(), eq("hello world"));

    msg.default_string_mut().or_default().set("accessors_test");
    assert_that!(msg.default_string(), eq("accessors_test"));
    assert_that!(msg.default_string_opt(), eq(Optional::Set("accessors_test".into())));
    assert_that!(msg.default_string_mut(), is_set());
    assert_that!(msg.default_string_mut().get(), eq("accessors_test"));
    assert_that!(msg.default_string_mut().or_default().get(), eq("accessors_test"));

    msg.default_string_mut().clear();
    assert_that!(msg.default_string(), eq("hello"));
    assert_that!(msg.default_string_opt(), eq(Optional::Unset("hello".into())));
    assert_that!(msg.default_string_mut(), is_unset());

    msg.default_string_mut().set("");
    assert_that!(msg.default_string(), eq(""));
    assert_that!(msg.default_string_opt(), eq(Optional::Set("".into())));

    msg.default_string_mut().clear();
    msg.default_string_mut().or_default();
    assert_that!(msg.default_string(), eq("hello"));
    assert_that!(msg.default_string_opt(), eq(Optional::Set("hello".into())));
}

#[test]
fn test_singular_msg_field() {
    use TestAllTypes_::{NestedMessageMut, NestedMessageView};

    let mut msg = TestAllTypes::new();
    let msg_view: NestedMessageView = msg.optional_nested_message();
    // testing reading an int inside a view
    assert_that!(msg_view.bb(), eq(0));

    let msg_mut: NestedMessageMut = msg.optional_nested_message_mut().or_default();
    // test reading an int inside a mut
    assert_that!(msg_mut.bb(), eq(0));
}

#[test]
fn test_message_opt() {
    let msg = TestAllTypes::new();
    let opt: Optional<
        unittest_proto::TestAllTypes_::NestedMessageView<'_>,
        unittest_proto::TestAllTypes_::NestedMessageView<'_>,
    > = msg.optional_nested_message_opt();
    assert_that!(opt.is_set(), eq(false));
    assert_that!(opt.into_inner().bb(), eq(0));
}

#[test]
fn test_message_opt_set() {
    let mut msg = TestAllTypes::new();
    //let opt = msg.optional_nested_message_mut().or_default();
    //assert_that!(opt.is_set(), eq(false));
    //todo: check for set after prereq cl
    //assert_that!(opt.into_inner().bb(), eq(0));
}

#[test]
fn test_optional_nested_enum_accessors() {
    use TestAllTypes_::NestedEnum;

    let mut msg = TestAllTypes::new();
    assert_that!(msg.optional_nested_enum_opt(), eq(Optional::Unset(NestedEnum::Foo)));
    assert_that!(msg.optional_nested_enum(), eq(NestedEnum::Foo));

    msg.optional_nested_enum_mut().set(NestedEnum::Neg);
    assert_that!(msg.optional_nested_enum_opt(), eq(Optional::Set(NestedEnum::Neg)));
    assert_that!(msg.optional_nested_enum(), eq(NestedEnum::Neg));

    msg.optional_nested_enum_mut().clear();
    assert_that!(msg.optional_nested_enum_opt(), eq(Optional::Unset(NestedEnum::Foo)));
    assert_that!(msg.optional_nested_enum(), eq(NestedEnum::Foo));
}

#[test]
fn test_default_nested_enum_accessors() {
    use TestAllTypes_::NestedEnum;

    let mut msg = TestAllTypes::new();
    assert_that!(msg.default_nested_enum(), eq(NestedEnum::Bar));
    assert_that!(msg.default_nested_enum_mut().get(), eq(NestedEnum::Bar));
    assert_that!(msg.default_nested_enum_mut().is_set(), eq(false));
    assert_that!(msg.default_nested_enum_opt(), eq(Optional::Unset(NestedEnum::Bar)));

    msg.default_nested_enum_mut().set(NestedEnum::Baz);
    assert_that!(msg.default_nested_enum(), eq(NestedEnum::Baz));
    assert_that!(msg.default_nested_enum_mut().get(), eq(NestedEnum::Baz));
    assert_that!(msg.default_nested_enum_mut().is_set(), eq(true));
    assert_that!(msg.default_nested_enum_opt(), eq(Optional::Set(NestedEnum::Baz)));

    msg.default_nested_enum_mut().clear();
    assert_that!(msg.default_nested_enum(), eq(NestedEnum::Bar));
    assert_that!(msg.default_nested_enum_mut().get(), eq(NestedEnum::Bar));
    assert_that!(msg.default_nested_enum_mut().is_set(), eq(false));
    assert_that!(msg.default_nested_enum_opt(), eq(Optional::Unset(NestedEnum::Bar)));

    msg.default_nested_enum_mut().or_default();
    assert_that!(msg.default_nested_enum(), eq(NestedEnum::Bar));
    assert_that!(msg.default_nested_enum_mut().get(), eq(NestedEnum::Bar));
    assert_that!(msg.default_nested_enum_mut().is_set(), eq(true));
    assert_that!(msg.default_nested_enum_opt(), eq(Optional::Set(NestedEnum::Bar)));
}

#[test]
fn test_optional_foreign_enum_accessors() {
    use unittest_proto::ForeignEnum;

    let mut msg = TestAllTypes::new();
    assert_that!(msg.optional_foreign_enum_opt(), eq(Optional::Unset(ForeignEnum::ForeignFoo)));
    assert_that!(msg.optional_foreign_enum(), eq(ForeignEnum::ForeignFoo));

    msg.optional_foreign_enum_mut().set(ForeignEnum::ForeignBax);
    assert_that!(msg.optional_foreign_enum_opt(), eq(Optional::Set(ForeignEnum::ForeignBax)));
    assert_that!(msg.optional_foreign_enum(), eq(ForeignEnum::ForeignBax));

    msg.optional_foreign_enum_mut().clear();
    assert_that!(msg.optional_foreign_enum_opt(), eq(Optional::Unset(ForeignEnum::ForeignFoo)));
    assert_that!(msg.optional_foreign_enum(), eq(ForeignEnum::ForeignFoo));
}

#[test]
fn test_default_foreign_enum_accessors() {
    use unittest_proto::ForeignEnum;

    let mut msg = TestAllTypes::new();
    assert_that!(msg.default_foreign_enum(), eq(ForeignEnum::ForeignBar));
    assert_that!(msg.default_foreign_enum_mut().get(), eq(ForeignEnum::ForeignBar));
    assert_that!(msg.default_foreign_enum_mut().is_set(), eq(false));
    assert_that!(msg.default_foreign_enum_opt(), eq(Optional::Unset(ForeignEnum::ForeignBar)));

    msg.default_foreign_enum_mut().set(ForeignEnum::ForeignBaz);
    assert_that!(msg.default_foreign_enum(), eq(ForeignEnum::ForeignBaz));
    assert_that!(msg.default_foreign_enum_mut().get(), eq(ForeignEnum::ForeignBaz));
    assert_that!(msg.default_foreign_enum_mut().is_set(), eq(true));
    assert_that!(msg.default_foreign_enum_opt(), eq(Optional::Set(ForeignEnum::ForeignBaz)));

    msg.default_foreign_enum_mut().clear();
    assert_that!(msg.default_foreign_enum(), eq(ForeignEnum::ForeignBar));
    assert_that!(msg.default_foreign_enum_mut().get(), eq(ForeignEnum::ForeignBar));
    assert_that!(msg.default_foreign_enum_mut().is_set(), eq(false));
    assert_that!(msg.default_foreign_enum_opt(), eq(Optional::Unset(ForeignEnum::ForeignBar)));

    msg.default_foreign_enum_mut().or_default();
    assert_that!(msg.default_foreign_enum(), eq(ForeignEnum::ForeignBar));
    assert_that!(msg.default_foreign_enum_mut().get(), eq(ForeignEnum::ForeignBar));
    assert_that!(msg.default_foreign_enum_mut().is_set(), eq(true));
    assert_that!(msg.default_foreign_enum_opt(), eq(Optional::Set(ForeignEnum::ForeignBar)));
}

#[test]
fn test_optional_import_enum_accessors() {
    use unittest_proto::ImportEnum;

    let mut msg = TestAllTypes::new();
    assert_that!(msg.optional_import_enum_opt(), eq(Optional::Unset(ImportEnum::ImportFoo)));
    assert_that!(msg.optional_import_enum(), eq(ImportEnum::ImportFoo));

    msg.optional_import_enum_mut().set(ImportEnum::ImportBar);
    assert_that!(msg.optional_import_enum_opt(), eq(Optional::Set(ImportEnum::ImportBar)));
    assert_that!(msg.optional_import_enum(), eq(ImportEnum::ImportBar));

    msg.optional_import_enum_mut().clear();
    assert_that!(msg.optional_import_enum_opt(), eq(Optional::Unset(ImportEnum::ImportFoo)));
    assert_that!(msg.optional_import_enum(), eq(ImportEnum::ImportFoo));
}

#[test]
fn test_default_import_enum_accessors() {
    use unittest_proto::ImportEnum;

    let mut msg = TestAllTypes::new();
    assert_that!(msg.default_import_enum(), eq(ImportEnum::ImportBar));
    assert_that!(msg.default_import_enum_mut().get(), eq(ImportEnum::ImportBar));
    assert_that!(msg.default_import_enum_mut().is_set(), eq(false));
    assert_that!(msg.default_import_enum_opt(), eq(Optional::Unset(ImportEnum::ImportBar)));

    msg.default_import_enum_mut().set(ImportEnum::ImportBaz);
    assert_that!(msg.default_import_enum(), eq(ImportEnum::ImportBaz));
    assert_that!(msg.default_import_enum_mut().get(), eq(ImportEnum::ImportBaz));
    assert_that!(msg.default_import_enum_mut().is_set(), eq(true));
    assert_that!(msg.default_import_enum_opt(), eq(Optional::Set(ImportEnum::ImportBaz)));

    msg.default_import_enum_mut().clear();
    assert_that!(msg.default_import_enum(), eq(ImportEnum::ImportBar));
    assert_that!(msg.default_import_enum_mut().get(), eq(ImportEnum::ImportBar));
    assert_that!(msg.default_import_enum_mut().is_set(), eq(false));
    assert_that!(msg.default_import_enum_opt(), eq(Optional::Unset(ImportEnum::ImportBar)));

    msg.default_import_enum_mut().or_default();
    assert_that!(msg.default_import_enum(), eq(ImportEnum::ImportBar));
    assert_that!(msg.default_import_enum_mut().get(), eq(ImportEnum::ImportBar));
    assert_that!(msg.default_import_enum_mut().is_set(), eq(true));
    assert_that!(msg.default_import_enum_opt(), eq(Optional::Set(ImportEnum::ImportBar)));
}

#[test]
fn test_oneof_accessors() {
    use unittest_proto::TestOneof2;
    use unittest_proto::TestOneof2_::{Foo::*, NestedEnum};

    let mut msg = TestOneof2::new();
    assert_that!(msg.foo(), matches_pattern!(not_set(_)));

    msg.foo_int_mut().set(7);
    assert_that!(msg.foo_int_opt(), eq(Optional::Set(7)));
    assert_that!(msg.foo(), matches_pattern!(FooInt(eq(7))));

    msg.foo_int_mut().clear();
    assert_that!(msg.foo_int_opt(), eq(Optional::Unset(0)));
    assert_that!(msg.foo(), matches_pattern!(not_set(_)));

    msg.foo_int_mut().set(7);
    msg.foo_bytes_mut().set(b"123");
    assert_that!(msg.foo_int_opt(), eq(Optional::Unset(0)));

    assert_that!(msg.foo(), matches_pattern!(FooBytes(eq(b"123"))));

    msg.foo_enum_mut().set(NestedEnum::Foo);
    assert_that!(msg.foo(), matches_pattern!(FooEnum(eq(NestedEnum::Foo))));

    // Test the accessors or $Msg$Mut
    let mut msg_mut = msg.as_mut();
    assert_that!(msg_mut.foo(), matches_pattern!(FooEnum(eq(NestedEnum::Foo))));
    msg_mut.foo_int_mut().set(7);
    msg_mut.foo_bytes_mut().set(b"123");
    assert_that!(msg_mut.foo(), matches_pattern!(FooBytes(eq(b"123"))));
    assert_that!(msg_mut.foo_int_opt(), eq(Optional::Unset(0)));

    // Test the accessors on $Msg$View
    let msg_view = msg.as_view();
    assert_that!(msg_view.foo(), matches_pattern!(FooBytes(eq(b"123"))));
    assert_that!(msg_view.foo_int_opt(), eq(Optional::Unset(0)));

    // TODO: Add tests covering a message-type field in a oneof.
}

#[test]
fn test_oneof_mut_accessors() {
    use unittest_proto::TestOneof2;
    use unittest_proto::TestOneof2_::{Foo, FooMut::*, NestedEnum};

    let mut msg = TestOneof2::new();
    assert_that!(msg.foo_mut(), matches_pattern!(not_set(_)));

    msg.foo_int_mut().set(7);

    match msg.foo_mut() {
        FooInt(mut v) => {
            assert_that!(v.get(), eq(7));
            v.set(8);
            assert_that!(v.get(), eq(8));
        }
        f => panic!("unexpected field_mut type! {:?}", f),
    }

    // Confirm that the mut write above applies to both the field accessor and the
    // oneof view accessor.
    assert_that!(msg.foo_int_opt(), eq(Optional::Set(8)));
    assert_that!(msg.foo(), matches_pattern!(Foo::FooInt(_)));

    msg.foo_int_mut().clear();
    assert_that!(msg.foo_mut(), matches_pattern!(not_set(_)));

    msg.foo_int_mut().set(7);
    msg.foo_bytes_mut().set(b"123");
    assert_that!(msg.foo_mut(), matches_pattern!(FooBytes(_)));

    msg.foo_enum_mut().set(NestedEnum::Baz);
    assert_that!(msg.foo_mut(), matches_pattern!(FooEnum(_)));

    // Test the mut accessors or $Msg$Mut
    let mut msg_mut = msg.as_mut();
    match msg_mut.foo_mut() {
        FooEnum(mut v) => {
            assert_that!(v.get(), eq(NestedEnum::Baz));
            v.set(NestedEnum::Bar);
            assert_that!(v.get(), eq(NestedEnum::Bar));
        }
        f => panic!("unexpected field_mut type! {:?}", f),
    }
    assert_that!(msg.foo_enum(), eq(NestedEnum::Bar));

    // TODO:  Add tests covering a message-type field in a oneof.
}

#[test]
fn test_msg_oneof_default_accessors() {
    use unittest_proto::TestOneof2_::{Bar::*, NestedEnum};

    let mut msg = unittest_proto::TestOneof2::new();
    assert_that!(msg.bar(), matches_pattern!(not_set(_)));

    msg.bar_int_mut().set(7);
    assert_that!(msg.bar_int_opt(), eq(Optional::Set(7)));
    assert_that!(msg.bar(), matches_pattern!(BarInt(eq(7))));

    msg.bar_int_mut().clear();
    assert_that!(msg.bar_int_opt(), eq(Optional::Unset(5)));
    assert_that!(msg.bar(), matches_pattern!(not_set(_)));

    msg.bar_int_mut().set(7);
    msg.bar_bytes_mut().set(b"123");
    assert_that!(msg.bar_int_opt(), eq(Optional::Unset(5)));
    assert_that!(msg.bar_enum_opt(), eq(Optional::Unset(NestedEnum::Bar)));
    assert_that!(msg.bar(), matches_pattern!(BarBytes(eq(b"123"))));

    msg.bar_enum_mut().set(NestedEnum::Baz);
    assert_that!(msg.bar(), matches_pattern!(BarEnum(eq(NestedEnum::Baz))));
    assert_that!(msg.bar_int_opt(), eq(Optional::Unset(5)));

    // TODO: Add tests covering a message-type field in a oneof.
}

#[test]
fn test_oneof_default_mut_accessors() {
    use unittest_proto::TestOneof2_::{Bar, BarMut, BarMut::*, NestedEnum};

    let mut msg = unittest_proto::TestOneof2::new();
    assert_that!(msg.bar_mut(), matches_pattern!(not_set(_)));

    msg.bar_int_mut().set(7);

    match msg.bar_mut() {
        BarInt(mut v) => {
            assert_that!(v.get(), eq(7));
            v.set(8);
            assert_that!(v.get(), eq(8));
        }
        f => panic!("unexpected field_mut type! {:?}", f),
    }

    // Confirm that the mut write above applies to all three of:
    // - The field accessor
    // - The oneof mut accessor
    // - The oneof view accessor
    // And then each of the applicable cases on:
    // - The owned msg directly
    // - The msg as a $Msg$Mut
    // - The msg as a $Msg$View
    assert_that!(msg.bar_int_opt(), eq(Optional::Set(8)));
    assert_that!(msg.bar_mut(), matches_pattern!(BarMut::BarInt(_)));
    assert_that!(msg.bar(), matches_pattern!(Bar::BarInt(_)));

    let mut msg_mut = msg.as_mut();
    assert_that!(msg_mut.bar_int_opt(), eq(Optional::Set(8)));
    assert_that!(msg_mut.bar_mut(), matches_pattern!(BarMut::BarInt(_)));
    assert_that!(msg_mut.bar(), matches_pattern!(Bar::BarInt(_)));

    let msg_view = msg.as_view();
    assert_that!(msg_view.bar_int_opt(), eq(Optional::Set(8)));
    // This test correctly fails to compile if this line is uncommented:
    // assert_that!(msg_view.bar_mut(), matches_pattern!(BarMut::BarInt(_)));
    assert_that!(msg_view.bar(), matches_pattern!(Bar::BarInt(_)));

    msg.bar_int_mut().clear();
    assert_that!(msg.bar_mut(), matches_pattern!(not_set(_)));

    msg.bar_int_mut().set(7);
    msg.bar_bytes_mut().set(b"123");
    assert_that!(msg.bar_mut(), matches_pattern!(BarBytes(_)));

    msg.bar_enum_mut().set(NestedEnum::Baz);
    assert_that!(msg.bar_mut(), matches_pattern!(BarEnum(_)));

    // TODO: Add tests covering a message-type field in a oneof.
}

#[test]
fn test_set_message_from_view() {
    use protobuf::MutProxy;

    let mut m1 = TestAllTypes::new();
    m1.optional_int32_mut().set(1);
    let mut m2 = TestAllTypes::new();
    m2.as_mut().set(m1.as_view());

    assert_that!(m2.optional_int32(), eq(1i32));
}
