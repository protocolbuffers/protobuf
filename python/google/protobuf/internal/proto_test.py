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
from google.protobuf.internal import test_proto2_pb2
from google.protobuf.internal import test_util
from google.protobuf.internal import testing_refleaks

from absl.testing import parameterized
from google.protobuf import unittest_pb2
from google.protobuf import unittest_proto3_arena_pb2


@parameterized.named_parameters(
    ('_proto2', unittest_pb2),
    ('_proto3', unittest_proto3_arena_pb2),
)
@testing_refleaks.TestCase
class ProtoTest(unittest.TestCase):

  def test_simple_serialize_parse(self, message_module):
    msg = message_module.TestAllTypes()
    test_util.SetAllFields(msg)
    serialized_data = proto.serialize(msg)
    parsed_msg = proto.parse(message_module.TestAllTypes, serialized_data)
    self.assertEqual(msg, parsed_msg)

  def test_serialize_parse_length_prefixed_empty(self, message_module):
    empty_alltypes = message_module.TestAllTypes()
    out = io.BytesIO()
    proto.serialize_length_prefixed(empty_alltypes, out)

    input_bytes = io.BytesIO(out.getvalue())
    msg = proto.parse_length_prefixed(message_module.TestAllTypes, input_bytes)

    self.assertEqual(msg, empty_alltypes)

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

  def test_serialize_length_prefixed_fake_io(self, message_module):
    class FakeBytesIO(io.BytesIO):

      def write(self, b: bytes) -> int:
        return 0

    msg = message_module.TestAllTypes(optional_int32=123)
    out = FakeBytesIO()
    with self.assertRaises(TypeError) as context:
      proto.serialize_length_prefixed(msg, out)
    self.assertIn(
        'Failed to write complete message (wrote: 0, expected: 2)',
        str(context.exception),
    )

  def test_byte_size(self, message_module):
    msg = message_module.TestAllTypes()
    self.assertEqual(0, proto.byte_size(msg))
    msg.optional_int32 = 123
    self.assertEqual(2, proto.byte_size(msg))

  def test_clear_message(self, message_module):
    msg = message_module.TestAllTypes()
    msg.oneof_uint32 = 11
    msg.repeated_nested_message.add(bb=1)
    proto.clear_message(msg)
    self.assertIsNone(msg.WhichOneof('oneof_field'))
    self.assertEqual(0, len(msg.repeated_nested_message))

  def test_clear_field(self, message_module):
    msg = message_module.TestAllTypes()
    msg.optional_int32 = 123
    self.assertEqual(123, msg.optional_int32)
    proto.clear_field(msg, 'optional_int32')
    self.assertEqual(0, msg.optional_int32)


class SelfFieldTest(unittest.TestCase):

  def test_pytype_allows_unset_self_field(self):
    self.assertEqual(
        test_proto2_pb2.MessageWithSelfField(something=123).something, 123
    )

  def test_pytype_allows_unset_self_and_self_underscore_field(self):
    self.assertEqual(
        test_proto2_pb2.MessageWithSelfAndSelfUnderscoreField(
            something=123
        ).something,
        123,
    )


_EXPECTED_PROTO3 = b'\x04r\x02hi\x06\x08\x01r\x02hi\x06\x08\x02r\x02hi'
_EXPECTED_PROTO2 = b'\x06\x08\x00r\x02hi\x06\x08\x01r\x02hi\x06\x08\x02r\x02hi'


@parameterized.named_parameters(
    ('_proto2', unittest_pb2, _EXPECTED_PROTO2),
    ('_proto3', unittest_proto3_arena_pb2, _EXPECTED_PROTO3),
)
@testing_refleaks.TestCase
class LengthPrefixedWithGolden(unittest.TestCase):

  def test_serialize_length_prefixed(self, message_module, expected):
    number_of_messages = 3

    out = io.BytesIO()
    for index in range(0, number_of_messages):
      msg = message_module.TestAllTypes(
          optional_int32=index, optional_string='hi'
      )
      proto.serialize_length_prefixed(msg, out)

    self.assertEqual(out.getvalue(), expected)

  def test_parse_length_prefixed(self, message_module, input_bytes):
    expected_number_of_messages = 3

    input_io = io.BytesIO(input_bytes)
    index = 0
    while True:
      msg = proto.parse_length_prefixed(message_module.TestAllTypes, input_io)
      if msg is None:
        break
      self.assertEqual(msg.optional_int32, index)
      self.assertEqual(msg.optional_string, 'hi')
      index += 1

    self.assertEqual(index, expected_number_of_messages)


if __name__ == '__main__':
  unittest.main()
