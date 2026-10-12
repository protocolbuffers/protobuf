load("//bazel/private:compat_flag.bzl", "compat_label_flag")

package(
    default_applicable_licenses = ["//:license"],
    default_visibility = [
        "//bazel/private:__pkg__",
        "//bazel/tests:__subpackages__",
        "//devtools/blaze/exoblaze/mac/integration:__subpackages__",
        "//devtools/blaze/integration:__subpackages__",
    ],
)

exports_files(
    ["BUILD"],
)

compat_label_flag(
    name = "proto_toolchain_for_java",
    build_setting_default = "@bazel_tools//tools/proto:java_toolchain",
    fragment_field = "proto_toolchain_for_java",
)

compat_label_flag(
    name = "proto_toolchain_for_javalite",
    build_setting_default = "@bazel_tools//tools/proto:javalite_toolchain",
    fragment_field = "proto_toolchain_for_javalite",
)
