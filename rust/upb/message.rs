// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use crate::opaque_pointee::opaque_pointee;
use crate::{upb_ExtensionRegistry, upb_MiniTable, upb_MiniTableField, RawArena};
use std::ptr::NonNull;

opaque_pointee!(upb_Message);
pub type RawMessage = NonNull<upb_Message>;

extern "C" {
    /// SAFETY: No constraints.
    pub fn upb_Message_New(mini_table: *const upb_MiniTable, arena: RawArena)
    -> Option<RawMessage>;

    pub fn upb_Message_DeepCopy(
        dst: RawMessage,
        src: RawMessage,
        mini_table: *const upb_MiniTable,
        arena: RawArena,
    );

    pub fn upb_Message_DeepClone(
        m: RawMessage,
        mini_table: *const upb_MiniTable,
        arena: RawArena,
    ) -> Option<RawMessage>;

    pub fn upb_Message_SetBaseField(
        m: RawMessage,
        mini_table: *const upb_MiniTableField,
        val: *const std::ffi::c_void,
    );

    pub fn upb_Message_IsEqual(
        m1: RawMessage,
        m2: RawMessage,
        mini_table: *const upb_MiniTable,
        options: i32,
    ) -> bool;

    pub fn upb_Message_MergeFrom(
        dst: RawMessage,
        src: RawMessage,
        mini_table: *const upb_MiniTable,
        extreg: *const upb_ExtensionRegistry,
        arena: RawArena,
    ) -> bool;
}
