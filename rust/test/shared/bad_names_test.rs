// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use bad_names_rust_proto::*;
use googletest::prelude::*;
use protobuf::proto;

#[gtest]
fn test_reserved_keyword_in_accessors() {
    let msg = Self__mangled_because_ident_isnt_a_legal_raw_identifier::new();
    let res = msg.self__mangled_because_ident_isnt_a_legal_raw_identifier().r#for();
    assert_that!(res, eq(0));
}

#[gtest]
fn test_reserved_keyword_in_messages() {
    let _ = r#enum::new();
    let _ = Ref::new().r#const();
}

#[gtest]
fn test_reserved_keyword_with_proto_macro() {
    let _ = proto!(Self__mangled_because_ident_isnt_a_legal_raw_identifier {
        r#true: false,
        r#match: [0i32],
    });
}

#[gtest]
fn test_collision_in_accessors() {
    let mut m = AccessorsCollide::new();
    m.set_x_mut_5(false);
    assert_that!(m.x_mut_5(), eq(false));
    assert_that!(m.has_x_mut_5(), eq(true));
    assert_that!(m.has_x(), eq(false));
    assert_that!(m.has_set_x_2(), eq(false));
}
