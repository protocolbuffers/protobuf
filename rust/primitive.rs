// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
use crate::__internal::SealedInternal;
use crate::{AsView, IntoView, MapKey, Proxied};

macro_rules! impl_singular_primitives {
  ($($t:ty),*) => {
      $(
        impl SealedInternal for $t {}

        impl Proxied for $t {
            type View<'msg> = $t;
        }

        impl AsView for $t {
            type Proxied = $t;

            fn as_view(&self) -> $t {
              *self
          }
        }

        impl<'msg> IntoView<'msg> for $t {
            fn into_view<'shorter>(self) -> $t
            where
                'msg: 'shorter {
              self
          }
        }

        // Singular is implemented in {cpp,upb}.rs
      )*
  }
}

impl_singular_primitives!(bool, f32, f64, i32, i64, u32, u64);

// All numerics except `float` and `double` are allowed as map keys.
impl MapKey for bool {}
impl MapKey for i32 {}
impl MapKey for i64 {}
impl MapKey for u32 {}
impl MapKey for u64 {}
