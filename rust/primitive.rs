// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use crate::__internal::Private;
use crate::__runtime::InnerPrimitiveMut;
use crate::repeated::RepeatedMut;
use crate::vtable::{
    PrimitiveOptionalMutVTable, PrimitiveVTable, ProxiedWithRawOptionalVTable,
    ProxiedWithRawVTable, RawVTableOptionalMutatorData,
};
use crate::{Mut, MutProxy, Proxied, ProxiedWithPresence, SettableValue, View, ViewProxy};

#[derive(Debug)]
pub struct SingularPrimitiveMut<'a, T: ProxiedWithRawVTable> {
    inner: InnerPrimitiveMut<'a, T>,
}

#[derive(Debug)]
pub enum PrimitiveMut<'a, T: ProxiedWithRawVTable> {
    Singular(SingularPrimitiveMut<'a, T>),
    Repeated(RepeatedMut<'a, T>, usize),
}

impl<'a, T: ProxiedWithRawVTable> PrimitiveMut<'a, T> {
    #[doc(hidden)]
    pub fn from_singular(_private: Private, inner: InnerPrimitiveMut<'a, T>) -> Self {
        PrimitiveMut::Singular(SingularPrimitiveMut::from_inner(_private, inner))
    }
}

impl<'a, T: ProxiedWithRawVTable> SingularPrimitiveMut<'a, T> {
    #[doc(hidden)]
    pub fn from_inner(_private: Private, inner: InnerPrimitiveMut<'a, T>) -> Self {
        Self { inner }
    }
}

unsafe impl<'a, T: ProxiedWithRawVTable> Sync for SingularPrimitiveMut<'a, T> {}

macro_rules! impl_singular_primitives {
  ($($t:ty),*) => {
      $(
          impl Proxied for $t {
              type View<'a> = $t;
              type Mut<'a> = PrimitiveMut<'a, $t>;
          }

          impl<'a> ViewProxy<'a> for $t {
              type Proxied = $t;

              fn as_view(&self) -> View<'_, Self::Proxied> {
                  *self
              }

              fn into_view<'shorter>(self) -> View<'shorter, Self::Proxied> {
                  self
              }
          }

          impl<'a> PrimitiveMut<'a, $t> {
              pub fn get(&self) -> View<'_, $t> {
                  match self {
                      PrimitiveMut::Singular(s) => {
                          s.get()
                      }
                      PrimitiveMut::Repeated(r, i) => {
                          r.get().get(*i).unwrap()
                      }
                  }
              }

              pub fn set(&mut self, val: impl SettableValue<$t>) {
                  val.set_on(Private, self.as_mut());
              }

              pub fn clear(&mut self) {
                  // The default value for a boolean field is false and 0 for numerical types. It
                  // matches the Rust default values for corresponding types. Let's use this fact.
                  SettableValue::<$t>::set_on(<$t>::default(), Private, MutProxy::as_mut(self));
              }
          }

          impl<'a> ViewProxy<'a> for PrimitiveMut<'a, $t> {
              type Proxied = $t;

              fn as_view(&self) -> View<'_, Self::Proxied> {
                  self.get()
              }

              fn into_view<'shorter>(self) -> View<'shorter, Self::Proxied> {
                  self.get()
              }
          }

          impl<'a> MutProxy<'a> for PrimitiveMut<'a, $t> {
              fn as_mut(&mut self) -> Mut<'_, Self::Proxied> {
                  match self {
                      PrimitiveMut::Singular(s) => {
                          PrimitiveMut::Singular(s.as_mut())
                      }
                      PrimitiveMut::Repeated(r, i) => {
                          PrimitiveMut::Repeated(r.as_mut(), *i)
                      }
                  }
              }

              fn into_mut<'shorter>(self) -> Mut<'shorter, Self::Proxied>
              where 'a: 'shorter,
              {
                  self
              }
          }

          impl SettableValue<$t> for $t {
              fn set_on<'a>(self, _private: Private, mutator: Mut<'a, $t>) where $t: 'a {
                match mutator {
                  PrimitiveMut::Singular(s) => {
                      unsafe { (s.inner).set(self) };
                  }
                  PrimitiveMut::Repeated(mut r, i) => {
                      r.set(i, self);
                  }
                }
              }
          }

          impl<'a> SingularPrimitiveMut<'a, $t> {
              pub fn get(&self) -> $t {
                  self.inner.get()
              }
              pub fn as_mut(&mut self) -> SingularPrimitiveMut<'_, $t> {
                  SingularPrimitiveMut::from_inner(Private, self.inner)
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
                PrimitiveMut::Singular(SingularPrimitiveMut::from_inner(Private, inner))
            }
          }

          impl ProxiedWithPresence for $t {
            type PresentMutData<'a> = RawVTableOptionalMutatorData<'a, $t>;
            type AbsentMutData<'a> = RawVTableOptionalMutatorData<'a, $t>;

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
