// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use super::sys::{
    message::array as sys_array, message::map as sys_map, message::message as sys_msg,
    mini_table::mini_table as sys_mt,
};
use super::StringView;
use super::{Arena, AssociatedMiniTable};
use core::marker::PhantomData;
use paste::paste;

pub type RawMessage = sys_msg::RawMessage;

#[derive(Debug)]
pub struct MessagePtr<T> {
    raw: RawMessage,
    _phantom: PhantomData<T>,
}

impl<T> Copy for MessagePtr<T> {}

impl<T> Clone for MessagePtr<T> {
    fn clone(&self) -> Self {
        *self
    }
}

impl<T> MessagePtr<T> {
    pub fn raw(&self) -> RawMessage {
        self.raw
    }
}

macro_rules! scalar_accessors {
    ( $ty:ty, $rust_ty_name:ident, $upb_ty_name:ident) => {
        paste! {
            impl<T: AssociatedMiniTable> MessagePtr<T> {
                /// # Safety
                /// - `self` must be legally dereferenceable.
                /// - The field at `index` must be a $ty field.
                pub unsafe fn [< get_ $rust_ty_name _at_index >] (self, index: u32, default_value: $ty) -> $ty {
                    let f = unsafe { sys_mt::upb_MiniTable_GetFieldByIndex(T::mini_table(), index) };
                    unsafe { sys_msg::[< upb_Message_Get $upb_ty_name >](self.raw, f, default_value) }
                }

                /// # Safety
                /// - `self` must be legally dereferenceable to a mutable message.
                /// - The field at `index` must be a $ty field.
                pub unsafe fn [< set_base_field_ $rust_ty_name _at_index >] (self, index: u32, value: $ty) {
                    let f = unsafe { sys_mt::upb_MiniTable_GetFieldByIndex(T::mini_table(), index) };
                    unsafe { sys_msg::[< upb_Message_SetBaseField $upb_ty_name >](self.raw, f, value) }
                }
            }
        }
    };
}

scalar_accessors!(bool, bool, Bool);
scalar_accessors!(i32, i32, Int32);
scalar_accessors!(i64, i64, Int64);
scalar_accessors!(u32, u32, UInt32);
scalar_accessors!(u64, u64, UInt64);
scalar_accessors!(f32, f32, Float);
scalar_accessors!(f64, f64, Double);
scalar_accessors!(StringView, string, String);

impl<T: AssociatedMiniTable> MessagePtr<T> {
    /// Constructs a new mutable message pointer.
    /// Will only return None if arena allocation fails.
    pub fn new(arena: &Arena) -> Option<MessagePtr<T>> {
        let raw = unsafe { sys_msg::upb_Message_New(T::mini_table(), arena.raw()) };
        raw.map(|raw| MessagePtr { raw, _phantom: PhantomData })
    }

    /// # Safety
    /// - `raw` must be a dereferenceable message pointer of message associated with `T::mini_table()`.
    pub unsafe fn wrap(raw: RawMessage) -> MessagePtr<T> {
        MessagePtr { raw, _phantom: PhantomData }
    }

    /// # Safety
    /// - `self` must be legally dereferenceable to a mutable message.
    pub unsafe fn clear(self) {
        unsafe { sys_msg::upb_Message_Clear(self.raw, T::mini_table()) }
    }

    /// Copies the contents of `src` to `self`, using `arena` for allocations.
    /// `arena` should be the one associated with `self`.
    /// # Safety
    /// - `self` must be legally dereferenceable to a mutable message.
    pub unsafe fn deep_copy(self, src: Self, arena: &Arena) -> bool {
        unsafe { sys_msg::upb_Message_DeepCopy(self.raw, src.raw, T::mini_table(), arena.raw()) }
    }

    /// Returns a pointer to a mutable message which is a clone of `self` on `arena`.
    /// # Safety
    /// - `self` must be legally dereferenceable.
    pub unsafe fn deep_clone(self, arena: &Arena) -> Option<MessagePtr<T>> {
        let raw = unsafe { sys_msg::upb_Message_DeepClone(self.raw, T::mini_table(), arena.raw()) };
        raw.map(|raw| MessagePtr { raw, _phantom: PhantomData })
    }

    /// # Safety
    /// - `self` must be legally dereferenceable.
    /// - `index` must be within bounds of `T::mini_table()`.
    pub unsafe fn clear_field_at_index(self, index: u32) {
        let f = unsafe { sys_mt::upb_MiniTable_GetFieldByIndex(T::mini_table(), index) };
        unsafe { sys_msg::upb_Message_ClearBaseField(self.raw, f) }
    }

    /// # Safety
    /// - `self` must be legally dereferenceable.
    /// - `index` must be within bounds of `T::mini_table()`.
    pub unsafe fn has_field_at_index(self, index: u32) -> bool {
        let f = unsafe { sys_mt::upb_MiniTable_GetFieldByIndex(T::mini_table(), index) };
        unsafe { sys_msg::upb_Message_HasBaseField(self.raw, f) }
    }

    /// Returns a constant message pointer, or None if the field is not present.
    ///
    /// # Safety
    /// - `self` must be legally dereferenceable.
    /// - The field at `index` must be a message field of type `ChildT`.
    pub unsafe fn get_message_at_index<ChildT>(self, index: u32) -> Option<MessagePtr<ChildT>> {
        let f = unsafe { sys_mt::upb_MiniTable_GetFieldByIndex(T::mini_table(), index) };

        let raw = unsafe { sys_msg::upb_Message_GetMessage(self.raw, f) };
        raw.map(|raw| MessagePtr { raw, _phantom: PhantomData })
    }

    /// # Safety
    /// - `self` must be legally dereferenceable.
    /// - The field at `index` must be a message field of type `ChildT`.
    /// - Caller must ensure that `value` outlives `self` (typically by being on the same arena).
    pub unsafe fn set_base_field_message_at_index<ChildT>(
        self,
        index: u32,
        value: MessagePtr<ChildT>,
    ) {
        let f = unsafe { sys_mt::upb_MiniTable_GetFieldByIndex(T::mini_table(), index) };
        unsafe { sys_msg::upb_Message_SetBaseFieldMessage(self.raw, f, value.raw) }
    }

    /// Returns a mutable message pointer, creating the field if it is not present.
    ///
    /// # Safety
    /// - `self` must be legally dereferenceable to a mutable message.
    /// - The field at `index` must be a message field of type `ChildT`.
    pub unsafe fn get_or_create_mutable_message_at_index<ChildT>(
        self,
        index: u32,
        arena: &Arena,
    ) -> Option<MessagePtr<ChildT>> {
        let f = unsafe { sys_mt::upb_MiniTable_GetFieldByIndex(T::mini_table(), index) };

        let raw =
            unsafe { sys_msg::upb_Message_GetOrCreateMutableMessage(self.raw, f, arena.raw()) };
        raw.map(|raw| MessagePtr { raw, _phantom: PhantomData })
    }

    /// Returns a constant pointer to an array. May return None if the repeated field is empty.
    ///
    /// # Safety
    /// - `self` must be legally dereferenceable.
    /// - The field at `index` must be a repeated field.
    pub unsafe fn get_array_at_index(self, index: u32) -> Option<sys_array::RawArray> {
        let f = unsafe { sys_mt::upb_MiniTable_GetFieldByIndex(T::mini_table(), index) };
        let raw = unsafe { sys_msg::upb_Message_GetArray(self.raw, f) };
        raw
    }

    /// # Safety
    /// - `self` must be legally dereferenceable.
    /// - The field at `index` must be a repeated field.
    /// - Caller must ensure that `value` outlives `self` (typically by being on the same arena).
    pub unsafe fn set_array_at_index(self, index: u32, value: sys_array::RawArray) {
        let f = unsafe { sys_mt::upb_MiniTable_GetFieldByIndex(T::mini_table(), index) };
        let value_ptr: *const *const core::ffi::c_void =
            &(value.as_ptr() as *const core::ffi::c_void);
        unsafe {
            sys_msg::upb_Message_SetBaseField(self.raw, f, value_ptr as *const core::ffi::c_void)
        }
    }

    /// Returns a mutable pointer to an array. Will only return None if arena allocation fails.
    ///
    /// # Safety
    /// - `self` must be legally dereferenceable to a mutable message.
    /// - The field at `index` must be a repeated field.
    pub unsafe fn get_or_create_mutable_array_at_index(
        self,
        index: u32,
        arena: &Arena,
    ) -> Option<sys_array::RawArray> {
        let f = unsafe { sys_mt::upb_MiniTable_GetFieldByIndex(T::mini_table(), index) };
        let raw = unsafe { sys_msg::upb_Message_GetOrCreateMutableArray(self.raw, f, arena.raw()) };
        raw
    }

    /// Returns a constant pointer to a map. May return None if the map is empty.
    ///
    /// # Safety
    /// - `self` must be legally dereferenceable.
    /// - The field at `index` must be a map.
    pub unsafe fn get_map_at_index(self, index: u32) -> Option<sys_map::RawMap> {
        let f = unsafe { sys_mt::upb_MiniTable_GetFieldByIndex(T::mini_table(), index) };
        unsafe { sys_msg::upb_Message_GetMap(self.raw, f) }
    }

    /// # Safety
    /// - `self` must be legally dereferenceable.
    /// - The field at `index` must be a map field.
    /// - Caller must ensure that `value` outlives `self` (typically by being on the same arena).
    pub unsafe fn set_map_at_index(self, index: u32, value: sys_map::RawMap) {
        let f = unsafe { sys_mt::upb_MiniTable_GetFieldByIndex(T::mini_table(), index) };
        let value_ptr: *const *const core::ffi::c_void =
            &(value.as_ptr() as *const core::ffi::c_void);
        unsafe {
            sys_msg::upb_Message_SetBaseField(self.raw, f, value_ptr as *const core::ffi::c_void)
        }
    }

    /// Returns a mutable pointer to a map. Will only return None if arena allocation fails.
    ///
    /// # Safety
    /// - `self` must be legally dereferenceable to a mutable message.
    /// - The field at `index` must be a map.
    pub unsafe fn get_or_create_mutable_map_at_index(
        self,
        index: u32,
        arena: &Arena,
    ) -> Option<sys_map::RawMap> {
        unsafe {
            let f = sys_mt::upb_MiniTable_GetFieldByIndex(T::mini_table(), index);
            let map_entry_mini_table = sys_mt::upb_MiniTable_SubMessage(f);
            sys_msg::upb_Message_GetOrCreateMutableMap(
                self.raw,
                map_entry_mini_table,
                f,
                arena.raw(),
            )
        }
    }

    /// Returns the number associated with the active field in a oneof, or 0 if the oneof is unset.
    /// `index` can be the field number of any field in the oneof.
    ///
    /// # Safety
    /// - `self` must be legally dereferenceable.
    /// - The field at `index` must be part of a oneof.
    pub unsafe fn which_oneof_field_number_by_index(self, index: u32) -> u32 {
        let f = unsafe { sys_mt::upb_MiniTable_GetFieldByIndex(T::mini_table(), index) };
        unsafe { sys_msg::upb_Message_WhichOneofFieldNumber(self.raw, f) }
    }
}
