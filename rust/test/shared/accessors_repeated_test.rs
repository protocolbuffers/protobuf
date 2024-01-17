// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use googletest::prelude::*;
use paste::paste;
use unittest_proto::proto2_unittest::{TestAllTypes, TestAllTypes_, TestAllTypes_::NestedMessage};

macro_rules! generate_repeated_numeric_test {
  ($(($t: ty, $field: ident)),*) => {
      paste! { $(
          #[test]
          fn [< test_repeated_ $field _accessors >]() {
              let mut msg = TestAllTypes::new();
              assert_that!(msg.[< repeated_ $field >]().len(), eq(0));
              assert_that!(msg.[<repeated_ $field >]().get(0), none());

              let mut mutator = msg.[<repeated_ $field _mut >]();
              mutator.push(1 as $t);
              assert_that!(mutator.len(), eq(1));
              assert_that!(mutator.iter().len(), eq(1));
              assert_that!(mutator.get(0), some(eq(1 as $t)));
              mutator.set(0, 2 as $t);
              assert_that!(mutator.get(0), some(eq(2 as $t)));
              mutator.push(1 as $t);

              mutator.push(3 as $t);
              mutator.set(2, 4 as $t);
              assert_that!(mutator.get(2), some(eq(4 as $t)));
              mutator.set(2, 0 as $t);

              assert_that!(
                  mutator.iter().collect::<Vec<_>>(),
                  elements_are![eq(2 as $t), eq(1 as $t), eq(0 as $t)]
              );
              assert_that!(
                  (*mutator).into_iter().collect::<Vec<_>>(),
                  elements_are![eq(2 as $t), eq(1 as $t), eq(0 as $t)]
              );

              for i in 0..mutator.len() {
                  mutator.set(i, 0 as $t);
              }
              assert_that!(
                  msg.[<repeated_ $field _mut >]().iter().collect::<Vec<_>>(),
                  each(eq(0 as $t))
              );
          }

          #[test]
          fn [< test_repeated_ $field _set >]() {
              let mut msg = TestAllTypes::new();
              let mut mutator = msg.[<repeated_ $field _mut>]();
              let mut msg2 = TestAllTypes::new();
              let mut mutator2 = msg2.[<repeated_ $field _mut>]();
              for i in 0..5 {
                  mutator2.push(i as $t);
              }
              protobuf::MutProxy::set(&mut mutator, *mutator2);

              assert_that!(
                  mutator.iter().collect::<Vec<_>>(),
                  eq(mutator2.iter().collect::<Vec<_>>())
              );
          }
      )* }
  };
}

generate_repeated_numeric_test!(
    (i32, int32),
    (u32, uint32),
    (i64, int64),
    (u64, uint64),
    (f32, float),
    (f64, double)
);

#[test]
fn test_repeated_bool_accessors() {
    let mut msg = TestAllTypes::new();
    assert_that!(msg.repeated_bool().len(), eq(0));
    assert_that!(msg.repeated_bool().get(0), none());

    let mut mutator = msg.repeated_bool_mut();
    mutator.push(true);
    assert_that!(mutator.len(), eq(1));
    assert_that!(mutator.get(0), some(eq(true)));
    mutator.set(0, false);
    assert_that!(mutator.get(0), some(eq(false)));
    mutator.push(true);

    mutator.push(false);
    mutator.set(2, true);
    assert_that!(mutator.get(2), some(eq(true)));
    mutator.set(2, false);
    assert_that!(mutator.get(2), some(eq(false)));

    assert_that!(mutator.iter().collect::<Vec<_>>(), elements_are![eq(false), eq(true), eq(false)]);
    assert_that!(
        (*mutator).into_iter().collect::<Vec<_>>(),
        elements_are![eq(false), eq(true), eq(false)]
    );

    for i in 0..mutator.len() {
        mutator.set(i, false);
    }
    assert_that!(msg.repeated_bool().iter().collect::<Vec<_>>(), each(eq(false)));
}

#[test]
fn test_repeated_enum_accessors() {
    use TestAllTypes_::NestedEnum;

    let mut msg = TestAllTypes::new();
    assert_that!(msg.repeated_nested_enum().len(), eq(0));
    assert_that!(msg.repeated_nested_enum().get(0), none());

    let mut mutator = msg.repeated_nested_enum_mut();
    mutator.push(NestedEnum::Foo);
    assert_that!(mutator.len(), eq(1));
    assert_that!(mutator.get(0), some(eq(NestedEnum::Foo)));
    mutator.set(0, NestedEnum::Bar);
    assert_that!(mutator.get(0), some(eq(NestedEnum::Bar)));
    mutator.push(NestedEnum::Baz);

    mutator.push(NestedEnum::Foo);
    mutator.set(2, NestedEnum::Neg);
    assert_that!(mutator.get(2), some(eq(NestedEnum::Neg)));
    mutator.set(2, NestedEnum::Foo);
    assert_that!(mutator.get(2), some(eq(NestedEnum::Foo)));

    assert_that!(
        mutator.iter().collect::<Vec<_>>(),
        elements_are![eq(NestedEnum::Bar), eq(NestedEnum::Baz), eq(NestedEnum::Foo)]
    );
    assert_that!(
        (*mutator).into_iter().collect::<Vec<_>>(),
        elements_are![eq(NestedEnum::Bar), eq(NestedEnum::Baz), eq(NestedEnum::Foo)]
    );

    for i in 0..mutator.len() {
        mutator.set(i, NestedEnum::Foo);
    }
    assert_that!(msg.repeated_nested_enum().iter().collect::<Vec<_>>(), each(eq(NestedEnum::Foo)));
}

#[test]
fn test_repeated_bool_set() {
    let mut msg = TestAllTypes::new();
    let mut mutator = msg.repeated_bool_mut();
    let mut msg2 = TestAllTypes::new();
    let mut mutator2 = msg2.repeated_bool_mut();
    for _ in 0..5 {
        mutator2.push(true);
    }
    protobuf::MutProxy::set(&mut mutator, *mutator2);

    assert_that!(mutator.iter().collect::<Vec<_>>(), eq(mutator2.iter().collect::<Vec<_>>()));
}

#[test]
fn test_repeated_message() {
    let mut msg = TestAllTypes::new();
    assert_that!(msg.repeated_nested_message().len(), eq(0));
    let mut nested = NestedMessage::new();
    nested.bb_mut().set(1);
    msg.repeated_nested_message_mut().push(nested.as_view());
    assert_that!(msg.repeated_nested_message().get(0).unwrap().bb(), eq(1));

    let mut msg2 = TestAllTypes::new();
    msg2.repeated_nested_message_mut().copy_from(msg.repeated_nested_message());
    assert_that!(msg2.repeated_nested_message().get(0).unwrap().bb(), eq(1));

    msg2.repeated_nested_message_mut().clear();
    assert_that!(msg2.repeated_nested_message().len(), eq(0));

    let mut nested2 = NestedMessage::new();
    nested2.bb_mut().set(2);
    msg.repeated_nested_message_mut().set(0, nested2.as_view());
    assert_that!(msg.repeated_nested_message().get(0).unwrap().bb(), eq(2));

    assert_that!(
        msg.repeated_nested_message().iter().map(|m| m.bb()).collect::<Vec<_>>(),
        eq(vec![2]),
    );
}
