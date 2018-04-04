#! /usr/bin/env python
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


import collections
import copy
import math
import operator
import pickle
import six
import sys
import warnings

try:
  import unittest2 as unittest  # PY26
except ImportError:
  import unittest
try:
  cmp                                   # Python 2
except NameError:
  cmp = lambda x, y: (x > y) - (x < y)  # Python 3

from google.protobuf import map_proto2_unittest_pb2
from google.protobuf import map_unittest_pb2
from google.protobuf import unittest_pb2
from google.protobuf import unittest_proto3_arena_pb2
from google.protobuf import descriptor_pb2
from google.protobuf import descriptor_pool
from google.protobuf import message_factory
from google.protobuf import text_format
from google.protobuf.internal import api_implementation
from google.protobuf.internal import encoder
from google.protobuf.internal import packed_field_test_pb2
from google.protobuf.internal import test_util
from google.protobuf.internal import testing_refleaks
from google.protobuf import message
from google.protobuf.internal import _parameterized

if six.PY3:
  long = int


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


BaseTestCase = testing_refleaks.BaseTestCase


@_parameterized.named_parameters(
    ('_proto2', unittest_pb2),
    ('_proto3', unittest_proto3_arena_pb2))
class MessageTest(BaseTestCase):

  def testBadUtf8String(self, message_module):
    if api_implementation.Type() != 'python':
      self.skipTest("Skipping testBadUtf8String, currently only the python "
                    "api implementation raises UnicodeDecodeError when a "
                    "string field contains bad utf-8.")
    bad_utf8_data = test_util.GoldenFileData('bad_utf8_string')
    with self.assertRaises(UnicodeDecodeError) as context:
      message_module.TestAllTypes.FromString(bad_utf8_data)
    self.assertIn('TestAllTypes.optional_string', str(context.exception))

  def testGoldenMessage(self, message_module):
    # Proto3 doesn't have the "default_foo" members or foreign enums,
    # and doesn't preserve unknown fields, so for proto3 we use a golden
    # message that doesn't have these fields set.
    if message_module is unittest_pb2:
      golden_data = test_util.GoldenFileData(
          'golden_message_oneof_implemented')
    else:
      golden_data = test_util.GoldenFileData('golden_message_proto3')

    golden_message = message_module.TestAllTypes()
    golden_message.ParseFromString(golden_data)
    if message_module is unittest_pb2:
      test_util.ExpectAllFieldsSet(self, golden_message)
    self.assertEqual(golden_data, golden_message.SerializeToString())
    golden_copy = copy.deepcopy(golden_message)
    self.assertEqual(golden_data, golden_copy.SerializeToString())

  def testGoldenPackedMessage(self, message_module):
    golden_data = test_util.GoldenFileData('golden_packed_fields_message')
    golden_message = message_module.TestPackedTypes()
    golden_message.ParseFromString(golden_data)
    all_set = message_module.TestPackedTypes()
    test_util.SetAllPackedFields(all_set)
    self.assertEqual(all_set, golden_message)
    self.assertEqual(golden_data, all_set.SerializeToString())
    golden_copy = copy.deepcopy(golden_message)
    self.assertEqual(golden_data, golden_copy.SerializeToString())

  def testParseErrors(self, message_module):
    msg = message_module.TestAllTypes()
    self.assertRaises(TypeError, msg.FromString, 0)
    self.assertRaises(Exception, msg.FromString, '0')
    # TODO(jieluo): Fix cpp extension to raise error instead of warning.
    # b/27494216
    end_tag = encoder.TagBytes(1, 4)
    if api_implementation.Type() == 'python':
      with self.assertRaises(message.DecodeError) as context:
        msg.FromString(end_tag)
      self.assertEqual('Unexpected end-group tag.', str(context.exception))
    else:
      with warnings.catch_warnings(record=True) as w:
        # Cause all warnings to always be triggered.
        warnings.simplefilter('always')
        msg.FromString(end_tag)
        assert len(w) == 1
        assert issubclass(w[-1].category, RuntimeWarning)
        self.assertEqual('Unexpected end-group tag: Not all data was converted',
                         str(w[-1].message))

  def testDeterminismParameters(self, message_module):
    # This message is always deterministically serialized, even if determinism
    # is disabled, so we can use it to verify that all the determinism
    # parameters work correctly.
    golden_data = (b'\xe2\x02\nOne string'
                   b'\xe2\x02\nTwo string'
                   b'\xe2\x02\nRed string'
                   b'\xe2\x02\x0bBlue string')
    golden_message = message_module.TestAllTypes()
    golden_message.repeated_string.extend([
        'One string',
        'Two string',
        'Red string',
        'Blue string',
    ])
    self.assertEqual(golden_data,
                     golden_message.SerializeToString(deterministic=None))
    self.assertEqual(golden_data,
                     golden_message.SerializeToString(deterministic=False))
    self.assertEqual(golden_data,
                     golden_message.SerializeToString(deterministic=True))

    class BadArgError(Exception):
      pass

    class BadArg(object):

      def __nonzero__(self):
        raise BadArgError()

      def __bool__(self):
        raise BadArgError()

    with self.assertRaises(BadArgError):
      golden_message.SerializeToString(deterministic=BadArg())

  def testPickleSupport(self, message_module):
    golden_data = test_util.GoldenFileData('golden_message')
    golden_message = message_module.TestAllTypes()
    golden_message.ParseFromString(golden_data)
    pickled_message = pickle.dumps(golden_message)

    unpickled_message = pickle.loads(pickled_message)
    self.assertEqual(unpickled_message, golden_message)

  def testPositiveInfinity(self, message_module):
    if message_module is unittest_pb2:
      golden_data = (b'\x5D\x00\x00\x80\x7F'
                     b'\x61\x00\x00\x00\x00\x00\x00\xF0\x7F'
                     b'\xCD\x02\x00\x00\x80\x7F'
                     b'\xD1\x02\x00\x00\x00\x00\x00\x00\xF0\x7F')
    else:
      golden_data = (b'\x5D\x00\x00\x80\x7F'
                     b'\x61\x00\x00\x00\x00\x00\x00\xF0\x7F'
                     b'\xCA\x02\x04\x00\x00\x80\x7F'
                     b'\xD2\x02\x08\x00\x00\x00\x00\x00\x00\xF0\x7F')

    golden_message = message_module.TestAllTypes()
    golden_message.ParseFromString(golden_data)
    self.assertTrue(IsPosInf(golden_message.optional_float))
    self.assertTrue(IsPosInf(golden_message.optional_double))
    self.assertTrue(IsPosInf(golden_message.repeated_float[0]))
    self.assertTrue(IsPosInf(golden_message.repeated_double[0]))
    self.assertEqual(golden_data, golden_message.SerializeToString())

  def testNegativeInfinity(self, message_module):
    if message_module is unittest_pb2:
      golden_data = (b'\x5D\x00\x00\x80\xFF'
                     b'\x61\x00\x00\x00\x00\x00\x00\xF0\xFF'
                     b'\xCD\x02\x00\x00\x80\xFF'
                     b'\xD1\x02\x00\x00\x00\x00\x00\x00\xF0\xFF')
    else:
      golden_data = (b'\x5D\x00\x00\x80\xFF'
                     b'\x61\x00\x00\x00\x00\x00\x00\xF0\xFF'
                     b'\xCA\x02\x04\x00\x00\x80\xFF'
                     b'\xD2\x02\x08\x00\x00\x00\x00\x00\x00\xF0\xFF')

    golden_message = message_module.TestAllTypes()
    golden_message.ParseFromString(golden_data)
    self.assertTrue(IsNegInf(golden_message.optional_float))
    self.assertTrue(IsNegInf(golden_message.optional_double))
    self.assertTrue(IsNegInf(golden_message.repeated_float[0]))
    self.assertTrue(IsNegInf(golden_message.repeated_double[0]))
    self.assertEqual(golden_data, golden_message.SerializeToString())

  def testNotANumber(self, message_module):
    golden_data = (b'\x5D\x00\x00\xC0\x7F'
                   b'\x61\x00\x00\x00\x00\x00\x00\xF8\x7F'
                   b'\xCD\x02\x00\x00\xC0\x7F'
                   b'\xD1\x02\x00\x00\x00\x00\x00\x00\xF8\x7F')
    golden_message = message_module.TestAllTypes()
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
    message = message_module.TestAllTypes()
    message.ParseFromString(serialized)
    self.assertTrue(isnan(message.optional_float))
    self.assertTrue(isnan(message.optional_double))
    self.assertTrue(isnan(message.repeated_float[0]))
    self.assertTrue(isnan(message.repeated_double[0]))

  def testPositiveInfinityPacked(self, message_module):
    golden_data = (b'\xA2\x06\x04\x00\x00\x80\x7F'
                   b'\xAA\x06\x08\x00\x00\x00\x00\x00\x00\xF0\x7F')
    golden_message = message_module.TestPackedTypes()
    golden_message.ParseFromString(golden_data)
    self.assertTrue(IsPosInf(golden_message.packed_float[0]))
    self.assertTrue(IsPosInf(golden_message.packed_double[0]))
    self.assertEqual(golden_data, golden_message.SerializeToString())

  def testNegativeInfinityPacked(self, message_module):
    golden_data = (b'\xA2\x06\x04\x00\x00\x80\xFF'
                   b'\xAA\x06\x08\x00\x00\x00\x00\x00\x00\xF0\xFF')
    golden_message = message_module.TestPackedTypes()
    golden_message.ParseFromString(golden_data)
    self.assertTrue(IsNegInf(golden_message.packed_float[0]))
    self.assertTrue(IsNegInf(golden_message.packed_double[0]))
    self.assertEqual(golden_data, golden_message.SerializeToString())

  def testNotANumberPacked(self, message_module):
    golden_data = (b'\xA2\x06\x04\x00\x00\xC0\x7F'
                   b'\xAA\x06\x08\x00\x00\x00\x00\x00\x00\xF8\x7F')
    golden_message = message_module.TestPackedTypes()
    golden_message.ParseFromString(golden_data)
    self.assertTrue(isnan(golden_message.packed_float[0]))
    self.assertTrue(isnan(golden_message.packed_double[0]))

    serialized = golden_message.SerializeToString()
    message = message_module.TestPackedTypes()
    message.ParseFromString(serialized)
    self.assertTrue(isnan(message.packed_float[0]))
    self.assertTrue(isnan(message.packed_double[0]))

  def testExtremeFloatValues(self, message_module):
    message = message_module.TestAllTypes()

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

  def testExtremeDoubleValues(self, message_module):
    message = message_module.TestAllTypes()

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

  def testFloatPrinting(self, message_module):
    message = message_module.TestAllTypes()
    message.optional_float = 2.0
    self.assertEqual(str(message), 'optional_float: 2.0\n')

  def testHighPrecisionFloatPrinting(self, message_module):
    message = message_module.TestAllTypes()
    message.optional_double = 0.12345678912345678
    if sys.version_info >= (3,):
      self.assertEqual(str(message), 'optional_double: 0.12345678912345678\n')
    else:
      self.assertEqual(str(message), 'optional_double: 0.123456789123\n')

  def testUnknownFieldPrinting(self, message_module):
    populated = message_module.TestAllTypes()
    test_util.SetAllNonLazyFields(populated)
    empty = message_module.TestEmptyMessage()
    empty.ParseFromString(populated.SerializeToString())
    self.assertEqual(str(empty), '')

  def testRepeatedNestedFieldIteration(self, message_module):
    msg = message_module.TestAllTypes()
    msg.repeated_nested_message.add(bb=1)
    msg.repeated_nested_message.add(bb=2)
    msg.repeated_nested_message.add(bb=3)
    msg.repeated_nested_message.add(bb=4)

    self.assertEqual([1, 2, 3, 4],
                     [m.bb for m in msg.repeated_nested_message])
    self.assertEqual([4, 3, 2, 1],
                     [m.bb for m in reversed(msg.repeated_nested_message)])
    self.assertEqual([4, 3, 2, 1],
                     [m.bb for m in msg.repeated_nested_message[::-1]])

  def testSortingRepeatedScalarFieldsDefaultComparator(self, message_module):
    """Check some different types with the default comparator."""
    message = message_module.TestAllTypes()

    # TODO(mattp): would testing more scalar types strengthen test?
    message.repeated_int32.append(1)
    message.repeated_int32.append(3)
    message.repeated_int32.append(2)
    message.repeated_int32.sort()
    self.assertEqual(message.repeated_int32[0], 1)
    self.assertEqual(message.repeated_int32[1], 2)
    self.assertEqual(message.repeated_int32[2], 3)
    self.assertEqual(str(message.repeated_int32), str([1, 2, 3]))

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
    self.assertEqual(str(message.repeated_string), str([u'a', u'b', u'c']))

    message.repeated_bytes.append(b'a')
    message.repeated_bytes.append(b'c')
    message.repeated_bytes.append(b'b')
    message.repeated_bytes.sort()
    self.assertEqual(message.repeated_bytes[0], b'a')
    self.assertEqual(message.repeated_bytes[1], b'b')
    self.assertEqual(message.repeated_bytes[2], b'c')
    self.assertEqual(str(message.repeated_bytes), str([b'a', b'b', b'c']))

  def testSortingRepeatedScalarFieldsCustomComparator(self, message_module):
    """Check some different types with custom comparator."""
    message = message_module.TestAllTypes()

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

  def testSortingRepeatedCompositeFieldsCustomComparator(self, message_module):
    """Check passing a custom comparator to sort a repeated composite field."""
    message = message_module.TestAllTypes()

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
    self.assertEqual(str(message.repeated_nested_message),
                     '[bb: 1\n, bb: 2\n, bb: 3\n, bb: 4\n, bb: 5\n, bb: 6\n]')

  def testSortingRepeatedCompositeFieldsStable(self, message_module):
    """Check passing a custom comparator to sort a repeated composite field."""
    message = message_module.TestAllTypes()

    message.repeated_nested_message.add().bb = 21
    message.repeated_nested_message.add().bb = 20
    message.repeated_nested_message.add().bb = 13
    message.repeated_nested_message.add().bb = 33
    message.repeated_nested_message.add().bb = 11
    message.repeated_nested_message.add().bb = 24
    message.repeated_nested_message.add().bb = 10
    message.repeated_nested_message.sort(key=lambda z: z.bb // 10)
    self.assertEqual(
        [13, 11, 10, 21, 20, 24, 33],
        [n.bb for n in message.repeated_nested_message])

    # Make sure that for the C++ implementation, the underlying fields
    # are actually reordered.
    pb = message.SerializeToString()
    message.Clear()
    message.MergeFromString(pb)
    self.assertEqual(
        [13, 11, 10, 21, 20, 24, 33],
        [n.bb for n in message.repeated_nested_message])

  def testRepeatedCompositeFieldSortArguments(self, message_module):
    """Check sorting a repeated composite field using list.sort() arguments."""
    message = message_module.TestAllTypes()

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
    if sys.version_info >= (3,): return  # No cmp sorting in PY3.
    message.repeated_nested_message.sort(sort_function=cmp_bb)
    self.assertEqual([k.bb for k in message.repeated_nested_message],
                     [1, 2, 3, 4, 5, 6])
    message.repeated_nested_message.sort(cmp=cmp_bb, reverse=True)
    self.assertEqual([k.bb for k in message.repeated_nested_message],
                     [6, 5, 4, 3, 2, 1])

  def testRepeatedScalarFieldSortArguments(self, message_module):
    """Check sorting a scalar field using list.sort() arguments."""
    message = message_module.TestAllTypes()

    message.repeated_int32.append(-3)
    message.repeated_int32.append(-2)
    message.repeated_int32.append(-1)
    message.repeated_int32.sort(key=abs)
    self.assertEqual(list(message.repeated_int32), [-1, -2, -3])
    message.repeated_int32.sort(key=abs, reverse=True)
    self.assertEqual(list(message.repeated_int32), [-3, -2, -1])
    if sys.version_info < (3,):  # No cmp sorting in PY3.
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
    if sys.version_info < (3,):  # No cmp sorting in PY3.
      len_cmp = lambda a, b: cmp(len(a), len(b))
      message.repeated_string.sort(sort_function=len_cmp)
      self.assertEqual(list(message.repeated_string), ['c', 'bb', 'aaa'])
      message.repeated_string.sort(cmp=len_cmp, reverse=True)
      self.assertEqual(list(message.repeated_string), ['aaa', 'bb', 'c'])

  def testRepeatedFieldsComparable(self, message_module):
    m1 = message_module.TestAllTypes()
    m2 = message_module.TestAllTypes()
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

    if sys.version_info >= (3,): return  # No cmp() in PY3.

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

  def testRepeatedFieldsAreSequences(self, message_module):
    m = message_module.TestAllTypes()
    self.assertIsInstance(m.repeated_int32, collections.MutableSequence)
    self.assertIsInstance(m.repeated_nested_message,
                          collections.MutableSequence)

  def testRepeatedFieldsNotHashable(self, message_module):
    m = message_module.TestAllTypes()
    with self.assertRaises(TypeError):
      hash(m.repeated_int32)
    with self.assertRaises(TypeError):
      hash(m.repeated_nested_message)

  def testRepeatedFieldInsideNestedMessage(self, message_module):
    m = message_module.NestedTestAllTypes()
    m.payload.repeated_int32.extend([])
    self.assertTrue(m.HasField('payload'))

  def ensureNestedMessageExists(self, msg, attribute):
    """Make sure that a nested message object exists.

    As soon as a nested message attribute is accessed, it will be present in the
    _fields dict, without being marked as actually being set.
    """
    getattr(msg, attribute)
    self.assertFalse(msg.HasField(attribute))

  def testOneofGetCaseNonexistingField(self, message_module):
    m = message_module.TestAllTypes()
    self.assertRaises(ValueError, m.WhichOneof, 'no_such_oneof_field')
    self.assertRaises(Exception, m.WhichOneof, 0)

  def testOneofDefaultValues(self, message_module):
    m = message_module.TestAllTypes()
    self.assertIs(None, m.WhichOneof('oneof_field'))
    self.assertFalse(m.HasField('oneof_uint32'))

    # Oneof is set even when setting it to a default value.
    m.oneof_uint32 = 0
    self.assertEqual('oneof_uint32', m.WhichOneof('oneof_field'))
    self.assertTrue(m.HasField('oneof_uint32'))
    self.assertFalse(m.HasField('oneof_string'))

    m.oneof_string = ""
    self.assertEqual('oneof_string', m.WhichOneof('oneof_field'))
    self.assertTrue(m.HasField('oneof_string'))
    self.assertFalse(m.HasField('oneof_uint32'))

  def testOneofSemantics(self, message_module):
    m = message_module.TestAllTypes()
    self.assertIs(None, m.WhichOneof('oneof_field'))

    m.oneof_uint32 = 11
    self.assertEqual('oneof_uint32', m.WhichOneof('oneof_field'))
    self.assertTrue(m.HasField('oneof_uint32'))

    m.oneof_string = u'foo'
    self.assertEqual('oneof_string', m.WhichOneof('oneof_field'))
    self.assertFalse(m.HasField('oneof_uint32'))
    self.assertTrue(m.HasField('oneof_string'))

    # Read nested message accessor without accessing submessage.
    m.oneof_nested_message
    self.assertEqual('oneof_string', m.WhichOneof('oneof_field'))
    self.assertTrue(m.HasField('oneof_string'))
    self.assertFalse(m.HasField('oneof_nested_message'))

    # Read accessor of nested message without accessing submessage.
    m.oneof_nested_message.bb
    self.assertEqual('oneof_string', m.WhichOneof('oneof_field'))
    self.assertTrue(m.HasField('oneof_string'))
    self.assertFalse(m.HasField('oneof_nested_message'))

    m.oneof_nested_message.bb = 11
    self.assertEqual('oneof_nested_message', m.WhichOneof('oneof_field'))
    self.assertFalse(m.HasField('oneof_string'))
    self.assertTrue(m.HasField('oneof_nested_message'))

    m.oneof_bytes = b'bb'
    self.assertEqual('oneof_bytes', m.WhichOneof('oneof_field'))
    self.assertFalse(m.HasField('oneof_nested_message'))
    self.assertTrue(m.HasField('oneof_bytes'))

  def testOneofCompositeFieldReadAccess(self, message_module):
    m = message_module.TestAllTypes()
    m.oneof_uint32 = 11

    self.ensureNestedMessageExists(m, 'oneof_nested_message')
    self.assertEqual('oneof_uint32', m.WhichOneof('oneof_field'))
    self.assertEqual(11, m.oneof_uint32)

  def testOneofWhichOneof(self, message_module):
    m = message_module.TestAllTypes()
    self.assertIs(None, m.WhichOneof('oneof_field'))
    if message_module is unittest_pb2:
      self.assertFalse(m.HasField('oneof_field'))

    m.oneof_uint32 = 11
    self.assertEqual('oneof_uint32', m.WhichOneof('oneof_field'))
    if message_module is unittest_pb2:
      self.assertTrue(m.HasField('oneof_field'))

    m.oneof_bytes = b'bb'
    self.assertEqual('oneof_bytes', m.WhichOneof('oneof_field'))

    m.ClearField('oneof_bytes')
    self.assertIs(None, m.WhichOneof('oneof_field'))
    if message_module is unittest_pb2:
      self.assertFalse(m.HasField('oneof_field'))

  def testOneofClearField(self, message_module):
    m = message_module.TestAllTypes()
    m.oneof_uint32 = 11
    m.ClearField('oneof_field')
    if message_module is unittest_pb2:
      self.assertFalse(m.HasField('oneof_field'))
    self.assertFalse(m.HasField('oneof_uint32'))
    self.assertIs(None, m.WhichOneof('oneof_field'))

  def testOneofClearSetField(self, message_module):
    m = message_module.TestAllTypes()
    m.oneof_uint32 = 11
    m.ClearField('oneof_uint32')
    if message_module is unittest_pb2:
      self.assertFalse(m.HasField('oneof_field'))
    self.assertFalse(m.HasField('oneof_uint32'))
    self.assertIs(None, m.WhichOneof('oneof_field'))

  def testOneofClearUnsetField(self, message_module):
    m = message_module.TestAllTypes()
    m.oneof_uint32 = 11
    self.ensureNestedMessageExists(m, 'oneof_nested_message')
    m.ClearField('oneof_nested_message')
    self.assertEqual(11, m.oneof_uint32)
    if message_module is unittest_pb2:
      self.assertTrue(m.HasField('oneof_field'))
    self.assertTrue(m.HasField('oneof_uint32'))
    self.assertEqual('oneof_uint32', m.WhichOneof('oneof_field'))

  def testOneofDeserialize(self, message_module):
    m = message_module.TestAllTypes()
    m.oneof_uint32 = 11
    m2 = message_module.TestAllTypes()
    m2.ParseFromString(m.SerializeToString())
    self.assertEqual('oneof_uint32', m2.WhichOneof('oneof_field'))

  def testOneofCopyFrom(self, message_module):
    m = message_module.TestAllTypes()
    m.oneof_uint32 = 11
    m2 = message_module.TestAllTypes()
    m2.CopyFrom(m)
    self.assertEqual('oneof_uint32', m2.WhichOneof('oneof_field'))

  def testOneofNestedMergeFrom(self, message_module):
    m = message_module.NestedTestAllTypes()
    m.payload.oneof_uint32 = 11
    m2 = message_module.NestedTestAllTypes()
    m2.payload.oneof_bytes = b'bb'
    m2.child.payload.oneof_bytes = b'bb'
    m2.MergeFrom(m)
    self.assertEqual('oneof_uint32', m2.payload.WhichOneof('oneof_field'))
    self.assertEqual('oneof_bytes', m2.child.payload.WhichOneof('oneof_field'))

  def testOneofMessageMergeFrom(self, message_module):
    m = message_module.NestedTestAllTypes()
    m.payload.oneof_nested_message.bb = 11
    m.child.payload.oneof_nested_message.bb = 12
    m2 = message_module.NestedTestAllTypes()
    m2.payload.oneof_uint32 = 13
    m2.MergeFrom(m)
    self.assertEqual('oneof_nested_message',
                     m2.payload.WhichOneof('oneof_field'))
    self.assertEqual('oneof_nested_message',
                     m2.child.payload.WhichOneof('oneof_field'))

  def testOneofNestedMessageInit(self, message_module):
    m = message_module.TestAllTypes(
        oneof_nested_message=message_module.TestAllTypes.NestedMessage())
    self.assertEqual('oneof_nested_message', m.WhichOneof('oneof_field'))

  def testOneofClear(self, message_module):
    m = message_module.TestAllTypes()
    m.oneof_uint32 = 11
    m.Clear()
    self.assertIsNone(m.WhichOneof('oneof_field'))
    m.oneof_bytes = b'bb'
    self.assertEqual('oneof_bytes', m.WhichOneof('oneof_field'))

  def testAssignByteStringToUnicodeField(self, message_module):
    """Assigning a byte string to a string field should result
    in the value being converted to a Unicode string."""
    m = message_module.TestAllTypes()
    m.optional_string = str('')
    self.assertIsInstance(m.optional_string, six.text_type)

  def testLongValuedSlice(self, message_module):
    """It should be possible to use long-valued indicies in slices

    This didn't used to work in the v2 C++ implementation.
    """
    m = message_module.TestAllTypes()

    # Repeated scalar
    m.repeated_int32.append(1)
    sl = m.repeated_int32[long(0):long(len(m.repeated_int32))]
    self.assertEqual(len(m.repeated_int32), len(sl))

    # Repeated composite
    m.repeated_nested_message.add().bb = 3
    sl = m.repeated_nested_message[long(0):long(len(m.repeated_nested_message))]
    self.assertEqual(len(m.repeated_nested_message), len(sl))

  def testExtendShouldNotSwallowExceptions(self, message_module):
    """This didn't use to work in the v2 C++ implementation."""
    m = message_module.TestAllTypes()
    with self.assertRaises(NameError) as _:
      m.repeated_int32.extend(a for i in range(10))  # pylint: disable=undefined-variable
    with self.assertRaises(NameError) as _:
      m.repeated_nested_enum.extend(
          a for i in range(10))  # pylint: disable=undefined-variable

  FALSY_VALUES = [None, False, 0, 0.0, b'', u'', bytearray(), [], {}, set()]

  def testExtendInt32WithNothing(self, message_module):
    """Test no-ops extending repeated int32 fields."""
    m = message_module.TestAllTypes()
    self.assertSequenceEqual([], m.repeated_int32)

    # TODO(ptucker): Deprecate this behavior. b/18413862
    for falsy_value in MessageTest.FALSY_VALUES:
      m.repeated_int32.extend(falsy_value)
      self.assertSequenceEqual([], m.repeated_int32)

    m.repeated_int32.extend([])
    self.assertSequenceEqual([], m.repeated_int32)

  def testExtendFloatWithNothing(self, message_module):
    """Test no-ops extending repeated float fields."""
    m = message_module.TestAllTypes()
    self.assertSequenceEqual([], m.repeated_float)

    # TODO(ptucker): Deprecate this behavior. b/18413862
    for falsy_value in MessageTest.FALSY_VALUES:
      m.repeated_float.extend(falsy_value)
      self.assertSequenceEqual([], m.repeated_float)

    m.repeated_float.extend([])
    self.assertSequenceEqual([], m.repeated_float)

  def testExtendStringWithNothing(self, message_module):
    """Test no-ops extending repeated string fields."""
    m = message_module.TestAllTypes()
    self.assertSequenceEqual([], m.repeated_string)

    # TODO(ptucker): Deprecate this behavior. b/18413862
    for falsy_value in MessageTest.FALSY_VALUES:
      m.repeated_string.extend(falsy_value)
      self.assertSequenceEqual([], m.repeated_string)

    m.repeated_string.extend([])
    self.assertSequenceEqual([], m.repeated_string)

  def testExtendInt32WithPythonList(self, message_module):
    """Test extending repeated int32 fields with python lists."""
    m = message_module.TestAllTypes()
    self.assertSequenceEqual([], m.repeated_int32)
    m.repeated_int32.extend([0])
    self.assertSequenceEqual([0], m.repeated_int32)
    m.repeated_int32.extend([1, 2])
    self.assertSequenceEqual([0, 1, 2], m.repeated_int32)
    m.repeated_int32.extend([3, 4])
    self.assertSequenceEqual([0, 1, 2, 3, 4], m.repeated_int32)

  def testExtendFloatWithPythonList(self, message_module):
    """Test extending repeated float fields with python lists."""
    m = message_module.TestAllTypes()
    self.assertSequenceEqual([], m.repeated_float)
    m.repeated_float.extend([0.0])
    self.assertSequenceEqual([0.0], m.repeated_float)
    m.repeated_float.extend([1.0, 2.0])
    self.assertSequenceEqual([0.0, 1.0, 2.0], m.repeated_float)
    m.repeated_float.extend([3.0, 4.0])
    self.assertSequenceEqual([0.0, 1.0, 2.0, 3.0, 4.0], m.repeated_float)

  def testExtendStringWithPythonList(self, message_module):
    """Test extending repeated string fields with python lists."""
    m = message_module.TestAllTypes()
    self.assertSequenceEqual([], m.repeated_string)
    m.repeated_string.extend([''])
    self.assertSequenceEqual([''], m.repeated_string)
    m.repeated_string.extend(['11', '22'])
    self.assertSequenceEqual(['', '11', '22'], m.repeated_string)
    m.repeated_string.extend(['33', '44'])
    self.assertSequenceEqual(['', '11', '22', '33', '44'], m.repeated_string)

  def testExtendStringWithString(self, message_module):
    """Test extending repeated string fields with characters from a string."""
    m = message_module.TestAllTypes()
    self.assertSequenceEqual([], m.repeated_string)
    m.repeated_string.extend('abc')
    self.assertSequenceEqual(['a', 'b', 'c'], m.repeated_string)

  class TestIterable(object):
    """This iterable object mimics the behavior of numpy.array.

    __nonzero__ fails for length > 1, and returns bool(item[0]) for length == 1.

    """

    def __init__(self, values=None):
      self._list = values or []

    def __nonzero__(self):
      size = len(self._list)
      if size == 0:
        return False
      if size == 1:
        return bool(self._list[0])
      raise ValueError('Truth value is ambiguous.')

    def __len__(self):
      return len(self._list)

    def __iter__(self):
      return self._list.__iter__()

  def testExtendInt32WithIterable(self, message_module):
    """Test extending repeated int32 fields with iterable."""
    m = message_module.TestAllTypes()
    self.assertSequenceEqual([], m.repeated_int32)
    m.repeated_int32.extend(MessageTest.TestIterable([]))
    self.assertSequenceEqual([], m.repeated_int32)
    m.repeated_int32.extend(MessageTest.TestIterable([0]))
    self.assertSequenceEqual([0], m.repeated_int32)
    m.repeated_int32.extend(MessageTest.TestIterable([1, 2]))
    self.assertSequenceEqual([0, 1, 2], m.repeated_int32)
    m.repeated_int32.extend(MessageTest.TestIterable([3, 4]))
    self.assertSequenceEqual([0, 1, 2, 3, 4], m.repeated_int32)

  def testExtendFloatWithIterable(self, message_module):
    """Test extending repeated float fields with iterable."""
    m = message_module.TestAllTypes()
    self.assertSequenceEqual([], m.repeated_float)
    m.repeated_float.extend(MessageTest.TestIterable([]))
    self.assertSequenceEqual([], m.repeated_float)
    m.repeated_float.extend(MessageTest.TestIterable([0.0]))
    self.assertSequenceEqual([0.0], m.repeated_float)
    m.repeated_float.extend(MessageTest.TestIterable([1.0, 2.0]))
    self.assertSequenceEqual([0.0, 1.0, 2.0], m.repeated_float)
    m.repeated_float.extend(MessageTest.TestIterable([3.0, 4.0]))
    self.assertSequenceEqual([0.0, 1.0, 2.0, 3.0, 4.0], m.repeated_float)

  def testExtendStringWithIterable(self, message_module):
    """Test extending repeated string fields with iterable."""
    m = message_module.TestAllTypes()
    self.assertSequenceEqual([], m.repeated_string)
    m.repeated_string.extend(MessageTest.TestIterable([]))
    self.assertSequenceEqual([], m.repeated_string)
    m.repeated_string.extend(MessageTest.TestIterable(['']))
    self.assertSequenceEqual([''], m.repeated_string)
    m.repeated_string.extend(MessageTest.TestIterable(['1', '2']))
    self.assertSequenceEqual(['', '1', '2'], m.repeated_string)
    m.repeated_string.extend(MessageTest.TestIterable(['3', '4']))
    self.assertSequenceEqual(['', '1', '2', '3', '4'], m.repeated_string)

  def testPickleRepeatedScalarContainer(self, message_module):
    # TODO(tibell): The pure-Python implementation support pickling of
    #   scalar containers in *some* cases. For now the cpp2 version
    #   throws an exception to avoid a segfault. Investigate if we
    #   want to support pickling of these fields.
    #
    # For more information see: https://b2.corp.google.com/u/0/issues/18677897
    if (api_implementation.Type() != 'cpp' or
        api_implementation.Version() == 2):
      return
    m = message_module.TestAllTypes()
    with self.assertRaises(pickle.PickleError) as _:
      pickle.dumps(m.repeated_int32, pickle.HIGHEST_PROTOCOL)

  def testSortEmptyRepeatedCompositeContainer(self, message_module):
    """Exercise a scenario that has led to segfaults in the past.
    """
    m = message_module.TestAllTypes()
    m.repeated_nested_message.sort()

  def testHasFieldOnRepeatedField(self, message_module):
    """Using HasField on a repeated field should raise an exception.
    """
    m = message_module.TestAllTypes()
    with self.assertRaises(ValueError) as _:
      m.HasField('repeated_int32')

  def testRepeatedScalarFieldPop(self, message_module):
    m = message_module.TestAllTypes()
    with self.assertRaises(IndexError) as _:
      m.repeated_int32.pop()
    m.repeated_int32.extend(range(5))
    self.assertEqual(4, m.repeated_int32.pop())
    self.assertEqual(0, m.repeated_int32.pop(0))
    self.assertEqual(2, m.repeated_int32.pop(1))
    self.assertEqual([1, 3], m.repeated_int32)

  def testRepeatedCompositeFieldPop(self, message_module):
    m = message_module.TestAllTypes()
    with self.assertRaises(IndexError) as _:
      m.repeated_nested_message.pop()
    with self.assertRaises(TypeError) as _:
      m.repeated_nested_message.pop('0')
    for i in range(5):
      n = m.repeated_nested_message.add()
      n.bb = i
    self.assertEqual(4, m.repeated_nested_message.pop().bb)
    self.assertEqual(0, m.repeated_nested_message.pop(0).bb)
    self.assertEqual(2, m.repeated_nested_message.pop(1).bb)
    self.assertEqual([1, 3], [n.bb for n in m.repeated_nested_message])

  def testRepeatedCompareWithSelf(self, message_module):
    m = message_module.TestAllTypes()
    for i in range(5):
      m.repeated_int32.insert(i, i)
      n = m.repeated_nested_message.add()
      n.bb = i
    self.assertSequenceEqual(m.repeated_int32, m.repeated_int32)
    self.assertEqual(m.repeated_nested_message, m.repeated_nested_message)

  def testReleasedNestedMessages(self, message_module):
    """A case that lead to a segfault when a message detached from its parent
    container has itself a child container.
    """
    m = message_module.NestedTestAllTypes()
    m = m.repeated_child.add()
    m = m.child
    m = m.repeated_child.add()
    self.assertEqual(m.payload.optional_int32, 0)

  def testSetRepeatedComposite(self, message_module):
    m = message_module.TestAllTypes()
    with self.assertRaises(AttributeError):
      m.repeated_int32 = []
    m.repeated_int32.append(1)
    if api_implementation.Type() == 'cpp':
      # For test coverage: cpp has a different path if composite
      # field is in cache
      with self.assertRaises(TypeError):
        m.repeated_int32 = []
    else:
      with self.assertRaises(AttributeError):
        m.repeated_int32 = []


# Class to test proto2-only features (required, extensions, etc.)
class Proto2Test(BaseTestCase):

  def testFieldPresence(self):
    message = unittest_pb2.TestAllTypes()

    self.assertFalse(message.HasField("optional_int32"))
    self.assertFalse(message.HasField("optional_bool"))
    self.assertFalse(message.HasField("optional_nested_message"))

    with self.assertRaises(ValueError):
      message.HasField("field_doesnt_exist")

    with self.assertRaises(ValueError):
      message.HasField("repeated_int32")
    with self.assertRaises(ValueError):
      message.HasField("repeated_nested_message")

    self.assertEqual(0, message.optional_int32)
    self.assertEqual(False, message.optional_bool)
    self.assertEqual(0, message.optional_nested_message.bb)

    # Fields are set even when setting the values to default values.
    message.optional_int32 = 0
    message.optional_bool = False
    message.optional_nested_message.bb = 0
    self.assertTrue(message.HasField("optional_int32"))
    self.assertTrue(message.HasField("optional_bool"))
    self.assertTrue(message.HasField("optional_nested_message"))

    # Set the fields to non-default values.
    message.optional_int32 = 5
    message.optional_bool = True
    message.optional_nested_message.bb = 15

    self.assertTrue(message.HasField("optional_int32"))
    self.assertTrue(message.HasField("optional_bool"))
    self.assertTrue(message.HasField("optional_nested_message"))

    # Clearing the fields unsets them and resets their value to default.
    message.ClearField("optional_int32")
    message.ClearField("optional_bool")
    message.ClearField("optional_nested_message")

    self.assertFalse(message.HasField("optional_int32"))
    self.assertFalse(message.HasField("optional_bool"))
    self.assertFalse(message.HasField("optional_nested_message"))
    self.assertEqual(0, message.optional_int32)
    self.assertEqual(False, message.optional_bool)
    self.assertEqual(0, message.optional_nested_message.bb)

  def testAssignInvalidEnum(self):
    """Assigning an invalid enum number is not allowed in proto2."""
    m = unittest_pb2.TestAllTypes()

    # Proto2 can not assign unknown enum.
    with self.assertRaises(ValueError) as _:
      m.optional_nested_enum = 1234567
    self.assertRaises(ValueError, m.repeated_nested_enum.append, 1234567)
    # Assignment is a different code path than append for the C++ impl.
    m.repeated_nested_enum.append(2)
    m.repeated_nested_enum[0] = 2
    with self.assertRaises(ValueError):
      m.repeated_nested_enum[0] = 123456

    # Unknown enum value can be parsed but is ignored.
    m2 = unittest_proto3_arena_pb2.TestAllTypes()
    m2.optional_nested_enum = 1234567
    m2.repeated_nested_enum.append(7654321)
    serialized = m2.SerializeToString()

    m3 = unittest_pb2.TestAllTypes()
    m3.ParseFromString(serialized)
    self.assertFalse(m3.HasField('optional_nested_enum'))
    # 1 is the default value for optional_nested_enum.
    self.assertEqual(1, m3.optional_nested_enum)
    self.assertEqual(0, len(m3.repeated_nested_enum))
    m2.Clear()
    m2.ParseFromString(m3.SerializeToString())
    self.assertEqual(1234567, m2.optional_nested_enum)
    self.assertEqual(7654321, m2.repeated_nested_enum[0])

  def testUnknownEnumMap(self):
    m = map_proto2_unittest_pb2.TestEnumMap()
    m.known_map_field[123] = 0
    with self.assertRaises(ValueError):
      m.unknown_map_field[1] = 123

  def testExtensionsErrors(self):
    msg = unittest_pb2.TestAllTypes()
    self.assertRaises(AttributeError, getattr, msg, 'Extensions')

  def testGoldenExtensions(self):
    golden_data = test_util.GoldenFileData('golden_message')
    golden_message = unittest_pb2.TestAllExtensions()
    golden_message.ParseFromString(golden_data)
    all_set = unittest_pb2.TestAllExtensions()
    test_util.SetAllExtensions(all_set)
    self.assertEqual(all_set, golden_message)
    self.assertEqual(golden_data, golden_message.SerializeToString())
    golden_copy = copy.deepcopy(golden_message)
    self.assertEqual(golden_data, golden_copy.SerializeToString())

  def testGoldenPackedExtensions(self):
    golden_data = test_util.GoldenFileData('golden_packed_fields_message')
    golden_message = unittest_pb2.TestPackedExtensions()
    golden_message.ParseFromString(golden_data)
    all_set = unittest_pb2.TestPackedExtensions()
    test_util.SetAllPackedExtensions(all_set)
    self.assertEqual(all_set, golden_message)
    self.assertEqual(golden_data, all_set.SerializeToString())
    golden_copy = copy.deepcopy(golden_message)
    self.assertEqual(golden_data, golden_copy.SerializeToString())

  def testPickleIncompleteProto(self):
    golden_message = unittest_pb2.TestRequired(a=1)
    pickled_message = pickle.dumps(golden_message)

    unpickled_message = pickle.loads(pickled_message)
    self.assertEqual(unpickled_message, golden_message)
    self.assertEqual(unpickled_message.a, 1)
    # This is still an incomplete proto - so serializing should fail
    self.assertRaises(message.EncodeError, unpickled_message.SerializeToString)


  # TODO(haberman): this isn't really a proto2-specific test except that this
  # message has a required field in it.  Should probably be factored out so
  # that we can test the other parts with proto3.
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

  def testPythonicInit(self):
    message = unittest_pb2.TestAllTypes(
        optional_int32=100,
        optional_fixed32=200,
        optional_float=300.5,
        optional_bytes=b'x',
        optionalgroup={'a': 400},
        optional_nested_message={'bb': 500},
        optional_foreign_message={},
        optional_nested_enum='BAZ',
        repeatedgroup=[{'a': 600},
                       {'a': 700}],
        repeated_nested_enum=['FOO', unittest_pb2.TestAllTypes.BAR],
        default_int32=800,
        oneof_string='y')
    self.assertIsInstance(message, unittest_pb2.TestAllTypes)
    self.assertEqual(100, message.optional_int32)
    self.assertEqual(200, message.optional_fixed32)
    self.assertEqual(300.5, message.optional_float)
    self.assertEqual(b'x', message.optional_bytes)
    self.assertEqual(400, message.optionalgroup.a)
    self.assertIsInstance(message.optional_nested_message,
                          unittest_pb2.TestAllTypes.NestedMessage)
    self.assertEqual(500, message.optional_nested_message.bb)
    self.assertTrue(message.HasField('optional_foreign_message'))
    self.assertEqual(message.optional_foreign_message,
                     unittest_pb2.ForeignMessage())
    self.assertEqual(unittest_pb2.TestAllTypes.BAZ,
                     message.optional_nested_enum)
    self.assertEqual(2, len(message.repeatedgroup))
    self.assertEqual(600, message.repeatedgroup[0].a)
    self.assertEqual(700, message.repeatedgroup[1].a)
    self.assertEqual(2, len(message.repeated_nested_enum))
    self.assertEqual(unittest_pb2.TestAllTypes.FOO,
                     message.repeated_nested_enum[0])
    self.assertEqual(unittest_pb2.TestAllTypes.BAR,
                     message.repeated_nested_enum[1])
    self.assertEqual(800, message.default_int32)
    self.assertEqual('y', message.oneof_string)
    self.assertFalse(message.HasField('optional_int64'))
    self.assertEqual(0, len(message.repeated_float))
    self.assertEqual(42, message.default_int64)

    message = unittest_pb2.TestAllTypes(optional_nested_enum=u'BAZ')
    self.assertEqual(unittest_pb2.TestAllTypes.BAZ,
                     message.optional_nested_enum)

    with self.assertRaises(ValueError):
      unittest_pb2.TestAllTypes(
          optional_nested_message={'INVALID_NESTED_FIELD': 17})

    with self.assertRaises(TypeError):
      unittest_pb2.TestAllTypes(
          optional_nested_message={'bb': 'INVALID_VALUE_TYPE'})

    with self.assertRaises(ValueError):
      unittest_pb2.TestAllTypes(optional_nested_enum='INVALID_LABEL')

    with self.assertRaises(ValueError):
      unittest_pb2.TestAllTypes(repeated_nested_enum='FOO')



# Class to test proto3-only features/behavior (updated field presence & enums)
class Proto3Test(BaseTestCase):

  # Utility method for comparing equality with a map.
  def assertMapIterEquals(self, map_iter, dict_value):
    # Avoid mutating caller's copy.
    dict_value = dict(dict_value)

    for k, v in map_iter:
      self.assertEqual(v, dict_value[k])
      del dict_value[k]

    self.assertEqual({}, dict_value)

  def testFieldPresence(self):
    message = unittest_proto3_arena_pb2.TestAllTypes()

    # We can't test presence of non-repeated, non-submessage fields.
    with self.assertRaises(ValueError):
      message.HasField('optional_int32')
    with self.assertRaises(ValueError):
      message.HasField('optional_float')
    with self.assertRaises(ValueError):
      message.HasField('optional_string')
    with self.assertRaises(ValueError):
      message.HasField('optional_bool')

    # But we can still test presence of submessage fields.
    self.assertFalse(message.HasField('optional_nested_message'))

    # As with proto2, we can't test presence of fields that don't exist, or
    # repeated fields.
    with self.assertRaises(ValueError):
      message.HasField('field_doesnt_exist')

    with self.assertRaises(ValueError):
      message.HasField('repeated_int32')
    with self.assertRaises(ValueError):
      message.HasField('repeated_nested_message')

    # Fields should default to their type-specific default.
    self.assertEqual(0, message.optional_int32)
    self.assertEqual(0, message.optional_float)
    self.assertEqual('', message.optional_string)
    self.assertEqual(False, message.optional_bool)
    self.assertEqual(0, message.optional_nested_message.bb)

    # Setting a submessage should still return proper presence information.
    message.optional_nested_message.bb = 0
    self.assertTrue(message.HasField('optional_nested_message'))

    # Set the fields to non-default values.
    message.optional_int32 = 5
    message.optional_float = 1.1
    message.optional_string = 'abc'
    message.optional_bool = True
    message.optional_nested_message.bb = 15

    # Clearing the fields unsets them and resets their value to default.
    message.ClearField('optional_int32')
    message.ClearField('optional_float')
    message.ClearField('optional_string')
    message.ClearField('optional_bool')
    message.ClearField('optional_nested_message')

    self.assertEqual(0, message.optional_int32)
    self.assertEqual(0, message.optional_float)
    self.assertEqual('', message.optional_string)
    self.assertEqual(False, message.optional_bool)
    self.assertEqual(0, message.optional_nested_message.bb)

  def testAssignUnknownEnum(self):
    """Assigning an unknown enum value is allowed and preserves the value."""
    m = unittest_proto3_arena_pb2.TestAllTypes()

    # Proto3 can assign unknown enums.
    m.optional_nested_enum = 1234567
    self.assertEqual(1234567, m.optional_nested_enum)
    m.repeated_nested_enum.append(22334455)
    self.assertEqual(22334455, m.repeated_nested_enum[0])
    # Assignment is a different code path than append for the C++ impl.
    m.repeated_nested_enum[0] = 7654321
    self.assertEqual(7654321, m.repeated_nested_enum[0])
    serialized = m.SerializeToString()

    m2 = unittest_proto3_arena_pb2.TestAllTypes()
    m2.ParseFromString(serialized)
    self.assertEqual(1234567, m2.optional_nested_enum)
    self.assertEqual(7654321, m2.repeated_nested_enum[0])

  # Map isn't really a proto3-only feature. But there is no proto2 equivalent
  # of google/protobuf/map_unittest.proto right now, so it's not easy to
  # test both with the same test like we do for the other proto2/proto3 tests.
  # (google/protobuf/map_proto2_unittest.proto is very different in the set
  # of messages and fields it contains).
  def testScalarMapDefaults(self):
    msg = map_unittest_pb2.TestMap()

    # Scalars start out unset.
    self.assertFalse(-123 in msg.map_int32_int32)
    self.assertFalse(-2**33 in msg.map_int64_int64)
    self.assertFalse(123 in msg.map_uint32_uint32)
    self.assertFalse(2**33 in msg.map_uint64_uint64)
    self.assertFalse(123 in msg.map_int32_double)
    self.assertFalse(False in msg.map_bool_bool)
    self.assertFalse('abc' in msg.map_string_string)
    self.assertFalse(111 in msg.map_int32_bytes)
    self.assertFalse(888 in msg.map_int32_enum)

    # Accessing an unset key returns the default.
    self.assertEqual(0, msg.map_int32_int32[-123])
    self.assertEqual(0, msg.map_int64_int64[-2**33])
    self.assertEqual(0, msg.map_uint32_uint32[123])
    self.assertEqual(0, msg.map_uint64_uint64[2**33])
    self.assertEqual(0.0, msg.map_int32_double[123])
    self.assertTrue(isinstance(msg.map_int32_double[123], float))
    self.assertEqual(False, msg.map_bool_bool[False])
    self.assertTrue(isinstance(msg.map_bool_bool[False], bool))
    self.assertEqual('', msg.map_string_string['abc'])
    self.assertEqual(b'', msg.map_int32_bytes[111])
    self.assertEqual(0, msg.map_int32_enum[888])

    # It also sets the value in the map
    self.assertTrue(-123 in msg.map_int32_int32)
    self.assertTrue(-2**33 in msg.map_int64_int64)
    self.assertTrue(123 in msg.map_uint32_uint32)
    self.assertTrue(2**33 in msg.map_uint64_uint64)
    self.assertTrue(123 in msg.map_int32_double)
    self.assertTrue(False in msg.map_bool_bool)
    self.assertTrue('abc' in msg.map_string_string)
    self.assertTrue(111 in msg.map_int32_bytes)
    self.assertTrue(888 in msg.map_int32_enum)

    self.assertIsInstance(msg.map_string_string['abc'], six.text_type)

    # Accessing an unset key still throws TypeError if the type of the key
    # is incorrect.
    with self.assertRaises(TypeError):
      msg.map_string_string[123]

    with self.assertRaises(TypeError):
      123 in msg.map_string_string

  def testMapGet(self):
    # Need to test that get() properly returns the default, even though the dict
    # has defaultdict-like semantics.
    msg = map_unittest_pb2.TestMap()

    self.assertIsNone(msg.map_int32_int32.get(5))
    self.assertEqual(10, msg.map_int32_int32.get(5, 10))
    self.assertIsNone(msg.map_int32_int32.get(5))

    msg.map_int32_int32[5] = 15
    self.assertEqual(15, msg.map_int32_int32.get(5))
    self.assertEqual(15, msg.map_int32_int32.get(5))
    with self.assertRaises(TypeError):
      msg.map_int32_int32.get('')

    self.assertIsNone(msg.map_int32_foreign_message.get(5))
    self.assertEqual(10, msg.map_int32_foreign_message.get(5, 10))

    submsg = msg.map_int32_foreign_message[5]
    self.assertIs(submsg, msg.map_int32_foreign_message.get(5))
    with self.assertRaises(TypeError):
      msg.map_int32_foreign_message.get('')

  def testScalarMap(self):
    msg = map_unittest_pb2.TestMap()

    self.assertEqual(0, len(msg.map_int32_int32))
    self.assertFalse(5 in msg.map_int32_int32)

    msg.map_int32_int32[-123] = -456
    msg.map_int64_int64[-2**33] = -2**34
    msg.map_uint32_uint32[123] = 456
    msg.map_uint64_uint64[2**33] = 2**34
    msg.map_int32_float[2] = 1.2
    msg.map_int32_double[1] = 3.3
    msg.map_string_string['abc'] = '123'
    msg.map_bool_bool[True] = True
    msg.map_int32_enum[888] = 2
    # Unknown numeric enum is supported in proto3.
    msg.map_int32_enum[123] = 456

    self.assertEqual([], msg.FindInitializationErrors())

    self.assertEqual(1, len(msg.map_string_string))

    # Bad key.
    with self.assertRaises(TypeError):
      msg.map_string_string[123] = '123'

    # Verify that trying to assign a bad key doesn't actually add a member to
    # the map.
    self.assertEqual(1, len(msg.map_string_string))

    # Bad value.
    with self.assertRaises(TypeError):
      msg.map_string_string['123'] = 123

    serialized = msg.SerializeToString()
    msg2 = map_unittest_pb2.TestMap()
    msg2.ParseFromString(serialized)

    # Bad key.
    with self.assertRaises(TypeError):
      msg2.map_string_string[123] = '123'

    # Bad value.
    with self.assertRaises(TypeError):
      msg2.map_string_string['123'] = 123

    self.assertEqual(-456, msg2.map_int32_int32[-123])
    self.assertEqual(-2**34, msg2.map_int64_int64[-2**33])
    self.assertEqual(456, msg2.map_uint32_uint32[123])
    self.assertEqual(2**34, msg2.map_uint64_uint64[2**33])
    self.assertAlmostEqual(1.2, msg.map_int32_float[2])
    self.assertEqual(3.3, msg.map_int32_double[1])
    self.assertEqual('123', msg2.map_string_string['abc'])
    self.assertEqual(True, msg2.map_bool_bool[True])
    self.assertEqual(2, msg2.map_int32_enum[888])
    self.assertEqual(456, msg2.map_int32_enum[123])
    # TODO(jieluo): Add cpp extension support.
    if api_implementation.Type() == 'python':
      self.assertEqual('{-123: -456}',
                       str(msg2.map_int32_int32))

  def testMapEntryAlwaysSerialized(self):
    msg = map_unittest_pb2.TestMap()
    msg.map_int32_int32[0] = 0
    msg.map_string_string[''] = ''
    self.assertEqual(msg.ByteSize(), 12)
    self.assertEqual(b'\n\x04\x08\x00\x10\x00r\x04\n\x00\x12\x00',
                     msg.SerializeToString())

  def testStringUnicodeConversionInMap(self):
    msg = map_unittest_pb2.TestMap()

    unicode_obj = u'\u1234'
    bytes_obj = unicode_obj.encode('utf8')

    msg.map_string_string[bytes_obj] = bytes_obj

    (key, value) = list(msg.map_string_string.items())[0]

    self.assertEqual(key, unicode_obj)
    self.assertEqual(value, unicode_obj)

    self.assertIsInstance(key, six.text_type)
    self.assertIsInstance(value, six.text_type)

  def testMessageMap(self):
    msg = map_unittest_pb2.TestMap()

    self.assertEqual(0, len(msg.map_int32_foreign_message))
    self.assertFalse(5 in msg.map_int32_foreign_message)

    msg.map_int32_foreign_message[123]
    # get_or_create() is an alias for getitem.
    msg.map_int32_foreign_message.get_or_create(-456)

    self.assertEqual(2, len(msg.map_int32_foreign_message))
    self.assertIn(123, msg.map_int32_foreign_message)
    self.assertIn(-456, msg.map_int32_foreign_message)
    self.assertEqual(2, len(msg.map_int32_foreign_message))

    # Bad key.
    with self.assertRaises(TypeError):
      msg.map_int32_foreign_message['123']

    # Can't assign directly to submessage.
    with self.assertRaises(ValueError):
      msg.map_int32_foreign_message[999] = msg.map_int32_foreign_message[123]

    # Verify that trying to assign a bad key doesn't actually add a member to
    # the map.
    self.assertEqual(2, len(msg.map_int32_foreign_message))

    serialized = msg.SerializeToString()
    msg2 = map_unittest_pb2.TestMap()
    msg2.ParseFromString(serialized)

    self.assertEqual(2, len(msg2.map_int32_foreign_message))
    self.assertIn(123, msg2.map_int32_foreign_message)
    self.assertIn(-456, msg2.map_int32_foreign_message)
    self.assertEqual(2, len(msg2.map_int32_foreign_message))
    # TODO(jieluo): Fix text format for message map.
    # TODO(jieluo): Add cpp extension support.
    if api_implementation.Type() == 'python':
      self.assertEqual(15,
                       len(str(msg2.map_int32_foreign_message)))

  def testNestedMessageMapItemDelete(self):
    msg = map_unittest_pb2.TestMap()
    msg.map_int32_all_types[1].optional_nested_message.bb = 1
    del msg.map_int32_all_types[1]
    msg.map_int32_all_types[2].optional_nested_message.bb = 2
    self.assertEqual(1, len(msg.map_int32_all_types))
    msg.map_int32_all_types[1].optional_nested_message.bb = 1
    self.assertEqual(2, len(msg.map_int32_all_types))

    serialized = msg.SerializeToString()
    msg2 = map_unittest_pb2.TestMap()
    msg2.ParseFromString(serialized)
    keys = [1, 2]
    # The loop triggers PyErr_Occurred() in c extension.
    for key in keys:
      del msg2.map_int32_all_types[key]

  def testMapByteSize(self):
    msg = map_unittest_pb2.TestMap()
    msg.map_int32_int32[1] = 1
    size = msg.ByteSize()
    msg.map_int32_int32[1] = 128
    self.assertEqual(msg.ByteSize(), size + 1)

    msg.map_int32_foreign_message[19].c = 1
    size = msg.ByteSize()
    msg.map_int32_foreign_message[19].c = 128
    self.assertEqual(msg.ByteSize(), size + 1)

  def testMergeFrom(self):
    msg = map_unittest_pb2.TestMap()
    msg.map_int32_int32[12] = 34
    msg.map_int32_int32[56] = 78
    msg.map_int64_int64[22] = 33
    msg.map_int32_foreign_message[111].c = 5
    msg.map_int32_foreign_message[222].c = 10

    msg2 = map_unittest_pb2.TestMap()
    msg2.map_int32_int32[12] = 55
    msg2.map_int64_int64[88] = 99
    msg2.map_int32_foreign_message[222].c = 15
    msg2.map_int32_foreign_message[222].d = 20
    old_map_value = msg2.map_int32_foreign_message[222]

    msg2.MergeFrom(msg)

    self.assertEqual(34, msg2.map_int32_int32[12])
    self.assertEqual(78, msg2.map_int32_int32[56])
    self.assertEqual(33, msg2.map_int64_int64[22])
    self.assertEqual(99, msg2.map_int64_int64[88])
    self.assertEqual(5, msg2.map_int32_foreign_message[111].c)
    self.assertEqual(10, msg2.map_int32_foreign_message[222].c)
    self.assertFalse(msg2.map_int32_foreign_message[222].HasField('d'))
    if api_implementation.Type() != 'cpp':
      # During the call to MergeFrom(), the C++ implementation will have
      # deallocated the underlying message, but this is very difficult to detect
      # properly. The line below is likely to cause a segmentation fault.
      # With the Python implementation, old_map_value is just 'detached' from
      # the main message. Using it will not crash of course, but since it still
      # have a reference to the parent message I'm sure we can find interesting
      # ways to cause inconsistencies.
      self.assertEqual(15, old_map_value.c)

    # Verify that there is only one entry per key, even though the MergeFrom
    # may have internally created multiple entries for a single key in the
    # list representation.
    as_dict = {}
    for key in msg2.map_int32_foreign_message:
      self.assertFalse(key in as_dict)
      as_dict[key] = msg2.map_int32_foreign_message[key].c

    self.assertEqual({111: 5, 222: 10}, as_dict)

    # Special case: test that delete of item really removes the item, even if
    # there might have physically been duplicate keys due to the previous merge.
    # This is only a special case for the C++ implementation which stores the
    # map as an array.
    del msg2.map_int32_int32[12]
    self.assertFalse(12 in msg2.map_int32_int32)

    del msg2.map_int32_foreign_message[222]
    self.assertFalse(222 in msg2.map_int32_foreign_message)
    with self.assertRaises(TypeError):
      del msg2.map_int32_foreign_message['']

  def testMapMergeFrom(self):
    msg = map_unittest_pb2.TestMap()
    msg.map_int32_int32[12] = 34
    msg.map_int32_int32[56] = 78
    msg.map_int64_int64[22] = 33
    msg.map_int32_foreign_message[111].c = 5
    msg.map_int32_foreign_message[222].c = 10

    msg2 = map_unittest_pb2.TestMap()
    msg2.map_int32_int32[12] = 55
    msg2.map_int64_int64[88] = 99
    msg2.map_int32_foreign_message[222].c = 15
    msg2.map_int32_foreign_message[222].d = 20

    msg2.map_int32_int32.MergeFrom(msg.map_int32_int32)
    self.assertEqual(34, msg2.map_int32_int32[12])
    self.assertEqual(78, msg2.map_int32_int32[56])

    msg2.map_int64_int64.MergeFrom(msg.map_int64_int64)
    self.assertEqual(33, msg2.map_int64_int64[22])
    self.assertEqual(99, msg2.map_int64_int64[88])

    msg2.map_int32_foreign_message.MergeFrom(msg.map_int32_foreign_message)
    self.assertEqual(5, msg2.map_int32_foreign_message[111].c)
    self.assertEqual(10, msg2.map_int32_foreign_message[222].c)
    self.assertFalse(msg2.map_int32_foreign_message[222].HasField('d'))

  def testMergeFromBadType(self):
    msg = map_unittest_pb2.TestMap()
    with self.assertRaisesRegexp(
        TypeError,
        r'Parameter to MergeFrom\(\) must be instance of same class: expected '
        r'.*TestMap got int\.'):
      msg.MergeFrom(1)

  def testCopyFromBadType(self):
    msg = map_unittest_pb2.TestMap()
    with self.assertRaisesRegexp(
        TypeError,
        r'Parameter to [A-Za-z]*From\(\) must be instance of same class: '
        r'expected .*TestMap got int\.'):
      msg.CopyFrom(1)

  def testIntegerMapWithLongs(self):
    msg = map_unittest_pb2.TestMap()
    msg.map_int32_int32[long(-123)] = long(-456)
    msg.map_int64_int64[long(-2**33)] = long(-2**34)
    msg.map_uint32_uint32[long(123)] = long(456)
    msg.map_uint64_uint64[long(2**33)] = long(2**34)

    serialized = msg.SerializeToString()
    msg2 = map_unittest_pb2.TestMap()
    msg2.ParseFromString(serialized)

    self.assertEqual(-456, msg2.map_int32_int32[-123])
    self.assertEqual(-2**34, msg2.map_int64_int64[-2**33])
    self.assertEqual(456, msg2.map_uint32_uint32[123])
    self.assertEqual(2**34, msg2.map_uint64_uint64[2**33])

  def testMapAssignmentCausesPresence(self):
    msg = map_unittest_pb2.TestMapSubmessage()
    msg.test_map.map_int32_int32[123] = 456

    serialized = msg.SerializeToString()
    msg2 = map_unittest_pb2.TestMapSubmessage()
    msg2.ParseFromString(serialized)

    self.assertEqual(msg, msg2)

    # Now test that various mutations of the map properly invalidate the
    # cached size of the submessage.
    msg.test_map.map_int32_int32[888] = 999
    serialized = msg.SerializeToString()
    msg2.ParseFromString(serialized)
    self.assertEqual(msg, msg2)

    msg.test_map.map_int32_int32.clear()
    serialized = msg.SerializeToString()
    msg2.ParseFromString(serialized)
    self.assertEqual(msg, msg2)

  def testMapAssignmentCausesPresenceForSubmessages(self):
    msg = map_unittest_pb2.TestMapSubmessage()
    msg.test_map.map_int32_foreign_message[123].c = 5

    serialized = msg.SerializeToString()
    msg2 = map_unittest_pb2.TestMapSubmessage()
    msg2.ParseFromString(serialized)

    self.assertEqual(msg, msg2)

    # Now test that various mutations of the map properly invalidate the
    # cached size of the submessage.
    msg.test_map.map_int32_foreign_message[888].c = 7
    serialized = msg.SerializeToString()
    msg2.ParseFromString(serialized)
    self.assertEqual(msg, msg2)

    msg.test_map.map_int32_foreign_message[888].MergeFrom(
        msg.test_map.map_int32_foreign_message[123])
    serialized = msg.SerializeToString()
    msg2.ParseFromString(serialized)
    self.assertEqual(msg, msg2)

    msg.test_map.map_int32_foreign_message.clear()
    serialized = msg.SerializeToString()
    msg2.ParseFromString(serialized)
    self.assertEqual(msg, msg2)

  def testModifyMapWhileIterating(self):
    msg = map_unittest_pb2.TestMap()

    string_string_iter = iter(msg.map_string_string)
    int32_foreign_iter = iter(msg.map_int32_foreign_message)

    msg.map_string_string['abc'] = '123'
    msg.map_int32_foreign_message[5].c = 5

    with self.assertRaises(RuntimeError):
      for key in string_string_iter:
        pass

    with self.assertRaises(RuntimeError):
      for key in int32_foreign_iter:
        pass

  def testSubmessageMap(self):
    msg = map_unittest_pb2.TestMap()

    submsg = msg.map_int32_foreign_message[111]
    self.assertIs(submsg, msg.map_int32_foreign_message[111])
    self.assertIsInstance(submsg, unittest_pb2.ForeignMessage)

    submsg.c = 5

    serialized = msg.SerializeToString()
    msg2 = map_unittest_pb2.TestMap()
    msg2.ParseFromString(serialized)

    self.assertEqual(5, msg2.map_int32_foreign_message[111].c)

    # Doesn't allow direct submessage assignment.
    with self.assertRaises(ValueError):
      msg.map_int32_foreign_message[88] = unittest_pb2.ForeignMessage()

  def testMapIteration(self):
    msg = map_unittest_pb2.TestMap()

    for k, v in msg.map_int32_int32.items():
      # Should not be reached.
      self.assertTrue(False)

    msg.map_int32_int32[2] = 4
    msg.map_int32_int32[3] = 6
    msg.map_int32_int32[4] = 8
    self.assertEqual(3, len(msg.map_int32_int32))

    matching_dict = {2: 4, 3: 6, 4: 8}
    self.assertMapIterEquals(msg.map_int32_int32.items(), matching_dict)

  def testPython2Map(self):
    if sys.version_info < (3,):
      msg = map_unittest_pb2.TestMap()
      msg.map_int32_int32[2] = 4
      msg.map_int32_int32[3] = 6
      msg.map_int32_int32[4] = 8
      msg.map_int32_int32[5] = 10
      map_int32 = msg.map_int32_int32
      self.assertEqual(4, len(map_int32))
      msg2 = map_unittest_pb2.TestMap()
      msg2.ParseFromString(msg.SerializeToString())

      def CheckItems(seq, iterator):
        self.assertEqual(next(iterator), seq[0])
        self.assertEqual(list(iterator), seq[1:])

      CheckItems(map_int32.items(), map_int32.iteritems())
      CheckItems(map_int32.keys(), map_int32.iterkeys())
      CheckItems(map_int32.values(), map_int32.itervalues())

      self.assertEqual(6, map_int32.get(3))
      self.assertEqual(None, map_int32.get(999))
      self.assertEqual(6, map_int32.pop(3))
      self.assertEqual(0, map_int32.pop(3))
      self.assertEqual(3, len(map_int32))
      key, value = map_int32.popitem()
      self.assertEqual(2 * key, value)
      self.assertEqual(2, len(map_int32))
      map_int32.clear()
      self.assertEqual(0, len(map_int32))

      with self.assertRaises(KeyError):
        map_int32.popitem()

      self.assertEqual(0, map_int32.setdefault(2))
      self.assertEqual(1, len(map_int32))

      map_int32.update(msg2.map_int32_int32)
      self.assertEqual(4, len(map_int32))

      with self.assertRaises(TypeError):
        map_int32.update(msg2.map_int32_int32,
                         msg2.map_int32_int32)
      with self.assertRaises(TypeError):
        map_int32.update(0)
      with self.assertRaises(TypeError):
        map_int32.update(value=12)

  def testMapItems(self):
    # Map items used to have strange behaviors when use c extension. Because
    # [] may reorder the map and invalidate any exsting iterators.
    # TODO(jieluo): Check if [] reordering the map is a bug or intended
    # behavior.
    msg = map_unittest_pb2.TestMap()
    msg.map_string_string['local_init_op'] = ''
    msg.map_string_string['trainable_variables'] = ''
    msg.map_string_string['variables'] = ''
    msg.map_string_string['init_op'] = ''
    msg.map_string_string['summaries'] = ''
    items1 = msg.map_string_string.items()
    items2 = msg.map_string_string.items()
    self.assertEqual(items1, items2)

  def testMapDeterministicSerialization(self):
    golden_data = (b'r\x0c\n\x07init_op\x12\x01d'
                   b'r\n\n\x05item1\x12\x01e'
                   b'r\n\n\x05item2\x12\x01f'
                   b'r\n\n\x05item3\x12\x01g'
                   b'r\x0b\n\x05item4\x12\x02QQ'
                   b'r\x12\n\rlocal_init_op\x12\x01a'
                   b'r\x0e\n\tsummaries\x12\x01e'
                   b'r\x18\n\x13trainable_variables\x12\x01b'
                   b'r\x0e\n\tvariables\x12\x01c')
    msg = map_unittest_pb2.TestMap()
    msg.map_string_string['local_init_op'] = 'a'
    msg.map_string_string['trainable_variables'] = 'b'
    msg.map_string_string['variables'] = 'c'
    msg.map_string_string['init_op'] = 'd'
    msg.map_string_string['summaries'] = 'e'
    msg.map_string_string['item1'] = 'e'
    msg.map_string_string['item2'] = 'f'
    msg.map_string_string['item3'] = 'g'
    msg.map_string_string['item4'] = 'QQ'

    # If deterministic serialization is not working correctly, this will be
    # "flaky" depending on the exact python dict hash seed.
    #
    # Fortunately, there are enough items in this map that it is extremely
    # unlikely to ever hit the "right" in-order combination, so the test
    # itself should fail reliably.
    self.assertEqual(golden_data, msg.SerializeToString(deterministic=True))

  def testMapIterationClearMessage(self):
    # Iterator needs to work even if message and map are deleted.
    msg = map_unittest_pb2.TestMap()

    msg.map_int32_int32[2] = 4
    msg.map_int32_int32[3] = 6
    msg.map_int32_int32[4] = 8

    it = msg.map_int32_int32.items()
    del msg

    matching_dict = {2: 4, 3: 6, 4: 8}
    self.assertMapIterEquals(it, matching_dict)

  def testMapConstruction(self):
    msg = map_unittest_pb2.TestMap(map_int32_int32={1: 2, 3: 4})
    self.assertEqual(2, msg.map_int32_int32[1])
    self.assertEqual(4, msg.map_int32_int32[3])

    msg = map_unittest_pb2.TestMap(
        map_int32_foreign_message={3: unittest_pb2.ForeignMessage(c=5)})
    self.assertEqual(5, msg.map_int32_foreign_message[3].c)

  def testMapValidAfterFieldCleared(self):
    # Map needs to work even if field is cleared.
    # For the C++ implementation this tests the correctness of
    # ScalarMapContainer::Release()
    msg = map_unittest_pb2.TestMap()
    int32_map = msg.map_int32_int32

    int32_map[2] = 4
    int32_map[3] = 6
    int32_map[4] = 8

    msg.ClearField('map_int32_int32')
    self.assertEqual(b'', msg.SerializeToString())
    matching_dict = {2: 4, 3: 6, 4: 8}
    self.assertMapIterEquals(int32_map.items(), matching_dict)

  def testMessageMapValidAfterFieldCleared(self):
    # Map needs to work even if field is cleared.
    # For the C++ implementation this tests the correctness of
    # ScalarMapContainer::Release()
    msg = map_unittest_pb2.TestMap()
    int32_foreign_message = msg.map_int32_foreign_message

    int32_foreign_message[2].c = 5

    msg.ClearField('map_int32_foreign_message')
    self.assertEqual(b'', msg.SerializeToString())
    self.assertTrue(2 in int32_foreign_message.keys())

  def testMapIterInvalidatedByClearField(self):
    # Map iterator is invalidated when field is cleared.
    # But this case does need to not crash the interpreter.
    # For the C++ implementation this tests the correctness of
    # ScalarMapContainer::Release()
    msg = map_unittest_pb2.TestMap()

    it = iter(msg.map_int32_int32)

    msg.ClearField('map_int32_int32')
    with self.assertRaises(RuntimeError):
      for _ in it:
        pass

    it = iter(msg.map_int32_foreign_message)
    msg.ClearField('map_int32_foreign_message')
    with self.assertRaises(RuntimeError):
      for _ in it:
        pass

  def testMapDelete(self):
    msg = map_unittest_pb2.TestMap()

    self.assertEqual(0, len(msg.map_int32_int32))

    msg.map_int32_int32[4] = 6
    self.assertEqual(1, len(msg.map_int32_int32))

    with self.assertRaises(KeyError):
      del msg.map_int32_int32[88]

    del msg.map_int32_int32[4]
    self.assertEqual(0, len(msg.map_int32_int32))

    with self.assertRaises(KeyError):
      del msg.map_int32_all_types[32]

  def testMapsAreMapping(self):
    msg = map_unittest_pb2.TestMap()
    self.assertIsInstance(msg.map_int32_int32, collections.Mapping)
    self.assertIsInstance(msg.map_int32_int32, collections.MutableMapping)
    self.assertIsInstance(msg.map_int32_foreign_message, collections.Mapping)
    self.assertIsInstance(msg.map_int32_foreign_message,
                          collections.MutableMapping)

  def testMapsCompare(self):
    msg = map_unittest_pb2.TestMap()
    msg.map_int32_int32[-123] = -456
    self.assertEqual(msg.map_int32_int32, msg.map_int32_int32)
    self.assertEqual(msg.map_int32_foreign_message,
                     msg.map_int32_foreign_message)
    self.assertNotEqual(msg.map_int32_int32, 0)

  def testMapFindInitializationErrorsSmokeTest(self):
    msg = map_unittest_pb2.TestMap()
    msg.map_string_string['abc'] = '123'
    msg.map_int32_int32[35] = 64
    msg.map_string_foreign_message['foo'].c = 5
    self.assertEqual(0, len(msg.FindInitializationErrors()))



class ValidTypeNamesTest(BaseTestCase):

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

class PackedFieldTest(BaseTestCase):

  def setMessage(self, message):
    message.repeated_int32.append(1)
    message.repeated_int64.append(1)
    message.repeated_uint32.append(1)
    message.repeated_uint64.append(1)
    message.repeated_sint32.append(1)
    message.repeated_sint64.append(1)
    message.repeated_fixed32.append(1)
    message.repeated_fixed64.append(1)
    message.repeated_sfixed32.append(1)
    message.repeated_sfixed64.append(1)
    message.repeated_float.append(1.0)
    message.repeated_double.append(1.0)
    message.repeated_bool.append(True)
    message.repeated_nested_enum.append(1)

  def testPackedFields(self):
    message = packed_field_test_pb2.TestPackedTypes()
    self.setMessage(message)
    golden_data = (b'\x0A\x01\x01'
                   b'\x12\x01\x01'
                   b'\x1A\x01\x01'
                   b'\x22\x01\x01'
                   b'\x2A\x01\x02'
                   b'\x32\x01\x02'
                   b'\x3A\x04\x01\x00\x00\x00'
                   b'\x42\x08\x01\x00\x00\x00\x00\x00\x00\x00'
                   b'\x4A\x04\x01\x00\x00\x00'
                   b'\x52\x08\x01\x00\x00\x00\x00\x00\x00\x00'
                   b'\x5A\x04\x00\x00\x80\x3f'
                   b'\x62\x08\x00\x00\x00\x00\x00\x00\xf0\x3f'
                   b'\x6A\x01\x01'
                   b'\x72\x01\x01')
    self.assertEqual(golden_data, message.SerializeToString())

  def testUnpackedFields(self):
    message = packed_field_test_pb2.TestUnpackedTypes()
    self.setMessage(message)
    golden_data = (b'\x08\x01'
                   b'\x10\x01'
                   b'\x18\x01'
                   b'\x20\x01'
                   b'\x28\x02'
                   b'\x30\x02'
                   b'\x3D\x01\x00\x00\x00'
                   b'\x41\x01\x00\x00\x00\x00\x00\x00\x00'
                   b'\x4D\x01\x00\x00\x00'
                   b'\x51\x01\x00\x00\x00\x00\x00\x00\x00'
                   b'\x5D\x00\x00\x80\x3f'
                   b'\x61\x00\x00\x00\x00\x00\x00\xf0\x3f'
                   b'\x68\x01'
                   b'\x70\x01')
    self.assertEqual(golden_data, message.SerializeToString())


@unittest.skipIf(api_implementation.Type() != 'cpp' or
                 sys.version_info < (2, 7),
                 'explicit tests of the C++ implementation for PY27 and above')
class OversizeProtosTest(BaseTestCase):

  @classmethod
  def setUpClass(cls):
    # At the moment, reference cycles between DescriptorPool and Message classes
    # are not detected and these objects are never freed.
    # To avoid errors with ReferenceLeakChecker, we create the class only once.
    file_desc = """
      name: "f/f.msg2"
      package: "f"
      message_type {
        name: "msg1"
        field {
          name: "payload"
          number: 1
          label: LABEL_OPTIONAL
          type: TYPE_STRING
        }
      }
      message_type {
        name: "msg2"
        field {
          name: "field"
          number: 1
          label: LABEL_OPTIONAL
          type: TYPE_MESSAGE
          type_name: "msg1"
        }
      }
    """
    pool = descriptor_pool.DescriptorPool()
    desc = descriptor_pb2.FileDescriptorProto()
    text_format.Parse(file_desc, desc)
    pool.Add(desc)
    cls.proto_cls = message_factory.MessageFactory(pool).GetPrototype(
        pool.FindMessageTypeByName('f.msg2'))

  def setUp(self):
    self.p = self.proto_cls()
    self.p.field.payload = 'c' * (1024 * 1024 * 64 + 1)
    self.p_serialized = self.p.SerializeToString()

  def testAssertOversizeProto(self):
    from google.protobuf.pyext._message import SetAllowOversizeProtos
    SetAllowOversizeProtos(False)
    q = self.proto_cls()
    try:
      q.ParseFromString(self.p_serialized)
    except message.DecodeError as e:
      self.assertEqual(str(e), 'Error parsing message')

  def testSucceedOversizeProto(self):
    from google.protobuf.pyext._message import SetAllowOversizeProtos
    SetAllowOversizeProtos(True)
    q = self.proto_cls()
    q.ParseFromString(self.p_serialized)
    self.assertEqual(self.p.field.payload, q.field.payload)

if __name__ == '__main__':
  unittest.main()
