# -*- coding: utf-8 -*-
# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Tests Pythonic protobuf APIs for text format"""

import unittest

from google.protobuf import proto_text

from absl.testing import parameterized
from google.protobuf import unittest_pb2
from google.protobuf import unittest_proto3_arena_pb2


@parameterized.parameters(unittest_pb2, unittest_proto3_arena_pb2)
class ProtoTextTest(unittest.TestCase):

  def test_simple_serialize(self, message_module):
    msg = message_module.TestAllTypes()
    msg.optional_int32 = 101
    expected = 'optional_int32: 101\n'
    self.assertEqual(expected, proto_text.serialize(msg))

  def test_simpor_parse(self, message_module):
    text = 'optional_int32: 123'
    msg = proto_text.parse(message_module.TestAllTypes, text)
    self.assertEqual(123, msg.optional_int32)  # pytype: disable=attribute-error


if __name__ == '__main__':
  unittest.main()
