// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use super::opaque_pointee::opaque_pointee;
use core::ptr::NonNull;

opaque_pointee!(upb_MiniTable);
pub type RawMiniTable = NonNull<upb_MiniTable>;

opaque_pointee!(upb_MiniTableField);
pub type RawMiniTableField = NonNull<upb_MiniTableField>;

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
