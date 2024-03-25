// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

//! Tests covering accessors for singular bool, int32, int64, and bytes fields.

use googletest::prelude::*;
use protobuf::{MutProxy, Optional};
use unittest_proto::{test_all_types, TestAllTypes};

#[test]
fn test_default_accessors() {
    let msg: TestAllTypes = Default::default();
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
    assert_that!(msg.has_optional_fixed32(), eq(false));
    assert_that!(msg.optional_fixed32_opt(), eq(Optional::Unset(0)));
    assert_that!(msg.optional_fixed32(), eq(0));

    msg.set_optional_fixed32(7);
    assert_that!(msg.has_optional_fixed32(), eq(true));
    assert_that!(msg.optional_fixed32_opt(), eq(Optional::Set(7)));
    assert_that!(msg.optional_fixed32(), eq(7));

    msg.clear_optional_fixed32();
    assert_that!(msg.has_optional_fixed32(), eq(false));
    assert_that!(msg.optional_fixed32_opt(), eq(Optional::Unset(0)));
    assert_that!(msg.optional_fixed32(), eq(0));
}

#[test]
fn test_default_fixed32_accessors() {
    let mut msg = TestAllTypes::new();
    assert_that!(msg.default_fixed32(), eq(47));
    assert_that!(msg.has_default_fixed32(), eq(false));
    assert_that!(msg.default_fixed32_opt(), eq(Optional::Unset(47)));

    msg.set_default_fixed32(7);
    assert_that!(msg.default_fixed32(), eq(7));
    assert_that!(msg.has_default_fixed32(), eq(true));
    assert_that!(msg.default_fixed32_opt(), eq(Optional::Set(7)));

    msg.clear_default_fixed32();
    assert_that!(msg.default_fixed32(), eq(47));
    assert_that!(msg.has_default_fixed32(), eq(false));
    assert_that!(msg.default_fixed32_opt(), eq(Optional::Unset(47)));
}

#[test]
fn test_optional_fixed64_accessors() {
    let mut msg = TestAllTypes::new();
    assert_that!(msg.optional_fixed64_opt(), eq(Optional::Unset(0)));
    assert_that!(msg.optional_fixed64(), eq(0));

    msg.set_optional_fixed64(99);
    assert_that!(msg.optional_fixed64_opt(), eq(Optional::Set(99)));
    assert_that!(msg.optional_fixed64(), eq(99));

    msg.set_optional_fixed64(2000);
    assert_that!(msg.optional_fixed64_opt(), eq(Optional::Set(2000)));
    assert_that!(msg.optional_fixed64(), eq(2000));

    msg.clear_optional_fixed64();
    assert_that!(msg.optional_fixed64_opt(), eq(Optional::Unset(0)));
    assert_that!(msg.optional_fixed64(), eq(0));
}

#[test]
fn test_default_fixed64_accessors() {
    let mut msg = TestAllTypes::new();
    assert_that!(msg.default_fixed64(), eq(48));
    assert_that!(msg.default_fixed64_opt(), eq(Optional::Unset(48)));

    msg.set_default_fixed64(4);
    assert_that!(msg.default_fixed64(), eq(4));
    assert_that!(msg.default_fixed64_opt(), eq(Optional::Set(4)));

    msg.set_default_fixed64(999);
    assert_that!(msg.default_fixed64(), eq(999));
    assert_that!(msg.default_fixed64_opt(), eq(Optional::Set(999)));

    msg.clear_default_fixed64();
    assert_that!(msg.default_fixed64(), eq(48));
    assert_that!(msg.default_fixed64_opt(), eq(Optional::Unset(48)));
}

#[test]
fn test_optional_int32_accessors() {
    let mut msg = TestAllTypes::new();
    assert_that!(msg.optional_int32_opt(), eq(Optional::Unset(0)));
    assert_that!(msg.optional_int32(), eq(0));

    msg.set_optional_int32(0);
    assert_that!(msg.optional_int32_opt(), eq(Optional::Set(0)));
    assert_that!(msg.optional_int32(), eq(0));

    msg.set_optional_int32(1);
    assert_that!(msg.optional_int32_opt(), eq(Optional::Set(1)));
    assert_that!(msg.optional_int32(), eq(1));

    msg.clear_optional_int32();
    assert_that!(msg.optional_int32_opt(), eq(Optional::Unset(0)));
    assert_that!(msg.optional_int32(), eq(0));
}

#[test]
fn test_default_int32_accessors() {
    let mut msg = TestAllTypes::new();
    assert_that!(msg.default_int32(), eq(41));
    assert_that!(msg.default_int32_opt(), eq(Optional::Unset(41)));

    msg.set_default_int32(41);
    assert_that!(msg.default_int32(), eq(41));
    assert_that!(msg.default_int32_opt(), eq(Optional::Set(41)));

    msg.clear_default_int32();
    assert_that!(msg.default_int32(), eq(41));
    assert_that!(msg.default_int32_opt(), eq(Optional::Unset(41)));

    msg.set_default_int32(999);
    assert_that!(msg.default_int32(), eq(999));
    assert_that!(msg.default_int32_opt(), eq(Optional::Set(999)));

    msg.clear_default_int32();
    assert_that!(msg.default_int32(), eq(41));
    assert_that!(msg.default_int32_opt(), eq(Optional::Unset(41)));
}

#[test]
fn test_optional_int64_accessors() {
    let mut msg = TestAllTypes::new();
    assert_that!(msg.optional_int64_opt(), eq(Optional::Unset(0)));
    assert_that!(msg.optional_int64(), eq(0));

    msg.set_optional_int64(42);
    assert_that!(msg.optional_int64_opt(), eq(Optional::Set(42)));
    assert_that!(msg.optional_int64(), eq(42));

    msg.clear_optional_int64();
    assert_that!(msg.optional_int64_opt(), eq(Optional::Unset(0)));
    assert_that!(msg.optional_int64(), eq(0));
}

#[test]
fn test_default_int64_accessors() {
    let mut msg = TestAllTypes::new();
    assert_that!(msg.default_int64(), eq(42));
    assert_that!(msg.default_int64_opt(), eq(Optional::Unset(42)));

    msg.set_default_int64(999);
    assert_that!(msg.default_int64(), eq(999));
    assert_that!(msg.default_int64_opt(), eq(Optional::Set(999)));

    msg.clear_default_int64();
    assert_that!(msg.default_int64(), eq(42));
    assert_that!(msg.default_int64_opt(), eq(Optional::Unset(42)));
}

#[test]
fn test_optional_sint32_accessors() {
    let mut msg = TestAllTypes::new();
    assert_that!(msg.optional_sint32_opt(), eq(Optional::Unset(0)));
    assert_that!(msg.optional_sint32(), eq(0));

    msg.set_optional_sint32(-22);
    assert_that!(msg.optional_sint32_opt(), eq(Optional::Set(-22)));
    assert_that!(msg.optional_sint32(), eq(-22));

    msg.clear_optional_sint32();
    assert_that!(msg.optional_sint32_opt(), eq(Optional::Unset(0)));
    assert_that!(msg.optional_sint32(), eq(0));
}

#[test]
fn test_default_sint32_accessors() {
    let mut msg = TestAllTypes::new();
    assert_that!(msg.default_sint32(), eq(-45));
    assert_that!(msg.default_sint32_opt(), eq(Optional::Unset(-45)));

    msg.set_default_sint32(999);
    assert_that!(msg.default_sint32(), eq(999));
    assert_that!(msg.default_sint32_opt(), eq(Optional::Set(999)));

    msg.clear_default_sint32();
    assert_that!(msg.default_sint32(), eq(-45));
    assert_that!(msg.default_sint32_opt(), eq(Optional::Unset(-45)));
}

#[test]
fn test_optional_sint64_accessors() {
    let mut msg = TestAllTypes::new();
    assert_that!(msg.optional_sint64_opt(), eq(Optional::Unset(0)));
    assert_that!(msg.optional_sint64(), eq(0));

    msg.set_optional_sint64(7000);
    assert_that!(msg.optional_sint64_opt(), eq(Optional::Set(7000)));
    assert_that!(msg.optional_sint64(), eq(7000));

    msg.clear_optional_sint64();
    assert_that!(msg.optional_sint64_opt(), eq(Optional::Unset(0)));
    assert_that!(msg.optional_sint64(), eq(0));
}

#[test]
fn test_default_sint64_accessors() {
    let mut msg = TestAllTypes::new();
    assert_that!(msg.default_sint64(), eq(46));
    assert_that!(msg.default_sint64_opt(), eq(Optional::Unset(46)));

    msg.set_default_sint64(999);
    assert_that!(msg.default_sint64(), eq(999));
    assert_that!(msg.default_sint64_opt(), eq(Optional::Set(999)));

    msg.clear_default_sint64();
    assert_that!(msg.default_sint64(), eq(46));
    assert_that!(msg.default_sint64_opt(), eq(Optional::Unset(46)));
}

#[test]
fn test_optional_uint32_accessors() {
    let mut msg = TestAllTypes::new();
    assert_that!(msg.optional_uint32_opt(), eq(Optional::Unset(0)));
    assert_that!(msg.optional_uint32(), eq(0));

    msg.set_optional_uint32(9001);
    assert_that!(msg.optional_uint32_opt(), eq(Optional::Set(9001)));
    assert_that!(msg.optional_uint32(), eq(9001));

    msg.clear_optional_uint32();
    assert_that!(msg.optional_uint32_opt(), eq(Optional::Unset(0)));
    assert_that!(msg.optional_uint32(), eq(0));
}

#[test]
fn test_default_uint32_accessors() {
    let mut msg = TestAllTypes::new();
    assert_that!(msg.default_uint32(), eq(43));
    assert_that!(msg.default_uint32_opt(), eq(Optional::Unset(43)));

    msg.set_default_uint32(999);
    assert_that!(msg.default_uint32(), eq(999));
    assert_that!(msg.default_uint32_opt(), eq(Optional::Set(999)));

    msg.clear_default_uint32();
    assert_that!(msg.default_uint32(), eq(43));
    assert_that!(msg.default_uint32_opt(), eq(Optional::Unset(43)));
}

#[test]
fn test_optional_uint64_accessors() {
    let mut msg = TestAllTypes::new();
    assert_that!(msg.optional_uint64_opt(), eq(Optional::Unset(0)));
    assert_that!(msg.optional_uint64(), eq(0));

    msg.set_optional_uint64(42);
    assert_that!(msg.optional_uint64_opt(), eq(Optional::Set(42)));
    assert_that!(msg.optional_uint64(), eq(42));

    msg.clear_optional_uint64();
    assert_that!(msg.optional_uint64_opt(), eq(Optional::Unset(0)));
    assert_that!(msg.optional_uint64(), eq(0));
}

#[test]
fn test_default_uint64_accessors() {
    let mut msg = TestAllTypes::new();
    assert_that!(msg.default_uint64(), eq(44));
    assert_that!(msg.default_uint64_opt(), eq(Optional::Unset(44)));

    msg.set_default_uint64(999);
    assert_that!(msg.default_uint64(), eq(999));
    assert_that!(msg.default_uint64_opt(), eq(Optional::Set(999)));

    msg.clear_default_uint64();
    assert_that!(msg.default_uint64(), eq(44));
    assert_that!(msg.default_uint64_opt(), eq(Optional::Unset(44)));
}

#[test]
fn test_optional_float_accessors() {
    let mut msg = TestAllTypes::new();
    assert_that!(msg.optional_float_opt(), eq(Optional::Unset(0.0)));
    assert_that!(msg.optional_float(), eq(0.0));

    msg.set_optional_float(std::f32::consts::PI);
    assert_that!(msg.optional_float_opt(), eq(Optional::Set(std::f32::consts::PI)));
    assert_that!(msg.optional_float(), eq(std::f32::consts::PI));

    msg.clear_optional_float();
    assert_that!(msg.optional_float_opt(), eq(Optional::Unset(0.0)));
    assert_that!(msg.optional_float(), eq(0.0));
}

#[test]
fn test_default_float_accessors() {
    let mut msg = TestAllTypes::new();
    assert_that!(msg.default_float(), eq(51.5));
    assert_that!(msg.default_float_opt(), eq(Optional::Unset(51.5)));

    msg.set_default_float(999.9);
    assert_that!(msg.default_float(), eq(999.9));
    assert_that!(msg.default_float_opt(), eq(Optional::Set(999.9)));

    msg.clear_default_float();
    assert_that!(msg.default_float(), eq(51.5));
    assert_that!(msg.default_float_opt(), eq(Optional::Unset(51.5)));
}

#[test]
fn test_optional_double_accessors() {
    let mut msg = TestAllTypes::new();
    assert_that!(msg.optional_double_opt(), eq(Optional::Unset(0.0)));
    assert_that!(msg.optional_double(), eq(0.0));

    msg.set_optional_double(-10.99);
    assert_that!(msg.optional_double_opt(), eq(Optional::Set(-10.99)));
    assert_that!(msg.optional_double(), eq(-10.99));

    msg.clear_optional_double();
    assert_that!(msg.optional_double_opt(), eq(Optional::Unset(0.0)));
    assert_that!(msg.optional_double(), eq(0.0));
}

#[test]
fn test_default_double_accessors() {
    let mut msg = TestAllTypes::new();
    assert_that!(msg.default_double(), eq(52e3));
    assert_that!(msg.default_double_opt(), eq(Optional::Unset(52e3)));

    msg.set_default_double(999.9);
    assert_that!(msg.default_double(), eq(999.9));
    assert_that!(msg.default_double_opt(), eq(Optional::Set(999.9)));

    msg.clear_default_double();
    assert_that!(msg.default_double(), eq(52e3));
    assert_that!(msg.default_double_opt(), eq(Optional::Unset(52e3)));
}

#[test]
fn test_optional_bool_accessors() {
    let mut msg = TestAllTypes::new();
    assert_that!(msg.optional_bool_opt(), eq(Optional::Unset(false)));

    msg.set_optional_bool(true);
    assert_that!(msg.optional_bool_opt(), eq(Optional::Set(true)));

    msg.clear_optional_bool();
    assert_that!(msg.optional_bool_opt(), eq(Optional::Unset(false)));
}

#[test]
fn test_default_bool_accessors() {
    let mut msg = TestAllTypes::new();
    assert_that!(msg.default_bool(), eq(true));
    assert_that!(msg.default_bool_opt(), eq(Optional::Unset(true)));

    msg.set_default_bool(false);
    assert_that!(msg.default_bool(), eq(false));
    assert_that!(msg.default_bool_opt(), eq(Optional::Set(false)));

    msg.clear_default_bool();
    assert_that!(msg.default_bool(), eq(true));
    assert_that!(msg.default_bool_opt(), eq(Optional::Unset(true)));
}

#[test]
fn test_optional_bytes_accessors() {
    let mut msg = TestAllTypes::new();
    assert_that!(*msg.optional_bytes(), empty());
    assert_that!(msg.has_optional_bytes(), eq(false));
    assert_that!(msg.optional_bytes_opt(), eq(Optional::Unset(&b""[..])));

    {
        let s = Vec::from(&b"hello world"[..]);
        msg.set_optional_bytes(&s[..]);
    }
    assert_that!(msg.optional_bytes(), eq(b"hello world"));
    assert_that!(msg.has_optional_bytes(), eq(true));
    assert_that!(msg.optional_bytes_opt(), eq(Optional::Set(&b"hello world"[..])));

    msg.clear_optional_bytes();
    assert_that!(*msg.optional_bytes(), empty());
    assert_that!(msg.has_optional_bytes(), eq(false));
    assert_that!(msg.optional_bytes_opt(), eq(Optional::Unset(&b""[..])));

    msg.set_optional_bytes(b"");
    assert_that!(*msg.optional_bytes(), empty());
    assert_that!(msg.has_optional_bytes(), eq(true));
    assert_that!(msg.optional_bytes_opt(), eq(Optional::Set(&b""[..])));
}

#[test]
fn test_nonempty_default_bytes_accessors() {
    let mut msg = TestAllTypes::new();
    assert_that!(msg.default_bytes(), eq(b"world"));
    assert_that!(msg.has_default_bytes(), eq(false));
    assert_that!(msg.default_bytes_opt(), eq(Optional::Unset(&b"world"[..])));

    {
        let s = String::from("hello world");
        msg.set_default_bytes(s.as_bytes());
    }
    assert_that!(msg.default_bytes(), eq(b"hello world"));
    assert_that!(msg.has_default_bytes(), eq(true));
    assert_that!(msg.default_bytes_opt(), eq(Optional::Set(&b"hello world"[..])));

    msg.clear_default_bytes();
    assert_that!(msg.default_bytes(), eq(b"world"));
    assert_that!(msg.has_default_bytes(), eq(false));
    assert_that!(msg.default_bytes_opt(), eq(Optional::Unset(&b"world"[..])));

    msg.set_default_bytes(b"");
    assert_that!(*msg.default_bytes(), empty());
    assert_that!(msg.default_bytes_opt(), eq(Optional::Set(&b""[..])));

    msg.clear_default_bytes();
    assert_that!(msg.default_bytes(), eq(b"world"));
    assert_that!(msg.default_bytes_opt(), eq(Optional::Unset(&b"world"[..])));
}

#[test]
fn test_optional_string_accessors() {
    let mut msg = TestAllTypes::new();
    assert_that!(msg.optional_string(), eq(""));
    assert_that!(msg.optional_string_opt(), eq(Optional::Unset("".into())));

    {
        let s = String::from("hello world");
        msg.set_optional_string(&s[..]);
    }
    assert_that!(msg.optional_string(), eq("hello world"));
    assert_that!(msg.optional_string_opt(), eq(Optional::Set("hello world".into())));

    msg.clear_optional_string();
    assert_that!(msg.optional_string(), eq(""));
    assert_that!(msg.optional_string_opt(), eq(Optional::Unset("".into())));

    msg.set_optional_string("");
    assert_that!(msg.optional_string(), eq(""));
    assert_that!(msg.optional_string_opt(), eq(Optional::Set("".into())));

    msg.clear_optional_string();
    assert_that!(msg.optional_string(), eq(""));
    assert_that!(msg.optional_string_opt(), eq(Optional::Unset("".into())));
}

#[test]
fn test_nonempty_default_string_accessors() {
    let mut msg = TestAllTypes::new();
    assert_that!(msg.default_string(), eq("hello"));
    assert_that!(msg.default_string_opt(), eq(Optional::Unset("hello".into())));

    {
        let s = String::from("hello world");
        msg.set_default_string(&s[..]);
    }
    assert_that!(msg.default_string(), eq("hello world"));
    assert_that!(msg.default_string_opt(), eq(Optional::Set("hello world".into())));

    msg.clear_default_string();
    assert_that!(msg.default_string(), eq("hello"));
    assert_that!(msg.default_string_opt(), eq(Optional::Unset("hello".into())));

    msg.set_default_string("");
    assert_that!(msg.default_string(), eq(""));
    assert_that!(msg.default_string_opt(), eq(Optional::Set("".into())));

    msg.clear_default_string();
    assert_that!(msg.default_string(), eq("hello"));
    assert_that!(msg.default_string_opt(), eq(Optional::Unset("hello".into())));
}

#[test]
fn test_singular_msg_field() {
    use test_all_types::*;

    let mut msg = TestAllTypes::new();
    let msg_view = msg.optional_nested_message();
    // testing reading an int inside a view
    assert_that!(msg_view.bb(), eq(0));

    assert_that!(msg.has_optional_nested_message(), eq(false));
    let mut nested_msg_mut = msg.optional_nested_message_mut();

    // test reading an int inside a mut
    assert_that!(nested_msg_mut.bb(), eq(0));

    // Test setting an owned NestedMessage onto another message.
    let mut new_nested = NestedMessage::new();
    new_nested.set_bb(7);
    nested_msg_mut.set(new_nested);
    assert_that!(nested_msg_mut.bb(), eq(7));

    assert_that!(msg.has_optional_nested_message(), eq(true));
}

#[test]
fn test_message_opt() {
    let msg = TestAllTypes::new();
    let opt: Optional<
        unittest_proto::test_all_types::NestedMessageView<'_>,
        unittest_proto::test_all_types::NestedMessageView<'_>,
    > = msg.optional_nested_message_opt();
    assert_that!(opt.is_set(), eq(false));
    assert_that!(opt.into_inner().bb(), eq(0));
}

#[test]
fn test_message_opt_set() {
    let mut msg = TestAllTypes::new();
    let submsg = test_all_types::NestedMessage::new();
    msg.set_optional_nested_message(submsg);
    msg.clear_optional_nested_message();
    assert_that!(msg.optional_nested_message_opt().is_set(), eq(false));
}

#[test]
fn test_setting_submsg() {
    let mut msg = TestAllTypes::new();
    let submsg = test_all_types::NestedMessage::new();

    assert_that!(msg.has_optional_nested_message(), eq(false));
    assert_that!(msg.optional_nested_message_opt().is_set(), eq(false));

    msg.set_optional_nested_message(submsg);
    // confirm that invoking .set on a submsg indeed flips the set bit
    assert_that!(msg.has_optional_nested_message(), eq(true));
    assert_that!(msg.optional_nested_message_opt().is_set(), eq(true));

    msg.clear_optional_nested_message();
    assert_that!(msg.has_optional_nested_message(), eq(false));
    assert_that!(msg.optional_nested_message_opt().is_set(), eq(false));
}

#[test]
fn test_msg_mut_initializes() {
    let mut msg = TestAllTypes::new();
    assert_that!(msg.has_optional_nested_message(), eq(false));
    assert_that!(msg.optional_nested_message_opt().is_set(), eq(false));

    let _ = msg.optional_nested_message_mut();
    // confirm that that optional_nested_message_mut makes the field Present
    assert_that!(msg.has_optional_nested_message(), eq(true));
    assert_that!(msg.optional_nested_message_opt().is_set(), eq(true));

    msg.clear_optional_nested_message();
    assert_that!(msg.has_optional_nested_message(), eq(false));
    assert_that!(msg.optional_nested_message_opt().is_set(), eq(false));
}

#[test]
fn test_optional_nested_enum_accessors() {
    use test_all_types::NestedEnum;

    let mut msg = TestAllTypes::new();
    assert_that!(msg.has_optional_nested_enum(), eq(false));
    assert_that!(msg.optional_nested_enum_opt(), eq(Optional::Unset(NestedEnum::Foo)));
    assert_that!(msg.optional_nested_enum(), eq(NestedEnum::Foo));

    msg.set_optional_nested_enum(NestedEnum::Neg);
    assert_that!(msg.has_optional_nested_enum(), eq(true));
    assert_that!(msg.optional_nested_enum_opt(), eq(Optional::Set(NestedEnum::Neg)));
    assert_that!(msg.optional_nested_enum(), eq(NestedEnum::Neg));

    msg.clear_optional_nested_enum();
    assert_that!(msg.has_optional_nested_enum(), eq(false));
    assert_that!(msg.optional_nested_enum_opt(), eq(Optional::Unset(NestedEnum::Foo)));
    assert_that!(msg.optional_nested_enum(), eq(NestedEnum::Foo));
}

#[test]
fn test_default_nested_enum_accessors() {
    use test_all_types::NestedEnum;

    let mut msg = TestAllTypes::new();
    assert_that!(msg.default_nested_enum(), eq(NestedEnum::Bar));
    assert_that!(msg.default_nested_enum_opt(), eq(Optional::Unset(NestedEnum::Bar)));

    msg.set_default_nested_enum(NestedEnum::Baz);
    assert_that!(msg.default_nested_enum(), eq(NestedEnum::Baz));
    assert_that!(msg.default_nested_enum_opt(), eq(Optional::Set(NestedEnum::Baz)));

    msg.clear_default_nested_enum();
    assert_that!(msg.default_nested_enum(), eq(NestedEnum::Bar));
    assert_that!(msg.default_nested_enum_opt(), eq(Optional::Unset(NestedEnum::Bar)));
}

#[test]
fn test_optional_foreign_enum_accessors() {
    use unittest_proto::ForeignEnum;

    let mut msg = TestAllTypes::new();
    assert_that!(msg.optional_foreign_enum_opt(), eq(Optional::Unset(ForeignEnum::ForeignFoo)));
    assert_that!(msg.optional_foreign_enum(), eq(ForeignEnum::ForeignFoo));

    msg.set_optional_foreign_enum(ForeignEnum::ForeignBax);
    assert_that!(msg.optional_foreign_enum_opt(), eq(Optional::Set(ForeignEnum::ForeignBax)));
    assert_that!(msg.optional_foreign_enum(), eq(ForeignEnum::ForeignBax));

    msg.clear_optional_foreign_enum();
    assert_that!(msg.optional_foreign_enum_opt(), eq(Optional::Unset(ForeignEnum::ForeignFoo)));
    assert_that!(msg.optional_foreign_enum(), eq(ForeignEnum::ForeignFoo));
}

#[test]
fn test_default_foreign_enum_accessors() {
    use unittest_proto::ForeignEnum;

    let mut msg = TestAllTypes::new();
    assert_that!(msg.default_foreign_enum(), eq(ForeignEnum::ForeignBar));
    assert_that!(msg.default_foreign_enum_opt(), eq(Optional::Unset(ForeignEnum::ForeignBar)));

    msg.set_default_foreign_enum(ForeignEnum::ForeignBaz);
    assert_that!(msg.default_foreign_enum(), eq(ForeignEnum::ForeignBaz));
    assert_that!(msg.default_foreign_enum_opt(), eq(Optional::Set(ForeignEnum::ForeignBaz)));

    msg.clear_default_foreign_enum();
    assert_that!(msg.default_foreign_enum(), eq(ForeignEnum::ForeignBar));
    assert_that!(msg.default_foreign_enum_opt(), eq(Optional::Unset(ForeignEnum::ForeignBar)));
}

#[test]
fn test_optional_import_enum_accessors() {
    use unittest_proto::ImportEnum;

    let mut msg = TestAllTypes::new();
    assert_that!(msg.optional_import_enum_opt(), eq(Optional::Unset(ImportEnum::ImportFoo)));
    assert_that!(msg.optional_import_enum(), eq(ImportEnum::ImportFoo));

    msg.set_optional_import_enum(ImportEnum::ImportBar);
    assert_that!(msg.optional_import_enum_opt(), eq(Optional::Set(ImportEnum::ImportBar)));
    assert_that!(msg.optional_import_enum(), eq(ImportEnum::ImportBar));

    msg.clear_optional_import_enum();
    assert_that!(msg.optional_import_enum_opt(), eq(Optional::Unset(ImportEnum::ImportFoo)));
    assert_that!(msg.optional_import_enum(), eq(ImportEnum::ImportFoo));
}

#[test]
fn test_default_import_enum_accessors() {
    use unittest_proto::ImportEnum;

    let mut msg = TestAllTypes::new();
    assert_that!(msg.default_import_enum(), eq(ImportEnum::ImportBar));
    assert_that!(msg.default_import_enum_opt(), eq(Optional::Unset(ImportEnum::ImportBar)));

    msg.set_default_import_enum(ImportEnum::ImportBaz);
    assert_that!(msg.default_import_enum(), eq(ImportEnum::ImportBaz));
    assert_that!(msg.default_import_enum_opt(), eq(Optional::Set(ImportEnum::ImportBaz)));

    msg.clear_default_import_enum();
    assert_that!(msg.default_import_enum(), eq(ImportEnum::ImportBar));
    assert_that!(msg.default_import_enum_opt(), eq(Optional::Unset(ImportEnum::ImportBar)));
}

#[test]
fn test_oneof_accessors() {
    use unittest_proto::test_oneof2::{Foo::*, FooCase, NestedEnum};
    use unittest_proto::TestOneof2;

    let mut msg = TestOneof2::new();
    assert_that!(msg.foo(), matches_pattern!(not_set(_)));
    assert_that!(msg.foo_case(), eq(FooCase::not_set));

    msg.set_foo_int(7);
    assert_that!(msg.has_foo_int(), eq(true));
    assert_that!(msg.foo_int_opt(), eq(Optional::Set(7)));
    assert_that!(msg.foo(), matches_pattern!(FooInt(eq(7))));
    assert_that!(msg.foo_case(), eq(FooCase::FooInt));

    msg.clear_foo_int();
    assert_that!(msg.has_foo_int(), eq(false));
    assert_that!(msg.foo_int_opt(), eq(Optional::Unset(0)));
    assert_that!(msg.foo(), matches_pattern!(not_set(_)));
    assert_that!(msg.foo_case(), eq(FooCase::not_set));

    msg.set_foo_int(7);
    msg.set_foo_bytes(b"123");
    assert_that!(msg.has_foo_int(), eq(false));
    assert_that!(msg.foo_int_opt(), eq(Optional::Unset(0)));

    assert_that!(msg.foo(), matches_pattern!(FooBytes(eq(b"123"))));
    assert_that!(msg.foo_case(), eq(FooCase::FooBytes));

    msg.set_foo_enum(NestedEnum::Foo);
    assert_that!(msg.foo(), matches_pattern!(FooEnum(eq(NestedEnum::Foo))));
    assert_that!(msg.foo_case(), eq(FooCase::FooEnum));

    // Test the accessors or $Msg$Mut
    let mut msg_mut = msg.as_mut();
    assert_that!(msg_mut.foo(), matches_pattern!(FooEnum(eq(NestedEnum::Foo))));
    msg_mut.set_foo_int(7);
    msg_mut.set_foo_bytes(b"123");
    assert_that!(msg_mut.foo(), matches_pattern!(FooBytes(eq(b"123"))));
    assert_that!(msg_mut.foo_case(), eq(FooCase::FooBytes));
    assert_that!(msg_mut.foo_int_opt(), eq(Optional::Unset(0)));

    // Test the accessors on $Msg$View
    let msg_view = msg.as_view();
    assert_that!(msg_view.foo(), matches_pattern!(FooBytes(eq(b"123"))));
    assert_that!(msg_view.foo_case(), eq(FooCase::FooBytes));
    assert_that!(msg_view.foo_int_opt(), eq(Optional::Unset(0)));

    // TODO: Add tests covering a message-type field in a oneof.
}

#[test]
fn test_msg_oneof_default_accessors() {
    use unittest_proto::test_oneof2::{Bar::*, BarCase, NestedEnum};

    let mut msg = unittest_proto::TestOneof2::new();
    assert_that!(msg.bar(), matches_pattern!(not_set(_)));

    msg.set_bar_int(7);
    assert_that!(msg.bar_int_opt(), eq(Optional::Set(7)));
    assert_that!(msg.bar(), matches_pattern!(BarInt(eq(7))));
    assert_that!(msg.bar_case(), eq(BarCase::BarInt));

    msg.clear_bar_int();
    assert_that!(msg.bar_int_opt(), eq(Optional::Unset(5)));
    assert_that!(msg.bar(), matches_pattern!(not_set(_)));
    assert_that!(msg.bar_case(), eq(BarCase::not_set));

    msg.set_bar_int(7);
    msg.set_bar_bytes(b"123");
    assert_that!(msg.bar_int_opt(), eq(Optional::Unset(5)));
    assert_that!(msg.bar_enum_opt(), eq(Optional::Unset(NestedEnum::Bar)));
    assert_that!(msg.bar(), matches_pattern!(BarBytes(eq(b"123"))));
    assert_that!(msg.bar_case(), eq(BarCase::BarBytes));

    msg.set_bar_enum(NestedEnum::Baz);
    assert_that!(msg.bar(), matches_pattern!(BarEnum(eq(NestedEnum::Baz))));
    assert_that!(msg.bar_case(), eq(BarCase::BarEnum));
    assert_that!(msg.bar_int_opt(), eq(Optional::Unset(5)));

    // TODO: Add tests covering a message-type field in a oneof.
}

#[test]
fn test_set_message_from_view() {
    use protobuf::MutProxy;

    let mut m1 = TestAllTypes::new();
    m1.set_optional_int32(1);
    let mut m2 = TestAllTypes::new();
    m2.as_mut().set(m1.as_view());

    assert_that!(m2.optional_int32(), eq(1i32));
}

#[test]
fn test_group() {
    let mut m = TestAllTypes::new();

    // Groups are exposed the same as nested message types.
    assert_that!(m.optionalgroup_opt().is_set(), eq(false));
    assert_that!(m.optionalgroup().a(), eq(0));

    m.optionalgroup_mut().set_a(7);
    assert_that!(m.optionalgroup_opt().is_set(), eq(true));
    assert_that!(m.optionalgroup().a(), eq(7));
}

#[test]
fn test_submsg_setter() {
    use test_all_types::*;

    let mut nested = NestedMessage::new();
    nested.set_bb(7);

    let mut parent = TestAllTypes::new();
    parent.set_optional_nested_message(nested);

    assert_that!(parent.optional_nested_message().bb(), eq(7));
}
