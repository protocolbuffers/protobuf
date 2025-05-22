// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Rust Protobuf runtime using the C++ kernel.

use crate::__internal::{Enum, MatcherEq, Private};
use crate::{
    AsView, IntoProxied, Map, MapIter, MapMut, MapView, Message, MessageViewInterop, Mut,
    ProtoBytes, ProtoStr, ProtoString, Proxied, ProxiedInMapValue, ProxiedInRepeated, Repeated,
    RepeatedMut, RepeatedView, View,
};
use core::fmt::Debug;
use paste::paste;
use std::convert::identity;
use std::ffi::{c_int, c_void};
use std::fmt;
use std::marker::PhantomData;
use std::mem::{ManuallyDrop, MaybeUninit};
use std::ops::Deref;
use std::ptr::{self, NonNull};
use std::slice;

/// Defines a set of opaque, unique, non-accessible pointees.
///
/// The [Rustonomicon][nomicon] currently recommends a zero-sized struct,
/// though this should use [`extern type`] when that is stabilized.
/// [nomicon]: https://doc.rust-lang.org/nomicon/ffi.html#representing-opaque-structs
/// [`extern type`]: https://github.com/rust-lang/rust/issues/43467
#[doc(hidden)]
mod _opaque_pointees {
    /// Opaque pointee for [`RawMessage`]
    ///
    /// This type is not meant to be dereferenced in Rust code.
    /// It is only meant to provide type safety for raw pointers
    /// which are manipulated behind FFI.
    ///
    /// [`RawMessage`]: super::RawMessage
    #[repr(C)]
    pub struct RawMessageData {
        _data: [u8; 0],
        _marker: std::marker::PhantomData<(*mut u8, ::std::marker::PhantomPinned)>,
    }

    /// Opaque pointee for [`RawRepeatedField`]
    ///
    /// This type is not meant to be dereferenced in Rust code.
    /// It is only meant to provide type safety for raw pointers
    /// which are manipulated behind FFI.
    #[repr(C)]
    pub struct RawRepeatedFieldData {
        _data: [u8; 0],
        _marker: std::marker::PhantomData<(*mut u8, ::std::marker::PhantomPinned)>,
    }

    /// Opaque pointee for [`RawMap`]
    ///
    /// This type is not meant to be dereferenced in Rust code.
    /// It is only meant to provide type safety for raw pointers
    /// which are manipulated behind FFI.
    #[repr(C)]
    pub struct RawMapData {
        _data: [u8; 0],
        _marker: std::marker::PhantomData<(*mut u8, ::std::marker::PhantomPinned)>,
    }

    /// Opaque pointee for [`CppStdString`]
    ///
    /// This type is not meant to be dereferenced in Rust code.
    /// It is only meant to provide type safety for raw pointers
    /// which are manipulated behind FFI.
    #[repr(C)]
    pub struct CppStdStringData {
        _data: [u8; 0],
        _marker: std::marker::PhantomData<(*mut u8, ::std::marker::PhantomPinned)>,
    }
}

/// A raw pointer to the underlying message for this runtime.
#[doc(hidden)]
pub type RawMessage = NonNull<_opaque_pointees::RawMessageData>;

/// A raw pointer to the underlying repeated field container for this runtime.
#[doc(hidden)]
pub type RawRepeatedField = NonNull<_opaque_pointees::RawRepeatedFieldData>;

/// A raw pointer to the underlying arena for this runtime.
#[doc(hidden)]
pub type RawMap = NonNull<_opaque_pointees::RawMapData>;

/// A raw pointer to a std::string.
#[doc(hidden)]
pub type CppStdString = NonNull<_opaque_pointees::CppStdStringData>;

/// Kernel-specific owned `string` and `bytes` field type.
#[derive(Debug)]
#[doc(hidden)]
pub struct InnerProtoString {
    owned_ptr: CppStdString,
}

extern "C" {
    pub fn proto2_rust_Message_delete(m: RawMessage);
    pub fn proto2_rust_Message_clear(m: RawMessage);
    pub fn proto2_rust_Message_parse(m: RawMessage, input: SerializedData) -> bool;
    pub fn proto2_rust_Message_parse_dont_enforce_required(
        m: RawMessage,
        input: SerializedData,
    ) -> bool;
    pub fn proto2_rust_Message_serialize(m: RawMessage, output: &mut SerializedData) -> bool;
    pub fn proto2_rust_Message_copy_from(dst: RawMessage, src: RawMessage) -> bool;
    pub fn proto2_rust_Message_merge_from(dst: RawMessage, src: RawMessage) -> bool;
}

impl Drop for InnerProtoString {
    fn drop(&mut self) {
        // SAFETY: `self.owned_ptr` points to a valid std::string object.
        unsafe {
            proto2_rust_cpp_delete_string(self.owned_ptr);
        }
    }
}

impl InnerProtoString {
    pub(crate) fn as_bytes(&self) -> &[u8] {
        // SAFETY: `self.owned_ptr` points to a valid std::string object.
        unsafe { proto2_rust_cpp_string_to_view(self.owned_ptr).as_ref() }
    }

    pub fn into_raw(self) -> CppStdString {
        let s = ManuallyDrop::new(self);
        s.owned_ptr
    }

    /// # Safety
    ///  - `src` points to a valid CppStdString.
    pub unsafe fn from_raw(src: CppStdString) -> InnerProtoString {
        InnerProtoString { owned_ptr: src }
    }
}

impl From<&[u8]> for InnerProtoString {
    fn from(val: &[u8]) -> Self {
        // SAFETY: `val` is valid byte slice.
        let owned_ptr: CppStdString = unsafe { proto2_rust_cpp_new_string(val.into()) };
        InnerProtoString { owned_ptr }
    }
}

extern "C" {
    fn proto2_rust_cpp_new_string(src: PtrAndLen) -> CppStdString;
    fn proto2_rust_cpp_delete_string(src: CppStdString);
    fn proto2_rust_cpp_string_to_view(src: CppStdString) -> PtrAndLen;
}

/// Represents an ABI-stable version of `NonNull<[u8]>`/`string_view` (a
/// borrowed slice of bytes) for FFI use only.
///
/// Has semantics similar to `std::string_view` in C++ and `&[u8]` in Rust,
/// but is not ABI-compatible with either.
///
/// If `len` is 0, then `ptr` can be null or dangling. C++ considers a dangling
/// 0-len `std::string_view` to be invalid, and Rust considers a `&[u8]` with a
/// null data pointer to be invalid.
#[repr(C)]
#[derive(Copy, Clone)]
#[doc(hidden)]
pub struct PtrAndLen {
    /// Pointer to the first byte.
    /// Borrows the memory.
    pub ptr: *const u8,

    /// Length of the `[u8]` pointed to by `ptr`.
    pub len: usize,
}

impl PtrAndLen {
    /// Unsafely dereference this slice.
    ///
    /// # Safety
    /// - `self.ptr` must be dereferencable and immutable for `self.len` bytes
    ///   for the lifetime `'a`. It can be null or dangling if `self.len == 0`.
    pub unsafe fn as_ref<'a>(self) -> &'a [u8] {
        if self.ptr.is_null() {
            assert_eq!(self.len, 0, "Non-empty slice with null data pointer");
            &[]
        } else {
            // SAFETY:
            // - `ptr` is non-null
            // - `ptr` is valid for `len` bytes as promised by the caller.
            unsafe { slice::from_raw_parts(self.ptr, self.len) }
        }
    }
}

impl From<&[u8]> for PtrAndLen {
    fn from(slice: &[u8]) -> Self {
        Self { ptr: slice.as_ptr(), len: slice.len() }
    }
}

impl From<&ProtoStr> for PtrAndLen {
    fn from(s: &ProtoStr) -> Self {
        let bytes = s.as_bytes();
        Self { ptr: bytes.as_ptr(), len: bytes.len() }
    }
}

/// Serialized Protobuf wire format data. It's typically produced by
/// `<Message>.serialize()`.
///
/// This struct is ABI-compatible with the equivalent struct on the C++ side. It
/// owns (and drops) its data.
#[repr(C)]
#[doc(hidden)]
pub struct SerializedData {
    /// Owns the memory.
    data: NonNull<u8>,
    len: usize,
}

impl SerializedData {
    pub fn new() -> Self {
        Self { data: NonNull::dangling(), len: 0 }
    }

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

    /// Converts into a Vec<u8>.
    pub fn into_vec(self) -> Vec<u8> {
        // We need to prevent self from being dropped, because we are going to transfer
        // ownership of self.data to the Vec<u8>.
        let s = ManuallyDrop::new(self);

        unsafe {
            // SAFETY:
            // - `data` was allocated by the Rust global allocator.
            // - `data` was allocated with an alignment of 1 for u8.
            // - The allocated size was `len`.
            // - The length and capacity are equal.
            // - All `len` bytes are initialized.
            // - The capacity (`len` in this case) is the size the pointer was allocated
            //   with.
            // - The allocated size is no more than isize::MAX, because the protobuf
            //   serializer will refuse to serialize a message if the output would exceed
            //   2^31 - 1 bytes.
            Vec::<u8>::from_raw_parts(s.data.as_ptr(), s.len, s.len)
        }
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

/// A type to transfer an owned Rust string across the FFI boundary:
///   * This struct is ABI-compatible with the equivalent C struct.
///   * It owns its data but does not drop it. Immediately turn it into a
///     `String` by calling `.into()` on it.
///   * `.data` points to a valid UTF-8 string that has been allocated with the
///     Rust allocator and is 1-byte aligned.
///   * `.data` contains exactly `.len` bytes.
///   * The empty string is represented as `.data.is_null() == true`.
#[repr(C)]
#[doc(hidden)]
pub struct RustStringRawParts {
    data: *const u8,
    len: usize,
}

impl From<RustStringRawParts> for String {
    fn from(value: RustStringRawParts) -> Self {
        if value.data.is_null() {
            // Handle the case where the string is empty.
            return String::new();
        }
        // SAFETY:
        //  - `value.data` contains valid UTF-8 bytes as promised by
        //    `RustStringRawParts`.
        //  - `value.data` has been allocated with the Rust allocator and is 1-byte
        //    aligned as promised by `RustStringRawParts`.
        //  - `value.data` contains and is allocated for exactly `value.len` bytes.
        unsafe { String::from_raw_parts(value.data as *mut u8, value.len, value.len) }
    }
}

extern "C" {
    fn proto2_rust_utf8_debug_string(msg: RawMessage) -> RustStringRawParts;
}

pub fn debug_string(msg: RawMessage, f: &mut fmt::Formatter<'_>) -> fmt::Result {
    // SAFETY:
    // - `msg` is a valid protobuf message.
    let dbg_str: String = unsafe { proto2_rust_utf8_debug_string(msg) }.into();
    write!(f, "{dbg_str}")
}

extern "C" {
    /// # Safety
    /// - `msg1` and `msg2` legally dereferencable MessageLite* pointers.
    #[link_name = "proto2_rust_messagelite_equals"]
    pub fn raw_message_equals(msg1: RawMessage, msg2: RawMessage) -> bool;
}

pub type RawMapIter = UntypedMapIterator;

/// The raw contents of every generated message.
#[derive(Debug)]
#[doc(hidden)]
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
#[doc(hidden)]
pub struct MutatorMessageRef<'msg> {
    msg: RawMessage,
    _phantom: PhantomData<&'msg mut ()>,
}
impl<'msg> MutatorMessageRef<'msg> {
    #[allow(clippy::needless_pass_by_ref_mut)] // Sound construction requires mutable access.
    pub fn new(msg: &'msg mut MessageInner) -> Self {
        MutatorMessageRef { msg: msg.msg, _phantom: PhantomData }
    }

    /// # Safety
    /// - The underlying pointer must be sound and live for the lifetime 'msg.
    pub unsafe fn wrap_raw(raw: RawMessage) -> Self {
        MutatorMessageRef { msg: raw, _phantom: PhantomData }
    }

    pub fn from_parent(
        _parent_msg: MutatorMessageRef<'msg>,
        message_field_ptr: RawMessage,
    ) -> Self {
        Self { msg: message_field_ptr, _phantom: PhantomData }
    }

    pub fn msg(&self) -> RawMessage {
        self.msg
    }

    pub fn from_raw_msg(msg: &RawMessage) -> Self {
        Self { msg: *msg, _phantom: PhantomData }
    }
}

/// The raw type-erased version of an owned `Repeated`.
#[derive(Debug)]
#[doc(hidden)]
pub struct InnerRepeated {
    raw: RawRepeatedField,
}

impl InnerRepeated {
    pub fn as_mut(&mut self) -> InnerRepeatedMut<'_> {
        InnerRepeatedMut::new(self.raw)
    }

    pub fn raw(&self) -> RawRepeatedField {
        self.raw
    }

    /// # Safety
    /// - `raw` must be a valid `proto2::RepeatedField*` or
    ///   `proto2::RepeatedPtrField*`.
    pub unsafe fn from_raw(raw: RawRepeatedField) -> Self {
        Self { raw }
    }
}

/// The raw type-erased pointer version of `RepeatedMut`.
///
/// Contains a `proto2::RepeatedField*` or `proto2::RepeatedPtrField*`.
#[derive(Clone, Copy, Debug)]
#[doc(hidden)]
pub struct InnerRepeatedMut<'msg> {
    pub(crate) raw: RawRepeatedField,
    _phantom: PhantomData<&'msg ()>,
}

impl<'msg> InnerRepeatedMut<'msg> {
    #[doc(hidden)]
    pub fn new(raw: RawRepeatedField) -> Self {
        InnerRepeatedMut { raw, _phantom: PhantomData }
    }
}

trait CppTypeConversions: Proxied {
    type InsertElemType;
    type ElemType;

    fn elem_to_view<'msg>(v: Self::ElemType) -> View<'msg, Self>;
    fn into_insertelem(v: Self) -> Self::InsertElemType;
}

macro_rules! impl_cpp_type_conversions_for_scalars {
    ($($t:ty),* $(,)?) => {
        $(
            impl CppTypeConversions for $t {
                type InsertElemType = Self;
                type ElemType = Self;

                fn elem_to_view<'msg>(v: Self) -> View<'msg, Self> {
                    v
                }

                fn into_insertelem(v: Self) -> Self {
                    v
                }
            }
        )*
    }
}

impl_cpp_type_conversions_for_scalars!(i32, u32, i64, u64, f32, f64, bool);

impl CppTypeConversions for ProtoString {
    type InsertElemType = CppStdString;
    type ElemType = PtrAndLen;

    fn elem_to_view<'msg>(v: PtrAndLen) -> View<'msg, ProtoString> {
        ptrlen_to_str(v)
    }

    fn into_insertelem(v: Self) -> CppStdString {
        v.into_inner(Private).into_raw()
    }
}

impl CppTypeConversions for ProtoBytes {
    type InsertElemType = CppStdString;
    type ElemType = PtrAndLen;

    fn elem_to_view<'msg>(v: Self::ElemType) -> View<'msg, Self> {
        ptrlen_to_bytes(v)
    }

    fn into_insertelem(v: Self) -> CppStdString {
        v.into_inner(Private).into_raw()
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
        $copy_from_thunk:ident,
        $reserve_thunk:ident $(,)?
    ]),* $(,)?) => {
        $(
            extern "C" {
                fn $new_thunk() -> RawRepeatedField;
                fn $free_thunk(f: RawRepeatedField);
                fn $add_thunk(f: RawRepeatedField, v: <$t as CppTypeConversions>::InsertElemType);
                fn $size_thunk(f: RawRepeatedField) -> usize;
                fn $get_thunk(
                    f: RawRepeatedField,
                    i: usize) -> <$t as CppTypeConversions>::ElemType;
                fn $set_thunk(
                    f: RawRepeatedField,
                    i: usize,
                    v: <$t as CppTypeConversions>::InsertElemType);
                fn $clear_thunk(f: RawRepeatedField);
                fn $copy_from_thunk(src: RawRepeatedField, dst: RawRepeatedField);
                fn $reserve_thunk(
                    f: RawRepeatedField,
                    additional: usize);
            }

            unsafe impl ProxiedInRepeated for $t {
                #[allow(dead_code)]
                #[inline]
                fn repeated_new(_: Private) -> Repeated<$t> {
                    Repeated::from_inner(Private, InnerRepeated {
                        raw: unsafe { $new_thunk() }
                    })
                }
                #[allow(dead_code)]
                #[inline]
                unsafe fn repeated_free(_: Private, f: &mut Repeated<$t>) {
                    unsafe { $free_thunk(f.as_mut().as_raw(Private)) }
                }
                #[inline]
                fn repeated_len(f: View<Repeated<$t>>) -> usize {
                    unsafe { $size_thunk(f.as_raw(Private)) }
                }
                #[inline]
                fn repeated_push(mut f: Mut<Repeated<$t>>, v: impl IntoProxied<$t>) {
                    unsafe { $add_thunk(f.as_raw(Private), <$t as CppTypeConversions>::into_insertelem(v.into_proxied(Private))) }
                }
                #[inline]
                fn repeated_clear(mut f: Mut<Repeated<$t>>) {
                    unsafe { $clear_thunk(f.as_raw(Private)) }
                }
                #[inline]
                unsafe fn repeated_get_unchecked(f: View<Repeated<$t>>, i: usize) -> View<$t> {
                    <$t as CppTypeConversions>::elem_to_view(
                        unsafe { $get_thunk(f.as_raw(Private), i) })
                }
                #[inline]
                unsafe fn repeated_set_unchecked(mut f: Mut<Repeated<$t>>, i: usize, v: impl IntoProxied<$t>) {
                    unsafe { $set_thunk(f.as_raw(Private), i, <$t as CppTypeConversions>::into_insertelem(v.into_proxied(Private))) }
                }
                #[inline]
                fn repeated_copy_from(src: View<Repeated<$t>>, mut dest: Mut<Repeated<$t>>) {
                    unsafe { $copy_from_thunk(src.as_raw(Private), dest.as_raw(Private)) }
                }
                #[inline]
                fn repeated_reserve(mut f: Mut<Repeated<$t>>, additional: usize) {
                    unsafe { $reserve_thunk(f.as_raw(Private), additional) }
                }
            }
        )*
    };
    ($($t:ty),* $(,)?) => {
        paste!{
            impl_repeated_primitives!(@impl $(
                $t => [
                    [< proto2_rust_RepeatedField_ $t _new >],
                    [< proto2_rust_RepeatedField_ $t _free >],
                    [< proto2_rust_RepeatedField_ $t _add >],
                    [< proto2_rust_RepeatedField_ $t _size >],
                    [< proto2_rust_RepeatedField_ $t _get >],
                    [< proto2_rust_RepeatedField_ $t _set >],
                    [< proto2_rust_RepeatedField_ $t _clear >],
                    [< proto2_rust_RepeatedField_ $t _copy_from >],
                    [< proto2_rust_RepeatedField_ $t _reserve >],
                ],
            )*);
        }
    };
}

impl_repeated_primitives!(i32, u32, i64, u64, f32, f64, bool, ProtoString, ProtoBytes);

extern "C" {
    pub fn proto2_rust_RepeatedField_Message_new() -> RawRepeatedField;
    pub fn proto2_rust_RepeatedField_Message_free(field: RawRepeatedField);
    pub fn proto2_rust_RepeatedField_Message_size(field: RawRepeatedField) -> usize;
    pub fn proto2_rust_RepeatedField_Message_get(
        field: RawRepeatedField,
        index: usize,
    ) -> RawMessage;
    pub fn proto2_rust_RepeatedField_Message_get_mut(
        field: RawRepeatedField,
        index: usize,
    ) -> RawMessage;
    pub fn proto2_rust_RepeatedField_Message_add(
        field: RawRepeatedField,
        prototype: RawMessage,
    ) -> RawMessage;
    pub fn proto2_rust_RepeatedField_Message_clear(field: RawRepeatedField);
    pub fn proto2_rust_RepeatedField_Message_copy_from(
        dst: RawRepeatedField,
        src: RawRepeatedField,
    );
    pub fn proto2_rust_RepeatedField_Message_reserve(field: RawRepeatedField, additional: usize);
}

/// Cast a `RepeatedView<SomeEnum>` to `RepeatedView<c_int>`.
pub fn cast_enum_repeated_view<E: Enum + ProxiedInRepeated>(
    repeated: RepeatedView<E>,
) -> RepeatedView<c_int> {
    // SAFETY: the implementer of `Enum` has promised that this
    // raw repeated is a type-erased `proto2::RepeatedField<int>*`.
    unsafe { RepeatedView::from_raw(Private, repeated.as_raw(Private)) }
}

/// Cast a `RepeatedMut<SomeEnum>` to `RepeatedMut<c_int>`.
///
/// Writing an unknown value is sound because all enums
/// are representationally open.
pub fn cast_enum_repeated_mut<E: Enum + ProxiedInRepeated>(
    mut repeated: RepeatedMut<E>,
) -> RepeatedMut<c_int> {
    // SAFETY: the implementer of `Enum` has promised that this
    // raw repeated is a type-erased `proto2::RepeatedField<int>*`.
    unsafe {
        RepeatedMut::from_inner(
            Private,
            InnerRepeatedMut { raw: repeated.as_raw(Private), _phantom: PhantomData },
        )
    }
}

/// Cast a `RepeatedMut<SomeEnum>` to `RepeatedMut<c_int>` and call
/// repeated_reserve.
pub fn reserve_enum_repeated_mut<E: Enum + ProxiedInRepeated>(
    repeated: RepeatedMut<E>,
    additional: usize,
) {
    let int_repeated = cast_enum_repeated_mut(repeated);
    ProxiedInRepeated::repeated_reserve(int_repeated, additional);
}

pub fn new_enum_repeated<E: Enum + ProxiedInRepeated>() -> Repeated<E> {
    let int_repeated = Repeated::<c_int>::new();
    let raw = int_repeated.inner.raw();
    std::mem::forget(int_repeated);
    unsafe { Repeated::from_inner(Private, InnerRepeated::from_raw(raw)) }
}

/// Cast a `RepeatedMut<SomeEnum>` to `RepeatedMut<c_int>` and call
/// repeated_free.
/// # Safety
/// - The passed in `&mut Repeated<E>` must not be used after this function is
///   called.
pub unsafe fn free_enum_repeated<E: Enum + ProxiedInRepeated>(repeated: &mut Repeated<E>) {
    unsafe {
        let mut int_r: Repeated<c_int> =
            Repeated::from_inner(Private, InnerRepeated::from_raw(repeated.inner.raw()));
        ProxiedInRepeated::repeated_free(Private, &mut int_r);
        std::mem::forget(int_r);
    }
}

#[derive(Debug)]
#[doc(hidden)]
pub struct InnerMap {
    pub(crate) raw: RawMap,
}

impl InnerMap {
    pub fn new(raw: RawMap) -> Self {
        Self { raw }
    }

    pub fn as_mut(&mut self) -> InnerMapMut<'_> {
        InnerMapMut { raw: self.raw, _phantom: PhantomData }
    }
}

#[derive(Clone, Copy, Debug)]
#[doc(hidden)]
pub struct InnerMapMut<'msg> {
    pub(crate) raw: RawMap,
    _phantom: PhantomData<&'msg ()>,
}

#[doc(hidden)]
impl<'msg> InnerMapMut<'msg> {
    pub fn new(raw: RawMap) -> Self {
        InnerMapMut { raw, _phantom: PhantomData }
    }

    pub fn as_raw(&self) -> RawMap {
        self.raw
    }
}

/// An untyped iterator in a map, produced via `.cbegin()` on a typed map.
///
/// This struct is ABI-compatible with `proto2::internal::UntypedMapIterator`.
/// It is trivially constructible and destructible.
#[repr(C)]
#[doc(hidden)]
pub struct UntypedMapIterator {
    node: *mut c_void,
    map: *const c_void,
    bucket_index: u32,
}

impl UntypedMapIterator {
    /// Returns `true` if this iterator is at the end of the map.
    fn at_end(&self) -> bool {
        // This behavior is verified via test `IteratorNodeFieldIsNullPtrAtEnd`.
        self.node.is_null()
    }

    /// Assumes that the map iterator is for the input types, gets the current
    /// entry, and moves the iterator forward to the next entry.
    ///
    /// Conversion to and from FFI types is provided by the user.
    /// This is a helper function for implementing
    /// `ProxiedInMapValue::iter_next`.
    ///
    /// # Safety
    /// - The backing map must be valid and not be mutated for `'a`.
    /// - The thunk must be safe to call if the iterator is not at the end of
    ///   the map.
    /// - The thunk must always write to the `key` and `value` fields, but not
    ///   read from them.
    /// - The get thunk must not move the iterator forward or backward.
    #[inline(always)]
    pub unsafe fn next_unchecked<'a, K, V, FfiKey, FfiValue>(
        &mut self,

        iter_get_thunk: unsafe fn(
            iter: &mut UntypedMapIterator,
            key: *mut FfiKey,
            value: *mut FfiValue,
        ),
        from_ffi_key: impl FnOnce(FfiKey) -> View<'a, K>,
        from_ffi_value: impl FnOnce(FfiValue) -> View<'a, V>,
    ) -> Option<(View<'a, K>, View<'a, V>)>
    where
        K: Proxied + 'a,
        V: ProxiedInMapValue<K> + 'a,
    {
        if self.at_end() {
            return None;
        }
        let mut ffi_key = MaybeUninit::uninit();
        let mut ffi_value = MaybeUninit::uninit();
        // SAFETY:
        // - The backing map outlives `'a`.
        // - The iterator is not at the end (node is non-null).
        // - `ffi_key` and `ffi_value` are not read (as uninit) as promised by the
        //   caller.
        unsafe { (iter_get_thunk)(self, ffi_key.as_mut_ptr(), ffi_value.as_mut_ptr()) }

        // SAFETY:
        // - The backing map is alive as promised by the caller.
        // - `self.at_end()` is false and the `get` does not change that.
        // - `UntypedMapIterator` has the same ABI as
        //   `proto2::internal::UntypedMapIterator`. It is statically checked to be:
        //   - Trivially copyable.
        //   - Trivially destructible.
        //   - Standard layout.
        //   - The size and alignment of the Rust type above.
        //   - With the `node_` field first.
        unsafe { proto2_rust_thunk_UntypedMapIterator_increment(self) }

        // SAFETY:
        // - The `get` function always writes valid values to `ffi_key` and `ffi_value`
        //   as promised by the caller.
        unsafe {
            Some((from_ffi_key(ffi_key.assume_init()), from_ffi_value(ffi_value.assume_init())))
        }
    }
}

// LINT.IfChange(map_ffi)
#[doc(hidden)]
#[repr(u8)]
#[derive(Debug, PartialEq)]
// Copy of UntypedMapBase::TypeKind
pub enum MapValueTag {
    Bool,
    U32,
    U64,
    F32,
    F64,
    String,
    Message,
}
// For the purposes of FFI, we treat all integral types of a given size the same
// way. For example, u32 and i32 values are all represented as a u32.
// Likewise, u64 and i64 values are all stored in a u64.
#[doc(hidden)]
#[repr(C)]
pub union MapValueUnion {
    pub b: bool,
    pub u: u32,
    pub uu: u64,
    pub f: f32,
    pub ff: f64,
    // Generally speaking, if s is set then it should not be None. However, we
    // do set it to None in the special case where the MapValue is just a
    // "prototype" (see below). In that scenario, we just want to indicate the
    // value type without having to allocate a real C++ std::string.
    pub s: Option<CppStdString>,
    pub m: RawMessage,
}

// We use this tagged union to represent map values for the purposes of FFI.
#[doc(hidden)]
#[repr(C)]
pub struct MapValue {
    pub tag: MapValueTag,
    pub val: MapValueUnion,
}
// LINT.ThenChange(//depot/google3/third_party/protobuf/rust/cpp_kernel/map.cc:
// map_ffi)

impl MapValue {
    fn make_bool(b: bool) -> Self {
        MapValue { tag: MapValueTag::Bool, val: MapValueUnion { b } }
    }

    pub fn make_u32(u: u32) -> Self {
        MapValue { tag: MapValueTag::U32, val: MapValueUnion { u } }
    }

    fn make_u64(uu: u64) -> Self {
        MapValue { tag: MapValueTag::U64, val: MapValueUnion { uu } }
    }

    pub fn make_f32(f: f32) -> Self {
        MapValue { tag: MapValueTag::F32, val: MapValueUnion { f } }
    }

    fn make_f64(ff: f64) -> Self {
        MapValue { tag: MapValueTag::F64, val: MapValueUnion { ff } }
    }

    fn make_string(s: CppStdString) -> Self {
        MapValue { tag: MapValueTag::String, val: MapValueUnion { s: Some(s) } }
    }

    pub fn make_message(m: RawMessage) -> Self {
        MapValue { tag: MapValueTag::Message, val: MapValueUnion { m } }
    }
}

pub trait CppMapTypeConversions: Proxied {
    // We have a notion of a map value "prototype", which is a MapValue that
    // contains just enough information to indicate the value type of the map.
    // We need this on the C++ side to be able to determine size and offset
    // information about the map entry. For messages, the prototype is
    // the message default instance. For all other types, it is just a MapValue
    // with the appropriate tag.
    fn get_prototype() -> MapValue;

    fn to_map_value(self) -> MapValue;

    /// # Safety
    /// - `value` must store the correct type for `Self`. If it is a string or
    ///   bytes, then it must not be None. If `Self` is a closed enum, then
    ///   `value` must store a valid value for that enum. If `Self` is a
    ///   message, then `value` must store a message of the same type.
    /// - The value must be valid for `'a` lifetime.
    unsafe fn from_map_value<'a>(value: MapValue) -> View<'a, Self>;

    /// # Safety
    /// - `value` must store a message of the same type as `Self`.
    /// - `value` must be valid and have exclusive mutable access for `'a` lifetime.
    #[allow(unused_variables)]
    unsafe fn mut_from_map_value<'a>(value: MapValue) -> Mut<'a, Self>
    where
        Self: Message,
    {
        panic!("mut_from_map_value is only implemented for messages")
    }
}

impl CppMapTypeConversions for u32 {
    fn get_prototype() -> MapValue {
        MapValue::make_u32(0)
    }
    fn to_map_value(self) -> MapValue {
        MapValue::make_u32(self)
    }
    unsafe fn from_map_value<'a>(value: MapValue) -> View<'a, Self> {
        debug_assert_eq!(value.tag, MapValueTag::U32);
        unsafe { value.val.u }
    }
}

impl CppMapTypeConversions for i32 {
    fn get_prototype() -> MapValue {
        MapValue::make_u32(0)
    }
    fn to_map_value(self) -> MapValue {
        MapValue::make_u32(self as u32)
    }
    unsafe fn from_map_value<'a>(value: MapValue) -> View<'a, Self> {
        debug_assert_eq!(value.tag, MapValueTag::U32);
        unsafe { value.val.u as i32 }
    }
}

impl CppMapTypeConversions for u64 {
    fn get_prototype() -> MapValue {
        MapValue::make_u64(0)
    }
    fn to_map_value(self) -> MapValue {
        MapValue::make_u64(self)
    }
    unsafe fn from_map_value<'a>(value: MapValue) -> View<'a, Self> {
        debug_assert_eq!(value.tag, MapValueTag::U64);
        unsafe { value.val.uu }
    }
}

impl CppMapTypeConversions for i64 {
    fn get_prototype() -> MapValue {
        MapValue::make_u64(0)
    }
    fn to_map_value(self) -> MapValue {
        MapValue::make_u64(self as u64)
    }
    unsafe fn from_map_value<'a>(value: MapValue) -> View<'a, Self> {
        debug_assert_eq!(value.tag, MapValueTag::U64);
        unsafe { value.val.uu as i64 }
    }
}

impl CppMapTypeConversions for f32 {
    fn get_prototype() -> MapValue {
        MapValue::make_f32(0f32)
    }
    fn to_map_value(self) -> MapValue {
        MapValue::make_f32(self)
    }
    unsafe fn from_map_value<'a>(value: MapValue) -> View<'a, Self> {
        debug_assert_eq!(value.tag, MapValueTag::F32);
        unsafe { value.val.f }
    }
}

impl CppMapTypeConversions for f64 {
    fn get_prototype() -> MapValue {
        MapValue::make_f64(0.0)
    }
    fn to_map_value(self) -> MapValue {
        MapValue::make_f64(self)
    }
    unsafe fn from_map_value<'a>(value: MapValue) -> View<'a, Self> {
        debug_assert_eq!(value.tag, MapValueTag::F64);
        unsafe { value.val.ff }
    }
}

impl CppMapTypeConversions for bool {
    fn get_prototype() -> MapValue {
        MapValue::make_bool(false)
    }
    fn to_map_value(self) -> MapValue {
        MapValue::make_bool(self)
    }
    unsafe fn from_map_value<'a>(value: MapValue) -> View<'a, Self> {
        debug_assert_eq!(value.tag, MapValueTag::Bool);
        unsafe { value.val.b }
    }
}

impl CppMapTypeConversions for ProtoString {
    fn get_prototype() -> MapValue {
        MapValue { tag: MapValueTag::String, val: MapValueUnion { s: None } }
    }

    fn to_map_value(self) -> MapValue {
        MapValue::make_string(protostr_into_cppstdstring(self))
    }

    unsafe fn from_map_value<'a>(value: MapValue) -> &'a ProtoStr {
        debug_assert_eq!(value.tag, MapValueTag::String);
        unsafe {
            ProtoStr::from_utf8_unchecked(
                ptrlen_to_str(proto2_rust_cpp_string_to_view(value.val.s.unwrap())).into(),
            )
        }
    }
}

impl CppMapTypeConversions for ProtoBytes {
    fn get_prototype() -> MapValue {
        MapValue { tag: MapValueTag::String, val: MapValueUnion { s: None } }
    }

    fn to_map_value(self) -> MapValue {
        MapValue::make_string(protobytes_into_cppstdstring(self))
    }

    unsafe fn from_map_value<'a>(value: MapValue) -> &'a [u8] {
        debug_assert_eq!(value.tag, MapValueTag::String);
        unsafe { proto2_rust_cpp_string_to_view(value.val.s.unwrap()).as_ref() }
    }
}

// This trait encapsulates functionality that is specific to each map key type.
// We need this primarily so that we can call the appropriate FFI function for
// the key type.
#[doc(hidden)]
pub trait MapKey
where
    Self: Proxied,
{
    type FfiKey;

    fn to_view<'a>(key: Self::FfiKey) -> View<'a, Self>;

    unsafe fn insert(m: RawMap, key: View<'_, Self>, value: MapValue) -> bool;

    unsafe fn get(m: RawMap, key: View<'_, Self>, value: *mut MapValue) -> bool;

    unsafe fn iter_get(iter: &mut UntypedMapIterator, key: *mut Self::FfiKey, value: *mut MapValue);

    unsafe fn remove(m: RawMap, key: View<'_, Self>) -> bool;
}

macro_rules! generate_map_key_impl {
    ( $($key:ty, $mutable_ffi_key:ty, $to_ffi:expr, $from_ffi:expr;)* ) => {
        paste! {
        $(
        impl MapKey for $key {
            type FfiKey = $mutable_ffi_key;

            #[inline]
            fn to_view<'a>(key: Self::FfiKey) -> View<'a, Self> {
                $from_ffi(key)
            }

            #[inline]
            unsafe fn insert(
                m: RawMap,
                key: View<'_, Self>,
                value: MapValue,
            ) -> bool {
                unsafe { [< proto2_rust_map_insert_ $key >](m, $to_ffi(key), value) }
            }

            #[inline]
            unsafe fn get(
                m: RawMap,
                key: View<'_, Self>,
                value: *mut MapValue,
            ) -> bool {
                unsafe { [< proto2_rust_map_get_ $key >](m, $to_ffi(key), value) }
            }

            #[inline]
            unsafe fn iter_get(
                iter: &mut UntypedMapIterator,
                key: *mut Self::FfiKey,
                value: *mut MapValue,
            ) {
                unsafe { [< proto2_rust_map_iter_get_ $key >](iter, key, value) }
            }

            #[inline]
            unsafe fn remove(m: RawMap, key: View<'_, Self>) -> bool {
                unsafe { [< proto2_rust_map_remove_ $key >](m, $to_ffi(key)) }
            }
        }
        )*
        }
    }
}

generate_map_key_impl!(
    bool, bool, identity, identity;
    i32, i32, identity, identity;
    u32, u32, identity, identity;
    i64, i64, identity, identity;
    u64, u64, identity, identity;
    ProtoString, PtrAndLen, str_to_ptrlen, ptrlen_to_str;
);

impl<Key, Value> ProxiedInMapValue<Key> for Value
where
    Key: Proxied + MapKey + CppMapTypeConversions,
    Value: Proxied + CppMapTypeConversions,
{
    fn map_new(_private: Private) -> Map<Key, Self> {
        unsafe {
            Map::from_inner(
                Private,
                InnerMap::new(proto2_rust_map_new(Key::get_prototype(), Value::get_prototype())),
            )
        }
    }

    unsafe fn map_free(_private: Private, map: &mut Map<Key, Self>) {
        unsafe {
            proto2_rust_map_free(map.as_raw(Private));
        }
    }

    fn map_clear(mut map: MapMut<Key, Self>) {
        unsafe {
            proto2_rust_map_clear(map.as_raw(Private));
        }
    }

    fn map_len(map: MapView<Key, Self>) -> usize {
        unsafe { proto2_rust_map_size(map.as_raw(Private)) }
    }

    fn map_insert(
        mut map: MapMut<Key, Self>,
        key: View<'_, Key>,
        value: impl IntoProxied<Self>,
    ) -> bool {
        unsafe { Key::insert(map.as_raw(Private), key, value.into_proxied(Private).to_map_value()) }
    }

    fn map_get<'a>(map: MapView<'a, Key, Self>, key: View<'_, Key>) -> Option<View<'a, Self>> {
        let mut value = std::mem::MaybeUninit::uninit();
        let found = unsafe { Key::get(map.as_raw(Private), key, value.as_mut_ptr()) };
        if !found {
            return None;
        }
        unsafe { Some(Self::from_map_value(value.assume_init())) }
    }

    fn map_get_mut<'a>(mut map: MapMut<'a, Key, Self>, key: View<'_, Key>) -> Option<Mut<'a, Self>>
    where
        Value: Message,
    {
        let mut value = std::mem::MaybeUninit::uninit();
        let found = unsafe { Key::get(map.as_raw(Private), key, value.as_mut_ptr()) };
        if !found {
            return None;
        }
        // SAFETY: `value` has been initialized because it was found.
        // - `value` is a message as required by the trait.
        // - `value` is valid for the `'a` lifetime of the `MapMut`.
        unsafe { Some(Self::mut_from_map_value(value.assume_init())) }
    }

    fn map_remove(mut map: MapMut<Key, Self>, key: View<'_, Key>) -> bool {
        unsafe { Key::remove(map.as_raw(Private), key) }
    }

    fn map_iter(map: MapView<Key, Self>) -> MapIter<Key, Self> {
        // SAFETY:
        // - The backing map for `map.as_raw` is valid for at least '_.
        // - A View that is live for '_ guarantees the backing map is unmodified for '_.
        // - The `iter` function produces an iterator that is valid for the key and
        //   value types, and live for at least '_.
        unsafe { MapIter::from_raw(Private, proto2_rust_map_iter(map.as_raw(Private))) }
    }

    fn map_iter_next<'a>(
        iter: &mut MapIter<'a, Key, Self>,
    ) -> Option<(View<'a, Key>, View<'a, Self>)> {
        // SAFETY:
        // - The `MapIter` API forbids the backing map from being mutated for 'a, and
        //   guarantees that it's the correct key and value types.
        // - The thunk is safe to call as long as the iterator isn't at the end.
        // - The thunk always writes to key and value fields and does not read.
        // - The thunk does not increment the iterator.
        unsafe {
            iter.as_raw_mut(Private).next_unchecked::<Key, Self, _, _>(
                |iter, key, value| Key::iter_get(iter, key, value),
                |ffi_key| Key::to_view(ffi_key),
                |value| Self::from_map_value(value),
            )
        }
    }
}

macro_rules! impl_map_primitives {
    (@impl $(($rust_type:ty, $cpp_type:ty) => [
        $insert_thunk:ident,
        $get_thunk:ident,
        $iter_get_thunk:ident,
        $remove_thunk:ident,
    ]),* $(,)?) => {
        $(
            extern "C" {
                pub fn $insert_thunk(
                    m: RawMap,
                    key: $cpp_type,
                    value: MapValue,
                ) -> bool;
                pub fn $get_thunk(
                    m: RawMap,
                    key: $cpp_type,
                    value: *mut MapValue,
                ) -> bool;
                pub fn $iter_get_thunk(
                    iter: &mut UntypedMapIterator,
                    key: *mut $cpp_type,
                    value: *mut MapValue,
                );
                pub fn $remove_thunk(m: RawMap, key: $cpp_type) -> bool;
            }
        )*
    };
    ($($rust_type:ty, $cpp_type:ty;)* $(,)?) => {
        paste!{
            impl_map_primitives!(@impl $(
                    ($rust_type, $cpp_type) => [
                    [< proto2_rust_map_insert_ $rust_type >],
                    [< proto2_rust_map_get_ $rust_type >],
                    [< proto2_rust_map_iter_get_ $rust_type >],
                    [< proto2_rust_map_remove_ $rust_type >],
                ],
            )*);
        }
    };
}

impl_map_primitives!(
    i32, i32;
    u32, u32;
    i64, i64;
    u64, u64;
    bool, bool;
    ProtoString, PtrAndLen;
);

extern "C" {
    fn proto2_rust_thunk_UntypedMapIterator_increment(iter: &mut UntypedMapIterator);

    pub fn proto2_rust_map_new(key_prototype: MapValue, value_prototype: MapValue) -> RawMap;
    pub fn proto2_rust_map_free(m: RawMap);
    pub fn proto2_rust_map_clear(m: RawMap);
    pub fn proto2_rust_map_size(m: RawMap) -> usize;
    pub fn proto2_rust_map_iter(m: RawMap) -> UntypedMapIterator;
}

fn str_to_ptrlen<'msg>(val: impl Into<&'msg ProtoStr>) -> PtrAndLen {
    val.into().as_bytes().into()
}

// Warning: this function is unsound on its own! `val.as_ref()` must be safe to
// call.
fn ptrlen_to_str<'msg>(val: PtrAndLen) -> &'msg ProtoStr {
    unsafe { ProtoStr::from_utf8_unchecked(val.as_ref()) }
}

fn protostr_into_cppstdstring(val: ProtoString) -> CppStdString {
    val.into_inner(Private).into_raw()
}

fn protobytes_into_cppstdstring(val: ProtoBytes) -> CppStdString {
    val.into_inner(Private).into_raw()
}

// Warning: this function is unsound on its own! `val.as_ref()` must be safe to
// call.
fn ptrlen_to_bytes<'msg>(val: PtrAndLen) -> &'msg [u8] {
    unsafe { val.as_ref() }
}

impl<T> MatcherEq for T
where
    Self: AsView + Debug,
    for<'a> View<'a, <Self as AsView>::Proxied>: MessageViewInterop<'a>,
{
    fn matches(&self, o: &Self) -> bool {
        unsafe {
            raw_message_equals(
                NonNull::new_unchecked(self.as_view().__unstable_as_raw_message() as *mut _),
                NonNull::new_unchecked(o.as_view().__unstable_as_raw_message() as *mut _),
            )
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use googletest::prelude::*;

    // We need to allocate the byte array so SerializedData can own it and
    // deallocate it in its drop. This function makes it easier to do so for our
    // tests.
    fn allocate_byte_array(content: &'static [u8]) -> (*mut u8, usize) {
        let content: &mut [u8] = Box::leak(content.into());
        (content.as_mut_ptr(), content.len())
    }

    #[gtest]
    fn test_serialized_data_roundtrip() {
        let (ptr, len) = allocate_byte_array(b"Hello world");
        let serialized_data = SerializedData { data: NonNull::new(ptr).unwrap(), len };
        assert_that!(&*serialized_data, eq(b"Hello world"));
    }

    #[gtest]
    fn test_empty_string() {
        let empty_str: String = RustStringRawParts { data: std::ptr::null(), len: 0 }.into();
        assert_that!(empty_str, eq(""));
    }
}
