# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Tests for google.protobuf.internal.keywords."""

import unittest


from google.protobuf.internal import more_messages_pb2
from google.protobuf import descriptor_pool


class KeywordsConflictTest(unittest.TestCase):

  def setUp(self):
    super(KeywordsConflictTest, self).setUp()
    self.pool = descriptor_pool.Default()

  def testMessage(self):
    message = getattr(more_messages_pb2, 'class')()
    message.int_field = 123
    self.assertEqual(message.int_field, 123)
    des = self.pool.FindMessageTypeByName('google.protobuf.internal.class')
    self.assertEqual(des.name, 'class')

  def testNestedMessage(self):
    message = getattr(more_messages_pb2, 'class')()
    message.nested_message.field = 234
    self.assertEqual(message.nested_message.field, 234)
    des = self.pool.FindMessageTypeByName('google.protobuf.internal.class.try')
    self.assertEqual(des.name, 'try')

  def testField(self):
    message = getattr(more_messages_pb2, 'class')()
    setattr(message, 'if', 123)
    setattr(message, 'as', 1)
    self.assertEqual(getattr(message, 'if'), 123)
    self.assertEqual(getattr(message, 'as'), 1)

  def testEnum(self):
    class_ = getattr(more_messages_pb2, 'class')
    message = class_()
    # Normal enum value.
    message.enum_field = more_messages_pb2.default
    self.assertEqual(message.enum_field, more_messages_pb2.default)
    # Top level enum value.
    message.enum_field = getattr(more_messages_pb2, 'else')
    self.assertEqual(message.enum_field, 1)
    # Nested enum value
    message.nested_enum_field = getattr(class_, 'True')
    self.assertEqual(message.nested_enum_field, 1)

  def testExtension(self):
    message = getattr(more_messages_pb2, 'class')()
    # Top level extension
    extension1 = getattr(more_messages_pb2, 'continue')
    message.Extensions[extension1] = 456
    self.assertEqual(message.Extensions[extension1], 456)
    # None top level extension
    extension2 = getattr(more_messages_pb2.ExtendClass, 'return')
    message.Extensions[extension2] = 789
    self.assertEqual(message.Extensions[extension2], 789)

  def testExtensionForNestedMessage(self):
    message = getattr(more_messages_pb2, 'class')()
    extension = getattr(more_messages_pb2, 'with')
    message.nested_message.Extensions[extension] = 999
    self.assertEqual(message.nested_message.Extensions[extension], 999)

  def TestFullKeywordUsed(self):
    message = more_messages_pb2.TestFullKeyword()
    message.field2.int_field = 123


if __name__ == '__main__':
  unittest.main()
