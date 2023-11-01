// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

//! Rust Protobuf Runtime
//!
//! This file forwards to `shared.rs`, which exports a kernel-specific
//! `__runtime`. Rust Protobuf gencode actually depends directly on kernel
//! specific crates. The only reason this crate exists is to be able to use
//! `protobuf` as a crate name for both cpp and upb kernels from user code.

#[cfg(cpp_kernel)]
use protobuf_cpp as kernel;

#[cfg(upb_kernel)]
use protobuf_upb as kernel;

pub use kernel::__public::*;
