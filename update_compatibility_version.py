#!/usr/bin/env python
"""Compatibility tests between last released and the current version.

Usage: ./update_compatibility_version.py <MAJOR>.<MINOR>.<MICRO> [<RC version>]
Example: ./update_compatibility_version.py 3.7.1
"""

from __future__ import print_function
import re
import sys

if len(sys.argv) < 2 or len(sys.argv) > 3:
  print("""
[ERROR] Please specify a version.

./update_compatibility_version.py.py <MAJOR>.<MINOR>.<MICRO> [<RC version>]

Example:
./update_compatibility_version.py.py 3.7.1 2
""")
  exit(1)

NEW_VERSION = sys.argv[1]
NEW_VERSION_INFO = NEW_VERSION.split('.')
if len(NEW_VERSION_INFO) != 3:
  print("""
[ERROR] Version must be in the format <MAJOR>.<MINOR>.<MICRO>

Example:
./update_compatibility_version.py.py 3.7.3
""")
  exit(1)

if len(sys.argv) > 2:
  RC_VERSION = int(sys.argv[2])
  # Do not update compatibility versions for rc release
  if RC_VERSION != 0:
    exit(0)


def RewriteTextFile(filename, line_rewriter):
  lines = open(filename, 'r').readlines()
  updated_lines = []
  for line in lines:
    updated_lines.append(line_rewriter(line))
  if lines == updated_lines:
    print('%s was not updated. Please double check.' % filename)
  f = open(filename, 'w')
  f.write(''.join(updated_lines))
  f.close()


def ReplaceVersion(line):
  return re.sub(r'LAST_RELEASED=.*$', 'LAST_RELEASED=%s' % NEW_VERSION, line)

RewriteTextFile('tests.sh', ReplaceVersion)
