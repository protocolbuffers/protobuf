// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google LLC. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

use crate::__internal::Private;
use crate::__runtime::InnerPrimitiveMut;
use crate::vtable::{PrimitiveVTable, ProxiedWithRawVTable};
use crate::{Mut, MutProxy, Proxied, SettableValue, View, ViewProxy};

#[derive(Debug)]
pub struct PrimitiveMut<'a, T: ProxiedWithRawVTable> {
    inner: InnerPrimitiveMut<'a, T>,
}

impl<'a, T: ProxiedWithRawVTable> PrimitiveMut<'a, T> {
    #[doc(hidden)]
    pub fn from_inner(_private: Private, inner: InnerPrimitiveMut<'a, T>) -> Self {
        Self { inner }
    }
}

unsafe impl<'a, T: ProxiedWithRawVTable> Sync for PrimitiveMut<'a, T> {}

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
                  PrimitiveMut::from_inner(Private, self.inner)
              }

              fn into_mut<'shorter>(self) -> Mut<'shorter, Self::Proxied>
              where 'a: 'shorter,
              {
                  self
              }
          }

          impl SettableValue<$t> for $t {
              fn set_on(self, _private: Private, mutator: Mut<'_, $t>) {
                unsafe { (mutator.inner).set(self) };
              }
          }

          impl<'a> PrimitiveMut<'a, $t> {
              pub fn set(&mut self, val: impl SettableValue<$t>) {
                  val.set_on(Private, self.as_mut());
              }

              pub fn get(&self) -> $t {
                  self.inner.get()
              }

              pub fn clear(&mut self) {
                  // The default value for a boolean field is false and 0 for numerical types. It
                  // matches the Rust default values for corresponding types. Let's use this fact.
                  SettableValue::<$t>::set_on(<$t>::default(), Private, MutProxy::as_mut(self));
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
      )*
  }
}

impl_singular_primitives!(bool, f32, f64, i32, i64, u32, u64);
