// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

//! Reflection support for the C++ kernel.
//!
//! This module is not compiled at all in lite builds, so nothing in it needs
//! its own `#[cfg(not(lite_runtime))]`. See `mod.rs` for the single gate.

use super::*;

unsafe extern "C" {
    /// From message.cc
    ///
    /// # Safety
    /// - `m` is a legally dereferenceable MessageLite* pointer.
    pub fn proto2_rust_Message_get_descriptor(m: RawMessage) -> *const std::ffi::c_void;

    /// From message.cc
    ///
    /// # Safety
    /// - `m` is a legally dereferenceable MessageLite* that supports reflection.
    pub fn proto2_rust_Message_print_to_text_format(m: RawMessage) -> RustStringRawParts;
}
