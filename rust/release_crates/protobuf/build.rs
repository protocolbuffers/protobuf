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

    let mut cmd = std::process::Command::new("protoc");
    cmd.arg(format!("--rust_out={}", std::env::var("OUT_DIR").unwrap()))
        .arg("--rust_opt=experimental-codegen=enabled,kernel=upb,build_system=cargo")
        .arg("-Iproto");
    for f in well_known_types {
        cmd.arg(f);
    }
    let output = cmd.output().map_err(|e| format!("failed to run protoc: {}", e)).unwrap();
    println!("{}", std::str::from_utf8(&output.stdout).unwrap());
    eprintln!("{}", std::str::from_utf8(&output.stderr).unwrap());
    assert!(output.status.success());

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
