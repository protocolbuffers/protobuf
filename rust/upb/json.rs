// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use core::ffi::c_int;
use core::fmt;

#[cfg(not(bzl))]
use super::super::upb;
#[cfg(not(bzl))]
use super::reflection;
#[cfg(not(bzl))]
use super::sys::json as sys_json;

use reflection::{DefPool, MessageDef};
use sys_json::{json_encode_options, upb_JsonEncode};
use upb::{encode_to_string_with, MessagePtr, Status};

/// Options for upb's JSON encoder.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct JsonEncodeOptions {
    /// Emit fields with default values (proto3 implicit-presence fields).
    pub emit_defaults: bool,
    /// Use original proto field names instead of JSON names.
    pub use_proto_names: bool,
    /// Emit enum values as integers instead of their names.
    pub format_enums_as_integers: bool,
}

impl JsonEncodeOptions {
    /// The bitmask that `upb_JsonEncode` expects.
    pub fn to_bits(self) -> c_int {
        let mut bits = 0;
        if self.emit_defaults {
            bits |= json_encode_options::EMIT_DEFAULTS;
        }
        if self.use_proto_names {
            bits |= json_encode_options::USE_PROTO_NAMES;
        }
        if self.format_enums_as_integers {
            bits |= json_encode_options::FORMAT_ENUMS_AS_INTEGERS;
        }
        bits
    }
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct JsonEncodeError {
    message: String,
}

impl fmt::Display for JsonEncodeError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str(&self.message)
    }
}

impl std::error::Error for JsonEncodeError {}

/// Encodes `msg` to a JSON string using `options`.
///
/// `ext_pool` is used to resolve both extensions and `google.protobuf.Any` type URLs.
///
/// # Safety
/// - `msg` must be legally dereferenceable and its message type must match `def`.
/// - If `ext_pool` is `Some`, it must contain every extension present in `msg` and its reachable
///   submessages.
#[allow(unused)] // Not used yet.
pub unsafe fn json_encode<'pool, T>(
    msg: MessagePtr<T>,
    def: MessageDef<'pool>,
    ext_pool: Option<&DefPool>,
    options: JsonEncodeOptions,
) -> Result<String, JsonEncodeError> {
    let msg = msg.raw();
    let def_raw = def.raw();
    let ext_pool_raw = ext_pool.map(|p| p.raw());
    let options = options.to_bits();
    let mut status = Status::new();

    encode_to_string_with(|buf| {
        // SAFETY:
        // - `msg`, `def_raw`, and `ext_pool_raw` (if `Some`) satisfy `upb_JsonEncode`'s
        //   preconditions per the caller's contract and the `&DefPool` borrow.
        // - `buf` is writable for `buf.len()` bytes, and `status` is a valid local `upb_Status`.
        unsafe {
            upb_JsonEncode(
                msg,
                def_raw,
                ext_pool_raw,
                options,
                buf.as_mut_ptr(),
                buf.len(),
                status.as_mut_ptr(),
            )
        }
    })
    .map_err(|_| JsonEncodeError { message: status.error_message() })
}
