// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Rust Protobuf runtime using the C++ kernel.

use crate::__internal::{Private, RawArena, RawMap, RawMessage, RawRepeatedField};
use crate::{Mut, PrimitiveMut, ProxiedInRepeated, Repeated, View};
use core::fmt::Debug;
use paste::paste;
use std::alloc::Layout;
use std::cell::UnsafeCell;
use std::fmt;
use std::marker::PhantomData;
use std::mem::MaybeUninit;
use std::ops::Deref;
use std::ptr::{self, NonNull};

/// A wrapper over a `proto2::Arena`.
///
/// This is not a safe wrapper per se, because the allocation functions still
/// have sharp edges (see their safety docs for more info).
///
/// This is an owning type and will automatically free the arena when
/// dropped.
///
/// Note that this type is neither `Sync` nor `Send`.
#[derive(Debug)]
pub struct Arena {
    #[allow(dead_code)]
    ptr: RawArena,
    _not_sync: PhantomData<UnsafeCell<()>>,
}

impl Arena {
    /// Allocates a fresh arena.
    #[inline]
    #[allow(clippy::new_without_default)]
    pub fn new() -> Self {
        Self { ptr: NonNull::dangling(), _not_sync: PhantomData }
    }

    /// Returns the raw, C++-managed pointer to the arena.
    #[inline]
    pub fn raw(&self) -> ! {
        unimplemented!()
    }

    /// Allocates some memory on the arena.
    ///
    /// # Safety
    ///
    /// TODO alignment requirement for layout
    #[inline]
    pub unsafe fn alloc(&self, _layout: Layout) -> &mut [MaybeUninit<u8>] {
        unimplemented!()
    }

    /// Resizes some memory on the arena.
    ///
    /// # Safety
    ///
    /// After calling this function, `ptr` is essentially zapped. `old` must
    /// be the layout `ptr` was allocated with via [`Arena::alloc()`].
    /// TODO alignment for layout
    #[inline]
    pub unsafe fn resize(&self, _ptr: *mut u8, _old: Layout, _new: Layout) -> &[MaybeUninit<u8>] {
        unimplemented!()
    }
}

impl Drop for Arena {
    #[inline]
    fn drop(&mut self) {
        // unimplemented
    }
}

/// Serialized Protobuf wire format data. It's typically produced by
/// `<Message>.serialize()`.
///
/// This struct is ABI-compatible with the equivalent struct on the C++ side. It
/// owns (and drops) its data.
#[repr(C)]
pub struct SerializedData {
    /// Owns the memory.
    data: NonNull<u8>,
    len: usize,
}

impl SerializedData {
    /// Constructs owned serialized data from raw components.
    ///
    /// # Safety
    /// - `data` must be readable for `len` bytes.
    /// - `data` must be an owned pointer and valid until deallocated.
    /// - `data` must have been allocated by the Rust global allocator with a
    ///   size of `len` and align of 1.
    pub unsafe fn from_raw_parts(data: NonNull<u8>, len: usize) -> Self {
        Self { data, len }
    }

    /// Gets a raw slice pointer.
    pub fn as_ptr(&self) -> *const [u8] {
        ptr::slice_from_raw_parts(self.data.as_ptr(), self.len)
    }

    /// Gets a mutable raw slice pointer.
    fn as_mut_ptr(&mut self) -> *mut [u8] {
        ptr::slice_from_raw_parts_mut(self.data.as_ptr(), self.len)
    }
}

impl Deref for SerializedData {
    type Target = [u8];
    fn deref(&self) -> &Self::Target {
        // SAFETY: `data` is valid for `len` bytes until deallocated as promised by
        // `from_raw_parts`.
        unsafe { &*self.as_ptr() }
    }
}

impl Drop for SerializedData {
    fn drop(&mut self) {
        // SAFETY: `data` was allocated by the Rust global allocator with a
        // size of `len` and align of 1 as promised by `from_raw_parts`.
        unsafe { drop(Box::from_raw(self.as_mut_ptr())) }
    }
}

impl fmt::Debug for SerializedData {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        fmt::Debug::fmt(self.deref(), f)
    }
}

pub type BytesPresentMutData<'msg> = crate::vtable::RawVTableOptionalMutatorData<'msg, [u8]>;
pub type BytesAbsentMutData<'msg> = crate::vtable::RawVTableOptionalMutatorData<'msg, [u8]>;
pub type InnerBytesMut<'msg> = crate::vtable::RawVTableMutator<'msg, [u8]>;
pub type InnerPrimitiveMut<'a, T> = crate::vtable::RawVTableMutator<'a, T>;

/// The raw contents of every generated message.
#[derive(Debug)]
pub struct MessageInner {
    pub msg: RawMessage,
}

/// Mutators that point to their original message use this to do so.
///
/// Since C++ messages manage their own memory, this can just copy the
/// `RawMessage` instead of referencing an arena like UPB must.
///
/// Note: even though this type is `Copy`, it should only be copied by
/// protobuf internals that can maintain mutation invariants:
///
/// - No concurrent mutation for any two fields in a message: this means
///   mutators cannot be `Send` but are `Sync`.
/// - If there are multiple accessible `Mut` to a single message at a time, they
///   must be different fields, and not be in the same oneof. As such, a `Mut`
///   cannot be `Clone` but *can* reborrow itself with `.as_mut()`, which
///   converts `&'b mut Mut<'a, T>` to `Mut<'b, T>`.
#[derive(Clone, Copy, Debug)]
pub struct MutatorMessageRef<'msg> {
    msg: RawMessage,
    _phantom: PhantomData<&'msg mut ()>,
}
impl<'msg> MutatorMessageRef<'msg> {
    #[allow(clippy::needless_pass_by_ref_mut)] // Sound construction requires mutable access.
    pub fn new(_private: Private, msg: &'msg mut MessageInner) -> Self {
        MutatorMessageRef { msg: msg.msg, _phantom: PhantomData }
    }

    pub fn from_parent(
        _private: Private,
        _parent_msg: &'msg mut MessageInner,
        message_field_ptr: RawMessage,
    ) -> Self {
        MutatorMessageRef { msg: message_field_ptr, _phantom: PhantomData }
    }

    pub fn msg(&self) -> RawMessage {
        self.msg
    }
}

pub fn copy_bytes_in_arena_if_needed_by_runtime<'a>(
    _msg_ref: MutatorMessageRef<'a>,
    val: &'a [u8],
) -> &'a [u8] {
    // Nothing to do, the message manages its own string memory for C++.
    val
}

/// The raw type-erased pointer version of `RepeatedMut`.
///
/// Contains a `proto2::RepeatedField*` or `proto2::RepeatedPtrField*`.
#[derive(Clone, Copy, Debug)]
pub struct InnerRepeatedMut<'msg> {
    pub(crate) raw: RawRepeatedField,
    _phantom: PhantomData<&'msg ()>,
}

impl<'msg> InnerRepeatedMut<'msg> {
    #[doc(hidden)]
    pub fn new(_private: Private, raw: RawRepeatedField) -> Self {
        InnerRepeatedMut { raw, _phantom: PhantomData }
    }
}

macro_rules! impl_repeated_primitives {
    (@impl $($t:ty => [
        $new_thunk:ident,
        $free_thunk:ident,
        $add_thunk:ident,
        $size_thunk:ident,
        $get_thunk:ident,
        $set_thunk:ident,
        $clear_thunk:ident,
        $copy_from_thunk:ident $(,)?
    ]),* $(,)?) => {
        $(
            extern "C" {
                fn $new_thunk() -> RawRepeatedField;
                fn $free_thunk(f: RawRepeatedField);
                fn $add_thunk(f: RawRepeatedField, v: $t);
                fn $size_thunk(f: RawRepeatedField) -> usize;
                fn $get_thunk(f: RawRepeatedField, i: usize) -> $t;
                fn $set_thunk(f: RawRepeatedField, i: usize, v: $t);
                fn $clear_thunk(f: RawRepeatedField);
                fn $copy_from_thunk(src: RawRepeatedField, dst: RawRepeatedField);
            }

            unsafe impl ProxiedInRepeated for $t {
                #[allow(dead_code)]
                fn repeated_new(_: Private) -> Repeated<$t> {
                    unsafe {
                        Repeated::from_inner(InnerRepeatedMut::new(Private, $new_thunk()))
                    }
                }
                #[allow(dead_code)]
                unsafe fn repeated_free(_: Private, f: &mut Repeated<$t>) {
                    unsafe { $free_thunk(f.as_mut().as_raw(Private)) }
                }
                fn repeated_len(f: View<Repeated<$t>>) -> usize {
                    unsafe { $size_thunk(f.as_raw(Private)) }
                }
                fn repeated_push(mut f: Mut<Repeated<$t>>, v: View<$t>) {
                    unsafe { $add_thunk(f.as_raw(Private), v) }
                }
                fn repeated_clear(mut f: Mut<Repeated<$t>>) {
                    unsafe { $clear_thunk(f.as_raw(Private)) }
                }
                unsafe fn repeated_get_unchecked(f: View<Repeated<$t>>, i: usize) -> View<$t> {
                    unsafe { $get_thunk(f.as_raw(Private), i) }
                }
                unsafe fn repeated_get_unchecked_mut(f: Mut<Repeated<$t>>, i: usize) -> Mut<$t> {
                    // SAFETY: `i` is in-bounds as promised by the caller
                    unsafe { PrimitiveMut::from_repeated(Private, f, i) }
                }
                unsafe fn repeated_set_unchecked(mut f: Mut<Repeated<$t>>, i: usize, v: View<$t>) {
                    unsafe { $set_thunk(f.as_raw(Private), i, v) }
                }
                fn repeated_copy_from(src: View<Repeated<$t>>, mut dst: Mut<Repeated<$t>>) {
                    unsafe { $copy_from_thunk(src.as_raw(Private), dst.as_raw(Private)) }
                }
            }
        )*
    };
    ($($t:ty),* $(,)?) => {
        paste!{
            impl_repeated_primitives!(@impl $(
                $t => [
                    [< __pb_rust_RepeatedField_ $t _new >],
                    [< __pb_rust_RepeatedField_ $t _free >],
                    [< __pb_rust_RepeatedField_ $t _add >],
                    [< __pb_rust_RepeatedField_ $t _size >],
                    [< __pb_rust_RepeatedField_ $t _get >],
                    [< __pb_rust_RepeatedField_ $t _set >],
                    [< __pb_rust_RepeatedField_ $t _clear >],
                    [< __pb_rust_RepeatedField_ $t _copy_from >],
                ],
            )*);
        }
    };
}

impl_repeated_primitives!(i32, u32, i64, u64, f32, f64, bool);

#[derive(Debug)]
pub struct MapInner<'msg, K: ?Sized, V: ?Sized> {
    pub raw: RawMap,
    pub _phantom_key: PhantomData<&'msg mut K>,
    pub _phantom_value: PhantomData<&'msg mut V>,
}

// These use manual impls instead of derives to avoid unnecessary bounds on `K`
// and `V`. This problem is referred to as "perfect derive".
// https://smallcultfollowing.com/babysteps/blog/2022/04/12/implied-bounds-and-perfect-derive/
impl<'msg, K: ?Sized, V: ?Sized> Copy for MapInner<'msg, K, V> {}
impl<'msg, K: ?Sized, V: ?Sized> Clone for MapInner<'msg, K, V> {
    fn clone(&self) -> MapInner<'msg, K, V> {
        *self
    }
}

macro_rules! impl_scalar_map_values {
    ($kt:ty, $trait:ident for $($t:ty),*) => {
        paste! { $(
            extern "C" {
                fn [< __pb_rust_Map_ $kt _ $t _new >]() -> RawMap;
                fn [< __pb_rust_Map_ $kt _ $t _clear >](m: RawMap);
                fn [< __pb_rust_Map_ $kt _ $t _size >](m: RawMap) -> usize;
                fn [< __pb_rust_Map_ $kt _ $t _insert >](m: RawMap, key: $kt, value: $t);
                fn [< __pb_rust_Map_ $kt _ $t _get >](m: RawMap, key: $kt, value: *mut $t) -> bool;
                fn [< __pb_rust_Map_ $kt _ $t _remove >](m: RawMap, key: $kt, value: *mut $t) -> bool;
            }
            impl $trait for $t {
                fn new_map() -> RawMap {
                    unsafe { [< __pb_rust_Map_ $kt _ $t _new >]() }
                }

                fn clear(m: RawMap) {
                    unsafe { [< __pb_rust_Map_ $kt _ $t _clear >](m) }
                }

                fn size(m: RawMap) -> usize {
                    unsafe { [< __pb_rust_Map_ $kt _ $t _size >](m) }
                }

                fn insert(m: RawMap, key: $kt, value: $t) {
                    unsafe { [< __pb_rust_Map_ $kt _ $t _insert >](m, key, value) }
                }

                fn get(m: RawMap, key: $kt) -> Option<$t> {
                    let mut val: $t = Default::default();
                    let found = unsafe { [< __pb_rust_Map_ $kt _ $t _get >](m, key, &mut val) };
                    if !found {
                        return None;
                    }
                    Some(val)
                }

                fn remove(m: RawMap, key: $kt) -> Option<$t> {
                    let mut val: $t = Default::default();
                    let removed =
                        unsafe { [< __pb_rust_Map_ $kt _ $t _remove >](m, key, &mut val) };
                    if !removed {
                        return None;
                    }
                    Some(val)
                }
            }
         )* }
    }
}

macro_rules! impl_scalar_maps {
    ($($t:ty),*) => {
        paste! { $(
                pub trait [< MapWith $t:camel KeyOps >] : Sync + Send + Copy + Clone + Debug {
                    fn new_map() -> RawMap;
                    fn clear(m: RawMap);
                    fn size(m: RawMap) -> usize;
                    fn insert(m: RawMap, key: $t, value: Self);
                    fn get(m: RawMap, key: $t) -> Option<Self>
                    where
                        Self: Sized;
                    fn remove(m: RawMap, key: $t) -> Option<Self>
                    where
                        Self: Sized;
                }

                impl_scalar_map_values!(
                    $t, [< MapWith $t:camel KeyOps >] for i32, u32, f32, f64, bool, u64, i64
                );

                impl<'msg, V: [< MapWith $t:camel KeyOps >]> Default for MapInner<'msg, $t, V> {
                    fn default() -> Self {
                        MapInner {
                            raw: V::new_map(),
                            _phantom_key: PhantomData,
                            _phantom_value: PhantomData
                        }
                    }
                }

                impl<'msg, V: [< MapWith $t:camel KeyOps >]> MapInner<'msg, $t, V> {
                    pub fn size(&self) -> usize {
                        V::size(self.raw)
                    }

                    pub fn clear(&mut self) {
                        V::clear(self.raw)
                    }

                    pub fn get(&self, key: $t) -> Option<V> {
                        V::get(self.raw, key)
                    }

                    pub fn remove(&mut self, key: $t) -> Option<V> {
                        V::remove(self.raw, key)
                    }

                    pub fn insert(&mut self, key: $t, value: V) -> bool {
                        V::insert(self.raw, key, value);
                        true
                    }
                }
        )* }
    }
}

impl_scalar_maps!(i32, u32, bool, u64, i64);

#[cfg(test)]
pub(crate) fn new_map_inner() -> MapInner<'static, i32, i64> {
    Default::default()
}

#[cfg(test)]
mod tests {
    use super::*;
    use googletest::prelude::*;
    use std::boxed::Box;

    // We need to allocate the byte array so SerializedData can own it and
    // deallocate it in its drop. This function makes it easier to do so for our
    // tests.
    fn allocate_byte_array(content: &'static [u8]) -> (*mut u8, usize) {
        let content: &mut [u8] = Box::leak(content.into());
        (content.as_mut_ptr(), content.len())
    }

    #[test]
    fn test_serialized_data_roundtrip() {
        let (ptr, len) = allocate_byte_array(b"Hello world");
        let serialized_data = SerializedData { data: NonNull::new(ptr).unwrap(), len };
        assert_that!(&*serialized_data, eq(b"Hello world"));
    }

    #[test]
    fn i32_i32_map() {
        let mut map: MapInner<'_, i32, i32> = Default::default();
        assert_that!(map.size(), eq(0));

        assert_that!(map.insert(1, 2), eq(true));
        assert_that!(map.get(1), eq(Some(2)));
        assert_that!(map.get(3), eq(None));
        assert_that!(map.size(), eq(1));

        assert_that!(map.remove(1), eq(Some(2)));
        assert_that!(map.size(), eq(0));
        assert_that!(map.remove(1), eq(None));

        assert_that!(map.insert(4, 5), eq(true));
        assert_that!(map.insert(6, 7), eq(true));
        map.clear();
        assert_that!(map.size(), eq(0));
    }

    #[test]
    fn i64_f64_map() {
        let mut map: MapInner<'_, i64, f64> = Default::default();
        assert_that!(map.size(), eq(0));

        assert_that!(map.insert(1, 2.5), eq(true));
        assert_that!(map.get(1), eq(Some(2.5)));
        assert_that!(map.get(3), eq(None));
        assert_that!(map.size(), eq(1));

        assert_that!(map.remove(1), eq(Some(2.5)));
        assert_that!(map.size(), eq(0));
        assert_that!(map.remove(1), eq(None));

        assert_that!(map.insert(4, 5.1), eq(true));
        assert_that!(map.insert(6, 7.2), eq(true));
        map.clear();
        assert_that!(map.size(), eq(0));
    }
}
