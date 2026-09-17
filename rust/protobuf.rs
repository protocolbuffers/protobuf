// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

//! Rust Protobuf Full Runtime
//!
//! This crate re-exports the `protobuf` lite runtime and adds reflection traits
//! and heavy APIs (like text formatting) that are banned in `lite` mode.

#[cfg(cpp_kernel)]
use protobuf_cpp as kernel;

#[cfg(upb_kernel)]
use protobuf_upb as kernel;

/// Blocks `__internal` from being re-exported by the `pub use` below, the same
/// way `protobuf_lite.rs` does. Application code must never reach it.
#[doc(hidden)]
#[allow(non_upper_case_globals)]
pub const __internal: () = ();

pub use kernel::*;
