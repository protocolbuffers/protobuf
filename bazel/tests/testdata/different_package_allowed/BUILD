load("//bazel/tests/testdata:check_collocated.bzl", "check_collocated")

package(
    default_applicable_licenses = ["//:license"],
    default_visibility = ["//visibility:public"],
)

check_collocated(
    name = "check_collocated",
    proto_dep = "//bazel/tests/testdata:simple_proto",
    tags = [
        "manual",
        "nobuilder",
        "notap",
    ],
)

check_collocated(
    name = "check_collocated_allow_exports",
    proto_dep = "//bazel/tests/testdata:allow_exports_proto",
    tags = [
        "manual",
        "nobuilder",
        "notap",
    ],
)

check_collocated(
    name = "check_collocated_disallow_exports",
    proto_dep = "//bazel/tests/testdata:disallow_exports_proto",
    tags = [
        "manual",
        "nobuilder",
        "notap",
    ],
)
