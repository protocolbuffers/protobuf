// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use googletest::prelude::*;
use map_unittest_rust_proto::TestMapWithMessages;
use protobuf_upb::proto;
use unittest_rust_proto::{
    test_all_types::NestedEnum as NestedEnumProto2,
    test_all_types::NestedMessage as NestedMessageProto2, TestAllTypes as TestAllTypesProto2,
};

#[gtest]
fn test_debug_string() {
    let mut msg = proto!(TestAllTypesProto2 {
        optional_int32: 42,
        optional_string: "Hello World",
        optional_nested_enum: NestedEnumProto2::Bar,
        oneof_uint32: 452235,
        optional_nested_message: proto!(NestedMessageProto2 { bb: 100 }),
    });
    let mut repeated_string = msg.repeated_string_mut();
    repeated_string.push("Hello World");
    repeated_string.push("Hello World");
    repeated_string.push("Hello World");

    let mut msg_map = TestMapWithMessages::new();
    println!("EMPTY MSG: {:?}", msg_map); // Make sure that we can print an empty message. 
    msg_map.map_string_all_types_mut().insert("hello", msg.as_view());
    msg_map.map_string_all_types_mut().insert("fizz", msg.as_view());
    msg_map.map_string_all_types_mut().insert("boo", msg.as_view());

    println!("{:?}", msg_map);
    println!("{:?}", msg_map.as_view()); // Make sure that we can print as_view
    println!("{:?}", msg_map.as_mut()); // Make sure that we can print as_mut
    let golden = r#"12 {
  key: "hello"
  value {
    1: 42
    14: "Hello World"
    18 {
      1: 100
    }
    21: 2
    44: "Hello World"
    44: "Hello World"
    44: "Hello World"
    111: 452235
  }
}
12 {
  key: "fizz"
  value {
    1: 42
    14: "Hello World"
    18 {
      1: 100
    }
    21: 2
    44: "Hello World"
    44: "Hello World"
    44: "Hello World"
    111: 452235
  }
}
12 {
  key: "boo"
  value {
    1: 42
    14: "Hello World"
    18 {
      1: 100
    }
    21: 2
    44: "Hello World"
    44: "Hello World"
    44: "Hello World"
    111: 452235
  }
}
"#;
    // C strings are null terminated while Rust strings are not.
    let null_terminated_str = format!("{}\0", golden);
    assert_that!(format!("{:?}", msg_map), eq(null_terminated_str.as_str()));
}
