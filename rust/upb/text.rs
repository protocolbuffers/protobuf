// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use super::{upb_MiniTable, RawMessage};

extern "C" {
    /// Returns the minimum needed length (excluding NULL) that `buf` has to be
    /// to hold the `msg`s debug string.
    ///
    /// SAFETY:
    /// - `msg` is pointing at a valid upb_Message with associated minitable
    ///   `mt`
    /// - `buf` is legally writable for `size` bytes (`buf` may be nullptr if
    ///   `size` is 0)
    fn upb_DebugString(
        msg: RawMessage,
        mt: *const upb_MiniTable,
        options: i32,
        buf: *mut u8,
        size: usize,
    ) -> usize;
}

#[allow(dead_code)]
#[repr(i32)]
enum Options {
    Default = 0,

    // When set, prints everything on a single line.
    SingleLine = 1,

    // When set, unknown fields are not printed.
    SkipUnknown = 2,

    // When set, maps are *not* sorted (this avoids allocating tmp mem).
    NoSortMaps = 4,
}

/// Returns a string of field number to value entries of a message.
///
/// # Safety
/// - `mt` must correspond to the `msg`s minitable.
pub unsafe fn debug_string(msg: RawMessage, mt: *const upb_MiniTable) -> String {
    // Only find out the length first to then allocate a buffer of the minimum size
    // needed.
    // SAFETY:
    // - `msg` is a legally dereferencable upb_Message whose associated minitable is
    //   `mt`
    // - `buf` is nullptr and `buf_len` is 0
    let len =
        unsafe { upb_DebugString(msg, mt, Options::Default as i32, core::ptr::null_mut(), 0) };
    assert!(len < isize::MAX as usize);
    // +1 for the trailing NULL
    let mut buf = vec![0u8; len + 1];
    // SAFETY:
    // - `msg` is a legally dereferencable upb_Message whose associated minitable is
    //   `mt`
    // - `buf` is legally writable for 'buf_len' bytes
    let written_len =
        unsafe { upb_DebugString(msg, mt, Options::Default as i32, buf.as_mut_ptr(), buf.len()) };
    assert_eq!(len, written_len);
    String::from_utf8_lossy(buf.as_slice()).to_string()
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
