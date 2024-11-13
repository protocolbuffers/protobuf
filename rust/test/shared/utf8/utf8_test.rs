// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

//! Tests demonstrating the Protobuf Rust behavior around UTF-8 under different
//! scenarios (proto2, proto3, editions).

// TODO: The behavior is currently subptimal (for example because
// b/333545903 or b/335140403). Design and implement desirable changes to this
// behavior. Do not assume that the Protobuf team is intentional about these
// behaviors while b/304774814 is open.

use googletest::prelude::*;
use protobuf::prelude::*;

use feature_verify_rust_proto::Verify;
use no_features_proto2_rust_proto::NoFeaturesProto2;
use no_features_proto3_rust_proto::NoFeaturesProto3;
use protobuf::{ParseError, ProtoStr};

// We use 0b1000_0000, since 0b1XXX_XXXX in UTF-8 denotes a byte 2-4, but never
// the first byte.
const NON_UTF8_BYTES: &[u8] = b"\x80";

// Returns ProtoStr with non-UTF-8 content.
fn make_non_utf8_proto_str() -> &'static ProtoStr {
    unsafe {
        // SAFETY: This is safe under current implementation of C++ and UPB kernels.
        // In the hypothethical pure Rust runtime this would be library-level UB - but
        // this test is specifically present to demonstrate UTF-8 behavior under
        // C++ and UPB kernels.
        ProtoStr::from_utf8_unchecked(NON_UTF8_BYTES)
    }
}

#[gtest]
fn test_proto2() {
    let non_utf8_str = make_non_utf8_proto_str();

    let mut msg = NoFeaturesProto2::new();

    // No error on setter
    msg.set_my_field(non_utf8_str);
    assert_that!(msg.my_field().as_bytes(), eq(NON_UTF8_BYTES));

    // No error on serialization
    let serialized_nonutf8 = msg.serialize().expect("serialization should not fail");

    // No error on parsing.
    let parsed_result = NoFeaturesProto2::parse(&serialized_nonutf8);
    assert_that!(parsed_result, ok(anything()));
}

#[gtest]
fn test_proto3() {
    let non_utf8_str = make_non_utf8_proto_str();

    let mut msg = NoFeaturesProto3::new();

    // No error on setter
    msg.set_my_field(non_utf8_str);
    assert_that!(msg.my_field().as_bytes(), eq(NON_UTF8_BYTES));

    // No error on serialization
    let serialized_nonutf8 = msg.serialize().expect("serialization should not fail");

    // Error on parsing.
    let parsed_result = NoFeaturesProto3::parse(&serialized_nonutf8);
    assert_that!(parsed_result, err(matches_pattern!(&ParseError)));
}

#[gtest]
fn test_verify() {
    let non_utf8_str = make_non_utf8_proto_str();

    let mut msg = Verify::new();

    // No error on setter
    msg.set_my_field(non_utf8_str);
    assert_that!(msg.my_field().as_bytes(), eq(NON_UTF8_BYTES));

    // No error on serialization
    let serialized_nonutf8 = msg.serialize().expect("serialization should not fail");

    // Error on parsing.
    let parsed_result = Verify::parse(&serialized_nonutf8);
    assert_that!(parsed_result, err(matches_pattern!(&ParseError)));
}
