// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

//! Tests covering accessors for singular bool, int32, int64, and bytes fields.

use googletest::prelude::*;
use protobuf::proto;
use unittest_rust_proto::{
    test_all_types::{self, NestedMessage},
    NestedTestAllTypes, TestAllTypes,
};

use map_unittest_rust_proto::{TestMap, TestMapWithMessages};

struct TestValue {
    val: i64,
}

#[gtest]
fn test_setting_literals() {
    let fixed64 = || 108;
    let test_ref = |x: &i64| *x;
    let test_ref_b = |x: &u32| *x;
    let one_oh_seven = [107_u32];
    let value = TestValue { val: 106 };

    let msg = proto!(TestAllTypes {
        optional_int32: 101,
        optional_int64: 102,
        optional_uint32: 103,
        optional_uint64: if true { 104 } else { 99 },
        optional_sint32: -105,
        optional_sint64: (test_ref(&value.val)),
        optional_fixed32: { test_ref_b(&one_oh_seven[0]) },
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

#[gtest]
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

    // field above and below it
    let msg = proto!(TestAllTypes {
        optional_int32: 1,
        optional_nested_message: __ { bb: 42 },
        optional_int64: 2
    });
    assert_that!(msg.optional_nested_message().bb(), eq(42));

    // test empty initializer
    let msg = proto!(TestAllTypes {});
    assert_that!(msg.has_optional_nested_message(), eq(false));

    // empty nested message should be present
    // make sure that qualified path names work
    let msg = proto!(::unittest_rust_proto::TestAllTypes {
        optional_nested_message: unittest_rust_proto::test_all_types::NestedMessage {}
    });
    assert_that!(msg.has_optional_nested_message(), eq(true));

    let msg = proto!(::unittest_rust_proto::TestAllTypes {
        optional_nested_message: ::unittest_rust_proto::test_all_types::NestedMessage {}
    });
    assert_that!(msg.has_optional_nested_message(), eq(true));

    let msg = proto!(::unittest_rust_proto::TestAllTypes { optional_nested_message: __ {} });
    assert_that!(msg.has_optional_nested_message(), eq(true));
}

#[gtest]
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

#[gtest]
fn test_spread_msg() {
    let msg = proto!(TestAllTypes { optional_nested_message: NestedMessage { bb: 42 } });
    let msg2 = proto!(TestAllTypes { ..msg.as_view() });
    assert_that!(msg2.optional_nested_message().bb(), eq(42));
    let msg3 = proto!(TestAllTypes { optional_int32: 1, ..msg.as_view() });
    assert_that!(msg3.optional_nested_message().bb(), eq(42));
    assert_that!(msg3.optional_int32(), eq(1));
}

#[gtest]
fn test_spread_nested_msg() {
    let msg = proto!(NestedTestAllTypes {
        child: NestedTestAllTypes {
            payload: TestAllTypes { optional_int32: 41 },
            child: NestedTestAllTypes {
                child: NestedTestAllTypes { payload: TestAllTypes { optional_int32: 43 } },
                payload: TestAllTypes { optional_int32: 42 }
            }
        }
    });
    let msg2 = proto!(NestedTestAllTypes {
        child: NestedTestAllTypes { payload: TestAllTypes { optional_int32: 100 }, ..msg.child() }
    });
    assert_that!(msg2.child().payload().optional_int32(), eq(100));
    assert_that!(msg2.child().child().payload().optional_int32(), eq(42));
    assert_that!(msg2.child().child().child().payload().optional_int32(), eq(43));
}

#[gtest]
fn test_repeated_i32() {
    let msg = proto!(TestAllTypes { repeated_int32: [1, 1 + 1, 3] });
    assert_that!(msg.repeated_int32().len(), eq(3));
    assert_that!(msg.repeated_int32().get(0).unwrap(), eq(1));
    assert_that!(msg.repeated_int32().get(1).unwrap(), eq(2));
    assert_that!(msg.repeated_int32().get(2).unwrap(), eq(3));
}

#[gtest]
fn test_repeated_msg() {
    let msg2 = proto!(NestedTestAllTypes { payload: TestAllTypes { optional_int32: 1 } });
    let msg = proto!(NestedTestAllTypes {
        child: NestedTestAllTypes {
            repeated_child: [
                NestedTestAllTypes { payload: TestAllTypes { optional_int32: 0 } },
                msg2,
                __ { payload: TestAllTypes { optional_int32: 2 } }
            ]
        },
        repeated_child: [
            __ { payload: __ { optional_int32: 1 } },
            NestedTestAllTypes { payload: TestAllTypes { optional_int32: 2 } }
        ]
    });
    assert_that!(msg.child().repeated_child().len(), eq(3));
    assert_that!(msg.child().repeated_child().get(0).unwrap().payload().optional_int32(), eq(0));
    assert_that!(msg.child().repeated_child().get(1).unwrap().payload().optional_int32(), eq(1));
    assert_that!(msg.child().repeated_child().get(2).unwrap().payload().optional_int32(), eq(2));

    assert_that!(msg.repeated_child().len(), eq(2));
    assert_that!(msg.repeated_child().get(0).unwrap().payload().optional_int32(), eq(1));
    assert_that!(msg.repeated_child().get(1).unwrap().payload().optional_int32(), eq(2));
}

#[gtest]
fn test_string_maps() {
    let msg =
        proto!(TestMap { map_string_string: [("foo", "bar"), ("baz", "qux"), ("quux", "quuz")] });
    assert_that!(msg.map_string_string().len(), eq(3));
    assert_that!(msg.map_string_string().get("foo").unwrap(), eq("bar"));
    assert_that!(msg.map_string_string().get("baz").unwrap(), eq("qux"));
    assert_that!(msg.map_string_string().get("quux").unwrap(), eq("quuz"));
}

#[gtest]
fn test_message_maps() {
    let msg3 = proto!(TestAllTypes { optional_int32: 3 });
    let kv3 = ("quux", msg3);
    let msg = proto!(TestMapWithMessages {
        map_string_all_types: [
            ("foo", TestAllTypes { optional_int32: 1 }),
            ("baz", __ { optional_int32: 2 }),
            kv3
        ]
    });
    assert_that!(msg.map_string_all_types().len(), eq(3));
    assert_that!(msg.map_string_all_types().get("foo").unwrap().optional_int32(), eq(1));
    assert_that!(msg.map_string_all_types().get("baz").unwrap().optional_int32(), eq(2));
    assert_that!(msg.map_string_all_types().get("quux").unwrap().optional_int32(), eq(3));
}
