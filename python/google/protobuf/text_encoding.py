# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd
"""Encoding related utilities."""


def _AsciiIsPrint(i):
  return i >= 32 and i < 127


def _MakeStrEscapes():
  ret = {}
  for i in range(0, 128):
    if not _AsciiIsPrint(i):
      ret[i] = r'\%03o' % i
  ret[ord('\t')] = r'\t'  # optional escape
  ret[ord('\n')] = r'\n'  # optional escape
  ret[ord('\r')] = r'\r'  # optional escape
  ret[ord('"')] = r'\"'  # necessary escape
  ret[ord("'")] = r'\''  # optional escape
  ret[ord('\\')] = r'\\'  # necessary escape
  return ret


# Maps int -> char, performing string escapes.
_str_escapes = _MakeStrEscapes()

# Maps int -> char, performing byte escaping and string escapes
_byte_escapes = {i: chr(i) for i in range(0, 256)}
_byte_escapes.update(_str_escapes)
_byte_escapes.update({i: r'\%03o' % i for i in range(128, 256)})


def _DecodeUtf8EscapeErrors(text_bytes):
  ret = ''
  while text_bytes:
    try:
      ret += text_bytes.decode('utf-8').translate(_str_escapes)
      text_bytes = ''
    except UnicodeDecodeError as e:
      ret += text_bytes[: e.start].decode('utf-8').translate(_str_escapes)
      ret += _byte_escapes[text_bytes[e.start]]
      text_bytes = text_bytes[e.start + 1 :]
  return ret


def CEscape(text, as_utf8) -> str:
  """Escape a bytes string for use in an text protocol buffer.

  Args:
    text: A byte string to be escaped.
    as_utf8: Specifies if result may contain non-ASCII characters. In Python 3
      this allows unescaped non-ASCII Unicode characters. In Python 2 the return
      value will be valid UTF-8 rather than only ASCII.

  Returns:
    Escaped string (str).
  """
  # Python's text.encode() 'string_escape' or 'unicode_escape' codecs do not
  # satisfy our needs; they encodes unprintable characters using two-digit hex
  # escapes whereas our C++ unescaping function allows hex escapes to be any
  # length.  So, "\0011".encode('string_escape') ends up being "\\x011", which
  # will be decoded in C++ as a single-character string with char code 0x11.
  text_is_unicode = isinstance(text, str)
  if as_utf8:
    if text_is_unicode:
      return text.translate(_str_escapes)
    else:
      return _DecodeUtf8EscapeErrors(text)
  else:
    if text_is_unicode:
      text = text.encode('utf-8')
    return ''.join([_byte_escapes[c] for c in text])


_HEX_DIGITS = frozenset('0123456789abcdefABCDEF')


def _ReplaceSingleDigitHexEscapes(text: str) -> str:
  """Rewrites single-digit hex escapes to two digits, in linear time.

  Semantically identical to substituting the regex ``(\\+)x([0-9a-fA-F])
  (?![0-9a-fA-F])`` with ``\\1x0\\2`` when the run of backslashes is odd and
  with the match unchanged otherwise, but without a backtracking regex.
  The old pattern paired a greedy, unbounded ``(\\+)`` with a required
  ``x``: for a run of N backslashes not followed by ``x`` the engine retried
  every prefix of the run at every offset, i.e. Theta(N^2) steps.
  """
  i = text.find('\\')
  if i < 0:
    return text
  n = len(text)
  chunks = []
  start = 0
  while i >= 0:
    j = i
    while j < n and text[j] == '\\':
      j += 1
    run = j - i
    if (j + 1 < n and text[j] == 'x' and text[j + 1] in _HEX_DIGITS
        and (j + 2 >= n or text[j + 2] not in _HEX_DIGITS)):
      chunks.append(text[start:i])
      if run & 1:
        chunks.append(text[i:j])
        chunks.append('x0')
        chunks.append(text[j + 1])
      else:
        chunks.append(text[i:j + 2])
      start = j + 2
      i = text.find('\\', j + 2)
    else:
      i = text.find('\\', j)
  chunks.append(text[start:])
  return ''.join(chunks)


def CUnescape(text: str) -> bytes:
  """Unescape a text string with C-style escape sequences to UTF-8 bytes.

  Args:
    text: The data to parse in a str.

  Returns:
    A byte string.
  """

  # This is required because the 'string_escape' encoding doesn't
  # allow single-digit hex escapes (like '\xf').
  result = _ReplaceSingleDigitHexEscapes(text)

  # Replaces Unicode escape sequences with their character equivalents.
  result = result.encode('raw_unicode_escape').decode('raw_unicode_escape')
  # Encode Unicode characters as UTF-8, then decode to Latin-1 escaping
  # unprintable characters.
  result = result.encode('utf-8').decode('unicode_escape')
  # Convert Latin-1 text back to a byte string (latin-1 codec also works here).
  return result.encode('latin-1')
