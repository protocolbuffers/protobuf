# -*- coding: utf-8 -*-
# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Test decoder."""

import unittest

from google.protobuf import message
from google.protobuf.internal import decoder
from google.protobuf.internal import testing_refleaks
from google.protobuf.internal import wire_format

@testing_refleaks.TestCase
class DecoderTest(unittest.TestCase):
  def test_decode_unknown_group_field_too_many_levels(self):
    data = memoryview(b'\023' * 5_000_000)
    self.assertRaisesRegex(
        message.DecodeError,
        'Error parsing message',
        decoder._DecodeUnknownField,
        data,
        1,
        wire_format.WIRETYPE_START_GROUP,
        1
    )

if __name__ == '__main__':
  unittest.main()
  