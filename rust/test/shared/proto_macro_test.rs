// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

//! Tests covering accessors for singular bool, int32, int64, and bytes fields.

use googletest::prelude::*;
use protobuf::proto;
use unittest_proto::{
    test_all_types::{self, NestedMessage},
    NestedTestAllTypes, TestAllTypes,
};

#[test]
fn test_setting_literals() {
    let fixed64 = || 108;
    let msg = proto!(TestAllTypes {
        optional_int32: 101,
        optional_int64: 102,
        optional_uint32: 103,
        optional_uint64: 104,
        optional_sint32: -105,
        optional_sint64: 106,
        optional_fixed32: 107,
        optional_fixed64: fixed64(), //108
        optional_sfixed32: 100 + 9,
        optional_sfixed64: {
            let x = 10;
            100 + x
        },
        optional_nested_message: NestedMessage { bb: 42 },
        optional_float: 111.5,
        optional_double: 112000.5,
        optional_bool: true,
        optional_string: "foo",
        optional_bytes: b"bar",
        optional_nested_enum: test_all_types::NestedEnum::Baz,
    });

    assert_that!(msg.optional_int32(), eq(101));
    assert_that!(msg.optional_int64(), eq(102));
    assert_that!(msg.optional_uint32(), eq(103));
    assert_that!(msg.optional_uint64(), eq(104));
    assert_that!(msg.optional_sint32(), eq(-105));
    assert_that!(msg.optional_sint64(), eq(106));
    assert_that!(msg.optional_fixed32(), eq(107));
    assert_that!(msg.optional_fixed64(), eq(108));
    assert_that!(msg.optional_sfixed32(), eq(109));
    assert_that!(msg.optional_sfixed64(), eq(110));
    assert_that!(msg.optional_float(), eq(111.5));
    assert_that!(msg.optional_double(), eq(112000.5));
    assert_that!(msg.optional_bool(), eq(true));
    assert_that!(msg.optional_string(), eq("foo"));
    assert_that!(msg.optional_bytes(), eq(b"bar"));
    assert_that!(msg.optional_nested_enum(), eq(test_all_types::NestedEnum::Baz));
}

#[test]
fn single_nested_message() {
    let msg = proto!(TestAllTypes { optional_nested_message: NestedMessage { bb: 42 } });
    assert_that!(msg.optional_nested_message().bb(), eq(42));

    // field above it
    let msg = proto!(TestAllTypes {
        optional_int32: 1,
        optional_nested_message: NestedMessage { bb: 42 }
    });
    assert_that!(msg.optional_nested_message().bb(), eq(42));

    // field below it
    let msg = proto!(TestAllTypes {
        optional_nested_message: NestedMessage { bb: 42 },
        optional_int32: 1
    });
    assert_that!(msg.optional_nested_message().bb(), eq(42));

    // field above and below it
    let msg = proto!(TestAllTypes {
        optional_int32: 1,
        optional_nested_message: NestedMessage { bb: 42 },
        optional_int64: 2
    });
    assert_that!(msg.optional_nested_message().bb(), eq(42));

    // test empty initializer
    let msg = proto!(TestAllTypes {});
    assert_that!(msg.has_optional_nested_message(), eq(false));

    // empty nested message should be present
    // make sure that qualified path names work
    let msg = proto!(::unittest_proto::TestAllTypes {
        optional_nested_message: unittest_proto::test_all_types::NestedMessage {}
    });
    assert_that!(msg.has_optional_nested_message(), eq(true));
}

#[test]
fn test_recursive_msg() {
    let msg = proto!(NestedTestAllTypes {
        child: NestedTestAllTypes {
            payload: TestAllTypes { optional_int32: 41 },
            child: NestedTestAllTypes {
                child: NestedTestAllTypes { payload: TestAllTypes { optional_int32: 43 } },
                payload: TestAllTypes { optional_int32: 42 }
            }
        }
    });

    assert_that!(msg.child().payload().optional_int32(), eq(41));
    assert_that!(msg.child().child().payload().optional_int32(), eq(42));
    assert_that!(msg.child().child().child().payload().optional_int32(), eq(43));
}
