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

use super::{message_def, upb_reflection, UpbGetMessagePtr, UpbWithReflection};
use crate::__internal::Private;

/// Implemented by every generated message whose `MessageDef` can be looked up at runtime.
///
/// `UpbGetMessagePtr<Msg = Self>` ties the message pointer that reflection APIs operate on to the
/// `MessageDef` they look up by `Self`, so the two can never describe different message types.
pub trait KernelWithReflection: UpbWithReflection + UpbGetMessagePtr<Msg = Self> {}
impl<T: UpbWithReflection + UpbGetMessagePtr<Msg = T>> KernelWithReflection for T {}

/// Returns the protobuf TextFormat representation of `msg`.
///
/// Extensions are currently not printed: upb needs every set extension's `FieldDef` to be present
/// in the pool it is given, but the global pool is only populated with the files of the messages
/// that have been reflected on so far.
pub fn print_to_text_format<T: crate::WithReflection>(msg: &T) -> String {
    let ptr = msg.get_ptr(Private);
    let def = message_def::<T>();
    // SAFETY:
    // - `ptr` points to a live message that `msg` keeps alive for the duration of the call.
    // - `def` describes `T`, and `KernelWithReflection` guarantees `ptr` is a `MessagePtr<T>`.
    // TODO: Support configuring options.
    unsafe { upb_reflection::text_encode(ptr, def, None, 0) }
}
