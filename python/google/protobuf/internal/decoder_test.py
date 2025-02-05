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
    (size, pos) = decoder._DecodeVarint(_INPUT_BYTES, 0)
    self.assertEqual(size, _EXPECTED[0])
    self.assertEqual(pos, 2)

    (size, pos) = decoder._DecodeVarint(_INPUT_BYTES, 2)
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

  @parameterized.parameters(int(0), float(0.0), False, '')
  def test_default_scalar(self, value):
    self.assertTrue(decoder.IsDefaultScalarValue(value))

  @parameterized.parameters(int(1), float(-0.0), float(1.0), True, 'a')
  def test_not_default_scalar(self, value):
    self.assertFalse(decoder.IsDefaultScalarValue(value))


if __name__ == '__main__':
  unittest.main()
