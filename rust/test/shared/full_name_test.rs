// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use googletest::prelude::*;
use protobuf::MessageFullName;
use unittest_rust_proto::{test_all_types, NestedTestAllTypes, TestAllTypes};

#[gtest]
fn generated_messages_expose_full_name_via_concrete_syntax() {
    assert_that!(TestAllTypes::FULL_NAME, eq("rust_unittest.TestAllTypes"));
    assert_that!(
        NestedTestAllTypes::FULL_NAME,
        eq("rust_unittest.NestedTestAllTypes")
    );
    assert_that!(
        test_all_types::NestedMessage::FULL_NAME,
        eq("rust_unittest.TestAllTypes.NestedMessage")
    );
}

fn generic_message_full_name<T: MessageFullName>() -> &'static str {
    T::FULL_NAME
}

#[gtest]
fn generated_messages_expose_full_name_via_generic_trait() {
    assert_that!(
        generic_message_full_name::<TestAllTypes>(),
        eq("rust_unittest.TestAllTypes")
    );
    assert_that!(
        generic_message_full_name::<NestedTestAllTypes>(),
        eq("rust_unittest.NestedTestAllTypes")
    );
    assert_that!(
        generic_message_full_name::<test_all_types::NestedMessage>(),
        eq("rust_unittest.TestAllTypes.NestedMessage")
    );
}

#[gtest]
fn generated_messages_expose_full_name_via_ufcs() {
    assert_that!(
        <TestAllTypes as MessageFullName>::FULL_NAME,
        eq(TestAllTypes::FULL_NAME)
    );
    assert_that!(
        <NestedTestAllTypes as MessageFullName>::FULL_NAME,
        eq(NestedTestAllTypes::FULL_NAME)
    );
    assert_that!(
        <test_all_types::NestedMessage as MessageFullName>::FULL_NAME,
        eq(test_all_types::NestedMessage::FULL_NAME)
    );
}
