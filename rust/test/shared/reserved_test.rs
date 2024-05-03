// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use googletest::prelude::*;
/// Test covering proto compilation with reserved words.
use reserved_proto::naming::Reserved;

#[test]
fn test_reserved_keyword_in_accessors() {
    let msg = Reserved::new();
    let res = msg.r#for();
    assert_that!(res, eq(0));
}

#[test]
fn test_reserved_keyword_in_messages() {
    let msg = Reserved::new();
    let _ = msg.r#pub();
}
