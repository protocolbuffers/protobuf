"""Tests that java_proto_library propagates the native libraries of the proto runtime.

The `JavaInfo` of a java_proto_library carries the native (C++) libraries that
its Java code needs at runtime. They must all come from the toolchain's runtime,
and none from the proto_library deps.
"""

load("@rules_java//java/common:java_info.bzl", "JavaInfo")

# Gives the test access to the toolchain's runtime, to compare its native
# libraries with those of the java_proto_library.
TEST_ATTRS = {
    "runtime": {
        "@attr": attr.label,
        "default": Label(":java_runtime"),
        "mandatory": False,
    },
}

def _native_library_owners(java_info):
    owners = []
    for linker_input in java_info.cc_link_params_info.linking_context.linker_inputs.to_list():
        for library in linker_input.libraries:
            # Which artifacts a library has depends on the configuration (e.g. PIC).
            artifact = library.static_library or library.pic_static_library or library.dynamic_library
            owners.append(artifact.owner)
    return owners

def _test_java_proto_library_propagates_java_cc_info(env, target):
    runtime = env.ctx.attr.runtime

    # Attributes with a configuration transition are lists.
    if type(runtime) == "list":
        runtime = runtime[0]
    runtime_libraries = _native_library_owners(runtime[JavaInfo])
    env.expect.that_collection(runtime_libraries).contains(Label(":native_dep_of_runtime"))

    # Besides the runtime's native dependency, this may include runtime
    # libraries that the C++ toolchain adds to every cc_library.
    env.expect.that_collection(_native_library_owners(target[JavaInfo])).contains_exactly(runtime_libraries)

TESTS = {
    ":foo_java_proto": [_test_java_proto_library_propagates_java_cc_info],
}
