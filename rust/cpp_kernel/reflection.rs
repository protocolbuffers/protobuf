// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

//! Reflection support for the C++ kernel.
//!
//! The descriptor itself is fetched through a per-message thunk emitted into
//! that message's generated C++ code, so that only protos which actually need
//! reflection pull in the full C++ runtime.

use super::*;

pub trait KernelWithReflection: MessageDescriptorInterop {}
impl<T: MessageDescriptorInterop> KernelWithReflection for T {}

#[cfg(not(lite_runtime))]
unsafe extern "C" {
    /// From reflection.cc
    ///
    /// # Safety
    /// - `m` is a legally dereferenceable MessageLite* that supports
    ///   reflection.
    fn proto2_rust_Message_print_to_text_format(m: RawMessage) -> RustStringRawParts;

    /// From reflection.cc
    ///
    /// # Safety
    /// - `m` is a legally dereferenceable and mutable MessageLite* that
    ///   supports reflection.
    /// - `input` is a valid `PtrAndLen` pointing to readable memory for its
    ///   length.
    fn proto2_rust_Message_parse_from_text_format(m: RawMessage, input: PtrAndLen) -> bool;
}

/// Returns the protobuf TextFormat representation of `msg`.
#[cfg(not(lite_runtime))]
pub fn print_to_text_format<T: crate::WithReflection>(msg: &T) -> String {
    let raw = msg.as_view().get_raw_message(Private);
    // SAFETY: `raw` points to a live message, and `WithReflection` guarantees it
    // supports reflection.
    unsafe { proto2_rust_Message_print_to_text_format(raw) }.into()
}

/// Parses a message from its protobuf TextFormat representation.
#[cfg(not(lite_runtime))]
pub fn parse_from_text_format<T: crate::WithReflection>(input: &str) -> Result<T, ParseError> {
    let mut msg = T::default();
    let raw = msg.get_raw_message_mut(Private);
    // SAFETY:
    // - `raw` points to a live, mutable message, and `WithReflection`
    //   guarantees it supports reflection.
    // - `input` is converted from a valid `&str` slice.
    unsafe { proto2_rust_Message_parse_from_text_format(raw, input.as_bytes().into()) }
        .then_some(msg)
        .ok_or(ParseError)
}
