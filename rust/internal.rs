// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
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
//     * Neither the name of Google LLC. nor the names of its
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

//! Kernel-agnostic logic for the Rust Protobuf runtime that should not be
//! exposed to through the `protobuf` path but must be public for use by
//! generated code.

use std::slice;

/// Used to protect internal-only items from being used accidentally.
pub struct Private;

/// Defines a set of opaque pointers and a unique non-accessible pointees.
///
/// This provides a type safety benefit over using `NonNull<u8>` everywhere.
/// The [Rustonomicon][nomicon] currently recommends a zero-sized struct,
/// though this should use [`extern type`] when that is stabilized.
///
/// Because this defines a new private module, it can only be called once per
/// module.
///
/// [nomicon]: https://doc.rust-lang.org/nomicon/ffi.html#representing-opaque-structs
/// [`extern type`]: https://github.com/rust-lang/rust/issues/43467
macro_rules! define_opaque_nonnulls {
    ($($(#[$meta:meta])* $vis:vis type $name:ident = NonNull<$raw_name:ident>;)*) => {
        mod _opaque_pointees {
            $(
                #[doc = concat!("Opaque pointee for [`", stringify!($name), "`][pointer]")]
                ///
                /// This type is not meant to be dereferenced in Rust code.
                /// It is only meant to provide type safety for raw pointers
                /// which are manipulated behind FFI.
                #[doc = concat!("[pointer]: super::", stringify!($name))]
                #[repr(C)]
                pub struct $raw_name {
                    _data: [u8; 0],
                    _marker: ::std::marker::PhantomData<(*mut u8, ::std::marker::PhantomPinned)>,
                }
            )*
        }
        $(
            $(#[$meta])*
            ///
            /// This is an opaque pointer used for FFI:
            /// do not dereference this type in Rust code.
            $vis type $name = ::std::ptr::NonNull<_opaque_pointees::$raw_name>;
        )*
    };
}

pub(crate) use define_opaque_nonnulls;

define_opaque_nonnulls!(
    /// A raw pointer to the underlying arena for this runtime.
    pub type RawArena = NonNull<RawArenaData>;

    /// A raw pointer to the underlying message for this runtime.
    pub type RawMessage = NonNull<RawMessageData>;
);

/// Represents an ABI-stable version of `NonNull<[u8]>`/`string_view` (a
/// borrowed slice of bytes) for FFI use only.
///
/// Has semantics similar to `std::string_view` in C++ and `&[u8]` in Rust,
/// but is not ABI-compatible with either.
///
/// If `len` is 0, then `ptr` can be null or dangling. C++ considers a dangling
/// 0-len `std::string_view` to be invalid, and Rust considers a `&[u8]` with a
/// null data pointer to be invalid.
#[repr(C)]
#[derive(Copy, Clone)]
pub struct PtrAndLen {
    /// Pointer to the first byte.
    /// Borrows the memory.
    pub ptr: *const u8,

    /// Length of the `[u8]` pointed to by `ptr`.
    pub len: usize,
}

impl PtrAndLen {
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
            unsafe { slice::from_raw_parts(self.ptr, self.len) }
        }
    }
}
