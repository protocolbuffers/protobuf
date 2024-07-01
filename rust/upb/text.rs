// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use crate::{upb_MiniTable, RawMessage};

extern "C" {
    /// SAFETY: No constraints.
    pub fn upb_DebugString(
        msg: RawMessage,
        mt: *const upb_MiniTable,
        options: i32,
        buf: *mut u8,
        size: usize,
    ) -> usize;
}
