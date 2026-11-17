# Protocol Buffers - Google's data interchange format
# Copyright 2023 Google LLC.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""The py_test() script for staleness_test() rules.

Note that this file is preprocessed!  The INSERT_<...> text below is replaced
with the actual list of files before we actually run the script.
"""

from __future__ import absolute_import

from upb.cmake import staleness_test_lib
import unittest
import sys

file_list = """
  INSERT_FILE_LIST_HERE
""".split()

config = staleness_test_lib.Config(file_list)


class TestFilesMatch(unittest.TestCase):

  def testFilesMatch(self):
    errors = staleness_test_lib.CheckFilesMatch(config)
    self.assertFalse(errors, errors)


if len(sys.argv) > 1 and sys.argv[1] == "--fix":
  print_diffs = "--print-diffs" in sys.argv
  staleness_test_lib.FixFiles(config, print_diffs=print_diffs)
else:
  unittest.main()
