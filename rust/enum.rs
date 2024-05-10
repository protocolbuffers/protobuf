// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use crate::__internal::Private;
use std::{
    error::Error,
    fmt::{Debug, Display},
    marker::PhantomData,
};

/// Implemented by all generated enum types.
///
/// # Safety
/// - A `RepeatedView<Self>` or `RepeatedMut<Self>` must have the same internal
///   representation as erased enums in the runtime.
///   - For C++, this is `proto2::RepeatedField<c_int>`
///   - For UPB, this is an array compatible with `int32`
pub unsafe trait Enum: TryFrom<i32> {
    /// The name of the enum.
    const NAME: &'static str;

    /// Returns `true` if the given numeric value matches one of the `Self`'s
    /// defined values.
    ///
    /// If `Self` is a closed enum, then `TryFrom<i32>` for `value` succeeds if
    /// and only if this function returns `true`.
    fn is_known(value: i32) -> bool;
}

/// An integer value wasn't known for an enum while converting.
pub struct UnknownEnumValue<T>(i32, PhantomData<T>);

impl<T> UnknownEnumValue<T> {
    #[doc(hidden)]
    pub fn new(_private: Private, unknown_value: i32) -> Self {
        Self(unknown_value, PhantomData)
    }
}

impl<T> Debug for UnknownEnumValue<T> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_tuple("UnknownEnumValue").field(&self.0).finish()
    }
}

impl<T: Enum> Display for UnknownEnumValue<T> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let val = self.0;
        let enum_name = T::NAME;
        write!(f, "{val} is not a known value for {enum_name}")
    }
}

impl<T: Enum> Error for UnknownEnumValue<T> {}
