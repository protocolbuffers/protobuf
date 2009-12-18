#! /usr/bin/python
#
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

"""Unittest for google.protobuf.internal.descriptor."""

__author__ = 'robinson@google.com (Will Robinson)'

import unittest
from google.protobuf import unittest_import_pb2
from google.protobuf import unittest_pb2
from google.protobuf import descriptor_pb2
from google.protobuf import descriptor
from google.protobuf import text_format


TEST_EMPTY_MESSAGE_DESCRIPTOR_ASCII = """
name: 'TestEmptyMessage'
"""


class DescriptorTest(unittest.TestCase):

  def setUp(self):
    self.my_file = descriptor.FileDescriptor(
        name='some/filename/some.proto',
        package='protobuf_unittest'
        )
    self.my_enum = descriptor.EnumDescriptor(
        name='ForeignEnum',
        full_name='protobuf_unittest.ForeignEnum',
        filename=None,
        file=self.my_file,
        values=[
          descriptor.EnumValueDescriptor(name='FOREIGN_FOO', index=0, number=4),
          descriptor.EnumValueDescriptor(name='FOREIGN_BAR', index=1, number=5),
          descriptor.EnumValueDescriptor(name='FOREIGN_BAZ', index=2, number=6),
        ])
    self.my_message = descriptor.Descriptor(
        name='NestedMessage',
        full_name='protobuf_unittest.TestAllTypes.NestedMessage',
        filename=None,
        file=self.my_file,
        containing_type=None,
        fields=[
          descriptor.FieldDescriptor(
            name='bb',
            full_name='protobuf_unittest.TestAllTypes.NestedMessage.bb',
            index=0, number=1,
            type=5, cpp_type=1, label=1,
            has_default_value=False, default_value=0,
            message_type=None, enum_type=None, containing_type=None,
            is_extension=False, extension_scope=None),
        ],
        nested_types=[],
        enum_types=[
          self.my_enum,
        ],
        extensions=[])
    self.my_method = descriptor.MethodDescriptor(
        name='Bar',
        full_name='protobuf_unittest.TestService.Bar',
        index=0,
        containing_service=None,
        input_type=None,
        output_type=None)
    self.my_service = descriptor.ServiceDescriptor(
        name='TestServiceWithOptions',
        full_name='protobuf_unittest.TestServiceWithOptions',
        file=self.my_file,
        index=0,
        methods=[
            self.my_method
        ])

  def testEnumFixups(self):
    self.assertEqual(self.my_enum, self.my_enum.values[0].type)

  def testContainingTypeFixups(self):
    self.assertEqual(self.my_message, self.my_message.fields[0].containing_type)
    self.assertEqual(self.my_message, self.my_enum.containing_type)

  def testContainingServiceFixups(self):
    self.assertEqual(self.my_service, self.my_method.containing_service)

  def testGetOptions(self):
    self.assertEqual(self.my_enum.GetOptions(),
                     descriptor_pb2.EnumOptions())
    self.assertEqual(self.my_enum.values[0].GetOptions(),
                     descriptor_pb2.EnumValueOptions())
    self.assertEqual(self.my_message.GetOptions(),
                     descriptor_pb2.MessageOptions())
    self.assertEqual(self.my_message.fields[0].GetOptions(),
                     descriptor_pb2.FieldOptions())
    self.assertEqual(self.my_method.GetOptions(),
                     descriptor_pb2.MethodOptions())
    self.assertEqual(self.my_service.GetOptions(),
                     descriptor_pb2.ServiceOptions())

  def testFileDescriptorReferences(self):
    self.assertEqual(self.my_enum.file, self.my_file)
    self.assertEqual(self.my_message.file, self.my_file)

  def testFileDescriptor(self):
    self.assertEqual(self.my_file.name, 'some/filename/some.proto')
    self.assertEqual(self.my_file.package, 'protobuf_unittest')


class DescriptorCopyToProtoTest(unittest.TestCase):
  """Tests for CopyTo functions of Descriptor."""

  def _AssertProtoEqual(self, actual_proto, expected_class, expected_ascii):
    expected_proto = expected_class()
    text_format.Merge(expected_ascii, expected_proto)

    self.assertEqual(
        actual_proto, expected_proto,
        'Not equal,\nActual:\n%s\nExpected:\n%s\n'
        % (str(actual_proto), str(expected_proto)))

  def _InternalTestCopyToProto(self, desc, expected_proto_class,
                               expected_proto_ascii):
    actual = expected_proto_class()
    desc.CopyToProto(actual)
    self._AssertProtoEqual(
        actual, expected_proto_class, expected_proto_ascii)

  def testCopyToProto_EmptyMessage(self):
    self._InternalTestCopyToProto(
        unittest_pb2.TestEmptyMessage.DESCRIPTOR,
        descriptor_pb2.DescriptorProto,
        TEST_EMPTY_MESSAGE_DESCRIPTOR_ASCII)

  def testCopyToProto_NestedMessage(self):
    TEST_NESTED_MESSAGE_ASCII = """
      name: 'NestedMessage'
      field: <
        name: 'bb'
        number: 1
        label: 1  # Optional
        type: 5  # TYPE_INT32
      >
      """

    self._InternalTestCopyToProto(
        unittest_pb2.TestAllTypes.NestedMessage.DESCRIPTOR,
        descriptor_pb2.DescriptorProto,
        TEST_NESTED_MESSAGE_ASCII)

  def testCopyToProto_ForeignNestedMessage(self):
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

    self._InternalTestCopyToProto(
        unittest_pb2.TestForeignNested.DESCRIPTOR,
        descriptor_pb2.DescriptorProto,
        TEST_FOREIGN_NESTED_ASCII)

  def testCopyToProto_ForeignEnum(self):
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

    self._InternalTestCopyToProto(
        unittest_pb2._FOREIGNENUM,
        descriptor_pb2.EnumDescriptorProto,
        TEST_FOREIGN_ENUM_ASCII)

  def testCopyToProto_Options(self):
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
      """

    self._InternalTestCopyToProto(
        unittest_pb2.TestDeprecatedFields.DESCRIPTOR,
        descriptor_pb2.DescriptorProto,
        TEST_DEPRECATED_FIELDS_ASCII)

  def testCopyToProto_AllExtensions(self):
    TEST_EMPTY_MESSAGE_WITH_EXTENSIONS_ASCII = """
      name: 'TestEmptyMessageWithExtensions'
      extension_range: <
        start: 1
        end: 536870912
      >
      """

    self._InternalTestCopyToProto(
        unittest_pb2.TestEmptyMessageWithExtensions.DESCRIPTOR,
        descriptor_pb2.DescriptorProto,
        TEST_EMPTY_MESSAGE_WITH_EXTENSIONS_ASCII)

  def testCopyToProto_SeveralExtensions(self):
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

    self._InternalTestCopyToProto(
        unittest_pb2.TestMultipleExtensionRanges.DESCRIPTOR,
        descriptor_pb2.DescriptorProto,
        TEST_MESSAGE_WITH_SEVERAL_EXTENSIONS_ASCII)

  def testCopyToProto_FileDescriptor(self):
    UNITTEST_IMPORT_FILE_DESCRIPTOR_ASCII = ("""
      name: 'google/protobuf/unittest_import.proto'
      package: 'protobuf_unittest_import'
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
      options: <
        java_package: 'com.google.protobuf.test'
        optimize_for: 1  # SPEED
      >
      """)

    self._InternalTestCopyToProto(
        unittest_import_pb2.DESCRIPTOR,
        descriptor_pb2.FileDescriptorProto,
        UNITTEST_IMPORT_FILE_DESCRIPTOR_ASCII)

  def testCopyToProto_ServiceDescriptor(self):
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

    self._InternalTestCopyToProto(
        unittest_pb2.TestService.DESCRIPTOR,
        descriptor_pb2.ServiceDescriptorProto,
        TEST_SERVICE_ASCII)


if __name__ == '__main__':
  unittest.main()
