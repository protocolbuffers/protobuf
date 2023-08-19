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

import collections.abc as collections_abc
import datetime

import pytest

from google.protobuf import any_pb2
from google.protobuf import duration_pb2
from google.protobuf import struct_pb2
from google.protobuf import timestamp_pb2
from google.protobuf.internal import any_test_pb2
from google.protobuf.internal import well_known_types
from google.protobuf import text_format
from google.protobuf import unittest_pb2

try:
    # New module in Python 3.9:
    import zoneinfo  # pylint:disable=g-import-not-at-top
    _TZ_JAPAN = zoneinfo.ZoneInfo('Japan')
    _TZ_PACIFIC = zoneinfo.ZoneInfo('US/Pacific')
except ImportError:
    _TZ_JAPAN = datetime.timezone(datetime.timedelta(hours=9), 'Japan')
    _TZ_PACIFIC = datetime.timezone(datetime.timedelta(hours=-8), 'US/Pacific')


class TestTimeUtilBase:
    def check_timestamp_conversion(self, message, text):
        assert text == message.ToJsonString()
        parsed_message = timestamp_pb2.Timestamp()
        parsed_message.FromJsonString(text)
        assert message == parsed_message

    def check_duration_conversion(self, message, text):
        assert text == message.ToJsonString()
        parsed_message = duration_pb2.Duration()
        parsed_message.FromJsonString(text)
        assert message == parsed_message


class TestTimeUtil(TestTimeUtilBase):
    def test_timestamp_serialize_and_parse(self):
        message = timestamp_pb2.Timestamp()
        # Generated output should contain 3, 6, or 9 fractional digits.
        message.seconds = 0
        message.nanos = 0
        self.check_timestamp_conversion(message, '1970-01-01T00:00:00Z')
        message.nanos = 10000000
        self.check_timestamp_conversion(message, '1970-01-01T00:00:00.010Z')
        message.nanos = 10000
        self.check_timestamp_conversion(message, '1970-01-01T00:00:00.000010Z')
        message.nanos = 10
        self.check_timestamp_conversion(message, '1970-01-01T00:00:00.000000010Z')
        # Test min timestamps.
        message.seconds = -62135596800
        message.nanos = 0
        self.check_timestamp_conversion(message, '0001-01-01T00:00:00Z')
        # Test max timestamps.
        message.seconds = 253402300799
        message.nanos = 999999999
        self.check_timestamp_conversion(message, '9999-12-31T23:59:59.999999999Z')
        # Test negative timestamps.
        message.seconds = -1
        self.check_timestamp_conversion(message, '1969-12-31T23:59:59.999999999Z')

        # Parsing accepts an fractional digits as long as they fit into nano
        # precision.
        message.FromJsonString('1970-01-01T00:00:00.1Z')
        assert 0 == message.seconds
        assert 100000000 == message.nanos
        # Parsing accepts offsets.
        message.FromJsonString('1970-01-01T00:00:00-08:00')
        assert 8 * 3600 == message.seconds
        assert 0 == message.nanos

        # It is not easy to check with current time. For test coverage only.
        message.GetCurrentTime()
        assert 8 * 3600 != message.seconds

    def test_duration_serialize_and_parse(self):
        message = duration_pb2.Duration()
        # Generated output should contain 3, 6, or 9 fractional digits.
        message.seconds = 0
        message.nanos = 0
        self.check_duration_conversion(message, '0s')
        message.nanos = 10000000
        self.check_duration_conversion(message, '0.010s')
        message.nanos = 10000
        self.check_duration_conversion(message, '0.000010s')
        message.nanos = 10
        self.check_duration_conversion(message, '0.000000010s')

        # Test min and max
        message.seconds = 315576000000
        message.nanos = 999999999
        self.check_duration_conversion(message, '315576000000.999999999s')
        message.seconds = -315576000000
        message.nanos = -999999999
        self.check_duration_conversion(message, '-315576000000.999999999s')

        # Parsing accepts an fractional digits as long as they fit into nano
        # precision.
        message.FromJsonString('0.1s')
        assert 100000000 == message.nanos
        message.FromJsonString('0.0000001s')
        assert 100 == message.nanos

    def test_timestamp_integer_conversion(self):
        message = timestamp_pb2.Timestamp()
        message.FromNanoseconds(1)
        assert '1970-01-01T00:00:00.000000001Z' == message.ToJsonString()
        assert 1 == message.ToNanoseconds()

        message.FromNanoseconds(-1)
        assert '1969-12-31T23:59:59.999999999Z' == message.ToJsonString()
        assert -1 == message.ToNanoseconds()

        message.FromMicroseconds(1)
        assert '1970-01-01T00:00:00.000001Z' == message.ToJsonString()
        assert 1 == message.ToMicroseconds()

        message.FromMicroseconds(-1)
        assert '1969-12-31T23:59:59.999999Z' == message.ToJsonString()
        assert -1 == message.ToMicroseconds()

        message.FromMilliseconds(1)
        assert '1970-01-01T00:00:00.001Z' == message.ToJsonString()
        assert 1 == message.ToMilliseconds()

        message.FromMilliseconds(-1)
        assert '1969-12-31T23:59:59.999Z' == message.ToJsonString()
        assert -1 == message.ToMilliseconds()

        message.FromSeconds(1)
        assert '1970-01-01T00:00:01Z' == message.ToJsonString()
        assert 1 == message.ToSeconds()

        message.FromSeconds(-1)
        assert '1969-12-31T23:59:59Z' == message.ToJsonString()
        assert -1 == message.ToSeconds()

        message.FromNanoseconds(1999)
        assert 1 == message.ToMicroseconds()
        # For negative values, Timestamp will be rounded down.
        # For example, "1969-12-31T23:59:59.5Z" (i.e., -0.5s) rounded to seconds
        # will be "1969-12-31T23:59:59Z" (i.e., -1s) rather than
        # "1970-01-01T00:00:00Z" (i.e., 0s).
        message.FromNanoseconds(-1999)
        assert -2 == message.ToMicroseconds()

    def test_duration_integer_conversion(self):
        message = duration_pb2.Duration()
        message.FromNanoseconds(1)
        assert '0.000000001s' == message.ToJsonString()
        assert 1 == message.ToNanoseconds()

        message.FromNanoseconds(-1)
        assert '-0.000000001s' == message.ToJsonString()
        assert -1 == message.ToNanoseconds()

        message.FromMicroseconds(1)
        assert '0.000001s' == message.ToJsonString()
        assert 1 == message.ToMicroseconds()

        message.FromMicroseconds(-1)
        assert '-0.000001s' == message.ToJsonString()
        assert -1 == message.ToMicroseconds()

        message.FromMilliseconds(1)
        assert '0.001s' == message.ToJsonString()
        assert 1 == message.ToMilliseconds()

        message.FromMilliseconds(-1)
        assert '-0.001s' == message.ToJsonString()
        assert -1 == message.ToMilliseconds()

        message.FromSeconds(1)
        assert '1s' == message.ToJsonString()
        assert 1 == message.ToSeconds()

        message.FromSeconds(-1)
        assert '-1s' == message.ToJsonString()
        assert -1 == message.ToSeconds()

        # Test truncation behavior.
        message.FromNanoseconds(1999)
        assert 1 == message.ToMicroseconds()

        # For negative values, Duration will be rounded towards 0.
        message.FromNanoseconds(-1999)
        assert -1 == message.ToMicroseconds()

    def test_timezone_naive_datetime_conversion(self):
        message = timestamp_pb2.Timestamp()
        naive_utc_epoch = datetime.datetime(1970, 1, 1)
        message.FromDatetime(naive_utc_epoch)
        assert 0 == message.seconds
        assert 0 == message.nanos

        assert naive_utc_epoch == message.ToDatetime()

        naive_epoch_morning = datetime.datetime(1970, 1, 1, 8, 0, 0, 1)
        message.FromDatetime(naive_epoch_morning)
        assert 8 * 3600 == message.seconds
        assert 1000 == message.nanos

        assert naive_epoch_morning == message.ToDatetime()

        message.FromMilliseconds(1999)
        assert 1 == message.seconds
        assert 999_000_000 == message.nanos

        assert datetime.datetime(1970, 1, 1, 0, 0, 1, 999000) == message.ToDatetime()

        naive_future = datetime.datetime(2555, 2, 22, 1, 2, 3, 456789)
        message.FromDatetime(naive_future)
        assert naive_future == message.ToDatetime()

        naive_end_of_time = datetime.datetime.max
        message.FromDatetime(naive_end_of_time)
        assert naive_end_of_time == message.ToDatetime()

    # Two hours after the Unix Epoch, around the world.
    @pytest.mark.parametrize('date_parts,tzinfo', [
        ([1970, 1, 1, 2], datetime.timezone.utc),
        ([1970, 1, 1, 11], _TZ_JAPAN),
        ([1969, 12, 31, 18], _TZ_PACIFIC)
    ], ids=['London', 'Tokyo', 'LA'])
    def test_timezone_aware_datetime_conversion(self, date_parts, tzinfo):
        original_datetime = datetime.datetime(*date_parts, tzinfo=tzinfo)  # pylint:disable=g-tzinfo-datetime

        message = timestamp_pb2.Timestamp()
        message.FromDatetime(original_datetime)
        assert 7200 == message.seconds
        assert 0 == message.nanos

        # ToDatetime() with no parameters produces a naive UTC datetime, i.e. it not
        # only loses the original timezone information (e.g. US/Pacific) as it's
        # "normalised" to UTC, but also drops the information that the datetime
        # represents a UTC one.
        naive_datetime = message.ToDatetime()
        assert datetime.datetime(1970, 1, 1, 2) == naive_datetime
        assert naive_datetime.tzinfo is None
        assert original_datetime != naive_datetime  # not even for UTC!

        # In contrast, ToDatetime(tzinfo=) produces an aware datetime in the given
        # timezone.
        aware_datetime = message.ToDatetime(tzinfo=tzinfo)
        assert original_datetime == aware_datetime
        assert datetime.datetime(1970, 1, 1, 2, tzinfo=datetime.timezone.utc) == aware_datetime
        assert tzinfo == aware_datetime.tzinfo

    def test_timedelta_conversion(self):
        message = duration_pb2.Duration()
        message.FromNanoseconds(1999999999)
        td = message.ToTimedelta()
        assert 1 == td.seconds
        assert 999999 == td.microseconds

        message.FromNanoseconds(-1999999999)
        td = message.ToTimedelta()
        assert -1 == td.days
        assert 86398 == td.seconds
        assert 1 == td.microseconds

        message.FromMicroseconds(-1)
        td = message.ToTimedelta()
        assert -1 == td.days
        assert 86399 == td.seconds
        assert 999999 == td.microseconds
        converted_message = duration_pb2.Duration()
        converted_message.FromTimedelta(td)
        assert message == converted_message

    def test_invalid_timestamp(self):
        message = timestamp_pb2.Timestamp()
        with pytest.raises(ValueError, match='Failed to parse timestamp: missing valid timezone offset.'):
            message.FromJsonString('')
        with pytest.raises(ValueError, match='Failed to parse timestamp: invalid trailing data 1970-01-01T00:00:01Ztrail.'):
            message.FromJsonString('1970-01-01T00:00:01Ztrail')
        with pytest.raises(ValueError, match="time data '10000-01-01T00:00:00' does not match format '%Y-%m-%dT%H:%M:%S'"):
            message.FromJsonString('10000-01-01T00:00:00.00Z')
        with pytest.raises(ValueError, match='nanos 0123456789012 more than 9 fractional digits.'):
            message.FromJsonString('1970-01-01T00:00:00.0123456789012Z')
        with pytest.raises(ValueError, match=r'Invalid timezone offset value: \+08.'):
            message.FromJsonString('1972-01-01T01:00:00.01+08')   
        with pytest.raises(ValueError, match='year (0 )?is out of range'):
            message.FromJsonString('0000-01-01T00:00:00Z')         
        message.seconds = 253402300800
        with pytest.raises(OverflowError, match="date value out of range"):
            message.ToJsonString()

    def test_invalid_duration(self):
        message = duration_pb2.Duration()
        with pytest.raises(ValueError, match='Duration must end with letter "s": 1.'):
            message.FromJsonString('1')
        with pytest.raises(ValueError, match='Couldn\'t parse duration: 1...2s.'):
            message.FromJsonString('1...2s')
        text = '-315576000001.000000000s'
        with pytest.raises(ValueError, match=r'Duration is not valid\: Seconds -315576000001 must be in range \[-315576000000\, 315576000000\].'):
            message.FromJsonString(text)
        text = '315576000001.000000000s'
        with pytest.raises(ValueError, match=r'Duration is not valid\: Seconds 315576000001 must be in range \[-315576000000\, 315576000000\].'):
            message.FromJsonString(text)
        message.seconds = -315576000001
        message.nanos = 0
        with pytest.raises(ValueError, match=r'Duration is not valid\: Seconds -315576000001 must be in range \[-315576000000\, 315576000000\].'):
            message.ToJsonString()
        message.seconds = 0
        message.nanos = 999999999 + 1
        with pytest.raises(ValueError, match=r'Duration is not valid\: Nanos 1000000000 must be in range \[-999999999\, 999999999\].'):
            message.ToJsonString()
        message.seconds = -1
        message.nanos = 1
        with pytest.raises(ValueError, match=r'Duration is not valid\: Sign mismatch.'):
            message.ToJsonString()


class TestStruct:
    def test_struct(self):
        struct = struct_pb2.Struct()
        assert isinstance(struct, collections_abc.Mapping)
        assert 0 == len(struct)
        struct_class = struct.__class__

        struct['key1'] = 5
        struct['key2'] = 'abc'
        struct['key3'] = True
        struct.get_or_create_struct('key4')['subkey'] = 11.0
        struct_list = struct.get_or_create_list('key5')
        assert isinstance(struct_list, collections_abc.Sequence)
        struct_list.extend([6, 'seven', True, False, None])
        struct_list.add_struct()['subkey2'] = 9
        struct['key6'] = {'subkey': {}}
        struct['key7'] = [2, False]

        assert 7 == len(struct)
        assert isinstance(struct, well_known_types.Struct)
        assert 5 == struct['key1']
        assert 'abc' == struct['key2']
        assert struct['key3'] is True
        assert 11 == struct['key4']['subkey']
        inner_struct = struct_class()
        inner_struct['subkey2'] = 9
        assert [6, 'seven', True, False, None, inner_struct] == list(struct['key5'].items())
        assert {} == dict(struct['key6']['subkey'].fields)
        assert [2, False] == list(struct['key7'].items())

        serialized = struct.SerializeToString()
        struct2 = struct_pb2.Struct()
        struct2.ParseFromString(serialized)

        assert struct == struct2
        for key, value in struct.items():
            assert key in struct
            assert key in struct2
            assert value == struct2[key]

        assert 7 == len(struct.keys())
        assert 7 == len(struct.values())
        for key in struct.keys():
            assert key in struct
            assert key in struct2
            assert struct[key] == struct2[key]

        item = (next(iter(struct.keys())), next(iter(struct.values())))
        assert item == next(iter(struct.items()))

        assert isinstance(struct2, well_known_types.Struct)
        assert 5 == struct2['key1']
        assert 'abc' == struct2['key2']
        assert struct2['key3'] is True
        assert 11 == struct2['key4']['subkey']
        assert [6, 'seven', True, False, None, inner_struct] == list(struct2['key5'].items())

        struct_list = struct2['key5']
        assert 6 == struct_list[0]
        assert 'seven' == struct_list[1]
        assert struct_list[2] is True
        assert struct_list[3] is False
        assert struct_list[4] is None
        assert inner_struct == struct_list[5]

        struct_list[1] = 7
        assert 7 == struct_list[1]

        struct_list.add_list().extend([1, 'two', True, False, None])
        assert [1, 'two', True, False, None] == list(struct_list[6].items())
        struct_list.extend([{'nested_struct': 30}, ['nested_list', 99], {}, []])
        assert 11 == len(struct_list.values)
        assert 30 == struct_list[7]['nested_struct']
        assert 'nested_list' == struct_list[8][0]
        assert 99 == struct_list[8][1]
        assert {} == dict(struct_list[9].fields)
        assert [] == list(struct_list[10].items())
        struct_list[0] = {'replace': 'set'}
        struct_list[1] = ['replace', 'set']
        assert 'set' == struct_list[0]['replace']
        assert ['replace', 'set'] == list(struct_list[1].items())

        text_serialized = str(struct)
        struct3 = struct_pb2.Struct()
        text_format.Merge(text_serialized, struct3)
        assert struct == struct3

        struct.get_or_create_struct('key3')['replace'] = 12
        assert 12 == struct['key3']['replace']

        # Tests empty list.
        struct.get_or_create_list('empty_list')
        empty_list = struct['empty_list']
        assert [] == list(empty_list.items())
        list2 = struct_pb2.ListValue()
        list2.add_list()
        empty_list = list2[0]
        assert [] == list(empty_list.items())

        # Tests empty struct.
        struct.get_or_create_struct('empty_struct')
        empty_struct = struct['empty_struct']
        assert {} == dict(empty_struct.fields)
        list2.add_struct()
        empty_struct = list2[1]
        assert {} == dict(empty_struct.fields)

        assert 9 == len(struct)
        del struct['key3']
        del struct['key4']
        assert 7 == len(struct)
        assert 6 == len(struct['key5'])
        del struct['key5'][1]
        assert 5 == len(struct['key5'])
        assert [6, True, False, None, inner_struct] == list(struct['key5'].items())

    def test_struct_assignment(self):
        # Tests struct assignment from another struct
        s1 = struct_pb2.Struct()
        s2 = struct_pb2.Struct()
        for value in [1, 'a', [1], ['a'], {'a': 'b'}]:
            s1['x'] = value
            s2['x'] = s1['x']
            assert s1['x'] == s2['x']

    def test_merge_from(self):
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
        assert 5 == struct['key1']
        assert 'abc' == struct['key2']
        assert struct['key3'] is True
        assert 11 == struct['key4']['subkey']
        inner_struct = struct_class()
        inner_struct['subkey2'] = 9
        assert [6, 'seven', True, False, None, inner_struct] == list(struct['key5'].items())
        assert 2 == len(struct['key6'][0].values)
        assert 'nested_list' == struct['key6'][0][0]
        assert struct['key6'][0][1] is True
        empty_list = struct['empty_list']
        assert [] == list(empty_list.items())
        empty_struct = struct['empty_struct']
        assert {} == dict(empty_struct.fields)

        # According to documentation: "When parsing from the wire or when merging,
        # if there are duplicate map keys the last key seen is used".
        duplicate = {
            'key4': {'replace': 20},
            'key5': [[False, 5]]
        }
        struct.update(duplicate)
        assert 1 == len(struct['key4'].fields)
        assert 20 == struct['key4']['replace']
        assert 1 == len(struct['key5'].values)
        assert struct['key5'][0][0] is False
        assert 5 == struct['key5'][0][1]


class TestAny:
    def test_any_message(self):
        # Creates and sets message.
        msg = any_test_pb2.TestAny()
        msg_descriptor = msg.DESCRIPTOR
        all_types = unittest_pb2.TestAllTypes()
        all_descriptor = all_types.DESCRIPTOR
        all_types.repeated_string.append('\u00fc\ua71f')
        # Packs to Any.
        msg.value.Pack(all_types)
        assert msg.value.type_url == 'type.googleapis.com/%s' % all_descriptor.full_name
        assert msg.value.value == all_types.SerializeToString()
        # Tests Is() method.
        assert msg.value.Is(all_descriptor)
        assert not msg.value.Is(msg_descriptor)
        # Unpacks Any.
        unpacked_message = unittest_pb2.TestAllTypes()
        assert msg.value.Unpack(unpacked_message)
        assert all_types == unpacked_message
        # Unpacks to different type.
        assert not msg.value.Unpack(msg)
        # Only Any messages have Pack method.
        try:
            msg.Pack(all_types)
        except AttributeError:
            pass
        else:
            raise AttributeError('%s should not have Pack method.' %
                                 msg_descriptor.full_name)

    def test_unpack_with_no_slash_in_type_url(self):
        msg = any_test_pb2.TestAny()
        all_types = unittest_pb2.TestAllTypes()
        all_descriptor = all_types.DESCRIPTOR
        msg.value.Pack(all_types)
        # Reset type_url to part of type_url after '/'
        msg.value.type_url = msg.value.TypeName()
        assert not msg.value.Is(all_descriptor)
        unpacked_message = unittest_pb2.TestAllTypes()
        assert not msg.value.Unpack(unpacked_message)

    def test_message_name(self):
        # Creates and sets message.
        submessage = any_test_pb2.TestAny()
        submessage.int_value = 12345
        msg = any_pb2.Any()
        msg.Pack(submessage)
        assert msg.TypeName() == 'google.protobuf.internal.TestAny'

    def test_pack_with_custom_type_url(self):
        submessage = any_test_pb2.TestAny()
        submessage.int_value = 12345
        msg = any_pb2.Any()
        # Pack with a custom type URL prefix.
        msg.Pack(submessage, 'type.myservice.com')
        assert msg.type_url == 'type.myservice.com/%s' % submessage.DESCRIPTOR.full_name
        # Pack with a custom type URL prefix ending with '/'.
        msg.Pack(submessage, 'type.myservice.com/')
        assert msg.type_url == 'type.myservice.com/%s' % submessage.DESCRIPTOR.full_name
        # Pack with an empty type URL prefix.
        msg.Pack(submessage, '')
        assert msg.type_url == '/%s' % submessage.DESCRIPTOR.full_name
        # Test unpacking the type.
        unpacked_message = any_test_pb2.TestAny()
        assert msg.Unpack(unpacked_message)
        assert submessage == unpacked_message

    def test_pack_deterministic(self):
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
        assert golden == serialized
