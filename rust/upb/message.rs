use crate::opaque_pointee::opaque_pointee;
use crate::{upb_MiniTable, upb_MiniTableField, RawArena};
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
}
