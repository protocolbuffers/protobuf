# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Tests for google.protobuf.text_encoding."""

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
