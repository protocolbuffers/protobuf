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
    primitive::PrimitiveMut,
    vtable::ProxiedWithRawVTable,
};

#[derive(Clone, Copy)]
pub struct RepeatedFieldRef<'a> {
    pub repeated_field: RawRepeatedField,
    pub _phantom: PhantomData<&'a mut ()>,
}

unsafe impl<'a> Send for RepeatedFieldRef<'a> {}
unsafe impl<'a> Sync for RepeatedFieldRef<'a> {}

#[derive(Clone, Copy)]
#[repr(transparent)]
pub struct RepeatedView<'a, T: ?Sized> {
    inner: RepeatedField<'a, T>,
}

unsafe impl<'a, T: ProxiedWithRawVTable> Sync for RepeatedView<'a, T> {}
unsafe impl<'a, T: ProxiedWithRawVTable> Send for RepeatedView<'a, T> {}

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
#[derive(Debug)]
pub struct RepeatedMut<'a, T: ?Sized> {
    inner: RepeatedField<'a, T>,
}

unsafe impl<'a, T: ProxiedWithRawVTable> Sync for RepeatedMut<'a, T> {}

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

macro_rules! impl_repeated_primitives {
    ($($t:ty),*) => {
        $(
            impl Proxied for Repeated<$t> {
                type View<'a> = RepeatedView<'a, $t>;
                type Mut<'a> = RepeatedMut<'a, $t>;
            }

            impl<'a> ViewProxy<'a> for RepeatedView<'a, $t> {
                type Proxied = Repeated<$t>;

                fn as_view(&self) -> View<'_, Self::Proxied> {
                    *self
                }

                fn into_view<'shorter>(self) -> View<'shorter, Self::Proxied>
                where 'a: 'shorter,
                {
                    RepeatedView { inner: self.inner }
                }
            }

            impl<'a> ViewProxy<'a> for RepeatedMut<'a, $t> {
                type Proxied = Repeated<$t>;

                fn as_view(&self) -> View<'_, Self::Proxied> {
                    **self
                }

                fn into_view<'shorter>(self) -> View<'shorter, Self::Proxied>
                where 'a: 'shorter,
                {
                    *self.into_mut::<'shorter>()
                }
            }

            impl<'a> MutProxy<'a> for RepeatedMut<'a, $t> {
                fn as_mut(&mut self) -> Mut<'_, Self::Proxied> {
                    RepeatedMut { inner: self.inner }
                }

                fn into_mut<'shorter>(self) -> Mut<'shorter, Self::Proxied>
                where 'a: 'shorter,
                {
                    RepeatedMut { inner: self.inner }
                }
            }

            impl <'a> SettableValue<Repeated<$t>> for RepeatedView<'a, $t> {
                fn set_on<'b> (self, _private: Private, mut mutator: Mut<'b, Repeated<$t>>)
                where
                    Repeated<$t>: 'b {
                    mutator.copy_from(self);
                }
            }

            impl<'a> RepeatedView<'a, $t> {
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

            impl<'a> RepeatedMut<'a, $t> {
                pub fn push(&mut self, val: $t) {
                    self.inner.push(val)
                }
                pub fn set(&mut self, index: usize, val: $t) {
                    self.inner.set(index, val)
                }
                pub fn get_mut(&mut self, index: usize) -> Option<Mut<'_, $t>> {
                    if index >= self.len() {
                        return None;
                    }
                    Some(PrimitiveMut::Repeated(self.as_mut(), index))
                }
                pub fn iter(&self) -> RepeatedFieldIter<'_, $t> {
                    self.as_view().into_iter()
                }
                pub fn iter_mut(&mut self) -> RepeatedFieldIterMut<'_, $t> {
                    self.as_mut().into_iter()
                }
                pub fn copy_from(&mut self, src: RepeatedView<'_, $t>) {
                    self.inner.copy_from(&src.inner);
                }
            }

            impl<'a> std::iter::Iterator for RepeatedFieldIter<'a, $t> {
                type Item = $t;
                fn next(&mut self) -> Option<Self::Item> {
                    let val = self.inner.get(self.current_index);
                    if val.is_some() {
                        self.current_index += 1;
                    }
                    val
                }
            }

            impl<'a> std::iter::IntoIterator for RepeatedView<'a, $t> {
                type Item = $t;
                type IntoIter = RepeatedFieldIter<'a, $t>;
                fn into_iter(self) -> Self::IntoIter {
                    RepeatedFieldIter { inner: self.inner, current_index: 0 }
                }
            }

            impl <'a> std::iter::Iterator for RepeatedFieldIterMut<'a, $t> {
                type Item = Mut<'a, $t>;
                fn next(&mut self) -> Option<Self::Item> {
                    if self.current_index >= self.inner.len() {
                        return None;
                    }
                    let elem = PrimitiveMut::Repeated(
                        // While this appears to allow mutable aliasing
                        // (multiple `Self::Item`s can co-exist), each `Item`
                        // only references a specific unique index.
                        RepeatedMut{ inner: self.inner.inner },
                        self.current_index,
                    );
                    self.current_index += 1;
                    Some(elem)
                }
            }

            impl<'a> std::iter::IntoIterator for RepeatedMut<'a, $t> {
                type Item = Mut<'a, $t>;
                type IntoIter = RepeatedFieldIterMut<'a, $t>;
                fn into_iter(self) -> Self::IntoIter {
                    RepeatedFieldIterMut { inner: self, current_index: 0 }
                }
            }
        )*
    }
}

impl_repeated_primitives!(i32, u32, bool, f32, f64, i64, u64);
