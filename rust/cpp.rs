// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Rust Protobuf runtime using the C++ kernel.

use crate::ProtoStr;
use crate::__internal::{Private, PtrAndLen, RawArena, RawMap, RawMessage, RawRepeatedField};
use core::fmt::Debug;
use paste::paste;
use std::alloc::Layout;
use std::cell::UnsafeCell;
use std::convert::identity;
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

/// RepeatedField impls delegate out to `extern "C"` functions exposed by
/// `cpp_api.h` and store either a RepeatedField* or a RepeatedPtrField*
/// depending on the type.
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
#[derive(Debug)]
pub struct RepeatedField<'msg, T: ?Sized> {
    inner: RepeatedFieldInner<'msg>,
    _phantom: PhantomData<&'msg mut T>,
}

/// CPP runtime-specific arguments for initializing a RepeatedField.
/// See RepeatedField comment about mutation invariants for when this type can
/// be copied.
#[derive(Clone, Copy, Debug)]
pub struct RepeatedFieldInner<'msg> {
    pub raw: RawRepeatedField,
    pub _phantom: PhantomData<&'msg ()>,
}

impl<'msg, T: ?Sized> RepeatedField<'msg, T> {
    pub fn from_inner(_private: Private, inner: RepeatedFieldInner<'msg>) -> Self {
        RepeatedField { inner, _phantom: PhantomData }
    }
}

// These use manual impls instead of derives to avoid unnecessary bounds on `T`.
// This problem is referred to as "perfect derive".
// https://smallcultfollowing.com/babysteps/blog/2022/04/12/implied-bounds-and-perfect-derive/
impl<'msg, T: ?Sized> Copy for RepeatedField<'msg, T> {}
impl<'msg, T: ?Sized> Clone for RepeatedField<'msg, T> {
    fn clone(&self) -> RepeatedField<'msg, T> {
        *self
    }
}

pub trait RepeatedScalarOps {
    fn new_repeated_field() -> RawRepeatedField;
    fn push(f: RawRepeatedField, v: Self);
    fn len(f: RawRepeatedField) -> usize;
    fn get(f: RawRepeatedField, i: usize) -> Self;
    fn set(f: RawRepeatedField, i: usize, v: Self);
    fn copy_from(src: RawRepeatedField, dst: RawRepeatedField);
}

macro_rules! impl_repeated_scalar_ops {
    ($($t: ty),*) => {
        paste! { $(
            extern "C" {
                fn [< __pb_rust_RepeatedField_ $t _new >]() -> RawRepeatedField;
                fn [< __pb_rust_RepeatedField_ $t _add >](f: RawRepeatedField, v: $t);
                fn [< __pb_rust_RepeatedField_ $t _size >](f: RawRepeatedField) -> usize;
                fn [< __pb_rust_RepeatedField_ $t _get >](f: RawRepeatedField, i: usize) -> $t;
                fn [< __pb_rust_RepeatedField_ $t _set >](f: RawRepeatedField, i: usize, v: $t);
                fn [< __pb_rust_RepeatedField_ $t _copy_from >](src: RawRepeatedField, dst: RawRepeatedField);
            }
            impl RepeatedScalarOps for $t {
                fn new_repeated_field() -> RawRepeatedField {
                    unsafe { [< __pb_rust_RepeatedField_ $t _new >]() }
                }
                fn push(f: RawRepeatedField, v: Self) {
                    unsafe { [< __pb_rust_RepeatedField_ $t _add >](f, v) }
                }
                fn len(f: RawRepeatedField) -> usize {
                    unsafe { [< __pb_rust_RepeatedField_ $t _size >](f) }
                }
                fn get(f: RawRepeatedField, i: usize) -> Self {
                    unsafe { [< __pb_rust_RepeatedField_ $t _get >](f, i) }
                }
                fn set(f: RawRepeatedField, i: usize, v: Self) {
                    unsafe { [< __pb_rust_RepeatedField_ $t _set >](f, i, v) }
                }
                fn copy_from(src: RawRepeatedField, dst: RawRepeatedField) {
                    unsafe { [< __pb_rust_RepeatedField_ $t _copy_from >](src, dst) }
                }
            }
        )* }
    };
}

impl_repeated_scalar_ops!(i32, u32, i64, u64, f32, f64, bool);

impl<'msg, T: RepeatedScalarOps> RepeatedField<'msg, T> {
    #[allow(clippy::new_without_default, dead_code)]
    /// new() is not currently used in our normal pathways, it is only used
    /// for testing. Existing `RepeatedField<>`s are owned by, and retrieved
    /// from, the containing `Message`.
    pub fn new() -> Self {
        Self::from_inner(
            Private,
            RepeatedFieldInner::<'msg> { raw: T::new_repeated_field(), _phantom: PhantomData },
        )
    }
    pub fn push(&mut self, val: T) {
        T::push(self.inner.raw, val)
    }
    pub fn len(&self) -> usize {
        T::len(self.inner.raw)
    }
    pub fn is_empty(&self) -> bool {
        self.len() == 0
    }
    pub fn get(&self, index: usize) -> Option<T> {
        if index >= self.len() {
            return None;
        }
        Some(T::get(self.inner.raw, index))
    }
    pub fn set(&mut self, index: usize, val: T) {
        if index >= self.len() {
            return;
        }
        T::set(self.inner.raw, index, val)
    }
    pub fn copy_from(&mut self, src: &RepeatedField<'_, T>) {
        T::copy_from(src.inner.raw, self.inner.raw)
    }
}

#[derive(Debug)]
pub struct MapInner<'msg, K: ?Sized, V: ?Sized> {
    pub raw: RawMap,
    pub _phantom_key: PhantomData<&'msg mut K>,
    pub _phantom_value: PhantomData<&'msg mut V>,
}

impl<'msg, K: ?Sized, V: ?Sized> Copy for MapInner<'msg, K, V> {}
impl<'msg, K: ?Sized, V: ?Sized> Clone for MapInner<'msg, K, V> {
    fn clone(&self) -> MapInner<'msg, K, V> {
        *self
    }
}

macro_rules! generate_map_with_key_ops_traits {
    ($($t:ty, $sized_t:ty;)*) => {
        paste! {
            $(
                pub trait [< MapWith $t:camel KeyOps >] {
                    type Value<'a>: Sized;

                    fn new_map() -> RawMap;
                    fn clear(m: RawMap);
                    fn size(m: RawMap) -> usize;
                    fn insert(m: RawMap, key: $sized_t, value: Self::Value<'_>) -> bool;
                    fn get<'a>(m: RawMap, key: $sized_t) -> Option<Self::Value<'a>>;
                    fn remove<'a>(m: RawMap, key: $sized_t) -> bool;
                }

                impl<'msg, V: [< MapWith $t:camel KeyOps >] + ?Sized> Default for MapInner<'msg, $t, V> {
                    fn default() -> Self {
                        MapInner {
                            raw: V::new_map(),
                            _phantom_key: PhantomData,
                            _phantom_value: PhantomData
                        }
                    }
                }

                impl<'msg, V: [< MapWith $t:camel KeyOps >] + ?Sized> MapInner<'msg, $t, V> {
                    pub fn size(&self) -> usize {
                        V::size(self.raw)
                    }

                    pub fn clear(&mut self) {
                        V::clear(self.raw)
                    }

                    pub fn get<'a>(&self, key: $sized_t) -> Option<V::Value<'a>> {
                        V::get(self.raw, key)
                    }

                    pub fn remove<'a>(&mut self, key: $sized_t) -> bool {
                        V::remove(self.raw, key)
                    }

                    pub fn insert(&mut self, key: $sized_t, value: V::Value<'_>) -> bool {
                        V::insert(self.raw, key, value);
                        true
                    }
                }
            )*
        }
    }
}

generate_map_with_key_ops_traits!(
    i32, i32;
    u32, u32;
    i64, i64;
    u64, u64;
    bool, bool;
    ProtoStr, &ProtoStr;
);

macro_rules! impl_scalar_map_with_key_op_for_scalar_values {
    ($key_t:ty, $sized_key_t:ty, $ffi_key_t:ty, $to_ffi_key:expr, $trait:ident for $($t:ty, $sized_t:ty, $ffi_t:ty, $to_ffi_value:expr, $from_ffi_value:expr, $zero_val:literal;)*) => {
        paste! { $(
            extern "C" {
                fn [< __pb_rust_Map_ $key_t _ $t _new >]() -> RawMap;
                fn [< __pb_rust_Map_ $key_t _ $t _clear >](m: RawMap);
                fn [< __pb_rust_Map_ $key_t _ $t _size >](m: RawMap) -> usize;
                fn [< __pb_rust_Map_ $key_t _ $t _insert >](m: RawMap, key: $ffi_key_t, value: $ffi_t);
                fn [< __pb_rust_Map_ $key_t _ $t _get >](m: RawMap, key: $ffi_key_t, value: *mut $ffi_t) -> bool;
                fn [< __pb_rust_Map_ $key_t _ $t _remove >](m: RawMap, key: $ffi_key_t, value: *mut $ffi_t) -> bool;
            }
            impl $trait for $t {
                type Value<'a> = $sized_t;

                fn new_map() -> RawMap {
                    unsafe { [< __pb_rust_Map_ $key_t _ $t _new >]() }
                }

                fn clear(m: RawMap) {
                    unsafe { [< __pb_rust_Map_ $key_t _ $t _clear >](m) }
                }

                fn size(m: RawMap) -> usize {
                    unsafe { [< __pb_rust_Map_ $key_t _ $t _size >](m) }
                }

                fn insert(m: RawMap, key: $sized_key_t, value: Self::Value<'_>) -> bool {
                    let ffi_key = $to_ffi_key(key);
                    let ffi_value = $to_ffi_value(value);
                    unsafe { [< __pb_rust_Map_ $key_t _ $t _insert >](m, ffi_key, ffi_value) }
                    true
                }

                fn get<'a>(m: RawMap, key: $sized_key_t) -> Option<Self::Value<'a>> {
                    let ffi_key = $to_ffi_key(key);
                    let mut ffi_value = $to_ffi_value($zero_val);
                    let found = unsafe { [< __pb_rust_Map_ $key_t _ $t _get >](m, ffi_key, &mut ffi_value) };
                    if !found {
                        return None;
                    }
                    Some($from_ffi_value(ffi_value))
                }

                fn remove<'a>(m: RawMap, key: $sized_key_t) -> bool {
                    let ffi_key = $to_ffi_key(key);
                    let mut ffi_value = $to_ffi_value($zero_val);
                    unsafe { [< __pb_rust_Map_ $key_t _ $t _remove >](m, ffi_key, &mut ffi_value) }
                }
            }
         )* }
    }
}

fn str_to_ptrlen<'a>(val: impl Into<&'a ProtoStr>) -> PtrAndLen {
    val.into().as_bytes().into()
}

fn ptrlen_to_str<'a>(val: PtrAndLen) -> &'a ProtoStr {
    unsafe { ProtoStr::from_utf8_unchecked(val.as_ref()) }
}

macro_rules! impl_map_with_key_ops_for_scalar_values {
    ($($t:ty, $t_sized:ty, $ffi_t:ty, $to_ffi_key:expr;)*) => {
        paste! {
            $(
                impl_scalar_map_with_key_op_for_scalar_values!($t, $t_sized, $ffi_t, $to_ffi_key, [< MapWith $t:camel KeyOps >] for
                    f32, f32, f32, identity, identity, 0f32;
                    f64, f64, f64, identity, identity, 0f64;
                    i32, i32, i32, identity, identity, 0i32;
                    u32, u32, u32, identity, identity, 0u32;
                    i64, i64, i64, identity, identity, 0i64;
                    u64, u64, u64, identity, identity, 0u64;
                    bool, bool, bool, identity, identity, false;
                    ProtoStr, &'a ProtoStr, PtrAndLen, str_to_ptrlen, ptrlen_to_str, "";
                );
            )*
        }
    }
}

impl_map_with_key_ops_for_scalar_values!(
    i32, i32, i32, identity;
    u32, u32, u32, identity;
    i64, i64, i64, identity;
    u64, u64, u64, identity;
    bool, bool, bool, identity;
    ProtoStr, &ProtoStr, PtrAndLen, str_to_ptrlen;
);

#[cfg(test)]
pub(crate) fn new_map_i32_i64() -> MapInner<'static, i32, i64> {
    Default::default()
}

#[cfg(test)]
pub(crate) fn new_map_str_str() -> MapInner<'static, ProtoStr, ProtoStr> {
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
    fn repeated_field() {
        let mut r = RepeatedField::<i32>::new();
        assert_that!(r.len(), eq(0));
        r.push(32);
        assert_that!(r.get(0), eq(Some(32)));

        let mut r = RepeatedField::<u32>::new();
        assert_that!(r.len(), eq(0));
        r.push(32);
        assert_that!(r.get(0), eq(Some(32)));

        let mut r = RepeatedField::<f64>::new();
        assert_that!(r.len(), eq(0));
        r.push(0.1234f64);
        assert_that!(r.get(0), eq(Some(0.1234)));

        let mut r = RepeatedField::<bool>::new();
        assert_that!(r.len(), eq(0));
        r.push(true);
        assert_that!(r.get(0), eq(Some(true)));
    }

    #[test]
    fn i32_i32_map() {
        let mut map: MapInner<'_, i32, i32> = Default::default();
        assert_that!(map.size(), eq(0));

        assert_that!(map.insert(1, 2), eq(true));
        assert_that!(map.get(1), eq(Some(2)));
        assert_that!(map.get(3), eq(None));
        assert_that!(map.size(), eq(1));

        assert_that!(map.remove(1), eq(true));
        assert_that!(map.size(), eq(0));
        assert_that!(map.remove(1), eq(false));

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

        assert_that!(map.remove(1), eq(true));
        assert_that!(map.size(), eq(0));
        assert_that!(map.remove(1), eq(false));

        assert_that!(map.insert(4, 5.1), eq(true));
        assert_that!(map.insert(6, 7.2), eq(true));
        map.clear();
        assert_that!(map.size(), eq(0));
    }

    #[test]
    fn str_str_map() {
        let mut map = MapInner::<'_, ProtoStr, ProtoStr>::default();
        assert_that!(map.size(), eq(0));

        map.insert("fizz".into(), "buzz".into());
        assert_that!(map.size(), eq(1));
        assert_that!(map.remove("fizz".into()), eq(true));
        map.clear();
        assert_that!(map.size(), eq(0));
    }

    #[test]
    fn u64_str_map() {
        let mut map = MapInner::<'_, u64, ProtoStr>::default();
        assert_that!(map.size(), eq(0));

        map.insert(1, "fizz".into());
        map.insert(2, "buzz".into());
        assert_that!(map.size(), eq(2));
        assert_that!(map.remove(1), eq(true));
        assert_that!(map.get(1), eq(None));
        map.clear();
        assert_that!(map.size(), eq(0));
    }

    #[test]
    fn test_all_maps_can_be_constructed() {
        macro_rules! gen_proto_values {
            ($key_t:ty, $($value_t:ty),*) => {
                $(
                    let map = MapInner::<'_, $key_t, $value_t>::default();
                    assert_that!(map.size(), eq(0));
                )*
            }
        }

        macro_rules! gen_proto_keys {
            ($($key_t:ty),*) => {
                $(
                    gen_proto_values!($key_t, f32, f64, i32, u32, i64, bool, ProtoStr);
                )*
            }
        }

        gen_proto_keys!(i32, u32, i64, u64, bool, ProtoStr);
    }
}
