// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use googletest::gtest;
use std::env;

const COMPILED_RUSTC_VERSION: &str = env!("COMPILED_RUSTC_VERSION");

#[gtest]
fn test_rust_version() {
    let Ok(exp_version) = env::var("KOKORO_RUST_VERSION") else {
        eprintln!(
            "No kokoro rust version found, skipping check (compiled with: {COMPILED_RUSTC_VERSION})"
        );
        return;
    };
    if exp_version.is_empty() {
        eprintln!(
            "No kokoro rust version found, skipping check (compiled with: {COMPILED_RUSTC_VERSION})"
        );
        return;
    }

    // `rustc --version` outputs e.g. `rustc 1.92.0 (ded5c06cf 2025-12-08)`.
    let actual_version = COMPILED_RUSTC_VERSION
        .strip_prefix("rustc ")
        .unwrap_or(COMPILED_RUSTC_VERSION)
        .split_whitespace()
        .next()
        .unwrap_or("");

    assert!(
        actual_version == exp_version
            || actual_version
                .strip_prefix(&exp_version)
                .is_some_and(|rest| rest.starts_with('.')),
        "Rust compile-time version mismatch: expected KOKORO_RUST_VERSION={exp_version:?}, got {actual_version:?} ({COMPILED_RUSTC_VERSION})"
    );
}
