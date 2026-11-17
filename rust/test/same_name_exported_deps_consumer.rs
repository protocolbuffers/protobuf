// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

pub fn f() {
    let a = same_name_exported_deps_rust_proto::Child::new();
    println!("{:?}", &a);
}
