// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use super::opaque_pointee::opaque_pointee;
use super::{upb_MessageValue, CType, RawArena};
use core::ptr::NonNull;

opaque_pointee!(upb_Map);
pub type RawMap = NonNull<upb_Map>;

#[repr(C)]
#[allow(dead_code)]
#[derive(PartialEq, Eq, Clone, Copy, Debug)]
pub enum MapInsertStatus {
    Inserted = 0,
    Replaced = 1,
    OutOfMemory = 2,
}

pub const UPB_MAP_BEGIN: usize = usize::MAX;

extern "C" {
    pub fn upb_Map_New(arena: RawArena, key_type: CType, value_type: CType) -> RawMap;
    pub fn upb_Map_Size(map: RawMap) -> usize;
    pub fn upb_Map_Insert(
        map: RawMap,
        key: upb_MessageValue,
        value: upb_MessageValue,
        arena: RawArena,
    ) -> MapInsertStatus;
    pub fn upb_Map_Get(map: RawMap, key: upb_MessageValue, value: *mut upb_MessageValue) -> bool;
    pub fn upb_Map_Delete(
        map: RawMap,
        key: upb_MessageValue,
        removed_value: *mut upb_MessageValue,
    ) -> bool;
    pub fn upb_Map_Clear(map: RawMap);
    pub fn upb_Map_Next(
        map: RawMap,
        key: *mut upb_MessageValue,
        value: *mut upb_MessageValue,
        iter: &mut usize,
    ) -> bool;
}

#[cfg(test)]
mod tests {
    use super::super::Arena;
    use super::*;
    use googletest::gtest;

    #[gtest]
    fn assert_map_linked() {
        use crate::assert_linked;
        assert_linked!(upb_Map_New);
        assert_linked!(upb_Map_Size);
        assert_linked!(upb_Map_Insert);
        assert_linked!(upb_Map_Get);
        assert_linked!(upb_Map_Delete);
        assert_linked!(upb_Map_Clear);
        assert_linked!(upb_Map_Next);
    }

    #[gtest]
    fn map_ffi_test() {
        // SAFETY: FFI unit test uses C API under expected patterns.
        unsafe {
            let arena = Arena::new();
            let raw_arena = arena.raw();
            let map = upb_Map_New(raw_arena, CType::Bool, CType::Double);
            assert_eq!(upb_Map_Size(map), 0);
            assert_eq!(
                upb_Map_Insert(
                    map,
                    upb_MessageValue { bool_val: true },
                    upb_MessageValue { double_val: 2.0 },
                    raw_arena
                ),
                MapInsertStatus::Inserted
            );
            assert_eq!(
                upb_Map_Insert(
                    map,
                    upb_MessageValue { bool_val: true },
                    upb_MessageValue { double_val: 3.0 },
                    raw_arena,
                ),
                MapInsertStatus::Replaced,
            );
            assert_eq!(upb_Map_Size(map), 1);
            upb_Map_Clear(map);
            assert_eq!(upb_Map_Size(map), 0);
            assert_eq!(
                upb_Map_Insert(
                    map,
                    upb_MessageValue { bool_val: true },
                    upb_MessageValue { double_val: 4.0 },
                    raw_arena
                ),
                MapInsertStatus::Inserted
            );

            let mut out = upb_MessageValue::zeroed();
            assert!(upb_Map_Get(map, upb_MessageValue { bool_val: true }, &mut out));
            assert_eq!(out.double_val, 4.0);
        }
    }
}
