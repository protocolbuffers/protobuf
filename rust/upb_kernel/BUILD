# This package contains Rust protobuf runtime implementation built on top of UPB.

cc_library(
    name = "upb_c_api",
    srcs = ["upb_api.c"],
    visibility = [
        "//src/google/protobuf:__subpackages__",
        "//rust:__subpackages__",
    ],
    deps = [
        "//upb:mem",
        "//upb:message",
    ],
)
