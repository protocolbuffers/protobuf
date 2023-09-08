# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Tests for google.protobuf.descriptor_database."""

__author__ = 'matthewtoia@google.com (Matt Toia)'

import unittest
import warnings

from google.protobuf import descriptor_pb2
from google.protobuf.internal import factory_test2_pb2
from google.protobuf.internal import no_package_pb2
from google.protobuf.internal import testing_refleaks
from google.protobuf import descriptor_database
from google.protobuf import unittest_pb2


@testing_refleaks.TestCase
class DescriptorDatabaseTest(unittest.TestCase):

  def testAdd(self):
    db = descriptor_database.DescriptorDatabase()
    file_desc_proto = descriptor_pb2.FileDescriptorProto.FromString(
        factory_test2_pb2.DESCRIPTOR.serialized_pb)
    file_desc_proto2 = descriptor_pb2.FileDescriptorProto.FromString(
        no_package_pb2.DESCRIPTOR.serialized_pb)
    db.Add(file_desc_proto)
    db.Add(file_desc_proto2)

    self.assertEqual(file_desc_proto, db.FindFileByName(
        'google/protobuf/internal/factory_test2.proto'))
    # Can find message type.
    self.assertEqual(file_desc_proto, db.FindFileContainingSymbol(
        'google.protobuf.python.internal.Factory2Message'))
    # Can find nested message type.
    self.assertEqual(file_desc_proto, db.FindFileContainingSymbol(
        'google.protobuf.python.internal.Factory2Message.NestedFactory2Message'))
    # Can find enum type.
    self.assertEqual(file_desc_proto, db.FindFileContainingSymbol(
        'google.protobuf.python.internal.Factory2Enum'))
    # Can find nested enum type.
    self.assertEqual(file_desc_proto, db.FindFileContainingSymbol(
        'google.protobuf.python.internal.Factory2Message.NestedFactory2Enum'))
    self.assertEqual(file_desc_proto, db.FindFileContainingSymbol(
        'google.protobuf.python.internal.MessageWithNestedEnumOnly.NestedEnum'))
    # Can find field.
    self.assertEqual(file_desc_proto, db.FindFileContainingSymbol(
        'google.protobuf.python.internal.Factory2Message.list_field'))
    # Can find enum value.
    self.assertEqual(file_desc_proto, db.FindFileContainingSymbol(
        'google.protobuf.python.internal.Factory2Enum.FACTORY_2_VALUE_0'))
    self.assertEqual(file_desc_proto, db.FindFileContainingSymbol(
        'google.protobuf.python.internal.FACTORY_2_VALUE_0'))
    self.assertEqual(file_desc_proto2, db.FindFileContainingSymbol(
        '.NO_PACKAGE_VALUE_0'))
    # Can find top level extension.
    self.assertEqual(file_desc_proto, db.FindFileContainingSymbol(
        'google.protobuf.python.internal.another_field'))
    # Can find nested extension inside a message.
    self.assertEqual(file_desc_proto, db.FindFileContainingSymbol(
        'google.protobuf.python.internal.Factory2Message.one_more_field'))

    # Can find service.
    file_desc_proto2 = descriptor_pb2.FileDescriptorProto.FromString(
        unittest_pb2.DESCRIPTOR.serialized_pb)
    db.Add(file_desc_proto2)
    self.assertEqual(file_desc_proto2, db.FindFileContainingSymbol(
        'protobuf_unittest.TestService'))

    # Non-existent field under a valid top level symbol can also be
    # found. The behavior is the same with protobuf C++.
    self.assertEqual(file_desc_proto2, db.FindFileContainingSymbol(
        'protobuf_unittest.TestAllTypes.none_field'))

    with self.assertRaisesRegex(KeyError, r'\'protobuf_unittest\.NoneMessage\''):
      db.FindFileContainingSymbol('protobuf_unittest.NoneMessage')

  def testConflictRegister(self):
    db = descriptor_database.DescriptorDatabase()
    unittest_fd = descriptor_pb2.FileDescriptorProto.FromString(
        unittest_pb2.DESCRIPTOR.serialized_pb)
    db.Add(unittest_fd)
    conflict_fd = descriptor_pb2.FileDescriptorProto.FromString(
        unittest_pb2.DESCRIPTOR.serialized_pb)
    conflict_fd.name = 'other_file2'
    with warnings.catch_warnings(record=True) as w:
      # Cause all warnings to always be triggered.
      warnings.simplefilter('always')
      db.Add(conflict_fd)
      self.assertTrue(len(w))
      self.assertIs(w[0].category, RuntimeWarning)
      self.assertIn('Conflict register for file "other_file2": ',
                    str(w[0].message))
      self.assertIn(
          'already defined in file '
          '"google/protobuf/unittest.proto"', str(w[0].message))

if __name__ == '__main__':
  unittest.main()
