// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use crate::{
    Mut, MutProxy, Proxied, SettableValue, View, ViewProxy,
    __internal::Private,
    __runtime::{MapInner, ProxiedInMapValue},
};
use std::marker::PhantomData;

#[repr(transparent)]
pub struct MapView<'msg, K: ?Sized, V: ?Sized> {
    inner: MapInner<'msg, K, V>,
}

impl<'msg, K: ?Sized, V: ?Sized> MapView<'msg, K, V> {
    pub fn from_inner(_private: Private, inner: MapInner<'msg, K, V>) -> Self {
        Self { inner }
    }
}

impl<'msg, K: ?Sized, V: ?Sized> Copy for MapView<'msg, K, V> {}
impl<'msg, K: ?Sized, V: ?Sized> Clone for MapView<'msg, K, V> {
    fn clone(&self) -> Self {
        *self
    }
}

unsafe impl<'msg, K: ?Sized, V: ?Sized> Sync for MapView<'msg, K, V> {}
unsafe impl<'msg, K: ?Sized, V: ?Sized> Send for MapView<'msg, K, V> {}

impl<'msg, K: ?Sized, V: ?Sized> std::fmt::Debug for MapView<'msg, K, V> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_tuple("MapView")
            .field(&std::any::type_name::<K>())
            .field(&std::any::type_name::<V>())
            .finish()
    }
}

#[repr(transparent)]
pub struct MapMut<'msg, K: ?Sized, V: ?Sized> {
    inner: MapInner<'msg, K, V>,
}

impl<'msg, K: ?Sized, V: ?Sized> MapMut<'msg, K, V> {
    pub fn from_inner(_private: Private, inner: MapInner<'msg, K, V>) -> Self {
        Self { inner }
    }
}

unsafe impl<'msg, K: ?Sized, V: ?Sized> Sync for MapMut<'msg, K, V> {}

impl<'msg, K: ?Sized, V: ?Sized> std::fmt::Debug for MapMut<'msg, K, V> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_tuple("MapMut")
            .field(&std::any::type_name::<K>())
            .field(&std::any::type_name::<V>())
            .finish()
    }
}

impl<'msg, K: ?Sized, V: ?Sized> std::ops::Deref for MapMut<'msg, K, V> {
    type Target = MapView<'msg, K, V>;
    fn deref(&self) -> &Self::Target {
        // SAFETY:
        //   - `Map{View,Mut}<'msg, T>` are both `#[repr(transparent)]` over
        //     `MapInner<'msg, T>`.
        //   - `MapInner` is a type alias for `NonNull`.
        unsafe { &*(self as *const Self as *const MapView<'msg, K, V>) }
    }
}

// This is a ZST type so we can implement `Proxied`. Users will work with
// `MapView` (`View<'_, Map>>) and `MapMut` (Mut<'_, Map>).
pub struct Map<K: ?Sized, V: ?Sized>(PhantomData<K>, PhantomData<V>);

impl<K: Proxied + ?Sized, V: ProxiedInMapValue<K> + ?Sized> Proxied for Map<K, V> {
    type View<'msg> = MapView<'msg, K, V> where K: 'msg, V: 'msg;
    type Mut<'msg> = MapMut<'msg, K, V> where K: 'msg, V: 'msg;
}

impl<'msg, K: Proxied + ?Sized, V: ProxiedInMapValue<K> + ?Sized> SettableValue<Map<K, V>>
    for MapView<'msg, K, V>
{
    fn set_on<'b>(self, _private: Private, mut mutator: Mut<'b, Map<K, V>>)
    where
        Map<K, V>: 'b,
    {
        mutator.copy_from(self);
    }
}

impl<'msg, K: Proxied + ?Sized, V: ProxiedInMapValue<K> + ?Sized> ViewProxy<'msg>
    for MapView<'msg, K, V>
{
    type Proxied = Map<K, V>;

    fn as_view(&self) -> View<'_, Self::Proxied> {
        *self
    }

    fn into_view<'shorter>(self) -> View<'shorter, Self::Proxied>
    where
        'msg: 'shorter,
    {
        MapView { inner: self.inner }
    }
}

impl<'msg, K: Proxied + ?Sized, V: ProxiedInMapValue<K> + ?Sized> ViewProxy<'msg>
    for MapMut<'msg, K, V>
{
    type Proxied = Map<K, V>;

    fn as_view(&self) -> View<'_, Self::Proxied> {
        **self
    }

    fn into_view<'shorter>(self) -> View<'shorter, Self::Proxied>
    where
        'msg: 'shorter,
    {
        *self.into_mut::<'shorter>()
    }
}

impl<'msg, K: Proxied + ?Sized, V: ProxiedInMapValue<K> + ?Sized> MutProxy<'msg>
    for MapMut<'msg, K, V>
{
    fn as_mut(&mut self) -> Mut<'_, Self::Proxied> {
        MapMut { inner: self.inner }
    }

    fn into_mut<'shorter>(self) -> Mut<'shorter, Self::Proxied>
    where
        'msg: 'shorter,
    {
        MapMut { inner: self.inner }
    }
}

impl<'msg, K, V> MapView<'msg, K, V>
where
    K: Proxied + ?Sized + 'msg,
    V: ProxiedInMapValue<K> + ?Sized + 'msg,
{
    pub fn get<'a>(&self, key: impl Into<View<'a, K>>) -> Option<View<'msg, V>>
    where
        K: 'a,
    {
        self.inner.get(key.into())
    }

    pub fn len(&self) -> usize {
        self.inner.size()
    }

    pub fn is_empty(&self) -> bool {
        self.len() == 0
    }
}

impl<'msg, K, V> MapMut<'msg, K, V>
where
    K: Proxied + ?Sized + 'msg,
    V: ProxiedInMapValue<K> + ?Sized + 'msg,
{
    pub fn insert<'a, 'b>(
        &mut self,
        key: impl Into<View<'a, K>>,
        value: impl Into<View<'b, V>>,
    ) -> bool
    where
        K: 'a,
        V: 'b,
    {
        self.inner.insert(key.into(), value.into())
    }

    pub fn remove<'a>(&mut self, key: impl Into<View<'a, K>>) -> bool
    where
        K: 'a,
    {
        self.inner.remove(key.into())
    }

    pub fn clear(&mut self) {
        self.inner.clear()
    }

    pub fn get<'a>(&self, key: impl Into<View<'a, K>>) -> Option<View<V>>
    where
        K: 'a,
    {
        self.as_view().get(key)
    }

    pub fn copy_from(&mut self, _src: MapView<'_, K, V>) {
        todo!("implement b/28530933");
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::__runtime::{new_map_i32_i64, new_map_str_str};
    use googletest::prelude::*;

    #[test]
    fn test_proxied_scalar() {
        let mut map_mut = MapMut::from_inner(Private, new_map_i32_i64());
        map_mut.insert(1, 2);
        assert_that!(map_mut.get(1), eq(Some(2)));

        let map_view_1 = map_mut.as_view();
        assert_that!(map_view_1.len(), eq(1));
        assert_that!(map_view_1.get(1), eq(Some(2)));

        map_mut.insert(3, 4);

        let map_view_2 = map_mut.into_view();
        assert_that!(map_view_2.len(), eq(2));
        assert_that!(map_view_2.get(3), eq(Some(4)));

        {
            let map_view_3 = map_view_2.as_view();
            assert_that!(map_view_3.is_empty(), eq(false));
        }

        let map_view_4 = map_view_2.into_view();
        assert_that!(map_view_4.is_empty(), eq(false));
    }

    #[test]
    fn test_proxied_str() {
        let mut map_mut = MapMut::from_inner(Private, new_map_str_str());
        map_mut.insert("a", "b");

        let map_view_1 = map_mut.as_view();
        assert_that!(map_view_1.len(), eq(1));
        assert_that!(map_view_1.get("a").unwrap(), eq("b"));

        map_mut.insert("c", "d");

        let map_view_2 = map_mut.into_view();
        assert_that!(map_view_2.len(), eq(2));
        assert_that!(map_view_2.get("c").unwrap(), eq("d"));

        {
            let map_view_3 = map_view_2.as_view();
            assert_that!(map_view_3.is_empty(), eq(false));
        }

        let map_view_4 = map_view_2.into_view();
        assert_that!(map_view_4.is_empty(), eq(false));
    }

    #[test]
    fn test_dbg() {
        let map_view = MapView::from_inner(Private, new_map_i32_i64());
        assert_that!(format!("{:?}", map_view), eq("MapView(\"i32\", \"i64\")"));
    }
}
