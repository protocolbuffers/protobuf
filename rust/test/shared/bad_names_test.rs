// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#[cfg(not(bzl))]
mod protos;
#[cfg(not(bzl))]
use protos::*;

use bad_names_rust_proto::*;
use googletest::prelude::*;
use protobuf::proto;

#[gtest]
fn test_reserved_keyword_in_accessors() {
    let mut msg = Self_::new();
    let res = msg.self_().r#for();
    assert_that!(res, eq(0));
    msg.set_new(true);
    assert_that!(msg.new_(), eq(true));
}

#[gtest]
fn test_reserved_keyword_in_messages() {
    let _ = r#enum::new();
    let _ = Ref::new().r#const();
}

#[gtest]
fn test_reserved_keyword_with_proto_macro() {
    let _ = proto!(Self_ { r#true: false, r#match: [0i32] });
}

#[gtest]
fn test_collision_in_accessors() {
    let mut m = AccessorsCollide::new();
    m.set_x_mut_5(false);
    assert_that!(m.x_mut_5(), eq(false));
    assert_that!(m.has_x_mut_5(), eq(true));
    assert_that!(m.has_set_x_2(), eq(false));

    assert_that!(m.has_x(), eq(false));
    m.x_mut();
    m.clear_x_7();
    assert_that!(m.has_x(), eq(true));
    assert_that!(m.has_clear_x_7(), eq(false));
}

#[gtest]
fn test_mangled_names() {
    let _ = MangleViewTest::new();
    let _ = MangleViewTestView_::new();
    let _ = SomeMsg::new();
    let _ = SomeMsgView_::Unspecified;
}

#[gtest]
fn test_msg_with_default_field() {
    let mut parent = ParentOfMsgWithDefaultField::new();

    expect_that!(parent.has_child(), eq(false));
    expect_that!(parent.child().default(), eq(42));
    expect_that!(parent.as_mut().child().default(), eq(42));
    expect_that!(parent.as_view().child().default(), eq(42));
    expect_that!(parent.default().default(), eq(42));
    expect_that!(parent.oneof_child().default(), eq(42));

    parent.child_mut().set_default(7);
    expect_that!(parent.has_child(), eq(true));
    expect_that!(parent.child().default(), eq(7));
    expect_that!(parent.as_mut().child().default(), eq(7));
    expect_that!(parent.as_view().child().default(), eq(7));

    parent.set_default(proto!(MsgWithDefaultField { default: 11 }));
    expect_that!(parent.default().default(), eq(11));

    parent.set_oneof_child(proto!(MsgWithDefaultField { default: 19 }));
    expect_that!(parent.oneof_child().default(), eq(19));

    expect_that!(parent.repeated_child_mut().push_default().default(), eq(42));
    parent.map_child_mut().insert(1, proto!(MsgWithDefaultField { default: 23 }));
    expect_that!(
        parent.map_child().get(1).expect("key 1 should be present in map_child").default(),
        eq(23)
    );
}
