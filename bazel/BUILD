load("@bazel_skylib//:bzl_library.bzl", "bzl_library")

package(default_applicable_licenses = ["//:license"])

bzl_library(
    name = "proto_library_bzl",
    srcs = ["proto_library.bzl"],
    visibility = ["//visibility:public"],
    deps = [
        "//bazel/private:proto_library_rule_bzl",
    ],
)

bzl_library(
    name = "cc_proto_library_bzl",
    srcs = ["cc_proto_library.bzl"],
    visibility = ["//visibility:public"],
    deps = [
        "//bazel/private/oss:cc_proto_library_bzl",
    ],
)

bzl_library(
    name = "java_proto_library_bzl",
    srcs = ["java_proto_library.bzl"],
    visibility = ["//visibility:public"],
    deps = [
        "//bazel/private:java_proto_library_bzl",
    ],
)

bzl_library(
    name = "java_lite_proto_library_bzl",
    srcs = ["java_lite_proto_library.bzl"],
    visibility = ["//visibility:public"],
    deps = ["//bazel/private:java_lite_proto_library_bzl"],
)

bzl_library(
    name = "py_proto_library_bzl",
    srcs = ["py_proto_library.bzl"],
    visibility = ["//visibility:public"],
    deps = [
        "//bazel/private/oss:py_proto_library_bzl",
    ],
)

bzl_library(
    name = "upb_c_proto_library_bzl",
    srcs = ["upb_c_proto_library.bzl"],
    deprecation = "Use upb/bazel:upb_proto_library_bzl instead",
    visibility = ["//visibility:public"],
    deps = ["//upb/bazel:upb_proto_library_bzl"],
)

bzl_library(
    name = "upb_proto_reflection_library_bzl",
    srcs = ["upb_proto_reflection_library.bzl"],
    deprecation = "Use upb/bazel:upb_proto_library_bzl instead",
    visibility = ["//visibility:public"],
    deps = ["//upb/bazel:upb_proto_library_bzl"],
)

bzl_library(
    name = "proto_descriptor_set_bzl",
    srcs = ["proto_descriptor_set.bzl"],
    visibility = ["//visibility:public"],
)

# The data in this target is exposed in //bazel/private:for_bazel_tests
filegroup(
    name = "for_bazel_tests",
    testonly = True,
    srcs = [
        "BUILD",
        ":cc_proto_library_bzl",
        ":java_lite_proto_library_bzl",
        ":proto_library_bzl",
        ":py_proto_library_bzl",
        ":upb_c_proto_library_bzl",
        ":upb_proto_reflection_library_bzl",
        "//bazel/common:for_bazel_tests",
        "//bazel/flags:for_bazel_tests",
        "//bazel/toolchains:for_bazel_tests",
    ],
    visibility = [
        "//bazel/private:__pkg__",
    ],
)
