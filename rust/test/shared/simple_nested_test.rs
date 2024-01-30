// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use googletest::prelude::*;
use nested_proto::Outer_::InnerMut;
use nested_proto::Outer_::InnerView;
use nested_proto::Outer_::Inner_::InnerEnum;
use nested_proto::*;
use protobuf::Optional;

#[test]
fn test_deeply_nested_message() {
    let deep =
        Outer_::Inner_::SuperInner_::DuperInner_::EvenMoreInner_::CantBelieveItsSoInner::new();
    assert_that!(deep.num(), eq(0));

    let outer_msg = Outer::new();
    assert_that!(outer_msg.deep().num(), eq(0));
}

#[test]
fn test_deeply_nested_enum() {
    use Outer_::Inner_::SuperInner_::DuperInner_::EvenMoreInner_::JustWayTooInner;
    let deep = JustWayTooInner::default();
    assert_that!(i32::from(deep), eq(0));

    let outer_msg = Outer::new();
    assert_that!(outer_msg.deep_enum(), eq(JustWayTooInner::Unspecified));
}

#[test]
fn test_nested_views() {
    let outer_msg = Outer::new();
    let inner_msg: InnerView<'_> = outer_msg.inner();
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

#[test]
fn test_nested_view_lifetimes() {
    // Ensure that views have the lifetime of the first layer of borrow, and don't
    // create intermediate borrows through nested accessors.

    let outer_msg = Outer::new();

    let string = outer_msg.inner().string();
    assert_that!(string, eq(""));

    let bytes = outer_msg.inner().bytes();
    assert_that!(bytes, eq(b""));

    let inner_submsg = outer_msg.inner().inner_submsg();
    assert_that!(inner_submsg.flag(), eq(false));

    let repeated_int32 = outer_msg.inner().repeated_int32();
    assert_that!(repeated_int32, empty());

    let repeated_inner_submsg = outer_msg.inner().repeated_inner_submsg();
    assert_that!(repeated_inner_submsg, empty());

    let string_map = outer_msg.inner().string_map();
    assert_that!(string_map.len(), eq(0));
}

#[test]
fn test_nested_muts() {
    // Covers the setting of a mut and the following assertion
    // confirming the new value. Replacement example:
    // old:
    //   inner_msg.double_mut().set(543.21);
    //   assert_that!(inner_msg.double_mut().get(), eq(543.21));
    // new:
    //   set_and_test_mut!(inner_msg => double_mut, 543.21);
    macro_rules! set_and_test_mut {
        ( $a:expr => $($target_mut:ident, $val:expr;)* ) => {
            $(
            $a.$target_mut().set($val);
            assert_that!($a.$target_mut().get(), eq($val));
            )*
        };
    }

    let mut outer_msg = Outer::new();
    let mut inner_msg: InnerMut<'_> = outer_msg.inner_mut().or_default();
    assert_that!(
        inner_msg,
        matches_pattern!(InnerMut{
            float(): eq(0.0),
            double(): eq(0.0),
            int32(): eq(0),
            int64(): eq(0),
            uint32(): eq(0),
            uint64(): eq(0),
            sint32(): eq(0),
            sint64(): eq(0),
            fixed32(): eq(0),
            fixed64(): eq(0),
            sfixed32(): eq(0),
            sfixed64(): eq(0),
            bool(): eq(false),
            inner_enum(): eq(InnerEnum::Unspecified)
        })
    );
    assert_that!(inner_msg.string_mut().get(), eq(""));
    assert_that!(inner_msg.bytes_mut().get(), eq(b""));

    set_and_test_mut!(inner_msg =>
        double_mut, 543.21;
        float_mut, 1.23;
        int32_mut, 12;
        int64_mut, 42;
        uint32_mut, 13;
        uint64_mut, 5000;
        sint32_mut, -2;
        sint64_mut, 322;
        fixed32_mut, 77;
        fixed64_mut, 999;
        bool_mut, true;
        string_mut, "hi";
        bytes_mut, b"bye";
        inner_enum_mut, InnerEnum::Foo;
    );
}

#[test]
fn test_msg_from_outside() {
    // let's make sure that we're not just working for messages nested inside
    // messages, messages from without and within should work
    let outer = Outer::new();
    assert_that!(outer.notinside().num(), eq(0));
}

#[test]
fn test_recursive_view() {
    let rec = nested_proto::Recursive::new();
    assert_that!(rec.num(), eq(0));
    assert_that!(rec.rec().num(), eq(0));
    assert_that!(rec.rec().rec().num(), eq(0)); // turtles all the way down...
    assert_that!(rec.rec().rec().rec().num(), eq(0)); // ... ad infinitum

    // Test that intermediate borrows are not created.
    let nested = rec.rec().rec().rec();
    assert_that!(nested.num(), eq(0));
}

#[test]
fn test_recursive_mut() {
    let mut rec = nested_proto::Recursive::new();
    let mut one = rec.rec_mut().or_default();
    let mut two = one.rec_mut().or_default();
    let mut three = two.rec_mut().or_default();
    let mut four = three.rec_mut().or_default();

    four.num_mut().set(1);
    assert_that!(four.num(), eq(1));

    assert_that!(rec.num(), eq(0));
    assert_that!(rec.rec().rec().num(), eq(0));
    assert_that!(rec.rec().rec().rec().rec().num(), eq(1));

    // This fails since `RecursiveMut` has `&mut self` methods.
    // See b/314989133.
    // let nested = rec.rec_mut().rec_mut().rec_mut();
    // assert_that!(nested.num(), eq(0));
}
