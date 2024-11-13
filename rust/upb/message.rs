// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use super::opaque_pointee::opaque_pointee;
use super::{
    upb_ExtensionRegistry, upb_MiniTable, upb_MiniTableField, RawArena, RawArray, RawMap,
    StringView,
};
use core::ptr::NonNull;

opaque_pointee!(upb_Message);
pub type RawMessage = NonNull<upb_Message>;

extern "C" {
    /// # Safety
    /// - `mini_table` and `arena` must be valid to deref
    pub fn upb_Message_New(mini_table: *const upb_MiniTable, arena: RawArena)
    -> Option<RawMessage>;

    /// # Safety
    /// - `m` and `mini_table` must be valid to deref
    /// - `mini_table` must be the MiniTable associated with `m`
    pub fn upb_Message_Clear(m: RawMessage, mini_table: *const upb_MiniTable);

    /// # Safety
    /// - `m` and `f` must be valid to deref
    /// - `f` must be a field associated with `f`
    pub fn upb_Message_ClearBaseField(m: RawMessage, f: *const upb_MiniTableField);

    /// # Safety
    /// - All four arguments must be valid to deref
    /// - `mini_table` must be the MiniTable associated with both `dst` and
    ///   `src`
    pub fn upb_Message_DeepCopy(
        dst: RawMessage,
        src: RawMessage,
        mini_table: *const upb_MiniTable,
        arena: RawArena,
    );

    /// # Safety
    /// - All three arguments must be valid to deref
    /// - `mini_table` must be the MiniTable associated with `m`
    pub fn upb_Message_DeepClone(
        m: RawMessage,
        mini_table: *const upb_MiniTable,
        arena: RawArena,
    ) -> Option<RawMessage>;

    /// # Safety
    /// - `m` and `f` must be valid to deref
    /// - `f` must be a bool field associated with `m`
    pub fn upb_Message_GetBool(
        m: RawMessage,
        f: *const upb_MiniTableField,
        default_val: bool,
    ) -> bool;

    /// # Safety
    /// - `m` and `f` must be valid to deref
    /// - `f` must be an i32 field associated with `m`
    pub fn upb_Message_GetInt32(
        m: RawMessage,
        f: *const upb_MiniTableField,
        default_val: i32,
    ) -> i32;

    /// # Safety
    /// - `m` and `f` must be valid to deref
    /// - `f` must be an i64 field associated with `m`
    pub fn upb_Message_GetInt64(
        m: RawMessage,
        f: *const upb_MiniTableField,
        default_val: i64,
    ) -> i64;

    /// # Safety
    /// - `m` and `f` must be valid to deref
    /// - `f` must be a u32 field associated with `m`
    pub fn upb_Message_GetUInt32(
        m: RawMessage,
        f: *const upb_MiniTableField,
        default_val: u32,
    ) -> u32;

    /// # Safety
    /// - `m` and `f` must be valid to deref
    /// - `f` must be a u64 field associated with `m`
    pub fn upb_Message_GetUInt64(
        m: RawMessage,
        f: *const upb_MiniTableField,
        default_val: u64,
    ) -> u64;

    /// # Safety
    /// - `m` and `f` must be valid to deref
    /// - `f` must be a f32 field associated with `m`
    pub fn upb_Message_GetFloat(
        m: RawMessage,
        f: *const upb_MiniTableField,
        default_val: f32,
    ) -> f32;

    /// # Safety
    /// - `m` and `f` must be valid to deref
    /// - `f` must be a f64 field associated with `m`
    pub fn upb_Message_GetDouble(
        m: RawMessage,
        f: *const upb_MiniTableField,
        default_val: f64,
    ) -> f64;

    /// # Safety
    /// - `m` and `f` must be valid to deref
    /// - `f` must be a string or bytes field associated with `m`
    pub fn upb_Message_GetString(
        m: RawMessage,
        f: *const upb_MiniTableField,
        default_val: StringView,
    ) -> StringView;

    /// Gets the const upb_Message* that is assigned to the field.
    ///
    /// This may return None which must be treated the same as if it returned
    /// Some of a valid RawMessage that is was the default message instance.
    ///
    /// # Safety
    /// - `m` and `f` must be valid to deref
    /// - `f` must be a message-typed field associated with `m`
    pub fn upb_Message_GetMessage(
        m: RawMessage,
        f: *const upb_MiniTableField,
    ) -> Option<RawMessage>;

    /// Gets or creates a mutable upb_Message* assigned to the corresponding
    /// field in the message.
    ///
    /// This will only return None if the Arena allocation fails.
    ///
    /// # Safety
    /// - All arguments must be valid to deref
    /// - `mini_table` must be the MiniTable associated with `m`
    /// - `f` must be a message-typed field associated with `m`
    pub fn upb_Message_GetOrCreateMutableMessage(
        m: RawMessage,
        mini_table: *const upb_MiniTable,
        f: *const upb_MiniTableField,
        arena: RawArena,
    ) -> Option<RawMessage>;

    /// Gets the const upb_Array* that is assigned to the field.
    ///
    /// This may return None which must be treated the same as if it returned
    /// Some of a valid RawArray that is empty.
    ///
    /// # Safety
    /// - `m` and `f` must be valid to deref
    /// - `f` must be a repeated field associated with `m`
    pub fn upb_Message_GetArray(m: RawMessage, f: *const upb_MiniTableField) -> Option<RawArray>;

    /// Gets or creates a mutable upb_Array* assigned to the corresponding field
    /// in the message.
    ///
    /// This will only return None if the Arena allocation fails.
    ///
    /// # Safety
    /// - `m` and `f` must be valid to deref
    /// - `f` must be a map field associated with `m`
    pub fn upb_Message_GetOrCreateMutableArray(
        m: RawMessage,
        f: *const upb_MiniTableField,
        arena: RawArena,
    ) -> Option<RawArray>;

    /// Gets the const upb_Map* that is assigned to the field.
    ///
    /// This may return None which must be treated the same as if it returned
    /// Some of a valid RawMap that is empty.
    ///
    /// # Safety
    /// - `m` and `f` must be valid to deref
    /// - `f` must be a repeated field associated with `m`
    pub fn upb_Message_GetMap(m: RawMessage, f: *const upb_MiniTableField) -> Option<RawMap>;

    /// Gets or creates a mutable upb_Map* assigned to the corresponding field
    /// in the message.
    ///
    /// This will only return None if the Arena allocation fails.
    ///
    /// # Safety
    /// - `m` and `f` must be valid to deref
    /// - `map_entry_mini_table` must be the MiniTable associated with `f`
    /// - `f` must be a map field associated with `m`
    pub fn upb_Message_GetOrCreateMutableMap(
        m: RawMessage,
        map_entry_mini_table: *const upb_MiniTable,
        f: *const upb_MiniTableField,
        arena: RawArena,
    ) -> Option<RawMap>;

    /// # Safety
    /// - `m` and `f` must be valid to deref
    /// - `mini_table` must be the MiniTable associated with `m`
    pub fn upb_Message_HasBaseField(m: RawMessage, f: *const upb_MiniTableField) -> bool;

    /// # Safety
    /// - `m` and `f` must be valid to deref
    /// - `f` must be a field associated with `m`
    /// - `val` must be a pointer to legally readable memory of the correct type
    ///   for the field described by `f`
    pub fn upb_Message_SetBaseField(
        m: RawMessage,
        f: *const upb_MiniTableField,
        val: *const core::ffi::c_void,
    );

    /// # Safety
    /// - All four arguments must be valid to deref
    /// - `mini_table` must be the MiniTable associated with both `m1` and `m2`
    pub fn upb_Message_IsEqual(
        m1: RawMessage,
        m2: RawMessage,
        mini_table: *const upb_MiniTable,
        options: i32,
    ) -> bool;

    /// # Safety
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

    /// # Safety
    /// - `m` and `f` must be valid to deref
    /// - `f` must be a bool field associated with `f`
    pub fn upb_Message_SetBaseFieldBool(m: RawMessage, f: *const upb_MiniTableField, val: bool);

    /// # Safety
    /// - `m` and `f` must be valid to deref
    /// - `f` must be an i32 field associated with `m`
    pub fn upb_Message_SetBaseFieldInt32(m: RawMessage, f: *const upb_MiniTableField, val: i32);

    /// # Safety
    /// - `m` and `f` must be valid to deref
    /// - `f` must be an i64 field associated with `m`
    pub fn upb_Message_SetBaseFieldInt64(m: RawMessage, f: *const upb_MiniTableField, val: i64);

    /// # Safety
    /// - `m` and `f` must be valid to deref
    /// - `f` must be a u32 field associated with `m`
    pub fn upb_Message_SetBaseFieldUInt32(m: RawMessage, f: *const upb_MiniTableField, val: u32);

    /// # Safety
    /// - `m` and `f` must be valid to deref
    /// - `f` must be a u64 field associated with `m`
    pub fn upb_Message_SetBaseFieldUInt64(m: RawMessage, f: *const upb_MiniTableField, val: u64);

    /// # Safety
    /// - `m` and `f` must be valid to deref
    /// - `f` must be an f32 field associated with `m`
    pub fn upb_Message_SetBaseFieldFloat(m: RawMessage, f: *const upb_MiniTableField, val: f32);

    /// # Safety
    /// - `m` and `f` must be valid to deref
    /// - `f` must be an f64 field associated with `m`
    pub fn upb_Message_SetBaseFieldDouble(m: RawMessage, f: *const upb_MiniTableField, val: f64);

    /// # Safety
    /// - `m` and `f` must be valid to deref
    /// - `f` must be an string or bytes field associated with `m`
    pub fn upb_Message_SetBaseFieldString(
        m: RawMessage,
        f: *const upb_MiniTableField,
        val: StringView,
    );

    /// # Safety
    /// - `m` and `f` must be valid to deref
    /// - `f` must be a message-typed field associated with `m`
    pub fn upb_Message_SetBaseFieldMessage(
        m: RawMessage,
        f: *const upb_MiniTableField,
        val: RawMessage,
    );

    /// Returns the field number of which oneof field is set, or 0 if none are.
    /// `f` is any arbitrary field contained within the oneof.
    ///
    /// # Safety
    /// - `m` and `f` must be valid to deref
    /// - `f` must be a field within a oneof associated with `m`
    pub fn upb_Message_WhichOneofFieldNumber(m: RawMessage, f: *const upb_MiniTableField) -> u32;
}

#[cfg(test)]
mod tests {
    use super::*;
    use googletest::gtest;

    #[gtest]
    fn assert_message_linked() {
        use crate::assert_linked;
        assert_linked!(upb_Message_New);
        assert_linked!(upb_Message_Clear);
        assert_linked!(upb_Message_ClearBaseField);
        assert_linked!(upb_Message_DeepCopy);
        assert_linked!(upb_Message_DeepClone);
        assert_linked!(upb_Message_GetBool);
        assert_linked!(upb_Message_GetInt32);
        assert_linked!(upb_Message_GetInt64);
        assert_linked!(upb_Message_GetUInt32);
        assert_linked!(upb_Message_GetUInt64);
        assert_linked!(upb_Message_GetFloat);
        assert_linked!(upb_Message_GetDouble);
        assert_linked!(upb_Message_GetString);
        assert_linked!(upb_Message_GetMessage);
        assert_linked!(upb_Message_GetOrCreateMutableMessage);
        assert_linked!(upb_Message_GetArray);
        assert_linked!(upb_Message_GetOrCreateMutableArray);
        assert_linked!(upb_Message_GetMap);
        assert_linked!(upb_Message_GetOrCreateMutableMap);
        assert_linked!(upb_Message_HasBaseField);
        assert_linked!(upb_Message_SetBaseField);
        assert_linked!(upb_Message_IsEqual);
        assert_linked!(upb_Message_MergeFrom);
        assert_linked!(upb_Message_SetBaseFieldBool);
        assert_linked!(upb_Message_SetBaseFieldInt32);
        assert_linked!(upb_Message_SetBaseFieldInt64);
        assert_linked!(upb_Message_SetBaseFieldUInt32);
        assert_linked!(upb_Message_SetBaseFieldUInt64);
        assert_linked!(upb_Message_SetBaseFieldFloat);
        assert_linked!(upb_Message_SetBaseFieldDouble);
        assert_linked!(upb_Message_SetBaseFieldString);
        assert_linked!(upb_Message_SetBaseFieldMessage);
        assert_linked!(upb_Message_WhichOneofFieldNumber);
    }
}
