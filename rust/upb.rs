// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

//! UPB FFI wrapper code for use by Rust Protobuf.

use crate::ProtoStr;
use crate::__internal::{Private, PtrAndLen, RawArena, RawMap, RawMessage, RawRepeatedField};
use core::fmt::Debug;
use paste::paste;
use std::alloc;
use std::alloc::Layout;
use std::cell::UnsafeCell;
use std::fmt;
use std::marker::PhantomData;
use std::mem::{size_of, MaybeUninit};
use std::ops::Deref;
use std::ptr::{self, NonNull};
use std::slice;
use std::sync::Once;

/// See `upb/port/def.inc`.
const UPB_MALLOC_ALIGN: usize = 8;

/// A wrapper over a `upb_Arena`.
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
    // Safety invariant: this must always be a valid arena
    raw: RawArena,
    _not_sync: PhantomData<UnsafeCell<()>>,
}

extern "C" {
    // `Option<NonNull<T: Sized>>` is ABI-compatible with `*mut T`
    fn upb_Arena_New() -> Option<RawArena>;
    fn upb_Arena_Free(arena: RawArena);
    fn upb_Arena_Malloc(arena: RawArena, size: usize) -> *mut u8;
    fn upb_Arena_Realloc(arena: RawArena, ptr: *mut u8, old: usize, new: usize) -> *mut u8;
}

impl Arena {
    /// Allocates a fresh arena.
    #[inline]
    pub fn new() -> Self {
        #[inline(never)]
        #[cold]
        fn arena_new_failed() -> ! {
            panic!("Could not create a new UPB arena");
        }

        // SAFETY:
        // - `upb_Arena_New` is assumed to be implemented correctly and always sound to
        //   call; if it returned a non-null pointer, it is a valid arena.
        unsafe {
            let Some(raw) = upb_Arena_New() else { arena_new_failed() };
            Self { raw, _not_sync: PhantomData }
        }
    }

    /// Returns the raw, UPB-managed pointer to the arena.
    #[inline]
    pub fn raw(&self) -> RawArena {
        self.raw
    }

    /// Allocates some memory on the arena.
    ///
    /// # Safety
    ///
    /// - `layout`'s alignment must be less than `UPB_MALLOC_ALIGN`.
    #[inline]
    pub unsafe fn alloc(&self, layout: Layout) -> &mut [MaybeUninit<u8>] {
        debug_assert!(layout.align() <= UPB_MALLOC_ALIGN);
        // SAFETY: `self.raw` is a valid UPB arena
        let ptr = unsafe { upb_Arena_Malloc(self.raw, layout.size()) };
        if ptr.is_null() {
            alloc::handle_alloc_error(layout);
        }

        // SAFETY:
        // - `upb_Arena_Malloc` promises that if the return pointer is non-null, it is
        //   dereferencable for `size` bytes and has an alignment of `UPB_MALLOC_ALIGN`
        //   until the arena is destroyed.
        // - `[MaybeUninit<u8>]` has no alignment requirement, and `ptr` is aligned to a
        //   `UPB_MALLOC_ALIGN` boundary.
        unsafe { slice::from_raw_parts_mut(ptr.cast(), layout.size()) }
    }

    /// Resizes some memory on the arena.
    ///
    /// # Safety
    ///
    /// - `ptr` must be the data pointer returned by a previous call to `alloc`
    ///   or `resize` on `self`.
    /// - After calling this function, `ptr` is no longer dereferencable - it is
    ///   zapped.
    /// - `old` must be the layout `ptr` was allocated with via `alloc` or
    ///   `realloc`.
    /// - `new`'s alignment must be less than `UPB_MALLOC_ALIGN`.
    #[inline]
    pub unsafe fn resize(&self, ptr: *mut u8, old: Layout, new: Layout) -> &mut [MaybeUninit<u8>] {
        debug_assert!(new.align() <= UPB_MALLOC_ALIGN);
        // SAFETY:
        // - `self.raw` is a valid UPB arena
        // - `ptr` was allocated by a previous call to `alloc` or `realloc` as promised
        //   by the caller.
        let ptr = unsafe { upb_Arena_Realloc(self.raw, ptr, old.size(), new.size()) };
        if ptr.is_null() {
            alloc::handle_alloc_error(new);
        }

        // SAFETY:
        // - `upb_Arena_Realloc` promises that if the return pointer is non-null, it is
        //   dereferencable for the new `size` in bytes until the arena is destroyed.
        // - `[MaybeUninit<u8>]` has no alignment requirement, and `ptr` is aligned to a
        //   `UPB_MALLOC_ALIGN` boundary.
        unsafe { slice::from_raw_parts_mut(ptr.cast(), new.size()) }
    }
}

impl Drop for Arena {
    #[inline]
    fn drop(&mut self) {
        unsafe {
            upb_Arena_Free(self.raw);
        }
    }
}

static mut INTERNAL_PTR: Option<RawMessage> = None;
static INIT: Once = Once::new();

// TODO:(b/304577017)
const ALIGN: usize = 32;
const UPB_SCRATCH_SPACE_BYTES: usize = 64_000;

/// Holds a zero-initialized block of memory for use by upb.
/// By default, if a message is not set in cpp, a default message is created.
/// upb departs from this and returns a null ptr. However, since contiguous
/// chunks of memory filled with zeroes are legit messages from upb's point of
/// view, we can allocate a large block and refer to that when dealing
/// with readonly access.
pub struct ScratchSpace;
impl ScratchSpace {
    pub fn zeroed_block(_private: Private) -> RawMessage {
        unsafe {
            INIT.call_once(|| {
                let layout =
                    std::alloc::Layout::from_size_align(UPB_SCRATCH_SPACE_BYTES, ALIGN).unwrap();
                let Some(ptr) =
                    crate::__internal::RawMessage::new(std::alloc::alloc_zeroed(layout).cast())
                else {
                    std::alloc::handle_alloc_error(layout)
                };
                INTERNAL_PTR = Some(ptr)
            });
            INTERNAL_PTR.unwrap()
        }
    }
}

/// Serialized Protobuf wire format data.
///
/// It's typically produced by `<Message>::serialize()`.
pub struct SerializedData {
    data: NonNull<u8>,
    len: usize,

    // The arena that owns `data`.
    _arena: Arena,
}

impl SerializedData {
    /// Construct `SerializedData` from raw pointers and its owning arena.
    ///
    /// # Safety
    /// - `arena` must be have allocated `data`
    /// - `data` must be readable for `len` bytes and not mutate while this
    ///   struct exists
    pub unsafe fn from_raw_parts(arena: Arena, data: NonNull<u8>, len: usize) -> Self {
        SerializedData { _arena: arena, data, len }
    }

    /// Gets a raw slice pointer.
    pub fn as_ptr(&self) -> *const [u8] {
        ptr::slice_from_raw_parts(self.data.as_ptr(), self.len)
    }
}

impl Deref for SerializedData {
    type Target = [u8];
    fn deref(&self) -> &Self::Target {
        // SAFETY: `data` is valid for `len` bytes as promised by
        //         the caller of `SerializedData::from_raw_parts`.
        unsafe { slice::from_raw_parts(self.data.as_ptr(), self.len) }
    }
}

impl fmt::Debug for SerializedData {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        fmt::Debug::fmt(self.deref(), f)
    }
}

// TODO: Investigate replacing this with direct access to UPB bits.
pub type BytesPresentMutData<'msg> = crate::vtable::RawVTableOptionalMutatorData<'msg, [u8]>;
pub type BytesAbsentMutData<'msg> = crate::vtable::RawVTableOptionalMutatorData<'msg, [u8]>;
pub type InnerBytesMut<'msg> = crate::vtable::RawVTableMutator<'msg, [u8]>;
pub type InnerPrimitiveMut<'a, T> = crate::vtable::RawVTableMutator<'a, T>;

/// The raw contents of every generated message.
#[derive(Debug)]
pub struct MessageInner {
    pub msg: RawMessage,
    pub arena: Arena,
}

/// Mutators that point to their original message use this to do so.
///
/// Since UPB expects runtimes to manage their own arenas, this needs to have
/// access to an `Arena`.
///
/// This has two possible designs:
/// - Store two pointers here, `RawMessage` and `&'msg Arena`. This doesn't
///   place any restriction on the layout of generated messages and their
///   mutators. This makes a vtable-based mutator three pointers, which can no
///   longer be returned in registers on most platforms.
/// - Store one pointer here, `&'msg MessageInner`, where `MessageInner` stores
///   a `RawMessage` and an `Arena`. This would require all generated messages
///   to store `MessageInner`, and since their mutators need to be able to
///   generate `BytesMut`, would also require `BytesMut` to store a `&'msg
///   MessageInner` since they can't store an owned `Arena`.
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
    arena: &'msg Arena,
}

impl<'msg> MutatorMessageRef<'msg> {
    #[doc(hidden)]
    #[allow(clippy::needless_pass_by_ref_mut)] // Sound construction requires mutable access.
    pub fn new(_private: Private, msg: &'msg mut MessageInner) -> Self {
        MutatorMessageRef { msg: msg.msg, arena: &msg.arena }
    }

    pub fn from_parent(
        _private: Private,
        parent_msg: &'msg mut MessageInner,
        message_field_ptr: RawMessage,
    ) -> Self {
        MutatorMessageRef { msg: message_field_ptr, arena: &parent_msg.arena }
    }

    pub fn msg(&self) -> RawMessage {
        self.msg
    }
}

pub fn copy_bytes_in_arena_if_needed_by_runtime<'a>(
    msg_ref: MutatorMessageRef<'a>,
    val: &'a [u8],
) -> &'a [u8] {
    // SAFETY: the alignment of `[u8]` is less than `UPB_MALLOC_ALIGN`.
    let new_alloc = unsafe { msg_ref.arena.alloc(Layout::for_value(val)) };
    debug_assert_eq!(new_alloc.len(), val.len());

    let start: *mut u8 = new_alloc.as_mut_ptr().cast();
    // SAFETY:
    // - `new_alloc` is writeable for `val.len()` bytes.
    // - After the copy, `new_alloc` is initialized for `val.len()` bytes.
    unsafe {
        val.as_ptr().copy_to_nonoverlapping(start, val.len());
        &*(new_alloc as *mut _ as *mut [u8])
    }
}

/// RepeatedFieldInner contains a `upb_Array*` as well as a reference to an
/// `Arena`, most likely that of the containing `Message`. upb requires an Arena
/// to perform mutations on a repeated field.
#[derive(Clone, Copy, Debug)]
pub struct RepeatedFieldInner<'msg> {
    pub raw: RawRepeatedField,
    pub arena: &'msg Arena,
}

#[derive(Debug)]
pub struct RepeatedField<'msg, T: ?Sized> {
    inner: RepeatedFieldInner<'msg>,
    _phantom: PhantomData<&'msg mut T>,
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

impl<'msg, T: ?Sized> RepeatedField<'msg, T> {
    pub fn len(&self) -> usize {
        unsafe { upb_Array_Size(self.inner.raw) }
    }
    pub fn is_empty(&self) -> bool {
        self.len() == 0
    }
    pub fn from_inner(_private: Private, inner: RepeatedFieldInner<'msg>) -> Self {
        Self { inner, _phantom: PhantomData }
    }
}

// Transcribed from google3/third_party/upb/upb/message/value.h
#[repr(C)]
#[derive(Clone, Copy)]
pub union upb_MessageValue {
    bool_val: bool,
    float_val: std::ffi::c_float,
    double_val: std::ffi::c_double,
    uint32_val: u32,
    int32_val: i32,
    uint64_val: u64,
    int64_val: i64,
    array_val: *const std::ffi::c_void,
    map_val: *const std::ffi::c_void,
    msg_val: *const std::ffi::c_void,
    str_val: PtrAndLen,
}

// Transcribed from google3/third_party/upb/upb/base/descriptor_constants.h
#[repr(C)]
#[allow(dead_code)]
pub enum UpbCType {
    Bool = 1,
    Float = 2,
    Int32 = 3,
    UInt32 = 4,
    Enum = 5,
    Message = 6,
    Double = 7,
    Int64 = 8,
    UInt64 = 9,
    String = 10,
    Bytes = 11,
}

extern "C" {
    #[allow(dead_code)]
    fn upb_Array_New(a: RawArena, r#type: std::ffi::c_int) -> RawRepeatedField;
    fn upb_Array_Size(arr: RawRepeatedField) -> usize;
    fn upb_Array_Set(arr: RawRepeatedField, i: usize, val: upb_MessageValue);
    fn upb_Array_Get(arr: RawRepeatedField, i: usize) -> upb_MessageValue;
    fn upb_Array_Append(arr: RawRepeatedField, val: upb_MessageValue, arena: RawArena);
    fn upb_Array_Resize(arr: RawRepeatedField, size: usize, arena: RawArena) -> bool;
    fn upb_Array_MutableDataPtr(arr: RawRepeatedField) -> *mut std::ffi::c_void;
    fn upb_Array_DataPtr(arr: RawRepeatedField) -> *const std::ffi::c_void;
}

macro_rules! impl_repeated_primitives {
    ($(($rs_type:ty, $ufield:ident, $upb_tag:expr)),*) => {
        $(
            impl<'msg> RepeatedField<'msg, $rs_type> {
                #[allow(dead_code)]
                fn new(arena: &'msg Arena) -> Self {
                    Self {
                        inner: RepeatedFieldInner {
                            raw: unsafe { upb_Array_New(arena.raw, $upb_tag as std::ffi::c_int) },
                            arena,
                        },
                        _phantom: PhantomData,
                    }
                }
                pub fn push(&mut self, val: $rs_type) {
                    unsafe { upb_Array_Append(
                        self.inner.raw,
                        upb_MessageValue { $ufield: val },
                        self.inner.arena.raw(),
                    ) }
                }
                pub fn get(&self, i: usize) -> Option<$rs_type> {
                    if i >= self.len() {
                        None
                    } else {
                        unsafe { Some(upb_Array_Get(self.inner.raw, i).$ufield) }
                    }
                }
                pub fn set(&self, i: usize, val: $rs_type) {
                    if i >= self.len() {
                        return;
                    }
                    unsafe { upb_Array_Set(
                        self.inner.raw,
                        i,
                        upb_MessageValue { $ufield: val },
                    ) }
                }
                pub fn copy_from(&mut self, src: &RepeatedField<'_, $rs_type>) {
                    // SAFETY:
                    // - `upb_Array_Resize` is unsafe but assumed to be always sound to call.
                    // - `copy_nonoverlapping` is unsafe but here we guarantee that both pointers
                    //   are valid, the pointers are `#[repr(u8)]`, and the size is correct.
                    unsafe {
                        if (!upb_Array_Resize(self.inner.raw, src.len(), self.inner.arena.raw())) {
                            panic!("upb_Array_Resize failed.");
                        }
                        ptr::copy_nonoverlapping(
                          upb_Array_DataPtr(src.inner.raw).cast::<u8>(),
                          upb_Array_MutableDataPtr(self.inner.raw).cast::<u8>(),
                          size_of::<$rs_type>() * src.len());
                    }
                }
            }
        )*
    }
}

impl_repeated_primitives!(
    (bool, bool_val, UpbCType::Bool),
    (f32, float_val, UpbCType::Float),
    (f64, double_val, UpbCType::Double),
    (i32, int32_val, UpbCType::Int32),
    (u32, uint32_val, UpbCType::UInt32),
    (i64, int64_val, UpbCType::Int64),
    (u64, uint64_val, UpbCType::UInt64)
);

/// Returns a static thread-local empty RepeatedFieldInner for use in a
/// RepeatedView.
///
/// # Safety
/// The returned array must never be mutated.
///
/// TODO: Split RepeatedFieldInner into mut and const variants to
/// enforce safety. The returned array must never be mutated.
pub unsafe fn empty_array() -> RepeatedFieldInner<'static> {
    // TODO: Consider creating empty array in C.
    fn new_repeated_field_inner() -> RepeatedFieldInner<'static> {
        let arena = Box::leak::<'static>(Box::new(Arena::new()));
        // Provide `i32` as a placeholder type.
        RepeatedField::<'static, i32>::new(arena).inner
    }
    thread_local! {
        static REPEATED_FIELD: RepeatedFieldInner<'static> = new_repeated_field_inner();
    }

    REPEATED_FIELD.with(|inner| *inner)
}

/// Returns a static thread-local empty MapInner for use in a
/// MapView.
///
/// # Safety
/// The returned map must never be mutated.
///
/// TODO: Split MapInner into mut and const variants to
/// enforce safety. The returned array must never be mutated.
pub unsafe fn empty_map<K: ?Sized + 'static, V: ?Sized + 'static>() -> MapInner<'static, K, V> {
    fn new_map_inner() -> MapInner<'static, i32, i32> {
        // TODO: Consider creating empty map in C.
        let arena = Box::leak::<'static>(Box::new(Arena::new()));
        // Provide `i32` as a placeholder type.
        MapInner::<'static, i32, i32>::new(arena)
    }
    thread_local! {
        static MAP: MapInner<'static, i32, i32> = new_map_inner();
    }

    MAP.with(|inner| MapInner {
        raw: inner.raw,
        arena: inner.arena,
        _phantom_key: PhantomData,
        _phantom_value: PhantomData,
    })
}

#[derive(Debug)]
pub struct MapInner<'msg, K: ?Sized, V: ?Sized> {
    pub raw: RawMap,
    pub arena: &'msg Arena,
    pub _phantom_key: PhantomData<&'msg mut K>,
    pub _phantom_value: PhantomData<&'msg mut V>,
}

impl<'msg, K: ?Sized, V: ?Sized> Copy for MapInner<'msg, K, V> {}
impl<'msg, K: ?Sized, V: ?Sized> Clone for MapInner<'msg, K, V> {
    fn clone(&self) -> MapInner<'msg, K, V> {
        *self
    }
}

macro_rules! generate_map_key_ops_traits {
    ($($t:ty, $sized_t:ty;)*) => {
        paste! {
            $(
                pub trait [< MapWith $t:camel KeyOps >] {
                    type Value<'a>: Sized;

                    fn new_map(a: RawArena) -> RawMap;
                    fn clear(m: RawMap) {
                        unsafe { upb_Map_Clear(m) }
                    }
                    fn size(m: RawMap) -> usize {
                        unsafe { upb_Map_Size(m) }
                    }
                    fn insert(m: RawMap, a: RawArena, key: $sized_t, value: Self::Value<'_>) -> bool;
                    fn get<'a>(m: RawMap, key: $sized_t) -> Option<Self::Value<'a>>;
                    fn remove<'a>(m: RawMap, key: $sized_t) -> bool;
                }

                impl<'msg, V: [< MapWith $t:camel KeyOps >] + ?Sized> MapInner<'msg, $t, V> {

                    pub fn new(arena: &'msg mut Arena) -> Self {
                        MapInner {
                            raw: V::new_map(arena.raw()),
                            arena,
                            _phantom_key: PhantomData,
                            _phantom_value: PhantomData
                        }
                    }

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
                        V::insert(self.raw, self.arena.raw(), key, value)
                    }
                }
            )*
        }
    }
}

generate_map_key_ops_traits!(
    i32, i32;
    u32, u32;
    i64, i64;
    u64, u64;
    bool, bool;
    ProtoStr, &ProtoStr;
);

macro_rules! impl_scalar_map_key_op_for_scalar_values {
    ($key_t:ty, $key_msg_val:expr, $key_upb_tag:expr, $trait:ident for $($t:ty, $sized_t:ty, $msg_val:expr, $from_msg_val:expr, $upb_tag:expr, $zero_val:literal;)*) => {
         $(
            impl $trait for $t {
                type Value<'a> = $sized_t;

                fn new_map(a: RawArena) -> RawMap {
                    unsafe { upb_Map_New(a, $key_upb_tag, $upb_tag) }
                }

                fn insert(m: RawMap, a: RawArena, key: $key_t, value: Self::Value<'_>) -> bool {
                    unsafe {
                        upb_Map_Set(
                            m,
                            $key_msg_val(key),
                            $msg_val(value),
                            a
                        )
                    }
                }

                fn get<'a>(m: RawMap, key: $key_t) -> Option<Self::Value<'a>> {
                    let mut val = $msg_val($zero_val);
                    let found = unsafe {
                        upb_Map_Get(m, $key_msg_val(key), &mut val)
                    };
                    if !found {
                        return None;
                    }
                    Some($from_msg_val(val))
                }

                fn remove<'a>(m: RawMap, key: $key_t) -> bool {
                    let mut val = $msg_val($zero_val);
                    unsafe {
                        upb_Map_Delete(m, $key_msg_val(key), &mut val)
                    }
                }
            }
         )*
    }
}

macro_rules! scalar_to_msg {
    ($ufield:ident) => {
        |val| upb_MessageValue { $ufield: val }
    };
}

macro_rules! scalar_from_msg {
    ($ufield:ident) => {
        |msg: upb_MessageValue| unsafe { msg.$ufield }
    };
}

fn str_to_msg<'a>(val: impl Into<&'a ProtoStr>) -> upb_MessageValue {
    upb_MessageValue { str_val: val.into().as_bytes().into() }
}

fn msg_to_str<'a>(msg: upb_MessageValue) -> &'a ProtoStr {
    unsafe { ProtoStr::from_utf8_unchecked(msg.str_val.as_ref()) }
}

macro_rules! impl_map_key_ops_for_scalar_values {
    ($($t:ty, $t_sized:ty, $key_msg_val:expr, $upb_tag:expr;)*) => {
        paste! {
            $(
                impl_scalar_map_key_op_for_scalar_values!($t_sized, $key_msg_val, $upb_tag, [< MapWith $t:camel KeyOps >] for
                    f32, f32, scalar_to_msg!(float_val), scalar_from_msg!(float_val),  UpbCType::Float, 0f32;
                    f64, f64, scalar_to_msg!(double_val), scalar_from_msg!(double_val),  UpbCType::Double, 0f64;
                    i32, i32, scalar_to_msg!(int32_val), scalar_from_msg!(int32_val),  UpbCType::Int32, 0i32;
                    u32, u32, scalar_to_msg!(uint32_val), scalar_from_msg!(uint32_val), UpbCType::UInt32, 0u32;
                    i64, i64, scalar_to_msg!(int64_val), scalar_from_msg!(int64_val), UpbCType::Int64, 0i64;
                    u64, u64, scalar_to_msg!(uint64_val), scalar_from_msg!(uint64_val), UpbCType::UInt64, 0u64;
                    bool, bool, scalar_to_msg!(bool_val), scalar_from_msg!(bool_val), UpbCType::Bool, false;
                    ProtoStr, &'a ProtoStr, str_to_msg, msg_to_str, UpbCType::String, "";
                );
            )*
        }
    }
}

impl_map_key_ops_for_scalar_values!(
    i32, i32, scalar_to_msg!(int32_val), UpbCType::Int32;
    u32, u32, scalar_to_msg!(uint32_val), UpbCType::UInt32;
    i64, i64, scalar_to_msg!(int64_val), UpbCType::Int64;
    u64, u64, scalar_to_msg!(uint64_val), UpbCType::UInt64;
    bool, bool, scalar_to_msg!(bool_val), UpbCType::Bool;
    ProtoStr, &ProtoStr, |val: &ProtoStr| upb_MessageValue { str_val: val.as_bytes().into() }, UpbCType::String;
);

extern "C" {
    fn upb_Map_New(arena: RawArena, key_type: UpbCType, value_type: UpbCType) -> RawMap;
    fn upb_Map_Size(map: RawMap) -> usize;
    fn upb_Map_Set(
        map: RawMap,
        key: upb_MessageValue,
        value: upb_MessageValue,
        arena: RawArena,
    ) -> bool;
    fn upb_Map_Get(map: RawMap, key: upb_MessageValue, value: *mut upb_MessageValue) -> bool;
    fn upb_Map_Delete(
        map: RawMap,
        key: upb_MessageValue,
        removed_value: *mut upb_MessageValue,
    ) -> bool;
    fn upb_Map_Clear(map: RawMap);
}

#[cfg(test)]
pub(crate) fn new_map_i32_i64() -> MapInner<'static, i32, i64> {
    let arena = Box::leak::<'static>(Box::new(Arena::new()));
    MapInner::<'static, i32, i64>::new(arena)
}

#[cfg(test)]
pub(crate) fn new_map_str_str() -> MapInner<'static, ProtoStr, ProtoStr> {
    let arena = Box::leak::<'static>(Box::new(Arena::new()));
    MapInner::<'static, ProtoStr, ProtoStr>::new(arena)
}

#[cfg(test)]
mod tests {
    use super::*;
    use googletest::prelude::*;

    #[test]
    fn test_arena_new_and_free() {
        let arena = Arena::new();
        drop(arena);
    }

    #[test]
    fn test_serialized_data_roundtrip() {
        let arena = Arena::new();
        let original_data = b"Hello world";
        let len = original_data.len();

        let serialized_data = unsafe {
            SerializedData::from_raw_parts(
                arena,
                NonNull::new(original_data as *const _ as *mut _).unwrap(),
                len,
            )
        };
        assert_that!(&*serialized_data, eq(b"Hello world"));
    }

    #[test]
    fn i32_array() {
        let arena = Arena::new();
        let mut arr = RepeatedField::<i32>::new(&arena);
        assert_that!(arr.len(), eq(0));
        arr.push(1);
        assert_that!(arr.get(0), eq(Some(1)));
        assert_that!(arr.len(), eq(1));
        arr.set(0, 3);
        assert_that!(arr.get(0), eq(Some(3)));
        for i in 0..2048 {
            arr.push(i);
            assert_that!(arr.get(arr.len() - 1), eq(Some(i)));
        }
    }
    #[test]
    fn u32_array() {
        let mut arena = Arena::new();
        let mut arr = RepeatedField::<u32>::new(&mut arena);
        assert_that!(arr.len(), eq(0));
        arr.push(1);
        assert_that!(arr.get(0), eq(Some(1)));
        assert_that!(arr.len(), eq(1));
        arr.set(0, 3);
        assert_that!(arr.get(0), eq(Some(3)));
        for i in 0..2048 {
            arr.push(i);
            assert_that!(arr.get(arr.len() - 1), eq(Some(i)));
        }
    }

    #[test]
    fn i32_i32_map() {
        let mut arena = Arena::new();
        let mut map = MapInner::<'_, i32, i32>::new(&mut arena);
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
        let mut arena = Arena::new();
        let mut map = MapInner::<'_, i64, f64>::new(&mut arena);
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
        let mut arena = Arena::new();
        let mut map = MapInner::<'_, ProtoStr, ProtoStr>::new(&mut arena);
        assert_that!(map.size(), eq(0));

        map.insert("fizz".into(), "buzz".into());
        assert_that!(map.size(), eq(1));
        assert_that!(map.remove("fizz".into()), eq(true));
        map.clear();
        assert_that!(map.size(), eq(0));
    }

    #[test]
    fn u64_str_map() {
        let mut arena = Arena::new();
        let mut map = MapInner::<'_, u64, ProtoStr>::new(&mut arena);
        assert_that!(map.size(), eq(0));

        map.insert(1, "fizz".into());
        map.insert(2, "buzz".into());
        assert_that!(map.size(), eq(2));
        assert_that!(map.remove(1), eq(true));
        map.clear();
        assert_that!(map.size(), eq(0));
    }

    #[test]
    fn test_all_maps_can_be_constructed() {
        macro_rules! gen_proto_values {
            ($key_t:ty, $($value_t:ty),*) => {
                let mut arena = Arena::new();
                $(
                    let map = MapInner::<'_, $key_t, $value_t>::new(&mut arena);
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
