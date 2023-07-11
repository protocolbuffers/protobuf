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

//! Items specific to `bytes` and `string` fields.
#![allow(dead_code)]
#![allow(unused)]

use crate::__internal::Private;
use crate::{Mut, MutProxy, Proxied, ProxiedWithPresence, SettableValue, View, ViewProxy};
use std::borrow::Cow;
use std::cmp::{Eq, Ord, Ordering, PartialEq, PartialOrd};
use std::convert::{AsMut, AsRef};
use std::hash::{Hash, Hasher};
use std::ops::{Deref, DerefMut};

/// This type will be replaced by something else in a future revision.
// TODO(b/285309330): remove this and any `impl`s using it.
pub type Todo<'msg> = (std::convert::Infallible, std::marker::PhantomData<&'msg mut ()>);

/// A mutator for `bytes` fields - this type is `protobuf::Mut<'msg, [u8]>`.
///
/// This type implements `DerefMut<Target = [u8]>`, so many operations are
/// provided through that, including indexing and slicing.
///
/// Conceptually, this type is a `&'msg mut Vec<u8>`, though the actual
/// implementation is dependent on runtime. Unlike `Vec<u8>`, this type has no
/// in-place concatenation functions like `extend_from_slice`. `BytesMut` is not
/// intended to be grown and reallocated like a `Vec`. It's recommended to
/// instead build a `Vec<u8>` or `String` and pass that directly to `set`, which
/// will reuse the allocation if supported by the runtime.
#[derive(Debug)]
pub struct BytesMut<'msg>(Todo<'msg>);

impl<'msg> BytesMut<'msg> {
    /// Sets the byte string to the given `val`, cloning any borrowed data.
    ///
    /// This method accepts both owned and borrowed byte strings; if the runtime
    /// supports it, an owned value will not reallocate when setting the
    /// string.
    pub fn set(&mut self, val: impl SettableValue<[u8]>) {
        val.set_on(Private, MutProxy::as_mut(self))
    }

    /// Truncates the byte string without reallocating.
    ///
    /// Has no effect if `new_len` is larger than the current `len`.
    pub fn truncate(&mut self, new_len: usize) {
        todo!("b/285309330")
    }

    /// Clears the byte string to the empty string.
    ///
    /// # Compared with `FieldEntry::clear`
    ///
    /// Note that this is different than marking an `optional bytes` field as
    /// absent; if these `bytes` are in an `optional`, `FieldEntry::is_set`
    /// will still return `true` after this method is invoked.
    ///
    /// This also means that if the field has a non-empty default,
    /// `BytesMut::clear` results in the accessor returning an empty string
    /// while `FieldEntry::clear` results in the non-empty default.
    ///
    /// However, for a proto3 `bytes` that has implicit presence, there is no
    /// distinction between these states: unset `bytes` is the same as empty
    /// `bytes` and the default is always the empty string.
    ///
    /// In the C++ API, this is the difference between `msg.clear_bytes_field()`
    /// and `msg.mutable_bytes_field()->clear()`.
    ///
    /// Having the same name and signature as `FieldEntry::clear` makes code
    /// that calls `field_mut().clear()` easier to migrate from implicit
    /// to explicit presence.
    pub fn clear(&mut self) {
        self.truncate(0);
    }
}

impl Deref for BytesMut<'_> {
    type Target = [u8];
    fn deref(&self) -> &[u8] {
        self.as_ref()
    }
}

impl DerefMut for BytesMut<'_> {
    fn deref_mut(&mut self) -> &mut [u8] {
        AsMut::as_mut(self)
    }
}

impl AsRef<[u8]> for BytesMut<'_> {
    fn as_ref(&self) -> &[u8] {
        todo!("b/285309330")
    }
}

impl AsMut<[u8]> for BytesMut<'_> {
    fn as_mut(&mut self) -> &mut [u8] {
        todo!("b/285309330")
    }
}

impl Proxied for [u8] {
    type View<'msg> = &'msg [u8];
    type Mut<'msg> = BytesMut<'msg>;
}

impl<'msg> ViewProxy<'msg> for Todo<'msg> {
    type Proxied = [u8];
    fn as_view(&self) -> &[u8] {
        unreachable!()
    }
    fn into_view<'shorter>(self) -> &'shorter [u8]
    where
        'msg: 'shorter,
    {
        unreachable!()
    }
}
impl<'msg> MutProxy<'msg> for Todo<'msg> {
    fn as_mut(&mut self) -> BytesMut<'msg> {
        unreachable!()
    }
    fn into_mut<'shorter>(self) -> BytesMut<'shorter>
    where
        'msg: 'shorter,
    {
        unreachable!()
    }
}

impl ProxiedWithPresence for [u8] {
    type PresentMutData<'msg> = Todo<'msg>;
    type AbsentMutData<'msg> = Todo<'msg>;

    fn clear_present_field<'a>(
        present_mutator: Self::PresentMutData<'a>,
    ) -> Self::AbsentMutData<'a> {
        todo!("b/285309330")
    }

    fn set_absent_to_default<'a>(
        absent_mutator: Self::AbsentMutData<'a>,
    ) -> Self::PresentMutData<'a> {
        todo!("b/285309330")
    }
}

impl<'msg> ViewProxy<'msg> for &'msg [u8] {
    type Proxied = [u8];

    fn as_view(&self) -> &[u8] {
        self
    }

    fn into_view<'shorter>(self) -> &'shorter [u8]
    where
        'msg: 'shorter,
    {
        self
    }
}

impl<'msg> ViewProxy<'msg> for BytesMut<'msg> {
    type Proxied = [u8];

    fn as_view(&self) -> &[u8] {
        self.as_ref()
    }

    fn into_view<'shorter>(self) -> &'shorter [u8]
    where
        'msg: 'shorter,
    {
        todo!("b/285309330")
    }
}

impl<'msg> MutProxy<'msg> for BytesMut<'msg> {
    fn as_mut(&mut self) -> BytesMut<'_> {
        todo!("b/285309330")
    }

    fn into_mut<'shorter>(self) -> BytesMut<'shorter>
    where
        'msg: 'shorter,
    {
        todo!("b/285309330")
    }
}

impl SettableValue<[u8]> for &'_ [u8] {
    fn set_on(self, _private: Private, mutator: BytesMut<'_>) {
        todo!("b/285309330")
    }
}

impl<const N: usize> SettableValue<[u8]> for &'_ [u8; N] {
    fn set_on(self, _private: Private, mutator: BytesMut<'_>) {
        self[..].set_on(Private, mutator)
    }
}

impl SettableValue<[u8]> for Vec<u8> {
    fn set_on(self, _private: Private, mutator: BytesMut<'_>) {
        todo!("b/285309330")
    }
}

impl SettableValue<[u8]> for Cow<'_, [u8]> {
    fn set_on(self, _private: Private, mutator: BytesMut<'_>) {
        match self {
            Cow::Borrowed(s) => s.set_on(Private, mutator),
            Cow::Owned(v) => v.set_on(Private, mutator),
        }
    }
}

impl Hash for BytesMut<'_> {
    fn hash<H: Hasher>(&self, state: &mut H) {
        self.deref().hash(state)
    }
}

/// Implements `PartialCmp` and `PartialEq` for the `lhs` against the `rhs`
/// using `AsRef<[u8]>`.
macro_rules! impl_bytes_partial_cmp {
    ($(<($($generics:tt)*)> $lhs:ty => $rhs:ty),+ $(,)?) => {
        $(
            impl<$($generics)*> PartialEq<$rhs> for $lhs {
                fn eq(&self, other: &$rhs) -> bool {
                    AsRef::<[u8]>::as_ref(self) == AsRef::<[u8]>::as_ref(other)
                }
            }
            impl<$($generics)*> PartialOrd<$rhs> for $lhs {
                fn partial_cmp(&self, other: &$rhs) -> Option<Ordering> {
                    AsRef::<[u8]>::as_ref(self).partial_cmp(AsRef::<[u8]>::as_ref(other))
                }
            }
        )*
    };
}

impl_bytes_partial_cmp!(
    <('a, 'b)> BytesMut<'a> => BytesMut<'b>,
    <('a)> BytesMut<'a> => [u8],
    <('a, const N: usize)> BytesMut<'a> => [u8; N],
    <('a)> BytesMut<'a> => str,
    <('a)> [u8] => BytesMut<'a>,
    <('a, const N: usize)> [u8; N] => BytesMut<'a>,
    <('a)> str => BytesMut<'a>,
);

impl Eq for BytesMut<'_> {}
impl<'msg> Ord for BytesMut<'msg> {
    fn cmp(&self, other: &BytesMut<'msg>) -> Ordering {
        self.deref().cmp(other.deref())
    }
}
