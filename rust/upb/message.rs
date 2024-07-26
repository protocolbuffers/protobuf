// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use super::opaque_pointee::opaque_pointee;
use super::{upb_ExtensionRegistry, upb_MiniTable, upb_MiniTableField, RawArena};
use std::ptr::NonNull;

opaque_pointee!(upb_Message);
pub type RawMessage = NonNull<upb_Message>;

extern "C" {
    /// SAFETY:
    /// - `mini_table` and `arena` must be valid to deref
    pub fn upb_Message_New(mini_table: *const upb_MiniTable, arena: RawArena)
    -> Option<RawMessage>;

    /// SAFETY:
    /// - `m` and `mini_table` must be valid to deref
    /// - `mini_table` must be the MiniTable associtaed with `m`
    pub fn upb_Message_Clear(m: RawMessage, mini_table: *const upb_MiniTable);

    /// SAFETY:
    /// - All four arguments must be valid to deref
    /// - `mini_table` must be the MiniTable associated with both `dst` and
    ///   `src`
    pub fn upb_Message_DeepCopy(
        dst: RawMessage,
        src: RawMessage,
        mini_table: *const upb_MiniTable,
        arena: RawArena,
    );

    /// SAFETY:
    /// - All three arguments must be valid to deref
    /// - `mini_table` must be the MiniTable associated with `m`
    pub fn upb_Message_DeepClone(
        m: RawMessage,
        mini_table: *const upb_MiniTable,
        arena: RawArena,
    ) -> Option<RawMessage>;

    /// SAFETY:
    /// - `m` and `mini_table` must be valid to deref
    /// - `mini_table` must be the MiniTable associated with `m`
    /// - `val` must be a pointer to legally readable memory of the correct type
    ///   for the field described by `mini_table`
    pub fn upb_Message_SetBaseField(
        m: RawMessage,
        mini_table: *const upb_MiniTableField,
        val: *const std::ffi::c_void,
    );

    /// SAFETY:
    /// - All four arguments must be valid to deref
    /// - `mini_table` must be the MiniTable associated with both `m1` and `m2`
    pub fn upb_Message_IsEqual(
        m1: RawMessage,
        m2: RawMessage,
        mini_table: *const upb_MiniTable,
        options: i32,
    ) -> bool;

    /// SAFETY:
    /// - `dst`, `src`, `mini_table` and `arena` must be valid to deref
    /// - `extreg` must be valid to deref or nullptr
    /// - `mini_table` must be the MiniTable associated with both `dst` and
    ///   `src`
    pub fn upb_Message_MergeFrom(
        dst: RawMessage,
        src: RawMessage,
        mini_table: *const upb_MiniTable,
        extreg: *const upb_ExtensionRegistry,
        arena: RawArena,
    ) -> bool;
}
