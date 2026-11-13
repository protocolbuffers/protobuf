# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd
"""Test that the protobuf library only adds existing directories to sys.path."""

import os
import sys
import unittest


class SysPathTest(unittest.TestCase):

  def testRunfilesEntriesExist(self):
    # Entries outside the runfiles come from the interpreter itself, which may
    # legitimately list paths that don't exist, such as python3XY.zip.
    runfiles = os.path.abspath(os.environ['TEST_SRCDIR'])
    for entry in sys.path:
      path = os.path.abspath(entry)
      if os.path.commonpath([path, runfiles]) == runfiles:
        self.assertTrue(os.path.isdir(path), path)


if __name__ == '__main__':
  unittest.main()
