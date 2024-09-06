// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use super::message::RawMessage;
use super::mini_table::upb_MiniTable;

extern "C" {
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
        mt: *const upb_MiniTable,
        options: i32,
        buf: *mut u8,
        size: usize,
    ) -> usize;
}

#[allow(dead_code)]
#[repr(i32)]
pub enum DebugStringOptions {
    // When set, prints everything on a single line.
    SingleLine = 1,

    // When set, unknown fields are not printed.
    SkipUnknown = 2,

    // When set, maps are *not* sorted (this avoids allocating tmp mem).
    NoSortMaps = 4,
}
