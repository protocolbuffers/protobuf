# This package contains Rust protobuf runtime implementation built on top of the C++ backend.

load("@rules_rust//rust:defs.bzl", "rust_library")

cc_library(
    name = "cpp_api",
    srcs = [
        "compare.cc",
        "debug.cc",
        "map.cc",
        "message.cc",
        "repeated.cc",
        "strings.cc",
    ],
    hdrs = [
        "compare.h",
        "debug.h",
        "rust_alloc_for_cpp_api.h",
        "serialized_data.h",
        "strings.h",
    ],
    visibility = [
        "//rust:__subpackages__",
        "//src/google/protobuf:__subpackages__",
    ],
    deps = [
        ":rust_alloc_for_cpp_api",  # buildcleaner: keep
        "//src/google/protobuf",
        "//src/google/protobuf:protobuf_lite",
        "//src/google/protobuf/io",
        "@abseil-cpp//absl/functional:overload",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/log:absl_log",
        "@abseil-cpp//absl/strings:string_view",
    ],
)

rust_library(
    name = "rust_alloc_for_cpp_api",
    srcs = ["rust_alloc_for_cpp_api.rs"],
    visibility = [
        "//rust:__subpackages__",
    ],
)
