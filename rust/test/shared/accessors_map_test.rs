// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use googletest::prelude::*;
use map_unittest_proto::TestMap;
use paste::paste;
use std::collections::HashMap;

macro_rules! generate_map_primitives_tests {
    (
        $(($k_type:ty, $v_type:ty, $k_field:ident, $v_field:ident,
           $k_nonzero:expr, $v_nonzero:expr $(,)?)),*
        $(,)?
    ) => {
        paste! { $(
            #[test]
            fn [< test_map_ $k_field _ $v_field >]() {
                let mut msg = TestMap::new();
                assert_that!(msg.[< map_ $k_field _ $v_field >]().len(), eq(0));
                assert_that!(
                    msg.[< map_ $k_field _ $v_field >]().iter().collect::<Vec<_>>(),
                    elements_are![]
                );
                let k = <$k_type>::default();
                let v = <$v_type>::default();
                assert_that!(msg.[< map_ $k_field _ $v_field _mut>]().insert(k, v), eq(true));
                assert_that!(msg.[< map_ $k_field _ $v_field _mut>]().insert(k, v), eq(false));
                assert_that!(msg.[< map_ $k_field _ $v_field >]().len(), eq(1));
                assert_that!(
                    msg.[< map_ $k_field _ $v_field >]().iter().collect::<Vec<_>>(),
                    elements_are![eq((k, v))]
                );

                let k: $k_type = $k_nonzero;
                let v: $v_type = $v_nonzero;
                assert_that!(msg.[< map_ $k_field _ $v_field _mut>]().insert(k, v), eq(true));
                assert_that!(msg.[< map_ $k_field _ $v_field >]().len(), eq(2));
                assert_that!(
                    msg.[< map_ $k_field _ $v_field >]().iter().collect::<Vec<_>>(),
                    unordered_elements_are![
                        eq((k, v)),
                        eq((<$k_type>::default(), <$v_type>::default())),
                    ]
                );
            }
        )* }
    };
}

generate_map_primitives_tests!(
    (i32, i32, int32, int32, 1, 1),
    (i64, i64, int64, int64, 1, 1),
    (u32, u32, uint32, uint32, 1, 1),
    (u64, u64, uint64, uint64, 1, 1),
    (i32, i32, sint32, sint32, 1, 1),
    (i64, i64, sint64, sint64, 1, 1),
    (u32, u32, fixed32, fixed32, 1, 1),
    (u64, u64, fixed64, fixed64, 1, 1),
    (i32, i32, sfixed32, sfixed32, 1, 1),
    (i64, i64, sfixed64, sfixed64, 1, 1),
    (i32, f32, int32, float, 1, 1.),
    (i32, f64, int32, double, 1, 1.),
    (bool, bool, bool, bool, true, true),
    (i32, &[u8], int32, bytes, 1, b"foo"),
);

#[test]
fn collect_as_hashmap() {
    // Highlights conversion from protobuf map to hashmap.
    let mut msg = TestMap::new();
    msg.map_string_string_mut().insert("hello", "world");
    msg.map_string_string_mut().insert("fizz", "buzz");
    msg.map_string_string_mut().insert("boo", "blah");
    let hashmap: HashMap<String, String> =
        msg.map_string_string().iter().map(|(k, v)| (k.to_string(), v.to_string())).collect();
    assert_that!(
        hashmap.into_iter().collect::<Vec<_>>(),
        unordered_elements_are![
            eq(("hello".to_owned(), "world".to_owned())),
            eq(("fizz".to_owned(), "buzz".to_owned())),
            eq(("boo".to_owned(), "blah".to_owned())),
        ]
    );
}

#[test]
fn test_string_maps() {
    let mut msg = TestMap::new();
    msg.map_string_string_mut().insert("hello", "world");
    msg.map_string_string_mut().insert("fizz", "buzz");
    assert_that!(msg.map_string_string().len(), eq(2));
    assert_that!(msg.map_string_string().get("fizz").unwrap(), eq("buzz"));
    assert_that!(msg.map_string_string().get("not found"), eq(None));
    msg.map_string_string_mut().clear();
    assert_that!(msg.map_string_string().len(), eq(0));
}

#[test]
fn test_bytes_and_string_copied() {
    let mut msg = TestMap::new();

    {
        // Ensure val is dropped after inserting into the map.
        let key = String::from("hello");
        let val = String::from("world");
        msg.map_string_string_mut().insert(key.as_str(), val.as_str());
        msg.map_int32_bytes_mut().insert(1, val.as_bytes());
    }

    assert_that!(msg.map_string_string_mut().get("hello").unwrap(), eq("world"));
    assert_that!(msg.map_int32_bytes_mut().get(1).unwrap(), eq(b"world"));
}
