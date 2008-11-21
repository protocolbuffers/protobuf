#! /usr/bin/python
#
# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
# http://code.google.com/p/protobuf/
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

"""Unittest for google.protobuf.internal.descriptor."""

__author__ = 'robinson@google.com (Will Robinson)'

import unittest
from google.protobuf import descriptor_pb2
from google.protobuf import descriptor

class DescriptorTest(unittest.TestCase):

  def setUp(self):
    self.my_enum = descriptor.EnumDescriptor(
        name='ForeignEnum',
        full_name='protobuf_unittest.ForeignEnum',
        filename='ForeignEnum',
        values=[
          descriptor.EnumValueDescriptor(name='FOREIGN_FOO', index=0, number=4),
          descriptor.EnumValueDescriptor(name='FOREIGN_BAR', index=1, number=5),
          descriptor.EnumValueDescriptor(name='FOREIGN_BAZ', index=2, number=6),
        ])
    self.my_message = descriptor.Descriptor(
        name='NestedMessage',
        full_name='protobuf_unittest.TestAllTypes.NestedMessage',
        filename='some/filename/some.proto',
        containing_type=None,
        fields=[
          descriptor.FieldDescriptor(
            name='bb',
            full_name='protobuf_unittest.TestAllTypes.NestedMessage.bb',
            index=0, number=1,
            type=5, cpp_type=1, label=1,
            default_value=0,
            message_type=None, enum_type=None, containing_type=None,
            is_extension=False, extension_scope=None),
        ],
        nested_types=[],
        enum_types=[
          self.my_enum,
        ],
        extensions=[])
    self.my_method = descriptor.MethodDescriptor(
        name='Bar',
        full_name='protobuf_unittest.TestService.Bar',
        index=0,
        containing_service=None,
        input_type=None,
        output_type=None)
    self.my_service = descriptor.ServiceDescriptor(
        name='TestServiceWithOptions',
        full_name='protobuf_unittest.TestServiceWithOptions',
        index=0,
        methods=[
            self.my_method
        ])

  def testEnumFixups(self):
    self.assertEqual(self.my_enum, self.my_enum.values[0].type)

  def testContainingTypeFixups(self):
    self.assertEqual(self.my_message, self.my_message.fields[0].containing_type)
    self.assertEqual(self.my_message, self.my_enum.containing_type)

  def testContainingServiceFixups(self):
    self.assertEqual(self.my_service, self.my_method.containing_service)

  def testGetOptions(self):
    self.assertEqual(self.my_enum.GetOptions(),
                     descriptor_pb2.EnumOptions())
    self.assertEqual(self.my_enum.values[0].GetOptions(),
                     descriptor_pb2.EnumValueOptions())
    self.assertEqual(self.my_message.GetOptions(),
                     descriptor_pb2.MessageOptions())
    self.assertEqual(self.my_message.fields[0].GetOptions(),
                     descriptor_pb2.FieldOptions())
    self.assertEqual(self.my_method.GetOptions(),
                     descriptor_pb2.MethodOptions())
    self.assertEqual(self.my_service.GetOptions(),
                     descriptor_pb2.ServiceOptions())

if __name__ == '__main__':
  unittest.main()
