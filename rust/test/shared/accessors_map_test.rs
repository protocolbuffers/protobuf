// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use enums_rust_proto::{test_map_with_nested_enum, TestMapWithNestedEnum};
use googletest::prelude::*;
use map_unittest_rust_proto::{MapEnum, TestMap, TestMapWithMessages};
use paste::paste;
use protobuf::ProtoString;
use std::collections::HashMap;
use unittest_rust_proto::TestAllTypes;

macro_rules! generate_map_primitives_tests {
    (
        $(($k_type:ty, $v_type:ty, $k_field:ident, $v_field:ident,
           $k_nonzero:expr, $v_nonzero:expr $(,)?)),*
        $(,)?
    ) => {
        paste! { $(
            #[gtest]
            fn [< test_map_ $k_field _ $v_field >]() {
                let mut msg = TestMap::new();
                assert_that!(msg.[< map_ $k_field _ $v_field >]().len(), eq(0));
                assert_that!(
                    msg.[< map_ $k_field _ $v_field >](),
                    elements_are![]
                );
                assert_that!(
                    msg.[< map_ $k_field _ $v_field >]().keys().collect::<Vec<_>>(),
                    elements_are![]
                );
                assert_that!(
                    msg.[< map_ $k_field _ $v_field >]().values().collect::<Vec<_>>(),
                    elements_are![]
                );
                let k = <$k_type>::default();
                let v = <$v_type>::default();
                assert_that!(msg.[< map_ $k_field _ $v_field _mut>]().insert(k, v), eq(true));
                assert_that!(msg.[< map_ $k_field _ $v_field _mut>]().insert(k, v), eq(false));
                assert_that!(msg.[< map_ $k_field _ $v_field >]().len(), eq(1));
                assert_that!(
                    msg.[< map_ $k_field _ $v_field >](),
                    elements_are![eq((k, v))]
                );
                assert_that!(
                    msg.[< map_ $k_field _ $v_field >]().keys().collect::<Vec<_>>(),
                    elements_are![eq(&k)]
                );
                assert_that!(
                    msg.[< map_ $k_field _ $v_field >]().values().collect::<Vec<_>>(),
                    elements_are![eq(&v)]
                );

                let k2: $k_type = $k_nonzero;
                let v2: $v_type = $v_nonzero;
                assert_that!(msg.[< map_ $k_field _ $v_field _mut>]().insert(k2, v2), eq(true));
                assert_that!(msg.[< map_ $k_field _ $v_field >](), len(eq(2)));
                assert_that!(
                    msg.[< map_ $k_field _ $v_field >](),
                    unordered_elements_are![
                        eq((k, v)),
                        eq((k2, v2)),
                    ]
                );
                assert_that!(
                    msg.[< map_ $k_field _ $v_field >]().keys().collect::<Vec<_>>(),
                    unordered_elements_are![eq(&k), eq(&k2)]
                );
                assert_that!(
                    msg.[< map_ $k_field _ $v_field >]().values().collect::<Vec<_>>(),
                    unordered_elements_are![eq(&v), eq(&v2)]
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
    (i32, MapEnum, int32, enum, 1, MapEnum::Baz),
);

#[gtest]
fn collect_as_hashmap() {
    // Highlights conversion from protobuf map to hashmap.
    let mut msg = TestMap::new();
    msg.map_string_string_mut().insert("hello", "world");
    msg.map_string_string_mut().insert("fizz", "buzz");
    msg.map_string_string_mut().insert("boo", "blah");
    let hashmap: HashMap<String, String> =
        msg.map_string_string().iter().map(|(k, v)| (k.to_string(), v.to_string())).collect();
    assert_that!(
        hashmap,
        unordered_elements_are![
            (eq("hello"), eq("world")),
            (eq("fizz"), eq("buzz")),
            (eq("boo"), eq("blah")),
        ]
    );
}

#[gtest]
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

#[gtest]
fn test_nested_enum_maps() {
    // Verify that C++ thunks are generated and are with the right name for strings
    TestMapWithNestedEnum::new()
        .string_map_mut()
        .insert("foo", test_map_with_nested_enum::inner_nested::NestedEnum::Foo);
}

#[gtest]
fn test_bytes_and_string_copied() {
    let mut msg = TestMap::new();

    {
        // Ensure val is dropped after inserting into the map.
        let mut key = String::from("hello");
        let mut val = String::from("world");
        msg.map_string_string_mut().insert(key.as_str(), &val);
        msg.map_int32_bytes_mut().insert(1, val.as_bytes());
        // Validate that map keys are copied by mutating the originals.
        key.replace_range(.., "ayo");
        val.replace_range(.., "wOrld");
    }

    assert_that!(msg.map_string_string_mut().get("hello").unwrap(), eq("world"));
    assert_that!(msg.map_string_string(), unordered_elements_are![(eq("hello"), eq("world"))]);
    assert_that!(msg.map_int32_bytes_mut().get(1).unwrap(), eq(b"world"));
}

#[gtest]
fn test_map_setter() {
    // Set Map
    {
        let mut msg = TestMap::new();
        let mut map = protobuf::Map::<ProtoString, ProtoString>::new();
        map.as_mut().copy_from([("hello", "world"), ("fizz", "buzz")]);
        msg.set_map_string_string(map);
        assert_that!(
            msg.map_string_string(),
            unordered_elements_are![
                eq(("hello".into(), "world".into())),
                eq(("fizz".into(), "buzz".into()))
            ]
        );
    }

    // Set MapView
    {
        let mut msg = TestMap::new();
        let mut map = protobuf::Map::<ProtoString, ProtoString>::new();
        map.as_mut().copy_from([("hello", "world"), ("fizz", "buzz")]);
        msg.set_map_string_string(map.as_view());
        assert_that!(
            msg.map_string_string(),
            unordered_elements_are![
                eq(("hello".into(), "world".into())),
                eq(("fizz".into(), "buzz".into()))
            ]
        );
    }

    // Set MapMut
    {
        let mut msg = TestMap::new();
        let mut map = protobuf::Map::<ProtoString, ProtoString>::new();
        map.as_mut().copy_from([("hello", "world"), ("fizz", "buzz")]);
        msg.set_map_string_string(map.as_mut());
        assert_that!(
            msg.map_string_string(),
            unordered_elements_are![
                eq(("hello".into(), "world".into())),
                eq(("fizz".into(), "buzz".into()))
            ]
        );

        // The original map should remain unchanged.
        assert_that!(
            map.as_view(),
            unordered_elements_are![
                eq(("hello".into(), "world".into())),
                eq(("fizz".into(), "buzz".into()))
            ]
        );
    }
}

#[test]
fn test_map_creation_with_message_values() {
    // Maps are usually created and owned by a parent message, but let's verify that
    // we can successfully create and destroy them independently.
    macro_rules! test_for_each_key {
        ($($key_t:ty, $key:expr;)*) => {
            $(
                let msg = TestAllTypes::new();
                let mut map = protobuf::Map::<$key_t, TestAllTypes>::new();
                map.as_mut().insert($key, msg);
                assert_that!(map.as_view().len(), eq(1));
            )*
        }
    }

    test_for_each_key!(
        i32, -5;
        u32, 13u32;
        i64, 7;
        u64, 11u64;
        bool, false;
        ProtoString, "looooooooooooooooooooooooong string";
    );
}

#[test]
fn test_map_clearing_with_message_values() {
    macro_rules! test_for_each_key {
        ($($key_t:ty, $key:expr;)*) => {
            $(
                let msg = TestAllTypes::new();
                let mut map = protobuf::Map::<$key_t, TestAllTypes>::new();
                map.as_mut().insert($key, msg);
                assert_that!(map.as_view().len(), eq(1));
                map.as_mut().clear();
                assert_that!(map.as_view().len(), eq(0));
            )*
        }
    }

    test_for_each_key!(
        i32, -5;
        u32, 13u32;
        i64, 7;
        u64, 11u64;
        bool, false;
        ProtoString, "looooooooooooooooooooooooong string";
    );
}

macro_rules! generate_map_with_msg_values_tests {
    (
        $(($k_field:ident, $k_nonzero:expr, $k_other:expr $(,)?)),*
        $(,)?
    ) => {
        paste! { $(
            #[gtest]
            fn [< test_map_ $k_field _all_types >]() {
                // We need to cover the following upb/c++ thunks:
                // TODO - b/323883851: Add test once Map::new is public.
                // * new
                // * free (covered implicitly by drop)
                // * clear, size, insert, get, remove, iter, iter_next (all covered below)
                let mut msg = TestMapWithMessages::new();
                assert_that!(msg.[< map_ $k_field _all_types >]().len(), eq(0));
                assert_that!(msg.[< map_ $k_field _all_types >]().get($k_nonzero), none());
                // this block makes sure `insert` copies/moves, not borrows.
                {
                    let mut msg_val = TestAllTypes::new();
                    msg_val.set_optional_int32(1001);
                    assert_that!(
                        msg
                            .[< map_ $k_field _all_types_mut >]()
                            .insert($k_nonzero, msg_val.as_view()),
                        eq(true),
                        "`insert` should return true when key was inserted."
                    );
                    assert_that!(
                        msg
                            .[< map_ $k_field _all_types_mut >]()
                            .insert($k_nonzero, msg_val.as_view()),
                        eq(false),
                        "`insert` should return false when key was already present."

                    );
                }

                assert_that!(
                    msg.[< map_ $k_field _all_types >]().len(),
                    eq(1),
                    "`size` thunk should return correct len.");

                assert_that!(
                    msg.[< map_ $k_field _all_types >]().get($k_nonzero),
                    some(anything()),
                    "`get` should return Some when key present.");
                assert_that!(
                    msg.[< map_ $k_field _all_types >]().get($k_nonzero).unwrap().optional_int32(),
                    eq(1001));
                assert_that!(
                    msg.[< map_ $k_field _all_types >]().get($k_other),
                    none(),
                    "`get` should return None when key missing.");

                msg.[< map_ $k_field _all_types_mut >]().clear();
                assert_that!(
                    msg.[< map_ $k_field _all_types >]().len(),
                    eq(0),
                    "`clear` should drop all elements.");


                assert_that!(
                    msg.[< map_ $k_field _all_types_mut >]().insert($k_nonzero, TestAllTypes::new()),
                    eq(true));
                assert_that!(
                    msg.[< map_ $k_field _all_types_mut >]().remove($k_nonzero),
                    eq(true),
                    "`remove` should return true when key was present.");
                assert_that!(msg.[< map_ $k_field _all_types >](), empty());
                assert_that!(
                    msg.[< map_ $k_field _all_types_mut >]().remove($k_nonzero),
                    eq(false),
                    "`remove` should return false when key was missing.");

                // empty iter
                // assert_that!(
                //     msg.[< map_ $k_field _all_types_mut >]().iter().collect::<Vec<_>>(),
                //     elements_are![],
                //     "`iter` should work when empty."
                // );
                assert_that!(
                    msg.[< map_ $k_field _all_types_mut >]().keys().count(),
                    eq(0),
                    "`iter` should work when empty."
                );
                assert_that!(
                    msg.[< map_ $k_field _all_types_mut >]().values().count(),
                    eq(0),
                    "`iter` should work when empty."
                );

                // single element iter
                assert_that!(
                    msg.[< map_ $k_field _all_types_mut >]().insert($k_nonzero, TestAllTypes::new()),
                    eq(true));
                // assert_that!(
                //     msg.[< map_ $k_field _all_types >]().iter().collect::<Vec<_>>(),
                //     unordered_elements_are![
                //         eq(($k_nonzero, anything())),
                //     ]
                // );
                assert_that!(
                    msg.[< map_ $k_field _all_types >]().keys().collect::<Vec<_>>(),
                    unordered_elements_are![eq(&$k_nonzero)]
                );
                assert_that!(
                    msg.[< map_ $k_field _all_types >]().values().count(),
                    eq(1));


                // 2 element iter
                assert_that!(
                    msg
                        .[< map_ $k_field _all_types_mut >]()
                        .insert($k_other, TestAllTypes::new()),
                    eq(true));

                assert_that!(
                    msg.[< map_ $k_field _all_types >](),
                    len(eq(2))
                );
                assert_that!(
                    msg.[< map_ $k_field _all_types >]().keys().collect::<Vec<_>>(),
                    unordered_elements_are![eq(&$k_nonzero), eq(&$k_other)]
                );
                assert_that!(
                    msg.[< map_ $k_field _all_types >]().values().count(),
                    eq(2)
                );
            }
        )* }
    }
}

generate_map_with_msg_values_tests!(
    (int32, 1i32, 2i32),
    (int64, 1i64, 2i64),
    (uint32, 1u32, 2u32),
    (uint64, 1u64, 2u64),
    (sint32, 1, 2),
    (sint64, 1, 2),
    (fixed32, 1u32, 2u32),
    (fixed64, 1u64, 2u64),
    (sfixed32, 1, 2),
    (sfixed64, 1, 2),
    (bool, true, false),
    (string, "foo", "bar"),
);
