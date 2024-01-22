// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use googletest::prelude::*;

// Tests that an proto file that declares edition="2023" works. Note that this
// is _not_ a test for Rust Edition 2023 (which doesn't exist) but instead
// Protobuf Edition 2023 (which exists).

#[test]
fn check_edition2023_works() {
    let mut msg = edition2023_proto::EditionsMessage::new();

    // plain_field supports presence.
    assert_that!(msg.plain_field_mut().or_default().get(), eq(0));

    // implicit presence fields' _mut() skips FieldEntry.
    assert_that!(msg.implicit_presence_field_mut().get(), eq(0));
}
