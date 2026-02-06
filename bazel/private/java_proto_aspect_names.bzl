"""Exposes Java proto aspect names to select paths"""

visibility([
    "@rules_java//java/google/rules",
])

java_lite_proto_aspect_name = "//bazel/private:java_lite_proto_library.bzl%_java_lite_proto_aspect"
java_proto_aspect_name = "//bazel/private:java_proto_library.bzl%java_proto_aspect"
java_mutable_proto_aspect_names = [
    "//bazel/private/google:java_mutable_proto_library.bzl%google_java_mutable_proto_aspect_services",
    "//bazel/private/google:java_mutable_proto_library.bzl%google_java_mutable_proto_aspect_noservices",
]
