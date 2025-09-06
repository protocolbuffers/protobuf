// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#[cfg(not(bzl))]
mod protos;
#[cfg(not(bzl))]
use protos::*;

use googletest::prelude::*;
#[allow(unused_imports)]
use protobuf::{AsView, IntoProxied, IntoView, MergeFrom, Parse, Serialize};

#[gtest]
fn test_simple_i32_extension() {
    assert_eq!(extensions_rust_proto::I32_EXTENSION.number(), 1);
    let mut test_extensions = extensions_rust_proto::TestExtensions::new();
    assert!(!extensions_rust_proto::I32_EXTENSION.has(&test_extensions));
    assert!(!extensions_rust_proto::I32_EXTENSION.has(test_extensions.as_view()));
    assert!(!extensions_rust_proto::I32_EXTENSION.has(test_extensions.as_mut()));

    assert_eq!(extensions_rust_proto::I32_EXTENSION.get(&test_extensions), 0);
    assert_eq!(extensions_rust_proto::I32_EXTENSION.get(test_extensions.as_view()), 0);
    assert_eq!(extensions_rust_proto::I32_EXTENSION.get(test_extensions.as_mut()), 0);

    extensions_rust_proto::I32_EXTENSION.set(&mut test_extensions, 42);
    extensions_rust_proto::I32_EXTENSION.set(test_extensions.as_mut(), 42);
    assert!(extensions_rust_proto::I32_EXTENSION.has(&test_extensions));
    assert!(extensions_rust_proto::I32_EXTENSION.has(test_extensions.as_view()));
    assert!(extensions_rust_proto::I32_EXTENSION.has(test_extensions.as_mut()));

    assert_eq!(extensions_rust_proto::I32_EXTENSION.get(&test_extensions), 42);
    assert_eq!(extensions_rust_proto::I32_EXTENSION.get(test_extensions.as_view()), 42);
    assert_eq!(extensions_rust_proto::I32_EXTENSION.get(test_extensions.as_mut()), 42);

    extensions_rust_proto::I32_EXTENSION.clear(&mut test_extensions);
    extensions_rust_proto::I32_EXTENSION.clear(test_extensions.as_mut());
    assert!(!extensions_rust_proto::I32_EXTENSION.has(&test_extensions));
    assert!(!extensions_rust_proto::I32_EXTENSION.has(test_extensions.as_mut()));

    assert_eq!(extensions_rust_proto::I32_EXTENSION.get(&test_extensions), 0);
    assert_eq!(extensions_rust_proto::I32_EXTENSION.get(test_extensions.as_view()), 0);
    assert_eq!(extensions_rust_proto::I32_EXTENSION.get(test_extensions.as_mut()), 0);
}

macro_rules! test_scalar_extension {
    ($test_name:ident, $ext:ident, $val:expr, $default_val:expr) => {
        #[gtest]
        fn $test_name() {
            let mut msg = extensions_rust_proto::TestExtensions::new();
            assert!(!extensions_rust_proto::$ext.has(&msg));
            assert!(!extensions_rust_proto::$ext.has(msg.as_view()));
            assert!(!extensions_rust_proto::$ext.has(msg.as_mut()));

            assert_eq!(extensions_rust_proto::$ext.get(&msg), $default_val);
            assert_eq!(extensions_rust_proto::$ext.get(msg.as_view()), $default_val);
            assert_eq!(extensions_rust_proto::$ext.get(msg.as_mut()), $default_val);

            extensions_rust_proto::$ext.set(&mut msg, $val);
            assert!(extensions_rust_proto::$ext.has(&msg));
            assert!(extensions_rust_proto::$ext.has(msg.as_view()));
            assert!(extensions_rust_proto::$ext.has(msg.as_mut()));

            assert_eq!(extensions_rust_proto::$ext.get(&msg), $val);
            assert_eq!(extensions_rust_proto::$ext.get(msg.as_view()), $val);
            assert_eq!(extensions_rust_proto::$ext.get(msg.as_mut()), $val);

            extensions_rust_proto::$ext.clear(&mut msg);
            assert!(!extensions_rust_proto::$ext.has(&msg));
            assert!(!extensions_rust_proto::$ext.has(msg.as_mut()));

            assert_eq!(extensions_rust_proto::$ext.get(&msg), $default_val);
            assert_eq!(extensions_rust_proto::$ext.get(msg.as_view()), $default_val);
            assert_eq!(extensions_rust_proto::$ext.get(msg.as_mut()), $default_val);
        }
    };
}

test_scalar_extension!(test_i64_extension, I64_EXTENSION, 42i64, 0i64);
test_scalar_extension!(test_u32_extension, U32_EXTENSION, 42u32, 0u32);
test_scalar_extension!(test_u64_extension, U64_EXTENSION, 42u64, 0u64);
test_scalar_extension!(test_f32_extension, F32_EXTENSION, 42.0f32, 0.0f32);
test_scalar_extension!(test_f64_extension, F64_EXTENSION, 42.0f64, 0.0f64);
test_scalar_extension!(test_bool_extension, BOOL_EXTENSION, true, false);

#[gtest]
fn test_str_extension() {
    assert_eq!(extensions_rust_proto::STR_EXTENSION.number(), 2);
    let mut test_extensions = extensions_rust_proto::TestExtensions::new();
    assert!(!extensions_rust_proto::STR_EXTENSION.has(&test_extensions));
    assert!(!extensions_rust_proto::STR_EXTENSION.has(test_extensions.as_view()));
    assert!(!extensions_rust_proto::STR_EXTENSION.has(test_extensions.as_mut()));

    assert_eq!(extensions_rust_proto::STR_EXTENSION.get(&test_extensions), "");
    assert_eq!(extensions_rust_proto::STR_EXTENSION.get(test_extensions.as_view()), "");
    assert_eq!(extensions_rust_proto::STR_EXTENSION.get(test_extensions.as_mut()), "");

    extensions_rust_proto::STR_EXTENSION.set(&mut test_extensions, "foo");
    assert!(extensions_rust_proto::STR_EXTENSION.has(&test_extensions));
    assert!(extensions_rust_proto::STR_EXTENSION.has(test_extensions.as_view()));
    assert!(extensions_rust_proto::STR_EXTENSION.has(test_extensions.as_mut()));

    assert_eq!(extensions_rust_proto::STR_EXTENSION.get(&test_extensions), "foo");
    assert_eq!(extensions_rust_proto::STR_EXTENSION.get(test_extensions.as_view()), "foo");
    assert_eq!(extensions_rust_proto::STR_EXTENSION.get(test_extensions.as_mut()), "foo");

    extensions_rust_proto::STR_EXTENSION.clear(&mut test_extensions);
    extensions_rust_proto::STR_EXTENSION.clear(test_extensions.as_mut());
    assert!(!extensions_rust_proto::STR_EXTENSION.has(&test_extensions));
    assert!(!extensions_rust_proto::STR_EXTENSION.has(test_extensions.as_mut()));

    assert_eq!(extensions_rust_proto::STR_EXTENSION.get(&test_extensions), "");
    assert_eq!(extensions_rust_proto::STR_EXTENSION.get(test_extensions.as_view()), "");
    assert_eq!(extensions_rust_proto::STR_EXTENSION.get(test_extensions.as_mut()), "");
}

#[gtest]
fn test_serialize_deserialize_i32_extension() {
    let mut test_extensions = extensions_rust_proto::TestExtensions::new();
    extensions_rust_proto::I32_EXTENSION.set(&mut test_extensions, 42);
    extensions_separate_file_rust_proto::I32_EXTENSION_SEPARATE_FILE.set(&mut test_extensions, 84);

    let serialized = test_extensions.serialize().unwrap();
    let test_extensions_deserialized =
        extensions_rust_proto::TestExtensions::parse(&serialized).unwrap();

    assert_eq!(extensions_rust_proto::I32_EXTENSION.get(&test_extensions_deserialized), 42);
    assert_eq!(
        extensions_separate_file_rust_proto::I32_EXTENSION_SEPARATE_FILE
            .get(&test_extensions_deserialized),
        84
    );
}

#[gtest]
fn test_defaults() {
    let mut test_extensions = extensions_rust_proto::TestExtensions::new();

    // get (default empty view)
    assert_eq!(extensions_rust_proto::REPEATED_I32_EXTENSION.get(&test_extensions).len(), 0);
    assert_eq!(extensions_rust_proto::I32_EXTENSION.get(test_extensions.as_view()), 0);
    assert_eq!(extensions_rust_proto::I32_EXTENSION.get(test_extensions.as_mut()), 0);

    assert_eq!(extensions_rust_proto::I32_EXTENSION_WITH_DEFAULT.get(&test_extensions), 100);
    assert_eq!(
        extensions_rust_proto::I32_EXTENSION_WITH_DEFAULT.get(test_extensions.as_view()),
        100
    );
    assert_eq!(
        extensions_rust_proto::I32_EXTENSION_WITH_DEFAULT.get(test_extensions.as_mut()),
        100
    );
}

#[gtest]
fn test_nested_extension() {
    let mut test_extensions = extensions_rust_proto::TestExtensions::new();
    assert!(!extensions_rust_proto::test_extensions::NESTED_EXTENSION.has(&test_extensions));
    extensions_rust_proto::test_extensions::NESTED_EXTENSION.set(&mut test_extensions, 42);
    assert!(extensions_rust_proto::test_extensions::NESTED_EXTENSION.has(&test_extensions));
    assert_eq!(extensions_rust_proto::test_extensions::NESTED_EXTENSION.get(&test_extensions), 42);
}

#[gtest]
fn test_submessage_extension() {
    let mut test_extensions = extensions_rust_proto::TestExtensions::new();
    assert!(!extensions_rust_proto::SUBMESSAGE_EXTENSION.has(&test_extensions));
    assert_eq!(extensions_rust_proto::SUBMESSAGE_EXTENSION.get(&test_extensions).i32_field(), 123);

    extensions_rust_proto::SUBMESSAGE_EXTENSION.get_mut(&mut test_extensions).set_i32_field(42);
    assert_eq!(extensions_rust_proto::SUBMESSAGE_EXTENSION.get(&test_extensions).i32_field(), 42);

    extensions_rust_proto::SUBMESSAGE_EXTENSION.get_mut(&mut test_extensions).set_i32_field(84);
    assert_eq!(extensions_rust_proto::SUBMESSAGE_EXTENSION.get(&test_extensions).i32_field(), 84);

    extensions_rust_proto::SUBMESSAGE_EXTENSION.clear(&mut test_extensions);
    extensions_rust_proto::SUBMESSAGE_EXTENSION.clear(test_extensions.as_mut());
    assert!(!extensions_rust_proto::SUBMESSAGE_EXTENSION.has(&test_extensions));
    assert_eq!(extensions_rust_proto::SUBMESSAGE_EXTENSION.get(&test_extensions).i32_field(), 123);
}

#[gtest]
fn test_repeated_extension() {
    let mut test_extensions = extensions_rust_proto::TestExtensions::new();

    // get (default empty view)
    assert_eq!(extensions_rust_proto::REPEATED_I32_EXTENSION.get(&test_extensions).len(), 0);

    // get_mut (creates array)
    {
        let mut rep_mut =
            extensions_rust_proto::REPEATED_I32_EXTENSION.get_mut(&mut test_extensions);
        rep_mut.push(1);
        rep_mut.push(2);
    }
    assert_eq!(extensions_rust_proto::REPEATED_I32_EXTENSION.get(&test_extensions).len(), 2);
    assert_eq!(extensions_rust_proto::REPEATED_I32_EXTENSION.get(&test_extensions).get(0), Some(1));
    assert_eq!(extensions_rust_proto::REPEATED_I32_EXTENSION.get(&test_extensions).get(1), Some(2));

    // set
    let rep = [3, 4, 5].into_iter().into_proxied(protobuf::__internal::Private);
    extensions_rust_proto::REPEATED_I32_EXTENSION.set(&mut test_extensions, rep);
    assert_eq!(extensions_rust_proto::REPEATED_I32_EXTENSION.get(&test_extensions).len(), 3);
    assert_eq!(extensions_rust_proto::REPEATED_I32_EXTENSION.get(&test_extensions).get(0), Some(3));
    assert_eq!(extensions_rust_proto::REPEATED_I32_EXTENSION.get(&test_extensions).get(2), Some(5));

    // clear
    extensions_rust_proto::REPEATED_I32_EXTENSION.clear(&mut test_extensions);
    assert_eq!(extensions_rust_proto::REPEATED_I32_EXTENSION.get(&test_extensions).len(), 0);

    // No has() extension method for repeated extensions by design

    // test string repeated extension
    {
        let mut rep_mut =
            extensions_rust_proto::REPEATED_STR_EXTENSION.get_mut(&mut test_extensions);
        rep_mut.push("hello");
        rep_mut.push("world");
    }
    assert_eq!(extensions_rust_proto::REPEATED_STR_EXTENSION.get(&test_extensions).len(), 2);
    assert_eq!(
        extensions_rust_proto::REPEATED_STR_EXTENSION.get(&test_extensions).get(1).unwrap(),
        "world"
    );
    extensions_rust_proto::REPEATED_STR_EXTENSION.clear(&mut test_extensions);
    assert_eq!(extensions_rust_proto::REPEATED_STR_EXTENSION.get(&test_extensions).len(), 0);

    // test submessage repeated extension
    {
        let mut rep_mut =
            extensions_rust_proto::REPEATED_SUBMESSAGE_EXTENSION.get_mut(&mut test_extensions);
        let mut msg1 = extensions_rust_proto::SimpleSubmessage::new();
        msg1.set_i32_field(10);
        rep_mut.push(msg1);
    }
    assert_eq!(extensions_rust_proto::REPEATED_SUBMESSAGE_EXTENSION.get(&test_extensions).len(), 1);
    assert_eq!(
        extensions_rust_proto::REPEATED_SUBMESSAGE_EXTENSION
            .get(&test_extensions)
            .get(0)
            .unwrap()
            .i32_field(),
        10
    );
    extensions_rust_proto::REPEATED_SUBMESSAGE_EXTENSION.clear(&mut test_extensions);
    assert_eq!(extensions_rust_proto::REPEATED_SUBMESSAGE_EXTENSION.get(&test_extensions).len(), 0);
}

#[gtest]
fn test_generic_extension() {
    #[allow(unused)]
    trait ExtensionTypes {
        type E: protobuf::Message;
        type M: protobuf::Message;
        const MESSAGE_EXTENSION: protobuf::ExtensionId<Self::E, Self::M>;
    }

    #[allow(unused)]
    struct TestExtensionTypes;

    impl ExtensionTypes for TestExtensionTypes {
        type E = extensions_rust_proto::TestExtensions;
        type M = extensions_rust_proto::SimpleSubmessage;
        const MESSAGE_EXTENSION: protobuf::ExtensionId<Self::E, Self::M> =
            extensions_rust_proto::SUBMESSAGE_EXTENSION;
    }

    // This generic function failed to compile before the fix.
    // It requires internal bounds for now but no longer needs complex Extension bounds.
    fn get_extension_generic<'a, M, E>(
        msg: &'a mut M,
        ext: &protobuf::ExtensionId<M, E>,
    ) -> protobuf::View<'a, E>
    where
        M: protobuf::Message,
        E: protobuf::Message,
    {
        ext.has(&mut *msg);
        ext.has(msg.as_view());
        ext.has(msg.as_mut());
        ext.get(&mut *msg);
        ext.get(msg.as_view());
        ext.get(msg.as_mut())
    }

    fn get_mut_extension_generic<'a, M, E>(
        msg: &'a mut M,
        ext: &protobuf::ExtensionId<M, E>,
    ) -> protobuf::Mut<'a, E>
    where
        M: protobuf::Message,
        E: protobuf::Message,
    {
        ext.clear(&mut *msg);
        ext.clear(msg.as_mut());
        ext.set(&mut *msg, E::default());
        ext.set(msg.as_mut(), E::default());
        ext.get_mut(&mut *msg);
        ext.get_mut(msg.as_mut())
    }

    let mut test_extensions = extensions_rust_proto::TestExtensions::new();
    TestExtensionTypes::MESSAGE_EXTENSION.get_mut(&mut test_extensions).set_i32_field(99);

    let submsg =
        get_extension_generic(&mut test_extensions, &TestExtensionTypes::MESSAGE_EXTENSION);
    assert_eq!(submsg.i32_field(), 99);
    let _mut_submsg =
        get_mut_extension_generic(&mut test_extensions, &TestExtensionTypes::MESSAGE_EXTENSION);
}

#[gtest]
fn test_multiple_gets_with_same_view() {
    let mut test_extensions = extensions_rust_proto::TestExtensions::new();
    extensions_rust_proto::I32_EXTENSION.set(&mut test_extensions, 42);
    extensions_rust_proto::STR_EXTENSION.set(&mut test_extensions, "foo");

    let view = test_extensions.as_view();

    // Passing `view` by value multiple times works because it's a `View` (Copy).
    let v1 = extensions_rust_proto::I32_EXTENSION.get(view);
    let v2 = extensions_rust_proto::STR_EXTENSION.get(view);

    assert_eq!(v1, 42);
    assert_eq!(v2, "foo");
}

#[gtest]
fn test_merge_from_with_extension() {
    let mut src = extensions_rust_proto::TestExtensions::new();
    extensions_rust_proto::I32_EXTENSION.set(&mut src, 42);
    extensions_rust_proto::STR_EXTENSION.set(&mut src, "foo");

    let mut dst = extensions_rust_proto::TestExtensions::new();
    dst.merge_from(src.as_view());

    assert!(extensions_rust_proto::I32_EXTENSION.has(&dst));
    assert_eq!(extensions_rust_proto::I32_EXTENSION.get(&dst), 42);

    assert!(extensions_rust_proto::STR_EXTENSION.has(&dst));
    assert_eq!(extensions_rust_proto::STR_EXTENSION.get(&dst), "foo");
}
