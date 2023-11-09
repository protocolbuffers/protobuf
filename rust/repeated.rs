// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

/// Repeated scalar fields are implemented around the runtime-specific
/// `RepeatedField` struct. `RepeatedField` stores an opaque pointer to the
/// runtime-specific representation of a repeated scalar (`upb_Array*` on upb,
/// and `RepeatedField<T>*` on cpp).
use std::marker::PhantomData;

use crate::{
    Mut, MutProxy, Proxied, SettableValue, View, ViewProxy,
    __internal::Private,
    __runtime::{RepeatedField, RepeatedFieldInner},
    primitive::PrimitiveMut,
};

#[repr(transparent)]
pub struct RepeatedView<'a, T: ?Sized> {
    inner: RepeatedField<'a, T>,
}

unsafe impl<'a, T> Sync for RepeatedView<'a, T> {}
unsafe impl<'a, T> Send for RepeatedView<'a, T> {}
impl<'msg, T: ?Sized> Copy for RepeatedView<'msg, T> {}
impl<'msg, T: ?Sized> Clone for RepeatedView<'msg, T> {
    fn clone(&self) -> RepeatedView<'msg, T> {
        *self
    }
}

impl<'msg, T: ?Sized> RepeatedView<'msg, T> {
    pub fn from_inner(_private: Private, inner: RepeatedFieldInner<'msg>) -> Self {
        Self { inner: RepeatedField::<'msg>::from_inner(_private, inner) }
    }
}

pub struct RepeatedFieldIter<'a, T> {
    inner: RepeatedField<'a, T>,
    current_index: usize,
}

impl<'a, T> std::fmt::Debug for RepeatedView<'a, T> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_tuple("RepeatedView").finish()
    }
}

#[repr(transparent)]
pub struct RepeatedMut<'a, T: ?Sized> {
    pub(crate) inner: RepeatedField<'a, T>,
}

impl<'a, T> std::fmt::Debug for RepeatedMut<'a, T> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_tuple("RepeatedMut").finish()
    }
}

unsafe impl<'a, T> Sync for RepeatedMut<'a, T> {}

impl<'msg, T: ?Sized> RepeatedMut<'msg, T> {
    pub fn from_inner(_private: Private, inner: RepeatedFieldInner<'msg>) -> Self {
        Self { inner: RepeatedField::from_inner(_private, inner) }
    }
    pub fn as_mut(&self) -> RepeatedMut<'_, T> {
        Self { inner: self.inner }
    }
}

impl<'a, T> std::ops::Deref for RepeatedMut<'a, T> {
    type Target = RepeatedView<'a, T>;
    fn deref(&self) -> &Self::Target {
        // SAFETY:
        //   - `Repeated{View,Mut}<'a, T>` are both `#[repr(transparent)]` over
        //     `RepeatedField<'a, T>`.
        //   - `RepeatedField` is a type alias for `NonNull`.
        unsafe { &*(self as *const Self as *const RepeatedView<'a, T>) }
    }
}

pub struct RepeatedFieldIterMut<'a, T> {
    inner: RepeatedMut<'a, T>,
    current_index: usize,
}

pub struct Repeated<T>(PhantomData<T>);

impl<T: Proxied> Proxied for Repeated<T>
where
    for<'msg> RepeatedField<'msg, T>: RepeatedI<'msg, T>,
{
    type View<'a> = RepeatedView<'a, T> where T: 'a;
    type Mut<'a> = RepeatedMut<'a, T> where T: 'a;
}

impl<T: Proxied> SettableValue<Repeated<T>> for RepeatedView<'_, T>
where
    for<'a> RepeatedField<'a, T>: RepeatedI<'a, T>,
{
    fn set_on(self, _private: Private, mut mutator: Mut<'_, Repeated<T>>) {
        mutator.inner.copy_from(self);
    }
}

impl<'a, T: Proxied> ViewProxy<'a> for RepeatedView<'a, T>
where
    for<'msg> RepeatedField<'msg, T>: RepeatedI<'msg, T>,
{
    type Proxied = Repeated<T>;

    fn as_view(&self) -> View<'_, Self::Proxied> {
        *self
    }

    fn into_view<'shorter>(self) -> View<'shorter, Self::Proxied>
    where
        'a: 'shorter,
    {
        RepeatedView { inner: self.inner }
    }
}

impl<'a, T: Proxied> ViewProxy<'a> for RepeatedMut<'a, T>
where
    for<'msg> RepeatedField<'msg, T>: RepeatedI<'msg, T>,
{
    type Proxied = Repeated<T>;

    fn as_view(&self) -> View<'_, Self::Proxied> {
        **self
    }

    fn into_view<'shorter>(self) -> View<'shorter, Self::Proxied>
    where
        'a: 'shorter,
    {
        *self.into_mut::<'shorter>()
    }
}

impl<'a, T: Proxied> MutProxy<'a> for RepeatedMut<'a, T>
where
    for<'msg> RepeatedField<'msg, T>: RepeatedI<'msg, T>,
{
    fn as_mut(&mut self) -> Mut<'_, Self::Proxied> {
        RepeatedMut { inner: self.inner }
    }

    fn into_mut<'shorter>(self) -> Mut<'shorter, Self::Proxied>
    where
        'a: 'shorter,
    {
        RepeatedMut { inner: self.inner }
    }
}

impl<'a, T: Proxied> RepeatedView<'a, T>
where
    for<'msg> RepeatedField<'msg, T>: RepeatedI<'msg, T>,
{
    pub fn len(&self) -> usize {
        self.inner.len()
    }
    pub fn is_empty(&self) -> bool {
        self.len() == 0
    }
    pub fn get(&self, index: usize) -> Option<View<'_, T>> {
        self.inner.get(index)
    }
    pub fn iter(&self) -> RepeatedFieldIter<'_, T> {
        (*self).into_iter()
    }
}

impl<'a, T: Proxied> std::iter::IntoIterator for RepeatedView<'a, T>
where
    for<'msg> RepeatedField<'msg, T>: RepeatedI<'msg, T>,
{
    type Item = View<'a, T>;
    type IntoIter = RepeatedFieldIter<'a, T>;
    fn into_iter(self) -> Self::IntoIter {
        RepeatedFieldIter { inner: self.inner, current_index: 0 }
    }
}

impl<'a, T: Proxied> std::iter::Iterator for RepeatedFieldIter<'a, T>
where
    for<'msg> RepeatedField<'msg, T>: RepeatedI<'msg, T>,
{
    type Item = View<'a, T>;
    fn next(&mut self) -> Option<Self::Item> {
        let val = self.inner.get(self.current_index);
        if val.is_some() {
            self.current_index += 1;
        }
        val
    }
}

impl<'a, T: Proxied> RepeatedMut<'a, T>
where
    for<'msg> RepeatedField<'msg, T>: RepeatedI<'msg, T>,
{
    pub fn push(&mut self, val: impl SettableValue<T>) {
        self.inner.push(val);
    }
    pub fn set(&mut self, index: usize, val: impl SettableValue<T>) {
        self.inner.set(index, val)
    }
    pub fn get_mut(&mut self, index: usize) -> Option<Mut<'_, T>> {
        self.inner.get_mut(index)
    }
    pub fn iter(&self) -> RepeatedFieldIter<'_, T> {
        self.as_view().into_iter()
    }
    pub fn iter_mut(&mut self) -> RepeatedFieldIterMut<'_, T> {
        self.as_mut().into_iter()
    }
    pub fn copy_from(&mut self, src: RepeatedView<'_, T>) {
        self.inner.copy_from(src);
    }
    pub fn push_default(&mut self) -> Mut<'a, T> {
        self.inner.push_default()
    }
}
impl<'a, T: Proxied> std::iter::Iterator for RepeatedFieldIterMut<'a, T>
where
    for<'msg> RepeatedField<'msg, T>: RepeatedI<'msg, T>,
{
    type Item = Mut<'a, T>;
    fn next(&mut self) -> Option<Self::Item> {
        if self.current_index >= self.inner.len() {
            return None;
        }
        // Re-create a `RepeatedMut` to eliminate the `&mut self` lifetime bound
        // on self.inner.  While this appears to allow mutable aliasing
        // (multiple `Self::Item`s can co-exist), each `Item` only references a
        // specific unique index.
        let elem = RepeatedMut { inner: self.inner.inner }.inner.get_mut(self.current_index);

        self.current_index += 1;
        elem
    }
}

impl<'a, T: Proxied> std::iter::IntoIterator for RepeatedMut<'a, T>
where
    for<'msg> RepeatedField<'msg, T>: RepeatedI<'msg, T>,
{
    type Item = Mut<'a, T>;
    type IntoIter = RepeatedFieldIterMut<'a, T>;
    fn into_iter(self) -> Self::IntoIter {
        RepeatedFieldIterMut { inner: self, current_index: 0 }
    }
}

pub trait RepeatedI<'msg, T: Proxied> {
    fn len(&self) -> usize;
    fn get(&self, index: usize) -> Option<View<'msg, T>>;
    fn get_mut(&self, index: usize) -> Option<Mut<'msg, T>>;
    fn push(&mut self, v: impl SettableValue<T>) -> Mut<'msg, T>;
    fn push_default(&mut self) -> Mut<'msg, T>;
    fn set(&mut self, index: usize, v: impl SettableValue<T>);
    fn copy_from(&mut self, src: RepeatedView<'_, T>);
}

pub trait RepeatedMessageOps: Proxied {
    fn len(f: RepeatedField<'_, Self>) -> usize;
    /// # Safety
    /// index must be < len
    unsafe fn get(f: RepeatedField<'_, Self>, index: usize) -> View<'_, Self>;
    /// # Safety
    /// index must be < len
    unsafe fn get_mut(f: RepeatedField<'_, Self>, index: usize) -> Mut<'_, Self>;
    fn push_default(f: RepeatedField<'_, Self>) -> Mut<'_, Self>;
    fn clear(f: RepeatedField<'_, Self>);
}

impl<'msg, T: RepeatedMessageOps> RepeatedI<'msg, T> for RepeatedField<'msg, T> {
    fn len(&self) -> usize {
        T::len(*self)
    }
    fn get(&self, index: usize) -> Option<View<'msg, T>> {
        if index >= self.len() {
            return None;
        }
        Some(unsafe { T::get(*self, index) })
    }
    fn get_mut(&self, index: usize) -> Option<Mut<'msg, T>> {
        if index >= self.len() {
            return None;
        }
        Some(unsafe { T::get_mut(*self, index) })
    }
    fn set(&mut self, index: usize, v: impl SettableValue<T>) {
        if let Some(mutator) = self.get_mut(index) {
            v.set_on(Private, mutator);
        }
    }
    fn push_default(&mut self) -> Mut<'msg, T> {
        T::push_default(*self)
    }
    fn push(&mut self, v: impl SettableValue<T>) -> Mut<'msg, T> {
        let mutator = self.push_default();
        v.set_on(Private, mutator);
        unsafe { T::get_mut(*self, self.len() - 1) }
    }
    fn copy_from(&mut self, src: RepeatedView<'_, T>) {
        T::clear(*self);
        for elem in src {
            self.push(elem);
        }
    }
}

macro_rules! impl_repeated_i_primitives {
    ($($t:ty),*) => {
        $(
            impl <'msg> RepeatedI<'msg, $t> for RepeatedField<'msg, $t> {
                fn len(&self) -> usize {
                    RepeatedField::len(self)
                }
                fn get(&self, index: usize) -> Option<View<'msg, $t>> {
                    RepeatedField::<'_, $t>::get(self, index)
                }
                fn get_mut(&self, index: usize) -> Option<Mut<'msg, $t>> {
                    if index >= self.len() {
                        return None;
                    }
                    Some(PrimitiveMut::Repeated(RepeatedMut{ inner: *self}, index))
                }
                fn set(&mut self, index: usize, v: impl SettableValue<$t>) {
                    let mutator = PrimitiveMut::Repeated(RepeatedMut{inner: *self}, index);
                    v.set_on(Private, mutator);
                }
                fn push_default(&mut self) -> Mut<'msg, $t> {
                    RepeatedField::<'_, $t>::push(self, Default::default());
                    PrimitiveMut::Repeated(RepeatedMut{ inner: *self}, self.len() - 1)
                }
                fn push(&mut self, v: impl SettableValue<$t>) -> Mut<'msg, $t> {
                    v.set_on(Private, self.push_default());
                    PrimitiveMut::Repeated(RepeatedMut{ inner: *self}, self.len() - 1)
                }
                fn copy_from(&mut self, src: RepeatedView<'_, $t>) {
                    self.copy_from(&src.inner);
                }
            }
        )*
    }
}

impl_repeated_i_primitives!(i32, u32, bool, f32, f64, i64, u64);
