# Protocol Buffers - Google's data interchange format
# Copyright 2026 Google LLC
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Test suite for py_proto_library that collects Python files from deps when srcs is empty."""

def test_collects_python_files_from_deps_when_srcs_is_empty(env, target):
    runfiles_paths = [f.basename for f in target[DefaultInfo].default_runfiles.files.to_list()]

    expected_basename = "b_pb2.py"
    env.expect.that_collection(runfiles_paths).contains(expected_basename)

TESTS = {
    ":a_py_pb2": [test_collects_python_files_from_deps_when_srcs_is_empty],
}
