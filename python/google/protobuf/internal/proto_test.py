# -*- coding: utf-8 -*-
# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Tests Nextgen Pythonic protobuf APIs."""

import unittest

from google.protobuf import proto

from google.protobuf.internal import test_util
from google.protobuf.internal import testing_refleaks
from google.protobuf.internal import _parameterized
from google.protobuf import unittest_pb2
from google.protobuf import unittest_proto3_arena_pb2

@_parameterized.named_parameters(('_proto2', unittest_pb2),
                                ('_proto3', unittest_proto3_arena_pb2))
@testing_refleaks.TestCase
class ProtoTest(unittest.TestCase):

  def testSerializeParse(self, message_module):
    msg = message_module.TestAllTypes()
    test_util.SetAllFields(msg)
    serialized_data = proto.serialize(msg)
    parsed_msg = proto.parse(message_module.TestAllTypes, serialized_data)
    self.assertEqual(msg, parsed_msg)

if __name__ == '__main__':
  unittest.main()
