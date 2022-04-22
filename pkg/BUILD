load("@rules_pkg//:pkg.bzl", "pkg_zip")
load("@rules_pkg//:mappings.bzl", "pkg_attributes", "pkg_files")
load("//:protobuf_release.bzl", "package_naming")
load(":cc_dist_library.bzl", "cc_dist_library")

pkg_files(
    name = "wkt_protos_files",
    srcs = [
        "//:any_proto",
        "//:api_proto",
        "//:duration_proto",
        "//:empty_proto",
        "//:field_mask_proto",
        "//:source_context_proto",
        "//:struct_proto",
        "//:timestamp_proto",
        "//:type_proto",
        "//:wrappers_proto",
    ],
    prefix = "include/google/protobuf",
    visibility = ["//visibility:private"],
)

pkg_files(
    name = "descriptor_protos_files",
    srcs = [
        "//:descriptor_proto",
    ],
    prefix = "include/google/protobuf",
    visibility = ["//visibility:private"],
)

pkg_files(
    name = "compiler_plugin_protos_files",
    srcs = ["//:compiler_plugin_proto"],
    prefix = "include/google/protobuf/compiler",
    visibility = ["//visibility:private"],
)

################################################################################
# Generates protoc release artifacts.
################################################################################

genrule(
    name = "protoc_readme",
    outs = ["readme.txt"],
    cmd = """
echo "Protocol Buffers - Google's data interchange format
Copyright 2008 Google Inc.
https://developers.google.com/protocol-buffers/
This package contains a precompiled binary version of the protocol buffer
compiler (protoc). This binary is intended for users who want to use Protocol
Buffers in languages other than C++ but do not want to compile protoc
themselves. To install, simply place this binary somewhere in your PATH.
If you intend to use the included well known types then don't forget to
copy the contents of the 'include' directory somewhere as well, for example
into '/usr/local/include/'.
Please refer to our official github site for more installation instructions:
  https://github.com/protocolbuffers/protobuf" > $@
    """,
    visibility = ["//visibility:private"],
)

pkg_files(
    name = "protoc_files",
    srcs = ["//:protoc"],
    attributes = pkg_attributes(mode = "0555"),
    prefix = "bin/",
    visibility = ["//visibility:private"],
)

package_naming(
    name = "protoc_pkg_naming",
)

pkg_zip(
    name = "protoc_release",
    srcs = [
        ":compiler_plugin_protos_files",
        ":descriptor_protos_files",
        ":protoc_files",
        ":protoc_readme",
        ":wkt_protos_files",
    ],
    package_file_name = "protoc-{version}-{platform}.zip",
    package_variables = ":protoc_pkg_naming",
)

################################################################################
# Protobuf runtime libraries.
################################################################################

cc_dist_library(
    name = "protobuf_lite",
    linkopts = select({
        "//:msvc": [],
        "//conditions:default": ["-lpthread"],
    }),
    deps = [
        "//:protobuf_lite",
    ],
)

cc_dist_library(
    name = "protobuf",
    linkopts = select({
        "//:msvc": [],
        "//conditions:default": [
            "-lz",
            "-lpthread",
        ],
    }),
    deps = [
        "//:protobuf",
        "//:protobuf_lite",
    ],
)
