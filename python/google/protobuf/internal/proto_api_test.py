# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Unittest for proto_api."""

import unittest

from google.protobuf.internal import more_extensions_pb2
from google.protobuf.internal import proto_api_example
from google.protobuf.internal import testing_refleaks

from google.protobuf import unittest_proto3_pb2


@testing_refleaks.TestCase
class ProtoApiTest(unittest.TestCase):

  def test_message_mutator_clear_generated_factory(self):
    msg = unittest_proto3_pb2.TestAllTypes(
        optional_int32=24, optional_string='optional_string'
    )
    self.assertEqual(24, msg.optional_int32)
    self.assertEqual('optional_string', msg.optional_string)
    self.assertTrue(proto_api_example.IsCppProtoLinked(msg))
    proto_api_example.ClearMessage(msg)
    self.assertEqual(0, msg.optional_int32)
    self.assertEqual('', msg.optional_string)

  def test_message_mutator_clear_dynamic_factory(self):
    msg = more_extensions_pb2.ForeignMessage(foreign_message_int=15)
    self.assertIn('foreign_message_int', msg)
    self.assertFalse(proto_api_example.IsCppProtoLinked(msg))
    proto_api_example.ClearMessage(msg)
    self.assertNotIn('foreign_message_int', msg)

  def test_not_a_message(self):
    with self.assertRaises(TypeError):
      proto_api_example.IsCppProtoLinked(112)
    with self.assertRaises(TypeError):
      proto_api_example.GetOptionalInt32(True)

  def test_message_mutator_parse(self):
    msg = unittest_proto3_pb2.TestAllTypes(
        optional_int32=24, optional_string='optional_string'
    )
    proto_api_example.ParseMessage('optional_string: "changed"', msg)
    self.assertEqual(0, msg.optional_int32)
    self.assertEqual(msg.optional_string, 'changed')

  def test_message_const_pointer_get(self):
    msg = unittest_proto3_pb2.TestAllTypes(optional_int32=123)
    self.assertEqual(123, proto_api_example.GetOptionalInt32(msg))

  def test_mutate_python_message_while_const_pinter_alive(self):
    msg = unittest_proto3_pb2.TestAllTypes()
    self.assertTrue(proto_api_example.MutateConstAlive(msg))


if __name__ == '__main__':
  unittest.main()
