// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#[cfg(not(bzl))]
mod protos;
#[cfg(not(bzl))]
use protos::*;

use googletest::prelude::*;

use std::sync::{Arc, Mutex};
use unittest_rust_proto::TestAllTypes;

#[gtest]
fn test_sending_owned_arc() {
    let msg = Arc::new(Mutex::new(TestAllTypes::default()));

    let msg_clone = Arc::clone(&msg);
    let handle = std::thread::spawn(move || {
        let mut locked_msg = msg_clone.lock().unwrap();
        locked_msg.set_optional_int32(123);
    });
    handle.join().unwrap();

    let locked_msg = msg.lock().unwrap();
    assert_eq!(locked_msg.optional_int32(), 123);
}

#[gtest]
fn test_sending_mut_scoped() {
    let mut msg = TestAllTypes::default();

    std::thread::scope(|scope| {
        let mut child_mut = msg.optional_nested_message_mut();
        scope.spawn(move || {
            child_mut.set_bb(123);
        });
    });

    assert_eq!(msg.optional_nested_message().bb(), 123);
}
