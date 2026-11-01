// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use super::sys::mem::arena as sys_arena;
use core::cell::UnsafeCell;
use core::marker::PhantomData;
use core::mem::{align_of_val, size_of_val, MaybeUninit};
use core::ptr;
use core::slice;

pub type RawArena = sys_arena::RawArena;

#[repr(C)]
struct CleanupHeader {
    next: *mut CleanupHeader,
    drop_fn: unsafe fn(*mut CleanupHeader),
    type_id: core::any::TypeId,
    owner: *const (),
}

#[repr(C)]
struct CleanupEntry<T> {
    header: CleanupHeader,
    value: T,
}

/// A wrapper over a `upb_Arena`.
///
/// This is not a safe wrapper per se, because the allocation functions still
/// have sharp edges (see their safety docs for more info).
///
/// This is an owning type and will automatically free the arena when
/// dropped.
///
/// Note that this type is not `Sync` as it implements unsynchronized interior
/// mutability. The upb_Arena C object could be understood as being Sync (at
/// least vacuously under current API since there are not any const upb_Arena*
/// API functions), but the Rust Arena is necessarily expressed as interior
/// mutability (&self rather than &mut self receivers) See https://doc.rust-lang.org/nomicon/lifetime-mismatch.html and
/// https://blog.reverberate.org/2021/12/19/arenas-and-rust.html, and the
/// 'known problems' section of https://rust-lang.github.io/rust-clippy/master/index.html#/mut_from_ref.
#[derive(Debug)]
pub struct Arena {
    // Safety invariant: this must always be a valid arena
    raw: RawArena,
    cleanups: UnsafeCell<*mut CleanupHeader>,
    _not_sync: PhantomData<UnsafeCell<()>>,
}

// SAFETY: `Arena` uniquely holds the underlying RawArena and has no
// thread-local data. All retained values in `cleanups` are required to be `Send`.
unsafe impl Send for Arena {}

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
        // - `upb_Arena_New` is assumed to be implemented correctly and always sound to call; if it
        //   returned a non-null pointer, it is a valid arena.
        unsafe {
            let Some(raw) = sys_arena::upb_Arena_New() else { arena_new_failed() };
            Self { raw, cleanups: UnsafeCell::new(ptr::null_mut()), _not_sync: PhantomData }
        }
    }

    /// Allocates a fresh arena with an initial block sized for at least
    /// `size_hint` bytes, so a first allocation of up to `size_hint` bytes
    /// will not require allocating a second block.
    #[inline]
    pub fn new_sized(size_hint: usize) -> Self {
        #[inline(never)]
        #[cold]
        fn arena_new_failed() -> ! {
            panic!("Could not create a new UPB arena");
        }

        // SAFETY:
        // - `upb_Arena_NewSized` is assumed to be implemented correctly and always sound to call;
        //   if it returned a non-null pointer, it is a valid arena.
        unsafe {
            let Some(raw) = sys_arena::upb_Arena_NewSized(size_hint) else { arena_new_failed() };
            Self { raw, cleanups: UnsafeCell::new(ptr::null_mut()), _not_sync: PhantomData }
        }
    }

    /// # Safety
    /// - The `raw_arena` must point to a valid arena.
    /// - The caller must ensure that the Arena's destructor does not run.
    pub unsafe fn from_raw(raw_arena: RawArena) -> Self {
        Arena { raw: raw_arena, cleanups: UnsafeCell::new(ptr::null_mut()), _not_sync: PhantomData }
    }

    /// Returns the raw, UPB-managed pointer to the arena.
    #[inline]
    pub fn raw(&self) -> RawArena {
        self.raw
    }

    /// Allocates some memory on the arena. Returns None if the allocation
    /// failed.
    ///
    /// # Safety
    ///
    /// - `align` must be less than `UPB_MALLOC_ALIGN`.
    #[allow(clippy::mut_from_ref)]
    #[inline]
    pub unsafe fn alloc(&self, size: usize, align: usize) -> Option<&mut [MaybeUninit<u8>]> {
        debug_assert!(align <= sys_arena::UPB_MALLOC_ALIGN);
        // SAFETY: `self.raw` is a valid UPB arena
        let ptr = unsafe { sys_arena::upb_Arena_Malloc(self.raw, size) };

        if ptr.is_null() {
            None
        } else {
            // SAFETY:
            // - `upb_Arena_Malloc` promises that if the return pointer is non-null, it is
            //   dereferencable for `size` bytes and has an alignment of `UPB_MALLOC_ALIGN` until
            //   the arena is destroyed.
            // - `[MaybeUninit<u8>]` has no alignment requirement, and `ptr` is aligned to a
            //   `UPB_MALLOC_ALIGN` boundary.
            Some(unsafe { slice::from_raw_parts_mut(ptr.cast(), size) })
        }
    }

    /// Same as alloc() but panics if `align > UPB_MALLOC_ALIGN`.
    #[allow(clippy::mut_from_ref)]
    #[inline]
    pub fn checked_alloc(&self, size: usize, align: usize) -> Option<&mut [MaybeUninit<u8>]> {
        assert!(align <= sys_arena::UPB_MALLOC_ALIGN);
        // SAFETY: align <= UPB_MALLOC_ALIGN asserted.
        unsafe { self.alloc(size, align) }
    }

    /// Copies the T into this arena and returns a pointer to the T data inside
    /// the arena. Returns None if the allocation failed.
    pub fn copy_in<'a, T: Copy>(&'a self, data: &T) -> Option<&'a T> {
        let size = size_of_val(data);
        let align = align_of_val(data);

        self.checked_alloc(size, align).map(|alloc| {
            // SAFETY:
            // - alloc is valid for `size` bytes and is the uninit bytes are written to not read
            //   from until written.
            // - T is copy so copying the bytes of the value is sound.
            unsafe {
                let alloc = alloc.as_mut_ptr().cast::<MaybeUninit<T>>();
                &*(*alloc).write(*data)
            }
        })
    }

    /// Copies the str into this arena and returns a pointer to the T data
    /// inside the arena. Returns None if the allocation failed.
    pub fn copy_str_in<'a>(&'a self, s: &str) -> Option<&'a str> {
        self.copy_slice_in(s.as_bytes()).map(|copied_bytes| {
            // SAFETY: `copied_bytes` has same contents as `s` and so must meet &str
            // criteria.
            unsafe { core::str::from_utf8_unchecked(copied_bytes) }
        })
    }

    /// Copies the slice into this arena and returns a pointer to the T data
    /// inside the arena. Returns None if the allocation failed.
    pub fn copy_slice_in<'a, T: Copy>(&'a self, data: &[T]) -> Option<&'a [T]> {
        let size = size_of_val(data);
        let align = align_of_val(data);
        self.checked_alloc(size, align).map(|alloc| {
            let alloc: *mut T = alloc.as_mut_ptr().cast();
            // SAFETY:
            // - uninit_alloc is valid for `layout.len()` bytes and is the uninit bytes are written
            //   to not read from until written.
            // - T is copy so copying the bytes of the values is sound.
            unsafe {
                ptr::copy_nonoverlapping(data.as_ptr(), alloc, data.len());
                slice::from_raw_parts(alloc, data.len())
            }
        })
    }

    /// Moves `value` into the arena associated with `owner`, replacing any
    /// previously retained value for `owner`, and runs its destructor when
    /// `owner`'s retained values or the arena are cleared/dropped.
    ///
    /// # Safety
    /// - The caller must ensure that the returned reference (and any aliased pointers into `value`
    ///   or into any previously retained value for `owner`) is not accessed after `clear_retained`,
    ///   `clear_retained_for(owner)`, or a subsequent `retain_for(owner, ...)` is called.
    /// - If `parent.fuse(self)` is called, ownership of retained values is transferred to `parent`
    ///   and they will be dropped when `parent` is dropped.
    pub unsafe fn retain_for<'a, T: Send + 'static>(&'a self, owner: *const (), value: T) -> &'a T {
        unsafe fn drop_entry<T>(ptr: *mut CleanupHeader) {
            unsafe {
                let entry = ptr.cast::<CleanupEntry<T>>();
                ptr::drop_in_place(&raw mut (*entry).value);
            }
        }

        unsafe fn drop_boxed_entry<T>(ptr: *mut CleanupHeader) {
            unsafe {
                drop(Box::from_raw(ptr.cast::<CleanupEntry<T>>()));
            }
        }

        let target_type = core::any::TypeId::of::<T>();
        unsafe {
            let mut prev_next: *mut *mut CleanupHeader = self.cleanups.get();
            while !(*prev_next).is_null() {
                let curr = *prev_next;
                if (*curr).owner == owner {
                    if (*curr).type_id == target_type {
                        let entry = curr.cast::<CleanupEntry<T>>();
                        ptr::drop_in_place(&raw mut (*entry).value);
                        (&raw mut (*entry).value).write(value);
                        return &(*entry).value;
                    }
                    *prev_next = (*curr).next;
                    ((*curr).drop_fn)(curr);
                } else {
                    prev_next = &raw mut (*curr).next;
                }
            }
        }

        let size = core::mem::size_of::<CleanupEntry<T>>();
        let align = core::mem::align_of::<CleanupEntry<T>>();

        unsafe {
            let head = *self.cleanups.get();
            if align <= sys_arena::UPB_MALLOC_ALIGN {
                if let Some(alloc) = self.checked_alloc(size, align) {
                    let entry_ptr = alloc.as_mut_ptr().cast::<CleanupEntry<T>>();
                    entry_ptr.write(CleanupEntry {
                        header: CleanupHeader {
                            next: head,
                            drop_fn: drop_entry::<T>,
                            type_id: target_type,
                            owner,
                        },
                        value,
                    });
                    *self.cleanups.get() = entry_ptr.cast::<CleanupHeader>();
                    return &(*entry_ptr).value;
                }
            }
            let boxed = Box::new(CleanupEntry {
                header: CleanupHeader {
                    next: head,
                    drop_fn: drop_boxed_entry::<T>,
                    type_id: target_type,
                    owner,
                },
                value,
            });
            let entry_ptr = Box::into_raw(boxed);
            *self.cleanups.get() = entry_ptr.cast::<CleanupHeader>();
            &(*entry_ptr).value
        }
    }

    /// Drops all values retained via [`Arena::retain_for`].
    ///
    /// # Safety
    /// - The caller must ensure that no references returned by [`Arena::retain_for`] or
    ///   [`Arena::get_retained_for`] and no aliased message fields pointing into those retained
    ///   values are accessed after this call.
    pub unsafe fn clear_retained(&self) {
        unsafe {
            let mut curr = ptr::replace(self.cleanups.get(), ptr::null_mut());
            while !curr.is_null() {
                let next = (*curr).next;
                ((*curr).drop_fn)(curr);
                curr = next;
            }
        }
    }

    /// Drops all values retained for `owner` via [`Arena::retain_for`].
    ///
    /// # Safety
    /// - The caller must ensure that no references returned by [`Arena::retain_for`] or
    ///   [`Arena::get_retained_for`] for `owner` and no aliased message fields pointing into those
    ///   retained values are accessed after this call.
    pub unsafe fn clear_retained_for(&self, owner: *const ()) {
        unsafe {
            let mut prev_next: *mut *mut CleanupHeader = self.cleanups.get();
            while !(*prev_next).is_null() {
                let curr = *prev_next;
                if (*curr).owner == owner {
                    *prev_next = (*curr).next;
                    ((*curr).drop_fn)(curr);
                } else {
                    prev_next = &raw mut (*curr).next;
                }
            }
        }
    }

    /// Returns a reference to the retained value of type `T` for `owner`, if any.
    pub fn get_retained_for<T: 'static>(&self, owner: *const ()) -> Option<&T> {
        let target = core::any::TypeId::of::<T>();
        unsafe {
            let mut curr = *self.cleanups.get();
            while !curr.is_null() {
                if (*curr).owner == owner && (*curr).type_id == target {
                    let entry = curr.cast::<CleanupEntry<T>>();
                    return Some(&(*entry).value);
                }
                curr = (*curr).next;
            }
            None
        }
    }

    /// Fuse two arenas so they share the same lifetime.
    ///
    /// `fuse` will make it so that the memory allocated by `self` or `other` is
    /// guaranteed to last until both `self` and `other` have been dropped.
    /// The pointers returned by `Arena::alloc` will continue to be valid so
    /// long as either `self` or `other` has not been dropped.
    pub fn fuse(&self, other: &Arena) {
        // SAFETY: `self.raw()` and `other.raw()` are both valid UPB arenas.
        let success = unsafe { sys_arena::upb_Arena_Fuse(self.raw(), other.raw()) };
        if !success {
            // Fusing can fail if any of the arenas has an initial block i.e. the arena is
            // backed by a preallocated chunk of memory that it doesn't own and thus cannot
            // lifetime extend. This function panics because this is typically not a
            // recoverable error but a logic bug in a program.
            panic!("Could not fuse two UPB arenas.");
        }
        if !ptr::eq(self, other) {
            unsafe {
                let other_head = ptr::replace(other.cleanups.get(), ptr::null_mut());
                if !other_head.is_null() {
                    let mut tail = other_head;
                    while !(*tail).next.is_null() {
                        tail = (*tail).next;
                    }
                    (*tail).next = *self.cleanups.get();
                    *self.cleanups.get() = other_head;
                }
            }
        }
    }
}

impl Default for Arena {
    fn default() -> Self {
        Self::new()
    }
}

impl Drop for Arena {
    #[inline]
    fn drop(&mut self) {
        unsafe {
            self.clear_retained();
            sys_arena::upb_Arena_Free(self.raw);
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use googletest::gtest;
    use std::sync::atomic::{AtomicUsize, Ordering};
    use std::sync::Arc;

    struct DropTracker(Arc<AtomicUsize>);
    impl Drop for DropTracker {
        fn drop(&mut self) {
            self.0.fetch_add(1, Ordering::SeqCst);
        }
    }

    #[gtest]
    fn test_arena_new_and_free() {
        let arena = Arena::new();
        drop(arena);
    }

    #[gtest]
    fn test_arena_new_sized() {
        let arena = Arena::new_sized(4096);
        let initial_space =
            unsafe { sys_arena::upb_Arena_SpaceAllocated(arena.raw(), ptr::null_mut()) };
        let slice = arena.copy_slice_in(&[42u8; 4096]).unwrap();
        let final_space =
            unsafe { sys_arena::upb_Arena_SpaceAllocated(arena.raw(), ptr::null_mut()) };
        assert_eq!(initial_space, final_space);
        assert_eq!(slice.len(), 4096);
        assert_eq!(slice[4095], 42);
    }

    #[gtest]
    fn test_arena_retain_for_and_drop() {
        let drops = Arc::new(AtomicUsize::new(0));
        let owner = 1usize as *const ();
        {
            let arena = Arena::new();
            let retained = unsafe {
                arena.retain_for(owner, (vec![1u8, 2, 3, 4], DropTracker(drops.clone())))
            };
            assert_eq!(retained.0.as_slice(), &[1, 2, 3, 4]);
            assert_eq!(
                arena.get_retained_for::<(Vec<u8>, DropTracker)>(owner).unwrap().0.as_slice(),
                &[1, 2, 3, 4]
            );
            assert_eq!(drops.load(Ordering::SeqCst), 0);
        }
        assert_eq!(drops.load(Ordering::SeqCst), 1);
    }

    #[gtest]
    fn test_arena_retain_for_replaces_previous_for_same_owner() {
        let drops1 = Arc::new(AtomicUsize::new(0));
        let drops2 = Arc::new(AtomicUsize::new(0));
        let owner = 1usize as *const ();
        let arena = Arena::new();
        unsafe {
            arena.retain_for(owner, (10u32, DropTracker(drops1.clone())));
            assert_eq!(drops1.load(Ordering::SeqCst), 0);

            arena.retain_for(owner, (20u32, DropTracker(drops2.clone())));
            assert_eq!(drops1.load(Ordering::SeqCst), 1);
            assert_eq!(drops2.load(Ordering::SeqCst), 0);
        }
        assert_eq!(arena.get_retained_for::<(u32, DropTracker)>(owner).unwrap().0, 20);
        drop(arena);
        assert_eq!(drops2.load(Ordering::SeqCst), 1);
    }

    #[gtest]
    fn test_arena_retain_for_and_clear_for() {
        let drops1 = Arc::new(AtomicUsize::new(0));
        let drops2 = Arc::new(AtomicUsize::new(0));
        let owner1 = 1usize as *const ();
        let owner2 = 2usize as *const ();
        let arena = Arena::new();
        unsafe {
            arena.retain_for(owner1, (10u32, DropTracker(drops1.clone())));
            arena.retain_for(owner2, (20u32, DropTracker(drops2.clone())));
        }
        assert_eq!(arena.get_retained_for::<(u32, DropTracker)>(owner1).unwrap().0, 10);
        assert_eq!(arena.get_retained_for::<(u32, DropTracker)>(owner2).unwrap().0, 20);

        unsafe {
            arena.clear_retained_for(owner1);
        }
        assert_eq!(drops1.load(Ordering::SeqCst), 1);
        assert_eq!(drops2.load(Ordering::SeqCst), 0);
        assert!(arena.get_retained_for::<(u32, DropTracker)>(owner1).is_none());
        assert_eq!(arena.get_retained_for::<(u32, DropTracker)>(owner2).unwrap().0, 20);

        drop(arena);
        assert_eq!(drops2.load(Ordering::SeqCst), 1);
    }

    #[gtest]
    fn test_arena_fuse_transfers_retained() {
        let drops = Arc::new(AtomicUsize::new(0));
        let owner = 99usize as *const ();
        let parent = Arena::new();
        {
            let child = Arena::new();
            unsafe {
                child.retain_for(owner, (vec![7u8, 8, 9], DropTracker(drops.clone())));
            }
            parent.fuse(&child);
        }
        assert_eq!(drops.load(Ordering::SeqCst), 0);
        assert_eq!(
            parent.get_retained_for::<(Vec<u8>, DropTracker)>(owner).unwrap().0.as_slice(),
            &[7, 8, 9]
        );
        drop(parent);
        assert_eq!(drops.load(Ordering::SeqCst), 1);
    }
}
