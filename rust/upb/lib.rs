mod arena;
pub use arena::{upb_Arena, Arena, RawArena};

mod array;
pub use array::{
    upb_Array, upb_Array_Append, upb_Array_DataPtr, upb_Array_Get, upb_Array_GetMutable,
    upb_Array_MutableDataPtr, upb_Array_New, upb_Array_Resize, upb_Array_Set, upb_Array_Size,
    RawArray,
};

mod ctype;
pub use ctype::CType;

mod extension_registry;
pub use extension_registry::{upb_ExtensionRegistry, RawExtensionRegistry};

mod map;
pub use map::{
    upb_Map, upb_Map_Clear, upb_Map_Delete, upb_Map_Get, upb_Map_Insert, upb_Map_New, upb_Map_Next,
    upb_Map_Size, MapInsertStatus, RawMap, __rust_proto_kUpb_Map_Begin,
};

mod message;
pub use message::{upb_Message, upb_Message_DeepClone, upb_Message_DeepCopy, RawMessage};

mod message_value;
pub use message_value::{upb_MessageValue, upb_MutableMessageValue};

mod mini_table;
pub use mini_table::{upb_MiniTable, RawMiniTable};

mod opaque_pointee;

mod string_view;
pub use string_view::StringView;

mod wire;
pub use wire::{upb_Decode, upb_Encode, DecodeStatus, EncodeStatus};
