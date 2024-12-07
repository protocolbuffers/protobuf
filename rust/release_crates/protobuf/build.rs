// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.

// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

const VERSION: &str = env!("CARGO_PKG_VERSION");

fn main() {
    // Generate code for the well-known types
    let well_known_types = &[
        "google/protobuf/any.proto",
        "google/protobuf/api.proto",
        "google/protobuf/duration.proto",
        "google/protobuf/empty.proto",
        "google/protobuf/field_mask.proto",
        "google/protobuf/source_context.proto",
        "google/protobuf/struct.proto",
        "google/protobuf/timestamp.proto",
        "google/protobuf/type.proto",
        "google/protobuf/wrappers.proto",
    ];
    for f in well_known_types {
        let mut cmd = std::process::Command::new("protoc");
        cmd.arg(format!("--rust_out={}", std::env::var("OUT_DIR").unwrap())).arg("--rust_opt=experimental-codegen=enabled,kernel=upb,bazel_crate_mapping=crate_mapping").arg(f);
        let output = cmd.output().map_err(|e| format!("failed to run protoc: {}", e)).unwrap();
        println!("{}", std::str::from_utf8(&output.stdout).unwrap());
        eprintln!("{}", std::str::from_utf8(&output.stderr).unwrap());
        assert!(output.status.success());
    }

    let generated_files = &[
        "google/protobuf/any.u.pb.rs",
        "google/protobuf/api.u.pb.rs",
        "google/protobuf/duration.u.pb.rs",
        "google/protobuf/empty.u.pb.rs",
        "google/protobuf/field_mask.u.pb.rs",
        "google/protobuf/source_context.u.pb.rs",
        "google/protobuf/struct.u.pb.rs",
        "google/protobuf/timestamp.u.pb.rs",
        "google/protobuf/type.u.pb.rs",
        "google/protobuf/wrappers.u.pb.rs",
    ];
    for f in generated_files {
        let mut cmd = std::process::Command::new("sed");
        cmd.arg("-i");
        cmd.arg("-e");
        cmd.arg("s/::crate/crate/g");
        cmd.arg("-e");
        cmd.arg("s/::protobuf/crate/g");
        cmd.arg(format!("{}/{}", std::env::var("OUT_DIR").unwrap(), f));
        let output = cmd.output().map_err(|e| format!("failed to run sed: {}", e)).unwrap();
        println!("{}", std::str::from_utf8(&output.stdout).unwrap());
        eprintln!("{}", std::str::from_utf8(&output.stderr).unwrap());
        assert!(output.status.success());
    }

    cc::Build::new()
        .flag("-std=c99")
        // TODO: Come up with a way to enable lto
        // .flag("-flto=thin")
        .warnings(false)
        .include("libupb")
        .include("libupb/third_party/utf8_range")
        .file("libupb/upb/upb.c")
        .file("libupb/third_party/utf8_range/utf8_range.c")
        .define("UPB_BUILD_API", Some("1"))
        .compile("libupb");
    let path = std::path::Path::new("libupb");
    println!("cargo:include={}", path.canonicalize().unwrap().display());
    println!("cargo:version={}", VERSION);
}
