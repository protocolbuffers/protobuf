"""Macro redirects for the blaze prelude"""

load("//bazel:cc_proto_library.bzl", _cc_proto_library = "cc_proto_library")
load("//bazel:java_lite_proto_library.bzl", _java_lite_proto_library = "java_lite_proto_library")
load("//bazel:java_proto_library.bzl", _java_proto_library = "java_proto_library")
load("//bazel:proto_library.bzl", _proto_library = "proto_library")

def _prelude_cc_proto_library(**kwargs):
    _cc_proto_library(**kwargs)

def _prelude_java_lite_proto_library(**kwargs):
    _java_lite_proto_library(**kwargs)

def _prelude_java_proto_library(**kwargs):
    _java_proto_library(**kwargs)

def _prelude_proto_library(**kwargs):
    _proto_library(**kwargs)

cc_proto_library = _prelude_cc_proto_library
java_lite_proto_library = _prelude_java_lite_proto_library
java_proto_library = _prelude_java_proto_library
proto_library = _prelude_proto_library
