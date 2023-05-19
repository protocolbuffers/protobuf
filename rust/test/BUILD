load("@rules_cc//cc:defs.bzl", "cc_proto_library")
load(
    "//rust:defs.bzl",
    "rust_cc_proto_library",
    "rust_proto_library",
    "rust_upb_proto_library",
)

UNITTEST_PROTO_TARGET = (
    "//src/google/protobuf:test_protos"
)

UNITTEST_CC_PROTO_TARGET = (
    "//src/google/protobuf:cc_test_protos"
)

alias(
    name = "unittest_cc_proto",
    actual = UNITTEST_CC_PROTO_TARGET,
    visibility = ["//rust/test:__subpackages__"],
)

rust_proto_library(
    name = "unittest_rust_proto",
    testonly = True,
    visibility = ["//rust/test/shared:__subpackages__"],
    deps = [UNITTEST_PROTO_TARGET],
)

rust_upb_proto_library(
    name = "unittest_upb_rust_proto",
    testonly = True,
    visibility = [
        "//rust/test/shared:__subpackages__",
        "//rust/test/upb:__subpackages__",
    ],
    deps = [UNITTEST_PROTO_TARGET],
)

rust_cc_proto_library(
    name = "unittest_cc_rust_proto",
    testonly = True,
    visibility = [
        "//rust/test/cpp:__subpackages__",
        "//rust/test/shared:__subpackages__",
    ],
    deps = [UNITTEST_CC_PROTO_TARGET],
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
    visibility = [
        "//rust/test/shared:__subpackages__",
        "//rust/test/upb:__subpackages__",
    ],
    deps = [":parent_proto"],
)

rust_upb_proto_library(
    name = "child_upb_rust_proto",
    testonly = True,
    visibility = [
        "//rust/test/shared:__subpackages__",
        "//rust/test/upb:__subpackages__",
    ],
    deps = [":child_proto"],
)

cc_proto_library(
    name = "parent_cc_proto",
    testonly = True,
    deps = [":parent_proto"],
)

cc_proto_library(
    name = "child_cc_proto",
    testonly = True,
    deps = [":child_proto"],
)

rust_cc_proto_library(
    name = "parent_cc_rust_proto",
    testonly = True,
    visibility = [
        "//rust/test/cpp:__subpackages__",
        "//rust/test/shared:__subpackages__",
    ],
    deps = [":parent_cc_proto"],
)

rust_cc_proto_library(
    name = "child_cc_rust_proto",
    testonly = True,
    visibility = [
        "//rust/test/cpp:__subpackages__",
        "//rust/test/shared:__subpackages__",
    ],
    deps = [":child_cc_proto"],
)

proto_library(
    name = "dots_in_package_proto",
    testonly = True,
    srcs = ["dots_in_package.proto"],
)

cc_proto_library(
    name = "dots_in_package_cc_proto",
    testonly = True,
    deps = [":dots_in_package_proto"],
)

rust_cc_proto_library(
    name = "dots_in_package_cc_rust_proto",
    testonly = True,
    visibility = [
        "//rust/test/cpp:__subpackages__",
        "//rust/test/shared:__subpackages__",
    ],
    deps = [":dots_in_package_cc_proto"],
)

rust_upb_proto_library(
    name = "dots_in_package_upb_rust_proto",
    testonly = True,
    visibility = [
        "//rust/test/shared:__subpackages__",
        "//rust/test/upb:__subpackages__",
    ],
    deps = [":dots_in_package_proto"],
)

proto_library(
    name = "no_package_proto",
    testonly = True,
    srcs = ["no_package.proto"],
)

cc_proto_library(
    name = "no_package_cc_proto",
    testonly = True,
    deps = [":no_package_proto"],
)

rust_cc_proto_library(
    name = "no_package_cc_rust_proto",
    testonly = True,
    visibility = [
        "//rust/test/cpp:__subpackages__",
        "//rust/test/shared:__subpackages__",
    ],
    deps = [":no_package_cc_proto"],
)

rust_upb_proto_library(
    name = "no_package_upb_rust_proto",
    testonly = True,
    visibility = [
        "//rust/test/shared:__subpackages__",
        "//rust/test/upb:__subpackages__",
    ],
    deps = [":no_package_proto"],
)
