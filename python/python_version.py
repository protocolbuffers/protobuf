# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd
"""Test that Kokoro is using the expected version of python."""

import os
import sys
import unittest


class PythonVersionTest(unittest.TestCase):

  def testPython3(self):
    """Test that we can import nested import public messages."""

    exp = os.getenv('KOKORO_PYTHON_VERSION', '')
    if not exp:
      print('No kokoro python version found, skipping check', file=sys.stderr)
      return
    self.assertTrue(
        sys.version.startswith(exp),
        'Expected Python %s but found Python %s' % (exp, sys.version))


if __name__ == '__main__':
  unittest.main()
