// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#[cfg(test)]
mod tests {
    use googletest::prelude::*;
    use unittest_rust_proto::TestAllTypes;

    #[gtest]
    fn test_print_to_text_format() {
        let mut msg = TestAllTypes::new();
        msg.set_optional_int32(42);
        msg.set_optional_string("super secret");

        let text = protobuf::text_format::print(&msg);
        expect_eq!(text, "optional_int32: 42\noptional_string: \"super secret\"\n");
    }
}
