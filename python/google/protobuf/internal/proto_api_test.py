# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Unittest for proto_api."""

import unittest

from google.protobuf.internal import proto_api_example
from google.protobuf.internal import testing_refleaks

from google.protobuf import unittest_pb2


@testing_refleaks.TestCase
class ProtoApiTest(unittest.TestCase):

  def test_message_mutator_clear(self):
    msg = unittest_pb2.TestAllTypes(
        optional_int32=24, optional_string='optional_string'
    )
    proto_api_example.ClearMessage(msg)
    self.assertNotIn('optional_int32', msg)
    self.assertNotIn('optional_string', msg)

  def test_message_mutator_parse(self):
    msg = unittest_pb2.TestAllTypes(
        optional_int32=24, optional_string='optional_string'
    )
    proto_api_example.ParseMessage('optional_string: "changed"', msg)
    self.assertNotIn('optional_int32', msg)
    self.assertEqual(msg.optional_string, 'changed')


if __name__ == '__main__':
  unittest.main()
