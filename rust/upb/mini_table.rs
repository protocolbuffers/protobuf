// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use super::sys::mini_table::mini_table as mt_sys;
use super::AssociatedMiniTable;
use core::marker::PhantomData;

pub type MiniTable = mt_sys::upb_MiniTable;

pub struct MiniTableFieldPtr<T> {
    pub(crate) raw: *const mt_sys::upb_MiniTableField,
    _phantom: PhantomData<T>,
}

impl<T: AssociatedMiniTable> MiniTableFieldPtr<T> {
    pub unsafe fn get_field_by_index(index: u32) -> MiniTableFieldPtr<T> {
        let field = unsafe { mt_sys::upb_MiniTable_GetFieldByIndex(T::mini_table(), index) };
        return MiniTableFieldPtr { raw: field, _phantom: PhantomData };
    }
}
