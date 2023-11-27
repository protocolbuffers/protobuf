load("@rules_cc//cc:defs.bzl", "cc_library")

licenses(["notice"])  # BSD/MIT-like license (for zlib)

exports_files(["zlib.BUILD"])

_ZLIB_HEADERS = [
    "crc32.h",
    "deflate.h",
    "gzguts.h",
    "inffast.h",
    "inffixed.h",
    "inflate.h",
    "inftrees.h",
    "trees.h",
    "zconf.h",
    "zlib.h",
    "zutil.h",
]

_ZLIB_PREFIXED_HEADERS = ["zlib/include/" + hdr for hdr in _ZLIB_HEADERS]

# In order to limit the damage from the `includes` propagation
# via `:zlib`, copy the public headers to a subdirectory and
# expose those.
genrule(
    name = "copy_public_headers",
    srcs = _ZLIB_HEADERS,
    outs = _ZLIB_PREFIXED_HEADERS,
    cmd_bash = "cp $(SRCS) $(@D)/zlib/include/",
    cmd_bat = " && ".join(
        ["@copy /Y $(location %s) $(@D)\\zlib\\include\\  >NUL" %
         s for s in _ZLIB_HEADERS],
    ),
)

cc_library(
    name = "zlib",
    srcs = [
        "adler32.c",
        "compress.c",
        "crc32.c",
        "deflate.c",
        "gzclose.c",
        "gzlib.c",
        "gzread.c",
        "gzwrite.c",
        "infback.c",
        "inffast.c",
        "inflate.c",
        "inftrees.c",
        "trees.c",
        "uncompr.c",
        "zutil.c",
        # Include the un-prefixed headers in srcs to work
        # around the fact that zlib isn't consistent in its
        # choice of <> or "" delimiter when including itself.
    ] + _ZLIB_HEADERS,
    hdrs = _ZLIB_PREFIXED_HEADERS,
    copts = select({
        "@platforms//os:windows": [],
        "//conditions:default": [
            "-Wno-deprecated-non-prototype",
            "-Wno-unused-variable",
            "-Wno-implicit-function-declaration",
        ],
    }),
    includes = ["zlib/include/"],
    visibility = ["//visibility:public"],
)
