// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use googletest::prelude::*;
use map_unittest_rust_proto::TestMap;
use paste::paste;
use protobuf::proto;
use protobuf_gtest_matchers::{proto_eq, proto_partially_eq};
use unittest_proto3_rust_proto::test_all_types::NestedMessage;
use unittest_proto3_rust_proto::TestAllTypes as TestAllTypesProto3;
use unittest_rust_proto::TestAllTypes;

macro_rules! generate_eq_msgs_tests {
  ($(($type: ident, $name_ext: ident)),*) => {
        paste! {$(
            #[gtest]
            fn [<expect_eq_msgs_ $name_ext>]() {
              let mut msg = [< $type >]::new();
              let mut msg2 = [< $type >]::new();
              msg.set_optional_int32(1);
              msg2.set_optional_int32(1);
              assert_that!(&msg.as_view(), proto_eq(msg2.as_view()));
              assert_that!(&msg.as_mut(), proto_eq(msg2.as_mut()));
              assert_that!(msg, proto_eq(msg2));
            }
        )*}
    }
}

macro_rules! generate_not_eq_msgs_tests {
  ($(($type: ident, $name_ext: ident)),*) => {
        paste! {$(
            #[gtest]
            fn [<expect_not_eq_msgs_ $name_ext>]() {
                let mut msg = [< $type >]::new();
                let mut msg2 = [< $type >]::new();
                msg.set_optional_int32(1);
                msg2.set_optional_int32(0);
                assert_that!(&msg.as_view(), not(proto_eq(msg2.as_view())));
                assert_that!(&msg.as_mut(), not(proto_eq(msg2.as_mut())));
                assert_that!(&msg, not(proto_eq(msg2)));
            }
        )*}
    }
}

macro_rules! generate_partially_eq_msgs_tests {
  ($(($type: ident, $name_ext: ident)),*) => {
        paste! {$(
            #[gtest]
            fn [<expect_partially_eq_msgs_ $name_ext>]() {
                let mut actual = [< $type >]::new();
                let mut expected = [< $type >]::new();
                actual.set_optional_int32(1);
                actual.set_optional_int64(42);
                expected.set_optional_int32(1);

                // actual has an extra field set (optional_int64) -> matches partially
                assert_that!(&actual.as_view(), proto_partially_eq(expected.as_view()));
                assert_that!(&actual, proto_partially_eq(&expected));

                // missing field in actual -> does NOT match partially
                let mut missing_actual = [< $type >]::new();
                missing_actual.set_optional_int64(42);
                assert_that!(&missing_actual, not(proto_partially_eq(&expected)));

                // different value in actual -> does NOT match partially
                let mut diff_actual = [< $type >]::new();
                diff_actual.set_optional_int32(2);
                diff_actual.set_optional_int64(42);
                assert_that!(&diff_actual, not(proto_partially_eq(&expected)));
            }

            #[gtest]
            fn [<expect_partially_presence_field_set_to_default_ $name_ext>]() {
                let mut expected = [< $type >]::new();
                expected.optional_nested_message_mut().set_bb(0);

                // actual is empty (optional_nested_message is unset in actual, but set in expected)
                let actual_empty = [< $type >]::new();
                assert_that!(&actual_empty, not(proto_partially_eq(&expected)));

                // actual has optional_nested_message set to same value -> matches partially
                let mut actual_matching = [< $type >]::new();
                actual_matching.optional_nested_message_mut().set_bb(0);
                actual_matching.set_optional_int64(42);
                assert_that!(&actual_matching, proto_partially_eq(&expected));
            }

            #[gtest]
            fn [<expect_partially_empty_expected_ $name_ext>]() {
                let mut actual = [< $type >]::new();
                let expected = [< $type >]::new();
                actual.set_optional_int32(100);
                actual.set_optional_int64(200);

                // An empty expected message matches any actual message partially
                assert_that!(&actual, proto_partially_eq(&expected));
            }

            #[gtest]
            fn [<expect_partially_nested_message_ $name_ext>]() {
                let mut actual = [< $type >]::new();
                actual.set_optional_int32(1);
                actual.optional_nested_message_mut().set_bb(10);

                let mut expected = [< $type >]::new();
                expected.optional_nested_message_mut().set_bb(10);

                // Matches partially when nested submessage field matches and actual has extra
                // top-level field
                assert_that!(&actual, proto_partially_eq(&expected));

                // Mismatched nested submessage field -> does NOT match
                let mut diff_nested = [< $type >]::new();
                diff_nested.optional_nested_message_mut().set_bb(20);
                assert_that!(&diff_nested, not(proto_partially_eq(&expected)));
            }

            #[gtest]
            fn [<expect_partially_repeated_fields_ $name_ext>]() {
                let mut actual = [< $type >]::new();
                actual.set_optional_int32(1);
                actual.set_repeated_int32([10, 20].into_iter());

                let mut expected = [< $type >]::new();
                expected.set_repeated_int32([10, 20].into_iter());

                // Matches partially when repeated fields are identical and actual has extra scalar
                // field
                assert_that!(&actual, proto_partially_eq(&expected));

                // Repeated array mismatch (different length) -> does NOT match partially
                let mut diff_repeated = [< $type >]::new();
                diff_repeated.set_repeated_int32([10, 20, 30].into_iter());
                assert_that!(&diff_repeated, not(proto_partially_eq(&expected)));
            }
        )*}
    }
}

macro_rules! generate_partially_map_tests {
  ($(($type: ident, $name_ext: ident)),*) => {
        paste! {$(
            #[gtest]
            fn [<expect_partially_map_matching_ $name_ext>]() {
                let mut actual = [< $type >]::new();
                let mut expected = [< $type >]::new();

                actual.map_string_string_mut().insert("k1", "v1");
                actual.map_string_string_mut().insert("k2", "v2");

                expected.map_string_string_mut().insert("k1", "v1");

                // actual has an extra key "k2" -> matches partially
                assert_that!(&actual.as_view(), proto_partially_eq(expected.as_view()));
                assert_that!(&actual, proto_partially_eq(&expected));

                // diff value for key "k1" -> does NOT match partially
                let mut diff_actual = [< $type >]::new();
                diff_actual.map_string_string_mut().insert("k1", "v2");
                diff_actual.map_string_string_mut().insert("k2", "v2");
                assert_that!(&diff_actual, not(proto_partially_eq(&expected)));

                // missing key "k1" -> does NOT match partially
                let mut missing_actual = [< $type >]::new();
                missing_actual.map_string_string_mut().insert("k2", "v2");
                assert_that!(&missing_actual, not(proto_partially_eq(&expected)));
            }
        )*}
    }
}

generate_eq_msgs_tests!((TestAllTypes, editions), (TestAllTypesProto3, proto3));

generate_not_eq_msgs_tests!((TestAllTypes, editions), (TestAllTypesProto3, proto3));

generate_partially_eq_msgs_tests!((TestAllTypes, editions), (TestAllTypesProto3, proto3));

generate_partially_map_tests!((TestMap, editions));

#[gtest]
fn proto_eq_works_on_view() {
    // This exercises the `impl<T> Matcher<T> for MessageMatcher<T>
    // where T: MatcherEq + Copy` implementation.
    let msg = proto!(TestAllTypesProto3 {
        repeated_nested_message: [
            NestedMessage { bb: 10 },
            NestedMessage { bb: 20 },
            NestedMessage { bb: 30 }
        ]
    });

    expect_that!(
        msg.repeated_nested_message(),
        unordered_elements_are![
            proto_eq(proto!(NestedMessage { bb: 10 }).as_view()),
            proto_eq(proto!(NestedMessage { bb: 20 }).as_view()),
            proto_eq(proto!(NestedMessage { bb: 30 }).as_view()),
        ]
    );
}
