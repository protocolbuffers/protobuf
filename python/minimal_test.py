# Protocol Buffers - Google's data interchange format
# Copyright 2023 Google LLC.  All rights reserved.
# https://developers.google.com/protocol-buffers/
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
#     * Neither the name of Google LLC nor the names of its
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

"""A bare-bones unit test that doesn't load any generated code."""


import unittest
from google.protobuf.pyext import _message
from google3.net.proto2.python.internal import api_implementation
from google.protobuf import unittest_pb2
from google.protobuf import map_unittest_pb2
from google.protobuf import descriptor_pool
from google.protobuf import text_format
from google.protobuf import message_factory
from google.protobuf import message
from google3.net.proto2.python.internal import factory_test1_pb2
from google3.net.proto2.python.internal import factory_test2_pb2
from google3.net.proto2.python.internal import more_extensions_pb2
from google.protobuf import descriptor_pb2

class TestMessageExtension(unittest.TestCase):

    def test_descriptor_pool(self):
        serialized_desc = b'\n\ntest.proto\"\x0e\n\x02M1*\x08\x08\x01\x10\x80\x80\x80\x80\x02:\x15\n\x08test_ext\x12\x03.M1\x18\x01 \x01(\x05'
        pool = _message.DescriptorPool()
        file_desc = pool.AddSerializedFile(serialized_desc)
        self.assertEqual("test.proto", file_desc.name)
        ext_desc = pool.FindExtensionByName("test_ext")
        self.assertEqual(1, ext_desc.number)

        # Test object cache: repeatedly retrieving the same descriptor
        # should result in the same object
        self.assertIs(ext_desc, pool.FindExtensionByName("test_ext"))


    def test_lib_is_upb(self):
        # Ensure we are not pulling in a different protobuf library on the
        # system.
        print(_message._IS_UPB)
        self.assertTrue(_message._IS_UPB)
        self.assertEqual(api_implementation.Type(), "cpp")

    def test_repeated_field_slice_delete(self):
        def test_slice(start, end, step):
            vals = list(range(20))
            message = unittest_pb2.TestAllTypes(repeated_int32=vals)
            del vals[start:end:step]
            del message.repeated_int32[start:end:step]
            self.assertEqual(vals, list(message.repeated_int32))
        test_slice(3, 11, 1)
        test_slice(3, 11, 2)
        test_slice(3, 11, 3)
        test_slice(11, 3, -1)
        test_slice(11, 3, -2)
        test_slice(11, 3, -3)
        test_slice(10, 25, 4)
    
    def testExtensionsErrors(self):
        msg = unittest_pb2.TestAllTypes()
        self.assertRaises(AttributeError, getattr, msg, 'Extensions')
    
    def testClearStubMapField(self):
        msg = map_unittest_pb2.TestMapSubmessage()
        int32_map = msg.test_map.map_int32_int32
        msg.test_map.ClearField("map_int32_int32")
        int32_map[123] = 456
        self.assertEqual(0, msg.test_map.ByteSize())

    def testClearReifiedMapField(self):
        msg = map_unittest_pb2.TestMap()
        int32_map = msg.map_int32_int32
        int32_map[123] = 456
        msg.ClearField("map_int32_int32")
        int32_map[111] = 222
        self.assertEqual(0, msg.ByteSize())

    def testClearStubRepeatedField(self):
        msg = unittest_pb2.NestedTestAllTypes()
        int32_array = msg.payload.repeated_int32
        msg.payload.ClearField("repeated_int32")
        int32_array.append(123)
        self.assertEqual(0, msg.payload.ByteSize())

    def testClearReifiedRepeatdField(self):
        msg = unittest_pb2.TestAllTypes()
        int32_array = msg.repeated_int32
        int32_array.append(123)
        self.assertNotEqual(0, msg.ByteSize())
        msg.ClearField("repeated_int32")
        int32_array.append(123)
        self.assertEqual(0, msg.ByteSize())

    def testFloatPrinting(self):
        message = unittest_pb2.TestAllTypes()
        message.optional_float = -0.0
        self.assertEqual(str(message), 'optional_float: -0\n')

class OversizeProtosTest(unittest.TestCase):
  def setUp(self):
    msg = unittest_pb2.NestedTestAllTypes()
    m = msg
    for i in range(101):
      m = m.child
    m.Clear()
    self.p_serialized = msg.SerializeToString()
    
  def testAssertOversizeProto(self):
    from google.protobuf.pyext._message import SetAllowOversizeProtos
    SetAllowOversizeProtos(False)
    q = unittest_pb2.NestedTestAllTypes()
    with self.assertRaises(message.DecodeError):
      q.ParseFromString(self.p_serialized)
      print(q)
  
  def testSucceedOversizeProto(self):
    from google.protobuf.pyext._message import SetAllowOversizeProtos
    SetAllowOversizeProtos(True)
    q = unittest_pb2.NestedTestAllTypes()
    q.ParseFromString(self.p_serialized)

  def testExtensionIter(self):
    extendee_proto = more_extensions_pb2.ExtendedMessage()

    extension_int32 = more_extensions_pb2.optional_int_extension
    extendee_proto.Extensions[extension_int32] = 23

    extension_repeated = more_extensions_pb2.repeated_int_extension
    extendee_proto.Extensions[extension_repeated].append(11)

    extension_msg = more_extensions_pb2.optional_message_extension
    extendee_proto.Extensions[extension_msg].foreign_message_int = 56

    # Set some normal fields.
    extendee_proto.optional_int32 = 1
    extendee_proto.repeated_string.append('hi')

    expected = {
        extension_int32: True,
        extension_msg: True,
        extension_repeated: True
    }
    count = 0
    for item in extendee_proto.Extensions:
      del expected[item]
      self.assertIn(item, extendee_proto.Extensions)
      count += 1
    self.assertEqual(count, 3)
    self.assertEqual(len(expected), 0)
  
  def testIsInitializedStub(self):
    proto = unittest_pb2.TestRequiredForeign()
    self.assertTrue(proto.IsInitialized())
    self.assertFalse(proto.optional_message.IsInitialized())
    errors = []
    self.assertFalse(proto.optional_message.IsInitialized(errors))
    self.assertEqual(['a', 'b', 'c'], errors)
    self.assertRaises(message.EncodeError, proto.optional_message.SerializeToString)

if __name__ == '__main__':
    unittest.main(verbosity=2)
