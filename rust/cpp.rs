// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Rust Protobuf runtime using the C++ kernel.

use crate::__internal::{Enum, Private, PtrAndLen, RawArena, RawMap, RawMessage, RawRepeatedField};
use crate::{
    Map, Mut, ProtoStr, Proxied, ProxiedInMapValue, ProxiedInRepeated, Repeated, RepeatedMut,
    RepeatedView, SettableValue, View,
};
use core::fmt::Debug;
use paste::paste;
use std::alloc::Layout;
use std::cell::UnsafeCell;
use std::convert::identity;
use std::ffi::c_int;
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

impl SettableValue<[u8]> for SerializedData {
    fn set_on<'msg>(self, _private: Private, mut mutator: Mut<'msg, [u8]>)
    where
        [u8]: 'msg,
    {
        mutator.set(self.as_ref())
    }
}

pub type MessagePresentMutData<'msg, T> = crate::vtable::RawVTableOptionalMutatorData<'msg, T>;
pub type MessageAbsentMutData<'msg, T> = crate::vtable::RawVTableOptionalMutatorData<'msg, T>;
pub type BytesPresentMutData<'msg> = crate::vtable::RawVTableOptionalMutatorData<'msg, [u8]>;
pub type BytesAbsentMutData<'msg> = crate::vtable::RawVTableOptionalMutatorData<'msg, [u8]>;
pub type InnerBytesMut<'msg> = crate::vtable::RawVTableMutator<'msg, [u8]>;
pub type InnerPrimitiveMut<'msg, T> = crate::vtable::RawVTableMutator<'msg, T>;

#[derive(Debug)]
pub struct MessageVTable {
    pub getter: unsafe extern "C" fn(msg: RawMessage) -> RawMessage,
    pub mut_getter: unsafe extern "C" fn(msg: RawMessage) -> RawMessage,
    pub clearer: unsafe extern "C" fn(msg: RawMessage),
}

impl MessageVTable {
    pub const fn new(
        _private: Private,
        getter: unsafe extern "C" fn(msg: RawMessage) -> RawMessage,
        mut_getter: unsafe extern "C" fn(msg: RawMessage) -> RawMessage,
        clearer: unsafe extern "C" fn(msg: RawMessage),
    ) -> Self {
        MessageVTable { getter, mut_getter, clearer }
    }
}

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
        _parent_msg: MutatorMessageRef<'msg>,
        message_field_ptr: RawMessage,
    ) -> Self {
        MutatorMessageRef { msg: message_field_ptr, _phantom: PhantomData }
    }

    pub fn msg(&self) -> RawMessage {
        self.msg
    }
}

pub fn copy_bytes_in_arena_if_needed_by_runtime<'msg>(
    _msg_ref: MutatorMessageRef<'msg>,
    val: &'msg [u8],
) -> &'msg [u8] {
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

trait CppTypeConversions: Proxied {
    type ElemType;

    fn elem_to_view<'msg>(v: Self::ElemType) -> View<'msg, Self>;
}

macro_rules! impl_cpp_type_conversions_for_scalars {
    ($($t:ty),* $(,)?) => {
        $(
            impl CppTypeConversions for $t {
                type ElemType = Self;

                fn elem_to_view<'msg>(v: Self) -> View<'msg, Self> {
                    v
                }
            }
        )*
    }
}

impl_cpp_type_conversions_for_scalars!(i32, u32, i64, u64, f32, f64, bool);

impl CppTypeConversions for ProtoStr {
    type ElemType = PtrAndLen;

    fn elem_to_view<'msg>(v: PtrAndLen) -> View<'msg, ProtoStr> {
        ptrlen_to_str(v)
    }
}

impl CppTypeConversions for [u8] {
    type ElemType = PtrAndLen;

    fn elem_to_view<'msg>(v: Self::ElemType) -> View<'msg, Self> {
        ptrlen_to_bytes(v)
    }
}

// This type alias is used so macros can generate valid extern "C" symbol names
// for functions working with [u8] types.
type Bytes = [u8];

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
                fn $add_thunk(f: RawRepeatedField, v: <$t as CppTypeConversions>::ElemType);
                fn $size_thunk(f: RawRepeatedField) -> usize;
                fn $get_thunk(
                    f: RawRepeatedField,
                    i: usize) -> <$t as CppTypeConversions>::ElemType;
                fn $set_thunk(
                    f: RawRepeatedField,
                    i: usize,
                    v: <$t as CppTypeConversions>::ElemType);
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
                    unsafe { $add_thunk(f.as_raw(Private), v.into()) }
                }
                fn repeated_clear(mut f: Mut<Repeated<$t>>) {
                    unsafe { $clear_thunk(f.as_raw(Private)) }
                }
                unsafe fn repeated_get_unchecked(f: View<Repeated<$t>>, i: usize) -> View<$t> {
                    <$t as CppTypeConversions>::elem_to_view(
                        unsafe { $get_thunk(f.as_raw(Private), i) })
                }
                unsafe fn repeated_set_unchecked(mut f: Mut<Repeated<$t>>, i: usize, v: View<$t>) {
                    unsafe { $set_thunk(f.as_raw(Private), i, v.into()) }
                }
                fn repeated_copy_from(src: View<Repeated<$t>>, mut dest: Mut<Repeated<$t>>) {
                    unsafe { $copy_from_thunk(src.as_raw(Private), dest.as_raw(Private)) }
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

impl_repeated_primitives!(i32, u32, i64, u64, f32, f64, bool, ProtoStr, Bytes);

/// Cast a `RepeatedView<SomeEnum>` to `RepeatedView<c_int>`.
pub fn cast_enum_repeated_view<E: Enum + ProxiedInRepeated>(
    private: Private,
    repeated: RepeatedView<E>,
) -> RepeatedView<c_int> {
    // SAFETY: the implementer of `Enum` has promised that this
    // raw repeated is a type-erased `proto2::RepeatedField<int>*`.
    unsafe { RepeatedView::from_raw(private, repeated.as_raw(Private)) }
}

/// Cast a `RepeatedMut<SomeEnum>` to `RepeatedMut<c_int>`.
///
/// Writing an unknown value is sound because all enums
/// are representationally open.
pub fn cast_enum_repeated_mut<E: Enum + ProxiedInRepeated>(
    private: Private,
    mut repeated: RepeatedMut<E>,
) -> RepeatedMut<c_int> {
    // SAFETY: the implementer of `Enum` has promised that this
    // raw repeated is a type-erased `proto2::RepeatedField<int>*`.
    unsafe {
        RepeatedMut::from_inner(
            private,
            InnerRepeatedMut { raw: repeated.as_raw(Private), _phantom: PhantomData },
        )
    }
}

#[derive(Clone, Copy, Debug)]
pub struct InnerMapMut<'msg> {
    pub(crate) raw: RawMap,
    _phantom: PhantomData<&'msg ()>,
}

impl<'msg> InnerMapMut<'msg> {
    pub fn new(_private: Private, raw: RawMap) -> Self {
        InnerMapMut { raw, _phantom: PhantomData }
    }
}

macro_rules! impl_ProxiedInMapValue_for_non_generated_value_types {
    ($key_t:ty, $ffi_key_t:ty, $to_ffi_key:expr, for $($t:ty, $ffi_t:ty, $to_ffi_value:expr, $from_ffi_value:expr, $zero_val:literal;)*) => {
        paste! { $(
            extern "C" {
                fn [< __rust_proto_thunk__Map_ $key_t _ $t _new >]() -> RawMap;
                fn [< __rust_proto_thunk__Map_ $key_t _ $t _free >](m: RawMap);
                fn [< __rust_proto_thunk__Map_ $key_t _ $t _clear >](m: RawMap);
                fn [< __rust_proto_thunk__Map_ $key_t _ $t _size >](m: RawMap) -> usize;
                fn [< __rust_proto_thunk__Map_ $key_t _ $t _insert >](m: RawMap, key: $ffi_key_t, value: $ffi_t) -> bool;
                fn [< __rust_proto_thunk__Map_ $key_t _ $t _get >](m: RawMap, key: $ffi_key_t, value: *mut $ffi_t) -> bool;
                fn [< __rust_proto_thunk__Map_ $key_t _ $t _remove >](m: RawMap, key: $ffi_key_t, value: *mut $ffi_t) -> bool;
            }

            impl ProxiedInMapValue<$key_t> for $t {
                fn map_new(_private: Private) -> Map<$key_t, Self> {
                    unsafe {
                        Map::from_inner(
                            Private,
                            InnerMapMut {
                                raw: [< __rust_proto_thunk__Map_ $key_t _ $t _new >](),
                                _phantom: PhantomData
                            }
                        )
                    }
                }

                unsafe fn map_free(_private: Private, map: &mut Map<$key_t, Self>) {
                    // SAFETY:
                    // - `map.inner.raw` is a live `RawMap`
                    // - This function is only called once for `map` in `Drop`.
                    unsafe { [< __rust_proto_thunk__Map_ $key_t _ $t _free >](map.inner.raw); }
                }


                fn map_clear(map: Mut<'_, Map<$key_t, Self>>) {
                    unsafe { [< __rust_proto_thunk__Map_ $key_t _ $t _clear >](map.inner.raw); }
                }

                fn map_len(map: View<'_, Map<$key_t, Self>>) -> usize {
                    unsafe { [< __rust_proto_thunk__Map_ $key_t _ $t _size >](map.raw) }
                }

                fn map_insert(map: Mut<'_, Map<$key_t, Self>>, key: View<'_, $key_t>, value: View<'_, Self>) -> bool {
                    let ffi_key = $to_ffi_key(key);
                    let ffi_value = $to_ffi_value(value);
                    unsafe { [< __rust_proto_thunk__Map_ $key_t _ $t _insert >](map.inner.raw, ffi_key, ffi_value) }
                }

                fn map_get<'a>(map: View<'a, Map<$key_t, Self>>, key: View<'_, $key_t>) -> Option<View<'a, Self>> {
                    let ffi_key = $to_ffi_key(key);
                    let mut ffi_value = $to_ffi_value($zero_val);
                    let found = unsafe { [< __rust_proto_thunk__Map_ $key_t _ $t _get >](map.raw, ffi_key, &mut ffi_value) };
                    if !found {
                        return None;
                    }
                    Some($from_ffi_value(ffi_value))
                }

                fn map_remove(map: Mut<'_, Map<$key_t, Self>>, key: View<'_, $key_t>) -> bool {
                    let ffi_key = $to_ffi_key(key);
                    let mut ffi_value = $to_ffi_value($zero_val);
                    unsafe { [< __rust_proto_thunk__Map_ $key_t _ $t _remove >](map.inner.raw, ffi_key, &mut ffi_value) }
                }
            }
         )* }
    }
}

fn str_to_ptrlen<'msg>(val: impl Into<&'msg ProtoStr>) -> PtrAndLen {
    val.into().as_bytes().into()
}

// Warning: this function is unsound on its own! `val.as_ref()` must be safe to
// call.
fn ptrlen_to_str<'msg>(val: PtrAndLen) -> &'msg ProtoStr {
    unsafe { ProtoStr::from_utf8_unchecked(val.as_ref()) }
}

fn bytes_to_ptrlen(val: &[u8]) -> PtrAndLen {
    val.into()
}

// Warning: this function is unsound on its own! `val.as_ref()` must be safe to
// call.
fn ptrlen_to_bytes<'msg>(val: PtrAndLen) -> &'msg [u8] {
    unsafe { val.as_ref() }
}

macro_rules! impl_ProxiedInMapValue_for_key_types {
    ($($t:ty, $ffi_t:ty, $to_ffi_key:expr;)*) => {
        paste! {
            $(
                impl_ProxiedInMapValue_for_non_generated_value_types!($t, $ffi_t, $to_ffi_key, for
                    f32, f32, identity, identity, 0f32;
                    f64, f64, identity, identity, 0f64;
                    i32, i32, identity, identity, 0i32;
                    u32, u32, identity, identity, 0u32;
                    i64, i64, identity, identity, 0i64;
                    u64, u64, identity, identity, 0u64;
                    bool, bool, identity, identity, false;
                    ProtoStr, PtrAndLen, str_to_ptrlen, ptrlen_to_str, "";
                    Bytes, PtrAndLen, bytes_to_ptrlen, ptrlen_to_bytes, b"";
                );
            )*
        }
    }
}

impl_ProxiedInMapValue_for_key_types!(
    i32, i32, identity;
    u32, u32, identity;
    i64, i64, identity;
    u64, u64, identity;
    bool, bool, identity;
    ProtoStr, PtrAndLen, str_to_ptrlen;
);

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
}
