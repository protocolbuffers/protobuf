// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

//! Reflection support for the upb kernel.
//!
//! upb messages carry no descriptors of their own, so every reflective operation first looks up
//! the message's `MessageDef` in the process-wide `DefPool` (see `def_pool.rs`), then hands both
//! the message and the def to upb.

use super::upb::TextEncodeOptions;
use super::{message_def, upb_reflection, UpbGetMessagePtr, UpbWithReflection};
use crate::__internal::Private;
use crate::WithReflection;

/// Implemented by every generated message whose `MessageDef` can be looked up at runtime.
///
/// `UpbGetMessagePtr<Msg = Self>` ties the message pointer that reflection APIs operate on to the
/// `MessageDef` they look up by `Self`, so the two can never describe different message types.
pub trait KernelWithReflection: UpbWithReflection + UpbGetMessagePtr<Msg = Self> {}
impl<T: UpbWithReflection + UpbGetMessagePtr<Msg = T>> KernelWithReflection for T {}

/// Returns the protobuf TextFormat representation of `msg` with the default [`TextEncodeOptions`].
pub fn print_to_text_format<T: WithReflection>(msg: &T) -> String {
    print_to_text_format_with_options(msg, TextEncodeOptions::default())
}

/// Returns the protobuf TextFormat representation of `msg`, formatted according to `options`.
pub fn print_to_text_format_with_options<T: WithReflection>(
    msg: &T,
    options: TextEncodeOptions,
) -> String {
    let ptr = msg.get_ptr(Private);
    let def = message_def::<T>();
    // SAFETY:
    // - `ptr` points to a live message that `msg` keeps alive for the duration of the call.
    // - `def` describes `T`, and `WithReflection` guarantees `kernelWithReflection`, which
    //   guarantees `UpbGetMessagePtr<Msg = T>, therefore `ptr` is a `MessagePtr<T>`.
    // TODO: Pass the global DefPool as `ext_pool` so that extensions are printed.
    unsafe { upb_reflection::text_encode(ptr, def, None, options) }
}
