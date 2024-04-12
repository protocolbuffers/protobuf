#!/usr/bin/env python
# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
# https://developers.google.com/protocol-buffers/
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""Generates a friendly list of changes per language since the last release."""

import sys
import os

class Language(object):
  def __init__(self, name, pathspec):
    self.name = name
    self.pathspec = pathspec

languages = [
  Language("C++", [
      "':(glob)src/google/protobuf/*'",
      "src/google/protobuf/compiler/cpp",
      "src/google/protobuf/io",
      "src/google/protobuf/util",
      "src/google/protobuf/stubs",
  ]),
  Language("Java", [
      "java",
      "src/google/protobuf/compiler/java",
  ]),
  Language("Python", [
      "python",
      "src/google/protobuf/compiler/python",
  ]),
  Language("PHP", [
      "php",
      "src/google/protobuf/compiler/php",
  ]),
  Language("Ruby", [
      "ruby",
      "src/google/protobuf/compiler/ruby",
  ]),
  Language("Csharp", [
      "csharp",
      "src/google/protobuf/compiler/csharp",
  ]),
  Language("Objective C", [
      "objectivec",
      "src/google/protobuf/compiler/objectivec",
  ]),
]

if len(sys.argv) < 2:
  print("Usage: generate_changelog.py <previous release>")
  sys.exit(1)

previous = sys.argv[1]

for language in languages:
  print(language.name)
  sys.stdout.flush()
  os.system(("git log --pretty=oneline --abbrev-commit %s...HEAD %s | " +
             "sed -e 's/^/ - /'") % (previous, " ".join(language.pathspec)))
  print("")

print("To view a commit on GitHub: " +
      "https://github.com/protocolbuffers/protobuf/commit/<commit id>")
