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
from google.protobuf import text_format

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
    self.assertEqual(123, msg.optional_int32)  # pyrefly: ignore[missing-attribute]

  def test_parse_max_recursion_depth(self, message_module):
    text = 'child { child { child {} } }'
    self.assertRaisesRegex(
        text_format.ParseError,
        'Message too deep. Max recursion depth is 3',
        proto_text.parse,
        message_module.NestedTestAllTypes,
        text,
        max_recursion_depth=3,
    )
    msg = proto_text.parse(
        message_module.NestedTestAllTypes, text, max_recursion_depth=4
    )
    self.assertTrue(msg.HasField('child'))

    self.assertRaisesRegex(
        text_format.ParseError,
        'Message too deep. Max recursion depth is 100',
        proto_text.parse,
        message_module.NestedTestAllTypes,
        'child {' * 100 + '}' * 100,
    )
    msg = proto_text.parse(
        message_module.NestedTestAllTypes, 'child {' * 99 + '}' * 99
    )
    self.assertTrue(msg.HasField('child'))


if __name__ == '__main__':
  unittest.main()
