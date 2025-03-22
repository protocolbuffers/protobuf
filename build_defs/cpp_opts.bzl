"""C++ compile/link options for Protobuf libraries."""

COPTS = select({
    "//build_defs:config_msvc": [
        "/wd4065",  # switch statement contains 'default' but no 'case' labels
        "/wd4146",  # unary minus operator applied to unsigned type
        "/wd4244",  # 'conversion' conversion from 'type1' to 'type2', possible loss of data
        "/wd4251",  # 'identifier' : class 'type' needs to have dll-interface to be used by clients of class 'type2'
        "/wd4267",  # 'var' : conversion from 'size_t' to 'type', possible loss of data
        "/wd4305",  # 'identifier' : truncation from 'type1' to 'type2'
        "/wd4307",  # 'operator' : integral constant overflow
        "/wd4309",  # 'conversion' : truncation of constant value
        "/wd4334",  # 'operator' : result of 32-bit shift implicitly converted to 64 bits (was 64-bit shift intended?)
        "/wd4355",  # 'this' : used in base member initializer list
        "/wd4506",  # no definition for inline function 'function'
        "/wd4800",  # 'type' : forcing value to bool 'true' or 'false' (performance warning)
        "/wd4996",  # The compiler encountered a deprecated declaration.
    ],
    "//conditions:default": [
        "-Wno-sign-compare",
    ],
})

# Android and MSVC builds do not need to link in a separate pthread library.
LINK_OPTS = select({
    "@platforms//os:android": [],
    "//build_defs:config_android": [],
    "//build_defs:config_android-legacy-default-crosstool": [],
    "//build_defs:config_android-stlport": [],
    "//build_defs:config_android-libcpp": [],
    "//build_defs:config_android-gnu-libstdcpp": [],
    "//build_defs:config_android-default": [],
    "//build_defs:config_msvc": [
        # Suppress linker warnings about files with no symbols defined.
        "-ignore:4221",
        "Shell32.lib",
    ],
    "@platforms//os:macos": [
        "-lpthread",
        "-lm",
        "-framework CoreFoundation",
    ],
    "@platforms//os:windows": [
        "-ldbghelp",
        "-lpthread",
        "-lm",
    ],
    "//conditions:default": [
        "-lpthread",
        "-lm",
    ],
})
