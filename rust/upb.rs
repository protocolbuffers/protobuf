// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//! Hand-written bindings for the UPB API.

#![allow(dead_code)]

#[repr(C)]
pub struct upb_Message {
    _priv: u8,
}
#[repr(C)]
pub struct upb_Arena {
    _priv: u8,
}
#[repr(C)]
pub struct upb_MiniTableField {
    _priv: u8,
}

#[repr(C)]
pub struct upb_StringView {
    pub data: *const u8,
    pub size: usize, // Should be size_t but... here we are.
}

// upb/message/accessors.h
extern "C" {
    pub fn upb_Message_ClearField(msg: *mut upb_Message, field: *const upb_MiniTableField);
    pub fn upb_Message_HasField(msg: *mut upb_Message, field: *const upb_MiniTableField) -> bool;

    pub fn upb_Message_GetBool(
        msg: *mut upb_Message,
        field: *const upb_MiniTableField,
        default: bool,
    ) -> bool;
    pub fn upb_Message_SetBool(
        msg: *mut upb_Message,
        field: *const upb_MiniTableField,
        value: bool,
        a: *mut upb_Arena,
    ) -> bool;

    pub fn upb_Message_GetInt32(
        msg: *mut upb_Message,
        field: *const upb_MiniTableField,
        default: i32,
    ) -> i32;
    pub fn upb_Message_SetInt32(
        msg: *mut upb_Message,
        field: *const upb_MiniTableField,
        value: i32,
        a: *mut upb_Arena,
    ) -> bool;

    pub fn upb_Message_GetUInt32(
        msg: *mut upb_Message,
        field: *const upb_MiniTableField,
        default: u32,
    ) -> u32;
    pub fn upb_Message_SetUInt32(
        msg: *mut upb_Message,
        field: *const upb_MiniTableField,
        value: u32,
        a: *mut upb_Arena,
    ) -> bool;

    pub fn upb_Message_GetInt64(
        msg: *mut upb_Message,
        field: *const upb_MiniTableField,
        default: i64,
    ) -> i64;
    pub fn upb_Message_SetInt64(
        msg: *mut upb_Message,
        field: *const upb_MiniTableField,
        value: i64,
        a: *mut upb_Arena,
    ) -> bool;

    pub fn upb_Message_GetUInt64(
        msg: *mut upb_Message,
        field: *const upb_MiniTableField,
        default: u64,
    ) -> u64;
    pub fn upb_Message_SetUInt64(
        msg: *mut upb_Message,
        field: *const upb_MiniTableField,
        value: u64,
        a: *mut upb_Arena,
    ) -> bool;

    pub fn upb_Message_GetString(
        msg: *mut upb_Message,
        field: *const upb_MiniTableField,
        default: upb_StringView,
    ) -> upb_StringView;
    pub fn upb_Message_SetString(
        msg: *mut upb_Message,
        field: *const upb_MiniTableField,
        value: upb_StringView,
        a: *mut upb_Arena,
    ) -> bool;
}
