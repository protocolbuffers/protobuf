// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use std::alloc::{alloc, dealloc, Layout};

#[unsafe(no_mangle)]
unsafe extern "C" fn proto2_rust_alloc(size: usize, align: usize) -> *mut u8 {
    if size == 0 {
        // A 0-sized layout is legal but the global allocator isn't required to support
        // it so return a dangling pointer instead.
        std::ptr::null_mut::<u8>().wrapping_add(align)
    } else {
        let layout = Layout::from_size_align(size, align).unwrap();
        // SAFETY: `size != 0` and `layout` is valid.
        unsafe { alloc(layout) }
    }
}

#[unsafe(no_mangle)]
unsafe extern "C" fn proto2_rust_dealloc(ptr: *mut u8, size: usize, align: usize) {
    if size != 0 {
        let layout = Layout::from_size_align(size, align).unwrap();
        // SAFETY: `size != 0` and caller guarantees `ptr` was allocated with `layout`.
        unsafe { dealloc(ptr, layout) }
    }
}
