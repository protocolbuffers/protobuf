# Protocol Buffers - Google's data interchange format
# Copyright 2024 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd
#
"""Tests for `java_proto_library`."""

load("@rules_java//java/common:java_info.bzl", "JavaInfo")
load("@rules_testing//lib:analysis_test.bzl", "analysis_test", "test_suite")
load("@rules_testing//lib:truth.bzl", "subjects")
load("@rules_testing//lib:util.bzl", "util")
load("//bazel:java_proto_library.bzl", "java_proto_library")
load("//bazel:proto_library.bzl", "proto_library")
load("//bazel/private:bazel_java_proto_library_rule.bzl", java_proto_aspect = "bazel_java_proto_aspect")

def java_proto_library_test_suite(name):
    util.helper_target(
        java_proto_library,
        name = "simple_java_proto",
        deps = [":simple_proto"],
    )
    test_suite(
        name = name,
        tests = [
            _test_returned_java_info,
            _test_java_proto2_compiler_args,
        ],
    )

# Verifies JavaInfo returned from java_proto_library contains library and source jar.
def _test_returned_java_info(name):
    analysis_test(
        name = name,
        target = ":simple_java_proto",
        impl = _test_returned_java_info_impl,
        provider_subject_factories = [java_info_subject_factory],
    )

def _test_returned_java_info_impl(env, target):
    java_info = env.expect.that_target(target).provider(JavaInfo)
    java_info.transitive_source_jars().contains("{package}/simple_proto-speed-src.jar")
    java_info.transitive_runtime_jars().contains("{package}/libsimple_proto-speed.jar")

_java_proto_aspect_testing_aspect = util.make_testing_aspect(aspects = [java_proto_aspect])

# Verifies arguments of action created by java_proto_aspect.
def _test_java_proto2_compiler_args(name):
    # A nojp_simple_proto is defined to avoid conflict with native java_proto_library
    # aspect used in Bazel 6 and 7. Once Bazel 6,7 are not tested anymore it may be
    # replaced with simple_proto.
    util.helper_target(
        proto_library,
        name = "nojp_simple_proto",
        srcs = ["A.proto"],
    )
    analysis_test(
        name = name,
        target = ":nojp_simple_proto",
        testing_aspect = _java_proto_aspect_testing_aspect,
        impl = _test_java_proto2_compiler_args_impl,
    )

def _test_java_proto2_compiler_args_impl(env, target):
    proto_library = env.expect.that_target(target)
    gen_proto = proto_library.action_named("GenProto")
    gen_proto.argv().contains("{package}/A.proto")
    gen_proto.argv().contains_at_least([
        "-I.",
        "{package}/A.proto",
        "--java_out={bindir}/{package}/nojp_simple_proto-speed-src.jar",
    ])

def _java_info_subject(info, *, meta):
    """Creates a new `JavaInfoSubject` for a JavaInfo provider instance.

    Args:
        info: The JavaInfo object
        meta: ExpectMeta object.

    Returns:
        A `JavaInfoSubject` struct
    """
    self = struct(actual = info, meta = meta)
    public = struct(
        transitive_source_jars = lambda *a, **k: subjects.depset_file(
            self.actual.transitive_source_jars,
            meta = self.meta.derive("transitive_source_jars()"),
        ),
        transitive_runtime_jars = lambda *a, **k: subjects.depset_file(
            self.actual.transitive_runtime_jars,
            meta = self.meta.derive("transitive_runtime_jars()"),
        ),
    )
    return public

java_info_subject_factory = struct(
    type = JavaInfo,
    name = "JavaInfo",
    factory = _java_info_subject,
)
