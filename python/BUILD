load(
    "//bazel:build_defs.bzl",
    "UPB_DEFAULT_COPTS",
)

cc_binary(
    name = "message",
    srcs = [
        "descriptor.c",
        "descriptor.h",
        "descriptor_pool.c",
        "descriptor_pool.h",
        "protobuf.c",
        "protobuf.h",
    ],
    copts = UPB_DEFAULT_COPTS + [
        # The Python API requires patterns that are ISO C incompatible, like
        # casts between function pointers and object pointers.
        "-Wno-pedantic",
    ],
    linkopts = ["-Wl,--version-script,$(location :version_script.lds)"],
    linkshared = True,
    linkstatic = True,
    deps = [
        ":version_script.lds",
        "//:reflection",
        "//:upb",
        "@python_headers",
    ],
)
