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
use crate::{AsView, IntoProxied, IntoView, MapKey, Mut, MutProxied, Proxied, View};
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
/// Protobuf intends to maintain the invariant that a `string` fields are UTF-8 encoded text, and
/// by default the validity of the UTF-8 encoding is enforced at parse time.
///
/// However, the Rust implementation is designed to zero-copy integrate with C++Proto. C++Proto is
/// designed such that string fields should be valid UTF-8, and generally the validity is checked at
/// parse time, but it is not undefined behavior to set malformed UTF-8 data. For the reason,
/// RustProto uses a 'should-be-UTF-8' types, but it is not considered undefined behavior to set
/// arbitrary &[u8] onto a string field.
///
/// Doing so should be done with great caution however, as it can lead to difficult to debug
/// issues and problems in downstream systems.
///
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
        ProtoStr::from_utf8_unchecked(self.as_bytes())
    }

    pub fn as_bytes(&self) -> &[u8] {
        self.inner.as_bytes()
    }

    /// Converts bytes to a `ProtoString` without a check. Prefer using `.try_into()`
    /// where possible.
    ///
    /// The input `bytes` should be valid UTF-8. Note that unlike with `str` this
    /// method is not unsafe, as the underlying implementations are robust against
    /// invalid UTF-8 and this will not result in language undefined behavior.
    ///
    /// However, `string` fields are intended to maintain the invariant that they
    /// contain valid UTF-8, and the system behavior if invalid UTF-8 is contained may be
    /// poor, including that that you could end up storing malformed data which is not parsable.
    pub fn from_utf8_unchecked(v: &[u8]) -> Self {
        Self { inner: InnerProtoString::from(v) }
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

impl Deref for ProtoString {
    type Target = ProtoStr;

    fn deref(&self) -> &Self::Target {
        self.as_view()
    }
}

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
        Self::from_utf8_unchecked(v.as_bytes())
    }
}

impl TryFrom<&[u8]> for ProtoString {
    type Error = Utf8Error;

    fn try_from(v: &[u8]) -> Result<Self, Self::Error> {
        let s = std::str::from_utf8(v)?;
        Ok(ProtoString::from(s))
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
        ProtoString::from_utf8_unchecked(self.as_bytes())
    }
}

impl IntoProxied<ProtoString> for String {
    fn into_proxied(self, _private: Private) -> ProtoString {
        ProtoString::from(self.as_str())
    }
}

impl IntoProxied<ProtoString> for &String {
    fn into_proxied(self, _private: Private) -> ProtoString {
        ProtoString::from_utf8_unchecked(self.as_bytes())
    }
}

impl IntoProxied<ProtoString> for OsString {
    fn into_proxied(self, private: Private) -> ProtoString {
        self.as_os_str().into_proxied(private)
    }
}

impl IntoProxied<ProtoString> for &OsStr {
    fn into_proxied(self, _private: Private) -> ProtoString {
        ProtoString::from_utf8_unchecked(self.as_encoded_bytes())
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
    pub const fn as_bytes(&self) -> &[u8] {
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
    pub const fn to_str(&self) -> Result<&str, Utf8Error> {
        // Note: cannot use `?` here because of the `const` context.
        match std::str::from_utf8(&self.0) {
            Ok(s) => Ok(s),
            Err(e) => Err(Utf8Error { inner: e }),
        }
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
    pub const fn is_empty(&self) -> bool {
        self.0.is_empty()
    }

    /// Returns the length of `self`.
    ///
    /// Like `&str`, this is a length in bytes, not `char`s or graphemes.
    pub const fn len(&self) -> usize {
        self.0.len()
    }

    /// Converts bytes to a `&ProtoStr` without a check. Prefer using `.try_into()`
    /// where possible.
    ///
    /// The input `bytes` should be valid UTF-8. Note that unlike with `str` this
    /// method is not unsafe, as the underlying implementations are robust against
    /// invalid UTF-8 and this will not result in language undefined behavior.
    ///
    /// However, `string` fields are intended to maintain the invariant that they
    /// contain valid UTF-8, and the system behavior if invalid UTF-8 is contained may be
    /// poor, including that that you could end up storing malformed data which is not parsable.
    pub const fn from_utf8_unchecked(bytes: &[u8]) -> &Self {
        // SAFETY:
        // - `ProtoStr` is `#[repr(transparent)]` over `[u8]`, so it has the same
        //   layout.
        // - `ProtoStr` has the same pointer metadata and element size as `[u8]`.
        unsafe { &*(bytes as *const [u8] as *const Self) }
    }

    /// Interprets a string slice as a `&ProtoStr`.
    pub const fn from_str(string: &str) -> &Self {
        Self::from_utf8_unchecked(string.as_bytes())
    }

    pub const fn is_ascii(&self) -> bool {
        self.0.is_ascii()
    }

    pub fn contains<T>(&self, other: &T) -> bool
    where
        T: AsRef<[u8]> + ?Sized,
    {
        let other = other.as_ref();
        if other.is_empty() {
            return true;
        }
        // Note: this sliding window approach is suboptimal, but simple and correct and can be
        // optimized later if needed.
        self.0.windows(other.len()).any(|window| window == other)
    }

    pub fn starts_with<T>(&self, other: &T) -> bool
    where
        T: AsRef<[u8]> + ?Sized,
    {
        self.0.starts_with(other.as_ref())
    }

    pub fn ends_with<T>(&self, other: &T) -> bool
    where
        T: AsRef<[u8]> + ?Sized,
    {
        self.0.ends_with(other.as_ref())
    }

    pub fn find<T>(&self, other: &T) -> Option<usize>
    where
        T: AsRef<[u8]> + ?Sized,
    {
        let other = other.as_ref();
        if other.is_empty() {
            return Some(0);
        }
        // Note: this sliding window approach is suboptimal, but simple and correct and can be
        // optimized later if needed.
        self.0.windows(other.len()).position(|window| window == other)
    }

    pub const fn trim_ascii(&self) -> &Self {
        Self::from_utf8_unchecked(self.0.trim_ascii())
    }

    pub const fn trim_ascii_start(&self) -> &Self {
        Self::from_utf8_unchecked(self.0.trim_ascii_start())
    }

    pub const fn trim_ascii_end(&self) -> &Self {
        Self::from_utf8_unchecked(self.0.trim_ascii_end())
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
        let s = std::str::from_utf8(val)?;
        Ok(ProtoStr::from_str(s))
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

    #[gtest]
    fn test_proto_string_try_from() -> googletest::Result<()> {
        let valid_utf8 = b"hello";
        let s = ProtoString::try_from(&valid_utf8[..])?;
        verify_eq!(s.as_bytes(), valid_utf8)?;

        let invalid_utf8 = b"\xff";
        let res = ProtoString::try_from(&invalid_utf8[..]);
        verify_that!(res, err(anything()))?;
        Ok(())
    }

    #[gtest]
    fn test_proto_string_from_utf8_unchecked() -> googletest::Result<()> {
        let invalid_utf8 = b"\xff";
        let s = ProtoString::from_utf8_unchecked(invalid_utf8);
        verify_eq!(s.as_bytes(), invalid_utf8)?;
        Ok(())
    }

    #[gtest]
    fn test_proto_str_methods() -> googletest::Result<()> {
        let s = ProtoStr::from_str("  hello world  ");

        // contains
        verify_eq!(s.contains(s), true)?;
        verify_eq!(s.contains("hello"), true)?;
        verify_eq!(s.contains("world"), true)?;
        verify_eq!(s.contains("o w"), true)?;
        verify_eq!(s.contains("xyz"), false)?;
        verify_eq!(s.contains(""), true)?;

        // starts_with / ends_with
        verify_eq!(s.starts_with(s), true)?;
        verify_eq!(s.ends_with(s), true)?;
        verify_eq!(s.starts_with("  he"), true)?;
        verify_eq!(s.ends_with("d  "), true)?;
        verify_eq!(s.starts_with("hel"), false)?;

        // find
        verify_eq!(s.find(s), Some(0))?;
        verify_eq!(s.find("hello"), Some(2))?;
        verify_eq!(s.find("world"), Some(8))?;
        verify_eq!(s.find("xyz"), None)?;
        verify_eq!(s.find(""), Some(0))?;

        // trim
        verify_eq!(s.trim_ascii(), "hello world")?;
        verify_eq!(s.trim_ascii_start(), "hello world  ")?;
        verify_eq!(s.trim_ascii_end(), "  hello world")?;

        Ok(())
    }

    #[gtest]
    fn test_proto_string_deref() -> googletest::Result<()> {
        let s = ProtoString::from("  hello  ");
        verify_eq!(s.contains("hello"), true)?;
        verify_eq!(s.trim_ascii(), "hello")?;

        let s2 = ProtoStr::from_str("he");
        verify_eq!(s.contains(s2), true)?;

        let s2 = ProtoStr::from_str("world");
        verify_eq!(s.contains(s2), false)?;

        Ok(())
    }

    #[gtest]
    fn test_const_proto_str() -> googletest::Result<()> {
        const S: &ProtoStr = ProtoStr::from_str("hello");
        verify_eq!(S.contains("hello"), true)?;

        const S_BYTES: &[u8] = S.as_bytes();
        verify_eq!(S_BYTES, b"hello")?;

        const S_TO_STR: core::result::Result<&str, Utf8Error> = S.to_str();
        verify_eq!(S_TO_STR.unwrap(), "hello")?;

        const S_IS_EMPTY: bool = S.is_empty();
        verify_eq!(S_IS_EMPTY, false)?;
        const EMPTY: &ProtoStr = ProtoStr::from_str("");
        const EMPTY_IS_EMPTY: bool = EMPTY.is_empty();
        verify_eq!(EMPTY_IS_EMPTY, true)?;

        const S_LEN: usize = S.len();
        verify_eq!(S_LEN, 5)?;

        const S_IS_ASCII: bool = S.is_ascii();
        verify_eq!(S_IS_ASCII, true)?;

        const TRIM_ME: &ProtoStr = ProtoStr::from_str("  foo  ");
        const TRIMMED: &ProtoStr = TRIM_ME.trim_ascii();
        const TRIMMED_START: &ProtoStr = TRIM_ME.trim_ascii_start();
        const TRIMMED_END: &ProtoStr = TRIM_ME.trim_ascii_end();
        verify_eq!(TRIMMED.as_bytes(), b"foo")?;
        verify_eq!(TRIMMED_START.as_bytes(), b"foo  ")?;
        verify_eq!(TRIMMED_END.as_bytes(), b"  foo")?;

        const S2: &ProtoStr = ProtoStr::from_utf8_unchecked(b"world");
        verify_eq!(S2.contains("world"), true)?;

        Ok(())
    }
}
