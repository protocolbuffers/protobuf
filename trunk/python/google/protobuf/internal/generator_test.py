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

# TODO(robinson): Flesh this out considerably.  We focused on reflection_test.py
# first, since it's testing the subtler code, and since it provides decent
# indirect testing of the protocol compiler output.

"""Unittest that directly tests the output of the pure-Python protocol
compiler.  See //net/proto2/internal/reflection_test.py for a test which
further ensures that we can use Python protocol message objects as we expect.
"""

__author__ = 'robinson@google.com (Will Robinson)'

import unittest
from google.protobuf import unittest_mset_pb2
from google.protobuf import unittest_pb2


class GeneratorTest(unittest.TestCase):

  def testNestedMessageDescriptor(self):
    field_name = 'optional_nested_message'
    proto_type = unittest_pb2.TestAllTypes
    self.assertEqual(
        proto_type.NestedMessage.DESCRIPTOR,
        proto_type.DESCRIPTOR.fields_by_name[field_name].message_type)

  def testEnums(self):
    # We test only module-level enums here.
    # TODO(robinson): Examine descriptors directly to check
    # enum descriptor output.
    self.assertEqual(4, unittest_pb2.FOREIGN_FOO)
    self.assertEqual(5, unittest_pb2.FOREIGN_BAR)
    self.assertEqual(6, unittest_pb2.FOREIGN_BAZ)

    proto = unittest_pb2.TestAllTypes()
    self.assertEqual(1, proto.FOO)
    self.assertEqual(1, unittest_pb2.TestAllTypes.FOO)
    self.assertEqual(2, proto.BAR)
    self.assertEqual(2, unittest_pb2.TestAllTypes.BAR)
    self.assertEqual(3, proto.BAZ)
    self.assertEqual(3, unittest_pb2.TestAllTypes.BAZ)

  def testContainingTypeBehaviorForExtensions(self):
    self.assertEqual(unittest_pb2.optional_int32_extension.containing_type,
                     unittest_pb2.TestAllExtensions.DESCRIPTOR)
    self.assertEqual(unittest_pb2.TestRequired.single.containing_type,
                     unittest_pb2.TestAllExtensions.DESCRIPTOR)

  def testExtensionScope(self):
    self.assertEqual(unittest_pb2.optional_int32_extension.extension_scope,
                     None)
    self.assertEqual(unittest_pb2.TestRequired.single.extension_scope,
                     unittest_pb2.TestRequired.DESCRIPTOR)

  def testIsExtension(self):
    self.assertTrue(unittest_pb2.optional_int32_extension.is_extension)
    self.assertTrue(unittest_pb2.TestRequired.single.is_extension)

    message_descriptor = unittest_pb2.TestRequired.DESCRIPTOR
    non_extension_descriptor = message_descriptor.fields_by_name['a']
    self.assertTrue(not non_extension_descriptor.is_extension)

  def testOptions(self):
    proto = unittest_mset_pb2.TestMessageSet()
    self.assertTrue(proto.DESCRIPTOR.GetOptions().message_set_wire_format)


if __name__ == '__main__':
  unittest.main()
