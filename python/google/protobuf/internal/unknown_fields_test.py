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
from google.protobuf import unittest_mset_pb2
from google.protobuf import unittest_pb2
from google.protobuf import unittest_proto3_arena_pb2
from google.protobuf.internal import api_implementation
from google.protobuf.internal import encoder
from google.protobuf.internal import message_set_extensions_pb2
from google.protobuf.internal import missing_enum_values_pb2
from google.protobuf.internal import test_util
from google.protobuf.internal import type_checkers


def SkipIfCppImplementation(func):
  return unittest.skipIf(
      api_implementation.Type() == 'cpp' and api_implementation.Version() == 2,
      'C++ implementation does not expose unknown fields to Python')(func)


class UnknownFieldsTest(unittest.TestCase):

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
    # Verify that proto3 doesn't preserve unknown fields.
    message = unittest_proto3_arena_pb2.TestEmptyMessage()
    message.ParseFromString(self.all_fields_data)
    self.assertEqual(0, len(message.SerializeToString()))

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


class UnknownFieldsAccessorsTest(unittest.TestCase):

  def setUp(self):
    self.descriptor = unittest_pb2.TestAllTypes.DESCRIPTOR
    self.all_fields = unittest_pb2.TestAllTypes()
    test_util.SetAllFields(self.all_fields)
    self.all_fields_data = self.all_fields.SerializeToString()
    self.empty_message = unittest_pb2.TestEmptyMessage()
    self.empty_message.ParseFromString(self.all_fields_data)
    if api_implementation.Type() != 'cpp':
      # _unknown_fields is an implementation detail.
      self.unknown_fields = self.empty_message._unknown_fields

  # All the tests that use GetField() check an implementation detail of the
  # Python implementation, which stores unknown fields as serialized strings.
  # These tests are skipped by the C++ implementation: it's enough to check that
  # the message is correctly serialized.

  def GetField(self, name):
    field_descriptor = self.descriptor.fields_by_name[name]
    wire_type = type_checkers.FIELD_TYPE_TO_WIRE_TYPE[field_descriptor.type]
    field_tag = encoder.TagBytes(field_descriptor.number, wire_type)
    result_dict = {}
    for tag_bytes, value in self.unknown_fields:
      if tag_bytes == field_tag:
        decoder = unittest_pb2.TestAllTypes._decoders_by_tag[tag_bytes][0]
        decoder(value, 0, len(value), self.all_fields, result_dict)
    return result_dict[field_descriptor]

  @SkipIfCppImplementation
  def testEnum(self):
    value = self.GetField('optional_nested_enum')
    self.assertEqual(self.all_fields.optional_nested_enum, value)

  @SkipIfCppImplementation
  def testRepeatedEnum(self):
    value = self.GetField('repeated_nested_enum')
    self.assertEqual(self.all_fields.repeated_nested_enum, value)

  @SkipIfCppImplementation
  def testVarint(self):
    value = self.GetField('optional_int32')
    self.assertEqual(self.all_fields.optional_int32, value)

  @SkipIfCppImplementation
  def testFixed32(self):
    value = self.GetField('optional_fixed32')
    self.assertEqual(self.all_fields.optional_fixed32, value)

  @SkipIfCppImplementation
  def testFixed64(self):
    value = self.GetField('optional_fixed64')
    self.assertEqual(self.all_fields.optional_fixed64, value)

  @SkipIfCppImplementation
  def testLengthDelimited(self):
    value = self.GetField('optional_string')
    self.assertEqual(self.all_fields.optional_string, value)

  @SkipIfCppImplementation
  def testGroup(self):
    value = self.GetField('optionalgroup')
    self.assertEqual(self.all_fields.optionalgroup, value)

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
    destination.ParseFromString(message.SerializeToString())

    destination.MergeFrom(source)
    # Check that the fields where correctly merged, even stored in the unknown
    # fields set.
    message.ParseFromString(destination.SerializeToString())
    self.assertEqual(message.optional_int32, 1)
    self.assertEqual(message.optional_uint32, 2)
    self.assertEqual(message.optional_int64, 3)

  def testClear(self):
    self.empty_message.Clear()
    # All cleared, even unknown fields.
    self.assertEqual(self.empty_message.SerializeToString(), b'')

  def testUnknownExtensions(self):
    message = unittest_pb2.TestEmptyMessageWithExtensions()
    message.ParseFromString(self.all_fields_data)
    self.assertEqual(message.SerializeToString(), self.all_fields_data)


class UnknownEnumValuesTest(unittest.TestCase):

  def setUp(self):
    self.descriptor = missing_enum_values_pb2.TestEnumValues.DESCRIPTOR

    self.message = missing_enum_values_pb2.TestEnumValues()
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
    if api_implementation.Type() != 'cpp':
      # _unknown_fields is an implementation detail.
      self.unknown_fields = self.missing_message._unknown_fields

  # All the tests that use GetField() check an implementation detail of the
  # Python implementation, which stores unknown fields as serialized strings.
  # These tests are skipped by the C++ implementation: it's enough to check that
  # the message is correctly serialized.

  def GetField(self, name):
    field_descriptor = self.descriptor.fields_by_name[name]
    wire_type = type_checkers.FIELD_TYPE_TO_WIRE_TYPE[field_descriptor.type]
    field_tag = encoder.TagBytes(field_descriptor.number, wire_type)
    result_dict = {}
    for tag_bytes, value in self.unknown_fields:
      if tag_bytes == field_tag:
        decoder = missing_enum_values_pb2.TestEnumValues._decoders_by_tag[
          tag_bytes][0]
        decoder(value, 0, len(value), self.message, result_dict)
    return result_dict[field_descriptor]

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

  @SkipIfCppImplementation
  def testUnknownEnumValue(self):
    self.assertFalse(self.missing_message.HasField('optional_nested_enum'))
    value = self.GetField('optional_nested_enum')
    self.assertEqual(self.message.optional_nested_enum, value)

  @SkipIfCppImplementation
  def testUnknownRepeatedEnumValue(self):
    value = self.GetField('repeated_nested_enum')
    self.assertEqual(self.message.repeated_nested_enum, value)

  @SkipIfCppImplementation
  def testUnknownPackedEnumValue(self):
    value = self.GetField('packed_nested_enum')
    self.assertEqual(self.message.packed_nested_enum, value)

  def testRoundTrip(self):
    new_message = missing_enum_values_pb2.TestEnumValues()
    new_message.ParseFromString(self.missing_message.SerializeToString())
    self.assertEqual(self.message, new_message)


if __name__ == '__main__':
  unittest.main()
