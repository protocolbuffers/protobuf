// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use googletest::prelude::*;
use paste::paste;
use std::ops::Deref;
use unittest_edition_rust_proto::{
    ForeignEnum as ForeignEnumEditions, TestAllTypes as TestAllTypesEditions,
    TestPackedTypes as TestPackedTypesEditions, TestUnpackedTypes as TestUnpackedTypesEditions,
};
use unittest_proto3_optional_rust_proto::TestProto3Optional;
use unittest_proto3_rust_proto::{
    test_all_types as test_all_types_proto3, ForeignEnum as ForeignEnumProto3,
    TestAllTypes as TestAllTypesProto3, TestPackedTypes as TestPackedTypesProto3,
    TestUnpackedTypes as TestUnpackedTypesProto3,
};
use unittest_rust_proto::{
    ForeignEnum as ForeignEnumProto2, TestAllTypes as TestAllTypesProto2,
    TestPackedTypes as TestPackedTypesProto2, TestUnpackedTypes as TestUnpackedTypesProto2,
};

const GOLDEN_DATA_PACKED: &[u8; 86] = b"\xd2\x05\x01\x01\xDA\x05\x01\x01\xe2\x05\x01\x01\xEA\x05\x01\x01\xf2\x05\x01\x02\xfa\x05\x01\x02\x82\x06\x04\x01\x00\x00\x00\x8a\x06\x08\x01\x00\x00\x00\x00\x00\x00\x00\x92\x06\x04\x01\x00\x00\x00\x9a\x06\x08\x01\x00\x00\x00\x00\x00\x00\x00\xa2\x06\x04\x00\x00\x80\x3f\xaa\x06\x08\x00\x00\x00\x00\x00\x00\xf0\x3f\xb2\x06\x01\x01\xba\x06\x01\x04";
const GOLDEN_DATA_UNPACKED: &[u8; 72] = b"\xD0\x05\x01\xD8\x05\x01\xe0\x05\x01\xe8\x05\x01\xf0\x05\x02\xf8\x05\x02\x85\x06\x01\x00\x00\x00\x89\x06\x01\x00\x00\x00\x00\x00\x00\x00\x95\x06\x01\x00\x00\x00\x99\x06\x01\x00\x00\x00\x00\x00\x00\x00\xA5\x06\x00\x00\x80\x3f\xA9\x06\x00\x00\x00\x00\x00\x00\xf0\x3f\xb0\x06\x01\xb8\x06\x04";
// For proto3 message TestUnpackedTypes whose field numbers are 1 through 14
const GOLDEN_DATA_UNPACKED_LOW: &[u8; 58] = b"\x08\x01\x10\x01\x18\x01\x20\x01\x28\x02\x30\x02\x3D\x01\x00\x00\x00\x41\x01\x00\x00\x00\x00\x00\x00\x00\x4D\x01\x00\x00\x00\x51\x01\x00\x00\x00\x00\x00\x00\x00\x5D\x00\x00\x80\x3f\x61\x00\x00\x00\x00\x00\x00\xf0\x3f\x68\x01\x70\x01";

macro_rules! generate_parameterized_serialization_test {
    ($(($type: ident, $name_ext: ident)),*) => {
        paste! { $(
            #[test]
            fn [< serialization_zero_length_ $name_ext >]() {
                let mut msg = [< $type >]::new();

                let serialized = msg.serialize().unwrap();
                assert_that!(serialized.len(), eq(0));

                let serialized = msg.as_view().serialize().unwrap();
                assert_that!(serialized.len(), eq(0));

                let serialized = msg.as_mut().serialize().unwrap();
                assert_that!(serialized.len(), eq(0));
            }

            #[test]
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

            #[test]
            fn [< deserialize_empty_ $name_ext>]() {
                assert!([< $type >]::parse(&[]).is_ok());
            }

            #[test]
            fn [< deserialize_error_ $name_ext>]() {
                assert!([< $type >]::parse(b"not a serialized proto").is_err());
            }

            #[test]
            fn [< set_bytes_with_serialized_data_ $name_ext>]() {
                let mut msg = [< $type >]::new();
                msg.set_optional_int64(42);
                msg.set_optional_bool(true);
                let mut msg2 = [< $type >]::new();
                msg2.set_optional_bytes(msg.serialize().unwrap());
                assert_that!(msg2.optional_bytes(), eq(msg.serialize().unwrap().as_ref()));
            }

            #[test]
            fn [< deserialize_on_previously_allocated_message_ $name_ext>]() {
                let mut msg = [< $type >]::new();
                msg.set_optional_int64(42);
                msg.set_optional_bool(true);
                msg.set_optional_bytes(b"serialize deserialize test");

                let serialized = msg.serialize().unwrap();

                let mut msg2 = Box::new([< $type >]::new());
                assert!(msg2.clear_and_parse(&serialized).is_ok());
                assert_that!(msg.optional_int64(), eq(msg2.optional_int64()));
                assert_that!(msg.optional_bool(), eq(msg2.optional_bool()));
                assert_that!(msg.optional_bytes(), eq(msg2.optional_bytes()));
            }

        )* }
    };
  }

generate_parameterized_serialization_test!(
    (TestAllTypesProto2, proto2),
    (TestAllTypesProto3, proto3),
    (TestAllTypesEditions, editions),
    (TestProto3Optional, proto3_optional)
);

macro_rules! generate_parameterized_int32_byte_size_test {
    ($(($type: ident, $name_ext: ident)),*) => {
        paste! { $(

            #[test]
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
    (TestAllTypesProto2, proto2),
    (TestProto3Optional, proto3_optional), /* Test would fail if we were to use
                                            * TestAllTypesProto3: optional_int32 follows "no
                                            * presence" semantics and setting it to 0 (default
                                            * value) will cause it to not be serialized */
    (TestAllTypesEditions, editions)
);

macro_rules! set_message {
    ($type: ident, $field_name: ident, $name_ext: ident, $enum_field: ident, $fully_qualified_enum: expr) => {
        paste! {

            fn [< set_message_ $name_ext >]() -> $type {
                let mut message = [< $type >]::new();
                message.[< $field_name _int32_mut>]().push(1);
                message.[< $field_name _int64_mut>]().push(1);
                message.[< $field_name _uint32_mut>]().push(1);
                message.[< $field_name _uint64_mut>]().push(1);
                message.[< $field_name _sint32_mut>]().push(1);
                message.[< $field_name _sint64_mut>]().push(1);
                message.[< $field_name _fixed32_mut>]().push(1);
                message.[< $field_name _fixed64_mut>]().push(1);
                message.[< $field_name _sfixed32_mut>]().push(1);
                message.[< $field_name _sfixed64_mut>]().push(1);
                message.[< $field_name _float_mut>]().push(1.0);
                message.[< $field_name _double_mut>]().push(1.0);
                message.[< $field_name _bool_mut>]().push(true);
                message.[< $enum_field>]().push($fully_qualified_enum);
                message

            }

        }
    };
}

macro_rules! generate_packed_unpacked_test {
    ($(($type: ident, $field_name: ident, $name_ext: ident, $enum_field: ident, $fully_qualified_enum: expr, $golden_data: ident)),*) => {
        paste! { $(
        #[googletest::test]
        fn [<test_fields_ $name_ext>]() {
            set_message!(
                $type,
                $field_name,
                $name_ext,
                $enum_field,
                $fully_qualified_enum
            );
            let message = [<set_message_ $name_ext>]();
            let serialized = message.serialize().unwrap();
            expect_that!(
                $golden_data.len(),
                eq(serialized.len()),
                "Golden data len: {} ; Serialized data len: {}",
                $golden_data.len(),
                serialized.len()
            );
            assert_that!(
                $golden_data,
                eq(serialized.deref()),
                "Golden data: {:?} ; Serialized data: {:?}",
                $golden_data,
                serialized.deref()
            );
        }


        )*}
    };
}

// Generates tests for both TestPackedTypes and TestUnpackedTypes messages
generate_packed_unpacked_test!(
    (
        TestUnpackedTypesProto3,
        repeated,
        unpacked_proto3,
        repeated_nested_enum_mut,
        test_all_types_proto3::NestedEnum::Foo,
        GOLDEN_DATA_UNPACKED_LOW
    ),
    (
        TestUnpackedTypesProto2,
        unpacked,
        unpacked_proto2,
        unpacked_enum_mut,
        ForeignEnumProto2::ForeignFoo,
        GOLDEN_DATA_UNPACKED
    ),
    (
        TestUnpackedTypesEditions,
        unpacked,
        unpacked_editions,
        unpacked_enum_mut,
        ForeignEnumEditions::ForeignFoo,
        GOLDEN_DATA_UNPACKED
    ),
    (
        TestPackedTypesProto3,
        packed,
        packed_proto3,
        packed_enum_mut,
        ForeignEnumProto3::ForeignFoo,
        GOLDEN_DATA_PACKED
    ),
    (
        TestPackedTypesProto2,
        packed,
        packed_proto2,
        packed_enum_mut,
        ForeignEnumProto2::ForeignFoo,
        GOLDEN_DATA_PACKED
    ),
    (
        TestPackedTypesEditions,
        packed,
        packed_editions,
        packed_enum_mut,
        ForeignEnumEditions::ForeignFoo,
        GOLDEN_DATA_PACKED
    )
);
