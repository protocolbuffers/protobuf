// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

mod sys {
    pub use super::super::super::*;
}

use core::ptr::NonNull;
use sys::mem::arena::RawArena;
use sys::mini_table::mini_table::RawMiniTableExtension;
use sys::opaque_pointee::opaque_pointee;

opaque_pointee!(upb_ExtensionRegistry);

#[allow(unused)] // Not used yet but intended in the future.
pub type RawExtensionRegistry = NonNull<upb_ExtensionRegistry>;

// LINT.IfChange
#[repr(C)]
#[derive(PartialEq, Eq, Copy, Clone, Debug)]
#[allow(unused)] // C struct values used in FFI.
pub enum ExtensionRegistryStatus {
    Ok = 0,
    DuplicateEntry = 1,
    OutOfMemory = 2,
    InvalidExtension = 3,
}
// LINT.ThenChange(//depot/google3/third_party/upb/upb/mini_table/extension_registry.h)

unsafe extern "C" {
    pub fn upb_ExtensionRegistry_New(arena: RawArena) -> RawExtensionRegistry;
    pub fn upb_ExtensionRegistry_Add(
        r: RawExtensionRegistry,
        e: RawMiniTableExtension,
    ) -> ExtensionRegistryStatus;
}
