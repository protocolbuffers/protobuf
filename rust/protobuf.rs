// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

//! Rust Protobuf Runtime
//!
//! This file exists as the public entry point for the Rust Protobuf runtime. It
//! is a thin re-export of the `shared.rs` file but is needed for two reasons:
//! - To create a single `protobuf` crate name for either cpp and upb kernels
//!   from user code (toggled at compile time).
//! - Blocks the __internal module from being re-exported to application code,
//!   unless they use one of our visibility-restricted targets (gencode does
//!   have access to them).

#[cfg(cpp_kernel)]
use protobuf_cpp as kernel;

#[cfg(upb_kernel)]
use protobuf_upb as kernel;

/// Block these two mods from being re-exported by the `pub use`
/// below (glob use automatically only adds things that aren't otherwise
/// defined).
///
/// By creating a const instead of an empty mod it is easier to have a test
/// that confirms this targeted 'blocking' is working as intended.
#[doc(hidden)]
#[allow(non_upper_case_globals)]
pub const __internal: () = ();

pub use kernel::*;
