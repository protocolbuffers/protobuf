// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use super::sys::{
    upb_Decode, upb_Encode, upb_MiniTable, DecodeOption, DecodeStatus, EncodeStatus, RawMessage,
};
use super::Arena;

/// If Err, then EncodeStatus != Ok.
///
/// # Safety
/// - `msg` must be associated with `mini_table`.
pub unsafe fn encode(
    msg: RawMessage,
    mini_table: *const upb_MiniTable,
) -> Result<Vec<u8>, EncodeStatus> {
    let arena = Arena::new();
    let mut buf: *mut u8 = std::ptr::null_mut();
    let mut len = 0usize;

    // SAFETY:
    // - `mini_table` is the one associated with `msg`.
    // - `buf` and `buf_size` are legally writable.
    let status = unsafe { upb_Encode(msg, mini_table, 0, arena.raw(), &mut buf, &mut len) };

    if status == EncodeStatus::Ok {
        assert!(!buf.is_null()); // EncodeStatus Ok should never return NULL data, even for len=0.
        // SAFETY: upb guarantees that `buf` is valid to read for `len`.
        Ok(unsafe { &*std::ptr::slice_from_raw_parts(buf, len) }.to_vec())
    } else {
        Err(status)
    }
}

/// Decodes into the provided message (merge semantics). If Err, then
/// DecodeStatus != Ok.
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
    let len = buf.len();
    let buf = buf.as_ptr();
    let options = DecodeOption::CheckRequired as i32;

    // SAFETY:
    // - `mini_table` is the one associated with `msg`
    // - `buf` is legally readable for at least `buf_size` bytes.
    // - `extreg` is null.
    let status =
        unsafe { upb_Decode(buf, len, msg, mini_table, std::ptr::null(), options, arena.raw()) };
    match status {
        DecodeStatus::Ok => Ok(()),
        _ => Err(status),
    }
}
