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

"""Test for google.protobuf.internal.encoder."""

__author__ = 'robinson@google.com (Will Robinson)'

import struct
import logging
import unittest
import mox
from google.protobuf.internal import wire_format
from google.protobuf.internal import encoder
from google.protobuf.internal import output_stream
from google.protobuf import message


class EncoderTest(unittest.TestCase):

  def setUp(self):
    self.mox = mox.Mox()
    self.encoder = encoder.Encoder()
    self.mock_stream = self.mox.CreateMock(output_stream.OutputStream)
    self.mock_message = self.mox.CreateMock(message.Message)
    self.encoder._stream = self.mock_stream

  def PackTag(self, field_number, wire_type):
    return wire_format.PackTag(field_number, wire_type)

  def AppendScalarTestHelper(self, test_name, encoder_method,
                             expected_stream_method_name,
                             wire_type, field_value, expected_value=None):
    """Helper for testAppendScalars.

    Calls one of the Encoder methods, and ensures that the Encoder
    in turn makes the expected calls into its OutputStream.

    Args:
      test_name: Name of this test, used only for logging.
      encoder_method: Callable on self.encoder, which should
        accept |field_value| as an argument.  This is the Encoder
        method we're testing.
      expected_stream_method_name: (string) Name of the OutputStream
        method we expect Encoder to call to actually put the value
        on the wire.
      wire_type: The WIRETYPE_* constant we expect encoder to
        use in the specified encoder_method.
      field_value: The value we're trying to encode.  Passed
        into encoder_method.
      expected_value: The value we expect Encoder to pass into
        the OutputStream method.  If None, we expect field_value
        to pass through unmodified.
    """
    if expected_value is None:
      expected_value = field_value

    logging.info('Testing %s scalar output.\n'
                 'Calling %r(%r), and expecting that to call the '
                 'stream method %s(%r).' % (
        test_name, encoder_method, field_value,
        expected_stream_method_name, expected_value))

    field_number = 10
    # Should first append the field number and type information.
    self.mock_stream.AppendVarUInt32(self.PackTag(field_number, wire_type))
    # If we're length-delimited, we should then append the length.
    if wire_type == wire_format.WIRETYPE_LENGTH_DELIMITED:
      self.mock_stream.AppendVarUInt32(len(field_value))
    # Should then append the value itself.
    # We have to use names instead of methods to work around some
    # mox weirdness.  (ResetAll() is overzealous).
    expected_stream_method = getattr(self.mock_stream,
                                     expected_stream_method_name)
    expected_stream_method(expected_value)

    self.mox.ReplayAll()
    encoder_method(field_number, field_value)
    self.mox.VerifyAll()
    self.mox.ResetAll()

  def testAppendScalars(self):
    scalar_tests = [
        ['int32', self.encoder.AppendInt32, 'AppendVarint32',
         wire_format.WIRETYPE_VARINT, 0],
        ['int64', self.encoder.AppendInt64, 'AppendVarint64',
         wire_format.WIRETYPE_VARINT, 0],
        ['uint32', self.encoder.AppendUInt32, 'AppendVarUInt32',
         wire_format.WIRETYPE_VARINT, 0],
        ['uint64', self.encoder.AppendUInt64, 'AppendVarUInt64',
         wire_format.WIRETYPE_VARINT, 0],
        ['fixed32', self.encoder.AppendFixed32, 'AppendLittleEndian32',
         wire_format.WIRETYPE_FIXED32, 0],
        ['fixed64', self.encoder.AppendFixed64, 'AppendLittleEndian64',
         wire_format.WIRETYPE_FIXED64, 0],
        ['sfixed32', self.encoder.AppendSFixed32, 'AppendLittleEndian32',
         wire_format.WIRETYPE_FIXED32, -1, 0xffffffff],
        ['sfixed64', self.encoder.AppendSFixed64, 'AppendLittleEndian64',
         wire_format.WIRETYPE_FIXED64, -1, 0xffffffffffffffff],
        ['float', self.encoder.AppendFloat, 'AppendRawBytes',
         wire_format.WIRETYPE_FIXED32, 0.0, struct.pack('f', 0.0)],
        ['double', self.encoder.AppendDouble, 'AppendRawBytes',
         wire_format.WIRETYPE_FIXED64, 0.0, struct.pack('d', 0.0)],
        ['bool', self.encoder.AppendBool, 'AppendVarint32',
         wire_format.WIRETYPE_VARINT, False],
        ['enum', self.encoder.AppendEnum, 'AppendVarint32',
         wire_format.WIRETYPE_VARINT, 0],
        ['string', self.encoder.AppendString, 'AppendRawBytes',
         wire_format.WIRETYPE_LENGTH_DELIMITED,
         "You're in a maze of twisty little passages, all alike."],
        # We test zigzag encoding routines more extensively below.
        ['sint32', self.encoder.AppendSInt32, 'AppendVarUInt32',
         wire_format.WIRETYPE_VARINT, -1, 1],
        ['sint64', self.encoder.AppendSInt64, 'AppendVarUInt64',
         wire_format.WIRETYPE_VARINT, -1, 1],
        ]
    # Ensure that we're testing different Encoder methods and using
    # different test names in all test cases above.
    self.assertEqual(len(scalar_tests), len(set(t[0] for t in scalar_tests)))
    self.assertEqual(len(scalar_tests), len(set(t[1] for t in scalar_tests)))
    for args in scalar_tests:
      self.AppendScalarTestHelper(*args)

  def testAppendGroup(self):
    field_number = 23
    # Should first append the start-group marker.
    self.mock_stream.AppendVarUInt32(
        self.PackTag(field_number, wire_format.WIRETYPE_START_GROUP))
    # Should then serialize itself.
    self.mock_message.SerializeToString().AndReturn('foo')
    self.mock_stream.AppendRawBytes('foo')
    # Should finally append the end-group marker.
    self.mock_stream.AppendVarUInt32(
        self.PackTag(field_number, wire_format.WIRETYPE_END_GROUP))

    self.mox.ReplayAll()
    self.encoder.AppendGroup(field_number, self.mock_message)
    self.mox.VerifyAll()

  def testAppendMessage(self):
    field_number = 23
    byte_size = 42
    # Should first append the field number and type information.
    self.mock_stream.AppendVarUInt32(
        self.PackTag(field_number, wire_format.WIRETYPE_LENGTH_DELIMITED))
    # Should then append its length.
    self.mock_message.ByteSize().AndReturn(byte_size)
    self.mock_stream.AppendVarUInt32(byte_size)
    # Should then serialize itself to the encoder.
    self.mock_message.SerializeToString().AndReturn('foo')
    self.mock_stream.AppendRawBytes('foo')

    self.mox.ReplayAll()
    self.encoder.AppendMessage(field_number, self.mock_message)
    self.mox.VerifyAll()

  def testAppendMessageSetItem(self):
    field_number = 23
    byte_size = 42
    # Should first append the field number and type information.
    self.mock_stream.AppendVarUInt32(
        self.PackTag(1, wire_format.WIRETYPE_START_GROUP))
    self.mock_stream.AppendVarUInt32(
        self.PackTag(2, wire_format.WIRETYPE_VARINT))
    self.mock_stream.AppendVarint32(field_number)
    self.mock_stream.AppendVarUInt32(
        self.PackTag(3, wire_format.WIRETYPE_LENGTH_DELIMITED))
    # Should then append its length.
    self.mock_message.ByteSize().AndReturn(byte_size)
    self.mock_stream.AppendVarUInt32(byte_size)
    # Should then serialize itself to the encoder.
    self.mock_message.SerializeToString().AndReturn('foo')
    self.mock_stream.AppendRawBytes('foo')
    self.mock_stream.AppendVarUInt32(
        self.PackTag(1, wire_format.WIRETYPE_END_GROUP))

    self.mox.ReplayAll()
    self.encoder.AppendMessageSetItem(field_number, self.mock_message)
    self.mox.VerifyAll()

  def testAppendSFixed(self):
    # Most of our bounds-checking is done in output_stream.py,
    # but encoder.py is responsible for transforming signed
    # fixed-width integers into unsigned ones, so we test here
    # to ensure that we're not losing any entropy when we do
    # that conversion.
    field_number = 10
    self.assertRaises(message.EncodeError, self.encoder.AppendSFixed32,
        10, wire_format.UINT32_MAX + 1)
    self.assertRaises(message.EncodeError, self.encoder.AppendSFixed32,
        10, -(1 << 32))
    self.assertRaises(message.EncodeError, self.encoder.AppendSFixed64,
        10, wire_format.UINT64_MAX + 1)
    self.assertRaises(message.EncodeError, self.encoder.AppendSFixed64,
        10, -(1 << 64))


if __name__ == '__main__':
  unittest.main()
