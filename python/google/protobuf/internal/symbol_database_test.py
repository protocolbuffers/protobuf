# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Tests for google.protobuf.symbol_database."""

import unittest

from google.protobuf import descriptor
from google.protobuf import descriptor_pool
from google.protobuf import symbol_database
from google.protobuf import unittest_pb2


class SymbolDatabaseTest(unittest.TestCase):

  def _Database(self):
    if descriptor._USE_C_DESCRIPTORS:
      # The C++ implementation does not allow mixing descriptors from
      # different pools.
      db = symbol_database.SymbolDatabase(pool=descriptor_pool.Default())
    else:
      db = symbol_database.SymbolDatabase()
    # Register representative types from unittest_pb2.
    db.RegisterFileDescriptor(unittest_pb2.DESCRIPTOR)
    db.RegisterMessage(unittest_pb2.TestAllTypes)
    db.RegisterMessage(unittest_pb2.TestAllTypes.NestedMessage)
    db.RegisterMessage(unittest_pb2.TestAllTypes.OptionalGroup)
    db.RegisterMessage(unittest_pb2.TestAllTypes.RepeatedGroup)
    db.RegisterEnumDescriptor(unittest_pb2.ForeignEnum.DESCRIPTOR)
    db.RegisterEnumDescriptor(unittest_pb2.TestAllTypes.NestedEnum.DESCRIPTOR)
    db.RegisterServiceDescriptor(unittest_pb2._TESTSERVICE)
    return db

  def testGetSymbol(self):
    self.assertEqual(
        unittest_pb2.TestAllTypes, self._Database().GetSymbol(
            'protobuf_unittest.TestAllTypes'))
    self.assertEqual(
        unittest_pb2.TestAllTypes.NestedMessage, self._Database().GetSymbol(
            'protobuf_unittest.TestAllTypes.NestedMessage'))
    self.assertEqual(
        unittest_pb2.TestAllTypes.OptionalGroup, self._Database().GetSymbol(
            'protobuf_unittest.TestAllTypes.OptionalGroup'))
    self.assertEqual(
        unittest_pb2.TestAllTypes.RepeatedGroup, self._Database().GetSymbol(
            'protobuf_unittest.TestAllTypes.RepeatedGroup'))

  def testEnums(self):
    # Check registration of types in the pool.
    self.assertEqual(
        'protobuf_unittest.ForeignEnum',
        self._Database().pool.FindEnumTypeByName(
            'protobuf_unittest.ForeignEnum').full_name)
    self.assertEqual(
        'protobuf_unittest.TestAllTypes.NestedEnum',
        self._Database().pool.FindEnumTypeByName(
            'protobuf_unittest.TestAllTypes.NestedEnum').full_name)

  def testFindMessageTypeByName(self):
    self.assertEqual(
        'protobuf_unittest.TestAllTypes',
        self._Database().pool.FindMessageTypeByName(
            'protobuf_unittest.TestAllTypes').full_name)
    self.assertEqual(
        'protobuf_unittest.TestAllTypes.NestedMessage',
        self._Database().pool.FindMessageTypeByName(
            'protobuf_unittest.TestAllTypes.NestedMessage').full_name)

  def testFindServiceByName(self):
    self.assertEqual(
        'protobuf_unittest.TestService',
        self._Database().pool.FindServiceByName(
            'protobuf_unittest.TestService').full_name)

  def testFindFileContainingSymbol(self):
    # Lookup based on either enum or message.
    self.assertEqual(
        'google/protobuf/unittest.proto',
        self._Database().pool.FindFileContainingSymbol(
            'protobuf_unittest.TestAllTypes.NestedEnum').name)
    self.assertEqual(
        'google/protobuf/unittest.proto',
        self._Database().pool.FindFileContainingSymbol(
            'protobuf_unittest.TestAllTypes').name)

  def testFindFileByName(self):
    self.assertEqual(
        'google/protobuf/unittest.proto',
        self._Database().pool.FindFileByName(
            'google/protobuf/unittest.proto').name)


if __name__ == '__main__':
  unittest.main()
