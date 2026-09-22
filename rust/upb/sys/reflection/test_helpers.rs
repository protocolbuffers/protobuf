// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Intentionally duplicated from `sys/test_helpers.rs`.

/// Force a compiler error if the passed in function is not linked.
macro_rules! assert_linked {
    ($($vals:tt)+) => {
        let _ = std::hint::black_box($($vals)+ as *const ());
    }
}

pub(crate) use assert_linked;
