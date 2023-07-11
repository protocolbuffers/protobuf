// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google LLC. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//! Kernel-agnostic logic for the Rust Protobuf Runtime.
//!
//! For kernel-specific logic this crate delegates to the respective `__runtime`
//! crate.

/// Everything in `__runtime` is allowed to change without it being considered
/// a breaking change for the protobuf library. Nothing in here should be
/// exported in `protobuf.rs`.
#[cfg(cpp_kernel)]
#[path = "cpp.rs"]
pub mod __runtime;
#[cfg(upb_kernel)]
#[path = "upb.rs"]
pub mod __runtime;

mod optional;
mod proxied;
mod string;

pub use optional::{AbsentField, FieldEntry, Optional, PresentField};
pub use proxied::{Mut, MutProxy, Proxied, ProxiedWithPresence, SettableValue, View, ViewProxy};
pub use string::BytesMut;

/// Everything in `__internal` is allowed to change without it being considered
/// a breaking change for the protobuf library. Nothing in here should be
/// exported in `protobuf.rs`.
#[path = "internal.rs"]
pub mod __internal;

use std::fmt;

/// An error that happened during deserialization.
#[derive(Debug, Clone)]
pub struct ParseError;

impl fmt::Display for ParseError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "Couldn't deserialize given bytes into a proto")
    }
}
