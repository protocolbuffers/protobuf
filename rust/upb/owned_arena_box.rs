// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use super::Arena;
use core::fmt::{self, Debug};
use core::ops::{Deref, DerefMut};
use core::ptr::NonNull;

/// An 'owned' T, similar to a Box<T> where the T is data
/// held in a upb Arena. By holding the data pointer and a corresponding arena
/// together the data liveness is be maintained.
///
/// This struct is conceptually self-referential, where `data` points at memory
/// inside `arena`. This avoids typical concerns of self-referential data
/// structures because `arena` modifications (other than drop) will never
/// invalidate `data`, and `data` and `arena` are both behind indirections which
/// avoids any concern with core::mem::swap.
pub struct OwnedArenaBox<T: ?Sized + 'static> {
    data: NonNull<T>,
    arena: Arena,
}

impl<T: ?Sized + 'static> OwnedArenaBox<T> {
    /// Construct `OwnedArenaBox` from raw pointers and its owning arena.
    ///
    /// # Safety
    /// - `data` must satisfy the safety constraints of pointer::as_mut::<'a>()
    ///   where 'a is the passed arena's lifetime (`data` should be valid and
    ///   not mutated while this struct is live).
    /// - `data` should be a pointer into a block from a previous allocation on
    ///   `arena`, or to another arena fused to it, or should be pointing at
    ///   'static data (and if it is pointing at any struct like upb_Message,
    ///   all data transitively reachable should similarly be kept live by
    ///   `arena` or be 'static).
    pub unsafe fn new(data: NonNull<T>, arena: Arena) -> Self {
        OwnedArenaBox { arena, data }
    }

    pub fn data(&self) -> *const T {
        self.data.as_ptr()
    }

    pub fn into_parts(self) -> (NonNull<T>, Arena) {
        (self.data, self.arena)
    }
}

impl<T: ?Sized + 'static> Deref for OwnedArenaBox<T> {
    type Target = T;
    fn deref(&self) -> &Self::Target {
        self.as_ref()
    }
}

impl<T: ?Sized + 'static> DerefMut for OwnedArenaBox<T> {
    fn deref_mut(&mut self) -> &mut Self::Target {
        self.as_mut()
    }
}

impl<T: ?Sized + 'static> AsRef<T> for OwnedArenaBox<T> {
    fn as_ref(&self) -> &T {
        // SAFETY:
        // - `data` is valid under the conditions set on ::new().
        unsafe { self.data.as_ref() }
    }
}

impl<T: ?Sized + 'static> AsMut<T> for OwnedArenaBox<T> {
    fn as_mut(&mut self) -> &mut T {
        // SAFETY:
        // - `data` is valid under the conditions set on ::new().
        unsafe { self.data.as_mut() }
    }
}

impl<T: Debug + 'static> Debug for OwnedArenaBox<T> {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        f.debug_tuple("OwnedArenaBox").field(self.deref()).finish()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use core::str;
    use googletest::gtest;

    #[gtest]
    fn test_byte_slice_pointer_roundtrip() {
        let arena = Arena::new();
        let original_data: &'static [u8] = b"Hello world";
        let owned_data = unsafe { OwnedArenaBox::new(original_data.into(), arena) };
        assert_eq!(&*owned_data, b"Hello world");
    }

    #[gtest]
    fn test_alloc_str_roundtrip() {
        let arena = Arena::new();
        let s: &str = "Hello";
        let arena_alloc_str: NonNull<str> = arena.copy_str_in(s).unwrap().into();
        let owned_data = unsafe { OwnedArenaBox::new(arena_alloc_str, arena) };
        assert_eq!(&*owned_data, s);
    }

    #[gtest]
    fn test_sized_type_roundtrip() {
        let arena = Arena::new();
        let arena_alloc_u32: NonNull<u32> = arena.copy_in(&7u32).unwrap().into();
        let mut owned_data = unsafe { OwnedArenaBox::new(arena_alloc_u32, arena) };
        assert_eq!(*owned_data, 7);
        *owned_data = 8;
        assert_eq!(*owned_data, 8);
    }
}
