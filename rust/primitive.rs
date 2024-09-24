// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
use crate::__internal::SealedInternal;
use crate::{AsView, IntoView, Proxied, Proxy, ViewProxy};

macro_rules! impl_singular_primitives {
  ($($t:ty),*) => {
      $(
        impl SealedInternal for $t {}

        impl Proxied for $t {
            type View<'msg> = $t;
        }

        impl<'msg> Proxy<'msg> for $t {
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

        impl<'msg> ViewProxy<'msg> for $t {}

        // ProxiedInRepeated is implemented in {cpp,upb}.rs
      )*
  }
}

impl_singular_primitives!(bool, f32, f64, i32, i64, u32, u64);
