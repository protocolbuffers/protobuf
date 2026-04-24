load("@rules_shell//shell:sh_test.bzl", "sh_test")

package(default_applicable_licenses = ["//:license"])

config_setting(
    name = "android",
    constraint_values = ["@platforms//os:android"],
)

config_setting(
    name = "wasm",
    constraint_values = ["@platforms//os:emscripten"],
)

PLATFORM_ARGS = select({
    ":android": ["android"],
    ":wasm": ["wasm"],
    "//conditions:default": ["not_supported"],
})

# This works on all platforms today because unused symbols are correctly stripped.
sh_test(
    name = "unused_symbols_test",
    srcs = ["verify_unused_symbols.sh"],
    args = [
        "$(rootpath //rust/test/treeshaking/base:treeshaking_app)",
        "$(rootpath //third_party/crosstool/google3_users:stable_nm)",
    ] + PLATFORM_ARGS,
    data = [
        "//rust/test/treeshaking/base:treeshaking_app",
        "//third_party/crosstool/google3_users:stable_nm",
    ],
)

# This only works on platforms with panic=abort or panic=immediate-abort.
sh_test(
    name = "except_tables_test",
    srcs = ["verify_except_tables.sh"],
    args = [
        "$(rootpath //rust/test/treeshaking/base:treeshaking_app)",
        "$(rootpath //rust/test/treeshaking/test:treeshaking_app)",
        "$(rootpath //third_party/crosstool/google3_users:stable_nm)",
    ] + PLATFORM_ARGS,
    data = [
        "//rust/test/treeshaking/base:treeshaking_app",
        "//rust/test/treeshaking/test:treeshaking_app",
        "//third_party/crosstool/google3_users:stable_nm",
    ],
)
