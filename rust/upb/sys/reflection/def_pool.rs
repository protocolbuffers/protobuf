// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use super::message_def::RawMessageDef;
use super::opaque_pointee::opaque_pointee;
use core::ffi::{c_char, c_int};
use core::ptr::NonNull;
use sys::base::string_view::StringView;
use sys::mem::arena::RawArena;
use sys::mini_table::mini_table::{RawMiniTable, RawMiniTableEnum, RawMiniTableExtension};

opaque_pointee!(upb_DefPool);
pub type RawDefPool = NonNull<upb_DefPool>;

opaque_pointee!(upb_DefPool_Init);
pub type RawDefPoolInit = NonNull<upb_DefPool_Init>;

opaque_pointee!(upb_MiniTableFile);
pub type RawMiniTableFile = NonNull<upb_MiniTableFile>;

unsafe extern "C" {
    /// Creates a new upb_DefPool.
    ///
    /// The caller takes ownership of the returned pointer and must call `upb_DefPool_Free`
    /// on it to avoid leaking memory.
    pub fn upb_DefPool_New() -> Option<RawDefPool>;

    /// Frees a upb_DefPool and all message definitions allocated within it.
    ///
    /// # Safety
    /// - `s` must be a valid pointer to a `upb_DefPool` created by `upb_DefPool_New` that has not been freed yet.
    pub fn upb_DefPool_Free(s: RawDefPool);

    /// Assembles a `upb_MiniTableFile` on `arena` from the MiniTables a generated file owns.
    ///
    /// # Safety
    /// - `arena` must be a valid, live arena.
    /// - Each array must be readable for its stated count, and a count of zero permits a
    ///   null pointer.
    pub fn upb_MiniTableFile_New(
        arena: RawArena,
        msgs: *const RawMiniTable,
        msg_count: c_int,
        enums: *const RawMiniTableEnum,
        enum_count: c_int,
        exts: *const RawMiniTableExtension,
        ext_count: c_int,
    ) -> Option<RawMiniTableFile>;

    /// Assembles a `upb_DefPool_Init` on `arena`.
    ///
    /// # Safety
    /// - `arena` must be a valid, live arena.
    /// - `filename` must point to a null-terminated string that matches the name inside
    ///   `descriptor`; it is the key the pool dedupes on.
    /// - `descriptor` must be a readable serialized `FileDescriptorProto`.
    /// - `deps` must be null, or point to a null-terminated array of inits for the files
    ///   `descriptor` imports.
    /// - `layout` must be null, in which case upb builds its own MiniTables, or describe
    ///   `descriptor`.
    pub fn upb_DefPool_Init_New(
        arena: RawArena,
        filename: *const c_char,
        descriptor: StringView,
        deps: *mut Option<RawDefPoolInit>,
        layout: *const upb_MiniTableFile,
    ) -> Option<RawDefPoolInit>;

    /// Loads a generated descriptor, and everything it imports, into the pool. It recurses into the
    /// init's dependencies first, so a caller only has to name the root file.
    ///
    /// # Safety
    /// - `s` must be a valid pointer to a `upb_DefPool` that has not been freed yet.
    /// - `init`, and every init reachable from it, must be valid and outlive this call.
    /// - Any layout the init carries must match its descriptor in correct counts. The
    ///   descriptors must be in the correct order.
    #[link_name = "_upb_DefPool_LoadDefInit"]
    pub fn upb_DefPool_LoadDefInit(s: RawDefPool, init: *const upb_DefPool_Init) -> bool;

    /// Looks up a message by fully-qualified name.
    ///
    /// # Safety
    /// - `s` must be a valid pointer to a `upb_DefPool` that has not been freed yet.
    /// - `sym` must be legally readable for at least `len` bytes.
    pub fn upb_DefPool_FindMessageByNameWithSize(
        s: RawDefPool,
        sym: *const c_char,
        len: usize,
    ) -> Option<RawMessageDef>;
}

#[cfg(test)]
mod tests {
    use googletest::gtest;

    #[gtest]
    fn assert_def_pool_linked() {
        use super::super::test_helpers::assert_linked;
        assert_linked!(super::upb_DefPool_New);
        assert_linked!(super::upb_DefPool_Free);
        assert_linked!(super::upb_MiniTableFile_New);
        assert_linked!(super::upb_DefPool_Init_New);
        assert_linked!(super::upb_DefPool_LoadDefInit);
        assert_linked!(super::upb_DefPool_FindMessageByNameWithSize);
    }
}
