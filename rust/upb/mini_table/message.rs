// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// We have some dead code that has been ported from C but is not yet used.
#![allow(dead_code)]

use super::r#enum::MiniTableEnum;
use super::field::{CType, MiniTableField};

#[repr(C)]
union SubInternal {
    submsg: *const *const MiniTable,
    subenum: *const MiniTableEnum,
}

#[repr(C)]
pub struct MiniTable {
    subs: *const SubInternal,
    fields: *const MiniTableField,
    size: u16,
    field_count: u16,
    ext: u8,
    dense_below: u8,
    table_mask: u8,
    required_count: u8,
}

enum ExtMode {
    NonExtendable = 0,
    Extendable = 1,
    IsMessageSet = 2,
    IsMessageSetItem = 3,
    IsMapEntry = 4,
}

impl MiniTable {
    fn fields(&self) -> &[MiniTableField] {
        unsafe { std::slice::from_raw_parts(self.fields, self.field_count as usize) }
    }

    pub fn field_by_index(&self, idx: u32) -> &MiniTableField {
        &self.fields()[idx as usize]
    }

    /// # Safety
    /// - `idx` must be less than the number of fields in the message.
    pub unsafe fn field_by_index_unchecked(&self, idx: u32) -> &MiniTableField {
        // TODO: replace debug_assert!() with a precondition!() macro, which has the
        // following behavior:
        // * In debug mode, it is a debug_assert!()
        // * In release mode, it is: if !x { core::hint::unreachable_unchecked() }
        debug_assert!(idx < self.field_count as u32);
        self.field_by_index(idx)
    }

    pub fn sub_message(&self, f: &MiniTableField) -> Option<&MiniTable> {
        match f.c_type() {
            CType::Message => unsafe {
                let sub = &*self.subs.add(f.submsg_index());
                Some(&**sub.submsg)
            },
            _ => None,
        }
    }
}
