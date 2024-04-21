// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

/// Test covering proto compilation with reserved words.
use googletest::prelude::*;
use reserved_proto::Self__mangled_because_ident_isnt_a_legal_raw_identifier;
use reserved_proto::{r#enum, Ref};

#[test]
fn test_reserved_keyword_in_accessors() {
    let msg = Self__mangled_because_ident_isnt_a_legal_raw_identifier::new();
    let res = msg.self__mangled_because_ident_isnt_a_legal_raw_identifier().r#for();
    assert_that!(res, eq(0));
}

#[test]
fn test_reserved_keyword_in_messages() {
    let _ = r#enum::new();
    let _ = Ref::new().r#const();
}
