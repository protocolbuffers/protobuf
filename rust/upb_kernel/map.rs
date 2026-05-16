use super::*;

/// Returns a static empty MapView.
pub fn empty_map<K, V>() -> MapView<'static, K, V>
where
    K: MapKey,
    V: MapValue,
{
    // TODO: Consider creating a static empty map in C.

    // Use `<bool, bool>` for a shared empty map for all map types.
    //
    // This relies on an implicit contract with UPB that it is OK to use an empty
    // Map<bool, bool> as an empty map of all other types. The only const
    // function on `upb_Map` that will care about the size of key or value is
    // `get()` where it will hash the appropriate number of bytes of the
    // provided `upb_MessageValue`, and that bool being the smallest type in the
    // union means it will happen to work for all possible key types.
    //
    // If we used a larger key, then UPB would hash more bytes of the key than Rust
    // initialized.
    static EMPTY_MAP_VIEW: OnceLock<Map<bool, bool>> = OnceLock::new();

    // SAFETY:
    // - The map is empty and never mutated.
    // - The value type is never used.
    // - The size of the key type is used when `get()` computes the hash of the key.
    //   The map is empty, therefore it doesn't matter what hash is computed, but we
    //   have to use `bool` type as the smallest key possible (otherwise UPB would
    //   read more bytes than Rust allocated).
    unsafe {
        MapView::from_raw(Private, EMPTY_MAP_VIEW.get_or_init(Map::new).as_view().as_raw(Private))
    }
}

impl<'msg, K: ?Sized, V: ?Sized> MapMut<'msg, K, V> {
    // Returns a `RawArena` which is live for at least `'msg`
    #[doc(hidden)]
    pub fn raw_arena(&mut self, _private: Private) -> RawArena {
        self.inner.arena.raw()
    }

    // Returns an `Arena` which is live for at least `'msg`
    #[doc(hidden)]
    pub fn arena(&self, _private: Private) -> &'msg Arena {
        self.inner.arena
    }
}

#[derive(Debug)]
#[doc(hidden)]
pub struct InnerMap {
    pub(crate) raw: RawMap,
    arena: Arena,
}

impl InnerMap {
    pub fn new(raw: RawMap, arena: Arena) -> Self {
        Self { raw, arena }
    }

    pub fn as_mut(&mut self) -> InnerMapMut<'_> {
        InnerMapMut { raw: self.raw, arena: &self.arena }
    }
}

#[derive(Clone, Copy, Debug)]
#[doc(hidden)]
pub struct InnerMapMut<'msg> {
    pub(crate) raw: RawMap,
    arena: &'msg Arena,
}

#[doc(hidden)]
impl<'msg> InnerMapMut<'msg> {
    pub fn new(raw: RawMap, arena: &'msg Arena) -> Self {
        InnerMapMut { raw, arena }
    }

    #[doc(hidden)]
    pub fn as_raw(&self) -> RawMap {
        self.raw
    }

    pub fn arena(&mut self) -> &Arena {
        self.arena
    }

    #[doc(hidden)]
    pub fn raw_arena(&mut self) -> RawArena {
        self.arena.raw()
    }
}

#[doc(hidden)]
pub struct RawMapIter {
    // TODO: Replace this `RawMap` with the const type.
    map: RawMap,
    iter: usize,
}

impl RawMapIter {
    pub fn new(map: RawMap) -> Self {
        RawMapIter { map, iter: UPB_MAP_BEGIN }
    }

    /// # Safety
    /// - `self.map` must be valid, and remain valid while the return value is
    ///   in use.
    pub unsafe fn next_unchecked(&mut self) -> Option<(upb_MessageValue, upb_MessageValue)> {
        let mut key = MaybeUninit::uninit();
        let mut value = MaybeUninit::uninit();
        // SAFETY: the `map` is valid as promised by the caller
        unsafe { upb_Map_Next(self.map, key.as_mut_ptr(), value.as_mut_ptr(), &mut self.iter) }
            // SAFETY: if upb_Map_Next returns true, then key and value have been populated.
            .then(|| unsafe { (key.assume_init(), value.assume_init()) })
    }
}

impl<MessageType> MapValue for MessageType
where
    Self: Proxied + EntityType + UpbTypeConversions<<Self as EntityType>::Tag>,
{
    fn map_new<K: MapKey>(_private: Private) -> Map<K, Self> {
        let arena = Arena::new();
        let raw = unsafe { upb_Map_New(arena.raw(), K::upb_type(), Self::upb_type()) };

        Map::from_inner(Private, InnerMap::new(raw, arena))
    }

    unsafe fn map_free<K: MapKey>(_private: Private, _map: &mut Map<K, Self>) {
        // No-op: the memory will be dropped by the arena.
    }

    fn map_clear<K: MapKey>(_private: Private, mut map: MapMut<K, Self>) {
        unsafe {
            upb_Map_Clear(map.as_raw(Private));
        }
    }

    fn map_len<K: MapKey>(_private: Private, map: MapView<K, Self>) -> usize {
        unsafe { upb_Map_Size(map.as_raw(Private)) }
    }

    fn map_insert<K: MapKey>(
        _private: Private,
        mut map: MapMut<K, Self>,
        key: View<'_, K>,
        value: impl IntoProxied<Self>,
    ) -> bool {
        let arena = map.inner(Private).raw_arena();
        let insert_status = unsafe {
            upb_Map_Insert(
                map.as_raw(Private),
                K::to_message_value(key),
                Self::into_message_value_fuse_if_required(arena, value.into_proxied(Private)),
                arena,
            )
        };
        match insert_status {
            upb::MapInsertStatus::Inserted => true,
            upb::MapInsertStatus::Replaced => false,
            upb::MapInsertStatus::OutOfMemory => {
                panic!("map insert failed (alloc should be infallible)")
            }
        }
    }

    fn map_get<'a, K: MapKey>(
        _private: Private,
        map: MapView<'a, K, Self>,
        key: View<'_, K>,
    ) -> Option<View<'a, Self>> {
        let mut val = MaybeUninit::uninit();
        let found =
            unsafe { upb_Map_Get(map.as_raw(Private), K::to_message_value(key), val.as_mut_ptr()) };
        if !found {
            return None;
        }
        Some(unsafe { Self::from_message_value(val.assume_init()) })
    }

    fn map_get_mut<'a, K: MapKey>(
        _private: Private,
        mut map: MapMut<'a, K, Self>,
        key: View<'_, K>,
    ) -> Option<Mut<'a, Self>>
    where
        Self: Message,
    {
        // SAFETY: The map is valid as promised by the caller.
        let val = unsafe { upb_Map_GetMutable(map.as_raw(Private), K::to_message_value(key)) };
        // SAFETY: The lifetime of the MapMut is guaranteed to outlive the returned Mut.
        NonNull::new(val).map(|msg| unsafe { Self::from_message_mut(msg, map.arena(Private)) })
    }

    fn map_remove<K: MapKey>(
        _private: Private,
        mut map: MapMut<K, Self>,
        key: View<'_, K>,
    ) -> bool {
        unsafe { upb_Map_Delete(map.as_raw(Private), K::to_message_value(key), ptr::null_mut()) }
    }
    fn map_iter<K: MapKey>(_private: Private, map: MapView<K, Self>) -> MapIter<K, Self> {
        // SAFETY: MapView<'_,..>> guarantees its RawMap outlives '_.
        unsafe { MapIter::from_raw(Private, RawMapIter::new(map.as_raw(Private))) }
    }

    fn map_iter_next<'a, K: MapKey>(
        _private: Private,
        iter: &mut MapIter<'a, K, Self>,
    ) -> Option<(View<'a, K>, View<'a, Self>)> {
        // SAFETY: MapIter<'a, ..> guarantees its RawMapIter outlives 'a.
        unsafe { iter.as_raw_mut(Private).next_unchecked() }
            // SAFETY: MapIter<K, V> returns key and values message values
            //         with the variants for K and V active.
            .map(|(k, v)| unsafe { (K::from_message_value(k), Self::from_message_value(v)) })
    }
}
