// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

mod sys {
    pub use super::super::super::*;
}

use sys::message::message::RawMessage;
use sys::mini_table::mini_table::RawMiniTable;

unsafe extern "C" {
    /// Returns the minimum needed length (excluding NULL) that `buf` has to be
    /// to hold the `msg`s debug string.
    ///
    /// SAFETY:
    /// - `msg` is pointing at a valid upb_Message with associated minitable
    ///   `mt`
    /// - `buf` is legally writable for `size` bytes (`buf` may be nullptr if
    ///   `size` is 0)
    pub fn upb_DebugString(
        msg: RawMessage,
        mt: RawMiniTable,
        options: i32, // bitmask of `text_encode_options` values
        buf: *mut u8,
        size: usize,
    ) -> usize;
}

/// Encoding options.
#[allow(dead_code)]
pub mod text_encode_options {
    /// When set, prints everything on a single line.
    pub const SINGLE_LINE: i32 = 1;

    /// When set, unknown fields are not printed.
    pub const SKIP_UNKNOWN: i32 = 2;

    /// When set, maps are *not* sorted (this avoids allocating tmp mem).
    pub const NO_SORT_MAPS: i32 = 4;
}

#[cfg(test)]
mod tests {
    use super::*;
    use googletest::gtest;

    #[gtest]
    fn assert_text_linked() {
        use crate::assert_linked;
        assert_linked!(upb_DebugString);
    }
}
