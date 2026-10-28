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

use crate::__internal::SealedInternal;
use crate::codegen_traits::{entity_tag, EntityType};
use crate::{Proxied, Singular};

/// Implemented by all generated enum types.
///
/// # Safety
/// - A `RepeatedView<Self>` or `RepeatedMut<Self>` must have the same internal representation as
///   erased enums in the runtime.
///   - For C++, this is `proto2::RepeatedField<c_int>`
///   - For UPB, this is an array compatible with `int32`
pub unsafe trait Enum:
    TryFrom<i32>
    + Into<i32>
    + Copy
    + for<'a> Proxied<View<'a> = Self>
    + EntityType<Tag = entity_tag::EnumTag>
    + Singular
    + SealedInternal
{
    /// The name of the enum.
    const NAME: &'static str;

    /// Returns `true` if the given numeric value matches one of the `Self`'s
    /// defined values.
    ///
    /// If `Self` is a closed enum, then `TryFrom<i32>` for `value` succeeds if
    /// and only if this function returns `true`.
    fn is_known(value: i32) -> bool;
}

/// Converts the values of a generated enum to and from their names as written
/// in the .proto file. Implemented by every generated enum except those from
/// `optimize_for = LITE_RUNTIME` files or built with the lite runtime forced,
/// which omit their value names so that Lite and mobile binaries don't embed
/// them. Code that needs the names fails to compile with those enums.
///
/// Each implementing enum also has these as inherent functions, with `name()`
/// a `const fn`.
pub trait EnumNames: Enum {
    /// Returns the name of this value as written in the .proto file (e.g.
    /// `"FOO_BAR"`), or `None` for a number that has no declared value.
    ///
    /// If several names share this number (`allow_alias`), returns the first
    /// one declared.
    fn name(self) -> Option<&'static str>;

    /// Returns the value with the given name as written in the .proto file
    /// (e.g. `"FOO_BAR"`), or `None` if no value has that name. Accepts every
    /// alias.
    fn from_name(name: &str) -> Option<Self>;
}

/// Returns the number of the value named `name` in `names_by_name`, a generated
/// enum's (name, number) pairs, including aliases, sorted by name. Used by the
/// generated `from_name()`.
#[doc(hidden)]
pub fn enum_value_from_name(names_by_name: &[(&str, i32)], name: &str) -> Option<i32> {
    names_by_name.binary_search_by(|(n, _)| (*n).cmp(name)).ok().map(|i| names_by_name[i].1)
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
