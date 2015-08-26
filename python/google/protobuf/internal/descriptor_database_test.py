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

"""Tests for google.protobuf.descriptor_database."""

__author__ = 'matthewtoia@google.com (Matt Toia)'

try:
  import unittest2 as unittest
except ImportError:
  import unittest
from google.protobuf import descriptor_pb2
from google.protobuf.internal import factory_test2_pb2
from google.protobuf import descriptor_database


class DescriptorDatabaseTest(unittest.TestCase):

  def testAdd(self):
    db = descriptor_database.DescriptorDatabase()
    file_desc_proto = descriptor_pb2.FileDescriptorProto.FromString(
        factory_test2_pb2.DESCRIPTOR.serialized_pb)
    db.Add(file_desc_proto)

    self.assertEqual(file_desc_proto, db.FindFileByName(
        'google/protobuf/internal/factory_test2.proto'))
    self.assertEqual(file_desc_proto, db.FindFileContainingSymbol(
        'google.protobuf.python.internal.Factory2Message'))
    self.assertEqual(file_desc_proto, db.FindFileContainingSymbol(
        'google.protobuf.python.internal.Factory2Message.NestedFactory2Message'))
    self.assertEqual(file_desc_proto, db.FindFileContainingSymbol(
        'google.protobuf.python.internal.Factory2Enum'))
    self.assertEqual(file_desc_proto, db.FindFileContainingSymbol(
        'google.protobuf.python.internal.Factory2Message.NestedFactory2Enum'))
    self.assertEqual(file_desc_proto, db.FindFileContainingSymbol(
        'google.protobuf.python.internal.MessageWithNestedEnumOnly.NestedEnum'))

if __name__ == '__main__':
  unittest.main()
