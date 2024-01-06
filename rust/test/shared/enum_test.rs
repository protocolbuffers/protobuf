// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

//! Tests covering enum type generation.

use enums_proto::enums;
use googletest::prelude::*;
use unittest_proto::proto2_unittest;

#[test]
fn test_nested_enum_values() {
    assert_that!(i32::from(proto2_unittest::TestAllTypes_::NestedEnum::Foo), eq(1));
    assert_that!(i32::from(proto2_unittest::TestAllTypes_::NestedEnum::Bar), eq(2));
    assert_that!(i32::from(proto2_unittest::TestAllTypes_::NestedEnum::Baz), eq(3));
    assert_that!(i32::from(proto2_unittest::TestAllTypes_::NestedEnum::Neg), eq(-1));
}

#[test]
fn test_isolated_nested_enum() {
    // Ensure that the enum is generated even when it's the only nested type for the
    // message.
    assert_that!(i32::from(proto2_unittest::TestRequiredEnumNoMask_::NestedEnum::Foo), eq(2));
}

#[test]
fn test_enum_value_name_same_as_enum() {
    assert_that!(i32::from(enums::TestEnumValueNameSameAsEnum::TestEnumValueNameSameAsEnum), eq(1));
}

#[test]
fn test_enum_defaults() {
    assert_that!(
        proto2_unittest::TestSparseEnum::default(),
        eq(proto2_unittest::TestSparseEnum::SparseA)
    );
    assert_that!(
        proto2_unittest::TestEnumWithDupValue::default(),
        eq(proto2_unittest::TestEnumWithDupValue::Foo1)
    );
    assert_that!(
        proto2_unittest::TestEnumWithDupValue::default(),
        eq(proto2_unittest::TestEnumWithDupValue::Foo2)
    );
    assert_that!(
        enums::TestEnumWithDuplicateStrippedPrefixNames::default(),
        eq(enums::TestEnumWithDuplicateStrippedPrefixNames::Unknown)
    );
    assert_that!(
        proto2_unittest::TestAllTypes_::NestedEnum::default(),
        eq(proto2_unittest::TestAllTypes_::NestedEnum::Foo)
    );
}

#[test]
#[deny(unreachable_patterns)]
#[allow(clippy::let_unit_value)]
fn test_closed_enum_is_nonexhaustive() {
    let val = proto2_unittest::ForeignEnum::ForeignFoo;
    let _it_compiles: () = match val {
        proto2_unittest::ForeignEnum::ForeignFoo => (),
        proto2_unittest::ForeignEnum::ForeignBar => (),
        proto2_unittest::ForeignEnum::ForeignBaz => (),
        proto2_unittest::ForeignEnum::ForeignBax => (),
        _ => unreachable!(),
    };
}

#[test]
fn test_closed_enum_conversion() {
    assert_that!(i32::from(proto2_unittest::TestSparseEnum::SparseA), eq(123));
    assert_that!(
        proto2_unittest::TestSparseEnum::try_from(123),
        ok(eq(proto2_unittest::TestSparseEnum::SparseA))
    );

    assert_that!(i32::from(proto2_unittest::TestSparseEnum::SparseD), eq(-15));
    assert_that!(
        proto2_unittest::TestSparseEnum::try_from(-15),
        ok(eq(proto2_unittest::TestSparseEnum::SparseD))
    );

    assert_that!(
        proto2_unittest::TestSparseEnum::try_from(0),
        ok(eq(proto2_unittest::TestSparseEnum::SparseF))
    );
    assert_that!(proto2_unittest::TestSparseEnum::try_from(1), err(anything()));
}

#[test]
fn test_closed_aliased_enum_conversion() {
    assert_that!(i32::from(proto2_unittest::TestEnumWithDupValue::Foo1), eq(1));
    assert_that!(i32::from(proto2_unittest::TestEnumWithDupValue::Foo2), eq(1));
    assert_that!(i32::from(proto2_unittest::TestEnumWithDupValue::Bar1), eq(2));
    assert_that!(i32::from(proto2_unittest::TestEnumWithDupValue::Bar2), eq(2));
    assert_that!(i32::from(proto2_unittest::TestEnumWithDupValue::Baz), eq(3));

    assert_that!(
        proto2_unittest::TestEnumWithDupValue::try_from(1),
        ok(eq(proto2_unittest::TestEnumWithDupValue::Foo1))
    );
    assert_that!(
        proto2_unittest::TestEnumWithDupValue::try_from(2),
        ok(eq(proto2_unittest::TestEnumWithDupValue::Bar1))
    );
    assert_that!(
        proto2_unittest::TestEnumWithDupValue::try_from(3),
        ok(eq(proto2_unittest::TestEnumWithDupValue::Baz))
    );
    assert_that!(proto2_unittest::TestEnumWithDupValue::try_from(0), err(anything()));
    assert_that!(proto2_unittest::TestEnumWithDupValue::try_from(4), err(anything()));

    assert_that!(
        proto2_unittest::TestEnumWithDupValue::Foo1,
        eq(proto2_unittest::TestEnumWithDupValue::Foo2)
    );
    assert_that!(
        proto2_unittest::TestEnumWithDupValue::Bar1,
        eq(proto2_unittest::TestEnumWithDupValue::Bar2)
    );
}

#[test]
#[deny(unreachable_patterns)]
#[allow(clippy::let_unit_value)]
fn test_open_enum_is_nonexhaustive() {
    let val = enums::TestEnumValueNameSameAsEnum::Unknown;
    let _it_compiles: () = match val {
        enums::TestEnumValueNameSameAsEnum::Unknown => (),
        enums::TestEnumValueNameSameAsEnum::TestEnumValueNameSameAsEnum => (),
        _ => unreachable!(),
    };
}

#[test]
fn test_open_enum_conversion() {
    assert_that!(i32::from(enums::TestEnumWithNumericNames::Unknown), eq(0));
    assert_that!(i32::from(enums::TestEnumWithNumericNames::_2020), eq(1));
    assert_that!(i32::from(enums::TestEnumWithNumericNames::_2021), eq(2));
    assert_that!(i32::from(enums::TestEnumWithNumericNames::_2022), eq(3));

    assert_that!(
        enums::TestEnumWithNumericNames::from(0),
        eq(enums::TestEnumWithNumericNames::Unknown)
    );
    assert_that!(
        enums::TestEnumWithNumericNames::from(1),
        eq(enums::TestEnumWithNumericNames::_2020)
    );
    assert_that!(
        enums::TestEnumWithNumericNames::from(2),
        eq(enums::TestEnumWithNumericNames::_2021)
    );
    assert_that!(
        enums::TestEnumWithNumericNames::from(3),
        eq(enums::TestEnumWithNumericNames::_2022)
    );
    assert_that!(
        enums::TestEnumWithNumericNames::from(4),
        not(any![
            eq(enums::TestEnumWithNumericNames::Unknown),
            eq(enums::TestEnumWithNumericNames::_2020),
            eq(enums::TestEnumWithNumericNames::_2021),
            eq(enums::TestEnumWithNumericNames::_2022),
        ])
    );
    assert_that!(i32::from(enums::TestEnumWithNumericNames::from(-1)), eq(-1));
}

#[test]
fn test_open_aliased_enum_conversion() {
    assert_that!(i32::from(enums::TestEnumWithDuplicateStrippedPrefixNames::Unknown), eq(0));
    assert_that!(i32::from(enums::TestEnumWithDuplicateStrippedPrefixNames::Foo), eq(1));
    assert_that!(i32::from(enums::TestEnumWithDuplicateStrippedPrefixNames::Bar), eq(2));
    assert_that!(
        i32::from(enums::TestEnumWithDuplicateStrippedPrefixNames::DifferentNameAlias),
        eq(2)
    );

    assert_that!(
        enums::TestEnumWithDuplicateStrippedPrefixNames::from(0),
        eq(enums::TestEnumWithDuplicateStrippedPrefixNames::Unknown)
    );
    assert_that!(
        enums::TestEnumWithDuplicateStrippedPrefixNames::from(1),
        eq(enums::TestEnumWithDuplicateStrippedPrefixNames::Foo)
    );
    assert_that!(
        enums::TestEnumWithDuplicateStrippedPrefixNames::from(2),
        eq(enums::TestEnumWithDuplicateStrippedPrefixNames::Bar)
    );
    assert_that!(
        enums::TestEnumWithDuplicateStrippedPrefixNames::from(2),
        eq(enums::TestEnumWithDuplicateStrippedPrefixNames::DifferentNameAlias)
    );
    assert_that!(
        enums::TestEnumWithDuplicateStrippedPrefixNames::from(3),
        not(any![
            eq(enums::TestEnumWithDuplicateStrippedPrefixNames::Unknown),
            eq(enums::TestEnumWithDuplicateStrippedPrefixNames::Foo),
            eq(enums::TestEnumWithDuplicateStrippedPrefixNames::Bar),
        ])
    );
    assert_that!(i32::from(enums::TestEnumWithDuplicateStrippedPrefixNames::from(5)), eq(5));
}

#[test]
fn test_enum_conversion_failure_display() {
    let err = proto2_unittest::TestSparseEnum::try_from(1).unwrap_err();
    assert_that!(format!("{err}"), eq("1 is not a known value for TestSparseEnum"));
}

#[test]
fn test_enum_conversion_failure_impls_std_error() {
    let err = proto2_unittest::TestSparseEnum::try_from(1).unwrap_err();
    let _test_compiles: &dyn std::error::Error = &err;
}
