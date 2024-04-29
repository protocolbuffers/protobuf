// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

//! Kernel-agnostic logic for the Rust Protobuf Runtime.
//!
//! For kernel-specific logic this crate delegates to the respective `__runtime`
//! crate.
#![deny(unsafe_op_in_unsafe_fn)]

use std::fmt;

/// Everything in `__public` is re-exported in `protobuf.rs`.
/// These are the items protobuf users can access directly.
#[doc(hidden)]
pub mod __public {
    pub use crate::optional::{AbsentField, FieldEntry, Optional, PresentField};
    pub use crate::primitive::PrimitiveMut;
    pub use crate::proxied::{
        Mut, MutProxy, Proxied, ProxiedWithPresence, SettableValue, View, ViewProxy,
    };
    pub use crate::string::{BytesMut, ProtoStr, ProtoStrMut};
}
pub use __public::*;

/// Everything in `__internal` is allowed to change without it being considered
/// a breaking change for the protobuf library. Nothing in here should be
/// exported in `protobuf.rs`.
#[path = "internal.rs"]
pub mod __internal;

/// Everything in `__runtime` is allowed to change without it being considered
/// a breaking change for the protobuf library. Nothing in here should be
/// exported in `protobuf.rs`.
#[cfg(cpp_kernel)]
#[path = "cpp.rs"]
pub mod __runtime;
#[cfg(upb_kernel)]
#[path = "upb.rs"]
pub mod __runtime;

mod macros;
mod optional;
mod primitive;
mod proxied;
mod string;
mod vtable;

/// An error that happened during deserialization.
#[derive(Debug, Clone)]
pub struct ParseError;

impl fmt::Display for ParseError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "Couldn't deserialize given bytes into a proto")
    }
}
