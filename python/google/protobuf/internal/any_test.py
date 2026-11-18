# -*- coding: utf-8 -*-
# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Tests proto Any APIs."""

import unittest

from google.protobuf import any as proto_any

from google.protobuf import any_pb2
from google.protobuf import unittest_pb2


class AnyTest(unittest.TestCase):

  def test_pack_unpack(self):
    all_types = unittest_pb2.TestAllTypes()
    any_msg = proto_any.pack(all_types)
    all_descriptor = all_types.DESCRIPTOR
    self.assertEqual(
        any_msg.type_url, 'type.googleapis.com/%s' % all_descriptor.full_name
    )

    # Any can be successfully unpacked to the correct message type.
    unpacked_message = unittest_pb2.TestAllTypes()
    self.assertTrue(proto_any.unpack(any_msg, unpacked_message))

    proto_any.unpack_as(any_msg, unittest_pb2.TestAllTypes)

    # Any can't be unpacked to an incorrect message type.
    self.assertFalse(
        proto_any.unpack(any_msg, unittest_pb2.TestAllTypes.NestedMessage())
    )

    with self.assertRaises(TypeError) as catcher:
      proto_any.unpack_as(any_msg, unittest_pb2.TestAllTypes.NestedMessage)
    self.assertIn('Attempted to unpack', catcher.exception.args[0])

  def test_type_name(self):
    all_types = unittest_pb2.TestAllTypes()
    any_msg = proto_any.pack(all_types)
    self.assertEqual(
        proto_any.type_name(any_msg), 'proto2_unittest.TestAllTypes'
    )

  def test_is_type(self):
    all_types = unittest_pb2.TestAllTypes()
    any_msg = proto_any.pack(all_types)
    all_descriptor = all_types.DESCRIPTOR
    self.assertTrue(proto_any.is_type(any_msg, all_descriptor))

    empty_any = any_pb2.Any()
    self.assertFalse(proto_any.is_type(empty_any, all_descriptor))


if __name__ == '__main__':
  unittest.main()
