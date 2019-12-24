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

"""Tests for google.protobuf.text_encoding."""

try:
  import unittest2 as unittest  #PY26
except ImportError:
  import unittest

from google.protobuf import text_encoding

TEST_VALUES = [
    ("foo\\rbar\\nbaz\\t",
     "foo\\rbar\\nbaz\\t",
     b"foo\rbar\nbaz\t"),
    ("\\'full of \\\"sound\\\" and \\\"fury\\\"\\'",
     "\\'full of \\\"sound\\\" and \\\"fury\\\"\\'",
     b"'full of \"sound\" and \"fury\"'"),
    ("signi\\\\fying\\\\ nothing\\\\",
     "signi\\\\fying\\\\ nothing\\\\",
     b"signi\\fying\\ nothing\\"),
    ("\\010\\t\\n\\013\\014\\r",
     "\x08\\t\\n\x0b\x0c\\r",
     b"\010\011\012\013\014\015")]


class TextEncodingTestCase(unittest.TestCase):
  def testCEscape(self):
    for escaped, escaped_utf8, unescaped in TEST_VALUES:
      self.assertEqual(escaped,
                        text_encoding.CEscape(unescaped, as_utf8=False))
      self.assertEqual(escaped_utf8,
                        text_encoding.CEscape(unescaped, as_utf8=True))

  def testCUnescape(self):
    for escaped, escaped_utf8, unescaped in TEST_VALUES:
      self.assertEqual(unescaped, text_encoding.CUnescape(escaped))
      self.assertEqual(unescaped, text_encoding.CUnescape(escaped_utf8))


if __name__ == "__main__":
  unittest.main()
