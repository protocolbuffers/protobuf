# -*- coding: utf-8 -*-
# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Unittest for reflection.py, with C++ protos linked in."""

import copy
import unittest

from google.protobuf.internal import testing_refleaks
from absl.testing import parameterized
from google.protobuf import unittest_pb2
from google.protobuf import unittest_proto3_arena_pb2


@parameterized.named_parameters(
    ('_proto2', unittest_pb2), ('_proto3', unittest_proto3_arena_pb2)
)
@testing_refleaks.TestCase
class ReflectionTest(unittest.TestCase):

  def testEmptyCompositeContainerDeepCopy(self, message_module):
    proto1 = message_module.NestedTestAllTypes(
        payload=message_module.TestAllTypes(optional_string='foo')
    )
    nested2 = copy.deepcopy(proto1.child.repeated_child)
    self.assertEqual(0, len(nested2))


if __name__ == '__main__':
  unittest.main()
