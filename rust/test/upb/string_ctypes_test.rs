// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use googletest::prelude::*;
use unittest_proto3_rust_proto::TestAllTypes;

#[gtest]
fn test_stringpiece_repeated() {
    let mut msg = TestAllTypes::new();
    assert_that!(msg.repeated_string_piece().len(), eq(0));
    msg.repeated_string_piece_mut().push("hello");
    assert_that!(msg.repeated_string_piece().len(), eq(1));
    assert_that!(msg.repeated_string_piece().get(0), some(eq("hello")));
}

#[gtest]
fn test_cord_repeated() {
    let mut msg = TestAllTypes::new();
    assert_that!(msg.repeated_cord().len(), eq(0));
    msg.repeated_cord_mut().push("hello");
    assert_that!(msg.repeated_cord().len(), eq(1));
    assert_that!(msg.repeated_cord().get(0), some(eq("hello")));
}
