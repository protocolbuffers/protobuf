load("@rules_java//java:defs.bzl", "java_library")

java_library(
    name = "util",
    srcs = glob([
        "src/main/java/com/google/protobuf/util/*.java",
    ]),
    javacopts = [
        "-source 7",
        "-target 7",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//external:error_prone_annotations",
        "//external:gson",
        "//external:guava",
        "//java/core",
        "//java/lite",
    ],
)
