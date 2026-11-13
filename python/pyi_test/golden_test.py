# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

import argparse
import difflib
import os
import pathlib
import sys


parser = argparse.ArgumentParser()
parser.add_argument("--fix", action="store_true")
parser.add_argument("--print-diffs", action="store_true")
parser.add_argument("generated", nargs="*")
args = parser.parse_args()
directory = pathlib.Path(__file__).absolute().parent
generated = [
    pathlib.Path(path)
    for argument in args.generated
    for path in argument.split()
]
if args.fix and not generated:
    # regenerate_stale_files.sh invokes the built executable without Bazel args.
    generated = list(directory.glob("*_pb2.pyi"))

for path in generated:
    compile(path.read_text(), str(path), "exec")

actual = {
    path.name + ".golden": path.read_text()
    for path in generated
    if path.absolute().parent == directory
}
if not actual:
    parser.error("no generated fixture stubs were supplied")

expected = {path.name: path.read_text() for path in directory.glob("*_pb2.pyi.golden")}
if not args.fix and actual.keys() != expected.keys():
    missing = sorted(actual.keys() - expected.keys())
    orphaned = sorted(expected.keys() - actual.keys())
    if missing:
        print("missing goldens: " + ", ".join(missing), file=sys.stderr)
    if orphaned:
        print("orphaned goldens: " + ", ".join(orphaned), file=sys.stderr)
    sys.exit(1)

different = False
for name in sorted(actual.keys() | expected.keys()):
    if actual.get(name) == expected.get(name):
        continue
    different = True
    if args.fix and not args.print_diffs:
        continue
    sys.stderr.writelines(
        difflib.unified_diff(
            expected.get(name, "").splitlines(keepends=True),
            actual.get(name, "").splitlines(keepends=True),
            fromfile=name,
            tofile=name.removesuffix(".golden"),
        )
    )
if args.fix:
    workspace = pathlib.Path(os.environ.get("BUILD_WORKSPACE_DIRECTORY", os.getcwd()))
    destination = workspace / "python" / "pyi_test"
    for path in destination.glob("*_pb2.pyi.golden"):
        if path.name not in actual:
            path.unlink()
    for name, content in actual.items():
        (destination / name).write_text(content)
    sys.exit(0)

if different:
    print("fix with: bazel run //python/pyi_test:golden_test -- --fix", file=sys.stderr)
    sys.exit(1)
