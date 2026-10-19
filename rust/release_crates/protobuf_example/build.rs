// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use protobuf_codegen::CodeGen;

fn main() {
    CodeGen::new()
        .inputs(["proto_example/foo.proto", "proto_example/bar/bar.proto"])
        .include("proto")
        .dependency(protobuf_well_known_types::get_dependency("protobuf_well_known_types"))
        .generate_and_compile()
        .unwrap();
}
