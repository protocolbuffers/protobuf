// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

//! Rust Protobuf Full Runtime
//!
//! This crate re-exports the `protobuf` lite runtime and (in the future) adds reflection
//! traits and heavy APIs (like text formatting) that are banned in `lite` mode.

pub use protobuf_lite::*;
