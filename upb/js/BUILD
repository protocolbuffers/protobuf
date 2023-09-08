# Copyright 2023 Google LLC
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("//third_party/bazel_rules/rules_wasm/wasm:defs.bzl", "wasm_cc_binary")
load("//testing/karma/builddefs:karma_web_test_suite.bzl", "karma_web_test_suite")
load("//javascript/tools/jscompiler/builddefs:flags.bzl", "DEBUG_FLAGS", "RECOMMENDED_FLAGS")
load("//tools/build_defs/js:rules.bzl", "js_binary", "js_library")
load("//javascript/typescript:build_defs.bzl", "ts_declaration", "ts_library")
load("@bazel_skylib//rules:common_settings.bzl", "bool_flag")

# begin:google_only
# package(default_applicable_licenses = ["//:license"])
# end:google_only

bool_flag(
    name = "upb_js_disable_bits",
    build_setting_default = False,
)

config_setting(
    name = "upb_js_disable_bits_config",
    flag_values = {
        ":upb_js_disable_bits": "true",
    },
)

# The minimal set of functions we need to be exported in the standard accessor mode (which
# implements the rest of the functionality using jsbits).
# buildifier: keep sorted
EXPORTED_FUNCTIONS_MINIMAL = [
    "malloc",
    "upb_Arena_Free",
    "upb_Arena_New",
    "upb_Decode",
    "upb_Encode",
    "upb_Message_New",
    "upb_MiniTableEnum_Build",
    "upb_MiniTableExtension_Build",
    "upb_MiniTable_Build",
    "upb_MiniTable_Link",
]

# The full upb API (required if we're not using jsbits).
# buildifier: keep sorted
EXPORTED_FUNCTIONS_FULL = EXPORTED_FUNCTIONS_MINIMAL + [
    "upb_Array_DataPtr",
    "upb_Array_Size",
    "upb_Message_GetArray",
    "upb_Message_GetBool",
    "upb_Message_GetDouble",
    "upb_Message_GetFloat",
    "upb_Message_GetInt32",
    "upb_Message_GetInt64Hi",
    "upb_Message_GetInt64Lo",
    "upb_Message_GetString",
    "upb_Message_GetString_Data",
    "upb_Message_GetString_Size",
    "upb_Message_GetUInt32",
    "upb_Message_GetUInt64Hi",
    "upb_Message_GetUInt64Lo",
    "upb_Message_HasField",
    "upb_Message_SetBool",
    "upb_Message_SetDouble",
    "upb_Message_SetFloat",
    "upb_Message_SetInt32",
    "upb_Message_SetInt64Split",
    "upb_Message_SetString_FromDataAndSize",
    "upb_Message_SetUInt64Split",
    "upb_MiniTableField_HasPresence",
    "upb_MiniTable_GetFieldByIndex",
]

COMMON_EMSCRIPTEN_LINKOPTS = [
    "-Wl,--import-memory",
    "-s INITIAL_MEMORY=128kb",
    "-s MAXIMUM_MEMORY=4GB",
    "-s ALLOW_MEMORY_GROWTH=1",
]

# The minimial upb C api (requires jsbits to be used for accessors)
cc_library(
    name = "upb_js_c_minimal",
    srcs = ["upb_js_minimal.c"],
    copts = ["-Os"],
    linkopts = select({
        "//tools/cc_target_os:emscripten": COMMON_EMSCRIPTEN_LINKOPTS + [
            "-sEXPORTED_FUNCTIONS=" + ",".join(["_" + f for f in EXPORTED_FUNCTIONS_MINIMAL]),
        ],
        "//conditions:default": [],
    }),
    deps = [
        "//:mem",
        "//:mini_descriptor",
        "//:wire",
    ],
    alwayslink = 1,
)

# The full upb api (exposes all upb accessor functions).
cc_library(
    name = "upb_js_c_full",
    srcs = [
        "upb_js_full.c",
    ],
    copts = ["-Os"],
    linkopts = select({
        "//tools/cc_target_os:emscripten": COMMON_EMSCRIPTEN_LINKOPTS + [
            "-sEXPORTED_FUNCTIONS=" + ",".join(["_" + f for f in EXPORTED_FUNCTIONS_FULL]),
        ],
        "//conditions:default": [],
    }),
    deps = [
        "//:base",
        "//:mem",
        "//:message_accessors",
        "//:message_split64",
        "//:message_types",
        "//:mini_descriptor",
        "//:mini_table",
        "//:port",
        "//:wire",
    ],
    alwayslink = 1,
)

cc_binary(
    name = "upb_js_c",
    deps = select({
        "@upb//js:upb_js_disable_bits_config": [":upb_js_c_full"],
        "//conditions:default": [":upb_js_c_minimal"],
    }),
)

wasm_cc_binary(
    name = "upb_js_wasm",
    cc_target = ":upb_js_c",
    exception_handling = "wasm",
    longjmp = "wasm",
    standalone = 1,
    visibility = [
        "//third_party/dart/pb_benchmarks:__pkg__",
        "@upb//js/impl:__pkg__",
    ],
)

ts_declaration(
    name = "upb_externs",
    srcs = ["upb.d.ts"],
    visibility = ["@upb//js/impl:__pkg__"],
)

js_binary(
    name = "upb",
    defs = RECOMMENDED_FLAGS + select({
        "//tools/compilation_mode:opt": [],
        "//conditions:default": DEBUG_FLAGS + ["--define goog.DEBUG=true"],
    }) + select({
        "@upb//js:upb_js_disable_bits_config": ["--define 'upb.js.USE_JS_BITS'=false"],
        "//conditions:default": [],
    }) + [
        # Wrap output in a function to avoid polluting the global scope.
        "--isolation_mode=IIFE",
        # This avoids a Promise polyfill.
        "--browser_featureset_year=2021",
    ],
    visibility = [
        "//third_party/dart/pb_benchmarks:__pkg__",
        "//third_party/dart/pb_runtime:__pkg__",
    ],
    deps = ["@upb//js/impl"],
)

js_library(
    name = "upb_test_js_lib",
    testonly = 1,
    srcs = ["upb_test.js"],
    deps = [
        "//javascript/apps/jspb/testing:jasmine",
        "//third_party/javascript/typings/jasmine",
        ":upb_externs",
    ],
)

ts_library(
    name = "upb_test_ts_lib",
    testonly = 1,
    srcs = ["upb_test.ts"],
    deps = [
        ":upb_externs",
        "//third_party/javascript/typings/jasmine",
    ],
)

# Run the tests through JSCompiler to prove that the necessary externs are present for upb.js.
js_binary(
    name = "upb_compiled_test_binary",
    testonly = 1,
    defs = RECOMMENDED_FLAGS + [
        # This avoids a Promise polyfill.
        "--browser_featureset_year=2021",
    ],
    deps = [
        ":upb_test_js_lib",
        ":upb_test_ts_lib",
        "//javascript/closure:base",
    ],
)

js_library(
    name = "upb_compiled_test_binary_lib",
    testonly = 1,
    srcs = ["upb_compiled_test_binary.js"],
    # This is compiler output, not source code.  We are only wrapping this is js_library() because
    # karma_web_test_suite makes us.
    check_level = "OFF",
)

# Debug interactively with:
#   iblaze run @upb//js:upb_js_tests_debug
karma_web_test_suite(
    name = "upb_js_tests",
    browsers = [
        # Last two major versions of Chrome (106 is newest):
        "//testing/web/browsers/chrome_linux/v114_0_5735_106:chrome-linux",
        "//testing/web/browsers/chrome_win7/v105_0_5195_102:chrome-win7",
        # Last two major versions of Chrome Android (106 is newest):
        "//testing/web/browsers/chrome_android/v105_0_5195_136:chrome105_0_5195_136-android_phone",
        # Last two major versions of Firefox (107 is newest).
        "//testing/web/browsers/firefox_linux/v116_0:firefox-linux",
        "//testing/web/browsers/firefox_win7/v113_0:firefox-win7",
        # Safari on OSX, unfortunately we don't seem to have specific versions we can pick.
        "//testing/web/browsers:safari-osx",

        # Last two major versions of Safari iOS
        # Safari 15 is newest: https://en.wikipedia.org/wiki/Safari_version_history#Safari_15_2
        # TODO: iOS doesn't support streaming instantiation, add fallback.
        # "//testing/web/browsers/safari_ios/v14_5:safari-iphone",

        # These parts of the requirements may not be available:
        # TODO: Last two Android versions
        # TODO: Last two Firefox Android versions, and the Extended Release
        # TODO: Last two Edge major versions
        # TODO: Last two Safari major versions
        # TODO: Last two iOS major versions
    ],
    concatjs = 0,
    config_file = ":wasm_karma.conf.js",
    data = [
        "upb_js_test_dom.html",
        ":upb",
    ],
    target_compatible_with = ["//third_party/bazel_platforms/cpu:x86_64"],
    deps = [
        ":upb_compiled_test_binary_lib",
    ],
)
