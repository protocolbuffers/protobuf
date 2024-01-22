# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Tests for google.protobuf.message_factory."""

__author__ = 'matthewtoia@google.com (Matt Toia)'

import unittest
import gc

from google.protobuf import descriptor_pb2
from google.protobuf.internal import api_implementation
from google.protobuf.internal import factory_test1_pb2
from google.protobuf.internal import factory_test2_pb2
from google.protobuf.internal import testing_refleaks
from google.protobuf import descriptor_database
from google.protobuf import descriptor_pool
from google.protobuf import message_factory
from google.protobuf import descriptor

@testing_refleaks.TestCase
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
    cls = message_factory.GetMessageClass(pool.FindMessageTypeByName(
        'google.protobuf.python.internal.Factory2Message'))
    self.assertFalse(cls is factory_test2_pb2.Factory2Message)
    self._ExerciseDynamicClass(cls)
    cls2 = message_factory.GetMessageClass(pool.FindMessageTypeByName(
        'google.protobuf.python.internal.Factory2Message'))
    self.assertTrue(cls is cls2)

  def testGetExistingPrototype(self):
    # Get Existing Prototype should not create a new class.
    cls = message_factory.GetMessageClass(
        descriptor=factory_test2_pb2.Factory2Message.DESCRIPTOR)
    msg = factory_test2_pb2.Factory2Message()
    self.assertIsInstance(msg, cls)
    self.assertIsInstance(msg.factory_1_message,
                          factory_test1_pb2.Factory1Message)

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
                                              self.factory_test1_fd],
                                             descriptor_pool.Default())
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
      self.assertEqual(0, len(msg1.Extensions))
      msg1.Extensions[ext1] = 'test1'
      msg1.Extensions[ext2] = 'test2'
      self.assertEqual('test1', msg1.Extensions[ext1])
      self.assertEqual('test2', msg1.Extensions[ext2])
      self.assertEqual(None,
                       msg1.Extensions._FindExtensionByNumber(12321))
      self.assertEqual(2, len(msg1.Extensions))
      if api_implementation.Type() == 'python':
        self.assertEqual(None,
                         msg1.Extensions._FindExtensionByName(0))
        self.assertEqual(None,
                         msg1.Extensions._FindExtensionByNumber(''))
      else:
        self.assertRaises(TypeError, msg1.Extensions._FindExtensionByName, 0)
        self.assertRaises(TypeError, msg1.Extensions._FindExtensionByNumber, '')

  def testDuplicateExtensionNumber(self):
    pool = descriptor_pool.DescriptorPool()

    # Add Container message.
    f = descriptor_pb2.FileDescriptorProto(
        name='google/protobuf/internal/container.proto',
        package='google.protobuf.python.internal')
    f.message_type.add(name='Container').extension_range.add(start=1, end=10)
    pool.Add(f)
    msgs = message_factory.GetMessageClassesForFiles([f.name], pool)
    self.assertIn('google.protobuf.python.internal.Container', msgs)

    # Extend container.
    f = descriptor_pb2.FileDescriptorProto(
        name='google/protobuf/internal/extension.proto',
        package='google.protobuf.python.internal',
        dependency=['google/protobuf/internal/container.proto'])
    msg = f.message_type.add(name='Extension')
    msg.extension.add(
        name='extension_field',
        number=2,
        label=descriptor_pb2.FieldDescriptorProto.LABEL_OPTIONAL,
        type_name='Extension',
        extendee='Container',
    )
    pool.Add(f)
    msgs = message_factory.GetMessageClassesForFiles([f.name], pool)
    self.assertIn('google.protobuf.python.internal.Extension', msgs)

    # Add Duplicate extending the same field number.
    f = descriptor_pb2.FileDescriptorProto(
        name='google/protobuf/internal/duplicate.proto',
        package='google.protobuf.python.internal',
        dependency=['google/protobuf/internal/container.proto'])
    msg = f.message_type.add(name='Duplicate')
    msg.extension.add(
        name='extension_field',
        number=2,
        label=descriptor_pb2.FieldDescriptorProto.LABEL_OPTIONAL,
        type_name='Duplicate',
        extendee='Container',
    )
    pool.Add(f)

    with self.assertRaises(Exception) as cm:
      message_factory.GetMessageClassesForFiles([f.name], pool)

    self.assertIn(str(cm.exception),
                  ['Extensions '
                   '"google.protobuf.python.internal.Duplicate.extension_field" and'
                   ' "google.protobuf.python.internal.Extension.extension_field"'
                   ' both try to extend message type'
                   ' "google.protobuf.python.internal.Container"'
                   ' with field number 2.',
                   'Double registration of Extensions'])

  def testExtensionValueInDifferentFile(self):
    # Add Container message.
    f1 = descriptor_pb2.FileDescriptorProto(
        name='google/protobuf/internal/container.proto',
        package='google.protobuf.python.internal')
    f1.message_type.add(name='Container').extension_range.add(start=1, end=10)

    # Add ValueType message.
    f2 = descriptor_pb2.FileDescriptorProto(
        name='google/protobuf/internal/value_type.proto',
        package='google.protobuf.python.internal')
    f2.message_type.add(name='ValueType').field.add(
        name='setting',
        number=1,
        label=descriptor_pb2.FieldDescriptorProto.LABEL_OPTIONAL,
        type=descriptor_pb2.FieldDescriptorProto.TYPE_INT32,
        default_value='123')

    # Extend container with field of ValueType.
    f3 = descriptor_pb2.FileDescriptorProto(
        name='google/protobuf/internal/extension.proto',
        package='google.protobuf.python.internal',
        dependency=[f1.name, f2.name])
    f3.extension.add(
        name='top_level_extension_field',
        number=2,
        label=descriptor_pb2.FieldDescriptorProto.LABEL_OPTIONAL,
        type_name='ValueType',
        extendee='Container',
    )
    f3.message_type.add(name='Extension').extension.add(
        name='nested_extension_field',
        number=3,
        label=descriptor_pb2.FieldDescriptorProto.LABEL_OPTIONAL,
        type_name='ValueType',
        extendee='Container',
    )

    pool = descriptor_pool.Default()
    try:
      pool.Add(f1)
      pool.Add(f2)
      pool.Add(f3)
    except:
      pass
    msgs = message_factory.GetMessageClassesForFiles(
        [f1.name, f3.name], pool)  # Deliberately not f2.
    msg = msgs['google.protobuf.python.internal.Container']
    desc = msgs['google.protobuf.python.internal.Extension'].DESCRIPTOR
    ext1 = desc.file.extensions_by_name['top_level_extension_field']
    ext2 = desc.extensions_by_name['nested_extension_field']
    m = msg()
    m.Extensions[ext1].setting = 234
    m.Extensions[ext2].setting = 345
    serialized = m.SerializeToString()

    f1.name='google/protobuf/internal/another/container.proto'
    f1.package='google.protobuf.python.internal.another'
    f2.name='google/protobuf/internal/another/value_type.proto'
    f2.package='google.protobuf.python.internal.another'
    f3.name='google/protobuf/internal/another/extension.proto'
    f3.package='google.protobuf.python.internal.another'
    f3.ClearField('dependency')
    f3.dependency.extend([f1.name, f2.name])
    try:
      pool.Add(f1)
      pool.Add(f2)
      pool.Add(f3)
    except:
      pass
    msgs = message_factory.GetMessageClassesForFiles(
        [f1.name, f3.name], pool)  # Deliberately not f2.
    msg = msgs['google.protobuf.python.internal.another.Container']
    desc = msgs['google.protobuf.python.internal.another.Extension'].DESCRIPTOR
    ext1 = desc.file.extensions_by_name['top_level_extension_field']
    ext2 = desc.extensions_by_name['nested_extension_field']
    m = msg.FromString(serialized)
    self.assertEqual(2, len(m.ListFields()))
    self.assertEqual(234, m.Extensions[ext1].setting)
    self.assertEqual(345, m.Extensions[ext2].setting)

  def testDescriptorKeepConcreteClass(self):
    def loadFile():
      f= descriptor_pb2.FileDescriptorProto(
        name='google/protobuf/internal/meta_class.proto',
        package='google.protobuf.python.internal')
      msg_proto = f.message_type.add(name='Empty')
      msg_proto.nested_type.add(name='Nested')
      msg_proto.field.add(name='nested_field',
                          number=1,
                          label=descriptor.FieldDescriptor.LABEL_REPEATED,
                          type=descriptor.FieldDescriptor.TYPE_MESSAGE,
                          type_name='Nested')
      return message_factory.GetMessages([f])

    messages = loadFile()
    for des, meta_class in messages.items():
      message = meta_class()
      nested_des = message.DESCRIPTOR.nested_types_by_name['Nested']
      nested_msg = nested_des._concrete_class()

  def testOndemandCreateMetaClass(self):
    def loadFile():
      f = descriptor_pb2.FileDescriptorProto.FromString(
        factory_test1_pb2.DESCRIPTOR.serialized_pb)
      return message_factory.GetMessages([f])

    messages = loadFile()
    data = factory_test1_pb2.Factory1Message()
    data.map_field['hello'] = 'welcome'
    # Force GC to collect. UPB python will clean up the map entry class.
    # cpp extension and pure python will still keep the map entry class.
    gc.collect()
    message = messages['google.protobuf.python.internal.Factory1Message']()
    message.ParseFromString(data.SerializeToString())
    value = message.map_field
    values = [
        # The entry class will be created on demand in upb python.
        value.GetEntryClass()(key=k, value=value[k]) for k in sorted(value)
    ]
    gc.collect()
    self.assertEqual(1, len(values))
    self.assertEqual('hello', values[0].key)
    self.assertEqual('welcome', values[0].value)

if __name__ == '__main__':
  unittest.main()
