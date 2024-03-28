// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use googletest::prelude::*;
use unittest_proto::TestAllTypes;

#[test]
fn serialize_zero_length() {
    let mut msg = TestAllTypes::new();

    let serialized = msg.serialize();
    assert_that!(serialized.len(), eq(0));

    let serialized = msg.as_view().serialize();
    assert_that!(serialized.len(), eq(0));

    let serialized = msg.as_mut().serialize();
    assert_that!(serialized.len(), eq(0));
}

#[test]
fn serialize_deserialize_message() {
    let mut msg = TestAllTypes::new();
    msg.set_optional_int64(42);
    msg.set_optional_bool(true);
    msg.set_optional_bytes(b"serialize deserialize test");

    let serialized = msg.serialize();

    let msg2 = TestAllTypes::parse(&serialized).unwrap();
    assert_that!(msg.optional_int64(), eq(msg2.optional_int64()));
    assert_that!(msg.optional_bool(), eq(msg2.optional_bool()));
    assert_that!(msg.optional_bytes(), eq(msg2.optional_bytes()));
}

#[test]
fn deserialize_empty() {
    assert!(TestAllTypes::parse(&[]).is_ok());
}

#[test]
fn deserialize_error() {
    assert!(TestAllTypes::parse(b"not a serialized proto").is_err());
}

#[test]
fn set_bytes_with_serialized_data() {
    let mut msg = TestAllTypes::new();
    msg.set_optional_int64(42);
    msg.set_optional_bool(true);
    let mut msg2 = TestAllTypes::new();
    msg2.set_optional_bytes(msg.serialize());
    assert_that!(msg2.optional_bytes(), eq(msg.serialize().as_ref()));
}

#[test]
fn deserialize_on_previously_allocated_message() {
    let mut msg = TestAllTypes::new();
    msg.set_optional_int64(42);
    msg.set_optional_bool(true);
    msg.set_optional_bytes(b"serialize deserialize test");

    let serialized = msg.serialize();

    let mut msg2 = Box::new(TestAllTypes::new());
    assert!(msg2.clear_and_parse(&serialized).is_ok());
    assert_that!(msg.optional_int64(), eq(msg2.optional_int64()));
    assert_that!(msg.optional_bool(), eq(msg2.optional_bool()));
    assert_that!(msg.optional_bytes(), eq(msg2.optional_bytes()));
}
