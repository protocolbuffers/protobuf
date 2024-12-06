// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#![deny(unsafe_op_in_unsafe_fn)]

use std::fmt;

// There are a number of manual `Debug` and similar impls instead of using their
// derives, in order to to avoid unnecessary bounds on a generic `T`.
// This problem is referred to as "perfect derive".
// https://smallcultfollowing.com/babysteps/blog/2022/04/12/implied-bounds-and-perfect-derive/

pub use crate::codegen_traits::{
    create::Parse,
    interop::{MessageMutInterop, MessageViewInterop, OwnedMessageInterop},
    read::Serialize,
    write::{Clear, ClearAndParse, MergeFrom},
    Message, MessageMut, MessageView,
};
pub use crate::cord::{ProtoBytesCow, ProtoStringCow};
pub use crate::map::{Map, MapIter, MapMut, MapView, ProxiedInMapValue};
pub use crate::optional::Optional;
pub use crate::proxied::{
    AsMut, AsView, IntoMut, IntoProxied, IntoView, Mut, MutProxied, MutProxy, Proxied, Proxy, View,
    ViewProxy,
};
pub use crate::r#enum::{Enum, UnknownEnumValue};
pub use crate::repeated::{ProxiedInRepeated, Repeated, RepeatedIter, RepeatedMut, RepeatedView};
pub use crate::string::{ProtoBytes, ProtoStr, ProtoString, Utf8Error};

pub mod prelude;

/// The `__internal` module is for necessary encapsulation breaks between
/// generated code and the runtime.
///
/// These symbols are never intended to be used by application code under any
/// circumstances.
///
/// In blaze/bazel builds, this symbol is actively hidden from application
/// code by having a shim crate in front that does not re-export this symbol,
/// and a different BUILD visibility-restricted target that is used by the
/// generated code.
///
/// In Cargo builds we have no good way to technically hide this
/// symbol while still allowing it from codegen, so it is only by private by
/// convention. As application code should never use this module, anything
/// changes under `__internal` is not considered a semver breaking change.
#[path = "internal.rs"]
pub mod __internal;

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
