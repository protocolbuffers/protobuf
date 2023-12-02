// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use googletest::prelude::*;
use nested_proto::nest::Outer;
use nested_proto::nest::Outer_::InnerMut;
use nested_proto::nest::Outer_::InnerView;

#[test]
fn test_deeply_nested_definition() {
    let deep = nested_proto::nest::Outer_::Inner_::SuperInner_::DuperInner_::EvenMoreInner_
            ::CantBelieveItsSoInner::new();
    assert_that!(deep.num(), eq(0));

    let outer_msg = Outer::new();
    assert_that!(outer_msg.deep().num(), eq(0));
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
    ( $a:expr => $($target_mut:ident, $val:literal;)* ) => {
        $(
          $a.$target_mut().set($val);
          assert_that!($a.$target_mut().get(), eq($val));
        )*
    };
}

    let mut outer_msg = Outer::new();
    let inner_msg: InnerMut<'_> = outer_msg.inner_mut();
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
            bool(): eq(false)
        })
    );

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

    );
    // TODO: add mutation tests for strings and bytes
}
