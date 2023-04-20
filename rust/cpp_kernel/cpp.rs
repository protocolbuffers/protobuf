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

use std::alloc;
use std::alloc::Layout;
use std::boxed::Box;
use std::cell::UnsafeCell;
use std::fmt;
use std::marker::PhantomData;
use std::mem::MaybeUninit;
use std::ops::Deref;
use std::ptr::NonNull;
use std::slice;

/// A wrapper over a `proto2::Arena`.
///
/// This is not a safe wrapper per se, because the allocation functions still
/// have sharp edges (see their safety docs for more info).
///
/// This is an owning type and will automatically free the arena when
/// dropped.
///
/// Note that this type is neither `Sync` nor `Send`.
///
/// TODO(b/272728844): Replace this placeholder code with a real implementation.
pub struct Arena {
    ptr: NonNull<u8>,
    _not_sync: PhantomData<UnsafeCell<()>>,
}

impl Arena {
    /// Allocates a fresh arena.
    #[inline]
    pub fn new() -> Self {
        Self { ptr: NonNull::dangling(), _not_sync: PhantomData }
    }

    /// Returns the raw, C++-managed pointer to the arena.
    #[inline]
    pub fn raw(&self) -> ! {
        unimplemented!()
    }

    /// Allocates some memory on the arena.
    ///
    /// # Safety
    ///
    /// `layout`'s alignment must be less than `UPB_MALLOC_ALIGN`.
    #[inline]
    pub unsafe fn alloc(&self, layout: Layout) -> &mut [MaybeUninit<u8>] {
        unimplemented!()
    }

    /// Resizes some memory on the arena.
    ///
    /// # Safety
    ///
    /// After calling this function, `ptr` is essentially zapped. `old` must
    /// be the layout `ptr` was allocated with via [`Arena::alloc()`]. `new`'s
    /// alignment must be less than `UPB_MALLOC_ALIGN`.
    #[inline]
    pub unsafe fn resize(&self, ptr: *mut u8, old: Layout, new: Layout) -> &[MaybeUninit<u8>] {
        unimplemented!()
    }
}

impl Drop for Arena {
    #[inline]
    fn drop(&mut self) {
        // unimplemented
    }
}

/// Represents serialized Protobuf wire format data. It's typically produced by
/// `<Message>.serialize()`.
///
/// This struct is ABI compatible with the equivalent struct on the C++ side. It
/// owns (and drops) its data.
// copybara:strip_begin
// LINT.IfChange
// copybara:strip_end
#[repr(C)]
pub struct SerializedData {
    /// Owns the memory.
    data: NonNull<u8>,
    len: usize,
}
// copybara:strip_begin
// LINT.ThenChange(//depot/google3/third_party/protobuf/rust/cpp_kernel/cpp_api.
// h) copybara:strip_end

impl SerializedData {
    pub unsafe fn from_raw_parts(data: NonNull<u8>, len: usize) -> Self {
        Self { data, len }
    }
}

impl Deref for SerializedData {
    type Target = [u8];
    fn deref(&self) -> &Self::Target {
        unsafe { slice::from_raw_parts(self.data.as_ptr(), self.len) }
    }
}

impl Drop for SerializedData {
    fn drop(&mut self) {
        unsafe {
            alloc::dealloc(self.data.as_ptr(), Layout::array::<u8>(self.len).unwrap());
        };
    }
}

impl fmt::Debug for SerializedData {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        fmt::Debug::fmt(self.deref(), f)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    // We need to allocate the byte array so SerializedData can own it and
    // deallocate it in its drop. This function makes it easier to do so for our
    // tests.
    fn allocate_byte_array(content: &'static [u8]) -> (*mut u8, usize) {
        let content: &mut [u8] = Box::leak(content.into());
        (content.as_mut_ptr(), content.len())
    }

    #[test]
    fn test_serialized_data_roundtrip() {
        let (ptr, len) = allocate_byte_array(b"Hello world");
        let serialized_data = SerializedData { data: NonNull::new(ptr).unwrap(), len: len };
        assert_eq!(&*serialized_data, b"Hello world");
    }
}
