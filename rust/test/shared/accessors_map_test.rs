// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use googletest::prelude::*;
use map_unittest_proto::proto2_unittest::TestMap;
use paste::paste;

macro_rules! generate_map_primitives_tests {
    (
        $(($k_type:ty, $v_type:ty, $k_field:ident, $v_field:ident)),*
    ) => {
        paste! { $(
            #[test]
            fn [< test_map_ $k_field _ $v_field >]() {
                let mut msg = TestMap::new();
                let k: $k_type = Default::default();
                let v: $v_type = Default::default();
                assert_that!(msg.[< map_ $k_field _ $v_field _mut>]().insert(k, v), eq(true));
                assert_that!(msg.[< map_ $k_field _ $v_field >]().len(), eq(1));
            }
        )* }
    };
}

generate_map_primitives_tests!(
    (i32, i32, int32, int32),
    (i64, i64, int64, int64),
    (u32, u32, uint32, uint32),
    (u64, u64, uint64, uint64),
    (i32, i32, sint32, sint32),
    (i64, i64, sint64, sint64),
    (u32, u32, fixed32, fixed32),
    (u64, u64, fixed64, fixed64),
    (i32, i32, sfixed32, sfixed32),
    (i64, i64, sfixed64, sfixed64),
    (i32, f32, int32, float),
    (i32, f64, int32, double),
    (bool, bool, bool, bool)
);

#[test]
fn test_string_maps() {
    let mut msg = TestMap::new();
    msg.map_string_string_mut().insert("hello", "world".into());
    msg.map_string_string_mut().insert("fizz", "buzz".into());
    assert_that!(msg.map_string_string().len(), eq(2));
    assert_that!(msg.map_string_string().get("fizz").unwrap(), eq("buzz"));
    assert_that!(msg.map_string_string().get("not found"), eq(None));
    msg.map_string_string_mut().clear();
    assert_that!(msg.map_string_string().len(), eq(0));
}
