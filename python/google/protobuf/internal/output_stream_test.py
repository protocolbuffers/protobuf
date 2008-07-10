# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.
# http://code.google.com/p/protobuf/
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Test for google.protobuf.internal.output_stream."""

__author__ = 'robinson@google.com (Will Robinson)'

import unittest
from google.protobuf import message
from google.protobuf.internal import output_stream
from google.protobuf.internal import wire_format


class OutputStreamTest(unittest.TestCase):

  def setUp(self):
    self.stream = output_stream.OutputStream()

  def testAppendRawBytes(self):
    # Empty string.
    self.stream.AppendRawBytes('')
    self.assertEqual('', self.stream.ToString())

    # Nonempty string.
    self.stream.AppendRawBytes('abc')
    self.assertEqual('abc', self.stream.ToString())

    # Ensure that we're actually appending.
    self.stream.AppendRawBytes('def')
    self.assertEqual('abcdef', self.stream.ToString())

  def AppendNumericTestHelper(self, append_fn, values_and_strings):
    """For each (value, expected_string) pair in values_and_strings,
    calls an OutputStream.Append*(value) method on an OutputStream and ensures
    that the string written to that stream matches expected_string.

    Args:
      append_fn: Unbound OutputStream method that takes an integer or
        long value as input.
      values_and_strings: Iterable of (value, expected_string) pairs.
    """
    for conversion in (int, long):
      for value, string in values_and_strings:
        stream = output_stream.OutputStream()
        expected_string = ''
        append_fn(stream, conversion(value))
        expected_string += string
        self.assertEqual(expected_string, stream.ToString())

  def AppendOverflowTestHelper(self, append_fn, value):
    """Calls an OutputStream.Append*(value) method and asserts
    that the method raises message.EncodeError.

    Args:
      append_fn: Unbound OutputStream method that takes an integer or
        long value as input.
      value: Value to pass to append_fn which should cause an
        message.EncodeError.
    """
    stream = output_stream.OutputStream()
    self.assertRaises(message.EncodeError, append_fn, stream, value)

  def testAppendLittleEndian32(self):
    append_fn = output_stream.OutputStream.AppendLittleEndian32
    values_and_expected_strings = [
        (0, '\x00\x00\x00\x00'),
        (1, '\x01\x00\x00\x00'),
        ((1 << 32) - 1, '\xff\xff\xff\xff'),
        ]
    self.AppendNumericTestHelper(append_fn, values_and_expected_strings)

    self.AppendOverflowTestHelper(append_fn, 1 << 32)
    self.AppendOverflowTestHelper(append_fn, -1)

  def testAppendLittleEndian64(self):
    append_fn = output_stream.OutputStream.AppendLittleEndian64
    values_and_expected_strings = [
        (0, '\x00\x00\x00\x00\x00\x00\x00\x00'),
        (1, '\x01\x00\x00\x00\x00\x00\x00\x00'),
        ((1 << 64) - 1, '\xff\xff\xff\xff\xff\xff\xff\xff'),
        ]
    self.AppendNumericTestHelper(append_fn, values_and_expected_strings)

    self.AppendOverflowTestHelper(append_fn, 1 << 64)
    self.AppendOverflowTestHelper(append_fn, -1)

  def testAppendVarint32(self):
    append_fn = output_stream.OutputStream.AppendVarint32
    values_and_expected_strings = [
        (0, '\x00'),
        (1, '\x01'),
        (127, '\x7f'),
        (128, '\x80\x01'),
        (-1, '\xff\xff\xff\xff\xff\xff\xff\xff\xff\x01'),
        (wire_format.INT32_MAX, '\xff\xff\xff\xff\x07'),
        (wire_format.INT32_MIN, '\x80\x80\x80\x80\xf8\xff\xff\xff\xff\x01'),
        ]
    self.AppendNumericTestHelper(append_fn, values_and_expected_strings)

    self.AppendOverflowTestHelper(append_fn, wire_format.INT32_MAX + 1)
    self.AppendOverflowTestHelper(append_fn, wire_format.INT32_MIN - 1)

  def testAppendVarUInt32(self):
    append_fn = output_stream.OutputStream.AppendVarUInt32
    values_and_expected_strings = [
        (0, '\x00'),
        (1, '\x01'),
        (127, '\x7f'),
        (128, '\x80\x01'),
        (wire_format.UINT32_MAX, '\xff\xff\xff\xff\x0f'),
        ]
    self.AppendNumericTestHelper(append_fn, values_and_expected_strings)

    self.AppendOverflowTestHelper(append_fn, -1)
    self.AppendOverflowTestHelper(append_fn, wire_format.UINT32_MAX + 1)

  def testAppendVarint64(self):
    append_fn = output_stream.OutputStream.AppendVarint64
    values_and_expected_strings = [
        (0, '\x00'),
        (1, '\x01'),
        (127, '\x7f'),
        (128, '\x80\x01'),
        (-1, '\xff\xff\xff\xff\xff\xff\xff\xff\xff\x01'),
        (wire_format.INT64_MAX, '\xff\xff\xff\xff\xff\xff\xff\xff\x7f'),
        (wire_format.INT64_MIN, '\x80\x80\x80\x80\x80\x80\x80\x80\x80\x01'),
        ]
    self.AppendNumericTestHelper(append_fn, values_and_expected_strings)

    self.AppendOverflowTestHelper(append_fn, wire_format.INT64_MAX + 1)
    self.AppendOverflowTestHelper(append_fn, wire_format.INT64_MIN - 1)

  def testAppendVarUInt64(self):
    append_fn = output_stream.OutputStream.AppendVarUInt64
    values_and_expected_strings = [
        (0, '\x00'),
        (1, '\x01'),
        (127, '\x7f'),
        (128, '\x80\x01'),
        (wire_format.UINT64_MAX, '\xff\xff\xff\xff\xff\xff\xff\xff\xff\x01'),
        ]
    self.AppendNumericTestHelper(append_fn, values_and_expected_strings)

    self.AppendOverflowTestHelper(append_fn, -1)
    self.AppendOverflowTestHelper(append_fn, wire_format.UINT64_MAX + 1)


if __name__ == '__main__':
  unittest.main()
