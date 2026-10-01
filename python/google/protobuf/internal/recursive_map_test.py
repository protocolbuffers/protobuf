# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Tests for recursive message-valued maps."""

import unittest
from unittest import mock

from google.protobuf import message
from google.protobuf import struct_pb2
from google.protobuf import text_format


def _Varint(value):
  output = bytearray()
  while True:
    byte = value & 0x7F
    value >>= 7
    output.append(byte | 0x80 if value else byte)
    if not value:
      return bytes(output)


def _MessageField(field_number, payload):
  return bytes([(field_number << 3) | 2]) + _Varint(len(payload)) + payload


def _BinaryStruct(depth):
  inner = b''
  for _ in range(depth):
    inner = _MessageField(
        1, b'\x0a\x01k' + _MessageField(2, _MessageField(5, inner))
    )
  return inner


def _AssertStructDepth(test_case, parsed, depth):
  original_bytes = parsed.SerializeToString()
  current = parsed
  for _ in range(depth):
    test_case.assertIn('k', current.fields)
    current = current.fields['k'].struct_value
  test_case.assertFalse(current.fields)
  current.fields['leaf'].string_value = 'value'
  test_case.assertNotEqual(original_bytes, parsed.SerializeToString())


class RecursiveMapTest(unittest.TestCase):

  def _ParseWithoutMessageCopies(self, parse):
    copy_calls = 0
    original_copy_from = message.Message.CopyFrom

    def CountCopyFrom(destination, source):
      nonlocal copy_calls
      copy_calls += 1
      original_copy_from(destination, source)

    with mock.patch.object(message.Message, 'CopyFrom', CountCopyFrom):
      parsed = parse()

    self.assertEqual(0, copy_calls)
    return parsed

  def testTextFormatParseDoesNotCopyRecursiveMapValues(self):
    # Given a text-format Struct nested through message-valued maps.
    depth = 8
    payload = ('fields { key: "k" value { struct_value { ' * depth) + (
        '} } }' * depth
    )

    # When the payload is parsed.
    parsed = self._ParseWithoutMessageCopies(
        lambda: text_format.Parse(payload, struct_pb2.Struct())
    )

    # Then the nested values are transferred without deep-copying each level.
    _AssertStructDepth(self, parsed, depth)

  def testBinaryParseDoesNotCopyRecursiveMapValues(self):
    # Given a binary Struct nested through message-valued maps.
    depth = 8
    payload = _BinaryStruct(depth)

    # When the payload is parsed.
    parsed = self._ParseWithoutMessageCopies(
        lambda: struct_pb2.Struct.FromString(payload)
    )

    # Then the nested values are transferred without deep-copying each level.
    _AssertStructDepth(self, parsed, depth)


if __name__ == '__main__':
  unittest.main()
