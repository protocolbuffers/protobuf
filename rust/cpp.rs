// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Rust Protobuf runtime using the C++ kernel.

use crate::__internal::{Enum, Private};
use crate::{
    Map, MapIter, Mut, ProtoStr, Proxied, ProxiedInMapValue, ProxiedInRepeated, Repeated,
    RepeatedMut, RepeatedView, View,
};
use core::fmt::Debug;
use paste::paste;
use std::convert::identity;
use std::ffi::{c_int, c_void};
use std::fmt;
use std::marker::PhantomData;
use std::mem::MaybeUninit;
use std::ops::Deref;
use std::ptr::{self, NonNull};
use std::slice;

/// Defines a set of opaque, unique, non-accessible pointees.
///
/// The [Rustonomicon][nomicon] currently recommends a zero-sized struct,
/// though this should use [`extern type`] when that is stabilized.
/// [nomicon]: https://doc.rust-lang.org/nomicon/ffi.html#representing-opaque-structs
/// [`extern type`]: https://github.com/rust-lang/rust/issues/43467
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
}

/// A raw pointer to the underlying message for this runtime.
pub type RawMessage = NonNull<_opaque_pointees::RawMessageData>;

/// A raw pointer to the underlying repeated field container for this runtime.
pub type RawRepeatedField = NonNull<_opaque_pointees::RawRepeatedFieldData>;

/// A raw pointer to the underlying arena for this runtime.
pub type RawMap = NonNull<_opaque_pointees::RawMapData>;

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

// TODO: remove after IntoProxied has been implemented for bytes.
impl AsRef<[u8]> for SerializedData {
    fn as_ref(&self) -> &[u8] {
        self
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
    fn utf8_debug_string(msg: RawMessage) -> RustStringRawParts;
}

pub fn debug_string(_private: Private, msg: RawMessage, f: &mut fmt::Formatter<'_>) -> fmt::Result {
    // SAFETY:
    // - `msg` is a valid protobuf message.
    let dbg_str: String = unsafe { utf8_debug_string(msg) }.into();
    write!(f, "{dbg_str}")
}

pub type RawMapIter = UntypedMapIterator;

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
        Self { msg: message_field_ptr, _phantom: PhantomData }
    }

    pub fn msg(&self) -> RawMessage {
        self.msg
    }

    pub fn from_raw_msg(_private: Private, msg: &RawMessage) -> Self {
        Self { msg: *msg, _phantom: PhantomData }
    }
}

pub fn copy_bytes_in_arena_if_needed_by_runtime<'msg>(
    _msg_ref: MutatorMessageRef<'msg>,
    val: &'msg [u8],
) -> &'msg [u8] {
    // Nothing to do, the message manages its own string memory for C++.
    val
}

/// The raw type-erased version of an owned `Repeated`.
#[derive(Debug)]
pub struct InnerRepeated {
    raw: RawRepeatedField,
}

impl InnerRepeated {
    pub fn as_mut(&mut self) -> InnerRepeatedMut<'_> {
        InnerRepeatedMut::new(Private, self.raw)
    }

    pub fn raw(&self) -> RawRepeatedField {
        self.raw
    }
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
                    Repeated::from_inner(InnerRepeated {
                        raw: unsafe { $new_thunk() }
                    })
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

#[derive(Debug)]
pub struct InnerMap {
    pub(crate) raw: RawMap,
}

impl InnerMap {
    pub fn new(_private: Private, raw: RawMap) -> Self {
        Self { raw }
    }

    pub fn as_mut(&mut self) -> InnerMapMut<'_> {
        InnerMapMut { raw: self.raw, _phantom: PhantomData }
    }
}

#[derive(Clone, Copy, Debug)]
pub struct InnerMapMut<'msg> {
    pub(crate) raw: RawMap,
    _phantom: PhantomData<&'msg ()>,
}

#[doc(hidden)]
impl<'msg> InnerMapMut<'msg> {
    pub fn new(_private: Private, raw: RawMap) -> Self {
        InnerMapMut { raw, _phantom: PhantomData }
    }

    #[doc(hidden)]
    pub fn as_raw(&self, _private: Private) -> RawMap {
        self.raw
    }
}

/// An untyped iterator in a map, produced via `.cbegin()` on a typed map.
///
/// This struct is ABI-compatible with `proto2::internal::UntypedMapIterator`.
/// It is trivially constructible and destructible.
#[repr(C)]
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
        _private: Private,
        iter_get_thunk: unsafe extern "C" fn(
            iter: &mut UntypedMapIterator,
            key: *mut FfiKey,
            value: *mut FfiValue,
        ),
        from_ffi_key: impl FnOnce(FfiKey) -> View<'a, K>,
        from_ffi_value: impl FnOnce(FfiValue) -> View<'a, V>,
    ) -> Option<(View<'a, K>, View<'a, V>)>
    where
        K: Proxied + ?Sized + 'a,
        V: ProxiedInMapValue<K> + ?Sized + 'a,
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
        unsafe { __rust_proto_thunk__UntypedMapIterator_increment(self) }

        // SAFETY:
        // - The `get` function always writes valid values to `ffi_key` and `ffi_value`
        //   as promised by the caller.
        unsafe {
            Some((from_ffi_key(ffi_key.assume_init()), from_ffi_value(ffi_value.assume_init())))
        }
    }
}

extern "C" {
    fn __rust_proto_thunk__UntypedMapIterator_increment(iter: &mut UntypedMapIterator);
}

macro_rules! impl_ProxiedInMapValue_for_non_generated_value_types {
    ($key_t:ty, $ffi_key_t:ty, $to_ffi_key:expr, $from_ffi_key:expr, for $($t:ty, $ffi_t:ty, $to_ffi_value:expr, $from_ffi_value:expr;)*) => {
        paste! { $(
            extern "C" {
                fn [< __rust_proto_thunk__Map_ $key_t _ $t _new >]() -> RawMap;
                fn [< __rust_proto_thunk__Map_ $key_t _ $t _free >](m: RawMap);
                fn [< __rust_proto_thunk__Map_ $key_t _ $t _clear >](m: RawMap);
                fn [< __rust_proto_thunk__Map_ $key_t _ $t _size >](m: RawMap) -> usize;
                fn [< __rust_proto_thunk__Map_ $key_t _ $t _insert >](m: RawMap, key: $ffi_key_t, value: $ffi_t) -> bool;
                fn [< __rust_proto_thunk__Map_ $key_t _ $t _get >](m: RawMap, key: $ffi_key_t, value: *mut $ffi_t) -> bool;
                fn [< __rust_proto_thunk__Map_ $key_t _ $t _iter >](m: RawMap) -> UntypedMapIterator;
                fn [< __rust_proto_thunk__Map_ $key_t _ $t _iter_get >](iter: &mut UntypedMapIterator, key: *mut $ffi_key_t, value: *mut $ffi_t);
                fn [< __rust_proto_thunk__Map_ $key_t _ $t _remove >](m: RawMap, key: $ffi_key_t, value: *mut $ffi_t) -> bool;
            }

            impl ProxiedInMapValue<$key_t> for $t {
                fn map_new(_private: Private) -> Map<$key_t, Self> {
                    unsafe {
                        Map::from_inner(
                            Private,
                            InnerMap {
                                raw: [< __rust_proto_thunk__Map_ $key_t _ $t _new >](),
                            }
                        )
                    }
                }

                unsafe fn map_free(_private: Private, map: &mut Map<$key_t, Self>) {
                    // SAFETY:
                    // - `map.inner.raw` is a live `RawMap`
                    // - This function is only called once for `map` in `Drop`.
                    unsafe { [< __rust_proto_thunk__Map_ $key_t _ $t _free >](map.as_mut().as_raw(Private)); }
                }


                fn map_clear(mut map: Mut<'_, Map<$key_t, Self>>) {
                    unsafe { [< __rust_proto_thunk__Map_ $key_t _ $t _clear >](map.as_raw(Private)); }
                }

                fn map_len(map: View<'_, Map<$key_t, Self>>) -> usize {
                    unsafe { [< __rust_proto_thunk__Map_ $key_t _ $t _size >](map.as_raw(Private)) }
                }

                fn map_insert(mut map: Mut<'_, Map<$key_t, Self>>, key: View<'_, $key_t>, value: View<'_, Self>) -> bool {
                    let ffi_key = $to_ffi_key(key);
                    let ffi_value = $to_ffi_value(value);
                    unsafe { [< __rust_proto_thunk__Map_ $key_t _ $t _insert >](map.as_raw(Private), ffi_key, ffi_value) }
                }

                fn map_get<'a>(map: View<'a, Map<$key_t, Self>>, key: View<'_, $key_t>) -> Option<View<'a, Self>> {
                    let ffi_key = $to_ffi_key(key);
                    let mut ffi_value = MaybeUninit::uninit();
                    let found = unsafe { [< __rust_proto_thunk__Map_ $key_t _ $t _get >](map.as_raw(Private), ffi_key, ffi_value.as_mut_ptr()) };

                    if !found {
                        return None;
                    }
                    // SAFETY: if `found` is true, then the `ffi_value` was written to by `get`.
                    Some($from_ffi_value(unsafe { ffi_value.assume_init() }))
                }

                fn map_remove(mut map: Mut<'_, Map<$key_t, Self>>, key: View<'_, $key_t>) -> bool {
                    let ffi_key = $to_ffi_key(key);
                    let mut ffi_value = MaybeUninit::uninit();
                    unsafe { [< __rust_proto_thunk__Map_ $key_t _ $t _remove >](map.as_raw(Private), ffi_key, ffi_value.as_mut_ptr()) }
                }

                fn map_iter(map: View<'_, Map<$key_t, Self>>) -> MapIter<'_, $key_t, Self> {
                    // SAFETY:
                    // - The backing map for `map.as_raw` is valid for at least '_.
                    // - A View that is live for '_ guarantees the backing map is unmodified for '_.
                    // - The `iter` function produces an iterator that is valid for the key
                    //   and value types, and live for at least '_.
                    unsafe {
                        MapIter::from_raw(
                            Private,
                            [< __rust_proto_thunk__Map_ $key_t _ $t _iter >](map.as_raw(Private))
                        )
                    }
                }

                fn map_iter_next<'a>(iter: &mut MapIter<'a, $key_t, Self>) -> Option<(View<'a, $key_t>, View<'a, Self>)> {
                    // SAFETY:
                    // - The `MapIter` API forbids the backing map from being mutated for 'a,
                    //   and guarantees that it's the correct key and value types.
                    // - The thunk is safe to call as long as the iterator isn't at the end.
                    // - The thunk always writes to key and value fields and does not read.
                    // - The thunk does not increment the iterator.
                    unsafe {
                        iter.as_raw_mut(Private).next_unchecked::<$key_t, Self, _, _>(
                            Private,
                            [< __rust_proto_thunk__Map_ $key_t _ $t _iter_get >],
                            $from_ffi_key,
                            $from_ffi_value,
                        )
                    }
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
    ($($t:ty, $ffi_t:ty, $to_ffi_key:expr, $from_ffi_key:expr;)*) => {
        paste! {
            $(
                impl_ProxiedInMapValue_for_non_generated_value_types!(
                    $t, $ffi_t, $to_ffi_key, $from_ffi_key, for
                    f32, f32, identity, identity;
                    f64, f64, identity, identity;
                    i32, i32, identity, identity;
                    u32, u32, identity, identity;
                    i64, i64, identity, identity;
                    u64, u64, identity, identity;
                    bool, bool, identity, identity;
                    ProtoStr, PtrAndLen, str_to_ptrlen, ptrlen_to_str;
                    Bytes, PtrAndLen, bytes_to_ptrlen, ptrlen_to_bytes;
                );
            )*
        }
    }
}

impl_ProxiedInMapValue_for_key_types!(
    i32, i32, identity, identity;
    u32, u32, identity, identity;
    i64, i64, identity, identity;
    u64, u64, identity, identity;
    bool, bool, identity, identity;
    ProtoStr, PtrAndLen, str_to_ptrlen, ptrlen_to_str;
);

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

    #[test]
    fn test_serialized_data_roundtrip() {
        let (ptr, len) = allocate_byte_array(b"Hello world");
        let serialized_data = SerializedData { data: NonNull::new(ptr).unwrap(), len };
        assert_that!(&*serialized_data, eq(b"Hello world"));
    }

    #[test]
    fn test_empty_string() {
        let empty_str: String = RustStringRawParts { data: std::ptr::null(), len: 0 }.into();
        assert_that!(empty_str, eq(""));
    }
}
