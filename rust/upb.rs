// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

//! UPB FFI wrapper code for use by Rust Protobuf.

use crate::__internal::{Enum, MatcherEq, Private, SealedInternal};
use crate::{
    AsMut, AsView, Clear, ClearAndParse, CopyFrom, IntoProxied, Map, MapIter, MapMut, MapView,
    MergeFrom, Message, MessageMut, MessageMutInterop, MessageView, MessageViewInterop, Mut,
    OwnedMessageInterop, ParseError, ProtoBytes, ProtoStr, ProtoString, Proxied, ProxiedInMapValue,
    ProxiedInRepeated, Repeated, RepeatedMut, RepeatedView, Serialize, SerializeError, TakeFrom,
    View,
};
use std::fmt::Debug;
use std::marker::PhantomData;
use std::mem::{size_of, ManuallyDrop, MaybeUninit};
use std::ptr::{self, NonNull};
use std::slice;
use std::sync::OnceLock;

#[cfg(bzl)]
extern crate upb;
#[cfg(not(bzl))]
use crate::upb;

// Temporarily 'pub' since the gencode is directly referencing various parts of upb.
pub use upb::upb_MiniTableEnum_Build;
pub use upb::upb_MiniTable_Build;
pub use upb::upb_MiniTable_Link;
pub use upb::Arena;
pub use upb::AssociatedMiniTable;
pub use upb::AssociatedMiniTableEnum;
pub use upb::MessagePtr;
pub use upb::RawMiniTable;
pub use upb::RawMiniTableEnum;
use upb::*;

pub fn debug_string<T: UpbGetMessagePtr>(msg: &T) -> String {
    let ptr = msg.get_ptr(Private);
    // SAFETY: `ptr` is legally dereferenceable.
    unsafe { upb::debug_string(ptr) }
}

pub(crate) type RawRepeatedField = upb::RawArray;
pub(crate) type RawMap = upb::RawMap;
pub(crate) type PtrAndLen = upb::StringView;

// This struct represents a raw minitable pointer. We need it to be Send and Sync so that we can
// store it in a static OnceLock for lazy initialization of minitables. It should not be used for
// any other purpose.
pub struct MiniTablePtr(pub RawMiniTable);
unsafe impl Send for MiniTablePtr {}
unsafe impl Sync for MiniTablePtr {}

// Same as above, but for enum minitables.
pub struct MiniTableEnumPtr(pub RawMiniTableEnum);
unsafe impl Send for MiniTableEnumPtr {}
unsafe impl Sync for MiniTableEnumPtr {}

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
struct ScratchSpace([u8; UPB_SCRATCH_SPACE_BYTES]);
impl ScratchSpace {
    pub fn zeroed_block() -> RawMessage {
        static ZEROED_BLOCK: ScratchSpace = ScratchSpace([0; UPB_SCRATCH_SPACE_BYTES]);
        NonNull::from(&ZEROED_BLOCK).cast()
    }
}

thread_local! {
    // We need to avoid dropping this Arena, because we use it to build mini tables that
    // effectively have 'static lifetimes.
    pub static THREAD_LOCAL_ARENA: ManuallyDrop<Arena> = ManuallyDrop::new(Arena::new());
}

#[doc(hidden)]
pub type SerializedData = upb::OwnedArenaBox<[u8]>;

impl SealedInternal for SerializedData {}

impl IntoProxied<ProtoBytes> for SerializedData {
    fn into_proxied(self, _private: Private) -> ProtoBytes {
        ProtoBytes { inner: InnerProtoString(self) }
    }
}

#[derive(Debug)]
#[doc(hidden)]
pub struct OwnedMessageInner<T> {
    ptr: MessagePtr<T>,
    arena: Arena,
}

impl<T: Message + AssociatedMiniTable> Default for OwnedMessageInner<T> {
    fn default() -> Self {
        Self::new()
    }
}

impl<T: Message + AssociatedMiniTable> OwnedMessageInner<T> {
    pub fn new() -> Self {
        let arena = Arena::new();
        let ptr = MessagePtr::new(&arena).expect("alloc should never fail");
        Self { ptr, arena }
    }

    /// # Safety
    /// - The underlying pointer must of type `T` and be allocated on `arena`.
    pub unsafe fn wrap_raw(raw: RawMessage, arena: Arena) -> Self {
        // SAFETY:
        // - Caller guaranteed `raw` is valid and of type `T`
        let ptr = unsafe { MessagePtr::wrap(raw) };
        OwnedMessageInner { ptr, arena }
    }

    pub fn ptr_mut(&mut self) -> MessagePtr<T> {
        self.ptr
    }

    pub fn ptr(&self) -> MessagePtr<T> {
        self.ptr
    }

    pub fn raw(&self) -> RawMessage {
        self.ptr.raw()
    }

    #[allow(clippy::needless_pass_by_ref_mut)] // Sound access requires mutable access.
    pub fn arena(&mut self) -> &Arena {
        &self.arena
    }
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
/// - Store one pointer here, `&'msg OwnedMessageInner`, where `OwnedMessageInner` stores
///   a `RawMessage` and an `Arena`. This would require all generated messages
///   to store `OwnedMessageInner`, and since their mutators need to be able to
///   generate `BytesMut`, would also require `BytesMut` to store a `&'msg
///   OwnedMessageInner` since they can't store an owned `Arena`.
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
#[doc(hidden)]
pub struct MessageMutInner<'msg, T> {
    ptr: MessagePtr<T>,
    arena: &'msg Arena,
}

impl<'msg, T: Message + AssociatedMiniTable> Clone for MessageMutInner<'msg, T> {
    fn clone(&self) -> Self {
        *self
    }
}
impl<'msg, T: Message + AssociatedMiniTable> Copy for MessageMutInner<'msg, T> {}

impl<'msg, T: Message + AssociatedMiniTable> MessageMutInner<'msg, T> {
    /// # Safety
    /// - `msg` must be a valid `RawMessage`
    /// - `arena` must hold the memory for `msg`
    pub unsafe fn wrap_raw(raw: RawMessage, arena: &'msg Arena) -> Self {
        // SAFETY:
        // - Caller guaranteed `raw` is valid and of type `T`
        let ptr = unsafe { MessagePtr::wrap(raw) };
        MessageMutInner { ptr, arena }
    }

    #[allow(clippy::needless_pass_by_ref_mut)] // Sound construction requires mutable access.
    pub fn mut_of_owned(msg: &'msg mut OwnedMessageInner<T>) -> Self {
        MessageMutInner { ptr: msg.ptr, arena: &msg.arena }
    }

    pub fn from_parent<ParentT>(
        parent_msg: MessageMutInner<'msg, ParentT>,
        ptr: MessagePtr<T>,
    ) -> Self {
        MessageMutInner { ptr, arena: parent_msg.arena }
    }

    pub fn ptr_mut(&mut self) -> MessagePtr<T> {
        self.ptr
    }

    pub fn ptr(&self) -> MessagePtr<T> {
        self.ptr
    }

    pub fn raw(&self) -> RawMessage {
        self.ptr.raw()
    }

    pub fn arena(&self) -> &Arena {
        self.arena
    }
}

#[derive(Debug)]
#[doc(hidden)]
pub struct MessageViewInner<'msg, T> {
    ptr: MessagePtr<T>,
    _phantom: PhantomData<&'msg ()>,
}

impl<'msg, T: Message + AssociatedMiniTable> Clone for MessageViewInner<'msg, T> {
    fn clone(&self) -> Self {
        *self
    }
}
impl<'msg, T: Message + AssociatedMiniTable> Copy for MessageViewInner<'msg, T> {}

impl<'msg, T: Message + AssociatedMiniTable> MessageViewInner<'msg, T> {
    /// # Safety
    /// - The underlying pointer must live as long as `'msg`.
    pub unsafe fn wrap(ptr: MessagePtr<T>) -> Self {
        // SAFETY:
        // - Caller guaranteed `raw` is valid
        MessageViewInner { ptr, _phantom: PhantomData }
    }

    /// # Safety
    /// - The underlying pointer must of type `T` and live as long as `'msg`.
    pub unsafe fn wrap_raw(raw: RawMessage) -> Self {
        // SAFETY:
        // - Caller guaranteed `raw` is valid and of type `T`
        let ptr = unsafe { MessagePtr::wrap(raw) };
        MessageViewInner { ptr, _phantom: PhantomData }
    }

    #[allow(clippy::needless_pass_by_ref_mut)] // Sound construction requires mutable access.
    pub fn view_of_owned(owned: &'msg OwnedMessageInner<T>) -> Self {
        MessageViewInner { ptr: owned.ptr, _phantom: PhantomData }
    }

    #[allow(clippy::needless_pass_by_ref_mut)] // Sound construction requires mutable access.
    pub fn view_of_mut(msg_mut: MessageMutInner<'msg, T>) -> Self {
        MessageViewInner { ptr: msg_mut.ptr, _phantom: PhantomData }
    }

    pub fn ptr(&self) -> MessagePtr<T> {
        self.ptr
    }

    pub fn raw(&self) -> RawMessage {
        self.ptr.raw()
    }
}

impl<T: Message + AssociatedMiniTable> Default for MessageViewInner<'static, T> {
    fn default() -> Self {
        unsafe {
            // SAFETY:
            // - `ScratchSpace::zeroed_block()` is a valid const `RawMessage` for all possible T.
            // - `ScratchSpace::zeroed_block()' has 'static lifetime.
            Self::wrap_raw(ScratchSpace::zeroed_block())
        }
    }
}

/// Kernel-specific owned `string` and `bytes` field type.
#[doc(hidden)]
pub struct InnerProtoString(OwnedArenaBox<[u8]>);

impl InnerProtoString {
    pub(crate) fn as_bytes(&self) -> &[u8] {
        &self.0
    }

    #[doc(hidden)]
    pub fn into_raw_parts(self) -> (PtrAndLen, Arena) {
        let (data_ptr, arena) = self.0.into_parts();
        (unsafe { data_ptr.as_ref().into() }, arena)
    }
}

impl From<&[u8]> for InnerProtoString {
    fn from(val: &[u8]) -> InnerProtoString {
        let arena = Arena::new();
        let in_arena_copy = arena.copy_slice_in(val).unwrap();
        // SAFETY:
        // - `in_arena_copy` is valid slice that will live for `arena`'s lifetime and
        //   this is the only reference in the program to it.
        // - `in_arena_copy` is a pointer into an allocation on `arena`
        InnerProtoString(unsafe { OwnedArenaBox::new(Into::into(in_arena_copy), arena) })
    }
}

/// The raw type-erased version of an owned `Repeated`.
#[derive(Debug)]
#[doc(hidden)]
pub struct InnerRepeated {
    raw: RawRepeatedField,
    arena: Arena,
}

impl InnerRepeated {
    pub fn as_mut(&mut self) -> InnerRepeatedMut<'_> {
        InnerRepeatedMut::new(self.raw, &self.arena)
    }

    pub fn raw(&self) -> RawRepeatedField {
        self.raw
    }

    pub fn arena(&self) -> &Arena {
        &self.arena
    }

    /// # Safety
    /// - `raw` must be a valid `RawRepeatedField`
    pub unsafe fn from_raw_parts(raw: RawRepeatedField, arena: Arena) -> Self {
        Self { raw, arena }
    }
}

/// The raw type-erased pointer version of `RepeatedMut`.
#[derive(Clone, Copy, Debug)]
#[doc(hidden)]
pub struct InnerRepeatedMut<'msg> {
    pub(crate) raw: RawRepeatedField,
    arena: &'msg Arena,
}

impl<'msg> InnerRepeatedMut<'msg> {
    #[doc(hidden)]
    pub fn new(raw: RawRepeatedField, arena: &'msg Arena) -> Self {
        InnerRepeatedMut { raw, arena }
    }
}

unsafe impl<T> ProxiedInRepeated for T
where
    T: EntityType + UpbTypeConversions<T::Tag>,
{
    fn repeated_new(_private: Private) -> Repeated<Self> {
        let arena = Arena::new();
        Repeated::from_inner(Private, unsafe {
            InnerRepeated::from_raw_parts(upb_Array_New(arena.raw(), T::upb_type()), arena)
        })
    }

    unsafe fn repeated_free(_private: Private, _repeated: &mut Repeated<Self>) {
        // No-op: the memory will be dropped by the arena.
    }

    fn repeated_len(repeated: View<Repeated<Self>>) -> usize {
        // SAFETY: `repeated.as_raw()` is a valid `upb_Array*`.
        unsafe { upb_Array_Size(repeated.as_raw(Private)) }
    }

    fn repeated_push(mut repeated: Mut<Repeated<Self>>, val: impl IntoProxied<Self>) {
        // SAFETY:
        // - `repeated.as_raw()` is a valid `upb_Array*`.
        // - `msg_ptr` is a valid `upb_Message*`.
        unsafe {
            upb_Array_Append(
                repeated.as_raw(Private),
                T::into_message_value_fuse_if_required(
                    repeated.raw_arena(Private),
                    val.into_proxied(Private),
                ),
                repeated.raw_arena(Private),
            );
        };
    }

    fn repeated_clear(mut repeated: Mut<Repeated<Self>>) {
        // SAFETY:
        // - `repeated.as_raw()` is a valid `upb_Array*`.
        unsafe { upb_Array_Resize(repeated.as_raw(Private), 0, repeated.raw_arena(Private)) };
    }

    unsafe fn repeated_get_unchecked<'a>(
        repeated: View<'a, Repeated<Self>>,
        index: usize,
    ) -> View<'a, Self> {
        // SAFETY:
        // - `repeated.as_raw()` is a valid `const upb_Array*`.
        // - `index < len(repeated)` is promised by the caller.
        let val = unsafe { upb_Array_Get(repeated.as_raw(Private), index) };
        // SAFETY:
        // - `val` has the correct variant for Self.
        // - `val` is valid for `'a` lifetime.
        unsafe { Self::from_message_value(val) }
    }

    unsafe fn repeated_get_mut_unchecked<'a>(
        mut repeated: Mut<'a, Repeated<Self>>,
        index: usize,
    ) -> Mut<'a, Self>
    where
        Self: Message,
    {
        // SAFETY:
        // - `repeated.as_raw()` is a valid `upb_Array*`.
        // - `repeated` is a an array of message-valued elements.
        // - `index < len(repeated)` is promised by the caller.
        let val = unsafe { upb_Array_GetMutable(repeated.as_raw(Private), index) };
        // SAFETY:
        // - `val` is the correct variant for `Self`.
        // - `val` is valid for `'a` lifetime.
        unsafe { Self::from_message_mut(val, repeated.arena(Private)) }
    }

    unsafe fn repeated_set_unchecked(
        mut repeated: Mut<Repeated<Self>>,
        index: usize,
        val: impl IntoProxied<Self>,
    ) {
        unsafe {
            upb_Array_Set(
                repeated.as_raw(Private),
                index,
                T::into_message_value_fuse_if_required(
                    repeated.raw_arena(Private),
                    val.into_proxied(Private),
                ),
            )
        }
    }

    fn repeated_copy_from(src: View<Repeated<Self>>, mut dest: Mut<Repeated<Self>>) {
        // SAFETY:
        // - `src.as_raw()` and `dest.as_raw()` are both valid arrays of `Self`.
        // - `dest.as_raw()` is mutable.
        // - `dest.raw_arena()` will outlive `dest.as_raw()`.
        unsafe {
            Self::copy_repeated(src.as_raw(Private), dest.as_raw(Private), dest.raw_arena(Private));
        }
    }

    fn repeated_reserve(mut repeated: Mut<Repeated<Self>>, additional: usize) {
        // SAFETY:
        // - `repeated.as_raw()` is a valid `upb_Array*`.
        unsafe {
            let size = upb_Array_Size(repeated.as_raw(Private));
            upb_Array_Reserve(
                repeated.as_raw(Private),
                size + additional,
                repeated.raw_arena(Private),
            );
        }
    }
}

impl<'msg, T> RepeatedMut<'msg, T> {
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

/// Cast a `RepeatedView<SomeEnum>` to `RepeatedView<i32>`.
pub fn cast_enum_repeated_view<E: Enum + ProxiedInRepeated>(
    repeated: RepeatedView<E>,
) -> RepeatedView<i32> {
    // SAFETY: Reading an enum array as an i32 array is sound.
    unsafe { RepeatedView::from_raw(Private, repeated.as_raw(Private)) }
}

/// Cast a `RepeatedMut<SomeEnum>` to `RepeatedMut<i32>`.
///
/// Writing an unknown value is sound because all enums
/// are representationally open.
pub fn cast_enum_repeated_mut<E: Enum + ProxiedInRepeated>(
    repeated: RepeatedMut<E>,
) -> RepeatedMut<i32> {
    // SAFETY:
    // - Reading an enum array as an i32 array is sound.
    // - No shared mutation is possible through the output.
    unsafe {
        let InnerRepeatedMut { arena, raw, .. } = repeated.inner;
        RepeatedMut::from_inner(Private, InnerRepeatedMut { arena, raw })
    }
}

/// Cast a `RepeatedMut<SomeEnum>` to `RepeatedMut<i32>` and call
/// repeated_reserve.
pub fn reserve_enum_repeated_mut<E: Enum + ProxiedInRepeated>(
    repeated: RepeatedMut<E>,
    additional: usize,
) {
    let int_repeated = cast_enum_repeated_mut(repeated);
    ProxiedInRepeated::repeated_reserve(int_repeated, additional);
}

pub fn new_enum_repeated<E: Enum + ProxiedInRepeated>() -> Repeated<E> {
    let arena = Arena::new();
    // SAFETY:
    // - `upb_Array_New` is unsafe but assumed to be sound when called on a valid
    //   arena.
    unsafe {
        let raw = upb_Array_New(arena.raw(), upb::CType::Int32);
        Repeated::from_inner(Private, InnerRepeated::from_raw_parts(raw, arena))
    }
}

pub fn free_enum_repeated<E: Enum + ProxiedInRepeated>(_repeated: &mut Repeated<E>) {
    // No-op: the memory will be dropped by the arena.
}

/// Returns a static empty RepeatedView.
pub fn empty_array<T: ProxiedInRepeated>() -> RepeatedView<'static, T> {
    // TODO: Consider creating a static empty array in C.

    // Use `i32` for a shared empty repeated for all repeated types in the program.
    static EMPTY_REPEATED_VIEW: OnceLock<Repeated<i32>> = OnceLock::new();

    // SAFETY:
    // - Because the repeated is never mutated, the repeated type is unused and
    //   therefore valid for `T`.
    unsafe {
        RepeatedView::from_raw(
            Private,
            EMPTY_REPEATED_VIEW.get_or_init(Repeated::new).as_view().as_raw(Private),
        )
    }
}

/// Returns a static empty MapView.
pub fn empty_map<K, V>() -> MapView<'static, K, V>
where
    K: Proxied,
    V: ProxiedInMapValue<K>,
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

/// This trait allows us to associate a tag with each type of protobuf entity. The tag indicates
/// whether the entity is a message, enum, or primitive. The main purpose of this is to allow us to
/// have separate blanket implementations of UpbTypeConversions for messages and enums.
pub trait EntityType {
    type Tag;
}

pub struct MessageTag;
pub struct EnumTag;
pub struct PrimitiveTag;

macro_rules! impl_entity_type_for_primitives {
    ($($t:ty,)*) => {
        $(
            impl EntityType for $t {
                type Tag = PrimitiveTag;
            }
        )*
    };
}

impl_entity_type_for_primitives!(f32, f64, i32, u32, i64, u64, bool, ProtoBytes, ProtoString,);

pub trait UpbTypeConversions<Tag>: Proxied {
    fn upb_type() -> upb::CType;

    fn to_message_value(val: View<'_, Self>) -> upb_MessageValue;

    /// # Safety
    /// - `raw_arena` must point to a valid upb arena.
    unsafe fn into_message_value_fuse_if_required(
        raw_arena: RawArena,
        val: Self,
    ) -> upb_MessageValue;

    /// # Safety
    /// - `msg_val` must be the correct variant for `Self`.
    /// - `msg_val` pointers must point to memory valid for `'msg` lifetime.
    /// - If `Self` is a closed enum, then `msg_val.int32_val` must be a valid enum entry.
    unsafe fn from_message_value<'msg>(msg_val: upb_MessageValue) -> View<'msg, Self>;

    /// # Safety
    /// - `raw` must be the correct variant for `Self`.
    /// - `raw` pointers must point to memory valid for `'msg` lifetime.
    #[allow(unused_variables)]
    unsafe fn from_message_mut<'msg>(raw: RawMessage, arena: &'msg Arena) -> Mut<'msg, Self>
    where
        Self: Message,
    {
        panic!("mut_from_message_value is only implemented for messages.")
    }

    /// # Safety
    /// - `src` must be a valid array of `Self`.
    /// - `dest` must be a valid mutable array of `Self`.
    /// - `arena` must point to an arena that will outlive `dest`.
    unsafe fn copy_repeated(src: RawArray, dest: RawArray, arena: RawArena);
}

impl<T> UpbTypeConversions<MessageTag> for T
where
    Self: Message + AssociatedMiniTable + UpbGetArena + UpbGetMessagePtr,
    for<'a> View<'a, Self>: UpbGetMessagePtr + MessageViewInterop<'a>,
    for<'a> Mut<'a, Self>: From<MessageMutInner<'a, Self>>,
{
    fn upb_type() -> CType {
        CType::Message
    }

    fn to_message_value(val: View<'_, Self>) -> upb_MessageValue {
        upb_MessageValue { msg_val: Some(val.get_ptr(Private).raw()) }
    }

    unsafe fn into_message_value_fuse_if_required(
        raw_parent_arena: RawArena,
        mut val: Self,
    ) -> upb_MessageValue {
        // SAFETY: The arena memory is not freed due to `ManuallyDrop`.
        let parent_arena =
            std::mem::ManuallyDrop::new(unsafe { Arena::from_raw(raw_parent_arena) });

        parent_arena.fuse(val.get_arena(Private));
        upb_MessageValue { msg_val: Some(val.get_ptr(Private).raw()) }
    }

    unsafe fn from_message_value<'msg>(msg: upb_MessageValue) -> View<'msg, Self> {
        unsafe {
            let raw = msg.msg_val.expect("expected present message value in map");
            View::<Self>::__unstable_wrap_raw_message_unchecked_lifetime(
                raw.as_ptr() as *const std::ffi::c_void
            )
        }
    }

    unsafe fn from_message_mut<'msg>(msg: RawMessage, arena: &'msg Arena) -> Mut<'msg, Self> {
        unsafe { MessageMutInner::<'msg, Self>::wrap_raw(msg, arena).into() }
    }

    unsafe fn copy_repeated(src: RawArray, dest: RawArray, arena: RawArena) {
        // SAFETY:
        // - `src` is a valid `const upb_Array*`.
        // - `dest` is a valid `upb_Array*`.
        // - Elements of `src` and `dest` have minitable `Self::mini_table()`.
        unsafe {
            let size = upb_Array_Size(src);
            if !upb_Array_Resize(dest, size, arena) {
                panic!("upb_Array_Resize failed (alloc should be infallible)");
            }
            for i in 0..size {
                let src_msg =
                    upb_Array_Get(src, i).msg_val.expect("upb_Array* element should not be NULL");
                // Avoid the use of `upb_Array_DeepClone` as it creates an
                // entirely new `upb_Array*` at a new memory address.
                let cloned_msg = upb_Message_DeepClone(src_msg, Self::mini_table(), arena)
                    .expect("upb_Message_DeepClone failed (alloc should be infallible)");
                upb_Array_Set(dest, i, upb_MessageValue { msg_val: Some(cloned_msg) });
            }
        }
    }
}

impl<T> UpbTypeConversions<EnumTag> for T
where
    Self: Into<i32> + TryFrom<i32> + for<'a> Proxied<View<'a> = Self> + 'static,
{
    fn upb_type() -> CType {
        CType::Enum
    }

    fn to_message_value(val: View<'_, Self>) -> upb_MessageValue {
        upb_MessageValue { int32_val: val.into() }
    }

    unsafe fn into_message_value_fuse_if_required(
        _raw_parent_arena: RawArena,
        val: Self,
    ) -> upb_MessageValue {
        upb_MessageValue { int32_val: val.into() }
    }

    unsafe fn from_message_value<'msg>(val: upb_MessageValue) -> View<'msg, Self> {
        // SAFETY: The caller guarantees that `val` is the correct variant.
        let result = Self::try_from(unsafe { val.int32_val });
        std::debug_assert!(result.is_ok());
        // SAFETY:
        // - The caller guarantees that `val.int32_val` is valid for this enum.
        unsafe { result.unwrap_unchecked() }
    }

    unsafe fn copy_repeated(src: RawArray, dest: RawArray, arena: RawArena) {
        // SAFETY:
        // - Enum arrays have the same representation as i32 arrays.
        // - The caller guarantees that src and dest are enum arrays and that `arena` will outlive
        //   `dest`.
        unsafe {
            <i32 as UpbTypeConversions<PrimitiveTag>>::copy_repeated(src, dest, arena);
        }
    }
}

macro_rules! impl_upb_type_conversions_for_scalars {
    ($($t:ty, $ufield:ident, $upb_tag:expr, $zero_val:literal;)*) => {
        $(
            impl UpbTypeConversions<PrimitiveTag> for $t {
                #[inline(always)]
                fn upb_type() -> upb::CType {
                    $upb_tag
                }

                #[inline(always)]
                fn to_message_value(val: View<'_, $t>) -> upb_MessageValue {
                    upb_MessageValue { $ufield: val }
                }

                #[inline(always)]
                unsafe fn into_message_value_fuse_if_required(_: RawArena, val: $t) -> upb_MessageValue {
                    <Self as UpbTypeConversions<PrimitiveTag>>::to_message_value(val)
                }

                #[inline(always)]
                unsafe fn from_message_value<'msg>(msg: upb_MessageValue) -> View<'msg, $t> {
                    unsafe { msg.$ufield }
                }

                #[inline(always)]
                unsafe fn copy_repeated(src: RawArray, dest: RawArray, arena: RawArena) {
                    // SAFETY:
                    // - `upb_Array_Resize` is unsafe but assumed to be always sound to call.
                    // - `copy_nonoverlapping` is unsafe but here we guarantee that both pointers
                    //   are valid, the pointers are `#[repr(u8)]`, and the size is correct.
                    unsafe {
                        let len = upb_Array_Size(src);
                        if (!upb_Array_Resize(dest, len, arena)) {
                            panic!("upb_Array_Resize failed (alloc should be infallible)");
                        }
                        ptr::copy_nonoverlapping(
                          upb_Array_DataPtr(src).cast::<u8>(),
                          upb_Array_MutableDataPtr(dest).cast::<u8>(),
                          size_of::<$t>() * len);
                    }
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

/// # Safety
/// - `src` must be a valid array of string or bytes.
/// - `dest` must be a valid mutable array of the same type as `src`.
/// - `arena` must point to an arena that will outlive `dest`.
unsafe fn copy_repeated_bytes(src: RawArray, dest: RawArray, arena: RawArena) {
    // SAFETY:
    // - `upb_Array_Resize` is unsafe but assumed to be always sound to call.
    // - `upb_Array` ensures its elements are never uninitialized memory.
    // - The `DataPtr` and `MutableDataPtr` functions return pointers to spans
    //   of memory that are valid for at least `len` elements of PtrAndLen.
    // - `copy_nonoverlapping` is unsafe but here we guarantee that both pointers
    //   are valid, the pointers are `#[repr(u8)]`, and the size is correct.
    // - The bytes held within a valid array are valid.
    unsafe {
        let len = upb_Array_Size(src);
        let arena = ManuallyDrop::new(Arena::from_raw(arena));
        if !upb_Array_Resize(dest, len, arena.raw()) {
            panic!("upb_Array_Resize failed (alloc should be infallible)");
        }
        let src_ptrs: &[PtrAndLen] = slice::from_raw_parts(upb_Array_DataPtr(src).cast(), len);
        let dest_ptrs: &mut [PtrAndLen] =
            slice::from_raw_parts_mut(upb_Array_MutableDataPtr(dest).cast(), len);
        for (src_ptr, dest_ptr) in src_ptrs.iter().zip(dest_ptrs) {
            *dest_ptr = arena.copy_slice_in(src_ptr.as_ref()).unwrap().into();
        }
    }
}

impl UpbTypeConversions<PrimitiveTag> for ProtoBytes {
    fn upb_type() -> upb::CType {
        upb::CType::Bytes
    }

    fn to_message_value(val: View<'_, ProtoBytes>) -> upb_MessageValue {
        upb_MessageValue { str_val: val.into() }
    }

    unsafe fn into_message_value_fuse_if_required(
        raw_parent_arena: RawArena,
        val: ProtoBytes,
    ) -> upb_MessageValue {
        // SAFETY: The arena memory is not freed due to `ManuallyDrop`.
        let parent_arena = ManuallyDrop::new(unsafe { Arena::from_raw(raw_parent_arena) });

        let (view, arena) = val.inner.into_raw_parts();
        parent_arena.fuse(&arena);

        upb_MessageValue { str_val: view }
    }

    unsafe fn from_message_value<'msg>(msg: upb_MessageValue) -> View<'msg, ProtoBytes> {
        unsafe { msg.str_val.as_ref() }
    }

    unsafe fn copy_repeated(src: RawArray, dest: RawArray, arena: RawArena) {
        unsafe {
            copy_repeated_bytes(src, dest, arena);
        }
    }
}

impl UpbTypeConversions<PrimitiveTag> for ProtoString {
    fn upb_type() -> upb::CType {
        upb::CType::String
    }

    fn to_message_value(val: View<'_, ProtoString>) -> upb_MessageValue {
        upb_MessageValue { str_val: val.as_bytes().into() }
    }

    unsafe fn into_message_value_fuse_if_required(
        raw_arena: RawArena,
        val: ProtoString,
    ) -> upb_MessageValue {
        // SAFETY: `raw_arena` is valid as promised by the caller
        unsafe {
            <ProtoBytes as UpbTypeConversions<PrimitiveTag>>::into_message_value_fuse_if_required(
                raw_arena,
                val.into(),
            )
        }
    }

    unsafe fn from_message_value<'msg>(msg: upb_MessageValue) -> View<'msg, ProtoString> {
        unsafe { ProtoStr::from_utf8_unchecked(msg.str_val.as_ref()) }
    }

    unsafe fn copy_repeated(src: RawArray, dest: RawArray, arena: RawArena) {
        unsafe {
            copy_repeated_bytes(src, dest, arena);
        }
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

impl<Key, MessageType> ProxiedInMapValue<Key> for MessageType
where
    Key: Proxied + EntityType + UpbTypeConversions<Key::Tag>,
    Self: Proxied + EntityType + UpbTypeConversions<<Self as EntityType>::Tag>,
{
    fn map_new(_private: Private) -> Map<Key, Self> {
        let arena = Arena::new();
        let raw = unsafe { upb_Map_New(arena.raw(), Key::upb_type(), Self::upb_type()) };

        Map::from_inner(Private, InnerMap::new(raw, arena))
    }

    unsafe fn map_free(_private: Private, _map: &mut Map<Key, Self>) {
        // No-op: the memory will be dropped by the arena.
    }

    fn map_clear(mut map: MapMut<Key, Self>) {
        unsafe {
            upb_Map_Clear(map.as_raw(Private));
        }
    }

    fn map_len(map: MapView<Key, Self>) -> usize {
        unsafe { upb_Map_Size(map.as_raw(Private)) }
    }

    fn map_insert(
        mut map: MapMut<Key, Self>,
        key: View<'_, Key>,
        value: impl IntoProxied<Self>,
    ) -> bool {
        let arena = map.inner(Private).raw_arena();
        unsafe {
            upb_Map_InsertAndReturnIfInserted(
                map.as_raw(Private),
                Key::to_message_value(key),
                Self::into_message_value_fuse_if_required(arena, value.into_proxied(Private)),
                arena,
            )
        }
    }

    fn map_get<'a>(map: MapView<'a, Key, Self>, key: View<'_, Key>) -> Option<View<'a, Self>> {
        let mut val = MaybeUninit::uninit();
        let found = unsafe {
            upb_Map_Get(map.as_raw(Private), Key::to_message_value(key), val.as_mut_ptr())
        };
        if !found {
            return None;
        }
        Some(unsafe { Self::from_message_value(val.assume_init()) })
    }

    fn map_get_mut<'a>(mut map: MapMut<'a, Key, Self>, key: View<'_, Key>) -> Option<Mut<'a, Self>>
    where
        Self: Message,
    {
        // SAFETY: The map is valid as promised by the caller.
        let val = unsafe { upb_Map_GetMutable(map.as_raw(Private), Key::to_message_value(key)) };
        // SAFETY: The lifetime of the MapMut is guaranteed to outlive the returned Mut.
        NonNull::new(val).map(|msg| unsafe { Self::from_message_mut(msg, map.arena(Private)) })
    }

    fn map_remove(mut map: MapMut<Key, Self>, key: View<'_, Key>) -> bool {
        unsafe { upb_Map_Delete(map.as_raw(Private), Key::to_message_value(key), ptr::null_mut()) }
    }
    fn map_iter(map: MapView<Key, Self>) -> MapIter<Key, Self> {
        // SAFETY: MapView<'_,..>> guarantees its RawMap outlives '_.
        unsafe { MapIter::from_raw(Private, RawMapIter::new(map.as_raw(Private))) }
    }

    fn map_iter_next<'a>(
        iter: &mut MapIter<'a, Key, Self>,
    ) -> Option<(View<'a, Key>, View<'a, Self>)> {
        // SAFETY: MapIter<'a, ..> guarantees its RawMapIter outlives 'a.
        unsafe { iter.as_raw_mut(Private).next_unchecked() }
            // SAFETY: MapIter<K, V> returns key and values message values
            //         with the variants for K and V active.
            .map(|(k, v)| unsafe { (Key::from_message_value(k), Self::from_message_value(v)) })
    }
}

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

/// Internal-only trait to support blanket impls that need const access to raw messages
/// on codegen. Should never be used by application code.
#[doc(hidden)]
pub unsafe trait UpbGetMessagePtr: SealedInternal {
    type Msg: AssociatedMiniTable + Message;

    fn get_ptr(&self, _private: Private) -> MessagePtr<Self::Msg>;
}

/// Internal-only trait to support blanket impls that need mutable access to raw messages
/// on codegen. Must not be implemented on View proxies. Should never be used by application code.
#[doc(hidden)]
pub unsafe trait UpbGetMessagePtrMut: SealedInternal {
    type Msg: AssociatedMiniTable + Message;

    fn get_ptr_mut(&mut self, _private: Private) -> MessagePtr<Self::Msg>;
}

/// Internal-only trait to support blanket impls that need const access to raw messages
/// on codegen. Should never be used by application code.
#[doc(hidden)]
pub unsafe trait UpbGetArena: SealedInternal {
    fn get_arena(&mut self, _private: Private) -> &Arena;
}

// The upb kernel doesn't support any owned message or message mut interop.
impl<T: Message> OwnedMessageInterop for T {}
impl<'a, T: MessageMut<'a>> MessageMutInterop<'a> for T {}

impl<'a, T> MessageViewInterop<'a> for T
where
    Self: UpbGetMessagePtr
        + MessageView<'a>
        + From<MessageViewInner<'a, <Self as MessageView<'a>>::Message>>,
    <Self as MessageView<'a>>::Message: AssociatedMiniTable,
{
    unsafe fn __unstable_wrap_raw_message(msg: &'a *const std::ffi::c_void) -> Self {
        let raw = RawMessage::new(*msg as *mut _).unwrap();
        let inner = unsafe { MessageViewInner::wrap_raw(raw) };
        inner.into()
    }
    unsafe fn __unstable_wrap_raw_message_unchecked_lifetime(msg: *const std::ffi::c_void) -> Self {
        let raw = RawMessage::new(msg as *mut _).unwrap();
        let inner = unsafe { MessageViewInner::wrap_raw(raw) };
        inner.into()
    }
    fn __unstable_as_raw_message(&self) -> *const std::ffi::c_void {
        self.get_ptr(Private).raw().as_ptr() as *const _
    }
}

impl<T> MatcherEq for T
where
    Self: AssociatedMiniTable + AsView + Debug,
    for<'a> View<'a, <Self as AsView>::Proxied>: UpbGetMessagePtr,
{
    fn matches(&self, o: &Self) -> bool {
        unsafe {
            upb_Message_IsEqual(
                self.as_view().get_ptr(Private).raw(),
                o.as_view().get_ptr(Private).raw(),
                Self::mini_table(),
                0,
            )
        }
    }
}

impl<T: UpbGetMessagePtrMut> Clear for T {
    fn clear(&mut self) {
        unsafe { self.get_ptr_mut(Private).clear() }
    }
}

fn clear_and_parse_helper<T>(
    msg: &mut T,
    data: &[u8],
    decode_options: i32,
) -> Result<(), ParseError>
where
    T: AssociatedMiniTable + UpbGetMessagePtrMut + UpbGetArena,
{
    Clear::clear(msg);
    // SAFETY:
    // - `msg` is a valid mutable message.
    // - `mini_table` is the one associated with `msg`
    // - `msg.arena().raw()` is held for the same lifetime as `msg`.
    unsafe {
        upb::wire::decode_with_options(
            data,
            msg.get_ptr_mut(Private),
            msg.get_arena(Private),
            decode_options,
        )
    }
    .map(|_| ())
    .map_err(|_| ParseError)
}

impl<T> ClearAndParse for T
where
    Self: AssociatedMiniTable + UpbGetMessagePtrMut + UpbGetArena,
{
    fn clear_and_parse(&mut self, data: &[u8]) -> Result<(), ParseError> {
        clear_and_parse_helper(self, data, upb::wire::decode_options::CHECK_REQUIRED)
    }

    fn clear_and_parse_dont_enforce_required(&mut self, data: &[u8]) -> Result<(), ParseError> {
        clear_and_parse_helper(self, data, 0)
    }
}

impl<T> Serialize for T
where
    Self: AssociatedMiniTable + UpbGetMessagePtr,
{
    fn serialize(&self) -> Result<Vec<u8>, SerializeError> {
        //~ TODO: This discards the info we have about the reason
        //~ of the failure, we should try to keep it instead.
        upb::wire::encode(self.get_ptr(Private)).map_err(|_| SerializeError)
    }
}

impl<T> TakeFrom for T
where
    Self: CopyFrom + AsMut,
    for<'a> Mut<'a, <Self as AsMut>::MutProxied>: Clear,
{
    fn take_from(&mut self, mut src: impl AsMut<MutProxied = Self::Proxied>) {
        let mut src = src.as_mut();
        // TODO: b/393559271 - Optimize this copy out.
        CopyFrom::copy_from(self, AsView::as_view(&src));
        Clear::clear(&mut src);
    }
}

impl<T> CopyFrom for T
where
    Self: AsView + AssociatedMiniTable + UpbGetArena + UpbGetMessagePtr,
    for<'a> View<'a, Self::Proxied>: UpbGetMessagePtr,
{
    fn copy_from(&mut self, src: impl AsView<Proxied = Self::Proxied>) {
        // SAFETY: self and src are both valid `T`s associated with
        // `Self::mini_table()`.
        unsafe {
            assert!(upb_Message_DeepCopy(
                self.get_ptr(Private).raw(),
                src.as_view().get_ptr(Private).raw(),
                <Self as AssociatedMiniTable>::mini_table(),
                self.get_arena(Private).raw()
            ));
        }
    }
}

impl<T> MergeFrom for T
where
    Self: AsView + AssociatedMiniTable + UpbGetArena + UpbGetMessagePtr,
    for<'a> View<'a, Self::Proxied>: UpbGetMessagePtr,
{
    fn merge_from(&mut self, src: impl AsView<Proxied = Self::Proxied>) {
        // SAFETY: self and src are both valid `T`s.
        unsafe {
            assert!(upb_Message_MergeFrom(
                self.get_ptr(Private).raw(),
                src.as_view().get_ptr(Private).raw(),
                <Self as AssociatedMiniTable>::mini_table(),
                // Use a nullptr for the ExtensionRegistry.
                std::ptr::null(),
                self.get_arena(Private).raw()
            ));
        }
    }
}
