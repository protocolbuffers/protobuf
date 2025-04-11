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
