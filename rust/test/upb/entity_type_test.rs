// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use googletest::prelude::*;
use protobuf::__internal::EntityType;
use unittest_rust_proto::test_all_types::NestedEnum;
use unittest_rust_proto::{TestAllTypes, TestAllTypesMut, TestAllTypesView};

#[gtest]
fn test_type_tags() {
    assert_that!(
        std::any::type_name::<<TestAllTypes as EntityType>::Tag>(),
        eq("protobuf_upb::codegen_traits::entity_tag::MessageTag")
    );
    assert_that!(
        std::any::type_name::<<TestAllTypesView as EntityType>::Tag>(),
        eq("protobuf_upb::codegen_traits::entity_tag::ViewProxyTag")
    );
    assert_that!(
        std::any::type_name::<<TestAllTypesMut as EntityType>::Tag>(),
        eq("protobuf_upb::codegen_traits::entity_tag::MutProxyTag")
    );
    assert_that!(
        std::any::type_name::<<NestedEnum as EntityType>::Tag>(),
        eq("protobuf_upb::codegen_traits::entity_tag::EnumTag")
    );
    assert_that!(
        std::any::type_name::<<i32 as EntityType>::Tag>(),
        eq("protobuf_upb::codegen_traits::entity_tag::PrimitiveTag")
    );
}
