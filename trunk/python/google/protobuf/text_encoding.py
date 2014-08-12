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

#PY25 compatible for GAE.
#
"""Encoding related utilities."""

import re
import sys  ##PY25

# Lookup table for utf8
_cescape_utf8_to_str = [chr(i) for i in xrange(0, 256)]
_cescape_utf8_to_str[9] = r'\t'  # optional escape
_cescape_utf8_to_str[10] = r'\n'  # optional escape
_cescape_utf8_to_str[13] = r'\r'  # optional escape
_cescape_utf8_to_str[39] = r"\'"  # optional escape

_cescape_utf8_to_str[34] = r'\"'  # necessary escape
_cescape_utf8_to_str[92] = r'\\'  # necessary escape

# Lookup table for non-utf8, with necessary escapes at (o >= 127 or o < 32)
_cescape_byte_to_str = ([r'\%03o' % i for i in xrange(0, 32)] +
                        [chr(i) for i in xrange(32, 127)] +
                        [r'\%03o' % i for i in xrange(127, 256)])
_cescape_byte_to_str[9] = r'\t'  # optional escape
_cescape_byte_to_str[10] = r'\n'  # optional escape
_cescape_byte_to_str[13] = r'\r'  # optional escape
_cescape_byte_to_str[39] = r"\'"  # optional escape

_cescape_byte_to_str[34] = r'\"'  # necessary escape
_cescape_byte_to_str[92] = r'\\'  # necessary escape


def CEscape(text, as_utf8):
  """Escape a bytes string for use in an ascii protocol buffer.

  text.encode('string_escape') does not seem to satisfy our needs as it
  encodes unprintable characters using two-digit hex escapes whereas our
  C++ unescaping function allows hex escapes to be any length.  So,
  "\0011".encode('string_escape') ends up being "\\x011", which will be
  decoded in C++ as a single-character string with char code 0x11.

  Args:
    text: A byte string to be escaped
    as_utf8: Specifies if result should be returned in UTF-8 encoding
  Returns:
    Escaped string
  """
  # PY3 hack: make Ord work for str and bytes:
  # //platforms/networking/data uses unicode here, hence basestring.
  Ord = ord if isinstance(text, basestring) else lambda x: x
  if as_utf8:
    return ''.join(_cescape_utf8_to_str[Ord(c)] for c in text)
  return ''.join(_cescape_byte_to_str[Ord(c)] for c in text)


_CUNESCAPE_HEX = re.compile(r'(\\+)x([0-9a-fA-F])(?![0-9a-fA-F])')
_cescape_highbit_to_str = ([chr(i) for i in range(0, 127)] +
                           [r'\%03o' % i for i in range(127, 256)])


def CUnescape(text):
  """Unescape a text string with C-style escape sequences to UTF-8 bytes."""

  def ReplaceHex(m):
    # Only replace the match if the number of leading back slashes is odd. i.e.
    # the slash itself is not escaped.
    if len(m.group(1)) & 1:
      return m.group(1) + 'x0' + m.group(2)
    return m.group(0)

  # This is required because the 'string_escape' encoding doesn't
  # allow single-digit hex escapes (like '\xf').
  result = _CUNESCAPE_HEX.sub(ReplaceHex, text)

  if sys.version_info[0] < 3:  ##PY25
##!PY25  if str is bytes:  # PY2
    return result.decode('string_escape')
  result = ''.join(_cescape_highbit_to_str[ord(c)] for c in result)
  return (result.encode('ascii')  # Make it bytes to allow decode.
          .decode('unicode_escape')
          # Make it bytes again to return the proper type.
          .encode('raw_unicode_escape'))
