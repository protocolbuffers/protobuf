// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use super::{RawArray, RawMap, RawMessage, StringView};

// Transcribed from google3/third_party/upb/upb/message/value.h
#[repr(C)]
#[derive(Clone, Copy)]
pub union upb_MessageValue {
    pub bool_val: bool,
    pub float_val: core::ffi::c_float,
    pub double_val: core::ffi::c_double,
    pub uint32_val: u32,
    pub int32_val: i32,
    pub uint64_val: u64,
    pub int64_val: i64,
    // TODO: Replace this `RawMessage` with the const type.
    pub array_val: Option<RawArray>,
    pub map_val: Option<RawMap>,
    pub msg_val: Option<RawMessage>,
    pub str_val: StringView,

    tagged_msg_val: *const core::ffi::c_void,
}

impl upb_MessageValue {
    pub fn zeroed() -> Self {
        // SAFETY: zero bytes is a valid representation for at least one value in the
        // union (actually valid for all of them).
        unsafe { core::mem::zeroed() }
    }
}

#[repr(C)]
#[derive(Clone, Copy)]
pub union upb_MutableMessageValue {
    pub array: Option<RawArray>,
    pub map: Option<RawMap>,
    pub msg: Option<RawMessage>,
}
