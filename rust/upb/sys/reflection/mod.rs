// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

pub mod def_pool;
pub mod message_def;

mod opaque_pointee;

use def_pool::RawDefPool;
use message_def::RawMessageDef;
use sys::message::message::RawMessage;

unsafe extern "C" {
    /// Encodes a message to text format.
    ///
    /// Returns the exact number of bytes required to store the resulting text (excluding
    /// the null terminator). If `size` is greater than 0, the text is written into `buf`
    /// up to `size - 1` bytes and null-terminated.
    ///
    /// # Safety
    /// - `msg` must point to a valid `upb_Message` whose shape perfectly aligns with the
    ///   supplied descriptor `m`.
    /// - `m` must be a valid, securely allocated `RawMessageDef`.
    /// - `ext_pool` may be `None`, but if `Some`, it must hold a valid `RawDefPool`.
    /// - `buf` must be legally writable for at least `size` bytes (or may be null if `size` is 0).
    pub fn upb_TextEncode(
        msg: RawMessage,
        m: RawMessageDef,
        ext_pool: Option<RawDefPool>,
        options: i32, // bitmask of `text_encode_options` values
        buf: *mut u8,
        size: usize,
    ) -> usize;
}

#[cfg(test)]
mod test_helpers;

#[cfg(test)]
mod tests {
    use super::*;
    use googletest::gtest;

    #[gtest]
    fn assert_text_encode_linked() {
        use super::test_helpers::assert_linked;
        assert_linked!(upb_TextEncode);
    }
}
