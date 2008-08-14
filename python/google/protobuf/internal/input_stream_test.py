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

"""Test for google.protobuf.internal.input_stream."""

__author__ = 'robinson@google.com (Will Robinson)'

import unittest
from google.protobuf import message
from google.protobuf.internal import wire_format
from google.protobuf.internal import input_stream


class InputStreamTest(unittest.TestCase):

  def testEndOfStream(self):
    stream = input_stream.InputStream('abcd')
    self.assertFalse(stream.EndOfStream())
    self.assertEqual('abcd', stream.ReadString(10))
    self.assertTrue(stream.EndOfStream())

  def testPosition(self):
    stream = input_stream.InputStream('abcd')
    self.assertEqual(0, stream.Position())
    self.assertEqual(0, stream.Position())  # No side-effects.
    stream.ReadString(1)
    self.assertEqual(1, stream.Position())
    stream.ReadString(1)
    self.assertEqual(2, stream.Position())
    stream.ReadString(10)
    self.assertEqual(4, stream.Position())  # Can't go past end of stream.

  def testGetSubBuffer(self):
    stream = input_stream.InputStream('abcd')
    # Try leaving out the size.
    self.assertEqual('abcd', str(stream.GetSubBuffer()))
    stream.SkipBytes(1)
    # GetSubBuffer() always starts at current size.
    self.assertEqual('bcd', str(stream.GetSubBuffer()))
    # Try 0-size.
    self.assertEqual('', str(stream.GetSubBuffer(0)))
    # Negative sizes should raise an error.
    self.assertRaises(message.DecodeError, stream.GetSubBuffer, -1)
    # Positive sizes should work as expected.
    self.assertEqual('b', str(stream.GetSubBuffer(1)))
    self.assertEqual('bc', str(stream.GetSubBuffer(2)))
    # Sizes longer than remaining bytes in the buffer should
    # return the whole remaining buffer.
    self.assertEqual('bcd', str(stream.GetSubBuffer(1000)))

  def testSkipBytes(self):
    stream = input_stream.InputStream('')
    # Skipping bytes when at the end of stream
    # should have no effect.
    stream.SkipBytes(0)
    stream.SkipBytes(1)
    stream.SkipBytes(2)
    self.assertTrue(stream.EndOfStream())
    self.assertEqual(0, stream.Position())

    # Try skipping within a stream.
    stream = input_stream.InputStream('abcd')
    self.assertEqual(0, stream.Position())
    stream.SkipBytes(1)
    self.assertEqual(1, stream.Position())
    stream.SkipBytes(10)  # Can't skip past the end.
    self.assertEqual(4, stream.Position())

    # Ensure that a negative skip raises an exception.
    stream = input_stream.InputStream('abcd')
    stream.SkipBytes(1)
    self.assertRaises(message.DecodeError, stream.SkipBytes, -1)

  def testReadString(self):
    s = 'abcd'
    # Also test going past the total stream length.
    for i in range(len(s) + 10):
      stream = input_stream.InputStream(s)
      self.assertEqual(s[:i], stream.ReadString(i))
      self.assertEqual(min(i, len(s)), stream.Position())
    stream = input_stream.InputStream(s)
    self.assertRaises(message.DecodeError, stream.ReadString, -1)

  def EnsureFailureOnEmptyStream(self, input_stream_method):
    """Helper for integer-parsing tests below.
    Ensures that the given InputStream method raises a DecodeError
    if called on a stream with no bytes remaining.
    """
    stream = input_stream.InputStream('')
    self.assertRaises(message.DecodeError, input_stream_method, stream)

  def testReadLittleEndian32(self):
    self.EnsureFailureOnEmptyStream(input_stream.InputStream.ReadLittleEndian32)
    s = ''
    # Read 0.
    s += '\x00\x00\x00\x00'
    # Read 1.
    s += '\x01\x00\x00\x00'
    # Read a bunch of different bytes.
    s += '\x01\x02\x03\x04'
    # Read max unsigned 32-bit int.
    s += '\xff\xff\xff\xff'
    # Try a read with fewer than 4 bytes left in the stream.
    s += '\x00\x00\x00'
    stream = input_stream.InputStream(s)
    self.assertEqual(0, stream.ReadLittleEndian32())
    self.assertEqual(4, stream.Position())
    self.assertEqual(1, stream.ReadLittleEndian32())
    self.assertEqual(8, stream.Position())
    self.assertEqual(0x04030201, stream.ReadLittleEndian32())
    self.assertEqual(12, stream.Position())
    self.assertEqual(wire_format.UINT32_MAX, stream.ReadLittleEndian32())
    self.assertEqual(16, stream.Position())
    self.assertRaises(message.DecodeError, stream.ReadLittleEndian32)

  def testReadLittleEndian64(self):
    self.EnsureFailureOnEmptyStream(input_stream.InputStream.ReadLittleEndian64)
    s = ''
    # Read 0.
    s += '\x00\x00\x00\x00\x00\x00\x00\x00'
    # Read 1.
    s += '\x01\x00\x00\x00\x00\x00\x00\x00'
    # Read a bunch of different bytes.
    s += '\x01\x02\x03\x04\x05\x06\x07\x08'
    # Read max unsigned 64-bit int.
    s += '\xff\xff\xff\xff\xff\xff\xff\xff'
    # Try a read with fewer than 8 bytes left in the stream.
    s += '\x00\x00\x00'
    stream = input_stream.InputStream(s)
    self.assertEqual(0, stream.ReadLittleEndian64())
    self.assertEqual(8, stream.Position())
    self.assertEqual(1, stream.ReadLittleEndian64())
    self.assertEqual(16, stream.Position())
    self.assertEqual(0x0807060504030201, stream.ReadLittleEndian64())
    self.assertEqual(24, stream.Position())
    self.assertEqual(wire_format.UINT64_MAX, stream.ReadLittleEndian64())
    self.assertEqual(32, stream.Position())
    self.assertRaises(message.DecodeError, stream.ReadLittleEndian64)

  def ReadVarintSuccessTestHelper(self, varints_and_ints, read_method):
    """Helper for tests below that test successful reads of various varints.

    Args:
      varints_and_ints: Iterable of (str, integer) pairs, where the string
        gives the wire encoding and the integer gives the value we expect
        to be returned by the read_method upon encountering this string.
      read_method: Unbound InputStream method that is capable of reading
        the encoded strings provided in the first elements of varints_and_ints.
    """
    s = ''.join(s for s, i in varints_and_ints)
    stream = input_stream.InputStream(s)
    expected_pos = 0
    self.assertEqual(expected_pos, stream.Position())
    for s, expected_int in varints_and_ints:
      self.assertEqual(expected_int, read_method(stream))
      expected_pos += len(s)
      self.assertEqual(expected_pos, stream.Position())

  def testReadVarint32Success(self):
    varints_and_ints = [
        ('\x00', 0),
        ('\x01', 1),
        ('\x7f', 127),
        ('\x80\x01', 128),
        ('\xff\xff\xff\xff\xff\xff\xff\xff\xff\x01', -1),
        ('\xff\xff\xff\xff\x07', wire_format.INT32_MAX),
        ('\x80\x80\x80\x80\xf8\xff\xff\xff\xff\x01', wire_format.INT32_MIN),
        ]
    self.ReadVarintSuccessTestHelper(varints_and_ints,
                                     input_stream.InputStream.ReadVarint32)

  def testReadVarint32Failure(self):
    self.EnsureFailureOnEmptyStream(input_stream.InputStream.ReadVarint32)

    # Try and fail to read INT32_MAX + 1.
    s = '\x80\x80\x80\x80\x08'
    stream = input_stream.InputStream(s)
    self.assertRaises(message.DecodeError, stream.ReadVarint32)

    # Try and fail to read INT32_MIN - 1.
    s = '\xfe\xff\xff\xff\xf7\xff\xff\xff\xff\x01'
    stream = input_stream.InputStream(s)
    self.assertRaises(message.DecodeError, stream.ReadVarint32)

    # Try and fail to read something that looks like
    # a varint with more than 10 bytes.
    s = '\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\x00'
    stream = input_stream.InputStream(s)
    self.assertRaises(message.DecodeError, stream.ReadVarint32)

  def testReadVarUInt32Success(self):
    varints_and_ints = [
        ('\x00', 0),
        ('\x01', 1),
        ('\x7f', 127),
        ('\x80\x01', 128),
        ('\xff\xff\xff\xff\x0f', wire_format.UINT32_MAX),
        ]
    self.ReadVarintSuccessTestHelper(varints_and_ints,
                                     input_stream.InputStream.ReadVarUInt32)

  def testReadVarUInt32Failure(self):
    self.EnsureFailureOnEmptyStream(input_stream.InputStream.ReadVarUInt32)
    # Try and fail to read UINT32_MAX + 1
    s = '\x80\x80\x80\x80\x10'
    stream = input_stream.InputStream(s)
    self.assertRaises(message.DecodeError, stream.ReadVarUInt32)

    # Try and fail to read something that looks like
    # a varint with more than 10 bytes.
    s = '\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\x00'
    stream = input_stream.InputStream(s)
    self.assertRaises(message.DecodeError, stream.ReadVarUInt32)

  def testReadVarint64Success(self):
    varints_and_ints = [
        ('\x00', 0),
        ('\x01', 1),
        ('\xff\xff\xff\xff\xff\xff\xff\xff\xff\x01', -1),
        ('\x7f', 127),
        ('\x80\x01', 128),
        ('\xff\xff\xff\xff\xff\xff\xff\xff\x7f', wire_format.INT64_MAX),
        ('\x80\x80\x80\x80\x80\x80\x80\x80\x80\x01', wire_format.INT64_MIN),
        ]
    self.ReadVarintSuccessTestHelper(varints_and_ints,
                                     input_stream.InputStream.ReadVarint64)

  def testReadVarint64Failure(self):
    self.EnsureFailureOnEmptyStream(input_stream.InputStream.ReadVarint64)
    # Try and fail to read something with the mythical 64th bit set.
    s = '\x80\x80\x80\x80\x80\x80\x80\x80\x80\x02'
    stream = input_stream.InputStream(s)
    self.assertRaises(message.DecodeError, stream.ReadVarint64)

    # Try and fail to read something that looks like
    # a varint with more than 10 bytes.
    s = '\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\x00'
    stream = input_stream.InputStream(s)
    self.assertRaises(message.DecodeError, stream.ReadVarint64)

  def testReadVarUInt64Success(self):
    varints_and_ints = [
        ('\x00', 0),
        ('\x01', 1),
        ('\x7f', 127),
        ('\x80\x01', 128),
        ('\x80\x80\x80\x80\x80\x80\x80\x80\x80\x01', 1 << 63),
        ]
    self.ReadVarintSuccessTestHelper(varints_and_ints,
                                     input_stream.InputStream.ReadVarUInt64)

  def testReadVarUInt64Failure(self):
    self.EnsureFailureOnEmptyStream(input_stream.InputStream.ReadVarUInt64)
    # Try and fail to read something with the mythical 64th bit set.
    s = '\x80\x80\x80\x80\x80\x80\x80\x80\x80\x02'
    stream = input_stream.InputStream(s)
    self.assertRaises(message.DecodeError, stream.ReadVarUInt64)

    # Try and fail to read something that looks like
    # a varint with more than 10 bytes.
    s = '\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\x00'
    stream = input_stream.InputStream(s)
    self.assertRaises(message.DecodeError, stream.ReadVarUInt64)

if __name__ == '__main__':
  unittest.main()
