# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

import difflib
import os
import pathlib
import subprocess
import sys


fix = sys.argv[1:] == ["--fix"]
assert not sys.argv[1:] or fix

directory = pathlib.Path(__file__).absolute().parent
root = directory.parents[1]
runfiles = root.parent


def runfile(pattern):
    matches = list(runfiles.glob(pattern))
    assert len(matches) == 1, f"expected one runfile for {pattern}: {matches}"
    return matches[0]


source = directory / "typing_test.py"
environment = os.environ.copy()
environment.pop("PYTHONPATH", None)
environment["NODE_NO_WARNINGS"] = "1"

commands = {
    "mypy": [
        directory / "mypy_bin",
        "--config-file",
        directory / "mypy.ini",
        "--no-error-summary",
        "--no-color-output",
        "--cache-dir=/dev/null",
        source,
    ],
    "ty": [
        runfile("*/tools/ty/ty"),
        "check",
        "--config-file",
        directory / "ty.toml",
        "--output-format",
        "concise",
        "--no-progress",
        "--color",
        "never",
        "--extra-search-path",
        root,
        "--extra-search-path",
        root / "python",
        source,
    ],
    "pyright": [
        runfile("*/bin/nodejs/bin/node"),
        runfile("*pyright/index.js"),
        "--project",
        directory / "pyrightconfig.json",
        source,
    ],
    "pyrefly": [
        runfile("*/tools/pyrefly/pyrefly"),
        "check",
        "--preset",
        "strict",
        "--config",
        directory / "pyrefly.toml",
        "--output-format",
        "min-text",
        "--summary=none",
        "--progress-bar",
        "no",
        "--color",
        "never",
        source,
    ],
}

actual = {}
for checker, command in commands.items():
    checker_environment = environment.copy()
    if checker == "mypy":
        checker_environment["MYPYPATH"] = str(root)
        checker_environment["PYTHONNOUSERSITE"] = "true"
    result = subprocess.run(
        command,
        capture_output=True,
        cwd=root,
        env=checker_environment,
        text=True,
    )
    output = (result.stdout + result.stderr).replace("\r\n", "\n").replace("\u00a0", " ")
    for prefix in (str(root.absolute()) + "/", str(root.resolve()) + "/"):
        output = output.replace(prefix, "")
    if result.returncode != 1:
        print(output, end="", file=sys.stderr)
        raise AssertionError(f"{checker} exited {result.returncode}, expected 1")
    actual[checker + ".golden"] = output

if fix:
    workspace = pathlib.Path(os.environ.get("BUILD_WORKSPACE_DIRECTORY", os.getcwd()))
    destination = workspace / "python" / "pyi_test"
    for name, content in actual.items():
        (destination / name).write_text(content)
    sys.exit(0)

different = False
for name, output in actual.items():
    expected = (directory / name).read_text()
    if output == expected:
        continue
    different = True
    sys.stderr.writelines(
        difflib.unified_diff(
            expected.splitlines(keepends=True),
            output.splitlines(keepends=True),
            fromfile=name,
            tofile=name.removesuffix(".golden"),
        )
    )
if different:
    print(
        "fix with: bazel run //python/pyi_test:typecheck_golden_test -- --fix",
        file=sys.stderr,
    )
    sys.exit(1)
