use std::path::Path;

include!(concat!(env!("OUT_DIR"), "/protobuf_generated/google/protobuf/generated.rs"));

pub fn get_dependency(crate_name: &str) -> Vec<protobuf_codegen::Dependency> {
    vec![protobuf_codegen::Dependency {
        crate_name: crate_name.to_string(),
        proto_import_paths: vec![Path::new(env!("CARGO_MANIFEST_DIR")).join("proto")],
        c_include_paths: vec![Path::new(env!("OUT_DIR")).join("protobuf_generated")],
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
