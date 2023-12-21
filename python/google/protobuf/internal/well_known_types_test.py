# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Test for google.protobuf.internal.well_known_types."""

__author__ = 'jieluo@google.com (Jie Luo)'

import collections.abc as collections_abc
import datetime
import unittest

from google.protobuf import any_pb2
from google.protobuf.internal import any_test_pb2
from google.protobuf import duration_pb2
from google.protobuf import struct_pb2
from google.protobuf import timestamp_pb2
from google.protobuf.internal import well_known_types
from google.protobuf import text_format
from google.protobuf.internal import _parameterized
from google.protobuf import unittest_pb2

try:
  # New module in Python 3.9:
  import zoneinfo  # pylint:disable=g-import-not-at-top
  _TZ_JAPAN = zoneinfo.ZoneInfo('Japan')
  _TZ_PACIFIC = zoneinfo.ZoneInfo('US/Pacific')
  has_zoneinfo = True
except ImportError:
  _TZ_JAPAN = datetime.timezone(datetime.timedelta(hours=9), 'Japan')
  _TZ_PACIFIC = datetime.timezone(datetime.timedelta(hours=-8), 'US/Pacific')
  has_zoneinfo = False


class TimeUtilTestBase(_parameterized.TestCase):

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

  def testTimezoneNaiveDatetimeConversionNearEpoch(self):
    message = timestamp_pb2.Timestamp()
    naive_utc_epoch = datetime.datetime(1970, 1, 1)
    message.FromDatetime(naive_utc_epoch)
    self.assertEqual(0, message.seconds)
    self.assertEqual(0, message.nanos)
    self.assertEqual(naive_utc_epoch, message.ToDatetime())

    naive_epoch_morning = datetime.datetime(1970, 1, 1, 8, 0, 0, 1)
    message.FromDatetime(naive_epoch_morning)
    self.assertEqual(8 * 3600, message.seconds)
    self.assertEqual(1000, message.nanos)
    self.assertEqual(naive_epoch_morning, message.ToDatetime())

    message.FromMilliseconds(1999)
    self.assertEqual(1, message.seconds)
    self.assertEqual(999_000_000, message.nanos)

    self.assertEqual(datetime.datetime(1970, 1, 1, 0, 0, 1, 999000),
                     message.ToDatetime())

  def testTimezoneNaiveDatetimeConversionWhereTimestampLosesPrecision(self):
    ts = timestamp_pb2.Timestamp()
    naive_future = datetime.datetime(2555, 2, 22, 1, 2, 3, 456789)
    # The float timestamp for this datetime does not represent the integer
    # millisecond value with full precision.
    self.assertNotEqual(
        naive_future.astimezone(datetime.timezone.utc),
        datetime.datetime.fromtimestamp(
            naive_future.timestamp(), datetime.timezone.utc
        ),
    )
    # It still round-trips correctly.
    ts.FromDatetime(naive_future)
    self.assertEqual(naive_future, ts.ToDatetime())

  def testTimezoneNaiveMaxDatetimeConversion(self):
    ts = timestamp_pb2.Timestamp()
    naive_max_datetime = datetime.datetime(9999, 12, 31, 23, 59, 59, 999999)
    ts.FromDatetime(naive_max_datetime)
    self.assertEqual(naive_max_datetime, ts.ToDatetime())

  def testTimezoneNaiveMinDatetimeConversion(self):
    ts = timestamp_pb2.Timestamp()
    naive_min_datetime = datetime.datetime(1, 1, 1)
    ts.FromDatetime(naive_min_datetime)
    self.assertEqual(naive_min_datetime, ts.ToDatetime())

  # Two hours after the Unix Epoch, around the world.
  @_parameterized.named_parameters(
      ('London', [1970, 1, 1, 2], datetime.timezone.utc),
      ('Tokyo', [1970, 1, 1, 11], _TZ_JAPAN),
      ('LA', [1969, 12, 31, 18], _TZ_PACIFIC),
  )
  def testTimezoneAwareDatetimeConversion(self, date_parts, tzinfo):
    original_datetime = datetime.datetime(*date_parts, tzinfo=tzinfo)  # pylint:disable=g-tzinfo-datetime

    message = timestamp_pb2.Timestamp()
    message.FromDatetime(original_datetime)
    self.assertEqual(7200, message.seconds)
    self.assertEqual(0, message.nanos)

    # ToDatetime() with no parameters produces a naive UTC datetime, i.e. it not
    # only loses the original timezone information (e.g. US/Pacific) as it's
    # "normalised" to UTC, but also drops the information that the datetime
    # represents a UTC one.
    naive_datetime = message.ToDatetime()
    self.assertEqual(datetime.datetime(1970, 1, 1, 2), naive_datetime)
    self.assertIsNone(naive_datetime.tzinfo)
    self.assertNotEqual(original_datetime, naive_datetime)  # not even for UTC!

    # In contrast, ToDatetime(tzinfo=) produces an aware datetime in the given
    # timezone.
    aware_datetime = message.ToDatetime(tzinfo=tzinfo)
    self.assertEqual(original_datetime, aware_datetime)
    self.assertEqual(
        datetime.datetime(1970, 1, 1, 2, tzinfo=datetime.timezone.utc),
        aware_datetime)
    self.assertEqual(tzinfo, aware_datetime.tzinfo)

  @unittest.skipIf(
      not has_zoneinfo,
      'Versions without zoneinfo use a fixed-offset timezone that does not'
      ' demonstrate this problem.',
  )
  def testDatetimeConversionWithDifferentUtcOffsetThanEpoch(self):
    # This timezone has a different UTC offset at this date than at the epoch.
    # The datetime returned by FromDatetime needs to have the correct offset
    # for the moment represented.
    tz = _TZ_PACIFIC
    dt = datetime.datetime(2016, 6, 26, tzinfo=tz)
    epoch_dt = datetime.datetime.fromtimestamp(
        0, tz=datetime.timezone.utc
    ).astimezone(tz)
    self.assertNotEqual(dt.utcoffset(), epoch_dt.utcoffset())
    ts = timestamp_pb2.Timestamp()
    ts.FromDatetime(dt)
    self.assertEqual(dt, ts.ToDatetime(tzinfo=dt.tzinfo))

  def testTimezoneAwareDatetimeConversionWhereTimestampLosesPrecision(self):
    tz = _TZ_PACIFIC
    ts = timestamp_pb2.Timestamp()
    tz_aware_future = datetime.datetime(2555, 2, 22, 1, 2, 3, 456789, tzinfo=tz)
    # The float timestamp for this datetime does not represent the integer
    # millisecond value with full precision.
    self.assertNotEqual(
        tz_aware_future,
        datetime.datetime.fromtimestamp(tz_aware_future.timestamp(), tz),
    )
    # It still round-trips correctly.
    ts.FromDatetime(tz_aware_future)
    self.assertEqual(tz_aware_future, ts.ToDatetime(tz))

  def testTimezoneAwareMaxDatetimeConversion(self):
    ts = timestamp_pb2.Timestamp()
    tz_aware_max_datetime = datetime.datetime(
        9999, 12, 31, 23, 59, 59, 999999, tzinfo=datetime.timezone.utc
    )
    ts.FromDatetime(tz_aware_max_datetime)
    self.assertEqual(
        tz_aware_max_datetime, ts.ToDatetime(datetime.timezone.utc)
    )

  def testTimezoneAwareMinDatetimeConversion(self):
    ts = timestamp_pb2.Timestamp()
    tz_aware_min_datetime = datetime.datetime(
        1, 1, 1, tzinfo=datetime.timezone.utc
    )
    ts.FromDatetime(tz_aware_min_datetime)
    self.assertEqual(
        tz_aware_min_datetime, ts.ToDatetime(datetime.timezone.utc)
    )

  def testNanosOneSecond(self):
    # TODO: b/301980950 - Test error behavior instead once ToDatetime validates
    # that nanos are in expected range.
    tz = _TZ_PACIFIC
    ts = timestamp_pb2.Timestamp(nanos=1_000_000_000)
    self.assertEqual(ts.ToDatetime(), datetime.datetime(1970, 1, 1, 0, 0, 1))
    self.assertEqual(
        ts.ToDatetime(tz), datetime.datetime(1969, 12, 31, 16, 0, 1, tzinfo=tz)
    )

  def testNanosNegativeOneSecond(self):
    # TODO: b/301980950 - Test error behavior instead once ToDatetime validates
    # that nanos are in expected range.
    tz = _TZ_PACIFIC
    ts = timestamp_pb2.Timestamp(nanos=-1_000_000_000)
    self.assertEqual(
        ts.ToDatetime(), datetime.datetime(1969, 12, 31, 23, 59, 59)
    )
    self.assertEqual(
        ts.ToDatetime(tz),
        datetime.datetime(1969, 12, 31, 15, 59, 59, tzinfo=tz),
    )

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
    self.assertRaisesRegex(
        ValueError, 'Failed to parse timestamp: missing valid timezone offset.',
        message.FromJsonString, '')
    self.assertRaisesRegex(
        ValueError, 'Failed to parse timestamp: invalid trailing data '
        '1970-01-01T00:00:01Ztrail.', message.FromJsonString,
        '1970-01-01T00:00:01Ztrail')
    self.assertRaisesRegex(
        ValueError, 'time data \'10000-01-01T00:00:00\' does not match'
        ' format \'%Y-%m-%dT%H:%M:%S\'', message.FromJsonString,
        '10000-01-01T00:00:00.00Z')
    self.assertRaisesRegex(
        ValueError, 'nanos 0123456789012 more than 9 fractional digits.',
        message.FromJsonString, '1970-01-01T00:00:00.0123456789012Z')
    self.assertRaisesRegex(
        ValueError,
        (r'Invalid timezone offset value: \+08.'),
        message.FromJsonString,
        '1972-01-01T01:00:00.01+08',
    )
    self.assertRaisesRegex(ValueError, 'year (0 )?is out of range',
                           message.FromJsonString, '0000-01-01T00:00:00Z')
    message.seconds = 253402300800
    self.assertRaisesRegex(OverflowError, 'date value out of range',
                           message.ToJsonString)

  def testInvalidDuration(self):
    message = duration_pb2.Duration()
    self.assertRaisesRegex(ValueError, 'Duration must end with letter "s": 1.',
                           message.FromJsonString, '1')
    self.assertRaisesRegex(ValueError, 'Couldn\'t parse duration: 1...2s.',
                           message.FromJsonString, '1...2s')
    text = '-315576000001.000000000s'
    self.assertRaisesRegex(
        ValueError,
        r'Duration is not valid\: Seconds -315576000001 must be in range'
        r' \[-315576000000\, 315576000000\].', message.FromJsonString, text)
    text = '315576000001.000000000s'
    self.assertRaisesRegex(
        ValueError,
        r'Duration is not valid\: Seconds 315576000001 must be in range'
        r' \[-315576000000\, 315576000000\].', message.FromJsonString, text)
    message.seconds = -315576000001
    message.nanos = 0
    self.assertRaisesRegex(
        ValueError,
        r'Duration is not valid\: Seconds -315576000001 must be in range'
        r' \[-315576000000\, 315576000000\].', message.ToJsonString)
    message.seconds = 0
    message.nanos = 999999999 + 1
    self.assertRaisesRegex(
        ValueError, r'Duration is not valid\: Nanos 1000000000 must be in range'
        r' \[-999999999\, 999999999\].', message.ToJsonString)
    message.seconds = -1
    message.nanos = 1
    self.assertRaisesRegex(ValueError,
                           r'Duration is not valid\: Sign mismatch.',
                           message.ToJsonString)


class StructTest(unittest.TestCase):

  def testStruct(self):
    struct = struct_pb2.Struct()
    self.assertIsInstance(struct, collections_abc.Mapping)
    self.assertEqual(0, len(struct))
    struct_class = struct.__class__

    struct['key1'] = 5
    struct['key2'] = 'abc'
    struct['key3'] = True
    struct.get_or_create_struct('key4')['subkey'] = 11.0
    struct_list = struct.get_or_create_list('key5')
    self.assertIsInstance(struct_list, collections_abc.Sequence)
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

  def testStructAssignment(self):
    # Tests struct assignment from another struct
    s1 = struct_pb2.Struct()
    s2 = struct_pb2.Struct()
    for value in [1, 'a', [1], ['a'], {'a': 'b'}]:
      s1['x'] = value
      s2['x'] = s1['x']
      self.assertEqual(s1['x'], s2['x'])

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
        'empty_list': [],
        'tuple': ((3,2), ())
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
