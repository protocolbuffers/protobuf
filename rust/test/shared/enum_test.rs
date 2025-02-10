// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

//! Tests covering enum type generation.

use enums_rust_proto::*;
use googletest::prelude::*;
use protobuf::Enum;
use unittest_rust_proto::*;

#[gtest]
fn test_nested_enum_values() {
    assert_that!(i32::from(test_all_types::NestedEnum::Foo), eq(1));
    assert_that!(i32::from(test_all_types::NestedEnum::Bar), eq(2));
    assert_that!(i32::from(test_all_types::NestedEnum::Baz), eq(3));
    assert_that!(i32::from(test_all_types::NestedEnum::Neg), eq(-1));
}

#[gtest]
fn test_isolated_nested_enum() {
    // Ensure that the enum is generated even when it's the only nested type for the
    // message.
    assert_that!(i32::from(test_required_enum_no_mask::NestedEnum::Foo), eq(2));
}

#[gtest]
fn test_enum_value_name_same_as_enum() {
    assert_that!(i32::from(TestEnumValueNameSameAsEnum::TestEnumValueNameSameAsEnum), eq(1));
}

#[gtest]
fn test_enum_defaults() {
    assert_that!(TestSparseEnum::default(), eq(TestSparseEnum::SparseA));
    assert_that!(TestEnumWithDupValue::default(), eq(TestEnumWithDupValue::Foo1));
    assert_that!(TestEnumWithDupValue::default(), eq(TestEnumWithDupValue::Foo2));
    assert_that!(
        TestEnumWithDuplicateStrippedPrefixNames::default(),
        eq(TestEnumWithDuplicateStrippedPrefixNames::Unknown)
    );
    assert_that!(test_all_types::NestedEnum::default(), eq(test_all_types::NestedEnum::Foo));
}

#[gtest]
#[deny(unreachable_patterns)]
#[allow(clippy::let_unit_value)]
fn test_closed_enum_is_nonexhaustive() {
    let val = ForeignEnum::ForeignFoo;
    let _it_compiles: () = match val {
        ForeignEnum::ForeignFoo => (),
        ForeignEnum::ForeignBar => (),
        ForeignEnum::ForeignBaz => (),
        ForeignEnum::ForeignBax => (),
        _ => unreachable!(),
    };
}

#[gtest]
fn test_closed_enum_conversion() {
    assert_that!(i32::from(TestSparseEnum::SparseA), eq(123));
    assert_that!(TestSparseEnum::try_from(123), ok(eq(&TestSparseEnum::SparseA)));

    assert_that!(i32::from(TestSparseEnum::SparseD), eq(-15));
    assert_that!(TestSparseEnum::try_from(-15), ok(eq(&TestSparseEnum::SparseD)));

    assert_that!(TestSparseEnum::try_from(0), ok(eq(&TestSparseEnum::SparseF)));
    assert_that!(TestSparseEnum::try_from(1), err(anything()));
}

#[gtest]
fn test_closed_aliased_enum_conversion() {
    assert_that!(i32::from(TestEnumWithDupValue::Foo1), eq(1));
    assert_that!(i32::from(TestEnumWithDupValue::Foo2), eq(1));
    assert_that!(i32::from(TestEnumWithDupValue::Bar1), eq(2));
    assert_that!(i32::from(TestEnumWithDupValue::Bar2), eq(2));
    assert_that!(i32::from(TestEnumWithDupValue::Baz), eq(3));

    assert_that!(TestEnumWithDupValue::try_from(1), ok(eq(&TestEnumWithDupValue::Foo1)));
    assert_that!(TestEnumWithDupValue::try_from(2), ok(eq(&TestEnumWithDupValue::Bar1)));
    assert_that!(TestEnumWithDupValue::try_from(3), ok(eq(&TestEnumWithDupValue::Baz)));
    assert_that!(TestEnumWithDupValue::try_from(0), err(anything()));
    assert_that!(TestEnumWithDupValue::try_from(4), err(anything()));

    assert_that!(TestEnumWithDupValue::Foo1, eq(TestEnumWithDupValue::Foo2));
    assert_that!(TestEnumWithDupValue::Bar1, eq(TestEnumWithDupValue::Bar2));
}

#[gtest]
#[deny(unreachable_patterns)]
#[allow(clippy::let_unit_value)]
fn test_open_enum_is_nonexhaustive() {
    let val = TestEnumValueNameSameAsEnum::Unknown;
    let _it_compiles: () = match val {
        TestEnumValueNameSameAsEnum::Unknown => (),
        TestEnumValueNameSameAsEnum::TestEnumValueNameSameAsEnum => (),
        _ => unreachable!(),
    };
}

#[gtest]
fn test_open_enum_conversion() {
    assert_that!(i32::from(TestEnumWithNumericNames::Unknown), eq(0));
    assert_that!(i32::from(TestEnumWithNumericNames::_2020), eq(1));
    assert_that!(i32::from(TestEnumWithNumericNames::_2021), eq(2));
    assert_that!(i32::from(TestEnumWithNumericNames::_2022), eq(3));

    assert_that!(TestEnumWithNumericNames::from(0), eq(TestEnumWithNumericNames::Unknown));
    assert_that!(TestEnumWithNumericNames::from(1), eq(TestEnumWithNumericNames::_2020));
    assert_that!(TestEnumWithNumericNames::from(2), eq(TestEnumWithNumericNames::_2021));
    assert_that!(TestEnumWithNumericNames::from(3), eq(TestEnumWithNumericNames::_2022));
    assert_that!(
        TestEnumWithNumericNames::from(4),
        not(any![
            eq(TestEnumWithNumericNames::Unknown),
            eq(TestEnumWithNumericNames::_2020),
            eq(TestEnumWithNumericNames::_2021),
            eq(TestEnumWithNumericNames::_2022),
        ])
    );
    assert_that!(i32::from(TestEnumWithNumericNames::from(-1)), eq(-1));
}

#[gtest]
fn test_open_aliased_enum_conversion() {
    assert_that!(i32::from(TestEnumWithDuplicateStrippedPrefixNames::Unknown), eq(0));
    assert_that!(i32::from(TestEnumWithDuplicateStrippedPrefixNames::Foo), eq(1));
    assert_that!(i32::from(TestEnumWithDuplicateStrippedPrefixNames::Bar), eq(2));
    assert_that!(i32::from(TestEnumWithDuplicateStrippedPrefixNames::DifferentNameAlias), eq(2));

    assert_that!(
        TestEnumWithDuplicateStrippedPrefixNames::from(0),
        eq(TestEnumWithDuplicateStrippedPrefixNames::Unknown)
    );
    assert_that!(
        TestEnumWithDuplicateStrippedPrefixNames::from(1),
        eq(TestEnumWithDuplicateStrippedPrefixNames::Foo)
    );
    assert_that!(
        TestEnumWithDuplicateStrippedPrefixNames::from(2),
        eq(TestEnumWithDuplicateStrippedPrefixNames::Bar)
    );
    assert_that!(
        TestEnumWithDuplicateStrippedPrefixNames::from(2),
        eq(TestEnumWithDuplicateStrippedPrefixNames::DifferentNameAlias)
    );
    assert_that!(
        TestEnumWithDuplicateStrippedPrefixNames::from(3),
        not(any![
            eq(TestEnumWithDuplicateStrippedPrefixNames::Unknown),
            eq(TestEnumWithDuplicateStrippedPrefixNames::Foo),
            eq(TestEnumWithDuplicateStrippedPrefixNames::Bar),
        ])
    );
    assert_that!(i32::from(TestEnumWithDuplicateStrippedPrefixNames::from(5)), eq(5));
}

#[gtest]
fn test_enum_conversion_failure_display() {
    let err = TestSparseEnum::try_from(1).unwrap_err();
    assert_that!(format!("{err}"), eq("1 is not a known value for TestSparseEnum"));
}

#[gtest]
fn test_enum_conversion_failure_impls_std_error() {
    let err = TestSparseEnum::try_from(1).unwrap_err();
    let _test_compiles: &dyn std::error::Error = &err;
}

#[gtest]
fn test_is_known_for_closed_enum() {
    assert_that!(test_all_types::NestedEnum::is_known(-2), eq(false));
    assert_that!(test_all_types::NestedEnum::is_known(-1), eq(true));
    assert_that!(test_all_types::NestedEnum::is_known(0), eq(false));
    assert_that!(test_all_types::NestedEnum::is_known(1), eq(true));
    assert_that!(test_all_types::NestedEnum::is_known(2), eq(true));
    assert_that!(test_all_types::NestedEnum::is_known(3), eq(true));
    assert_that!(test_all_types::NestedEnum::is_known(4), eq(false));
}

#[gtest]
fn test_is_known_for_open_enum() {
    assert_that!(TestEnumWithNumericNames::is_known(-1), eq(false));
    assert_that!(TestEnumWithNumericNames::is_known(0), eq(true));
    assert_that!(TestEnumWithNumericNames::is_known(1), eq(true));
    assert_that!(TestEnumWithNumericNames::is_known(2), eq(true));
    assert_that!(TestEnumWithNumericNames::is_known(3), eq(true));
    assert_that!(TestEnumWithNumericNames::is_known(4), eq(false));
}

#[gtest]
fn test_debug_string() {
    assert_that!(
        format!("{:?}", { TestEnumWithNumericNames::_2020 }),
        eq("TestEnumWithNumericNames::_2020")
    );

    assert_that!(
        format!("{:?}", { TestEnumWithNumericNames::from(2) }),
        eq("TestEnumWithNumericNames::_2021")
    );

    // There is no name for 42, so the integer number is printed instead:
    assert_that!(
        format!("{:?}", { TestEnumWithNumericNames::from(42) }),
        eq("TestEnumWithNumericNames::from(42)")
    );
}

#[gtest]
fn test_enum_in_hash_set() {
    use test_all_types::NestedEnum;
    let mut s = std::collections::HashSet::<NestedEnum>::new();
    s.insert(NestedEnum::Foo);
    s.insert(NestedEnum::Bar);
    s.insert(NestedEnum::try_from(1).unwrap()); // FOO = 1
    assert_that!(s.len(), eq(2));
    assert_that!(s.contains(&NestedEnum::Bar), eq(true));
    assert_that!(s.contains(&NestedEnum::Baz), eq(false));
}

#[gtest]
fn test_enum_in_btree() {
    use test_all_types::NestedEnum;
    let mut s = std::collections::BTreeMap::<NestedEnum, i32>::new();

    s.insert(NestedEnum::Baz, 1);
    s.insert(NestedEnum::Bar, 2);
    s.insert(NestedEnum::Foo, 3);
    s.insert(NestedEnum::try_from(1).unwrap(), 4); // FOO = 1

    // The order of entries should be sorted by the enum constant value, which is
    // ordered FOO < BAR < BAZ.
    assert_that!(s.pop_first(), some(eq((NestedEnum::Foo, 4))));
    assert_that!(s.pop_first(), some(eq((NestedEnum::Bar, 2))));
    assert_that!(s.pop_first(), some(eq((NestedEnum::Baz, 1))));
    assert_that!(s.pop_first(), none());
}
