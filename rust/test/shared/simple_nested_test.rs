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
    // TODO: add actual mutation logic, this just peeks at InnerMut at
    // the moment
    let mut outer_msg = Outer::new();
    let inner_msg: InnerMut<'_> = outer_msg.inner_mut();
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
fn test_deeply_nested_definition() {
    let deep = nested_proto::nest::Outer_::Inner_::SuperInner_::DuperInner_::EvenMoreInner_
            ::CantBelieveItsSoInner::new();
    assert_eq!(deep.num(), 0);

    let outer_msg = Outer::new();
    assert_eq!(outer_msg.deep().num(), 0);
}
