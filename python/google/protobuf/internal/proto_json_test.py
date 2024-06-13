# -*- coding: utf-8 -*-
# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Tests Nextgen Pythonic protobuf APIs."""

import unittest

from google.protobuf import proto_json
from google.protobuf.util import json_format_proto3_pb2


class ProtoJsonTest(unittest.TestCase):

  def test_simple_serialize(self):
    message = json_format_proto3_pb2.TestMessage()
    message.int32_value = 12345
    expected = {'int32Value': 12345}
    self.assertEqual(expected, proto_json.serialize(message))

  def test_simple_parse(self):
    expected = 12345
    js_dict = {'int32Value': expected}
    message = proto_json.parse(json_format_proto3_pb2.TestMessage,
                               js_dict)
    self.assertEqual(expected, message.int32_value)  # pytype: disable=attribute-error


if __name__ == "__main__":
  unittest.main()
