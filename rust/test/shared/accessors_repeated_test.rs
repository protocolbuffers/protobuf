// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use googletest::prelude::*;
use paste::paste;
use unittest_proto::proto2_unittest::TestAllTypes;

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
              assert_that!(mutator.get(0), some(eq(1 as $t)));
              mutator.set(0, 2 as $t);
              assert_that!(mutator.get(0), some(eq(2 as $t)));
              mutator.push(1 as $t);

              mutator.push(3 as $t);
              assert_that!(mutator.get_mut(2).is_some(), eq(true));
              let mut mut_elem = mutator.get_mut(2).unwrap();
              mut_elem.set(4 as $t);
              assert_that!(mut_elem.get(), eq(4 as $t));
              mut_elem.clear();
              assert_that!(mut_elem.get(), eq(0 as $t));

              assert_that!(
                  mutator.iter().collect::<Vec<_>>(),
                  eq(vec![2 as $t, 1 as $t, 0 as $t])
              );
              assert_that!(
                  (*mutator).into_iter().collect::<Vec<_>>(),
                  eq(vec![2 as $t, 1 as $t, 0 as $t])
              );

              for mut mutable_elem in msg.[<repeated_ $field _mut >]() {
                  mutable_elem.set(0 as $t);
              }
              assert_that!(
                  msg.[<repeated_ $field _mut >]().iter().all(|v| v == (0 as $t)),
                  eq(true)
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
    assert_that!(mutator.get_mut(2), pat!(Some(_)));
    let mut mut_elem = mutator.get_mut(2).unwrap();
    mut_elem.set(true);
    assert_that!(mut_elem.get(), eq(true));
    mut_elem.clear();
    assert_that!(mut_elem.get(), eq(false));

    assert_that!(mutator.iter().collect::<Vec<_>>(), eq(vec![false, true, false]));
    assert_that!((*mutator).into_iter().collect::<Vec<_>>(), eq(vec![false, true, false]));

    for mut mutable_elem in msg.repeated_bool_mut() {
        mutable_elem.set(false);
    }
    assert_that!(msg.repeated_bool().iter().all(|v| v), eq(false));
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
