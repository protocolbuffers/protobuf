// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

//! Tests for borrowing messages as a C++ `proto2::MessageLite`.

use googletest::matchers::eq;
use googletest::{expect_that, gtest};
use interop_test_rust_proto::InteropTestMessage;
use protobuf_cpp::{CppMessageLiteType, MessageMutInterop, MessageViewInterop};
use std::ffi::c_void;
use std::marker::PhantomPinned;
use std::pin::Pin;

/// Stands in for the Crubit binding of `proto2::MessageLite`.
struct FakeMessageLite {
    _pinned: PhantomPinned,
}

// SAFETY: `FakeMessageLite` is zero-sized with an alignment of 1, so it is no
// larger or more strictly aligned than `proto2::MessageLite` and has no bytes
// outside of an `UnsafeCell`. It is `!Unpin` because of `PhantomPinned`.
unsafe impl CppMessageLiteType for FakeMessageLite {}

// These take the binding type as a parameter, like the Crubit bindings of C++
// functions taking a `proto2::MessageLite`, so that callers don't need to
// specify it.
fn address_of(lite: &FakeMessageLite) -> *const c_void {
    std::ptr::from_ref(lite).cast()
}

fn address_of_mut(lite: Pin<&mut FakeMessageLite>) -> *const c_void {
    std::ptr::from_ref(&*lite).cast()
}

#[gtest]
fn as_cpp_message_lite_borrows_message() {
    let msg = InteropTestMessage::new();
    let view = msg.as_view();
    expect_that!(address_of(view.as_cpp_message_lite()), eq(view.__unstable_as_raw_message()));
}

#[gtest]
fn as_cpp_message_lite_mut_borrows_message() {
    let mut msg = InteropTestMessage::new();
    let raw = msg.as_mut().__unstable_as_raw_message_mut().cast_const();
    expect_that!(address_of_mut(msg.as_mut().as_cpp_message_lite_mut()), eq(raw));
}
