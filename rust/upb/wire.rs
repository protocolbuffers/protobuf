// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use super::{upb_ExtensionRegistry, upb_MiniTable, Arena, RawArena, RawMessage};

// LINT.IfChange(encode_status)
#[repr(C)]
#[derive(PartialEq, Eq, Copy, Clone, Debug)]
pub enum EncodeStatus {
    Ok = 0,
    OutOfMemory = 1,
    MaxDepthExceeded = 2,
    MissingRequired = 3,
}
// LINT.ThenChange()

// LINT.IfChange(decode_status)
#[repr(C)]
#[derive(PartialEq, Eq, Copy, Clone, Debug)]
pub enum DecodeStatus {
    Ok = 0,
    Malformed = 1,
    OutOfMemory = 2,
    BadUtf8 = 3,
    MaxDepthExceeded = 4,
    MissingRequired = 5,
    UnlinkedSubMessage = 6,
}
// LINT.ThenChange()

/// Contains the decode options that can be passed to `decode_with_options`.
pub mod decode_options {
    // LINT.IfChange(decode_option)
    pub const ALIAS_STRING: i32 = 1;
    pub const CHECK_REQUIRED: i32 = 2;
    pub const EXPERIMENTAL_ALLOW_UNLINKED: i32 = 4;
    pub const ALWAYS_VALIDATE_UTF8: i32 = 8;
    // LINT.ThenChange()
}

/// If Err, then EncodeStatus != Ok.
///
/// # Safety
/// - `msg` must be associated with `mini_table`.
pub unsafe fn encode(
    msg: RawMessage,
    mini_table: *const upb_MiniTable,
) -> Result<Vec<u8>, EncodeStatus> {
    let arena = Arena::new();
    let mut buf: *mut u8 = core::ptr::null_mut();
    let mut len = 0usize;

    // SAFETY:
    // - `mini_table` is the one associated with `msg`.
    // - `buf` and `buf_size` are legally writable.
    let status = unsafe { upb_Encode(msg, mini_table, 0, arena.raw(), &mut buf, &mut len) };

    if status == EncodeStatus::Ok {
        assert!(!buf.is_null()); // EncodeStatus Ok should never return NULL data, even for len=0.

        // SAFETY: upb guarantees that `buf` is valid to read for `len`.
        Ok(unsafe { &*core::ptr::slice_from_raw_parts(buf, len) }.to_vec())
    } else {
        Err(status)
    }
}

/// Decodes into the provided message (merge semantics). If Err, then
/// DecodeStatus != Ok.
///
/// Equivalent to `decode_with_options()` with the
/// `decode_options::CHECK_REQUIRED` option set.
///
/// # Safety
/// - `msg` must be mutable.
/// - `msg` must be associated with `mini_table`.
pub unsafe fn decode(
    buf: &[u8],
    msg: RawMessage,
    mini_table: *const upb_MiniTable,
    arena: &Arena,
) -> Result<(), DecodeStatus> {
    // SAFETY:
    // - `msg` is mutable and is associated with `mini_table`.
    // - `decode_options::CHECK_REQUIRED` is a valid decode option.
    unsafe { decode_with_options(buf, msg, mini_table, arena, decode_options::CHECK_REQUIRED) }
}

/// Decodes into the provided message (merge semantics). If Err, then
/// DecodeStatus != Ok.
///
/// # Safety
/// - `msg` must be mutable.
/// - `msg` must be associated with `mini_table`.
/// - `decode_options_bitmask` is a bitmask of constants from the `decode_options` module.
pub unsafe fn decode_with_options(
    buf: &[u8],
    msg: RawMessage,
    mini_table: *const upb_MiniTable,
    arena: &Arena,
    decode_options_bitmask: i32,
) -> Result<(), DecodeStatus> {
    let len = buf.len();
    let buf = buf.as_ptr();

    // SAFETY:
    // - `mini_table` is the one associated with `msg`
    // - `buf` is legally readable for at least `buf_size` bytes.
    // - `extreg` is null.
    // - `decode_options` is a valid DecodeOptions, so contains only allowed bits.
    let status = unsafe {
        upb_Decode(
            buf,
            len,
            msg,
            mini_table,
            core::ptr::null(),
            decode_options_bitmask,
            arena.raw(),
        )
    };
    match status {
        DecodeStatus::Ok => Ok(()),
        _ => Err(status),
    }
}

extern "C" {
    // SAFETY:
    // - `mini_table` is the one associated with `msg`
    // - `buf` and `buf_size` are legally writable.
    pub fn upb_Encode(
        msg: RawMessage,
        mini_table: *const upb_MiniTable,
        options: i32,
        arena: RawArena,
        buf: *mut *mut u8,
        buf_size: *mut usize,
    ) -> EncodeStatus;

    // SAFETY:
    // - `mini_table` is the one associated with `msg`
    // - `buf` is legally readable for at least `buf_size` bytes.
    // - `extreg` is either null or points at a valid upb_ExtensionRegistry.
    pub fn upb_Decode(
        buf: *const u8,
        buf_size: usize,
        msg: RawMessage,
        mini_table: *const upb_MiniTable,
        extreg: *const upb_ExtensionRegistry,
        options: i32,
        arena: RawArena,
    ) -> DecodeStatus;
}

#[cfg(test)]
mod tests {
    use super::*;
    use googletest::gtest;

    #[gtest]
    fn assert_wire_linked() {
        use crate::assert_linked;
        assert_linked!(upb_Encode);
        assert_linked!(upb_Decode);
    }
}
