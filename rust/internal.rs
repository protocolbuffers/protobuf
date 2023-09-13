// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

//! Kernel-agnostic logic for the Rust Protobuf runtime that should not be
//! exposed to through the `protobuf` path but must be public for use by
//! generated code.

pub use crate::vtable::{
    new_vtable_field_entry, BytesMutVTable, BytesOptionalMutVTable, PrimitiveVTable,
    RawVTableMutator,
};
use std::ptr::NonNull;
use std::slice;

/// Used to protect internal-only items from being used accidentally.
pub struct Private;

/// Defines a set of opaque, unique, non-accessible pointees.
///
/// The [Rustonomicon][nomicon] currently recommends a zero-sized struct,
/// though this should use [`extern type`] when that is stabilized.
/// [nomicon]: https://doc.rust-lang.org/nomicon/ffi.html#representing-opaque-structs
/// [`extern type`]: https://github.com/rust-lang/rust/issues/43467
mod _opaque_pointees {
    /// Opaque pointee for [`RawMessage`]
    ///
    /// This type is not meant to be dereferenced in Rust code.
    /// It is only meant to provide type safety for raw pointers
    /// which are manipulated behind FFI.
    ///
    /// [`RawMessage`]: super::RawMessage
    #[repr(C)]
    pub struct RawMessageData {
        _data: [u8; 0],
        _marker: std::marker::PhantomData<(*mut u8, ::std::marker::PhantomPinned)>,
    }

    /// Opaque pointee for [`RawArena`]
    ///
    /// This type is not meant to be dereferenced in Rust code.
    /// It is only meant to provide type safety for raw pointers
    /// which are manipulated behind FFI.
    ///
    /// [`RawArena`]: super::RawArena
    #[repr(C)]
    pub struct RawArenaData {
        _data: [u8; 0],
        _marker: std::marker::PhantomData<(*mut u8, ::std::marker::PhantomPinned)>,
    }
}

/// A raw pointer to the underlying message for this runtime.
pub type RawMessage = NonNull<_opaque_pointees::RawMessageData>;

/// A raw pointer to the underlying arena for this runtime.
pub type RawArena = NonNull<_opaque_pointees::RawArenaData>;

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

impl From<&[u8]> for PtrAndLen {
    fn from(slice: &[u8]) -> Self {
        Self { ptr: slice.as_ptr(), len: slice.len() }
    }
}
