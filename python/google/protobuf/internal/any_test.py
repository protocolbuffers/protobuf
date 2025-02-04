# -*- coding: utf-8 -*-
# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Tests proto Any APIs."""

import unittest

from google.protobuf import any

from google.protobuf import any_pb2
from google.protobuf import unittest_pb2


class AnyTest(unittest.TestCase):

  def test_pack_unpack(self):
    all_types = unittest_pb2.TestAllTypes()
    any_msg = any.pack(all_types)
    all_descriptor = all_types.DESCRIPTOR
    self.assertEqual(
        any_msg.type_url, 'type.googleapis.com/%s' % all_descriptor.full_name
    )
    unpacked_message = unittest_pb2.TestAllTypes()
    self.assertTrue(any.unpack(any_msg, unpacked_message))

  def test_type_name(self):
    all_types = unittest_pb2.TestAllTypes()
    any_msg = any.pack(all_types)
    self.assertEqual(any.type_name(any_msg), 'proto2_unittest.TestAllTypes')

  def test_is_type(self):
    all_types = unittest_pb2.TestAllTypes()
    any_msg = any.pack(all_types)
    all_descriptor = all_types.DESCRIPTOR
    self.assertTrue(any.is_type(any_msg, all_descriptor))

    empty_any = any_pb2.Any()
    self.assertFalse(any.is_type(empty_any, all_descriptor))


if __name__ == '__main__':
  unittest.main()
