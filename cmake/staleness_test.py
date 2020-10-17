"""The py_test() script for generated_file_staleness_test() rules.

Note that this file is preprocessed!  The INSERT_<...> text below is replaced
with the actual list of files before we actually run the script.
"""

from __future__ import absolute_import

from cmake import staleness_test_lib
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
  staleness_test_lib.FixFiles(config)
else:
  unittest.main()
