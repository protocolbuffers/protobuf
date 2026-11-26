# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

import difflib
import os
import pathlib
import sys


fix = sys.argv[-1:] == ["--fix"]
directory = pathlib.Path(__file__).absolute().parent
generated = [
    pathlib.Path(path)
    for argument in sys.argv[1 : -1 if fix else None]
    for path in argument.split()
]
if not generated:
    generated = list(directory.glob("*_pb2.pyi"))
assert generated

for path in generated:
    compile(path.read_text(), str(path), "exec")

actual = {
    path.name + ".golden": path.read_text()
    for path in generated
    if path.absolute().parent == directory
}
assert actual

if fix:
    workspace = pathlib.Path(os.environ.get("BUILD_WORKSPACE_DIRECTORY", os.getcwd()))
    destination = workspace / "python" / "pyi_test"
    for path in destination.glob("*_pb2.pyi.golden"):
        if path.name not in actual:
            path.unlink()
    for name, content in actual.items():
        (destination / name).write_text(content)
    sys.exit(0)

expected = {path.name: path.read_text() for path in directory.glob("*_pb2.pyi.golden")}
if actual.keys() != expected.keys():
    missing = sorted(actual.keys() - expected.keys())
    orphaned = sorted(expected.keys() - actual.keys())
    if missing:
        print("missing goldens: " + ", ".join(missing), file=sys.stderr)
    if orphaned:
        print("orphaned goldens: " + ", ".join(orphaned), file=sys.stderr)
    sys.exit(1)

different = False
for name in sorted(actual):
    if actual[name] == expected[name]:
        continue
    different = True
    sys.stderr.writelines(
        difflib.unified_diff(
            expected[name].splitlines(keepends=True),
            actual[name].splitlines(keepends=True),
            fromfile=name,
            tofile=name.removesuffix(".golden"),
        )
    )
if different:
    print("fix with: bazel run //python/pyi_test:golden_test -- --fix", file=sys.stderr)
    sys.exit(1)
