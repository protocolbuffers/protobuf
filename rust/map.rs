// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use crate::{
    AsMut, AsView, IntoMut, IntoProxied, IntoView, MutProxied, MutProxy, Proxied, Proxy, View,
    ViewProxy,
    __internal::runtime::{InnerMap, InnerMapMut, RawMap, RawMapIter},
    __internal::{Private, SealedInternal},
};
use std::marker::PhantomData;

#[repr(transparent)]
pub struct MapView<'msg, K: ?Sized, V: ?Sized> {
    raw: RawMap,
    _phantom: PhantomData<(&'msg K, &'msg V)>,
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
    _phantom: PhantomData<(&'msg mut K, &'msg mut V)>,
}

impl<'msg, K: ?Sized, V: ?Sized> MapMut<'msg, K, V> {
    pub fn inner(&self, _private: Private) -> InnerMapMut {
        self.inner
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

pub struct Map<K: Proxied, V: ProxiedInMapValue<K>> {
    inner: InnerMap,
    _phantom: PhantomData<(PhantomData<K>, PhantomData<V>)>,
}

// SAFETY: `Map` is Sync because it does not implement interior mutability.
unsafe impl<K: Proxied, V: ProxiedInMapValue<K>> Sync for Map<K, V> {}

// SAFETY: `Map` is Send because it's not bound to a specific thread e.g.
// it does not use thread-local data or similar.
unsafe impl<K: Proxied, V: ProxiedInMapValue<K>> Send for Map<K, V> {}

impl<K: Proxied, V: ProxiedInMapValue<K>> SealedInternal for Map<K, V> {}

impl<K: Proxied, V: ProxiedInMapValue<K>> Drop for Map<K, V> {
    fn drop(&mut self) {
        // SAFETY:
        // - `drop` is only called once.
        // - 'map_free` is only called here.
        unsafe { V::map_free(Private, self) }
    }
}

pub trait ProxiedInMapValue<K>: Proxied
where
    K: Proxied,
{
    fn map_new(_private: Private) -> Map<K, Self>;

    /// # Safety
    /// - After `map_free`, no other methods on the input are safe to call.
    unsafe fn map_free(_private: Private, map: &mut Map<K, Self>);

    fn map_clear(map: MapMut<K, Self>);
    fn map_len(map: MapView<K, Self>) -> usize;
    fn map_insert(map: MapMut<K, Self>, key: View<'_, K>, value: impl IntoProxied<Self>) -> bool;
    fn map_get<'a>(map: MapView<'a, K, Self>, key: View<'_, K>) -> Option<View<'a, Self>>;
    fn map_remove(map: MapMut<K, Self>, key: View<'_, K>) -> bool;

    fn map_iter(map: MapView<K, Self>) -> MapIter<K, Self>;
    fn map_iter_next<'a>(iter: &mut MapIter<'a, K, Self>) -> Option<(View<'a, K>, View<'a, Self>)>;
}

impl<K: Proxied, V: ProxiedInMapValue<K>> Proxied for Map<K, V> {
    type View<'msg>
        = MapView<'msg, K, V>
    where
        K: 'msg,
        V: 'msg;
}

impl<K: Proxied, V: ProxiedInMapValue<K>> AsView for Map<K, V> {
    type Proxied = Self;

    fn as_view(&self) -> MapView<'_, K, V> {
        self.as_view()
    }
}

impl<K: Proxied, V: ProxiedInMapValue<K>> MutProxied for Map<K, V> {
    type Mut<'msg>
        = MapMut<'msg, K, V>
    where
        K: 'msg,
        V: 'msg;
}

impl<K: Proxied, V: ProxiedInMapValue<K>> AsMut for Map<K, V> {
    type MutProxied = Self;

    fn as_mut(&mut self) -> MapMut<'_, K, V> {
        self.as_mut()
    }
}

impl<'msg, K: Proxied, V: ProxiedInMapValue<K>> SealedInternal for MapView<'msg, K, V> {}

impl<'msg, K: Proxied, V: ProxiedInMapValue<K>> Proxy<'msg> for MapView<'msg, K, V> {}

impl<'msg, K: Proxied, V: ProxiedInMapValue<K>> AsView for MapView<'msg, K, V> {
    type Proxied = Map<K, V>;

    fn as_view(&self) -> MapView<'_, K, V> {
        *self
    }
}

impl<'msg, K: Proxied, V: ProxiedInMapValue<K>> IntoView<'msg> for MapView<'msg, K, V> {
    fn into_view<'shorter>(self) -> MapView<'shorter, K, V>
    where
        'msg: 'shorter,
    {
        MapView { raw: self.raw, _phantom: PhantomData }
    }
}

impl<'msg, K: Proxied, V: ProxiedInMapValue<K>> ViewProxy<'msg> for MapView<'msg, K, V> {}

impl<'msg, K: Proxied, V: ProxiedInMapValue<K>> SealedInternal for MapMut<'msg, K, V> {}

impl<'msg, K: Proxied, V: ProxiedInMapValue<K>> Proxy<'msg> for MapMut<'msg, K, V> {}

impl<'msg, K: Proxied, V: ProxiedInMapValue<K>> AsView for MapMut<'msg, K, V> {
    type Proxied = Map<K, V>;

    fn as_view(&self) -> MapView<'_, K, V> {
        MapView { raw: self.inner.raw, _phantom: PhantomData }
    }
}

impl<'msg, K: Proxied, V: ProxiedInMapValue<K>> IntoView<'msg> for MapMut<'msg, K, V> {
    fn into_view<'shorter>(self) -> MapView<'shorter, K, V>
    where
        'msg: 'shorter,
    {
        MapView { raw: self.inner.raw, _phantom: PhantomData }
    }
}

impl<'msg, K: Proxied, V: ProxiedInMapValue<K>> AsMut for MapMut<'msg, K, V> {
    type MutProxied = Map<K, V>;

    fn as_mut(&mut self) -> MapMut<'_, K, V> {
        MapMut { inner: self.inner, _phantom: PhantomData }
    }
}

impl<'msg, K: Proxied, V: ProxiedInMapValue<K>> IntoMut<'msg> for MapMut<'msg, K, V> {
    fn into_mut<'shorter>(self) -> MapMut<'shorter, K, V>
    where
        'msg: 'shorter,
    {
        MapMut { inner: self.inner, _phantom: PhantomData }
    }
}

impl<'msg, K: Proxied, V: ProxiedInMapValue<K>> MutProxy<'msg> for MapMut<'msg, K, V> {}

impl<K, V> Map<K, V>
where
    K: Proxied,
    V: ProxiedInMapValue<K>,
{
    pub fn new() -> Self {
        V::map_new(Private)
    }

    pub fn as_mut(&mut self) -> MapMut<'_, K, V> {
        MapMut { inner: self.inner.as_mut(), _phantom: PhantomData }
    }

    pub fn as_view(&self) -> MapView<'_, K, V> {
        MapView { raw: self.inner.raw, _phantom: PhantomData }
    }

    #[doc(hidden)]
    pub fn from_inner(_private: Private, inner: InnerMap) -> Self {
        Self { inner, _phantom: PhantomData }
    }

    pub fn as_raw(&self, _private: Private) -> RawMap {
        self.inner.raw
    }
}

impl<K, V> Default for Map<K, V>
where
    K: Proxied,
    V: ProxiedInMapValue<K>,
{
    fn default() -> Self {
        Map::new()
    }
}

#[doc(hidden)]
impl<'msg, K: ?Sized, V: ?Sized> MapView<'msg, K, V> {
    #[doc(hidden)]
    pub fn as_raw(&self, _private: Private) -> RawMap {
        self.raw
    }

    /// # Safety
    /// - `raw` must be valid to read from for `'msg`.
    #[doc(hidden)]
    pub unsafe fn from_raw(_private: Private, raw: RawMap) -> Self {
        Self { raw, _phantom: PhantomData }
    }
}

impl<'msg, K, V> MapView<'msg, K, V>
where
    K: Proxied + 'msg,
    V: ProxiedInMapValue<K> + 'msg,
{
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

    /// Returns an iterator visiting all key-value pairs in arbitrary order.
    ///
    /// The iterator element type is `(View<K>, View<V>)`.
    /// This is an alias for `<Self as IntoIterator>::into_iter`.
    pub fn iter(self) -> MapIter<'msg, K, V> {
        self.into_iter()
    }

    /// Returns an iterator visiting all keys in arbitrary order.
    ///
    /// The iterator element type is `View<K>`.
    pub fn keys(self) -> impl Iterator<Item = View<'msg, K>> + 'msg {
        self.into_iter().map(|(k, _)| k)
    }

    /// Returns an iterator visiting all values in arbitrary order.
    ///
    /// The iterator element type is `View<V>`.
    pub fn values(self) -> impl Iterator<Item = View<'msg, V>> + 'msg {
        self.into_iter().map(|(_, v)| v)
    }
}

#[doc(hidden)]
impl<'msg, K: ?Sized, V: ?Sized> MapMut<'msg, K, V> {
    /// # Safety
    /// - `inner` must be valid to read and write from for `'msg`.
    #[doc(hidden)]
    pub unsafe fn from_inner(_private: Private, inner: InnerMapMut<'msg>) -> Self {
        Self { inner, _phantom: PhantomData }
    }

    #[doc(hidden)]
    pub fn as_raw(&mut self, _private: Private) -> RawMap {
        self.inner.raw
    }
}

impl<'msg, K, V> MapMut<'msg, K, V>
where
    K: Proxied + 'msg,
    V: ProxiedInMapValue<K> + 'msg,
{
    pub fn len(&self) -> usize {
        self.as_view().len()
    }

    pub fn is_empty(&self) -> bool {
        self.len() == 0
    }

    /// Adds a key-value pair to the map.
    ///
    /// Returns `true` if the entry was newly inserted.
    pub fn insert<'a>(&mut self, key: impl Into<View<'a, K>>, value: impl IntoProxied<V>) -> bool
    where
        K: 'a,
    {
        V::map_insert(self.as_mut(), key.into(), value)
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

    pub fn copy_from<'a>(
        &mut self,
        src: impl IntoIterator<Item = (impl Into<View<'a, K>>, impl IntoProxied<V>)>,
    ) where
        K: 'a,
    {
        //TODO
        self.clear();
        for (k, v) in src.into_iter() {
            self.insert(k, v);
        }
    }

    /// Returns an iterator visiting all key-value pairs in arbitrary order.
    ///
    /// The iterator element type is `(View<K>, View<V>)`.
    pub fn iter(&self) -> MapIter<K, V> {
        self.into_iter()
    }

    /// Returns an iterator visiting all keys in arbitrary order.
    ///
    /// The iterator element type is `View<K>`.
    pub fn keys(&self) -> impl Iterator<Item = View<'_, K>> + '_ {
        self.as_view().keys()
    }

    /// Returns an iterator visiting all values in arbitrary order.
    ///
    /// The iterator element type is `View<V>`.
    pub fn values(&self) -> impl Iterator<Item = View<'_, V>> + '_ {
        self.as_view().values()
    }
}

impl<'msg, K, V> IntoProxied<Map<K, V>> for MapView<'msg, K, V>
where
    K: Proxied,
    V: ProxiedInMapValue<K>,
    View<'msg, V>: IntoProxied<V>,
{
    fn into_proxied(self, _private: Private) -> Map<K, V> {
        let mut m = Map::<K, V>::new();
        m.as_mut().copy_from(self);
        m
    }
}

impl<'msg, K, V> IntoProxied<Map<K, V>> for MapMut<'msg, K, V>
where
    K: Proxied,
    V: ProxiedInMapValue<K>,
    View<'msg, V>: IntoProxied<V>,
{
    fn into_proxied(self, _private: Private) -> Map<K, V> {
        self.into_view().into_proxied(Private)
    }
}

impl<'msg, 'k, 'v, K, KView, V, VView, I> IntoProxied<Map<K, V>> for I
where
    I: Iterator<Item = (KView, VView)>,
    K: Proxied + 'msg + 'k,
    V: ProxiedInMapValue<K> + 'msg + 'v,
    KView: Into<View<'k, K>>,
    VView: IntoProxied<V>,
{
    fn into_proxied(self, _private: Private) -> Map<K, V> {
        let mut m = Map::<K, V>::new();
        m.as_mut().extend(self);
        m
    }
}

/// An iterator visiting all key-value pairs in arbitrary order.
///
/// The iterator element type is `(View<Key>, View<Value>)`.
pub struct MapIter<'msg, K: ?Sized, V: ?Sized> {
    raw: RawMapIter,
    _phantom: PhantomData<(&'msg K, &'msg V)>,
}

impl<'msg, K: ?Sized, V: ?Sized> MapIter<'msg, K, V> {
    /// # Safety
    /// - `raw` must be a valid instance of the raw iterator for `'msg`.
    /// - The untyped `raw` iterator must be for a map of `K,V`.
    /// - The backing map must be live and unmodified for `'msg`.
    #[doc(hidden)]
    pub unsafe fn from_raw(_private: Private, raw: RawMapIter) -> Self {
        Self { raw, _phantom: PhantomData }
    }

    #[doc(hidden)]
    pub fn as_raw_mut(&mut self, _private: Private) -> &mut RawMapIter {
        &mut self.raw
    }
}

impl<'msg, K, V> Iterator for MapIter<'msg, K, V>
where
    K: Proxied + 'msg,
    V: ProxiedInMapValue<K> + 'msg,
{
    type Item = (View<'msg, K>, View<'msg, V>);

    fn next(&mut self) -> Option<Self::Item> {
        V::map_iter_next(self)
    }
}

impl<'msg, K, V> IntoIterator for MapView<'msg, K, V>
where
    K: Proxied + 'msg,
    V: ProxiedInMapValue<K> + 'msg,
{
    type IntoIter = MapIter<'msg, K, V>;
    type Item = (View<'msg, K>, View<'msg, V>);

    fn into_iter(self) -> MapIter<'msg, K, V> {
        V::map_iter(self)
    }
}

impl<'msg, K, V> IntoIterator for &'msg Map<K, V>
where
    K: Proxied + 'msg,
    V: ProxiedInMapValue<K> + 'msg,
{
    type IntoIter = MapIter<'msg, K, V>;
    type Item = (View<'msg, K>, View<'msg, V>);

    fn into_iter(self) -> MapIter<'msg, K, V> {
        self.as_view().into_iter()
    }
}

impl<'a, 'msg, K, V> IntoIterator for &'a MapView<'msg, K, V>
where
    'msg: 'a,
    K: Proxied + 'msg,
    V: ProxiedInMapValue<K> + 'msg,
{
    type IntoIter = MapIter<'msg, K, V>;
    type Item = (View<'msg, K>, View<'msg, V>);

    fn into_iter(self) -> MapIter<'msg, K, V> {
        (*self).into_iter()
    }
}

impl<'a, 'msg, K, V> IntoIterator for &'a MapMut<'msg, K, V>
where
    'msg: 'a,
    K: Proxied + 'msg,
    V: ProxiedInMapValue<K> + 'msg,
{
    type IntoIter = MapIter<'a, K, V>;
    // The View's are valid for 'a instead of 'msg.
    // This is because the mutator may mutate past 'a but before 'msg expires.
    type Item = (View<'a, K>, View<'a, V>);

    fn into_iter(self) -> MapIter<'a, K, V> {
        self.as_view().into_iter()
    }
}

impl<'msg, 'k, 'v, KView, VView, K, V> Extend<(KView, VView)> for MapMut<'msg, K, V>
where
    K: Proxied + 'msg + 'k,
    V: ProxiedInMapValue<K> + 'msg + 'v,
    KView: Into<View<'k, K>>,
    VView: IntoProxied<V>,
{
    fn extend<T: IntoIterator<Item = (KView, VView)>>(&mut self, iter: T) {
        for (k, v) in iter.into_iter() {
            self.insert(k, v);
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{ProtoBytes, ProtoStr, ProtoString};
    use googletest::prelude::*;

    #[gtest]
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

    #[gtest]
    fn test_proxied_str() {
        let mut map: Map<ProtoString, ProtoString> = Map::new();
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

    #[gtest]
    fn test_proxied_iter() {
        let mut map: Map<i32, ProtoString> = Map::new();
        let mut map_mut = map.as_mut();
        map_mut.insert(15, "fizzbuzz");
        map_mut.insert(5, "buzz");
        map_mut.insert(3, "fizz");

        // ProtoStr::from_str is necessary below because
        // https://doc.rust-lang.org/std/primitive.tuple.html#impl-PartialEq-for-(T,)
        // only compares where the types are the same, even when the tuple types can
        // compare with each other.
        // googletest-rust matchers also do not currently implement Clone.
        assert_that!(
            map.as_view().iter().collect::<Vec<_>>(),
            unordered_elements_are![
                eq(&(3, ProtoStr::from_str("fizz"))),
                eq(&(5, ProtoStr::from_str("buzz"))),
                eq(&(15, ProtoStr::from_str("fizzbuzz")))
            ]
        );
        assert_that!(
            map.as_view(),
            unordered_elements_are![
                eq((3, ProtoStr::from_str("fizz"))),
                eq((5, ProtoStr::from_str("buzz"))),
                eq((15, ProtoStr::from_str("fizzbuzz")))
            ]
        );
        assert_that!(
            map.as_mut().iter().collect::<Vec<_>>(),
            unordered_elements_are![
                eq(&(3, ProtoStr::from_str("fizz"))),
                eq(&(5, ProtoStr::from_str("buzz"))),
                eq(&(15, ProtoStr::from_str("fizzbuzz")))
            ]
        );
        assert_that!(
            map.as_mut(),
            unordered_elements_are![
                eq((3, ProtoStr::from_str("fizz"))),
                eq((5, ProtoStr::from_str("buzz"))),
                eq((15, ProtoStr::from_str("fizzbuzz")))
            ]
        );
    }

    #[gtest]
    fn test_overwrite_insert() {
        let mut map: Map<i32, ProtoString> = Map::new();
        let mut map_mut = map.as_mut();
        assert!(map_mut.insert(0, "fizz"));
        // insert should return false when the key is already present
        assert!(!map_mut.insert(0, "buzz"));
        assert_that!(map.as_mut(), unordered_elements_are![eq((0, ProtoStr::from_str("buzz"))),]);
    }

    #[gtest]
    fn test_extend() {
        let mut map: Map<i32, ProtoString> = Map::new();
        let mut map_mut = map.as_mut();

        map_mut.extend([(0, ""); 0]);
        assert_that!(map_mut.len(), eq(0));

        map_mut.extend([(0, "fizz"), (1, "buzz"), (2, "fizzbuzz")]);

        assert_that!(
            map.as_view(),
            unordered_elements_are![
                eq((0, ProtoStr::from_str("fizz"))),
                eq((1, ProtoStr::from_str("buzz"))),
                eq((2, ProtoStr::from_str("fizzbuzz")))
            ]
        );

        let mut map_2: Map<i32, ProtoString> = Map::new();
        let mut map_2_mut = map_2.as_mut();
        map_2_mut.extend([(2, "bing"), (3, "bong")]);

        let mut map_mut = map.as_mut();
        map_mut.extend(&map_2);

        assert_that!(
            map.as_view(),
            unordered_elements_are![
                eq((0, ProtoStr::from_str("fizz"))),
                eq((1, ProtoStr::from_str("buzz"))),
                eq((2, ProtoStr::from_str("bing"))),
                eq((3, ProtoStr::from_str("bong")))
            ]
        );
    }

    #[gtest]
    fn test_copy_from() {
        let mut map: Map<i32, ProtoString> = Map::new();
        let mut map_mut = map.as_mut();
        map_mut.copy_from([(0, "fizz"), (1, "buzz"), (2, "fizzbuzz")]);

        assert_that!(
            map.as_view(),
            unordered_elements_are![
                eq((0, ProtoStr::from_str("fizz"))),
                eq((1, ProtoStr::from_str("buzz"))),
                eq((2, ProtoStr::from_str("fizzbuzz")))
            ]
        );

        let mut map_2: Map<i32, ProtoString> = Map::new();
        let mut map_2_mut = map_2.as_mut();
        map_2_mut.copy_from([(2, "bing"), (3, "bong")]);

        let mut map_mut = map.as_mut();
        map_mut.copy_from(&map_2);

        assert_that!(
            map.as_view(),
            unordered_elements_are![
                eq((2, ProtoStr::from_str("bing"))),
                eq((3, ProtoStr::from_str("bong")))
            ]
        );
    }

    #[gtest]
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
                    gen_proto_values!($key_t, f32, f64, i32, u32, i64, bool, ProtoString, ProtoBytes);
                )*
            }
        }

        gen_proto_keys!(i32, u32, i64, u64, bool, ProtoString);
    }

    #[gtest]
    fn test_dbg() {
        let mut map = Map::<i32, f64>::new();
        assert_that!(format!("{:?}", map.as_view()), eq("MapView(\"i32\", \"f64\")"));
        assert_that!(format!("{:?}", map.as_mut()), eq("MapMut(\"i32\", \"f64\")"));
    }
}
