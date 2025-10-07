// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use super::super::opaque_pointee::opaque_pointee;
use core::mem::align_of;
use core::ptr::NonNull;

opaque_pointee!(upb_Arena);
pub type RawArena = NonNull<upb_Arena>;

/// See `upb/port/def.inc`.
pub const UPB_MALLOC_ALIGN: usize = 8;
const _CHECK_UPB_MALLOC_ALIGN_AT_LEAST_POINTER_ALIGNED: () =
    assert!(UPB_MALLOC_ALIGN >= align_of::<*const ()>());

unsafe extern "C" {
    // `Option<NonNull<T: Sized>>` is ABI-compatible with `*mut T`
    pub fn upb_Arena_New() -> Option<RawArena>;
    pub fn upb_Arena_Free(arena: RawArena);
    pub fn upb_Arena_Malloc(arena: RawArena, size: usize) -> *mut u8;
    pub fn upb_Arena_Fuse(arena1: RawArena, arena2: RawArena) -> bool;
}

#[cfg(test)]
mod tests {
    use super::*;
    use googletest::gtest;

    #[gtest]
    fn assert_arena_linked() {
        use crate::assert_linked;
        assert_linked!(upb_Arena_New);
        assert_linked!(upb_Arena_Free);
        assert_linked!(upb_Arena_Malloc);
        assert_linked!(upb_Arena_Fuse);
    }

    #[gtest]
    fn raw_ffi_test() {
        // SAFETY: FFI unit test uses C API under expected patterns.
        unsafe {
            let arena = upb_Arena_New().unwrap();
            let bytes = upb_Arena_Malloc(arena, 3);
            *bytes.add(2) = 7;
            upb_Arena_Free(arena);
        }
    }
}
