// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

//! Tests covering nested types.

use googletest::prelude::*;

#[gtest]
fn test_nested_messages_accessible() {
    let _parent: unittest_rust_proto::TestAllTypes;
    let _child: unittest_rust_proto::test_all_types::NestedMessage;
    unittest_rust_proto::test_child_extension_data::
    nested_test_all_extensions_data::NestedDynamicExtensions::new();
}

#[gtest]
fn test_nested_enums_accessible() {
    let _parent: unittest_rust_proto::TestAllTypes;
    let _child: unittest_rust_proto::test_all_types::NestedEnum;
    unittest_rust_proto::test_dynamic_extensions::DynamicEnumType::default();
}
