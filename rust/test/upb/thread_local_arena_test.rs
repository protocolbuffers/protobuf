// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use googletest::prelude::*;
use protobuf::prelude::*;
use unittest_rust_proto::TestAllTypes;

#[gtest]
fn test_thread_local_arena() {
    // We use a thread-local arena to allocate minitables, so let's verify that a minitable remains
    // valid even after that the thread that initialized it has exited.
    let handle = std::thread::spawn(|| {
        let mut m = TestAllTypes::new();
        m.set_optional_int32(3);
        let _ = m.serialize();
    });
    handle.join().unwrap();

    let mut m1 = TestAllTypes::new();
    m1.set_optional_int32(5);
    let m2 = TestAllTypes::parse(&m1.serialize().unwrap()).unwrap();
    assert_that!(m1.optional_int32(), eq(m2.optional_int32()));
}
