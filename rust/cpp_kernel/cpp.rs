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

// Rust Protobuf runtime using the C++ kernel.

use std::alloc::{dealloc, Layout};
use std::boxed::Box;
use std::ops::Deref;
use std::ptr::NonNull;
use std::slice;

/// TODO(b/272728844): Replace this placeholder code with a real implementation.
#[repr(C)]
pub struct Arena {
    _data: [u8; 0],
}

impl Arena {
    pub unsafe fn new() -> *mut Self {
        let arena = Box::new(Arena { _data: [] });
        Box::leak(arena) as *mut _
    }

    pub unsafe fn free(arena: *mut Self) {
        let arena = Box::from_raw(arena);
        std::mem::drop(arena);
    }
}

/// Represents a sequence of raw bytes.
///
/// Example use-cases:
/// * Return value of `<Message>.SerializeToString()`.
/// * Parameter of setters for `bytes` fields.
///
/// This struct is ABI compatible with the equivalent struct on the C++ side. It
/// owns (and drops) its data.
// copybara:strip_begin
// LINT.IfChange
// copybara:strip_end
#[repr(C)]
#[derive(Debug)]
pub struct Bytes {
    /// Owns the memory.
    data: NonNull<u8>,
    len: usize,
}
// copybara:strip_begin
// LINT.ThenChange(//depot/google3/third_party/protobuf/rust/cpp_kernel/cpp_api.
// h) copybara:strip_end

impl Deref for Bytes {
    type Target = [u8];
    fn deref(&self) -> &Self::Target {
        unsafe { slice::from_raw_parts(self.data.as_ptr() as *const _, self.len) }
    }
}

impl Drop for Bytes {
    fn drop(&mut self) {
        unsafe {
            dealloc(self.data.as_ptr(), Layout::array::<u8>(self.len).unwrap());
        };
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::alloc::alloc;
    use std::ptr::copy;

    // We need to allocate the byte array so `Bytes` can own it and
    // deallocate it in its drop. This function makes it easier to do so for our
    // tests.
    fn allocate_byte_array(content: &'static [u8]) -> (*mut u8, usize) {
        let len = content.len();
        let layout = Layout::array::<u8>(len).unwrap();

        let ptr = unsafe {
            let ptr = alloc(layout);
            copy(content.as_ptr(), ptr, len);
            ptr
        };
        (ptr, len)
    }

    #[test]
    fn test_bytes_roundtrip() {
        let (ptr, len) = allocate_byte_array(b"Hello world");
        let bytes = Bytes { data: NonNull::new(ptr).unwrap(), len: len };
        assert_eq!(&*bytes, b"Hello world");
    }
}
