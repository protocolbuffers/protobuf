// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#![deny(unsafe_op_in_unsafe_fn)]
#![cfg_attr(not(bzl), allow(unused_imports))]

mod arena;

pub use arena::{upb_Arena, Arena, RawArena};

mod array;
pub use array::{
    upb_Array, upb_Array_Append, upb_Array_DataPtr, upb_Array_Get, upb_Array_GetMutable,
    upb_Array_MutableDataPtr, upb_Array_New, upb_Array_Reserve, upb_Array_Resize, upb_Array_Set,
    upb_Array_Size, RawArray,
};

mod associated_mini_table;
pub use associated_mini_table::AssociatedMiniTable;

mod ctype;
pub use ctype::CType;

mod extension_registry;
pub use extension_registry::{upb_ExtensionRegistry, RawExtensionRegistry};

mod map;
pub use map::{
    upb_Map, upb_Map_Clear, upb_Map_Delete, upb_Map_Get, upb_Map_Insert, upb_Map_New, upb_Map_Next,
    upb_Map_Size, MapInsertStatus, RawMap, UPB_MAP_BEGIN,
};

mod message;
pub use message::*;

mod message_value;
pub use message_value::{upb_MessageValue, upb_MutableMessageValue};

mod mini_table;
pub use mini_table::{
    upb_MiniTable, upb_MiniTableField, upb_MiniTable_FindFieldByNumber,
    upb_MiniTable_GetFieldByIndex, upb_MiniTable_SubMessage, RawMiniTable, RawMiniTableField,
};

mod opaque_pointee;

mod owned_arena_box;
pub use owned_arena_box::OwnedArenaBox;

mod string_view;
pub use string_view::StringView;

mod text;
pub use text::debug_string;

pub mod wire;
pub use wire::{upb_Decode, DecodeStatus, EncodeStatus};

#[cfg(test)]
mod test {
    #[macro_export]
    /// Force a compiler error if the passed in function is not linked.
    macro_rules! assert_linked {
        ($($vals:tt)+) => {
            let _ = std::hint::black_box($($vals)+ as *const ());
        }
    }
}
