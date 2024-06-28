// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use crate::opaque_pointee::opaque_pointee;
use std::ptr::NonNull;

opaque_pointee!(upb_MiniTable);
pub type RawMiniTable = NonNull<upb_MiniTable>;

opaque_pointee!(upb_MiniTableField);
pub type RawMiniTableField = NonNull<upb_MiniTableField>;

extern "C" {
    pub fn upb_MiniTable_FindFieldByNumber(
        m: *const upb_MiniTable,
        number: u32,
    ) -> *const upb_MiniTableField;
}
