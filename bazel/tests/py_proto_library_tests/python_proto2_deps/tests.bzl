"""Tests that py_proto_library depends on the python proto2 library."""

def _test_python_proto2_deps(env, target):
    runfiles_paths = [f.basename for f in target[DefaultInfo].default_runfiles.files.to_list()]

    env.expect.that_collection(runfiles_paths).contains("message.py")

TESTS = [
    (_test_python_proto2_deps, ":proto2_deps_bin"),
]
