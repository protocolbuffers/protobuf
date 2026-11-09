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
    ("foo\\rbar\\nbaz\\t", "foo\\rbar\\nbaz\\t", b"foo\rbar\nbaz\t"),
    (
        '\\\'full of \\"sound\\" and \\"fury\\"\\\'',
        '\\\'full of \\"sound\\" and \\"fury\\"\\\'',
        b'\'full of "sound" and "fury"\'',
    ),
    (
        "signi\\\\fying\\\\ nothing\\\\",
        "signi\\\\fying\\\\ nothing\\\\",
        b"signi\\fying\\ nothing\\",
    ),
    (
        "\\010\\t\\n\\013\\014\\r",
        "\\010\\t\\n\\013\\014\\r",
        b"\010\011\012\013\014\015",
    ),
]


class TextEncodingTestCase(unittest.TestCase):

  def testCEscape(self):
    for escaped, escaped_utf8, unescaped in TEST_VALUES:
      self.assertEqual(escaped, text_encoding.CEscape(unescaped, as_utf8=False))
      self.assertEqual(
          escaped_utf8, text_encoding.CEscape(unescaped, as_utf8=True)
      )

  def testCUnescape(self):
    for escaped, escaped_utf8, unescaped in TEST_VALUES:
      self.assertEqual(unescaped, text_encoding.CUnescape(escaped))
      self.assertEqual(unescaped, text_encoding.CUnescape(escaped_utf8))

  def testCUnescapeTrigraph(self):
    self.assertEqual(b"?", text_encoding.CUnescape("?"))
    self.assertEqual(b"?", text_encoding.CUnescape("\\?"))
    self.assertEqual(b"\\?", text_encoding.CUnescape("\\\\?"))
    self.assertEqual(b"\\?", text_encoding.CUnescape("\\\\\\?"))

  def testCUnescapeUnicode(self):
    self.assertEqual(b"\xc3\xa9", text_encoding.CUnescape("\\u00e9"))
    self.assertEqual(
        b"\xf0\x9f\x98\x80", text_encoding.CUnescape("\\U0001f600")
    )
    self.assertEqual(b"\\u00e9", text_encoding.CUnescape("\\\\u00e9"))

  def testCUnescapeUnicodeBackslash(self):
    self.assertEqual(b"\\", text_encoding.CUnescape("\\u005c"))
    self.assertEqual(b"\\n", text_encoding.CUnescape("\\u005cn"))
    self.assertEqual(b"\\x41", text_encoding.CUnescape("\\U0000005cx41"))
    self.assertEqual(b"\\\\", text_encoding.CUnescape("\\\\\\u005c"))


if __name__ == "__main__":
  unittest.main()
