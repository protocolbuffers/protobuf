#!/usr/bin/python
#
# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of Google LLC nor the
#       names of its contributors may be used to endorse or promote products
#       derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import sys
import re
import os

INCLUDE_RE = re.compile('^#include "([^"]*)"$')

def parse_include(line):
  match = INCLUDE_RE.match(line)
  return match.groups()[0] if match else None

class Amalgamator:
  def __init__(self, output_path, prefix):
    self.include_paths = ["."]
    self.included = set(["upb/port_def.inc", "upb/port_undef.inc"])
    self.output_h = open(output_path + prefix + "upb.h", "w")
    self.output_c = open(output_path + prefix + "upb.c", "w")

    self.output_c.write("/* Amalgamated source file */\n")
    self.output_c.write('#include "%supb.h"\n' % (prefix))
    if prefix == "ruby-":
      self.output_h.write("// Ruby is still using proto3 enum semantics for proto2\n")
      self.output_h.write("#define UPB_DISABLE_PROTO2_ENUM_CHECKING\n")
    self.output_c.write(open("upb/port_def.inc").read())

    self.output_h.write("/* Amalgamated source file */\n")
    self.output_h.write(open("upb/port_def.inc").read())

  def add_include_path(self, path):
      self.include_paths.append(path)

  def finish(self):
    self._add_header("upb/port_undef.inc")
    self.add_src("upb/port_undef.inc")

  def _process_file(self, infile_name, outfile):
    file = None
    for path in self.include_paths:
        try:
            full_path = os.path.join(path, infile_name)
            file = open(full_path)
            break
        except IOError:
            pass
    if not file:
        raise RuntimeError("Couldn't open file " + infile_name)

    lines = file.readlines()

    has_copyright = lines[1].startswith(" * Copyright")
    if has_copyright:
      while not lines[0].startswith(" */"):
        lines.pop(0)
      lines.pop(0)

    lines.insert(0, "\n/** " + infile_name + " " + ("*" * 60) +"/");

    for line in lines:
      if not self._process_include(line, outfile):
        outfile.write(line)

  def _process_include(self, line, outfile):
    include = parse_include(line)
    if not include:
      return False
    if not (include.startswith("upb") or include.startswith("google")):
      return False
    if include.endswith("hpp"):
      # Skip, we don't support the amalgamation from C++.
      return True
    else:
      # Include this upb header inline.
      if include not in self.included:
        self.included.add(include)
        self._add_header(include)
      return True

  def _add_header(self, filename):
    self._process_file(filename, self.output_h)

  def add_src(self, filename):
    self._process_file(filename, self.output_c)

# ---- main ----

output_path = sys.argv[1]
prefix = sys.argv[2]
amalgamator = Amalgamator(output_path, prefix)
files = []

for arg in sys.argv[3:]:
  arg = arg.strip()
  if arg.startswith("-I"):
    amalgamator.add_include_path(arg[2:])
  elif arg.endswith(".h") or arg.endswith(".inc"):
    pass
  else:
    files.append(arg)

for filename in files:
    amalgamator.add_src(filename)

amalgamator.finish()
