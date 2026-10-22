"""Tests that mixing provided and non-provided protos in one proto_library is an error.

java_proto_library implicitly depends on a list of .proto files for which it
should not generate code (the toolchain's `denylisted_protos`), because they're
already compiled into the proto runtime. A proto_library whose `srcs` contain
both such a provided .proto and a non-provided .proto cannot be handled and must
be reported as an error.
"""

load("@rules_testing//lib:truth.bzl", "matching")

def _test_mixing_provided_and_not_provided_is_an_error(env, target):
    pkg = target.label.package
    env.expect.that_target(target).failures().contains_predicate(
        matching.str_matches(
            "*The 'srcs' attribute of '*//{pkg}:mixed_proto' contains protos for which ".format(pkg = pkg) +
            "'java_proto_library' shouldn't generate code ({pkg}/provided.proto), ".format(pkg = pkg) +
            "in addition to protos for which it should ({pkg}/non_provided.proto).\n".format(pkg = pkg) +
            "Separate '*//{pkg}:mixed_proto' into 2 proto_library rules.*".format(pkg = pkg),
        ),
    )

TESTS = {
    ":mixed_java_proto": [_test_mixing_provided_and_not_provided_is_an_error],
}
