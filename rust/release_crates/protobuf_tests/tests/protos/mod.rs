// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

pub mod unittest_import_rust_proto {
    include!(concat!(
        env!("OUT_DIR"),
        "/protobuf_generated/unittest_import/rust/test/generated.rs"
    ));
}

pub mod unittest_rust_proto {
    include!(concat!(env!("OUT_DIR"), "/protobuf_generated/unittest/rust/test/generated.rs"));
}

pub mod map_unittest_rust_proto {
    include!(concat!(env!("OUT_DIR"), "/protobuf_generated/map_unittest/rust/test/generated.rs"));
}
