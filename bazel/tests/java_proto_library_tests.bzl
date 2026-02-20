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
load("//bazel/private:java_proto_library.bzl", "java_proto_aspect")

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
            _test_binary_option_deps,
            _test_proto_library_builds_compiled_jar,
            _test_command_line_contains_target_label,
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

def _test_proto_library_builds_compiled_jar(name):
    util.helper_target(
        proto_library,
        name = "compiled",
        srcs = ["ok.proto"],
    )
    util.helper_target(
        java_proto_library,
        name = "compiled_java_pb2",
        deps = [":compiled"],
    )
    analysis_test(
        name = name,
        target = ":compiled_java_pb2",
        impl = _test_proto_library_builds_compiled_jar_impl,
    )

def _test_proto_library_builds_compiled_jar_impl(env, target):
    env.expect.that_target(target).default_outputs().contains(
        "{package}/libcompiled-speed.jar",
    )

def _test_command_line_contains_target_label(name):
    util.helper_target(
        proto_library,
        name = "cmd_line_proto",
        srcs = ["dummy.proto"],
    )
    analysis_test(
        name = name,
        target = ":cmd_line_proto",
        testing_aspect = _java_proto_aspect_testing_aspect,
        impl = _test_command_line_contains_target_label_impl,
    )

def _test_command_line_contains_target_label_impl(env, target):
    action = env.expect.that_target(target).action_named("Javac")
    action.argv().contains_at_least([
        "--target_label",
        "//{package}:cmd_line_proto",
        "--injecting_rule_kind",
        "java_proto_library",
    ])

# Tests that java_binaries which depend on proto_libraries depend on the right set of files for
# option_deps.
def _test_binary_option_deps(name):
    util.helper_target(
        proto_library,
        name = "option_deps_baz",
        srcs = ["baz.proto"],
    )
    util.helper_target(
        proto_library,
        name = "option_deps_foo",
        srcs = [
            "bar.proto",
            "foo.proto",
        ],
        option_deps = [":option_deps_baz"],
    )
    util.helper_target(
        java_proto_library,
        name = "option_deps_java_pb2",
        deps = [":option_deps_foo"],
    )
    analysis_test(
        name = name,
        target = ":option_deps_java_pb2",
        impl = _test_binary_option_deps_impl,
        provider_subject_factories = [java_info_subject_factory],
    )

def _test_binary_option_deps_impl(env, target):
    java_info = env.expect.that_target(target).provider(JavaInfo)
    java_info.transitive_source_jars_in_package().contains_exactly([
        "{package}/option_deps_foo-speed-src.jar",
    ])
    java_info.transitive_source_jars().contains(
        "java/core/liblite_runtime_only-src.jar",
    )
    java_info.transitive_compile_time_jars_in_package().contains_exactly([
        "{package}/liboption_deps_foo-speed-hjar.jar",
    ])
    java_info.transitive_compile_time_jars().contains(
        "java/core/liblite_runtime_only-hjar.jar",
    )

def _filter_inpackage(file_depset, owner):
    return depset([f for f in file_depset.to_list() if f.owner.package == owner.package])

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
        actual = info,
        transitive_source_jars = lambda *a, **k: subjects.depset_file(
            self.actual.transitive_source_jars,
            meta = self.meta.derive("transitive_source_jars()"),
        ),
        transitive_source_jars_in_package = lambda *a, **k: subjects.depset_file(
            _filter_inpackage(self.actual.transitive_source_jars, meta.ctx.label),
            meta = self.meta.derive("transitive_source_jars_in_package()"),
        ),
        transitive_runtime_jars = lambda *a, **k: subjects.depset_file(
            self.actual.transitive_runtime_jars,
            meta = self.meta.derive("transitive_runtime_jars()"),
        ),
        transitive_compile_time_jars = lambda *a, **k: subjects.depset_file(
            self.actual.transitive_compile_time_jars,
            meta = self.meta.derive("transitive_compile_time_jars()"),
        ),
        transitive_compile_time_jars_in_package = lambda *a, **k: subjects.depset_file(
            _filter_inpackage(self.actual.transitive_compile_time_jars, meta.ctx.label),
            meta = self.meta.derive("transitive_compile_time_jars_in_package()"),
        ),
    )
    return public

java_info_subject_factory = struct(
    type = JavaInfo,
    name = "JavaInfo",
    factory = _java_info_subject,
)
