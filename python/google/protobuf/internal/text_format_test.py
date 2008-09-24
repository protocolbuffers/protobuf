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

"""Test for google.protobuf.text_format."""

__author__ = 'kenton@google.com (Kenton Varda)'

import difflib

import unittest
from google.protobuf import text_format
from google.protobuf.internal import test_util
from google.protobuf import unittest_pb2
from google.protobuf import unittest_mset_pb2

class TextFormatTest(unittest.TestCase):
  def CompareToGoldenFile(self, text, golden_filename):
    f = test_util.GoldenFile(golden_filename)
    golden_lines = f.readlines()
    f.close()
    self.CompareToGoldenLines(text, golden_lines)

  def CompareToGoldenText(self, text, golden_text):
    self.CompareToGoldenLines(text, golden_text.splitlines(1))

  def CompareToGoldenLines(self, text, golden_lines):
    actual_lines = text.splitlines(1)
    self.assertEqual(golden_lines, actual_lines,
      "Text doesn't match golden.  Diff:\n" +
      ''.join(difflib.ndiff(golden_lines, actual_lines)))

  def testPrintAllFields(self):
    message = unittest_pb2.TestAllTypes()
    test_util.SetAllFields(message)
    self.CompareToGoldenFile(
      self.RemoveRedundantZeros(text_format.MessageToString(message)),
      'text_format_unittest_data.txt')

  def testPrintAllExtensions(self):
    message = unittest_pb2.TestAllExtensions()
    test_util.SetAllExtensions(message)
    self.CompareToGoldenFile(
      self.RemoveRedundantZeros(text_format.MessageToString(message)),
      'text_format_unittest_extensions_data.txt')

  def testPrintMessageSet(self):
    message = unittest_mset_pb2.TestMessageSetContainer()
    ext1 = unittest_mset_pb2.TestMessageSetExtension1.message_set_extension
    ext2 = unittest_mset_pb2.TestMessageSetExtension2.message_set_extension
    message.message_set.Extensions[ext1].i = 23
    message.message_set.Extensions[ext2].str = 'foo'
    self.CompareToGoldenText(text_format.MessageToString(message),
      'message_set {\n'
      '  [protobuf_unittest.TestMessageSetExtension1] {\n'
      '    i: 23\n'
      '  }\n'
      '  [protobuf_unittest.TestMessageSetExtension2] {\n'
      '    str: \"foo\"\n'
      '  }\n'
      '}\n')

  def testPrintExotic(self):
    message = unittest_pb2.TestAllTypes()
    message.repeated_int64.append(-9223372036854775808);
    message.repeated_uint64.append(18446744073709551615);
    message.repeated_double.append(123.456);
    message.repeated_double.append(1.23e22);
    message.repeated_double.append(1.23e-18);
    message.repeated_string.append('\000\001\a\b\f\n\r\t\v\\\'\"');
    self.CompareToGoldenText(
      self.RemoveRedundantZeros(text_format.MessageToString(message)),
      'repeated_int64: -9223372036854775808\n'
      'repeated_uint64: 18446744073709551615\n'
      'repeated_double: 123.456\n'
      'repeated_double: 1.23e+22\n'
      'repeated_double: 1.23e-18\n'
      'repeated_string: '
        '\"\\000\\001\\007\\010\\014\\n\\r\\t\\013\\\\\\\'\\\"\"\n')

  def testMessageToString(self):
    message = unittest_pb2.ForeignMessage()
    message.c = 123
    self.assertEqual('c: 123\n', str(message))

  def RemoveRedundantZeros(self, text):
    # Some platforms print 1e+5 as 1e+005.  This is fine, but we need to remove
    # these zeros in order to match the golden file.
    return text.replace('e+0','e+').replace('e+0','e+') \
               .replace('e-0','e-').replace('e-0','e-')

if __name__ == '__main__':
  unittest.main()

