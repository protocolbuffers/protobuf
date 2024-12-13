// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use googletest::prelude::*;

use unittest_rust_proto::{TestAllTypes, TestCord};

#[gtest]
fn test_bytes_cord() {
    let mut msg = TestCord::new();
    assert_that!(msg.has_optional_bytes_cord(), eq(false));
    assert_that!(msg.optional_bytes_cord(), eq("".as_bytes()));
    msg.set_optional_bytes_cord(b"hello");
    assert_that!(msg.has_optional_bytes_cord(), eq(true));
    assert_that!(msg.optional_bytes_cord(), eq("hello".as_bytes()));

    let mut msg2 = TestCord::new();
    msg2.set_optional_bytes_cord(msg.optional_bytes_cord());
    assert_that!(msg2.optional_bytes_cord(), eq("hello".as_bytes()));
}
