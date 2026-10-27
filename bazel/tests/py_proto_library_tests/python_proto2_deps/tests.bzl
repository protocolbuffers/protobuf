# Protocol Buffers - Google's data interchange format
# Copyright 2026 Google LLC
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Tests that py_proto_library depends on the python proto2 library."""

def _test_python_proto2_deps(env, target):
    runfiles_paths = [f.basename for f in target[DefaultInfo].default_runfiles.files.to_list()]

    env.expect.that_collection(runfiles_paths).contains("message.py")

TESTS = {
    ":proto2_deps_bin": [_test_python_proto2_deps],
}
