# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
# http://code.google.com/p/protobuf/
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""Test for google.protobuf.internal.encoder."""

__author__ = 'robinson@google.com (Will Robinson)'

import struct
import logging
import unittest
from google.protobuf.internal import wire_format
from google.protobuf.internal import encoder
from google.protobuf.internal import output_stream
from google.protobuf import message
import mox


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
                             wire_type, field_value,
                             expected_value=None, expected_length=None):
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
      expected_length: The length we expect Encoder to pass to the
        AppendVarUInt32 method. If None we expect the length of the
        field_value.
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
      if expected_length is None:
        expected_length = len(field_value)
      self.mock_stream.AppendVarUInt32(expected_length)
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
    utf8_bytes = '\xd0\xa2\xd0\xb5\xd1\x81\xd1\x82'
    utf8_string = unicode(utf8_bytes, 'utf-8')
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
        ['utf8-string', self.encoder.AppendString, 'AppendRawBytes',
         wire_format.WIRETYPE_LENGTH_DELIMITED, utf8_string,
         utf8_bytes, len(utf8_bytes)],
        # We test zigzag encoding routines more extensively below.
        ['sint32', self.encoder.AppendSInt32, 'AppendVarUInt32',
         wire_format.WIRETYPE_VARINT, -1, 1],
        ['sint64', self.encoder.AppendSInt64, 'AppendVarUInt64',
         wire_format.WIRETYPE_VARINT, -1, 1],
        ]
    # Ensure that we're testing different Encoder methods and using
    # different test names in all test cases above.
    self.assertEqual(len(scalar_tests), len(set(t[0] for t in scalar_tests)))
    self.assert_(len(scalar_tests) >= len(set(t[1] for t in scalar_tests)))
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
