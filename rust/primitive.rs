// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use crate::__internal::Private;
use crate::__runtime::InnerPrimitiveMut;
use crate::vtable::{
    PrimitiveOptionalMutVTable, PrimitiveVTable, ProxiedWithRawOptionalVTable,
    ProxiedWithRawVTable, RawVTableOptionalMutatorData,
};
use crate::{Mut, MutProxy, Proxied, ProxiedWithPresence, SettableValue, View, ViewProxy};

#[derive(Debug)]
pub struct PrimitiveMut<'msg, T: ProxiedWithRawVTable> {
    inner: InnerPrimitiveMut<'msg, T>,
}

impl<'msg, T: ProxiedWithRawVTable> PrimitiveMut<'msg, T> {
    #[doc(hidden)]
    pub fn from_inner(_private: Private, inner: InnerPrimitiveMut<'msg, T>) -> Self {
        Self { inner }
    }
}

unsafe impl<'msg, T: ProxiedWithRawVTable> Sync for PrimitiveMut<'msg, T> {}

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

          impl<'msg> PrimitiveMut<'msg, $t> {
              pub fn get(&self) -> View<'_, $t> {
                  self.inner.get()
              }

              pub fn set(&mut self, val: impl SettableValue<$t>) {
                  val.set_on(Private, self.as_mut());
              }
          }

          impl<'msg> ViewProxy<'msg> for PrimitiveMut<'msg, $t> {
              type Proxied = $t;

              fn as_view(&self) -> View<'_, Self::Proxied> {
                  self.get()
              }

              fn into_view<'shorter>(self) -> View<'shorter, Self::Proxied> {
                  self.get()
              }
          }

          impl<'msg> MutProxy<'msg> for PrimitiveMut<'msg, $t> {
              fn as_mut(&mut self) -> Mut<'_, Self::Proxied> {
                  PrimitiveMut { inner: self.inner }
              }

              fn into_mut<'shorter>(self) -> Mut<'shorter, Self::Proxied>
              where 'msg: 'shorter,
              {
                  self
              }
          }

          impl SettableValue<$t> for $t {
              fn set_on<'msg>(self, _private: Private, mutator: Mut<'msg, $t>) where $t: 'msg {
                // SAFETY: the raw mutator is valid for `'msg` as enforced by `Mut`
                unsafe { mutator.inner.set(self) }
              }
          }

          impl ProxiedWithRawVTable for $t {
            type VTable = PrimitiveVTable<$t>;

            fn make_view(
                _private: Private,
                mut_inner: InnerPrimitiveMut<'_, Self>
            ) -> View<'_, Self> {
                mut_inner.get()
            }

            fn make_mut(_private: Private, inner: InnerPrimitiveMut<'_, Self>) -> Mut<'_, Self> {
                PrimitiveMut::from_inner(Private, inner)
            }
          }

          impl ProxiedWithPresence for $t {
            type PresentMutData<'msg> = RawVTableOptionalMutatorData<'msg, $t>;
            type AbsentMutData<'msg> = RawVTableOptionalMutatorData<'msg, $t>;

            fn clear_present_field(
                present_mutator: Self::PresentMutData<'_>,
            ) -> Self::AbsentMutData<'_> {
                present_mutator.clear()
            }

            fn set_absent_to_default(
                absent_mutator: Self::AbsentMutData<'_>,
            ) -> Self::PresentMutData<'_> {
                absent_mutator.set_absent_to_default()
            }
          }

          impl ProxiedWithRawOptionalVTable for $t {
            type OptionalVTable = PrimitiveOptionalMutVTable<$t>;

            fn upcast_vtable(
                _private: Private,
                optional_vtable: &'static Self::OptionalVTable,
            ) -> &'static Self::VTable {
                &optional_vtable.base
            }
          }
      )*
  }
}

impl_singular_primitives!(bool, f32, f64, i32, i64, u32, u64);
