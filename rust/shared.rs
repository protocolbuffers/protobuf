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

// There are a number of manual `Debug` and similar impls instead of using their
// derives, in order to to avoid unnecessary bounds on a generic `T`.
// This problem is referred to as "perfect derive".
// https://smallcultfollowing.com/babysteps/blog/2022/04/12/implied-bounds-and-perfect-derive/

/// Everything in `__public` is re-exported in `protobuf.rs`.
/// These are the items protobuf users can access directly.
#[doc(hidden)]
pub mod __public {
    pub use crate::r#enum::{Enum, UnknownEnumValue};
    pub use crate::map::{Map, MapIter, MapMut, MapView, ProxiedInMapValue};
    pub use crate::optional::Optional;
    pub use crate::proto;
    pub use crate::proxied::{IntoProxied, Mut, MutProxied, MutProxy, Proxied, View, ViewProxy};
    pub use crate::repeated::{
        ProxiedInRepeated, Repeated, RepeatedIter, RepeatedMut, RepeatedView,
    };
    pub use crate::string::ProtoStr;
    pub use crate::ParseError;
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

#[path = "enum.rs"]
mod r#enum;
mod map;
mod optional;
mod primitive;
mod proto_macro;
mod proxied;
mod repeated;
mod string;

/// An error that happened during deserialization.
#[derive(Debug, Clone)]
pub struct ParseError;

impl std::error::Error for ParseError {}

impl fmt::Display for ParseError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "Couldn't deserialize given bytes into a proto")
    }
}
