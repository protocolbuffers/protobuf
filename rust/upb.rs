// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

//! UPB FFI wrapper code for use by Rust Protobuf.

use crate::__internal::{Enum, Private};
use crate::{
    Map, MapIter, MapMut, MapView, Mut, ProtoStr, Proxied, ProxiedInMapValue, ProxiedInRepeated,
    Repeated, RepeatedMut, RepeatedView, View, ViewProxy,
};
use core::fmt::Debug;
use std::alloc::Layout;
use std::mem::{size_of, ManuallyDrop, MaybeUninit};
use std::ptr::{self, NonNull};
use std::slice;
use std::sync::OnceLock;

extern crate upb;

// Temporarily 'pub' since a lot of gencode is directly calling any of the ffi
// fns.
pub use upb::*;

pub type RawArena = upb::RawArena;
pub type RawMessage = upb::RawMessage;
pub type RawRepeatedField = upb::RawArray;
pub type RawMap = upb::RawMap;
pub type PtrAndLen = upb::StringView;

impl From<&ProtoStr> for PtrAndLen {
    fn from(s: &ProtoStr) -> Self {
        let bytes = s.as_bytes();
        Self { ptr: bytes.as_ptr(), len: bytes.len() }
    }
}

/// The scratch size of 64 KiB matches the maximum supported size that a
/// upb_Message can possibly be.
const UPB_SCRATCH_SPACE_BYTES: usize = 65_536;

/// Holds a zero-initialized block of memory for use by upb.
///
/// By default, if a message is not set in cpp, a default message is created.
/// upb departs from this and returns a null ptr. However, since contiguous
/// chunks of memory filled with zeroes are legit messages from upb's point of
/// view, we can allocate a large block and refer to that when dealing
/// with readonly access.
#[repr(C, align(8))] // align to UPB_MALLOC_ALIGN = 8
pub struct ScratchSpace([u8; UPB_SCRATCH_SPACE_BYTES]);
impl ScratchSpace {
    pub fn zeroed_block(_private: Private) -> RawMessage {
        static ZEROED_BLOCK: ScratchSpace = ScratchSpace([0; UPB_SCRATCH_SPACE_BYTES]);
        NonNull::from(&ZEROED_BLOCK).cast()
    }
}

pub type SerializedData = upb::OwnedArenaBox<[u8]>;

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
        parent_msg: MutatorMessageRef<'msg>,
        message_field_ptr: RawMessage,
    ) -> Self {
        MutatorMessageRef { msg: message_field_ptr, arena: parent_msg.arena }
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
    copy_bytes_in_arena(msg_ref.arena, val)
}

fn copy_bytes_in_arena<'msg>(arena: &'msg Arena, val: &'msg [u8]) -> &'msg [u8] {
    // SAFETY: the alignment of `[u8]` is less than `UPB_MALLOC_ALIGN`.
    let new_alloc = unsafe { arena.alloc(Layout::for_value(val)) };
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

/// The raw type-erased version of an owned `Repeated`.
#[derive(Debug)]
pub struct InnerRepeated {
    raw: RawRepeatedField,
    arena: Arena,
}

impl InnerRepeated {
    pub fn as_mut(&mut self) -> InnerRepeatedMut<'_> {
        InnerRepeatedMut::new(Private, self.raw, &self.arena)
    }

    pub fn raw(&self) -> RawRepeatedField {
        self.raw
    }

    pub fn arena(&self) -> &Arena {
        &self.arena
    }
}

/// The raw type-erased pointer version of `RepeatedMut`.
#[derive(Clone, Copy, Debug)]
pub struct InnerRepeatedMut<'msg> {
    pub(crate) raw: RawRepeatedField,
    arena: &'msg Arena,
}

impl<'msg> InnerRepeatedMut<'msg> {
    #[doc(hidden)]
    pub fn new(_private: Private, raw: RawRepeatedField, arena: &'msg Arena) -> Self {
        InnerRepeatedMut { raw, arena }
    }
}

macro_rules! impl_repeated_base {
    ($t:ty, $elem_t:ty, $ufield:ident, $upb_tag:expr) => {
        #[allow(dead_code)]
        fn repeated_new(_: Private) -> Repeated<$t> {
            let arena = Arena::new();
            Repeated::from_inner(InnerRepeated {
                raw: unsafe { upb_Array_New(arena.raw(), $upb_tag) },
                arena,
            })
        }
        #[allow(dead_code)]
        unsafe fn repeated_free(_: Private, _f: &mut Repeated<$t>) {
            // No-op: the memory will be dropped by the arena.
        }
        fn repeated_len(f: View<Repeated<$t>>) -> usize {
            unsafe { upb_Array_Size(f.as_raw(Private)) }
        }
        fn repeated_push(mut f: Mut<Repeated<$t>>, v: View<$t>) {
            let arena = f.raw_arena(Private);
            unsafe {
                assert!(upb_Array_Append(
                    f.as_raw(Private),
                    <$t as UpbTypeConversions>::to_message_value_copy_if_required(arena, v),
                    arena,
                ));
            }
        }
        fn repeated_clear(mut f: Mut<Repeated<$t>>) {
            unsafe {
                upb_Array_Resize(f.as_raw(Private), 0, f.raw_arena(Private));
            }
        }
        unsafe fn repeated_get_unchecked(f: View<Repeated<$t>>, i: usize) -> View<$t> {
            unsafe {
                <$t as UpbTypeConversions>::from_message_value(upb_Array_Get(f.as_raw(Private), i))
            }
        }
        unsafe fn repeated_set_unchecked(mut f: Mut<Repeated<$t>>, i: usize, v: View<$t>) {
            let arena = f.raw_arena(Private);
            unsafe {
                upb_Array_Set(
                    f.as_raw(Private),
                    i,
                    <$t as UpbTypeConversions>::to_message_value_copy_if_required(arena, v.into()),
                )
            }
        }
    };
}

macro_rules! impl_repeated_primitives {
    ($(($t:ty, $elem_t:ty, $ufield:ident, $upb_tag:expr)),* $(,)?) => {
        $(
            unsafe impl ProxiedInRepeated for $t {
                impl_repeated_base!($t, $elem_t, $ufield, $upb_tag);

                fn repeated_copy_from(src: View<Repeated<$t>>, mut dest: Mut<Repeated<$t>>) {
                    let arena = dest.raw_arena(Private);
                    // SAFETY:
                    // - `upb_Array_Resize` is unsafe but assumed to be always sound to call.
                    // - `copy_nonoverlapping` is unsafe but here we guarantee that both pointers
                    //   are valid, the pointers are `#[repr(u8)]`, and the size is correct.
                    unsafe {
                        if (!upb_Array_Resize(dest.as_raw(Private), src.len(), arena)) {
                            panic!("upb_Array_Resize failed.");
                        }
                        ptr::copy_nonoverlapping(
                          upb_Array_DataPtr(src.as_raw(Private)).cast::<u8>(),
                          upb_Array_MutableDataPtr(dest.as_raw(Private)).cast::<u8>(),
                          size_of::<$elem_t>() * src.len());
                    }
                }
            }
        )*
    }
}

macro_rules! impl_repeated_bytes {
    ($(($t:ty, $upb_tag:expr)),* $(,)?) => {
        $(
            unsafe impl ProxiedInRepeated for $t {
                impl_repeated_base!($t, PtrAndLen, str_val, $upb_tag);

                fn repeated_copy_from(src: View<Repeated<$t>>, mut dest: Mut<Repeated<$t>>) {
                    let len = src.len();
                    // SAFETY:
                    // - `upb_Array_Resize` is unsafe but assumed to be always sound to call.
                    // - `upb_Array` ensures its elements are never uninitialized memory.
                    // - The `DataPtr` and `MutableDataPtr` functions return pointers to spans
                    //   of memory that are valid for at least `len` elements of PtrAndLen.
                    // - `copy_nonoverlapping` is unsafe but here we guarantee that both pointers
                    //   are valid, the pointers are `#[repr(u8)]`, and the size is correct.
                    // - The bytes held within a valid array are valid.
                    unsafe {
                        let arena = ManuallyDrop::new(Arena::from_raw(dest.raw_arena(Private)));
                        if (!upb_Array_Resize(dest.as_raw(Private), src.len(), arena.raw())) {
                            panic!("upb_Array_Resize failed.");
                        }
                        let src_ptrs: &[PtrAndLen] = slice::from_raw_parts(
                            upb_Array_DataPtr(src.as_raw(Private)).cast(),
                            len
                        );
                        let dest_ptrs: &mut [PtrAndLen] = slice::from_raw_parts_mut(
                            upb_Array_MutableDataPtr(dest.as_raw(Private)).cast(),
                            len
                        );
                        for (src_ptr, dest_ptr) in src_ptrs.iter().zip(dest_ptrs) {
                            *dest_ptr = copy_bytes_in_arena(&arena, src_ptr.as_ref()).into();
                        }
                    }
                }
            }
        )*
    }
}

impl<'msg, T: ?Sized> RepeatedMut<'msg, T> {
    // Returns a `RawArena` which is live for at least `'msg`
    #[doc(hidden)]
    pub fn raw_arena(&mut self, _private: Private) -> RawArena {
        self.inner.arena.raw()
    }
}

impl_repeated_primitives!(
    // proxied type, element type, upb_MessageValue field name, upb::CType variant
    (bool, bool, bool_val, upb::CType::Bool),
    (f32, f32, float_val, upb::CType::Float),
    (f64, f64, double_val, upb::CType::Double),
    (i32, i32, int32_val, upb::CType::Int32),
    (u32, u32, uint32_val, upb::CType::UInt32),
    (i64, i64, int64_val, upb::CType::Int64),
    (u64, u64, uint64_val, upb::CType::UInt64),
);

impl_repeated_bytes!((ProtoStr, upb::CType::String), ([u8], upb::CType::Bytes),);

/// Copy the contents of `src` into `dest`.
///
/// # Safety
/// - `minitable` must be a pointer to the minitable for message `T`.
pub unsafe fn repeated_message_copy_from<T: ProxiedInRepeated>(
    src: View<Repeated<T>>,
    mut dest: Mut<Repeated<T>>,
    minitable: *const upb_MiniTable,
) {
    // SAFETY:
    // - `src.as_raw()` is a valid `const upb_Array*`.
    // - `dest.as_raw()` is a valid `upb_Array*`.
    // - Elements of `src` and have message minitable `$minitable$`.
    unsafe {
        let size = upb_Array_Size(src.as_raw(Private));
        if !upb_Array_Resize(dest.as_raw(Private), size, dest.raw_arena(Private)) {
            panic!("upb_Array_Resize failed.");
        }
        for i in 0..size {
            let src_msg = upb_Array_Get(src.as_raw(Private), i)
                .msg_val
                .expect("upb_Array* element should not be NULL");
            // Avoid the use of `upb_Array_DeepClone` as it creates an
            // entirely new `upb_Array*` at a new memory address.
            let cloned_msg = upb_Message_DeepClone(src_msg, minitable, dest.raw_arena(Private))
                .expect("upb_Message_DeepClone failed.");
            upb_Array_Set(dest.as_raw(Private), i, upb_MessageValue { msg_val: Some(cloned_msg) });
        }
    }
}

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
        let InnerRepeatedMut { arena, raw, .. } = repeated.inner;
        RepeatedMut::from_inner(private, InnerRepeatedMut { arena, raw })
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
    static EMPTY_MAP_VIEW: OnceLock<MapView<'static, bool, bool>> = OnceLock::new();

    // SAFETY:
    // - The map is empty and never mutated.
    // - The value type is never used.
    // - The size of the key type is used when `get()` computes the hash of the key.
    //   The map is empty, therefore it doesn't matter what hash is computed, but we
    //   have to use `bool` type as the smallest key possible (otherwise UPB would
    //   read more bytes than Rust allocated).
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

impl<'msg, K: ?Sized, V: ?Sized> MapMut<'msg, K, V> {
    // Returns a `RawArena` which is live for at least `'msg`
    #[doc(hidden)]
    pub fn raw_arena(&mut self, _private: Private) -> RawArena {
        self.inner.arena.raw()
    }
}

#[derive(Debug)]
pub struct InnerMap {
    pub(crate) raw: RawMap,
    arena: Arena,
}

impl InnerMap {
    pub fn new(_private: Private, raw: RawMap, arena: Arena) -> Self {
        Self { raw, arena }
    }

    pub fn as_mut(&mut self) -> InnerMapMut<'_> {
        InnerMapMut { raw: self.raw, arena: &self.arena }
    }
}

#[derive(Clone, Copy, Debug)]
pub struct InnerMapMut<'msg> {
    pub(crate) raw: RawMap,
    arena: &'msg Arena,
}

#[doc(hidden)]
impl<'msg> InnerMapMut<'msg> {
    pub fn new(_private: Private, raw: RawMap, arena: &'msg Arena) -> Self {
        InnerMapMut { raw, arena }
    }

    #[doc(hidden)]
    pub fn as_raw(&self, _private: Private) -> RawMap {
        self.raw
    }

    #[doc(hidden)]
    pub fn raw_arena(&self, _private: Private) -> RawArena {
        self.arena.raw()
    }
}

pub trait UpbTypeConversions: Proxied {
    fn upb_type() -> upb::CType;
    fn to_message_value(val: View<'_, Self>) -> upb_MessageValue;

    /// # Safety
    /// - `raw_arena` must point to a valid upb arena.
    unsafe fn to_message_value_copy_if_required(
        raw_arena: RawArena,
        val: View<'_, Self>,
    ) -> upb_MessageValue;

    /// # Safety
    /// - `msg` must be the correct variant for `Self`.
    /// - `msg` pointers must point to memory valid for `'msg` lifetime.
    unsafe fn from_message_value<'msg>(msg: upb_MessageValue) -> View<'msg, Self>;
}

macro_rules! impl_upb_type_conversions_for_scalars {
    ($($t:ty, $ufield:ident, $upb_tag:expr, $zero_val:literal;)*) => {
        $(
            impl UpbTypeConversions for $t {
                #[inline(always)]
                fn upb_type() -> upb::CType {
                    $upb_tag
                }

                #[inline(always)]
                fn to_message_value(val: View<'_, $t>) -> upb_MessageValue {
                    upb_MessageValue { $ufield: val }
                }

                #[inline(always)]
                unsafe fn to_message_value_copy_if_required(_: RawArena, val: View<'_, $t>) -> upb_MessageValue {
                    Self::to_message_value(val)
                }

                #[inline(always)]
                unsafe fn from_message_value<'msg>(msg: upb_MessageValue) -> View<'msg, $t> {
                    unsafe { msg.$ufield }
                }
            }
        )*
    };
}

impl_upb_type_conversions_for_scalars!(
    f32, float_val, upb::CType::Float, 0f32;
    f64, double_val, upb::CType::Double, 0f64;
    i32, int32_val, upb::CType::Int32, 0i32;
    u32, uint32_val, upb::CType::UInt32, 0u32;
    i64, int64_val, upb::CType::Int64, 0i64;
    u64, uint64_val, upb::CType::UInt64, 0u64;
    bool, bool_val, upb::CType::Bool, false;
);

impl UpbTypeConversions for [u8] {
    fn upb_type() -> upb::CType {
        upb::CType::Bytes
    }

    fn to_message_value(val: View<'_, [u8]>) -> upb_MessageValue {
        upb_MessageValue { str_val: val.into() }
    }

    unsafe fn to_message_value_copy_if_required(
        raw_arena: RawArena,
        val: View<'_, [u8]>,
    ) -> upb_MessageValue {
        // SAFETY: The arena memory is not freed due to `ManuallyDrop`.
        let arena = ManuallyDrop::new(unsafe { Arena::from_raw(raw_arena) });
        let copied = copy_bytes_in_arena(&arena, val);
        let msg_val = Self::to_message_value(copied);
        msg_val
    }

    unsafe fn from_message_value<'msg>(msg: upb_MessageValue) -> View<'msg, [u8]> {
        unsafe { msg.str_val.as_ref() }
    }
}

impl UpbTypeConversions for ProtoStr {
    fn upb_type() -> upb::CType {
        upb::CType::String
    }

    fn to_message_value(val: View<'_, ProtoStr>) -> upb_MessageValue {
        upb_MessageValue { str_val: val.as_bytes().into() }
    }

    unsafe fn to_message_value_copy_if_required(
        raw_arena: RawArena,
        val: View<'_, ProtoStr>,
    ) -> upb_MessageValue {
        // SAFETY: `raw_arena` is valid as promised by the caller
        unsafe {
            <[u8] as UpbTypeConversions>::to_message_value_copy_if_required(
                raw_arena,
                val.as_bytes(),
            )
        }
    }

    unsafe fn from_message_value<'msg>(msg: upb_MessageValue) -> View<'msg, ProtoStr> {
        unsafe { ProtoStr::from_utf8_unchecked(msg.str_val.as_ref()) }
    }
}

pub struct RawMapIter {
    // TODO: Replace this `RawMap` with the const type.
    map: RawMap,
    iter: usize,
}

impl RawMapIter {
    pub fn new(_private: Private, map: RawMap) -> Self {
        // SAFETY: __rust_proto_kUpb_Map_Begin is never modified
        RawMapIter { map, iter: unsafe { __rust_proto_kUpb_Map_Begin } }
    }

    /// # Safety
    /// - `self.map` must be valid, and remain valid while the return value is
    ///   in use.
    pub unsafe fn next_unchecked(
        &mut self,
        _private: Private,
    ) -> Option<(upb_MessageValue, upb_MessageValue)> {
        let mut key = MaybeUninit::uninit();
        let mut value = MaybeUninit::uninit();
        // SAFETY: the `map` is valid as promised by the caller
        unsafe { upb_Map_Next(self.map, key.as_mut_ptr(), value.as_mut_ptr(), &mut self.iter) }
            // SAFETY: if upb_Map_Next returns true, then key and value have been populated.
            .then(|| unsafe { (key.assume_init(), value.assume_init()) })
    }
}

macro_rules! impl_ProxiedInMapValue_for_non_generated_value_types {
    ($key_t:ty ; $($t:ty),*) => {
         $(
            impl ProxiedInMapValue<$key_t> for $t {
                fn map_new(_private: Private) -> Map<$key_t, Self> {
                    let arena = Arena::new();
                    let raw = unsafe {
                        upb_Map_New(arena.raw(),
                            <$key_t as UpbTypeConversions>::upb_type(),
                            <$t as UpbTypeConversions>::upb_type())
                    };
                    Map::from_inner(Private, InnerMap { raw, arena })
                }

                unsafe fn map_free(_private: Private, _map: &mut Map<$key_t, Self>) {
                    // No-op: the memory will be dropped by the arena.
                }

                fn map_clear(mut map: Mut<'_, Map<$key_t, Self>>) {
                    unsafe {
                        upb_Map_Clear(map.as_raw(Private));
                    }
                }

                fn map_len(map: View<'_, Map<$key_t, Self>>) -> usize {
                    unsafe {
                        upb_Map_Size(map.as_raw(Private))
                    }
                }

                fn map_insert(mut map: Mut<'_, Map<$key_t, Self>>, key: View<'_, $key_t>, value: View<'_, Self>) -> bool {
                    let arena = map.raw_arena(Private);
                    unsafe {
                        upb_Map_InsertAndReturnIfInserted(
                            map.as_raw(Private),
                            <$key_t as UpbTypeConversions>::to_message_value(key),
                            <$t as UpbTypeConversions>::to_message_value_copy_if_required(arena, value),
                            arena
                        )
                    }
                }

                fn map_get<'a>(map: View<'a, Map<$key_t, Self>>, key: View<'_, $key_t>) -> Option<View<'a, Self>> {
                    let mut val = MaybeUninit::uninit();
                    let found = unsafe {
                        upb_Map_Get(map.as_raw(Private), <$key_t as UpbTypeConversions>::to_message_value(key),
                            val.as_mut_ptr())
                    };
                    if !found {
                        return None;
                    }
                    Some(unsafe { <$t as UpbTypeConversions>::from_message_value(val.assume_init()) })
                }

                fn map_remove(mut map: Mut<'_, Map<$key_t, Self>>, key: View<'_, $key_t>) -> bool {
                    unsafe {
                        upb_Map_Delete(map.as_raw(Private),
                            <$key_t as UpbTypeConversions>::to_message_value(key),
                            ptr::null_mut())
                    }
                }

                fn map_iter(map: View<'_, Map<$key_t, Self>>) -> MapIter<'_, $key_t, Self> {
                    // SAFETY: View<Map<'_,..>> guarantees its RawMap outlives '_.
                    unsafe {
                        MapIter::from_raw(Private, RawMapIter::new(Private, map.as_raw(Private)))
                    }
                }

                fn map_iter_next<'a>(
                    iter: &mut MapIter<'a, $key_t, Self>
                ) -> Option<(View<'a, $key_t>, View<'a, Self>)> {
                    // SAFETY: MapIter<'a, ..> guarantees its RawMapIter outlives 'a.
                    unsafe { iter.as_raw_mut(Private).next_unchecked(Private) }
                        // SAFETY: MapIter<K, V> returns key and values message values
                        //         with the variants for K and V active.
                        .map(|(k, v)| unsafe {(
                            <$key_t as UpbTypeConversions>::from_message_value(k),
                            <$t as UpbTypeConversions>::from_message_value(v),
                        )})
                }
            }
         )*
    }
}

macro_rules! impl_ProxiedInMapValue_for_key_types {
    ($($t:ty),*) => {
        $(
            impl_ProxiedInMapValue_for_non_generated_value_types!(
                $t ; f32, f64, i32, u32, i64, u64, bool, ProtoStr, [u8]
            );
        )*
    }
}

impl_ProxiedInMapValue_for_key_types!(i32, u32, i64, u64, bool, ProtoStr);

/// `upb_Map_Insert`, but returns a `bool` for whether insert occurred.
///
/// Returns `true` if the entry was newly inserted.
///
/// # Panics
/// Panics if the arena is out of memory.
///
/// # Safety
/// The same as `upb_Map_Insert`:
/// - `map` must be a valid map.
/// - The `arena` must be valid and outlive the map.
/// - The inserted value must outlive the map.
#[allow(non_snake_case)]
pub unsafe fn upb_Map_InsertAndReturnIfInserted(
    map: RawMap,
    key: upb_MessageValue,
    value: upb_MessageValue,
    arena: RawArena,
) -> bool {
    match unsafe { upb_Map_Insert(map, key, value, arena) } {
        upb::MapInsertStatus::Inserted => true,
        upb::MapInsertStatus::Replaced => false,
        upb::MapInsertStatus::OutOfMemory => panic!("map arena is out of memory"),
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use googletest::prelude::*;

    #[test]
    fn assert_c_type_sizes() {
        // TODO: add these same asserts in C++.
        use std::ffi::c_void;
        use std::mem::{align_of, size_of};
        assert_that!(
            size_of::<upb_MessageValue>(),
            eq(size_of::<*const c_void>() + size_of::<usize>())
        );
        assert_that!(align_of::<upb_MessageValue>(), eq(align_of::<*const c_void>()));

        assert_that!(size_of::<upb_MutableMessageValue>(), eq(size_of::<*const c_void>()));
        assert_that!(align_of::<upb_MutableMessageValue>(), eq(align_of::<*const c_void>()));
    }
}
