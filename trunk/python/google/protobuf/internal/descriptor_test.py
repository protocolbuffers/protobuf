# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.
# http://code.google.com/p/protobuf/
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

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
