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
import struct
import sys

try:
  import unittest2 as unittest  #PY26
except ImportError:
  import unittest

from google.protobuf import any_pb2
from google.protobuf import duration_pb2
from google.protobuf import field_mask_pb2
from google.protobuf import struct_pb2
from google.protobuf import timestamp_pb2
from google.protobuf import wrappers_pb2
from google.protobuf import any_test_pb2
from google.protobuf import unittest_mset_pb2
from google.protobuf import unittest_pb2
from google.protobuf.internal import test_proto3_optional_pb2
from google.protobuf import descriptor_pool
from google.protobuf import json_format
from google.protobuf.util import json_format_pb2
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

  def testUnknownEnumToJsonAndBack(self):
    text = '{\n  "enumValue": 999\n}'
    message = json_format_proto3_pb2.TestMessage()
    message.enum_value = 999
    self.assertEqual(json_format.MessageToJson(message),
                     text)
    parsed_message = json_format_proto3_pb2.TestMessage()
    json_format.Parse(text, parsed_message)
    self.assertEqual(message, parsed_message)

  def testExtensionToJsonAndBack(self):
    message = unittest_mset_pb2.TestMessageSetContainer()
    ext1 = unittest_mset_pb2.TestMessageSetExtension1.message_set_extension
    ext2 = unittest_mset_pb2.TestMessageSetExtension2.message_set_extension
    message.message_set.Extensions[ext1].i = 23
    message.message_set.Extensions[ext2].str = 'foo'
    message_text = json_format.MessageToJson(
        message
    )
    parsed_message = unittest_mset_pb2.TestMessageSetContainer()
    json_format.Parse(message_text, parsed_message)
    self.assertEqual(message, parsed_message)

  def testExtensionErrors(self):
    self.CheckError('{"[extensionField]": {}}',
                    'Message type proto3.TestMessage does not have extensions')

  def testExtensionToDictAndBack(self):
    message = unittest_mset_pb2.TestMessageSetContainer()
    ext1 = unittest_mset_pb2.TestMessageSetExtension1.message_set_extension
    ext2 = unittest_mset_pb2.TestMessageSetExtension2.message_set_extension
    message.message_set.Extensions[ext1].i = 23
    message.message_set.Extensions[ext2].str = 'foo'
    message_dict = json_format.MessageToDict(
        message
    )
    parsed_message = unittest_mset_pb2.TestMessageSetContainer()
    json_format.ParseDict(message_dict, parsed_message)
    self.assertEqual(message, parsed_message)

  def testExtensionToDictAndBackWithScalar(self):
    message = unittest_pb2.TestAllExtensions()
    ext1 = unittest_pb2.TestNestedExtension.test
    message.Extensions[ext1] = 'data'
    message_dict = json_format.MessageToDict(
        message
    )
    parsed_message = unittest_pb2.TestAllExtensions()
    json_format.ParseDict(message_dict, parsed_message)
    self.assertEqual(message, parsed_message)

  def testJsonParseDictToAnyDoesNotAlterInput(self):
    orig_dict = {
        'int32Value': 20,
        '@type': 'type.googleapis.com/proto3.TestMessage'
    }
    copied_dict = json.loads(json.dumps(orig_dict))
    parsed_message = any_pb2.Any()
    json_format.ParseDict(copied_dict, parsed_message)
    self.assertEqual(copied_dict, orig_dict)

  def testExtensionSerializationDictMatchesProto3Spec(self):
    """See go/proto3-json-spec for spec.
    """
    message = unittest_mset_pb2.TestMessageSetContainer()
    ext1 = unittest_mset_pb2.TestMessageSetExtension1.message_set_extension
    ext2 = unittest_mset_pb2.TestMessageSetExtension2.message_set_extension
    message.message_set.Extensions[ext1].i = 23
    message.message_set.Extensions[ext2].str = 'foo'
    message_dict = json_format.MessageToDict(
        message
    )
    golden_dict = {
        'messageSet': {
            '[protobuf_unittest.'
            'TestMessageSetExtension1.message_set_extension]': {
                'i': 23,
            },
            '[protobuf_unittest.'
            'TestMessageSetExtension2.message_set_extension]': {
                'str': u'foo',
            },
        },
    }
    self.assertEqual(golden_dict, message_dict)
    parsed_msg = unittest_mset_pb2.TestMessageSetContainer()
    json_format.ParseDict(golden_dict, parsed_msg)
    self.assertEqual(message, parsed_msg)

  def testExtensionSerializationDictMatchesProto3SpecMore(self):
    """See go/proto3-json-spec for spec.
    """
    message = json_format_pb2.TestMessageWithExtension()
    ext = json_format_pb2.TestExtension.ext
    message.Extensions[ext].value = 'stuff'
    message_dict = json_format.MessageToDict(
        message
    )
    expected_dict = {
        '[protobuf_unittest.TestExtension.ext]': {
            'value': u'stuff',
        },
    }
    self.assertEqual(expected_dict, message_dict)

  def testExtensionSerializationJsonMatchesProto3Spec(self):
    """See go/proto3-json-spec for spec.
    """
    message = unittest_mset_pb2.TestMessageSetContainer()
    ext1 = unittest_mset_pb2.TestMessageSetExtension1.message_set_extension
    ext2 = unittest_mset_pb2.TestMessageSetExtension2.message_set_extension
    message.message_set.Extensions[ext1].i = 23
    message.message_set.Extensions[ext2].str = 'foo'
    message_text = json_format.MessageToJson(
        message
    )
    ext1_text = ('protobuf_unittest.TestMessageSetExtension1.'
                 'message_set_extension')
    ext2_text = ('protobuf_unittest.TestMessageSetExtension2.'
                 'message_set_extension')
    golden_text = ('{"messageSet": {'
                   '    "[%s]": {'
                   '        "i": 23'
                   '    },'
                   '    "[%s]": {'
                   '        "str": "foo"'
                   '    }'
                   '}}') % (ext1_text, ext2_text)
    self.assertEqual(json.loads(golden_text), json.loads(message_text))

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

  def testProto3Optional(self):
    message = test_proto3_optional_pb2.TestProto3Optional()
    self.assertEqual(
        json.loads(
            json_format.MessageToJson(
                message, including_default_value_fields=True)),
        json.loads('{}'))
    message.optional_int32 = 0
    self.assertEqual(
        json.loads(
            json_format.MessageToJson(
                message, including_default_value_fields=True)),
        json.loads('{"optionalInt32": 0}'))

  def testIntegersRepresentedAsFloat(self):
    message = json_format_proto3_pb2.TestMessage()
    json_format.Parse('{"int32Value": -2.147483648e9}', message)
    self.assertEqual(message.int32_value, -2147483648)
    json_format.Parse('{"int32Value": 1e5}', message)
    self.assertEqual(message.int32_value, 100000)
    json_format.Parse('{"int32Value": 1.0}', message)
    self.assertEqual(message.int32_value, 1)

  def testMapFields(self):
    message = json_format_proto3_pb2.TestNestedMap()
    self.assertEqual(
        json.loads(json_format.MessageToJson(message, True)),
        json.loads('{'
                   '"boolMap": {},'
                   '"int32Map": {},'
                   '"int64Map": {},'
                   '"uint32Map": {},'
                   '"uint64Map": {},'
                   '"stringMap": {},'
                   '"mapMap": {}'
                   '}'))
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
    message.map_map['1'].bool_map[True] = 3
    self.assertEqual(
        json.loads(json_format.MessageToJson(message, False)),
        json.loads('{'
                   '"boolMap": {"false": 2, "true": 1},'
                   '"int32Map": {"1": 2, "2": 3},'
                   '"int64Map": {"1": 2, "2": 3},'
                   '"uint32Map": {"1": 2, "2": 3},'
                   '"uint64Map": {"1": 2, "2": 3},'
                   '"stringMap": {"1": 2, "null": 3},'
                   '"mapMap": {"1": {"boolMap": {"true": 3}}}'
                   '}'))
    parsed_message = json_format_proto3_pb2.TestNestedMap()
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

  def testSurrogates(self):
    # Test correct surrogate handling.
    message = json_format_proto3_pb2.TestMessage()
    json_format.Parse('{"stringValue": "\\uD83D\\uDE01"}', message)
    self.assertEqual(message.string_value,
                     b'\xF0\x9F\x98\x81'.decode('utf-8', 'strict'))

    # Error case: unpaired high surrogate.
    self.CheckError(
        '{"stringValue": "\\uD83D"}',
        r'Invalid \\uXXXX escape|Unpaired.*surrogate')

    # Unpaired low surrogate.
    self.CheckError(
        '{"stringValue": "\\uDE01"}',
        r'Invalid \\uXXXX escape|Unpaired.*surrogate')

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

    message.value.Clear()
    self.assertEqual(
        json_format.MessageToJson(message, True),
        '{\n'
        '  "value": ""\n'
        '}')
    self.CheckParseBack(message, parsed_message)

  def testWrapperMessage(self):
    message = json_format_proto3_pb2.TestWrapper()
    message.bool_value.value = False
    message.int32_value.value = 0
    message.string_value.value = ''
    message.bytes_value.value = b''
    message.repeated_bool_value.add().value = True
    message.repeated_bool_value.add().value = False
    message.repeated_int32_value.add()
    self.assertEqual(
        json.loads(json_format.MessageToJson(message, True)),
        json.loads('{\n'
                   '  "int32Value": 0,'
                   '  "boolValue": false,'
                   '  "stringValue": "",'
                   '  "bytesValue": "",'
                   '  "repeatedBoolValue": [true, false],'
                   '  "repeatedInt32Value": [0],'
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

  def testStructMessage(self):
    message = json_format_proto3_pb2.TestStruct()
    message.value['name'] = 'Jim'
    message.value['age'] = 10
    message.value['attend'] = True
    message.value['email'] = None
    message.value.get_or_create_struct('address')['city'] = 'SFO'
    message.value['address']['house_number'] = 1024
    message.value.get_or_create_struct('empty_struct')
    message.value.get_or_create_list('empty_list')
    struct_list = message.value.get_or_create_list('list')
    struct_list.extend([6, 'seven', True, False, None])
    struct_list.add_struct()['subkey2'] = 9
    message.repeated_value.add()['age'] = 11
    message.repeated_value.add()
    self.assertEqual(
        json.loads(json_format.MessageToJson(message, False)),
        json.loads(
            '{'
            '  "value": {'
            '    "address": {'
            '      "city": "SFO", '
            '      "house_number": 1024'
            '    }, '
            '    "empty_struct": {}, '
            '    "empty_list": [], '
            '    "age": 10, '
            '    "name": "Jim", '
            '    "attend": true, '
            '    "email": null, '
            '    "list": [6, "seven", true, false, null, {"subkey2": 9}]'
            '  },'
            '  "repeatedValue": [{"age": 11}, {}]'
            '}'))
    parsed_message = json_format_proto3_pb2.TestStruct()
    self.CheckParseBack(message, parsed_message)
    # check for regression; this used to raise
    parsed_message.value['empty_struct']
    parsed_message.value['empty_list']

  def testValueMessage(self):
    message = json_format_proto3_pb2.TestValue()
    message.value.string_value = 'hello'
    message.repeated_value.add().number_value = 11.1
    message.repeated_value.add().bool_value = False
    message.repeated_value.add().null_value = 0
    self.assertEqual(
        json.loads(json_format.MessageToJson(message, False)),
        json.loads(
            '{'
            '  "value": "hello",'
            '  "repeatedValue": [11.1, false, null]'
            '}'))
    parsed_message = json_format_proto3_pb2.TestValue()
    self.CheckParseBack(message, parsed_message)
    # Can't parse back if the Value message is not set.
    message.repeated_value.add()
    self.assertEqual(
        json.loads(json_format.MessageToJson(message, False)),
        json.loads(
            '{'
            '  "value": "hello",'
            '  "repeatedValue": [11.1, false, null, null]'
            '}'))
    message.Clear()
    json_format.Parse('{"value": null}', message)
    self.assertEqual(message.value.WhichOneof('kind'), 'null_value')

  def testListValueMessage(self):
    message = json_format_proto3_pb2.TestListValue()
    message.value.values.add().number_value = 11.1
    message.value.values.add().null_value = 0
    message.value.values.add().bool_value = True
    message.value.values.add().string_value = 'hello'
    message.value.values.add().struct_value['name'] = 'Jim'
    message.repeated_value.add().values.add().number_value = 1
    message.repeated_value.add()
    self.assertEqual(
        json.loads(json_format.MessageToJson(message, False)),
        json.loads(
            '{"value": [11.1, null, true, "hello", {"name": "Jim"}]\n,'
            '"repeatedValue": [[1], []]}'))
    parsed_message = json_format_proto3_pb2.TestListValue()
    self.CheckParseBack(message, parsed_message)

  def testNullValue(self):
    message = json_format_proto3_pb2.TestOneof()
    message.oneof_null_value = 0
    self.assertEqual(json_format.MessageToJson(message),
                     '{\n  "oneofNullValue": null\n}')
    parsed_message = json_format_proto3_pb2.TestOneof()
    self.CheckParseBack(message, parsed_message)
    # Check old format is also accepted
    new_message = json_format_proto3_pb2.TestOneof()
    json_format.Parse('{\n  "oneofNullValue": "NULL_VALUE"\n}',
                      new_message)
    self.assertEqual(json_format.MessageToJson(new_message),
                     '{\n  "oneofNullValue": null\n}')

  def testAnyMessage(self):
    message = json_format_proto3_pb2.TestAny()
    value1 = json_format_proto3_pb2.MessageType()
    value2 = json_format_proto3_pb2.MessageType()
    value1.value = 1234
    value2.value = 5678
    message.value.Pack(value1)
    message.repeated_value.add().Pack(value1)
    message.repeated_value.add().Pack(value2)
    message.repeated_value.add()
    self.assertEqual(
        json.loads(json_format.MessageToJson(message, True)),
        json.loads(
            '{\n'
            '  "repeatedValue": [ {\n'
            '    "@type": "type.googleapis.com/proto3.MessageType",\n'
            '    "value": 1234\n'
            '  }, {\n'
            '    "@type": "type.googleapis.com/proto3.MessageType",\n'
            '    "value": 5678\n'
            '  },\n'
            '  {}],\n'
            '  "value": {\n'
            '    "@type": "type.googleapis.com/proto3.MessageType",\n'
            '    "value": 1234\n'
            '  }\n'
            '}\n'))
    parsed_message = json_format_proto3_pb2.TestAny()
    self.CheckParseBack(message, parsed_message)
    # Must print @type first
    test_message = json_format_proto3_pb2.TestMessage(
        bool_value=True,
        int32_value=20,
        int64_value=-20,
        uint32_value=20,
        uint64_value=20,
        double_value=3.14,
        string_value='foo')
    message.Clear()
    message.value.Pack(test_message)
    self.assertEqual(
        json_format.MessageToJson(message, False)[0:68],
        '{\n'
        '  "value": {\n'
        '    "@type": "type.googleapis.com/proto3.TestMessage"')

  def testAnyMessageDescriptorPoolMissingType(self):
    packed_message = unittest_pb2.OneString()
    packed_message.data = 'string'
    message = any_test_pb2.TestAny()
    message.any_value.Pack(packed_message)
    empty_pool = descriptor_pool.DescriptorPool()
    with self.assertRaises(TypeError) as cm:
      json_format.MessageToJson(message, True, descriptor_pool=empty_pool)
    self.assertEqual(
        'Can not find message descriptor by type_url:'
        ' type.googleapis.com/protobuf_unittest.OneString.',
        str(cm.exception))

  def testWellKnownInAnyMessage(self):
    message = any_pb2.Any()
    int32_value = wrappers_pb2.Int32Value()
    int32_value.value = 1234
    message.Pack(int32_value)
    self.assertEqual(
        json.loads(json_format.MessageToJson(message, True)),
        json.loads(
            '{\n'
            '  "@type": \"type.googleapis.com/google.protobuf.Int32Value\",\n'
            '  "value": 1234\n'
            '}\n'))
    parsed_message = any_pb2.Any()
    self.CheckParseBack(message, parsed_message)

    timestamp = timestamp_pb2.Timestamp()
    message.Pack(timestamp)
    self.assertEqual(
        json.loads(json_format.MessageToJson(message, True)),
        json.loads(
            '{\n'
            '  "@type": "type.googleapis.com/google.protobuf.Timestamp",\n'
            '  "value": "1970-01-01T00:00:00Z"\n'
            '}\n'))
    self.CheckParseBack(message, parsed_message)

    duration = duration_pb2.Duration()
    duration.seconds = 1
    message.Pack(duration)
    self.assertEqual(
        json.loads(json_format.MessageToJson(message, True)),
        json.loads(
            '{\n'
            '  "@type": "type.googleapis.com/google.protobuf.Duration",\n'
            '  "value": "1s"\n'
            '}\n'))
    self.CheckParseBack(message, parsed_message)

    field_mask = field_mask_pb2.FieldMask()
    field_mask.paths.append('foo.bar')
    field_mask.paths.append('bar')
    message.Pack(field_mask)
    self.assertEqual(
        json.loads(json_format.MessageToJson(message, True)),
        json.loads(
            '{\n'
            '  "@type": "type.googleapis.com/google.protobuf.FieldMask",\n'
            '  "value": "foo.bar,bar"\n'
            '}\n'))
    self.CheckParseBack(message, parsed_message)

    struct_message = struct_pb2.Struct()
    struct_message['name'] = 'Jim'
    message.Pack(struct_message)
    self.assertEqual(
        json.loads(json_format.MessageToJson(message, True)),
        json.loads(
            '{\n'
            '  "@type": "type.googleapis.com/google.protobuf.Struct",\n'
            '  "value": {"name": "Jim"}\n'
            '}\n'))
    self.CheckParseBack(message, parsed_message)

    nested_any = any_pb2.Any()
    int32_value.value = 5678
    nested_any.Pack(int32_value)
    message.Pack(nested_any)
    self.assertEqual(
        json.loads(json_format.MessageToJson(message, True)),
        json.loads(
            '{\n'
            '  "@type": "type.googleapis.com/google.protobuf.Any",\n'
            '  "value": {\n'
            '    "@type": "type.googleapis.com/google.protobuf.Int32Value",\n'
            '    "value": 5678\n'
            '  }\n'
            '}\n'))
    self.CheckParseBack(message, parsed_message)

  def testParseNull(self):
    message = json_format_proto3_pb2.TestMessage()
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
                      '"repeatedInt32Value": null,'
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
    # Null and {} should have different behavior for sub message.
    self.assertFalse(parsed_message.HasField('message_value'))
    json_format.Parse('{"messageValue": {}}', parsed_message)
    self.assertTrue(parsed_message.HasField('message_value'))
    # Null is not allowed to be used as an element in repeated field.
    self.assertRaisesRegexp(
        json_format.ParseError,
        'Failed to parse repeatedInt32Value field: '
        'null is not allowed to be used as an element in a repeated field.',
        json_format.Parse,
        '{"repeatedInt32Value":[1, null]}',
        parsed_message)
    self.CheckError('{"repeatedMessageValue":[null]}',
                    'Failed to parse repeatedMessageValue field: null is not'
                    ' allowed to be used as an element in a repeated field.')

  def testNanFloat(self):
    message = json_format_proto3_pb2.TestMessage()
    message.float_value = float('nan')
    text = '{\n  "floatValue": "NaN"\n}'
    self.assertEqual(json_format.MessageToJson(message), text)
    parsed_message = json_format_proto3_pb2.TestMessage()
    json_format.Parse(text, parsed_message)
    self.assertTrue(math.isnan(parsed_message.float_value))

  def testParseDoubleToFloat(self):
    message = json_format_proto3_pb2.TestMessage()
    text = ('{"repeatedDoubleValue": [3.4028235e+39, 1.4028235e-39]\n}')
    json_format.Parse(text, message)
    self.assertEqual(message.repeated_double_value[0], 3.4028235e+39)
    self.assertEqual(message.repeated_double_value[1], 1.4028235e-39)
    text = ('{"repeatedFloatValue": [3.4028235e+39, 1.4028235e-39]\n}')
    self.CheckError(text,
                    'Failed to parse repeatedFloatValue field: '
                    'Float value too large.')

  def testFloatPrecision(self):
    message = json_format_proto3_pb2.TestMessage()
    message.float_value = 1.123456789
    # Set to 8 valid digits.
    text = '{\n  "floatValue": 1.1234568\n}'
    self.assertEqual(
        json_format.MessageToJson(message, float_precision=8), text)
    # Set to 7 valid digits.
    text = '{\n  "floatValue": 1.123457\n}'
    self.assertEqual(
        json_format.MessageToJson(message, float_precision=7), text)

    # Default float_precision will automatic print shortest float.
    message.float_value = 1.1000000011
    text = '{\n  "floatValue": 1.1\n}'
    self.assertEqual(
        json_format.MessageToJson(message), text)
    message.float_value = 1.00000075e-36
    text = '{\n  "floatValue": 1.00000075e-36\n}'
    self.assertEqual(
        json_format.MessageToJson(message), text)
    message.float_value = 12345678912345e+11
    text = '{\n  "floatValue": 1.234568e+24\n}'
    self.assertEqual(
        json_format.MessageToJson(message), text)

    # Test a bunch of data and check json encode/decode do not
    # lose precision
    value_list = [0x00, 0xD8, 0x6E, 0x00]
    msg2 = json_format_proto3_pb2.TestMessage()
    for a in range(0, 256):
      value_list[3] = a
      for b in range(0, 256):
        value_list[0] = b
        byte_array = bytearray(value_list)
        message.float_value = struct.unpack('<f', byte_array)[0]
        self.CheckParseBack(message, msg2)

  def testParseEmptyText(self):
    self.CheckError('',
                    r'Failed to load JSON: (Expecting value)|(No JSON).')

  def testParseEnumValue(self):
    message = json_format_proto3_pb2.TestMessage()
    text = '{"enumValue": 0}'
    json_format.Parse(text, message)
    text = '{"enumValue": 1}'
    json_format.Parse(text, message)
    self.CheckError(
        '{"enumValue": "baz"}',
        'Failed to parse enumValue field: Invalid enum value baz '
        'for enum type proto3.EnumType.')
    # Proto3 accepts numeric unknown enums.
    text = '{"enumValue": 12345}'
    json_format.Parse(text, message)
    # Proto2 does not accept unknown enums.
    message = unittest_pb2.TestAllTypes()
    self.assertRaisesRegexp(
        json_format.ParseError,
        'Failed to parse optionalNestedEnum field: Invalid enum value 12345 '
        'for enum type protobuf_unittest.TestAllTypes.NestedEnum.',
        json_format.Parse, '{"optionalNestedEnum": 12345}', message)

  def testBytes(self):
    message = json_format_proto3_pb2.TestMessage()
    # Test url base64
    text = '{"bytesValue": "-_"}'
    json_format.Parse(text, message)
    self.assertEqual(message.bytes_value, b'\xfb')
    # Test padding
    text = '{"bytesValue": "AQI="}'
    json_format.Parse(text, message)
    self.assertEqual(message.bytes_value, b'\x01\x02')
    text = '{"bytesValue": "AQI"}'
    json_format.Parse(text, message)
    self.assertEqual(message.bytes_value, b'\x01\x02')
    text = '{"bytesValue": "AQI*"}'
    json_format.Parse(text, message)
    self.assertEqual(message.bytes_value, b'\x01\x02')

  def testParseBadIdentifer(self):
    self.CheckError('{int32Value: 1}',
                    (r'Failed to load JSON: Expecting property name'
                     r'( enclosed in double quotes)?: line 1'))
    self.CheckError('{"unknownName": 1}',
                    'Message type "proto3.TestMessage" has no field named '
                    '"unknownName".')

  def testIgnoreUnknownField(self):
    text = '{"unknownName": 1}'
    parsed_message = json_format_proto3_pb2.TestMessage()
    json_format.Parse(text, parsed_message, ignore_unknown_fields=True)
    text = ('{\n'
            '  "repeatedValue": [ {\n'
            '    "@type": "type.googleapis.com/proto3.MessageType",\n'
            '    "unknownName": 1\n'
            '  }]\n'
            '}\n')
    parsed_message = json_format_proto3_pb2.TestAny()
    json_format.Parse(text, parsed_message, ignore_unknown_fields=True)

  def testDuplicateField(self):
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
    self.CheckError('{"int32Value": 1.5}',
                    'Failed to parse int32Value field: '
                    'Couldn\'t parse integer: 1.5.')
    self.CheckError('{"int32Value": 012345}',
                    (r'Failed to load JSON: Expecting \'?,\'? delimiter: '
                     r'line 1.'))
    self.CheckError('{"int32Value": " 1 "}',
                    'Failed to parse int32Value field: '
                    'Couldn\'t parse integer: " 1 ".')
    self.CheckError('{"int32Value": "1 "}',
                    'Failed to parse int32Value field: '
                    'Couldn\'t parse integer: "1 ".')
    self.CheckError('{"int32Value": false}',
                    'Failed to parse int32Value field: Bool value False '
                    'is not acceptable for integer field.')
    self.CheckError('{"int32Value": 12345678901234567890}',
                    'Failed to parse int32Value field: Value out of range: '
                    '12345678901234567890.')
    self.CheckError('{"uint32Value": -1}',
                    'Failed to parse uint32Value field: '
                    'Value out of range: -1.')

  def testInvalidFloatValue(self):
    self.CheckError('{"floatValue": "nan"}',
                    'Failed to parse floatValue field: Couldn\'t '
                    'parse float "nan", use "NaN" instead.')
    self.CheckError('{"floatValue": NaN}',
                    'Failed to parse floatValue field: Couldn\'t '
                    'parse NaN, use quoted "NaN" instead.')
    self.CheckError('{"floatValue": Infinity}',
                    'Failed to parse floatValue field: Couldn\'t parse Infinity'
                    ' or value too large, use quoted "Infinity" instead.')
    self.CheckError('{"floatValue": -Infinity}',
                    'Failed to parse floatValue field: Couldn\'t parse '
                    '-Infinity or value too small, '
                    'use quoted "-Infinity" instead.')
    self.CheckError('{"doubleValue": -1.89769e+308}',
                    'Failed to parse doubleValue field: Couldn\'t parse '
                    '-Infinity or value too small, '
                    'use quoted "-Infinity" instead.')
    self.CheckError('{"floatValue": 3.4028235e+39}',
                    'Failed to parse floatValue field: Float value too large.')
    self.CheckError('{"floatValue": -3.502823e+38}',
                    'Failed to parse floatValue field: Float value too small.')

  def testInvalidRepeated(self):
    self.CheckError('{"repeatedInt32Value": 12345}',
                    (r'Failed to parse repeatedInt32Value field: repeated field'
                     r' repeatedInt32Value must be in \[\] which is 12345.'))

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
    text = r'{"stringMap": 0}'
    self.assertRaisesRegexp(
        json_format.ParseError,
        'Failed to parse stringMap field: Map field string_map must be '
        'in a dict which is 0.',
        json_format.Parse, text, message)

  def testInvalidTimestamp(self):
    message = json_format_proto3_pb2.TestTimestamp()
    text = '{"value": "10000-01-01T00:00:00.00Z"}'
    self.assertRaisesRegexp(
        json_format.ParseError,
        'Failed to parse value field: '
        'time data \'10000-01-01T00:00:00\' does not match'
        ' format \'%Y-%m-%dT%H:%M:%S\'.',
        json_format.Parse, text, message)
    text = '{"value": "1970-01-01T00:00:00.0123456789012Z"}'
    self.assertRaisesRegexp(
        json_format.ParseError,
        'nanos 0123456789012 more than 9 fractional digits.',
        json_format.Parse, text, message)
    text = '{"value": "1972-01-01T01:00:00.01+08"}'
    self.assertRaisesRegexp(
        json_format.ParseError,
        (r'Invalid timezone offset value: \+08.'),
        json_format.Parse, text, message)
    # Time smaller than minimum time.
    text = '{"value": "0000-01-01T00:00:00Z"}'
    self.assertRaisesRegexp(
        json_format.ParseError,
        'Failed to parse value field: year (0 )?is out of range.',
        json_format.Parse, text, message)
    # Time bigger than maximum time.
    message.value.seconds = 253402300800
    self.assertRaisesRegexp(
        OverflowError,
        'date value out of range',
        json_format.MessageToJson, message)
    # Lower case t does not accept.
    text = '{"value": "0001-01-01t00:00:00Z"}'
    with self.assertRaises(json_format.ParseError) as e:
      json_format.Parse(text, message)
    self.assertEqual(
        'Failed to parse value field: '
        'time data \'0001-01-01t00:00:00\' does not match format '
        '\'%Y-%m-%dT%H:%M:%S\', lowercase \'t\' is not accepted.',
        str(e.exception))

  def testInvalidOneof(self):
    message = json_format_proto3_pb2.TestOneof()
    text = '{"oneofInt32Value": 1, "oneofStringValue": "2"}'
    self.assertRaisesRegexp(
        json_format.ParseError,
        'Message type "proto3.TestOneof"'
        ' should not have multiple "oneof_value" oneof fields.',
        json_format.Parse, text, message)

  def testInvalidListValue(self):
    message = json_format_proto3_pb2.TestListValue()
    text = '{"value": 1234}'
    self.assertRaisesRegexp(
        json_format.ParseError,
        r'Failed to parse value field: ListValue must be in \[\] which is 1234',
        json_format.Parse, text, message)

  def testInvalidStruct(self):
    message = json_format_proto3_pb2.TestStruct()
    text = '{"value": 1234}'
    self.assertRaisesRegexp(
        json_format.ParseError,
        'Failed to parse value field: Struct must be in a dict which is 1234',
        json_format.Parse, text, message)

  def testTimestampInvalidStringValue(self):
    message = json_format_proto3_pb2.TestTimestamp()
    text = '{"value": {"foo": 123}}'
    self.assertRaisesRegexp(
        json_format.ParseError,
        r"Timestamp JSON value not a string: {u?'foo': 123}",
        json_format.Parse, text, message)

  def testDurationInvalidStringValue(self):
    message = json_format_proto3_pb2.TestDuration()
    text = '{"value": {"foo": 123}}'
    self.assertRaisesRegexp(
        json_format.ParseError,
        r"Duration JSON value not a string: {u?'foo': 123}",
        json_format.Parse, text, message)

  def testFieldMaskInvalidStringValue(self):
    message = json_format_proto3_pb2.TestFieldMask()
    text = '{"value": {"foo": 123}}'
    self.assertRaisesRegexp(
        json_format.ParseError,
        r"FieldMask JSON value not a string: {u?'foo': 123}",
        json_format.Parse, text, message)

  def testInvalidAny(self):
    message = any_pb2.Any()
    text = '{"@type": "type.googleapis.com/google.protobuf.Int32Value"}'
    self.assertRaisesRegexp(
        KeyError,
        'value',
        json_format.Parse, text, message)
    text = '{"value": 1234}'
    self.assertRaisesRegexp(
        json_format.ParseError,
        '@type is missing when parsing any message.',
        json_format.Parse, text, message)
    text = '{"@type": "type.googleapis.com/MessageNotExist", "value": 1234}'
    self.assertRaisesRegexp(
        TypeError,
        'Can not find message descriptor by type_url: '
        'type.googleapis.com/MessageNotExist.',
        json_format.Parse, text, message)
    # Only last part is to be used: b/25630112
    text = (r'{"@type": "incorrect.googleapis.com/google.protobuf.Int32Value",'
            r'"value": 1234}')
    json_format.Parse(text, message)

  def testPreservingProtoFieldNames(self):
    message = json_format_proto3_pb2.TestMessage()
    message.int32_value = 12345
    self.assertEqual('{\n  "int32Value": 12345\n}',
                     json_format.MessageToJson(message))
    self.assertEqual('{\n  "int32_value": 12345\n}',
                     json_format.MessageToJson(message, False, True))
    # When including_default_value_fields is True.
    message = json_format_proto3_pb2.TestTimestamp()
    self.assertEqual('{\n  "repeatedValue": []\n}',
                     json_format.MessageToJson(message, True, False))
    self.assertEqual('{\n  "repeated_value": []\n}',
                     json_format.MessageToJson(message, True, True))

    # Parsers accept both original proto field names and lowerCamelCase names.
    message = json_format_proto3_pb2.TestMessage()
    json_format.Parse('{"int32Value": 54321}', message)
    self.assertEqual(54321, message.int32_value)
    json_format.Parse('{"int32_value": 12345}', message)
    self.assertEqual(12345, message.int32_value)

  def testIndent(self):
    message = json_format_proto3_pb2.TestMessage()
    message.int32_value = 12345
    self.assertEqual('{\n"int32Value": 12345\n}',
                     json_format.MessageToJson(message, indent=0))

  def testFormatEnumsAsInts(self):
    message = json_format_proto3_pb2.TestMessage()
    message.enum_value = json_format_proto3_pb2.BAR
    message.repeated_enum_value.append(json_format_proto3_pb2.FOO)
    message.repeated_enum_value.append(json_format_proto3_pb2.BAR)
    self.assertEqual(json.loads('{\n'
                                '  "enumValue": 1,\n'
                                '  "repeatedEnumValue": [0, 1]\n'
                                '}\n'),
                     json.loads(json_format.MessageToJson(
                         message, use_integers_for_enums=True)))

  def testParseDict(self):
    expected = 12345
    js_dict = {'int32Value': expected}
    message = json_format_proto3_pb2.TestMessage()
    json_format.ParseDict(js_dict, message)
    self.assertEqual(expected, message.int32_value)

  def testParseDictAnyDescriptorPoolMissingType(self):
    # Confirm that ParseDict does not raise ParseError with default pool
    js_dict = {
        'any_value': {
            '@type': 'type.googleapis.com/proto3.MessageType',
            'value': 1234
        }
    }
    json_format.ParseDict(js_dict, any_test_pb2.TestAny())
    # Check ParseDict raises ParseError with empty pool
    js_dict = {
        'any_value': {
            '@type': 'type.googleapis.com/proto3.MessageType',
            'value': 1234
        }
    }
    with self.assertRaises(json_format.ParseError) as cm:
      empty_pool = descriptor_pool.DescriptorPool()
      json_format.ParseDict(js_dict,
                            any_test_pb2.TestAny(),
                            descriptor_pool=empty_pool)
    self.assertEqual(
        str(cm.exception),
        'Failed to parse any_value field: Can not find message descriptor by'
        ' type_url: type.googleapis.com/proto3.MessageType..')

  def testParseDictUnknownValueType(self):
    class UnknownClass(object):

      def __repr__(self):
        return 'v'
    message = json_format_proto3_pb2.TestValue()
    self.assertRaisesRegexp(
        json_format.ParseError,
        r"Value v has unexpected type <class '.*\.UnknownClass'>.",
        json_format.ParseDict,
        {'value': UnknownClass()},
        message)

  def testMessageToDict(self):
    message = json_format_proto3_pb2.TestMessage()
    message.int32_value = 12345
    expected = {'int32Value': 12345}
    self.assertEqual(expected,
                     json_format.MessageToDict(message))

  def testJsonName(self):
    message = json_format_proto3_pb2.TestCustomJsonName()
    message.value = 12345
    self.assertEqual('{\n  "@value": 12345\n}',
                     json_format.MessageToJson(message))
    parsed_message = json_format_proto3_pb2.TestCustomJsonName()
    self.CheckParseBack(message, parsed_message)

  def testSortKeys(self):
    # Testing sort_keys is not perfectly working, as by random luck we could
    # get the output sorted. We just use a selection of names.
    message = json_format_proto3_pb2.TestMessage(bool_value=True,
                                                 int32_value=1,
                                                 int64_value=3,
                                                 uint32_value=4,
                                                 string_value='bla')
    self.assertEqual(
        json_format.MessageToJson(message, sort_keys=True),
        # We use json.dumps() instead of a hardcoded string due to differences
        # between Python 2 and Python 3.
        json.dumps({'boolValue': True, 'int32Value': 1, 'int64Value': '3',
                    'uint32Value': 4, 'stringValue': 'bla'},
                   indent=2, sort_keys=True))


if __name__ == '__main__':
  unittest.main()
