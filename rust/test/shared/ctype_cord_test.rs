// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use googletest::prelude::*;

use unittest_rust_proto::{TestAllTypes, TestCord};

#[gtest]
fn test_string_cord() {
    let mut msg = TestAllTypes::new();
    assert_that!(msg.has_optional_cord(), eq(false));
    assert_that!(msg.optional_cord(), eq(""));
    msg.set_optional_cord("hello");
    assert_that!(msg.has_optional_cord(), eq(true));
    assert_that!(msg.optional_cord(), eq("hello"));

    let mut msg2 = TestAllTypes::new();
    msg2.set_optional_cord(msg.optional_cord());
    assert_that!(msg2.optional_cord(), eq("hello"));
}

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
