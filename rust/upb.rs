// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

//! UPB FFI wrapper code for use by Rust Protobuf.

use crate::__internal::{Enum, Private, PtrAndLen, RawArena, RawMap, RawMessage, RawRepeatedField};
use crate::{
    Map, MapView, Mut, ProtoStr, Proxied, ProxiedInMapValue, ProxiedInRepeated, Repeated,
    RepeatedMut, RepeatedView, SettableValue, View, ViewProxy,
};
use core::fmt::Debug;
use paste::paste;
use std::alloc;
use std::alloc::Layout;
use std::cell::UnsafeCell;
use std::ffi::c_int;
use std::fmt;
use std::marker::PhantomData;
use std::mem::{size_of, MaybeUninit};
use std::ops::Deref;
use std::ptr::{self, NonNull};
use std::slice;
use std::sync::{Once, OnceLock};

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

impl SettableValue<[u8]> for SerializedData {
    fn set_on<'msg>(self, _private: Private, mut mutator: Mut<'msg, [u8]>)
    where
        [u8]: 'msg,
    {
        mutator.set(self.as_ref())
    }
}

// TODO: Investigate replacing this with direct access to UPB bits.
pub type BytesPresentMutData<'msg> = crate::vtable::RawVTableOptionalMutatorData<'msg, [u8]>;
pub type BytesAbsentMutData<'msg> = crate::vtable::RawVTableOptionalMutatorData<'msg, [u8]>;
pub type InnerBytesMut<'msg> = crate::vtable::RawVTableMutator<'msg, [u8]>;
pub type InnerPrimitiveMut<'msg, T> = crate::vtable::RawVTableMutator<'msg, T>;

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

    pub fn arena(&self, _private: Private) -> &Arena {
        self.arena
    }
}

pub fn copy_bytes_in_arena_if_needed_by_runtime<'msg>(
    msg_ref: MutatorMessageRef<'msg>,
    val: &'msg [u8],
) -> &'msg [u8] {
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

/// Opaque struct containing a upb_MiniTable.
///
/// This wrapper is a workaround until stabilization of [`extern type`].
/// TODO: convert to extern type once stabilized.
/// [`extern type`]: https://github.com/rust-lang/rust/issues/43467
#[repr(C)]
pub struct OpaqueMiniTable {
    // TODO: consider importing a minitable struct declared in
    // google3/third_party/upb/bits.
    _data: [u8; 0],
    _marker: std::marker::PhantomData<(*mut u8, ::std::marker::PhantomPinned)>,
}

extern "C" {
    pub fn upb_Message_DeepCopy(
        dst: RawMessage,
        src: RawMessage,
        mini_table: *const OpaqueMiniTable,
        arena: RawArena,
    );
}

/// The raw type-erased pointer version of `RepeatedMut`.
///
/// Contains a `upb_Array*` as well as `RawArena`, most likely that of the
/// containing message. upb requires a `RawArena` to perform mutations on
/// a repeated field.
///
/// An owned `Repeated` stores a `InnerRepeatedMut<'static>` and manages the
/// contained `RawArena`.
#[derive(Clone, Copy, Debug)]
pub struct InnerRepeatedMut<'msg> {
    pub(crate) raw: RawRepeatedField,
    // Storing a `RawArena` instead of `&Arena` allows this to be used for
    // both `RepeatedMut<T>` and `Repeated<T>`.
    arena: RawArena,
    _phantom: PhantomData<&'msg Arena>,
}

impl<'msg> InnerRepeatedMut<'msg> {
    #[doc(hidden)]
    #[allow(clippy::needless_pass_by_ref_mut)] // Sound construction requires mutable access.
    pub fn new(_private: Private, raw: RawRepeatedField, arena: &'msg Arena) -> Self {
        InnerRepeatedMut { raw, arena: arena.raw(), _phantom: PhantomData }
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
    ($(($t:ty, $ufield:ident, $upb_tag:expr)),* $(,)?) => {
        $(
            unsafe impl ProxiedInRepeated for $t {
                #[allow(dead_code)]
                fn repeated_new(_: Private) -> Repeated<$t> {
                    let arena = Arena::new();
                    let raw_arena = arena.raw();
                    std::mem::forget(arena);
                    unsafe {
                        Repeated::from_inner(InnerRepeatedMut {
                            raw: upb_Array_New(raw_arena, $upb_tag as c_int),
                            arena: raw_arena,
                            _phantom: PhantomData,
                        })
                    }
                }
                #[allow(dead_code)]
                unsafe fn repeated_free(_: Private, f: &mut Repeated<$t>) {
                    // Freeing the array itself is handled by `Arena::Drop`
                    // SAFETY:
                    // - `f.raw_arena()` is a live `upb_Arena*` as
                    // - This function is only called once for `f`
                    unsafe {
                        upb_Arena_Free(f.inner().arena);
                    }
                }
                fn repeated_len(f: View<Repeated<$t>>) -> usize {
                    unsafe { upb_Array_Size(f.as_raw(Private)) }
                }
                fn repeated_push(mut f: Mut<Repeated<$t>>, v: View<$t>) {
                    unsafe {
                        upb_Array_Append(
                            f.as_raw(Private),
                             upb_MessageValue { $ufield: v },
                            f.raw_arena(Private))
                    }
                }
                fn repeated_clear(mut f: Mut<Repeated<$t>>) {
                    unsafe { upb_Array_Resize(f.as_raw(Private), 0, f.raw_arena(Private)); }
                }
                unsafe fn repeated_get_unchecked(f: View<Repeated<$t>>, i: usize) -> View<$t> {
                    unsafe { upb_Array_Get(f.as_raw(Private), i).$ufield }
                }
                unsafe fn repeated_set_unchecked(mut f: Mut<Repeated<$t>>, i: usize, v: View<$t>) {
                    unsafe { upb_Array_Set(f.as_raw(Private), i, upb_MessageValue { $ufield: v.into() }) }
                }
                fn repeated_copy_from(src: View<Repeated<$t>>, mut dest: Mut<Repeated<$t>>) {
                    // SAFETY:
                    // - `upb_Array_Resize` is unsafe but assumed to be always sound to call.
                    // - `copy_nonoverlapping` is unsafe but here we guarantee that both pointers
                    //   are valid, the pointers are `#[repr(u8)]`, and the size is correct.
                    unsafe {
                        if (!upb_Array_Resize(dest.as_raw(Private), src.len(), dest.inner.arena)) {
                            panic!("upb_Array_Resize failed.");
                        }
                        ptr::copy_nonoverlapping(
                          upb_Array_DataPtr(src.as_raw(Private)).cast::<u8>(),
                          upb_Array_MutableDataPtr(dest.as_raw(Private)).cast::<u8>(),
                          size_of::<$t>() * src.len());
                    }
                }
            }
        )*
    }
}

impl<'msg, T: ?Sized> RepeatedMut<'msg, T> {
    // Returns a `RawArena` which is live for at least `'msg`
    #[doc(hidden)]
    pub fn raw_arena(&self, _private: Private) -> RawArena {
        self.inner.arena
    }
}

impl_repeated_primitives!(
    (bool, bool_val, UpbCType::Bool),
    (f32, float_val, UpbCType::Float),
    (f64, double_val, UpbCType::Double),
    (i32, int32_val, UpbCType::Int32),
    (u32, uint32_val, UpbCType::UInt32),
    (i64, int64_val, UpbCType::Int64),
    (u64, uint64_val, UpbCType::UInt64),
);

/// Cast a `RepeatedView<SomeEnum>` to `RepeatedView<i32>`.
pub fn cast_enum_repeated_view<E: Enum + ProxiedInRepeated>(
    private: Private,
    repeated: RepeatedView<E>,
) -> RepeatedView<i32> {
    // SAFETY: Reading an enum array as an i32 array is sound.
    unsafe { RepeatedView::from_raw(private, repeated.as_raw(Private)) }
}

/// Cast a `RepeatedMut<SomeEnum>` to `RepeatedMut<i32>`.
///
/// Writing an unknown value is sound because all enums
/// are representationally open.
pub fn cast_enum_repeated_mut<E: Enum + ProxiedInRepeated>(
    private: Private,
    repeated: RepeatedMut<E>,
) -> RepeatedMut<i32> {
    // SAFETY:
    // - Reading an enum array as an i32 array is sound.
    // - No shared mutation is possible through the output.
    unsafe {
        let InnerRepeatedMut { arena, raw, .. } = repeated.into_inner();
        RepeatedMut::from_inner(private, InnerRepeatedMut { arena, raw, _phantom: PhantomData })
    }
}

/// Returns a static empty RepeatedView.
pub fn empty_array<T: ?Sized + ProxiedInRepeated>() -> RepeatedView<'static, T> {
    // TODO: Consider creating a static empty array in C.

    // Use `i32` for a shared empty repeated for all repeated types in the program.
    static EMPTY_REPEATED_VIEW: OnceLock<RepeatedView<'static, i32>> = OnceLock::new();

    // SAFETY:
    // - Because the repeated is never mutated, the repeated type is unused and
    //   therefore valid for `T`.
    // - The view is leaked for `'static`.
    unsafe {
        RepeatedView::from_raw(
            Private,
            EMPTY_REPEATED_VIEW
                .get_or_init(|| Box::leak(Box::new(Repeated::new())).as_mut().into_view())
                .as_raw(Private),
        )
    }
}

/// Returns a static empty MapView.
pub fn empty_map<K, V>() -> MapView<'static, K, V>
where
    K: Proxied + ?Sized,
    V: ProxiedInMapValue<K> + ?Sized,
{
    // TODO: Consider creating a static empty map in C.

    // Use `i32` for a shared empty map for all map types.
    static EMPTY_MAP_VIEW: OnceLock<MapView<'static, i32, i32>> = OnceLock::new();

    // SAFETY:
    // - Because the map is never mutated, the map type is unused and therefore
    //   valid for `T`.
    // - The view is leaked for `'static`.
    unsafe {
        MapView::from_raw(
            Private,
            EMPTY_MAP_VIEW
                .get_or_init(|| Box::leak(Box::new(Map::new())).as_mut().into_view())
                .as_raw(Private),
        )
    }
}

#[derive(Clone, Copy, Debug)]
pub struct InnerMapMut<'msg> {
    pub(crate) raw: RawMap,
    raw_arena: RawArena,
    _phantom: PhantomData<&'msg Arena>,
}

impl<'msg> InnerMapMut<'msg> {
    pub fn new(_private: Private, raw: RawMap, raw_arena: RawArena) -> Self {
        InnerMapMut { raw, raw_arena, _phantom: PhantomData }
    }
}

macro_rules! impl_ProxiedInMapValue_for_non_generated_value_types {
    ($key_t:ty, $key_msg_val:expr, $key_upb_tag:expr, for $($t:ty, $msg_val:expr, $from_msg_val:expr, $upb_tag:expr, $zero_val:literal;)*) => {
         $(
            impl ProxiedInMapValue<$key_t> for $t {
                fn map_new(_private: Private) -> Map<$key_t, Self> {
                    let arena = Arena::new();
                    let raw_arena = arena.raw();
                    std::mem::forget(arena);

                    unsafe {
                        Map::from_inner(
                            Private,
                            InnerMapMut {
                                raw: upb_Map_New(raw_arena, $key_upb_tag, $upb_tag),
                                raw_arena,
                                _phantom: PhantomData
                            }
                        )
                    }
                }

                unsafe fn map_free(_private: Private, map: &mut Map<$key_t, Self>) {
                    // SAFETY:
                    // - `map.inner.raw_arena` is a live `upb_Arena*`
                    // - This function is only called once for `map` in `Drop`.
                    unsafe {
                        upb_Arena_Free(map.inner.raw_arena);
                    }
                }

                fn map_clear(map: Mut<'_, Map<$key_t, Self>>) {
                    unsafe {
                        upb_Map_Clear(map.inner.raw);
                    }
                }

                fn map_len(map: View<'_, Map<$key_t, Self>>) -> usize {
                    unsafe {
                        upb_Map_Size(map.raw)
                    }
                }

                fn map_insert(map: Mut<'_, Map<$key_t, Self>>, key: View<'_, $key_t>, value: View<'_, Self>) -> bool {
                    unsafe {
                        upb_Map_Set(
                            map.inner.raw,
                            $key_msg_val(key),
                            $msg_val(value),
                            map.inner.raw_arena
                        )
                    }
                }

                fn map_get<'a>(map: View<'a, Map<$key_t, Self>>, key: View<'_, $key_t>) -> Option<View<'a, Self>> {
                    let mut val = $msg_val($zero_val);
                    let found = unsafe {
                        upb_Map_Get(map.raw, ($key_msg_val)(key), &mut val)
                    };
                    if !found {
                        return None;
                    }
                    Some($from_msg_val(val))
                }

                fn map_remove(map: Mut<'_, Map<$key_t, Self>>, key: View<'_, $key_t>) -> bool {
                    let mut val = $msg_val($zero_val);
                    unsafe {
                        upb_Map_Delete(map.inner.raw, $key_msg_val(key), &mut val)
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

fn str_to_msg<'msg>(val: impl Into<&'msg ProtoStr>) -> upb_MessageValue {
    upb_MessageValue { str_val: val.into().as_bytes().into() }
}

fn msg_to_str<'msg>(msg: upb_MessageValue) -> &'msg ProtoStr {
    unsafe { ProtoStr::from_utf8_unchecked(msg.str_val.as_ref()) }
}

macro_rules! impl_ProxiedInMapValue_for_key_types {
    ($($t:ty, $t_sized:ty, $key_msg_val:expr, $upb_tag:expr;)*) => {
        paste! {
            $(
                impl_ProxiedInMapValue_for_non_generated_value_types!($t, $key_msg_val, $upb_tag, for
                    f32, scalar_to_msg!(float_val), scalar_from_msg!(float_val),  UpbCType::Float, 0f32;
                    f64, scalar_to_msg!(double_val), scalar_from_msg!(double_val),  UpbCType::Double, 0f64;
                    i32, scalar_to_msg!(int32_val), scalar_from_msg!(int32_val),  UpbCType::Int32, 0i32;
                    u32, scalar_to_msg!(uint32_val), scalar_from_msg!(uint32_val), UpbCType::UInt32, 0u32;
                    i64, scalar_to_msg!(int64_val), scalar_from_msg!(int64_val), UpbCType::Int64, 0i64;
                    u64, scalar_to_msg!(uint64_val), scalar_from_msg!(uint64_val), UpbCType::UInt64, 0u64;
                    bool, scalar_to_msg!(bool_val), scalar_from_msg!(bool_val), UpbCType::Bool, false;
                    ProtoStr, str_to_msg, msg_to_str, UpbCType::String, "";
                );
            )*
        }
    }
}

impl_ProxiedInMapValue_for_key_types!(
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
}
