# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd
"""Test that test runner is using the expected version of python."""

import os
import sys
import unittest
import platform


class PythonVersionTest(unittest.TestCase):

  def testPython3(self):
    """Test that we can import nested import public messages."""

    exp = os.getenv('KOKORO_PYTHON_VERSION', '')

    print(f':::: platform.uname: {platform.uname()}')
    print(f':::: platform.platform: {platform.platform()}')
    print(f':::: platform.python_branch: {platform.python_branch()}')
    print(f':::: platform.python_build: {platform.python_build()}')
    print(f':::: platform.python_compiler: {platform.python_compiler()}')
    print(f':::: platform.python_implementation: {platform.python_implementation()}')
    print(f':::: platform.python_revision: {platform.python_revision()}')
    print(f':::: platform.python_version: {platform.python_version()}')
    #print(f':::: sys.byte_order: {sys.byte_order}')
    print(f':::: sys.executable: {sys.executable}')
    print(f':::: sys.path: {sys.path}')
    print(f':::: sys.path_hooks: {sys.path_hooks}')
    print(f':::: sys.path_importer_cache: {sys.path_importer_cache}')
    print(f':::: sys.platform: {sys.platform}')
    print(f':::: sys.version: {sys.version}')
    print(f':::: sys.version_info: {sys.version_info}')

    if not exp:
      print('No system python version found, skipping check', file=sys.stderr)
      return
    self.assertTrue(
        sys.version.startswith(exp),
        'Expected Python %s but found Python %s' % (exp, sys.version))


if __name__ == '__main__':
  unittest.main()
