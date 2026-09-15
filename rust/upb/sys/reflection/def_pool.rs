// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use super::opaque_pointee::opaque_pointee;
use core::ptr::NonNull;

opaque_pointee!(upb_DefPool);
pub type RawDefPool = NonNull<upb_DefPool>;

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
}

#[cfg(test)]
mod tests {
    use googletest::gtest;

    #[gtest]
    fn assert_def_pool_linked() {
        use super::super::test_helpers::assert_linked;
        assert_linked!(super::upb_DefPool_New);
        assert_linked!(super::upb_DefPool_Free);
    }
}
