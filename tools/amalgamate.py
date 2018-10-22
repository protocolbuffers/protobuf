#!/usr/bin/python

import sys
import re

INCLUDE_RE = re.compile('^#include "([^"]*)"$')

def parse_include(line):
  match = INCLUDE_RE.match(line)
  return match.groups()[0] if match else None

class Amalgamator:
  def __init__(self, include_path, output_path):
    self.include_path = include_path
    self.included = set(["upb/port_def.inc", "upb/port_undef.inc"])
    self.output_h = open(output_path + "upb.h", "w")
    self.output_c = open(output_path + "upb.c", "w")

    self.output_c.write("// Amalgamated source file\n")
    self.output_c.write('#include "upb.h"\n')
    self.output_c.write(open("upb/port_def.inc").read())

    self.output_h.write("// Amalgamated source file\n")
    self.output_h.write(open("upb/port_def.inc").read())

  def finish(self):
    self.output_c.write(open("upb/port_undef.inc").read())
    self.output_h.write(open("upb/port_undef.inc").read())

  def _process_file(self, infile_name, outfile):
    for line in open(infile_name):
      include = parse_include(line)
      if include is not None and (include.startswith("upb") or
                                  include.startswith("google")):
        if include not in self.included:
          self.included.add(include)
          self._add_header(self.include_path + include)
      else:
        outfile.write(line)

  def _add_header(self, filename):
    self._process_file(filename, self.output_h)

  def add_src(self, filename):
    self._process_file(filename, self.output_c)

# ---- main ----

include_path = sys.argv[1]
output_path = sys.argv[2]
amalgamator = Amalgamator(include_path, output_path)

for filename in sys.argv[3:]:
  amalgamator.add_src(filename.strip())

amalgamator.finish()
