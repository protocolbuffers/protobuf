# Protocol Buffers - Google's data interchange format
# Copyright 2025 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd
"""Tests for `java_mutable_proto_library`."""

load("@rules_java//java/common:java_info.bzl", "JavaInfo")
load("@rules_testing//lib:analysis_test.bzl", "analysis_test", "test_suite")
load("@rules_testing//lib:truth.bzl", "subjects")
load("@rules_testing//lib:util.bzl", "util")
load("//bazel:java_mutable_proto_library.bzl", "java_mutable_proto_library")
load("//bazel:proto_library.bzl", "proto_library")
load("//bazel/private/google:java_mutable_proto_library.bzl", "google_java_mutable_proto_aspect_services")

def java_mutable_proto_library_test_suite(name):
    util.helper_target(
        proto_library,
        name = "simple_mutable_java_proto",
        srcs = ["A.proto"],
    )
    util.helper_target(
        java_mutable_proto_library,
        name = "simple_java_mutable_proto",
        deps = [":simple_mutable_java_proto"],
    )

    test_suite(
        name = name,
        tests = [
            _test_mutable_binary_deps,
            _test_mutable_proto_library_builds_compiled_jar,
            _test_mutable_binary_option_deps,
            _test_mutable_java_proto2_compiler_args,
            _test_mutable_command_line_contains_target_label,
            _test_mutable_same_version_compiler_arguments,
        ],
    )

# Tests that java_mutable_proto_library depends on the right set of files.
def _test_mutable_binary_deps(name):
    util.helper_target(
        proto_library,
        name = name + "_baz",
        srcs = ["baz.proto"],
    )
    util.helper_target(
        proto_library,
        name = name + "_foo",
        srcs = [
            "bar.proto",
            "foo.proto",
        ],
        deps = [":" + name + "_baz"],
    )
    util.helper_target(
        java_mutable_proto_library,
        name = name + "_pb2",
        deps = [":" + name + "_foo"],
    )
    analysis_test(
        name = name,
        target = ":" + name + "_pb2",
        impl = _test_mutable_binary_deps_impl,
        provider_subject_factories = [java_info_subject_factory],
    )

def _test_mutable_binary_deps_impl(env, target):
    java_info = env.expect.that_target(target).provider(JavaInfo)
    java_info.transitive_source_jars_in_package().contains_exactly([
        "{package}/test_mutable_binary_deps_foo-new-mutable-src.jar",
        "{package}/test_mutable_binary_deps_baz-new-mutable-src.jar",
    ])
    java_info.transitive_source_jars().contains(
        "third_party/java_src/protobuf/current/java/com/google/protobuf/libprotobuf_lite-src.jar",
    )
    java_info.transitive_compile_time_jars_in_package().contains_exactly([
        "{package}/libtest_mutable_binary_deps_foo-new-mutable-hjar.jar",
        "{package}/libtest_mutable_binary_deps_baz-new-mutable-hjar.jar",
    ])
    java_info.transitive_compile_time_jars().contains(
        "third_party/java_src/protobuf/current/java/com/google/protobuf/libprotobuf_lite-hjar.jar",
    )

def _test_mutable_proto_library_builds_compiled_jar(name):
    util.helper_target(
        proto_library,
        name = name + "_foo",
        srcs = ["ok.proto"],
    )
    util.helper_target(
        java_mutable_proto_library,
        name = name + "_java_pb2",
        deps = [":" + name + "_foo"],
    )
    analysis_test(
        name = name,
        target = ":" + name + "_java_pb2",
        impl = _test_mutable_proto_library_builds_compiled_jar_impl,
    )

def _test_mutable_proto_library_builds_compiled_jar_impl(env, target):
    env.expect.that_target(target).default_outputs().contains(
        "{package}/libtest_mutable_proto_library_builds_compiled_jar_foo-new-mutable.jar",
    )

# Tests that java_binaries which depend on proto_libraries depend on the right set of files for
# option_deps.
def _test_mutable_binary_option_deps(name):
    util.helper_target(
        proto_library,
        name = name + "_baz",
        srcs = ["baz.proto"],
    )
    util.helper_target(
        proto_library,
        name = name + "_foo",
        srcs = [
            "bar.proto",
            "foo.proto",
        ],
        option_deps = [":" + name + "_baz"],
    )
    util.helper_target(
        java_mutable_proto_library,
        name = name + "_pb2",
        deps = [":" + name + "_foo"],
    )
    analysis_test(
        name = name,
        target = ":" + name + "_pb2",
        impl = _test_mutable_binary_option_deps_impl,
        provider_subject_factories = [java_info_subject_factory],
    )

def _test_mutable_binary_option_deps_impl(env, target):
    java_info = env.expect.that_target(target).provider(JavaInfo)
    java_info.transitive_source_jars_in_package().contains_exactly([
        "{package}/test_mutable_binary_option_deps_foo-new-mutable-src.jar",
    ])
    java_info.transitive_source_jars().contains(
        "third_party/java_src/protobuf/current/java/com/google/protobuf/libprotobuf_lite-src.jar",
    )
    java_info.transitive_compile_time_jars_in_package().contains_exactly([
        "{package}/libtest_mutable_binary_option_deps_foo-new-mutable-hjar.jar",
    ])
    java_info.transitive_compile_time_jars().contains(
        "third_party/java_src/protobuf/current/java/com/google/protobuf/libprotobuf_lite-hjar.jar",
    )

_java_mutable_proto_aspect_testing_aspect = util.make_testing_aspect(aspects = [google_java_mutable_proto_aspect_services])

# Tests that we pass the correct arguments to the protocol compiler
def _test_mutable_java_proto2_compiler_args(name):
    util.helper_target(
        proto_library,
        name = name + "_protolib",
        srcs = ["file.proto"],
    )
    analysis_test(
        name = name,
        target = ":" + name + "_protolib",
        testing_aspect = _java_mutable_proto_aspect_testing_aspect,
        impl = _test_mutable_java_proto2_compiler_args_impl,
    )

def _test_mutable_java_proto2_compiler_args_impl(env, target):
    proto_library = env.expect.that_target(target)
    gen_proto = proto_library.action_named("GenProto")
    gen_proto.argv().contains("{package}/file.proto")
    gen_proto.argv().contains_at_least([
        "-I.",
        "{package}/file.proto",
        "--mutable_java_out={bindir}/{package}/test_mutable_java_proto2_compiler_args_protolib-new-mutable-src.jar",
    ]).in_order()

def _test_mutable_command_line_contains_target_label(name):
    util.helper_target(
        proto_library,
        name = name + "_proto",
        srcs = ["dummy.proto"],
    )
    analysis_test(
        name = name,
        target = ":" + name + "_proto",
        testing_aspect = _java_mutable_proto_aspect_testing_aspect,
        impl = _test_mutable_command_line_contains_target_label_impl,
    )

def _test_mutable_command_line_contains_target_label_impl(env, target):
    action = env.expect.that_target(target).action_named("Javac")
    action.argv().contains_at_least([
        "--target_label",
        "//{package}:test_mutable_command_line_contains_target_label_proto",
        "--injecting_rule_kind",
        "java_mutable_proto_library",
    ])

def _test_mutable_same_version_compiler_arguments(name):
    util.helper_target(
        proto_library,
        name = name + "_protolib",
    )
    util.helper_target(
        proto_library,
        name = name + "_protolib2",
        srcs = ["file.proto"],
        deps = [":" + name + "_protolib"],
    )
    analysis_test(
        name = name,
        target = ":" + name + "_protolib2",
        testing_aspect = _java_mutable_proto_aspect_testing_aspect,
        impl = _test_mutable_same_version_compiler_arguments_impl,
        provider_subject_factories = [java_info_subject_factory],
    )

def _test_mutable_same_version_compiler_arguments_impl(env, target):
    proto_library = env.expect.that_target(target)
    gen_proto = proto_library.action_named("GenProto")
    gen_proto.argv().contains("{package}/file.proto")
    gen_proto.argv().contains_at_least([
        "-I.",
        "{package}/file.proto",
        "--mutable_java_out={bindir}/{package}/test_mutable_same_version_compiler_arguments_protolib2-new-mutable-src.jar",
    ]).in_order()
    java_info = env.expect.that_target(target).provider(JavaInfo)
    java_info.transitive_runtime_jars().contains_at_least([
        "{package}/libtest_mutable_same_version_compiler_arguments_protolib2-new-mutable.jar",
        "third_party/java_src/protobuf/current/java/com/google/protobuf/libprotobuf_lite.jar",
    ])

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
