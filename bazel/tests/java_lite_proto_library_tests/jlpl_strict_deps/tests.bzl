"""Tests that strict deps are enforced by default on java_lite_proto_library rules.

A java_library depending on a java_lite_proto_library must only get the header
jar of the proto_library it directly wraps as a direct dependency, not the
header jars of the proto_library's transitive deps.
"""

load("@rules_testing//lib:util.bzl", "TestingAspectInfo")

def _flag_values(argv, flag):
    """Returns all values following `flag` in `argv`, up to the next flag."""
    values = []
    collecting = False
    for arg in argv:
        if arg == flag:
            collecting = True
        elif arg.startswith("--"):
            collecting = False
        elif collecting:
            values.append(arg)
    return values

def _test_jlpl_strict_deps(env, target):
    action = env.expect.that_target(target).action_generating("{package}/libjavalib.jar")
    direct_dependencies = env.expect.that_collection(
        _flag_values(action.actual.argv, "--direct_dependencies"),
        expr = "--direct_dependencies",
    )
    hjar_dir = "{}/{}".format(target[TestingAspectInfo].bin_path, target.label.package)
    direct_dependencies.contains(hjar_dir + "/libdirect_proto-lite-hjar.jar")

    # strict deps on java_lite_proto_library rules are enforced by default
    direct_dependencies.not_contains(hjar_dir + "/libindirect_proto-lite-hjar.jar")

TESTS = {
    ":javalib": [_test_jlpl_strict_deps],
}
