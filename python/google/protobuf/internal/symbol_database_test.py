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

import pytest

from google.protobuf import descriptor
from google.protobuf import descriptor_pool
from google.protobuf import symbol_database
from google.protobuf import unittest_pb2

@pytest.fixture(scope='session')
def database():
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

def test_get_prototype(database):
    instance = database.GetPrototype(unittest_pb2.TestAllTypes.DESCRIPTOR)
    assert instance is unittest_pb2.TestAllTypes

def test_get_messages(database):
    messages = database.GetMessages(['google/protobuf/unittest.proto'])
    assert (unittest_pb2.TestAllTypes is
            messages['protobuf_unittest.TestAllTypes'])

def test_get_symbol(database):
    assert (
        unittest_pb2.TestAllTypes
        == database.GetSymbol('protobuf_unittest.TestAllTypes'))
    assert (
        unittest_pb2.TestAllTypes.NestedMessage
        == database.GetSymbol(
            'protobuf_unittest.TestAllTypes.NestedMessage'))
    assert (
        unittest_pb2.TestAllTypes.OptionalGroup
        == database.GetSymbol(
            'protobuf_unittest.TestAllTypes.OptionalGroup'))
    assert (
        unittest_pb2.TestAllTypes.RepeatedGroup
        == database.GetSymbol(
            'protobuf_unittest.TestAllTypes.RepeatedGroup'))

def test_enums(database):
    # Check registration of types in the pool.
    assert (
        'protobuf_unittest.ForeignEnum'
        == database.pool.FindEnumTypeByName(
            'protobuf_unittest.ForeignEnum').full_name)
    assert (
        'protobuf_unittest.TestAllTypes.NestedEnum'
        == database.pool.FindEnumTypeByName(
            'protobuf_unittest.TestAllTypes.NestedEnum').full_name)

def test_find_message_type_by_name(database):
    assert (
        'protobuf_unittest.TestAllTypes'
        == database.pool.FindMessageTypeByName(
            'protobuf_unittest.TestAllTypes').full_name)
    assert (
        'protobuf_unittest.TestAllTypes.NestedMessage'
        == database.pool.FindMessageTypeByName(
            'protobuf_unittest.TestAllTypes.NestedMessage').full_name)

def test_find_service_by_name(database):
    assert (
        'protobuf_unittest.TestService'
        == database.pool.FindServiceByName(
            'protobuf_unittest.TestService').full_name)

def test_find_file_containing_symbol(database):
    # Lookup based on either enum or message.
    assert (
        'google/protobuf/unittest.proto'
        == database.pool.FindFileContainingSymbol(
            'protobuf_unittest.TestAllTypes.NestedEnum').name)
    assert (
        'google/protobuf/unittest.proto'
        == database.pool.FindFileContainingSymbol(
            'protobuf_unittest.TestAllTypes').name)

def test_find_file_by_name(database):
    assert (
        'google/protobuf/unittest.proto'
        == database.pool.FindFileByName(
            'google/protobuf/unittest.proto').name)
