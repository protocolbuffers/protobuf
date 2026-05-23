# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Tests for google.protobuf.proto_builder."""

import collections
import unittest
from unittest import mock

from google.protobuf import descriptor
from google.protobuf import descriptor_pb2
from google.protobuf import descriptor_pool
from google.protobuf import proto_builder
from google.protobuf import text_format
from google.protobuf.internal import api_implementation


class ProtoBuilderTest(unittest.TestCase):

  def setUp(self):
    super().setUp()

    self.ordered_fields = collections.OrderedDict([
        ('foo', descriptor_pb2.FieldDescriptorProto.TYPE_INT64),
        ('bar', descriptor_pb2.FieldDescriptorProto.TYPE_STRING),
    ])
    self._fields = dict(self.ordered_fields)

  def testMakeSimpleProtoClass(self):
    """Test that we can create a proto class."""
    proto_cls = proto_builder.MakeSimpleProtoClass(
        self._fields,
        full_name='net.proto2.python.public.proto_builder_test.Test',
    )
    proto = proto_cls()
    proto.foo = 12345
    proto.bar = 'asdf'
    self.assertMultiLineEqual(
        'bar: "asdf"\nfoo: 12345\n', text_format.MessageToString(proto)
    )

  def testOrderedFields(self):
    """Test that the field order is maintained when given an OrderedDict."""
    proto_cls = proto_builder.MakeSimpleProtoClass(
        self.ordered_fields,
        full_name='net.proto2.python.public.proto_builder_test.OrderedTest',
    )
    proto = proto_cls()
    proto.foo = 12345
    proto.bar = 'asdf'
    self.assertMultiLineEqual(
        'foo: 12345\nbar: "asdf"\n', text_format.MessageToString(proto)
    )

  def testMakeSameProtoClassTwice(self):
    """Test that the DescriptorPool is used."""
    pool = descriptor_pool.DescriptorPool()
    proto_cls1 = proto_builder.MakeSimpleProtoClass(
        self._fields,
        full_name='net.proto2.python.public.proto_builder_test.Test',
        pool=pool,
    )
    proto_cls2 = proto_builder.MakeSimpleProtoClass(
        self._fields,
        full_name='net.proto2.python.public.proto_builder_test.Test',
        pool=pool,
    )
    self.assertIs(proto_cls1.DESCRIPTOR, proto_cls2.DESCRIPTOR)

  def testMakeSimpleProtoClassUsesNonSecuritySha1(self):
    pool = descriptor_pool.DescriptorPool()
    with mock.patch.object(
        proto_builder.hashlib,
        'sha1',
        wraps=proto_builder.hashlib.sha1,
    ) as sha1:
      proto_builder.MakeSimpleProtoClass(
          self._fields,
          full_name='net.proto2.python.public.proto_builder_test.Sha1Test',
          pool=pool,
      )

    sha1.assert_called_once_with(usedforsecurity=False)

  def testMakeLargeProtoClass(self):
    """Test that large created protos don't use reserved field numbers."""
    num_fields = 65000
    fields = {
        'foo%d' % i: descriptor_pb2.FieldDescriptorProto.TYPE_INT64
        for i in range(num_fields)
    }
    if api_implementation.Type() == 'upb':
      with self.assertRaisesRegex(
          TypeError, "Couldn't build proto file into descriptor pool"
      ):
        proto_cls = proto_builder.MakeSimpleProtoClass(
            fields,
            full_name=(
                'net.proto2.python.public.proto_builder_test.LargeProtoTest'
            ),
        )
      return

    proto_cls = proto_builder.MakeSimpleProtoClass(
        fields,
        full_name='net.proto2.python.public.proto_builder_test.LargeProtoTest',
    )

    reserved_field_numbers = set(
        range(
            descriptor.FieldDescriptor.FIRST_RESERVED_FIELD_NUMBER,
            descriptor.FieldDescriptor.LAST_RESERVED_FIELD_NUMBER + 1,
        )
    )
    proto_field_numbers = set(proto_cls.DESCRIPTOR.fields_by_number)
    self.assertFalse(reserved_field_numbers.intersection(proto_field_numbers))

  def testMakeSimpleProtoClassWithRepeatedNoneFirst(self):
    """Test that we can create a proto class when repeated field list starts with None."""
    fields = {
        'my_repeated': [None, descriptor_pb2.FieldDescriptorProto.TYPE_INT64],
    }
    proto_cls = proto_builder.MakeSimpleProtoClass(
        fields,
        full_name=(
            'net.proto2.python.public.proto_builder_test.RepeatedNoneFirstTest'
        ),
    )
    proto = proto_cls()
    proto.my_repeated.extend([111, 222])
    self.assertMultiLineEqual(
        'my_repeated: 111\nmy_repeated: 222\n',
        text_format.MessageToString(proto),
    )


if __name__ == '__main__':
  unittest.main()
