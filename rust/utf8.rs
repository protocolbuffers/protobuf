// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

//! Lossy UTF-8 processing utilities.
#![deny(unsafe_op_in_unsafe_fn)]

// TODO: Replace this with the `std` versions once stable.
// This is adapted from https://github.com/rust-lang/rust/blob/e8ee0b7/library/core/src/str/lossy.rs
// The adaptations:
// - remove `#[unstable]` attributes.
// - replace `crate`/`super` paths with their `std` equivalents in code and
//   examples.
// - include `UTF8_CHAR_WIDTH`/`utf8_char_width` from `core::str::validations`.
// - use a custom `split_at_unchecked` instead of the nightly one

use std::fmt;
use std::fmt::Formatter;
use std::fmt::Write;
use std::iter::FusedIterator;
use std::str::from_utf8_unchecked;

/// An item returned by the [`Utf8Chunks`] iterator.
///
/// A `Utf8Chunk` stores a sequence of [`u8`] up to the first broken character
/// when decoding a UTF-8 string.
///
/// # Examples
///
/// ```
/// use utf8::Utf8Chunks;
///
/// // An invalid UTF-8 string
/// let bytes = b"foo\xF1\x80bar";
///
/// // Decode the first `Utf8Chunk`
/// let chunk = Utf8Chunks::new(bytes).next().unwrap();
///
/// // The first three characters are valid UTF-8
/// assert_eq!("foo", chunk.valid());
///
/// // The fourth character is broken
/// assert_eq!(b"\xF1\x80", chunk.invalid());
/// ```
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Utf8Chunk<'a> {
    valid: &'a str,
    invalid: &'a [u8],
}

impl<'a> Utf8Chunk<'a> {
    /// Returns the next validated UTF-8 substring.
    ///
    /// This substring can be empty at the start of the string or between
    /// broken UTF-8 characters.
    #[must_use]
    pub fn valid(&self) -> &'a str {
        self.valid
    }

    /// Returns the invalid sequence that caused a failure.
    ///
    /// The returned slice will have a maximum length of 3 and starts after the
    /// substring given by [`valid`]. Decoding will resume after this sequence.
    ///
    /// If empty, this is the last chunk in the string. If non-empty, an
    /// unexpected byte was encountered or the end of the input was reached
    /// unexpectedly.
    ///
    /// Lossy decoding would replace this sequence with [`U+FFFD REPLACEMENT
    /// CHARACTER`].
    ///
    /// [`valid`]: Self::valid
    /// [`U+FFFD REPLACEMENT CHARACTER`]: std::char::REPLACEMENT_CHARACTER
    #[must_use]
    pub fn invalid(&self) -> &'a [u8] {
        self.invalid
    }
}

#[must_use]
pub struct Debug<'a>(&'a [u8]);

impl fmt::Debug for Debug<'_> {
    fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
        f.write_char('"')?;

        for chunk in Utf8Chunks::new(self.0) {
            // Valid part.
            // Here we partially parse UTF-8 again which is suboptimal.
            {
                let valid = chunk.valid();
                let mut from = 0;
                for (i, c) in valid.char_indices() {
                    let esc = c.escape_debug();
                    // If char needs escaping, flush backlog so far and write, else skip
                    if esc.len() != 1 {
                        f.write_str(&valid[from..i])?;
                        for c in esc {
                            f.write_char(c)?;
                        }
                        from = i + c.len_utf8();
                    }
                }
                f.write_str(&valid[from..])?;
            }

            // Broken parts of string as hex escape.
            for &b in chunk.invalid() {
                write!(f, "\\x{:02X}", b)?;
            }
        }

        f.write_char('"')
    }
}

/// An iterator used to decode a slice of mostly UTF-8 bytes to string slices
/// ([`&str`]) and byte slices ([`&[u8]`][byteslice]).
///
/// If you want a simple conversion from UTF-8 byte slices to string slices,
/// [`from_utf8`] is easier to use.
///
/// [byteslice]: slice
/// [`from_utf8`]: std::str::from_utf8
///
/// # Examples
///
/// This can be used to create functionality similar to
/// [`String::from_utf8_lossy`] without allocating heap memory:
///
/// ```
/// use utf8::Utf8Chunks;
///
/// fn from_utf8_lossy<F>(input: &[u8], mut push: F) where F: FnMut(&str) {
///     for chunk in Utf8Chunks::new(input) {
///         push(chunk.valid());
///
///         if !chunk.invalid().is_empty() {
///             push("\u{FFFD}");
///         }
///     }
/// }
/// ```
#[must_use = "iterators are lazy and do nothing unless consumed"]
#[derive(Clone)]
pub struct Utf8Chunks<'a> {
    source: &'a [u8],
}

impl<'a> Utf8Chunks<'a> {
    /// Creates a new iterator to decode the bytes.
    pub fn new(bytes: &'a [u8]) -> Self {
        Self { source: bytes }
    }

    #[doc(hidden)]
    pub fn debug(&self) -> Debug<'_> {
        Debug(self.source)
    }
}

impl<'a> Iterator for Utf8Chunks<'a> {
    type Item = Utf8Chunk<'a>;

    fn next(&mut self) -> Option<Utf8Chunk<'a>> {
        if self.source.is_empty() {
            return None;
        }

        const TAG_CONT_U8: u8 = 128;
        fn safe_get(xs: &[u8], i: usize) -> u8 {
            *xs.get(i).unwrap_or(&0)
        }

        let mut i = 0;
        let mut valid_up_to = 0;
        while i < self.source.len() {
            // SAFETY: `i < self.source.len()` per previous line.
            // For some reason the following are both significantly slower:
            // while let Some(&byte) = self.source.get(i) {
            // while let Some(byte) = self.source.get(i).copied() {
            let byte = unsafe { *self.source.get_unchecked(i) };
            i += 1;

            if byte < 128 {
                // This could be a `1 => ...` case in the match below, but for
                // the common case of all-ASCII inputs, we bypass loading the
                // sizeable UTF8_CHAR_WIDTH table into cache.
            } else {
                let w = utf8_char_width(byte);

                match w {
                    2 => {
                        if safe_get(self.source, i) & 192 != TAG_CONT_U8 {
                            break;
                        }
                        i += 1;
                    }
                    3 => {
                        match (byte, safe_get(self.source, i)) {
                            (0xE0, 0xA0..=0xBF) => (),
                            (0xE1..=0xEC, 0x80..=0xBF) => (),
                            (0xED, 0x80..=0x9F) => (),
                            (0xEE..=0xEF, 0x80..=0xBF) => (),
                            _ => break,
                        }
                        i += 1;
                        if safe_get(self.source, i) & 192 != TAG_CONT_U8 {
                            break;
                        }
                        i += 1;
                    }
                    4 => {
                        match (byte, safe_get(self.source, i)) {
                            (0xF0, 0x90..=0xBF) => (),
                            (0xF1..=0xF3, 0x80..=0xBF) => (),
                            (0xF4, 0x80..=0x8F) => (),
                            _ => break,
                        }
                        i += 1;
                        if safe_get(self.source, i) & 192 != TAG_CONT_U8 {
                            break;
                        }
                        i += 1;
                        if safe_get(self.source, i) & 192 != TAG_CONT_U8 {
                            break;
                        }
                        i += 1;
                    }
                    _ => break,
                }
            }

            valid_up_to = i;
        }

        /// # Safety
        /// `index` must be in-bounds for `x`
        unsafe fn split_at_unchecked(x: &[u8], index: usize) -> (&[u8], &[u8]) {
            // SAFTEY: in-bounds as promised by the caller
            unsafe { (x.get_unchecked(..index), x.get_unchecked(index..)) }
        }

        // SAFETY: `i <= self.source.len()` because it is only ever incremented
        // via `i += 1` and in between every single one of those increments, `i`
        // is compared against `self.source.len()`. That happens either
        // literally by `i < self.source.len()` in the while-loop's condition,
        // or indirectly by `safe_get(self.source, i) & 192 != TAG_CONT_U8`. The
        // loop is terminated as soon as the latest `i += 1` has made `i` no
        // longer less than `self.source.len()`, which means it'll be at most
        // equal to `self.source.len()`.
        let (inspected, remaining) = unsafe { split_at_unchecked(self.source, i) };
        self.source = remaining;

        // SAFETY: `valid_up_to <= i` because it is only ever assigned via
        // `valid_up_to = i` and `i` only increases.
        let (valid, invalid) = unsafe { split_at_unchecked(inspected, valid_up_to) };

        Some(Utf8Chunk {
            // SAFETY: All bytes up to `valid_up_to` are valid UTF-8.
            valid: unsafe { from_utf8_unchecked(valid) },
            invalid,
        })
    }
}

impl FusedIterator for Utf8Chunks<'_> {}

impl fmt::Debug for Utf8Chunks<'_> {
    fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
        f.debug_struct("Utf8Chunks").field("source", &self.debug()).finish()
    }
}

// https://tools.ietf.org/html/rfc3629
const UTF8_CHAR_WIDTH: &[u8; 256] = &[
    // 1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 1
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 2
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 3
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 4
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 5
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 6
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 7
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 8
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 9
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // A
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // B
    0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // C
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // D
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, // E
    4, 4, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // F
];

/// Given a first byte, determines how many bytes are in this UTF-8 character.
#[must_use]
#[inline]
const fn utf8_char_width(b: u8) -> usize {
    UTF8_CHAR_WIDTH[b as usize] as usize
}
