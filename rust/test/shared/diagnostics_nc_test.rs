// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

//! A negative compilation test to ensure that compiler diagnostics print clean
//! type paths (e.g. `TestAllTypesView`) rather than internal implementation-detail
//! paths (`internal_do_not_use_...`).

extern crate diagnostics_nc_test_helper;
extern crate unittest_rust_proto; // Direct import to put the crate in scope!

fn main() {
    diagnostics_nc_test_helper::take_view();
}
