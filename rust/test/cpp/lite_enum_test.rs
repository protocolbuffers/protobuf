// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

//! Enums from `optimize_for = LITE_RUNTIME` files, and every enum when the lite
//! runtime is forced, omit their value names: they don't implement `EnumNames`
//! (nor have inherent `name()`/`from_name()`), so code that needs the names
//! doesn't compile with them.

use enums_rust_proto::TestEnumWithNumericNames;
use googletest::prelude::*;
use optimize_for_lite_rust_proto::OptimizeForLiteTestEnum;
use protobuf::EnumNames;
use std::marker::PhantomData;

/// `Probe::<E>::HAS_NAMES` is whether `E: EnumNames`: the inherent constant
/// applies when its bound holds, and the trait's constant otherwise.
struct Probe<E>(PhantomData<E>);

trait WithoutNames {
    const HAS_NAMES: bool = false;
}

impl<E> WithoutNames for Probe<E> {}

impl<E: EnumNames> Probe<E> {
    const HAS_NAMES: bool = true;
}

#[gtest]
fn test_optimize_for_lite_enum_has_no_names() {
    expect_false!(Probe::<OptimizeForLiteTestEnum>::HAS_NAMES);
}

#[cfg(not(lite_runtime))]
#[gtest]
fn test_enum_has_names() {
    expect_true!(Probe::<TestEnumWithNumericNames>::HAS_NAMES);
}

#[cfg(lite_runtime)]
#[gtest]
fn test_forced_lite_enum_has_no_names() {
    expect_false!(Probe::<TestEnumWithNumericNames>::HAS_NAMES);
}
