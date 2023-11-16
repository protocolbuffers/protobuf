// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use crate::{
    __internal::Private,
    __runtime::{
        MapInner, MapWithBoolKeyOps, MapWithI32KeyOps, MapWithI64KeyOps, MapWithU32KeyOps,
        MapWithU64KeyOps,
    },
};
use paste::paste;

#[derive(Clone, Copy)]
#[repr(transparent)]
pub struct MapView<'a, K: ?Sized, V: ?Sized> {
    inner: MapInner<'a, K, V>,
}

#[derive(Clone, Copy)]
#[repr(transparent)]
pub struct MapMut<'a, K: ?Sized, V: ?Sized> {
    inner: MapInner<'a, K, V>,
}

impl<'a, K: ?Sized, V: ?Sized> MapView<'a, K, V> {
    pub fn from_inner(_private: Private, inner: MapInner<'a, K, V>) -> Self {
        Self { inner }
    }
}

impl<'a, K: ?Sized, V: ?Sized> MapMut<'a, K, V> {
    pub fn from_inner(_private: Private, inner: MapInner<'a, K, V>) -> Self {
        Self { inner }
    }
}

macro_rules! impl_scalar_map_keys {
  ($(key_type $type:ty;)*) => {
      paste! { $(
        impl<'a, V: [< MapWith $type:camel KeyOps >]> MapView<'a, $type, V> {
          pub fn get(&self, key: $type) -> Option<V> {
            self.inner.get(key)
          }

          pub fn len(&self) -> usize {
            self.inner.size()
          }

          pub fn is_empty(&self) -> bool {
            self.len() == 0
          }
        }

        impl<'a, V: [< MapWith $type:camel KeyOps >]> MapMut<'a, $type, V> {
          pub fn insert(&mut self, key: $type, value: V) -> bool {
            self.inner.insert(key, value)
          }

          pub fn remove(&mut self, key: $type) -> Option<V> {
            self.inner.remove(key)
          }

          pub fn clear(&mut self) {
            self.inner.clear()
          }
        }
      )* }
  };
}

impl_scalar_map_keys!(
  key_type i32;
  key_type u32;
  key_type i64;
  key_type u64;
  key_type bool;
);
