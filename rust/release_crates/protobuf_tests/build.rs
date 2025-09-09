// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use protobuf_codegen::{CodeGen, Dependency};

use std::path::Path;
use std::path::PathBuf;

// Given "a/b/c.proto", returns "c".
fn short_name(proto_file_name: &str) -> String {
    proto_file_name.rsplit('/').next().unwrap().strip_suffix(".proto").unwrap().to_string()
}

fn main() {
    // All proto files needed for testing, grouped by protoc invocation.
    let proto_files = vec![
        vec!["rust/test/bad_names.proto".to_string()],
        vec!["rust/test/bad_names.proto".to_string()],
        vec!["rust/test/child.proto".to_string()],
        vec!["google/protobuf/cpp_features.proto".to_string()],
        vec!["google/protobuf/descriptor.proto".to_string()],
        vec!["rust/test/dots_in_package.proto".to_string()],
        vec!["rust/test/edition2023.proto".to_string()],
        vec!["rust/test/enums.proto".to_string()],
        vec!["rust/test/fields_with_imported_types.proto".to_string()],
        vec!["rust/test/imported_types.proto".to_string()],
        vec!["rust/test/import_public_grandparent.proto".to_string()],
        vec!["rust/test/import_public_non_primary_src1.proto".to_string()],
        vec!["rust/test/import_public_non_primary_src2.proto".to_string()],
        vec!["rust/test/import_public_primary_src.proto".to_string()],
        vec![
            "rust/test/import_public.proto".to_string(),
            "rust/test/import_public2.proto".to_string(),
        ],
        vec!["rust/test/map_unittest.proto".to_string()],
        vec!["rust/test/nested.proto".to_string()],
        vec!["rust/test/no_package_import.proto".to_string()],
        vec![
            "rust/test/no_package.proto".to_string(),
            "rust/test/no_package_other.proto".to_string(),
        ],
        vec!["rust/test/package_disabiguation1.proto".to_string()],
        vec!["rust/test/package_disabiguation2.proto".to_string()],
        vec!["rust/test/package_import.proto".to_string()],
        vec![
            "rust/test/package.proto".to_string(),
            "rust/test/package_other.proto".to_string(),
            "rust/test/package_other_different.proto".to_string(),
        ],
        vec!["rust/test/parent.proto".to_string()],
        vec!["rust/test/srcsless_library_test_child.proto".to_string()],
        vec!["rust/test/srcsless_library_test_parent.proto".to_string()],
        vec!["rust/test/unittest_import.proto".to_string()],
        vec!["rust/test/unittest.proto".to_string()],
        vec!["rust/test/unittest_proto3_optional.proto".to_string()],
        vec!["rust/test/unittest_proto3.proto".to_string()],
        vec!["rust/test/bad_names.proto".to_string()],
        vec!["rust/test/child.proto".to_string()],
        vec!["google/protobuf/cpp_features.proto".to_string()],
        vec!["google/protobuf/descriptor.proto".to_string()],
        vec!["rust/test/dots_in_package.proto".to_string()],
        vec!["rust/test/edition2023.proto".to_string()],
        vec!["rust/test/enums.proto".to_string()],
        vec!["rust/test/fields_with_imported_types.proto".to_string()],
        vec!["rust/test/imported_types.proto".to_string()],
        vec!["rust/test/import_public_grandparent.proto".to_string()],
        vec!["rust/test/import_public_non_primary_src1.proto".to_string()],
        vec!["rust/test/import_public_non_primary_src2.proto".to_string()],
        vec!["rust/test/import_public_primary_src.proto".to_string()],
        vec![
            "rust/test/import_public.proto".to_string(),
            "rust/test/import_public2.proto".to_string(),
        ],
        vec!["rust/test/map_unittest.proto".to_string()],
        vec!["rust/test/nested.proto".to_string()],
        vec!["rust/test/no_package_import.proto".to_string()],
        vec![
            "rust/test/no_package.proto".to_string(),
            "rust/test/no_package_other.proto".to_string(),
        ],
        vec!["rust/test/package_disabiguation1.proto".to_string()],
        vec!["rust/test/package_disabiguation2.proto".to_string()],
        vec!["rust/test/package_import.proto".to_string()],
        vec![
            "rust/test/package.proto".to_string(),
            "rust/test/package_other.proto".to_string(),
            "rust/test/package_other_different.proto".to_string(),
        ],
        vec!["rust/test/parent.proto".to_string()],
        vec!["rust/test/srcsless_library_test_child.proto".to_string()],
        vec!["rust/test/srcsless_library_test_parent.proto".to_string()],
        vec!["rust/test/unittest_import.proto".to_string()],
        vec!["rust/test/unittest.proto".to_string()],
        vec!["rust/test/unittest_proto3_optional.proto".to_string()],
        vec!["rust/test/unittest_proto3.proto".to_string()],
    ];

    let mut deps: Vec<Dependency> = vec![];
    for target in &proto_files {
        let name = short_name(&target[0]);
        deps.push(Dependency {
            crate_name: format!("crate::protos::{name}_rust_proto"),
            proto_import_paths: vec![Path::new(env!("CARGO_MANIFEST_DIR")).join("proto")],
            proto_files: target.to_vec(),
        });
    }

    for target in proto_files {
        let name = short_name(&target[0]);
        CodeGen::new()
            .inputs(target)
            .dependency(deps.clone())
            .output_dir(
                PathBuf::from(std::env::var("OUT_DIR").unwrap())
                    .join(format!("protobuf_generated/{name}")),
            )
            .include("proto")
            .generate_and_compile()
            .unwrap();
    }
}
