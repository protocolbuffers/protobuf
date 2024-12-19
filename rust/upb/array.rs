// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use super::opaque_pointee::opaque_pointee;
use super::{upb_MessageValue, upb_MutableMessageValue, CType, RawArena};
use core::ptr::NonNull;

opaque_pointee!(upb_Array);
pub type RawArray = NonNull<upb_Array>;

extern "C" {
    pub fn upb_Array_New(a: RawArena, r#type: CType) -> RawArray;
    pub fn upb_Array_Size(arr: RawArray) -> usize;
    pub fn upb_Array_Set(arr: RawArray, i: usize, val: upb_MessageValue);
    pub fn upb_Array_Get(arr: RawArray, i: usize) -> upb_MessageValue;
    pub fn upb_Array_Append(arr: RawArray, val: upb_MessageValue, arena: RawArena) -> bool;
    pub fn upb_Array_Resize(arr: RawArray, size: usize, arena: RawArena) -> bool;
    pub fn upb_Array_Reserve(arr: RawArray, size: usize, arena: RawArena) -> bool;
    pub fn upb_Array_MutableDataPtr(arr: RawArray) -> *mut core::ffi::c_void;
    pub fn upb_Array_DataPtr(arr: RawArray) -> *const core::ffi::c_void;
    pub fn upb_Array_GetMutable(arr: RawArray, i: usize) -> upb_MutableMessageValue;
}

#[cfg(test)]
mod tests {
    use super::super::Arena;
    use super::*;
    use googletest::gtest;

    #[gtest]
    fn assert_array_linked() {
        use crate::assert_linked;
        assert_linked!(upb_Array_New);
        assert_linked!(upb_Array_Size);
        assert_linked!(upb_Array_Set);
        assert_linked!(upb_Array_Get);
        assert_linked!(upb_Array_Append);
        assert_linked!(upb_Array_Resize);
        assert_linked!(upb_Array_Reserve);
        assert_linked!(upb_Array_MutableDataPtr);
        assert_linked!(upb_Array_DataPtr);
        assert_linked!(upb_Array_GetMutable);
    }

    #[gtest]
    fn array_ffi_test() {
        // SAFETY: FFI unit test uses C API under expected patterns.
        unsafe {
            let arena = Arena::new();
            let raw_arena = arena.raw();
            let array = upb_Array_New(raw_arena, CType::Float);

            assert!(upb_Array_Append(array, upb_MessageValue { float_val: 7.0 }, raw_arena));
            assert!(upb_Array_Append(array, upb_MessageValue { float_val: 42.0 }, raw_arena));
            assert_eq!(upb_Array_Size(array), 2);
            assert_eq!(upb_Array_Get(array, 1).float_val, 42.0);

            assert!(upb_Array_Resize(array, 3, raw_arena));
            assert_eq!(upb_Array_Size(array), 3);
            assert_eq!(upb_Array_Get(array, 2).float_val, 0.0);
        }
    }
}
