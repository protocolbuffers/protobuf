// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use super::mini_table::MiniTableFieldPtr;
use super::sys::{message::array as sys_array, message::message as sys_msg};
use super::StringView;
use super::{Arena, AssociatedMiniTable};
use core::marker::PhantomData;

pub type RawMessage = sys_msg::RawMessage;

#[derive(Copy, Clone)]
pub struct MessagePtr<T: AssociatedMiniTable> {
    raw: RawMessage,
    _phantom: PhantomData<T>,
}

impl<T: AssociatedMiniTable> MessagePtr<T> {
    /// Constructs a new mutable message pointer.
    /// Will only return None if arena allocation fails.
    pub fn new(arena: &Arena) -> Option<MessagePtr<T>> {
        let raw = unsafe { sys_msg::upb_Message_New(T::mini_table(), arena.raw()) };
        raw.map(|raw| MessagePtr { raw, _phantom: PhantomData })
    }

    fn wrap(raw: RawMessage) -> MessagePtr<T> {
        MessagePtr { raw, _phantom: PhantomData }
    }

    /// # Safety
    /// - `self` must be a legally dereferencable to a mutable message.
    pub unsafe fn clear(self) {
        unsafe { sys_msg::upb_Message_Clear(self.raw, T::mini_table()) }
    }

    /// # Safety
    /// - `self` must be a legally dereferencable to a mutable message.
    pub unsafe fn deep_copy(self, src: Self, arena: &Arena) -> bool {
        unsafe { sys_msg::upb_Message_DeepCopy(self.raw, src.raw, T::mini_table(), arena.raw()) }
    }

    /// # Safety
    /// - `self` must be a legally dereferencable.
    pub unsafe fn deep_clone(self, arena: &Arena) -> Option<MessagePtr<T>> {
        let raw = unsafe { sys_msg::upb_Message_DeepClone(self.raw, T::mini_table(), arena.raw()) };
        raw.map(MessagePtr::wrap)
    }

    /// # Safety
    /// - `self` must be a legally dereferencable.
    /// - `f` must be a bool field.
    pub unsafe fn get_bool(self, f: MiniTableFieldPtr<T>, default_value: bool) -> bool {
        unsafe { sys_msg::upb_Message_GetBool(self.raw, f.raw, default_value) }
    }

    /// # Safety
    /// - `self` must be a legally dereferencable.
    /// - `f` must be a bool field.
    pub unsafe fn get_i32(self, f: MiniTableFieldPtr<T>, default_value: i32) -> i32 {
        unsafe { sys_msg::upb_Message_GetInt32(self.raw, f.raw, default_value) }
    }

    /// # Safety
    /// - `self` must be a legally dereferencable.
    /// - `f` must be a int64 field.
    pub unsafe fn get_i64(self, f: MiniTableFieldPtr<T>, default_value: i64) -> i64 {
        unsafe { sys_msg::upb_Message_GetInt64(self.raw, f.raw, default_value) }
    }

    /// # Safety
    /// - `self` must be a legally dereferencable.
    /// - `f` must be a uint32 field.
    pub unsafe fn get_u32(self, f: MiniTableFieldPtr<T>, default_value: u32) -> u32 {
        unsafe { sys_msg::upb_Message_GetUInt32(self.raw, f.raw, default_value) }
    }

    /// # Safety
    /// - `self` must be a legally dereferencable.
    /// - `f` must be a uint64 field.
    pub unsafe fn get_u64(self, f: MiniTableFieldPtr<T>, default_value: u64) -> u64 {
        unsafe { sys_msg::upb_Message_GetUInt64(self.raw, f.raw, default_value) }
    }

    /// # Safety
    /// - `self` must be a legally dereferencable.
    /// - `f` must be a float field.
    pub unsafe fn get_f32(self, f: MiniTableFieldPtr<T>, default_value: f32) -> f32 {
        unsafe { sys_msg::upb_Message_GetFloat(self.raw, f.raw, default_value) }
    }

    /// # Safety
    /// - `self` must be a legally dereferencable.
    /// - `f` must be a double field.
    pub unsafe fn get_f64(self, f: MiniTableFieldPtr<T>, default_value: f64) -> f64 {
        unsafe { sys_msg::upb_Message_GetDouble(self.raw, f.raw, default_value) }
    }

    /// # Safety
    /// - `self` must be a legally dereferencable.
    /// - `f` must be a string field.
    pub unsafe fn get_string(
        self,
        f: MiniTableFieldPtr<T>,
        default_value: StringView,
    ) -> StringView {
        unsafe { sys_msg::upb_Message_GetString(self.raw, f.raw, default_value) }
    }

    /// Returns an constant message pointer, or None if the field is not present.
    ///
    /// # Safety
    /// - `self` must be a legally dereferencable.
    /// - `f` must be a message field.
    pub unsafe fn get_message(self, f: MiniTableFieldPtr<T>) -> Option<MessagePtr<T>> {
        let raw = unsafe { sys_msg::upb_Message_GetMessage(self.raw, f.raw) };
        raw.map(MessagePtr::wrap)
    }

    /// Returns a mutable message pointer, creating the field if it is not present.
    ///
    /// # Safety
    /// - `self` must be a legally deferencable.
    /// - `f` must be a message field.
    pub unsafe fn get_or_create_mutable_message(
        self,
        f: MiniTableFieldPtr<T>,
        arena: &Arena,
    ) -> Option<MessagePtr<T>> {
        let raw = unsafe {
            sys_msg::upb_Message_GetOrCreateMutableMessage(
                self.raw,
                T::mini_table(),
                f.raw,
                arena.raw(),
            )
        };
        raw.map(MessagePtr::wrap)
    }

    /// Returns an constant pointer to an array. May return None if the repeated field is empty.
    ///
    /// # Safety
    /// - `self` must be a legally dereferencable.
    /// - `f` must be a repeated field.
    pub unsafe fn get_array(self, f: MiniTableFieldPtr<T>) -> Option<sys_array::RawArray> {
        let raw = unsafe { sys_msg::upb_Message_GetArray(self.raw, f.raw) };
        raw
    }

    /// Returns a mutable pointer to an array. Will only return None if arena allocation fails.
    ///
    /// # Safety
    /// - `self` must be a legally dereferencable.
    /// - `f` must be a repeated field.
    pub unsafe fn get_or_create_mutable_array(
        self,
        f: MiniTableFieldPtr<T>,
        arena: &Arena,
    ) -> Option<sys_array::RawArray> {
        let raw =
            unsafe { sys_msg::upb_Message_GetOrCreateMutableArray(self.raw, f.raw, arena.raw()) };
        raw
    }
}
