// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

//! Items specific to `bytes` and `string` fields.
#![allow(dead_code)]
#![allow(unused)]

use crate::__internal::runtime::InnerProtoString;
use crate::__internal::{Private, SealedInternal};
use crate::{AsView, IntoProxied, IntoView, MapKey, Mut, MutProxied, Optional, Proxied, View};
use std::borrow::Cow;
use std::cmp::{Eq, Ord, Ordering, PartialEq, PartialOrd};
use std::convert::{AsMut, AsRef};
use std::ffi::{OsStr, OsString};
use std::fmt;
use std::hash::{Hash, Hasher};
use std::iter;
use std::ops::{Deref, DerefMut};
use std::ptr;
use std::rc::Rc;
use std::sync::Arc;

pub struct ProtoBytes {
    pub(crate) inner: InnerProtoString,
}

impl ProtoBytes {
    // Returns the kernel-specific container. This method is private in spirit and
    // must not be called by a user.
    #[doc(hidden)]
    pub fn into_inner(self, _private: Private) -> InnerProtoString {
        self.inner
    }

    #[doc(hidden)]
    pub fn from_inner(_private: Private, inner: InnerProtoString) -> ProtoBytes {
        Self { inner }
    }

    pub fn as_view(&self) -> &[u8] {
        self.inner.as_bytes()
    }
}

impl AsRef<[u8]> for ProtoBytes {
    fn as_ref(&self) -> &[u8] {
        self.inner.as_bytes()
    }
}

impl From<&[u8]> for ProtoBytes {
    fn from(v: &[u8]) -> ProtoBytes {
        ProtoBytes { inner: InnerProtoString::from(v) }
    }
}

impl<const N: usize> From<&[u8; N]> for ProtoBytes {
    fn from(v: &[u8; N]) -> ProtoBytes {
        ProtoBytes { inner: InnerProtoString::from(v.as_ref()) }
    }
}

impl SealedInternal for ProtoBytes {}

impl Proxied for ProtoBytes {
    type View<'msg> = &'msg [u8];
}

impl AsView for ProtoBytes {
    type Proxied = Self;

    fn as_view(&self) -> &[u8] {
        self.as_view()
    }
}

impl IntoProxied<ProtoBytes> for &[u8] {
    fn into_proxied(self, _private: Private) -> ProtoBytes {
        ProtoBytes::from(self)
    }
}

impl<const N: usize> IntoProxied<ProtoBytes> for &[u8; N] {
    fn into_proxied(self, _private: Private) -> ProtoBytes {
        ProtoBytes::from(self.as_ref())
    }
}

impl IntoProxied<ProtoBytes> for Vec<u8> {
    fn into_proxied(self, _private: Private) -> ProtoBytes {
        ProtoBytes::from(AsRef::<[u8]>::as_ref(&self))
    }
}

impl IntoProxied<ProtoBytes> for &Vec<u8> {
    fn into_proxied(self, _private: Private) -> ProtoBytes {
        ProtoBytes::from(AsRef::<[u8]>::as_ref(self))
    }
}

impl IntoProxied<ProtoBytes> for Box<[u8]> {
    fn into_proxied(self, _private: Private) -> ProtoBytes {
        ProtoBytes::from(AsRef::<[u8]>::as_ref(&self))
    }
}

impl IntoProxied<ProtoBytes> for Cow<'_, [u8]> {
    fn into_proxied(self, _private: Private) -> ProtoBytes {
        ProtoBytes::from(AsRef::<[u8]>::as_ref(&self))
    }
}

impl IntoProxied<ProtoBytes> for Rc<[u8]> {
    fn into_proxied(self, _private: Private) -> ProtoBytes {
        ProtoBytes::from(AsRef::<[u8]>::as_ref(&self))
    }
}

impl IntoProxied<ProtoBytes> for Arc<[u8]> {
    fn into_proxied(self, _private: Private) -> ProtoBytes {
        ProtoBytes::from(AsRef::<[u8]>::as_ref(&self))
    }
}

impl SealedInternal for &[u8] {}

impl AsView for &[u8] {
    type Proxied = ProtoBytes;

    fn as_view(&self) -> &[u8] {
        self
    }
}

impl<'msg> IntoView<'msg> for &'msg [u8] {
    fn into_view<'shorter>(self) -> &'shorter [u8]
    where
        'msg: 'shorter,
    {
        self
    }
}

/// The bytes were not valid UTF-8.
#[derive(Debug, PartialEq)]
pub struct Utf8Error {
    pub(crate) inner: std::str::Utf8Error,
}
impl std::fmt::Display for Utf8Error {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        self.inner.fmt(f)
    }
}

impl std::error::Error for Utf8Error {}

impl From<std::str::Utf8Error> for Utf8Error {
    fn from(inner: std::str::Utf8Error) -> Utf8Error {
        Utf8Error { inner }
    }
}

/// An owned type representing protobuf `string` field's contents.
///
/// # UTF-8
///
/// Protobuf [docs] state that a `string` field contains UTF-8 encoded text.
/// However, not every runtime enforces this, and the Rust runtime is designed
/// to integrate with other runtimes with FFI, like C++.
///
/// `ProtoString` represents a string type that is expected to contain valid
/// UTF-8. However, `ProtoString` is not validated, so users must
/// call [`ProtoString::to_string`] to perform a (possibly runtime-elided) UTF-8
/// validation check. This validation should rarely fail in pure Rust programs,
/// but is necessary to prevent UB when interacting with C++, or other languages
/// with looser restrictions.
///
///
/// # `Display` and `ToString`
/// `ProtoString` is ordinarily UTF-8 and so implements `Display`. If there are
/// any invalid UTF-8 sequences, they are replaced with [`U+FFFD REPLACEMENT
/// CHARACTER`]. Because anything implementing `Display` also implements
/// `ToString`, `ProtoString::to_string()` is equivalent to
/// `String::from_utf8_lossy(proto_string.as_bytes()).into_owned()`.
///
/// [`U+FFFD REPLACEMENT CHARACTER`]: std::char::REPLACEMENT_CHARACTER
pub struct ProtoString {
    pub(crate) inner: InnerProtoString,
}

impl ProtoString {
    pub fn as_view(&self) -> &ProtoStr {
        unsafe { ProtoStr::from_utf8_unchecked(self.as_bytes()) }
    }

    pub fn as_bytes(&self) -> &[u8] {
        self.inner.as_bytes()
    }

    // Returns the kernel-specific container. This method is private in spirit and
    // must not be called by a user.
    #[doc(hidden)]
    pub fn into_inner(self, _private: Private) -> InnerProtoString {
        self.inner
    }

    #[doc(hidden)]
    pub fn from_inner(_private: Private, inner: InnerProtoString) -> ProtoString {
        Self { inner }
    }
}

impl SealedInternal for ProtoString {}

impl AsRef<[u8]> for ProtoString {
    fn as_ref(&self) -> &[u8] {
        self.inner.as_bytes()
    }
}

impl From<ProtoString> for ProtoBytes {
    fn from(v: ProtoString) -> Self {
        ProtoBytes { inner: v.inner }
    }
}

impl From<&str> for ProtoString {
    fn from(v: &str) -> Self {
        Self::from(v.as_bytes())
    }
}

impl From<&[u8]> for ProtoString {
    fn from(v: &[u8]) -> Self {
        Self { inner: InnerProtoString::from(v) }
    }
}

impl SealedInternal for &str {}

impl SealedInternal for &ProtoStr {}

impl IntoProxied<ProtoString> for &str {
    fn into_proxied(self, _private: Private) -> ProtoString {
        ProtoString::from(self)
    }
}

impl IntoProxied<ProtoString> for &ProtoStr {
    fn into_proxied(self, _private: Private) -> ProtoString {
        ProtoString::from(self.as_bytes())
    }
}

impl IntoProxied<ProtoString> for String {
    fn into_proxied(self, _private: Private) -> ProtoString {
        ProtoString::from(self.as_str())
    }
}

impl IntoProxied<ProtoString> for &String {
    fn into_proxied(self, _private: Private) -> ProtoString {
        ProtoString::from(self.as_bytes())
    }
}

impl IntoProxied<ProtoString> for OsString {
    fn into_proxied(self, private: Private) -> ProtoString {
        self.as_os_str().into_proxied(private)
    }
}

impl IntoProxied<ProtoString> for &OsStr {
    fn into_proxied(self, _private: Private) -> ProtoString {
        ProtoString::from(self.as_encoded_bytes())
    }
}

impl IntoProxied<ProtoString> for Box<str> {
    fn into_proxied(self, _private: Private) -> ProtoString {
        ProtoString::from(AsRef::<str>::as_ref(&self))
    }
}

impl IntoProxied<ProtoString> for Cow<'_, str> {
    fn into_proxied(self, _private: Private) -> ProtoString {
        ProtoString::from(AsRef::<str>::as_ref(&self))
    }
}

impl IntoProxied<ProtoString> for Rc<str> {
    fn into_proxied(self, _private: Private) -> ProtoString {
        ProtoString::from(AsRef::<str>::as_ref(&self))
    }
}

impl IntoProxied<ProtoString> for Arc<str> {
    fn into_proxied(self, _private: Private) -> ProtoString {
        ProtoString::from(AsRef::<str>::as_ref(&self))
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
        write!(f, "\"");
        for chunk in self.as_bytes().utf8_chunks() {
            for ch in chunk.valid().chars() {
                write!(f, "{}", ch.escape_debug());
            }
            for byte in chunk.invalid() {
                // Format byte as \xff.
                write!(f, "\\x{:02X}", byte);
            }
        }
        write!(f, "\"");
        Ok(())
    }
}

impl fmt::Display for ProtoStr {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        fmt::Display::fmt(&String::from_utf8_lossy(self.as_bytes()), f)?;
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

impl Proxied for ProtoString {
    type View<'msg> = &'msg ProtoStr;
}

impl MapKey for ProtoString {}

impl AsView for ProtoString {
    type Proxied = Self;

    fn as_view(&self) -> &ProtoStr {
        self.as_view()
    }
}

impl AsView for &ProtoStr {
    type Proxied = ProtoString;

    fn as_view(&self) -> &ProtoStr {
        self
    }
}

impl<'msg> IntoView<'msg> for &'msg ProtoStr {
    fn into_view<'shorter>(self) -> &'shorter ProtoStr
    where
        'msg: 'shorter,
    {
        self
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
    // `ProtoStr` against protobuf types
    <()> ProtoStr => ProtoStr,

    // `ProtoStr` against foreign types
    <()> ProtoStr => str,
    <()> str => ProtoStr,
);

impl std::fmt::Debug for ProtoString {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> Result<(), std::fmt::Error> {
        std::fmt::Debug::fmt(self.as_view(), f)
    }
}

impl std::fmt::Debug for ProtoBytes {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> Result<(), std::fmt::Error> {
        std::fmt::Debug::fmt(self.as_view(), f)
    }
}

unsafe impl Sync for ProtoString {}
unsafe impl Send for ProtoString {}

unsafe impl Send for ProtoBytes {}
unsafe impl Sync for ProtoBytes {}

#[cfg(test)]
mod tests {
    use super::*;
    use googletest::prelude::*;

    // TODO: Add unit tests

    // Shorter and safe utility function to construct `ProtoStr` from bytes for
    // testing.
    fn test_proto_str(bytes: &[u8]) -> &ProtoStr {
        // SAFETY: The runtime that this test executes under does not elide UTF-8 checks
        // inside of `ProtoStr`.
        unsafe { ProtoStr::from_utf8_unchecked(bytes) }
    }
}
