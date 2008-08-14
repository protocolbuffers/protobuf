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

"""Test for google.protobuf.internal.decoder."""

__author__ = 'robinson@google.com (Will Robinson)'

import struct
import unittest
from google.protobuf.internal import wire_format
from google.protobuf.internal import encoder
from google.protobuf.internal import decoder
import logging
from google.protobuf.internal import input_stream
from google.protobuf import message
import mox


class DecoderTest(unittest.TestCase):

  def setUp(self):
    self.mox = mox.Mox()
    self.mock_stream = self.mox.CreateMock(input_stream.InputStream)
    self.mock_message = self.mox.CreateMock(message.Message)

  def testReadFieldNumberAndWireType(self):
    # Test field numbers that will require various varint sizes.
    for expected_field_number in (1, 15, 16, 2047, 2048):
      for expected_wire_type in range(6):  # Highest-numbered wiretype is 5.
        e = encoder.Encoder()
        e._AppendTag(expected_field_number, expected_wire_type)
        s = e.ToString()
        d = decoder.Decoder(s)
        field_number, wire_type = d.ReadFieldNumberAndWireType()
        self.assertEqual(expected_field_number, field_number)
        self.assertEqual(expected_wire_type, wire_type)

  def ReadScalarTestHelper(self, test_name, decoder_method, expected_result,
                           expected_stream_method_name,
                           stream_method_return, *args):
    """Helper for testReadScalars below.

    Calls one of the Decoder.Read*() methods and ensures that the results are
    as expected.

    Args:
      test_name: Name of this test, used for logging only.
      decoder_method: Unbound decoder.Decoder method to call.
      expected_result: Value we expect returned from decoder_method().
      expected_stream_method_name: (string) Name of the InputStream
        method we expect Decoder to call to actually read the value
        on the wire.
      stream_method_return: Value our mocked-out stream method should
        return to the decoder.
      args: Additional arguments that we expect to be passed to the
        stream method.
    """
    logging.info('Testing %s scalar input.\n'
                 'Calling %r(), and expecting that to call the '
                 'stream method %s(%r), which will return %r.  Finally, '
                 'expecting the Decoder method to return %r'% (
        test_name, decoder_method,
        expected_stream_method_name, args, stream_method_return,
        expected_result))

    d = decoder.Decoder('')
    d._stream = self.mock_stream
    if decoder_method in (decoder.Decoder.ReadString,
                          decoder.Decoder.ReadBytes):
      self.mock_stream.ReadVarUInt32().AndReturn(len(stream_method_return))
    # We have to use names instead of methods to work around some
    # mox weirdness.  (ResetAll() is overzealous).
    expected_stream_method = getattr(self.mock_stream,
                                     expected_stream_method_name)
    expected_stream_method(*args).AndReturn(stream_method_return)

    self.mox.ReplayAll()
    self.assertEqual(expected_result, decoder_method(d))
    self.mox.VerifyAll()
    self.mox.ResetAll()

  def testReadScalars(self):
    test_string = 'I can feel myself getting sutpider.'
    scalar_tests = [
        ['int32', decoder.Decoder.ReadInt32, 0, 'ReadVarint32', 0],
        ['int64', decoder.Decoder.ReadInt64, 0, 'ReadVarint64', 0],
        ['uint32', decoder.Decoder.ReadUInt32, 0, 'ReadVarUInt32', 0],
        ['uint64', decoder.Decoder.ReadUInt64, 0, 'ReadVarUInt64', 0],
        ['fixed32', decoder.Decoder.ReadFixed32, 0xffffffff,
         'ReadLittleEndian32', 0xffffffff],
        ['fixed64', decoder.Decoder.ReadFixed64, 0xffffffffffffffff,
        'ReadLittleEndian64', 0xffffffffffffffff],
        ['sfixed32', decoder.Decoder.ReadSFixed32, -1,
         'ReadLittleEndian32', 0xffffffff],
        ['sfixed64', decoder.Decoder.ReadSFixed64, -1,
         'ReadLittleEndian64', 0xffffffffffffffff],
        ['float', decoder.Decoder.ReadFloat, 0.0,
         'ReadString', struct.pack('f', 0.0), 4],
        ['double', decoder.Decoder.ReadDouble, 0.0,
         'ReadString', struct.pack('d', 0.0), 8],
        ['bool', decoder.Decoder.ReadBool, True, 'ReadVarUInt32', 1],
        ['enum', decoder.Decoder.ReadEnum, 23, 'ReadVarUInt32', 23],
        ['string', decoder.Decoder.ReadString,
         test_string, 'ReadString', test_string, len(test_string)],
        ['bytes', decoder.Decoder.ReadBytes,
         test_string, 'ReadString', test_string, len(test_string)],
        # We test zigzag decoding routines more extensively below.
        ['sint32', decoder.Decoder.ReadSInt32, -1, 'ReadVarUInt32', 1],
        ['sint64', decoder.Decoder.ReadSInt64, -1, 'ReadVarUInt64', 1],
        ]
    # Ensure that we're testing different Decoder methods and using
    # different test names in all test cases above.
    self.assertEqual(len(scalar_tests), len(set(t[0] for t in scalar_tests)))
    self.assertEqual(len(scalar_tests), len(set(t[1] for t in scalar_tests)))
    for args in scalar_tests:
      self.ReadScalarTestHelper(*args)

  def testReadMessageInto(self):
    length = 23
    def Test(simulate_error):
      d = decoder.Decoder('')
      d._stream = self.mock_stream
      self.mock_stream.ReadVarUInt32().AndReturn(length)
      sub_buffer = object()
      self.mock_stream.GetSubBuffer(length).AndReturn(sub_buffer)

      if simulate_error:
        self.mock_message.MergeFromString(sub_buffer).AndReturn(length - 1)
        self.mox.ReplayAll()
        self.assertRaises(
            message.DecodeError, d.ReadMessageInto, self.mock_message)
      else:
        self.mock_message.MergeFromString(sub_buffer).AndReturn(length)
        self.mock_stream.SkipBytes(length)
        self.mox.ReplayAll()
        d.ReadMessageInto(self.mock_message)

      self.mox.VerifyAll()
      self.mox.ResetAll()

    Test(simulate_error=False)
    Test(simulate_error=True)

  def testReadGroupInto_Success(self):
    # Test both the empty and nonempty cases.
    for num_bytes in (5, 0):
      field_number = expected_field_number = 10
      d = decoder.Decoder('')
      d._stream = self.mock_stream
      sub_buffer = object()
      self.mock_stream.GetSubBuffer().AndReturn(sub_buffer)
      self.mock_message.MergeFromString(sub_buffer).AndReturn(num_bytes)
      self.mock_stream.SkipBytes(num_bytes)
      self.mock_stream.ReadVarUInt32().AndReturn(wire_format.PackTag(
          field_number, wire_format.WIRETYPE_END_GROUP))
      self.mox.ReplayAll()
      d.ReadGroupInto(expected_field_number, self.mock_message)
      self.mox.VerifyAll()
      self.mox.ResetAll()

  def ReadGroupInto_FailureTestHelper(self, bytes_read):
    d = decoder.Decoder('')
    d._stream = self.mock_stream
    sub_buffer = object()
    self.mock_stream.GetSubBuffer().AndReturn(sub_buffer)
    self.mock_message.MergeFromString(sub_buffer).AndReturn(bytes_read)
    return d

  def testReadGroupInto_NegativeBytesReported(self):
    expected_field_number = 10
    d = self.ReadGroupInto_FailureTestHelper(bytes_read=-1)
    self.mox.ReplayAll()
    self.assertRaises(message.DecodeError,
                      d.ReadGroupInto, expected_field_number,
                      self.mock_message)
    self.mox.VerifyAll()

  def testReadGroupInto_NoEndGroupTag(self):
    field_number = expected_field_number = 10
    num_bytes = 5
    d = self.ReadGroupInto_FailureTestHelper(bytes_read=num_bytes)
    self.mock_stream.SkipBytes(num_bytes)
    # Right field number, wrong wire type.
    self.mock_stream.ReadVarUInt32().AndReturn(wire_format.PackTag(
        field_number, wire_format.WIRETYPE_LENGTH_DELIMITED))
    self.mox.ReplayAll()
    self.assertRaises(message.DecodeError,
                      d.ReadGroupInto, expected_field_number,
                      self.mock_message)
    self.mox.VerifyAll()

  def testReadGroupInto_WrongFieldNumberInEndGroupTag(self):
    expected_field_number = 10
    field_number = expected_field_number + 1
    num_bytes = 5
    d = self.ReadGroupInto_FailureTestHelper(bytes_read=num_bytes)
    self.mock_stream.SkipBytes(num_bytes)
    # Wrong field number, right wire type.
    self.mock_stream.ReadVarUInt32().AndReturn(wire_format.PackTag(
        field_number, wire_format.WIRETYPE_END_GROUP))
    self.mox.ReplayAll()
    self.assertRaises(message.DecodeError,
                      d.ReadGroupInto, expected_field_number,
                      self.mock_message)
    self.mox.VerifyAll()

  def testSkipBytes(self):
    d = decoder.Decoder('')
    num_bytes = 1024
    self.mock_stream.SkipBytes(num_bytes)
    d._stream = self.mock_stream
    self.mox.ReplayAll()
    d.SkipBytes(num_bytes)
    self.mox.VerifyAll()

if __name__ == '__main__':
  unittest.main()
