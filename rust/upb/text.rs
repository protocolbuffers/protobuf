// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use super::sys::text::text::{text_encode_options, upb_DebugString};
use super::{encode_to_string_with, AssociatedMiniTable, MessagePtr};

/// Options for upb's text encoders.
/// This is only used for `text_encode` for now, but it might be used for `debug_string` in the
/// future. That's why it must be in the non-reflection crate.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct TextEncodeOptions {
    /// Print everything on a single line.
    pub single_line: bool,
    /// Do not print unknown fields.
    pub skip_unknown: bool,
}

impl TextEncodeOptions {
    /// The bitmask that upb's text encoders expect.
    pub fn to_bits(self) -> i32 {
        let mut bits = 0;
        if self.single_line {
            bits |= text_encode_options::SINGLE_LINE;
        }
        if self.skip_unknown {
            bits |= text_encode_options::SKIP_UNKNOWN;
        }
        bits
    }
}

/// Returns a string of field number to value entries of a message.
///
/// # Safety
/// - `msg` must be legally dereferenceable.
pub unsafe fn debug_string<T: AssociatedMiniTable>(msg: MessagePtr<T>) -> String {
    let mt = T::mini_table();
    let msg = msg.raw();

    encode_to_string_with(|buf| {
        // SAFETY:
        // - `msg` is a legally dereferenceable `upb_Message` whose associated minitable is `mt`.
        // - `buf` is a mutable slice, so `buf.as_mut_ptr()` is writable for `buf.len()` bytes.
        unsafe { upb_DebugString(msg, mt, 0, buf.as_mut_ptr(), buf.len()) }
    })
    .expect("upb_DebugString should never fail")
}
