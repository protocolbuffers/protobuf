// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

//! A helper library for the diagnostics negative compilation test.
//! This defines a function in an external crate signature to force
//! `rustc` to resolve and format the type path of `TestAllTypesView` in diagnostics.

extern crate unittest_rust_proto;

pub fn take_view(_x: unittest_rust_proto::TestAllTypesView) {}
