# Protobuf JS runtime
#
# See also code generation logic under /src/google/protobuf/compiler/js.

load("@rules_pkg//:mappings.bzl", "pkg_files", "strip_prefix")

pkg_files(
    name = "dist_files",
    srcs = glob([
        "*.js",
        "*.json",
        "*.proto",
        "binary/*.js",
        "commonjs/*.js*",  # js, json
        "commonjs/**/*.proto",
        "compatibility_tests/v3.0.0/**/*.js*",
        "compatibility_tests/v3.0.0/**/*.proto",
        "compatibility_tests/v3.0.0/**/*.sh",
        "compatibility_tests/v3.1.0/**/*.js*",
        "compatibility_tests/v3.1.0/**/*.proto",
        "experimental/benchmarks/**/*.js",
        "experimental/runtime/**/*.js",
    ]) + [
        "BUILD",
        "README.md",
    ],
    strip_prefix = strip_prefix.from_root(""),
    visibility = ["//pkg:__pkg__"],
)
