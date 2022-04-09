load("@rules_pkg//:pkg.bzl", "pkg_zip")
load("@rules_pkg//:mappings.bzl", "pkg_attributes", "pkg_files")
load("//:protobuf_release.bzl", "package_naming")
load(":cc_dist_library.bzl", "cc_dist_library")

# Map of all well known protos.
# name => (include path, imports)
WELL_KNOWN_PROTO_MAP = {
    "any": ("src/google/protobuf/any.proto", []),
    "api": (
        "src/google/protobuf/api.proto",
        [
            "source_context",
            "type",
        ],
    ),
    "descriptor": ("src/google/protobuf/descriptor.proto", []),
    "duration": ("src/google/protobuf/duration.proto", []),
    "empty": ("src/google/protobuf/empty.proto", []),
    "field_mask": ("src/google/protobuf/field_mask.proto", []),
    "source_context": ("src/google/protobuf/source_context.proto", []),
    "struct": ("src/google/protobuf/struct.proto", []),
    "timestamp": ("src/google/protobuf/timestamp.proto", []),
    "type": (
        "src/google/protobuf/type.proto",
        [
            "any",
            "source_context",
        ],
    ),
    "wrappers": ("src/google/protobuf/wrappers.proto", []),
}

# plugin.proto is excluded from this list because it belongs in a nested folder (protobuf/compiler/plugin.proto)
pkg_files(
    name = "wkt_protos_files",
    srcs = ["//:" + value[0] for value in WELL_KNOWN_PROTO_MAP.values() if not value[0].endswith("plugin.proto")],
    prefix = "include/google/protobuf",
    visibility = ["//visibility:private"],
)

package_naming(
    name = "protoc_pkg_naming",
)

pkg_zip(
    name = "protoc_release",
    srcs = [
        ":wkt_protos_files",
        "//src/google/protobuf/compiler:compiler_plugin_protos_files",
        "//src/google/protobuf/compiler:protoc_files",
        "//src/google/protobuf/compiler:readme.txt",
    ],
    package_file_name = "protoc-{version}-{platform}.zip",
    package_variables = ":protoc_pkg_naming",
)

cc_dist_library(
    name = "protobuf_lite",
    deps = [
        "//:arena",
        "//:protobuf_lite",
        "//src/google/protobuf/io:lite",
        "//src/google/protobuf/stubs:lite",
    ],
)
