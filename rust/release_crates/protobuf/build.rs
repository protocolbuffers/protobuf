// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.

// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use std::path::Path;

const VERSION: &str = env!("CARGO_PKG_VERSION");

fn clean_path(path: &Path) -> String {
    let canonical = path.canonicalize().unwrap();
    let path_str = canonical.to_string_lossy();
    path_str
        .strip_prefix(r"\\?\")
        .unwrap_or(&path_str)
        .to_string()
}

fn main() {
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
    println!("cargo:include={}", clean_path(&path));
    println!("cargo:version={}", VERSION);
}
