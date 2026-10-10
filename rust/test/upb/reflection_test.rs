// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#[cfg(test)]
mod tests {
    use extensions_rust_proto::TestExtensions;
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

    #[gtest]
    fn test_print_extensions_to_text_format() {
        let mut msg = TestExtensions::new();
        extensions_rust_proto::I32_EXTENSION.set(&mut msg, 42);
        extensions_rust_proto::SUBMESSAGE_EXTENSION.get_mut(&mut msg).set_i32_field(99);
        extensions_rust_proto::test_extensions::NESTED_EXTENSION.set(&mut msg, 500);
        extensions_rust_proto::REPEATED_I32_EXTENSION.get_mut(&mut msg).push(1);
        extensions_rust_proto::REPEATED_I32_EXTENSION.get_mut(&mut msg).push(2);
        extensions_separate_file_rust_proto::I32_EXTENSION_SEPARATE_FILE.set(&mut msg, 84);
        extensions_separate_file_rust_proto::I32_EXTENSION_SEPARATE_FILE2.set(&mut msg, 85);

        let text = protobuf::text_format::print(&msg);
        expect_eq!(
            text,
            concat!(
                "[third_party_protobuf_rust_test.i32_extension]: 42\n",
                "[third_party_protobuf_rust_test.submessage_extension] {\n",
                "  i32_field: 99\n",
                "}\n",
                "[third_party_protobuf_rust_test.TestExtensions.nested_extension]: 500\n",
                "[third_party_protobuf_rust_test.repeated_i32_extension]: 1\n",
                "[third_party_protobuf_rust_test.repeated_i32_extension]: 2\n",
                "[third_party_protobuf_rust_test.i32_extension_separate_file]: 84\n",
                "[third_party_protobuf_rust_test.i32_extension_separate_file2]: 85\n",
            )
        );
    }
}
