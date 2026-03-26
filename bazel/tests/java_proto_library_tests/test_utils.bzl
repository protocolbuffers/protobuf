"""Common test utils for java_proto_library tests. 
This file is forked from tests/java_proto_library_tests.bzl"""

load("@rules_java//java/common:java_info.bzl", "JavaInfo")
load("@rules_testing//lib:truth.bzl", "subjects")
load("@rules_testing//lib:util.bzl", "util")
load("//bazel/private/google:java_mutable_proto_library.bzl", "google_java_mutable_proto_aspect_services")

# Using this test aspect to mimic the java_mutable_proto_library target. This is mainly used to test
# the java related actions created in the test that isn't possible to access in Starlark.
JAVA_MUTABLE_PROTO_TESTING_ASPECT = util.make_testing_aspect(aspects = [google_java_mutable_proto_aspect_services])

HAS_SERVICES_TEST_ATTRS = {
    "has_services": attr.bool(default = True),
}

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
