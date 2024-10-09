// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use googletest::prelude::*;

// Tests that an proto file that declares edition="2023" works. Note that this
// is _not_ a test for Rust Edition 2023 (which doesn't exist) but instead
// Protobuf Edition 2023 (which exists).

#[gtest]
fn check_edition2023_works() {
    let msg = edition2023_rust_proto::EditionsMessage::new();
    assert_that!(msg.plain_field_opt().into_inner(), eq(0));
    assert_that!(msg.implicit_presence_field(), eq(0));
}

#[gtest]
fn string_view_works() {
    let mut msg = edition2023_rust_proto::EditionsMessage::new();
    assert_that!(msg.str_view(), eq(""));
    assert_that!(msg.has_str_view(), eq(false));
    msg.set_str_view("hello");
    assert_that!(msg.str_view(), eq("hello"));
    assert_that!(msg.has_str_view(), eq(true));
}

#[gtest]
fn repeated_string_view_works() {
    let mut msg = edition2023_rust_proto::EditionsMessage::new();
    assert_that!(msg.repeated_str_view().len(), eq(0));
    msg.repeated_str_view_mut().push("first");
    assert_that!(msg.repeated_str_view().len(), eq(1));
    assert_that!(msg.repeated_str_view().get(0), some(eq("first")));
}
