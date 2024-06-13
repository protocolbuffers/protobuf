use crate::{RawArray, RawMap, RawMessage, StringView};

// Transcribed from google3/third_party/upb/upb/message/value.h
#[repr(C)]
#[derive(Clone, Copy)]
pub union upb_MessageValue {
    pub bool_val: bool,
    pub float_val: std::ffi::c_float,
    pub double_val: std::ffi::c_double,
    pub uint32_val: u32,
    pub int32_val: i32,
    pub uint64_val: u64,
    pub int64_val: i64,
    // TODO: Replace this `RawMessage` with the const type.
    pub array_val: Option<RawArray>,
    pub map_val: Option<RawMap>,
    pub msg_val: Option<RawMessage>,
    pub str_val: StringView,

    tagged_msg_val: *const std::ffi::c_void,
}

impl upb_MessageValue {
    pub fn zeroed() -> Self {
        // SAFETY: zero bytes is a valid representation for at least one value in the
        // union (actually valid for all of them).
        unsafe { std::mem::zeroed() }
    }
}

#[repr(C)]
#[derive(Clone, Copy)]
pub union upb_MutableMessageValue {
    pub array: Option<RawArray>,
    pub map: Option<RawMap>,
    pub msg: Option<RawMessage>,
}
