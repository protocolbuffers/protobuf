// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#![deny(unsafe_op_in_unsafe_fn)]

use core::ffi::c_int;

// In Bazel `upb`, `reflection` and `sys_json` are dependency crates. In Cargo this file is
// compiled as <crate_root>::upb::json, so bring the parent module, the sibling `reflection` module
// and upb's `sys::json` module into scope under the same names.
#[cfg(not(bzl))]
use super::super::upb;
#[cfg(not(bzl))]
use super::reflection;
#[cfg(not(bzl))]
use super::sys::json as sys_json;

use reflection::{DefPool, MessageDef};
use sys_json::json::upb_JsonEncode;
use upb::{encode_to_string_with, MessagePtr};

pub use sys_json::json::json_encode_options;
pub use upb::EncodeError as JsonEncodeError;

/// Encodes `msg` to a JSON string using `options` (a bitmask of [`json_encode_options`]).
///
/// `ext_pool` is used to resolve both extensions and `google.protobuf.Any` type URLs.
///
/// # Safety
/// - `msg` must be legally dereferenceable and its message type must match `def`.
#[allow(unused)] // Not used yet.
pub unsafe fn json_encode<'pool, T>(
    msg: MessagePtr<T>,
    def: MessageDef<'pool>,
    ext_pool: Option<&DefPool>,
    options: c_int,
) -> Result<String, JsonEncodeError> {
    let msg = msg.raw();
    let def_raw = def.raw();
    let ext_pool_raw = ext_pool.map(|p| p.raw());

    encode_to_string_with(|buf| {
        // SAFETY:
        // - `msg` and `def_raw` are valid and matching per the caller's safety contract.
        // - `ext_pool_raw`, if `Some`, comes from a live `&DefPool`.
        // - `buf` is a mutable slice, so `buf.as_mut_ptr()` is writable for `buf.len()` bytes.
        // - `status` is null.
        unsafe {
            upb_JsonEncode(
                msg,
                def_raw,
                ext_pool_raw,
                options,
                buf.as_mut_ptr(),
                buf.len(),
                core::ptr::null_mut(),
            )
        }
    })
}
