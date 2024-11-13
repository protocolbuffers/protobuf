#!/usr/bin/env python
# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Script to update a failure list file to add/remove failures.

When adding, will attempt to place them in their correct lexicographical
position relative to other test names. This requires that the failure list is
already sorted. This does not guarantee that the tests will appear neatly one
after the other, as there may be comments in between. If the failure list
is not sorted, sorted blocks may be produced, but the list as a whole will not.
Lastly, be wary of lists that have tests stripped from OSS; if you catch that
a test was added inside a stripped block, you may need to move it out.

This is sort of like comm(1), except it recognizes comments and ignores them.
"""

import argparse

parser = argparse.ArgumentParser(
    description='Adds/removes failures from the failure list.')
parser.add_argument('filename', type=str, help='failure list file to update')
parser.add_argument('--add', dest='add_list', action='append')
parser.add_argument('--remove', dest='remove_list', action='append')

DEFAULT_ALIGNMENT = 114
args = parser.parse_args()

add_set = set()
remove_set = set()

# Adds test + failure message
for add_file in (args.add_list or []):
  with open(add_file) as f:
    for line in f:
      add_set.add(line)

# We only remove by name
for remove_file in (args.remove_list or []):
  with open(remove_file) as f:
    for line in f:
      if line in add_set:
        raise Exception("Asked to both add and remove test: " + line)
      remove_set.add(line.split('#')[0].strip())

add_list = sorted(add_set, reverse=True)

with open(args.filename) as in_file:
  existing_list = in_file.read()

with open(args.filename, 'w') as f:
  split_lines = existing_list.splitlines(True)
  # Make sure that we have one newline for potential tests that have to be
  # added at the end.
  split_lines[-1] = split_lines[-1].rstrip() + '\n'
  for line in split_lines:
    test = line.split('#')[0].strip()
    # As long as the tests we are adding appear lexicographically before our
    # read test, put them first followed by our read test.
    while add_list and test > add_list[-1]:
      f.write(add_list.pop())
    if test not in remove_set:
      f.write(line)
  # Any remaining tests are added at the end
  while add_list:
    f.write(add_list.pop())

# Update our read of the existing file
with open(args.filename, 'r') as f:
  existing_list = f.read()

# Actual alignment of failure messages to 'DEFAULT_ALIGNMENT'
# If test name exceeds DEFAULT_ALIGNMENT, it cannot and will not be aligned.
with open(args.filename, 'w') as f:
  for line in existing_list.splitlines(True):
    split = line.split('#', 1)
    test = split[0].strip()
    if len(split) > 1 and test:
      message = split[1].lstrip()
      spaces = ' ' * (DEFAULT_ALIGNMENT - len(test))
      line = test + spaces + ' # ' + message
      f.write(line)
    else:  # ignore blank lines/lines that do not have '#'/comments
      f.write(line)
