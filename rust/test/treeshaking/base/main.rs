// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use treeshaking_rust_proto::UsedMessage;

#[cfg_attr(feature = "ios_app", unsafe(no_mangle))]
pub fn main() {
    let mut msg = UsedMessage::new();
    msg.set_id(42);
    println!("Used ID: {}", msg.id());
}
