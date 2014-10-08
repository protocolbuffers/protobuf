#! /usr/bin/python
#
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
import operator
import pickle
import sys

from google.apputils import basetest
from google.protobuf import unittest_pb2
from google.protobuf.internal import api_implementation
from google.protobuf.internal import test_util
from google.protobuf import message

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


class MessageTest(basetest.TestCase):

  def testBadUtf8String(self):
    if api_implementation.Type() != 'python':
      self.skipTest("Skipping testBadUtf8String, currently only the python "
                    "api implementation raises UnicodeDecodeError when a "
                    "string field contains bad utf-8.")
    bad_utf8_data = test_util.GoldenFileData('bad_utf8_string')
    with self.assertRaises(UnicodeDecodeError) as context:
      unittest_pb2.TestAllTypes.FromString(bad_utf8_data)
    self.assertIn('field: protobuf_unittest.TestAllTypes.optional_string',
                  str(context.exception))

  def testGoldenMessage(self):
    golden_data = test_util.GoldenFileData(
        'golden_message_oneof_implemented')
    golden_message = unittest_pb2.TestAllTypes()
    golden_message.ParseFromString(golden_data)
    test_util.ExpectAllFieldsSet(self, golden_message)
    self.assertEqual(golden_data, golden_message.SerializeToString())
    golden_copy = copy.deepcopy(golden_message)
    self.assertEqual(golden_data, golden_copy.SerializeToString())

  def testGoldenExtensions(self):
    golden_data = test_util.GoldenFileData('golden_message')
    golden_message = unittest_pb2.TestAllExtensions()
    golden_message.ParseFromString(golden_data)
    all_set = unittest_pb2.TestAllExtensions()
    test_util.SetAllExtensions(all_set)
    self.assertEquals(all_set, golden_message)
    self.assertEqual(golden_data, golden_message.SerializeToString())
    golden_copy = copy.deepcopy(golden_message)
    self.assertEqual(golden_data, golden_copy.SerializeToString())

  def testGoldenPackedMessage(self):
    golden_data = test_util.GoldenFileData('golden_packed_fields_message')
    golden_message = unittest_pb2.TestPackedTypes()
    golden_message.ParseFromString(golden_data)
    all_set = unittest_pb2.TestPackedTypes()
    test_util.SetAllPackedFields(all_set)
    self.assertEquals(all_set, golden_message)
    self.assertEqual(golden_data, all_set.SerializeToString())
    golden_copy = copy.deepcopy(golden_message)
    self.assertEqual(golden_data, golden_copy.SerializeToString())

  def testGoldenPackedExtensions(self):
    golden_data = test_util.GoldenFileData('golden_packed_fields_message')
    golden_message = unittest_pb2.TestPackedExtensions()
    golden_message.ParseFromString(golden_data)
    all_set = unittest_pb2.TestPackedExtensions()
    test_util.SetAllPackedExtensions(all_set)
    self.assertEquals(all_set, golden_message)
    self.assertEqual(golden_data, all_set.SerializeToString())
    golden_copy = copy.deepcopy(golden_message)
    self.assertEqual(golden_data, golden_copy.SerializeToString())

  def testPickleSupport(self):
    golden_data = test_util.GoldenFileData('golden_message')
    golden_message = unittest_pb2.TestAllTypes()
    golden_message.ParseFromString(golden_data)
    pickled_message = pickle.dumps(golden_message)

    unpickled_message = pickle.loads(pickled_message)
    self.assertEquals(unpickled_message, golden_message)


  def testPickleIncompleteProto(self):
    golden_message = unittest_pb2.TestRequired(a=1)
    pickled_message = pickle.dumps(golden_message)

    unpickled_message = pickle.loads(pickled_message)
    self.assertEquals(unpickled_message, golden_message)
    self.assertEquals(unpickled_message.a, 1)
    # This is still an incomplete proto - so serializing should fail
    self.assertRaises(message.EncodeError, unpickled_message.SerializeToString)

  def testPositiveInfinity(self):
    golden_data = (b'\x5D\x00\x00\x80\x7F'
                   b'\x61\x00\x00\x00\x00\x00\x00\xF0\x7F'
                   b'\xCD\x02\x00\x00\x80\x7F'
                   b'\xD1\x02\x00\x00\x00\x00\x00\x00\xF0\x7F')
    golden_message = unittest_pb2.TestAllTypes()
    golden_message.ParseFromString(golden_data)
    self.assertTrue(IsPosInf(golden_message.optional_float))
    self.assertTrue(IsPosInf(golden_message.optional_double))
    self.assertTrue(IsPosInf(golden_message.repeated_float[0]))
    self.assertTrue(IsPosInf(golden_message.repeated_double[0]))
    self.assertEqual(golden_data, golden_message.SerializeToString())

  def testNegativeInfinity(self):
    golden_data = (b'\x5D\x00\x00\x80\xFF'
                   b'\x61\x00\x00\x00\x00\x00\x00\xF0\xFF'
                   b'\xCD\x02\x00\x00\x80\xFF'
                   b'\xD1\x02\x00\x00\x00\x00\x00\x00\xF0\xFF')
    golden_message = unittest_pb2.TestAllTypes()
    golden_message.ParseFromString(golden_data)
    self.assertTrue(IsNegInf(golden_message.optional_float))
    self.assertTrue(IsNegInf(golden_message.optional_double))
    self.assertTrue(IsNegInf(golden_message.repeated_float[0]))
    self.assertTrue(IsNegInf(golden_message.repeated_double[0]))
    self.assertEqual(golden_data, golden_message.SerializeToString())

  def testNotANumber(self):
    golden_data = (b'\x5D\x00\x00\xC0\x7F'
                   b'\x61\x00\x00\x00\x00\x00\x00\xF8\x7F'
                   b'\xCD\x02\x00\x00\xC0\x7F'
                   b'\xD1\x02\x00\x00\x00\x00\x00\x00\xF8\x7F')
    golden_message = unittest_pb2.TestAllTypes()
    golden_message.ParseFromString(golden_data)
    self.assertTrue(isnan(golden_message.optional_float))
    self.assertTrue(isnan(golden_message.optional_double))
    self.assertTrue(isnan(golden_message.repeated_float[0]))
    self.assertTrue(isnan(golden_message.repeated_double[0]))

    # The protocol buffer may serialize to any one of multiple different
    # representations of a NaN.  Rather than verify a specific representation,
    # verify the serialized string can be converted into a correctly
    # behaving protocol buffer.
    serialized = golden_message.SerializeToString()
    message = unittest_pb2.TestAllTypes()
    message.ParseFromString(serialized)
    self.assertTrue(isnan(message.optional_float))
    self.assertTrue(isnan(message.optional_double))
    self.assertTrue(isnan(message.repeated_float[0]))
    self.assertTrue(isnan(message.repeated_double[0]))

  def testPositiveInfinityPacked(self):
    golden_data = (b'\xA2\x06\x04\x00\x00\x80\x7F'
                   b'\xAA\x06\x08\x00\x00\x00\x00\x00\x00\xF0\x7F')
    golden_message = unittest_pb2.TestPackedTypes()
    golden_message.ParseFromString(golden_data)
    self.assertTrue(IsPosInf(golden_message.packed_float[0]))
    self.assertTrue(IsPosInf(golden_message.packed_double[0]))
    self.assertEqual(golden_data, golden_message.SerializeToString())

  def testNegativeInfinityPacked(self):
    golden_data = (b'\xA2\x06\x04\x00\x00\x80\xFF'
                   b'\xAA\x06\x08\x00\x00\x00\x00\x00\x00\xF0\xFF')
    golden_message = unittest_pb2.TestPackedTypes()
    golden_message.ParseFromString(golden_data)
    self.assertTrue(IsNegInf(golden_message.packed_float[0]))
    self.assertTrue(IsNegInf(golden_message.packed_double[0]))
    self.assertEqual(golden_data, golden_message.SerializeToString())

  def testNotANumberPacked(self):
    golden_data = (b'\xA2\x06\x04\x00\x00\xC0\x7F'
                   b'\xAA\x06\x08\x00\x00\x00\x00\x00\x00\xF8\x7F')
    golden_message = unittest_pb2.TestPackedTypes()
    golden_message.ParseFromString(golden_data)
    self.assertTrue(isnan(golden_message.packed_float[0]))
    self.assertTrue(isnan(golden_message.packed_double[0]))

    serialized = golden_message.SerializeToString()
    message = unittest_pb2.TestPackedTypes()
    message.ParseFromString(serialized)
    self.assertTrue(isnan(message.packed_float[0]))
    self.assertTrue(isnan(message.packed_double[0]))

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

  def testExtremeDoubleValues(self):
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

  def testFloatPrinting(self):
    message = unittest_pb2.TestAllTypes()
    message.optional_float = 2.0
    self.assertEqual(str(message), 'optional_float: 2.0\n')

  def testHighPrecisionFloatPrinting(self):
    message = unittest_pb2.TestAllTypes()
    message.optional_double = 0.12345678912345678
    if sys.version_info.major >= 3:
      self.assertEqual(str(message), 'optional_double: 0.12345678912345678\n')
    else:
      self.assertEqual(str(message), 'optional_double: 0.123456789123\n')

  def testUnknownFieldPrinting(self):
    populated = unittest_pb2.TestAllTypes()
    test_util.SetAllNonLazyFields(populated)
    empty = unittest_pb2.TestEmptyMessage()
    empty.ParseFromString(populated.SerializeToString())
    self.assertEqual(str(empty), '')

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

    message.repeated_bytes.append(b'a')
    message.repeated_bytes.append(b'c')
    message.repeated_bytes.append(b'b')
    message.repeated_bytes.sort()
    self.assertEqual(message.repeated_bytes[0], b'a')
    self.assertEqual(message.repeated_bytes[1], b'b')
    self.assertEqual(message.repeated_bytes[2], b'c')

  def testSortingRepeatedScalarFieldsCustomComparator(self):
    """Check some different types with custom comparator."""
    message = unittest_pb2.TestAllTypes()

    message.repeated_int32.append(-3)
    message.repeated_int32.append(-2)
    message.repeated_int32.append(-1)
    message.repeated_int32.sort(key=abs)
    self.assertEqual(message.repeated_int32[0], -1)
    self.assertEqual(message.repeated_int32[1], -2)
    self.assertEqual(message.repeated_int32[2], -3)

    message.repeated_string.append('aaa')
    message.repeated_string.append('bb')
    message.repeated_string.append('c')
    message.repeated_string.sort(key=len)
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
    message.repeated_nested_message.sort(key=operator.attrgetter('bb'))
    self.assertEqual(message.repeated_nested_message[0].bb, 1)
    self.assertEqual(message.repeated_nested_message[1].bb, 2)
    self.assertEqual(message.repeated_nested_message[2].bb, 3)
    self.assertEqual(message.repeated_nested_message[3].bb, 4)
    self.assertEqual(message.repeated_nested_message[4].bb, 5)
    self.assertEqual(message.repeated_nested_message[5].bb, 6)

  def testRepeatedCompositeFieldSortArguments(self):
    """Check sorting a repeated composite field using list.sort() arguments."""
    message = unittest_pb2.TestAllTypes()

    get_bb = operator.attrgetter('bb')
    cmp_bb = lambda a, b: cmp(a.bb, b.bb)
    message.repeated_nested_message.add().bb = 1
    message.repeated_nested_message.add().bb = 3
    message.repeated_nested_message.add().bb = 2
    message.repeated_nested_message.add().bb = 6
    message.repeated_nested_message.add().bb = 5
    message.repeated_nested_message.add().bb = 4
    message.repeated_nested_message.sort(key=get_bb)
    self.assertEqual([k.bb for k in message.repeated_nested_message],
                     [1, 2, 3, 4, 5, 6])
    message.repeated_nested_message.sort(key=get_bb, reverse=True)
    self.assertEqual([k.bb for k in message.repeated_nested_message],
                     [6, 5, 4, 3, 2, 1])
    if sys.version_info.major >= 3: return  # No cmp sorting in PY3.
    message.repeated_nested_message.sort(sort_function=cmp_bb)
    self.assertEqual([k.bb for k in message.repeated_nested_message],
                     [1, 2, 3, 4, 5, 6])
    message.repeated_nested_message.sort(cmp=cmp_bb, reverse=True)
    self.assertEqual([k.bb for k in message.repeated_nested_message],
                     [6, 5, 4, 3, 2, 1])

  def testRepeatedScalarFieldSortArguments(self):
    """Check sorting a scalar field using list.sort() arguments."""
    message = unittest_pb2.TestAllTypes()

    message.repeated_int32.append(-3)
    message.repeated_int32.append(-2)
    message.repeated_int32.append(-1)
    message.repeated_int32.sort(key=abs)
    self.assertEqual(list(message.repeated_int32), [-1, -2, -3])
    message.repeated_int32.sort(key=abs, reverse=True)
    self.assertEqual(list(message.repeated_int32), [-3, -2, -1])
    if sys.version_info.major < 3:  # No cmp sorting in PY3.
      abs_cmp = lambda a, b: cmp(abs(a), abs(b))
      message.repeated_int32.sort(sort_function=abs_cmp)
      self.assertEqual(list(message.repeated_int32), [-1, -2, -3])
      message.repeated_int32.sort(cmp=abs_cmp, reverse=True)
      self.assertEqual(list(message.repeated_int32), [-3, -2, -1])

    message.repeated_string.append('aaa')
    message.repeated_string.append('bb')
    message.repeated_string.append('c')
    message.repeated_string.sort(key=len)
    self.assertEqual(list(message.repeated_string), ['c', 'bb', 'aaa'])
    message.repeated_string.sort(key=len, reverse=True)
    self.assertEqual(list(message.repeated_string), ['aaa', 'bb', 'c'])
    if sys.version_info.major < 3:  # No cmp sorting in PY3.
      len_cmp = lambda a, b: cmp(len(a), len(b))
      message.repeated_string.sort(sort_function=len_cmp)
      self.assertEqual(list(message.repeated_string), ['c', 'bb', 'aaa'])
      message.repeated_string.sort(cmp=len_cmp, reverse=True)
      self.assertEqual(list(message.repeated_string), ['aaa', 'bb', 'c'])

  def testRepeatedFieldsComparable(self):
    m1 = unittest_pb2.TestAllTypes()
    m2 = unittest_pb2.TestAllTypes()
    m1.repeated_int32.append(0)
    m1.repeated_int32.append(1)
    m1.repeated_int32.append(2)
    m2.repeated_int32.append(0)
    m2.repeated_int32.append(1)
    m2.repeated_int32.append(2)
    m1.repeated_nested_message.add().bb = 1
    m1.repeated_nested_message.add().bb = 2
    m1.repeated_nested_message.add().bb = 3
    m2.repeated_nested_message.add().bb = 1
    m2.repeated_nested_message.add().bb = 2
    m2.repeated_nested_message.add().bb = 3

    if sys.version_info.major >= 3: return  # No cmp() in PY3.

    # These comparisons should not raise errors.
    _ = m1 < m2
    _ = m1.repeated_nested_message < m2.repeated_nested_message

    # Make sure cmp always works. If it wasn't defined, these would be
    # id() comparisons and would all fail.
    self.assertEqual(cmp(m1, m2), 0)
    self.assertEqual(cmp(m1.repeated_int32, m2.repeated_int32), 0)
    self.assertEqual(cmp(m1.repeated_int32, [0, 1, 2]), 0)
    self.assertEqual(cmp(m1.repeated_nested_message,
                         m2.repeated_nested_message), 0)
    with self.assertRaises(TypeError):
      # Can't compare repeated composite containers to lists.
      cmp(m1.repeated_nested_message, m2.repeated_nested_message[:])

    # TODO(anuraag): Implement extensiondict comparison in C++ and then add test

  def testParsingMerge(self):
    """Check the merge behavior when a required or optional field appears
    multiple times in the input."""
    messages = [
        unittest_pb2.TestAllTypes(),
        unittest_pb2.TestAllTypes(),
        unittest_pb2.TestAllTypes() ]
    messages[0].optional_int32 = 1
    messages[1].optional_int64 = 2
    messages[2].optional_int32 = 3
    messages[2].optional_string = 'hello'

    merged_message = unittest_pb2.TestAllTypes()
    merged_message.optional_int32 = 3
    merged_message.optional_int64 = 2
    merged_message.optional_string = 'hello'

    generator = unittest_pb2.TestParsingMerge.RepeatedFieldsGenerator()
    generator.field1.extend(messages)
    generator.field2.extend(messages)
    generator.field3.extend(messages)
    generator.ext1.extend(messages)
    generator.ext2.extend(messages)
    generator.group1.add().field1.MergeFrom(messages[0])
    generator.group1.add().field1.MergeFrom(messages[1])
    generator.group1.add().field1.MergeFrom(messages[2])
    generator.group2.add().field1.MergeFrom(messages[0])
    generator.group2.add().field1.MergeFrom(messages[1])
    generator.group2.add().field1.MergeFrom(messages[2])

    data = generator.SerializeToString()
    parsing_merge = unittest_pb2.TestParsingMerge()
    parsing_merge.ParseFromString(data)

    # Required and optional fields should be merged.
    self.assertEqual(parsing_merge.required_all_types, merged_message)
    self.assertEqual(parsing_merge.optional_all_types, merged_message)
    self.assertEqual(parsing_merge.optionalgroup.optional_group_all_types,
                     merged_message)
    self.assertEqual(parsing_merge.Extensions[
                     unittest_pb2.TestParsingMerge.optional_ext],
                     merged_message)

    # Repeated fields should not be merged.
    self.assertEqual(len(parsing_merge.repeated_all_types), 3)
    self.assertEqual(len(parsing_merge.repeatedgroup), 3)
    self.assertEqual(len(parsing_merge.Extensions[
        unittest_pb2.TestParsingMerge.repeated_ext]), 3)

  def ensureNestedMessageExists(self, msg, attribute):
    """Make sure that a nested message object exists.

    As soon as a nested message attribute is accessed, it will be present in the
    _fields dict, without being marked as actually being set.
    """
    getattr(msg, attribute)
    self.assertFalse(msg.HasField(attribute))

  def testOneofGetCaseNonexistingField(self):
    m = unittest_pb2.TestAllTypes()
    self.assertRaises(ValueError, m.WhichOneof, 'no_such_oneof_field')

  def testOneofSemantics(self):
    m = unittest_pb2.TestAllTypes()
    self.assertIs(None, m.WhichOneof('oneof_field'))

    m.oneof_uint32 = 11
    self.assertEqual('oneof_uint32', m.WhichOneof('oneof_field'))
    self.assertTrue(m.HasField('oneof_uint32'))

    m.oneof_string = u'foo'
    self.assertEqual('oneof_string', m.WhichOneof('oneof_field'))
    self.assertFalse(m.HasField('oneof_uint32'))
    self.assertTrue(m.HasField('oneof_string'))

    m.oneof_nested_message.bb = 11
    self.assertEqual('oneof_nested_message', m.WhichOneof('oneof_field'))
    self.assertFalse(m.HasField('oneof_string'))
    self.assertTrue(m.HasField('oneof_nested_message'))

    m.oneof_bytes = b'bb'
    self.assertEqual('oneof_bytes', m.WhichOneof('oneof_field'))
    self.assertFalse(m.HasField('oneof_nested_message'))
    self.assertTrue(m.HasField('oneof_bytes'))

  def testOneofCompositeFieldReadAccess(self):
    m = unittest_pb2.TestAllTypes()
    m.oneof_uint32 = 11

    self.ensureNestedMessageExists(m, 'oneof_nested_message')
    self.assertEqual('oneof_uint32', m.WhichOneof('oneof_field'))
    self.assertEqual(11, m.oneof_uint32)

  def testOneofHasField(self):
    m = unittest_pb2.TestAllTypes()
    self.assertFalse(m.HasField('oneof_field'))
    m.oneof_uint32 = 11
    self.assertTrue(m.HasField('oneof_field'))
    m.oneof_bytes = b'bb'
    self.assertTrue(m.HasField('oneof_field'))
    m.ClearField('oneof_bytes')
    self.assertFalse(m.HasField('oneof_field'))

  def testOneofClearField(self):
    m = unittest_pb2.TestAllTypes()
    m.oneof_uint32 = 11
    m.ClearField('oneof_field')
    self.assertFalse(m.HasField('oneof_field'))
    self.assertFalse(m.HasField('oneof_uint32'))
    self.assertIs(None, m.WhichOneof('oneof_field'))

  def testOneofClearSetField(self):
    m = unittest_pb2.TestAllTypes()
    m.oneof_uint32 = 11
    m.ClearField('oneof_uint32')
    self.assertFalse(m.HasField('oneof_field'))
    self.assertFalse(m.HasField('oneof_uint32'))
    self.assertIs(None, m.WhichOneof('oneof_field'))

  def testOneofClearUnsetField(self):
    m = unittest_pb2.TestAllTypes()
    m.oneof_uint32 = 11
    self.ensureNestedMessageExists(m, 'oneof_nested_message')
    m.ClearField('oneof_nested_message')
    self.assertEqual(11, m.oneof_uint32)
    self.assertTrue(m.HasField('oneof_field'))
    self.assertTrue(m.HasField('oneof_uint32'))
    self.assertEqual('oneof_uint32', m.WhichOneof('oneof_field'))

  def testOneofDeserialize(self):
    m = unittest_pb2.TestAllTypes()
    m.oneof_uint32 = 11
    m2 = unittest_pb2.TestAllTypes()
    m2.ParseFromString(m.SerializeToString())
    self.assertEqual('oneof_uint32', m2.WhichOneof('oneof_field'))

  def testSortEmptyRepeatedCompositeContainer(self):
    """Exercise a scenario that has led to segfaults in the past.
    """
    m = unittest_pb2.TestAllTypes()
    m.repeated_nested_message.sort()

  def testHasFieldOnRepeatedField(self):
    """Using HasField on a repeated field should raise an exception.
    """
    m = unittest_pb2.TestAllTypes()
    with self.assertRaises(ValueError) as _:
      m.HasField('repeated_int32')


class ValidTypeNamesTest(basetest.TestCase):

  def assertImportFromName(self, msg, base_name):
    # Parse <type 'module.class_name'> to extra 'some.name' as a string.
    tp_name = str(type(msg)).split("'")[1]
    valid_names = ('Repeated%sContainer' % base_name,
                   'Repeated%sFieldContainer' % base_name)
    self.assertTrue(any(tp_name.endswith(v) for v in valid_names),
                    '%r does end with any of %r' % (tp_name, valid_names))

    parts = tp_name.split('.')
    class_name = parts[-1]
    module_name = '.'.join(parts[:-1])
    __import__(module_name, fromlist=[class_name])

  def testTypeNamesCanBeImported(self):
    # If import doesn't work, pickling won't work either.
    pb = unittest_pb2.TestAllTypes()
    self.assertImportFromName(pb.repeated_int32, 'Scalar')
    self.assertImportFromName(pb.repeated_nested_message, 'Composite')


if __name__ == '__main__':
  basetest.main()
