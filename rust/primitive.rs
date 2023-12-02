// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use std::fmt::Debug;

use crate::__internal::Private;
use crate::__runtime::InnerPrimitiveMut;
use crate::repeated::RepeatedMut;
use crate::vtable::{
    PrimitiveOptionalMutVTable, PrimitiveVTable, ProxiedWithRawOptionalVTable,
    ProxiedWithRawVTable, RawVTableOptionalMutatorData,
};
use crate::{
    Mut, MutProxy, Proxied, ProxiedInRepeated, ProxiedWithPresence, SettableValue, View, ViewProxy,
};

/// A mutator for a primitive (numeric or enum) value of `T`.
///
/// This type is `protobuf::Mut<'msg, T>`.
pub struct PrimitiveMut<'a, T: ProxiedWithRawVTable + ?Sized>(PrimitiveMutChoice<'a, T>);

impl<'a, T> Debug for PrimitiveMut<'a, T>
where
    T: ProxiedWithRawVTable + ?Sized,
{
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_tuple("PrimitiveMut").field(&self.0).finish()
    }
}

/// `PrimitiveMut` may either be a singular or repeated field.
/// This distinguishes between them. This is wrapped in a struct to prevent
/// direct access by users.
enum PrimitiveMutChoice<'a, T: ProxiedWithRawVTable + ?Sized> {
    Singular(InnerPrimitiveMut<'a, T>),
    /// Safety precondition: `.1` must be in-bounds for `.0` for `'a`
    Repeated(RepeatedMut<'a, T>, usize),
}

unsafe impl<'a, T: ProxiedWithRawVTable + ?Sized> Sync for PrimitiveMutChoice<'a, T> {}

impl<'a, T> Debug for PrimitiveMutChoice<'a, T>
where
    T: ProxiedWithRawVTable + ?Sized,
{
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Self::Singular(arg0) => f.debug_tuple("Singular").field(arg0).finish(),
            Self::Repeated(arg0, arg1) => {
                f.debug_tuple("Repeated").field(arg0).field(arg1).finish()
            }
        }
    }
}

impl<'a, T> PrimitiveMut<'a, T>
where
    T: ProxiedWithRawVTable + ?Sized,
{
    #[doc(hidden)]
    pub fn from_singular(_private: Private, inner: InnerPrimitiveMut<'a, T>) -> Self {
        Self(PrimitiveMutChoice::Singular(inner))
    }

    /// # Safety
    /// - `index` must be in-bounds for `repeated`
    /// - `repeated` must not have its length modified or be reallocated for
    ///   `'a`
    /// - For `'a`, there must be no other accessors of element `index` of
    ///   `repeated` (elements other than `index` may be mutated).
    #[doc(hidden)]
    pub unsafe fn from_repeated(
        _private: Private,
        repeated: RepeatedMut<'a, T>,
        index: usize,
    ) -> Self {
        Self(PrimitiveMutChoice::Repeated(repeated, index))
    }
}

impl<'a, T> PrimitiveMut<'a, T>
where
    T: ProxiedWithRawVTable + ProxiedInRepeated + ?Sized,
    Self: MutProxy<'a, Proxied = T>,
{
    pub fn get(&self) -> View<'_, T> {
        match &self.0 {
            PrimitiveMutChoice::Singular(s) => T::make_view(Private, *s),
            // SAFETY: `i` is in-bounds for `r` as a precondition of the type
            PrimitiveMutChoice::Repeated(r, i) => unsafe { r.as_view().get(*i).unwrap_unchecked() },
        }
    }

    pub fn set(&mut self, val: impl SettableValue<T>) {
        MutProxy::set(self, val)
        // val.set_on(Private, self.as_mut())
    }
}

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
                PrimitiveMut(match &mut self.0 {
                    PrimitiveMutChoice::Singular(s) => {
                        PrimitiveMutChoice::Singular(*s)
                    }
                    PrimitiveMutChoice::Repeated(r, i) => {
                        PrimitiveMutChoice::Repeated(r.as_mut(), *i)
                    }
                })
            }

            fn into_mut<'shorter>(self) -> Mut<'shorter, Self::Proxied>
            where 'a: 'shorter,
            {
                self
            }
        }

        impl SettableValue<$t> for $t {
            fn set_on<'a>(self, _private: Private, mutator: Mut<'a, $t>) where $t: 'a {
                match mutator.0 {
                    PrimitiveMutChoice::Singular(s) => {
                        unsafe { s.set(self) };
                    }
                    PrimitiveMutChoice::Repeated(mut r, i) => {
                        r.set(i, self);
                    }
                }
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
                PrimitiveMut::from_singular(Private, inner)
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

        // ProxiedInRepeated is implemented in {cpp,upb}.rs
      )*
  }
}

impl_singular_primitives!(bool, f32, f64, i32, i64, u32, u64);
