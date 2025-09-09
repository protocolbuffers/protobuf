// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use googletest::prelude::*;
use nested_rust_proto::outer::inner::InnerEnum;
use nested_rust_proto::outer::InnerView;
use nested_rust_proto::*;

#[gtest]
fn test_deeply_nested_message() {
    let deep =
        outer::inner::super_inner::duper_inner::even_more_inner::CantBelieveItsSoInner::new();
    assert_that!(deep.num(), eq(0));

    let outermsg = Outer::new();
    assert_that!(outermsg.deep().num(), eq(0));
}

#[gtest]
fn test_deeply_nested_enum() {
    use outer::inner::super_inner::duper_inner::even_more_inner::JustWayTooInner;
    let deep = JustWayTooInner::default();
    assert_that!(i32::from(deep), eq(0));

    let outermsg = Outer::new();
    assert_that!(outermsg.deep_enum(), eq(JustWayTooInner::Unspecified));
}

#[gtest]
fn test_nested_views() {
    let outermsg = Outer::new();
    let inner_msg: InnerView<'_> = outermsg.inner();
    assert_that!(inner_msg.double(), eq(0.0));
    assert_that!(inner_msg.float(), eq(0.0));
    assert_that!(inner_msg.int32(), eq(0));
    assert_that!(inner_msg.int64(), eq(0));
    assert_that!(inner_msg.uint32(), eq(0));
    assert_that!(inner_msg.uint64(), eq(0));
    assert_that!(inner_msg.sint32(), eq(0));
    assert_that!(inner_msg.sint64(), eq(0));
    assert_that!(inner_msg.fixed32(), eq(0));
    assert_that!(inner_msg.fixed64(), eq(0));
    assert_that!(inner_msg.sfixed32(), eq(0));
    assert_that!(inner_msg.sfixed64(), eq(0));
    assert_that!(inner_msg.bool(), eq(false));
    assert_that!(*inner_msg.string().as_bytes(), empty());
    assert_that!(*inner_msg.bytes(), empty());
    assert_that!(inner_msg.inner_submsg().flag(), eq(false));
    assert_that!(inner_msg.inner_enum(), eq(InnerEnum::Unspecified));
}

#[gtest]
fn test_nested_view_lifetimes() {
    // Ensure that views have the lifetime of the first layer of borrow, and don't
    // create intermediate borrows through nested accessors.

    let outermsg = Outer::new();

    let string = outermsg.inner().string();
    assert_that!(string, eq(""));

    let bytes = outermsg.inner().bytes();
    assert_that!(bytes, eq(b""));

    let inner_submsg = outermsg.inner().inner_submsg();
    assert_that!(inner_submsg.flag(), eq(false));

    let repeated_int32 = outermsg.inner().repeated_int32();
    assert_that!(repeated_int32, empty());

    let repeated_inner_submsg = outermsg.inner().repeated_inner_submsg();
    assert_that!(repeated_inner_submsg, empty());

    let string_map = outermsg.inner().string_map();
    assert_that!(string_map.len(), eq(0));
}

#[gtest]
fn test_msg_from_outside() {
    // let's make sure that we're not just working for messages nested inside
    // messages, messages from without and within should work
    let outer = Outer::new();
    assert_that!(outer.notinside().num(), eq(0));
}

#[gtest]
fn test_recursive_view() {
    let rec = nested_rust_proto::Recursive::new();
    assert_that!(rec.num(), eq(0));
    assert_that!(rec.rec().num(), eq(0));
    assert_that!(rec.rec().rec().num(), eq(0)); // turtles all the way down...
    assert_that!(rec.rec().rec().rec().num(), eq(0)); // ... ad infinitum

    // Test that intermediate borrows are not created.
    let nested = rec.rec().rec().rec();
    assert_that!(nested.num(), eq(0));
}

#[gtest]
fn test_recursive_mut() {
    let mut rec = nested_rust_proto::Recursive::new();
    let mut one = rec.rec_mut();
    let mut two = one.rec_mut();
    let mut three = two.rec_mut();
    let mut four = three.rec_mut();

    four.set_num(1);
    assert_that!(four.num(), eq(1));

    assert_that!(rec.num(), eq(0));
    assert_that!(rec.rec().rec().num(), eq(0));
    assert_that!(rec.rec().rec().rec().rec().num(), eq(1));

    // This fails since `RecursiveMut` has `&mut self` methods.
    // See b/314989133.
    // let nested = rec.rec_mut().rec_mut().rec_mut();
    // assert_that!(nested.num(), eq(0));
}
