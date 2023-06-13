// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//! Kernel-agnostic logic for the Rust Protobuf Runtime.
//!
//! For kernel-specific logic this crate delegates to the respective __runtime
//! crate.

#[cfg(cpp_kernel)]
#[path = "cpp.rs"]
pub mod __runtime;
#[cfg(upb_kernel)]
#[path = "upb.rs"]
pub mod __runtime;

mod proxied;

pub use __runtime::SerializedData;

use std::fmt;
use std::ptr::NonNull;
use std::slice;

/// An error that happened during deserialization.
#[derive(Debug, Clone)]
pub struct ParseError;

impl fmt::Display for ParseError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "Couldn't deserialize given bytes into a proto")
    }
}

/// An ABI-stable, FFI-safe borrowed slice of bytes.
///
/// Has semantics equivalent to `&[u8]` in Rust and `std::string_view` in C++,
/// but is not ABI-compatible with them.
#[repr(C)]
#[derive(Copy, Clone)]
pub struct PtrAndLen {
    /// Borrows the memory.
    pub ptr: *const u8,
    pub len: usize,
}

impl PtrAndLen {
    pub unsafe fn as_ref<'a>(self) -> &'a [u8] {
        assert!(self.len == 0 || !self.ptr.is_null());
        if self.len == 0 {
            slice::from_raw_parts(NonNull::dangling().as_ptr(), 0)
        } else {
            slice::from_raw_parts(self.ptr, self.len)
        }
    }
}
