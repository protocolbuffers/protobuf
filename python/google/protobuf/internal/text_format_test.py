# -*- coding: utf-8 -*-
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

"""Test for google.protobuf.text_format."""

import io
import math
import re
import string
import textwrap

import unittest
import pytest

from google.protobuf import any_pb2
from google.protobuf import struct_pb2
from google.protobuf import descriptor_pb2
from google.protobuf.internal import any_test_pb2 as test_extend_any
from google.protobuf.internal import api_implementation
from google.protobuf.internal import message_set_extensions_pb2
from google.protobuf.internal import test_proto3_optional_pb2
from google.protobuf.internal import test_util
from google.protobuf import descriptor_pool
from google.protobuf import text_format
from google.protobuf.internal import _parameterized
from google.protobuf import any_test_pb2
from google.protobuf import map_unittest_pb2
from google.protobuf import unittest_mset_pb2
from google.protobuf import unittest_custom_options_pb2
from google.protobuf import unittest_pb2
from google.protobuf import unittest_proto3_arena_pb2
# pylint: enable=g-import-not-at-top


# Low-level nuts-n-bolts tests.
class TestSimpleTextFormat:
    # The members of _QUOTES are formatted into a regexp template that
    # expects single characters.  Therefore it's an error (in addition to being
    # non-sensical in the first place) to try to specify a "quote mark" that is
    # more than one character.
    def test_quote_marks_are_single_chars(self):
        for quote in text_format._QUOTES:
            assert 1 == len(quote)


# Base class with some common functionality.
class TextFormatBase:
    def read_golden(self, golden_filename):
        with test_util.GoldenFile(golden_filename) as f:
            return (f.readlines() if str is bytes else  # PY3
                  [golden_line.decode('utf-8') for golden_line in f])

    def compare_to_golden_file(self, text, golden_filename):
        golden_lines = self.read_golden(golden_filename)
        assert text == ''.join(golden_lines)

    def compare_to_golden_text(self, text, golden_text):
        assert text == golden_text

    def remove_redundant_zeros(self, text):
        # Some platforms print 1e+5 as 1e+005.  This is fine, but we need to remove
        # these zeros in order to match the golden file.
        text = text.replace('e+0','e+').replace('e+0','e+') \
                  .replace('e-0','e-').replace('e-0','e-')
        # Floating point fields are printed with .0 suffix even if they are
        # actually integer numbers.
        text = re.compile(r'\.0$', re.MULTILINE).sub('', text)
        return text


@pytest.mark.parametrize(
    'message_module',
    [unittest_pb2, unittest_proto3_arena_pb2],
    ids=['_proto2', '_proto3'],
)
class TestTextFormatMessageToString(TextFormatBase):
    def test_print_exotic(self, message_module):
        message = message_module.TestAllTypes()
        message.repeated_int64.append(-9223372036854775808)
        message.repeated_uint64.append(18446744073709551615)
        message.repeated_double.append(123.456)
        message.repeated_double.append(1.23e22)
        message.repeated_double.append(1.23e-18)
        message.repeated_string.append('\000\001\a\b\f\n\r\t\v\\\'"')
        message.repeated_string.append(u'\u00fc\ua71f')
        self.compare_to_golden_text(
            self.remove_redundant_zeros(text_format.MessageToString(message)),
            'repeated_int64: -9223372036854775808\n'
            'repeated_uint64: 18446744073709551615\n'
            'repeated_double: 123.456\n'
            'repeated_double: 1.23e+22\n'
            'repeated_double: 1.23e-18\n'
            'repeated_string:'
            ' "\\000\\001\\007\\010\\014\\n\\r\\t\\013\\\\\\\'\\""\n'
            'repeated_string: "\\303\\274\\352\\234\\237"\n')

    def test_print_float_precision(self, message_module):
        message = message_module.TestAllTypes()

        message.repeated_float.append(0.0)
        message.repeated_float.append(0.8)
        message.repeated_float.append(1.0)
        message.repeated_float.append(1.2)
        message.repeated_float.append(1.23)
        message.repeated_float.append(1.234)
        message.repeated_float.append(1.2345)
        message.repeated_float.append(1.23456)
        message.repeated_float.append(1.2e10)
        message.repeated_float.append(1.23e10)
        message.repeated_float.append(1.234e10)
        message.repeated_float.append(1.2345e10)
        message.repeated_float.append(1.23456e10)
        message.repeated_float.append(float('NaN'))
        message.repeated_float.append(float('inf'))
        message.repeated_double.append(0.0)
        message.repeated_double.append(0.8)
        message.repeated_double.append(1.0)
        message.repeated_double.append(1.2)
        message.repeated_double.append(1.23)
        message.repeated_double.append(1.234)
        message.repeated_double.append(1.2345)
        message.repeated_double.append(1.23456)
        message.repeated_double.append(1.234567)
        message.repeated_double.append(1.2345678)
        message.repeated_double.append(1.23456789)
        message.repeated_double.append(1.234567898)
        message.repeated_double.append(1.2345678987)
        message.repeated_double.append(1.23456789876)
        message.repeated_double.append(1.234567898765)
        message.repeated_double.append(1.2345678987654)
        message.repeated_double.append(1.23456789876543)
        message.repeated_double.append(1.2e100)
        message.repeated_double.append(1.23e100)
        message.repeated_double.append(1.234e100)
        message.repeated_double.append(1.2345e100)
        message.repeated_double.append(1.23456e100)
        message.repeated_double.append(1.234567e100)
        message.repeated_double.append(1.2345678e100)
        message.repeated_double.append(1.23456789e100)
        message.repeated_double.append(1.234567898e100)
        message.repeated_double.append(1.2345678987e100)
        message.repeated_double.append(1.23456789876e100)
        message.repeated_double.append(1.234567898765e100)
        message.repeated_double.append(1.2345678987654e100)
        message.repeated_double.append(1.23456789876543e100)
        # pylint: disable=g-long-ternary
        self.compare_to_golden_text(
            self.remove_redundant_zeros(text_format.MessageToString(message)),
            'repeated_float: 0\n'
            'repeated_float: 0.8\n'
            'repeated_float: 1\n'
            'repeated_float: 1.2\n'
            'repeated_float: 1.23\n'
            'repeated_float: 1.234\n'
            'repeated_float: 1.2345\n'
            'repeated_float: 1.23456\n'
            # Note that these don't use scientific notation.
            'repeated_float: 12000000000\n'
            'repeated_float: 12300000000\n'
            'repeated_float: 12340000000\n'
            'repeated_float: 12345000000\n'
            'repeated_float: 12345600000\n'
            'repeated_float: nan\n'
            'repeated_float: inf\n'
            'repeated_double: 0\n'
            'repeated_double: 0.8\n'
            'repeated_double: 1\n'
            'repeated_double: 1.2\n'
            'repeated_double: 1.23\n'
            'repeated_double: 1.234\n'
            'repeated_double: 1.2345\n'
            'repeated_double: 1.23456\n'
            'repeated_double: 1.234567\n'
            'repeated_double: 1.2345678\n'
            'repeated_double: 1.23456789\n'
            'repeated_double: 1.234567898\n'
            'repeated_double: 1.2345678987\n'
            'repeated_double: 1.23456789876\n'
            'repeated_double: 1.234567898765\n'
            'repeated_double: 1.2345678987654\n'
            'repeated_double: 1.23456789876543\n'
            'repeated_double: 1.2e+100\n'
            'repeated_double: 1.23e+100\n'
            'repeated_double: 1.234e+100\n'
            'repeated_double: 1.2345e+100\n'
            'repeated_double: 1.23456e+100\n'
            'repeated_double: 1.234567e+100\n'
            'repeated_double: 1.2345678e+100\n'
            'repeated_double: 1.23456789e+100\n'
            'repeated_double: 1.234567898e+100\n'
            'repeated_double: 1.2345678987e+100\n'
            'repeated_double: 1.23456789876e+100\n'
            'repeated_double: 1.234567898765e+100\n'
            'repeated_double: 1.2345678987654e+100\n'
            'repeated_double: 1.23456789876543e+100\n')

    def test_print_exotic_unicode_subclass(self, message_module):

        class UnicodeSub(str):
            pass

        message = message_module.TestAllTypes()
        message.repeated_string.append(UnicodeSub(u'\u00fc\ua71f'))
        self.compare_to_golden_text(
            text_format.MessageToString(message),
            'repeated_string: "\\303\\274\\352\\234\\237"\n')

    def test_print_nested_message_as_one_line(self, message_module):
        message = message_module.TestAllTypes()
        msg = message.repeated_nested_message.add()
        msg.bb = 42
        self.compare_to_golden_text(
            text_format.MessageToString(message, as_one_line=True),
            'repeated_nested_message { bb: 42 }')

    def test_print_repeated_fields_as_one_line(self, message_module):
        message = message_module.TestAllTypes()
        message.repeated_int32.append(1)
        message.repeated_int32.append(1)
        message.repeated_int32.append(3)
        message.repeated_string.append('Google')
        message.repeated_string.append('Zurich')
        self.compare_to_golden_text(
            text_format.MessageToString(message, as_one_line=True),
            'repeated_int32: 1 repeated_int32: 1 repeated_int32: 3 '
            'repeated_string: "Google" repeated_string: "Zurich"')

    def verify_print_short_format_repeated_fields(self, message_module, as_one_line):
        message = message_module.TestAllTypes()
        message.repeated_int32.append(1)
        message.repeated_string.append('Google')
        message.repeated_string.append('Hello,World')
        message.repeated_foreign_enum.append(unittest_pb2.FOREIGN_FOO)
        message.repeated_foreign_enum.append(unittest_pb2.FOREIGN_BAR)
        message.repeated_foreign_enum.append(unittest_pb2.FOREIGN_BAZ)
        message.optional_nested_message.bb = 3
        for i in (21, 32):
            msg = message.repeated_nested_message.add()
            msg.bb = i
        expected_ascii = (
            'optional_nested_message {\n  bb: 3\n}\n'
            'repeated_int32: [1]\n'
            'repeated_string: "Google"\n'
            'repeated_string: "Hello,World"\n'
            'repeated_nested_message {\n  bb: 21\n}\n'
            'repeated_nested_message {\n  bb: 32\n}\n'
            'repeated_foreign_enum: [FOREIGN_FOO, FOREIGN_BAR, FOREIGN_BAZ]\n')
        if as_one_line:
            expected_ascii = expected_ascii.replace('\n', ' ')
            expected_ascii = re.sub(r'\s+', ' ', expected_ascii)
            expected_ascii = re.sub(r'\s$', '', expected_ascii)

        actual_ascii = text_format.MessageToString(
            message, use_short_repeated_primitives=True,
            as_one_line=as_one_line)
        self.compare_to_golden_text(actual_ascii, expected_ascii)
        parsed_message = message_module.TestAllTypes()
        text_format.Parse(actual_ascii, parsed_message)
        assert parsed_message == message

    def test_print_short_format_repeated_fields(self, message_module):
        self.verify_print_short_format_repeated_fields(message_module, False)
        self.verify_print_short_format_repeated_fields(message_module, True)

    def test_print_nested_new_line_in_string_as_one_line(self, message_module):
        message = message_module.TestAllTypes()
        message.optional_string = 'a\nnew\nline'
        self.compare_to_golden_text(
            text_format.MessageToString(message, as_one_line=True),
            'optional_string: "a\\nnew\\nline"')

    def test_print_exotic_as_one_line(self, message_module):
        message = message_module.TestAllTypes()
        message.repeated_int64.append(-9223372036854775808)
        message.repeated_uint64.append(18446744073709551615)
        message.repeated_double.append(123.456)
        message.repeated_double.append(1.23e22)
        message.repeated_double.append(1.23e-18)
        message.repeated_string.append('\000\001\a\b\f\n\r\t\v\\\'"')
        message.repeated_string.append(u'\u00fc\ua71f')
        self.compare_to_golden_text(
            self.remove_redundant_zeros(text_format.MessageToString(
                message, as_one_line=True)),
            'repeated_int64: -9223372036854775808'
            ' repeated_uint64: 18446744073709551615'
            ' repeated_double: 123.456'
            ' repeated_double: 1.23e+22'
            ' repeated_double: 1.23e-18'
            ' repeated_string: '
            '"\\000\\001\\007\\010\\014\\n\\r\\t\\013\\\\\\\'\\""'
            ' repeated_string: "\\303\\274\\352\\234\\237"')

    def test_round_trip_exotic_as_one_line(self, message_module):
        message = message_module.TestAllTypes()
        message.repeated_int64.append(-9223372036854775808)
        message.repeated_uint64.append(18446744073709551615)
        message.repeated_double.append(123.456)
        message.repeated_double.append(1.23e22)
        message.repeated_double.append(1.23e-18)
        message.repeated_string.append('\000\001\a\b\f\n\r\t\v\\\'"')
        message.repeated_string.append(u'\u00fc\ua71f')

        # Test as_utf8 = False.
        wire_text = text_format.MessageToString(message,
                                                as_one_line=True,
                                                as_utf8=False)
        parsed_message = message_module.TestAllTypes()
        r = text_format.Parse(wire_text, parsed_message)
        assert r is parsed_message
        assert message == parsed_message

        # Test as_utf8 = True.
        wire_text = text_format.MessageToString(message,
                                                as_one_line=True,
                                                as_utf8=True)
        parsed_message = message_module.TestAllTypes()
        r = text_format.Parse(wire_text, parsed_message)
        assert r is parsed_message
        assert message == parsed_message

    def test_print_raw_utf8_string(self, message_module):
        message = message_module.TestAllTypes()
        message.repeated_string.append(u'\u00fc\t\ua71f')
        text = text_format.MessageToString(message, as_utf8=True)
        golden_unicode = u'repeated_string: "\u00fc\\t\ua71f"\n'
        golden_text = golden_unicode
        # MessageToString always returns a native str.
        self.compare_to_golden_text(text, golden_text)
        parsed_message = message_module.TestAllTypes()
        text_format.Parse(text, parsed_message)
        assert message == parsed_message, (
            '\n%s != %s  (%s != %s)' %
            (message, parsed_message, message.repeated_string[0],
            parsed_message.repeated_string[0]))

    def test_print_float_format(self, message_module):
        # Check that float_format argument is passed to sub-message formatting.
        message = message_module.NestedTestAllTypes()
        message.payload.optional_float = 1.25
        # Check rounding at 15 significant digits
        message.payload.optional_double = -.000003456789012345678
        # Check no decimal point.
        message.payload.repeated_float.append(-5642)
        # Check no trailing zeros.
        message.payload.repeated_double.append(.000078900)
        formatted_fields = ['optional_float: 1.25',
                            'optional_double: -3.45678901234568e-6',
                            'repeated_float: -5642', 'repeated_double: 7.89e-5']
        text_message = text_format.MessageToString(message, float_format='.15g')
        self.compare_to_golden_text(
            self.remove_redundant_zeros(text_message),
            'payload {{\n  {0}\n  {1}\n  {2}\n  {3}\n}}\n'.format(
                *formatted_fields))
        # as_one_line=True is a separate code branch where float_format is passed.
        text_message = text_format.MessageToString(message,
                                                  as_one_line=True,
                                                  float_format='.15g')
        self.compare_to_golden_text(
            self.remove_redundant_zeros(text_message),
            'payload {{ {0} {1} {2} {3} }}'.format(*formatted_fields))

        # 32-bit 1.2 is noisy when extended to 64-bit:
        #  >>> struct.unpack('f', struct.pack('f', 1.2))[0]
        #  1.2000000476837158
        message.payload.optional_float = 1.2
        formatted_fields = ['optional_float: 1.2',
                            'optional_double: -3.45678901234568e-6',
                            'repeated_float: -5642', 'repeated_double: 7.89e-5']
        text_message = text_format.MessageToString(message, float_format='.7g',
                                                  double_format='.15g')
        self.compare_to_golden_text(
            self.remove_redundant_zeros(text_message),
            'payload {{\n  {0}\n  {1}\n  {2}\n  {3}\n}}\n'.format(
                *formatted_fields))

        # Test only set float_format affect both float and double fields.
        formatted_fields = ['optional_float: 1.2',
                            'optional_double: -3.456789e-6',
                            'repeated_float: -5642', 'repeated_double: 7.89e-5']
        text_message = text_format.MessageToString(message, float_format='.7g')
        self.compare_to_golden_text(
            self.remove_redundant_zeros(text_message),
            'payload {{\n  {0}\n  {1}\n  {2}\n  {3}\n}}\n'.format(
                *formatted_fields))

        # Test default float_format will automatic print shortest float.
        message.payload.optional_float = 1.2345678912
        message.payload.optional_double = 1.2345678912
        formatted_fields = ['optional_float: 1.2345679',
                            'optional_double: 1.2345678912',
                            'repeated_float: -5642', 'repeated_double: 7.89e-5']
        text_message = text_format.MessageToString(message)
        self.compare_to_golden_text(
            self.remove_redundant_zeros(text_message),
            'payload {{\n  {0}\n  {1}\n  {2}\n  {3}\n}}\n'.format(
                *formatted_fields))

        message.Clear()
        message.payload.optional_float = 1.1000000000011
        assert (text_format.MessageToString(message)
                == 'payload {\n  optional_float: 1.1\n}\n')
        message.payload.optional_float = 1.00000075e-36
        assert (text_format.MessageToString(message)
                == 'payload {\n  optional_float: 1.00000075e-36\n}\n')
        message.payload.optional_float = 12345678912345e+11
        assert (text_format.MessageToString(message)
                == 'payload {\n  optional_float: 1.234568e+24\n}\n')

    def test_message_to_string(self, message_module):
        message = message_module.ForeignMessage()
        message.c = 123
        assert 'c: 123\n' == str(message)

    def test_message_to_string_unicode(self, message_module):
        golden_unicode = 'Á short desçription and a 🍌.'
        golden_bytes = golden_unicode.encode('utf-8')
        message = message_module.TestAllTypes()
        message.optional_string = golden_unicode
        message.optional_bytes = golden_bytes
        text = text_format.MessageToString(message, as_utf8=True)
        golden_message = textwrap.dedent(
            'optional_string: "Á short desçription and a 🍌."\n'
            'optional_bytes: '
            r'"\303\201 short des\303\247ription and a \360\237\215\214."'
            '\n')
        self.compare_to_golden_text(text, golden_message)

    def test_message_to_string_ascii(self, message_module):
        golden_unicode = u'Á short desçription and a 🍌.'
        golden_bytes = golden_unicode.encode('utf-8')
        message = message_module.TestAllTypes()
        message.optional_string = golden_unicode
        message.optional_bytes = golden_bytes
        text = text_format.MessageToString(message, as_utf8=False)  # ASCII
        golden_message = (
            'optional_string: '
            r'"\303\201 short des\303\247ription and a \360\237\215\214."'
            '\n'
            'optional_bytes: '
            r'"\303\201 short des\303\247ription and a \360\237\215\214."'
            '\n')
        self.compare_to_golden_text(text, golden_message)

    def test_print_field(self, message_module):
        message = message_module.TestAllTypes()
        field = message.DESCRIPTOR.fields_by_name['optional_float']
        value = message.optional_float
        out = text_format.TextWriter(False)
        text_format.PrintField(field, value, out)
        assert 'optional_float: 0.0\n' == out.getvalue()
        out.close()
        # Test Printer
        out = text_format.TextWriter(False)
        printer = text_format._Printer(out)
        printer.PrintField(field, value)
        assert 'optional_float: 0.0\n' == out.getvalue()
        out.close()

    def test_print_field_value(self, message_module):
        message = message_module.TestAllTypes()
        field = message.DESCRIPTOR.fields_by_name['optional_float']
        value = message.optional_float
        out = text_format.TextWriter(False)
        text_format.PrintFieldValue(field, value, out)
        assert '0.0' == out.getvalue()
        out.close()
        # Test Printer
        out = text_format.TextWriter(False)
        printer = text_format._Printer(out)
        printer.PrintFieldValue(field, value)
        assert '0.0' == out.getvalue()
        out.close()

    def testCustomOptions(self, message_module):
      message_descriptor = (unittest_custom_options_pb2.
                            TestMessageWithCustomOptions.DESCRIPTOR)
      message_proto = descriptor_pb2.DescriptorProto()
      message_descriptor.CopyToProto(message_proto)
      expected_text = (
          'name: "TestMessageWithCustomOptions"\n'
          'field {\n'
          '  name: "field1"\n'
          '  number: 1\n'
          '  label: LABEL_OPTIONAL\n'
          '  type: TYPE_STRING\n'
          '  options {\n'
          '    ctype: CORD\n'
          '    [protobuf_unittest.field_opt1]: 8765432109\n'
          '  }\n'
          '}\n'
          'field {\n'
          '  name: "oneof_field"\n'
          '  number: 2\n'
          '  label: LABEL_OPTIONAL\n'
          '  type: TYPE_INT32\n'
          '  oneof_index: 0\n'
          '}\n'
          'field {\n'
          '  name: "map_field"\n'
          '  number: 3\n'
          '  label: LABEL_REPEATED\n'
          '  type: TYPE_MESSAGE\n'
          '  type_name: ".protobuf_unittest.TestMessageWithCustomOptions.'
          'MapFieldEntry"\n'
          '  options {\n'
          '    [protobuf_unittest.field_opt1]: 12345\n'
          '  }\n'
          '}\n'
          'nested_type {\n'
          '  name: "MapFieldEntry"\n'
          '  field {\n'
          '    name: "key"\n'
          '    number: 1\n'
          '    label: LABEL_OPTIONAL\n'
          '    type: TYPE_STRING\n'
          '  }\n'
          '  field {\n'
          '    name: "value"\n'
          '    number: 2\n'
          '    label: LABEL_OPTIONAL\n'
          '    type: TYPE_STRING\n'
          '  }\n'
          '  options {\n'
          '    map_entry: true\n'
          '  }\n'
          '}\n'
          'enum_type {\n'
          '  name: "AnEnum"\n'
          '  value {\n'
          '    name: "ANENUM_VAL1"\n'
          '    number: 1\n'
          '  }\n'
          '  value {\n'
          '    name: "ANENUM_VAL2"\n'
          '    number: 2\n'
          '    options {\n'
          '      [protobuf_unittest.enum_value_opt1]: 123\n'
          '    }\n'
          '  }\n'
          '  options {\n'
          '    [protobuf_unittest.enum_opt1]: -789\n'
          '  }\n'
          '}\n'
          'options {\n'
          '  message_set_wire_format: false\n'
          '  [protobuf_unittest.message_opt1]: -56\n'
          '}\n'
          'oneof_decl {\n'
          '  name: "AnOneof"\n'
          '  options {\n'
          '    [protobuf_unittest.oneof_opt1]: -99\n'
          '  }\n'
          '}\n')
      assert expected_text == text_format.MessageToString(message_proto)
      parsed_proto = descriptor_pb2.DescriptorProto()
      text_format.Parse(expected_text, parsed_proto)
      assert message_proto == parsed_proto

    @pytest.mark.skipif(
        api_implementation.Type() == 'upb',
        reason="upb API doesn't support old UnknownField API. The TextFormat library "
        "needs to convert to the new API.")
    def test_print_unknown_fields_embedded_message_in_bytes(self, message_module):
        inner_msg = message_module.TestAllTypes()
        inner_msg.optional_int32 = 101
        inner_msg.optional_double = 102.0
        inner_msg.optional_string = u'hello'
        inner_msg.optional_bytes = b'103'
        inner_msg.optional_nested_message.bb = 105
        inner_data = inner_msg.SerializeToString()
        outer_message = message_module.TestAllTypes()
        outer_message.optional_int32 = 101
        outer_message.optional_bytes = inner_data
        all_data = outer_message.SerializeToString()
        empty_message = message_module.TestEmptyMessage()
        empty_message.ParseFromString(all_data)

        assert ('  1: 101\n'
                '  15 {\n'
                '    1: 101\n'
                '    12: 4636878028842991616\n'
                '    14: "hello"\n'
                '    15: "103"\n'
                '    18 {\n'
                '      1: 105\n'
                '    }\n'
                '  }\n'
                == text_format.MessageToString(empty_message,
                                               indent=2,
                                               print_unknown_fields=True))
        assert ('1: 101 '
                '15 { '
                '1: 101 '
                '12: 4636878028842991616 '
                '14: "hello" '
                '15: "103" '
                '18 { 1: 105 } '
                '}'
                == text_format.MessageToString(empty_message,
                                               print_unknown_fields=True,
                                               as_one_line=True))


@pytest.mark.parametrize(
    'message_module',
    [unittest_pb2, unittest_proto3_arena_pb2],
    ids=['_proto2', '_proto3'],
)
class TestTextFormatMessageToTextBytes(TextFormatBase):
    def test_message_to_bytes(self, message_module):
        message = message_module.ForeignMessage()
        message.c = 123
        assert b'c: 123\n' == text_format.MessageToBytes(message)

    def test_raw_utf8_round_trip(self, message_module):
        message = message_module.TestAllTypes()
        message.repeated_string.append(u'\u00fc\t\ua71f')
        utf8_text = text_format.MessageToBytes(message, as_utf8=True)
        golden_bytes = b'repeated_string: "\xc3\xbc\\t\xea\x9c\x9f"\n'
        self.compare_to_golden_text(utf8_text, golden_bytes)
        parsed_message = message_module.TestAllTypes()
        text_format.Parse(utf8_text, parsed_message)
        assert message == parsed_message, (
            '\n%s != %s  (%s != %s)' %
            (message, parsed_message, message.repeated_string[0],
            parsed_message.repeated_string[0]))

    def test_escaped_utf8ascii_round_trip(self, message_module):
        message = message_module.TestAllTypes()
        message.repeated_string.append(u'\u00fc\t\ua71f')
        ascii_text = text_format.MessageToBytes(message)  # as_utf8=False default
        golden_bytes = b'repeated_string: "\\303\\274\\t\\352\\234\\237"\n'
        self.compare_to_golden_text(ascii_text, golden_bytes)
        parsed_message = message_module.TestAllTypes()
        text_format.Parse(ascii_text, parsed_message)
        assert message == parsed_message, (
            '\n%s != %s  (%s != %s)' %
            (message, parsed_message, message.repeated_string[0],
            parsed_message.repeated_string[0]))


@pytest.mark.parametrize(
    'message_module',
    [unittest_pb2, unittest_proto3_arena_pb2],
    ids=['_proto2', '_proto3'],
)
class TestTextFormatParser(TextFormatBase):
    def test_parse_all_fields(self, message_module):
        message = message_module.TestAllTypes()
        test_util.SetAllFields(message)
        ascii_text = text_format.MessageToString(message)

        parsed_message = message_module.TestAllTypes()
        text_format.Parse(ascii_text, parsed_message)
        assert message == parsed_message
        if message_module is unittest_pb2:
          test_util.ExpectAllFieldsSet(self, message)

    def test_parse_and_merge_utf8(self, message_module):
        message = message_module.TestAllTypes()
        test_util.SetAllFields(message)
        ascii_text = text_format.MessageToString(message)
        ascii_text = ascii_text.encode('utf-8')

        parsed_message = message_module.TestAllTypes()
        text_format.Parse(ascii_text, parsed_message)
        assert message == parsed_message
        if message_module is unittest_pb2:
          test_util.ExpectAllFieldsSet(self, message)

        parsed_message.Clear()
        text_format.Merge(ascii_text, parsed_message)
        assert message == parsed_message
        if message_module is unittest_pb2:
          test_util.ExpectAllFieldsSet(self, message)

        msg2 = message_module.TestAllTypes()
        text = (u'optional_string: "café"')
        text_format.Merge(text, msg2)
        assert msg2.optional_string == u'café'
        msg2.Clear()
        assert msg2.optional_string == u''
        text_format.Parse(text, msg2)
        assert msg2.optional_string == u'café'

    def test_parse_double_to_float(self, message_module):
        message = message_module.TestAllTypes()
        text = ('repeated_float: 3.4028235e+39\n'
                'repeated_float: 1.4028235e-39\n')
        text_format.Parse(text, message)
        assert message.repeated_float[0] == float('inf')
        assert message.repeated_float[1] == pytest.approx(1.4028235e-39)

    def test_parse_exotic(self, message_module):
        message = message_module.TestAllTypes()
        text = ('repeated_int64: -9223372036854775808\n'
                'repeated_uint64: 18446744073709551615\n'
                'repeated_double: 123.456\n'
                'repeated_double: 1.23e+22\n'
                'repeated_double: 1.23e-18\n'
                'repeated_string: \n'
                '"\\000\\001\\007\\010\\014\\n\\r\\t\\013\\\\\\\'\\""\n'
                'repeated_string: "foo" \'corge\' "grault"\n'
                'repeated_string: "\\303\\274\\352\\234\\237"\n'
                'repeated_string: "\\xc3\\xbc"\n'
                'repeated_string: "\xc3\xbc"\n')
        text_format.Parse(text, message)

        assert -9223372036854775808 == message.repeated_int64[0]
        assert 18446744073709551615 == message.repeated_uint64[0]
        assert 123.456 == message.repeated_double[0]
        assert 1.23e22 == message.repeated_double[1]
        assert 1.23e-18 == message.repeated_double[2]
        assert '\000\001\a\b\f\n\r\t\v\\\'"' == message.repeated_string[0]
        assert 'foocorgegrault' == message.repeated_string[1]
        assert u'\u00fc\ua71f' == message.repeated_string[2]
        assert u'\u00fc' == message.repeated_string[3]

    def test_parse_trailing_commas(self, message_module):
        message = message_module.TestAllTypes()
        text = ('repeated_int64: 100;\n'
                'repeated_int64: 200;\n'
                'repeated_int64: 300,\n'
                'repeated_string: "one",\n'
                'repeated_string: "two";\n')
        text_format.Parse(text, message)

        assert 100 == message.repeated_int64[0]
        assert 200 == message.repeated_int64[1]
        assert 300 == message.repeated_int64[2]
        assert u'one' == message.repeated_string[0]
        assert u'two' == message.repeated_string[1]

    def test_parse_repeated_scalar_short_format(self, message_module):
        message = message_module.TestAllTypes()
        text = ('repeated_int64: [100, 200];\n'
                'repeated_int64: []\n'
                'repeated_int64: 300,\n'
                'repeated_string: ["one", "two"];\n')
        text_format.Parse(text, message)

        assert 100 == message.repeated_int64[0]
        assert 200 == message.repeated_int64[1]
        assert 300 == message.repeated_int64[2]
        assert 'one' == message.repeated_string[0]
        assert 'two' == message.repeated_string[1]

    def test_parse_repeated_message_short_format(self, message_module):
        message = message_module.TestAllTypes()
        text = ('repeated_nested_message: [{bb: 100}, {bb: 200}],\n'
                'repeated_nested_message: {bb: 300}\n'
                'repeated_nested_message [{bb: 400}];\n')
        text_format.Parse(text, message)

        assert 100 == message.repeated_nested_message[0].bb
        assert 200 == message.repeated_nested_message[1].bb
        assert 300 == message.repeated_nested_message[2].bb
        assert 400 == message.repeated_nested_message[3].bb

    def test_parse_empty_text(self, message_module):
        message = message_module.TestAllTypes()
        text = ''
        text_format.Parse(text, message)
        assert message_module.TestAllTypes() == message

    def test_parse_invalid_utf8(self, message_module):
        message = message_module.TestAllTypes()
        text = b'invalid<\xc3\xc3>'
        with pytest.raises(text_format.ParseError):
            text_format.Parse(text, message)

    def test_parse_invalid_utf8_value(self, message_module):
        message = message_module.TestAllTypes()
        text = 'repeated_string: "\\xc3\\xc3"'
        with pytest.raises(text_format.ParseError) as e:
            text_format.Parse(text, message)
        assert e.value.GetLine() == 1
        assert e.value.GetColumn() == 28

    def test_parse_single_word(self, message_module):
        message = message_module.TestAllTypes()
        text = 'foo'
        with pytest.raises(
            text_format.ParseError,
            match=r'1:1 : Message type "\w+.TestAllTypes" has no field named '
            r'"foo".'):
            text_format.Parse(text, message)

    def test_parse_unknown_field(self, message_module):
        message = message_module.TestAllTypes()
        text = 'unknown_field: 8\n'
        with pytest.raises(
            text_format.ParseError,
            match=r'1:1 : Message type "\w+.TestAllTypes" has no field named '
            r'"unknown_field".'):
            text_format.Parse(text, message)
        text = ('optional_int32: 123\n'
                'unknown_field: 8\n'
                'optional_nested_message { bb: 45 }')
        text_format.Parse(text, message, allow_unknown_field=True)
        assert message.optional_nested_message.bb == 45
        assert message.optional_int32 == 123

    def test_parse_bad_enum_value(self, message_module):
        message = message_module.TestAllTypes()
        text = 'optional_nested_enum: BARR'
        with pytest.raises(
            text_format.ParseError,
            match=r'1:23 : \'optional_nested_enum: BARR\': '
                  r'Enum type "\w+.TestAllTypes.NestedEnum" '
                  r'has no value named BARR.'):
            text_format.Parse(text, message)

    def test_parse_bad_int_value(self, message_module):
        message = message_module.TestAllTypes()
        text = 'optional_int32: bork'
        with pytest.raises(
            text_format.ParseError,
            match='1:17 : \'optional_int32: bork\': '
                  'Couldn\'t parse integer: bork'):
            text_format.Parse(text, message)

    def test_parse_string_field_unescape(self, message_module):
        message = message_module.TestAllTypes()
        text = r'''repeated_string: "\xf\x62"
                  repeated_string: "\\xf\\x62"
                  repeated_string: "\\\xf\\\x62"
                  repeated_string: "\\\\xf\\\\x62"
                  repeated_string: "\\\\\xf\\\\\x62"
                  repeated_string: "\x5cx20"'''

        text_format.Parse(text, message)

        SLASH = '\\'
        assert '\x0fb' == message.repeated_string[0]
        assert SLASH + 'xf' + SLASH + 'x62' == message.repeated_string[1]
        assert SLASH + '\x0f' + SLASH + 'b' == message.repeated_string[2]
        assert SLASH + SLASH + 'xf' + SLASH + SLASH + 'x62' == message.repeated_string[3]
        assert SLASH + SLASH + '\x0f' + SLASH + SLASH + 'b' == message.repeated_string[4]
        assert SLASH + 'x20' == message.repeated_string[5]

    def test_parse_oneof(self, message_module):
        m = message_module.TestAllTypes()
        m.oneof_uint32 = 11
        m2 = message_module.TestAllTypes()
        text_format.Parse(text_format.MessageToString(m), m2)
        assert 'oneof_uint32' == m2.WhichOneof('oneof_field')

    def test_parse_multiple_oneof(self, message_module):
        m_string = '\n'.join(['oneof_uint32: 11', 'oneof_string: "foo"'])
        m2 = message_module.TestAllTypes()
        with pytest.raises(text_format.ParseError,
                           match=' is specified along with field '):
            text_format.Parse(m_string, m2)

    # This example contains non-ASCII codepoint unicode data as literals
    # which should come through as utf-8 for bytes, and as the unicode
    # itself for string fields.  It also demonstrates escaped binary data.
    # The ur"" string prefix is unfortunately missing from Python 3
    # so we resort to double escaping our \s so that they come through.
    _UNICODE_SAMPLE = """
        optional_bytes: 'Á short desçription'
        optional_string: 'Á short desçription'
        repeated_bytes: '\\303\\201 short des\\303\\247ription'
        repeated_bytes: '\\x12\\x34\\x56\\x78\\x90\\xab\\xcd\\xef'
        repeated_string: '\\xd0\\x9f\\xd1\\x80\\xd0\\xb8\\xd0\\xb2\\xd1\\x96\\xd1\\x82'
        """
    _BYTES_SAMPLE = _UNICODE_SAMPLE.encode('utf-8')
    _GOLDEN_UNICODE = 'Á short desçription'
    _GOLDEN_BYTES = _GOLDEN_UNICODE.encode('utf-8')
    _GOLDEN_BYTES_1 = b'\x12\x34\x56\x78\x90\xab\xcd\xef'
    _GOLDEN_STR_0 = 'Привіт'

    def test_parse_unicode(self, message_module):
        m = message_module.TestAllTypes()
        text_format.Parse(self._UNICODE_SAMPLE, m)
        assert m.optional_bytes == self._GOLDEN_BYTES
        assert m.optional_string == self._GOLDEN_UNICODE
        assert m.repeated_bytes[0] == self._GOLDEN_BYTES
        # repeated_bytes[1] contained simple \ escaped non-UTF-8 raw binary data.
        assert m.repeated_bytes[1] == self._GOLDEN_BYTES_1
        # repeated_string[0] contained \ escaped data representing the UTF-8
        # representation of _GOLDEN_STR_0 - it needs to decode as such.
        assert m.repeated_string[0] == self._GOLDEN_STR_0

    def test_parse_bytes(self, message_module):
        m = message_module.TestAllTypes()
        text_format.Parse(self._BYTES_SAMPLE, m)
        assert m.optional_bytes == self._GOLDEN_BYTES
        assert m.optional_string == self._GOLDEN_UNICODE
        assert m.repeated_bytes[0] == self._GOLDEN_BYTES
        # repeated_bytes[1] contained simple \ escaped non-UTF-8 raw binary data.
        assert m.repeated_bytes[1] == self._GOLDEN_BYTES_1
        # repeated_string[0] contained \ escaped data representing the UTF-8
        # representation of _GOLDEN_STR_0 - it needs to decode as such.
        assert m.repeated_string[0] == self._GOLDEN_STR_0

    def test_from_bytes_file(self, message_module):
        m = message_module.TestAllTypes()
        f = io.BytesIO(self._BYTES_SAMPLE)
        text_format.ParseLines(f, m)
        assert m.optional_bytes == self._GOLDEN_BYTES
        assert m.optional_string == self._GOLDEN_UNICODE
        assert m.repeated_bytes[0] == self._GOLDEN_BYTES

    def test_from_unicode_file(self, message_module):
        m = message_module.TestAllTypes()
        f = io.StringIO(self._UNICODE_SAMPLE)
        text_format.ParseLines(f, m)
        assert m.optional_bytes == self._GOLDEN_BYTES
        assert m.optional_string == self._GOLDEN_UNICODE
        assert m.repeated_bytes[0] == self._GOLDEN_BYTES

    def test_from_bytes_lines(self, message_module):
        m = message_module.TestAllTypes()
        text_format.ParseLines(self._BYTES_SAMPLE.split(b'\n'), m)
        assert m.optional_bytes == self._GOLDEN_BYTES
        assert m.optional_string == self._GOLDEN_UNICODE
        assert m.repeated_bytes[0] == self._GOLDEN_BYTES

    def test_from_unicode_lines(self, message_module):
        m = message_module.TestAllTypes()
        text_format.ParseLines(self._UNICODE_SAMPLE.split(u'\n'), m)
        assert m.optional_bytes == self._GOLDEN_BYTES
        assert m.optional_string == self._GOLDEN_UNICODE
        assert m.repeated_bytes[0] == self._GOLDEN_BYTES

    def test_parse_duplicate_messages(self, message_module):
        message = message_module.TestAllTypes()
        text = ('optional_nested_message { bb: 1 } '
                'optional_nested_message { bb: 2 }')
        with pytest.raises(
            text_format.ParseError,
            match=r'1:59 : Message type "\w+.TestAllTypes" '
            r'should not have multiple "optional_nested_message" fields.'):
            text_format.Parse(text, message)

    def test_parse_duplicate_scalars(self, message_module):
        message = message_module.TestAllTypes()
        text = ('optional_int32: 42 ' 'optional_int32: 67')
        with pytest.raises(
            text_format.ParseError,
            match=r'1:36 : Message type "\w+.TestAllTypes" should not '
            r'have multiple "optional_int32" fields.'):
            text_format.Parse(text, message)

    def test_parse_existing_scalar_in_message(self, message_module):
        message = message_module.TestAllTypes(optional_int32=42)
        text = 'optional_int32: 67'
        with pytest.raises(
            text_format.ParseError,
            match='Message type "\w+.TestAllTypes" should not '
                  r'have multiple "optional_int32" fields.'):
            text_format.Parse(text, message)


@pytest.mark.parametrize(
    'message_module',
    [unittest_pb2, unittest_proto3_arena_pb2],
    ids=['_proto2', '_proto3'],
)
class TestTextFormatMerge(TextFormatBase):
    def test_merge_duplicate_scalars_in_text(self, message_module):
        message = message_module.TestAllTypes()
        text = ('optional_int32: 42 ' 'optional_int32: 67')
        r = text_format.Merge(text, message)
        assert r == message
        assert 67 == message.optional_int32

    def test_merge_duplicate_nested_message_scalars(self, message_module):
        message = message_module.TestAllTypes()
        text = ('optional_nested_message { bb: 1 } '
                'optional_nested_message { bb: 2 }')
        r = text_format.Merge(text, message)
        assert r is message
        assert 2 == message.optional_nested_message.bb

    def test_replace_scalar_in_message(self, message_module):
        message = message_module.TestAllTypes(optional_int32=42)
        text = 'optional_int32: 67'
        r = text_format.Merge(text, message)
        assert r == message
        assert 67 == message.optional_int32

    def test_replace_message_in_message(self, message_module):
        message = message_module.TestAllTypes(
            optional_int32=42, optional_nested_message=dict())
        assert message.HasField('optional_nested_message')
        text = 'optional_nested_message{ bb: 3 }'
        r = text_format.Merge(text, message)
        assert r == message
        assert 3 == message.optional_nested_message.bb

    def test_merge_multiple_oneof(self, message_module):
        m_string = '\n'.join(['oneof_uint32: 11', 'oneof_string: "foo"'])
        m2 = message_module.TestAllTypes()
        text_format.Merge(m_string, m2)
        assert 'oneof_string' == m2.WhichOneof('oneof_field')


# These are tests that aren't fundamentally specific to proto2, but are at
# the moment because of differences between the proto2 and proto3 test schemas.
# Ideally the schemas would be made more similar so these tests could pass.
class TestOnlyWorksWithProto2RightNow(TextFormatBase):

  def test_print_all_fields_pointy(self):
    message = unittest_pb2.TestAllTypes()
    test_util.SetAllFields(message)
    self.compare_to_golden_file(
        self.remove_redundant_zeros(text_format.MessageToString(
            message, pointy_brackets=True)),
        'text_format_unittest_data_pointy_oneof.txt')

  def test_parse_golden(self):
    golden_text = '\n'.join(self.read_golden(
        'text_format_unittest_data_oneof_implemented.txt'))
    parsed_message = unittest_pb2.TestAllTypes()
    r = text_format.Parse(golden_text, parsed_message)
    assert r == parsed_message

    message = unittest_pb2.TestAllTypes()
    test_util.SetAllFields(message)
    assert message == parsed_message

  def test_print_all_fields(self):
    message = unittest_pb2.TestAllTypes()
    test_util.SetAllFields(message)
    self.compare_to_golden_file(
        self.remove_redundant_zeros(text_format.MessageToString(message)),
        'text_format_unittest_data_oneof_implemented.txt')

  def test_print_unknown_fields(self):
    message = unittest_pb2.TestAllTypes()
    message.optional_int32 = 101
    message.optional_double = 102.0
    message.optional_string = u'hello'
    message.optional_bytes = b'103'
    message.optionalgroup.a = 104
    message.optional_nested_message.bb = 105
    all_data = message.SerializeToString()
    empty_message = unittest_pb2.TestEmptyMessage()
    empty_message.ParseFromString(all_data)
    assert ('  1: 101\n'
            '  12: 4636878028842991616\n'
            '  14: "hello"\n'
            '  15: "103"\n'
            '  16 {\n'
            '    17: 104\n'
            '  }\n'
            '  18 {\n'
            '    1: 105\n'
            '  }\n'
            == text_format.MessageToString(empty_message,
                                        indent=2,
                                        print_unknown_fields=True))
    assert ('1: 101 '
            '12: 4636878028842991616 '
            '14: "hello" '
            '15: "103" '
            '16 { 17: 104 } '
            '18 { 1: 105 }'
            == text_format.MessageToString(empty_message,
                                        print_unknown_fields=True,
                                        as_one_line=True))

  def test_print_in_index_order(self):
    message = unittest_pb2.TestFieldOrderings()
    # Fields are listed in index order instead of field number.
    message.my_string = 'str'
    message.my_int = 101
    message.my_float = 111
    message.optional_nested_message.oo = 0
    message.optional_nested_message.bb = 1
    message.Extensions[unittest_pb2.my_extension_string] = 'ext_str0'
    # Extensions are listed based on the order of extension number.
    # Extension number 12.
    message.Extensions[unittest_pb2.TestExtensionOrderings2.
                       test_ext_orderings2].my_string = 'ext_str2'
    # Extension number 13.
    message.Extensions[unittest_pb2.TestExtensionOrderings1.
                       test_ext_orderings1].my_string = 'ext_str1'
    # Extension number 14.
    message.Extensions[
        unittest_pb2.TestExtensionOrderings2.TestExtensionOrderings3.
        test_ext_orderings3].my_string = 'ext_str3'

    # Print in index order.
    self.compare_to_golden_text(
        self.remove_redundant_zeros(
            text_format.MessageToString(message, use_index_order=True)),
        'my_string: "str"\n'
        'my_int: 101\n'
        'my_float: 111\n'
        'optional_nested_message {\n'
        '  oo: 0\n'
        '  bb: 1\n'
        '}\n'
        '[protobuf_unittest.TestExtensionOrderings2.test_ext_orderings2] {\n'
        '  my_string: "ext_str2"\n'
        '}\n'
        '[protobuf_unittest.TestExtensionOrderings1.test_ext_orderings1] {\n'
        '  my_string: "ext_str1"\n'
        '}\n'
        '[protobuf_unittest.TestExtensionOrderings2.TestExtensionOrderings3'
        '.test_ext_orderings3] {\n'
        '  my_string: "ext_str3"\n'
        '}\n'
        '[protobuf_unittest.my_extension_string]: "ext_str0"\n')
    # By default, print in field number order.
    self.compare_to_golden_text(
        self.remove_redundant_zeros(text_format.MessageToString(message)),
        'my_int: 101\n'
        'my_string: "str"\n'
        '[protobuf_unittest.TestExtensionOrderings2.test_ext_orderings2] {\n'
        '  my_string: "ext_str2"\n'
        '}\n'
        '[protobuf_unittest.TestExtensionOrderings1.test_ext_orderings1] {\n'
        '  my_string: "ext_str1"\n'
        '}\n'
        '[protobuf_unittest.TestExtensionOrderings2.TestExtensionOrderings3'
        '.test_ext_orderings3] {\n'
        '  my_string: "ext_str3"\n'
        '}\n'
        '[protobuf_unittest.my_extension_string]: "ext_str0"\n'
        'my_float: 111\n'
        'optional_nested_message {\n'
        '  bb: 1\n'
        '  oo: 0\n'
        '}\n')

  def test_merge_lines_golden(self):
    opened = self.read_golden('text_format_unittest_data_oneof_implemented.txt')
    parsed_message = unittest_pb2.TestAllTypes()
    r = text_format.MergeLines(opened, parsed_message)
    assert r == parsed_message

    message = unittest_pb2.TestAllTypes()
    test_util.SetAllFields(message)
    assert message == parsed_message

  def test_parse_lines_golden(self):
    opened = self.read_golden('text_format_unittest_data_oneof_implemented.txt')
    parsed_message = unittest_pb2.TestAllTypes()
    r = text_format.ParseLines(opened, parsed_message)
    assert r == parsed_message

    message = unittest_pb2.TestAllTypes()
    test_util.SetAllFields(message)
    assert message == parsed_message

  def test_print_map(self):
    message = map_unittest_pb2.TestMap()

    message.map_int32_int32[-123] = -456
    message.map_int64_int64[-2**33] = -2**34
    message.map_uint32_uint32[123] = 456
    message.map_uint64_uint64[2**33] = 2**34
    message.map_string_string['abc'] = '123'
    message.map_int32_foreign_message[111].c = 5

    # Maps are serialized to text format using their underlying repeated
    # representation.
    self.compare_to_golden_text(
        text_format.MessageToString(message), 'map_int32_int32 {\n'
        '  key: -123\n'
        '  value: -456\n'
        '}\n'
        'map_int64_int64 {\n'
        '  key: -8589934592\n'
        '  value: -17179869184\n'
        '}\n'
        'map_uint32_uint32 {\n'
        '  key: 123\n'
        '  value: 456\n'
        '}\n'
        'map_uint64_uint64 {\n'
        '  key: 8589934592\n'
        '  value: 17179869184\n'
        '}\n'
        'map_string_string {\n'
        '  key: "abc"\n'
        '  value: "123"\n'
        '}\n'
        'map_int32_foreign_message {\n'
        '  key: 111\n'
        '  value {\n'
        '    c: 5\n'
        '  }\n'
        '}\n')

  def test_duplicate_map_key(self):
    message = map_unittest_pb2.TestMap()
    text = (
        'map_uint64_uint64 {\n'
        '  key: 123\n'
        '  value: 17179869184\n'
        '}\n'
        'map_string_string {\n'
        '  key: "abc"\n'
        '  value: "first"\n'
        '}\n'
        'map_int32_foreign_message {\n'
        '  key: 111\n'
        '  value {\n'
        '    c: 5\n'
        '  }\n'
        '}\n'
        'map_uint64_uint64 {\n'
        '  key: 123\n'
        '  value: 321\n'
        '}\n'
        'map_string_string {\n'
        '  key: "abc"\n'
        '  value: "second"\n'
        '}\n'
        'map_int32_foreign_message {\n'
        '  key: 111\n'
        '  value {\n'
        '    d: 5\n'
        '  }\n'
        '}\n')
    text_format.Parse(text, message)
    self.compare_to_golden_text(
        text_format.MessageToString(message), 'map_uint64_uint64 {\n'
        '  key: 123\n'
        '  value: 321\n'
        '}\n'
        'map_string_string {\n'
        '  key: "abc"\n'
        '  value: "second"\n'
        '}\n'
        'map_int32_foreign_message {\n'
        '  key: 111\n'
        '  value {\n'
        '    d: 5\n'
        '  }\n'
        '}\n')

  # In cpp implementation, __str__ calls the cpp implementation of text format.
  def test_print_map_using_cpp_implementation(self):
    message = map_unittest_pb2.TestMap()
    inner_msg = message.map_int32_foreign_message[111]
    inner_msg.c = 1
    assert (
        str(message) ==
        'map_int32_foreign_message {\n'
        '  key: 111\n'
        '  value {\n'
        '    c: 1\n'
        '  }\n'
        '}\n')
    inner_msg.c = 2
    assert (
        str(message) ==
        'map_int32_foreign_message {\n'
        '  key: 111\n'
        '  value {\n'
        '    c: 2\n'
        '  }\n'
        '}\n')

  def test_map_order_enforcement(self):
    message = map_unittest_pb2.TestMap()
    for letter in string.ascii_uppercase[13:26]:
      message.map_string_string[letter] = 'dummy'
    for letter in reversed(string.ascii_uppercase[0:13]):
      message.map_string_string[letter] = 'dummy'
    golden = ''.join(('map_string_string {\n  key: "%c"\n  value: "dummy"\n}\n'
                      % (letter,) for letter in string.ascii_uppercase))
    self.compare_to_golden_text(text_format.MessageToString(message), golden)

  # TODO(teboring): In c/137553523, not serializing default value for map entry
  # message has been fixed. This test needs to be disabled in order to submit
  # that cl. Add this back when c/137553523 has been submitted.
  # def testMapOrderSemantics(self):
  #   golden_lines = self.read_golden('map_test_data.txt')

  #   message = map_unittest_pb2.TestMap()
  #   text_format.ParseLines(golden_lines, message)
  #   candidate = text_format.MessageToString(message)
  #   # The Python implementation emits "1.0" for the double value that the C++
  #   # implementation emits as "1".
  #   candidate = candidate.replace('1.0', '1', 2)
  #   candidate = candidate.replace('0.0', '0', 2)
  #   self.assertMultiLineEqual(candidate, ''.join(golden_lines))


# Tests of proto2-only features (MessageSet, extensions, etc.).
class TestProto2(TextFormatBase):
    def test_print_message_set(self):
        message = unittest_mset_pb2.TestMessageSetContainer()
        ext1 = unittest_mset_pb2.TestMessageSetExtension1.message_set_extension
        ext2 = unittest_mset_pb2.TestMessageSetExtension2.message_set_extension
        message.message_set.Extensions[ext1].i = 23
        message.message_set.Extensions[ext2].str = 'foo'
        self.compare_to_golden_text(
            text_format.MessageToString(message), 'message_set {\n'
            '  [protobuf_unittest.TestMessageSetExtension1] {\n'
            '    i: 23\n'
            '  }\n'
            '  [protobuf_unittest.TestMessageSetExtension2] {\n'
            '    str: \"foo\"\n'
            '  }\n'
            '}\n')

        message = message_set_extensions_pb2.TestMessageSet()
        ext = message_set_extensions_pb2.message_set_extension3
        message.Extensions[ext].text = 'bar'
        self.compare_to_golden_text(
            text_format.MessageToString(message),
            '[google.protobuf.internal.TestMessageSetExtension3] {\n'
            '  text: \"bar\"\n'
            '}\n')

    def test_print_message_set_by_field_number(self):
        out = text_format.TextWriter(False)
        message = unittest_mset_pb2.TestMessageSetContainer()
        ext1 = unittest_mset_pb2.TestMessageSetExtension1.message_set_extension
        ext2 = unittest_mset_pb2.TestMessageSetExtension2.message_set_extension
        message.message_set.Extensions[ext1].i = 23
        message.message_set.Extensions[ext2].str = 'foo'
        text_format.PrintMessage(message, out, use_field_number=True)
        self.compare_to_golden_text(out.getvalue(), '1 {\n'
                                '  1545008 {\n'
                                '    15: 23\n'
                                '  }\n'
                                '  1547769 {\n'
                                '    25: \"foo\"\n'
                                '  }\n'
                                '}\n')
        out.close()

    def test_print_message_set_as_one_line(self):
        message = unittest_mset_pb2.TestMessageSetContainer()
        ext1 = unittest_mset_pb2.TestMessageSetExtension1.message_set_extension
        ext2 = unittest_mset_pb2.TestMessageSetExtension2.message_set_extension
        message.message_set.Extensions[ext1].i = 23
        message.message_set.Extensions[ext2].str = 'foo'
        self.compare_to_golden_text(
            text_format.MessageToString(message, as_one_line=True),
            'message_set {'
            ' [protobuf_unittest.TestMessageSetExtension1] {'
            ' i: 23'
            ' }'
            ' [protobuf_unittest.TestMessageSetExtension2] {'
            ' str: \"foo\"'
            ' }'
            ' }')

    def test_parse_message_set(self):
        message = unittest_pb2.TestAllTypes()
        text = ('repeated_uint64: 1\n' 'repeated_uint64: 2\n')
        text_format.Parse(text, message)
        assert 1 == message.repeated_uint64[0]
        assert 2 == message.repeated_uint64[1]

        message = unittest_mset_pb2.TestMessageSetContainer()
        text = ('message_set {\n'
                '  [protobuf_unittest.TestMessageSetExtension1] {\n'
                '    i: 23\n'
                '  }\n'
                '  [protobuf_unittest.TestMessageSetExtension2] {\n'
                '    str: \"foo\"\n'
                '  }\n'
                '}\n')
        text_format.Parse(text, message)
        ext1 = unittest_mset_pb2.TestMessageSetExtension1.message_set_extension
        ext2 = unittest_mset_pb2.TestMessageSetExtension2.message_set_extension
        assert 23 == message.message_set.Extensions[ext1].i
        assert 'foo' == message.message_set.Extensions[ext2].str

    def test_extension_inside_any_message(self):
        message = test_extend_any.TestAny()
        text = ('value {\n'
                '  [type.googleapis.com/google.protobuf.internal.TestAny] {\n'
                '    [google.protobuf.internal.TestAnyExtension1.extension1] {\n'
                '      i: 10\n'
                '    }\n'
                '  }\n'
                '}\n')
        text_format.Merge(text, message, descriptor_pool=descriptor_pool.Default())
        self.compare_to_golden_text(
            text_format.MessageToString(
                message, descriptor_pool=descriptor_pool.Default()),
            text)

    def test_parse_message_by_field_number(self):
        message = unittest_pb2.TestAllTypes()
        text = ('34: 1\n' 'repeated_uint64: 2\n')
        text_format.Parse(text, message, allow_field_number=True)
        assert 1 == message.repeated_uint64[0]
        assert 2 == message.repeated_uint64[1]

        message = unittest_mset_pb2.TestMessageSetContainer()
        text = ('1 {\n'
                '  1545008 {\n'
                '    15: 23\n'
                '  }\n'
                '  1547769 {\n'
                '    25: \"foo\"\n'
                '  }\n'
                '}\n')
        text_format.Parse(text, message, allow_field_number=True)
        ext1 = unittest_mset_pb2.TestMessageSetExtension1.message_set_extension
        ext2 = unittest_mset_pb2.TestMessageSetExtension2.message_set_extension
        assert 23 == message.message_set.Extensions[ext1].i
        assert 'foo' == message.message_set.Extensions[ext2].str

        # Can't parse field number without set allow_field_number=True.
        message = unittest_pb2.TestAllTypes()
        text = '34:1\n'
        with pytest.raises(
            text_format.ParseError,
            match=r'1:1 : Message type "\w+.TestAllTypes" has no field named '
                  r'"34".'):
            text_format.Parse(text, message)

        # Can't parse if field number is not found.
        text = '1234:1\n'
        with pytest.raises (
            text_format.ParseError,
            match=r'1:1 : Message type "\w+.TestAllTypes" has no field named '
                  r'"1234".'):
            text_format.Parse(text, message, allow_field_number=True)

    def test_print_all_extensions(self):
        message = unittest_pb2.TestAllExtensions()
        test_util.SetAllExtensions(message)
        self.compare_to_golden_file(
            self.remove_redundant_zeros(text_format.MessageToString(message)),
            'text_format_unittest_extensions_data.txt')

    def test_print_all_extensions_pointy(self):
        message = unittest_pb2.TestAllExtensions()
        test_util.SetAllExtensions(message)
        self.compare_to_golden_file(
            self.remove_redundant_zeros(text_format.MessageToString(
                message, pointy_brackets=True)),
            'text_format_unittest_extensions_data_pointy.txt')

    def test_parse_golden_extensions(self):
        golden_text = '\n'.join(self.read_golden(
            'text_format_unittest_extensions_data.txt'))
        parsed_message = unittest_pb2.TestAllExtensions()
        text_format.Parse(golden_text, parsed_message)

        message = unittest_pb2.TestAllExtensions()
        test_util.SetAllExtensions(message)
        assert message == parsed_message

    def test_parse_all_extensions(self):
        message = unittest_pb2.TestAllExtensions()
        test_util.SetAllExtensions(message)
        ascii_text = text_format.MessageToString(message)

        parsed_message = unittest_pb2.TestAllExtensions()
        text_format.Parse(ascii_text, parsed_message)
        assert message == parsed_message

    def test_parse_allowed_unknown_extension(self):
        # Skip over unknown extension correctly.
        message = unittest_mset_pb2.TestMessageSetContainer()
        text = ('message_set {\n'
                '  [unknown_extension] {\n'
                '    i: 23\n'
                '    repeated_i: []\n'
                '    bin: "\xe0"\n'
                '    [nested_unknown_ext]: {\n'
                '      i: 23\n'
                '      repeated_i: [1, 2]\n'
                '      x: x\n'
                '      test: "test_string"\n'
                '      floaty_float: -0.315\n'
                '      num: -inf\n'
                '      multiline_str: "abc"\n'
                '          "def"\n'
                '          "xyz."\n'
                '      [nested_unknown_ext.ext]: <\n'
                '        i: 23\n'
                '        i: 24\n'
                '        pointfloat: .3\n'
                '        test: "test_string"\n'
                '        repeated_test: ["test_string1", "test_string2"]\n'
                '        floaty_float: -0.315\n'
                '        num: -inf\n'
                '        long_string: "test" "test2" \n'
                '      >\n'
                '    }\n'
                '  }\n'
                '  [unknown_extension]: 5\n'
                '  [unknown_extension_with_number_field] {\n'
                '    1: "some_field"\n'
                '    2: -0.451\n'
                '  }\n'
                '}\n')
        text_format.Parse(text, message, allow_unknown_extension=True)
        golden = 'message_set {\n}\n'
        self.compare_to_golden_text(text_format.MessageToString(message), golden)

        # Catch parse errors in unknown extension.
        message = unittest_mset_pb2.TestMessageSetContainer()
        malformed = ('message_set {\n'
                    '  [unknown_extension] {\n'
                    '    i:\n'  # Missing value.
                    '  }\n'
                    '}\n')
        with pytest.raises(text_format.ParseError,
                           match='Invalid field value: }'):
            text_format.Parse(malformed, message, allow_unknown_extension=True)

        message = unittest_mset_pb2.TestMessageSetContainer()
        malformed = ('message_set {\n'
                    '  [unknown_extension] {\n'
                    '    str: "malformed string\n'  # Missing closing quote.
                    '  }\n'
                    '}\n')
        with pytest.raises(text_format.ParseError,
                           match='Invalid field value: "'):
            text_format.Parse(malformed, message, allow_unknown_extension=True)

        message = unittest_mset_pb2.TestMessageSetContainer()
        malformed = ('message_set {\n'
                    '  [unknown_extension] {\n'
                    '    str: "malformed\n multiline\n string\n'
                    '  }\n'
                    '}\n')
        with pytest.raises(text_format.ParseError,
                           match='Invalid field value: "'):
            text_format.Parse(malformed, message, allow_unknown_extension=True)

        message = unittest_mset_pb2.TestMessageSetContainer()
        malformed = ('message_set {\n'
                    '  [malformed_extension] <\n'
                    '    i: -5\n'
                    '  \n'  # Missing '>' here.
                    '}\n')
        with pytest.raises(text_format.ParseError,
                           match='5:1 : \'}\': Expected ">".'):
            text_format.Parse(malformed, message, allow_unknown_extension=True)

        # Don't allow unknown fields with allow_unknown_extension=True.
        message = unittest_mset_pb2.TestMessageSetContainer()
        malformed = ('message_set {\n'
                    '  unknown_field: true\n'
                    '}\n')
        with pytest.raises(
            text_format.ParseError,
            match='2:3 : Message type '
            '"proto2_wireformat_unittest.TestMessageSet" has no'
            ' field named "unknown_field".'):
            text_format.Parse(malformed, message, allow_unknown_extension=True)

        # Parse known extension correctly.
        message = unittest_mset_pb2.TestMessageSetContainer()
        text = ('message_set {\n'
                '  [protobuf_unittest.TestMessageSetExtension1] {\n'
                '    i: 23\n'
                '  }\n'
                '  [protobuf_unittest.TestMessageSetExtension2] {\n'
                '    str: \"foo\"\n'
                '  }\n'
                '}\n')
        text_format.Parse(text, message, allow_unknown_extension=True)
        ext1 = unittest_mset_pb2.TestMessageSetExtension1.message_set_extension
        ext2 = unittest_mset_pb2.TestMessageSetExtension2.message_set_extension
        assert 23 == message.message_set.Extensions[ext1].i
        assert 'foo' == message.message_set.Extensions[ext2].str

        # Handle Any messages inside unknown extensions.
        message = any_test_pb2.TestAny()
        text = ('any_value {\n'
                '  [type.googleapis.com/google.protobuf.internal.TestAny] {\n'
                '    [unknown_extension] {\n'
                '      str: "string"\n'
                '      any_value {\n'
                '        [type.googleapis.com/protobuf_unittest.OneString] {\n'
                '          data: "string"\n'
                '        }\n'
                '      }\n'
                '    }\n'
                '  }\n'
                '}\n'
                'int32_value: 123')
        text_format.Parse(text, message, allow_unknown_extension=True)
        assert 123 == message.int32_value

        # Fail if invalid Any message type url inside unknown extensions.
        message = any_test_pb2.TestAny()
        text = ('any_value {\n'
                '  [type.googleapis.com.invalid/google.protobuf.internal.TestAny] {\n'
                '    [unknown_extension] {\n'
                '      str: "string"\n'
                '      any_value {\n'
                '        [type.googleapis.com/protobuf_unittest.OneString] {\n'
                '          data: "string"\n'
                '        }\n'
                '      }\n'
                '    }\n'
                '  }\n'
                '}\n'
                'int32_value: 123')
        with pytest.raises(
            text_format.ParseError,
            match='[type.googleapis.com.invalid/google.protobuf.internal.TestAny]'):
            text_format.Parse(text, message, allow_unknown_extension=True)

    def test_parse_bad_identifier(self):
        message = unittest_pb2.TestAllTypes()
        text = ('optional_nested_message { "bb": 1 }')
        with pytest.raises(
            text_format.ParseError,
            match='1:27 : \'optional_nested_message { "bb": 1 }\': '
                  'Expected identifier or number, got "bb".'):
            text_format.Parse(text, message)

    def test_parse_bad_extension(self):
        message = unittest_pb2.TestAllExtensions()
        text = '[unknown_extension]: 8\n'
        with pytest.raises(
            text_format.ParseError,
            match='1:2 : Extension "unknown_extension" not registered.'):
            text_format.Parse(text, message)
        message = unittest_pb2.TestAllTypes()
        with pytest.raises(
            text_format.ParseError,
            match='1:2 : Message type "protobuf_unittest.TestAllTypes" does not have extensions.'):
            text_format.Parse(text, message)

    def test_parse_numeric_unknown_enum(self):
        message = unittest_pb2.TestAllTypes()
        text = 'optional_nested_enum: 100'
        with pytest.raises(
            text_format.ParseError,
            match=r'1:23 : \'optional_nested_enum: 100\': '
              r'Enum type "\w+.TestAllTypes.NestedEnum" '
              r'has no value with number 100.'):
            text_format.Parse(text, message)

    def test_merge_duplicate_extension_scalars(self):
        message = unittest_pb2.TestAllExtensions()
        text = ('[protobuf_unittest.optional_int32_extension]: 42 '
                '[protobuf_unittest.optional_int32_extension]: 67')
        text_format.Merge(text, message)
        assert (67 ==
                        message.Extensions[unittest_pb2.optional_int32_extension])

    def test_parse_duplicate_extension_scalars(self):
        message = unittest_pb2.TestAllExtensions()
        text = ('[protobuf_unittest.optional_int32_extension]: 42 '
                '[protobuf_unittest.optional_int32_extension]: 67')
        with pytest.raises(
            text_format.ParseError,
            match='1:96 : Message type "protobuf_unittest.TestAllExtensions" '
            'should not have multiple '
            '"protobuf_unittest.optional_int32_extension" extensions.'):
            text_format.Parse(text, message)

    def test_parse_duplicate_extension_messages(self):
        message = unittest_pb2.TestAllExtensions()
        text = ('[protobuf_unittest.optional_nested_message_extension]: {} '
                '[protobuf_unittest.optional_nested_message_extension]: {}')
        with pytest.raises(
            text_format.ParseError,
            match='1:114 : Message type "protobuf_unittest.TestAllExtensions" '
            'should not have multiple '
            '"protobuf_unittest.optional_nested_message_extension" extensions.'):
            text_format.Parse(text, message)

    def test_parse_group_not_closed(self):
        message = unittest_pb2.TestAllTypes()
        text = 'RepeatedGroup: <'
        with pytest.raises(text_format.ParseError,
                           match='1:16 : Expected ">".'):
            text_format.Parse(text, message)
        text = 'RepeatedGroup: {'
        with pytest.raises(text_format.ParseError,
                           match='1:16 : Expected "}".'):
            text_format.Parse(text, message)

    def test_parse_empty_group(self):
        message = unittest_pb2.TestAllTypes()
        text = 'OptionalGroup: {}'
        text_format.Parse(text, message)
        assert message.HasField('optionalgroup')

        message.Clear()

        message = unittest_pb2.TestAllTypes()
        text = 'OptionalGroup: <>'
        text_format.Parse(text, message)
        assert message.HasField('optionalgroup')

    # Maps aren't really proto2-only, but our test schema only has maps for
    # proto2.
    def test_parse_map(self):
        text = """
        map_int32_int32 {
          key: -123
          value: -456
        }
        map_int64_int64 {
          key: -8589934592
          value: -17179869184
        }
        map_uint32_uint32 {
          key: 123
          value: 456
        }
        map_uint64_uint64 {
          key: 8589934592
          value: 17179869184
        }
        map_string_string {
          key: "abc"
          value: "123"
        }
        map_int32_foreign_message {
          key: 111
          value {
            c: 5
          }
        }
        """
        message = map_unittest_pb2.TestMap()
        text_format.Parse(text, message)

        assert -456 == message.map_int32_int32[-123]
        assert -2**34 == message.map_int64_int64[-2**33]
        assert 456 == message.map_uint32_uint32[123]
        assert 2**34 == message.map_uint64_uint64[2**33]
        assert '123' == message.map_string_string['abc']
        assert 5 == message.map_int32_foreign_message[111].c


class TestProto3:
    def test_print_message_expand_any(self):
        packed_message = unittest_pb2.OneString()
        packed_message.data = 'string'
        message = any_test_pb2.TestAny()
        message.any_value.Pack(packed_message)
        assert (
            text_format.MessageToString(message,
                                        descriptor_pool=descriptor_pool.Default())
            ==  
            'any_value {\n'
            '  [type.googleapis.com/protobuf_unittest.OneString] {\n'
            '    data: "string"\n'
            '  }\n'
            '}\n')

    def test_top_any_message(self):
        packed_msg = unittest_pb2.OneString()
        msg = any_pb2.Any()
        msg.Pack(packed_msg)
        text = text_format.MessageToString(msg)
        other_msg = text_format.Parse(text, any_pb2.Any())
        assert msg == other_msg

    def test_print_message_expand_any_repeated(self):
        packed_message = unittest_pb2.OneString()
        message = any_test_pb2.TestAny()
        packed_message.data = 'string0'
        message.repeated_any_value.add().Pack(packed_message)
        packed_message.data = 'string1'
        message.repeated_any_value.add().Pack(packed_message)
        assert (
            text_format.MessageToString(message) ==
            'repeated_any_value {\n'
            '  [type.googleapis.com/protobuf_unittest.OneString] {\n'
            '    data: "string0"\n'
            '  }\n'
            '}\n'
            'repeated_any_value {\n'
            '  [type.googleapis.com/protobuf_unittest.OneString] {\n'
            '    data: "string1"\n'
            '  }\n'
            '}\n')

    def test_print_message_expand_any_descriptor_pool_missing_type(self):
        packed_message = unittest_pb2.OneString()
        packed_message.data = 'string'
        message = any_test_pb2.TestAny()
        message.any_value.Pack(packed_message)
        empty_pool = descriptor_pool.DescriptorPool()
        assert (
            text_format.MessageToString(message, descriptor_pool=empty_pool)
            ==
            'any_value {\n'
            '  type_url: "type.googleapis.com/protobuf_unittest.OneString"\n'
            '  value: "\\n\\006string"\n'
            '}\n')

    def test_print_message_expand_any_pointy_brackets(self):
        packed_message = unittest_pb2.OneString()
        packed_message.data = 'string'
        message = any_test_pb2.TestAny()
        message.any_value.Pack(packed_message)
        assert (
            text_format.MessageToString(message, pointy_brackets=True) ==
            'any_value <\n'
            '  [type.googleapis.com/protobuf_unittest.OneString] <\n'
            '    data: "string"\n'
            '  >\n'
            '>\n')

    def test_print_message_expand_any_as_one_line(self):
        packed_message = unittest_pb2.OneString()
        packed_message.data = 'string'
        message = any_test_pb2.TestAny()
        message.any_value.Pack(packed_message)
        assert (
            text_format.MessageToString(message, as_one_line=True) ==
            'any_value {'
            ' [type.googleapis.com/protobuf_unittest.OneString]'
            ' { data: "string" } '
            '}')

    def test_print_message_expand_any_as_one_line_pointy_brackets(self):
        packed_message = unittest_pb2.OneString()
        packed_message.data = 'string'
        message = any_test_pb2.TestAny()
        message.any_value.Pack(packed_message)
        assert (
            text_format.MessageToString(message,
                                        as_one_line=True,
                                        pointy_brackets=True,
                                        descriptor_pool=descriptor_pool.Default())
            ==
            'any_value <'
            ' [type.googleapis.com/protobuf_unittest.OneString]'
            ' < data: "string" > '
            '>')

    def test_print_and_parse_message_invalid_any(self):
        packed_message = unittest_pb2.OneString()
        packed_message.data = 'string'
        message = any_test_pb2.TestAny()
        message.any_value.Pack(packed_message)
        # Only include string after last '/' in type_url.
        message.any_value.type_url = message.any_value.TypeName()
        text = text_format.MessageToString(message)
        assert (
            text
            ==
            'any_value {\n'
            '  type_url: "protobuf_unittest.OneString"\n'
            '  value: "\\n\\006string"\n'
            '}\n')

        parsed_message = any_test_pb2.TestAny()
        text_format.Parse(text, parsed_message)
        assert message == parsed_message

    def test_unknown_enums(self):
        message = unittest_proto3_arena_pb2.TestAllTypes()
        message2 = unittest_proto3_arena_pb2.TestAllTypes()
        message.optional_nested_enum = 999
        text_string = text_format.MessageToString(message)
        text_format.Parse(text_string, message2)
        assert 999 == message2.optional_nested_enum

    def test_merge_expanded_any(self):
        message = any_test_pb2.TestAny()
        text = ('any_value {\n'
                '  [type.googleapis.com/protobuf_unittest.OneString] {\n'
                '    data: "string"\n'
                '  }\n'
                '}\n')
        text_format.Merge(text, message)
        packed_message = unittest_pb2.OneString()
        message.any_value.Unpack(packed_message)
        assert 'string' == packed_message.data
        message.Clear()
        text_format.Parse(text, message)
        packed_message = unittest_pb2.OneString()
        message.any_value.Unpack(packed_message)
        assert 'string' == packed_message.data

    def test_merge_expanded_any_repeated(self):
        message = any_test_pb2.TestAny()
        text = ('repeated_any_value {\n'
                '  [type.googleapis.com/protobuf_unittest.OneString] {\n'
                '    data: "string0"\n'
                '  }\n'
                '}\n'
                'repeated_any_value {\n'
                '  [type.googleapis.com/protobuf_unittest.OneString] {\n'
                '    data: "string1"\n'
                '  }\n'
                '}\n')
        text_format.Merge(text, message)
        packed_message = unittest_pb2.OneString()
        message.repeated_any_value[0].Unpack(packed_message)
        assert 'string0' == packed_message.data
        message.repeated_any_value[1].Unpack(packed_message)
        assert 'string1' == packed_message.data

    def test_merge_expanded_any_pointy_brackets(self):
        message = any_test_pb2.TestAny()
        text = ('any_value {\n'
                '  [type.googleapis.com/protobuf_unittest.OneString] <\n'
                '    data: "string"\n'
                '  >\n'
                '}\n')
        text_format.Merge(text, message)
        packed_message = unittest_pb2.OneString()
        message.any_value.Unpack(packed_message)
        assert 'string' == packed_message.data

    def test_merge_alternative_url(self):
        message = any_test_pb2.TestAny()
        text = ('any_value {\n'
                '  [type.otherapi.com/protobuf_unittest.OneString] {\n'
                '    data: "string"\n'
                '  }\n'
                '}\n')
        text_format.Merge(text, message)
        packed_message = unittest_pb2.OneString()
        assert ('type.otherapi.com/protobuf_unittest.OneString'
                == message.any_value.type_url)

    def test_merge_expanded_any_descriptor_pool_missing_type(self):
        message = any_test_pb2.TestAny()
        text = ('any_value {\n'
                '  [type.googleapis.com/protobuf_unittest.OneString] {\n'
                '    data: "string"\n'
                '  }\n'
                '}\n')
        with pytest.raises(
            text_format.ParseError,
            match='Type protobuf_unittest.OneString not found in descriptor pool'):
            empty_pool = descriptor_pool.DescriptorPool()
            text_format.Merge(text, message, descriptor_pool=empty_pool)

    def test_merge_unexpanded_any(self):
        text = ('any_value {\n'
                '  type_url: "type.googleapis.com/protobuf_unittest.OneString"\n'
                '  value: "\\n\\006string"\n'
                '}\n')
        message = any_test_pb2.TestAny()
        text_format.Merge(text, message)
        packed_message = unittest_pb2.OneString()
        message.any_value.Unpack(packed_message)
        assert 'string' == packed_message.data

    def test_merge_missing_any_end_token(self):
        message = any_test_pb2.TestAny()
        text = ('any_value {\n'
                '  [type.googleapis.com/protobuf_unittest.OneString] {\n'
                '    data: "string"\n')
        with pytest.raises(text_format.ParseError,
                           match='3:11 : Expected "}".'):
            text_format.Merge(text, message)

    def test_parse_expanded_any_list_value(self):
        any_msg = any_pb2.Any()
        any_msg.Pack(struct_pb2.ListValue())
        msg = any_test_pb2.TestAny(any_value=any_msg)
        text = ('any_value {\n'
                '  [type.googleapis.com/google.protobuf.ListValue] {}\n'
                '}\n')
        parsed_msg = text_format.Parse(text, any_test_pb2.TestAny())
        assert msg == parsed_msg

    def test_proto3_optional(self):
        msg = test_proto3_optional_pb2.TestProto3Optional()
        assert text_format.MessageToString(msg) == ''
        msg.optional_int32 = 0
        msg.optional_float = 0.0
        msg.optional_string = ''
        msg.optional_nested_message.bb = 0
        text = ('optional_int32: 0\n'
                'optional_float: 0.0\n'
                'optional_string: ""\n'
                'optional_nested_message {\n'
                '  bb: 0\n'
                '}\n')
        assert text_format.MessageToString(msg) == text
        msg2 = test_proto3_optional_pb2.TestProto3Optional()
        text_format.Parse(text, msg2)
        assert text_format.MessageToString(msg2) == text


class TestTokenizer:
    def test_simple_token_cases(self):
        text = ('identifier1:"string1"\n     \n\n'
                'identifier2 : \n \n123  \n  identifier3 :\'string\'\n'
                'identifiER_4 : 1.1e+2 ID5:-0.23 ID6:\'aaaa\\\'bbbb\'\n'
                'ID7 : "aa\\"bb"\n\n\n\n ID8: {A:inf B:-inf C:true D:false}\n'
                'ID9: 22 ID10: -111111111111111111 ID11: -22\n'
                'ID12: 2222222222222222222 ID13: 1.23456f ID14: 1.2e+2f '
                'false_bool:  0 true_BOOL:t \n true_bool1:  1 false_BOOL1:f '
                'False_bool: False True_bool: True X:iNf Y:-inF Z:nAN')
        tokenizer = text_format.Tokenizer(text.splitlines())
        methods = [(tokenizer.ConsumeIdentifier, 'identifier1'), ':',
                  (tokenizer.ConsumeString, 'string1'),
                  (tokenizer.ConsumeIdentifier, 'identifier2'), ':',
                  (tokenizer.ConsumeInteger, 123),
                  (tokenizer.ConsumeIdentifier, 'identifier3'), ':',
                  (tokenizer.ConsumeString, 'string'),
                  (tokenizer.ConsumeIdentifier, 'identifiER_4'), ':',
                  (tokenizer.ConsumeFloat, 1.1e+2),
                  (tokenizer.ConsumeIdentifier, 'ID5'), ':',
                  (tokenizer.ConsumeFloat, -0.23),
                  (tokenizer.ConsumeIdentifier, 'ID6'), ':',
                  (tokenizer.ConsumeString, 'aaaa\'bbbb'),
                  (tokenizer.ConsumeIdentifier, 'ID7'), ':',
                  (tokenizer.ConsumeString, 'aa\"bb'),
                  (tokenizer.ConsumeIdentifier, 'ID8'), ':', '{',
                  (tokenizer.ConsumeIdentifier, 'A'), ':',
                  (tokenizer.ConsumeFloat, float('inf')),
                  (tokenizer.ConsumeIdentifier, 'B'), ':',
                  (tokenizer.ConsumeFloat, -float('inf')),
                  (tokenizer.ConsumeIdentifier, 'C'), ':',
                  (tokenizer.ConsumeBool, True),
                  (tokenizer.ConsumeIdentifier, 'D'), ':',
                  (tokenizer.ConsumeBool, False), '}',
                  (tokenizer.ConsumeIdentifier, 'ID9'), ':',
                  (tokenizer.ConsumeInteger, 22),
                  (tokenizer.ConsumeIdentifier, 'ID10'), ':',
                  (tokenizer.ConsumeInteger, -111111111111111111),
                  (tokenizer.ConsumeIdentifier, 'ID11'), ':',
                  (tokenizer.ConsumeInteger, -22),
                  (tokenizer.ConsumeIdentifier, 'ID12'), ':',
                  (tokenizer.ConsumeInteger, 2222222222222222222),
                  (tokenizer.ConsumeIdentifier, 'ID13'), ':',
                  (tokenizer.ConsumeFloat, 1.23456),
                  (tokenizer.ConsumeIdentifier, 'ID14'), ':',
                  (tokenizer.ConsumeFloat, 1.2e+2),
                  (tokenizer.ConsumeIdentifier, 'false_bool'), ':',
                  (tokenizer.ConsumeBool, False),
                  (tokenizer.ConsumeIdentifier, 'true_BOOL'), ':',
                  (tokenizer.ConsumeBool, True),
                  (tokenizer.ConsumeIdentifier, 'true_bool1'), ':',
                  (tokenizer.ConsumeBool, True),
                  (tokenizer.ConsumeIdentifier, 'false_BOOL1'), ':',
                  (tokenizer.ConsumeBool, False),
                  (tokenizer.ConsumeIdentifier, 'False_bool'), ':',
                  (tokenizer.ConsumeBool, False),
                  (tokenizer.ConsumeIdentifier, 'True_bool'), ':',
                  (tokenizer.ConsumeBool, True),
                  (tokenizer.ConsumeIdentifier, 'X'), ':',
                  (tokenizer.ConsumeFloat, float('inf')),
                  (tokenizer.ConsumeIdentifier, 'Y'), ':',
                  (tokenizer.ConsumeFloat, float('-inf')),
                  (tokenizer.ConsumeIdentifier, 'Z'), ':',
                  (tokenizer.ConsumeFloat, float('nan'))]

        i = 0
        while not tokenizer.AtEnd():
            m = methods[i]
            if isinstance(m, str):
                token = tokenizer.token
                assert token == m
                tokenizer.NextToken()
            elif isinstance(m[1], float) and math.isnan(m[1]):
                assert math.isnan(m[0]())
            else:
                assert m[1] == m[0]()
            i += 1

    def test_consume_abstract_integers(self):
        # This test only tests the failures in the integer parsing methods as well
        # as the '0' special cases.
        int64_max = (1 << 63) - 1
        uint32_max = (1 << 32) - 1
        text = '-1 %d %d' % (uint32_max + 1, int64_max + 1)
        tokenizer = text_format.Tokenizer(text.splitlines())
        assert -1 == tokenizer.ConsumeInteger()

        assert uint32_max + 1 == tokenizer.ConsumeInteger()

        assert int64_max + 1 == tokenizer.ConsumeInteger()
        assert tokenizer.AtEnd()

        text = '-0 0 0 1.2'
        tokenizer = text_format.Tokenizer(text.splitlines())
        assert 0 == tokenizer.ConsumeInteger()
        assert 0 == tokenizer.ConsumeInteger()
        assert True == tokenizer.TryConsumeInteger()
        assert False == tokenizer.TryConsumeInteger()
        with pytest.raises(text_format.ParseError):
            tokenizer.ConsumeInteger()
        assert 1.2 == tokenizer.ConsumeFloat()
        assert tokenizer.AtEnd()

    def test_consume_integers(self):
        # This test only tests the failures in the integer parsing methods as well
        # as the '0' special cases.
        int64_max = (1 << 63) - 1
        uint32_max = (1 << 32) - 1
        text = '-1 %d %d' % (uint32_max + 1, int64_max + 1)
        tokenizer = text_format.Tokenizer(text.splitlines())
        pytest.raises(text_format.ParseError,
                          text_format._ConsumeUint32, tokenizer)
        pytest.raises(text_format.ParseError,
                          text_format._ConsumeUint64, tokenizer)
        assert -1 == text_format._ConsumeInt32(tokenizer)

        pytest.raises(text_format.ParseError,
                          text_format._ConsumeUint32, tokenizer)
        pytest.raises(text_format.ParseError,
                          text_format._ConsumeInt32, tokenizer)
        assert uint32_max + 1 == text_format._ConsumeInt64(tokenizer)

        pytest.raises(text_format.ParseError,
                          text_format._ConsumeInt64, tokenizer)
        assert int64_max + 1 == text_format._ConsumeUint64(tokenizer)
        assert tokenizer.AtEnd()

        text = '-0 -0 0 0'
        tokenizer = text_format.Tokenizer(text.splitlines())
        assert 0 == text_format._ConsumeUint32(tokenizer)
        assert 0 == text_format._ConsumeUint64(tokenizer)
        assert 0 == text_format._ConsumeUint32(tokenizer)
        assert 0 == text_format._ConsumeUint64(tokenizer)
        assert tokenizer.AtEnd()

    def test_consume_octal_integers(self):
        """Test support for C style octal integers."""
        text = '00 -00 04 0755 -010 007 -0033 08 -09 01'
        tokenizer = text_format.Tokenizer(text.splitlines())
        assert 0 == tokenizer.ConsumeInteger()
        assert 0 == tokenizer.ConsumeInteger()
        assert 4 == tokenizer.ConsumeInteger()
        assert 0o755 == tokenizer.ConsumeInteger()
        assert -0o10 == tokenizer.ConsumeInteger()
        assert 7 == tokenizer.ConsumeInteger()
        assert -0o033 == tokenizer.ConsumeInteger()
        with pytest.raises(text_format.ParseError):
          tokenizer.ConsumeInteger()  # 08
        tokenizer.NextToken()
        with pytest.raises(text_format.ParseError):
              tokenizer.ConsumeInteger()  # -09
        tokenizer.NextToken()
        assert 1 == tokenizer.ConsumeInteger()
        assert tokenizer.AtEnd()

    def test_consume_byte_string(self):
        text = '"string1\''
        tokenizer = text_format.Tokenizer(text.splitlines())
        pytest.raises(text_format.ParseError, tokenizer.ConsumeByteString)

        text = 'string1"'
        tokenizer = text_format.Tokenizer(text.splitlines())
        pytest.raises(text_format.ParseError, tokenizer.ConsumeByteString)

        text = '\n"\\xt"'
        tokenizer = text_format.Tokenizer(text.splitlines())
        pytest.raises(text_format.ParseError, tokenizer.ConsumeByteString)

        text = '\n"\\"'
        tokenizer = text_format.Tokenizer(text.splitlines())
        pytest.raises(text_format.ParseError, tokenizer.ConsumeByteString)

        text = '\n"\\x"'
        tokenizer = text_format.Tokenizer(text.splitlines())
        pytest.raises(text_format.ParseError, tokenizer.ConsumeByteString)

    def test_consume_bool(self):
        text = 'not-a-bool'
        tokenizer = text_format.Tokenizer(text.splitlines())
        pytest.raises(text_format.ParseError, tokenizer.ConsumeBool)

    def test_skip_comment(self):
        tokenizer = text_format.Tokenizer('# some comment'.splitlines())
        assert tokenizer.AtEnd()
        pytest.raises(text_format.ParseError, tokenizer.ConsumeComment)

    def test_consume_comment(self):
        tokenizer = text_format.Tokenizer('# some comment'.splitlines(),
                                          skip_comments=False)
        assert not tokenizer.AtEnd()
        assert '# some comment' == tokenizer.ConsumeComment()
        assert tokenizer.AtEnd()

    def test_consume_two_comments(self):
        text = '# some comment\n# another comment'
        tokenizer = text_format.Tokenizer(text.splitlines(), skip_comments=False)
        assert '# some comment' == tokenizer.ConsumeComment()
        assert not tokenizer.AtEnd()
        assert '# another comment' == tokenizer.ConsumeComment()
        assert tokenizer.AtEnd()

    def test_consume_trailing_comment(self):
        text = 'some_number: 4\n# some comment'
        tokenizer = text_format.Tokenizer(text.splitlines(), skip_comments=False)
        pytest.raises(text_format.ParseError, tokenizer.ConsumeComment)

        assert 'some_number' == tokenizer.ConsumeIdentifier()
        assert tokenizer.token == ':'
        tokenizer.NextToken()
        pytest.raises(text_format.ParseError, tokenizer.ConsumeComment)
        assert 4 == tokenizer.ConsumeInteger()
        assert not tokenizer.AtEnd()

        assert '# some comment' == tokenizer.ConsumeComment()
        assert tokenizer.AtEnd()

    def test_consume_line_comment(self):
        tokenizer = text_format.Tokenizer('# some comment'.splitlines(),
                                          skip_comments=False)
        assert not tokenizer.AtEnd()
        assert ((False, '# some comment')
                == tokenizer.ConsumeCommentOrTrailingComment())
        assert tokenizer.AtEnd()

    def test_consume_two_line_comments(self):
        text = '# some comment\n# another comment'
        tokenizer = text_format.Tokenizer(text.splitlines(), skip_comments=False)
        assert ((False, '# some comment')
                == tokenizer.ConsumeCommentOrTrailingComment())
        assert not tokenizer.AtEnd()
        assert ((False, '# another comment')
                == tokenizer.ConsumeCommentOrTrailingComment())
        assert tokenizer.AtEnd()

    def test_consume_and_check_trailing_comment(self):
        text = 'some_number: 4  # some comment'  # trailing comment on the same line
        tokenizer = text_format.Tokenizer(text.splitlines(), skip_comments=False)
        pytest.raises(text_format.ParseError,
                          tokenizer.ConsumeCommentOrTrailingComment)

        assert 'some_number' == tokenizer.ConsumeIdentifier()
        assert tokenizer.token == ':'
        tokenizer.NextToken()
        pytest.raises(text_format.ParseError,
                          tokenizer.ConsumeCommentOrTrailingComment)
        assert 4 == tokenizer.ConsumeInteger()
        assert not tokenizer.AtEnd()

        assert ((True, '# some comment')
                == tokenizer.ConsumeCommentOrTrailingComment())
        assert tokenizer.AtEnd()

    def test_hashin_comment(self):
        text = 'some_number: 4  # some comment # not a new comment'
        tokenizer = text_format.Tokenizer(text.splitlines(), skip_comments=False)
        assert 'some_number' == tokenizer.ConsumeIdentifier()
        assert tokenizer.token == ':'
        tokenizer.NextToken()
        assert 4 == tokenizer.ConsumeInteger()
        assert ((True, '# some comment # not a new comment')
                == tokenizer.ConsumeCommentOrTrailingComment())
        assert tokenizer.AtEnd()

    def test_huge_string(self):
        # With pathologic backtracking, fails with Forge OOM.
        text = '"' + 'a' * (10 * 1024 * 1024) + '"'
        tokenizer = text_format.Tokenizer(text.splitlines(), skip_comments=False)
        tokenizer.ConsumeString()


# Tests for pretty printer functionality.
@pytest.mark.parametrize(
    'message_module',
    [unittest_pb2, unittest_proto3_arena_pb2],
    ids=['_proto2', '_proto3'],
)
class TestPrettyPrinter(TextFormatBase):
    def test_pretty_print_no_match(self, message_module):
        def printer(message, indent, as_one_line):
            del message, indent, as_one_line
            return None

        message = message_module.TestAllTypes()
        msg = message.repeated_nested_message.add()
        msg.bb = 42
        self.compare_to_golden_text(
            text_format.MessageToString(
                message, as_one_line=True, message_formatter=printer),
            'repeated_nested_message { bb: 42 }')

    def test_pretty_print_one_line(self, message_module):
        def printer(m, indent, as_one_line):
            del indent, as_one_line
            if m.DESCRIPTOR == message_module.TestAllTypes.NestedMessage.DESCRIPTOR:
                return 'My lucky number is %s' % m.bb

        message = message_module.TestAllTypes()
        msg = message.repeated_nested_message.add()
        msg.bb = 42
        self.compare_to_golden_text(
            text_format.MessageToString(
                message, as_one_line=True, message_formatter=printer),
            'repeated_nested_message { My lucky number is 42 }')

    def test_pretty_print_multi_line(self, message_module):
        def printer(m, indent, as_one_line):
            if m.DESCRIPTOR == message_module.TestAllTypes.NestedMessage.DESCRIPTOR:
                line_deliminator = (' ' if as_one_line else '\n') + ' ' * indent
                return 'My lucky number is:%s%s' % (line_deliminator, m.bb)
            return None

        message = message_module.TestAllTypes()
        msg = message.repeated_nested_message.add()
        msg.bb = 42
        self.compare_to_golden_text(
            text_format.MessageToString(
                message, as_one_line=True, message_formatter=printer),
            'repeated_nested_message { My lucky number is: 42 }')
        self.compare_to_golden_text(
            text_format.MessageToString(
                message, as_one_line=False, message_formatter=printer),
            'repeated_nested_message {\n  My lucky number is:\n  42\n}\n')

    def test_pretty_print_entire_message(self, message_module):
        def printer(m, indent, as_one_line):
            del indent, as_one_line
            if m.DESCRIPTOR == message_module.TestAllTypes.DESCRIPTOR:
                return 'The is the message!'
            return None

        message = message_module.TestAllTypes()
        self.compare_to_golden_text(
            text_format.MessageToString(
                message, as_one_line=False, message_formatter=printer),
            'The is the message!\n')
        self.compare_to_golden_text(
            text_format.MessageToString(
                message, as_one_line=True, message_formatter=printer),
            'The is the message!')

    def test_pretty_print_multiple_parts(self, message_module):
        def printer(m, indent, as_one_line):
            del indent, as_one_line
            if m.DESCRIPTOR == message_module.TestAllTypes.NestedMessage.DESCRIPTOR:
                return 'My lucky number is %s' % m.bb
            return None

        message = message_module.TestAllTypes()
        message.optional_int32 = 61
        msg = message.repeated_nested_message.add()
        msg.bb = 42
        msg = message.repeated_nested_message.add()
        msg.bb = 99
        msg = message.optional_nested_message
        msg.bb = 1
        self.compare_to_golden_text(
            text_format.MessageToString(
                message, as_one_line=True, message_formatter=printer),
            ('optional_int32: 61 '
            'optional_nested_message { My lucky number is 1 } '
            'repeated_nested_message { My lucky number is 42 } '
            'repeated_nested_message { My lucky number is 99 }'))

        out = text_format.TextWriter(False)
        text_format.PrintField(
            message_module.TestAllTypes.DESCRIPTOR.fields_by_name[
                'optional_nested_message'],
            message.optional_nested_message,
            out,
            message_formatter=printer)
        assert (
            'optional_nested_message {\n  My lucky number is 1\n}\n'
            == out.getvalue())
        out.close()

        out = text_format.TextWriter(False)
        text_format.PrintFieldValue(
            message_module.TestAllTypes.DESCRIPTOR.fields_by_name[
                'optional_nested_message'],
            message.optional_nested_message,
            out,
            message_formatter=printer)
        assert (
            '{\n  My lucky number is 1\n}'
            == out.getvalue())
        out.close()


class TestWhitespace(TextFormatBase):
    def setup_method(self, method):
        self.out = text_format.TextWriter(False)
        self.message = unittest_pb2.NestedTestAllTypes()
        self.message.child.payload.optional_string = 'value'
        self.field = self.message.DESCRIPTOR.fields_by_name['child']
        self.value = self.message.child

    def teardown_method(self, method):
        self.out.close()

    def test_message_to_string(self):
        self.compare_to_golden_text(
            text_format.MessageToString(self.message),
            textwrap.dedent("""\
                child {
                  payload {
                    optional_string: "value"
                  }
                }
                """))

    def test_print_message(self):
        text_format.PrintMessage(self.message, self.out)
        self.compare_to_golden_text(
            self.out.getvalue(),
            textwrap.dedent("""\
                child {
                  payload {
                    optional_string: "value"
                  }
                }
                """))

    def test_print_field(self):
        text_format.PrintField(self.field, self.value, self.out)
        self.compare_to_golden_text(
            self.out.getvalue(),
            textwrap.dedent("""\
                child {
                  payload {
                    optional_string: "value"
                  }
                }
                """))

    def test_print_field_value(self):
        text_format.PrintFieldValue(
            self.field, self.value, self.out)
        self.compare_to_golden_text(
            self.out.getvalue(),
            textwrap.dedent("""\
                {
                  payload {
                    optional_string: "value"
                  }
                }"""))


class TestOptionalColonMessageToString:
    def test_force_print_optional_colon(self):
        packed_message = unittest_pb2.OneString()
        packed_message.data = 'string'
        message = any_test_pb2.TestAny()
        message.any_value.Pack(packed_message)
        output = text_format.MessageToString(
            message,
            force_colon=True)
        expected = ('any_value: {\n'
                    '  [type.googleapis.com/protobuf_unittest.OneString]: {\n'
                    '    data: "string"\n'
                    '  }\n'
                    '}\n')
        assert expected == output

    def test_print_short_format_repeated_fields(self):
        message = unittest_pb2.TestAllTypes()
        message.repeated_int32.append(1)
        output = text_format.MessageToString(
            message, use_short_repeated_primitives=True, force_colon=True)
        assert 'repeated_int32: [1]\n' == output
