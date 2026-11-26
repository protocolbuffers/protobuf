// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#[cfg(not(bzl))]
use super::super::sys;
#[cfg(not(bzl))]
use super::reflection;

use core::ffi::c_int;
use reflection::def_pool::RawDefPool;
use reflection::message_def::RawMessageDef;
use sys::base::status::upb_Status;
use sys::mem::arena::RawArena;
use sys::message::message::RawMessage;

pub mod json_encode_options {
    use core::ffi::c_int;

    pub const EMIT_DEFAULTS: c_int = 1 << 0;
    pub const USE_PROTO_NAMES: c_int = 1 << 1;
    pub const FORMAT_ENUMS_AS_INTEGERS: c_int = 1 << 2;
}

pub mod json_decode_options {
    use core::ffi::c_int;

    pub const IGNORE_UNKNOWN: c_int = 1 << 0;
    pub const VALIDATE_UTF8_DISABLE: c_int = 1 << 1;
}

pub const UPB_JSON_DECODE_RESULT_OK: c_int = 0;

unsafe extern "C" {
    /// Encodes a message to JSON format.
    ///
    /// Returns the number of bytes required to store the resulting JSON (excluding the null
    /// terminator), or `usize::MAX` (`(size_t)-1`) if encoding fails.
    ///
    /// # Safety
    /// - `msg` must point to a valid `upb_Message` matching descriptor `m`.
    /// - `m` must be a valid `RawMessageDef`.
    /// - `ext_pool`, if `Some`, must be a valid `RawDefPool` that is not mutated during the call
    ///   and contains every extension present in `msg` and its reachable submessages.
    /// - `buf` must be writable for at least `size` bytes (or may be null if `size` is 0).
    /// - `status` must be either null or a valid writable pointer to `upb_Status`.
    pub fn upb_JsonEncode(
        msg: RawMessage,
        m: RawMessageDef,
        ext_pool: Option<RawDefPool>,
        options: c_int,
        buf: *mut u8,
        size: usize,
        status: *mut upb_Status,
    ) -> usize;

    /// Decodes a JSON string into `msg` (merge semantics).
    ///
    /// Returns 0 on success, or a non-zero error code on failure.
    ///
    /// # Safety
    /// - `buf` must be readable for `size` bytes (or may be null if `size` is 0).
    /// - `msg` must point to a valid, mutable (unfrozen) `upb_Message` matching valid `m`, outlived
    ///   by valid `arena`.
    /// - `symtab` must be a valid `RawDefPool` not mutated during the call, and any extensions
    ///   decoded into `msg` must outlive `msg`.
    /// - `status` must be either null or a valid writable pointer to `upb_Status`.
    pub fn upb_JsonDecodeDetectingNonconformance(
        buf: *const u8,
        size: usize,
        msg: RawMessage,
        m: RawMessageDef,
        symtab: RawDefPool,
        options: c_int,
        arena: RawArena,
        status: *mut upb_Status,
    ) -> c_int;
}

#[cfg(test)]
mod test_helpers;

#[cfg(test)]
mod tests {
    use super::*;
    use googletest::gtest;

    #[gtest]
    fn assert_json_linked() {
        use super::test_helpers::assert_linked;
        assert_linked!(upb_JsonEncode);
        assert_linked!(upb_JsonDecodeDetectingNonconformance);
    }
}
