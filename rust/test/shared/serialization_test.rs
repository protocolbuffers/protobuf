// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use googletest::prelude::*;
use protobuf::prelude::*;
use protobuf::View;

use paste::paste;
use unittest_proto3_optional_rust_proto::TestProto3Optional;
use unittest_proto3_rust_proto::TestAllTypes as TestAllTypesProto3;
use unittest_rust_proto::TestAllTypes;

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
