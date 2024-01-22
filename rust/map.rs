// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use crate::{
    Mut, MutProxy, Proxied, SettableValue, View, ViewProxy,
    __internal::{Private, RawMap},
    __runtime::InnerMapMut,
};
use std::marker::PhantomData;

#[repr(transparent)]
pub struct MapView<'msg, K: ?Sized, V: ?Sized> {
    pub raw: RawMap,
    _phantom_key: PhantomData<&'msg K>,
    _phantom_value: PhantomData<&'msg V>,
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

pub struct MapMut<'msg, K: ?Sized, V: ?Sized> {
    pub(crate) inner: InnerMapMut<'msg>,
    _phantom_key: PhantomData<&'msg K>,
    _phantom_value: PhantomData<&'msg V>,
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

pub struct Map<K: ?Sized + Proxied, V: ?Sized + ProxiedInMapValue<K>> {
    pub(crate) inner: InnerMapMut<'static>,
    _phantom_key: PhantomData<K>,
    _phantom_value: PhantomData<V>,
}

impl<K: ?Sized + Proxied, V: ?Sized + ProxiedInMapValue<K>> Drop for Map<K, V> {
    fn drop(&mut self) {
        // SAFETY:
        // - `drop` is only called once.
        // - 'map_free` is only called here.
        unsafe { V::map_free(Private, self) }
    }
}

pub trait ProxiedInMapValue<K>: Proxied
where
    K: Proxied + ?Sized,
{
    fn map_new(_private: Private) -> Map<K, Self>;

    /// # Safety
    /// - After `map_free`, no other methods on the input are safe to call.
    unsafe fn map_free(_private: Private, map: &mut Map<K, Self>);

    fn map_clear(map: Mut<'_, Map<K, Self>>);
    fn map_len(map: View<'_, Map<K, Self>>) -> usize;
    fn map_insert(map: Mut<'_, Map<K, Self>>, key: View<'_, K>, value: View<'_, Self>) -> bool;
    fn map_get<'a>(map: View<'a, Map<K, Self>>, key: View<'_, K>) -> Option<View<'a, Self>>;
    fn map_remove(map: Mut<'_, Map<K, Self>>, key: View<'_, K>) -> bool;
}

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
        MapView { raw: self.raw, _phantom_key: PhantomData, _phantom_value: PhantomData }
    }
}

impl<'msg, K: Proxied + ?Sized, V: ProxiedInMapValue<K> + ?Sized> ViewProxy<'msg>
    for MapMut<'msg, K, V>
{
    type Proxied = Map<K, V>;

    fn as_view(&self) -> View<'_, Self::Proxied> {
        MapView { raw: self.inner.raw, _phantom_key: PhantomData, _phantom_value: PhantomData }
    }

    fn into_view<'shorter>(self) -> View<'shorter, Self::Proxied>
    where
        'msg: 'shorter,
    {
        MapView { raw: self.inner.raw, _phantom_key: PhantomData, _phantom_value: PhantomData }
    }
}

impl<'msg, K: Proxied + ?Sized, V: ProxiedInMapValue<K> + ?Sized> MutProxy<'msg>
    for MapMut<'msg, K, V>
{
    fn as_mut(&mut self) -> Mut<'_, Self::Proxied> {
        MapMut { inner: self.inner, _phantom_key: PhantomData, _phantom_value: PhantomData }
    }

    fn into_mut<'shorter>(self) -> Mut<'shorter, Self::Proxied>
    where
        'msg: 'shorter,
    {
        MapMut { inner: self.inner, _phantom_key: PhantomData, _phantom_value: PhantomData }
    }
}

impl<K, V> Map<K, V>
where
    K: Proxied + ?Sized,
    V: ProxiedInMapValue<K> + ?Sized,
{
    #[allow(dead_code)]
    pub(crate) fn new() -> Self {
        V::map_new(Private)
    }

    pub fn as_mut(&mut self) -> MapMut<'_, K, V> {
        MapMut { inner: self.inner, _phantom_key: PhantomData, _phantom_value: PhantomData }
    }

    pub fn as_view(&self) -> MapView<'_, K, V> {
        MapView { raw: self.inner.raw, _phantom_key: PhantomData, _phantom_value: PhantomData }
    }

    /// # Safety
    /// - `inner` must be valid to read and write from for `'static`.
    /// - There must be no aliasing references or mutations on the same
    ///   underlying object.
    pub unsafe fn from_inner(_private: Private, inner: InnerMapMut<'static>) -> Self {
        Self { inner, _phantom_key: PhantomData, _phantom_value: PhantomData }
    }
}

impl<'msg, K, V> MapView<'msg, K, V>
where
    K: Proxied + ?Sized + 'msg,
    V: ProxiedInMapValue<K> + ?Sized + 'msg,
{
    #[doc(hidden)]
    pub fn as_raw(&self, _private: Private) -> RawMap {
        self.raw
    }

    /// # Safety
    /// - `raw` must be valid to read from for `'msg`.
    #[doc(hidden)]
    pub unsafe fn from_raw(_private: Private, raw: RawMap) -> Self {
        Self { raw, _phantom_key: PhantomData, _phantom_value: PhantomData }
    }

    pub fn get<'a>(self, key: impl Into<View<'a, K>>) -> Option<View<'msg, V>>
    where
        K: 'a,
    {
        V::map_get(self, key.into())
    }

    pub fn len(self) -> usize {
        V::map_len(self)
    }

    pub fn is_empty(self) -> bool {
        self.len() == 0
    }
}

impl<'msg, K, V> MapMut<'msg, K, V>
where
    K: Proxied + ?Sized + 'msg,
    V: ProxiedInMapValue<K> + ?Sized + 'msg,
{
    /// # Safety
    /// - `inner` must be valid to read and write from for `'msg`.
    pub unsafe fn from_inner(_private: Private, inner: InnerMapMut<'msg>) -> Self {
        Self { inner, _phantom_key: PhantomData, _phantom_value: PhantomData }
    }

    pub fn len(self) -> usize {
        self.as_view().len()
    }

    pub fn is_empty(self) -> bool {
        self.len() == 0
    }

    pub fn insert<'a, 'b>(
        &mut self,
        key: impl Into<View<'a, K>>,
        value: impl Into<View<'b, V>>,
    ) -> bool
    where
        K: 'a,
        V: 'b,
    {
        V::map_insert(self.as_mut(), key.into(), value.into())
    }

    pub fn remove<'a>(&mut self, key: impl Into<View<'a, K>>) -> bool
    where
        K: 'a,
    {
        V::map_remove(self.as_mut(), key.into())
    }

    pub fn clear(&mut self) {
        V::map_clear(self.as_mut())
    }

    pub fn get<'a>(&self, key: impl Into<View<'a, K>>) -> Option<View<V>>
    where
        K: 'a,
    {
        V::map_get(self.as_view(), key.into())
    }

    pub fn copy_from(&mut self, _src: MapView<'_, K, V>) {
        todo!("implement b/28530933");
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::ProtoStr;
    use googletest::prelude::*;

    #[test]
    fn test_proxied_scalar() {
        let mut map: Map<i32, i64> = Map::new();
        let mut map_mut = map.as_mut();
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
        let mut map: Map<ProtoStr, ProtoStr> = Map::new();
        let mut map_mut = map.as_mut();
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
    fn test_all_maps_can_be_constructed() {
        macro_rules! gen_proto_values {
            ($key_t:ty, $($value_t:ty),*) => {
                $(
                    let map = Map::<$key_t, $value_t>::new();
                    assert_that!(map.as_view().len(), eq(0));
                )*
            }
        }

        macro_rules! gen_proto_keys {
            ($($key_t:ty),*) => {
                $(
                    gen_proto_values!($key_t, f32, f64, i32, u32, i64, bool, ProtoStr, [u8]);
                )*
            }
        }

        gen_proto_keys!(i32, u32, i64, u64, bool, ProtoStr);
    }

    #[test]
    fn test_dbg() {
        let mut map = Map::<i32, f64>::new();
        assert_that!(format!("{:?}", map.as_view()), eq("MapView(\"i32\", \"f64\")"));
        assert_that!(format!("{:?}", map.as_mut()), eq("MapMut(\"i32\", \"f64\")"));
    }
}
