# -*- coding: utf-8 -*-
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

import sys
import tracemalloc
import unittest

import pytest

from google.protobuf.internal import api_implementation
from google.protobuf.internal import encoder
from google.protobuf.internal import message_set_extensions_pb2
from google.protobuf.internal import missing_enum_values_pb2
from google.protobuf.internal import test_util
from google.protobuf.internal import testing_refleaks
from google.protobuf.internal import type_checkers
from google.protobuf.internal import wire_format
from google.protobuf import descriptor
from google.protobuf import unknown_fields
from google.protobuf import map_unittest_pb2
from google.protobuf import unittest_mset_pb2
from google.protobuf import unittest_pb2
from google.protobuf import unittest_proto3_arena_pb2


@testing_refleaks.TestCase
class TestUnknownFields(unittest.TestCase):
    def setUp(self):
        self.descriptor = unittest_pb2.TestAllTypes.DESCRIPTOR
        self.all_fields = unittest_pb2.TestAllTypes()
        test_util.SetAllFields(self.all_fields)
        self.all_fields_data = self.all_fields.SerializeToString()
        self.empty_message = unittest_pb2.TestEmptyMessage()
        self.empty_message.ParseFromString(self.all_fields_data)

    def test_serialize(self):
        data = self.empty_message.SerializeToString()

        # Don't use assertEqual because we don't want to dump raw binary data to
        # stdout.
        assert data == self.all_fields_data

    def test_serialize_proto3(self):
        # Verify proto3 unknown fields behavior.
        message = unittest_proto3_arena_pb2.TestEmptyMessage()
        message.ParseFromString(self.all_fields_data)
        assert self.all_fields_data == message.SerializeToString()

    def test_byte_size(self):
        assert self.all_fields.ByteSize() == self.empty_message.ByteSize()

    def test_list_fields(self):
        # Make sure ListFields doesn't return unknown fields.
        assert 0 == len(self.empty_message.ListFields())

    def test_serialize_message_set_wire_format_unknown_extension(self):
        # Create a message using the message set wire format with an unknown
        # message.
        raw = unittest_mset_pb2.RawMessageSet()

        # Add an unknown extension.
        item = raw.item.add()
        item.type_id = 98218603
        message1 = message_set_extensions_pb2.TestMessageSetExtension1()
        message1.i = 12345
        item.message = message1.SerializeToString()

        serialized = raw.SerializeToString()

        # Parse message using the message set wire format.
        proto = message_set_extensions_pb2.TestMessageSet()
        proto.MergeFromString(serialized)

        unknown_field_set = unknown_fields.UnknownFieldSet(proto)
        assert len(unknown_field_set) == 1
        # Unknown field should have wire format data which can be parsed back to
        # original message.
        assert unknown_field_set[0].field_number == item.type_id
        assert unknown_field_set[0].wire_type == \
                        wire_format.WIRETYPE_LENGTH_DELIMITED
        d = unknown_field_set[0].data
        message_new = message_set_extensions_pb2.TestMessageSetExtension1()
        message_new.ParseFromString(d)
        assert message1 == message_new

        # Verify that the unknown extension is serialized unchanged
        reserialized = proto.SerializeToString()
        new_raw = unittest_mset_pb2.RawMessageSet()
        new_raw.MergeFromString(reserialized)
        assert raw == new_raw

    def test_equals(self):
        message = unittest_pb2.TestEmptyMessage()
        message.ParseFromString(self.all_fields_data)
        assert self.empty_message == message

        self.all_fields.ClearField('optional_string')
        message.ParseFromString(self.all_fields.SerializeToString())
        assert self.empty_message != message

    def test_discard_unknown_fields(self):
        self.empty_message.DiscardUnknownFields()
        assert b'' == self.empty_message.SerializeToString()
        # Test message field and repeated message field.
        message = unittest_pb2.TestAllTypes()
        other_message = unittest_pb2.TestAllTypes()
        other_message.optional_string = 'discard'
        message.optional_nested_message.ParseFromString(
            other_message.SerializeToString())
        message.repeated_nested_message.add().ParseFromString(
            other_message.SerializeToString())
        assert b'' != message.optional_nested_message.SerializeToString()
        assert b'' != message.repeated_nested_message[0].SerializeToString()
        message.DiscardUnknownFields()
        assert b'' == message.optional_nested_message.SerializeToString()
        assert b'' == message.repeated_nested_message[0].SerializeToString()

        msg = map_unittest_pb2.TestMap()
        msg.map_int32_all_types[1].optional_nested_message.ParseFromString(
            other_message.SerializeToString())
        msg.map_string_string['1'] = 'test'
        assert (
            b'' !=
            msg.map_int32_all_types[1].optional_nested_message.SerializeToString())
        msg.DiscardUnknownFields()
        assert (
            b'' ==
            msg.map_int32_all_types[1].optional_nested_message.SerializeToString())


@testing_refleaks.TestCase
class TestUnknownFieldsAccessors(unittest.TestCase):
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
        if api_implementation.Type() != 'python':
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

    def CheckUnknownField(self, name, unknown_field_set, expected_value):
        field_descriptor = self.descriptor.fields_by_name[name]
        expected_type = type_checkers.FIELD_TYPE_TO_WIRE_TYPE[
            field_descriptor.type]
        for unknown_field in unknown_field_set:
            if unknown_field.field_number == field_descriptor.number:
                self.assertEqual(expected_type, unknown_field.wire_type)
                if expected_type == 3:
                    # Check group
                    self.assertEqual(expected_value[0],
                                    unknown_field.data[0].field_number)
                    self.assertEqual(expected_value[1], unknown_field.data[0].wire_type)
                    self.assertEqual(expected_value[2], unknown_field.data[0].data)
                    continue
                if expected_type == wire_format.WIRETYPE_LENGTH_DELIMITED:
                    self.assertIn(type(unknown_field.data), (str, bytes))
                if field_descriptor.label == descriptor.FieldDescriptor.LABEL_REPEATED:
                    self.assertIn(unknown_field.data, expected_value)
                else:
                    self.assertEqual(expected_value, unknown_field.data)

    def test_check_unknown_field_value(self):
        unknown_field_set = unknown_fields.UnknownFieldSet(self.empty_message)
        # Test enum.
        self.CheckUnknownField('optional_nested_enum',
                              unknown_field_set,
                              self.all_fields.optional_nested_enum)
        self.InternalCheckUnknownField('optional_nested_enum',
                                      self.all_fields.optional_nested_enum)

        # Test repeated enum.
        self.CheckUnknownField('repeated_nested_enum',
                              unknown_field_set,
                              self.all_fields.repeated_nested_enum)
        self.InternalCheckUnknownField('repeated_nested_enum',
                                      self.all_fields.repeated_nested_enum)

        # Test varint.
        self.CheckUnknownField('optional_int32',
                              unknown_field_set,
                              self.all_fields.optional_int32)
        self.InternalCheckUnknownField('optional_int32',
                                      self.all_fields.optional_int32)

        # Test fixed32.
        self.CheckUnknownField('optional_fixed32',
                              unknown_field_set,
                              self.all_fields.optional_fixed32)
        self.InternalCheckUnknownField('optional_fixed32',
                                      self.all_fields.optional_fixed32)

        # Test fixed64.
        self.CheckUnknownField('optional_fixed64',
                              unknown_field_set,
                              self.all_fields.optional_fixed64)
        self.InternalCheckUnknownField('optional_fixed64',
                                      self.all_fields.optional_fixed64)

        # Test length delimited.
        self.CheckUnknownField('optional_string',
                              unknown_field_set,
                              self.all_fields.optional_string.encode('utf-8'))
        self.InternalCheckUnknownField('optional_string',
                                      self.all_fields.optional_string)

        # Test group.
        self.CheckUnknownField('optionalgroup',
                              unknown_field_set,
                              (17, 0, 117))
        self.InternalCheckUnknownField('optionalgroup',
                                      self.all_fields.optionalgroup)

        assert 98 == len(unknown_field_set)

    def test_copy_from(self):
        message = unittest_pb2.TestEmptyMessage()
        message.CopyFrom(self.empty_message)
        assert message.SerializeToString() == self.all_fields_data

    def test_merge_from(self):
        message = unittest_pb2.TestAllTypes()
        message.optional_int32 = 1
        message.optional_uint32 = 2
        source = unittest_pb2.TestEmptyMessage()
        source.ParseFromString(message.SerializeToString())

        message.ClearField('optional_int32')
        message.optional_int64 = 3
        message.optional_uint32 = 4
        destination = unittest_pb2.TestEmptyMessage()
        unknown_field_set = unknown_fields.UnknownFieldSet(destination)
        assert 0 == len(unknown_field_set)
        destination.ParseFromString(message.SerializeToString())
        assert 0 == len(unknown_field_set)
        unknown_field_set = unknown_fields.UnknownFieldSet(destination)
        assert 2 == len(unknown_field_set)
        destination.MergeFrom(source)
        assert 2 == len(unknown_field_set)
        # Check that the fields where correctly merged, even stored in the unknown
        # fields set.
        message.ParseFromString(destination.SerializeToString())
        assert message.optional_int32 == 1
        assert message.optional_uint32 == 2
        assert message.optional_int64 == 3

    def test_clear(self):
        unknown_field_set = unknown_fields.UnknownFieldSet(self.empty_message)
        self.empty_message.Clear()
        # All cleared, even unknown fields.
        assert self.empty_message.SerializeToString() == b''
        assert len(unknown_field_set) == 98

    def test_unknown_fields_no_memory_leak(self):
        # Call to UnknownFields must not leak memory
        nb_leaks = 1234

        def leaking_function():
            for _ in range(nb_leaks):
                unknown_fields.UnknownFieldSet(self.empty_message)

        tracemalloc.start()
        snapshot1 = tracemalloc.take_snapshot()
        leaking_function()
        snapshot2 = tracemalloc.take_snapshot()
        top_stats = snapshot2.compare_to(snapshot1, 'lineno')
        tracemalloc.stop()
        # There's no easy way to look for a precise leak source.
        # Rely on a "marker" count value while checking allocated memory.
        assert [] == [x for x in top_stats if x.count_diff == nb_leaks]

    def test_sub_unknown_fields(self):
        message = unittest_pb2.TestAllTypes()
        message.optionalgroup.a = 123
        destination = unittest_pb2.TestEmptyMessage()
        destination.ParseFromString(message.SerializeToString())
        sub_unknown_fields = unknown_fields.UnknownFieldSet(destination)[0].data
        assert 1 == len(sub_unknown_fields)
        assert sub_unknown_fields[0].data == 123
        destination.Clear()
        assert 1 == len(sub_unknown_fields)
        assert sub_unknown_fields[0].data == 123
        message.Clear()
        message.optional_uint32 = 456
        nested_message = unittest_pb2.NestedTestAllTypes()
        nested_message.payload.optional_nested_message.ParseFromString(
            message.SerializeToString())
        unknown_field_set = unknown_fields.UnknownFieldSet(
            nested_message.payload.optional_nested_message)
        assert unknown_field_set[0].data == 456
        nested_message.ClearField('payload')
        assert unknown_field_set[0].data == 456
        unknown_field_set = unknown_fields.UnknownFieldSet(
            nested_message.payload.optional_nested_message)
        assert 0 == len(unknown_field_set)

    def test_unknown_field(self):
        message = unittest_pb2.TestAllTypes()
        message.optional_int32 = 123
        destination = unittest_pb2.TestEmptyMessage()
        destination.ParseFromString(message.SerializeToString())
        unknown_field = unknown_fields.UnknownFieldSet(destination)[0]
        destination.Clear()
        assert unknown_field.data == 123

    def test_unknown_extensions(self):
        message = unittest_pb2.TestEmptyMessageWithExtensions()
        message.ParseFromString(self.all_fields_data)
        assert len(unknown_fields.UnknownFieldSet(message)) == 98
        assert message.SerializeToString() == self.all_fields_data


@testing_refleaks.TestCase
class TestUnknownEnumValues(unittest.TestCase):
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
        unknown_field_set = unknown_fields.UnknownFieldSet(self.missing_message)
        self.assertIsInstance(unknown_field_set, unknown_fields.UnknownFieldSet)
        count = 0
        for field in unknown_field_set:
            if field.field_number == field_descriptor.number:
                count += 1
                if field_descriptor.label == descriptor.FieldDescriptor.LABEL_REPEATED:
                    assert field.data in expected_value
                else:
                    assert expected_value == field.data
        if field_descriptor.label == descriptor.FieldDescriptor.LABEL_REPEATED:
            assert count == len(expected_value)
        else:
            assert count == 1

    def test_unknown_parse_mismatch_enum_value(self):
        just_string = missing_enum_values_pb2.JustString()
        just_string.dummy = 'blah'

        missing = missing_enum_values_pb2.TestEnumValues()
        # The parse is invalid, storing the string proto into the set of
        # unknown fields.
        missing.ParseFromString(just_string.SerializeToString())

        # Fetching the enum field shouldn't crash, instead returning the
        # default value.
        assert missing.optional_nested_enum == 0

    def test_unknown_enum_value(self):
        self.assertFalse(self.missing_message.HasField('optional_nested_enum'))
        assert self.missing_message.optional_nested_enum == 2
        # Clear does not do anything.
        serialized = self.missing_message.SerializeToString()
        self.missing_message.ClearField('optional_nested_enum')
        assert self.missing_message.SerializeToString() == serialized

    def test_unknown_repeated_enum_value(self):
        assert [] == self.missing_message.repeated_nested_enum

    def test_unknown_packed_enum_value(self):
        assert [] == self.missing_message.packed_nested_enum

    def test_check_unknown_field_value_for_enum(self):
        unknown_field_set = unknown_fields.UnknownFieldSet(self.missing_message)
        assert len(unknown_field_set) == 5
        self.CheckUnknownField('optional_nested_enum',
                              self.message.optional_nested_enum)
        self.CheckUnknownField('repeated_nested_enum',
                              self.message.repeated_nested_enum)
        self.CheckUnknownField('packed_nested_enum',
                              self.message.packed_nested_enum)

    def test_round_trip(self):
        new_message = missing_enum_values_pb2.TestEnumValues()
        new_message.ParseFromString(self.missing_message.SerializeToString())
        assert self.message == new_message
