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
use std::fmt;
use std::hash::{Hash, Hasher};
use std::iter;
use std::ops::{Deref, DerefMut};

/// This type will be replaced by something else in a future revision.
// TODO(b/285309330): remove this and any `impl`s using it.
pub type Todo<'msg> = (std::convert::Infallible, std::marker::PhantomData<&'msg mut ()>);

/// A mutator for `bytes` fields - this type is `protobuf::Mut<'msg, [u8]>`.
///
/// This type implements `Deref<Target = [u8]>`, so many operations are
/// provided through that, including indexing and slicing.
///
/// Conceptually, this type is like a `&'msg mut &'msg str`, though the actual
/// implementation is dependent on runtime and `'msg` is covariant.
///
/// Unlike `Vec<u8>`, this type has no in-place concatenation functions like
/// `extend_from_slice`.
///
/// `BytesMut` is not intended to be grown and reallocated like a `Vec`. It's
/// recommended to instead build a `Vec<u8>` or `String` and pass that directly
/// to `set`, which will reuse the allocation if supported by the runtime.
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

    /// Truncates the byte string.
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

impl AsRef<[u8]> for BytesMut<'_> {
    fn as_ref(&self) -> &[u8] {
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

impl Eq for BytesMut<'_> {}
impl<'msg> Ord for BytesMut<'msg> {
    fn cmp(&self, other: &BytesMut<'msg>) -> Ordering {
        self.deref().cmp(other.deref())
    }
}

/// The bytes were not valid UTF-8.
#[derive(Debug)]
pub struct Utf8Error(pub(crate) ());

impl From<std::str::Utf8Error> for Utf8Error {
    fn from(_: std::str::Utf8Error) -> Utf8Error {
        Utf8Error(())
    }
}

/// A shared immutable view of a protobuf `string` field's contents.
///
/// Like a `str`, it can be cheaply accessed as bytes and
/// is dynamically sized, requiring it be accessed through a pointer.
///
/// # UTF-8 and `&str` access
///
/// Protobuf [docs] state that a `string` field contains UTF-8 encoded text.
/// However, not every runtime enforces this, and the Rust runtime is designed
/// to integrate with other runtimes with FFI, like C++.
///
/// Because of this, in order to access the contents as a `&str`, users must
/// call [`ProtoStr::to_str`] to perform a (possibly runtime-elided) UTF-8
/// validation check. However, the Rust API only allows `set()`ting a `string`
/// field with data should be valid UTF-8 like a `&str` or a
/// `&ProtoStr`. This means that this check should rarely fail, but is necessary
/// to prevent UB when interacting with C++, which has looser restrictions.
///
/// Most of the time, users should not perform direct `&str` access to the
/// contents - this type implements `Display` and comparison with `str`,
/// so it's best to avoid a UTF-8 check by working directly with `&ProtoStr`
/// or converting to `&[u8]`.
///
/// # `Display` and `ToString`
/// `ProtoStr` is ordinarily UTF-8 and so implements `Display`. If there are
/// any invalid UTF-8 sequences, they are replaced with [`U+FFFD REPLACEMENT
/// CHARACTER`]. Because anything implementing `Display` also implements
/// `ToString`, `proto_str.to_string()` is equivalent to
/// `String::from_utf8_lossy(proto_str.as_bytes()).into_owned()`.
///
/// [docs]: https://protobuf.dev/programming-guides/proto2/#scalar
/// [dst]: https://doc.rust-lang.org/reference/dynamically-sized-types.html
/// [`U+FFFD REPLACEMENT CHARACTER`]: std::char::REPLACEMENT_CHARACTER
#[repr(transparent)]
pub struct ProtoStr([u8]);

impl ProtoStr {
    /// Converts `self` to a byte slice.
    ///
    /// Note: this type does not implement `Deref`; you must call `as_bytes()`
    /// or `AsRef<[u8]>` to get access to bytes.
    pub fn as_bytes(&self) -> &[u8] {
        &self.0
    }

    /// Yields a `&str` slice if `self` contains valid UTF-8.
    ///
    /// This may perform a runtime check, dependent on runtime.
    ///
    /// `String::from_utf8_lossy(proto_str.as_bytes())` can be used to
    /// infallibly construct a string, replacing invalid UTF-8 with
    /// [`U+FFFD REPLACEMENT CHARACTER`].
    ///
    /// [`U+FFFD REPLACEMENT CHARACTER`]: std::char::REPLACEMENT_CHARACTER
    // This is not `try_to_str` since `to_str` is shorter, with `CStr` as precedent.
    pub fn to_str(&self) -> Result<&str, Utf8Error> {
        Ok(std::str::from_utf8(&self.0)?)
    }

    /// Converts `self` to a string, including invalid characters.
    ///
    /// Invalid UTF-8 sequences are replaced with
    /// [`U+FFFD REPLACEMENT CHARACTER`].
    ///
    /// Users should be prefer this to `.to_string()` provided by `Display`.
    /// `.to_cow_lossy()` is the same operation, but it may avoid an
    /// allocation if the string is already UTF-8.
    ///
    /// [`U+FFFD REPLACEMENT CHARACTER`]: std::char::REPLACEMENT_CHARACTER
    //
    // This method is named `to_string_lossy` in `CStr`, but since `to_string`
    // also exists on this type, this name was chosen to avoid confusion.
    pub fn to_cow_lossy(&self) -> Cow<'_, str> {
        String::from_utf8_lossy(&self.0)
    }

    /// Returns `true` if `self` has a length of zero bytes.
    pub fn is_empty(&self) -> bool {
        self.0.is_empty()
    }

    /// Returns the length of `self`.
    ///
    /// Like `&str`, this is a length in bytes, not `char`s or graphemes.
    pub fn len(&self) -> usize {
        self.0.len()
    }

    /// Iterates over the `char`s in this protobuf `string`.
    ///
    /// Invalid UTF-8 sequences are replaced with
    /// [`U+FFFD REPLACEMENT CHARACTER`].
    ///
    /// [`U+FFFD REPLACEMENT CHARACTER`]: std::char::REPLACEMENT_CHARACTER
    pub fn chars(&self) -> impl Iterator<Item = char> + '_ {
        todo!("b/285309330: requires UTF-8 chunk splitting");
        ['a'].into_iter() // necessary for `impl Trait` to compile
    }

    /// Returns an iterator over chunks of UTF-8 data in the string.
    ///
    /// An `Ok(&str)` is yielded for every valid UTF-8 chunk, and an
    /// `Err(&[u8])` for non-UTF-8 chunks.
    pub fn utf8_chunks(&self) -> Todo<'_> {
        todo!("b/285309330: requires UTF-8 chunk splitting");
    }

    /// Converts known-UTF-8 bytes to a `ProtoStr` without a check.
    ///
    /// # Safety
    /// `bytes` must be valid UTF-8 if the current runtime requires it.
    pub unsafe fn from_utf8_unchecked(bytes: &[u8]) -> &Self {
        // SAFETY:
        // - `ProtoStr` is `#[repr(transparent)]` over `[u8]`, so it has the same
        //   layout.
        // - `ProtoStr` has the same pointer metadata and element size as `[u8]`.
        unsafe { &*(bytes as *const [u8] as *const Self) }
    }

    /// Interprets a string slice as a `&ProtoStr`.
    pub fn from_str(string: &str) -> &Self {
        // SAFETY: `string.as_bytes()` is valid UTF-8.
        unsafe { Self::from_utf8_unchecked(string.as_bytes()) }
    }
}

impl AsRef<[u8]> for ProtoStr {
    fn as_ref(&self) -> &[u8] {
        self.as_bytes()
    }
}

impl<'msg> From<&'msg ProtoStr> for &'msg [u8] {
    fn from(val: &'msg ProtoStr) -> &'msg [u8] {
        val.as_bytes()
    }
}

impl<'msg> TryFrom<&'msg ProtoStr> for &'msg str {
    type Error = Utf8Error;

    fn try_from(val: &'msg ProtoStr) -> Result<&'msg str, Utf8Error> {
        val.to_str()
    }
}

impl fmt::Debug for ProtoStr {
    fn fmt(&self, _f: &mut fmt::Formatter<'_>) -> fmt::Result {
        todo!("b/285309330: requires UTF-8 chunk splitting")
    }
}

impl fmt::Display for ProtoStr {
    fn fmt(&self, _f: &mut fmt::Formatter<'_>) -> fmt::Result {
        todo!("b/285309330: requires UTF-8 chunk splitting")
    }
}

// TODO(b/285309330): Add `ProtoStrMut`

/// Implements `PartialCmp` and `PartialEq` for the `lhs` against the `rhs`
/// using `AsRef<[u8]>`.
// TODO(kupiakos): consider improving to not require a `<()>` if no generics are
// needed
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
    // Should `BytesMut` compare with `str` and `ProtoStr[Mut]` with `[u8]`?
    // `[u8]` and `str` do not compare with each other in the stdlib.

    // `BytesMut` against protobuf types
    <('a, 'b)> BytesMut<'a> => BytesMut<'b>,

    // `BytesMut` against foreign types
    <('a)> BytesMut<'a> => [u8],
    <('a)> [u8] => BytesMut<'a>,
    <('a, const N: usize)> BytesMut<'a> => [u8; N],
    <('a, const N: usize)> [u8; N] => BytesMut<'a>,

    // `ProtoStr` against protobuf types
    <()> ProtoStr => ProtoStr,

    // `ProtoStr` against foreign types
    <()> ProtoStr => str,
    <()> str => ProtoStr,

    // TODO(b/285309330): `ProtoStrMut` impls
);

#[cfg(test)]
mod tests {
    // TODO(b/285309330): Add unit tests
}
