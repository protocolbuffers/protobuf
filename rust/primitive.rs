// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
use crate::__internal::Private;
use crate::{IntoProxied, Proxied, View, ViewProxy};

macro_rules! impl_singular_primitives {
  ($($t:ty),*) => {
      $(
        impl Proxied for $t {
            type View<'msg> = $t;
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

        impl IntoProxied<$t> for $t {
          fn into_proxied(self, _private: Private) -> $t {
            self
          }
        }

        // ProxiedInRepeated is implemented in {cpp,upb}.rs
      )*
  }
}

impl_singular_primitives!(bool, f32, f64, i32, i64, u32, u64);
