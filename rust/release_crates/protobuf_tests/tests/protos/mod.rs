// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

macro_rules! protobuf_module {
    ($m:ident, $n:literal, $d:literal) => {
        pub mod $m {
            include!(concat!(
                env!("OUT_DIR"),
                "/protobuf_generated/",
                $n,
                "/",
                $d,
                "/generated.rs"
            ));
        }
    };
}

protobuf_module!(bad_names_rust_proto, "bad_names", "rust/test");
protobuf_module!(child_rust_proto, "child", "rust/test");
protobuf_module!(cpp_features_rust_proto, "cpp_features", "google/protobuf");
protobuf_module!(descriptor_rust_proto, "descriptor", "google/protobuf");
protobuf_module!(dots_in_package_rust_proto, "dots_in_package", "rust/test");
protobuf_module!(edition2023_rust_proto, "edition2023", "rust/test");
protobuf_module!(enums_rust_proto, "enums", "rust/test");
protobuf_module!(fields_with_imported_types_rust_proto, "fields_with_imported_types", "rust/test");
protobuf_module!(imported_types_rust_proto, "imported_types", "rust/test");
protobuf_module!(import_public_grandparent_rust_proto, "import_public_grandparent", "rust/test");
protobuf_module!(
    import_public_non_primary_src1_rust_proto,
    "import_public_non_primary_src1",
    "rust/test"
);
protobuf_module!(
    import_public_non_primary_src2_rust_proto,
    "import_public_non_primary_src2",
    "rust/test"
);
protobuf_module!(import_public_primary_src_rust_proto, "import_public_primary_src", "rust/test");
protobuf_module!(import_public_rust_proto, "import_public", "rust/test");
protobuf_module!(map_unittest_rust_proto, "map_unittest", "rust/test");
protobuf_module!(nested_rust_proto, "nested", "rust/test");
protobuf_module!(no_package_import_rust_proto, "no_package_import", "rust/test");
protobuf_module!(no_package_rust_proto, "no_package", "rust/test");
protobuf_module!(package_disabiguation1_rust_proto, "package_disabiguation1", "rust/test");
protobuf_module!(package_disabiguation2_rust_proto, "package_disabiguation2", "rust/test");
protobuf_module!(package_import_rust_proto, "package_import", "rust/test");
protobuf_module!(package_rust_proto, "package", "rust/test");
protobuf_module!(parent_rust_proto, "parent", "rust/test");
protobuf_module!(
    srcsless_library_test_child_rust_proto,
    "srcsless_library_test_child",
    "rust/test"
);
protobuf_module!(
    srcsless_library_test_parent_rust_proto,
    "srcsless_library_test_parent",
    "rust/test"
);
protobuf_module!(unittest_import_rust_proto, "unittest_import", "rust/test");
protobuf_module!(unittest_rust_proto, "unittest", "rust/test");
protobuf_module!(unittest_proto3_optional_rust_proto, "unittest_proto3_optional", "rust/test");
protobuf_module!(unittest_proto3_rust_proto, "unittest_proto3", "rust/test");
