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
use super::{
    load_all_extensions, message_def, read_global_def_pool, upb_reflection, UpbGetMessagePtr,
    UpbWithReflection,
};
use crate::__internal::Private;
use crate::WithReflection;

/// Implemented by every generated message whose `MessageDef` can be looked up at runtime.
///
/// `UpbGetMessagePtr<Msg = Self>` ties the message pointer that reflection APIs operate on to the
/// `MessageDef` they look up by `Self`, so the two can never describe different message types.
pub trait KernelWithReflection: UpbWithReflection + UpbGetMessagePtr<Msg = Self> {
    fn kernel_full_name(_private: Private) -> &'static str {
        Self::FULL_NAME
    }
}
impl<T: UpbWithReflection + UpbGetMessagePtr<Msg = T>> KernelWithReflection for T {}

/// Returns the protobuf TextFormat representation of `msg` with custom options.
pub fn print_to_text_format<T: WithReflection>(
    msg: &T,
    single_line: bool,
    hide_unknown_fields: bool,
) -> String {
    let ptr = msg.get_ptr(Private);
    let def = message_def::<T>();
    let options = TextEncodeOptions { single_line, skip_unknown: hide_unknown_fields };

    // upb looks up every extension set in `msg` in `ext_pool`, so the pool must hold every
    // extension linked into the binary. This loading is a one-time cost paid by the first
    // `text_format::print` call.
    load_all_extensions();

    let ext_pool = read_global_def_pool();

    // SAFETY:
    // - `ptr` points to a live message that `msg` keeps alive for the duration of the call.
    // - `def` describes `T`, and `WithReflection` guarantees `KernelWithReflection`, which
    //   guarantees `UpbGetMessagePtr<Msg = T>`, therefore `ptr` is a `MessagePtr<T>`.
    unsafe { upb_reflection::text_encode(ptr, def, Some(&ext_pool.0), options) }
}
