#! /usr/bin/env python
# -*- coding: utf-8 -*-
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

"""Test for google.protobuf.text_format."""

__author__ = 'kenton@google.com (Kenton Varda)'


import math
import re
import six
import string

try:
  import unittest2 as unittest  # PY26, pylint: disable=g-import-not-at-top
except ImportError:
  import unittest  # pylint: disable=g-import-not-at-top

from google.protobuf.internal import _parameterized

from google.protobuf import any_pb2
from google.protobuf import any_test_pb2
from google.protobuf import map_unittest_pb2
from google.protobuf import unittest_mset_pb2
from google.protobuf import unittest_pb2
from google.protobuf import unittest_proto3_arena_pb2
from google.protobuf.internal import api_implementation
from google.protobuf.internal import any_test_pb2 as test_extend_any
from google.protobuf.internal import message_set_extensions_pb2
from google.protobuf.internal import test_util
from google.protobuf import descriptor_pool
from google.protobuf import text_format


# Low-level nuts-n-bolts tests.
class SimpleTextFormatTests(unittest.TestCase):

  # The members of _QUOTES are formatted into a regexp template that
  # expects single characters.  Therefore it's an error (in addition to being
  # non-sensical in the first place) to try to specify a "quote mark" that is
  # more than one character.
  def testQuoteMarksAreSingleChars(self):
    for quote in text_format._QUOTES:
      self.assertEqual(1, len(quote))


# Base class with some common functionality.
class TextFormatBase(unittest.TestCase):

  def ReadGolden(self, golden_filename):
    with test_util.GoldenFile(golden_filename) as f:
      return (f.readlines() if str is bytes else  # PY3
              [golden_line.decode('utf-8') for golden_line in f])

  def CompareToGoldenFile(self, text, golden_filename):
    golden_lines = self.ReadGolden(golden_filename)
    self.assertMultiLineEqual(text, ''.join(golden_lines))

  def CompareToGoldenText(self, text, golden_text):
    self.assertEqual(text, golden_text)

  def RemoveRedundantZeros(self, text):
    # Some platforms print 1e+5 as 1e+005.  This is fine, but we need to remove
    # these zeros in order to match the golden file.
    text = text.replace('e+0','e+').replace('e+0','e+') \
               .replace('e-0','e-').replace('e-0','e-')
    # Floating point fields are printed with .0 suffix even if they are
    # actualy integer numbers.
    text = re.compile(r'\.0$', re.MULTILINE).sub('', text)
    return text


@_parameterized.parameters((unittest_pb2), (unittest_proto3_arena_pb2))
class TextFormatTest(TextFormatBase):

  def testPrintExotic(self, message_module):
    message = message_module.TestAllTypes()
    message.repeated_int64.append(-9223372036854775808)
    message.repeated_uint64.append(18446744073709551615)
    message.repeated_double.append(123.456)
    message.repeated_double.append(1.23e22)
    message.repeated_double.append(1.23e-18)
    message.repeated_string.append('\000\001\a\b\f\n\r\t\v\\\'"')
    message.repeated_string.append(u'\u00fc\ua71f')
    self.CompareToGoldenText(
        self.RemoveRedundantZeros(text_format.MessageToString(message)),
        'repeated_int64: -9223372036854775808\n'
        'repeated_uint64: 18446744073709551615\n'
        'repeated_double: 123.456\n'
        'repeated_double: 1.23e+22\n'
        'repeated_double: 1.23e-18\n'
        'repeated_string:'
        ' "\\000\\001\\007\\010\\014\\n\\r\\t\\013\\\\\\\'\\""\n'
        'repeated_string: "\\303\\274\\352\\234\\237"\n')

  def testPrintExoticUnicodeSubclass(self, message_module):

    class UnicodeSub(six.text_type):
      pass

    message = message_module.TestAllTypes()
    message.repeated_string.append(UnicodeSub(u'\u00fc\ua71f'))
    self.CompareToGoldenText(
        text_format.MessageToString(message),
        'repeated_string: "\\303\\274\\352\\234\\237"\n')

  def testPrintNestedMessageAsOneLine(self, message_module):
    message = message_module.TestAllTypes()
    msg = message.repeated_nested_message.add()
    msg.bb = 42
    self.CompareToGoldenText(
        text_format.MessageToString(message, as_one_line=True),
        'repeated_nested_message { bb: 42 }')

  def testPrintRepeatedFieldsAsOneLine(self, message_module):
    message = message_module.TestAllTypes()
    message.repeated_int32.append(1)
    message.repeated_int32.append(1)
    message.repeated_int32.append(3)
    message.repeated_string.append('Google')
    message.repeated_string.append('Zurich')
    self.CompareToGoldenText(
        text_format.MessageToString(message, as_one_line=True),
        'repeated_int32: 1 repeated_int32: 1 repeated_int32: 3 '
        'repeated_string: "Google" repeated_string: "Zurich"')

  def testPrintNestedNewLineInStringAsOneLine(self, message_module):
    message = message_module.TestAllTypes()
    message.optional_string = 'a\nnew\nline'
    self.CompareToGoldenText(
        text_format.MessageToString(message, as_one_line=True),
        'optional_string: "a\\nnew\\nline"')

  def testPrintExoticAsOneLine(self, message_module):
    message = message_module.TestAllTypes()
    message.repeated_int64.append(-9223372036854775808)
    message.repeated_uint64.append(18446744073709551615)
    message.repeated_double.append(123.456)
    message.repeated_double.append(1.23e22)
    message.repeated_double.append(1.23e-18)
    message.repeated_string.append('\000\001\a\b\f\n\r\t\v\\\'"')
    message.repeated_string.append(u'\u00fc\ua71f')
    self.CompareToGoldenText(
        self.RemoveRedundantZeros(text_format.MessageToString(
            message, as_one_line=True)),
        'repeated_int64: -9223372036854775808'
        ' repeated_uint64: 18446744073709551615'
        ' repeated_double: 123.456'
        ' repeated_double: 1.23e+22'
        ' repeated_double: 1.23e-18'
        ' repeated_string: '
        '"\\000\\001\\007\\010\\014\\n\\r\\t\\013\\\\\\\'\\""'
        ' repeated_string: "\\303\\274\\352\\234\\237"')

  def testRoundTripExoticAsOneLine(self, message_module):
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
    self.assertIs(r, parsed_message)
    self.assertEqual(message, parsed_message)

    # Test as_utf8 = True.
    wire_text = text_format.MessageToString(message,
                                            as_one_line=True,
                                            as_utf8=True)
    parsed_message = message_module.TestAllTypes()
    r = text_format.Parse(wire_text, parsed_message)
    self.assertIs(r, parsed_message)
    self.assertEqual(message, parsed_message,
                     '\n%s != %s' % (message, parsed_message))

  def testPrintRawUtf8String(self, message_module):
    message = message_module.TestAllTypes()
    message.repeated_string.append(u'\u00fc\ua71f')
    text = text_format.MessageToString(message, as_utf8=True)
    self.CompareToGoldenText(text, 'repeated_string: "\303\274\352\234\237"\n')
    parsed_message = message_module.TestAllTypes()
    text_format.Parse(text, parsed_message)
    self.assertEqual(message, parsed_message,
                     '\n%s != %s' % (message, parsed_message))

  def testPrintFloatFormat(self, message_module):
    # Check that float_format argument is passed to sub-message formatting.
    message = message_module.NestedTestAllTypes()
    # We use 1.25 as it is a round number in binary.  The proto 32-bit float
    # will not gain additional imprecise digits as a 64-bit Python float and
    # show up in its str.  32-bit 1.2 is noisy when extended to 64-bit:
    #  >>> struct.unpack('f', struct.pack('f', 1.2))[0]
    #  1.2000000476837158
    #  >>> struct.unpack('f', struct.pack('f', 1.25))[0]
    #  1.25
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
    self.CompareToGoldenText(
        self.RemoveRedundantZeros(text_message),
        'payload {{\n  {0}\n  {1}\n  {2}\n  {3}\n}}\n'.format(
            *formatted_fields))
    # as_one_line=True is a separate code branch where float_format is passed.
    text_message = text_format.MessageToString(message,
                                               as_one_line=True,
                                               float_format='.15g')
    self.CompareToGoldenText(
        self.RemoveRedundantZeros(text_message),
        'payload {{ {0} {1} {2} {3} }}'.format(*formatted_fields))

  def testMessageToString(self, message_module):
    message = message_module.ForeignMessage()
    message.c = 123
    self.assertEqual('c: 123\n', str(message))

  def testPrintField(self, message_module):
    message = message_module.TestAllTypes()
    field = message.DESCRIPTOR.fields_by_name['optional_float']
    value = message.optional_float
    out = text_format.TextWriter(False)
    text_format.PrintField(field, value, out)
    self.assertEqual('optional_float: 0.0\n', out.getvalue())
    out.close()
    # Test Printer
    out = text_format.TextWriter(False)
    printer = text_format._Printer(out)
    printer.PrintField(field, value)
    self.assertEqual('optional_float: 0.0\n', out.getvalue())
    out.close()

  def testPrintFieldValue(self, message_module):
    message = message_module.TestAllTypes()
    field = message.DESCRIPTOR.fields_by_name['optional_float']
    value = message.optional_float
    out = text_format.TextWriter(False)
    text_format.PrintFieldValue(field, value, out)
    self.assertEqual('0.0', out.getvalue())
    out.close()
    # Test Printer
    out = text_format.TextWriter(False)
    printer = text_format._Printer(out)
    printer.PrintFieldValue(field, value)
    self.assertEqual('0.0', out.getvalue())
    out.close()

  def testParseAllFields(self, message_module):
    message = message_module.TestAllTypes()
    test_util.SetAllFields(message)
    ascii_text = text_format.MessageToString(message)

    parsed_message = message_module.TestAllTypes()
    text_format.Parse(ascii_text, parsed_message)
    self.assertEqual(message, parsed_message)
    if message_module is unittest_pb2:
      test_util.ExpectAllFieldsSet(self, message)

  def testParseAndMergeUtf8(self, message_module):
    message = message_module.TestAllTypes()
    test_util.SetAllFields(message)
    ascii_text = text_format.MessageToString(message)
    ascii_text = ascii_text.encode('utf-8')

    parsed_message = message_module.TestAllTypes()
    text_format.Parse(ascii_text, parsed_message)
    self.assertEqual(message, parsed_message)
    if message_module is unittest_pb2:
      test_util.ExpectAllFieldsSet(self, message)

    parsed_message.Clear()
    text_format.Merge(ascii_text, parsed_message)
    self.assertEqual(message, parsed_message)
    if message_module is unittest_pb2:
      test_util.ExpectAllFieldsSet(self, message)

    if six.PY2:
      msg2 = message_module.TestAllTypes()
      text = (u'optional_string: "café"')
      text_format.Merge(text, msg2)
      self.assertEqual(msg2.optional_string, u'café')
      msg2.Clear()
      text_format.Parse(text, msg2)
      self.assertEqual(msg2.optional_string, u'café')

  def testParseExotic(self, message_module):
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

    self.assertEqual(-9223372036854775808, message.repeated_int64[0])
    self.assertEqual(18446744073709551615, message.repeated_uint64[0])
    self.assertEqual(123.456, message.repeated_double[0])
    self.assertEqual(1.23e22, message.repeated_double[1])
    self.assertEqual(1.23e-18, message.repeated_double[2])
    self.assertEqual('\000\001\a\b\f\n\r\t\v\\\'"', message.repeated_string[0])
    self.assertEqual('foocorgegrault', message.repeated_string[1])
    self.assertEqual(u'\u00fc\ua71f', message.repeated_string[2])
    self.assertEqual(u'\u00fc', message.repeated_string[3])

  def testParseTrailingCommas(self, message_module):
    message = message_module.TestAllTypes()
    text = ('repeated_int64: 100;\n'
            'repeated_int64: 200;\n'
            'repeated_int64: 300,\n'
            'repeated_string: "one",\n'
            'repeated_string: "two";\n')
    text_format.Parse(text, message)

    self.assertEqual(100, message.repeated_int64[0])
    self.assertEqual(200, message.repeated_int64[1])
    self.assertEqual(300, message.repeated_int64[2])
    self.assertEqual(u'one', message.repeated_string[0])
    self.assertEqual(u'two', message.repeated_string[1])

  def testParseRepeatedScalarShortFormat(self, message_module):
    message = message_module.TestAllTypes()
    text = ('repeated_int64: [100, 200];\n'
            'repeated_int64: []\n'
            'repeated_int64: 300,\n'
            'repeated_string: ["one", "two"];\n')
    text_format.Parse(text, message)

    self.assertEqual(100, message.repeated_int64[0])
    self.assertEqual(200, message.repeated_int64[1])
    self.assertEqual(300, message.repeated_int64[2])
    self.assertEqual(u'one', message.repeated_string[0])
    self.assertEqual(u'two', message.repeated_string[1])

  def testParseRepeatedMessageShortFormat(self, message_module):
    message = message_module.TestAllTypes()
    text = ('repeated_nested_message: [{bb: 100}, {bb: 200}],\n'
            'repeated_nested_message: {bb: 300}\n'
            'repeated_nested_message [{bb: 400}];\n')
    text_format.Parse(text, message)

    self.assertEqual(100, message.repeated_nested_message[0].bb)
    self.assertEqual(200, message.repeated_nested_message[1].bb)
    self.assertEqual(300, message.repeated_nested_message[2].bb)
    self.assertEqual(400, message.repeated_nested_message[3].bb)

  def testParseEmptyText(self, message_module):
    message = message_module.TestAllTypes()
    text = ''
    text_format.Parse(text, message)
    self.assertEqual(message_module.TestAllTypes(), message)

  def testParseInvalidUtf8(self, message_module):
    message = message_module.TestAllTypes()
    text = 'repeated_string: "\\xc3\\xc3"'
    with self.assertRaises(text_format.ParseError) as e:
      text_format.Parse(text, message)
    self.assertEqual(e.exception.GetLine(), 1)
    self.assertEqual(e.exception.GetColumn(), 28)

  def testParseSingleWord(self, message_module):
    message = message_module.TestAllTypes()
    text = 'foo'
    six.assertRaisesRegex(self, text_format.ParseError, (
        r'1:1 : Message type "\w+.TestAllTypes" has no field named '
        r'"foo".'), text_format.Parse, text, message)

  def testParseUnknownField(self, message_module):
    message = message_module.TestAllTypes()
    text = 'unknown_field: 8\n'
    six.assertRaisesRegex(self, text_format.ParseError, (
        r'1:1 : Message type "\w+.TestAllTypes" has no field named '
        r'"unknown_field".'), text_format.Parse, text, message)

  def testParseBadEnumValue(self, message_module):
    message = message_module.TestAllTypes()
    text = 'optional_nested_enum: BARR'
    six.assertRaisesRegex(self, text_format.ParseError,
                          (r'1:23 : Enum type "\w+.TestAllTypes.NestedEnum" '
                           r'has no value named BARR.'), text_format.Parse,
                          text, message)

  def testParseBadIntValue(self, message_module):
    message = message_module.TestAllTypes()
    text = 'optional_int32: bork'
    six.assertRaisesRegex(self, text_format.ParseError,
                          ('1:17 : Couldn\'t parse integer: bork'),
                          text_format.Parse, text, message)

  def testParseStringFieldUnescape(self, message_module):
    message = message_module.TestAllTypes()
    text = r'''repeated_string: "\xf\x62"
               repeated_string: "\\xf\\x62"
               repeated_string: "\\\xf\\\x62"
               repeated_string: "\\\\xf\\\\x62"
               repeated_string: "\\\\\xf\\\\\x62"
               repeated_string: "\x5cx20"'''

    text_format.Parse(text, message)

    SLASH = '\\'
    self.assertEqual('\x0fb', message.repeated_string[0])
    self.assertEqual(SLASH + 'xf' + SLASH + 'x62', message.repeated_string[1])
    self.assertEqual(SLASH + '\x0f' + SLASH + 'b', message.repeated_string[2])
    self.assertEqual(SLASH + SLASH + 'xf' + SLASH + SLASH + 'x62',
                     message.repeated_string[3])
    self.assertEqual(SLASH + SLASH + '\x0f' + SLASH + SLASH + 'b',
                     message.repeated_string[4])
    self.assertEqual(SLASH + 'x20', message.repeated_string[5])

  def testMergeDuplicateScalars(self, message_module):
    message = message_module.TestAllTypes()
    text = ('optional_int32: 42 ' 'optional_int32: 67')
    r = text_format.Merge(text, message)
    self.assertIs(r, message)
    self.assertEqual(67, message.optional_int32)

  def testMergeDuplicateNestedMessageScalars(self, message_module):
    message = message_module.TestAllTypes()
    text = ('optional_nested_message { bb: 1 } '
            'optional_nested_message { bb: 2 }')
    r = text_format.Merge(text, message)
    self.assertTrue(r is message)
    self.assertEqual(2, message.optional_nested_message.bb)

  def testParseOneof(self, message_module):
    m = message_module.TestAllTypes()
    m.oneof_uint32 = 11
    m2 = message_module.TestAllTypes()
    text_format.Parse(text_format.MessageToString(m), m2)
    self.assertEqual('oneof_uint32', m2.WhichOneof('oneof_field'))

  def testMergeMultipleOneof(self, message_module):
    m_string = '\n'.join(['oneof_uint32: 11', 'oneof_string: "foo"'])
    m2 = message_module.TestAllTypes()
    text_format.Merge(m_string, m2)
    self.assertEqual('oneof_string', m2.WhichOneof('oneof_field'))

  def testParseMultipleOneof(self, message_module):
    m_string = '\n'.join(['oneof_uint32: 11', 'oneof_string: "foo"'])
    m2 = message_module.TestAllTypes()
    with self.assertRaisesRegexp(text_format.ParseError,
                                 ' is specified along with field '):
      text_format.Parse(m_string, m2)


# These are tests that aren't fundamentally specific to proto2, but are at
# the moment because of differences between the proto2 and proto3 test schemas.
# Ideally the schemas would be made more similar so these tests could pass.
class OnlyWorksWithProto2RightNowTests(TextFormatBase):

  def testPrintAllFieldsPointy(self):
    message = unittest_pb2.TestAllTypes()
    test_util.SetAllFields(message)
    self.CompareToGoldenFile(
        self.RemoveRedundantZeros(text_format.MessageToString(
            message, pointy_brackets=True)),
        'text_format_unittest_data_pointy_oneof.txt')

  def testParseGolden(self):
    golden_text = '\n'.join(self.ReadGolden(
        'text_format_unittest_data_oneof_implemented.txt'))
    parsed_message = unittest_pb2.TestAllTypes()
    r = text_format.Parse(golden_text, parsed_message)
    self.assertIs(r, parsed_message)

    message = unittest_pb2.TestAllTypes()
    test_util.SetAllFields(message)
    self.assertEqual(message, parsed_message)

  def testPrintAllFields(self):
    message = unittest_pb2.TestAllTypes()
    test_util.SetAllFields(message)
    self.CompareToGoldenFile(
        self.RemoveRedundantZeros(text_format.MessageToString(message)),
        'text_format_unittest_data_oneof_implemented.txt')

  def testPrintInIndexOrder(self):
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
    self.CompareToGoldenText(
        self.RemoveRedundantZeros(
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
    self.CompareToGoldenText(
        self.RemoveRedundantZeros(text_format.MessageToString(message)),
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

  def testMergeLinesGolden(self):
    opened = self.ReadGolden('text_format_unittest_data_oneof_implemented.txt')
    parsed_message = unittest_pb2.TestAllTypes()
    r = text_format.MergeLines(opened, parsed_message)
    self.assertIs(r, parsed_message)

    message = unittest_pb2.TestAllTypes()
    test_util.SetAllFields(message)
    self.assertEqual(message, parsed_message)

  def testParseLinesGolden(self):
    opened = self.ReadGolden('text_format_unittest_data_oneof_implemented.txt')
    parsed_message = unittest_pb2.TestAllTypes()
    r = text_format.ParseLines(opened, parsed_message)
    self.assertIs(r, parsed_message)

    message = unittest_pb2.TestAllTypes()
    test_util.SetAllFields(message)
    self.assertEqual(message, parsed_message)

  def testPrintMap(self):
    message = map_unittest_pb2.TestMap()

    message.map_int32_int32[-123] = -456
    message.map_int64_int64[-2**33] = -2**34
    message.map_uint32_uint32[123] = 456
    message.map_uint64_uint64[2**33] = 2**34
    message.map_string_string['abc'] = '123'
    message.map_int32_foreign_message[111].c = 5

    # Maps are serialized to text format using their underlying repeated
    # representation.
    self.CompareToGoldenText(
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

  def testMapOrderEnforcement(self):
    message = map_unittest_pb2.TestMap()
    for letter in string.ascii_uppercase[13:26]:
      message.map_string_string[letter] = 'dummy'
    for letter in reversed(string.ascii_uppercase[0:13]):
      message.map_string_string[letter] = 'dummy'
    golden = ''.join(('map_string_string {\n  key: "%c"\n  value: "dummy"\n}\n'
                      % (letter,) for letter in string.ascii_uppercase))
    self.CompareToGoldenText(text_format.MessageToString(message), golden)

  # TODO(teboring): In c/137553523, not serializing default value for map entry
  # message has been fixed. This test needs to be disabled in order to submit
  # that cl. Add this back when c/137553523 has been submitted.
  # def testMapOrderSemantics(self):
  #   golden_lines = self.ReadGolden('map_test_data.txt')

  #   message = map_unittest_pb2.TestMap()
  #   text_format.ParseLines(golden_lines, message)
  #   candidate = text_format.MessageToString(message)
  #   # The Python implementation emits "1.0" for the double value that the C++
  #   # implementation emits as "1".
  #   candidate = candidate.replace('1.0', '1', 2)
  #   candidate = candidate.replace('0.0', '0', 2)
  #   self.assertMultiLineEqual(candidate, ''.join(golden_lines))


# Tests of proto2-only features (MessageSet, extensions, etc.).
class Proto2Tests(TextFormatBase):

  def testPrintMessageSet(self):
    message = unittest_mset_pb2.TestMessageSetContainer()
    ext1 = unittest_mset_pb2.TestMessageSetExtension1.message_set_extension
    ext2 = unittest_mset_pb2.TestMessageSetExtension2.message_set_extension
    message.message_set.Extensions[ext1].i = 23
    message.message_set.Extensions[ext2].str = 'foo'
    self.CompareToGoldenText(
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
    self.CompareToGoldenText(
        text_format.MessageToString(message),
        '[google.protobuf.internal.TestMessageSetExtension3] {\n'
        '  text: \"bar\"\n'
        '}\n')

  def testPrintMessageSetByFieldNumber(self):
    out = text_format.TextWriter(False)
    message = unittest_mset_pb2.TestMessageSetContainer()
    ext1 = unittest_mset_pb2.TestMessageSetExtension1.message_set_extension
    ext2 = unittest_mset_pb2.TestMessageSetExtension2.message_set_extension
    message.message_set.Extensions[ext1].i = 23
    message.message_set.Extensions[ext2].str = 'foo'
    text_format.PrintMessage(message, out, use_field_number=True)
    self.CompareToGoldenText(out.getvalue(), '1 {\n'
                             '  1545008 {\n'
                             '    15: 23\n'
                             '  }\n'
                             '  1547769 {\n'
                             '    25: \"foo\"\n'
                             '  }\n'
                             '}\n')
    out.close()

  def testPrintMessageSetAsOneLine(self):
    message = unittest_mset_pb2.TestMessageSetContainer()
    ext1 = unittest_mset_pb2.TestMessageSetExtension1.message_set_extension
    ext2 = unittest_mset_pb2.TestMessageSetExtension2.message_set_extension
    message.message_set.Extensions[ext1].i = 23
    message.message_set.Extensions[ext2].str = 'foo'
    self.CompareToGoldenText(
        text_format.MessageToString(message, as_one_line=True),
        'message_set {'
        ' [protobuf_unittest.TestMessageSetExtension1] {'
        ' i: 23'
        ' }'
        ' [protobuf_unittest.TestMessageSetExtension2] {'
        ' str: \"foo\"'
        ' }'
        ' }')

  def testParseMessageSet(self):
    message = unittest_pb2.TestAllTypes()
    text = ('repeated_uint64: 1\n' 'repeated_uint64: 2\n')
    text_format.Parse(text, message)
    self.assertEqual(1, message.repeated_uint64[0])
    self.assertEqual(2, message.repeated_uint64[1])

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
    self.assertEqual(23, message.message_set.Extensions[ext1].i)
    self.assertEqual('foo', message.message_set.Extensions[ext2].str)

  def testExtensionInsideAnyMessage(self):
    message = test_extend_any.TestAny()
    text = ('value {\n'
            '  [type.googleapis.com/google.protobuf.internal.TestAny] {\n'
            '    [google.protobuf.internal.TestAnyExtension1.extension1] {\n'
            '      i: 10\n'
            '    }\n'
            '  }\n'
            '}\n')
    text_format.Merge(text, message, descriptor_pool=descriptor_pool.Default())
    self.CompareToGoldenText(
        text_format.MessageToString(
            message, descriptor_pool=descriptor_pool.Default()),
        text)

  def testParseMessageByFieldNumber(self):
    message = unittest_pb2.TestAllTypes()
    text = ('34: 1\n' 'repeated_uint64: 2\n')
    text_format.Parse(text, message, allow_field_number=True)
    self.assertEqual(1, message.repeated_uint64[0])
    self.assertEqual(2, message.repeated_uint64[1])

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
    self.assertEqual(23, message.message_set.Extensions[ext1].i)
    self.assertEqual('foo', message.message_set.Extensions[ext2].str)

    # Can't parse field number without set allow_field_number=True.
    message = unittest_pb2.TestAllTypes()
    text = '34:1\n'
    six.assertRaisesRegex(self, text_format.ParseError, (
        r'1:1 : Message type "\w+.TestAllTypes" has no field named '
        r'"34".'), text_format.Parse, text, message)

    # Can't parse if field number is not found.
    text = '1234:1\n'
    six.assertRaisesRegex(
        self,
        text_format.ParseError,
        (r'1:1 : Message type "\w+.TestAllTypes" has no field named '
         r'"1234".'),
        text_format.Parse,
        text,
        message,
        allow_field_number=True)

  def testPrintAllExtensions(self):
    message = unittest_pb2.TestAllExtensions()
    test_util.SetAllExtensions(message)
    self.CompareToGoldenFile(
        self.RemoveRedundantZeros(text_format.MessageToString(message)),
        'text_format_unittest_extensions_data.txt')

  def testPrintAllExtensionsPointy(self):
    message = unittest_pb2.TestAllExtensions()
    test_util.SetAllExtensions(message)
    self.CompareToGoldenFile(
        self.RemoveRedundantZeros(text_format.MessageToString(
            message, pointy_brackets=True)),
        'text_format_unittest_extensions_data_pointy.txt')

  def testParseGoldenExtensions(self):
    golden_text = '\n'.join(self.ReadGolden(
        'text_format_unittest_extensions_data.txt'))
    parsed_message = unittest_pb2.TestAllExtensions()
    text_format.Parse(golden_text, parsed_message)

    message = unittest_pb2.TestAllExtensions()
    test_util.SetAllExtensions(message)
    self.assertEqual(message, parsed_message)

  def testParseAllExtensions(self):
    message = unittest_pb2.TestAllExtensions()
    test_util.SetAllExtensions(message)
    ascii_text = text_format.MessageToString(message)

    parsed_message = unittest_pb2.TestAllExtensions()
    text_format.Parse(ascii_text, parsed_message)
    self.assertEqual(message, parsed_message)

  def testParseAllowedUnknownExtension(self):
    # Skip over unknown extension correctly.
    message = unittest_mset_pb2.TestMessageSetContainer()
    text = ('message_set {\n'
            '  [unknown_extension] {\n'
            '    i: 23\n'
            '    bin: "\xe0"'
            '    [nested_unknown_ext]: {\n'
            '      i: 23\n'
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
    self.CompareToGoldenText(text_format.MessageToString(message), golden)

    # Catch parse errors in unknown extension.
    message = unittest_mset_pb2.TestMessageSetContainer()
    malformed = ('message_set {\n'
                 '  [unknown_extension] {\n'
                 '    i:\n'  # Missing value.
                 '  }\n'
                 '}\n')
    six.assertRaisesRegex(self,
                          text_format.ParseError,
                          'Invalid field value: }',
                          text_format.Parse,
                          malformed,
                          message,
                          allow_unknown_extension=True)

    message = unittest_mset_pb2.TestMessageSetContainer()
    malformed = ('message_set {\n'
                 '  [unknown_extension] {\n'
                 '    str: "malformed string\n'  # Missing closing quote.
                 '  }\n'
                 '}\n')
    six.assertRaisesRegex(self,
                          text_format.ParseError,
                          'Invalid field value: "',
                          text_format.Parse,
                          malformed,
                          message,
                          allow_unknown_extension=True)

    message = unittest_mset_pb2.TestMessageSetContainer()
    malformed = ('message_set {\n'
                 '  [unknown_extension] {\n'
                 '    str: "malformed\n multiline\n string\n'
                 '  }\n'
                 '}\n')
    six.assertRaisesRegex(self,
                          text_format.ParseError,
                          'Invalid field value: "',
                          text_format.Parse,
                          malformed,
                          message,
                          allow_unknown_extension=True)

    message = unittest_mset_pb2.TestMessageSetContainer()
    malformed = ('message_set {\n'
                 '  [malformed_extension] <\n'
                 '    i: -5\n'
                 '  \n'  # Missing '>' here.
                 '}\n')
    six.assertRaisesRegex(self,
                          text_format.ParseError,
                          '5:1 : Expected ">".',
                          text_format.Parse,
                          malformed,
                          message,
                          allow_unknown_extension=True)

    # Don't allow unknown fields with allow_unknown_extension=True.
    message = unittest_mset_pb2.TestMessageSetContainer()
    malformed = ('message_set {\n'
                 '  unknown_field: true\n'
                 '}\n')
    six.assertRaisesRegex(self,
                          text_format.ParseError,
                          ('2:3 : Message type '
                           '"proto2_wireformat_unittest.TestMessageSet" has no'
                           ' field named "unknown_field".'),
                          text_format.Parse,
                          malformed,
                          message,
                          allow_unknown_extension=True)

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
    self.assertEqual(23, message.message_set.Extensions[ext1].i)
    self.assertEqual('foo', message.message_set.Extensions[ext2].str)

  def testParseBadIdentifier(self):
    message = unittest_pb2.TestAllTypes()
    text = ('optional_nested_message { "bb": 1 }')
    with self.assertRaises(text_format.ParseError) as e:
      text_format.Parse(text, message)
    self.assertEqual(str(e.exception),
                     '1:27 : Expected identifier or number, got "bb".')

  def testParseBadExtension(self):
    message = unittest_pb2.TestAllExtensions()
    text = '[unknown_extension]: 8\n'
    six.assertRaisesRegex(self, text_format.ParseError,
                          '1:2 : Extension "unknown_extension" not registered.',
                          text_format.Parse, text, message)
    message = unittest_pb2.TestAllTypes()
    six.assertRaisesRegex(self, text_format.ParseError, (
        '1:2 : Message type "protobuf_unittest.TestAllTypes" does not have '
        'extensions.'), text_format.Parse, text, message)

  def testParseNumericUnknownEnum(self):
    message = unittest_pb2.TestAllTypes()
    text = 'optional_nested_enum: 100'
    six.assertRaisesRegex(self, text_format.ParseError,
                          (r'1:23 : Enum type "\w+.TestAllTypes.NestedEnum" '
                           r'has no value with number 100.'), text_format.Parse,
                          text, message)

  def testMergeDuplicateExtensionScalars(self):
    message = unittest_pb2.TestAllExtensions()
    text = ('[protobuf_unittest.optional_int32_extension]: 42 '
            '[protobuf_unittest.optional_int32_extension]: 67')
    text_format.Merge(text, message)
    self.assertEqual(67,
                     message.Extensions[unittest_pb2.optional_int32_extension])

  def testParseDuplicateExtensionScalars(self):
    message = unittest_pb2.TestAllExtensions()
    text = ('[protobuf_unittest.optional_int32_extension]: 42 '
            '[protobuf_unittest.optional_int32_extension]: 67')
    six.assertRaisesRegex(self, text_format.ParseError, (
        '1:96 : Message type "protobuf_unittest.TestAllExtensions" '
        'should not have multiple '
        '"protobuf_unittest.optional_int32_extension" extensions.'),
                          text_format.Parse, text, message)

  def testParseDuplicateMessages(self):
    message = unittest_pb2.TestAllTypes()
    text = ('optional_nested_message { bb: 1 } '
            'optional_nested_message { bb: 2 }')
    six.assertRaisesRegex(self, text_format.ParseError, (
        '1:59 : Message type "protobuf_unittest.TestAllTypes" '
        'should not have multiple "optional_nested_message" fields.'),
                          text_format.Parse, text,
                          message)

  def testParseDuplicateExtensionMessages(self):
    message = unittest_pb2.TestAllExtensions()
    text = ('[protobuf_unittest.optional_nested_message_extension]: {} '
            '[protobuf_unittest.optional_nested_message_extension]: {}')
    six.assertRaisesRegex(self, text_format.ParseError, (
        '1:114 : Message type "protobuf_unittest.TestAllExtensions" '
        'should not have multiple '
        '"protobuf_unittest.optional_nested_message_extension" extensions.'),
                          text_format.Parse, text, message)

  def testParseDuplicateScalars(self):
    message = unittest_pb2.TestAllTypes()
    text = ('optional_int32: 42 ' 'optional_int32: 67')
    six.assertRaisesRegex(self, text_format.ParseError, (
        '1:36 : Message type "protobuf_unittest.TestAllTypes" should not '
        'have multiple "optional_int32" fields.'), text_format.Parse, text,
                          message)

  def testParseGroupNotClosed(self):
    message = unittest_pb2.TestAllTypes()
    text = 'RepeatedGroup: <'
    six.assertRaisesRegex(self, text_format.ParseError, '1:16 : Expected ">".',
                          text_format.Parse, text, message)
    text = 'RepeatedGroup: {'
    six.assertRaisesRegex(self, text_format.ParseError, '1:16 : Expected "}".',
                          text_format.Parse, text, message)

  def testParseEmptyGroup(self):
    message = unittest_pb2.TestAllTypes()
    text = 'OptionalGroup: {}'
    text_format.Parse(text, message)
    self.assertTrue(message.HasField('optionalgroup'))

    message.Clear()

    message = unittest_pb2.TestAllTypes()
    text = 'OptionalGroup: <>'
    text_format.Parse(text, message)
    self.assertTrue(message.HasField('optionalgroup'))

  # Maps aren't really proto2-only, but our test schema only has maps for
  # proto2.
  def testParseMap(self):
    text = ('map_int32_int32 {\n'
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
    message = map_unittest_pb2.TestMap()
    text_format.Parse(text, message)

    self.assertEqual(-456, message.map_int32_int32[-123])
    self.assertEqual(-2**34, message.map_int64_int64[-2**33])
    self.assertEqual(456, message.map_uint32_uint32[123])
    self.assertEqual(2**34, message.map_uint64_uint64[2**33])
    self.assertEqual('123', message.map_string_string['abc'])
    self.assertEqual(5, message.map_int32_foreign_message[111].c)


class Proto3Tests(unittest.TestCase):

  def testPrintMessageExpandAny(self):
    packed_message = unittest_pb2.OneString()
    packed_message.data = 'string'
    message = any_test_pb2.TestAny()
    message.any_value.Pack(packed_message)
    self.assertEqual(
        text_format.MessageToString(message,
                                    descriptor_pool=descriptor_pool.Default()),
        'any_value {\n'
        '  [type.googleapis.com/protobuf_unittest.OneString] {\n'
        '    data: "string"\n'
        '  }\n'
        '}\n')

  def testTopAnyMessage(self):
    packed_msg = unittest_pb2.OneString()
    msg = any_pb2.Any()
    msg.Pack(packed_msg)
    text = text_format.MessageToString(msg)
    other_msg = text_format.Parse(text, any_pb2.Any())
    self.assertEqual(msg, other_msg)

  def testPrintMessageExpandAnyRepeated(self):
    packed_message = unittest_pb2.OneString()
    message = any_test_pb2.TestAny()
    packed_message.data = 'string0'
    message.repeated_any_value.add().Pack(packed_message)
    packed_message.data = 'string1'
    message.repeated_any_value.add().Pack(packed_message)
    self.assertEqual(
        text_format.MessageToString(message),
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

  def testPrintMessageExpandAnyDescriptorPoolMissingType(self):
    packed_message = unittest_pb2.OneString()
    packed_message.data = 'string'
    message = any_test_pb2.TestAny()
    message.any_value.Pack(packed_message)
    empty_pool = descriptor_pool.DescriptorPool()
    self.assertEqual(
        text_format.MessageToString(message, descriptor_pool=empty_pool),
        'any_value {\n'
        '  type_url: "type.googleapis.com/protobuf_unittest.OneString"\n'
        '  value: "\\n\\006string"\n'
        '}\n')

  def testPrintMessageExpandAnyPointyBrackets(self):
    packed_message = unittest_pb2.OneString()
    packed_message.data = 'string'
    message = any_test_pb2.TestAny()
    message.any_value.Pack(packed_message)
    self.assertEqual(
        text_format.MessageToString(message,
                                    pointy_brackets=True),
        'any_value <\n'
        '  [type.googleapis.com/protobuf_unittest.OneString] <\n'
        '    data: "string"\n'
        '  >\n'
        '>\n')

  def testPrintMessageExpandAnyAsOneLine(self):
    packed_message = unittest_pb2.OneString()
    packed_message.data = 'string'
    message = any_test_pb2.TestAny()
    message.any_value.Pack(packed_message)
    self.assertEqual(
        text_format.MessageToString(message,
                                    as_one_line=True),
        'any_value {'
        ' [type.googleapis.com/protobuf_unittest.OneString]'
        ' { data: "string" } '
        '}')

  def testPrintMessageExpandAnyAsOneLinePointyBrackets(self):
    packed_message = unittest_pb2.OneString()
    packed_message.data = 'string'
    message = any_test_pb2.TestAny()
    message.any_value.Pack(packed_message)
    self.assertEqual(
        text_format.MessageToString(message,
                                    as_one_line=True,
                                    pointy_brackets=True,
                                    descriptor_pool=descriptor_pool.Default()),
        'any_value <'
        ' [type.googleapis.com/protobuf_unittest.OneString]'
        ' < data: "string" > '
        '>')

  def testUnknownEnums(self):
    message = unittest_proto3_arena_pb2.TestAllTypes()
    message2 = unittest_proto3_arena_pb2.TestAllTypes()
    message.optional_nested_enum = 999
    text_string = text_format.MessageToString(message)
    text_format.Parse(text_string, message2)
    self.assertEqual(999, message2.optional_nested_enum)

  def testMergeExpandedAny(self):
    message = any_test_pb2.TestAny()
    text = ('any_value {\n'
            '  [type.googleapis.com/protobuf_unittest.OneString] {\n'
            '    data: "string"\n'
            '  }\n'
            '}\n')
    text_format.Merge(text, message)
    packed_message = unittest_pb2.OneString()
    message.any_value.Unpack(packed_message)
    self.assertEqual('string', packed_message.data)
    message.Clear()
    text_format.Parse(text, message)
    packed_message = unittest_pb2.OneString()
    message.any_value.Unpack(packed_message)
    self.assertEqual('string', packed_message.data)

  def testMergeExpandedAnyRepeated(self):
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
    self.assertEqual('string0', packed_message.data)
    message.repeated_any_value[1].Unpack(packed_message)
    self.assertEqual('string1', packed_message.data)

  def testMergeExpandedAnyPointyBrackets(self):
    message = any_test_pb2.TestAny()
    text = ('any_value {\n'
            '  [type.googleapis.com/protobuf_unittest.OneString] <\n'
            '    data: "string"\n'
            '  >\n'
            '}\n')
    text_format.Merge(text, message)
    packed_message = unittest_pb2.OneString()
    message.any_value.Unpack(packed_message)
    self.assertEqual('string', packed_message.data)

  def testMergeAlternativeUrl(self):
    message = any_test_pb2.TestAny()
    text = ('any_value {\n'
            '  [type.otherapi.com/protobuf_unittest.OneString] {\n'
            '    data: "string"\n'
            '  }\n'
            '}\n')
    text_format.Merge(text, message)
    packed_message = unittest_pb2.OneString()
    self.assertEqual('type.otherapi.com/protobuf_unittest.OneString',
                     message.any_value.type_url)

  def testMergeExpandedAnyDescriptorPoolMissingType(self):
    message = any_test_pb2.TestAny()
    text = ('any_value {\n'
            '  [type.googleapis.com/protobuf_unittest.OneString] {\n'
            '    data: "string"\n'
            '  }\n'
            '}\n')
    with self.assertRaises(text_format.ParseError) as e:
      empty_pool = descriptor_pool.DescriptorPool()
      text_format.Merge(text, message, descriptor_pool=empty_pool)
    self.assertEqual(
        str(e.exception),
        'Type protobuf_unittest.OneString not found in descriptor pool')

  def testMergeUnexpandedAny(self):
    text = ('any_value {\n'
            '  type_url: "type.googleapis.com/protobuf_unittest.OneString"\n'
            '  value: "\\n\\006string"\n'
            '}\n')
    message = any_test_pb2.TestAny()
    text_format.Merge(text, message)
    packed_message = unittest_pb2.OneString()
    message.any_value.Unpack(packed_message)
    self.assertEqual('string', packed_message.data)

  def testMergeMissingAnyEndToken(self):
    message = any_test_pb2.TestAny()
    text = ('any_value {\n'
            '  [type.googleapis.com/protobuf_unittest.OneString] {\n'
            '    data: "string"\n')
    with self.assertRaises(text_format.ParseError) as e:
      text_format.Merge(text, message)
    self.assertEqual(str(e.exception), '3:11 : Expected "}".')


class TokenizerTest(unittest.TestCase):

  def testSimpleTokenCases(self):
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
        self.assertEqual(token, m)
        tokenizer.NextToken()
      elif isinstance(m[1], float) and math.isnan(m[1]):
        self.assertTrue(math.isnan(m[0]()))
      else:
        self.assertEqual(m[1], m[0]())
      i += 1

  def testConsumeAbstractIntegers(self):
    # This test only tests the failures in the integer parsing methods as well
    # as the '0' special cases.
    int64_max = (1 << 63) - 1
    uint32_max = (1 << 32) - 1
    text = '-1 %d %d' % (uint32_max + 1, int64_max + 1)
    tokenizer = text_format.Tokenizer(text.splitlines())
    self.assertEqual(-1, tokenizer.ConsumeInteger())

    self.assertEqual(uint32_max + 1, tokenizer.ConsumeInteger())

    self.assertEqual(int64_max + 1, tokenizer.ConsumeInteger())
    self.assertTrue(tokenizer.AtEnd())

    text = '-0 0 0 1.2'
    tokenizer = text_format.Tokenizer(text.splitlines())
    self.assertEqual(0, tokenizer.ConsumeInteger())
    self.assertEqual(0, tokenizer.ConsumeInteger())
    self.assertEqual(True, tokenizer.TryConsumeInteger())
    self.assertEqual(False, tokenizer.TryConsumeInteger())
    with self.assertRaises(text_format.ParseError):
      tokenizer.ConsumeInteger()
    self.assertEqual(1.2, tokenizer.ConsumeFloat())
    self.assertTrue(tokenizer.AtEnd())

  def testConsumeIntegers(self):
    # This test only tests the failures in the integer parsing methods as well
    # as the '0' special cases.
    int64_max = (1 << 63) - 1
    uint32_max = (1 << 32) - 1
    text = '-1 %d %d' % (uint32_max + 1, int64_max + 1)
    tokenizer = text_format.Tokenizer(text.splitlines())
    self.assertRaises(text_format.ParseError,
                      text_format._ConsumeUint32, tokenizer)
    self.assertRaises(text_format.ParseError,
                      text_format._ConsumeUint64, tokenizer)
    self.assertEqual(-1, text_format._ConsumeInt32(tokenizer))

    self.assertRaises(text_format.ParseError,
                      text_format._ConsumeUint32, tokenizer)
    self.assertRaises(text_format.ParseError,
                      text_format._ConsumeInt32, tokenizer)
    self.assertEqual(uint32_max + 1, text_format._ConsumeInt64(tokenizer))

    self.assertRaises(text_format.ParseError,
                      text_format._ConsumeInt64, tokenizer)
    self.assertEqual(int64_max + 1, text_format._ConsumeUint64(tokenizer))
    self.assertTrue(tokenizer.AtEnd())

    text = '-0 -0 0 0'
    tokenizer = text_format.Tokenizer(text.splitlines())
    self.assertEqual(0, text_format._ConsumeUint32(tokenizer))
    self.assertEqual(0, text_format._ConsumeUint64(tokenizer))
    self.assertEqual(0, text_format._ConsumeUint32(tokenizer))
    self.assertEqual(0, text_format._ConsumeUint64(tokenizer))
    self.assertTrue(tokenizer.AtEnd())

  def testConsumeByteString(self):
    text = '"string1\''
    tokenizer = text_format.Tokenizer(text.splitlines())
    self.assertRaises(text_format.ParseError, tokenizer.ConsumeByteString)

    text = 'string1"'
    tokenizer = text_format.Tokenizer(text.splitlines())
    self.assertRaises(text_format.ParseError, tokenizer.ConsumeByteString)

    text = '\n"\\xt"'
    tokenizer = text_format.Tokenizer(text.splitlines())
    self.assertRaises(text_format.ParseError, tokenizer.ConsumeByteString)

    text = '\n"\\"'
    tokenizer = text_format.Tokenizer(text.splitlines())
    self.assertRaises(text_format.ParseError, tokenizer.ConsumeByteString)

    text = '\n"\\x"'
    tokenizer = text_format.Tokenizer(text.splitlines())
    self.assertRaises(text_format.ParseError, tokenizer.ConsumeByteString)

  def testConsumeBool(self):
    text = 'not-a-bool'
    tokenizer = text_format.Tokenizer(text.splitlines())
    self.assertRaises(text_format.ParseError, tokenizer.ConsumeBool)

  def testSkipComment(self):
    tokenizer = text_format.Tokenizer('# some comment'.splitlines())
    self.assertTrue(tokenizer.AtEnd())
    self.assertRaises(text_format.ParseError, tokenizer.ConsumeComment)

  def testConsumeComment(self):
    tokenizer = text_format.Tokenizer('# some comment'.splitlines(),
                                      skip_comments=False)
    self.assertFalse(tokenizer.AtEnd())
    self.assertEqual('# some comment', tokenizer.ConsumeComment())
    self.assertTrue(tokenizer.AtEnd())

  def testConsumeTwoComments(self):
    text = '# some comment\n# another comment'
    tokenizer = text_format.Tokenizer(text.splitlines(), skip_comments=False)
    self.assertEqual('# some comment', tokenizer.ConsumeComment())
    self.assertFalse(tokenizer.AtEnd())
    self.assertEqual('# another comment', tokenizer.ConsumeComment())
    self.assertTrue(tokenizer.AtEnd())

  def testConsumeTrailingComment(self):
    text = 'some_number: 4\n# some comment'
    tokenizer = text_format.Tokenizer(text.splitlines(), skip_comments=False)
    self.assertRaises(text_format.ParseError, tokenizer.ConsumeComment)

    self.assertEqual('some_number', tokenizer.ConsumeIdentifier())
    self.assertEqual(tokenizer.token, ':')
    tokenizer.NextToken()
    self.assertRaises(text_format.ParseError, tokenizer.ConsumeComment)
    self.assertEqual(4, tokenizer.ConsumeInteger())
    self.assertFalse(tokenizer.AtEnd())

    self.assertEqual('# some comment', tokenizer.ConsumeComment())
    self.assertTrue(tokenizer.AtEnd())

  def testConsumeLineComment(self):
    tokenizer = text_format.Tokenizer('# some comment'.splitlines(),
                                      skip_comments=False)
    self.assertFalse(tokenizer.AtEnd())
    self.assertEqual((False, '# some comment'),
                     tokenizer.ConsumeCommentOrTrailingComment())
    self.assertTrue(tokenizer.AtEnd())

  def testConsumeTwoLineComments(self):
    text = '# some comment\n# another comment'
    tokenizer = text_format.Tokenizer(text.splitlines(), skip_comments=False)
    self.assertEqual((False, '# some comment'),
                     tokenizer.ConsumeCommentOrTrailingComment())
    self.assertFalse(tokenizer.AtEnd())
    self.assertEqual((False, '# another comment'),
                     tokenizer.ConsumeCommentOrTrailingComment())
    self.assertTrue(tokenizer.AtEnd())

  def testConsumeAndCheckTrailingComment(self):
    text = 'some_number: 4  # some comment'  # trailing comment on the same line
    tokenizer = text_format.Tokenizer(text.splitlines(), skip_comments=False)
    self.assertRaises(text_format.ParseError,
                      tokenizer.ConsumeCommentOrTrailingComment)

    self.assertEqual('some_number', tokenizer.ConsumeIdentifier())
    self.assertEqual(tokenizer.token, ':')
    tokenizer.NextToken()
    self.assertRaises(text_format.ParseError,
                      tokenizer.ConsumeCommentOrTrailingComment)
    self.assertEqual(4, tokenizer.ConsumeInteger())
    self.assertFalse(tokenizer.AtEnd())

    self.assertEqual((True, '# some comment'),
                     tokenizer.ConsumeCommentOrTrailingComment())
    self.assertTrue(tokenizer.AtEnd())

  def testHashinComment(self):
    text = 'some_number: 4  # some comment # not a new comment'
    tokenizer = text_format.Tokenizer(text.splitlines(), skip_comments=False)
    self.assertEqual('some_number', tokenizer.ConsumeIdentifier())
    self.assertEqual(tokenizer.token, ':')
    tokenizer.NextToken()
    self.assertEqual(4, tokenizer.ConsumeInteger())
    self.assertEqual((True, '# some comment # not a new comment'),
                     tokenizer.ConsumeCommentOrTrailingComment())
    self.assertTrue(tokenizer.AtEnd())


# Tests for pretty printer functionality.
@_parameterized.parameters((unittest_pb2), (unittest_proto3_arena_pb2))
class PrettyPrinterTest(TextFormatBase):

  def testPrettyPrintNoMatch(self, message_module):

    def printer(message, indent, as_one_line):
      del message, indent, as_one_line
      return None

    message = message_module.TestAllTypes()
    msg = message.repeated_nested_message.add()
    msg.bb = 42
    self.CompareToGoldenText(
        text_format.MessageToString(
            message, as_one_line=True, message_formatter=printer),
        'repeated_nested_message { bb: 42 }')

  def testPrettyPrintOneLine(self, message_module):

    def printer(m, indent, as_one_line):
      del indent, as_one_line
      if m.DESCRIPTOR == message_module.TestAllTypes.NestedMessage.DESCRIPTOR:
        return 'My lucky number is %s' % m.bb

    message = message_module.TestAllTypes()
    msg = message.repeated_nested_message.add()
    msg.bb = 42
    self.CompareToGoldenText(
        text_format.MessageToString(
            message, as_one_line=True, message_formatter=printer),
        'repeated_nested_message { My lucky number is 42 }')

  def testPrettyPrintMultiLine(self, message_module):

    def printer(m, indent, as_one_line):
      if m.DESCRIPTOR == message_module.TestAllTypes.NestedMessage.DESCRIPTOR:
        line_deliminator = (' ' if as_one_line else '\n') + ' ' * indent
        return 'My lucky number is:%s%s' % (line_deliminator, m.bb)
      return None

    message = message_module.TestAllTypes()
    msg = message.repeated_nested_message.add()
    msg.bb = 42
    self.CompareToGoldenText(
        text_format.MessageToString(
            message, as_one_line=True, message_formatter=printer),
        'repeated_nested_message { My lucky number is: 42 }')
    self.CompareToGoldenText(
        text_format.MessageToString(
            message, as_one_line=False, message_formatter=printer),
        'repeated_nested_message {\n  My lucky number is:\n  42\n}\n')

  def testPrettyPrintEntireMessage(self, message_module):

    def printer(m, indent, as_one_line):
      del indent, as_one_line
      if m.DESCRIPTOR == message_module.TestAllTypes.DESCRIPTOR:
        return 'The is the message!'
      return None

    message = message_module.TestAllTypes()
    self.CompareToGoldenText(
        text_format.MessageToString(
            message, as_one_line=False, message_formatter=printer),
        'The is the message!\n')
    self.CompareToGoldenText(
        text_format.MessageToString(
            message, as_one_line=True, message_formatter=printer),
        'The is the message!')

  def testPrettyPrintMultipleParts(self, message_module):

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
    self.CompareToGoldenText(
        text_format.MessageToString(
            message, as_one_line=True, message_formatter=printer),
        ('optional_int32: 61 '
         'optional_nested_message { My lucky number is 1 } '
         'repeated_nested_message { My lucky number is 42 } '
         'repeated_nested_message { My lucky number is 99 }'))

if __name__ == '__main__':
  unittest.main()
