// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use protobuf_codegen::{CodeGen, Dependency};

use std::path::Path;
use std::path::PathBuf;

fn main() {
    CodeGen::new()
        .inputs(["rust/test/unittest_import.proto"])
        .output_dir(
            PathBuf::from(std::env::var("OUT_DIR").unwrap())
                .join("protobuf_generated/unittest_import"),
        )
        .include("proto")
        .generate_and_compile()
        .unwrap();

    CodeGen::new()
        .inputs(["rust/test/unittest.proto"])
        .dependency(vec![Dependency {
            crate_name: "crate::protos::unittest_import_rust_proto".to_string(),
            proto_import_paths: vec![Path::new(env!("CARGO_MANIFEST_DIR")).join("proto")],
            proto_files: vec!["rust/test/unittest_import.proto".to_string()],
        }])
        .output_dir(
            PathBuf::from(std::env::var("OUT_DIR").unwrap()).join("protobuf_generated/unittest"),
        )
        .include("proto")
        .generate_and_compile()
        .unwrap();

    CodeGen::new()
        .inputs(["rust/test/map_unittest.proto"])
        .dependency(vec![Dependency {
            crate_name: "crate::protos::unittest_rust_proto".to_string(),
            proto_import_paths: vec![Path::new(env!("CARGO_MANIFEST_DIR")).join("proto")],
            proto_files: vec!["rust/test/unittest.proto".to_string()],
        }])
        .output_dir(
            PathBuf::from(std::env::var("OUT_DIR").unwrap())
                .join("protobuf_generated/map_unittest"),
        )
        .include("proto")
        .generate_and_compile()
        .unwrap();
}
