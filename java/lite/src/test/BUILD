load("@rules_java//java:defs.bzl", "java_library", "java_test")

java_library(
    name = "test",
    srcs = glob(["java/**/*.java"]),
    data = [
        "//src/google/protobuf/testdata",
    ],
    deps = [
        "//:map_lite_unittest_java_proto",
        "//:unittest_import_lite_java_proto",
        "//:unittest_import_public_lite_java_proto",
        "//:unittest_lite_java_proto",
        "//java/core/src/test:test_java_lite_protos",
        "//java/core/src/test:test_util_lite",
        "//java/lite",
        "@maven//:junit_junit",
    ],
)

[
    java_test(
        name = src[:-len(".java")],
        runtime_deps = [
            ":test",
        ],
    )
    for src in glob(["java/**/*Test.java"])
]
