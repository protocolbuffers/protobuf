// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use googletest::prelude::*;
use protobuf::prelude::*;

#[gtest]
fn test_canonical_types() {
    let _child = child_rust_proto::Child::new();
    let _parent = parent_rust_proto::Parent::new();
    // Parent from child_rust_proto crate should be the same type as Parent from
    // parent_rust_proto crate.
    let _parent_from_child: child_rust_proto::Parent = parent_rust_proto::Parent::new();
}

#[gtest]
fn test_parent_serialization() {
    assert_that!(*parent_rust_proto::Parent::new().serialize().unwrap(), empty());
}

#[gtest]
fn test_child_serialization() {
    assert_that!(*child_rust_proto::Child::new().serialize().unwrap(), empty());
}
