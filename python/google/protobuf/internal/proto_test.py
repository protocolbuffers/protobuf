# -*- coding: utf-8 -*-
# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Tests Nextgen Pythonic protobuf APIs."""

import io
import unittest

from google.protobuf import proto
from google.protobuf.internal import encoder
from google.protobuf.internal import test_util
from google.protobuf.internal import testing_refleaks

from google.protobuf.internal import _parameterized
from google.protobuf import unittest_pb2
from google.protobuf import unittest_proto3_arena_pb2

@_parameterized.named_parameters(('_proto2', unittest_pb2),
                                ('_proto3', unittest_proto3_arena_pb2))
@testing_refleaks.TestCase
class ProtoTest(unittest.TestCase):

  def test_serialize_parse(self, message_module):
    msg = message_module.TestAllTypes()
    test_util.SetAllFields(msg)
    serialized_data = proto.serialize(msg)
    parsed_msg = proto.parse(message_module.TestAllTypes, serialized_data)
    self.assertEqual(msg, parsed_msg)

  def test_serialize_parse_length_prefixed(self, message_module):
    number_of_messages = 10

    out = io.BytesIO()
    for index in range(0, number_of_messages):
      msg = message_module.TestAllTypes(
          optional_int32=index, optional_string='hello'
      )
      proto.serialize_length_prefixed(msg, out)

    input_bytes = io.BytesIO(out.getvalue())
    index = 0
    while True:
      msg = proto.parse_length_prefixed(
          message_module.TestAllTypes, input_bytes
      )
      if msg is None:
        break
      self.assertEqual(msg.optional_int32, index)  # pytype: disable=attribute-error
      self.assertEqual(msg.optional_string, 'hello')  # pytype: disable=attribute-error
      index += 1

    self.assertEqual(index, number_of_messages)

  def test_serialize_parse_length_prefixed_empty(self, message_module):
    number_of_messages = 10

    empty_alltypes = message_module.TestAllTypes()
    out = io.BytesIO()
    for index in range(0, number_of_messages):
      proto.serialize_length_prefixed(empty_alltypes, out)

    input_bytes = io.BytesIO(out.getvalue())
    index = 0
    while True:
      msg = proto.parse_length_prefixed(
          message_module.TestAllTypes, input_bytes
      )
      if msg is None:
        break
      self.assertEqual(msg, empty_alltypes)
      index += 1

    self.assertEqual(index, number_of_messages)

  def test_serialize_parse_length_prefixed_bytes(self, message_module):
    out = io.BytesIO()
    msg = message_module.TestAllTypes(
        optional_int32=123, optional_string='hello'
    )
    proto.serialize_length_prefixed(msg, out)
    input_bytes = out.getvalue()
    parsed_msg = proto.parse_length_prefixed(
        message_module.TestAllTypes, input_bytes
    )
    self.assertEqual(msg, parsed_msg)

  def test_parse_length_prefixed_truncated(self, message_module):
    out = io.BytesIO()
    encoder._VarintEncoder()(out.write, 9999)
    msg = message_module.TestAllTypes(optional_int32=1)
    out.write(proto.serialize(msg))

    input_bytes = io.BytesIO(out.getvalue())
    with self.assertRaises(ValueError) as context:
      proto.parse_length_prefixed(message_module.TestAllTypes, input_bytes)
      self.assertEqual(
          str(context.exception),
          'Truncated message or non-buffered input_bytes: '
          'Expected 9999 bytes but only 2 bytes parsed for '
          'TestAllTypes.',
      )


if __name__ == '__main__':
  unittest.main()
