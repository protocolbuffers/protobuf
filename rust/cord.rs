// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use crate::__internal::runtime::{
    InnerOwnedProtoCord, InnerProtoCord, InnerProtoCordChunks, InnerProtoString,
};
use crate::__internal::{Private, SealedInternal};
use crate::{
    AsView, IntoProxied, IntoView, ProtoBytes, ProtoObject, ProtoStr, ProtoString, Proxied, View,
};
use paste::paste;
use std::borrow::Cow;
use std::cmp::{Ordering, PartialEq};
use std::fmt;
use std::iter::{Copied, Flatten, FusedIterator};
use std::ops::Deref;

macro_rules! impl_cord_types {
    ($($t:ty, $vt:ty);*) => {
        paste! { $(
          #[derive(Debug)]
          pub struct [< $t Cord>](Private);

          #[derive(Debug)]
          pub enum [< $t Cow>]<'a> {
            Borrowed(View<'a, $t>),
            Owned($t),
          }

          impl SealedInternal for [< $t Cord>] {}

          impl<'msg> SealedInternal for [< $t Cow>]<'msg> {}

          impl Proxied for [< $t Cord>] {
            type View<'msg> = [< $t Cow>]<'msg>;
          }

          impl AsView for [< $t Cord>] {
            type Proxied = Self;
            fn as_view(&self) -> [< $t Cow>]<'_> {
              unimplemented!("Proto Cord should never be constructed");
            }
          }

          impl<'msg> AsView for [< $t Cow>]<'msg> {
            type Proxied = [< $t Cord>];

            fn as_view(&self) -> [< $t Cow>]<'_> {
              match self {
                [< $t Cow>]::Owned(owned) => [< $t Cow>]::Borrowed((*owned).as_view()),
                [< $t Cow>]::Borrowed(borrowed) => [< $t Cow>]::Borrowed(borrowed),
              }
            }
          }

          impl<'msg> IntoView<'msg> for [< $t Cow>]<'msg> {
            fn into_view<'shorter>(self) -> [< $t Cow>]<'shorter>
            where
                'msg: 'shorter, {
              match self {
                [< $t Cow>]::Owned(owned) => [< $t Cow>]::Owned(owned),
                [< $t Cow>]::Borrowed(borrow) => [< $t Cow>]::Borrowed(borrow.into_view()),
              }
            }
          }

          impl IntoProxied<$t> for [< $t Cow>]<'_> {
              fn into_proxied(self, _private: Private) -> $t {
                match self {
                  [< $t Cow>]::Owned(owned) => owned,
                  [< $t Cow>]::Borrowed(borrowed) => borrowed.into_proxied(Private),
                }
              }
          }

          impl<'a> Deref for [< $t Cow>]<'a> {
            type Target = $vt;

            fn deref(&self) -> View<'_, $t> {
                match self {
                    [< $t Cow>]::Borrowed(borrow) => borrow,
                    [< $t Cow>]::Owned(owned) => (*owned).as_view(),
                }
            }
          }

          impl AsRef<[u8]> for [< $t Cow>]<'_> {
            fn as_ref(&self) -> &[u8] {
                match self {
                  [< $t Cow>]::Borrowed(borrow) => borrow.as_ref(),
                  [< $t Cow>]::Owned(owned) => owned.as_ref(),
                }
            }
          }
          )*
        }
    }
}

impl_cord_types!(
    ProtoString, ProtoStr;
    ProtoBytes, [u8]
);

macro_rules! impl_eq {
  ($($t1:ty, $t2:ty);*) => {
      paste! { $(
        impl PartialEq<$t1> for $t2 {
          fn eq(&self, rhs: &$t1) -> bool {
              AsRef::<[u8]>::as_ref(self) == AsRef::<[u8]>::as_ref(rhs)
          }
        }
        )*
      }
  }
}

impl_eq!(
  ProtoStringCow<'_>, ProtoStringCow<'_>;
  str, ProtoStringCow<'_>;
  ProtoStringCow<'_>, str;
  ProtoBytesCow<'_>, ProtoBytesCow<'_>;
  [u8], ProtoBytesCow<'_>;
  ProtoBytesCow<'_>, [u8]
);

/// A borrowed view of a potentially non-contiguous byte sequence backed by a
/// Protobuf `[ctype = CORD]` field or [`OwnedProtoCord`].
///
/// On the C++ kernel, a `ProtoCord` borrows the underlying `absl::Cord`
/// directly without flattening multi-chunk cords. On the UPB kernel, a
/// `ProtoCord` wraps the contiguous arena-backed byte slice.
#[derive(Copy, Clone)]
pub struct ProtoCord<'msg> {
    pub(crate) inner: InnerProtoCord<'msg>,
}

impl Default for ProtoCord<'_> {
    fn default() -> Self {
        Self::new()
    }
}

impl<'msg> ProtoCord<'msg> {
    /// Creates an empty `ProtoCord`.
    pub const fn new() -> Self {
        Self { inner: InnerProtoCord::empty() }
    }

    /// Creates a `ProtoCord` borrowing a contiguous byte slice without copying.
    pub const fn from_slice(bytes: &'msg [u8]) -> Self {
        Self { inner: InnerProtoCord::from_slice(bytes) }
    }

    /// Creates a `ProtoCord` borrowing a UTF-8 `&str` without copying.
    #[allow(clippy::should_implement_trait)]
    pub const fn from_str(s: &'msg str) -> Self {
        Self::from_slice(s.as_bytes())
    }

    #[doc(hidden)]
    pub fn from_inner(_private: Private, inner: InnerProtoCord<'msg>) -> Self {
        Self { inner }
    }

    /// Returns the total byte length across all chunks.
    pub fn len(&self) -> usize {
        self.inner.len()
    }

    /// Returns `true` if the cord has length 0.
    pub fn is_empty(&self) -> bool {
        self.len() == 0
    }

    /// Returns the cord's contents as a single contiguous slice if it is stored
    /// flat in memory (always `Some` when empty or on the UPB kernel).
    pub fn try_flat(&self) -> Option<&'msg [u8]> {
        self.inner.try_flat()
    }

    /// Returns an iterator over the contiguous non-empty byte chunks of this
    /// cord. Yields zero items when [`Self::is_empty`] is `true`.
    pub fn chunks(&self) -> ProtoCordChunks<'msg> {
        ProtoCordChunks { inner: self.inner.chunks() }
    }

    /// Returns an iterator over every byte in this cord.
    pub fn bytes(&self) -> Copied<Flatten<ProtoCordChunks<'msg>>> {
        self.chunks().flatten().copied()
    }

    /// Copies all bytes of this cord into `dst`.
    ///
    /// # Panics
    /// Panics if `dst.len() != self.len()`.
    pub fn copy_to_slice(&self, dst: &mut [u8]) {
        self.inner.copy_to_slice(dst);
    }

    /// Copies the bytes of this cord into a newly allocated `Vec<u8>`.
    pub fn to_vec(&self) -> Vec<u8> {
        self.inner.to_vec()
    }

    /// Borrows the cord's bytes if flat, or copies them into an owned `Vec<u8>`
    /// if fragmented across multiple chunks.
    pub fn to_cow_bytes(&self) -> Cow<'msg, [u8]> {
        match self.try_flat() {
            Some(flat) => Cow::Borrowed(flat),
            None => Cow::Owned(self.to_vec()),
        }
    }

    /// Validates that the cord contains UTF-8 and returns a borrowed `&str`
    /// when flat or an owned `String` when fragmented across multiple chunks.
    pub fn to_utf8(&self) -> Result<Cow<'msg, str>, std::str::Utf8Error> {
        match self.try_flat() {
            Some(flat) => std::str::from_utf8(flat).map(Cow::Borrowed),
            None => {
                let bytes = self.to_vec();
                std::str::from_utf8(&bytes)?;
                // SAFETY: `bytes` was just validated as UTF-8 above.
                Ok(Cow::Owned(unsafe { String::from_utf8_unchecked(bytes) }))
            }
        }
    }

    /// Creates an [`OwnedProtoCord`] with the same contents.
    ///
    /// On the C++ kernel, this shares the underlying `absl::Cord` reference
    /// count in O(1) when `self` is backed by a C++ `absl::Cord`.
    pub fn to_owned_cord(&self) -> OwnedProtoCord {
        OwnedProtoCord { inner: self.inner.to_owned_cord() }
    }
}

/// Iterator over the contiguous byte chunks of a [`ProtoCord`].
#[derive(Clone)]
pub struct ProtoCordChunks<'msg> {
    inner: InnerProtoCordChunks<'msg>,
}

impl<'msg> Iterator for ProtoCordChunks<'msg> {
    type Item = &'msg [u8];

    fn next(&mut self) -> Option<Self::Item> {
        self.inner.next()
    }
}

impl FusedIterator for ProtoCordChunks<'_> {}

impl fmt::Debug for ProtoCordChunks<'_> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("ProtoCordChunks").finish_non_exhaustive()
    }
}

/// An owned, kernel-agnostic cord container used for zero-copy message
/// serialization/parsing and `[ctype = CORD]` field manipulation.
///
/// On the C++ kernel, `OwnedProtoCord` wraps an owned `absl::Cord` (`Clone` is
/// O(1) reference-counted). On the UPB kernel, `OwnedProtoCord` wraps a
/// `'static` slice or owned `Vec<u8>`.
#[derive(Clone)]
pub struct OwnedProtoCord {
    pub(crate) inner: InnerOwnedProtoCord,
}

impl Default for OwnedProtoCord {
    fn default() -> Self {
        Self::new()
    }
}

impl OwnedProtoCord {
    /// Creates an empty `OwnedProtoCord`.
    pub fn new() -> Self {
        Self { inner: InnerOwnedProtoCord::new() }
    }

    /// Creates an `OwnedProtoCord` referencing a `'static` byte slice without
    /// copying on either kernel.
    pub fn from_static(bytes: &'static [u8]) -> Self {
        Self { inner: InnerOwnedProtoCord::from_static(bytes) }
    }

    /// Creates an `OwnedProtoCord` referencing a `'static` UTF-8 string without
    /// copying on either kernel.
    pub fn from_static_str(s: &'static str) -> Self {
        Self::from_static(s.as_bytes())
    }

    #[doc(hidden)]
    pub fn from_inner(_private: Private, inner: InnerOwnedProtoCord) -> Self {
        Self { inner }
    }

    #[doc(hidden)]
    pub fn into_inner(self, _private: Private) -> InnerOwnedProtoCord {
        self.inner
    }

    #[doc(hidden)]
    pub fn as_inner(&self, _private: Private) -> &InnerOwnedProtoCord {
        &self.inner
    }

    /// Borrows this owned cord as a [`ProtoCord`].
    pub fn as_view(&self) -> ProtoCord<'_> {
        ProtoCord { inner: self.inner.as_view() }
    }

    /// Returns the total byte length across all chunks.
    pub fn len(&self) -> usize {
        self.as_view().len()
    }

    /// Returns `true` if the cord has length 0.
    pub fn is_empty(&self) -> bool {
        self.len() == 0
    }

    /// Returns the cord's contents as a single contiguous slice if flat.
    pub fn try_flat(&self) -> Option<&[u8]> {
        self.as_view().try_flat()
    }

    /// Returns an iterator over the contiguous non-empty byte chunks.
    pub fn chunks(&self) -> ProtoCordChunks<'_> {
        self.as_view().chunks()
    }

    /// Returns an iterator over every byte in this cord.
    pub fn bytes(&self) -> Copied<Flatten<ProtoCordChunks<'_>>> {
        self.as_view().bytes()
    }

    /// Copies all bytes of this cord into `dst`.
    ///
    /// # Panics
    /// Panics if `dst.len() != self.len()`.
    pub fn copy_to_slice(&self, dst: &mut [u8]) {
        self.as_view().copy_to_slice(dst);
    }

    /// Copies the bytes of this cord into a `Vec<u8>`.
    pub fn to_vec(&self) -> Vec<u8> {
        self.as_view().to_vec()
    }

    /// Consumes `self` and returns its contents as a `Vec<u8>` (moving without
    /// reallocating on the UPB kernel when already backed by a `Vec<u8>`).
    pub fn into_vec(self) -> Vec<u8> {
        self.inner.into_vec()
    }

    /// Validates UTF-8 and returns a borrowed `&str` when flat or an owned
    /// `String` when fragmented.
    pub fn to_utf8(&self) -> Result<Cow<'_, str>, std::str::Utf8Error> {
        self.as_view().to_utf8()
    }
}

impl SealedInternal for OwnedProtoCord {}
impl SealedInternal for ProtoCord<'_> {}

impl ProtoObject for OwnedProtoCord {
    type View<'msg> = ProtoCord<'msg>;
}

impl AsView for OwnedProtoCord {
    type Proxied = Self;

    fn as_view(&self) -> ProtoCord<'_> {
        OwnedProtoCord::as_view(self)
    }
}

impl<'msg> AsView for ProtoCord<'msg> {
    type Proxied = OwnedProtoCord;

    fn as_view(&self) -> ProtoCord<'_> {
        *self
    }
}

impl<'msg> IntoView<'msg> for ProtoCord<'msg> {
    fn into_view<'shorter>(self) -> ProtoCord<'shorter>
    where
        'msg: 'shorter,
    {
        self
    }
}

impl<'msg> From<&'msg [u8]> for ProtoCord<'msg> {
    fn from(value: &'msg [u8]) -> Self {
        Self::from_slice(value)
    }
}

impl<'msg, const N: usize> From<&'msg [u8; N]> for ProtoCord<'msg> {
    fn from(value: &'msg [u8; N]) -> Self {
        Self::from_slice(value.as_slice())
    }
}

impl<'msg> From<&'msg str> for ProtoCord<'msg> {
    fn from(value: &'msg str) -> Self {
        Self::from_str(value)
    }
}

impl<'msg> From<&'msg ProtoStr> for ProtoCord<'msg> {
    fn from(value: &'msg ProtoStr) -> Self {
        Self::from_slice(value.as_bytes())
    }
}

impl<'msg> From<&'msg OwnedProtoCord> for ProtoCord<'msg> {
    fn from(value: &'msg OwnedProtoCord) -> Self {
        value.as_view()
    }
}

fn write_escaped_chunks(
    name: &str,
    chunks: ProtoCordChunks<'_>,
    f: &mut fmt::Formatter<'_>,
) -> fmt::Result {
    write!(f, "{name}(\"")?;
    for chunk in chunks {
        for &byte in chunk {
            for escaped in std::ascii::escape_default(byte) {
                write!(f, "{}", escaped as char)?;
            }
        }
    }
    write!(f, "\")")
}

impl fmt::Debug for ProtoCord<'_> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write_escaped_chunks("ProtoCord", self.chunks(), f)
    }
}

impl fmt::Debug for OwnedProtoCord {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write_escaped_chunks("OwnedProtoCord", self.chunks(), f)
    }
}

fn cmp_cord_chunks(mut a: ProtoCordChunks<'_>, mut b: ProtoCordChunks<'_>) -> Ordering {
    let mut sa: &[u8] = &[];
    let mut sb: &[u8] = &[];
    loop {
        if sa.is_empty() {
            match a.next() {
                Some(next) => sa = next,
                None => {
                    return if sb.is_empty() && b.next().is_none() {
                        Ordering::Equal
                    } else {
                        Ordering::Less
                    };
                }
            }
        }
        if sb.is_empty() {
            match b.next() {
                Some(next) => sb = next,
                None => return Ordering::Greater,
            }
        }
        let n = sa.len().min(sb.len());
        match sa[..n].cmp(&sb[..n]) {
            Ordering::Equal => {
                sa = &sa[n..];
                sb = &sb[n..];
            }
            non_eq => return non_eq,
        }
    }
}

impl<'other> PartialEq<ProtoCord<'other>> for ProtoCord<'_> {
    fn eq(&self, other: &ProtoCord<'other>) -> bool {
        self.inner.eq_cord(&other.inner)
    }
}

impl Eq for ProtoCord<'_> {}

impl<'other> PartialOrd<ProtoCord<'other>> for ProtoCord<'_> {
    fn partial_cmp(&self, other: &ProtoCord<'other>) -> Option<Ordering> {
        Some(cmp_cord_chunks(self.chunks(), other.chunks()))
    }
}

impl Ord for ProtoCord<'_> {
    fn cmp(&self, other: &Self) -> Ordering {
        cmp_cord_chunks(self.chunks(), other.chunks())
    }
}

impl PartialEq for OwnedProtoCord {
    fn eq(&self, other: &Self) -> bool {
        self.as_view() == other.as_view()
    }
}

impl Eq for OwnedProtoCord {}

impl PartialOrd for OwnedProtoCord {
    fn partial_cmp(&self, other: &Self) -> Option<Ordering> {
        Some(self.cmp(other))
    }
}

impl Ord for OwnedProtoCord {
    fn cmp(&self, other: &Self) -> Ordering {
        self.as_view().cmp(&other.as_view())
    }
}

impl PartialEq<OwnedProtoCord> for ProtoCord<'_> {
    fn eq(&self, other: &OwnedProtoCord) -> bool {
        *self == other.as_view()
    }
}

impl PartialEq<ProtoCord<'_>> for OwnedProtoCord {
    fn eq(&self, other: &ProtoCord<'_>) -> bool {
        self.as_view() == *other
    }
}

macro_rules! impl_cord_bytes_eq {
    ($($rhs:ty),* $(,)?) => {
        $(
            impl PartialEq<$rhs> for ProtoCord<'_> {
                fn eq(&self, other: &$rhs) -> bool {
                    self.inner.eq_bytes(other.as_ref())
                }
            }

            impl PartialEq<ProtoCord<'_>> for $rhs {
                fn eq(&self, other: &ProtoCord<'_>) -> bool {
                    other.inner.eq_bytes(self.as_ref())
                }
            }

            impl PartialEq<$rhs> for OwnedProtoCord {
                fn eq(&self, other: &$rhs) -> bool {
                    self.as_view().inner.eq_bytes(other.as_ref())
                }
            }

            impl PartialEq<OwnedProtoCord> for $rhs {
                fn eq(&self, other: &OwnedProtoCord) -> bool {
                    other.as_view().inner.eq_bytes(self.as_ref())
                }
            }
        )*
    };
}

impl_cord_bytes_eq!(
    [u8],
    &[u8],
    Vec<u8>,
    str,
    &str,
    String,
    ProtoStr,
    &ProtoStr,
    ProtoBytes,
    ProtoString,
    ProtoBytesCow<'_>,
    ProtoStringCow<'_>,
);

impl<const N: usize> PartialEq<[u8; N]> for ProtoCord<'_> {
    fn eq(&self, other: &[u8; N]) -> bool {
        self.inner.eq_bytes(other.as_slice())
    }
}

impl<const N: usize> PartialEq<ProtoCord<'_>> for [u8; N] {
    fn eq(&self, other: &ProtoCord<'_>) -> bool {
        other.inner.eq_bytes(self.as_slice())
    }
}

impl<const N: usize> PartialEq<&[u8; N]> for ProtoCord<'_> {
    fn eq(&self, other: &&[u8; N]) -> bool {
        self.inner.eq_bytes(other.as_slice())
    }
}

impl<const N: usize> PartialEq<ProtoCord<'_>> for &[u8; N] {
    fn eq(&self, other: &ProtoCord<'_>) -> bool {
        other.inner.eq_bytes(self.as_slice())
    }
}

impl<const N: usize> PartialEq<[u8; N]> for OwnedProtoCord {
    fn eq(&self, other: &[u8; N]) -> bool {
        self.as_view().inner.eq_bytes(other.as_slice())
    }
}

impl<const N: usize> PartialEq<OwnedProtoCord> for [u8; N] {
    fn eq(&self, other: &OwnedProtoCord) -> bool {
        other.as_view().inner.eq_bytes(self.as_slice())
    }
}

impl<const N: usize> PartialEq<&[u8; N]> for OwnedProtoCord {
    fn eq(&self, other: &&[u8; N]) -> bool {
        self.as_view().inner.eq_bytes(other.as_slice())
    }
}

impl<const N: usize> PartialEq<OwnedProtoCord> for &[u8; N] {
    fn eq(&self, other: &OwnedProtoCord) -> bool {
        other.as_view().inner.eq_bytes(self.as_slice())
    }
}

/// Internal value representation produced by [`IntoProtoCord`].
#[doc(hidden)]
pub enum ProtoCordValue<'a> {
    BorrowedSlice(&'a [u8]),
    BorrowedCord(ProtoCord<'a>),
    OwnedCord(OwnedProtoCord),
    OwnedVec(Vec<u8>),
    OwnedProtoString(InnerProtoString),
}

/// Conversion trait accepted by `set_<field>_cord`, `parse_from_cord`, and
/// `clear_and_parse_cord`.
///
/// Transfers ownership or reference counts without copying whenever supported
/// by the active kernel (for example `OwnedProtoCord`, `ProtoCord`, `Vec<u8>`,
/// `String`, `Box<[u8]>`, `ProtoBytes`, and `ProtoString` on the C++ kernel).
///
/// Note: When used with `set_<field>_cord` on a `string [ctype = CORD]` field,
/// callers should provide valid UTF-8 bytes to uphold Protobuf `string` wire
/// expectations.
///
/// This trait must not be implemented on types outside the Protobuf codegen and
/// runtime. We expect it to change in backwards incompatible ways in the
/// future.
pub trait IntoProtoCord {
    #[doc(hidden)]
    fn with_cord_value<R>(
        self,
        _private: Private,
        f: impl for<'a> FnOnce(ProtoCordValue<'a>) -> R,
    ) -> R;
}

impl IntoProtoCord for ProtoCord<'_> {
    fn with_cord_value<R>(
        self,
        _private: Private,
        f: impl for<'a> FnOnce(ProtoCordValue<'a>) -> R,
    ) -> R {
        f(ProtoCordValue::BorrowedCord(self))
    }
}

impl IntoProtoCord for &ProtoCord<'_> {
    fn with_cord_value<R>(
        self,
        _private: Private,
        f: impl for<'a> FnOnce(ProtoCordValue<'a>) -> R,
    ) -> R {
        f(ProtoCordValue::BorrowedCord(*self))
    }
}

impl IntoProtoCord for OwnedProtoCord {
    fn with_cord_value<R>(
        self,
        _private: Private,
        f: impl for<'a> FnOnce(ProtoCordValue<'a>) -> R,
    ) -> R {
        f(ProtoCordValue::OwnedCord(self))
    }
}

impl IntoProtoCord for &OwnedProtoCord {
    fn with_cord_value<R>(
        self,
        _private: Private,
        f: impl for<'a> FnOnce(ProtoCordValue<'a>) -> R,
    ) -> R {
        f(ProtoCordValue::BorrowedCord(self.as_view()))
    }
}

impl IntoProtoCord for &[u8] {
    fn with_cord_value<R>(
        self,
        _private: Private,
        f: impl for<'a> FnOnce(ProtoCordValue<'a>) -> R,
    ) -> R {
        f(ProtoCordValue::BorrowedSlice(self))
    }
}

impl<const N: usize> IntoProtoCord for &[u8; N] {
    fn with_cord_value<R>(
        self,
        _private: Private,
        f: impl for<'a> FnOnce(ProtoCordValue<'a>) -> R,
    ) -> R {
        f(ProtoCordValue::BorrowedSlice(self.as_slice()))
    }
}

impl<const N: usize> IntoProtoCord for [u8; N] {
    fn with_cord_value<R>(
        self,
        _private: Private,
        f: impl for<'a> FnOnce(ProtoCordValue<'a>) -> R,
    ) -> R {
        f(ProtoCordValue::BorrowedSlice(&self))
    }
}

impl IntoProtoCord for Vec<u8> {
    fn with_cord_value<R>(
        self,
        _private: Private,
        f: impl for<'a> FnOnce(ProtoCordValue<'a>) -> R,
    ) -> R {
        f(ProtoCordValue::OwnedVec(self))
    }
}

impl IntoProtoCord for &Vec<u8> {
    fn with_cord_value<R>(
        self,
        _private: Private,
        f: impl for<'a> FnOnce(ProtoCordValue<'a>) -> R,
    ) -> R {
        f(ProtoCordValue::BorrowedSlice(self.as_slice()))
    }
}

impl IntoProtoCord for Box<[u8]> {
    fn with_cord_value<R>(
        self,
        _private: Private,
        f: impl for<'a> FnOnce(ProtoCordValue<'a>) -> R,
    ) -> R {
        f(ProtoCordValue::OwnedVec(self.into_vec()))
    }
}

impl IntoProtoCord for &str {
    fn with_cord_value<R>(
        self,
        _private: Private,
        f: impl for<'a> FnOnce(ProtoCordValue<'a>) -> R,
    ) -> R {
        f(ProtoCordValue::BorrowedSlice(self.as_bytes()))
    }
}

impl IntoProtoCord for String {
    fn with_cord_value<R>(
        self,
        _private: Private,
        f: impl for<'a> FnOnce(ProtoCordValue<'a>) -> R,
    ) -> R {
        f(ProtoCordValue::OwnedVec(self.into_bytes()))
    }
}

impl IntoProtoCord for &String {
    fn with_cord_value<R>(
        self,
        _private: Private,
        f: impl for<'a> FnOnce(ProtoCordValue<'a>) -> R,
    ) -> R {
        f(ProtoCordValue::BorrowedSlice(self.as_bytes()))
    }
}

impl IntoProtoCord for &ProtoStr {
    fn with_cord_value<R>(
        self,
        _private: Private,
        f: impl for<'a> FnOnce(ProtoCordValue<'a>) -> R,
    ) -> R {
        f(ProtoCordValue::BorrowedSlice(self.as_bytes()))
    }
}

impl IntoProtoCord for Cow<'_, [u8]> {
    fn with_cord_value<R>(
        self,
        _private: Private,
        f: impl for<'a> FnOnce(ProtoCordValue<'a>) -> R,
    ) -> R {
        match self {
            Cow::Borrowed(b) => f(ProtoCordValue::BorrowedSlice(b)),
            Cow::Owned(v) => f(ProtoCordValue::OwnedVec(v)),
        }
    }
}

impl IntoProtoCord for Cow<'_, str> {
    fn with_cord_value<R>(
        self,
        _private: Private,
        f: impl for<'a> FnOnce(ProtoCordValue<'a>) -> R,
    ) -> R {
        match self {
            Cow::Borrowed(s) => f(ProtoCordValue::BorrowedSlice(s.as_bytes())),
            Cow::Owned(s) => f(ProtoCordValue::OwnedVec(s.into_bytes())),
        }
    }
}

impl IntoProtoCord for ProtoBytes {
    fn with_cord_value<R>(
        self,
        _private: Private,
        f: impl for<'a> FnOnce(ProtoCordValue<'a>) -> R,
    ) -> R {
        f(ProtoCordValue::OwnedProtoString(self.into_inner(Private)))
    }
}

impl IntoProtoCord for &ProtoBytes {
    fn with_cord_value<R>(
        self,
        _private: Private,
        f: impl for<'a> FnOnce(ProtoCordValue<'a>) -> R,
    ) -> R {
        f(ProtoCordValue::BorrowedSlice(self.as_view()))
    }
}

impl IntoProtoCord for ProtoString {
    fn with_cord_value<R>(
        self,
        _private: Private,
        f: impl for<'a> FnOnce(ProtoCordValue<'a>) -> R,
    ) -> R {
        f(ProtoCordValue::OwnedProtoString(self.into_inner(Private)))
    }
}

impl IntoProtoCord for &ProtoString {
    fn with_cord_value<R>(
        self,
        _private: Private,
        f: impl for<'a> FnOnce(ProtoCordValue<'a>) -> R,
    ) -> R {
        f(ProtoCordValue::BorrowedSlice(self.as_view().as_bytes()))
    }
}

impl IntoProtoCord for ProtoBytesCow<'_> {
    fn with_cord_value<R>(
        self,
        _private: Private,
        f: impl for<'a> FnOnce(ProtoCordValue<'a>) -> R,
    ) -> R {
        match self {
            ProtoBytesCow::Borrowed(b) => f(ProtoCordValue::BorrowedSlice(b)),
            ProtoBytesCow::Owned(o) => f(ProtoCordValue::OwnedProtoString(o.into_inner(Private))),
        }
    }
}

impl IntoProtoCord for ProtoStringCow<'_> {
    fn with_cord_value<R>(
        self,
        _private: Private,
        f: impl for<'a> FnOnce(ProtoCordValue<'a>) -> R,
    ) -> R {
        match self {
            ProtoStringCow::Borrowed(s) => f(ProtoCordValue::BorrowedSlice(s.as_bytes())),
            ProtoStringCow::Owned(o) => f(ProtoCordValue::OwnedProtoString(o.into_inner(Private))),
        }
    }
}

macro_rules! impl_from_for_owned_proto_cord {
    ($($src:ty),* $(,)?) => {
        $(
            impl From<$src> for OwnedProtoCord {
                fn from(value: $src) -> Self {
                    Self { inner: InnerOwnedProtoCord::from_input(value) }
                }
            }
        )*
    };
}

impl_from_for_owned_proto_cord!(
    ProtoCord<'_>,
    &ProtoCord<'_>,
    &OwnedProtoCord,
    &[u8],
    Vec<u8>,
    &Vec<u8>,
    Box<[u8]>,
    &str,
    String,
    &String,
    &ProtoStr,
    Cow<'_, [u8]>,
    Cow<'_, str>,
    ProtoBytes,
    &ProtoBytes,
    ProtoString,
    &ProtoString,
    ProtoBytesCow<'_>,
    ProtoStringCow<'_>,
);

impl<const N: usize> From<&[u8; N]> for OwnedProtoCord {
    fn from(value: &[u8; N]) -> Self {
        Self { inner: InnerOwnedProtoCord::from_input(value) }
    }
}

impl<const N: usize> From<[u8; N]> for OwnedProtoCord {
    fn from(value: [u8; N]) -> Self {
        Self { inner: InnerOwnedProtoCord::from_input(value) }
    }
}

impl IntoProxied<OwnedProtoCord> for ProtoCord<'_> {
    fn into_proxied(self, _private: Private) -> OwnedProtoCord {
        self.to_owned_cord()
    }
}

impl IntoProxied<ProtoBytes> for ProtoCord<'_> {
    fn into_proxied(self, _private: Private) -> ProtoBytes {
        match self.try_flat() {
            Some(flat) => ProtoBytes::from(flat),
            None => ProtoBytes::from(self.to_vec().as_slice()),
        }
    }
}

impl IntoProxied<ProtoBytes> for OwnedProtoCord {
    fn into_proxied(self, _private: Private) -> ProtoBytes {
        self.as_view().into_proxied(Private)
    }
}
