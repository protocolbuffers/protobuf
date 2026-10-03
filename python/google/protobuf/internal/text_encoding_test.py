# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Tests for google.protobuf.text_encoding."""

import time
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

# Single digit hex escapes (like '\xf') are rewritten to two digits only when
# the run of backslashes in front of them is odd, i.e. when the last backslash
# is not itself escaped.  The pairs below pin that behaviour for both parities
# and for the lookahead that stops the rewrite when a second hex digit follows.
CUNESCAPE_SINGLE_DIGIT_HEX = [
    (r"\x1", b"\x01"),
    (r"\\x1", b"\\x1"),
    (r"\\\x1", b"\\\x01"),
    (r"\\\\x1", b"\\\\x1"),
    (r"\\x1;", b"\\x1;"),
    (r"\\x1a", b"\\x1a"),
    (r"a\\x1b", b"a\\x1b"),
    (r"\\x0\\x1", b"\\x0\\x1"),
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

  def testCUnescapeSingleDigitHexEscapes(self):
    for escaped, unescaped in CUNESCAPE_SINGLE_DIGIT_HEX:
      self.assertEqual(unescaped, text_encoding.CUnescape(escaped))

  def testCUnescapeLongBackslashRunIsLinear(self):
    # A run of backslashes that is not followed by a hex escape used to make
    # the scan quadratic (about 200s for the input below); the bound is three
    # orders of magnitude above the linear implementation's runtime.
    text = '\\' * (256 * 1024)
    start = time.time()
    result = text_encoding.CUnescape(text)
    self.assertLess(time.time() - start, 30.0)
    self.assertEqual(b'\\' * (128 * 1024), result)


if __name__ == "__main__":
  unittest.main()
