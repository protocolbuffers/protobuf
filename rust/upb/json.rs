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
use sys_json::{
    json_decode_options, json_encode_options, upb_JsonDecodeDetectingNonconformance,
    upb_JsonEncode, UPB_JSON_DECODE_RESULT_OK,
};
use upb::{encode_to_string_with, Arena, MessagePtr, Status};

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

/// Options for upb's JSON decoder.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct JsonDecodeOptions {
    /// Ignore unknown fields and unknown enum string values.
    pub ignore_unknown: bool,
}

impl JsonDecodeOptions {
    /// The bitmask that `upb_JsonDecodeDetectingNonconformance` expects.
    pub fn to_bits(self) -> c_int {
        let mut bits = 0;
        if self.ignore_unknown {
            bits |= json_decode_options::IGNORE_UNKNOWN;
        }
        bits
    }
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct JsonDecodeError {
    message: String,
}

impl fmt::Display for JsonDecodeError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str(&self.message)
    }
}

impl std::error::Error for JsonDecodeError {}

/// Decodes a JSON string into `msg` using `options`.
///
/// # Safety
/// - `msg` must be legally dereferenceable, mutable (unfrozen), matching `def`, and outlived by
///   `arena`.
/// - Any extensions in `symtab` that may be decoded into `msg` must outlive `msg`.
#[allow(unused)]
pub unsafe fn json_decode<T>(
    input: &str,
    msg: MessagePtr<T>,
    def: MessageDef<'_>,
    symtab: &DefPool,
    arena: &Arena,
    options: JsonDecodeOptions,
) -> Result<(), JsonDecodeError> {
    // `input` is a Rust `&str`, so it is already valid UTF-8.
    let options = options.to_bits() | json_decode_options::VALIDATE_UTF8_DISABLE;
    let mut status = Status::new();

    // SAFETY: `input` is readable for `input.len()` bytes; `msg`, `def`, `symtab`, and `arena`
    // satisfy the caller contract and Rust borrows; `status` is a valid local `upb_Status`.
    let result = unsafe {
        upb_JsonDecodeDetectingNonconformance(
            input.as_ptr(),
            input.len(),
            msg.raw(),
            def.raw(),
            symtab.raw(),
            options,
            arena.raw(),
            status.as_mut_ptr(),
        )
    };

    if result == UPB_JSON_DECODE_RESULT_OK {
        Ok(())
    } else {
        let mut message = status.error_message();
        if message.is_empty() {
            // Empty input returns non-zero without setting `status`.
            message = "upb JSON decoding failed".to_string();
        }
        Err(JsonDecodeError { message })
    }
}
