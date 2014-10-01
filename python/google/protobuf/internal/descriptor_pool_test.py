#! /usr/bin/python
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
import unittest

from google.apputils import basetest
from google.protobuf import unittest_pb2
from google.protobuf import descriptor_pb2
from google.protobuf.internal import api_implementation
from google.protobuf.internal import descriptor_pool_test1_pb2
from google.protobuf.internal import descriptor_pool_test2_pb2
from google.protobuf.internal import factory_test1_pb2
from google.protobuf.internal import factory_test2_pb2
from google.protobuf import descriptor
from google.protobuf import descriptor_database
from google.protobuf import descriptor_pool


class DescriptorPoolTest(basetest.TestCase):

  def setUp(self):
    self.pool = descriptor_pool.DescriptorPool()
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
    self.assertEquals(name1, file_desc1.name)
    self.assertEquals('google.protobuf.python.internal', file_desc1.package)
    self.assertIn('Factory1Message', file_desc1.message_types_by_name)

    name2 = 'google/protobuf/internal/factory_test2.proto'
    file_desc2 = self.pool.FindFileByName(name2)
    self.assertIsInstance(file_desc2, descriptor.FileDescriptor)
    self.assertEquals(name2, file_desc2.name)
    self.assertEquals('google.protobuf.python.internal', file_desc2.package)
    self.assertIn('Factory2Message', file_desc2.message_types_by_name)

  def testFindFileByNameFailure(self):
    with self.assertRaises(KeyError):
      self.pool.FindFileByName('Does not exist')

  def testFindFileContainingSymbol(self):
    file_desc1 = self.pool.FindFileContainingSymbol(
        'google.protobuf.python.internal.Factory1Message')
    self.assertIsInstance(file_desc1, descriptor.FileDescriptor)
    self.assertEquals('google/protobuf/internal/factory_test1.proto',
                      file_desc1.name)
    self.assertEquals('google.protobuf.python.internal', file_desc1.package)
    self.assertIn('Factory1Message', file_desc1.message_types_by_name)

    file_desc2 = self.pool.FindFileContainingSymbol(
        'google.protobuf.python.internal.Factory2Message')
    self.assertIsInstance(file_desc2, descriptor.FileDescriptor)
    self.assertEquals('google/protobuf/internal/factory_test2.proto',
                      file_desc2.name)
    self.assertEquals('google.protobuf.python.internal', file_desc2.package)
    self.assertIn('Factory2Message', file_desc2.message_types_by_name)

  def testFindFileContainingSymbolFailure(self):
    with self.assertRaises(KeyError):
      self.pool.FindFileContainingSymbol('Does not exist')

  def testFindMessageTypeByName(self):
    msg1 = self.pool.FindMessageTypeByName(
        'google.protobuf.python.internal.Factory1Message')
    self.assertIsInstance(msg1, descriptor.Descriptor)
    self.assertEquals('Factory1Message', msg1.name)
    self.assertEquals('google.protobuf.python.internal.Factory1Message',
                      msg1.full_name)
    self.assertEquals(None, msg1.containing_type)

    nested_msg1 = msg1.nested_types[0]
    self.assertEquals('NestedFactory1Message', nested_msg1.name)
    self.assertEquals(msg1, nested_msg1.containing_type)

    nested_enum1 = msg1.enum_types[0]
    self.assertEquals('NestedFactory1Enum', nested_enum1.name)
    self.assertEquals(msg1, nested_enum1.containing_type)

    self.assertEquals(nested_msg1, msg1.fields_by_name[
        'nested_factory_1_message'].message_type)
    self.assertEquals(nested_enum1, msg1.fields_by_name[
        'nested_factory_1_enum'].enum_type)

    msg2 = self.pool.FindMessageTypeByName(
        'google.protobuf.python.internal.Factory2Message')
    self.assertIsInstance(msg2, descriptor.Descriptor)
    self.assertEquals('Factory2Message', msg2.name)
    self.assertEquals('google.protobuf.python.internal.Factory2Message',
                      msg2.full_name)
    self.assertIsNone(msg2.containing_type)

    nested_msg2 = msg2.nested_types[0]
    self.assertEquals('NestedFactory2Message', nested_msg2.name)
    self.assertEquals(msg2, nested_msg2.containing_type)

    nested_enum2 = msg2.enum_types[0]
    self.assertEquals('NestedFactory2Enum', nested_enum2.name)
    self.assertEquals(msg2, nested_enum2.containing_type)

    self.assertEquals(nested_msg2, msg2.fields_by_name[
        'nested_factory_2_message'].message_type)
    self.assertEquals(nested_enum2, msg2.fields_by_name[
        'nested_factory_2_enum'].enum_type)

    self.assertTrue(msg2.fields_by_name['int_with_default'].has_default_value)
    self.assertEquals(
        1776, msg2.fields_by_name['int_with_default'].default_value)

    self.assertTrue(
        msg2.fields_by_name['double_with_default'].has_default_value)
    self.assertEquals(
        9.99, msg2.fields_by_name['double_with_default'].default_value)

    self.assertTrue(
        msg2.fields_by_name['string_with_default'].has_default_value)
    self.assertEquals(
        'hello world', msg2.fields_by_name['string_with_default'].default_value)

    self.assertTrue(msg2.fields_by_name['bool_with_default'].has_default_value)
    self.assertFalse(msg2.fields_by_name['bool_with_default'].default_value)

    self.assertTrue(msg2.fields_by_name['enum_with_default'].has_default_value)
    self.assertEquals(
        1, msg2.fields_by_name['enum_with_default'].default_value)

    msg3 = self.pool.FindMessageTypeByName(
        'google.protobuf.python.internal.Factory2Message.NestedFactory2Message')
    self.assertEquals(nested_msg2, msg3)

    self.assertTrue(msg2.fields_by_name['bytes_with_default'].has_default_value)
    self.assertEquals(
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
    self.assertEquals(0, enum1.values_by_name['FACTORY_1_VALUE_0'].number)
    self.assertEquals(1, enum1.values_by_name['FACTORY_1_VALUE_1'].number)

    nested_enum1 = self.pool.FindEnumTypeByName(
        'google.protobuf.python.internal.Factory1Message.NestedFactory1Enum')
    self.assertIsInstance(nested_enum1, descriptor.EnumDescriptor)
    self.assertEquals(
        0, nested_enum1.values_by_name['NESTED_FACTORY_1_VALUE_0'].number)
    self.assertEquals(
        1, nested_enum1.values_by_name['NESTED_FACTORY_1_VALUE_1'].number)

    enum2 = self.pool.FindEnumTypeByName(
        'google.protobuf.python.internal.Factory2Enum')
    self.assertIsInstance(enum2, descriptor.EnumDescriptor)
    self.assertEquals(0, enum2.values_by_name['FACTORY_2_VALUE_0'].number)
    self.assertEquals(1, enum2.values_by_name['FACTORY_2_VALUE_1'].number)

    nested_enum2 = self.pool.FindEnumTypeByName(
        'google.protobuf.python.internal.Factory2Message.NestedFactory2Enum')
    self.assertIsInstance(nested_enum2, descriptor.EnumDescriptor)
    self.assertEquals(
        0, nested_enum2.values_by_name['NESTED_FACTORY_2_VALUE_0'].number)
    self.assertEquals(
        1, nested_enum2.values_by_name['NESTED_FACTORY_2_VALUE_1'].number)

  def testFindEnumTypeByNameFailure(self):
    with self.assertRaises(KeyError):
      self.pool.FindEnumTypeByName('Does not exist')

  def testUserDefinedDB(self):
    db = descriptor_database.DescriptorDatabase()
    self.pool = descriptor_pool.DescriptorPool(db)
    db.Add(self.factory_test1_fd)
    db.Add(self.factory_test2_fd)
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



class ProtoFile(object):

  def __init__(self, name, package, messages, dependencies=None):
    self.name = name
    self.package = package
    self.messages = messages
    self.dependencies = dependencies or []

  def CheckFile(self, test, pool):
    file_desc = pool.FindFileByName(self.name)
    test.assertEquals(self.name, file_desc.name)
    test.assertEquals(self.package, file_desc.package)
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
    test.assertEqual(enum_desc.values_by_name[self.default_value].index,
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


class AddDescriptorTest(basetest.TestCase):

  def _TestMessage(self, prefix):
    pool = descriptor_pool.DescriptorPool()
    pool.AddDescriptor(unittest_pb2.TestAllTypes.DESCRIPTOR)
    self.assertEquals(
        'protobuf_unittest.TestAllTypes',
        pool.FindMessageTypeByName(
            prefix + 'protobuf_unittest.TestAllTypes').full_name)

    # AddDescriptor is not recursive.
    with self.assertRaises(KeyError):
      pool.FindMessageTypeByName(
          prefix + 'protobuf_unittest.TestAllTypes.NestedMessage')

    pool.AddDescriptor(unittest_pb2.TestAllTypes.NestedMessage.DESCRIPTOR)
    self.assertEquals(
        'protobuf_unittest.TestAllTypes.NestedMessage',
        pool.FindMessageTypeByName(
            prefix + 'protobuf_unittest.TestAllTypes.NestedMessage').full_name)

    # Files are implicitly also indexed when messages are added.
    self.assertEquals(
        'google/protobuf/unittest.proto',
        pool.FindFileByName(
            'google/protobuf/unittest.proto').name)

    self.assertEquals(
        'google/protobuf/unittest.proto',
        pool.FindFileContainingSymbol(
            prefix + 'protobuf_unittest.TestAllTypes.NestedMessage').name)

  def testMessage(self):
    self._TestMessage('')
    self._TestMessage('.')

  def _TestEnum(self, prefix):
    pool = descriptor_pool.DescriptorPool()
    pool.AddEnumDescriptor(unittest_pb2.ForeignEnum.DESCRIPTOR)
    self.assertEquals(
        'protobuf_unittest.ForeignEnum',
        pool.FindEnumTypeByName(
            prefix + 'protobuf_unittest.ForeignEnum').full_name)

    # AddEnumDescriptor is not recursive.
    with self.assertRaises(KeyError):
      pool.FindEnumTypeByName(
          prefix + 'protobuf_unittest.ForeignEnum.NestedEnum')

    pool.AddEnumDescriptor(unittest_pb2.TestAllTypes.NestedEnum.DESCRIPTOR)
    self.assertEquals(
        'protobuf_unittest.TestAllTypes.NestedEnum',
        pool.FindEnumTypeByName(
            prefix + 'protobuf_unittest.TestAllTypes.NestedEnum').full_name)

    # Files are implicitly also indexed when enums are added.
    self.assertEquals(
        'google/protobuf/unittest.proto',
        pool.FindFileByName(
            'google/protobuf/unittest.proto').name)

    self.assertEquals(
        'google/protobuf/unittest.proto',
        pool.FindFileContainingSymbol(
            prefix + 'protobuf_unittest.TestAllTypes.NestedEnum').name)

  def testEnum(self):
    self._TestEnum('')
    self._TestEnum('.')

  def testFile(self):
    pool = descriptor_pool.DescriptorPool()
    pool.AddFileDescriptor(unittest_pb2.DESCRIPTOR)
    self.assertEquals(
        'google/protobuf/unittest.proto',
        pool.FindFileByName(
            'google/protobuf/unittest.proto').name)

    # AddFileDescriptor is not recursive; messages and enums within files must
    # be explicitly registered.
    with self.assertRaises(KeyError):
      pool.FindFileContainingSymbol(
          'protobuf_unittest.TestAllTypes')


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
  basetest.main()
