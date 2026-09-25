# -*- coding: utf-8 -*-
# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Test decoder."""

import io
import unittest

from google.protobuf import message
from google.protobuf.internal import api_implementation
from google.protobuf.internal import decoder
from google.protobuf.internal import message_set_extensions_pb2
from google.protobuf.internal import testing_refleaks
from google.protobuf.internal import wire_format

from absl.testing import parameterized

_INPUT_BYTES = b'\x84r\x12'
_EXPECTED = (14596, 18)


@testing_refleaks.TestCase
class DecoderTest(parameterized.TestCase):

  def test_decode_varint_bytes(self):
    size, pos = decoder._DecodeVarint(_INPUT_BYTES, 0)
    self.assertEqual(size, _EXPECTED[0])
    self.assertEqual(pos, 2)

    size, pos = decoder._DecodeVarint(_INPUT_BYTES, 2)
    self.assertEqual(size, _EXPECTED[1])
    self.assertEqual(pos, 3)

  def test_decode_varint_bytes_empty(self):
    with self.assertRaises(IndexError) as context:
      decoder._DecodeVarint(b'', 0)
    self.assertIn('index out of range', str(context.exception))

  def test_decode_varint_bytesio(self):
    index = 0
    input_io = io.BytesIO(_INPUT_BYTES)
    while True:
      size = decoder._DecodeVarint(input_io)
      if size is None:
        break
      self.assertEqual(size, _EXPECTED[index])
      index += 1
    self.assertEqual(index, len(_EXPECTED))

  def test_decode_varint_bytesio_empty(self):
    input_io = io.BytesIO(b'')
    size = decoder._DecodeVarint(input_io)
    self.assertIsNone(size)

  def test_decode_unknown_group_field(self):
    data = memoryview(b'\013\020\003\014\040\005')
    parsed, pos = decoder._DecodeUnknownField(
        data, 1, len(data), 1, wire_format.WIRETYPE_START_GROUP
    )

    self.assertEqual(pos, 4)
    self.assertEqual(len(parsed), 1)
    self.assertEqual(parsed[0].field_number, 2)
    self.assertEqual(parsed[0].data, 3)

  def test_decode_unknown_group_field_nested(self):
    data = memoryview(b'\013\023\013\030\004\014\024\014\050\006')
    parsed, pos = decoder._DecodeUnknownField(
        data, 1, len(data), 1, wire_format.WIRETYPE_START_GROUP
    )

    self.assertEqual(pos, 8)
    self.assertEqual(len(parsed), 1)
    self.assertEqual(parsed[0].field_number, 2)
    self.assertEqual(len(parsed[0].data), 1)
    self.assertEqual(parsed[0].data[0].field_number, 1)
    self.assertEqual(len(parsed[0].data[0].data), 1)
    self.assertEqual(parsed[0].data[0].data[0].field_number, 3)
    self.assertEqual(parsed[0].data[0].data[0].data, 4)

  def test_decode_unknown_group_field_too_many_levels(self):
    data = memoryview(b'\023' * 5_000_000)
    self.assertRaisesRegex(
        message.DecodeError,
        'Error parsing message',
        decoder._DecodeUnknownField,
        data,
        1,
        len(data),
        1,
        wire_format.WIRETYPE_START_GROUP,
    )

  def test_decode_unknown_mismatched_end_group(self):
    self.assertRaisesRegex(
        message.DecodeError,
        'Missing group end tag.*',
        decoder._DecodeUnknownField,
        memoryview(b'\013\024'),
        1,
        2,
        1,
        wire_format.WIRETYPE_START_GROUP,
    )

  def test_decode_unknown_mismatched_end_group_nested(self):
    self.assertRaisesRegex(
        message.DecodeError,
        'Missing group end tag.*',
        decoder._DecodeUnknownField,
        memoryview(b'\013\023\034\024\014'),
        1,
        5,
        1,
        wire_format.WIRETYPE_START_GROUP,
    )

  def test_decode_message_set_unknown_mismatched_end_group(self):
    proto = message_set_extensions_pb2.TestMessageSet()
    self.assertRaisesRegex(
        message.DecodeError,
        'Unexpected end-group tag.'
        if api_implementation.Type() == 'python'
        else '.*',
        proto.ParseFromString,
        b'\013\054\014',
    )

  def test_unknown_message_set_decoder_mismatched_end_group(self):
    # This behavior isn't actually reachable in practice, but it's good to
    # test anyway.
    decode = decoder.UnknownMessageSetItemDecoder()
    self.assertRaisesRegex(
        message.DecodeError,
        'Unexpected end-group tag.',
        decode,
        memoryview(b'\054\014'),
    )

  def test_messageset_item_decoder_propagates_current_depth(self):
    """DecodeItem must forward current_depth to _DecodeUnknownField.

    Without the fix, _DecodeUnknownField defaults current_depth=0
    regardless of the caller's actual depth, allowing an attacker to reset
    the recursion counter within a MessageSet item (GHSA-8qvm-5x2c-j2w7
    bypass).
    """
    if api_implementation.Type() != 'python':
      self.skipTest('Pure-Python decoder-specific test')

    # Build a MessageSet item body with a type_id, empty message, and
    # several nested unknown groups (field 5, START/END_GROUP).
    num_nested = 5
    content = (
        b'\x10\x63'              # field 2 (type_id) varint = 99
        + b'\x1a\x00'            # field 3 (message) length-delimited, size 0
        + b'\x2b' * num_nested   # 5 x field 5 START_GROUP
        + b'\x2c' * num_nested   # 5 x field 5 END_GROUP
        + b'\x0c'                # field 1 END_GROUP (item end)
    )
    buf = memoryview(content)

    decode_item = decoder.MessageSetItemDecoder(
        message_set_extensions_pb2.TestMessageSet.DESCRIPTOR
    )

    old_limit = decoder._recursion_limit
    decoder.SetRecursionLimit(10)
    try:
      # Starting at depth 0, 5 nested groups (max depth 5) < 10 -> succeeds.
      msg = message_set_extensions_pb2.TestMessageSet()
      decode_item(buf, 0, len(buf), msg, msg._fields, current_depth=0)

      # Starting at depth 8, the 2nd nested group reaches depth 10 >= limit
      # -> must raise DecodeError.  Without the fix the depth resets to 0
      # and the call would silently succeed.
      msg2 = message_set_extensions_pb2.TestMessageSet()
      self.assertRaisesRegex(
          message.DecodeError,
          'Error parsing message',
          decode_item,
          buf,
          0,
          len(buf),
          msg2,
          msg2._fields,
          8,
      )
    finally:
      decoder.SetRecursionLimit(old_limit)

  def test_unknown_message_set_decoder_propagates_current_depth(self):
    """DecodeUnknownItem must forward current_depth to _DecodeUnknownField.

    Mirrors the MessageSetItemDecoder test for the
    UnknownMessageSetItemDecoder code path.
    """
    # Same item body layout: type_id=99, empty message, 5 nested groups.
    num_nested = 5
    item_data = (
        b'\x10\x63'
        + b'\x1a\x00'
        + b'\x2b' * num_nested
        + b'\x2c' * num_nested
        + b'\x0c'
    )

    decode = decoder.UnknownMessageSetItemDecoder()

    old_limit = decoder._recursion_limit
    decoder.SetRecursionLimit(10)
    try:
      # Depth 0 -> max depth 5 < 10 -> succeeds and returns type_id.
      type_id, _ = decode(memoryview(item_data), current_depth=0)
      self.assertEqual(type_id, 99)

      # Depth 8 -> reaches limit -> DecodeError.
      self.assertRaisesRegex(
          message.DecodeError,
          'Error parsing message',
          decode,
          memoryview(item_data),
          8,
      )
    finally:
      decoder.SetRecursionLimit(old_limit)

  def test_messageset_deeply_nested_unknown_groups_raises_decode_error(self):
    """Deeply nested unknown groups in a MessageSet raise DecodeError.

    Verifies that the parser raises message.DecodeError (not
    RecursionError) when unknown groups inside a MessageSet item exceed the
    recursion limit.
    """
    if api_implementation.Type() != 'python':
      self.skipTest('Pure-Python decoder-specific test')

    # 150 nested unknown groups exceed the default recursion limit of 100.
    num_nested = 150
    data = (
        b'\x0b'                  # Item START_GROUP (field 1)
        + b'\x10\x63'            # type_id = 99
        + b'\x1a\x00'            # message = empty
        + b'\x2b' * num_nested   # field 5 START_GROUP
        + b'\x2c' * num_nested   # field 5 END_GROUP
        + b'\x0c'                # Item END_GROUP (field 1)
    )
    proto = message_set_extensions_pb2.TestMessageSet()
    self.assertRaisesRegex(
        message.DecodeError,
        'Error parsing message',
        proto.ParseFromString,
        data,
    )

  @parameterized.parameters(int(0), float(0.0), False, '')
  def test_default_scalar(self, value):
    self.assertTrue(decoder.IsDefaultScalarValue(value))

  @parameterized.parameters(int(1), float(-0.0), float(1.0), True, 'a')
  def test_not_default_scalar(self, value):
    self.assertFalse(decoder.IsDefaultScalarValue(value))


if __name__ == '__main__':
  unittest.main()
