// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#![deny(unsafe_op_in_unsafe_fn)]
#![cfg_attr(not(bzl), allow(unused_imports))]

mod arena;
pub use arena::Arena;

mod associated_mini_table;
pub use associated_mini_table::{AssociatedMiniTable, AssociatedMiniTableEnum};

mod text;
pub use text::debug_string;

mod message;
pub use message::MessagePtr;

mod owned_arena_box;
pub use owned_arena_box::OwnedArenaBox;

pub mod wire;

// When building in Bazel `sys` is a separate crate; when building in Cargo it is a module.
#[cfg(bzl)]
extern crate sys;
#[cfg(not(bzl))]
#[path = "sys/lib.rs"]
mod sys;

// All sys re-exports below here intended to be burned down.
pub use sys::base::ctype::CType;
pub use sys::base::string_view::StringView;
pub use sys::mem::arena::{upb_Arena, RawArena};
pub use sys::message::array::{
    upb_Array, upb_Array_Append, upb_Array_DataPtr, upb_Array_Get, upb_Array_GetMutable,
    upb_Array_MutableDataPtr, upb_Array_New, upb_Array_Reserve, upb_Array_Resize, upb_Array_Set,
    upb_Array_Size, RawArray,
};
pub use sys::message::map::{
    upb_Map_Clear, upb_Map_Delete, upb_Map_Get, upb_Map_GetMutable, upb_Map_Insert, upb_Map_New,
    upb_Map_Next, upb_Map_Size, MapInsertStatus, RawMap, UPB_MAP_BEGIN,
};
pub use sys::message::message::{
    upb_Message_ClearExtension, upb_Message_DeepClone, upb_Message_DeepCopy,
    upb_Message_GetExtensionArray, upb_Message_GetExtensionBool, upb_Message_GetExtensionDouble,
    upb_Message_GetExtensionFloat, upb_Message_GetExtensionInt32, upb_Message_GetExtensionInt64,
    upb_Message_GetExtensionMessage, upb_Message_GetExtensionMutableArray,
    upb_Message_GetExtensionString, upb_Message_GetExtensionUInt32, upb_Message_GetExtensionUInt64,
    upb_Message_GetMap, upb_Message_GetOrCreateMutableMap, upb_Message_HasExtension,
    upb_Message_IsEqual, upb_Message_MergeFrom, upb_Message_SetBaseField, upb_Message_SetExtension,
    upb_Message_SetExtensionBool, upb_Message_SetExtensionDouble, upb_Message_SetExtensionFloat,
    upb_Message_SetExtensionInt32, upb_Message_SetExtensionInt64, upb_Message_SetExtensionMessage,
    upb_Message_SetExtensionString, upb_Message_SetExtensionUInt32, upb_Message_SetExtensionUInt64,
    upb_Message_WhichOneofFieldNumber, RawMessage,
};

pub use sys::message::message_value::upb_MessageValue;
pub use sys::mini_table::extension_registry::{
    upb_ExtensionRegistry_Add, upb_ExtensionRegistry_New, RawExtensionRegistry,
};
pub use sys::mini_table::mini_table::{
    upb_MiniTable, upb_MiniTableEnum, upb_MiniTableEnum_Build, upb_MiniTableExtension,
    upb_MiniTableExtension_Build, upb_MiniTableExtension_SetSubEnum,
    upb_MiniTableExtension_SetSubMessage, upb_MiniTable_Build, upb_MiniTable_Link,
    upb_MiniTable_SubMessage, upb_Status, RawMiniTable, RawMiniTableEnum, RawMiniTableExtension,
};
