// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

//! Items specific to `bytes` and `string` fields.
#![allow(dead_code)]
#![allow(unused)]

use crate::__internal::{Private, PtrAndLen, RawMessage};
use crate::__runtime::{BytesAbsentMutData, BytesPresentMutData, InnerBytesMut};
use crate::macros::impl_forwarding_settable_value;
use crate::{
    AbsentField, FieldEntry, Mut, MutProxy, Optional, PresentField, Proxied, ProxiedWithPresence,
    SettableValue, View, ViewProxy,
};
use std::borrow::Cow;
use std::cmp::{Eq, Ord, Ordering, PartialEq, PartialOrd};
use std::convert::{AsMut, AsRef};
use std::fmt;
use std::hash::{Hash, Hasher};
use std::iter;
use std::ops::{Deref, DerefMut};
use utf8::Utf8Chunks;

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
pub struct BytesMut<'msg> {
    inner: InnerBytesMut<'msg>,
}

// SAFETY:
// - Protobuf Rust messages don't allow shared mutation across threads.
// - Protobuf Rust messages don't share arenas.
// - All access that touches an arena occurs behind a `&mut`.
// - All mutators that store an arena are `!Send`.
unsafe impl Sync for BytesMut<'_> {}

impl<'msg> BytesMut<'msg> {
    /// Constructs a new `BytesMut` from its internal, runtime-dependent part.
    #[doc(hidden)]
    pub fn from_inner(_private: Private, inner: InnerBytesMut<'msg>) -> Self {
        Self { inner }
    }

    /// Gets the current value of the field.
    pub fn get(&self) -> &[u8] {
        self.as_view()
    }

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
        self.inner.truncate(new_len)
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
        unsafe { self.inner.get() }
    }
}

impl Proxied for [u8] {
    type View<'msg> = &'msg [u8];
    type Mut<'msg> = BytesMut<'msg>;
}

impl ProxiedWithPresence for [u8] {
    type PresentMutData<'msg> = BytesPresentMutData<'msg>;
    type AbsentMutData<'msg> = BytesAbsentMutData<'msg>;

    fn clear_present_field<'a>(
        present_mutator: Self::PresentMutData<'a>,
    ) -> Self::AbsentMutData<'a> {
        present_mutator.clear()
    }

    fn set_absent_to_default<'a>(
        absent_mutator: Self::AbsentMutData<'a>,
    ) -> Self::PresentMutData<'a> {
        absent_mutator.set_absent_to_default()
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
        self.inner.get()
    }
}

impl<'msg> MutProxy<'msg> for BytesMut<'msg> {
    fn as_mut(&mut self) -> BytesMut<'_> {
        BytesMut { inner: self.inner }
    }

    fn into_mut<'shorter>(self) -> BytesMut<'shorter>
    where
        'msg: 'shorter,
    {
        BytesMut { inner: self.inner }
    }
}

impl SettableValue<[u8]> for &'_ [u8] {
    fn set_on(self, _private: Private, mutator: BytesMut<'_>) {
        // SAFETY: this is a `bytes` field with no restriction on UTF-8.
        unsafe { mutator.inner.set(self) }
    }

    fn set_on_absent(
        self,
        _private: Private,
        absent_mutator: <[u8] as ProxiedWithPresence>::AbsentMutData<'_>,
    ) -> <[u8] as ProxiedWithPresence>::PresentMutData<'_> {
        // SAFETY: this is a `bytes` field with no restriction on UTF-8.
        unsafe { absent_mutator.set(self) }
    }

    fn set_on_present(
        self,
        _private: Private,
        present_mutator: <[u8] as ProxiedWithPresence>::PresentMutData<'_>,
    ) {
        // SAFETY: this is a `bytes` field with no restriction on UTF-8.
        unsafe {
            present_mutator.set(self);
        }
    }
}

impl<const N: usize> SettableValue<[u8]> for &'_ [u8; N] {
    // forward to `self[..]`
    impl_forwarding_settable_value!([u8], self => &self[..]);
}

impl SettableValue<[u8]> for Vec<u8> {
    // TODO: Investigate taking ownership of this when allowed by the
    // runtime.
    impl_forwarding_settable_value!([u8], self => &self[..]);
}

impl SettableValue<[u8]> for Cow<'_, [u8]> {
    // TODO: Investigate taking ownership of this when allowed by the
    // runtime.
    impl_forwarding_settable_value!([u8], self => &self[..]);
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
#[derive(Debug, PartialEq)]
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
        Utf8Chunks::new(self.as_bytes()).flat_map(|chunk| {
            let mut yield_replacement_char = !chunk.invalid().is_empty();
            chunk.valid().chars().chain(iter::from_fn(move || {
                // Yield a single replacement character for every
                // non-empty invalid sequence.
                yield_replacement_char.then(|| {
                    yield_replacement_char = false;
                    char::REPLACEMENT_CHARACTER
                })
            }))
        })
    }

    /// Returns an iterator over chunks of UTF-8 data in the string.
    ///
    /// An `Ok(&str)` is yielded for every valid UTF-8 chunk, and an
    /// `Err(&[u8])` for each non-UTF-8 chunk. An `Err` will be emitted
    /// multiple times in a row for contiguous invalid chunks. Each invalid
    /// chunk in an `Err` has a maximum length of 3 bytes.
    pub fn utf8_chunks(&self) -> impl Iterator<Item = Result<&str, &[u8]>> + '_ {
        Utf8Chunks::new(self.as_bytes()).flat_map(|chunk| {
            let valid = chunk.valid();
            let invalid = chunk.invalid();
            (!valid.is_empty())
                .then_some(Ok(valid))
                .into_iter()
                .chain((!invalid.is_empty()).then_some(Err(invalid)))
        })
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

impl<'msg> From<&'msg str> for &'msg ProtoStr {
    fn from(val: &'msg str) -> &'msg ProtoStr {
        ProtoStr::from_str(val)
    }
}

impl<'msg> TryFrom<&'msg ProtoStr> for &'msg str {
    type Error = Utf8Error;

    fn try_from(val: &'msg ProtoStr) -> Result<&'msg str, Utf8Error> {
        val.to_str()
    }
}

impl<'msg> TryFrom<&'msg [u8]> for &'msg ProtoStr {
    type Error = Utf8Error;

    fn try_from(val: &'msg [u8]) -> Result<&'msg ProtoStr, Utf8Error> {
        Ok(ProtoStr::from_str(std::str::from_utf8(val)?))
    }
}

impl fmt::Debug for ProtoStr {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        fmt::Debug::fmt(&Utf8Chunks::new(self.as_bytes()).debug(), f)
    }
}

impl fmt::Display for ProtoStr {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        use std::fmt::Write as _;
        for chunk in Utf8Chunks::new(self.as_bytes()) {
            fmt::Display::fmt(chunk.valid(), f)?;
            if !chunk.invalid().is_empty() {
                // One invalid chunk is emitted per detected invalid sequence.
                f.write_char(char::REPLACEMENT_CHARACTER)?;
            }
        }
        Ok(())
    }
}

impl Hash for ProtoStr {
    fn hash<H: Hasher>(&self, state: &mut H) {
        self.as_bytes().hash(state)
    }
}

impl Eq for ProtoStr {}
impl Ord for ProtoStr {
    fn cmp(&self, other: &ProtoStr) -> Ordering {
        self.as_bytes().cmp(other.as_bytes())
    }
}

impl Proxied for ProtoStr {
    type View<'msg> = &'msg ProtoStr;
    type Mut<'msg> = ProtoStrMut<'msg>;
}

impl ProxiedWithPresence for ProtoStr {
    type PresentMutData<'msg> = StrPresentMutData<'msg>;
    type AbsentMutData<'msg> = StrAbsentMutData<'msg>;

    fn clear_present_field(present_mutator: Self::PresentMutData<'_>) -> Self::AbsentMutData<'_> {
        StrAbsentMutData(present_mutator.0.clear())
    }

    fn set_absent_to_default(absent_mutator: Self::AbsentMutData<'_>) -> Self::PresentMutData<'_> {
        StrPresentMutData(absent_mutator.0.set_absent_to_default())
    }
}

impl<'msg> ViewProxy<'msg> for &'msg ProtoStr {
    type Proxied = ProtoStr;

    fn as_view(&self) -> &ProtoStr {
        self
    }

    fn into_view<'shorter>(self) -> &'shorter ProtoStr
    where
        'msg: 'shorter,
    {
        self
    }
}

/// Non-exported newtype for `ProxiedWithPresence::PresentData`
#[derive(Debug)]
pub struct StrPresentMutData<'msg>(BytesPresentMutData<'msg>);

impl<'msg> ViewProxy<'msg> for StrPresentMutData<'msg> {
    type Proxied = ProtoStr;

    fn as_view(&self) -> View<'_, ProtoStr> {
        // SAFETY: The `ProtoStr` API guards against non-UTF-8 data. The runtime does
        // not require `ProtoStr` to be UTF-8 if it could be mutated outside of these
        // guards, such as through FFI.
        unsafe { ProtoStr::from_utf8_unchecked(self.0.as_view()) }
    }

    fn into_view<'shorter>(self) -> View<'shorter, ProtoStr>
    where
        'msg: 'shorter,
    {
        // SAFETY: The `ProtoStr` API guards against non-UTF-8 data. The runtime does
        // not require `ProtoStr` to be UTF-8 if it could be mutated outside of these
        // guards, such as through FFI.
        unsafe { ProtoStr::from_utf8_unchecked(self.0.into_view()) }
    }
}

impl<'msg> MutProxy<'msg> for StrPresentMutData<'msg> {
    fn as_mut(&mut self) -> Mut<'_, ProtoStr> {
        ProtoStrMut { bytes: self.0.as_mut() }
    }

    fn into_mut<'shorter>(self) -> Mut<'shorter, ProtoStr>
    where
        'msg: 'shorter,
    {
        ProtoStrMut { bytes: self.0.into_mut() }
    }
}

/// Non-exported newtype for `ProxiedWithPresence::AbsentData`
#[derive(Debug)]
pub struct StrAbsentMutData<'msg>(BytesAbsentMutData<'msg>);

impl<'msg> ViewProxy<'msg> for StrAbsentMutData<'msg> {
    type Proxied = ProtoStr;

    fn as_view(&self) -> View<'_, ProtoStr> {
        // SAFETY: The `ProtoStr` API guards against non-UTF-8 data. The runtime does
        // not require `ProtoStr` to be UTF-8 if it could be mutated outside of these
        // guards, such as through FFI.
        unsafe { ProtoStr::from_utf8_unchecked(self.0.as_view()) }
    }

    fn into_view<'shorter>(self) -> View<'shorter, ProtoStr>
    where
        'msg: 'shorter,
    {
        // SAFETY: The `ProtoStr` API guards against non-UTF-8 data. The runtime does
        // not require `ProtoStr` to be UTF-8 if it could be mutated outside of these
        // guards, such as through FFI.
        unsafe { ProtoStr::from_utf8_unchecked(self.0.into_view()) }
    }
}

#[derive(Debug)]
pub struct ProtoStrMut<'msg> {
    bytes: BytesMut<'msg>,
}

impl<'msg> ProtoStrMut<'msg> {
    /// Constructs a new `ProtoStrMut` from its internal, runtime-dependent
    /// part.
    #[doc(hidden)]
    pub fn from_inner(_private: Private, inner: InnerBytesMut<'msg>) -> Self {
        Self { bytes: BytesMut { inner } }
    }

    /// Converts a `bytes` `FieldEntry` into a `string` one. Used by gencode.
    #[doc(hidden)]
    pub fn field_entry_from_bytes(
        _private: Private,
        field_entry: FieldEntry<'_, [u8]>,
    ) -> FieldEntry<ProtoStr> {
        match field_entry {
            Optional::Set(present) => {
                Optional::Set(PresentField::from_inner(Private, StrPresentMutData(present.inner)))
            }
            Optional::Unset(absent) => {
                Optional::Unset(AbsentField::from_inner(Private, StrAbsentMutData(absent.inner)))
            }
        }
    }

    /// Gets the current value of the field.
    pub fn get(&self) -> &ProtoStr {
        self.as_view()
    }

    /// Sets the string to the given `val`, cloning any borrowed data.
    ///
    /// This method accepts both owned and borrowed strings; if the runtime
    /// supports it, an owned value will not reallocate when setting the
    /// string.
    pub fn set(&mut self, val: impl SettableValue<ProtoStr>) {
        val.set_on(Private, MutProxy::as_mut(self))
    }

    /// Truncates the string.
    ///
    /// Has no effect if `new_len` is larger than the current `len`.
    ///
    /// If `new_len` does not lie on a UTF-8 `char` boundary, behavior is
    /// runtime-dependent. If this occurs, the runtime may:
    ///
    /// - Panic
    /// - Truncate the string further to be on a `char` boundary.
    /// - Truncate to `new_len`, resulting in a `ProtoStr` with a non-UTF8 tail.
    pub fn truncate(&mut self, new_len: usize) {
        self.bytes.truncate(new_len)
    }

    /// Clears the string, setting it to the empty string.
    ///
    /// # Compared with `FieldEntry::clear`
    ///
    /// Note that this is different than marking an `optional string` field as
    /// absent; if this cleared `string` is in an `optional`,
    /// `FieldEntry::is_set` will still return `true` after this method is
    /// invoked.
    ///
    /// This also means that if the field has a non-empty default,
    /// `ProtoStrMut::clear` results in the accessor returning an empty string
    /// while `FieldEntry::clear` results in the non-empty default.
    ///
    /// However, for a proto3 `string` that has implicit presence, there is no
    /// distinction between these states: unset `string` is the same as empty
    /// `string` and the default is always the empty string.
    ///
    /// In the C++ API, this is the difference between
    /// `msg.clear_string_field()`
    /// and `msg.mutable_string_field()->clear()`.
    ///
    /// Having the same name and signature as `FieldEntry::clear` makes code
    /// that calls `field_mut().clear()` easier to migrate from implicit
    /// to explicit presence.
    pub fn clear(&mut self) {
        self.truncate(0);
    }
}

impl Deref for ProtoStrMut<'_> {
    type Target = ProtoStr;
    fn deref(&self) -> &ProtoStr {
        self.as_view()
    }
}

impl AsRef<ProtoStr> for ProtoStrMut<'_> {
    fn as_ref(&self) -> &ProtoStr {
        self.as_view()
    }
}

impl AsRef<[u8]> for ProtoStrMut<'_> {
    fn as_ref(&self) -> &[u8] {
        self.as_view().as_bytes()
    }
}

impl<'msg> ViewProxy<'msg> for ProtoStrMut<'msg> {
    type Proxied = ProtoStr;

    fn as_view(&self) -> &ProtoStr {
        // SAFETY: The `ProtoStr` API guards against non-UTF-8 data. The runtime does
        // not require `ProtoStr` to be UTF-8 if it could be mutated outside of these
        // guards, such as through FFI.
        unsafe { ProtoStr::from_utf8_unchecked(self.bytes.as_view()) }
    }

    fn into_view<'shorter>(self) -> &'shorter ProtoStr
    where
        'msg: 'shorter,
    {
        unsafe { ProtoStr::from_utf8_unchecked(self.bytes.into_view()) }
    }
}

impl<'msg> MutProxy<'msg> for ProtoStrMut<'msg> {
    fn as_mut(&mut self) -> ProtoStrMut<'_> {
        ProtoStrMut { bytes: BytesMut { inner: self.bytes.inner } }
    }

    fn into_mut<'shorter>(self) -> ProtoStrMut<'shorter>
    where
        'msg: 'shorter,
    {
        ProtoStrMut { bytes: BytesMut { inner: self.bytes.inner } }
    }
}

impl SettableValue<ProtoStr> for &'_ ProtoStr {
    fn set_on(self, _private: Private, mutator: ProtoStrMut<'_>) {
        // SAFETY: A `ProtoStr` has the same UTF-8 validity requirement as the runtime.
        unsafe { mutator.bytes.inner.set(self.as_bytes()) }
    }

    fn set_on_absent(
        self,
        _private: Private,
        absent_mutator: <ProtoStr as ProxiedWithPresence>::AbsentMutData<'_>,
    ) -> <ProtoStr as ProxiedWithPresence>::PresentMutData<'_> {
        // SAFETY: A `ProtoStr` has the same UTF-8 validity requirement as the runtime.
        StrPresentMutData(unsafe { absent_mutator.0.set(self.as_bytes()) })
    }

    fn set_on_present(
        self,
        _private: Private,
        present_mutator: <ProtoStr as ProxiedWithPresence>::PresentMutData<'_>,
    ) {
        // SAFETY: A `ProtoStr` has the same UTF-8 validity requirement as the runtime.
        unsafe {
            present_mutator.0.set(self.as_bytes());
        }
    }
}

impl SettableValue<ProtoStr> for &'_ str {
    impl_forwarding_settable_value!(ProtoStr, self => ProtoStr::from_str(self));
}

impl SettableValue<ProtoStr> for String {
    // TODO: Investigate taking ownership of this when allowed by the
    // runtime.
    impl_forwarding_settable_value!(ProtoStr, self => ProtoStr::from_str(&self));
}

impl SettableValue<ProtoStr> for Cow<'_, str> {
    // TODO: Investigate taking ownership of this when allowed by the
    // runtime.
    impl_forwarding_settable_value!(ProtoStr, self => ProtoStr::from_str(&self));
}

impl Hash for ProtoStrMut<'_> {
    fn hash<H: Hasher>(&self, state: &mut H) {
        self.deref().hash(state)
    }
}

impl Eq for ProtoStrMut<'_> {}
impl<'msg> Ord for ProtoStrMut<'msg> {
    fn cmp(&self, other: &ProtoStrMut<'msg>) -> Ordering {
        self.deref().cmp(other.deref())
    }
}

/// Implements `PartialCmp` and `PartialEq` for the `lhs` against the `rhs`
/// using `AsRef<[u8]>`.
// TODO: consider improving to not require a `<()>` if no generics are
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
    <('a)> ProtoStr => ProtoStrMut<'a>,

    // `ProtoStr` against foreign types
    <()> ProtoStr => str,
    <()> str => ProtoStr,

    // `ProtoStrMut` against protobuf types
    <('a, 'b)> ProtoStrMut<'a> => ProtoStrMut<'b>,
    <('a)> ProtoStrMut<'a> => ProtoStr,

    // `ProtoStrMut` against foreign types
    <('a)> ProtoStrMut<'a> => str,
    <('a)> str => ProtoStrMut<'a>,
);

#[cfg(test)]
mod tests {
    use super::*;

    // TODO: Add unit tests

    // Shorter and safe utility function to construct `ProtoStr` from bytes for
    // testing.
    fn test_proto_str(bytes: &[u8]) -> &ProtoStr {
        // SAFETY: The runtime that this test executes under does not elide UTF-8 checks
        // inside of `ProtoStr`.
        unsafe { ProtoStr::from_utf8_unchecked(bytes) }
    }

    // UTF-8 test cases copied from:
    // https://github.com/rust-lang/rust/blob/e8ee0b7/library/core/tests/str_lossy.rs

    #[test]
    fn proto_str_debug() {
        assert_eq!(&format!("{:?}", test_proto_str(b"Hello There")), "\"Hello There\"");
        assert_eq!(
            &format!(
                "{:?}",
                test_proto_str(b"Hello\xC0\x80 There\xE6\x83 Goodbye\xf4\x8d\x93\xaa"),
            ),
            "\"Hello\\xC0\\x80 There\\xE6\\x83 Goodbye\\u{10d4ea}\"",
        );
    }

    #[test]
    fn proto_str_display() {
        assert_eq!(&test_proto_str(b"Hello There").to_string(), "Hello There");
        assert_eq!(
            &test_proto_str(b"Hello\xC0\x80 There\xE6\x83 Goodbye\xf4\x8d\x93\xaa").to_string(),
            "Hello�� There� Goodbye\u{10d4ea}",
        );
    }

    #[test]
    fn proto_str_to_rust_str() {
        assert_eq!(test_proto_str(b"hello").to_str(), Ok("hello"));
        assert_eq!(test_proto_str("ศไทย中华Việt Nam".as_bytes()).to_str(), Ok("ศไทย中华Việt Nam"));
        for expect_fail in [
            &b"Hello\xC2 There\xFF Goodbye"[..],
            b"Hello\xC0\x80 There\xE6\x83 Goodbye",
            b"\xF5foo\xF5\x80bar",
            b"\xF1foo\xF1\x80bar\xF1\x80\x80baz",
            b"\xF4foo\xF4\x80bar\xF4\xBFbaz",
            b"\xF0\x80\x80\x80foo\xF0\x90\x80\x80bar",
            b"\xED\xA0\x80foo\xED\xBF\xBFbar",
        ] {
            assert_eq!(test_proto_str(expect_fail).to_str(), Err(Utf8Error(())), "{expect_fail:?}");
        }
    }

    #[test]
    fn proto_str_to_cow() {
        assert_eq!(test_proto_str(b"hello").to_cow_lossy(), Cow::Borrowed("hello"));
        assert_eq!(
            test_proto_str("ศไทย中华Việt Nam".as_bytes()).to_cow_lossy(),
            Cow::Borrowed("ศไทย中华Việt Nam")
        );
        for (bytes, lossy_str) in [
            (&b"Hello\xC2 There\xFF Goodbye"[..], "Hello� There� Goodbye"),
            (b"Hello\xC0\x80 There\xE6\x83 Goodbye", "Hello�� There� Goodbye"),
            (b"\xF5foo\xF5\x80bar", "�foo��bar"),
            (b"\xF1foo\xF1\x80bar\xF1\x80\x80baz", "�foo�bar�baz"),
            (b"\xF4foo\xF4\x80bar\xF4\xBFbaz", "�foo�bar��baz"),
            (b"\xF0\x80\x80\x80foo\xF0\x90\x80\x80bar", "����foo\u{10000}bar"),
            (b"\xED\xA0\x80foo\xED\xBF\xBFbar", "���foo���bar"),
        ] {
            let cow = test_proto_str(bytes).to_cow_lossy();
            assert!(matches!(cow, Cow::Owned(_)));
            assert_eq!(&*cow, lossy_str, "{bytes:?}");
        }
    }

    #[test]
    fn proto_str_utf8_chunks() {
        macro_rules! assert_chunks {
            ($bytes:expr, $($chunks:expr),* $(,)?) => {
                let bytes = $bytes;
                let chunks: &[Result<&str, &[u8]>] = &[$($chunks),*];
                let s = test_proto_str(bytes);
                let mut got_chunks = s.utf8_chunks();
                let mut expected_chars = chunks.iter().copied();
                assert!(got_chunks.eq(expected_chars), "{bytes:?} -> {chunks:?}");
            };
        }
        assert_chunks!(b"hello", Ok("hello"));
        assert_chunks!("ศไทย中华Việt Nam".as_bytes(), Ok("ศไทย中华Việt Nam"));
        assert_chunks!(
            b"Hello\xC2 There\xFF Goodbye",
            Ok("Hello"),
            Err(b"\xC2"),
            Ok(" There"),
            Err(b"\xFF"),
            Ok(" Goodbye"),
        );
        assert_chunks!(
            b"Hello\xC0\x80 There\xE6\x83 Goodbye",
            Ok("Hello"),
            Err(b"\xC0"),
            Err(b"\x80"),
            Ok(" There"),
            Err(b"\xE6\x83"),
            Ok(" Goodbye"),
        );
        assert_chunks!(
            b"\xF5foo\xF5\x80bar",
            Err(b"\xF5"),
            Ok("foo"),
            Err(b"\xF5"),
            Err(b"\x80"),
            Ok("bar"),
        );
        assert_chunks!(
            b"\xF1foo\xF1\x80bar\xF1\x80\x80baz",
            Err(b"\xF1"),
            Ok("foo"),
            Err(b"\xF1\x80"),
            Ok("bar"),
            Err(b"\xF1\x80\x80"),
            Ok("baz"),
        );
        assert_chunks!(
            b"\xF4foo\xF4\x80bar\xF4\xBFbaz",
            Err(b"\xF4"),
            Ok("foo"),
            Err(b"\xF4\x80"),
            Ok("bar"),
            Err(b"\xF4"),
            Err(b"\xBF"),
            Ok("baz"),
        );
        assert_chunks!(
            b"\xF0\x80\x80\x80foo\xF0\x90\x80\x80bar",
            Err(b"\xF0"),
            Err(b"\x80"),
            Err(b"\x80"),
            Err(b"\x80"),
            Ok("foo\u{10000}bar"),
        );
        assert_chunks!(
            b"\xED\xA0\x80foo\xED\xBF\xBFbar",
            Err(b"\xED"),
            Err(b"\xA0"),
            Err(b"\x80"),
            Ok("foo"),
            Err(b"\xED"),
            Err(b"\xBF"),
            Err(b"\xBF"),
            Ok("bar"),
        );
    }

    #[test]
    fn proto_str_chars() {
        macro_rules! assert_chars {
            ($bytes:expr, $chars:expr) => {
                let bytes = $bytes;
                let chars = $chars;
                let s = test_proto_str(bytes);
                let mut got_chars = s.chars();
                let mut expected_chars = chars.into_iter();
                assert!(got_chars.eq(expected_chars), "{bytes:?} -> {chars:?}");
            };
        }
        assert_chars!(b"hello", ['h', 'e', 'l', 'l', 'o']);
        assert_chars!(
            "ศไทย中华Việt Nam".as_bytes(),
            ['ศ', 'ไ', 'ท', 'ย', '中', '华', 'V', 'i', 'ệ', 't', ' ', 'N', 'a', 'm']
        );
        assert_chars!(
            b"Hello\xC2 There\xFF Goodbye",
            [
                'H', 'e', 'l', 'l', 'o', '�', ' ', 'T', 'h', 'e', 'r', 'e', '�', ' ', 'G', 'o',
                'o', 'd', 'b', 'y', 'e'
            ]
        );
        assert_chars!(
            b"Hello\xC0\x80 There\xE6\x83 Goodbye",
            [
                'H', 'e', 'l', 'l', 'o', '�', '�', ' ', 'T', 'h', 'e', 'r', 'e', '�', ' ', 'G',
                'o', 'o', 'd', 'b', 'y', 'e'
            ]
        );
        assert_chars!(b"\xF5foo\xF5\x80bar", ['�', 'f', 'o', 'o', '�', '�', 'b', 'a', 'r']);
        assert_chars!(
            b"\xF1foo\xF1\x80bar\xF1\x80\x80baz",
            ['�', 'f', 'o', 'o', '�', 'b', 'a', 'r', '�', 'b', 'a', 'z']
        );
        assert_chars!(
            b"\xF4foo\xF4\x80bar\xF4\xBFbaz",
            ['�', 'f', 'o', 'o', '�', 'b', 'a', 'r', '�', '�', 'b', 'a', 'z']
        );
        assert_chars!(
            b"\xF0\x80\x80\x80foo\xF0\x90\x80\x80bar",
            ['�', '�', '�', '�', 'f', 'o', 'o', '\u{10000}', 'b', 'a', 'r']
        );
        assert_chars!(
            b"\xED\xA0\x80foo\xED\xBF\xBFbar",
            ['�', '�', '�', 'f', 'o', 'o', '�', '�', '�', 'b', 'a', 'r']
        );
    }
}
