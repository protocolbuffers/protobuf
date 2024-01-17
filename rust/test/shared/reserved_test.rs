// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

/// Test covering proto compilation with reserved words.
use googletest::prelude::*;
use reserved_proto::r#type::r#type::r#enum;
use reserved_proto::r#type::r#type::Self__mangled_because_symbol_is_a_rust_raw_identifier;

#[test]
fn test_reserved_keyword_in_accessors() {
    let msg = Self__mangled_because_symbol_is_a_rust_raw_identifier::new();
    let res = msg.self__mangled_because_symbol_is_a_rust_raw_identifier().r#for();
    assert_that!(res, eq(0));
}

#[test]
fn test_reserved_keyword_in_messages() {
    let msg = r#enum::new();
    let _ = msg.r#const();
}
