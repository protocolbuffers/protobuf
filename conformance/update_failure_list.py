#!/usr/bin/env python
# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Script to update a failure list file to add/remove failures.

This is sort of like comm(1), except it recognizes comments and ignores them.
"""

import argparse

parser = argparse.ArgumentParser(
    description='Adds/removes failures from the failure list.')
parser.add_argument('filename', type=str, help='failure list file to update')
parser.add_argument('--add', dest='add_list', action='append')
parser.add_argument('--remove', dest='remove_list', action='append')

args = parser.parse_args()

add_set = set()
remove_set = set()

for add_file in (args.add_list or []):
  with open(add_file) as f:
    for line in f:
      add_set.add(line)

for remove_file in (args.remove_list or []):
  with open(remove_file) as f:
    for line in f:
      if line in add_set:
        raise Exception("Asked to both add and remove test: " + line)
      remove_set.add(line.strip())

add_list = sorted(add_set, reverse=True)

with open(args.filename) as in_file:
    existing_list = in_file.read()

with open(args.filename, "w") as f:
  for line in existing_list.splitlines(True):
    test = line.split("#")[0].strip()
    while len(add_list) > 0 and test > add_list[-1]:
      f.write(add_list.pop())
    if test not in remove_set:
      f.write(line)
