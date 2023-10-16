// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use nested_proto::nest::Outer;

#[test]
fn test_simple_nested_proto() {
    let outer_msg = Outer::new();
    assert_eq!(outer_msg.inner().num(), 0);
    assert!(!outer_msg.inner().boolean());
}
