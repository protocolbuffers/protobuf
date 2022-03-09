load(
    "//bazel:build_defs.bzl",
    "UPB_DEFAULT_COPTS",
)

def py_extension(name, srcs, deps = []):
    version_script = name + "_version_script.lds"
    symbol = "PyInit_" + name
    native.genrule(
        name = "gen_" + version_script,
        outs = [version_script],
        cmd = "echo 'message { global: " + symbol + "; local: *; };' > $@",
    )

    native.cc_binary(
        name = name,
        srcs = srcs,
        copts = UPB_DEFAULT_COPTS + [
            # The Python API requires patterns that are ISO C incompatible, like
            # casts between function pointers and object pointers.
            "-Wno-pedantic",
        ],
        # We use a linker script to hide all symbols except the entry point for
        # the module.
        linkopts = select({
            "@platforms//os:linux": ["-Wl,--version-script,$(location :" + version_script + ")"],
            "@platforms//os:macos": [
                "-Wl,-exported_symbol",
                "-Wl,_" + symbol,
            ],
        }),
        linkshared = True,
        linkstatic = True,
        deps = deps + [
            ":" + version_script,
            "@system_python//:python_headers",
        ],
    )
