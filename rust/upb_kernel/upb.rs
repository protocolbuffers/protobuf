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

// Rust Protobuf runtime using the UPB kernel.

/// Represents UPB's upb_Arena.
use std::ops::Deref;
use std::ptr::NonNull;
use std::slice;

#[repr(C)]
pub struct Arena {
    _data: [u8; 0],
    _marker: core::marker::PhantomData<(*mut u8, core::marker::PhantomPinned)>,
}

impl Arena {
    pub unsafe fn new() -> *mut Self {
        upb_Arena_New()
    }

    pub unsafe fn free(arena: *mut Self) {
        upb_Arena_Free(arena)
    }
}

extern "C" {
    pub fn upb_Arena_New() -> *mut Arena;
    pub fn upb_Arena_Free(arena: *mut Arena);
}

/// Represents a sequence of raw bytes.
///
/// Example use-cases:
/// * Return value of `<Message>.SerializeToString()`.
/// * Parameter of setters for `bytes` fields.
#[derive(Debug)]
pub struct Bytes {
    data: NonNull<u8>,
    len: usize,
    arena: *mut Arena,
}

impl Bytes {
    pub unsafe fn from_raw_parts(arena: *mut Arena, data: NonNull<u8>, len: usize) -> Self {
        Bytes { arena, data, len }
    }
}

impl Deref for Bytes {
    type Target = [u8];
    fn deref(&self) -> &Self::Target {
        unsafe { slice::from_raw_parts(self.data.as_ptr() as *const _, self.len) }
    }
}

impl Drop for Bytes {
    fn drop(&mut self) {
        unsafe { Arena::free(self.arena) };
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_arena_new_and_free() {
        let arena = unsafe { Arena::new() };
        unsafe { Arena::free(arena) };
    }

    #[test]
    fn test_bytes_roundtrip() {
        let arena = unsafe { Arena::new() };
        let original_data = b"Hello world";
        let len = original_data.len();

        let bytes = unsafe {
            Bytes::from_raw_parts(
                arena,
                NonNull::new(original_data as *const _ as *mut _).unwrap(),
                len,
            )
        };
        assert_eq!(&*bytes, b"Hello world");
    }
}
