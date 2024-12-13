// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use std::fmt::{self, Debug};
use std::iter;
use std::iter::FusedIterator;
/// Repeated scalar fields are implemented around the runtime-specific
/// `RepeatedField` struct. `RepeatedField` stores an opaque pointer to the
/// runtime-specific representation of a repeated scalar (`upb_Array*` on upb,
/// and `RepeatedField<T>*` on cpp).
use std::marker::PhantomData;

use crate::{
    AsMut, AsView, IntoMut, IntoProxied, IntoView, Mut, MutProxied, MutProxy, Proxied, Proxy, View,
    ViewProxy,
    __internal::runtime::{InnerRepeated, InnerRepeatedMut, RawRepeatedField},
    __internal::{Private, SealedInternal},
};

/// Views the elements in a `repeated` field of `T`.
#[repr(transparent)]
pub struct RepeatedView<'msg, T> {
    // This does not need to carry an arena in upb, so it can be just the raw repeated field
    raw: RawRepeatedField,
    _phantom: PhantomData<&'msg T>,
}

impl<'msg, T> Copy for RepeatedView<'msg, T> {}
impl<'msg, T> Clone for RepeatedView<'msg, T> {
    fn clone(&self) -> Self {
        *self
    }
}

unsafe impl<'msg, T> Sync for RepeatedView<'msg, T> {}
unsafe impl<'msg, T> Send for RepeatedView<'msg, T> {}

impl<'msg, T> Debug for RepeatedView<'msg, T> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("RepeatedView").field("raw", &self.raw).finish()
    }
}

/// Mutates the elements in a `repeated` field of `T`.
pub struct RepeatedMut<'msg, T> {
    pub(crate) inner: InnerRepeatedMut<'msg>,
    _phantom: PhantomData<&'msg mut T>,
}

unsafe impl<'msg, T> Sync for RepeatedMut<'msg, T> {}

impl<'msg, T> Debug for RepeatedMut<'msg, T> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("RepeatedMut").field("raw", &self.inner.raw).finish()
    }
}

#[doc(hidden)]
impl<'msg, T> RepeatedView<'msg, T> {
    #[doc(hidden)]
    #[inline]
    pub fn as_raw(&self, _private: Private) -> RawRepeatedField {
        self.raw
    }

    /// # Safety
    /// - `inner` must be valid to read from for `'msg`
    #[doc(hidden)]
    #[inline]
    pub unsafe fn from_raw(_private: Private, raw: RawRepeatedField) -> Self {
        Self { raw, _phantom: PhantomData }
    }
}

impl<'msg, T> RepeatedView<'msg, T>
where
    T: ProxiedInRepeated + 'msg,
{
    /// Gets the length of the repeated field.
    #[inline]
    pub fn len(&self) -> usize {
        T::repeated_len(*self)
    }

    /// Returns true if the repeated field has no values.
    #[inline]
    pub fn is_empty(&self) -> bool {
        self.len() == 0
    }

    /// Gets the value at `index`.
    ///
    /// Returns `None` if `index > len`.
    #[inline]
    pub fn get(self, index: usize) -> Option<View<'msg, T>> {
        if index >= self.len() {
            return None;
        }
        // SAFETY: `index` has been checked to be in-bounds
        Some(unsafe { self.get_unchecked(index) })
    }

    /// Gets the value at `index` without bounds-checking.
    ///
    /// # Safety
    /// Undefined behavior if `index >= len`
    #[inline]
    pub unsafe fn get_unchecked(self, index: usize) -> View<'msg, T> {
        // SAFETY: in-bounds as promised
        unsafe { T::repeated_get_unchecked(self, index) }
    }

    /// Iterates over the values in the repeated field.
    pub fn iter(self) -> RepeatedIter<'msg, T> {
        self.into_iter()
    }
}

#[doc(hidden)]
impl<'msg, T> RepeatedMut<'msg, T> {
    /// # Safety
    /// - `inner` must be valid to read and write from for `'msg`
    /// - There must be no aliasing references or mutations on the same
    ///   underlying object.
    #[doc(hidden)]
    #[inline]
    pub unsafe fn from_inner(_private: Private, inner: InnerRepeatedMut<'msg>) -> Self {
        Self { inner, _phantom: PhantomData }
    }

    #[doc(hidden)]
    #[inline]
    pub fn as_raw(&mut self, _private: Private) -> RawRepeatedField {
        self.inner.raw
    }
}

impl<'msg, T> RepeatedMut<'msg, T>
where
    T: ProxiedInRepeated + 'msg,
{
    /// Gets the length of the repeated field.
    #[inline]
    pub fn len(&self) -> usize {
        self.as_view().len()
    }

    /// Returns true if the repeated field has no values.
    #[inline]
    pub fn is_empty(&self) -> bool {
        self.len() == 0
    }

    /// Gets the value at `index`.
    ///
    /// Returns `None` if `index > len`.
    #[inline]
    pub fn get(&self, index: usize) -> Option<View<T>> {
        self.as_view().get(index)
    }

    /// Gets the value at `index` without bounds-checking.
    ///
    /// # Safety
    /// Undefined behavior if `index >= len`
    #[inline]
    pub unsafe fn get_unchecked(&self, index: usize) -> View<T> {
        // SAFETY: in-bounds as promised
        unsafe { self.as_view().get_unchecked(index) }
    }

    /// Appends `val` to the end of the repeated field.
    #[inline]
    pub fn push(&mut self, val: impl IntoProxied<T>) {
        T::repeated_push(self.as_mut(), val);
    }

    /// Sets the value at `index` to the value `val`.
    ///
    /// # Panics
    /// Panics if `index >= len`
    #[inline]
    pub fn set(&mut self, index: usize, val: impl IntoProxied<T>) {
        let len = self.len();
        if index >= len {
            panic!("index {index} >= repeated len {len}");
        }
        unsafe { self.set_unchecked(index, val) }
    }

    /// Sets the value at `index` to the value `val`.
    ///
    /// # Safety
    /// Undefined behavior if `index >= len`
    #[inline]
    pub unsafe fn set_unchecked(&mut self, index: usize, val: impl IntoProxied<T>) {
        unsafe { T::repeated_set_unchecked(self.as_mut(), index, val) }
    }

    /// Iterates over the values in the repeated field.
    pub fn iter(&self) -> RepeatedIter<T> {
        self.as_view().into_iter()
    }

    /// Copies from the `src` repeated field into this one.
    ///
    /// Also provided by [`MutProxy::set`].
    pub fn copy_from(&mut self, src: RepeatedView<'_, T>) {
        T::repeated_copy_from(src, self.as_mut())
    }

    /// Clears the repeated field.
    pub fn clear(&mut self) {
        T::repeated_clear(self.as_mut())
    }
}

impl<T> Repeated<T>
where
    T: ProxiedInRepeated,
{
    pub fn as_view(&self) -> View<Repeated<T>> {
        RepeatedView { raw: self.inner.raw(), _phantom: PhantomData }
    }

    #[doc(hidden)]
    pub fn inner(&self, _private: Private) -> &InnerRepeated {
        &self.inner
    }
}

impl<'msg, T> IntoProxied<Repeated<T>> for RepeatedView<'msg, T>
where
    T: 'msg + ProxiedInRepeated,
{
    fn into_proxied(self, _private: Private) -> Repeated<T> {
        let mut repeated: Repeated<T> = Repeated::new();
        T::repeated_copy_from(self, repeated.as_mut());
        repeated
    }
}

impl<'msg, T> IntoProxied<Repeated<T>> for RepeatedMut<'msg, T>
where
    T: 'msg + ProxiedInRepeated,
{
    fn into_proxied(self, _private: Private) -> Repeated<T> {
        IntoProxied::into_proxied(self.as_view(), _private)
    }
}

impl<'msg, T, I, U> IntoProxied<Repeated<T>> for I
where
    I: Iterator<Item = U>,
    T: 'msg + ProxiedInRepeated,
    U: IntoProxied<T>,
{
    fn into_proxied(self, _private: Private) -> Repeated<T> {
        let mut repeated: Repeated<T> = Repeated::new();
        repeated.as_mut().extend(self);
        repeated
    }
}

/// Types that can appear in a `Repeated<T>`.
///
/// This trait is implemented by generated code to communicate how the proxied
/// type can be manipulated for a repeated field.
///
/// Scalars and messages implement `ProxiedInRepeated`.
///
/// # Safety
/// - It must be sound to call `*_unchecked*(x)` with an `index` less than
///   `repeated_len(x)`.
pub unsafe trait ProxiedInRepeated: Proxied {
    /// Constructs a new owned `Repeated` field.
    #[doc(hidden)]
    fn repeated_new(_private: Private) -> Repeated<Self>;

    /// Frees the repeated field in-place, for use in `Drop`.
    ///
    /// # Safety
    /// - After `repeated_free`, no other methods on the input are safe to call.
    #[doc(hidden)]
    unsafe fn repeated_free(_private: Private, _repeated: &mut Repeated<Self>);

    /// Gets the length of the repeated field.
    fn repeated_len(repeated: View<Repeated<Self>>) -> usize;

    /// Appends a new element to the end of the repeated field.
    fn repeated_push(repeated: Mut<Repeated<Self>>, val: impl IntoProxied<Self>);

    /// Clears the repeated field of elements.
    fn repeated_clear(repeated: Mut<Repeated<Self>>);

    /// # Safety
    /// `index` must be less than `Self::repeated_len(repeated)`
    unsafe fn repeated_get_unchecked(repeated: View<Repeated<Self>>, index: usize) -> View<Self>;

    /// # Safety
    /// `index` must be less than `Self::repeated_len(repeated)`
    unsafe fn repeated_set_unchecked(
        repeated: Mut<Repeated<Self>>,
        index: usize,
        val: impl IntoProxied<Self>,
    );

    /// Copies the values in the `src` repeated field into `dest`.
    fn repeated_copy_from(src: View<Repeated<Self>>, dest: Mut<Repeated<Self>>);

    /// Ensures that the repeated field has enough space allocated to insert at
    /// least `additional` values without an allocation.
    fn repeated_reserve(repeated: Mut<Repeated<Self>>, additional: usize);
}

/// An iterator over the values inside of a [`View<Repeated<T>>`](RepeatedView).
pub struct RepeatedIter<'msg, T> {
    view: RepeatedView<'msg, T>,
    current_index: usize,
}

impl<'msg, T> Debug for RepeatedIter<'msg, T> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("RepeatedIter")
            .field("view", &self.view)
            .field("current_index", &self.current_index)
            .finish()
    }
}

/// A `repeated` field of `T`, used as the owned target for `Proxied`.
///
/// Users will generally write [`View<Repeated<T>>`](RepeatedView) or
/// [`Mut<Repeated<T>>`](RepeatedMut) to access the repeated elements
pub struct Repeated<T: ProxiedInRepeated> {
    pub(crate) inner: InnerRepeated,
    _phantom: PhantomData<T>,
}

// SAFETY: `Repeated` is Sync because it does not implement interior mutability.
unsafe impl<T: ProxiedInRepeated> Sync for Repeated<T> {}

// SAFETY: `Repeated` is Send because it's not bound to a specific thread e.g.
// it does not use thread-local data or similar.
unsafe impl<T: ProxiedInRepeated> Send for Repeated<T> {}

impl<T: ProxiedInRepeated> Repeated<T> {
    pub fn new() -> Self {
        T::repeated_new(Private)
    }
    #[doc(hidden)]
    pub fn from_inner(_private: Private, inner: InnerRepeated) -> Self {
        Self { inner, _phantom: PhantomData }
    }

    pub(crate) fn as_mut(&mut self) -> RepeatedMut<'_, T> {
        RepeatedMut { inner: self.inner.as_mut(), _phantom: PhantomData }
    }
}

impl<T: ProxiedInRepeated> Default for Repeated<T> {
    fn default() -> Self {
        Repeated::new()
    }
}

impl<T: ProxiedInRepeated> Drop for Repeated<T> {
    fn drop(&mut self) {
        // SAFETY: only called once
        unsafe { T::repeated_free(Private, self) }
    }
}

impl<T> Proxied for Repeated<T>
where
    T: ProxiedInRepeated,
{
    type View<'msg>
        = RepeatedView<'msg, T>
    where
        Repeated<T>: 'msg;
}

impl<T> SealedInternal for Repeated<T> where T: ProxiedInRepeated {}

impl<T> AsView for Repeated<T>
where
    T: ProxiedInRepeated,
{
    type Proxied = Self;

    fn as_view(&self) -> RepeatedView<'_, T> {
        self.as_view()
    }
}

impl<T> MutProxied for Repeated<T>
where
    T: ProxiedInRepeated,
{
    type Mut<'msg>
        = RepeatedMut<'msg, T>
    where
        Repeated<T>: 'msg;
}

impl<T> AsMut for Repeated<T>
where
    T: ProxiedInRepeated,
{
    type MutProxied = Self;

    fn as_mut(&mut self) -> RepeatedMut<'_, T> {
        self.as_mut()
    }
}

impl<'msg, T> SealedInternal for RepeatedView<'msg, T> where T: ProxiedInRepeated + 'msg {}

impl<'msg, T> Proxy<'msg> for RepeatedView<'msg, T> where T: ProxiedInRepeated + 'msg {}

impl<'msg, T> AsView for RepeatedView<'msg, T>
where
    T: ProxiedInRepeated + 'msg,
{
    type Proxied = Repeated<T>;

    #[inline]
    fn as_view(&self) -> View<'msg, Self::Proxied> {
        *self
    }
}

impl<'msg, T> IntoView<'msg> for RepeatedView<'msg, T>
where
    T: ProxiedInRepeated + 'msg,
{
    #[inline]
    fn into_view<'shorter>(self) -> View<'shorter, Self::Proxied>
    where
        'msg: 'shorter,
    {
        RepeatedView { raw: self.raw, _phantom: PhantomData }
    }
}

impl<'msg, T> ViewProxy<'msg> for RepeatedView<'msg, T> where T: ProxiedInRepeated + 'msg {}

impl<'msg, T> SealedInternal for RepeatedMut<'msg, T> where T: ProxiedInRepeated + 'msg {}

impl<'msg, T> Proxy<'msg> for RepeatedMut<'msg, T> where T: ProxiedInRepeated + 'msg {}

impl<'msg, T> AsView for RepeatedMut<'msg, T>
where
    T: ProxiedInRepeated + 'msg,
{
    type Proxied = Repeated<T>;

    #[inline]
    fn as_view(&self) -> RepeatedView<'_, T> {
        RepeatedView { raw: self.inner.raw, _phantom: PhantomData }
    }
}

impl<'msg, T> IntoView<'msg> for RepeatedMut<'msg, T>
where
    T: ProxiedInRepeated + 'msg,
{
    #[inline]
    fn into_view<'shorter>(self) -> RepeatedView<'shorter, T>
    where
        'msg: 'shorter,
    {
        RepeatedView { raw: self.inner.raw, _phantom: PhantomData }
    }
}

impl<'msg, T> AsMut for RepeatedMut<'msg, T>
where
    T: ProxiedInRepeated + 'msg,
{
    type MutProxied = Repeated<T>;

    #[inline]
    fn as_mut(&mut self) -> RepeatedMut<'_, T> {
        RepeatedMut { inner: self.inner, _phantom: PhantomData }
    }
}

impl<'msg, T> IntoMut<'msg> for RepeatedMut<'msg, T>
where
    T: ProxiedInRepeated + 'msg,
{
    #[inline]
    fn into_mut<'shorter>(self) -> RepeatedMut<'shorter, T>
    where
        'msg: 'shorter,
    {
        RepeatedMut { inner: self.inner, _phantom: PhantomData }
    }
}

impl<'msg, T> MutProxy<'msg> for RepeatedMut<'msg, T> where T: ProxiedInRepeated + 'msg {}

impl<'msg, T> iter::Iterator for RepeatedIter<'msg, T>
where
    T: ProxiedInRepeated + 'msg,
{
    type Item = View<'msg, T>;

    #[inline]
    fn next(&mut self) -> Option<Self::Item> {
        let val = self.view.get(self.current_index);
        if val.is_some() {
            self.current_index += 1;
        }
        val
    }

    fn size_hint(&self) -> (usize, Option<usize>) {
        let len = self.len();
        (len, Some(len))
    }
}

impl<'msg, T: ProxiedInRepeated> ExactSizeIterator for RepeatedIter<'msg, T> {
    fn len(&self) -> usize {
        self.view.len() - self.current_index
    }
}

// TODO: impl DoubleEndedIterator
impl<'msg, T: ProxiedInRepeated> FusedIterator for RepeatedIter<'msg, T> {}

impl<'msg, T> iter::IntoIterator for RepeatedView<'msg, T>
where
    T: ProxiedInRepeated + 'msg,
{
    type Item = View<'msg, T>;
    type IntoIter = RepeatedIter<'msg, T>;

    fn into_iter(self) -> Self::IntoIter {
        RepeatedIter { view: self, current_index: 0 }
    }
}

impl<'msg, T> iter::IntoIterator for &'_ RepeatedView<'msg, T>
where
    T: ProxiedInRepeated + 'msg,
{
    type Item = View<'msg, T>;
    type IntoIter = RepeatedIter<'msg, T>;

    fn into_iter(self) -> Self::IntoIter {
        RepeatedIter { view: *self, current_index: 0 }
    }
}

impl<'borrow, T> iter::IntoIterator for &'borrow RepeatedMut<'_, T>
where
    T: ProxiedInRepeated + 'borrow,
{
    type Item = View<'borrow, T>;
    type IntoIter = RepeatedIter<'borrow, T>;

    fn into_iter(self) -> Self::IntoIter {
        RepeatedIter { view: self.as_view(), current_index: 0 }
    }
}

impl<'msg, 'view, T, ViewT> Extend<ViewT> for RepeatedMut<'msg, T>
where
    T: ProxiedInRepeated + 'view,
    ViewT: IntoProxied<T>,
{
    fn extend<I: IntoIterator<Item = ViewT>>(&mut self, iter: I) {
        let iter = iter.into_iter();
        T::repeated_reserve(self.as_mut(), iter.size_hint().0);
        for item in iter {
            self.push(item);
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use googletest::prelude::*;

    #[gtest]
    fn test_primitive_repeated() {
        macro_rules! primitive_repeated_tests {
            ($($t:ty => [$($vals:expr),* $(,)?]),* $(,)?) => {
                $({
                // Constructs a new, owned, `Repeated`, only used for tests.
                let mut r = Repeated::<$t>::new();
                let mut r = r.as_mut();
                assert_that!(r.len(), eq(0));
                assert!(r.iter().next().is_none(), "starts with empty iter");
                assert!(r.iter().next().is_none(), "starts with empty mut iter");
                assert!(r.is_empty(), "starts is_empty");

                let mut expected_len = 0usize;
                $(
                    let val: View<$t> = $vals;
                    r.push(val);
                    assert_that!(r.get(expected_len), eq(Some(val)));
                    expected_len += 1;
                    assert_that!(r.len(), eq(expected_len));

                )*
                assert_that!(r, elements_are![$(eq($vals)),*]);
                r.set(0, <$t as Default>::default());
                assert_that!(r.get(0).expect("elem 0"), eq(<$t as Default>::default()));

                r.clear();
                assert!(r.is_empty(), "is_empty after clear");
                assert!(r.iter().next().is_none(), "iter empty after clear");
                assert!(r.into_iter().next().is_none(), "mut iter empty after clear");
                })*
            }
        }
        primitive_repeated_tests!(
            u32 => [1,2,3],
            i32 => [1,2],
            f64 => [10.0, 0.1234f64],
            bool => [false, true, true, false],
        );
    }

    #[gtest]
    fn test_repeated_extend() {
        let mut r = Repeated::<i32>::new();

        r.as_mut().extend([0; 0]);
        assert_that!(r.as_mut().len(), eq(0));

        r.as_mut().extend([0, 1]);
        assert_that!(r.as_mut(), elements_are![eq(0), eq(1)]);
        let mut x = Repeated::<i32>::new();
        x.as_mut().extend([2, 3]);

        r.as_mut().extend(&x.as_mut());

        assert_that!(r.as_mut(), elements_are![eq(0), eq(1), eq(2), eq(3)]);
    }

    #[gtest]
    fn test_repeated_iter_into_proxied() {
        let r: Repeated<i32> = [0, 1, 2, 3].into_iter().into_proxied(Private);
        assert_that!(r.as_view(), elements_are![eq(0), eq(1), eq(2), eq(3)]);
    }
}
