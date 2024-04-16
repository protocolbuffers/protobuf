use crate::opaque_pointee::opaque_pointee;
use std::alloc::{self, Layout};
use std::cell::UnsafeCell;
use std::marker::PhantomData;
use std::mem::{align_of, MaybeUninit};
use std::ptr::NonNull;
use std::slice;

opaque_pointee!(upb_Arena);
pub type RawArena = NonNull<upb_Arena>;

/// See `upb/port/def.inc`.
const UPB_MALLOC_ALIGN: usize = 8;
const _CHECK_UPB_MALLOC_ALIGN_AT_LEAST_POINTER_ALIGNED: () =
    assert!(UPB_MALLOC_ALIGN >= align_of::<*const ()>());

/// A wrapper over a `upb_Arena`.
///
/// This is not a safe wrapper per se, because the allocation functions still
/// have sharp edges (see their safety docs for more info).
///
/// This is an owning type and will automatically free the arena when
/// dropped.
///
/// Note that this type is neither `Sync` nor `Send`. The upb_Arena C object
/// could be understood as being Sync (at least vacuously, as there are no
/// const-ptr functions on upb_Arena's API), but the Rust Arena is
/// necessarily expressed as interior mutability (&self rather than &mut self
/// receivers) See https://doc.rust-lang.org/nomicon/lifetime-mismatch.html and
/// https://blog.reverberate.org/2021/12/19/arenas-and-rust.html, and the
/// 'known problems' section of https://rust-lang.github.io/rust-clippy/master/index.html#/mut_from_ref.
#[derive(Debug)]
pub struct Arena {
    // Safety invariant: this must always be a valid arena
    raw: RawArena,
    _not_sync: PhantomData<UnsafeCell<()>>,
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

    /// # Safety
    /// - The `raw_arena` must point to a valid arena.
    /// - The caller must ensure that the Arena's destructor does not run.
    pub unsafe fn from_raw(raw_arena: RawArena) -> Self {
        Arena { raw: raw_arena, _not_sync: PhantomData }
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
    #[allow(clippy::mut_from_ref)]
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
            upb_Arena_Free(self.raw);
        }
    }
}

extern "C" {
    // `Option<NonNull<T: Sized>>` is ABI-compatible with `*mut T`
    fn upb_Arena_New() -> Option<RawArena>;
    fn upb_Arena_Free(arena: RawArena);
    fn upb_Arena_Malloc(arena: RawArena, size: usize) -> *mut u8;
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn raw_ffi_test() {
        // SAFETY: FFI unit test uses C API under expected patterns.
        unsafe {
            let arena = upb_Arena_New().unwrap();
            let bytes = upb_Arena_Malloc(arena, 3);
            *bytes.add(2) = 7;
            upb_Arena_Free(arena);
        }
    }

    #[test]
    fn test_arena_new_and_free() {
        let arena = Arena::new();
        drop(arena);
    }
}
