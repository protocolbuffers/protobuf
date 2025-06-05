// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use super::sys::wire::wire::{upb_Decode, upb_Encode, DecodeStatus, EncodeStatus};
use super::{Arena, AssociatedMiniTable, MessagePtr, MiniTable, RawMessage};

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
    mini_table: *const MiniTable,
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
pub unsafe fn decode<T: AssociatedMiniTable>(
    buf: &[u8],
    msg: MessagePtr<T>,
    arena: &Arena,
) -> Result<(), DecodeStatus> {
    // SAFETY:
    // - `msg` is mutable and is associated with `mini_table`.
    // - `decode_options::CHECK_REQUIRED` is a valid decode option.
    unsafe { decode_with_options(buf, msg, arena, decode_options::CHECK_REQUIRED) }
}

/// Decodes into the provided message (merge semantics). If Err, then
/// DecodeStatus != Ok.
///
/// # Safety
/// - `msg` must be mutable.
/// - `decode_options_bitmask` is a bitmask of constants from the `decode_options` module.
pub unsafe fn decode_with_options<T: AssociatedMiniTable>(
    buf: &[u8],
    msg: MessagePtr<T>,
    arena: &Arena,
    decode_options_bitmask: i32,
) -> Result<(), DecodeStatus> {
    let len = buf.len();
    let buf = buf.as_ptr();

    // SAFETY:
    // - `mini_table` is the one associated with `msg`
    // - `buf` is legally readable for at least `buf_size` bytes.
    // - `extreg` is null.
    // - `decode_options_bitmask` is a bitmask of constants from the `decode_options` module.
    let status = unsafe {
        upb_Decode(
            buf,
            len,
            msg.raw(),
            T::mini_table(),
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
