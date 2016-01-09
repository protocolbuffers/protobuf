#! /usr/bin/env python
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

"""Tests for google.protobuf.descriptor_pool."""

__author__ = 'matthewtoia@google.com (Matt Toia)'

import os

try:
  import unittest2 as unittest
except ImportError:
  import unittest
from google.protobuf import unittest_import_pb2
from google.protobuf import unittest_import_public_pb2
from google.protobuf import unittest_pb2
from google.protobuf import descriptor_pb2
from google.protobuf.internal import api_implementation
from google.protobuf.internal import descriptor_pool_test1_pb2
from google.protobuf.internal import descriptor_pool_test2_pb2
from google.protobuf.internal import factory_test1_pb2
from google.protobuf.internal import factory_test2_pb2
from google.protobuf.internal import test_util
from google.protobuf import descriptor
from google.protobuf import descriptor_database
from google.protobuf import descriptor_pool
from google.protobuf import message_factory
from google.protobuf import symbol_database


class DescriptorPoolTest(unittest.TestCase):

  def CreatePool(self):
    return descriptor_pool.DescriptorPool()

  def setUp(self):
    self.pool = self.CreatePool()
    self.factory_test1_fd = descriptor_pb2.FileDescriptorProto.FromString(
        factory_test1_pb2.DESCRIPTOR.serialized_pb)
    self.factory_test2_fd = descriptor_pb2.FileDescriptorProto.FromString(
        factory_test2_pb2.DESCRIPTOR.serialized_pb)
    self.pool.Add(self.factory_test1_fd)
    self.pool.Add(self.factory_test2_fd)

  def testFindFileByName(self):
    name1 = 'google/protobuf/internal/factory_test1.proto'
    file_desc1 = self.pool.FindFileByName(name1)
    self.assertIsInstance(file_desc1, descriptor.FileDescriptor)
    self.assertEqual(name1, file_desc1.name)
    self.assertEqual('google.protobuf.python.internal', file_desc1.package)
    self.assertIn('Factory1Message', file_desc1.message_types_by_name)

    name2 = 'google/protobuf/internal/factory_test2.proto'
    file_desc2 = self.pool.FindFileByName(name2)
    self.assertIsInstance(file_desc2, descriptor.FileDescriptor)
    self.assertEqual(name2, file_desc2.name)
    self.assertEqual('google.protobuf.python.internal', file_desc2.package)
    self.assertIn('Factory2Message', file_desc2.message_types_by_name)

  def testFindFileByNameFailure(self):
    with self.assertRaises(KeyError):
      self.pool.FindFileByName('Does not exist')

  def testFindFileContainingSymbol(self):
    file_desc1 = self.pool.FindFileContainingSymbol(
        'google.protobuf.python.internal.Factory1Message')
    self.assertIsInstance(file_desc1, descriptor.FileDescriptor)
    self.assertEqual('google/protobuf/internal/factory_test1.proto',
                     file_desc1.name)
    self.assertEqual('google.protobuf.python.internal', file_desc1.package)
    self.assertIn('Factory1Message', file_desc1.message_types_by_name)

    file_desc2 = self.pool.FindFileContainingSymbol(
        'google.protobuf.python.internal.Factory2Message')
    self.assertIsInstance(file_desc2, descriptor.FileDescriptor)
    self.assertEqual('google/protobuf/internal/factory_test2.proto',
                     file_desc2.name)
    self.assertEqual('google.protobuf.python.internal', file_desc2.package)
    self.assertIn('Factory2Message', file_desc2.message_types_by_name)

  def testFindFileContainingSymbolFailure(self):
    with self.assertRaises(KeyError):
      self.pool.FindFileContainingSymbol('Does not exist')

  def testFindMessageTypeByName(self):
    msg1 = self.pool.FindMessageTypeByName(
        'google.protobuf.python.internal.Factory1Message')
    self.assertIsInstance(msg1, descriptor.Descriptor)
    self.assertEqual('Factory1Message', msg1.name)
    self.assertEqual('google.protobuf.python.internal.Factory1Message',
                     msg1.full_name)
    self.assertEqual(None, msg1.containing_type)

    nested_msg1 = msg1.nested_types[0]
    self.assertEqual('NestedFactory1Message', nested_msg1.name)
    self.assertEqual(msg1, nested_msg1.containing_type)

    nested_enum1 = msg1.enum_types[0]
    self.assertEqual('NestedFactory1Enum', nested_enum1.name)
    self.assertEqual(msg1, nested_enum1.containing_type)

    self.assertEqual(nested_msg1, msg1.fields_by_name[
        'nested_factory_1_message'].message_type)
    self.assertEqual(nested_enum1, msg1.fields_by_name[
        'nested_factory_1_enum'].enum_type)

    msg2 = self.pool.FindMessageTypeByName(
        'google.protobuf.python.internal.Factory2Message')
    self.assertIsInstance(msg2, descriptor.Descriptor)
    self.assertEqual('Factory2Message', msg2.name)
    self.assertEqual('google.protobuf.python.internal.Factory2Message',
                     msg2.full_name)
    self.assertIsNone(msg2.containing_type)

    nested_msg2 = msg2.nested_types[0]
    self.assertEqual('NestedFactory2Message', nested_msg2.name)
    self.assertEqual(msg2, nested_msg2.containing_type)

    nested_enum2 = msg2.enum_types[0]
    self.assertEqual('NestedFactory2Enum', nested_enum2.name)
    self.assertEqual(msg2, nested_enum2.containing_type)

    self.assertEqual(nested_msg2, msg2.fields_by_name[
        'nested_factory_2_message'].message_type)
    self.assertEqual(nested_enum2, msg2.fields_by_name[
        'nested_factory_2_enum'].enum_type)

    self.assertTrue(msg2.fields_by_name['int_with_default'].has_default_value)
    self.assertEqual(
        1776, msg2.fields_by_name['int_with_default'].default_value)

    self.assertTrue(
        msg2.fields_by_name['double_with_default'].has_default_value)
    self.assertEqual(
        9.99, msg2.fields_by_name['double_with_default'].default_value)

    self.assertTrue(
        msg2.fields_by_name['string_with_default'].has_default_value)
    self.assertEqual(
        'hello world', msg2.fields_by_name['string_with_default'].default_value)

    self.assertTrue(msg2.fields_by_name['bool_with_default'].has_default_value)
    self.assertFalse(msg2.fields_by_name['bool_with_default'].default_value)

    self.assertTrue(msg2.fields_by_name['enum_with_default'].has_default_value)
    self.assertEqual(
        1, msg2.fields_by_name['enum_with_default'].default_value)

    msg3 = self.pool.FindMessageTypeByName(
        'google.protobuf.python.internal.Factory2Message.NestedFactory2Message')
    self.assertEqual(nested_msg2, msg3)

    self.assertTrue(msg2.fields_by_name['bytes_with_default'].has_default_value)
    self.assertEqual(
        b'a\xfb\x00c',
        msg2.fields_by_name['bytes_with_default'].default_value)

    self.assertEqual(1, len(msg2.oneofs))
    self.assertEqual(1, len(msg2.oneofs_by_name))
    self.assertEqual(2, len(msg2.oneofs[0].fields))
    for name in ['oneof_int', 'oneof_string']:
      self.assertEqual(msg2.oneofs[0],
                       msg2.fields_by_name[name].containing_oneof)
      self.assertIn(msg2.fields_by_name[name], msg2.oneofs[0].fields)

  def testFindMessageTypeByNameFailure(self):
    with self.assertRaises(KeyError):
      self.pool.FindMessageTypeByName('Does not exist')

  def testFindEnumTypeByName(self):
    enum1 = self.pool.FindEnumTypeByName(
        'google.protobuf.python.internal.Factory1Enum')
    self.assertIsInstance(enum1, descriptor.EnumDescriptor)
    self.assertEqual(0, enum1.values_by_name['FACTORY_1_VALUE_0'].number)
    self.assertEqual(1, enum1.values_by_name['FACTORY_1_VALUE_1'].number)

    nested_enum1 = self.pool.FindEnumTypeByName(
        'google.protobuf.python.internal.Factory1Message.NestedFactory1Enum')
    self.assertIsInstance(nested_enum1, descriptor.EnumDescriptor)
    self.assertEqual(
        0, nested_enum1.values_by_name['NESTED_FACTORY_1_VALUE_0'].number)
    self.assertEqual(
        1, nested_enum1.values_by_name['NESTED_FACTORY_1_VALUE_1'].number)

    enum2 = self.pool.FindEnumTypeByName(
        'google.protobuf.python.internal.Factory2Enum')
    self.assertIsInstance(enum2, descriptor.EnumDescriptor)
    self.assertEqual(0, enum2.values_by_name['FACTORY_2_VALUE_0'].number)
    self.assertEqual(1, enum2.values_by_name['FACTORY_2_VALUE_1'].number)

    nested_enum2 = self.pool.FindEnumTypeByName(
        'google.protobuf.python.internal.Factory2Message.NestedFactory2Enum')
    self.assertIsInstance(nested_enum2, descriptor.EnumDescriptor)
    self.assertEqual(
        0, nested_enum2.values_by_name['NESTED_FACTORY_2_VALUE_0'].number)
    self.assertEqual(
        1, nested_enum2.values_by_name['NESTED_FACTORY_2_VALUE_1'].number)

  def testFindEnumTypeByNameFailure(self):
    with self.assertRaises(KeyError):
      self.pool.FindEnumTypeByName('Does not exist')

  def testFindFieldByName(self):
    field = self.pool.FindFieldByName(
        'google.protobuf.python.internal.Factory1Message.list_value')
    self.assertEqual(field.name, 'list_value')
    self.assertEqual(field.label, field.LABEL_REPEATED)
    with self.assertRaises(KeyError):
      self.pool.FindFieldByName('Does not exist')

  def testFindExtensionByName(self):
    # An extension defined in a message.
    extension = self.pool.FindExtensionByName(
        'google.protobuf.python.internal.Factory2Message.one_more_field')
    self.assertEqual(extension.name, 'one_more_field')
    # An extension defined at file scope.
    extension = self.pool.FindExtensionByName(
        'google.protobuf.python.internal.another_field')
    self.assertEqual(extension.name, 'another_field')
    self.assertEqual(extension.number, 1002)
    with self.assertRaises(KeyError):
      self.pool.FindFieldByName('Does not exist')

  def testExtensionsAreNotFields(self):
    with self.assertRaises(KeyError):
      self.pool.FindFieldByName('google.protobuf.python.internal.another_field')
    with self.assertRaises(KeyError):
      self.pool.FindFieldByName(
          'google.protobuf.python.internal.Factory2Message.one_more_field')
    with self.assertRaises(KeyError):
      self.pool.FindExtensionByName(
          'google.protobuf.python.internal.Factory1Message.list_value')

  def testUserDefinedDB(self):
    db = descriptor_database.DescriptorDatabase()
    self.pool = descriptor_pool.DescriptorPool(db)
    db.Add(self.factory_test1_fd)
    db.Add(self.factory_test2_fd)
    self.testFindMessageTypeByName()

  def testAddSerializedFile(self):
    self.pool = descriptor_pool.DescriptorPool()
    self.pool.AddSerializedFile(self.factory_test1_fd.SerializeToString())
    self.pool.AddSerializedFile(self.factory_test2_fd.SerializeToString())
    self.testFindMessageTypeByName()

  def testComplexNesting(self):
    test1_desc = descriptor_pb2.FileDescriptorProto.FromString(
        descriptor_pool_test1_pb2.DESCRIPTOR.serialized_pb)
    test2_desc = descriptor_pb2.FileDescriptorProto.FromString(
        descriptor_pool_test2_pb2.DESCRIPTOR.serialized_pb)
    self.pool.Add(test1_desc)
    self.pool.Add(test2_desc)
    TEST1_FILE.CheckFile(self, self.pool)
    TEST2_FILE.CheckFile(self, self.pool)


  def testEnumDefaultValue(self):
    """Test the default value of enums which don't start at zero."""
    def _CheckDefaultValue(file_descriptor):
      default_value = (file_descriptor
                       .message_types_by_name['DescriptorPoolTest1']
                       .fields_by_name['nested_enum']
                       .default_value)
      self.assertEqual(default_value,
                       descriptor_pool_test1_pb2.DescriptorPoolTest1.BETA)
    # First check what the generated descriptor contains.
    _CheckDefaultValue(descriptor_pool_test1_pb2.DESCRIPTOR)
    # Then check the generated pool. Normally this is the same descriptor.
    file_descriptor = symbol_database.Default().pool.FindFileByName(
        'google/protobuf/internal/descriptor_pool_test1.proto')
    self.assertIs(file_descriptor, descriptor_pool_test1_pb2.DESCRIPTOR)
    _CheckDefaultValue(file_descriptor)

    # Then check the dynamic pool and its internal DescriptorDatabase.
    descriptor_proto = descriptor_pb2.FileDescriptorProto.FromString(
        descriptor_pool_test1_pb2.DESCRIPTOR.serialized_pb)
    self.pool.Add(descriptor_proto)
    # And do the same check as above
    file_descriptor = self.pool.FindFileByName(
        'google/protobuf/internal/descriptor_pool_test1.proto')
    _CheckDefaultValue(file_descriptor)

  def testDefaultValueForCustomMessages(self):
    """Check the value returned by non-existent fields."""
    def _CheckValueAndType(value, expected_value, expected_type):
      self.assertEqual(value, expected_value)
      self.assertIsInstance(value, expected_type)

    def _CheckDefaultValues(msg):
      try:
        int64 = long
      except NameError:  # Python3
        int64 = int
      try:
        unicode_type = unicode
      except NameError:  # Python3
        unicode_type = str
      _CheckValueAndType(msg.optional_int32, 0, int)
      _CheckValueAndType(msg.optional_uint64, 0, (int64, int))
      _CheckValueAndType(msg.optional_float, 0, (float, int))
      _CheckValueAndType(msg.optional_double, 0, (float, int))
      _CheckValueAndType(msg.optional_bool, False, bool)
      _CheckValueAndType(msg.optional_string, u'', unicode_type)
      _CheckValueAndType(msg.optional_bytes, b'', bytes)
      _CheckValueAndType(msg.optional_nested_enum, msg.FOO, int)
    # First for the generated message
    _CheckDefaultValues(unittest_pb2.TestAllTypes())
    # Then for a message built with from the DescriptorPool.
    pool = descriptor_pool.DescriptorPool()
    pool.Add(descriptor_pb2.FileDescriptorProto.FromString(
        unittest_import_public_pb2.DESCRIPTOR.serialized_pb))
    pool.Add(descriptor_pb2.FileDescriptorProto.FromString(
        unittest_import_pb2.DESCRIPTOR.serialized_pb))
    pool.Add(descriptor_pb2.FileDescriptorProto.FromString(
        unittest_pb2.DESCRIPTOR.serialized_pb))
    message_class = message_factory.MessageFactory(pool).GetPrototype(
        pool.FindMessageTypeByName(
            unittest_pb2.TestAllTypes.DESCRIPTOR.full_name))
    _CheckDefaultValues(message_class())


@unittest.skipIf(api_implementation.Type() != 'cpp',
                            'explicit tests of the C++ implementation')
class CppDescriptorPoolTest(DescriptorPoolTest):
  # TODO(amauryfa): remove when descriptor_pool.DescriptorPool() creates true
  # C++ descriptor pool object for C++ implementation.

  def CreatePool(self):
    # pylint: disable=g-import-not-at-top
    from google.protobuf.pyext import _message
    return _message.DescriptorPool()


class ProtoFile(object):

  def __init__(self, name, package, messages, dependencies=None):
    self.name = name
    self.package = package
    self.messages = messages
    self.dependencies = dependencies or []

  def CheckFile(self, test, pool):
    file_desc = pool.FindFileByName(self.name)
    test.assertEqual(self.name, file_desc.name)
    test.assertEqual(self.package, file_desc.package)
    dependencies_names = [f.name for f in file_desc.dependencies]
    test.assertEqual(self.dependencies, dependencies_names)
    for name, msg_type in self.messages.items():
      msg_type.CheckType(test, None, name, file_desc)


class EnumType(object):

  def __init__(self, values):
    self.values = values

  def CheckType(self, test, msg_desc, name, file_desc):
    enum_desc = msg_desc.enum_types_by_name[name]
    test.assertEqual(name, enum_desc.name)
    expected_enum_full_name = '.'.join([msg_desc.full_name, name])
    test.assertEqual(expected_enum_full_name, enum_desc.full_name)
    test.assertEqual(msg_desc, enum_desc.containing_type)
    test.assertEqual(file_desc, enum_desc.file)
    for index, (value, number) in enumerate(self.values):
      value_desc = enum_desc.values_by_name[value]
      test.assertEqual(value, value_desc.name)
      test.assertEqual(index, value_desc.index)
      test.assertEqual(number, value_desc.number)
      test.assertEqual(enum_desc, value_desc.type)
      test.assertIn(value, msg_desc.enum_values_by_name)


class MessageType(object):

  def __init__(self, type_dict, field_list, is_extendable=False,
               extensions=None):
    self.type_dict = type_dict
    self.field_list = field_list
    self.is_extendable = is_extendable
    self.extensions = extensions or []

  def CheckType(self, test, containing_type_desc, name, file_desc):
    if containing_type_desc is None:
      desc = file_desc.message_types_by_name[name]
      expected_full_name = '.'.join([file_desc.package, name])
    else:
      desc = containing_type_desc.nested_types_by_name[name]
      expected_full_name = '.'.join([containing_type_desc.full_name, name])

    test.assertEqual(name, desc.name)
    test.assertEqual(expected_full_name, desc.full_name)
    test.assertEqual(containing_type_desc, desc.containing_type)
    test.assertEqual(desc.file, file_desc)
    test.assertEqual(self.is_extendable, desc.is_extendable)
    for name, subtype in self.type_dict.items():
      subtype.CheckType(test, desc, name, file_desc)

    for index, (name, field) in enumerate(self.field_list):
      field.CheckField(test, desc, name, index)

    for index, (name, field) in enumerate(self.extensions):
      field.CheckField(test, desc, name, index)


class EnumField(object):

  def __init__(self, number, type_name, default_value):
    self.number = number
    self.type_name = type_name
    self.default_value = default_value

  def CheckField(self, test, msg_desc, name, index):
    field_desc = msg_desc.fields_by_name[name]
    enum_desc = msg_desc.enum_types_by_name[self.type_name]
    test.assertEqual(name, field_desc.name)
    expected_field_full_name = '.'.join([msg_desc.full_name, name])
    test.assertEqual(expected_field_full_name, field_desc.full_name)
    test.assertEqual(index, field_desc.index)
    test.assertEqual(self.number, field_desc.number)
    test.assertEqual(descriptor.FieldDescriptor.TYPE_ENUM, field_desc.type)
    test.assertEqual(descriptor.FieldDescriptor.CPPTYPE_ENUM,
                     field_desc.cpp_type)
    test.assertTrue(field_desc.has_default_value)
    test.assertEqual(enum_desc.values_by_name[self.default_value].number,
                     field_desc.default_value)
    test.assertEqual(msg_desc, field_desc.containing_type)
    test.assertEqual(enum_desc, field_desc.enum_type)


class MessageField(object):

  def __init__(self, number, type_name):
    self.number = number
    self.type_name = type_name

  def CheckField(self, test, msg_desc, name, index):
    field_desc = msg_desc.fields_by_name[name]
    field_type_desc = msg_desc.nested_types_by_name[self.type_name]
    test.assertEqual(name, field_desc.name)
    expected_field_full_name = '.'.join([msg_desc.full_name, name])
    test.assertEqual(expected_field_full_name, field_desc.full_name)
    test.assertEqual(index, field_desc.index)
    test.assertEqual(self.number, field_desc.number)
    test.assertEqual(descriptor.FieldDescriptor.TYPE_MESSAGE, field_desc.type)
    test.assertEqual(descriptor.FieldDescriptor.CPPTYPE_MESSAGE,
                     field_desc.cpp_type)
    test.assertFalse(field_desc.has_default_value)
    test.assertEqual(msg_desc, field_desc.containing_type)
    test.assertEqual(field_type_desc, field_desc.message_type)


class StringField(object):

  def __init__(self, number, default_value):
    self.number = number
    self.default_value = default_value

  def CheckField(self, test, msg_desc, name, index):
    field_desc = msg_desc.fields_by_name[name]
    test.assertEqual(name, field_desc.name)
    expected_field_full_name = '.'.join([msg_desc.full_name, name])
    test.assertEqual(expected_field_full_name, field_desc.full_name)
    test.assertEqual(index, field_desc.index)
    test.assertEqual(self.number, field_desc.number)
    test.assertEqual(descriptor.FieldDescriptor.TYPE_STRING, field_desc.type)
    test.assertEqual(descriptor.FieldDescriptor.CPPTYPE_STRING,
                     field_desc.cpp_type)
    test.assertTrue(field_desc.has_default_value)
    test.assertEqual(self.default_value, field_desc.default_value)


class ExtensionField(object):

  def __init__(self, number, extended_type):
    self.number = number
    self.extended_type = extended_type

  def CheckField(self, test, msg_desc, name, index):
    field_desc = msg_desc.extensions_by_name[name]
    test.assertEqual(name, field_desc.name)
    expected_field_full_name = '.'.join([msg_desc.full_name, name])
    test.assertEqual(expected_field_full_name, field_desc.full_name)
    test.assertEqual(self.number, field_desc.number)
    test.assertEqual(index, field_desc.index)
    test.assertEqual(descriptor.FieldDescriptor.TYPE_MESSAGE, field_desc.type)
    test.assertEqual(descriptor.FieldDescriptor.CPPTYPE_MESSAGE,
                     field_desc.cpp_type)
    test.assertFalse(field_desc.has_default_value)
    test.assertTrue(field_desc.is_extension)
    test.assertEqual(msg_desc, field_desc.extension_scope)
    test.assertEqual(msg_desc, field_desc.message_type)
    test.assertEqual(self.extended_type, field_desc.containing_type.name)


class AddDescriptorTest(unittest.TestCase):

  def _TestMessage(self, prefix):
    pool = descriptor_pool.DescriptorPool()
    pool.AddDescriptor(unittest_pb2.TestAllTypes.DESCRIPTOR)
    self.assertEqual(
        'protobuf_unittest.TestAllTypes',
        pool.FindMessageTypeByName(
            prefix + 'protobuf_unittest.TestAllTypes').full_name)

    # AddDescriptor is not recursive.
    with self.assertRaises(KeyError):
      pool.FindMessageTypeByName(
          prefix + 'protobuf_unittest.TestAllTypes.NestedMessage')

    pool.AddDescriptor(unittest_pb2.TestAllTypes.NestedMessage.DESCRIPTOR)
    self.assertEqual(
        'protobuf_unittest.TestAllTypes.NestedMessage',
        pool.FindMessageTypeByName(
            prefix + 'protobuf_unittest.TestAllTypes.NestedMessage').full_name)

    # Files are implicitly also indexed when messages are added.
    self.assertEqual(
        'google/protobuf/unittest.proto',
        pool.FindFileByName(
            'google/protobuf/unittest.proto').name)

    self.assertEqual(
        'google/protobuf/unittest.proto',
        pool.FindFileContainingSymbol(
            prefix + 'protobuf_unittest.TestAllTypes.NestedMessage').name)

  @unittest.skipIf(api_implementation.Type() == 'cpp',
                    'With the cpp implementation, Add() must be called first')
  def testMessage(self):
    self._TestMessage('')
    self._TestMessage('.')

  def _TestEnum(self, prefix):
    pool = descriptor_pool.DescriptorPool()
    pool.AddEnumDescriptor(unittest_pb2.ForeignEnum.DESCRIPTOR)
    self.assertEqual(
        'protobuf_unittest.ForeignEnum',
        pool.FindEnumTypeByName(
            prefix + 'protobuf_unittest.ForeignEnum').full_name)

    # AddEnumDescriptor is not recursive.
    with self.assertRaises(KeyError):
      pool.FindEnumTypeByName(
          prefix + 'protobuf_unittest.ForeignEnum.NestedEnum')

    pool.AddEnumDescriptor(unittest_pb2.TestAllTypes.NestedEnum.DESCRIPTOR)
    self.assertEqual(
        'protobuf_unittest.TestAllTypes.NestedEnum',
        pool.FindEnumTypeByName(
            prefix + 'protobuf_unittest.TestAllTypes.NestedEnum').full_name)

    # Files are implicitly also indexed when enums are added.
    self.assertEqual(
        'google/protobuf/unittest.proto',
        pool.FindFileByName(
            'google/protobuf/unittest.proto').name)

    self.assertEqual(
        'google/protobuf/unittest.proto',
        pool.FindFileContainingSymbol(
            prefix + 'protobuf_unittest.TestAllTypes.NestedEnum').name)

  @unittest.skipIf(api_implementation.Type() == 'cpp',
                    'With the cpp implementation, Add() must be called first')
  def testEnum(self):
    self._TestEnum('')
    self._TestEnum('.')

  @unittest.skipIf(api_implementation.Type() == 'cpp',
                    'With the cpp implementation, Add() must be called first')
  def testFile(self):
    pool = descriptor_pool.DescriptorPool()
    pool.AddFileDescriptor(unittest_pb2.DESCRIPTOR)
    self.assertEqual(
        'google/protobuf/unittest.proto',
        pool.FindFileByName(
            'google/protobuf/unittest.proto').name)

    # AddFileDescriptor is not recursive; messages and enums within files must
    # be explicitly registered.
    with self.assertRaises(KeyError):
      pool.FindFileContainingSymbol(
          'protobuf_unittest.TestAllTypes')

  def _GetDescriptorPoolClass(self):
    # Test with both implementations of descriptor pools.
    if api_implementation.Type() == 'cpp':
      # pylint: disable=g-import-not-at-top
      from google.protobuf.pyext import _message
      return _message.DescriptorPool
    else:
      return descriptor_pool.DescriptorPool

  def testEmptyDescriptorPool(self):
    # Check that an empty DescriptorPool() contains no message.
    pool = self._GetDescriptorPoolClass()()
    proto_file_name = descriptor_pb2.DESCRIPTOR.name
    self.assertRaises(KeyError, pool.FindFileByName, proto_file_name)
    # Add the above file to the pool
    file_descriptor = descriptor_pb2.FileDescriptorProto()
    descriptor_pb2.DESCRIPTOR.CopyToProto(file_descriptor)
    pool.Add(file_descriptor)
    # Now it exists.
    self.assertTrue(pool.FindFileByName(proto_file_name))

  def testCustomDescriptorPool(self):
    # Create a new pool, and add a file descriptor.
    pool = self._GetDescriptorPoolClass()()
    file_desc = descriptor_pb2.FileDescriptorProto(
        name='some/file.proto', package='package')
    file_desc.message_type.add(name='Message')
    pool.Add(file_desc)
    self.assertEqual(pool.FindFileByName('some/file.proto').name,
                     'some/file.proto')
    self.assertEqual(pool.FindMessageTypeByName('package.Message').name,
                     'Message')


@unittest.skipIf(
    api_implementation.Type() != 'cpp',
    'default_pool is only supported by the C++ implementation')
class DefaultPoolTest(unittest.TestCase):

  def testFindMethods(self):
    # pylint: disable=g-import-not-at-top
    from google.protobuf.pyext import _message
    pool = _message.default_pool
    self.assertIs(
        pool.FindFileByName('google/protobuf/unittest.proto'),
        unittest_pb2.DESCRIPTOR)
    self.assertIs(
        pool.FindMessageTypeByName('protobuf_unittest.TestAllTypes'),
        unittest_pb2.TestAllTypes.DESCRIPTOR)
    self.assertIs(
        pool.FindFieldByName('protobuf_unittest.TestAllTypes.optional_int32'),
        unittest_pb2.TestAllTypes.DESCRIPTOR.fields_by_name['optional_int32'])
    self.assertIs(
        pool.FindExtensionByName('protobuf_unittest.optional_int32_extension'),
        unittest_pb2.DESCRIPTOR.extensions_by_name['optional_int32_extension'])
    self.assertIs(
        pool.FindEnumTypeByName('protobuf_unittest.ForeignEnum'),
        unittest_pb2.ForeignEnum.DESCRIPTOR)
    self.assertIs(
        pool.FindOneofByName('protobuf_unittest.TestAllTypes.oneof_field'),
        unittest_pb2.TestAllTypes.DESCRIPTOR.oneofs_by_name['oneof_field'])

  def testAddFileDescriptor(self):
    # pylint: disable=g-import-not-at-top
    from google.protobuf.pyext import _message
    pool = _message.default_pool
    file_desc = descriptor_pb2.FileDescriptorProto(name='some/file.proto')
    pool.Add(file_desc)
    pool.AddSerializedFile(file_desc.SerializeToString())


TEST1_FILE = ProtoFile(
    'google/protobuf/internal/descriptor_pool_test1.proto',
    'google.protobuf.python.internal',
    {
        'DescriptorPoolTest1': MessageType({
            'NestedEnum': EnumType([('ALPHA', 1), ('BETA', 2)]),
            'NestedMessage': MessageType({
                'NestedEnum': EnumType([('EPSILON', 5), ('ZETA', 6)]),
                'DeepNestedMessage': MessageType({
                    'NestedEnum': EnumType([('ETA', 7), ('THETA', 8)]),
                }, [
                    ('nested_enum', EnumField(1, 'NestedEnum', 'ETA')),
                    ('nested_field', StringField(2, 'theta')),
                ]),
            }, [
                ('nested_enum', EnumField(1, 'NestedEnum', 'ZETA')),
                ('nested_field', StringField(2, 'beta')),
                ('deep_nested_message', MessageField(3, 'DeepNestedMessage')),
            ])
        }, [
            ('nested_enum', EnumField(1, 'NestedEnum', 'BETA')),
            ('nested_message', MessageField(2, 'NestedMessage')),
        ], is_extendable=True),

        'DescriptorPoolTest2': MessageType({
            'NestedEnum': EnumType([('GAMMA', 3), ('DELTA', 4)]),
            'NestedMessage': MessageType({
                'NestedEnum': EnumType([('IOTA', 9), ('KAPPA', 10)]),
                'DeepNestedMessage': MessageType({
                    'NestedEnum': EnumType([('LAMBDA', 11), ('MU', 12)]),
                }, [
                    ('nested_enum', EnumField(1, 'NestedEnum', 'MU')),
                    ('nested_field', StringField(2, 'lambda')),
                ]),
            }, [
                ('nested_enum', EnumField(1, 'NestedEnum', 'IOTA')),
                ('nested_field', StringField(2, 'delta')),
                ('deep_nested_message', MessageField(3, 'DeepNestedMessage')),
            ])
        }, [
            ('nested_enum', EnumField(1, 'NestedEnum', 'GAMMA')),
            ('nested_message', MessageField(2, 'NestedMessage')),
        ]),
    })


TEST2_FILE = ProtoFile(
    'google/protobuf/internal/descriptor_pool_test2.proto',
    'google.protobuf.python.internal',
    {
        'DescriptorPoolTest3': MessageType({
            'NestedEnum': EnumType([('NU', 13), ('XI', 14)]),
            'NestedMessage': MessageType({
                'NestedEnum': EnumType([('OMICRON', 15), ('PI', 16)]),
                'DeepNestedMessage': MessageType({
                    'NestedEnum': EnumType([('RHO', 17), ('SIGMA', 18)]),
                }, [
                    ('nested_enum', EnumField(1, 'NestedEnum', 'RHO')),
                    ('nested_field', StringField(2, 'sigma')),
                ]),
            }, [
                ('nested_enum', EnumField(1, 'NestedEnum', 'PI')),
                ('nested_field', StringField(2, 'nu')),
                ('deep_nested_message', MessageField(3, 'DeepNestedMessage')),
            ])
        }, [
            ('nested_enum', EnumField(1, 'NestedEnum', 'XI')),
            ('nested_message', MessageField(2, 'NestedMessage')),
        ], extensions=[
            ('descriptor_pool_test',
             ExtensionField(1001, 'DescriptorPoolTest1')),
        ]),
    },
    dependencies=['google/protobuf/internal/descriptor_pool_test1.proto'])


if __name__ == '__main__':
  unittest.main()
