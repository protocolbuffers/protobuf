"""Tests that strict deps is enabled when compiling proto generated Java code."""

load("@rules_java//java/common:java_info.bzl", "JavaInfo")
load("@rules_testing//lib:truth.bzl", "matching")

def _get_direct_dependencies(argv):
    direct_deps = []
    in_direct_deps = False
    for arg in argv:
        if arg == "--direct_dependencies":
            in_direct_deps = True
            continue
        if in_direct_deps:
            if arg.startswith("--"):
                in_direct_deps = False
            else:
                direct_deps.append(arg)
    return direct_deps

def _output_dir(action, basename):
    for f in action.outputs.to_list():
        if f.basename == basename:
            return f.dirname
    fail("action has no output named " + basename)

def _test_generated_code_is_compiled_with_strict_deps(env, target):
    foo_action = env.expect.that_target(target).action_generating("{package}/libfoo_proto-speed.jar")
    foo_direct_deps = _get_direct_dependencies(foo_action.actual.argv)

    # baz_proto has no proto deps, so the direct dependencies of its compile
    # action are exactly the jars of the proto toolchain's runtime. This keeps
    # the test independent of the runtime used in each environment.
    baz_action = env.expect.that_target(env.ctx.attr.baz_proto).action_generating(
        "{package}/libbaz_proto-speed.jar",
    )
    runtime_direct_deps = _get_direct_dependencies(baz_action.actual.argv)
    env.expect.that_collection(runtime_direct_deps).contains_predicate(
        matching.custom("runtime header jar", lambda d: d.endswith("-hjar.jar")),
    )
    pkg_bindir = _output_dir(baz_action.actual, "libbaz_proto-speed.jar") + "/"
    env.expect.that_collection(
        [d for d in runtime_direct_deps if d.startswith(pkg_bindir)],
    ).contains_exactly([])

    # Only the direct proto dependency (bar) and the runtime are direct
    # dependencies; the transitive proto dependency (baz) is not.
    bar_hjar = _output_dir(foo_action.actual, "libfoo_proto-speed.jar") + "/libbar_proto-speed-hjar.jar"
    env.expect.that_collection(foo_direct_deps).contains_exactly(
        [bar_hjar] + runtime_direct_deps,
    )

def _test_java_proto_library_analyzes(env, target):
    env.expect.that_target(target).has_provider(JavaInfo)
    env.expect.that_target(target).default_outputs().contains("{package}/libfoo_proto-speed.jar")

TESTS = {
    ":foo_proto": [_test_generated_code_is_compiled_with_strict_deps],
    ":foo_java_proto": [_test_java_proto_library_analyzes],
}
