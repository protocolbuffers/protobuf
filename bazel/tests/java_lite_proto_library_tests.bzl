"""Tests for java_lite_proto_library rule."""

load("@rules_java//java/common:java_info.bzl", "JavaInfo")
load("@rules_java//java/common:proguard_spec_info.bzl", "ProguardSpecInfo")
load("@rules_testing//lib:analysis_test.bzl", "analysis_test", "test_suite")
load("@rules_testing//lib:truth.bzl", "matching")
load("@rules_testing//lib:util.bzl", "util")
load("//bazel:java_lite_proto_library.bzl", "java_lite_proto_library")
load("//bazel:proto_library.bzl", "proto_library")
load("//bazel/private:java_lite_proto_library.bzl", "java_lite_proto_aspect")

_java_lite_proto_aspect_testing_aspect = util.make_testing_aspect(aspects = [java_lite_proto_aspect])

def _to_list(x):
    if type(x) == "depset":
        return x.to_list()
    return x

# buildifier: disable=unused-variable
def _noop_impl(env, target):
    pass

# Aspect & Rule for test `testJavaLiteProtoLibraryAspectProviders`
MyInfo = provider(
    doc = "Exposes aspect target verification result.",
    fields = ["result"],
)

def _foo_aspect_impl(target, ctx):
    proto_found = hasattr(target, "proto_java")
    if hasattr(ctx.rule.attr, "deps"):
        for dep in ctx.rule.attr.deps:
            if MyInfo in dep:
                proto_found = proto_found or dep[MyInfo].result
    return [MyInfo(result = proto_found)]

foo_aspect = aspect(
    implementation = _foo_aspect_impl,
    attr_aspects = ["deps"],
)

def _foo_rule_impl(ctx):
    return [MyInfo(result = ctx.attr.dep[MyInfo].result)]

foo_rule = rule(
    implementation = _foo_rule_impl,
    attrs = {
        "dep": attr.label(aspects = [foo_aspect]),
    },
)

# Verifies that compiling a java_lite_proto_library depends transitively on correct generated source
# jars, library jars, and runtime dependencies while excluding other protocol compilers.
def _test_lite_binary_deps(name):
    util.helper_target(
        proto_library,
        name = name + "/baz",
        srcs = ["baz.proto"],
    )
    util.helper_target(
        proto_library,
        name = name + "/foo",
        srcs = ["foo.proto", "bar.proto"],
        deps = [name + "/baz"],
    )
    util.helper_target(
        java_lite_proto_library,
        name = name + "/lite_pb2",
        deps = [name + "/foo"],
    )
    analysis_test(
        name = name,
        impl = _test_lite_binary_deps_impl,
        target = name + "/lite_pb2",
    )

def _test_lite_binary_deps_impl(env, target):
    java_info = target[JavaInfo]
    runtime_jars = [f.basename for f in _to_list(java_info.transitive_runtime_jars)]
    env.expect.that_collection(runtime_jars).contains_at_least([
        "libfoo-lite.jar",
        "libbaz-lite.jar",
        "liblite.jar",
    ])
    source_jars = [f.basename for f in _to_list(java_info.transitive_source_jars)]
    env.expect.that_collection(source_jars).contains_at_least([
        "foo-lite-src.jar",
        "baz-lite-src.jar",
    ])

# Verifies that spawning the protocol compiler on a target with java_lite_proto_aspect applied passes
# the correct `--java_out`, `-I`, and input proto file arguments to `protoc`.
def _test_lite_java_proto2_compiler_args(name):
    util.helper_target(
        proto_library,
        name = name + "/protolib",
        srcs = ["file.proto"],
    )
    analysis_test(
        name = name,
        impl = _test_lite_java_proto2_compiler_args_impl,
        target = name + "/protolib",
        testing_aspect = _java_lite_proto_aspect_testing_aspect,
    )

def _test_lite_java_proto2_compiler_args_impl(env, target):
    action = env.expect.that_target(target).action_generating(
        "{package}/{test_name}/protolib-lite-src.jar",
    )
    action.argv().contains_at_least([
        "--java_out=lite:{bindir}/{package}/{test_name}/protolib-lite-src.jar",
        "-I.",
        "{package}/file.proto",
    ]).in_order()
    action.inputs().contains_at_least_predicates([
        # Can't use an exact match because of Windows .exe suffix.
        matching.file_basename_contains("protoc"),
    ])
    action.inputs().not_contains_predicate(
        matching.file_basename_equals("protoc-gen-rpc"),
    )

# Verifies that compiled proto libraries compile successfully and output the correct `lib*-lite.jar`
# file in default outputs.
def _test_lite_proto_library_builds_compiled_jar(name):
    util.helper_target(
        proto_library,
        name = name + "/compiled",
        srcs = ["ok.proto"],
    )
    util.helper_target(
        java_lite_proto_library,
        name = name + "/lite_pb2",
        deps = [name + "/compiled"],
    )
    analysis_test(
        name = name,
        impl = _test_lite_proto_library_builds_compiled_jar_impl,
        target = name + "/lite_pb2",
    )

def _test_lite_proto_library_builds_compiled_jar_impl(env, target):
    basenames = [f.basename for f in target[DefaultInfo].files.to_list()]
    env.expect.that_collection(basenames).contains("libcompiled-lite.jar")

# Verifies that the Javac compiler action registers and passes the correct `--target_label` option
# pointing to compile boundaries of the generated code.
def _test_lite_command_line_contains_target_label(name):
    util.helper_target(
        proto_library,
        name = name + "/proto",
        srcs = ["dummy.proto"],
    )
    analysis_test(
        name = name,
        impl = _test_lite_command_line_contains_target_label_impl,
        target = name + "/proto",
        testing_aspect = _java_lite_proto_aspect_testing_aspect,
    )

def _test_lite_command_line_contains_target_label_impl(env, target):
    action = env.expect.that_target(target).action_generating(
        "{package}/{test_name}/libproto-lite.jar",
    )
    action.argv().contains_at_least([
        "--target_label",
        "//{package}:{test_name}/proto",
    ]).in_order()

# Verifies that a java_lite_proto_library with a middle empty proto library compiles successfully,
# resulting in empty compilation jars and source jars lists.
def _test_lite_empty_srcs_for_java_api(name):
    util.helper_target(
        proto_library,
        name = name + "/null_lib",
    )
    util.helper_target(
        java_lite_proto_library,
        name = name + "/lite_pb2",
        deps = [name + "/null_lib"],
    )
    analysis_test(
        name = name,
        impl = _test_lite_empty_srcs_for_java_api_impl,
        target = name + "/lite_pb2",
    )

def _test_lite_empty_srcs_for_java_api_impl(env, target):
    java_info = target[JavaInfo]
    env.expect.that_collection(_to_list(java_info.compile_jars)).contains_exactly([])
    env.expect.that_collection(_to_list(java_info.source_jars)).contains_exactly([])

# Verifies same version compiler options propagation and ensures transitive runtime jars contain
# correct generated lib and protobuf compiler runtime dependencies.
def _test_lite_same_version_compiler_arguments(name):
    util.helper_target(
        proto_library,
        name = name + "/alpha",
    )
    util.helper_target(
        proto_library,
        name = name + "/bravo",
        srcs = ["bravo.proto"],
        deps = [name + "/alpha"],
    )
    analysis_test(
        name = name,
        impl = _test_lite_same_version_compiler_arguments_impl,
        target = name + "/bravo",
        testing_aspect = _java_lite_proto_aspect_testing_aspect,
    )

def _test_lite_same_version_compiler_arguments_impl(env, target):
    action = env.expect.that_target(target).action_generating(
        "{package}/{test_name}/bravo-lite-src.jar",
    )
    action.argv().contains_at_least([
        # In google3 the command_line flag is "lite,immutable", while in OSS Bazel it is "lite".
        "--java_out=lite:{bindir}/{package}/{test_name}/bravo-lite-src.jar",
        "-I.",
        "{package}/bravo.proto",
    ]).in_order()

    java_info = target[JavaInfo]
    runtime_jars = [f.basename for f in _to_list(java_info.transitive_runtime_jars)]
    env.expect.that_collection(runtime_jars).contains_at_least([
        "libbravo-lite.jar",
        # In google3 the runtime dependency is libprotobuf_lite.jar, while in
        # OSS Bazel it is liblite.jar from the local protobuf build target.
        "liblite.jar",
    ])

# This test is ignored because proguard specs are not yet supported in the
# Starlark version of java_lite_proto_library, or Proguard support might be
# removed from Java rules entirely.
# Verifies proguard specs returned by support compiler libraries.
# buildifier: disable=unused-variable
def _test_lite_exports_proguard_specs_for_support_libraries(name):
    util.helper_target(
        proto_library,
        name = name + "/bar",
    )
    util.helper_target(
        proto_library,
        name = name + "/foo",
        deps = [name + "/bar"],
    )
    util.helper_target(
        java_lite_proto_library,
        name = name + "/lite_pb2",
        deps = [name + "/foo"],
    )
    analysis_test(
        name = name,
        impl = _test_lite_exports_proguard_specs_for_support_libraries_impl,
        target = name + "/lite_pb2",
    )

# buildifier: disable=unused-variable
def _test_lite_exports_proguard_specs_for_support_libraries_impl(env, target):
    proguard_spec_info = target[ProguardSpecInfo]
    basenames = [f.basename for f in proguard_spec_info.specs.to_list()]
    env.expect.that_collection(basenames).contains_exactly(["javalite_runtime.pro_valid"])

# This test is ignored because ExtraActionArtifactsProvider is a legacy
# internal Java-only Bazel feature that is being deprecated and is not
# represented or supported under Starlark rules_testing.
# buildifier: disable=unused-variable
def _test_lite_experimental_proto_extra_actions(name):
    util.helper_target(
        proto_library,
        name = name + "/proto",
        srcs = ["file.proto"],
    )
    util.helper_target(
        java_lite_proto_library,
        name = name + "/lite_pb2",
        deps = [name + "/proto"],
    )
    analysis_test(
        name = name,
        impl = _test_lite_experimental_proto_extra_actions_impl,
        target = name + "/lite_pb2",
    )

# buildifier: disable=unused-variable
def _test_lite_experimental_proto_extra_actions_impl(env, target):
    fail("ExtraActionArtifactsProvider is a Java class that is not accessible in Starlark")

# Verifies that Starlark rules can successfully lookup and inspect `JavaInfo` on target dependencies.
def _test_lite_java_protos_expose_starlark_providers(name):
    util.helper_target(
        proto_library,
        name = name + "/proto",
    )
    util.helper_target(
        java_lite_proto_library,
        name = name + "/lite_pb2",
        deps = [name + "/proto"],
    )
    analysis_test(
        name = name,
        impl = _test_lite_java_protos_expose_starlark_providers_impl,
        target = name + "/lite_pb2",
    )

def _test_lite_java_protos_expose_starlark_providers_impl(env, target):
    env.expect.that_bool(JavaInfo in target).equals(True)

# Verifies that standard proto_library targets interoperate seamlessly under target analysis.
def _test_lite_proto_library_interop(name):
    util.helper_target(
        proto_library,
        name = name + "/proto",
    )
    util.helper_target(
        java_lite_proto_library,
        name = name + "/lite_pb2",
        deps = [name + "/proto"],
    )
    analysis_test(
        name = name,
        impl = _test_lite_proto_library_interop_impl,
        target = name + "/lite_pb2",
    )

# buildifier: disable=unused-variable
def _test_lite_proto_library_interop_impl(env, target):
    pass

# Verifies that when compiling with strict deps enabled, JPL correctly tracks direct compilation
# jars of the aliased target on the compile classpath.
def _test_lite_jpl_correctly_defines_direct_jars_strict_deps_enabled_alias_proto(name):
    util.helper_target(
        proto_library,
        name = name + "/bar_proto",
        srcs = ["bar.proto"],
    )
    util.helper_target(
        proto_library,
        name = name + "/foo_proto",
        deps = [name + "/bar_proto"],
    )
    util.helper_target(
        java_lite_proto_library,
        name = name + "/foo_java_proto_lite",
        deps = [name + "/foo_proto"],
    )
    analysis_test(
        name = name,
        impl = _test_lite_jpl_correctly_defines_direct_jars_strict_deps_enabled_alias_proto_impl,
        target = name + "/foo_java_proto_lite",
    )

def _test_lite_jpl_correctly_defines_direct_jars_strict_deps_enabled_alias_proto_impl(env, target):
    java_info = target[JavaInfo]
    test_name = target.label.name.split("/")[0]
    expected = ["%s/%s/libbar_proto-lite-hjar.jar" % (target.label.package, test_name)]
    env.expect.that_collection([f.short_path for f in _to_list(java_info.compile_jars)]).contains_exactly(expected)

# This test is ignored because it is obsolete and needs to be systematized
# with its new version (b/216484418).
# Verifies compiled direct jars when strict deps are globally disabled.
# buildifier: disable=unused-variable
def _test_lite_jpl_correctly_defines_direct_jars_strict_deps_disabled(name):
    util.helper_target(
        proto_library,
        name = name + "/baz",
        srcs = ["baz.proto"],
    )
    util.helper_target(
        proto_library,
        name = name + "/bar",
        srcs = ["bar.proto"],
        deps = [name + "/baz"],
    )
    util.helper_target(
        java_lite_proto_library,
        name = name + "/bar_lite_pb",
        deps = [name + "/bar"],
    )
    util.helper_target(
        proto_library,
        name = name + "/foo",
        srcs = ["foo.proto"],
        deps = [name + "/bar"],
    )
    util.helper_target(
        java_lite_proto_library,
        name = name + "/foo_lite_pb",
        deps = [name + "/foo"],
    )
    analysis_test(
        name = name,
        impl = _test_lite_jpl_correctly_defines_direct_jars_strict_deps_disabled_impl,
        target = name + "/foo_lite_pb",
    )

# buildifier: disable=unused-variable
def _test_lite_jpl_correctly_defines_direct_jars_strict_deps_disabled_impl(env, target):
    java_info = target[JavaInfo]
    env.expect.that_collection(_to_list(java_info.compile_jars)).contains_exactly([])

# This test is ignored because the proto_java provider is not yet returned
# from the aspect in the Starlark version of java_lite_proto_library.
# Verifies that the java_lite_proto_library aspect exposes target `proto_java` providers recursively.
# buildifier: disable=unused-variable
def _test_lite_java_lite_proto_library_aspect_providers(name):
    util.helper_target(
        proto_library,
        name = name + "/foo_proto",
        srcs = ["foo.proto"],
    )
    util.helper_target(
        java_lite_proto_library,
        name = name + "/foo_java_proto",
        deps = [name + "/foo_proto"],
    )
    util.helper_target(
        foo_rule,
        name = name + "/foo_rule",
        dep = name + "/foo_java_proto",
    )
    analysis_test(
        name = name,
        impl = _test_lite_java_lite_proto_library_aspect_providers_impl,
        target = name + "/foo_rule",
    )

# buildifier: disable=unused-variable
def _test_lite_java_lite_proto_library_aspect_providers_impl(env, target):
    env.expect.that_bool(target[MyInfo].result).equals(True)

def java_lite_proto_library_test_suite(name):
    test_suite(
        name = name,
        tests = [
            _test_lite_binary_deps,
            _test_lite_java_proto2_compiler_args,
            _test_lite_proto_library_builds_compiled_jar,
            _test_lite_command_line_contains_target_label,
            _test_lite_empty_srcs_for_java_api,
            _test_lite_same_version_compiler_arguments,
            # TODO: Enable this test when proguard specs are supported in the Starlark version of
            # java_lite_proto_library OR delete this if Proguard support will be removed.
            # _test_lite_exports_proguard_specs_for_support_libraries,
            # TODO: ExtraActionArtifactsProvider is a legacy internal Java-only Bazel feature that is being
            # deprecated and is not represented/supported under Starlark rules_testing.
            # _test_lite_experimental_proto_extra_actions,
            _test_lite_java_protos_expose_starlark_providers,
            _test_lite_proto_library_interop,
            _test_lite_jpl_correctly_defines_direct_jars_strict_deps_enabled_alias_proto,
            # TODO: Systematize this test with its new version.
            # _test_lite_jpl_correctly_defines_direct_jars_strict_deps_disabled,
            # TODO: Enable this test when proto_java is returned from the aspect in Starlark
            # version of java_lite_proto_library.
            # _test_lite_java_lite_proto_library_aspect_providers,
        ],
    )
