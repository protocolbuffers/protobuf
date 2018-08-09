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

"""Tests for google.protobuf.message_factory."""

__author__ = 'matthewtoia@google.com (Matt Toia)'

try:
  import unittest2 as unittest  #PY26
except ImportError:
  import unittest

from google.protobuf import descriptor_pb2
from google.protobuf.internal import api_implementation
from google.protobuf.internal import factory_test1_pb2
from google.protobuf.internal import factory_test2_pb2
from google.protobuf import descriptor_database
from google.protobuf import descriptor_pool
from google.protobuf import message_factory


class MessageFactoryTest(unittest.TestCase):

  def setUp(self):
    self.factory_test1_fd = descriptor_pb2.FileDescriptorProto.FromString(
        factory_test1_pb2.DESCRIPTOR.serialized_pb)
    self.factory_test2_fd = descriptor_pb2.FileDescriptorProto.FromString(
        factory_test2_pb2.DESCRIPTOR.serialized_pb)

  def _ExerciseDynamicClass(self, cls):
    msg = cls()
    msg.mandatory = 42
    msg.nested_factory_2_enum = 0
    msg.nested_factory_2_message.value = 'nested message value'
    msg.factory_1_message.factory_1_enum = 1
    msg.factory_1_message.nested_factory_1_enum = 0
    msg.factory_1_message.nested_factory_1_message.value = (
        'nested message value')
    msg.factory_1_message.scalar_value = 22
    msg.factory_1_message.list_value.extend([u'one', u'two', u'three'])
    msg.factory_1_message.list_value.append(u'four')
    msg.factory_1_enum = 1
    msg.nested_factory_1_enum = 0
    msg.nested_factory_1_message.value = 'nested message value'
    msg.circular_message.mandatory = 1
    msg.circular_message.circular_message.mandatory = 2
    msg.circular_message.scalar_value = 'one deep'
    msg.scalar_value = 'zero deep'
    msg.list_value.extend([u'four', u'three', u'two'])
    msg.list_value.append(u'one')
    msg.grouped.add()
    msg.grouped[0].part_1 = 'hello'
    msg.grouped[0].part_2 = 'world'
    msg.grouped.add(part_1='testing', part_2='123')
    msg.loop.loop.mandatory = 2
    msg.loop.loop.loop.loop.mandatory = 4
    serialized = msg.SerializeToString()
    converted = factory_test2_pb2.Factory2Message.FromString(serialized)
    reserialized = converted.SerializeToString()
    self.assertEqual(serialized, reserialized)
    result = cls.FromString(reserialized)
    self.assertEqual(msg, result)

  def testGetPrototype(self):
    db = descriptor_database.DescriptorDatabase()
    pool = descriptor_pool.DescriptorPool(db)
    db.Add(self.factory_test1_fd)
    db.Add(self.factory_test2_fd)
    factory = message_factory.MessageFactory()
    cls = factory.GetPrototype(pool.FindMessageTypeByName(
        'google.protobuf.python.internal.Factory2Message'))
    self.assertFalse(cls is factory_test2_pb2.Factory2Message)
    self._ExerciseDynamicClass(cls)
    cls2 = factory.GetPrototype(pool.FindMessageTypeByName(
        'google.protobuf.python.internal.Factory2Message'))
    self.assertTrue(cls is cls2)

  def testGetMessages(self):
    # performed twice because multiple calls with the same input must be allowed
    for _ in range(2):
      # GetMessage should work regardless of the order the FileDescriptorProto
      # are provided. In particular, the function should succeed when the files
      # are not in the topological order of dependencies.

      # Assuming factory_test2_fd depends on factory_test1_fd.
      self.assertIn(self.factory_test1_fd.name,
                    self.factory_test2_fd.dependency)
      # Get messages should work when a file comes before its dependencies:
      # factory_test2_fd comes before factory_test1_fd.
      messages = message_factory.GetMessages([self.factory_test2_fd,
                                              self.factory_test1_fd])
      self.assertTrue(
          set(['google.protobuf.python.internal.Factory2Message',
               'google.protobuf.python.internal.Factory1Message'],
             ).issubset(set(messages.keys())))
      self._ExerciseDynamicClass(
          messages['google.protobuf.python.internal.Factory2Message'])
      factory_msg1 = messages['google.protobuf.python.internal.Factory1Message']
      self.assertTrue(set(
          ['google.protobuf.python.internal.Factory2Message.one_more_field',
           'google.protobuf.python.internal.another_field'],).issubset(set(
               ext.full_name
               for ext in factory_msg1.DESCRIPTOR.file.pool.FindAllExtensions(
                   factory_msg1.DESCRIPTOR))))
      msg1 = messages['google.protobuf.python.internal.Factory1Message']()
      ext1 = msg1.Extensions._FindExtensionByName(
          'google.protobuf.python.internal.Factory2Message.one_more_field')
      ext2 = msg1.Extensions._FindExtensionByName(
          'google.protobuf.python.internal.another_field')
      msg1.Extensions[ext1] = 'test1'
      msg1.Extensions[ext2] = 'test2'
      self.assertEqual('test1', msg1.Extensions[ext1])
      self.assertEqual('test2', msg1.Extensions[ext2])
      self.assertEqual(None,
                       msg1.Extensions._FindExtensionByNumber(12321))
      self.assertRaises(TypeError, len, msg1.Extensions)
      if api_implementation.Type() == 'cpp':
        self.assertRaises(TypeError,
                          msg1.Extensions._FindExtensionByName, 0)
        self.assertRaises(TypeError,
                          msg1.Extensions._FindExtensionByNumber, '')
      else:
        self.assertEqual(None,
                         msg1.Extensions._FindExtensionByName(0))
        self.assertEqual(None,
                         msg1.Extensions._FindExtensionByNumber(''))

  def testDuplicateExtensionNumber(self):
    pool = descriptor_pool.DescriptorPool()
    factory = message_factory.MessageFactory(pool=pool)

    # Add Container message.
    f = descriptor_pb2.FileDescriptorProto()
    f.name = 'google/protobuf/internal/container.proto'
    f.package = 'google.protobuf.python.internal'
    msg = f.message_type.add()
    msg.name = 'Container'
    rng = msg.extension_range.add()
    rng.start = 1
    rng.end = 10
    pool.Add(f)
    msgs = factory.GetMessages([f.name])
    self.assertIn('google.protobuf.python.internal.Container', msgs)

    # Extend container.
    f = descriptor_pb2.FileDescriptorProto()
    f.name = 'google/protobuf/internal/extension.proto'
    f.package = 'google.protobuf.python.internal'
    f.dependency.append('google/protobuf/internal/container.proto')
    msg = f.message_type.add()
    msg.name = 'Extension'
    ext = msg.extension.add()
    ext.name = 'extension_field'
    ext.number = 2
    ext.label = descriptor_pb2.FieldDescriptorProto.LABEL_OPTIONAL
    ext.type_name = 'Extension'
    ext.extendee = 'Container'
    pool.Add(f)
    msgs = factory.GetMessages([f.name])
    self.assertIn('google.protobuf.python.internal.Extension', msgs)

    # Add Duplicate extending the same field number.
    f = descriptor_pb2.FileDescriptorProto()
    f.name = 'google/protobuf/internal/duplicate.proto'
    f.package = 'google.protobuf.python.internal'
    f.dependency.append('google/protobuf/internal/container.proto')
    msg = f.message_type.add()
    msg.name = 'Duplicate'
    ext = msg.extension.add()
    ext.name = 'extension_field'
    ext.number = 2
    ext.label = descriptor_pb2.FieldDescriptorProto.LABEL_OPTIONAL
    ext.type_name = 'Duplicate'
    ext.extendee = 'Container'
    pool.Add(f)

    with self.assertRaises(Exception) as cm:
      factory.GetMessages([f.name])

    self.assertIn(str(cm.exception),
                  ['Extensions '
                   '"google.protobuf.python.internal.Duplicate.extension_field" and'
                   ' "google.protobuf.python.internal.Extension.extension_field"'
                   ' both try to extend message type'
                   ' "google.protobuf.python.internal.Container"'
                   ' with field number 2.',
                   'Double registration of Extensions'])


if __name__ == '__main__':
  unittest.main()
