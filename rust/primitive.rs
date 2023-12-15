// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use std::fmt::Debug;

use crate::__internal::Private;
use crate::__runtime::InnerPrimitiveMut;
use crate::vtable::{PrimitiveWithRawVTable, ProxiedWithRawVTable, RawVTableOptionalMutatorData};
use crate::{Mut, MutProxy, Proxied, ProxiedWithPresence, SettableValue, View, ViewProxy};

/// A mutator for a primitive (numeric or enum) value of `T`.
///
/// This type is `protobuf::Mut<'msg, T>`.
pub struct PrimitiveMut<'msg, T> {
    inner: InnerPrimitiveMut<'msg, T>,
}

impl<'msg, T> Debug for PrimitiveMut<'msg, T>
where
    T: PrimitiveWithRawVTable,
{
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("PrimitiveMut").field("inner", &self.inner).finish()
    }
}

impl<'msg, T> PrimitiveMut<'msg, T> {
    /// # Safety
    /// `inner` must be valid and non-aliased for `T` for `'msg`
    #[doc(hidden)]
    pub unsafe fn from_inner(_private: Private, inner: InnerPrimitiveMut<'msg, T>) -> Self {
        Self { inner }
    }
}

// SAFETY: all `T` that can perform mutations don't mutate through a shared
// reference.
unsafe impl<'msg, T> Sync for PrimitiveMut<'msg, T> {}

impl<'msg, T> PrimitiveMut<'msg, T>
where
    T: PrimitiveWithRawVTable,
{
    /// Gets the current value of the field.
    pub fn get(&self) -> View<'_, T> {
        T::make_view(Private, self.inner)
    }

    /// Sets a new value for the field.
    pub fn set(&mut self, val: impl SettableValue<T>) {
        val.set_on(Private, self.as_mut())
    }

    #[doc(hidden)]
    pub fn set_primitive(&mut self, _private: Private, value: T) {
        // SAFETY: the raw mutator is valid for `'msg` as enforced by `Mut`
        unsafe { self.inner.set(value) }
    }
}

impl<'msg, T> ViewProxy<'msg> for PrimitiveMut<'msg, T>
where
    T: PrimitiveWithRawVTable,
{
    type Proxied = T;

    fn as_view(&self) -> View<'_, Self::Proxied> {
        self.get()
    }

    fn into_view<'shorter>(self) -> View<'shorter, Self::Proxied> {
        self.get()
    }
}

impl<'msg, T> MutProxy<'msg> for PrimitiveMut<'msg, T>
where
    T: PrimitiveWithRawVTable,
{
    fn as_mut(&mut self) -> Mut<'_, Self::Proxied> {
        PrimitiveMut { inner: self.inner }
    }

    fn into_mut<'shorter>(self) -> Mut<'shorter, Self::Proxied>
    where
        'msg: 'shorter,
    {
        self
    }
}

macro_rules! impl_singular_primitives {
  ($($t:ty),*) => {
      $(
        impl Proxied for $t {
            type View<'msg> = $t;
            type Mut<'msg> = PrimitiveMut<'msg, $t>;
        }

        impl<'msg> ViewProxy<'msg> for $t {
            type Proxied = $t;

            fn as_view(&self) -> View<'_, Self::Proxied> {
                *self
            }

            fn into_view<'shorter>(self) -> View<'shorter, Self::Proxied> {
                self
            }
        }

        impl SettableValue<$t> for $t {
            fn set_on<'msg>(self, private: Private, mut mutator: Mut<'msg, $t>) where $t: 'msg {
                mutator.set_primitive(private, self)
            }

            fn set_on_absent(
                self,
                _private: Private,
                absent_mutator: <$t as ProxiedWithPresence>::PresentMutData<'_>,
            ) -> <$t as ProxiedWithPresence>::AbsentMutData<'_>
            {
                absent_mutator.set(Private, self)
            }
        }

        impl ProxiedWithPresence for $t {
            type PresentMutData<'msg> = RawVTableOptionalMutatorData<'msg, $t>;
            type AbsentMutData<'msg> = RawVTableOptionalMutatorData<'msg, $t>;

            fn clear_present_field(
                present_mutator: Self::PresentMutData<'_>,
            ) -> Self::AbsentMutData<'_> {
                present_mutator.clear(Private)
            }

            fn set_absent_to_default(
                absent_mutator: Self::AbsentMutData<'_>,
            ) -> Self::PresentMutData<'_> {
                absent_mutator.set_absent_to_default(Private)
            }
        }

        impl PrimitiveWithRawVTable for $t {}

        // ProxiedInRepeated is implemented in {cpp,upb}.rs
      )*
  }
}

impl_singular_primitives!(bool, f32, f64, i32, i64, u32, u64);
