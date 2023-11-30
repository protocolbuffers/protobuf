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

    inner_msg.double_mut().set(543.21);
    assert_that!(inner_msg.double_mut().get(), eq(543.21));
    inner_msg.float_mut().set(1.23);
    assert_that!(inner_msg.float_mut().get(), eq(1.23));
    inner_msg.int32_mut().set(12);
    assert_that!(inner_msg.int32_mut().get(), eq(12));
    inner_msg.int64_mut().set(42);
    assert_that!(inner_msg.int64_mut().get(), eq(42));
    inner_msg.uint32_mut().set(13);
    assert_that!(inner_msg.uint32_mut().get(), eq(13));
    inner_msg.uint64_mut().set(5000);
    assert_that!(inner_msg.uint64_mut().get(), eq(5000));
    inner_msg.sint32_mut().set(-2);
    assert_that!(inner_msg.sint32_mut().get(), eq(-2));
    inner_msg.sint64_mut().set(322);
    assert_that!(inner_msg.sint64_mut().get(), eq(322));
    inner_msg.fixed32_mut().set(77);
    assert_that!(inner_msg.fixed32_mut().get(), eq(77));
    inner_msg.fixed64_mut().set(999);
    assert_that!(inner_msg.fixed64_mut().get(), eq(999));
    inner_msg.bool_mut().set(true);
    assert_that!(inner_msg.bool_mut().get(), eq(true));

    // TODO: add mutation tests for strings and bytes
}

#[test]
fn test_deeply_nested_definition() {
    let deep = nested_proto::nest::Outer_::Inner_::SuperInner_::DuperInner_::EvenMoreInner_
            ::CantBelieveItsSoInner::new();
    assert_eq!(deep.num(), 0);

    let outer_msg = Outer::new();
    assert_eq!(outer_msg.deep().num(), 0);
}
