// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#[cfg(not(bzl))]
mod protos;
#[cfg(not(bzl))]
use protos::*;

use googletest::prelude::*;
use protobuf::prelude::*;
use protobuf::{SerializeOptions, View};

use map_unittest_rust_proto::{TestMap, TestMapSubmessage};
use paste::paste;
use unittest_proto3_optional_rust_proto::TestProto3Optional;
use unittest_proto3_rust_proto::TestAllTypes as TestAllTypesProto3;
use unittest_rust_proto::{TestAllTypes, TestRequired};

macro_rules! generate_parameterized_serialization_test {
    ($(($type: ident, $name_ext: ident)),*) => {
        paste! { $(
            #[gtest]
            fn [< serialization_zero_length_ $name_ext >]() {
                let mut msg = [< $type >]::new();

                let serialized = msg.serialize().unwrap();
                assert_that!(serialized.len(), eq(0));

                let serialized = msg.as_view().serialize().unwrap();
                assert_that!(serialized.len(), eq(0));

                let serialized = msg.as_mut().serialize().unwrap();
                assert_that!(serialized.len(), eq(0));
            }

            #[gtest]
            fn [< deterministic_serialization_zero_length_ $name_ext >]() {
                let mut msg = [< $type >]::new();
                let options = SerializeOptions::new().deterministic(true);

                let serialized = msg.serialize_with_options(options).unwrap();
                expect_that!(serialized.len(), eq(0));

                let serialized = msg.as_view().serialize_with_options(options).unwrap();
                expect_that!(serialized.len(), eq(0));

                let serialized = msg.as_mut().serialize_with_options(options).unwrap();
                expect_that!(serialized.len(), eq(0));
            }

            #[gtest]
            fn [< serialize_with_default_options_matches_serialize_ $name_ext >]() {
                let mut msg = [< $type >]::new();
                msg.set_optional_int64(42);
                msg.set_optional_bool(true);
                msg.set_optional_bytes(b"serialize deserialize test");

                let serialized = msg.serialize().unwrap();
                expect_that!(
                    msg.serialize_with_options(SerializeOptions::default()).unwrap(),
                    eq(&serialized)
                );
            }

            #[gtest]
            fn [< serialized_len_matches_serialize_ $name_ext >]() {
                let mut msg = [< $type >]::new();
                msg.set_optional_int64(42);
                msg.set_optional_bool(true);
                msg.set_optional_bytes(b"serialize deserialize test");

                let serialized = msg.serialize().unwrap();
                assert_that!(msg.serialized_len(), eq(serialized.len()));

                let serialized = msg.as_view().serialize().unwrap();
                assert_that!(msg.as_view().serialized_len(), eq(serialized.len()));

                let serialized = msg.as_mut().serialize().unwrap();
                assert_that!(msg.as_mut().serialized_len(), eq(serialized.len()));
            }

            #[gtest]
            fn [< serialized_len_empty_message_ $name_ext >]() {
                let msg = [< $type >]::new();
                assert_that!(msg.serialized_len(), eq(0));
            }

            #[gtest]
            fn [< serialize_default_view $name_ext>]() {
                let default = View::<[< $type >]>::default();
                assert_that!(default.serialize().unwrap().len(), eq(0));
            }

            #[gtest]
            fn [< serialize_deserialize_message_ $name_ext>]() {
                let mut msg = [< $type >]::new();
                msg.set_optional_int64(42);
                msg.set_optional_bool(true);
                msg.set_optional_bytes(b"serialize deserialize test");

                let serialized = msg.serialize().unwrap();

                let msg2 = [< $type >]::parse(&serialized).unwrap();
                assert_that!(msg.optional_int64(), eq(msg2.optional_int64()));
                assert_that!(msg.optional_bool(), eq(msg2.optional_bool()));
                assert_that!(msg.optional_bytes(), eq(msg2.optional_bytes()));
            }

            #[gtest]
            fn [< deserialize_empty_ $name_ext>]() {
                assert!([< $type >]::parse(&[]).is_ok());
            }

            #[gtest]
            fn [< deserialize_error_ $name_ext>]() {
                assert!([< $type >]::parse(b"not a serialized proto").is_err());
            }

            #[gtest]
            fn [< set_bytes_with_serialized_data_ $name_ext>]() {
                let mut msg = [< $type >]::new();
                msg.set_optional_int64(42);
                msg.set_optional_bool(true);
                let mut msg2 = [< $type >]::new();
                msg2.set_optional_bytes(msg.serialize().unwrap());
                assert_that!(msg2.optional_bytes(), eq(msg.serialize().unwrap()));
            }

            #[gtest]
            fn [< deserialize_on_previously_allocated_message_ $name_ext>]() {
                let mut msg = [< $type >]::new();
                msg.set_optional_int64(42);
                msg.set_optional_bytes(b"serialize deserialize test");

                let serialized = msg.serialize().unwrap();

                let mut msg2 = Box::new([< $type >]::new());
                msg2.set_optional_bool(true);

                assert!(msg2.clear_and_parse(&serialized).is_ok());
                assert_that!(msg2.optional_int64(), eq(msg.optional_int64()));
                assert_that!(msg2.optional_bytes(), eq(msg.optional_bytes()));
                assert_that!(msg2.optional_bool(), eq(false));
            }

            #[gtest]
            fn [< deserialize_on_previously_allocated_message_mut_ $name_ext>]() {
                let mut msg = [< $type >]::new();
                msg.set_optional_int64(42);
                msg.set_optional_bytes(b"serialize deserialize test");

                let serialized = msg.serialize().unwrap();

                let mut msg2 = Box::new([< $type >]::new());
                let msg2_mut = msg2.as_mut();
                msg2_mut.set_optional_bool(true);

                assert!(msg2_mut.clear_and_parse(&serialized).is_ok());
                assert_that!(msg2.optional_int64(), eq(msg.optional_int64()));
                assert_that!(msg2.optional_bytes(), eq(msg.optional_bytes()));
                assert_that!(msg2.optional_bool(), eq(false));
            }

        )* }
    };
  }

generate_parameterized_serialization_test!(
    (TestAllTypes, editions),
    (TestAllTypesProto3, proto3),
    (TestProto3Optional, proto3_optional)
);

macro_rules! generate_parameterized_int32_byte_size_test {
    ($(($type: ident, $name_ext: ident)),*) => {
        paste! { $(

            #[gtest]
            fn [< test_int32_byte_size_ $name_ext>]() {
                let args = vec![(0, 1), (127, 1), (128, 2), (-1, 10)];
                for arg in args {
                    let value = arg.0;
                    let expected_value_size = arg.1;
                    let mut msg = [< $type >]::new();
                    // tag for optional_int32 only takes 1 byte
                    msg.set_optional_int32(value);
                    let serialized = msg.serialize().unwrap();
                    // 1 byte for tag and n from expected_value_size
                    assert_that!(serialized.len(), eq(expected_value_size + 1), "Test failed. Value: {value}. Expected_value_size: {expected_value_size}.");
                }

            }
        )* }
    };
  }

generate_parameterized_int32_byte_size_test!(
    (TestAllTypes, editions),
    (TestProto3Optional, proto3_optional) /* Test would fail if we were to use
                                           * TestAllTypesProto3: optional_int32 follows "no
                                           * presence" semantics and setting it to 0 (default
                                           * value) will cause it to not be serialized */
);

#[gtest]
fn test_required_field_enforced() {
    // Empty bytes slice is a valid binaryproto with no fields set -- therefore it should not parse
    // as a message with required fields.
    expect_that!(TestRequired::parse(&[]), err(anything()));

    let mut msg = TestRequired::new();
    expect_that!(msg.clear_and_parse(&[]), err(anything()));
}

#[gtest]
fn test_required_field_not_enforced() {
    // Empty bytes slice is a valid binaryproto with no fields set.
    let mut msg = TestRequired::parse_dont_enforce_required(&[]).unwrap();
    expect_that!(msg.has_a(), eq(false));

    msg.set_a(1);
    msg.clear_and_parse_dont_enforce_required(&[]).unwrap();
    expect_that!(msg.has_a(), eq(false));
}

#[gtest]
fn test_clear_and_parse_clears_on_error() {
    let mut msg = TestAllTypes::new();
    msg.set_optional_int32(42);
    // Malformed wire format (tag 1 with incomplete varint).
    let malformed_data = [0x08, 0x80];
    expect_that!(msg.clear_and_parse(&malformed_data), err(anything()));
    expect_that!(msg.has_optional_int32(), eq(false));
    expect_that!(msg.optional_int32(), eq(0));
}

#[gtest]
fn test_clear_and_parse_clears_on_error_oneof_partial() {
    let mut msg = TestAllTypesProto3::new();
    msg.set_oneof_uint32(7);

    // Payload containing a valid oneof_bytes followed by an invalid UTF-8 oneof_string.
    // field 114 (oneof_bytes):
    //   tag = (114 << 3) | 2 = 914 = [0x92, 0x07], len = 4, data = [1, 2, 3, 4]
    // field 113 (oneof_string):
    //   tag = (113 << 3) | 2 = 906 = [0x8a, 0x07], len = 1, data = [0x80] (invalid UTF-8)
    let payload = [0x92, 0x07, 0x04, 0x01, 0x02, 0x03, 0x04, 0x8a, 0x07, 0x01, 0x80];
    expect_that!(msg.clear_and_parse(&payload), err(anything()));
    expect_that!(msg.oneof_uint32_opt(), eq(None));
    expect_that!(msg.oneof_bytes_opt(), eq(None));
    expect_that!(msg.oneof_string_opt(), eq(None));
}

/// Returns a `TestMap` whose map fields are populated by inserting `keys` in the given order.
fn test_map_with_keys(keys: impl IntoIterator<Item = i32>) -> TestMap {
    let mut msg = TestMap::new();
    for key in keys {
        msg.map_int32_int32_mut().insert(key, key * 10);
        msg.map_string_string_mut()
            .insert(format!("key{key}").as_str(), format!("value{key}").as_str());
        let mut value = TestAllTypes::new();
        value.set_optional_int32(key);
        msg.map_int32_all_types_mut().insert(key, value);
    }
    msg
}

#[gtest]
fn deterministic_serialization_ignores_map_insertion_order() {
    let options = SerializeOptions::new().deterministic(true);
    let ascending = test_map_with_keys(-16..16);
    let mut descending = test_map_with_keys((-16..16).rev());

    let expected = ascending.serialize_with_options(options).unwrap();
    expect_that!(descending.serialize_with_options(options).unwrap(), eq(&expected));
    expect_that!(descending.as_view().serialize_with_options(options).unwrap(), eq(&expected));
    expect_that!(descending.as_mut().serialize_with_options(options).unwrap(), eq(&expected));

    // The output is regular wire format that parses back to the same contents.
    let parsed = TestMap::parse(&expected).unwrap();
    expect_that!(parsed.map_int32_int32().len(), eq(32));
    expect_that!(parsed.map_string_string().len(), eq(32));
    expect_that!(parsed.map_int32_all_types().len(), eq(32));
    expect_that!(parsed.serialize_with_options(options).unwrap(), eq(&expected));
}

#[gtest]
fn deterministic_serialization_applies_to_nested_maps() {
    let options = SerializeOptions::new().deterministic(true);
    let mut ascending = TestMapSubmessage::new();
    ascending.set_test_map(test_map_with_keys(-16..16));
    let mut descending = TestMapSubmessage::new();
    descending.set_test_map(test_map_with_keys((-16..16).rev()));

    expect_that!(
        descending.serialize_with_options(options).unwrap(),
        eq(&ascending.serialize_with_options(options).unwrap())
    );
}

#[gtest]
fn test_clear_and_parse_owned_and_get_retained() {
    let mut src = TestAllTypes::new();
    src.set_optional_int64(99);
    src.set_optional_bytes(b"zero-copy aliased payload bytes");
    src.set_optional_string("zero-copy aliased string");
    let serialized: Vec<u8> = src.serialize().unwrap();

    let mut msg = TestAllTypes::new();
    msg.clear_and_parse_owned(serialized.clone()).unwrap();
    expect_that!(msg.optional_int64(), eq(99));
    expect_that!(msg.optional_bytes(), eq(b"zero-copy aliased payload bytes"));
    expect_that!(msg.optional_string(), eq("zero-copy aliased string"));

    if let Some(retained) = msg.get_retained::<Vec<u8>>() {
        expect_that!(retained, eq(&serialized));
        let buf_range = retained.as_ptr_range();
        expect_that!(buf_range.contains(&msg.optional_bytes().as_ptr()), eq(true));
        expect_that!(buf_range.contains(&msg.optional_string().as_bytes().as_ptr()), eq(true));
    }

    // Subsequent non-owned clear_and_parse clears the retained buffer.
    msg.clear_and_parse(&serialized).unwrap();
    expect_that!(msg.get_retained::<Vec<u8>>(), eq(None));

    // Direct Clear::clear and CopyFrom::copy_from also clear the retained buffer.
    msg.clear_and_parse_owned(serialized.clone()).unwrap();
    msg.clear();
    expect_that!(msg.get_retained::<Vec<u8>>(), eq(None));

    msg.clear_and_parse_owned(serialized).unwrap();
    msg.copy_from(&src);
    expect_that!(msg.get_retained::<Vec<u8>>(), eq(None));
}

#[gtest]
fn test_clear_and_parse_owned_clears_retained_on_error() {
    let mut src = TestAllTypes::new();
    src.set_optional_bytes(b"valid payload");
    let valid_bytes = src.serialize().unwrap();

    let mut msg = TestAllTypes::new();
    msg.clear_and_parse_owned(valid_bytes).unwrap();

    let malformed_data = vec![0x08u8, 0x80];
    expect_that!(msg.clear_and_parse_owned(malformed_data), err(anything()));
    expect_that!(msg.get_retained::<Vec<u8>>(), eq(None));
    expect_that!(msg.has_optional_bytes(), eq(false));
}

#[gtest]
fn test_parse_owned_fused_into_parent_and_containers() {
    use unittest_rust_proto::test_all_types::NestedMessage;

    let mut nested_src = NestedMessage::new();
    nested_src.set_bb(1234);
    let nested_bytes = nested_src.serialize().unwrap();

    let mut parent = TestAllTypes::new();
    let is_upb = {
        let mut child = NestedMessage::parse_owned(nested_bytes.clone()).unwrap();
        let has_retained = child.get_retained::<Vec<u8>>().is_some();
        parent.set_optional_nested_message(child);

        let repeated_child = NestedMessage::parse_owned(nested_bytes.clone()).unwrap();
        parent.repeated_nested_message_mut().push(repeated_child);
        has_retained
    };
    expect_that!(parent.optional_nested_message().bb(), eq(1234));
    expect_that!(parent.repeated_nested_message().get(0).unwrap().bb(), eq(1234));

    if is_upb {
        expect_that!(parent.get_retained::<Vec<u8>>(), eq(None));
        expect_that!(
            parent.optional_nested_message_mut().get_retained::<Vec<u8>>(),
            some(eq(&nested_bytes))
        );
        expect_that!(
            parent.repeated_nested_message_mut().get_mut(0).unwrap().get_retained::<Vec<u8>>(),
            some(eq(&nested_bytes))
        );
    }
}
