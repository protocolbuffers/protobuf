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

"""Tests for google.protobuf.symbol_database."""

try:
  import unittest2 as unittest  #PY26
except ImportError:
  import unittest

from google.protobuf import unittest_pb2
from google.protobuf import descriptor
from google.protobuf import descriptor_pool
from google.protobuf import symbol_database


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

  def testGetPrototype(self):
    instance = self._Database().GetPrototype(
        unittest_pb2.TestAllTypes.DESCRIPTOR)
    self.assertTrue(instance is unittest_pb2.TestAllTypes)

  def testGetMessages(self):
    messages = self._Database().GetMessages(
        ['google/protobuf/unittest.proto'])
    self.assertTrue(
        unittest_pb2.TestAllTypes is
        messages['protobuf_unittest.TestAllTypes'])

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
