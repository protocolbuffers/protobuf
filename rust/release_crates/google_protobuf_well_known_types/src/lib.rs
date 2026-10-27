// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use std::path::Path;

mod generated;
pub use generated::*;

pub fn get_dependency(crate_name: &str) -> Vec<protobuf_codegen::Dependency> {
    vec![protobuf_codegen::Dependency {
        crate_name: crate_name.to_string(),
        proto_import_paths: vec![Path::new(env!("CARGO_MANIFEST_DIR")).join("proto")],
        proto_files: vec![
            "google/protobuf/any.proto".to_string(),
            "google/protobuf/api.proto".to_string(),
            "google/protobuf/duration.proto".to_string(),
            "google/protobuf/empty.proto".to_string(),
            "google/protobuf/field_mask.proto".to_string(),
            "google/protobuf/source_context.proto".to_string(),
            "google/protobuf/struct.proto".to_string(),
            "google/protobuf/timestamp.proto".to_string(),
            "google/protobuf/type.proto".to_string(),
            "google/protobuf/wrappers.proto".to_string(),
        ],
    }]
}
