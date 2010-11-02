#! /usr/bin/python
#
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

"""Tests python protocol buffers against the golden message.

Note that the golden messages exercise every known field type, thus this
test ends up exercising and verifying nearly all of the parsing and
serialization code in the whole library.

TODO(kenton):  Merge with wire_format_test?  It doesn't make a whole lot of
sense to call this a test of the "message" module, which only declares an
abstract interface.
"""

__author__ = 'gps@google.com (Gregory P. Smith)'

import copy
import math
import unittest
from google.protobuf import unittest_import_pb2
from google.protobuf import unittest_pb2
from google.protobuf.internal import test_util

# Python pre-2.6 does not have isinf() or isnan() functions, so we have
# to provide our own.
def isnan(val):
  # NaN is never equal to itself.
  return val != val
def isinf(val):
  # Infinity times zero equals NaN.
  return not isnan(val) and isnan(val * 0)
def IsPosInf(val):
  return isinf(val) and (val > 0)
def IsNegInf(val):
  return isinf(val) and (val < 0)

class MessageTest(unittest.TestCase):

  def testGoldenMessage(self):
    golden_data = test_util.GoldenFile('golden_message').read()
    golden_message = unittest_pb2.TestAllTypes()
    golden_message.ParseFromString(golden_data)
    test_util.ExpectAllFieldsSet(self, golden_message)
    self.assertTrue(golden_message.SerializeToString() == golden_data)
    golden_copy = copy.deepcopy(golden_message)
    self.assertTrue(golden_copy.SerializeToString() == golden_data)

  def testGoldenExtensions(self):
    golden_data = test_util.GoldenFile('golden_message').read()
    golden_message = unittest_pb2.TestAllExtensions()
    golden_message.ParseFromString(golden_data)
    all_set = unittest_pb2.TestAllExtensions()
    test_util.SetAllExtensions(all_set)
    self.assertEquals(all_set, golden_message)
    self.assertTrue(golden_message.SerializeToString() == golden_data)
    golden_copy = copy.deepcopy(golden_message)
    self.assertTrue(golden_copy.SerializeToString() == golden_data)

  def testGoldenPackedMessage(self):
    golden_data = test_util.GoldenFile('golden_packed_fields_message').read()
    golden_message = unittest_pb2.TestPackedTypes()
    golden_message.ParseFromString(golden_data)
    all_set = unittest_pb2.TestPackedTypes()
    test_util.SetAllPackedFields(all_set)
    self.assertEquals(all_set, golden_message)
    self.assertTrue(all_set.SerializeToString() == golden_data)
    golden_copy = copy.deepcopy(golden_message)
    self.assertTrue(golden_copy.SerializeToString() == golden_data)

  def testGoldenPackedExtensions(self):
    golden_data = test_util.GoldenFile('golden_packed_fields_message').read()
    golden_message = unittest_pb2.TestPackedExtensions()
    golden_message.ParseFromString(golden_data)
    all_set = unittest_pb2.TestPackedExtensions()
    test_util.SetAllPackedExtensions(all_set)
    self.assertEquals(all_set, golden_message)
    self.assertTrue(all_set.SerializeToString() == golden_data)
    golden_copy = copy.deepcopy(golden_message)
    self.assertTrue(golden_copy.SerializeToString() == golden_data)

  def testPositiveInfinity(self):
    golden_data = ('\x5D\x00\x00\x80\x7F'
                   '\x61\x00\x00\x00\x00\x00\x00\xF0\x7F'
                   '\xCD\x02\x00\x00\x80\x7F'
                   '\xD1\x02\x00\x00\x00\x00\x00\x00\xF0\x7F')
    golden_message = unittest_pb2.TestAllTypes()
    golden_message.ParseFromString(golden_data)
    self.assertTrue(IsPosInf(golden_message.optional_float))
    self.assertTrue(IsPosInf(golden_message.optional_double))
    self.assertTrue(IsPosInf(golden_message.repeated_float[0]))
    self.assertTrue(IsPosInf(golden_message.repeated_double[0]))
    self.assertTrue(golden_message.SerializeToString() == golden_data)

  def testNegativeInfinity(self):
    golden_data = ('\x5D\x00\x00\x80\xFF'
                   '\x61\x00\x00\x00\x00\x00\x00\xF0\xFF'
                   '\xCD\x02\x00\x00\x80\xFF'
                   '\xD1\x02\x00\x00\x00\x00\x00\x00\xF0\xFF')
    golden_message = unittest_pb2.TestAllTypes()
    golden_message.ParseFromString(golden_data)
    self.assertTrue(IsNegInf(golden_message.optional_float))
    self.assertTrue(IsNegInf(golden_message.optional_double))
    self.assertTrue(IsNegInf(golden_message.repeated_float[0]))
    self.assertTrue(IsNegInf(golden_message.repeated_double[0]))
    self.assertTrue(golden_message.SerializeToString() == golden_data)

  def testNotANumber(self):
    golden_data = ('\x5D\x00\x00\xC0\x7F'
                   '\x61\x00\x00\x00\x00\x00\x00\xF8\x7F'
                   '\xCD\x02\x00\x00\xC0\x7F'
                   '\xD1\x02\x00\x00\x00\x00\x00\x00\xF8\x7F')
    golden_message = unittest_pb2.TestAllTypes()
    golden_message.ParseFromString(golden_data)
    self.assertTrue(isnan(golden_message.optional_float))
    self.assertTrue(isnan(golden_message.optional_double))
    self.assertTrue(isnan(golden_message.repeated_float[0]))
    self.assertTrue(isnan(golden_message.repeated_double[0]))
    self.assertTrue(golden_message.SerializeToString() == golden_data)

  def testPositiveInfinityPacked(self):
    golden_data = ('\xA2\x06\x04\x00\x00\x80\x7F'
                   '\xAA\x06\x08\x00\x00\x00\x00\x00\x00\xF0\x7F')
    golden_message = unittest_pb2.TestPackedTypes()
    golden_message.ParseFromString(golden_data)
    self.assertTrue(IsPosInf(golden_message.packed_float[0]))
    self.assertTrue(IsPosInf(golden_message.packed_double[0]))
    self.assertTrue(golden_message.SerializeToString() == golden_data)

  def testNegativeInfinityPacked(self):
    golden_data = ('\xA2\x06\x04\x00\x00\x80\xFF'
                   '\xAA\x06\x08\x00\x00\x00\x00\x00\x00\xF0\xFF')
    golden_message = unittest_pb2.TestPackedTypes()
    golden_message.ParseFromString(golden_data)
    self.assertTrue(IsNegInf(golden_message.packed_float[0]))
    self.assertTrue(IsNegInf(golden_message.packed_double[0]))
    self.assertTrue(golden_message.SerializeToString() == golden_data)

  def testNotANumberPacked(self):
    golden_data = ('\xA2\x06\x04\x00\x00\xC0\x7F'
                   '\xAA\x06\x08\x00\x00\x00\x00\x00\x00\xF8\x7F')
    golden_message = unittest_pb2.TestPackedTypes()
    golden_message.ParseFromString(golden_data)
    self.assertTrue(isnan(golden_message.packed_float[0]))
    self.assertTrue(isnan(golden_message.packed_double[0]))
    self.assertTrue(golden_message.SerializeToString() == golden_data)

  def testExtremeFloatValues(self):
    message = unittest_pb2.TestAllTypes()

    # Most positive exponent, no significand bits set.
    kMostPosExponentNoSigBits = math.pow(2, 127)
    message.optional_float = kMostPosExponentNoSigBits
    message.ParseFromString(message.SerializeToString())
    self.assertTrue(message.optional_float == kMostPosExponentNoSigBits)

    # Most positive exponent, one significand bit set.
    kMostPosExponentOneSigBit = 1.5 * math.pow(2, 127)
    message.optional_float = kMostPosExponentOneSigBit
    message.ParseFromString(message.SerializeToString())
    self.assertTrue(message.optional_float == kMostPosExponentOneSigBit)

    # Repeat last two cases with values of same magnitude, but negative.
    message.optional_float = -kMostPosExponentNoSigBits
    message.ParseFromString(message.SerializeToString())
    self.assertTrue(message.optional_float == -kMostPosExponentNoSigBits)

    message.optional_float = -kMostPosExponentOneSigBit
    message.ParseFromString(message.SerializeToString())
    self.assertTrue(message.optional_float == -kMostPosExponentOneSigBit)

    # Most negative exponent, no significand bits set.
    kMostNegExponentNoSigBits = math.pow(2, -127)
    message.optional_float = kMostNegExponentNoSigBits
    message.ParseFromString(message.SerializeToString())
    self.assertTrue(message.optional_float == kMostNegExponentNoSigBits)

    # Most negative exponent, one significand bit set.
    kMostNegExponentOneSigBit = 1.5 * math.pow(2, -127)
    message.optional_float = kMostNegExponentOneSigBit
    message.ParseFromString(message.SerializeToString())
    self.assertTrue(message.optional_float == kMostNegExponentOneSigBit)

    # Repeat last two cases with values of the same magnitude, but negative.
    message.optional_float = -kMostNegExponentNoSigBits
    message.ParseFromString(message.SerializeToString())
    self.assertTrue(message.optional_float == -kMostNegExponentNoSigBits)

    message.optional_float = -kMostNegExponentOneSigBit
    message.ParseFromString(message.SerializeToString())
    self.assertTrue(message.optional_float == -kMostNegExponentOneSigBit)

  def testExtremeFloatValues(self):
    message = unittest_pb2.TestAllTypes()

    # Most positive exponent, no significand bits set.
    kMostPosExponentNoSigBits = math.pow(2, 1023)
    message.optional_double = kMostPosExponentNoSigBits
    message.ParseFromString(message.SerializeToString())
    self.assertTrue(message.optional_double == kMostPosExponentNoSigBits)

    # Most positive exponent, one significand bit set.
    kMostPosExponentOneSigBit = 1.5 * math.pow(2, 1023)
    message.optional_double = kMostPosExponentOneSigBit
    message.ParseFromString(message.SerializeToString())
    self.assertTrue(message.optional_double == kMostPosExponentOneSigBit)

    # Repeat last two cases with values of same magnitude, but negative.
    message.optional_double = -kMostPosExponentNoSigBits
    message.ParseFromString(message.SerializeToString())
    self.assertTrue(message.optional_double == -kMostPosExponentNoSigBits)

    message.optional_double = -kMostPosExponentOneSigBit
    message.ParseFromString(message.SerializeToString())
    self.assertTrue(message.optional_double == -kMostPosExponentOneSigBit)

    # Most negative exponent, no significand bits set.
    kMostNegExponentNoSigBits = math.pow(2, -1023)
    message.optional_double = kMostNegExponentNoSigBits
    message.ParseFromString(message.SerializeToString())
    self.assertTrue(message.optional_double == kMostNegExponentNoSigBits)

    # Most negative exponent, one significand bit set.
    kMostNegExponentOneSigBit = 1.5 * math.pow(2, -1023)
    message.optional_double = kMostNegExponentOneSigBit
    message.ParseFromString(message.SerializeToString())
    self.assertTrue(message.optional_double == kMostNegExponentOneSigBit)

    # Repeat last two cases with values of the same magnitude, but negative.
    message.optional_double = -kMostNegExponentNoSigBits
    message.ParseFromString(message.SerializeToString())
    self.assertTrue(message.optional_double == -kMostNegExponentNoSigBits)

    message.optional_double = -kMostNegExponentOneSigBit
    message.ParseFromString(message.SerializeToString())
    self.assertTrue(message.optional_double == -kMostNegExponentOneSigBit)

  def testSortingRepeatedScalarFieldsDefaultComparator(self):
    """Check some different types with the default comparator."""
    message = unittest_pb2.TestAllTypes()

    # TODO(mattp): would testing more scalar types strengthen test?
    message.repeated_int32.append(1)
    message.repeated_int32.append(3)
    message.repeated_int32.append(2)
    message.repeated_int32.sort()
    self.assertEqual(message.repeated_int32[0], 1)
    self.assertEqual(message.repeated_int32[1], 2)
    self.assertEqual(message.repeated_int32[2], 3)

    message.repeated_float.append(1.1)
    message.repeated_float.append(1.3)
    message.repeated_float.append(1.2)
    message.repeated_float.sort()
    self.assertAlmostEqual(message.repeated_float[0], 1.1)
    self.assertAlmostEqual(message.repeated_float[1], 1.2)
    self.assertAlmostEqual(message.repeated_float[2], 1.3)

    message.repeated_string.append('a')
    message.repeated_string.append('c')
    message.repeated_string.append('b')
    message.repeated_string.sort()
    self.assertEqual(message.repeated_string[0], 'a')
    self.assertEqual(message.repeated_string[1], 'b')
    self.assertEqual(message.repeated_string[2], 'c')

    message.repeated_bytes.append('a')
    message.repeated_bytes.append('c')
    message.repeated_bytes.append('b')
    message.repeated_bytes.sort()
    self.assertEqual(message.repeated_bytes[0], 'a')
    self.assertEqual(message.repeated_bytes[1], 'b')
    self.assertEqual(message.repeated_bytes[2], 'c')

  def testSortingRepeatedScalarFieldsCustomComparator(self):
    """Check some different types with custom comparator."""
    message = unittest_pb2.TestAllTypes()

    message.repeated_int32.append(-3)
    message.repeated_int32.append(-2)
    message.repeated_int32.append(-1)
    message.repeated_int32.sort(lambda x,y: cmp(abs(x), abs(y)))
    self.assertEqual(message.repeated_int32[0], -1)
    self.assertEqual(message.repeated_int32[1], -2)
    self.assertEqual(message.repeated_int32[2], -3)

    message.repeated_string.append('aaa')
    message.repeated_string.append('bb')
    message.repeated_string.append('c')
    message.repeated_string.sort(lambda x,y: cmp(len(x), len(y)))
    self.assertEqual(message.repeated_string[0], 'c')
    self.assertEqual(message.repeated_string[1], 'bb')
    self.assertEqual(message.repeated_string[2], 'aaa')

  def testSortingRepeatedCompositeFieldsCustomComparator(self):
    """Check passing a custom comparator to sort a repeated composite field."""
    message = unittest_pb2.TestAllTypes()

    message.repeated_nested_message.add().bb = 1
    message.repeated_nested_message.add().bb = 3
    message.repeated_nested_message.add().bb = 2
    message.repeated_nested_message.add().bb = 6
    message.repeated_nested_message.add().bb = 5
    message.repeated_nested_message.add().bb = 4
    message.repeated_nested_message.sort(lambda x,y: cmp(x.bb, y.bb))
    self.assertEqual(message.repeated_nested_message[0].bb, 1)
    self.assertEqual(message.repeated_nested_message[1].bb, 2)
    self.assertEqual(message.repeated_nested_message[2].bb, 3)
    self.assertEqual(message.repeated_nested_message[3].bb, 4)
    self.assertEqual(message.repeated_nested_message[4].bb, 5)
    self.assertEqual(message.repeated_nested_message[5].bb, 6)


if __name__ == '__main__':
  unittest.main()
