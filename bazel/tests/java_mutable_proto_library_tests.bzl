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
            _test_proto_library_builds_compiled_mutable_jar,
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
    java_info.transitive_source_jars().contains_at_least([
        "{package}/test_mutable_binary_deps_foo-new-mutable-src.jar",
        "{package}/test_mutable_binary_deps_baz-new-mutable-src.jar",
    ])
    java_info.transitive_source_jars().contains(
        "java/core/liblite_runtime_only-src.jar",
    )
    java_info.transitive_compile_time_jars().contains_at_least([
        "{package}/libtest_mutable_binary_deps_foo-new-mutable-hjar.jar",
        "{package}/libtest_mutable_binary_deps_baz-new-mutable-hjar.jar",
    ])
    java_info.transitive_compile_time_jars().contains(
        "third_party/java_src/protobuf/current/java/com/google/protobuf/libprotobuf_lite-hjar.jar",
    )

def _test_proto_library_builds_compiled_mutable_jar(name):
    util.helper_target(
        proto_library,
        name = "compiled_mutable",
        srcs = ["ok.proto"],
    )
    util.helper_target(
        java_mutable_proto_library,
        name = "compiled_mutable_java_pb2",
        deps = [":compiled_mutable"],
    )
    analysis_test(
        name = name,
        target = ":compiled_mutable_java_pb2",
        impl = _test_proto_library_builds_compiled_mutable_jar_impl,
    )

def _test_proto_library_builds_compiled_mutable_jar_impl(env, target):
    env.expect.that_target(target).default_outputs().contains(
        "{package}/libcompiled_mutable-new-mutable.jar",
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
