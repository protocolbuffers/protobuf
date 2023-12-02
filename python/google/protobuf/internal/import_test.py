# -*- coding: utf-8 -*-
# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Unittest for nested public imports."""

import unittest

from google.protobuf.internal.import_test_package import outer_pb2


class ImportTest(unittest.TestCase):

  def testPackageInitializationImport(self):
    """Test that we can import nested import public messages."""

    msg = outer_pb2.Outer()
    self.assertEqual(58, msg.import_public_nested.value)


if __name__ == '__main__':
  unittest.main()
