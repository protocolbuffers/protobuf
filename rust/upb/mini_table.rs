// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use super::arena::RawArena;
use super::opaque_pointee::opaque_pointee;
use core::ptr::NonNull;
use std::ffi::{c_char, c_uchar};

opaque_pointee!(upb_MiniTable);
pub type RawMiniTable = NonNull<upb_MiniTable>;

opaque_pointee!(upb_MiniTableEnum);

opaque_pointee!(upb_MiniTableField);
pub type RawMiniTableField = NonNull<upb_MiniTableField>;

// We could represent this type in Rust, but for now we treat it as opaque since
// we are not currently using it.
opaque_pointee!(upb_Status);

extern "C" {
    /// Finds the field with the provided number, will return NULL if no such
    /// field is found.
    ///
    /// # Safety
    /// - `m` must be legal to deref
    pub fn upb_MiniTable_FindFieldByNumber(
        m: *const upb_MiniTable,
        number: u32,
    ) -> *const upb_MiniTableField;

    /// Gets the field with the corresponding upb field index. This will never
    /// return null: the provided number must be within bounds or else this is
    /// UB.
    ///
    /// # Safety
    /// - `m` must be legal to deref
    /// - `number` must be a valid field index in the `m` table
    pub fn upb_MiniTable_GetFieldByIndex(
        m: *const upb_MiniTable,
        number: u32,
    ) -> *const upb_MiniTableField;

    /// Gets the sub-MiniTable associated with `f`.
    /// # Safety
    /// - `m` and `f` must be valid to deref
    /// - `f` must be a mesage or map typed field associated with `m`
    pub fn upb_MiniTable_SubMessage(
        m: *const upb_MiniTable,
        f: *const upb_MiniTableField,
    ) -> *const upb_MiniTable;

    /// Builds a mini table from the data encoded in the buffer [data, len]. If
    /// any errors occur, returns null and sets a status message if status is
    /// non-null. In the success case, the caller must call upb_MiniTable_Link()
    /// to link the table to its sub-tables.
    ///
    /// # Safety
    /// - `data` must point to a valid array of length len
    /// - `status` must be either null or valid to deref
    pub fn upb_MiniTable_Build(
        data: *const c_uchar,
        len: usize,
        arena: RawArena,
        status: *mut upb_Status,
    ) -> *mut upb_MiniTable;

    /// Builds a upb_MiniTableEnum from an enum minidescriptor. If any errors
    /// occur, returns null and sets a status message if status is non-null.
    ///
    /// # Safety
    /// - `data` must point to a valid array of length len
    /// - `status` must be either null or valid to deref
    pub fn upb_MiniTableEnum_Build(
        data: *const c_uchar,
        len: usize,
        arena: RawArena,
        status: *mut upb_Status,
    ) -> *const upb_MiniTableEnum;

    /// Links a message to its sub-messages and sub-enums. The caller must pass
    /// arrays of sub-tables and sub-enums, in the same length and order as is
    /// returned by upb_MiniTable_GetSubList().
    ///
    /// Returns false if either array is too short, or if any of the tables
    /// fails to link.
    ///
    /// # Safety
    /// - `m` must be valid to deref
    /// - `sub_tables` must point to a valid array of `sub_table_count` pointers
    ///   to `upb_MiniTable`
    /// - `sub_enums` must point to a valid array of `sub_enums_count` pointers
    ///   to `upb_MiniTableEnum`.
    /// - This must only be called once for a given MiniTable.
    pub fn upb_MiniTable_Link(
        m: *mut upb_MiniTable,
        sub_tables: *const *const upb_MiniTable,
        sub_table_count: usize,
        sub_enums: *const *const upb_MiniTableEnum,
        sub_enum_count: usize,
    ) -> bool;
}

#[cfg(test)]
mod tests {
    use super::*;
    use googletest::gtest;

    #[gtest]
    fn assert_mini_table_linked() {
        use crate::assert_linked;
        assert_linked!(upb_MiniTable_FindFieldByNumber);
        assert_linked!(upb_MiniTable_GetFieldByIndex);
        assert_linked!(upb_MiniTable_SubMessage);
    }
}
