# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd
"""Tests for EnumTypeWrapper."""

__author__ = "kmonte@google.com (Kyle Montemayor)"

import types
import unittest

from google.protobuf.internal import enum_type_wrapper

from google.protobuf import unittest_pb2


class EnumTypeWrapperTest(unittest.TestCase):

  def test_type_union(self):
    enum_type = enum_type_wrapper.EnumTypeWrapper(
        unittest_pb2.TestAllTypes.NestedEnum.DESCRIPTOR
    )
    union_type = enum_type | int
    self.assertIsInstance(union_type, types.UnionType)

    def get_union() -> union_type:
      return enum_type

    union = get_union()
    self.assertIsInstance(union, enum_type_wrapper.EnumTypeWrapper)
    self.assertEqual(
        union.DESCRIPTOR, unittest_pb2.TestAllTypes.NestedEnum.DESCRIPTOR
    )


if __name__ == "__main__":
  unittest.main()
