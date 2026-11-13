# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

import os
import pathlib
import subprocess
import unittest


directory = pathlib.Path(__file__).absolute().parent
root = directory.parents[1]
environment = os.environ.copy()
environment.pop("PYTHONPATH", None)
node, = (
    root / path
    for path in environment["NODE_FILES"].split()
    if pathlib.Path(path).name == "node"
)

commands = {
    "mypy": [
        root / environment["MYPY"],
        "--config-file",
        directory / "mypy.ini",
        "--no-color-output",
        "--cache-dir=/dev/null",
    ],
    "ty": [
        root / environment["TY"],
        "check",
        "--config-file",
        directory / "ty.toml",
        "--no-progress",
        "--color",
        "never",
        "--extra-search-path",
        root,
        "--extra-search-path",
        root / "python",
    ],
    "pyright": [
        node,
        root / environment["PYRIGHT"],
        "--project",
        directory / "pyrightconfig.json",
    ],
    "pyrefly": [
        root / environment["PYREFLY"],
        "check",
        "--preset",
        "strict",
        "--config",
        directory / "pyrefly.toml",
        "--progress-bar",
        "no",
        "--color",
        "never",
    ],
}


class TypecheckTest(unittest.TestCase):

    def test_typecheck(self):
        positive = directory / "typing_test.py"
        for checker, command in commands.items():
            # Check each invalid program separately so one error cannot mask another.
            for source in [positive, *sorted(directory.glob("invalid_*.py"))]:
                with self.subTest(checker=checker, source=source.name):
                    # Both the workspace and its python/ directory are import roots.
                    # Give mypy the canonical module name rather than an ambiguous path.
                    target = (
                        ["-m", f"python.pyi_test.{source.stem}"]
                        if checker == "mypy"
                        else [source]
                    )
                    result = subprocess.run(
                        command + target,
                        capture_output=True,
                        cwd=root,
                        env=environment,
                        text=True,
                    )
                    self.assertEqual(
                        result.returncode,
                        0 if source == positive else 1,
                        result.stdout + result.stderr,
                    )


if __name__ == "__main__":
    unittest.main()
