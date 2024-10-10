"""C++ compile/link options for Protobuf libraries."""

# Android and MSVC builds do not need to link in a separate pthread library.
LINK_OPTS = select({
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
    "//conditions:default": [
        "-lpthread",
        "-lm",
    ],
})
