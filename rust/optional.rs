// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

//! Items specific to `optional` fields.
#![allow(dead_code)]
#![allow(unused)]

use crate::__internal::Private;
use crate::{Mut, MutProxied, MutProxy, Proxied, View, ViewProxy};
use std::convert::{AsMut, AsRef};
use std::fmt::{self, Debug};
use std::panic;
use std::ptr;

/// A protobuf value from a field that may not be set.
///
/// This can be pattern matched with `match` or `if let` to determine if the
/// field is set and access the field data.
///
/// Two `Optional`s are equal if they match both presence and the field values.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Optional<T> {
    /// The field is set; it is present in the serialized message.
    ///
    /// - For an `_opt()` accessor, this contains a `View<impl Proxied>`.
    /// - For a `_mut()` accessor, this contains a [`PresentField`] that can be
    ///   used to access the current value, convert to [`Mut`], clear presence,
    ///   or set a new value.
    Set(T),

    /// The field is unset; it is absent in the serialized message.
    ///
    /// - For an `_opt()` accessor, this contains a `View<impl Proxied>` with
    ///   the default value.
    /// - For a `_mut()` accessor, this contains an [`AbsentField`] that can be
    ///   used to access the default or set a new value.
    Unset(T),
}

impl<T> Optional<T> {
    /// Gets the field value, ignoring whether it was set or not.
    pub fn into_inner(self) -> T {
        match self {
            Optional::Set(x) | Optional::Unset(x) => x,
        }
    }

    /// Constructs an `Optional<T>` with a `T` value and presence bit.
    pub fn new(val: T, is_set: bool) -> Self {
        if is_set { Optional::Set(val) } else { Optional::Unset(val) }
    }

    /// Converts into an `Option` of the set value, ignoring any unset value.
    pub fn into_option(self) -> Option<T> {
        if let Optional::Set(x) = self { Some(x) } else { None }
    }

    /// Returns if the field is set.
    pub fn is_set(&self) -> bool {
        matches!(self, Optional::Set(_))
    }

    /// Returns if the field is unset.
    pub fn is_unset(&self) -> bool {
        matches!(self, Optional::Unset(_))
    }
}

impl<T> From<Optional<T>> for Option<T> {
    fn from(x: Optional<T>) -> Option<T> {
        x.into_option()
    }
}
