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

"""Tests for google.protobuf.proto_builder."""

import collections
try:
  import unittest2 as unittest
except ImportError:
  import unittest

from google.protobuf import descriptor_pb2  # pylint: disable=g-import-not-at-top
from google.protobuf import descriptor
from google.protobuf import descriptor_pool
from google.protobuf import proto_builder
from google.protobuf import text_format


class ProtoBuilderTest(unittest.TestCase):

  def setUp(self):
    self.ordered_fields = collections.OrderedDict([
        ('foo', descriptor_pb2.FieldDescriptorProto.TYPE_INT64),
        ('bar', descriptor_pb2.FieldDescriptorProto.TYPE_STRING),
        ])
    self._fields = dict(self.ordered_fields)

  def testMakeSimpleProtoClass(self):
    """Test that we can create a proto class."""
    proto_cls = proto_builder.MakeSimpleProtoClass(
        self._fields,
        full_name='net.proto2.python.public.proto_builder_test.Test')
    proto = proto_cls()
    proto.foo = 12345
    proto.bar = 'asdf'
    self.assertMultiLineEqual(
        'bar: "asdf"\nfoo: 12345\n', text_format.MessageToString(proto))

  def testOrderedFields(self):
    """Test that the field order is maintained when given an OrderedDict."""
    proto_cls = proto_builder.MakeSimpleProtoClass(
        self.ordered_fields,
        full_name='net.proto2.python.public.proto_builder_test.OrderedTest')
    proto = proto_cls()
    proto.foo = 12345
    proto.bar = 'asdf'
    self.assertMultiLineEqual(
        'foo: 12345\nbar: "asdf"\n', text_format.MessageToString(proto))

  def testMakeSameProtoClassTwice(self):
    """Test that the DescriptorPool is used."""
    pool = descriptor_pool.DescriptorPool()
    proto_cls1 = proto_builder.MakeSimpleProtoClass(
        self._fields,
        full_name='net.proto2.python.public.proto_builder_test.Test',
        pool=pool)
    proto_cls2 = proto_builder.MakeSimpleProtoClass(
        self._fields,
        full_name='net.proto2.python.public.proto_builder_test.Test',
        pool=pool)
    self.assertIs(proto_cls1.DESCRIPTOR, proto_cls2.DESCRIPTOR)

  def testMakeLargeProtoClass(self):
    """Test that large created protos don't use reserved field numbers."""
    num_fields = 123456
    fields = {
        'foo%d' % i: descriptor_pb2.FieldDescriptorProto.TYPE_INT64
        for i in range(num_fields)
    }
    proto_cls = proto_builder.MakeSimpleProtoClass(
        fields,
        full_name='net.proto2.python.public.proto_builder_test.LargeProtoTest')

    reserved_field_numbers = set(
        range(descriptor.FieldDescriptor.FIRST_RESERVED_FIELD_NUMBER,
              descriptor.FieldDescriptor.LAST_RESERVED_FIELD_NUMBER + 1))
    proto_field_numbers = set(proto_cls.DESCRIPTOR.fields_by_number)
    self.assertFalse(reserved_field_numbers.intersection(proto_field_numbers))


if __name__ == '__main__':
  unittest.main()
