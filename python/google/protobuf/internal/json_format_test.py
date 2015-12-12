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

"""Test for google.protobuf.json_format."""

__author__ = 'jieluo@google.com (Jie Luo)'

import json
import math
import sys

try:
  import unittest2 as unittest
except ImportError:
  import unittest
from google.protobuf.internal import well_known_types
from google.protobuf import json_format
from google.protobuf.util import json_format_proto3_pb2


class JsonFormatBase(unittest.TestCase):

  def FillAllFields(self, message):
    message.int32_value = 20
    message.int64_value = -20
    message.uint32_value = 3120987654
    message.uint64_value = 12345678900
    message.float_value = float('-inf')
    message.double_value = 3.1415
    message.bool_value = True
    message.string_value = 'foo'
    message.bytes_value = b'bar'
    message.message_value.value = 10
    message.enum_value = json_format_proto3_pb2.BAR
    # Repeated
    message.repeated_int32_value.append(0x7FFFFFFF)
    message.repeated_int32_value.append(-2147483648)
    message.repeated_int64_value.append(9007199254740992)
    message.repeated_int64_value.append(-9007199254740992)
    message.repeated_uint32_value.append(0xFFFFFFF)
    message.repeated_uint32_value.append(0x7FFFFFF)
    message.repeated_uint64_value.append(9007199254740992)
    message.repeated_uint64_value.append(9007199254740991)
    message.repeated_float_value.append(0)

    message.repeated_double_value.append(1E-15)
    message.repeated_double_value.append(float('inf'))
    message.repeated_bool_value.append(True)
    message.repeated_bool_value.append(False)
    message.repeated_string_value.append('Few symbols!#$,;')
    message.repeated_string_value.append('bar')
    message.repeated_bytes_value.append(b'foo')
    message.repeated_bytes_value.append(b'bar')
    message.repeated_message_value.add().value = 10
    message.repeated_message_value.add().value = 11
    message.repeated_enum_value.append(json_format_proto3_pb2.FOO)
    message.repeated_enum_value.append(json_format_proto3_pb2.BAR)
    self.message = message

  def CheckParseBack(self, message, parsed_message):
    json_format.Parse(json_format.MessageToJson(message),
                      parsed_message)
    self.assertEqual(message, parsed_message)

  def CheckError(self, text, error_message):
    message = json_format_proto3_pb2.TestMessage()
    self.assertRaisesRegexp(
        json_format.ParseError,
        error_message,
        json_format.Parse, text, message)


class JsonFormatTest(JsonFormatBase):

  def testEmptyMessageToJson(self):
    message = json_format_proto3_pb2.TestMessage()
    self.assertEqual(json_format.MessageToJson(message),
                     '{}')
    parsed_message = json_format_proto3_pb2.TestMessage()
    self.CheckParseBack(message, parsed_message)

  def testPartialMessageToJson(self):
    message = json_format_proto3_pb2.TestMessage(
        string_value='test',
        repeated_int32_value=[89, 4])
    self.assertEqual(json.loads(json_format.MessageToJson(message)),
                     json.loads('{"stringValue": "test", '
                                '"repeatedInt32Value": [89, 4]}'))
    parsed_message = json_format_proto3_pb2.TestMessage()
    self.CheckParseBack(message, parsed_message)

  def testAllFieldsToJson(self):
    message = json_format_proto3_pb2.TestMessage()
    text = ('{"int32Value": 20, '
            '"int64Value": "-20", '
            '"uint32Value": 3120987654,'
            '"uint64Value": "12345678900",'
            '"floatValue": "-Infinity",'
            '"doubleValue": 3.1415,'
            '"boolValue": true,'
            '"stringValue": "foo",'
            '"bytesValue": "YmFy",'
            '"messageValue": {"value": 10},'
            '"enumValue": "BAR",'
            '"repeatedInt32Value": [2147483647, -2147483648],'
            '"repeatedInt64Value": ["9007199254740992", "-9007199254740992"],'
            '"repeatedUint32Value": [268435455, 134217727],'
            '"repeatedUint64Value": ["9007199254740992", "9007199254740991"],'
            '"repeatedFloatValue": [0],'
            '"repeatedDoubleValue": [1e-15, "Infinity"],'
            '"repeatedBoolValue": [true, false],'
            '"repeatedStringValue": ["Few symbols!#$,;", "bar"],'
            '"repeatedBytesValue": ["Zm9v", "YmFy"],'
            '"repeatedMessageValue": [{"value": 10}, {"value": 11}],'
            '"repeatedEnumValue": ["FOO", "BAR"]'
            '}')
    self.FillAllFields(message)
    self.assertEqual(
        json.loads(json_format.MessageToJson(message)),
        json.loads(text))
    parsed_message = json_format_proto3_pb2.TestMessage()
    json_format.Parse(text, parsed_message)
    self.assertEqual(message, parsed_message)

  def testJsonEscapeString(self):
    message = json_format_proto3_pb2.TestMessage()
    if sys.version_info[0] < 3:
      message.string_value = '&\n<\"\r>\b\t\f\\\001/\xe2\x80\xa8\xe2\x80\xa9'
    else:
      message.string_value = '&\n<\"\r>\b\t\f\\\001/'
      message.string_value += (b'\xe2\x80\xa8\xe2\x80\xa9').decode('utf-8')
    self.assertEqual(
        json_format.MessageToJson(message),
        '{\n  "stringValue": '
        '"&\\n<\\\"\\r>\\b\\t\\f\\\\\\u0001/\\u2028\\u2029"\n}')
    parsed_message = json_format_proto3_pb2.TestMessage()
    self.CheckParseBack(message, parsed_message)
    text = u'{"int32Value": "\u0031"}'
    json_format.Parse(text, message)
    self.assertEqual(message.int32_value, 1)

  def testAlwaysSeriliaze(self):
    message = json_format_proto3_pb2.TestMessage(
        string_value='foo')
    self.assertEqual(
        json.loads(json_format.MessageToJson(message, True)),
        json.loads('{'
                   '"repeatedStringValue": [],'
                   '"stringValue": "foo",'
                   '"repeatedBoolValue": [],'
                   '"repeatedUint32Value": [],'
                   '"repeatedInt32Value": [],'
                   '"enumValue": "FOO",'
                   '"int32Value": 0,'
                   '"floatValue": 0,'
                   '"int64Value": "0",'
                   '"uint32Value": 0,'
                   '"repeatedBytesValue": [],'
                   '"repeatedUint64Value": [],'
                   '"repeatedDoubleValue": [],'
                   '"bytesValue": "",'
                   '"boolValue": false,'
                   '"repeatedEnumValue": [],'
                   '"uint64Value": "0",'
                   '"doubleValue": 0,'
                   '"repeatedFloatValue": [],'
                   '"repeatedInt64Value": [],'
                   '"repeatedMessageValue": []}'))
    parsed_message = json_format_proto3_pb2.TestMessage()
    self.CheckParseBack(message, parsed_message)

  def testMapFields(self):
    message = json_format_proto3_pb2.TestMap()
    message.bool_map[True] = 1
    message.bool_map[False] = 2
    message.int32_map[1] = 2
    message.int32_map[2] = 3
    message.int64_map[1] = 2
    message.int64_map[2] = 3
    message.uint32_map[1] = 2
    message.uint32_map[2] = 3
    message.uint64_map[1] = 2
    message.uint64_map[2] = 3
    message.string_map['1'] = 2
    message.string_map['null'] = 3
    self.assertEqual(
        json.loads(json_format.MessageToJson(message, True)),
        json.loads('{'
                   '"boolMap": {"false": 2, "true": 1},'
                   '"int32Map": {"1": 2, "2": 3},'
                   '"int64Map": {"1": 2, "2": 3},'
                   '"uint32Map": {"1": 2, "2": 3},'
                   '"uint64Map": {"1": 2, "2": 3},'
                   '"stringMap": {"1": 2, "null": 3}'
                   '}'))
    parsed_message = json_format_proto3_pb2.TestMap()
    self.CheckParseBack(message, parsed_message)

  def testOneofFields(self):
    message = json_format_proto3_pb2.TestOneof()
    # Always print does not affect oneof fields.
    self.assertEqual(
        json_format.MessageToJson(message, True),
        '{}')
    message.oneof_int32_value = 0
    self.assertEqual(
        json_format.MessageToJson(message, True),
        '{\n'
        '  "oneofInt32Value": 0\n'
        '}')
    parsed_message = json_format_proto3_pb2.TestOneof()
    self.CheckParseBack(message, parsed_message)

  def testTimestampMessage(self):
    message = json_format_proto3_pb2.TestTimestamp()
    message.value.seconds = 0
    message.value.nanos = 0
    message.repeated_value.add().seconds = 20
    message.repeated_value[0].nanos = 1
    message.repeated_value.add().seconds = 0
    message.repeated_value[1].nanos = 10000
    message.repeated_value.add().seconds = 100000000
    message.repeated_value[2].nanos = 0
    # Maximum time
    message.repeated_value.add().seconds = 253402300799
    message.repeated_value[3].nanos = 999999999
    # Minimum time
    message.repeated_value.add().seconds = -62135596800
    message.repeated_value[4].nanos = 0
    self.assertEqual(
        json.loads(json_format.MessageToJson(message, True)),
        json.loads('{'
                   '"value": "1970-01-01T00:00:00Z",'
                   '"repeatedValue": ['
                   '  "1970-01-01T00:00:20.000000001Z",'
                   '  "1970-01-01T00:00:00.000010Z",'
                   '  "1973-03-03T09:46:40Z",'
                   '  "9999-12-31T23:59:59.999999999Z",'
                   '  "0001-01-01T00:00:00Z"'
                   ']'
                   '}'))
    parsed_message = json_format_proto3_pb2.TestTimestamp()
    self.CheckParseBack(message, parsed_message)
    text = (r'{"value": "1970-01-01T00:00:00.01+08:00",'
            r'"repeatedValue":['
            r'  "1970-01-01T00:00:00.01+08:30",'
            r'  "1970-01-01T00:00:00.01-01:23"]}')
    json_format.Parse(text, parsed_message)
    self.assertEqual(parsed_message.value.seconds, -8 * 3600)
    self.assertEqual(parsed_message.value.nanos, 10000000)
    self.assertEqual(parsed_message.repeated_value[0].seconds, -8.5 * 3600)
    self.assertEqual(parsed_message.repeated_value[1].seconds, 3600 + 23 * 60)

  def testDurationMessage(self):
    message = json_format_proto3_pb2.TestDuration()
    message.value.seconds = 1
    message.repeated_value.add().seconds = 0
    message.repeated_value[0].nanos = 10
    message.repeated_value.add().seconds = -1
    message.repeated_value[1].nanos = -1000
    message.repeated_value.add().seconds = 10
    message.repeated_value[2].nanos = 11000000
    message.repeated_value.add().seconds = -315576000000
    message.repeated_value.add().seconds = 315576000000
    self.assertEqual(
        json.loads(json_format.MessageToJson(message, True)),
        json.loads('{'
                   '"value": "1s",'
                   '"repeatedValue": ['
                   '  "0.000000010s",'
                   '  "-1.000001s",'
                   '  "10.011s",'
                   '  "-315576000000s",'
                   '  "315576000000s"'
                   ']'
                   '}'))
    parsed_message = json_format_proto3_pb2.TestDuration()
    self.CheckParseBack(message, parsed_message)

  def testFieldMaskMessage(self):
    message = json_format_proto3_pb2.TestFieldMask()
    message.value.paths.append('foo.bar')
    message.value.paths.append('bar')
    self.assertEqual(
        json_format.MessageToJson(message, True),
        '{\n'
        '  "value": "foo.bar,bar"\n'
        '}')
    parsed_message = json_format_proto3_pb2.TestFieldMask()
    self.CheckParseBack(message, parsed_message)

  def testWrapperMessage(self):
    message = json_format_proto3_pb2.TestWrapper()
    message.bool_value.value = False
    message.int32_value.value = 0
    message.string_value.value = ''
    message.bytes_value.value = b''
    message.repeated_bool_value.add().value = True
    message.repeated_bool_value.add().value = False
    self.assertEqual(
        json.loads(json_format.MessageToJson(message, True)),
        json.loads('{\n'
                   '  "int32Value": 0,'
                   '  "boolValue": false,'
                   '  "stringValue": "",'
                   '  "bytesValue": "",'
                   '  "repeatedBoolValue": [true, false],'
                   '  "repeatedInt32Value": [],'
                   '  "repeatedUint32Value": [],'
                   '  "repeatedFloatValue": [],'
                   '  "repeatedDoubleValue": [],'
                   '  "repeatedBytesValue": [],'
                   '  "repeatedInt64Value": [],'
                   '  "repeatedUint64Value": [],'
                   '  "repeatedStringValue": []'
                   '}'))
    parsed_message = json_format_proto3_pb2.TestWrapper()
    self.CheckParseBack(message, parsed_message)

  def testParseNull(self):
    message = json_format_proto3_pb2.TestMessage()
    message.repeated_int32_value.append(1)
    message.repeated_int32_value.append(2)
    message.repeated_int32_value.append(3)
    parsed_message = json_format_proto3_pb2.TestMessage()
    self.FillAllFields(parsed_message)
    json_format.Parse('{"int32Value": null, '
                      '"int64Value": null, '
                      '"uint32Value": null,'
                      '"uint64Value": null,'
                      '"floatValue": null,'
                      '"doubleValue": null,'
                      '"boolValue": null,'
                      '"stringValue": null,'
                      '"bytesValue": null,'
                      '"messageValue": null,'
                      '"enumValue": null,'
                      '"repeatedInt32Value": [1, 2, null, 3],'
                      '"repeatedInt64Value": null,'
                      '"repeatedUint32Value": null,'
                      '"repeatedUint64Value": null,'
                      '"repeatedFloatValue": null,'
                      '"repeatedDoubleValue": null,'
                      '"repeatedBoolValue": null,'
                      '"repeatedStringValue": null,'
                      '"repeatedBytesValue": null,'
                      '"repeatedMessageValue": null,'
                      '"repeatedEnumValue": null'
                      '}',
                      parsed_message)
    self.assertEqual(message, parsed_message)

  def testNanFloat(self):
    message = json_format_proto3_pb2.TestMessage()
    message.float_value = float('nan')
    text = '{\n  "floatValue": "NaN"\n}'
    self.assertEqual(json_format.MessageToJson(message), text)
    parsed_message = json_format_proto3_pb2.TestMessage()
    json_format.Parse(text, parsed_message)
    self.assertTrue(math.isnan(parsed_message.float_value))

  def testParseEmptyText(self):
    self.CheckError('',
                    r'Failed to load JSON: (Expecting value)|(No JSON).')

  def testParseBadEnumValue(self):
    self.CheckError(
        '{"enumValue": 1}',
        'Enum value must be a string literal with double quotes. '
        'Type "proto3.EnumType" has no value named 1.')
    self.CheckError(
        '{"enumValue": "baz"}',
        'Enum value must be a string literal with double quotes. '
        'Type "proto3.EnumType" has no value named baz.')

  def testParseBadIdentifer(self):
    self.CheckError('{int32Value: 1}',
                    (r'Failed to load JSON: Expecting property name'
                     r'( enclosed in double quotes)?: line 1'))
    self.CheckError('{"unknownName": 1}',
                    'Message type "proto3.TestMessage" has no field named '
                    '"unknownName".')

  def testDuplicateField(self):
    # Duplicate key check is not supported for python2.6
    if sys.version_info < (2, 7):
      return
    self.CheckError('{"int32Value": 1,\n"int32Value":2}',
                    'Failed to load JSON: duplicate key int32Value.')

  def testInvalidBoolValue(self):
    self.CheckError('{"boolValue": 1}',
                    'Failed to parse boolValue field: '
                    'Expected true or false without quotes.')
    self.CheckError('{"boolValue": "true"}',
                    'Failed to parse boolValue field: '
                    'Expected true or false without quotes.')

  def testInvalidIntegerValue(self):
    message = json_format_proto3_pb2.TestMessage()
    text = '{"int32Value": 0x12345}'
    self.assertRaises(json_format.ParseError,
                      json_format.Parse, text, message)
    self.CheckError('{"int32Value": 012345}',
                    (r'Failed to load JSON: Expecting \'?,\'? delimiter: '
                     r'line 1.'))
    self.CheckError('{"int32Value": 1.0}',
                    'Failed to parse int32Value field: '
                    'Couldn\'t parse integer: 1.0.')
    self.CheckError('{"int32Value": " 1 "}',
                    'Failed to parse int32Value field: '
                    'Couldn\'t parse integer: " 1 ".')
    self.CheckError('{"int32Value": "1 "}',
                    'Failed to parse int32Value field: '
                    'Couldn\'t parse integer: "1 ".')
    self.CheckError('{"int32Value": 12345678901234567890}',
                    'Failed to parse int32Value field: Value out of range: '
                    '12345678901234567890.')
    self.CheckError('{"int32Value": 1e5}',
                    'Failed to parse int32Value field: '
                    'Couldn\'t parse integer: 100000.0.')
    self.CheckError('{"uint32Value": -1}',
                    'Failed to parse uint32Value field: '
                    'Value out of range: -1.')

  def testInvalidFloatValue(self):
    self.CheckError('{"floatValue": "nan"}',
                    'Failed to parse floatValue field: Couldn\'t '
                    'parse float "nan", use "NaN" instead.')

  def testInvalidBytesValue(self):
    self.CheckError('{"bytesValue": "AQI"}',
                    'Failed to parse bytesValue field: Incorrect padding.')
    self.CheckError('{"bytesValue": "AQI*"}',
                    'Failed to parse bytesValue field: Incorrect padding.')

  def testInvalidMap(self):
    message = json_format_proto3_pb2.TestMap()
    text = '{"int32Map": {"null": 2, "2": 3}}'
    self.assertRaisesRegexp(
        json_format.ParseError,
        'Failed to parse int32Map field: invalid literal',
        json_format.Parse, text, message)
    text = '{"int32Map": {1: 2, "2": 3}}'
    self.assertRaisesRegexp(
        json_format.ParseError,
        (r'Failed to load JSON: Expecting property name'
         r'( enclosed in double quotes)?: line 1'),
        json_format.Parse, text, message)
    text = '{"boolMap": {"null": 1}}'
    self.assertRaisesRegexp(
        json_format.ParseError,
        'Failed to parse boolMap field: Expected "true" or "false", not null.',
        json_format.Parse, text, message)
    if sys.version_info < (2, 7):
      return
    text = r'{"stringMap": {"a": 3, "\u0061": 2}}'
    self.assertRaisesRegexp(
        json_format.ParseError,
        'Failed to load JSON: duplicate key a',
        json_format.Parse, text, message)

  def testInvalidTimestamp(self):
    message = json_format_proto3_pb2.TestTimestamp()
    text = '{"value": "10000-01-01T00:00:00.00Z"}'
    self.assertRaisesRegexp(
        json_format.ParseError,
        'time data \'10000-01-01T00:00:00\' does not match'
        ' format \'%Y-%m-%dT%H:%M:%S\'.',
        json_format.Parse, text, message)
    text = '{"value": "1970-01-01T00:00:00.0123456789012Z"}'
    self.assertRaisesRegexp(
        well_known_types.ParseError,
        'nanos 0123456789012 more than 9 fractional digits.',
        json_format.Parse, text, message)
    text = '{"value": "1972-01-01T01:00:00.01+08"}'
    self.assertRaisesRegexp(
        well_known_types.ParseError,
        (r'Invalid timezone offset value: \+08.'),
        json_format.Parse, text, message)
    # Time smaller than minimum time.
    text = '{"value": "0000-01-01T00:00:00Z"}'
    self.assertRaisesRegexp(
        json_format.ParseError,
        'Failed to parse value field: year is out of range.',
        json_format.Parse, text, message)
    # Time bigger than maxinum time.
    message.value.seconds = 253402300800
    self.assertRaisesRegexp(
        OverflowError,
        'date value out of range',
        json_format.MessageToJson, message)

  def testInvalidOneof(self):
    message = json_format_proto3_pb2.TestOneof()
    text = '{"oneofInt32Value": 1, "oneofStringValue": "2"}'
    self.assertRaisesRegexp(
        json_format.ParseError,
        'Message type "proto3.TestOneof"'
        ' should not have multiple "oneof_value" oneof fields.',
        json_format.Parse, text, message)


if __name__ == '__main__':
  unittest.main()
