// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use descriptor_rust_proto::FileDescriptorProto;
use googletest::prelude::*;
use protobuf::prelude::*;

#[gtest]
fn test_generated_descriptors() {
    let descriptor = FileDescriptorProto::parse(
            unittest_rust_proto::__unstable::RUST_TEST_UNITTEST_DESCRIPTOR_INFO
            .descriptor,
    )
    .unwrap();
    expect_that!(descriptor.name().to_str().unwrap(), ends_with("rust/test/unittest.proto"));
    assert_that!(descriptor.dependency().len(), eq(1));
    expect_that!(
        descriptor.dependency().get(0).unwrap().to_str().unwrap(),
        ends_with("rust/test/unittest_import.proto")
    );

    assert_that!(
            unittest_rust_proto::__unstable::RUST_TEST_UNITTEST_DESCRIPTOR_INFO
            .deps
            .len(),
        eq(1)
    );
    let dep = FileDescriptorProto::parse(
            unittest_rust_proto::__unstable::RUST_TEST_UNITTEST_DESCRIPTOR_INFO
            .deps[0]
            .descriptor,
    )
    .unwrap();
    expect_that!(dep.name().to_str().unwrap(), ends_with("rust/test/unittest_import.proto"));
}
