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
    pub use crate::codegen_traits::{
        create::Parse,
        interop::{MessageMutInterop, MessageViewInterop, OwnedMessageInterop},
        read::Serialize,
        write::{Clear, ClearAndParse, MergeFrom},
        Message, MessageMut, MessageView,
    };
    pub use crate::cord::{ProtoBytesCow, ProtoStringCow};
    pub use crate::r#enum::{Enum, UnknownEnumValue};
    pub use crate::map::{Map, MapIter, MapMut, MapView, ProxiedInMapValue};
    pub use crate::optional::Optional;
    pub use crate::proto;
    pub use crate::proxied::{
        AsMut, AsView, IntoMut, IntoProxied, IntoView, Mut, MutProxied, MutProxy, Proxied, Proxy,
        View, ViewProxy,
    };
    pub use crate::repeated::{
        ProxiedInRepeated, Repeated, RepeatedIter, RepeatedMut, RepeatedView,
    };
    pub use crate::string::{ProtoBytes, ProtoStr, ProtoString};
    pub use crate::{ParseError, SerializeError};
}
pub use __public::*;

pub mod prelude;

/// Everything in `__internal` is allowed to change without it being considered
/// a breaking change for the protobuf library. Nothing in here should be
/// exported in `protobuf.rs`.
#[path = "internal.rs"]
pub mod __internal;

/// Everything in `__runtime` is allowed to change without it being considered
/// a breaking change for the protobuf library. Nothing in here should be
/// exported in `protobuf.rs`.
#[cfg(all(bzl, cpp_kernel))]
#[path = "cpp.rs"]
pub mod __runtime;
#[cfg(any(not(bzl), upb_kernel))]
#[path = "upb.rs"]
pub mod __runtime;

mod codegen_traits;
mod cord;
#[path = "enum.rs"]
mod r#enum;
mod map;
mod optional;
mod primitive;
mod proto_macro;
mod proxied;
mod repeated;
mod string;

#[cfg(not(bzl))]
#[path = "upb/lib.rs"]
mod upb;

#[cfg(not(bzl))]
mod utf8;

// Forces the utf8 crate to be accessible from crate::.
#[cfg(bzl)]
#[allow(clippy::single_component_path_imports)]
use utf8;

// If the Upb and C++ kernels are both linked into the same binary, this symbol
// will be defined twice and cause a link error.
#[no_mangle]
extern "C" fn __Disallow_Upb_And_Cpp_In_Same_Binary() {}

/// An error that happened during parsing.
#[derive(Debug, Clone)]
pub struct ParseError;

impl std::error::Error for ParseError {}

impl fmt::Display for ParseError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "Couldn't deserialize given bytes into a proto")
    }
}

/// An error that happened during serialization.
#[derive(Debug, Clone)]
pub struct SerializeError;

impl std::error::Error for SerializeError {}

impl fmt::Display for SerializeError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "Couldn't serialize proto into bytes (depth too deep or missing required fields)")
    }
}

pub fn get_repeated_default_value<T: repeated::ProxiedInRepeated + Default>(
    _: __internal::Private,
    _: repeated::RepeatedView<'_, T>,
) -> T {
    Default::default()
}
