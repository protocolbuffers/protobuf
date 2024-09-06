// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#[repr(C)]
pub struct MiniTableEnum {
    mask_limit: u32,
    value_count: u32,

    // A dynamically-sized array of values.  The layout is:
    //   mask: [u32; mask_limit / 32],   // Membership bitmask.
    //   values: [u32; value_count],     // List of values.
    //
    // Invariant: mask_limit >= 64. It follows that mask.len() >= 2.
    data: [u32; 2],
}

impl MiniTableEnum {
    fn invariants_hold(&self) -> bool {
        self.mask_limit >= 64
    }

    fn mask(&self) -> &[u32] {
        debug_assert!(self.invariants_hold());
        unsafe { std::slice::from_raw_parts(self.data.as_ptr(), self.mask_limit as usize / 32) }
    }

    fn values(&self) -> &[u32] {
        unsafe {
            std::slice::from_raw_parts(self.mask().as_ptr_range().end, self.value_count as usize)
        }
    }

    #[inline]
    pub fn has_value(&self, value: u32) -> bool {
        if value >= 64 {
            self.has_value_fallback(value)
        } else {
            // Fast case: a single bitmask test.
            let mask = self.mask();
            let mask_64: u64 = mask[0] as u64 | ((mask[1] as u64) << 32);
            mask_64 & ((1 as u64) << (value % 64)) != 0
        }
    }

    fn has_value_fallback(&self, value: u32) -> bool {
        if value >= self.mask_limit {
            self.has_value_fallback_slow(value)
        } else {
            // Medium case: a bitmask array test.
            self.mask()[value as usize / 32] & (1 << (value % 32)) != 0
        }
    }

    fn has_value_fallback_slow(&self, value: u32) -> bool {
        // Slow case: binary search. TODO: should we use a linear search for short
        // lists?
        self.values().binary_search(&value).is_ok()
    }
}
