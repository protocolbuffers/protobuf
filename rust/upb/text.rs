// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use super::sys::text::text::{text_encode_options, upb_DebugString};
use super::{AssociatedMiniTable, MessagePtr};

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

/// Encodes `snprintf`-style upb text output into an owned `String`.
///
/// It uses a stack-allocated fast path for outputs under 1 KiB, falling back to a heap buffer
/// when the output reaches or exceeds the stack buffer size.
pub fn encode_to_string_with(encode_fn: impl Fn(&mut [u8]) -> usize) -> String {
    let mut stack_buf = [0u8; 1024];
    let min_len = encode_fn(&mut stack_buf);
    // Strictly `<`: when `min_len == stack_buf.len()`, upb overwrites the last byte with NUL.
    if min_len < stack_buf.len() {
        return String::from_utf8_lossy(&stack_buf[..min_len]).into_owned();
    }

    assert!(min_len < isize::MAX as usize);
    // +1 for the trailing NULL
    let mut buf = vec![0u8; min_len + 1];
    let written_len = encode_fn(&mut buf);
    assert_eq!(min_len, written_len);
    // Drop the trailing NULL written by upb's encoder.
    buf.truncate(written_len);
    // TODO: Use `String::from_utf8_lossy_owned(buf)` once MSRV >= 1.99.
    String::from_utf8(buf)
        .unwrap_or_else(|err| String::from_utf8_lossy(err.as_bytes()).into_owned())
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
}

#[cfg(test)]
mod tests {
    use super::*;
    use core::cell::Cell;
    use googletest::gtest;

    /// Stub out real text encoders to test the `encode_to_string_with` function. They behave like
    /// `snprintf`: writes as much of `output` as fits in `buf`, always NUL-terminates within
    /// `buf`, and returns the length of the whole `output` no matter how large `buf` is.
    fn fake_encode(output: &[u8], buf: &mut [u8]) -> usize {
        let written = output.len().min(buf.len());
        buf[..written].copy_from_slice(&output[..written]);
        // Like upb, overwrite the last byte with the NUL if `output` leaves no room for it.
        if let Some(last) = buf.len().checked_sub(1) {
            buf[written.min(last)] = 0;
        }
        output.len()
    }

    /// Encodes `output` through `fake_encode`. Returns the resulting `String` and how many times
    /// the encoder ran; every run stands for a full encoding pass over a message.
    fn encode_with_fake(output: &[u8]) -> (String, usize) {
        let encoder_calls = Cell::new(0);
        let encoded = encode_to_string_with(|buf| {
            encoder_calls.set(encoder_calls.get() + 1);
            fake_encode(output, buf)
        });
        (encoded, encoder_calls.get())
    }

    #[gtest]
    fn test_output_that_fits_stack_buffer_is_encoded_once() {
        // 1023 bytes is the longest output that fits the 1024-byte stack buffer with its NUL.
        let output = "a".repeat(1023);

        let (encoded, encoder_calls) = encode_with_fake(output.as_bytes());

        assert_eq!(encoded, output);
        assert_eq!(encoder_calls, 1);
    }

    #[gtest]
    fn test_output_as_long_as_stack_buffer_is_encoded_twice() {
        // 1024 bytes fill the stack buffer, so the NUL overwrites the last of them.
        let output = "a".repeat(1024);

        let (encoded, encoder_calls) = encode_with_fake(output.as_bytes());

        assert_eq!(encoded, output);
        assert_eq!(encoder_calls, 2);
    }

    #[gtest]
    fn test_output_longer_than_stack_buffer_is_encoded_twice() {
        let output = "a".repeat(1025);

        let (encoded, encoder_calls) = encode_with_fake(output.as_bytes());

        assert_eq!(encoded, output);
        assert_eq!(encoder_calls, 2);
    }

    #[gtest]
    fn test_invalid_utf8_that_fits_stack_buffer_is_replaced() {
        let (encoded, _) = encode_with_fake(b"a\xFFb");

        assert_eq!(encoded, "a\u{FFFD}b");
    }

    #[gtest]
    fn test_invalid_utf8_longer_than_stack_buffer_is_replaced() {
        // 1200 bytes.
        let output = b"a\xFFb".repeat(400);

        let (encoded, _) = encode_with_fake(&output);

        assert_eq!(encoded, "a\u{FFFD}b".repeat(400));
    }

    #[gtest]
    #[should_panic]
    fn test_encoder_reporting_inconsistent_lengths_panics() {
        // Reports an output of 2000 bytes on the first call and of 1999 bytes on the second.
        let is_first_call = Cell::new(true);

        let _ =
            encode_to_string_with(|_buf| if is_first_call.replace(false) { 2000 } else { 1999 });
    }
}
