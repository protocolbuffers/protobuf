# -*- coding: utf-8 -*-
# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Test decoder."""

import io
import unittest

from google.protobuf.internal import decoder
from google.protobuf.internal import testing_refleaks


_INPUT_BYTES = b'\x84r\x12'
_EXPECTED = (14596, 18)


@testing_refleaks.TestCase
class DecoderTest(unittest.TestCase):

  def test_decode_varint_bytes(self):
    (size, pos) = decoder._DecodeVarint(_INPUT_BYTES, 0)
    self.assertEqual(size, _EXPECTED[0])
    self.assertEqual(pos, 2)

    (size, pos) = decoder._DecodeVarint(_INPUT_BYTES, 2)
    self.assertEqual(size, _EXPECTED[1])
    self.assertEqual(pos, 3)

  def test_decode_varint_bytes_empty(self):
    with self.assertRaises(IndexError) as context:
      (size, pos) = decoder._DecodeVarint(b'', 0)
    self.assertIn('index out of range', str(context.exception))

  def test_decode_varint_bytesio(self):
    index = 0
    input_io = io.BytesIO(_INPUT_BYTES)
    while True:
      size = decoder._DecodeVarint(input_io)
      if size is None:
        break
      self.assertEqual(size, _EXPECTED[index])
      index += 1
    self.assertEqual(index, len(_EXPECTED))

  def test_decode_varint_bytesio_empty(self):
    input_io = io.BytesIO(b'')
    size = decoder._DecodeVarint(input_io)
    self.assertEqual(size, None)


if __name__ == '__main__':
  unittest.main()
