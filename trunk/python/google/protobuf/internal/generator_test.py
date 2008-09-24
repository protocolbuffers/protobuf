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
