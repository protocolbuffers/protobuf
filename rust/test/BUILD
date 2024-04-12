load("//bazel:cc_proto_library.bzl", "cc_proto_library")
load(
    "//rust:defs.bzl",
    "rust_cc_proto_library",
    "rust_proto_library",
    "rust_upb_proto_library",
)

UNITTEST_PROTO_TARGET = "//src/google/protobuf:test_protos"

UNITTEST_PROTO3_TARGET = "//src/google/protobuf:test_protos"

UNITTEST_PROTO3_OPTIONAL_TARGET = "//src/google/protobuf:test_protos"

rust_proto_library(
    name = "unittest_rust_proto",
    testonly = True,
    visibility = ["//visibility:private"],
    deps = [UNITTEST_PROTO_TARGET],
)

rust_upb_proto_library(
    name = "unittest_upb_rust_proto",
    testonly = True,
    visibility = ["//rust/test/shared:__subpackages__"],
    deps = [UNITTEST_PROTO_TARGET],
)

rust_cc_proto_library(
    name = "unittest_cc_rust_proto",
    testonly = True,
    visibility = [
        "//rust/test/cpp:__subpackages__",
        "//rust/test/shared:__subpackages__",
    ],
    deps = [UNITTEST_PROTO_TARGET],
)

rust_proto_library(
    name = "unittest_proto3_rust_proto",
    testonly = True,
    visibility = ["//visibility:private"],
    deps = [
        UNITTEST_PROTO3_TARGET,
    ],
)

rust_cc_proto_library(
    name = "unittest_proto3_cc_rust_proto",
    testonly = True,
    visibility = ["//rust/test/shared:__subpackages__"],
    deps = [UNITTEST_PROTO3_TARGET],
)

rust_upb_proto_library(
    name = "unittest_proto3_upb_rust_proto",
    testonly = True,
    visibility = ["//rust/test/shared:__subpackages__"],
    deps = [UNITTEST_PROTO3_TARGET],
)

rust_proto_library(
    name = "unittest_proto3_optional_rust_proto",
    testonly = True,
    visibility = ["//visibility:private"],
    deps = [
        UNITTEST_PROTO3_OPTIONAL_TARGET,
    ],
)

rust_cc_proto_library(
    name = "unittest_proto3_optional_cc_rust_proto",
    testonly = True,
    visibility = ["//rust/test/shared:__subpackages__"],
    deps = [UNITTEST_PROTO3_OPTIONAL_TARGET],
)

rust_upb_proto_library(
    name = "unittest_proto3_optional_upb_rust_proto",
    testonly = True,
    visibility = ["//rust/test/shared:__subpackages__"],
    deps = [UNITTEST_PROTO3_OPTIONAL_TARGET],
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
    visibility = ["//rust/test/shared:__subpackages__"],
    deps = [":parent_proto"],
)

rust_upb_proto_library(
    name = "child_upb_rust_proto",
    testonly = True,
    visibility = ["//rust/test/shared:__subpackages__"],
    deps = [":child_proto"],
)

rust_cc_proto_library(
    name = "parent_cc_rust_proto",
    testonly = True,
    visibility = ["//rust/test/shared:__subpackages__"],
    deps = [":parent_proto"],
)

rust_cc_proto_library(
    name = "child_cc_rust_proto",
    testonly = True,
    visibility = ["//rust/test/shared:__subpackages__"],
    deps = [":child_proto"],
)

proto_library(
    name = "dots_in_package_proto",
    testonly = True,
    srcs = ["dots_in_package.proto"],
)

rust_cc_proto_library(
    name = "dots_in_package_cc_rust_proto",
    testonly = True,
    visibility = ["//rust/test/shared:__subpackages__"],
    deps = [":dots_in_package_proto"],
)

rust_upb_proto_library(
    name = "dots_in_package_upb_rust_proto",
    testonly = True,
    visibility = ["//rust/test/shared:__subpackages__"],
    deps = [":dots_in_package_proto"],
)

proto_library(
    name = "edition2023_proto",
    testonly = True,
    srcs = ["edition2023.proto"],
)

rust_cc_proto_library(
    name = "edition2023_cc_rust_proto",
    testonly = True,
    visibility = ["//rust/test/shared:__subpackages__"],
    deps = [":edition2023_proto"],
)

rust_upb_proto_library(
    name = "edition2023_upb_rust_proto",
    testonly = True,
    visibility = ["//rust/test/shared:__subpackages__"],
    deps = [":edition2023_proto"],
)

proto_library(
    name = "enums_proto",
    testonly = True,
    srcs = ["enums.proto"],
    deps = ["//devtools/staticanalysis/pipeline/analyzers/proto_best_practices/proto:optouts_proto"],
)

rust_cc_proto_library(
    name = "enums_cc_rust_proto",
    testonly = True,
    visibility = ["//rust/test/shared:__subpackages__"],
    deps = [":enums_proto"],
)

rust_upb_proto_library(
    name = "enums_upb_rust_proto",
    testonly = True,
    visibility = ["//rust/test/shared:__subpackages__"],
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

cc_proto_library(
    name = "import_public_cc_proto",
    testonly = True,
    deps = [":import_public_proto"],
)

rust_cc_proto_library(
    name = "import_public_cc_rust_proto",
    testonly = True,
    visibility = ["//rust/test/shared:__subpackages__"],
    deps = [":import_public_cc_proto"],
)

rust_upb_proto_library(
    name = "import_public_upb_rust_proto",
    testonly = True,
    visibility = ["//rust/test/shared:__subpackages__"],
    deps = [":import_public_proto"],
)

proto_library(
    name = "no_package_import_proto",
    testonly = True,
    srcs = ["no_package_import.proto"],
    deps = ["//devtools/staticanalysis/pipeline/analyzers/proto_best_practices/proto:optouts_proto"],
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
        "//devtools/staticanalysis/pipeline/analyzers/proto_best_practices/proto:optouts_proto",
    ],
)

rust_cc_proto_library(
    name = "no_package_cc_rust_proto",
    testonly = True,
    visibility = ["//rust/test/shared:__subpackages__"],
    deps = [":no_package_proto"],
)

rust_upb_proto_library(
    name = "no_package_upb_rust_proto",
    testonly = True,
    visibility = ["//rust/test/shared:__subpackages__"],
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
    name = "package_cc_rust_proto",
    testonly = True,
    visibility = ["//rust/test/shared:__subpackages__"],
    deps = [":package_proto"],
)

rust_upb_proto_library(
    name = "package_upb_rust_proto",
    testonly = True,
    visibility = ["//rust/test/shared:__subpackages__"],
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
    visibility = ["//rust/test/shared:__subpackages__"],
    deps = [":package_disabiguation_proto"],
)

cc_proto_library(
    name = "package_disabiguation_cc_proto",
    testonly = True,
    deps = [":package_disabiguation_proto"],
)

rust_cc_proto_library(
    name = "package_disabiguation_cc_rust_proto",
    testonly = True,
    visibility = ["//rust/test/shared:__subpackages__"],
    deps = [":package_disabiguation_cc_proto"],
)

proto_library(
    name = "reserved_proto",
    testonly = True,
    srcs = ["reserved.proto"],
)

rust_cc_proto_library(
    name = "reserved_cc_rust_proto",
    testonly = True,
    visibility = ["//rust/test/shared:__subpackages__"],
    deps = [":reserved_proto"],
)

rust_upb_proto_library(
    name = "reserved_upb_rust_proto",
    testonly = True,
    visibility = ["//rust/test/shared:__subpackages__"],
    deps = [":reserved_proto"],
)

proto_library(
    name = "nested_proto",
    testonly = True,
    srcs = ["nested.proto"],
)

rust_cc_proto_library(
    name = "nested_cc_rust_proto",
    testonly = True,
    visibility = [
        "//rust/test/cpp:__subpackages__",
        "//rust/test/shared:__subpackages__",
    ],
    deps = [":nested_proto"],
)

rust_upb_proto_library(
    name = "nested_upb_rust_proto",
    testonly = True,
    visibility = [
        "//rust/test/shared:__subpackages__",
        "//rust/test/upb:__subpackages__",
    ],
    deps = [":nested_proto"],
)

rust_cc_proto_library(
    name = "map_unittest_cc_rust_proto",
    testonly = True,
    visibility = [
        "//rust/test/shared:__subpackages__",
    ],
    deps = ["//src/google/protobuf:map_unittest_proto"],
)

rust_upb_proto_library(
    name = "map_unittest_upb_rust_proto",
    testonly = True,
    visibility = [
        "//rust/test/shared:__subpackages__",
    ],
    deps = ["//src/google/protobuf:map_unittest_proto"],
)

proto_library(
    name = "struct",
    testonly = True,
    srcs = ["struct.proto"],
)

cc_proto_library(
    name = "struct_cc_proto",
    testonly = True,
    deps = [":struct"],
)

rust_cc_proto_library(
    name = "struct_cc_rust_proto",
    testonly = True,
    visibility = [
        "//rust/test/shared:__subpackages__",
    ],
    deps = [":struct_cc_proto"],
)

rust_upb_proto_library(
    name = "struct_upb_rust_proto",
    testonly = True,
    visibility = [
        "//rust/test/shared:__subpackages__",
    ],
    deps = [":struct"],
)

proto_library(
    name = "imported_types_proto",
    testonly = True,
    srcs = ["imported_types.proto"],
)

cc_proto_library(
    name = "imported_types_cc_proto",
    testonly = True,
    deps = [":imported_types_proto"],
)

rust_cc_proto_library(
    name = "imported_types_cc_rust_proto",
    testonly = True,
    visibility = [
        "//rust/test/shared:__subpackages__",
    ],
    deps = [":imported_types_cc_proto"],
)

rust_upb_proto_library(
    name = "imported_types_upb_rust_proto",
    testonly = True,
    visibility = [
        "//rust/test/shared:__subpackages__",
    ],
    deps = [":imported_types_proto"],
)

proto_library(
    # Test '-' occurrences are replaced with '_'.
    name = "fields-with-imported-types_proto",
    testonly = True,
    srcs = ["fields_with_imported_types.proto"],
    deps = [":imported_types_proto"],
)

cc_proto_library(
    name = "fields_with_imported_types_cc_proto",
    testonly = True,
    deps = [":fields-with-imported-types_proto"],
)

rust_cc_proto_library(
    name = "fields_with_imported_types_cc_rust_proto",
    testonly = True,
    visibility = [
        "//rust/test/shared:__subpackages__",
    ],
    deps = [":fields_with_imported_types_cc_proto"],
)

rust_upb_proto_library(
    name = "fields_with_imported_types_upb_rust_proto",
    testonly = True,
    visibility = [
        "//rust/test/shared:__subpackages__",
    ],
    deps = [":fields-with-imported-types_proto"],
)
