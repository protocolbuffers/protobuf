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
    # We use a linker script to hide all symbols except the entry point for
    # the module.
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

# Copy the extension into the location recognized by Python.
genrule(
    name = "message_ext",
    srcs = [":message"],
    outs = ["google/protobuf/pyext/_message.cpython-39-x86_64-linux-gnu.so"],
    cmd = "cp $< $@",
)

py_test(
    name = "minimal_test",
    srcs = ["minimal_test.py"],
    data = [":message_ext"],
    imports = ["."],
    legacy_create_init = False,
)
