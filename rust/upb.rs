// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

//! UPB FFI wrapper code for use by Rust Protobuf.

use crate::__internal::{Private, RawArena, RawMessage};
use std::alloc;
use std::alloc::Layout;
use std::cell::UnsafeCell;
use std::fmt;
use std::marker::PhantomData;
use std::mem::MaybeUninit;
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
    pub fn zeroed_block() -> RawMessage {
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

#[cfg(test)]
mod tests {
    use super::*;

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
        assert_eq!(&*serialized_data, b"Hello world");
    }
}
