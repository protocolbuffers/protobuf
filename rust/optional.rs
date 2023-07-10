// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google LLC. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//! Items specific to `optional` fields.
#![allow(dead_code)]
#![allow(unused)]

use crate::__internal::Private;
use crate::{Mut, MutProxy, Proxied, SettableValue, View, ViewProxy};
use std::convert::{AsMut, AsRef};
use std::fmt::{self, Debug};

/// The type that will go here is not yet defined.
pub type Todo<T = ()> = (std::marker::PhantomData<T>, std::convert::Infallible);

/// A protobuf value from a field that may not be set.
///
/// This can be pattern matched with `match` or `if let` to determine if the
/// field is set and access the field data.
///
/// [`FieldEntry`], a specific type alias for `Optional`, provides much of the
/// functionality for this type.
///
/// Two `Optional`s are equal if they match both presence and the field values.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Optional<SetVal, UnsetVal = SetVal> {
    /// The field is present. It can be accessed through this `T`.
    ///
    /// - For an `_opt()` accessor, this contains a `View<impl Proxied>`.
    /// - For a `_mut()` accessor, this contains a [`PresentField`].
    Set(SetVal),

    /// The field is unset.
    ///
    /// - For an `_opt()` accessor, this contains a `View<impl Proxied>` with
    ///   the default value.
    /// - For a `_mut()` accessor, this contains an [`AbsentField`] that can be
    ///   used to access the default or set a new value.
    Unset(UnsetVal),
}

impl<T> Optional<T> {
    /// Gets the field value, ignoring whether it was set or not.
    pub fn into_inner(self) -> T {
        match self {
            Optional::Set(x) | Optional::Unset(x) => x,
        }
    }
}

impl<T, A> Optional<T, A> {
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

/// A mutable view into the value of an optional field, which may be set or
/// unset.
pub type FieldEntry<'a, T> = Optional<PresentField<'a, T>, AbsentField<'a, T>>;

/// Methods for `_mut()` accessors of optional types.
///
/// The most common methods are [`set`] and [`or_default`].
impl<'msg, T: Proxied + ?Sized + 'msg> FieldEntry<'msg, T> {
    // is_set() is provided by `impl<T, A> Optional<T, A>`

    /// Gets a mutator for this field. Sets to the default value if not set.
    pub fn or_default(self) -> Mut<'msg, T> {
        match self {
            Optional::Set(x) => x.into_mut(),
            Optional::Unset(x) => x.set_default().into_mut(),
        }
    }

    /// Sets the value of this field to `val`.
    ///
    /// Equivalent to `self.or_default().set(val)`.
    pub fn set(&mut self, val: impl SettableValue<T>) {
        match self {
            Optional::Set(x) => x.set(val),
            Optional::Unset(x) => todo!(),
        }
    }

    /// Clears the field; `is_set()` will return `false`.
    pub fn clear(&mut self) {
        todo!("b/285308646: Requires a trait method")
    }

    /// Gets an immutable view of this field, using its default value if not
    /// set.
    ///
    /// This has a shorter lifetime than the `field_name()` message accessor;
    /// `into_view` provides that lifetime.
    pub fn get(self) -> View<'msg, T> {
        self.into_view()
    }
}

impl<'msg, T: Proxied + ?Sized + 'msg> ViewProxy<'msg> for FieldEntry<'msg, T> {
    type Proxied = T;

    fn as_view(&self) -> View<'_, T> {
        match self {
            Optional::Set(x) => x.as_view(),
            Optional::Unset(x) => x.as_view(),
        }
    }

    fn into_view<'shorter>(self) -> View<'shorter, T>
    where
        'msg: 'shorter,
    {
        match self {
            Optional::Set(x) => x.into_view(),
            Optional::Unset(x) => x.into_view(),
        }
    }
}

// `MutProxy` not implemented for `FieldEntry` since the field may not be set,
// and `as_mut`/`into_mut` should not insert.

/// A field mutator capable of clearing that is statically known to point to a
/// set field.
pub struct PresentField<'msg, T>
where
    T: Proxied + ?Sized + 'msg,
{
    inner: Todo<Mut<'msg, T>>,
}

impl<'msg, T: Proxied + ?Sized + 'msg> Debug for PresentField<'msg, T> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        todo!()
    }
}

impl<'msg, T: Proxied + ?Sized + 'msg> PresentField<'msg, T> {
    pub fn get(self) -> View<'msg, T> {
        self.into_view()
    }

    pub fn set(&mut self, val: impl SettableValue<T>) {
        val.set_on(Private, self.as_mut())
    }

    /// See [`FieldEntry::clear`].
    pub fn clear(self) -> AbsentField<'msg, T> {
        todo!("b/285308646: Requires a trait method")
    }

    // This cannot provide `reborrow` - `clear` consumes after setting the field
    // because it would violate a condition of `PresentField` - the field being set.
}

impl<'msg, T> ViewProxy<'msg> for PresentField<'msg, T>
where
    T: Proxied + ?Sized + 'msg,
{
    type Proxied = T;

    fn as_view(&self) -> View<'_, T> {
        todo!("b/285308646: Requires a trait method")
    }

    fn into_view<'shorter>(self) -> View<'shorter, T>
    where
        'msg: 'shorter,
    {
        todo!("b/285308646: Requires a trait method")
    }
}

impl<'msg, T> MutProxy<'msg> for PresentField<'msg, T>
where
    T: Proxied + ?Sized + 'msg,
{
    fn as_mut(&mut self) -> Mut<'_, T> {
        todo!("b/285308646: Requires a trait method")
    }

    fn into_mut<'shorter>(self) -> Mut<'shorter, T>
    where
        'msg: 'shorter,
    {
        todo!("b/285308646: Requires a trait method")
    }
}

/// A field mutator capable of setting that is statically known to point to a
/// non-set field.
pub struct AbsentField<'a, T>
where
    T: Proxied + ?Sized + 'a,
{
    inner: Todo<Option<Mut<'a, T>>>,
}

impl<'msg, T: Proxied + ?Sized + 'msg> Debug for AbsentField<'msg, T> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        todo!()
    }
}

impl<'msg, T: Proxied + ?Sized> AbsentField<'msg, T> {
    /// Gets the default value for this unset field.
    ///
    /// This is the same value that the primitive accessor would provide.
    pub fn default_value(self) -> View<'msg, T> {
        self.into_view()
    }

    /// See [`FieldEntry::set`]. Note that this consumes and returns a
    /// `PresentField`.
    pub fn set(self, val: impl SettableValue<T>) -> PresentField<'msg, T> {
        todo!("b/285308646: Requires a trait method")
    }

    /// Sets this absent field to its default value.
    pub fn set_default(self) -> PresentField<'msg, T> {
        todo!("b/285308646: Requires a trait method")
    }

    // This cannot provide `reborrow` - `set` consumes after setting the field
    // because it would violate a condition of `AbsentField` - the field being
    // unset.
}

impl<'msg, T> ViewProxy<'msg> for AbsentField<'msg, T>
where
    T: Proxied + ?Sized + 'msg,
{
    type Proxied = T;

    fn as_view(&self) -> View<'_, T> {
        todo!("b/285308646: Requires a trait method")
    }

    fn into_view<'shorter>(self) -> View<'shorter, T>
    where
        'msg: 'shorter,
    {
        todo!("b/285308646: Requires a trait method")
    }
}
