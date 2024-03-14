// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use crate::Viewable;

macro_rules! impl_singular_primitives {
  ($($t:ty),*) => {
      $(
        impl Viewable for $t {
            type View<'msg> = $t;
        }
        // ProxiedInRepeated is implemented in {cpp,upb}.rs
      )*
  }
}

impl_singular_primitives!(bool, f32, f64, i32, i64, u32, u64);
