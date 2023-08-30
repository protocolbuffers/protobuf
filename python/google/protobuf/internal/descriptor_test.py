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

"""Unittest for google.protobuf.internal.descriptor."""

__author__ = 'robinson@google.com (Will Robinson)'

import warnings

import pytest

from google.protobuf import descriptor_pb2
from google.protobuf.internal import api_implementation
from google.protobuf import descriptor
from google.protobuf import descriptor_pool
from google.protobuf import symbol_database
from google.protobuf import text_format
from google.protobuf import unittest_custom_options_pb2
from google.protobuf import unittest_import_pb2
from google.protobuf import unittest_pb2


TEST_EMPTY_MESSAGE_DESCRIPTOR_ASCII = """
name: 'TestEmptyMessage'
"""

TEST_FILE_DESCRIPTOR_DEBUG = """syntax = "proto2";

package protobuf_unittest;

message NestedMessage {
  enum ForeignEnum {
    FOREIGN_FOO = 4;
    FOREIGN_BAR = 5;
    FOREIGN_BAZ = 6;
  }
  optional int32 bb = 1;
}

message ResponseMessage {
}

service DescriptorTestService {
  rpc CallMethod(.protobuf_unittest.NestedMessage) returns (.protobuf_unittest.ResponseMessage);
}

"""


warnings.simplefilter('error', DeprecationWarning)


class TestDescriptor:
    def setup_method(self, method):
        file_proto = descriptor_pb2.FileDescriptorProto(
            name='some/filename/some.proto',
            package='protobuf_unittest')
        message_proto = file_proto.message_type.add(
            name='NestedMessage')
        message_proto.field.add(
            name='bb',
            number=1,
            type=descriptor_pb2.FieldDescriptorProto.TYPE_INT32,
            label=descriptor_pb2.FieldDescriptorProto.LABEL_OPTIONAL)
        enum_proto = message_proto.enum_type.add(
            name='ForeignEnum')
        enum_proto.value.add(name='FOREIGN_FOO', number=4)
        enum_proto.value.add(name='FOREIGN_BAR', number=5)
        enum_proto.value.add(name='FOREIGN_BAZ', number=6)

        file_proto.message_type.add(name='ResponseMessage')
        service_proto = file_proto.service.add(name='DescriptorTestService')
        method_proto = service_proto.method.add(
            name='CallMethod',
            input_type='.protobuf_unittest.NestedMessage',
            output_type='.protobuf_unittest.ResponseMessage')

        # Note: Calling DescriptorPool.Add() multiple times with the same file only
        # works if the input is canonical; in particular, all type names must be
        # fully qualified.
        self.pool = self.get_descriptor_pool()
        self.pool.Add(file_proto)
        self.my_file = self.pool.FindFileByName(file_proto.name)
        self.my_message = self.my_file.message_types_by_name[message_proto.name]
        self.my_enum = self.my_message.enum_types_by_name[enum_proto.name]
        self.my_service = self.my_file.services_by_name[service_proto.name]
        self.my_method = self.my_service.methods_by_name[method_proto.name]

    def get_descriptor_pool(self):
        return symbol_database.Default().pool

    def test_missing_package(self):
        file_proto = descriptor_pb2.FileDescriptorProto(
            name='some/filename/some.proto')
        serialized = file_proto.SerializeToString()
        pool = descriptor_pool.DescriptorPool()
        file_descriptor = pool.AddSerializedFile(serialized)
        assert '' == file_descriptor.package

    def test_empty_package(self):
        file_proto = descriptor_pb2.FileDescriptorProto(
            name='some/filename/some.proto', package='')
        serialized = file_proto.SerializeToString()
        pool = descriptor_pool.DescriptorPool()
        file_descriptor = pool.AddSerializedFile(serialized)
        assert '' == file_descriptor.package

    def test_reserved_name(self):
        text = """
        name: "foo.proto"
        message_type {
          name: "BrokenMessageFoo"
          reserved_name: "is_deprecated"
        }
        """

        fdp = text_format.Parse(text, descriptor_pb2.FileDescriptorProto())
        serialized = fdp.SerializeToString()
        # AddSerializedFile() will allow duplicate adds but only if the descriptors
        # are identical and can round-trip through a FileDescriptor losslessly.
        desc1 = descriptor_pool.Default().AddSerializedFile(serialized)
        desc2 = descriptor_pool.Default().AddSerializedFile(serialized)
        assert desc1 == desc2

    def test_reserved_range(self):
        text = """
        name: "bar.proto"
        message_type {
          name: "BrokenMessageBar"
          reserved_range {
            start: 101
            end: 102
          }
        }
        """

        fdp = text_format.Parse(text, descriptor_pb2.FileDescriptorProto())
        serialized = fdp.SerializeToString()
        # AddSerializedFile() will allow duplicate adds but only if the descriptors
        # are identical and can round-trip through a FileDescriptor losslessly.
        desc1 = descriptor_pool.Default().AddSerializedFile(serialized)
        desc2 = descriptor_pool.Default().AddSerializedFile(serialized)
        assert desc1 == desc2

    def test_reserved_name_enum(self):
        text = """
        name: "baz.proto"
        enum_type {
          name: "BrokenMessageBaz"
          value: <
            name: 'ENUM_BAZ'
            number: 114
          >
          reserved_name: "is_deprecated"
        }
        """

        fdp = text_format.Parse(text, descriptor_pb2.FileDescriptorProto())
        serialized = fdp.SerializeToString()
        # AddSerializedFile() will allow duplicate adds but only if the descriptors
        # are identical and can round-trip through a FileDescriptor losslessly.
        desc1 = descriptor_pool.Default().AddSerializedFile(serialized)
        desc2 = descriptor_pool.Default().AddSerializedFile(serialized)
        assert desc1 == desc2

    def test_reserved_range_enum(self):
        text = """
        name: "bat.proto"
        enum_type {
          name: "BrokenMessageBat"
          value: <
            name: 'ENUM_BAT'
            number: 115
          >
          reserved_range {
            start: 1001
            end: 1002
          }
        }
        """

        fdp = text_format.Parse(text, descriptor_pb2.FileDescriptorProto())
        serialized = fdp.SerializeToString()
        # AddSerializedFile() will allow duplicate adds but only if the descriptors
        # are identical and can round-trip through a FileDescriptor losslessly.
        desc1 = descriptor_pool.Default().AddSerializedFile(serialized)
        desc2 = descriptor_pool.Default().AddSerializedFile(serialized)
        assert desc1 == desc2

    def test_find_method_by_name(self):
        service_descriptor = (unittest_custom_options_pb2.
                              TestServiceWithCustomOptions.DESCRIPTOR)
        method_descriptor = service_descriptor.FindMethodByName('Foo')
        assert method_descriptor.name == 'Foo'
        with pytest.raises(KeyError):
            service_descriptor.FindMethodByName('MethodDoesNotExist')

    def test_enum_value_name(self):
        assert self.my_message.EnumValueName('ForeignEnum', 4) == 'FOREIGN_FOO'

        assert (
            self.my_message.enum_types_by_name['ForeignEnum'].values_by_number[4].name
            == self.my_message.EnumValueName('ForeignEnum', 4))
        with pytest.raises(KeyError):
            self.my_message.EnumValueName('ForeignEnum', 999)
        with pytest.raises(KeyError):
            self.my_message.EnumValueName('NoneEnum', 999)
        with pytest.raises(TypeError):
            self.my_message.EnumValueName()

    def test_enum_fixups(self):
        assert self.my_enum == self.my_enum.values[0].type

    def test_containing_type_fixups(self):
        assert self.my_message == self.my_message.fields[0].containing_type
        assert self.my_message == self.my_enum.containing_type

    def test_containing_service_fixups(self):
        assert self.my_service == self.my_method.containing_service

    @pytest.mark.skipif(
        api_implementation.Type() == 'python',
        reason='GetDebugString is only available with the cpp implementation',
    )
    def testGetDebugString(self):
        assert self.my_file.GetDebugString() == TEST_FILE_DESCRIPTOR_DEBUG

    def test_get_options(self):
        assert self.my_enum.GetOptions()  == descriptor_pb2.EnumOptions()
        assert self.my_enum.values[0].GetOptions()  == descriptor_pb2.EnumValueOptions()
        assert self.my_message.GetOptions()  == descriptor_pb2.MessageOptions()
        assert self.my_message.fields[0].GetOptions()  == descriptor_pb2.FieldOptions()
        assert self.my_method.GetOptions()  == descriptor_pb2.MethodOptions()
        assert self.my_service.GetOptions()  == descriptor_pb2.ServiceOptions()

    def test_simple_custom_options(self):
        file_descriptor = unittest_custom_options_pb2.DESCRIPTOR
        message_descriptor = (unittest_custom_options_pb2.
                              TestMessageWithCustomOptions.DESCRIPTOR)
        field_descriptor = message_descriptor.fields_by_name['field1']
        oneof_descriptor = message_descriptor.oneofs_by_name['AnOneof']
        enum_descriptor = message_descriptor.enum_types_by_name['AnEnum']
        enum_value_descriptor = (message_descriptor.
                                enum_values_by_name['ANENUM_VAL2'])
        other_enum_value_descriptor = (message_descriptor.
                                      enum_values_by_name['ANENUM_VAL1'])
        service_descriptor = (unittest_custom_options_pb2.
                              TestServiceWithCustomOptions.DESCRIPTOR)
        method_descriptor = service_descriptor.FindMethodByName('Foo')

        file_options = file_descriptor.GetOptions()
        file_opt1 = unittest_custom_options_pb2.file_opt1
        assert 9876543210 == file_options.Extensions[file_opt1]
        message_options = message_descriptor.GetOptions()
        message_opt1 = unittest_custom_options_pb2.message_opt1
        assert -56 == message_options.Extensions[message_opt1]
        field_options = field_descriptor.GetOptions()
        field_opt1 = unittest_custom_options_pb2.field_opt1
        assert 8765432109 == field_options.Extensions[field_opt1]
        field_opt2 = unittest_custom_options_pb2.field_opt2
        assert 42 == field_options.Extensions[field_opt2]
        oneof_options = oneof_descriptor.GetOptions()
        oneof_opt1 = unittest_custom_options_pb2.oneof_opt1
        assert -99 == oneof_options.Extensions[oneof_opt1]
        enum_options = enum_descriptor.GetOptions()
        enum_opt1 = unittest_custom_options_pb2.enum_opt1
        assert -789 == enum_options.Extensions[enum_opt1]
        enum_value_options = enum_value_descriptor.GetOptions()
        enum_value_opt1 = unittest_custom_options_pb2.enum_value_opt1
        assert 123 == enum_value_options.Extensions[enum_value_opt1]

        service_options = service_descriptor.GetOptions()
        service_opt1 = unittest_custom_options_pb2.service_opt1
        assert -9876543210 == service_options.Extensions[service_opt1]
        method_options = method_descriptor.GetOptions()
        method_opt1 = unittest_custom_options_pb2.method_opt1
        assert unittest_custom_options_pb2.METHODOPT1_VAL2  == method_options.Extensions[method_opt1]

        message_descriptor = (
            unittest_custom_options_pb2.DummyMessageContainingEnum.DESCRIPTOR)
        assert file_descriptor.has_options
        assert not message_descriptor.has_options
        assert field_descriptor.has_options
        assert oneof_descriptor.has_options
        assert enum_descriptor.has_options
        assert enum_value_descriptor.has_options
        assert not other_enum_value_descriptor.has_options

    def test_custom_options_copy_to(self):
        message_descriptor = (unittest_custom_options_pb2.
                              TestMessageWithCustomOptions.DESCRIPTOR)
        message_proto = descriptor_pb2.DescriptorProto()
        message_descriptor.CopyToProto(message_proto)
        assert len(message_proto.options.ListFields())  == 2

    def test_different_custom_option_types(self):
        kint32min = -2**31
        kint64min = -2**63
        kint32max = 2**31 - 1
        kint64max = 2**63 - 1
        kuint32max = 2**32 - 1
        kuint64max = 2**64 - 1

        message_descriptor = \
            unittest_custom_options_pb2.CustomOptionMinIntegerValues.DESCRIPTOR
        message_options = message_descriptor.GetOptions()
        assert message_options.Extensions[unittest_custom_options_pb2.bool_opt] is False
        assert kint32min == message_options.Extensions[unittest_custom_options_pb2.int32_opt]
        assert kint64min == message_options.Extensions[unittest_custom_options_pb2.int64_opt]
        assert 0 == message_options.Extensions[unittest_custom_options_pb2.uint32_opt]
        assert 0 == message_options.Extensions[unittest_custom_options_pb2.uint64_opt]
        assert kint32min  == message_options.Extensions[unittest_custom_options_pb2.sint32_opt]
        assert kint64min  == message_options.Extensions[unittest_custom_options_pb2.sint64_opt]
        assert 0 == message_options.Extensions[unittest_custom_options_pb2.fixed32_opt]
        assert 0 == message_options.Extensions[unittest_custom_options_pb2.fixed64_opt]
        assert kint32min == message_options.Extensions[unittest_custom_options_pb2.sfixed32_opt]
        assert kint64min == message_options.Extensions[unittest_custom_options_pb2.sfixed64_opt]

        message_descriptor = \
            unittest_custom_options_pb2.CustomOptionMaxIntegerValues.DESCRIPTOR
        message_options = message_descriptor.GetOptions()
        assert message_options.Extensions[unittest_custom_options_pb2.bool_opt] is True
        assert kint32max  == message_options.Extensions[unittest_custom_options_pb2.int32_opt]
        assert kint64max  == message_options.Extensions[unittest_custom_options_pb2.int64_opt]
        assert kuint32max  == message_options.Extensions[unittest_custom_options_pb2.uint32_opt]
        assert kuint64max  == message_options.Extensions[unittest_custom_options_pb2.uint64_opt]
        assert kint32max  == message_options.Extensions[unittest_custom_options_pb2.sint32_opt]
        assert kint64max  == message_options.Extensions[unittest_custom_options_pb2.sint64_opt]
        assert kuint32max  == message_options.Extensions[unittest_custom_options_pb2.fixed32_opt]
        assert kuint64max  == message_options.Extensions[unittest_custom_options_pb2.fixed64_opt]
        assert kint32max  == message_options.Extensions[unittest_custom_options_pb2.sfixed32_opt]
        assert kint64max  == message_options.Extensions[unittest_custom_options_pb2.sfixed64_opt]

        message_descriptor = \
            unittest_custom_options_pb2.CustomOptionOtherValues.DESCRIPTOR
        message_options = message_descriptor.GetOptions()
        assert -100  == message_options.Extensions[unittest_custom_options_pb2.int32_opt]
        assert 12.3456789 == pytest.approx(message_options.Extensions[unittest_custom_options_pb2.float_opt], rel=1e-6)
        assert 1.234567890123456789 == pytest.approx(message_options.Extensions[unittest_custom_options_pb2.double_opt])
        assert "Hello, \"World\""  == message_options.Extensions[unittest_custom_options_pb2.string_opt]
        assert b"Hello\0World"  == message_options.Extensions[unittest_custom_options_pb2.bytes_opt]
        dummy_enum = unittest_custom_options_pb2.DummyMessageContainingEnum
        assert dummy_enum.TEST_OPTION_ENUM_TYPE2  == message_options.Extensions[unittest_custom_options_pb2.enum_opt]

        message_descriptor = \
            unittest_custom_options_pb2.SettingRealsFromPositiveInts.DESCRIPTOR
        message_options = message_descriptor.GetOptions()
        assert 12 == pytest.approx(message_options.Extensions[unittest_custom_options_pb2.float_opt], rel=1e-6)
        assert 154 == pytest.approx(message_options.Extensions[unittest_custom_options_pb2.double_opt])

        message_descriptor =\
            unittest_custom_options_pb2.SettingRealsFromNegativeInts.DESCRIPTOR
        message_options = message_descriptor.GetOptions()
        assert -12 == pytest.approx(message_options.Extensions[unittest_custom_options_pb2.float_opt], rel=1e-6)
        assert -154 == pytest.approx(message_options.Extensions[unittest_custom_options_pb2.double_opt])

    def test_complex_extension_options(self):
        descriptor =\
            unittest_custom_options_pb2.VariousComplexOptions.DESCRIPTOR
        options = descriptor.GetOptions()
        assert 42  == options.Extensions[unittest_custom_options_pb2.complex_opt1].foo
        assert 324  == options.Extensions[unittest_custom_options_pb2.complex_opt1].Extensions[unittest_custom_options_pb2.mooo]
        assert 876  == options.Extensions[unittest_custom_options_pb2.complex_opt1].Extensions[unittest_custom_options_pb2.corge].moo
        assert 987  == options.Extensions[unittest_custom_options_pb2.complex_opt2].baz
        assert 654  == options.Extensions[unittest_custom_options_pb2.complex_opt2].Extensions[unittest_custom_options_pb2.grault]
        assert 743  == options.Extensions[unittest_custom_options_pb2.complex_opt2].bar.foo
        assert 1999  == options.Extensions[unittest_custom_options_pb2.complex_opt2].bar.Extensions[unittest_custom_options_pb2.mooo]
        assert 2008  == options.Extensions[unittest_custom_options_pb2.complex_opt2].bar.Extensions[unittest_custom_options_pb2.corge].moo
        assert 741  == options.Extensions[unittest_custom_options_pb2.complex_opt2].Extensions[unittest_custom_options_pb2.garply].foo
        assert 1998  == options.Extensions[unittest_custom_options_pb2.complex_opt2].Extensions[unittest_custom_options_pb2.garply].Extensions[unittest_custom_options_pb2.mooo]
        assert 2121  == options.Extensions[unittest_custom_options_pb2.complex_opt2].Extensions[unittest_custom_options_pb2.garply].Extensions[unittest_custom_options_pb2.corge].moo
        assert 1971  == options.Extensions[unittest_custom_options_pb2.ComplexOptionType2.ComplexOptionType4.complex_opt4].waldo
        assert 321  == options.Extensions[unittest_custom_options_pb2.complex_opt2].fred.waldo
        assert 9  == options.Extensions[unittest_custom_options_pb2.complex_opt3].moo
        assert 22  == options.Extensions[unittest_custom_options_pb2.complex_opt3].complexoptiontype5.plugh
        assert 24  == options.Extensions[unittest_custom_options_pb2.complexopt6].xyzzy

    # Check that aggregate options were parsed and saved correctly in
    # the appropriate descriptors.
    def test_aggregate_options(self):
        file_descriptor = unittest_custom_options_pb2.DESCRIPTOR
        message_descriptor = \
            unittest_custom_options_pb2.AggregateMessage.DESCRIPTOR
        field_descriptor = message_descriptor.fields_by_name["fieldname"]
        enum_descriptor = unittest_custom_options_pb2.AggregateEnum.DESCRIPTOR
        enum_value_descriptor = enum_descriptor.values_by_name["VALUE"]
        service_descriptor = \
            unittest_custom_options_pb2.AggregateService.DESCRIPTOR
        method_descriptor = service_descriptor.FindMethodByName("Method")

        # Tests for the different types of data embedded in fileopt
        file_options = file_descriptor.GetOptions().Extensions[
            unittest_custom_options_pb2.fileopt]
        assert 100 == file_options.i
        assert "FileAnnotation" == file_options.s
        assert "NestedFileAnnotation" == file_options.sub.s
        assert ("FileExtensionAnnotation" == file_options.file.Extensions[
            unittest_custom_options_pb2.fileopt].s)
        assert ("EmbeddedMessageSetElement" == file_options.mset.Extensions[
            unittest_custom_options_pb2.AggregateMessageSetElement
            .message_set_extension].s)

        # Simple tests for all the other types of annotations
        assert (
            "MessageAnnotation" ==
            message_descriptor.GetOptions().Extensions[
                unittest_custom_options_pb2.msgopt].s)
        assert (
            "FieldAnnotation" ==
            field_descriptor.GetOptions().Extensions[
                unittest_custom_options_pb2.fieldopt].s)
        assert (
            "EnumAnnotation" ==
            enum_descriptor.GetOptions().Extensions[
                unittest_custom_options_pb2.enumopt].s)
        assert (
            "EnumValueAnnotation" ==
            enum_value_descriptor.GetOptions().Extensions[
                unittest_custom_options_pb2.enumvalopt].s)
        assert (
            "ServiceAnnotation" ==
            service_descriptor.GetOptions().Extensions[
                unittest_custom_options_pb2.serviceopt].s)
        assert (
            "MethodAnnotation" ==
            method_descriptor.GetOptions().Extensions[
                unittest_custom_options_pb2.methodopt].s)

    def test_nested_options(self):
        nested_message = \
            unittest_custom_options_pb2.NestedOptionType.NestedMessage.DESCRIPTOR
        assert 1001 == nested_message.GetOptions().Extensions[
            unittest_custom_options_pb2.message_opt1]
        nested_field = nested_message.fields_by_name["nested_field"]
        assert 1002 == nested_field.GetOptions().Extensions[
            unittest_custom_options_pb2.field_opt1]
        outer_message = \
            unittest_custom_options_pb2.NestedOptionType.DESCRIPTOR
        nested_enum = outer_message.enum_types_by_name["NestedEnum"]
        assert 1003 == nested_enum.GetOptions().Extensions[
            unittest_custom_options_pb2.enum_opt1]
        nested_enum_value = outer_message.enum_values_by_name["NESTED_ENUM_VALUE"]
        assert 1004 == nested_enum_value.GetOptions().Extensions[
            unittest_custom_options_pb2.enum_value_opt1]
        nested_extension = outer_message.extensions_by_name["nested_extension"]
        assert 1005 == nested_extension.GetOptions().Extensions[
            unittest_custom_options_pb2.field_opt2]

    def test_file_descriptor_references(self):
        assert self.my_enum.file == self.my_file
        assert self.my_message.file == self.my_file

    def test_file_descriptor(self):
        assert self.my_file.name == 'some/filename/some.proto'
        assert self.my_file.package == 'protobuf_unittest'
        assert self.my_file.pool == self.pool
        assert not self.my_file.has_options
        file_proto = descriptor_pb2.FileDescriptorProto()
        self.my_file.CopyToProto(file_proto)
        assert self.my_file.serialized_pb  == file_proto.SerializeToString()
        # Generated modules also belong to the default pool.
        assert unittest_pb2.DESCRIPTOR.pool == descriptor_pool.Default()

    @pytest.mark.skipif(
        api_implementation.Type() == 'python',
        reason='Immutability of descriptors is only enforced in v2 implementation')
    def test_immutable_cpp_descriptor(self):
        file_descriptor = unittest_pb2.DESCRIPTOR
        message_descriptor = unittest_pb2.TestAllTypes.DESCRIPTOR
        field_descriptor = message_descriptor.fields_by_name['optional_int32']
        enum_descriptor = message_descriptor.enum_types_by_name['NestedEnum']
        oneof_descriptor = message_descriptor.oneofs_by_name['oneof_field']
        with pytest.raises(AttributeError):
            message_descriptor.fields_by_name = None
        with pytest.raises(TypeError):
            message_descriptor.fields_by_name['Another'] = None
        with pytest.raises(TypeError):
            message_descriptor.fields.append(None)
        with pytest.raises(AttributeError):
            field_descriptor.containing_type = message_descriptor
        with pytest.raises(AttributeError):
            file_descriptor.has_options = False
        with pytest.raises(AttributeError):
            field_descriptor.has_options = False
        with pytest.raises(AttributeError):
            oneof_descriptor.has_options = False
        with pytest.raises(AttributeError):
            enum_descriptor.has_options = False
        with pytest.raises(AttributeError, match='attribute is not writable: has_options'):
            message_descriptor.has_options = True

    def test_default(self):
        message_descriptor = unittest_pb2.TestAllTypes.DESCRIPTOR
        field = message_descriptor.fields_by_name['repeated_int32']
        assert field.default_value == []
        field = message_descriptor.fields_by_name['repeated_nested_message']
        assert field.default_value == []
        field = message_descriptor.fields_by_name['optionalgroup']
        assert field.default_value is None
        field = message_descriptor.fields_by_name['optional_nested_message']
        assert field.default_value is None


class TestNewDescriptor(TestDescriptor):
    """Redo the same tests as above, but with a separate DescriptorPool."""

    def get_descriptor_pool(self):
        return descriptor_pool.DescriptorPool()


class TestGeneratedDescriptor:
    """Tests for the properties of descriptors in generated code."""

    def check_message_descriptor(self, message_descriptor):
        # Basic properties
        assert message_descriptor.name == 'TestAllTypes'
        assert message_descriptor.full_name  == 'protobuf_unittest.TestAllTypes'
        # Test equality and hashability
        assert message_descriptor == message_descriptor
        assert message_descriptor.fields[0].containing_type  == message_descriptor
        assert message_descriptor in [message_descriptor]
        assert message_descriptor in {message_descriptor: None}
        # Test field containers
        self.check_descriptor_sequence(message_descriptor.fields)
        self.check_descriptor_mapping(message_descriptor.fields_by_name)
        self.check_descriptor_mapping(message_descriptor.fields_by_number)
        self.check_descriptor_mapping(message_descriptor.fields_by_camelcase_name)
        self.check_descriptor_mapping(message_descriptor.enum_types_by_name)
        self.check_descriptor_mapping(message_descriptor.enum_values_by_name)
        self.check_descriptor_mapping(message_descriptor.oneofs_by_name)
        self.check_descriptor_mapping(message_descriptor.enum_types[0].values_by_name)
        # Test extension range
        assert message_descriptor.extension_ranges == []

    def check_field_descriptor(self, field_descriptor):
        # Basic properties
        assert field_descriptor.name == 'optional_int32'
        assert field_descriptor.camelcase_name == 'optionalInt32'
        assert field_descriptor.full_name  == 'protobuf_unittest.TestAllTypes.optional_int32'
        assert field_descriptor.containing_type.name == 'TestAllTypes'
        assert field_descriptor.file == unittest_pb2.DESCRIPTOR
        # Test equality and hashability
        assert field_descriptor == field_descriptor
        assert field_descriptor.containing_type.fields_by_name['optional_int32']  == field_descriptor
        assert field_descriptor.containing_type.fields_by_camelcase_name['optionalInt32']  == field_descriptor
        assert field_descriptor in [field_descriptor]
        assert field_descriptor in {field_descriptor: None}
        assert field_descriptor.extension_scope is None
        assert field_descriptor.enum_type is None
        assert field_descriptor.has_presence
        if api_implementation.Type() == 'cpp':
            # For test coverage only
            assert field_descriptor.id == field_descriptor.id

    def check_descriptor_sequence(self, sequence):
        # Verifies that a property like 'messageDescriptor.fields' has all the
        # properties of an immutable abc.Sequence.
        assert sequence != unittest_pb2.TestAllExtensions.DESCRIPTOR.fields
        assert sequence != []
        assert sequence != 1
        assert not sequence == 1  # Only for cpp test coverage
        assert sequence == sequence
        expected_list = list(sequence)
        assert expected_list == sequence
        assert len(sequence) > 0  # Sized
        assert len(sequence) == len(expected_list)  # Iterable
        assert sequence[len(sequence) -1] == sequence[-1]
        item = sequence[0]
        assert item == sequence[0]
        assert item in sequence  # Container
        assert sequence.index(item) == 0
        assert sequence.count(item) == 1
        other_item = unittest_pb2.NestedTestAllTypes.DESCRIPTOR.fields[0]
        assert other_item not in sequence
        assert sequence.count(other_item) == 0
        pytest.raises(ValueError, sequence.index, other_item)
        pytest.raises(ValueError, sequence.index, [])
        reversed_iterator = reversed(sequence)
        assert list(reversed_iterator) == list(sequence)[::-1]
        pytest.raises(StopIteration, next, reversed_iterator)
        expected_list[0] = 'change value'
        assert expected_list != sequence
        # TODO(jieluo): Change __repr__ support for DescriptorSequence.
        if api_implementation.Type() == 'python':
            assert str(list(sequence)) == str(sequence)
        else:
            assert str(sequence)[0] == '<'

    def check_descriptor_mapping(self, mapping):
        # Verifies that a property like 'messageDescriptor.fields' has all the
        # properties of an immutable abc.Mapping.
        iterated_keys = []
        for key in mapping:
            iterated_keys.append(key)
        assert len(iterated_keys) == len(mapping)
        assert set(iterated_keys) == set(mapping.keys())

        assert mapping != unittest_pb2.TestAllExtensions.DESCRIPTOR.fields_by_name
        assert mapping != {}
        assert mapping != 1
        assert not mapping == 1  # Only for cpp test coverage
        excepted_dict = dict(mapping.items())
        assert mapping == excepted_dict
        assert mapping == mapping
        assert len(mapping) > 0  # Sized
        assert len(mapping) == len(excepted_dict)  # Iterable
        key, item = next(iter(mapping.items()))
        assert key in mapping  # Container
        assert mapping.get(key) == item
        with pytest.raises(TypeError):
            mapping.get()
        # TODO(jieluo): Fix python and cpp extension diff.
        if api_implementation.Type() == 'cpp':
            assert mapping.get([]) is None
        else:
            pytest.raises(TypeError, mapping.get, [])
            with pytest.raises(TypeError):
                if [] in mapping:
                    pass
            with pytest.raises(TypeError):
                _ = mapping[[]]
        # keys(), iterkeys() &co
        item = (next(iter(mapping.keys())), next(iter(mapping.values())))
        assert item == next(iter(mapping.items()))
        excepted_dict[key] = 'change value'
        assert mapping != excepted_dict
        del excepted_dict[key]
        excepted_dict['new_key'] = 'new'
        assert mapping != excepted_dict
        pytest.raises(KeyError, mapping.__getitem__, 'key_error')
        pytest.raises(KeyError, mapping.__getitem__, len(mapping) + 1)
        # TODO(jieluo): Add __repr__ support for DescriptorMapping.
        if api_implementation.Type() == 'cpp':
            assert str(mapping)[0] == '<'
        else:
            print(str(dict(mapping.items()))[:100])
            print(str(mapping)[:100])
            assert len(str(dict(mapping.items()))) == len(str(mapping))

    def test_descriptor(self):
        message_descriptor = unittest_pb2.TestAllTypes.DESCRIPTOR
        self.check_message_descriptor(message_descriptor)
        field_descriptor = message_descriptor.fields_by_name['optional_int32']
        self.check_field_descriptor(field_descriptor)
        field_descriptor = message_descriptor.fields_by_camelcase_name[
            'optionalInt32']
        self.check_field_descriptor(field_descriptor)
        enum_descriptor = unittest_pb2.DESCRIPTOR.enum_types_by_name[
            'ForeignEnum']
        assert enum_descriptor.containing_type is None
        # Test extension range
        assert unittest_pb2.TestAllExtensions.DESCRIPTOR.extension_ranges, [(1  == 536870912)]
        assert unittest_pb2.TestMultipleExtensionRanges.DESCRIPTOR.extension_ranges, [(42, 43), (4143, 4244), (65536  == 536870912)]

    def test_cpp_descriptor_container(self):
        containing_file = unittest_pb2.DESCRIPTOR
        self.check_descriptor_sequence(containing_file.dependencies)
        self.check_descriptor_mapping(containing_file.message_types_by_name)
        self.check_descriptor_mapping(containing_file.enum_types_by_name)
        self.check_descriptor_mapping(containing_file.services_by_name)
        self.check_descriptor_mapping(containing_file.extensions_by_name)
        self.check_descriptor_mapping(
            unittest_pb2.TestNestedExtension.DESCRIPTOR.extensions_by_name)

    def test_cpp_descriptor_container_iterator(self):
        # Same test with the iterator
        enum = unittest_pb2.TestAllTypes.DESCRIPTOR.enum_types_by_name['NestedEnum']
        values_iter = iter(enum.values)
        del enum
        assert 'FOO' == next(values_iter).name

    def test_descriptor_nested_types_container(self):
        message_descriptor = unittest_pb2.TestAllTypes.DESCRIPTOR
        nested_message_descriptor = unittest_pb2.TestAllTypes.NestedMessage.DESCRIPTOR
        assert len(message_descriptor.nested_types) == 3
        assert not None in message_descriptor.nested_types
        assert nested_message_descriptor in message_descriptor.nested_types

    def test_service_descriptor(self):
        service_descriptor = unittest_pb2.DESCRIPTOR.services_by_name['TestService']
        assert service_descriptor.name == 'TestService'
        assert service_descriptor.methods[0].name == 'Foo'
        assert service_descriptor.file is unittest_pb2.DESCRIPTOR
        assert service_descriptor.index == 0
        self.check_descriptor_mapping(service_descriptor.methods_by_name)

    def test_oneof_descriptor(self):
          message_descriptor = unittest_pb2.TestAllTypes.DESCRIPTOR
          oneof_descriptor = message_descriptor.oneofs_by_name['oneof_field']
          assert not oneof_descriptor.has_options
          assert message_descriptor == oneof_descriptor.containing_type
          assert 'oneof_field' == oneof_descriptor.name
          assert 'protobuf_unittest.TestAllTypes.oneof_field' == oneof_descriptor.full_name
          assert 0 == oneof_descriptor.index


class TestDescriptorCopyToProto:
    """Tests for CopyTo functions of Descriptor."""

    def _assert_proto_equal(self, actual_proto, expected_class, expected_ascii):
        expected_proto = expected_class()
        text_format.Merge(expected_ascii, expected_proto)

        assert actual_proto == expected_proto, (
            'Not equal,\nActual:\n%s\nExpected:\n%s\n'
            % (str(actual_proto), str(expected_proto)))

    def _internal_test_copy_to_proto(self, desc, expected_proto_class,
                                     expected_proto_ascii):
        actual = expected_proto_class()
        desc.CopyToProto(actual)
        self._assert_proto_equal(
            actual, expected_proto_class, expected_proto_ascii)

    def test_copy_to_proto_empty_message(self):
        self._internal_test_copy_to_proto(
            unittest_pb2.TestEmptyMessage.DESCRIPTOR,
            descriptor_pb2.DescriptorProto,
            TEST_EMPTY_MESSAGE_DESCRIPTOR_ASCII)

    def test_copy_to_proto_nested_message(self):
        TEST_NESTED_MESSAGE_ASCII = """
        name: 'NestedMessage'
        field: <
          name: 'bb'
          number: 1
          label: 1  # Optional
          type: 5  # TYPE_INT32
        >
        """

        self._internal_test_copy_to_proto(
            unittest_pb2.TestAllTypes.NestedMessage.DESCRIPTOR,
            descriptor_pb2.DescriptorProto,
            TEST_NESTED_MESSAGE_ASCII)

    def test_copy_to_proto_foreign_nested_message(self):
        TEST_FOREIGN_NESTED_ASCII = """
        name: 'TestForeignNested'
        field: <
          name: 'foreign_nested'
          number: 1
          label: 1  # Optional
          type: 11  # TYPE_MESSAGE
          type_name: '.protobuf_unittest.TestAllTypes.NestedMessage'
        >
        """

        self._internal_test_copy_to_proto(
            unittest_pb2.TestForeignNested.DESCRIPTOR,
            descriptor_pb2.DescriptorProto,
            TEST_FOREIGN_NESTED_ASCII)

    def test_copy_to_proto_foreign_enum(self):
        TEST_FOREIGN_ENUM_ASCII = """
        name: 'ForeignEnum'
        value: <
          name: 'FOREIGN_FOO'
          number: 4
        >
        value: <
          name: 'FOREIGN_BAR'
          number: 5
        >
        value: <
          name: 'FOREIGN_BAZ'
          number: 6
        >
        """

        self._internal_test_copy_to_proto(
            unittest_pb2.ForeignEnum.DESCRIPTOR,
            descriptor_pb2.EnumDescriptorProto,
            TEST_FOREIGN_ENUM_ASCII)

    def test_copy_to_proto_options(self):
        TEST_DEPRECATED_FIELDS_ASCII = """
        name: 'TestDeprecatedFields'
        field: <
          name: 'deprecated_int32'
          number: 1
          label: 1  # Optional
          type: 5  # TYPE_INT32
          options: <
            deprecated: true
          >
        >
        field: {
          name: 'deprecated_repeated_string'
          number: 4
          label: LABEL_REPEATED
          type: TYPE_STRING
          options: {
            deprecated: true
          }
        }
        field {
          name: "deprecated_message"
          number: 3
          label: LABEL_OPTIONAL
          type: TYPE_MESSAGE
          type_name: ".protobuf_unittest.TestAllTypes.NestedMessage"
          options {
            deprecated: true
          }
        }
        field {
          name: "deprecated_int32_in_oneof"
          number: 2
          label: LABEL_OPTIONAL
          type: TYPE_INT32
          options {
            deprecated: true
          }
          oneof_index: 0
        }
        field {
          name: "nested"
          number: 5
          label: LABEL_OPTIONAL
          type: TYPE_MESSAGE
          type_name: ".protobuf_unittest.TestDeprecatedFields"
        }
        oneof_decl {
          name: "oneof_fields"
        }
        """

        self._internal_test_copy_to_proto(
            unittest_pb2.TestDeprecatedFields.DESCRIPTOR,
            descriptor_pb2.DescriptorProto,
            TEST_DEPRECATED_FIELDS_ASCII)

    def test_copy_to_proto_all_extensions(self):
        TEST_EMPTY_MESSAGE_WITH_EXTENSIONS_ASCII = """
        name: 'TestEmptyMessageWithExtensions'
        extension_range: <
          start: 1
          end: 536870912
        >
        """

        self._internal_test_copy_to_proto(
            unittest_pb2.TestEmptyMessageWithExtensions.DESCRIPTOR,
            descriptor_pb2.DescriptorProto,
            TEST_EMPTY_MESSAGE_WITH_EXTENSIONS_ASCII)

    def test_copy_to_proto_several_extensions(self):
        TEST_MESSAGE_WITH_SEVERAL_EXTENSIONS_ASCII = """
        name: 'TestMultipleExtensionRanges'
        extension_range: <
          start: 42
          end: 43
        >
        extension_range: <
          start: 4143
          end: 4244
        >
        extension_range: <
          start: 65536
          end: 536870912
        >
        """

        self._internal_test_copy_to_proto(
            unittest_pb2.TestMultipleExtensionRanges.DESCRIPTOR,
            descriptor_pb2.DescriptorProto,
            TEST_MESSAGE_WITH_SEVERAL_EXTENSIONS_ASCII)

    def test_copy_to_proto_file_descriptor(self):
        UNITTEST_IMPORT_FILE_DESCRIPTOR_ASCII = ("""
        name: 'google/protobuf/unittest_import.proto'
        package: 'protobuf_unittest_import'
        dependency: 'google/protobuf/unittest_import_public.proto'
        message_type: <
          name: 'ImportMessage'
          field: <
            name: 'd'
            number: 1
            label: 1  # Optional
            type: 5  # TYPE_INT32
          >
        >
        """ +
        """enum_type: <
          name: 'ImportEnum'
          value: <
            name: 'IMPORT_FOO'
            number: 7
          >
          value: <
            name: 'IMPORT_BAR'
            number: 8
          >
          value: <
            name: 'IMPORT_BAZ'
            number: 9
          >
        >
        enum_type: <
          name: 'ImportEnumForMap'
          value: <
            name: 'UNKNOWN'
            number: 0
          >
          value: <
            name: 'FOO'
            number: 1
          >
          value: <
            name: 'BAR'
            number: 2
          >
        >
        options: <
          java_package: 'com.google.protobuf.test'
          optimize_for: 1  # SPEED
        """ +
        """
          cc_enable_arenas: true
          >
          public_dependency: 0
        """)
        self._internal_test_copy_to_proto(
            unittest_import_pb2.DESCRIPTOR,
            descriptor_pb2.FileDescriptorProto,
            UNITTEST_IMPORT_FILE_DESCRIPTOR_ASCII)

    def test_copy_to_proto_service_descriptor(self):
        TEST_SERVICE_ASCII = """
        name: 'TestService'
        method: <
          name: 'Foo'
          input_type: '.protobuf_unittest.FooRequest'
          output_type: '.protobuf_unittest.FooResponse'
        >
        method: <
          name: 'Bar'
          input_type: '.protobuf_unittest.BarRequest'
          output_type: '.protobuf_unittest.BarResponse'
        >
        """
        self._internal_test_copy_to_proto(
            unittest_pb2.TestService.DESCRIPTOR,
            descriptor_pb2.ServiceDescriptorProto,
            TEST_SERVICE_ASCII)

    def test_copy_to_proto_method_descriptor(self):
        expected_ascii = """
        name: 'Foo'
        input_type: '.protobuf_unittest.FooRequest'
        output_type: '.protobuf_unittest.FooResponse'
        """
        method_descriptor = unittest_pb2.TestService.DESCRIPTOR.FindMethodByName('Foo')
        self._internal_test_copy_to_proto(
            method_descriptor,
            descriptor_pb2.MethodDescriptorProto,
            expected_ascii)

    @pytest.mark.skipif(
        api_implementation.Type() == 'python',
        reason='Pure python does not raise error.')
    # TODO(jieluo): Fix pure python to check with the proto type.
    def test_copy_to_proto_type_error(self):
        file_proto = descriptor_pb2.FileDescriptorProto()
        pytest.raises(TypeError,
                      unittest_pb2.TestEmptyMessage.DESCRIPTOR.CopyToProto,
                      file_proto)
        pytest.raises(TypeError,
                      unittest_pb2.ForeignEnum.DESCRIPTOR.CopyToProto,
                      file_proto)
        pytest.raises(TypeError,
                      unittest_pb2.TestService.DESCRIPTOR.CopyToProto,
                      file_proto)
        proto = descriptor_pb2.DescriptorProto()
        pytest.raises(TypeError,
                      unittest_import_pb2.DESCRIPTOR.CopyToProto,
                      proto)


class TestMakeDescriptor:
    def test_make_descriptor_with_nested_fields(self):
        file_descriptor_proto = descriptor_pb2.FileDescriptorProto()
        file_descriptor_proto.name = 'Foo2'
        message_type = file_descriptor_proto.message_type.add()
        message_type.name = file_descriptor_proto.name
        nested_type = message_type.nested_type.add()
        nested_type.name = 'Sub'
        enum_type = nested_type.enum_type.add()
        enum_type.name = 'FOO'
        enum_type_val = enum_type.value.add()
        enum_type_val.name = 'BAR'
        enum_type_val.number = 3
        field = message_type.field.add()
        field.number = 1
        field.name = 'uint64_field'
        field.label = descriptor.FieldDescriptor.LABEL_REQUIRED
        field.type = descriptor.FieldDescriptor.TYPE_UINT64
        field = message_type.field.add()
        field.number = 2
        field.name = 'nested_message_field'
        field.label = descriptor.FieldDescriptor.LABEL_REQUIRED
        field.type = descriptor.FieldDescriptor.TYPE_MESSAGE
        field.type_name = 'Sub'
        enum_field = nested_type.field.add()
        enum_field.number = 2
        enum_field.name = 'bar_field'
        enum_field.label = descriptor.FieldDescriptor.LABEL_REQUIRED
        enum_field.type = descriptor.FieldDescriptor.TYPE_ENUM
        enum_field.type_name = 'Foo2.Sub.FOO'

        result = descriptor.MakeDescriptor(message_type)
        assert result.fields[0].cpp_type == descriptor.FieldDescriptor.CPPTYPE_UINT64
        assert result.fields[0].cpp_type == result.fields[0].CPPTYPE_UINT64
        assert result.fields[1].cpp_type == descriptor.FieldDescriptor.CPPTYPE_MESSAGE
        assert result.fields[1].cpp_type == result.fields[1].CPPTYPE_MESSAGE
        assert result.fields[1].message_type.containing_type == result
        assert result.nested_types[0].fields[0].full_name == 'Foo2.Sub.bar_field'
        assert result.nested_types[0].fields[0].enum_type == result.nested_types[0].enum_types[0]
        assert not result.has_options
        assert not result.fields[0].has_options
        if api_implementation.Type() == 'cpp':
            with pytest.raises(AttributeError):
                result.fields[0].has_options = False

    def test_make_descriptor_with_unsigned_int_field(self):
        file_descriptor_proto = descriptor_pb2.FileDescriptorProto()
        file_descriptor_proto.name = 'Foo'
        message_type = file_descriptor_proto.message_type.add()
        message_type.name = file_descriptor_proto.name
        enum_type = message_type.enum_type.add()
        enum_type.name = 'FOO'
        enum_type_val = enum_type.value.add()
        enum_type_val.name = 'BAR'
        enum_type_val.number = 3
        field = message_type.field.add()
        field.number = 1
        field.name = 'uint64_field'
        field.label = descriptor.FieldDescriptor.LABEL_REQUIRED
        field.type = descriptor.FieldDescriptor.TYPE_UINT64
        enum_field = message_type.field.add()
        enum_field.number = 2
        enum_field.name = 'bar_field'
        enum_field.label = descriptor.FieldDescriptor.LABEL_REQUIRED
        enum_field.type = descriptor.FieldDescriptor.TYPE_ENUM
        enum_field.type_name = 'Foo.FOO'

        result = descriptor.MakeDescriptor(message_type)
        assert result.fields[0].cpp_type == descriptor.FieldDescriptor.CPPTYPE_UINT64


    def test_make_descriptor_with_options(self):
        descriptor_proto = descriptor_pb2.DescriptorProto()
        aggregate_message = unittest_custom_options_pb2.AggregateMessage
        aggregate_message.DESCRIPTOR.CopyToProto(descriptor_proto)
        reformed_descriptor = descriptor.MakeDescriptor(descriptor_proto)

        options = reformed_descriptor.GetOptions()
        assert 101 == options.Extensions[unittest_custom_options_pb2.msgopt].i

    def test_camelcase_name(self):
        descriptor_proto = descriptor_pb2.DescriptorProto()
        descriptor_proto.options.deprecated_legacy_json_field_conflicts = True
        descriptor_proto.name = 'Bar'
        names = ['foo_foo', 'FooBar', 'fooBaz', 'fooFoo', 'foobar']
        camelcase_names = ['fooFoo', 'fooBar', 'fooBaz', 'fooFoo', 'foobar']
        for index, _ in enumerate(names):
            field = descriptor_proto.field.add()
            field.number = index + 1
            field.name = names[index]
        result = descriptor.MakeDescriptor(descriptor_proto)
        for index, _ in enumerate(camelcase_names):
            assert result.fields[index].camelcase_name == camelcase_names[index]

    def test_json_name(self):
        descriptor_proto = descriptor_pb2.DescriptorProto()
        descriptor_proto.options.deprecated_legacy_json_field_conflicts = True
        descriptor_proto.name = 'TestJsonName'
        names = ['field_name', 'fieldName', 'FieldName',
                '_field_name', 'FIELD_NAME', 'json_name']
        json_names = ['fieldName', 'fieldName', 'FieldName',
                      'FieldName', 'FIELDNAME', '@type']
        for index, _ in enumerate(names):
            field = descriptor_proto.field.add()
            field.number = index + 1
            field.name = names[index]
        field.json_name = '@type'
        result = descriptor.MakeDescriptor(descriptor_proto)
        for index, _ in enumerate(json_names):
            assert result.fields[index].json_name == json_names[index]
