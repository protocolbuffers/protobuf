#!/usr/bin/python

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
    self.output_c.write(open("upb/port_def.inc").read())

    self.output_h.write("/* Amalgamated source file */\n")
    self.output_h.write('#include <stdint.h>')
    self.output_h.write(open("upb/port_def.inc").read())

  def add_include_path(self, path):
      self.include_paths.append(path)

  def finish(self):
    self.output_c.write(open("upb/port_undef.inc").read())
    self.output_h.write(open("upb/port_undef.inc").read())

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

    for line in file:
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
