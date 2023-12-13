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
    __internal::{Private, RawRepeatedField},
    __runtime::{RepeatedField, RepeatedFieldInner},
    vtable::ProxiedWithRawVTable,
};

#[derive(Clone, Copy)]
pub struct RepeatedFieldRef<'msg> {
    pub repeated_field: RawRepeatedField,
    pub _phantom: PhantomData<&'msg mut ()>,
}

unsafe impl<'msg> Send for RepeatedFieldRef<'msg> {}
unsafe impl<'msg> Sync for RepeatedFieldRef<'msg> {}

#[derive(Clone, Copy)]
#[repr(transparent)]
pub struct RepeatedView<'msg, T: ?Sized> {
    inner: RepeatedField<'msg, T>,
}

unsafe impl<'msg, T: ProxiedWithRawVTable> Sync for RepeatedView<'msg, T> {}
unsafe impl<'msg, T: ProxiedWithRawVTable> Send for RepeatedView<'msg, T> {}

impl<'msg, T: ?Sized> RepeatedView<'msg, T> {
    pub fn from_inner(_private: Private, inner: RepeatedFieldInner<'msg>) -> Self {
        Self { inner: RepeatedField::<'msg>::from_inner(_private, inner) }
    }
}

pub struct RepeatedFieldIter<'msg, T> {
    inner: RepeatedField<'msg, T>,
    current_index: usize,
}

impl<'msg, T> std::fmt::Debug for RepeatedView<'msg, T> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_tuple("RepeatedView").finish()
    }
}

#[repr(transparent)]
#[derive(Debug)]
pub struct RepeatedMut<'msg, T: ?Sized> {
    inner: RepeatedField<'msg, T>,
}

unsafe impl<'msg, T: ProxiedWithRawVTable> Sync for RepeatedMut<'msg, T> {}

impl<'msg, T: ?Sized> RepeatedMut<'msg, T> {
    pub fn from_inner(_private: Private, inner: RepeatedFieldInner<'msg>) -> Self {
        Self { inner: RepeatedField::from_inner(_private, inner) }
    }
    pub fn as_mut(&self) -> RepeatedMut<'_, T> {
        Self { inner: self.inner }
    }
}

impl<'msg, T> std::ops::Deref for RepeatedMut<'msg, T> {
    type Target = RepeatedView<'msg, T>;
    fn deref(&self) -> &Self::Target {
        // SAFETY:
        //   - `Repeated{View,Mut}<'msg, T>` are both `#[repr(transparent)]` over
        //     `RepeatedField<'msg, T>`.
        //   - `RepeatedField` is a type alias for `NonNull`.
        unsafe { &*(self as *const Self as *const RepeatedView<'msg, T>) }
    }
}

pub struct Repeated<T>(PhantomData<T>);

macro_rules! impl_repeated_primitives {
    ($($t:ty),*) => {
        $(
            impl Proxied for Repeated<$t> {
                type View<'msg> = RepeatedView<'msg, $t>;
                type Mut<'msg> = RepeatedMut<'msg, $t>;
            }

            impl<'msg> ViewProxy<'msg> for RepeatedView<'msg, $t> {
                type Proxied = Repeated<$t>;

                fn as_view(&self) -> View<'_, Self::Proxied> {
                    *self
                }

                fn into_view<'shorter>(self) -> View<'shorter, Self::Proxied>
                where 'msg: 'shorter,
                {
                    RepeatedView { inner: self.inner }
                }
            }

            impl<'msg> ViewProxy<'msg> for RepeatedMut<'msg, $t> {
                type Proxied = Repeated<$t>;

                fn as_view(&self) -> View<'_, Self::Proxied> {
                    **self
                }

                fn into_view<'shorter>(self) -> View<'shorter, Self::Proxied>
                where 'msg: 'shorter,
                {
                    *self.into_mut::<'shorter>()
                }
            }

            impl<'msg> MutProxy<'msg> for RepeatedMut<'msg, $t> {
                fn as_mut(&mut self) -> Mut<'_, Self::Proxied> {
                    RepeatedMut { inner: self.inner }
                }

                fn into_mut<'shorter>(self) -> Mut<'shorter, Self::Proxied>
                where 'msg: 'shorter,
                {
                    RepeatedMut { inner: self.inner }
                }
            }

            impl <'msg> SettableValue<Repeated<$t>> for RepeatedView<'msg, $t> {
                fn set_on<'b> (self, _private: Private, mut mutator: Mut<'b, Repeated<$t>>)
                where
                    Repeated<$t>: 'b {
                    mutator.copy_from(self);
                }
            }

            impl<'msg> RepeatedView<'msg, $t> {
                pub fn len(&self) -> usize {
                    self.inner.len()
                }
                pub fn is_empty(&self) -> bool {
                    self.len() == 0
                }
                pub fn get(&self, index: usize) -> Option<$t> {
                    self.inner.get(index)
                }
                pub fn iter(&self) -> RepeatedFieldIter<'_, $t> {
                    (*self).into_iter()
                }
            }

            impl<'msg> RepeatedMut<'msg, $t> {
                pub fn push(&mut self, val: $t) {
                    self.inner.push(val)
                }
                pub fn set(&mut self, index: usize, val: $t) {
                    self.inner.set(index, val)
                }
                pub fn iter(&self) -> RepeatedFieldIter<'_, $t> {
                    self.as_view().into_iter()
                }
                pub fn copy_from(&mut self, src: RepeatedView<'_, $t>) {
                    self.inner.copy_from(&src.inner);
                }
            }

            impl<'msg> std::iter::Iterator for RepeatedFieldIter<'msg, $t> {
                type Item = $t;
                fn next(&mut self) -> Option<Self::Item> {
                    let val = self.inner.get(self.current_index);
                    if val.is_some() {
                        self.current_index += 1;
                    }
                    val
                }
            }

            impl<'msg> std::iter::IntoIterator for RepeatedView<'msg, $t> {
                type Item = $t;
                type IntoIter = RepeatedFieldIter<'msg, $t>;
                fn into_iter(self) -> Self::IntoIter {
                    RepeatedFieldIter { inner: self.inner, current_index: 0 }
                }
            }
        )*
    }
}

impl_repeated_primitives!(i32, u32, bool, f32, f64, i64, u64);
