// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

/// ABI compatible struct with upb_StringView.
///
/// Note that this has semantics similar to `std::string_view` in C++ and
/// `&[u8]` in Rust, but is not ABI-compatible with either.
///
/// If `len` is 0, then `ptr` is allowed to be either null or dangling. C++
/// considers a dangling 0-len `std::string_view` to be invalid, and Rust
/// considers a `&[u8]` with a null data pointer to be invalid.
#[repr(C)]
#[derive(Copy, Clone)]
pub struct StringView {
    /// Pointer to the first byte.
    /// Borrows the memory.
    pub ptr: *const u8,

    /// Length of the `[u8]` pointed to by `ptr`.
    pub len: usize,
}

impl StringView {
    /// Unsafely dereference this slice.
    ///
    /// # Safety
    /// - `self.ptr` must be dereferencable and immutable for `self.len` bytes
    ///   for the lifetime `'a`. It can be null or dangling if `self.len == 0`.
    pub unsafe fn as_ref<'a>(self) -> &'a [u8] {
        if self.ptr.is_null() {
            assert_eq!(self.len, 0, "Non-empty slice with null data pointer");
            &[]
        } else {
            // SAFETY:
            // - `ptr` is non-null
            // - `ptr` is valid for `len` bytes as promised by the caller.
            unsafe { core::slice::from_raw_parts(self.ptr, self.len) }
        }
    }
}

impl From<&[u8]> for StringView {
    fn from(slice: &[u8]) -> Self {
        Self { ptr: slice.as_ptr(), len: slice.len() }
    }
}

impl<const N: usize> From<&[u8; N]> for StringView {
    fn from(slice: &[u8; N]) -> Self {
        Self { ptr: slice.as_ptr(), len: N }
    }
}
