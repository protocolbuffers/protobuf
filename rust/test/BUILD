# Protocol Buffers - Google's data interchange format
# Copyright 2023 Google LLC.  All rights reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("//bazel:proto_library.bzl", "proto_library")
load(
    "//rust:defs.bzl",
    "rust_cc_proto_library",
    "rust_upb_proto_library",
)

package(
    default_visibility = ["//rust/test:__subpackages__"],
)

proto_library(
    name = "parent_proto",
    srcs = ["parent.proto"],
)

proto_library(
    name = "child_proto",
    testonly = True,
    srcs = ["child.proto"],
    exports = [":parent_proto"],
    deps = [":parent_proto"],
)

rust_upb_proto_library(
    name = "parent_upb_rust_proto",
    testonly = True,
    deps = [":parent_proto"],
)

rust_upb_proto_library(
    name = "child_upb_rust_proto",
    testonly = True,
    deps = [":child_proto"],
)

rust_cc_proto_library(
    name = "parent_cpp_rust_proto",
    testonly = True,
    deps = [":parent_proto"],
)

rust_cc_proto_library(
    name = "child_cpp_rust_proto",
    testonly = True,
    deps = [":child_proto"],
)

proto_library(
    name = "dots_in_package_proto",
    testonly = True,
    srcs = ["dots_in_package.proto"],
)

rust_cc_proto_library(
    name = "dots_in_package_cpp_rust_proto",
    testonly = True,
    deps = [":dots_in_package_proto"],
)

rust_upb_proto_library(
    name = "dots_in_package_upb_rust_proto",
    testonly = True,
    deps = [":dots_in_package_proto"],
)

proto_library(
    name = "edition2023_proto",
    testonly = True,
    srcs = ["edition2023.proto"],
    deps = ["//src/google/protobuf:cpp_features_proto"],
)

rust_cc_proto_library(
    name = "edition2023_cpp_rust_proto",
    testonly = True,
    deps = [":edition2023_proto"],
)

rust_upb_proto_library(
    name = "edition2023_upb_rust_proto",
    testonly = True,
    deps = [":edition2023_proto"],
)

proto_library(
    name = "enums_proto",
    testonly = True,
    srcs = ["enums.proto"],
)

rust_cc_proto_library(
    name = "enums_cpp_rust_proto",
    testonly = True,
    deps = [":enums_proto"],
)

rust_upb_proto_library(
    name = "enums_upb_rust_proto",
    testonly = True,
    deps = [":enums_proto"],
)

proto_library(
    name = "import_public_grandparent_proto",
    testonly = True,
    srcs = [":import_public_grandparent.proto"],
)

proto_library(
    name = "import_public_primary_src_proto",
    testonly = True,
    srcs = ["import_public_primary_src.proto"],
    exports = [":import_public_grandparent_proto"],
    deps = [":import_public_grandparent_proto"],
)

proto_library(
    name = "import_public_non_primary_src_proto",
    testonly = True,
    srcs = [
        "import_public_non_primary_src1.proto",
        "import_public_non_primary_src2.proto",
    ],
)

proto_library(
    name = "import_public_proto",
    testonly = True,
    srcs = [
        "import_public.proto",
        "import_public2.proto",
    ],
    exports = [
        ":import_public_non_primary_src_proto",
        ":import_public_primary_src_proto",
    ],
    deps = [
        ":import_public_non_primary_src_proto",
        ":import_public_primary_src_proto",
    ],
)

rust_cc_proto_library(
    name = "import_public_cpp_rust_proto",
    testonly = True,
    deps = [":import_public_proto"],
)

rust_upb_proto_library(
    name = "import_public_upb_rust_proto",
    testonly = True,
    deps = [":import_public_proto"],
)

proto_library(
    name = "no_package_import_proto",
    testonly = True,
    srcs = ["no_package_import.proto"],
    deps = [
    ],
)

proto_library(
    name = "no_package_proto",
    testonly = True,
    srcs = [
        "no_package.proto",
        "no_package_other.proto",
    ],
    exports = [":no_package_import_proto"],
    deps = [
        ":no_package_import_proto",
    ],
)

rust_cc_proto_library(
    name = "no_package_cpp_rust_proto",
    testonly = True,
    deps = [":no_package_proto"],
)

rust_upb_proto_library(
    name = "no_package_upb_rust_proto",
    testonly = True,
    deps = [":no_package_proto"],
)

proto_library(
    name = "package_import_proto",
    testonly = True,
    srcs = ["package_import.proto"],
)

proto_library(
    name = "package_proto",
    testonly = True,
    srcs = [
        "package.proto",
        "package_other.proto",
        "package_other_different.proto",
    ],
    exports = [":package_import_proto"],
    deps = [":package_import_proto"],
)

rust_cc_proto_library(
    name = "package_cpp_rust_proto",
    testonly = True,
    deps = [":package_proto"],
)

rust_upb_proto_library(
    name = "package_upb_rust_proto",
    testonly = True,
    deps = [":package_proto"],
)

proto_library(
    name = "package_disabiguation_proto",
    testonly = True,
    srcs = [
        "package_disabiguation1.proto",
        # TODO: b/321220129 - Uncomment once we support ambiguous messages.
        # "package_disabiguation2.proto",
    ],
)

rust_upb_proto_library(
    name = "package_disabiguation_upb_rust_proto",
    testonly = True,
    deps = [":package_disabiguation_proto"],
)

rust_cc_proto_library(
    name = "package_disabiguation_cpp_rust_proto",
    testonly = True,
    deps = [":package_disabiguation_proto"],
)

proto_library(
    name = "bad_names_proto",
    testonly = True,
    srcs = ["bad_names.proto"],
)

rust_cc_proto_library(
    name = "bad_names_cpp_rust_proto",
    testonly = True,
    deps = [":bad_names_proto"],
)

rust_upb_proto_library(
    name = "bad_names_upb_rust_proto",
    testonly = True,
    deps = [":bad_names_proto"],
)

proto_library(
    name = "nested_proto",
    testonly = True,
    srcs = ["nested.proto"],
)

rust_cc_proto_library(
    name = "nested_cpp_rust_proto",
    testonly = True,
    deps = [":nested_proto"],
)

rust_upb_proto_library(
    name = "nested_upb_rust_proto",
    testonly = True,
    deps = [":nested_proto"],
)

proto_library(
    name = "map_unittest_proto",
    srcs = [
        "map_unittest.proto",
    ],
    deps = [
        ":unittest_proto",
    ],
)

rust_cc_proto_library(
    name = "map_unittest_cpp_rust_proto",
    testonly = True,
    deps = [":map_unittest_proto"],
)

rust_upb_proto_library(
    name = "map_unittest_upb_rust_proto",
    testonly = True,
    deps = [":map_unittest_proto"],
)

proto_library(
    name = "imported_types_proto",
    testonly = True,
    srcs = ["imported_types.proto"],
)

rust_cc_proto_library(
    name = "imported_types_cpp_rust_proto",
    testonly = True,
    deps = [":imported_types_proto"],
)

rust_upb_proto_library(
    name = "imported_types_upb_rust_proto",
    testonly = True,
    deps = [":imported_types_proto"],
)

proto_library(
    # Test '-' occurrences are replaced with '_'.
    name = "fields-with-imported-types_proto",
    testonly = True,
    srcs = ["fields_with_imported_types.proto"],
    deps = [":imported_types_proto"],
)

rust_cc_proto_library(
    name = "fields_with_imported_types_cpp_rust_proto",
    testonly = True,
    deps = [":fields-with-imported-types_proto"],
)

rust_upb_proto_library(
    name = "fields_with_imported_types_upb_rust_proto",
    testonly = True,
    deps = [":fields-with-imported-types_proto"],
)

proto_library(
    name = "unittest_proto",
    srcs = ["unittest.proto"],
    deps = [":unittest_import_proto"],
)

rust_upb_proto_library(
    name = "unittest_upb_rust_proto",
    testonly = True,
    deps = [":unittest_proto"],
)

rust_cc_proto_library(
    name = "unittest_cpp_rust_proto",
    testonly = True,
    deps = ["unittest_proto"],
)

proto_library(
    name = "unittest_import_proto",
    srcs = ["unittest_import.proto"],
)

rust_upb_proto_library(
    name = "unittest_import_upb_rust_proto",
    testonly = True,
    deps = [":unittest_import_proto"],
)

rust_cc_proto_library(
    name = "unittest_import_cpp_rust_proto",
    testonly = True,
    deps = ["unittest_import_proto"],
)

proto_library(
    name = "unittest_proto3_proto",
    srcs = ["unittest_proto3.proto"],
    deps = [":unittest_import_proto"],
)

rust_upb_proto_library(
    name = "unittest_proto3_upb_rust_proto",
    testonly = True,
    deps = [":unittest_proto3_proto"],
)

rust_cc_proto_library(
    name = "unittest_proto3_cpp_rust_proto",
    testonly = True,
    deps = ["unittest_proto3_proto"],
)

proto_library(
    name = "unittest_proto3_optional_proto",
    srcs = ["unittest_proto3_optional.proto"],
)

rust_upb_proto_library(
    name = "unittest_proto3_optional_upb_rust_proto",
    testonly = True,
    deps = [":unittest_proto3_optional_proto"],
)

rust_cc_proto_library(
    name = "unittest_proto3_optional_cpp_rust_proto",
    testonly = True,
    deps = ["unittest_proto3_optional_proto"],
)

proto_library(
    name = "srcsless_library_test_child_proto",
    testonly = True,
    srcs = ["srcsless_library_test_child.proto"],
)

proto_library(
    name = "srcsless_library_test_alias_proto",
    testonly = True,
    deps = [":srcsless_library_test_child_proto"],
)

proto_library(
    name = "srcsless_library_test_parent_proto",
    testonly = True,
    srcs = ["srcsless_library_test_parent.proto"],
    deps = [":srcsless_library_test_alias_proto"],
)

rust_cc_proto_library(
    name = "srcsless_library_test_parent_cpp_rust_proto",
    testonly = True,
    deps = [":srcsless_library_test_parent_proto"],
)

rust_upb_proto_library(
    name = "srcsless_library_test_parent_upb_rust_proto",
    testonly = True,
    deps = [":srcsless_library_test_parent_proto"],
)
