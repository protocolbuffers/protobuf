# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Bazel support functions related to CMake support."""

def staleness_test(name, outs, generated_pattern, target_files = None, tags = [], **kwargs):
    """Tests that checked-in file(s) match the contents of generated file(s).

    The resulting test will verify that all output files exist and have the
    correct contents.  If the test fails, it can be invoked with --fix to
    bring the checked-in files up to date.

    Args:
      name: Name of the rule.
      outs: the checked-in files that are copied from generated files.
      generated_pattern: the pattern for transforming each "out" file into a
        generated file.  For example, if generated_pattern="generated/%s" then
        a file foo.txt will look for generated file generated/foo.txt.
      target_files: A glob representing all of the files that should be
      covered by this rule.  Files in this glob but not generated will
      be deleted.  (Not currently implemented in OSS).
      **kwargs: Additional keyword arguments to pass through to py_test().
    """

    script_name = name + ".py"
    script_src = Label("//upb/cmake:staleness_test.py")

    # Filter out non-existing rules so Blaze doesn't error out before we even
    # run the test.
    existing_outs = native.glob(include = outs, allow_empty = True)

    # The file list contains a few extra bits of information at the end.
    # These get unpacked by the Config class in staleness_test_lib.py.
    file_list = outs + [generated_pattern, native.package_name() or ".", name]

    native.genrule(
        name = name + "_makescript",
        outs = [script_name],
        srcs = [script_src],
        testonly = 1,
        cmd = "cp $< $@; " +
              "sed -i.bak -e 's|INSERT_FILE_LIST_HERE|" + "\\\n  ".join(file_list) + "|' $@",
    )

    native.py_test(
        name = name,
        srcs = [script_name],
        data = existing_outs + [generated_pattern % file for file in outs],
        python_version = "PY3",
        deps = [
            Label("//upb/cmake:staleness_test_lib"),
        ],
        tags = ["staleness_test"] + tags,
        **kwargs
    )
