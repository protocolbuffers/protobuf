// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use super::sys::text::text::upb_DebugString;
use super::{AssociatedMiniTable, MessagePtr};

/// Returns a string of field number to value entries of a message.
///
/// # Safety
/// - `msg` must be legally dereferenceable.
pub unsafe fn debug_string<T: AssociatedMiniTable>(msg: MessagePtr<T>) -> String {
    let mt = T::mini_table();
    let msg = msg.raw();

    // Only find out the length first to then allocate a buffer of the minimum size
    // needed.
    // SAFETY:
    // - `msg` is a legally dereferencable upb_Message whose associated minitable is
    //   `mt`
    // - `buf` is nullptr and `buf_len` is 0
    let len = unsafe { upb_DebugString(msg, mt, 0, core::ptr::null_mut(), 0) };
    assert!(len < isize::MAX as usize);
    // +1 for the trailing NULL
    let mut buf = vec![0u8; len + 1];
    // SAFETY:
    // - `msg` is a legally dereferencable upb_Message whose associated minitable is
    //   `mt`
    // - `buf` is legally writable for 'buf_len' bytes
    let written_len = unsafe { upb_DebugString(msg, mt, 0, buf.as_mut_ptr(), buf.len()) };
    assert_eq!(len, written_len);
    String::from_utf8_lossy(buf.as_slice()).to_string()
}
