// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use googletest::prelude::*;
use nested_proto::nest::Outer;

#[test]
fn test_simple_nested_proto() {
    let outer_msg = Outer::new();
    assert_that!(outer_msg.inner().num(), eq(0));
    assert_that!(outer_msg.inner().boolean(), eq(false));
}

#[test]
fn test_message_mutation() {
    // TODO: add actual mutation logic, this just peeks at inner_mut at
    // the moment
    let mut outer_msg = Outer::new();
    assert_that!(outer_msg.inner().num(), eq(0));
    assert_that!(outer_msg.inner_mut().num(), eq(0));
}
