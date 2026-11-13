// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use std::env;
use std::process::Command;

fn main() {
    println!("cargo:rerun-if-env-changed=RUSTC");
    let rustc = env::var("RUSTC").expect("RUSTC environment variable must be set");
    let output = Command::new(&rustc)
        .arg("--version")
        .output()
        .unwrap_or_else(|e| panic!("failed to execute `{rustc} --version`: {e}"));
    assert!(output.status.success(), "`{rustc} --version` exited with {}", output.status);
    let version =
        String::from_utf8(output.stdout).expect("`rustc --version` output was not valid UTF-8");
    println!("cargo:rustc-env=COMPILED_RUSTC_VERSION={}", version.trim());
}
