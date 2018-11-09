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

"""Test for google.protobuf.internal.well_known_types."""

__author__ = 'jieluo@google.com (Jie Luo)'

import collections
import datetime

try:
  import unittest2 as unittest  #PY26
except ImportError:
  import unittest

from google.protobuf import any_pb2
from google.protobuf import duration_pb2
from google.protobuf import field_mask_pb2
from google.protobuf import struct_pb2
from google.protobuf import timestamp_pb2
from google.protobuf import map_unittest_pb2
from google.protobuf import unittest_pb2
from google.protobuf.internal import any_test_pb2
from google.protobuf.internal import test_util
from google.protobuf.internal import well_known_types
from google.protobuf import descriptor
from google.protobuf import text_format


class TimeUtilTestBase(unittest.TestCase):

  def CheckTimestampConversion(self, message, text):
    self.assertEqual(text, message.ToJsonString())
    parsed_message = timestamp_pb2.Timestamp()
    parsed_message.FromJsonString(text)
    self.assertEqual(message, parsed_message)

  def CheckDurationConversion(self, message, text):
    self.assertEqual(text, message.ToJsonString())
    parsed_message = duration_pb2.Duration()
    parsed_message.FromJsonString(text)
    self.assertEqual(message, parsed_message)


class TimeUtilTest(TimeUtilTestBase):

  def testTimestampSerializeAndParse(self):
    message = timestamp_pb2.Timestamp()
    # Generated output should contain 3, 6, or 9 fractional digits.
    message.seconds = 0
    message.nanos = 0
    self.CheckTimestampConversion(message, '1970-01-01T00:00:00Z')
    message.nanos = 10000000
    self.CheckTimestampConversion(message, '1970-01-01T00:00:00.010Z')
    message.nanos = 10000
    self.CheckTimestampConversion(message, '1970-01-01T00:00:00.000010Z')
    message.nanos = 10
    self.CheckTimestampConversion(message, '1970-01-01T00:00:00.000000010Z')
    # Test min timestamps.
    message.seconds = -62135596800
    message.nanos = 0
    self.CheckTimestampConversion(message, '0001-01-01T00:00:00Z')
    # Test max timestamps.
    message.seconds = 253402300799
    message.nanos = 999999999
    self.CheckTimestampConversion(message, '9999-12-31T23:59:59.999999999Z')
    # Test negative timestamps.
    message.seconds = -1
    self.CheckTimestampConversion(message, '1969-12-31T23:59:59.999999999Z')

    # Parsing accepts an fractional digits as long as they fit into nano
    # precision.
    message.FromJsonString('1970-01-01T00:00:00.1Z')
    self.assertEqual(0, message.seconds)
    self.assertEqual(100000000, message.nanos)
    # Parsing accepts offsets.
    message.FromJsonString('1970-01-01T00:00:00-08:00')
    self.assertEqual(8 * 3600, message.seconds)
    self.assertEqual(0, message.nanos)

    # It is not easy to check with current time. For test coverage only.
    message.GetCurrentTime()
    self.assertNotEqual(8 * 3600, message.seconds)

  def testDurationSerializeAndParse(self):
    message = duration_pb2.Duration()
    # Generated output should contain 3, 6, or 9 fractional digits.
    message.seconds = 0
    message.nanos = 0
    self.CheckDurationConversion(message, '0s')
    message.nanos = 10000000
    self.CheckDurationConversion(message, '0.010s')
    message.nanos = 10000
    self.CheckDurationConversion(message, '0.000010s')
    message.nanos = 10
    self.CheckDurationConversion(message, '0.000000010s')

    # Test min and max
    message.seconds = 315576000000
    message.nanos = 999999999
    self.CheckDurationConversion(message, '315576000000.999999999s')
    message.seconds = -315576000000
    message.nanos = -999999999
    self.CheckDurationConversion(message, '-315576000000.999999999s')

    # Parsing accepts an fractional digits as long as they fit into nano
    # precision.
    message.FromJsonString('0.1s')
    self.assertEqual(100000000, message.nanos)
    message.FromJsonString('0.0000001s')
    self.assertEqual(100, message.nanos)

  def testTimestampIntegerConversion(self):
    message = timestamp_pb2.Timestamp()
    message.FromNanoseconds(1)
    self.assertEqual('1970-01-01T00:00:00.000000001Z',
                     message.ToJsonString())
    self.assertEqual(1, message.ToNanoseconds())

    message.FromNanoseconds(-1)
    self.assertEqual('1969-12-31T23:59:59.999999999Z',
                     message.ToJsonString())
    self.assertEqual(-1, message.ToNanoseconds())

    message.FromMicroseconds(1)
    self.assertEqual('1970-01-01T00:00:00.000001Z',
                     message.ToJsonString())
    self.assertEqual(1, message.ToMicroseconds())

    message.FromMicroseconds(-1)
    self.assertEqual('1969-12-31T23:59:59.999999Z',
                     message.ToJsonString())
    self.assertEqual(-1, message.ToMicroseconds())

    message.FromMilliseconds(1)
    self.assertEqual('1970-01-01T00:00:00.001Z',
                     message.ToJsonString())
    self.assertEqual(1, message.ToMilliseconds())

    message.FromMilliseconds(-1)
    self.assertEqual('1969-12-31T23:59:59.999Z',
                     message.ToJsonString())
    self.assertEqual(-1, message.ToMilliseconds())

    message.FromSeconds(1)
    self.assertEqual('1970-01-01T00:00:01Z',
                     message.ToJsonString())
    self.assertEqual(1, message.ToSeconds())

    message.FromSeconds(-1)
    self.assertEqual('1969-12-31T23:59:59Z',
                     message.ToJsonString())
    self.assertEqual(-1, message.ToSeconds())

    message.FromNanoseconds(1999)
    self.assertEqual(1, message.ToMicroseconds())
    # For negative values, Timestamp will be rounded down.
    # For example, "1969-12-31T23:59:59.5Z" (i.e., -0.5s) rounded to seconds
    # will be "1969-12-31T23:59:59Z" (i.e., -1s) rather than
    # "1970-01-01T00:00:00Z" (i.e., 0s).
    message.FromNanoseconds(-1999)
    self.assertEqual(-2, message.ToMicroseconds())

  def testDurationIntegerConversion(self):
    message = duration_pb2.Duration()
    message.FromNanoseconds(1)
    self.assertEqual('0.000000001s',
                     message.ToJsonString())
    self.assertEqual(1, message.ToNanoseconds())

    message.FromNanoseconds(-1)
    self.assertEqual('-0.000000001s',
                     message.ToJsonString())
    self.assertEqual(-1, message.ToNanoseconds())

    message.FromMicroseconds(1)
    self.assertEqual('0.000001s',
                     message.ToJsonString())
    self.assertEqual(1, message.ToMicroseconds())

    message.FromMicroseconds(-1)
    self.assertEqual('-0.000001s',
                     message.ToJsonString())
    self.assertEqual(-1, message.ToMicroseconds())

    message.FromMilliseconds(1)
    self.assertEqual('0.001s',
                     message.ToJsonString())
    self.assertEqual(1, message.ToMilliseconds())

    message.FromMilliseconds(-1)
    self.assertEqual('-0.001s',
                     message.ToJsonString())
    self.assertEqual(-1, message.ToMilliseconds())

    message.FromSeconds(1)
    self.assertEqual('1s', message.ToJsonString())
    self.assertEqual(1, message.ToSeconds())

    message.FromSeconds(-1)
    self.assertEqual('-1s',
                     message.ToJsonString())
    self.assertEqual(-1, message.ToSeconds())

    # Test truncation behavior.
    message.FromNanoseconds(1999)
    self.assertEqual(1, message.ToMicroseconds())

    # For negative values, Duration will be rounded towards 0.
    message.FromNanoseconds(-1999)
    self.assertEqual(-1, message.ToMicroseconds())

  def testDatetimeConverison(self):
    message = timestamp_pb2.Timestamp()
    dt = datetime.datetime(1970, 1, 1)
    message.FromDatetime(dt)
    self.assertEqual(dt, message.ToDatetime())

    message.FromMilliseconds(1999)
    self.assertEqual(datetime.datetime(1970, 1, 1, 0, 0, 1, 999000),
                     message.ToDatetime())

  def testDatetimeConversionWithTimezone(self):
    class TZ(datetime.tzinfo):

      def utcoffset(self, _):
        return datetime.timedelta(hours=1)

      def dst(self, _):
        return datetime.timedelta(0)

      def tzname(self, _):
        return 'UTC+1'

    message1 = timestamp_pb2.Timestamp()
    dt = datetime.datetime(1970, 1, 1, 1, tzinfo=TZ())
    message1.FromDatetime(dt)
    message2 = timestamp_pb2.Timestamp()
    dt = datetime.datetime(1970, 1, 1, 0)
    message2.FromDatetime(dt)
    self.assertEqual(message1, message2)

  def testTimedeltaConversion(self):
    message = duration_pb2.Duration()
    message.FromNanoseconds(1999999999)
    td = message.ToTimedelta()
    self.assertEqual(1, td.seconds)
    self.assertEqual(999999, td.microseconds)

    message.FromNanoseconds(-1999999999)
    td = message.ToTimedelta()
    self.assertEqual(-1, td.days)
    self.assertEqual(86398, td.seconds)
    self.assertEqual(1, td.microseconds)

    message.FromMicroseconds(-1)
    td = message.ToTimedelta()
    self.assertEqual(-1, td.days)
    self.assertEqual(86399, td.seconds)
    self.assertEqual(999999, td.microseconds)
    converted_message = duration_pb2.Duration()
    converted_message.FromTimedelta(td)
    self.assertEqual(message, converted_message)

  def testInvalidTimestamp(self):
    message = timestamp_pb2.Timestamp()
    self.assertRaisesRegexp(
        well_known_types.ParseError,
        'Failed to parse timestamp: missing valid timezone offset.',
        message.FromJsonString,
        '')
    self.assertRaisesRegexp(
        well_known_types.ParseError,
        'Failed to parse timestamp: invalid trailing data '
        '1970-01-01T00:00:01Ztrail.',
        message.FromJsonString,
        '1970-01-01T00:00:01Ztrail')
    self.assertRaisesRegexp(
        ValueError,
        'time data \'10000-01-01T00:00:00\' does not match'
        ' format \'%Y-%m-%dT%H:%M:%S\'',
        message.FromJsonString, '10000-01-01T00:00:00.00Z')
    self.assertRaisesRegexp(
        well_known_types.ParseError,
        'nanos 0123456789012 more than 9 fractional digits.',
        message.FromJsonString,
        '1970-01-01T00:00:00.0123456789012Z')
    self.assertRaisesRegexp(
        well_known_types.ParseError,
        (r'Invalid timezone offset value: \+08.'),
        message.FromJsonString,
        '1972-01-01T01:00:00.01+08',)
    self.assertRaisesRegexp(
        ValueError,
        'year (0 )?is out of range',
        message.FromJsonString,
        '0000-01-01T00:00:00Z')
    message.seconds = 253402300800
    self.assertRaisesRegexp(
        OverflowError,
        'date value out of range',
        message.ToJsonString)

  def testInvalidDuration(self):
    message = duration_pb2.Duration()
    self.assertRaisesRegexp(
        well_known_types.ParseError,
        'Duration must end with letter "s": 1.',
        message.FromJsonString, '1')
    self.assertRaisesRegexp(
        well_known_types.ParseError,
        'Couldn\'t parse duration: 1...2s.',
        message.FromJsonString, '1...2s')
    text = '-315576000001.000000000s'
    self.assertRaisesRegexp(
        well_known_types.Error,
        r'Duration is not valid\: Seconds -315576000001 must be in range'
        r' \[-315576000000\, 315576000000\].',
        message.FromJsonString, text)
    text = '315576000001.000000000s'
    self.assertRaisesRegexp(
        well_known_types.Error,
        r'Duration is not valid\: Seconds 315576000001 must be in range'
        r' \[-315576000000\, 315576000000\].',
        message.FromJsonString, text)
    message.seconds = -315576000001
    message.nanos = 0
    self.assertRaisesRegexp(
        well_known_types.Error,
        r'Duration is not valid\: Seconds -315576000001 must be in range'
        r' \[-315576000000\, 315576000000\].',
        message.ToJsonString)
    message.seconds = 0
    message.nanos = 999999999 + 1
    self.assertRaisesRegexp(
        well_known_types.Error,
        r'Duration is not valid\: Nanos 1000000000 must be in range'
        r' \[-999999999\, 999999999\].',
        message.ToJsonString)
    message.seconds = -1
    message.nanos = 1
    self.assertRaisesRegexp(
        well_known_types.Error,
        r'Duration is not valid\: Sign mismatch.',
        message.ToJsonString)


class FieldMaskTest(unittest.TestCase):

  def testStringFormat(self):
    mask = field_mask_pb2.FieldMask()
    self.assertEqual('', mask.ToJsonString())
    mask.paths.append('foo')
    self.assertEqual('foo', mask.ToJsonString())
    mask.paths.append('bar')
    self.assertEqual('foo,bar', mask.ToJsonString())

    mask.FromJsonString('')
    self.assertEqual('', mask.ToJsonString())
    mask.FromJsonString('foo')
    self.assertEqual(['foo'], mask.paths)
    mask.FromJsonString('foo,bar')
    self.assertEqual(['foo', 'bar'], mask.paths)

    # Test camel case
    mask.Clear()
    mask.paths.append('foo_bar')
    self.assertEqual('fooBar', mask.ToJsonString())
    mask.paths.append('bar_quz')
    self.assertEqual('fooBar,barQuz', mask.ToJsonString())

    mask.FromJsonString('')
    self.assertEqual('', mask.ToJsonString())
    mask.FromJsonString('fooBar')
    self.assertEqual(['foo_bar'], mask.paths)
    mask.FromJsonString('fooBar,barQuz')
    self.assertEqual(['foo_bar', 'bar_quz'], mask.paths)

  def testDescriptorToFieldMask(self):
    mask = field_mask_pb2.FieldMask()
    msg_descriptor = unittest_pb2.TestAllTypes.DESCRIPTOR
    mask.AllFieldsFromDescriptor(msg_descriptor)
    self.assertEqual(75, len(mask.paths))
    self.assertTrue(mask.IsValidForDescriptor(msg_descriptor))
    for field in msg_descriptor.fields:
      self.assertTrue(field.name in mask.paths)

  def testIsValidForDescriptor(self):
    msg_descriptor = unittest_pb2.TestAllTypes.DESCRIPTOR
    # Empty mask
    mask = field_mask_pb2.FieldMask()
    self.assertTrue(mask.IsValidForDescriptor(msg_descriptor))
    # All fields from descriptor
    mask.AllFieldsFromDescriptor(msg_descriptor)
    self.assertTrue(mask.IsValidForDescriptor(msg_descriptor))
    # Child under optional message
    mask.paths.append('optional_nested_message.bb')
    self.assertTrue(mask.IsValidForDescriptor(msg_descriptor))
    # Repeated field is only allowed in the last position of path
    mask.paths.append('repeated_nested_message.bb')
    self.assertFalse(mask.IsValidForDescriptor(msg_descriptor))
    # Invalid top level field
    mask = field_mask_pb2.FieldMask()
    mask.paths.append('xxx')
    self.assertFalse(mask.IsValidForDescriptor(msg_descriptor))
    # Invalid field in root
    mask = field_mask_pb2.FieldMask()
    mask.paths.append('xxx.zzz')
    self.assertFalse(mask.IsValidForDescriptor(msg_descriptor))
    # Invalid field in internal node
    mask = field_mask_pb2.FieldMask()
    mask.paths.append('optional_nested_message.xxx.zzz')
    self.assertFalse(mask.IsValidForDescriptor(msg_descriptor))
    # Invalid field in leaf
    mask = field_mask_pb2.FieldMask()
    mask.paths.append('optional_nested_message.xxx')
    self.assertFalse(mask.IsValidForDescriptor(msg_descriptor))

  def testCanonicalFrom(self):
    mask = field_mask_pb2.FieldMask()
    out_mask = field_mask_pb2.FieldMask()
    # Paths will be sorted.
    mask.FromJsonString('baz.quz,bar,foo')
    out_mask.CanonicalFormFromMask(mask)
    self.assertEqual('bar,baz.quz,foo', out_mask.ToJsonString())
    # Duplicated paths will be removed.
    mask.FromJsonString('foo,bar,foo')
    out_mask.CanonicalFormFromMask(mask)
    self.assertEqual('bar,foo', out_mask.ToJsonString())
    # Sub-paths of other paths will be removed.
    mask.FromJsonString('foo.b1,bar.b1,foo.b2,bar')
    out_mask.CanonicalFormFromMask(mask)
    self.assertEqual('bar,foo.b1,foo.b2', out_mask.ToJsonString())

    # Test more deeply nested cases.
    mask.FromJsonString(
        'foo.bar.baz1,foo.bar.baz2.quz,foo.bar.baz2')
    out_mask.CanonicalFormFromMask(mask)
    self.assertEqual('foo.bar.baz1,foo.bar.baz2',
                     out_mask.ToJsonString())
    mask.FromJsonString(
        'foo.bar.baz1,foo.bar.baz2,foo.bar.baz2.quz')
    out_mask.CanonicalFormFromMask(mask)
    self.assertEqual('foo.bar.baz1,foo.bar.baz2',
                     out_mask.ToJsonString())
    mask.FromJsonString(
        'foo.bar.baz1,foo.bar.baz2,foo.bar.baz2.quz,foo.bar')
    out_mask.CanonicalFormFromMask(mask)
    self.assertEqual('foo.bar', out_mask.ToJsonString())
    mask.FromJsonString(
        'foo.bar.baz1,foo.bar.baz2,foo.bar.baz2.quz,foo')
    out_mask.CanonicalFormFromMask(mask)
    self.assertEqual('foo', out_mask.ToJsonString())

  def testUnion(self):
    mask1 = field_mask_pb2.FieldMask()
    mask2 = field_mask_pb2.FieldMask()
    out_mask = field_mask_pb2.FieldMask()
    mask1.FromJsonString('foo,baz')
    mask2.FromJsonString('bar,quz')
    out_mask.Union(mask1, mask2)
    self.assertEqual('bar,baz,foo,quz', out_mask.ToJsonString())
    # Overlap with duplicated paths.
    mask1.FromJsonString('foo,baz.bb')
    mask2.FromJsonString('baz.bb,quz')
    out_mask.Union(mask1, mask2)
    self.assertEqual('baz.bb,foo,quz', out_mask.ToJsonString())
    # Overlap with paths covering some other paths.
    mask1.FromJsonString('foo.bar.baz,quz')
    mask2.FromJsonString('foo.bar,bar')
    out_mask.Union(mask1, mask2)
    self.assertEqual('bar,foo.bar,quz', out_mask.ToJsonString())
    src = unittest_pb2.TestAllTypes()
    with self.assertRaises(ValueError):
      out_mask.Union(src, mask2)

  def testIntersect(self):
    mask1 = field_mask_pb2.FieldMask()
    mask2 = field_mask_pb2.FieldMask()
    out_mask = field_mask_pb2.FieldMask()
    # Test cases without overlapping.
    mask1.FromJsonString('foo,baz')
    mask2.FromJsonString('bar,quz')
    out_mask.Intersect(mask1, mask2)
    self.assertEqual('', out_mask.ToJsonString())
    # Overlap with duplicated paths.
    mask1.FromJsonString('foo,baz.bb')
    mask2.FromJsonString('baz.bb,quz')
    out_mask.Intersect(mask1, mask2)
    self.assertEqual('baz.bb', out_mask.ToJsonString())
    # Overlap with paths covering some other paths.
    mask1.FromJsonString('foo.bar.baz,quz')
    mask2.FromJsonString('foo.bar,bar')
    out_mask.Intersect(mask1, mask2)
    self.assertEqual('foo.bar.baz', out_mask.ToJsonString())
    mask1.FromJsonString('foo.bar,bar')
    mask2.FromJsonString('foo.bar.baz,quz')
    out_mask.Intersect(mask1, mask2)
    self.assertEqual('foo.bar.baz', out_mask.ToJsonString())

  def testMergeMessageWithoutMapFields(self):
    # Test merge one field.
    src = unittest_pb2.TestAllTypes()
    test_util.SetAllFields(src)
    for field in src.DESCRIPTOR.fields:
      if field.containing_oneof:
        continue
      field_name = field.name
      dst = unittest_pb2.TestAllTypes()
      # Only set one path to mask.
      mask = field_mask_pb2.FieldMask()
      mask.paths.append(field_name)
      mask.MergeMessage(src, dst)
      # The expected result message.
      msg = unittest_pb2.TestAllTypes()
      if field.label == descriptor.FieldDescriptor.LABEL_REPEATED:
        repeated_src = getattr(src, field_name)
        repeated_msg = getattr(msg, field_name)
        if field.cpp_type == descriptor.FieldDescriptor.CPPTYPE_MESSAGE:
          for item in repeated_src:
            repeated_msg.add().CopyFrom(item)
        else:
          repeated_msg.extend(repeated_src)
      elif field.cpp_type == descriptor.FieldDescriptor.CPPTYPE_MESSAGE:
        getattr(msg, field_name).CopyFrom(getattr(src, field_name))
      else:
        setattr(msg, field_name, getattr(src, field_name))
      # Only field specified in mask is merged.
      self.assertEqual(msg, dst)

    # Test merge nested fields.
    nested_src = unittest_pb2.NestedTestAllTypes()
    nested_dst = unittest_pb2.NestedTestAllTypes()
    nested_src.child.payload.optional_int32 = 1234
    nested_src.child.child.payload.optional_int32 = 5678
    mask = field_mask_pb2.FieldMask()
    mask.FromJsonString('child.payload')
    mask.MergeMessage(nested_src, nested_dst)
    self.assertEqual(1234, nested_dst.child.payload.optional_int32)
    self.assertEqual(0, nested_dst.child.child.payload.optional_int32)

    mask.FromJsonString('child.child.payload')
    mask.MergeMessage(nested_src, nested_dst)
    self.assertEqual(1234, nested_dst.child.payload.optional_int32)
    self.assertEqual(5678, nested_dst.child.child.payload.optional_int32)

    nested_dst.Clear()
    mask.FromJsonString('child.child.payload')
    mask.MergeMessage(nested_src, nested_dst)
    self.assertEqual(0, nested_dst.child.payload.optional_int32)
    self.assertEqual(5678, nested_dst.child.child.payload.optional_int32)

    nested_dst.Clear()
    mask.FromJsonString('child')
    mask.MergeMessage(nested_src, nested_dst)
    self.assertEqual(1234, nested_dst.child.payload.optional_int32)
    self.assertEqual(5678, nested_dst.child.child.payload.optional_int32)

    # Test MergeOptions.
    nested_dst.Clear()
    nested_dst.child.payload.optional_int64 = 4321
    # Message fields will be merged by default.
    mask.FromJsonString('child.payload')
    mask.MergeMessage(nested_src, nested_dst)
    self.assertEqual(1234, nested_dst.child.payload.optional_int32)
    self.assertEqual(4321, nested_dst.child.payload.optional_int64)
    # Change the behavior to replace message fields.
    mask.FromJsonString('child.payload')
    mask.MergeMessage(nested_src, nested_dst, True, False)
    self.assertEqual(1234, nested_dst.child.payload.optional_int32)
    self.assertEqual(0, nested_dst.child.payload.optional_int64)

    # By default, fields missing in source are not cleared in destination.
    nested_dst.payload.optional_int32 = 1234
    self.assertTrue(nested_dst.HasField('payload'))
    mask.FromJsonString('payload')
    mask.MergeMessage(nested_src, nested_dst)
    self.assertTrue(nested_dst.HasField('payload'))
    # But they are cleared when replacing message fields.
    nested_dst.Clear()
    nested_dst.payload.optional_int32 = 1234
    mask.FromJsonString('payload')
    mask.MergeMessage(nested_src, nested_dst, True, False)
    self.assertFalse(nested_dst.HasField('payload'))

    nested_src.payload.repeated_int32.append(1234)
    nested_dst.payload.repeated_int32.append(5678)
    # Repeated fields will be appended by default.
    mask.FromJsonString('payload.repeatedInt32')
    mask.MergeMessage(nested_src, nested_dst)
    self.assertEqual(2, len(nested_dst.payload.repeated_int32))
    self.assertEqual(5678, nested_dst.payload.repeated_int32[0])
    self.assertEqual(1234, nested_dst.payload.repeated_int32[1])
    # Change the behavior to replace repeated fields.
    mask.FromJsonString('payload.repeatedInt32')
    mask.MergeMessage(nested_src, nested_dst, False, True)
    self.assertEqual(1, len(nested_dst.payload.repeated_int32))
    self.assertEqual(1234, nested_dst.payload.repeated_int32[0])

    # Test Merge oneof field.
    new_msg = unittest_pb2.TestOneof2()
    dst = unittest_pb2.TestOneof2()
    dst.foo_message.qux_int = 1
    mask = field_mask_pb2.FieldMask()
    mask.FromJsonString('fooMessage,fooLazyMessage.quxInt')
    mask.MergeMessage(new_msg, dst)
    self.assertTrue(dst.HasField('foo_message'))
    self.assertFalse(dst.HasField('foo_lazy_message'))

  def testMergeMessageWithMapField(self):
    empty_map = map_unittest_pb2.TestRecursiveMapMessage()
    src_level_2 = map_unittest_pb2.TestRecursiveMapMessage()
    src_level_2.a['src level 2'].CopyFrom(empty_map)
    src = map_unittest_pb2.TestRecursiveMapMessage()
    src.a['common key'].CopyFrom(src_level_2)
    src.a['src level 1'].CopyFrom(src_level_2)

    dst_level_2 = map_unittest_pb2.TestRecursiveMapMessage()
    dst_level_2.a['dst level 2'].CopyFrom(empty_map)
    dst = map_unittest_pb2.TestRecursiveMapMessage()
    dst.a['common key'].CopyFrom(dst_level_2)
    dst.a['dst level 1'].CopyFrom(empty_map)

    mask = field_mask_pb2.FieldMask()
    mask.FromJsonString('a')
    mask.MergeMessage(src, dst)

    # map from dst is replaced with map from src.
    self.assertEqual(dst.a['common key'], src_level_2)
    self.assertEqual(dst.a['src level 1'], src_level_2)
    self.assertEqual(dst.a['dst level 1'], empty_map)

  def testMergeErrors(self):
    src = unittest_pb2.TestAllTypes()
    dst = unittest_pb2.TestAllTypes()
    mask = field_mask_pb2.FieldMask()
    test_util.SetAllFields(src)
    mask.FromJsonString('optionalInt32.field')
    with self.assertRaises(ValueError) as e:
      mask.MergeMessage(src, dst)
    self.assertEqual('Error: Field optional_int32 in message '
                     'protobuf_unittest.TestAllTypes is not a singular '
                     'message field and cannot have sub-fields.',
                     str(e.exception))

  def testSnakeCaseToCamelCase(self):
    self.assertEqual('fooBar',
                     well_known_types._SnakeCaseToCamelCase('foo_bar'))
    self.assertEqual('FooBar',
                     well_known_types._SnakeCaseToCamelCase('_foo_bar'))
    self.assertEqual('foo3Bar',
                     well_known_types._SnakeCaseToCamelCase('foo3_bar'))

    # No uppercase letter is allowed.
    self.assertRaisesRegexp(
        well_known_types.Error,
        'Fail to print FieldMask to Json string: Path name Foo must '
        'not contain uppercase letters.',
        well_known_types._SnakeCaseToCamelCase,
        'Foo')
    # Any character after a "_" must be a lowercase letter.
    #   1. "_" cannot be followed by another "_".
    #   2. "_" cannot be followed by a digit.
    #   3. "_" cannot appear as the last character.
    self.assertRaisesRegexp(
        well_known_types.Error,
        'Fail to print FieldMask to Json string: The character after a '
        '"_" must be a lowercase letter in path name foo__bar.',
        well_known_types._SnakeCaseToCamelCase,
        'foo__bar')
    self.assertRaisesRegexp(
        well_known_types.Error,
        'Fail to print FieldMask to Json string: The character after a '
        '"_" must be a lowercase letter in path name foo_3bar.',
        well_known_types._SnakeCaseToCamelCase,
        'foo_3bar')
    self.assertRaisesRegexp(
        well_known_types.Error,
        'Fail to print FieldMask to Json string: Trailing "_" in path '
        'name foo_bar_.',
        well_known_types._SnakeCaseToCamelCase,
        'foo_bar_')

  def testCamelCaseToSnakeCase(self):
    self.assertEqual('foo_bar',
                     well_known_types._CamelCaseToSnakeCase('fooBar'))
    self.assertEqual('_foo_bar',
                     well_known_types._CamelCaseToSnakeCase('FooBar'))
    self.assertEqual('foo3_bar',
                     well_known_types._CamelCaseToSnakeCase('foo3Bar'))
    self.assertRaisesRegexp(
        well_known_types.ParseError,
        'Fail to parse FieldMask: Path name foo_bar must not contain "_"s.',
        well_known_types._CamelCaseToSnakeCase,
        'foo_bar')


class StructTest(unittest.TestCase):

  def testStruct(self):
    struct = struct_pb2.Struct()
    self.assertIsInstance(struct, collections.Mapping)
    self.assertEqual(0, len(struct))
    struct_class = struct.__class__

    struct['key1'] = 5
    struct['key2'] = 'abc'
    struct['key3'] = True
    struct.get_or_create_struct('key4')['subkey'] = 11.0
    struct_list = struct.get_or_create_list('key5')
    self.assertIsInstance(struct_list, collections.Sequence)
    struct_list.extend([6, 'seven', True, False, None])
    struct_list.add_struct()['subkey2'] = 9
    struct['key6'] = {'subkey': {}}
    struct['key7'] = [2, False]

    self.assertEqual(7, len(struct))
    self.assertTrue(isinstance(struct, well_known_types.Struct))
    self.assertEqual(5, struct['key1'])
    self.assertEqual('abc', struct['key2'])
    self.assertIs(True, struct['key3'])
    self.assertEqual(11, struct['key4']['subkey'])
    inner_struct = struct_class()
    inner_struct['subkey2'] = 9
    self.assertEqual([6, 'seven', True, False, None, inner_struct],
                     list(struct['key5'].items()))
    self.assertEqual({}, dict(struct['key6']['subkey'].fields))
    self.assertEqual([2, False], list(struct['key7'].items()))

    serialized = struct.SerializeToString()
    struct2 = struct_pb2.Struct()
    struct2.ParseFromString(serialized)

    self.assertEqual(struct, struct2)
    for key, value in struct.items():
      self.assertIn(key, struct)
      self.assertIn(key, struct2)
      self.assertEqual(value, struct2[key])

    self.assertEqual(7, len(struct.keys()))
    self.assertEqual(7, len(struct.values()))
    for key in struct.keys():
      self.assertIn(key, struct)
      self.assertIn(key, struct2)
      self.assertEqual(struct[key], struct2[key])

    item = (next(iter(struct.keys())), next(iter(struct.values())))
    self.assertEqual(item, next(iter(struct.items())))

    self.assertTrue(isinstance(struct2, well_known_types.Struct))
    self.assertEqual(5, struct2['key1'])
    self.assertEqual('abc', struct2['key2'])
    self.assertIs(True, struct2['key3'])
    self.assertEqual(11, struct2['key4']['subkey'])
    self.assertEqual([6, 'seven', True, False, None, inner_struct],
                     list(struct2['key5'].items()))

    struct_list = struct2['key5']
    self.assertEqual(6, struct_list[0])
    self.assertEqual('seven', struct_list[1])
    self.assertEqual(True, struct_list[2])
    self.assertEqual(False, struct_list[3])
    self.assertEqual(None, struct_list[4])
    self.assertEqual(inner_struct, struct_list[5])

    struct_list[1] = 7
    self.assertEqual(7, struct_list[1])

    struct_list.add_list().extend([1, 'two', True, False, None])
    self.assertEqual([1, 'two', True, False, None],
                     list(struct_list[6].items()))
    struct_list.extend([{'nested_struct': 30}, ['nested_list', 99], {}, []])
    self.assertEqual(11, len(struct_list.values))
    self.assertEqual(30, struct_list[7]['nested_struct'])
    self.assertEqual('nested_list', struct_list[8][0])
    self.assertEqual(99, struct_list[8][1])
    self.assertEqual({}, dict(struct_list[9].fields))
    self.assertEqual([], list(struct_list[10].items()))
    struct_list[0] = {'replace': 'set'}
    struct_list[1] = ['replace', 'set']
    self.assertEqual('set', struct_list[0]['replace'])
    self.assertEqual(['replace', 'set'], list(struct_list[1].items()))

    text_serialized = str(struct)
    struct3 = struct_pb2.Struct()
    text_format.Merge(text_serialized, struct3)
    self.assertEqual(struct, struct3)

    struct.get_or_create_struct('key3')['replace'] = 12
    self.assertEqual(12, struct['key3']['replace'])

    # Tests empty list.
    struct.get_or_create_list('empty_list')
    empty_list = struct['empty_list']
    self.assertEqual([], list(empty_list.items()))
    list2 = struct_pb2.ListValue()
    list2.add_list()
    empty_list = list2[0]
    self.assertEqual([], list(empty_list.items()))

    # Tests empty struct.
    struct.get_or_create_struct('empty_struct')
    empty_struct = struct['empty_struct']
    self.assertEqual({}, dict(empty_struct.fields))
    list2.add_struct()
    empty_struct = list2[1]
    self.assertEqual({}, dict(empty_struct.fields))

    self.assertEqual(9, len(struct))
    del struct['key3']
    del struct['key4']
    self.assertEqual(7, len(struct))
    self.assertEqual(6, len(struct['key5']))
    del struct['key5'][1]
    self.assertEqual(5, len(struct['key5']))
    self.assertEqual([6, True, False, None, inner_struct],
                     list(struct['key5'].items()))

  def testMergeFrom(self):
    struct = struct_pb2.Struct()
    struct_class = struct.__class__

    dictionary = {
        'key1': 5,
        'key2': 'abc',
        'key3': True,
        'key4': {'subkey': 11.0},
        'key5': [6, 'seven', True, False, None, {'subkey2': 9}],
        'key6': [['nested_list', True]],
        'empty_struct': {},
        'empty_list': []
    }
    struct.update(dictionary)
    self.assertEqual(5, struct['key1'])
    self.assertEqual('abc', struct['key2'])
    self.assertIs(True, struct['key3'])
    self.assertEqual(11, struct['key4']['subkey'])
    inner_struct = struct_class()
    inner_struct['subkey2'] = 9
    self.assertEqual([6, 'seven', True, False, None, inner_struct],
                     list(struct['key5'].items()))
    self.assertEqual(2, len(struct['key6'][0].values))
    self.assertEqual('nested_list', struct['key6'][0][0])
    self.assertEqual(True, struct['key6'][0][1])
    empty_list = struct['empty_list']
    self.assertEqual([], list(empty_list.items()))
    empty_struct = struct['empty_struct']
    self.assertEqual({}, dict(empty_struct.fields))

    # According to documentation: "When parsing from the wire or when merging,
    # if there are duplicate map keys the last key seen is used".
    duplicate = {
        'key4': {'replace': 20},
        'key5': [[False, 5]]
    }
    struct.update(duplicate)
    self.assertEqual(1, len(struct['key4'].fields))
    self.assertEqual(20, struct['key4']['replace'])
    self.assertEqual(1, len(struct['key5'].values))
    self.assertEqual(False, struct['key5'][0][0])
    self.assertEqual(5, struct['key5'][0][1])


class AnyTest(unittest.TestCase):

  def testAnyMessage(self):
    # Creates and sets message.
    msg = any_test_pb2.TestAny()
    msg_descriptor = msg.DESCRIPTOR
    all_types = unittest_pb2.TestAllTypes()
    all_descriptor = all_types.DESCRIPTOR
    all_types.repeated_string.append(u'\u00fc\ua71f')
    # Packs to Any.
    msg.value.Pack(all_types)
    self.assertEqual(msg.value.type_url,
                     'type.googleapis.com/%s' % all_descriptor.full_name)
    self.assertEqual(msg.value.value,
                     all_types.SerializeToString())
    # Tests Is() method.
    self.assertTrue(msg.value.Is(all_descriptor))
    self.assertFalse(msg.value.Is(msg_descriptor))
    # Unpacks Any.
    unpacked_message = unittest_pb2.TestAllTypes()
    self.assertTrue(msg.value.Unpack(unpacked_message))
    self.assertEqual(all_types, unpacked_message)
    # Unpacks to different type.
    self.assertFalse(msg.value.Unpack(msg))
    # Only Any messages have Pack method.
    try:
      msg.Pack(all_types)
    except AttributeError:
      pass
    else:
      raise AttributeError('%s should not have Pack method.' %
                           msg_descriptor.full_name)

  def testUnpackWithNoSlashInTypeUrl(self):
    msg = any_test_pb2.TestAny()
    all_types = unittest_pb2.TestAllTypes()
    all_descriptor = all_types.DESCRIPTOR
    msg.value.Pack(all_types)
    # Reset type_url to part of type_url after '/'
    msg.value.type_url = msg.value.TypeName()
    self.assertFalse(msg.value.Is(all_descriptor))
    unpacked_message = unittest_pb2.TestAllTypes()
    self.assertFalse(msg.value.Unpack(unpacked_message))

  def testMessageName(self):
    # Creates and sets message.
    submessage = any_test_pb2.TestAny()
    submessage.int_value = 12345
    msg = any_pb2.Any()
    msg.Pack(submessage)
    self.assertEqual(msg.TypeName(), 'google.protobuf.internal.TestAny')

  def testPackWithCustomTypeUrl(self):
    submessage = any_test_pb2.TestAny()
    submessage.int_value = 12345
    msg = any_pb2.Any()
    # Pack with a custom type URL prefix.
    msg.Pack(submessage, 'type.myservice.com')
    self.assertEqual(msg.type_url,
                     'type.myservice.com/%s' % submessage.DESCRIPTOR.full_name)
    # Pack with a custom type URL prefix ending with '/'.
    msg.Pack(submessage, 'type.myservice.com/')
    self.assertEqual(msg.type_url,
                     'type.myservice.com/%s' % submessage.DESCRIPTOR.full_name)
    # Pack with an empty type URL prefix.
    msg.Pack(submessage, '')
    self.assertEqual(msg.type_url,
                     '/%s' % submessage.DESCRIPTOR.full_name)
    # Test unpacking the type.
    unpacked_message = any_test_pb2.TestAny()
    self.assertTrue(msg.Unpack(unpacked_message))
    self.assertEqual(submessage, unpacked_message)

  def testPackDeterministic(self):
    submessage = any_test_pb2.TestAny()
    for i in range(10):
      submessage.map_value[str(i)] = i * 2
    msg = any_pb2.Any()
    msg.Pack(submessage, deterministic=True)
    serialized = msg.SerializeToString(deterministic=True)
    golden = (b'\n4type.googleapis.com/google.protobuf.internal.TestAny\x12F'
              b'\x1a\x05\n\x010\x10\x00\x1a\x05\n\x011\x10\x02\x1a\x05\n\x01'
              b'2\x10\x04\x1a\x05\n\x013\x10\x06\x1a\x05\n\x014\x10\x08\x1a'
              b'\x05\n\x015\x10\n\x1a\x05\n\x016\x10\x0c\x1a\x05\n\x017\x10'
              b'\x0e\x1a\x05\n\x018\x10\x10\x1a\x05\n\x019\x10\x12')
    self.assertEqual(golden, serialized)


if __name__ == '__main__':
  unittest.main()
