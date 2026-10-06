// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#[cfg(test)]
mod tests {
    use googletest::prelude::*;
    use map_unittest_rust_proto::TestMap;
    use protobuf::prelude::*;
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
    fn test_print_to_text_format_single_line() {
        let mut msg = TestAllTypes::new();
        msg.set_optional_int32(42);
        msg.set_optional_string("super secret");

        let text = protobuf::text_format::print_with_options(
            &msg,
            &protobuf::text_format::PrintOptions::new().single_line(true),
        );
        expect_eq!(text, "optional_int32: 42 optional_string: \"super secret\" ");
    }

    #[gtest]
    fn test_print_to_text_format_hide_unknown_fields() {
        // Wire format for field 999 (varint) = 1, which `TestAllTypes` does not define.
        let mut msg = TestAllTypes::parse(&[0xB8, 0x3E, 0x01]).unwrap();
        msg.set_optional_int32(42);

        expect_eq!(protobuf::text_format::print(&msg), "optional_int32: 42\n999: 1\n");

        let text = protobuf::text_format::print_with_options(
            &msg,
            &protobuf::text_format::PrintOptions::new().hide_unknown_fields(true),
        );
        expect_eq!(text, "optional_int32: 42\n");
    }

    #[gtest]
    fn test_print_to_text_format_no_sort_maps() {
        let mut msg = TestMap::new();
        msg.map_int32_int32_mut().insert(8, 80);
        msg.map_int32_int32_mut().insert(1, 10);

        // By default, map entries are sorted by key.
        expect_eq!(
            protobuf::text_format::print(&msg),
            "map_int32_int32 {\n  key: 1\n  value: 10\n}\nmap_int32_int32 {\n  key: 8\n  value: 80\n}\n"
        );

        // With `no_sort_maps`, entries are printed in the map's iteration order instead.
        let text = protobuf::text_format::print_with_options(
            &msg,
            &protobuf::text_format::PrintOptions::new().no_sort_maps(true),
        );
        let expected: String = msg
            .map_int32_int32()
            .iter()
            .map(|(k, v)| format!("map_int32_int32 {{\n  key: {k}\n  value: {v}\n}}\n"))
            .collect();
        expect_eq!(text, expected);
    }
}
