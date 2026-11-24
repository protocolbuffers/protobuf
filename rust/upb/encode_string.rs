// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use core::fmt;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct EncodeError;

impl fmt::Display for EncodeError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str("upb encoder reported an error")
    }
}

impl std::error::Error for EncodeError {}

/// Encodes `snprintf`-style upb output into an owned `String`, or returns `Err(EncodeError)` if
/// `encode_fn` returns `usize::MAX`.
///
/// It uses a stack-allocated fast path for outputs under 1 KiB, falling back to a heap buffer
/// when the output reaches or exceeds the stack buffer size.
pub fn encode_to_string_with(
    mut encode_fn: impl FnMut(&mut [u8]) -> usize,
) -> Result<String, EncodeError> {
    let mut stack_buf = [0u8; 1024];
    let min_len = encode_fn(&mut stack_buf);
    if min_len == usize::MAX {
        return Err(EncodeError);
    }

    // Fast path: the output fits in the stack buffer.
    if min_len < stack_buf.len() {
        return Ok(String::from_utf8_lossy(&stack_buf[..min_len]).into_owned());
    }

    assert!(min_len < isize::MAX as usize);
    // +1 for the trailing NULL
    let mut buf = vec![0u8; min_len + 1];
    let written_len = encode_fn(&mut buf);
    if written_len == usize::MAX {
        return Err(EncodeError);
    }
    assert_eq!(min_len, written_len);
    // Drop the trailing NULL written by upb's encoder.
    buf.truncate(written_len);
    // TODO: Use `String::from_utf8_lossy_owned(buf)` once MSRV >= 1.99.
    Ok(String::from_utf8(buf)
        .unwrap_or_else(|err| String::from_utf8_lossy(err.as_bytes()).into_owned()))
}

#[cfg(test)]
mod tests {
    use super::*;
    use googletest::prelude::*;

    /// Stub out real encoders to test the `encode_to_string_with` function. They behave like
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
        let mut encoder_calls = 0;
        let encoded = encode_to_string_with(|buf| {
            encoder_calls += 1;
            fake_encode(output, buf)
        })
        .expect("fake_encode never fails");
        (encoded, encoder_calls)
    }

    #[gtest]
    fn test_output_that_fits_stack_buffer_is_encoded_once() {
        // 1023 bytes is the longest output that fits the 1024-byte stack buffer with its NUL.
        let output = "a".repeat(1023);

        let (encoded, encoder_calls) = encode_with_fake(output.as_bytes());

        expect_eq!(encoded, output);
        expect_eq!(encoder_calls, 1);
    }

    #[gtest]
    fn test_output_as_long_as_stack_buffer_is_encoded_twice() {
        // 1024 bytes fill the stack buffer, so the NUL overwrites the last of them.
        let output = "a".repeat(1024);

        let (encoded, encoder_calls) = encode_with_fake(output.as_bytes());

        expect_eq!(encoded, output);
        expect_eq!(encoder_calls, 2);
    }

    #[gtest]
    fn test_output_longer_than_stack_buffer_is_encoded_twice() {
        let output = "a".repeat(1025);

        let (encoded, encoder_calls) = encode_with_fake(output.as_bytes());

        expect_eq!(encoded, output);
        expect_eq!(encoder_calls, 2);
    }

    #[gtest]
    fn test_invalid_utf8_that_fits_stack_buffer_is_replaced() {
        let (encoded, _) = encode_with_fake(b"a\xFFb");

        expect_eq!(encoded, "a\u{FFFD}b");
    }

    #[gtest]
    fn test_invalid_utf8_longer_than_stack_buffer_is_replaced() {
        // 1200 bytes.
        let output = b"a\xFFb".repeat(400);

        let (encoded, _) = encode_with_fake(&output);

        expect_eq!(encoded, "a\u{FFFD}b".repeat(400));
    }

    #[gtest]
    #[should_panic]
    fn test_encoder_reporting_inconsistent_lengths_panics() {
        // Reports an output of 2000 bytes on the first call and of 1999 bytes on the second.
        let mut is_first_call = true;

        let _ = encode_to_string_with(|_buf| {
            if is_first_call {
                is_first_call = false;
                2000
            } else {
                1999
            }
        });
    }

    #[gtest]
    fn test_encoder_returning_usize_max_returns_err() {
        let mut encoder_calls = 0;
        let result = encode_to_string_with(|_buf| {
            encoder_calls += 1;
            usize::MAX
        });

        expect_eq!(result, Err(EncodeError));
        expect_eq!(encoder_calls, 1);
    }

    #[gtest]
    fn test_encoder_returning_usize_max_on_second_call_returns_err() {
        // Reports an output of 2000 bytes on the first call and failure on the second.
        let mut is_first_call = true;

        let result = encode_to_string_with(|_buf| {
            if is_first_call {
                is_first_call = false;
                2000
            } else {
                usize::MAX
            }
        });

        expect_eq!(result, Err(EncodeError));
    }
}
