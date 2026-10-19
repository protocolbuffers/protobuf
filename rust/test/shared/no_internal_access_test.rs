// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use googletest::gtest;
use protobuf::__internal;

#[gtest]
#[allow(clippy::unit_cmp)]
fn test_no_internal_access() {
    // This test is to ensure that the `__internal` is 'blocked' by instead being a
    // unit type instead of a module.
    assert_eq!(__internal, ());
}
