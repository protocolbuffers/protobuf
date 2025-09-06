// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use super::arena::Arena;
use super::sys::mini_table::mini_table::{
    upb_MiniTable, upb_MiniTableEnum, upb_MiniTableEnum_Build, upb_MiniTable_Build,
    upb_MiniTable_Link,
};
use std::mem::ManuallyDrop;
use std::sync::OnceLock;

/// A trait for types which have an associated MiniTable (e.g. generated
/// messages, and their mut and view proxy types).
///
/// upb_Message in C is effectively a DST type, where instances are created with
/// a MiniTable (and have a size dependent on the given MiniTable). Many upb
/// operations on the upb_Message* will require the same upb_MiniTable* to be
/// passed in as a parameter, which is referred to as 'the associated MiniTable
/// for the upb_Message instance' in safety comments.
///
/// This trait is a way to associate a MiniTable with Rust types
/// which hold upb_Message* to simplify ensuring the upb C invariants
/// are maintained.
///
/// SAFETY:
/// - The MiniTable pointer must be from Protobuf code generation and follow the
///   corresponding invariants associated with upb's C API (the pointer should
///   always have the same non-null value, the underlying pointee should never
///   be modified and should have 'static lifetime).
pub unsafe trait AssociatedMiniTable {
    fn mini_table() -> *const upb_MiniTable;
}

/// A trait for closed enums that have an associated MiniTable.
pub unsafe trait AssociatedMiniTableEnum {
    fn mini_table() -> MiniTableEnumPtr;
}

pub trait MiniDescriptorEnum {
    fn mini_descriptor() -> &'static str;
}

#[derive(Clone, Copy)]
pub struct MiniTablePtr(pub *mut upb_MiniTable);
unsafe impl Send for MiniTablePtr {}
unsafe impl Sync for MiniTablePtr {}

#[derive(Clone, Copy)]
#[repr(transparent)]
pub struct MiniTableEnumPtr(pub *const upb_MiniTableEnum);
unsafe impl Send for MiniTableEnumPtr {}
unsafe impl Sync for MiniTableEnumPtr {}

thread_local! {
    // We need to avoid dropping this Arena, because we use it to build mini tables that
    // effectively have 'static lifetimes.
    pub static THREAD_LOCAL_ARENA: ManuallyDrop<Arena> = ManuallyDrop::new(Arena::new());
}

pub fn build_mini_table(mini_descriptor: &'static str) -> MiniTablePtr {
    unsafe {
        MiniTablePtr(upb_MiniTable_Build(
            mini_descriptor.as_ptr(),
            mini_descriptor.len(),
            THREAD_LOCAL_ARENA.with(|a| a.raw()),
            std::ptr::null_mut(),
        ))
    }
}

pub fn link_mini_table(
    mini_table: *mut upb_MiniTable,
    submessages: &[*const upb_MiniTable],
    subenums: &[MiniTableEnumPtr],
) {
    unsafe {
        assert!(upb_MiniTable_Link(
            mini_table,
            submessages.as_ptr() as *const *const upb_MiniTable,
            submessages.len(),
            subenums.as_ptr() as *const *const upb_MiniTableEnum,
            subenums.len()
        ));
    }
}

unsafe impl<T: MiniDescriptorEnum> AssociatedMiniTableEnum for T {
    fn mini_table() -> MiniTableEnumPtr {
        static MINI_TABLE: OnceLock<MiniTableEnumPtr> = OnceLock::new();
        *MINI_TABLE.get_or_init(|| unsafe {
            let mini_descriptor = Self::mini_descriptor();
            MiniTableEnumPtr(upb_MiniTableEnum_Build(
                mini_descriptor.as_ptr(),
                mini_descriptor.len(),
                THREAD_LOCAL_ARENA.with(|a| a.raw()),
                std::ptr::null_mut(),
            ))
        })
    }
}
