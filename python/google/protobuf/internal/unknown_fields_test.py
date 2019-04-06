#! /usr/bin/env python
# -*- coding: utf-8 -*-
#
# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
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

"""Test for preservation of unknown fields in the pure Python implementation."""

__author__ = 'bohdank@google.com (Bohdan Koval)'

try:
  import unittest2 as unittest  #PY26
except ImportError:
  import unittest
from google.protobuf import map_unittest_pb2
from google.protobuf import unittest_mset_pb2
from google.protobuf import unittest_pb2
from google.protobuf import unittest_proto3_arena_pb2
from google.protobuf.internal import api_implementation
from google.protobuf.internal import encoder
from google.protobuf.internal import message_set_extensions_pb2
from google.protobuf.internal import missing_enum_values_pb2
from google.protobuf.internal import test_util
from google.protobuf.internal import testing_refleaks
from google.protobuf.internal import type_checkers
from google.protobuf import descriptor


BaseTestCase = testing_refleaks.BaseTestCase


class UnknownFieldsTest(BaseTestCase):

  def setUp(self):
    self.descriptor = unittest_pb2.TestAllTypes.DESCRIPTOR
    self.all_fields = unittest_pb2.TestAllTypes()
    test_util.SetAllFields(self.all_fields)
    self.all_fields_data = self.all_fields.SerializeToString()
    self.empty_message = unittest_pb2.TestEmptyMessage()
    self.empty_message.ParseFromString(self.all_fields_data)

  def testSerialize(self):
    data = self.empty_message.SerializeToString()

    # Don't use assertEqual because we don't want to dump raw binary data to
    # stdout.
    self.assertTrue(data == self.all_fields_data)

  def testSerializeProto3(self):
    # Verify proto3 unknown fields behavior.
    message = unittest_proto3_arena_pb2.TestEmptyMessage()
    message.ParseFromString(self.all_fields_data)
    self.assertEqual(self.all_fields_data, message.SerializeToString())

  def testByteSize(self):
    self.assertEqual(self.all_fields.ByteSize(), self.empty_message.ByteSize())

  def testListFields(self):
    # Make sure ListFields doesn't return unknown fields.
    self.assertEqual(0, len(self.empty_message.ListFields()))

  def testSerializeMessageSetWireFormatUnknownExtension(self):
    # Create a message using the message set wire format with an unknown
    # message.
    raw = unittest_mset_pb2.RawMessageSet()

    # Add an unknown extension.
    item = raw.item.add()
    item.type_id = 98418603
    message1 = message_set_extensions_pb2.TestMessageSetExtension1()
    message1.i = 12345
    item.message = message1.SerializeToString()

    serialized = raw.SerializeToString()

    # Parse message using the message set wire format.
    proto = message_set_extensions_pb2.TestMessageSet()
    proto.MergeFromString(serialized)

    # Verify that the unknown extension is serialized unchanged
    reserialized = proto.SerializeToString()
    new_raw = unittest_mset_pb2.RawMessageSet()
    new_raw.MergeFromString(reserialized)
    self.assertEqual(raw, new_raw)

  def testEquals(self):
    message = unittest_pb2.TestEmptyMessage()
    message.ParseFromString(self.all_fields_data)
    self.assertEqual(self.empty_message, message)

    self.all_fields.ClearField('optional_string')
    message.ParseFromString(self.all_fields.SerializeToString())
    self.assertNotEqual(self.empty_message, message)

  def testDiscardUnknownFields(self):
    self.empty_message.DiscardUnknownFields()
    self.assertEqual(b'', self.empty_message.SerializeToString())
    # Test message field and repeated message field.
    message = unittest_pb2.TestAllTypes()
    other_message = unittest_pb2.TestAllTypes()
    other_message.optional_string = 'discard'
    message.optional_nested_message.ParseFromString(
        other_message.SerializeToString())
    message.repeated_nested_message.add().ParseFromString(
        other_message.SerializeToString())
    self.assertNotEqual(
        b'', message.optional_nested_message.SerializeToString())
    self.assertNotEqual(
        b'', message.repeated_nested_message[0].SerializeToString())
    message.DiscardUnknownFields()
    self.assertEqual(b'', message.optional_nested_message.SerializeToString())
    self.assertEqual(
        b'', message.repeated_nested_message[0].SerializeToString())

    msg = map_unittest_pb2.TestMap()
    msg.map_int32_all_types[1].optional_nested_message.ParseFromString(
        other_message.SerializeToString())
    msg.map_string_string['1'] = 'test'
    self.assertNotEqual(
        b'',
        msg.map_int32_all_types[1].optional_nested_message.SerializeToString())
    msg.DiscardUnknownFields()
    self.assertEqual(
        b'',
        msg.map_int32_all_types[1].optional_nested_message.SerializeToString())


class UnknownFieldsAccessorsTest(BaseTestCase):

  def setUp(self):
    self.descriptor = unittest_pb2.TestAllTypes.DESCRIPTOR
    self.all_fields = unittest_pb2.TestAllTypes()
    test_util.SetAllFields(self.all_fields)
    self.all_fields_data = self.all_fields.SerializeToString()
    self.empty_message = unittest_pb2.TestEmptyMessage()
    self.empty_message.ParseFromString(self.all_fields_data)

  # InternalCheckUnknownField() is an additional Pure Python check which checks
  # a detail of unknown fields. It cannot be used by the C++
  # implementation because some protect members are called.
  # The test is added for historical reasons. It is not necessary as
  # serialized string is checked.
  # TODO(jieluo): Remove message._unknown_fields.
  def InternalCheckUnknownField(self, name, expected_value):
    if api_implementation.Type() == 'cpp':
      return
    field_descriptor = self.descriptor.fields_by_name[name]
    wire_type = type_checkers.FIELD_TYPE_TO_WIRE_TYPE[field_descriptor.type]
    field_tag = encoder.TagBytes(field_descriptor.number, wire_type)
    result_dict = {}
    for tag_bytes, value in self.empty_message._unknown_fields:
      if tag_bytes == field_tag:
        decoder = unittest_pb2.TestAllTypes._decoders_by_tag[tag_bytes][0]
        decoder(memoryview(value), 0, len(value), self.all_fields, result_dict)
    self.assertEqual(expected_value, result_dict[field_descriptor])

  def CheckUnknownField(self, name, unknown_fields, expected_value):
    field_descriptor = self.descriptor.fields_by_name[name]
    expected_type = type_checkers.FIELD_TYPE_TO_WIRE_TYPE[
        field_descriptor.type]
    for unknown_field in unknown_fields:
      if unknown_field.field_number == field_descriptor.number:
        self.assertEqual(expected_type, unknown_field.wire_type)
        if expected_type == 3:
          # Check group
          self.assertEqual(expected_value[0],
                           unknown_field.data[0].field_number)
          self.assertEqual(expected_value[1], unknown_field.data[0].wire_type)
          self.assertEqual(expected_value[2], unknown_field.data[0].data)
          continue
        if field_descriptor.label == descriptor.FieldDescriptor.LABEL_REPEATED:
          self.assertIn(unknown_field.data, expected_value)
        else:
          self.assertEqual(expected_value, unknown_field.data)

  def testCheckUnknownFieldValue(self):
    unknown_fields = self.empty_message.UnknownFields()
    # Test enum.
    self.CheckUnknownField('optional_nested_enum',
                           unknown_fields,
                           self.all_fields.optional_nested_enum)
    self.InternalCheckUnknownField('optional_nested_enum',
                                   self.all_fields.optional_nested_enum)

    # Test repeated enum.
    self.CheckUnknownField('repeated_nested_enum',
                           unknown_fields,
                           self.all_fields.repeated_nested_enum)
    self.InternalCheckUnknownField('repeated_nested_enum',
                                   self.all_fields.repeated_nested_enum)

    # Test varint.
    self.CheckUnknownField('optional_int32',
                           unknown_fields,
                           self.all_fields.optional_int32)
    self.InternalCheckUnknownField('optional_int32',
                                   self.all_fields.optional_int32)

    # Test fixed32.
    self.CheckUnknownField('optional_fixed32',
                           unknown_fields,
                           self.all_fields.optional_fixed32)
    self.InternalCheckUnknownField('optional_fixed32',
                                   self.all_fields.optional_fixed32)

    # Test fixed64.
    self.CheckUnknownField('optional_fixed64',
                           unknown_fields,
                           self.all_fields.optional_fixed64)
    self.InternalCheckUnknownField('optional_fixed64',
                                   self.all_fields.optional_fixed64)

    # Test lengthd elimited.
    self.CheckUnknownField('optional_string',
                           unknown_fields,
                           self.all_fields.optional_string.encode('utf-8'))
    self.InternalCheckUnknownField('optional_string',
                                   self.all_fields.optional_string)

    # Test group.
    self.CheckUnknownField('optionalgroup',
                           unknown_fields,
                           (17, 0, 117))
    self.InternalCheckUnknownField('optionalgroup',
                                   self.all_fields.optionalgroup)

    self.assertEqual(97, len(unknown_fields))

  def testCopyFrom(self):
    message = unittest_pb2.TestEmptyMessage()
    message.CopyFrom(self.empty_message)
    self.assertEqual(message.SerializeToString(), self.all_fields_data)

  def testMergeFrom(self):
    message = unittest_pb2.TestAllTypes()
    message.optional_int32 = 1
    message.optional_uint32 = 2
    source = unittest_pb2.TestEmptyMessage()
    source.ParseFromString(message.SerializeToString())

    message.ClearField('optional_int32')
    message.optional_int64 = 3
    message.optional_uint32 = 4
    destination = unittest_pb2.TestEmptyMessage()
    unknown_fields = destination.UnknownFields()
    self.assertEqual(0, len(unknown_fields))
    destination.ParseFromString(message.SerializeToString())
    # ParseFromString clears the message thus unknown fields is invalid.
    with self.assertRaises(ValueError) as context:
      len(unknown_fields)
    self.assertIn('UnknownFields does not exist.',
                  str(context.exception))
    unknown_fields = destination.UnknownFields()
    self.assertEqual(2, len(unknown_fields))
    destination.MergeFrom(source)
    self.assertEqual(4, len(unknown_fields))
    # Check that the fields where correctly merged, even stored in the unknown
    # fields set.
    message.ParseFromString(destination.SerializeToString())
    self.assertEqual(message.optional_int32, 1)
    self.assertEqual(message.optional_uint32, 2)
    self.assertEqual(message.optional_int64, 3)

  def testClear(self):
    unknown_fields = self.empty_message.UnknownFields()
    self.empty_message.Clear()
    # All cleared, even unknown fields.
    self.assertEqual(self.empty_message.SerializeToString(), b'')
    with self.assertRaises(ValueError) as context:
      len(unknown_fields)
    self.assertIn('UnknownFields does not exist.',
                  str(context.exception))

  def testSubUnknownFields(self):
    message = unittest_pb2.TestAllTypes()
    message.optionalgroup.a = 123
    destination = unittest_pb2.TestEmptyMessage()
    destination.ParseFromString(message.SerializeToString())
    sub_unknown_fields = destination.UnknownFields()[0].data
    self.assertEqual(1, len(sub_unknown_fields))
    self.assertEqual(sub_unknown_fields[0].data, 123)
    destination.Clear()
    with self.assertRaises(ValueError) as context:
      len(sub_unknown_fields)
    self.assertIn('UnknownFields does not exist.',
                  str(context.exception))
    with self.assertRaises(ValueError) as context:
      # pylint: disable=pointless-statement
      sub_unknown_fields[0]
    self.assertIn('UnknownFields does not exist.',
                  str(context.exception))
    message.Clear()
    message.optional_uint32 = 456
    nested_message = unittest_pb2.NestedTestAllTypes()
    nested_message.payload.optional_nested_message.ParseFromString(
        message.SerializeToString())
    unknown_fields = (
        nested_message.payload.optional_nested_message.UnknownFields())
    self.assertEqual(unknown_fields[0].data, 456)
    nested_message.ClearField('payload')
    self.assertEqual(unknown_fields[0].data, 456)
    unknown_fields = (
        nested_message.payload.optional_nested_message.UnknownFields())
    self.assertEqual(0, len(unknown_fields))

  def testUnknownField(self):
    message = unittest_pb2.TestAllTypes()
    message.optional_int32 = 123
    destination = unittest_pb2.TestEmptyMessage()
    destination.ParseFromString(message.SerializeToString())
    unknown_field = destination.UnknownFields()[0]
    destination.Clear()
    with self.assertRaises(ValueError) as context:
      unknown_field.data    # pylint: disable=pointless-statement
    self.assertIn('The parent message might be cleared.',
                  str(context.exception))

  def testUnknownExtensions(self):
    message = unittest_pb2.TestEmptyMessageWithExtensions()
    message.ParseFromString(self.all_fields_data)
    self.assertEqual(message.SerializeToString(), self.all_fields_data)


class UnknownEnumValuesTest(BaseTestCase):

  def setUp(self):
    self.descriptor = missing_enum_values_pb2.TestEnumValues.DESCRIPTOR

    self.message = missing_enum_values_pb2.TestEnumValues()
    # TestEnumValues.ZERO = 0, but does not exist in the other NestedEnum.
    self.message.optional_nested_enum = (
        missing_enum_values_pb2.TestEnumValues.ZERO)
    self.message.repeated_nested_enum.extend([
        missing_enum_values_pb2.TestEnumValues.ZERO,
        missing_enum_values_pb2.TestEnumValues.ONE,
        ])
    self.message.packed_nested_enum.extend([
        missing_enum_values_pb2.TestEnumValues.ZERO,
        missing_enum_values_pb2.TestEnumValues.ONE,
        ])
    self.message_data = self.message.SerializeToString()
    self.missing_message = missing_enum_values_pb2.TestMissingEnumValues()
    self.missing_message.ParseFromString(self.message_data)

  # CheckUnknownField() is an additional Pure Python check which checks
  # a detail of unknown fields. It cannot be used by the C++
  # implementation because some protect members are called.
  # The test is added for historical reasons. It is not necessary as
  # serialized string is checked.

  def CheckUnknownField(self, name, expected_value):
    field_descriptor = self.descriptor.fields_by_name[name]
    unknown_fields = self.missing_message.UnknownFields()
    for field in unknown_fields:
      if field.field_number == field_descriptor.number:
        if field_descriptor.label == descriptor.FieldDescriptor.LABEL_REPEATED:
          self.assertIn(field.data, expected_value)
        else:
          self.assertEqual(expected_value, field.data)

  def testUnknownParseMismatchEnumValue(self):
    just_string = missing_enum_values_pb2.JustString()
    just_string.dummy = 'blah'

    missing = missing_enum_values_pb2.TestEnumValues()
    # The parse is invalid, storing the string proto into the set of
    # unknown fields.
    missing.ParseFromString(just_string.SerializeToString())

    # Fetching the enum field shouldn't crash, instead returning the
    # default value.
    self.assertEqual(missing.optional_nested_enum, 0)

  def testUnknownEnumValue(self):
    self.assertFalse(self.missing_message.HasField('optional_nested_enum'))
    self.assertEqual(self.missing_message.optional_nested_enum, 2)
    # Clear does not do anything.
    serialized = self.missing_message.SerializeToString()
    self.missing_message.ClearField('optional_nested_enum')
    self.assertEqual(self.missing_message.SerializeToString(), serialized)

  def testUnknownRepeatedEnumValue(self):
    self.assertEqual([], self.missing_message.repeated_nested_enum)

  def testUnknownPackedEnumValue(self):
    self.assertEqual([], self.missing_message.packed_nested_enum)

  def testCheckUnknownFieldValueForEnum(self):
    self.CheckUnknownField('optional_nested_enum',
                           self.message.optional_nested_enum)
    self.CheckUnknownField('repeated_nested_enum',
                           self.message.repeated_nested_enum)
    self.CheckUnknownField('packed_nested_enum',
                           self.message.packed_nested_enum)

  def testRoundTrip(self):
    new_message = missing_enum_values_pb2.TestEnumValues()
    new_message.ParseFromString(self.missing_message.SerializeToString())
    self.assertEqual(self.message, new_message)


if __name__ == '__main__':
  unittest.main()
