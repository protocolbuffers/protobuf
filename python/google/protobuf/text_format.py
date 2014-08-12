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
# Copyright 2007 Google Inc. All Rights Reserved.

"""Contains routines for printing protocol messages in text format."""

__author__ = 'kenton@google.com (Kenton Varda)'

import cStringIO
import re

from google.protobuf.internal import type_checkers
from google.protobuf import descriptor
from google.protobuf import text_encoding

__all__ = ['MessageToString', 'PrintMessage', 'PrintField',
           'PrintFieldValue', 'Merge']


_INTEGER_CHECKERS = (type_checkers.Uint32ValueChecker(),
                     type_checkers.Int32ValueChecker(),
                     type_checkers.Uint64ValueChecker(),
                     type_checkers.Int64ValueChecker())
_FLOAT_INFINITY = re.compile('-?inf(?:inity)?f?', re.IGNORECASE)
_FLOAT_NAN = re.compile('nanf?', re.IGNORECASE)
_FLOAT_TYPES = frozenset([descriptor.FieldDescriptor.CPPTYPE_FLOAT,
                          descriptor.FieldDescriptor.CPPTYPE_DOUBLE])


class Error(Exception):
  """Top-level module error for text_format."""


class ParseError(Error):
  """Thrown in case of ASCII parsing error."""


def MessageToString(message, as_utf8=False, as_one_line=False,
                    pointy_brackets=False, use_index_order=False,
                    float_format=None):
  """Convert protobuf message to text format.

  Floating point values can be formatted compactly with 15 digits of
  precision (which is the most that IEEE 754 "double" can guarantee)
  using float_format='.15g'.

  Args:
    message: The protocol buffers message.
    as_utf8: Produce text output in UTF8 format.
    as_one_line: Don't introduce newlines between fields.
    pointy_brackets: If True, use angle brackets instead of curly braces for
      nesting.
    use_index_order: If True, print fields of a proto message using the order
      defined in source code instead of the field number. By default, use the
      field number order.
    float_format: If set, use this to specify floating point number formatting
      (per the "Format Specification Mini-Language"); otherwise, str() is used.

  Returns:
    A string of the text formatted protocol buffer message.
  """
  out = cStringIO.StringIO()
  PrintMessage(message, out, as_utf8=as_utf8, as_one_line=as_one_line,
               pointy_brackets=pointy_brackets,
               use_index_order=use_index_order,
               float_format=float_format)
  result = out.getvalue()
  out.close()
  if as_one_line:
    return result.rstrip()
  return result


def PrintMessage(message, out, indent=0, as_utf8=False, as_one_line=False,
                 pointy_brackets=False, use_index_order=False,
                 float_format=None):
  fields = message.ListFields()
  if use_index_order:
    fields.sort(key=lambda x: x[0].index)
  for field, value in fields:
    if field.label == descriptor.FieldDescriptor.LABEL_REPEATED:
      for element in value:
        PrintField(field, element, out, indent, as_utf8, as_one_line,
                   pointy_brackets=pointy_brackets,
                   float_format=float_format)
    else:
      PrintField(field, value, out, indent, as_utf8, as_one_line,
                 pointy_brackets=pointy_brackets,
                 float_format=float_format)


def PrintField(field, value, out, indent=0, as_utf8=False, as_one_line=False,
               pointy_brackets=False, float_format=None):
  """Print a single field name/value pair.  For repeated fields, the value
  should be a single element."""

  out.write(' ' * indent)
  if field.is_extension:
    out.write('[')
    if (field.containing_type.GetOptions().message_set_wire_format and
        field.type == descriptor.FieldDescriptor.TYPE_MESSAGE and
        field.message_type == field.extension_scope and
        field.label == descriptor.FieldDescriptor.LABEL_OPTIONAL):
      out.write(field.message_type.full_name)
    else:
      out.write(field.full_name)
    out.write(']')
  elif field.type == descriptor.FieldDescriptor.TYPE_GROUP:
    # For groups, use the capitalized name.
    out.write(field.message_type.name)
  else:
    out.write(field.name)

  if field.cpp_type != descriptor.FieldDescriptor.CPPTYPE_MESSAGE:
    # The colon is optional in this case, but our cross-language golden files
    # don't include it.
    out.write(': ')

  PrintFieldValue(field, value, out, indent, as_utf8, as_one_line,
                  pointy_brackets=pointy_brackets,
                  float_format=float_format)
  if as_one_line:
    out.write(' ')
  else:
    out.write('\n')


def PrintFieldValue(field, value, out, indent=0, as_utf8=False,
                    as_one_line=False, pointy_brackets=False,
                    float_format=None):
  """Print a single field value (not including name).  For repeated fields,
  the value should be a single element."""

  if pointy_brackets:
    openb = '<'
    closeb = '>'
  else:
    openb = '{'
    closeb = '}'

  if field.cpp_type == descriptor.FieldDescriptor.CPPTYPE_MESSAGE:
    if as_one_line:
      out.write(' %s ' % openb)
      PrintMessage(value, out, indent, as_utf8, as_one_line,
                   pointy_brackets=pointy_brackets,
                   float_format=float_format)
      out.write(closeb)
    else:
      out.write(' %s\n' % openb)
      PrintMessage(value, out, indent + 2, as_utf8, as_one_line,
                   pointy_brackets=pointy_brackets,
                   float_format=float_format)
      out.write(' ' * indent + closeb)
  elif field.cpp_type == descriptor.FieldDescriptor.CPPTYPE_ENUM:
    enum_value = field.enum_type.values_by_number.get(value, None)
    if enum_value is not None:
      out.write(enum_value.name)
    else:
      out.write(str(value))
  elif field.cpp_type == descriptor.FieldDescriptor.CPPTYPE_STRING:
    out.write('\"')
    if isinstance(value, unicode):
      out_value = value.encode('utf-8')
    else:
      out_value = value
    if field.type == descriptor.FieldDescriptor.TYPE_BYTES:
      # We need to escape non-UTF8 chars in TYPE_BYTES field.
      out_as_utf8 = False
    else:
      out_as_utf8 = as_utf8
    out.write(text_encoding.CEscape(out_value, out_as_utf8))
    out.write('\"')
  elif field.cpp_type == descriptor.FieldDescriptor.CPPTYPE_BOOL:
    if value:
      out.write('true')
    else:
      out.write('false')
  elif field.cpp_type in _FLOAT_TYPES and float_format is not None:
    out.write('{1:{0}}'.format(float_format, value))
  else:
    out.write(str(value))


def _ParseOrMerge(lines, message, allow_multiple_scalars):
  """Converts an ASCII representation of a protocol message into a message.

  Args:
    lines: Lines of a message's ASCII representation.
    message: A protocol buffer message to merge into.
    allow_multiple_scalars: Determines if repeated values for a non-repeated
      field are permitted, e.g., the string "foo: 1 foo: 2" for a
      required/optional field named "foo".

  Raises:
    ParseError: On ASCII parsing problems.
  """
  tokenizer = _Tokenizer(lines)
  while not tokenizer.AtEnd():
    _MergeField(tokenizer, message, allow_multiple_scalars)


def Parse(text, message):
  """Parses an ASCII representation of a protocol message into a message.

  Args:
    text: Message ASCII representation.
    message: A protocol buffer message to merge into.

  Returns:
    The same message passed as argument.

  Raises:
    ParseError: On ASCII parsing problems.
  """
  if not isinstance(text, str): text = text.decode('utf-8')
  return ParseLines(text.split('\n'), message)


def Merge(text, message):
  """Parses an ASCII representation of a protocol message into a message.

  Like Parse(), but allows repeated values for a non-repeated field, and uses
  the last one.

  Args:
    text: Message ASCII representation.
    message: A protocol buffer message to merge into.

  Returns:
    The same message passed as argument.

  Raises:
    ParseError: On ASCII parsing problems.
  """
  return MergeLines(text.split('\n'), message)


def ParseLines(lines, message):
  """Parses an ASCII representation of a protocol message into a message.

  Args:
    lines: An iterable of lines of a message's ASCII representation.
    message: A protocol buffer message to merge into.

  Returns:
    The same message passed as argument.

  Raises:
    ParseError: On ASCII parsing problems.
  """
  _ParseOrMerge(lines, message, False)
  return message


def MergeLines(lines, message):
  """Parses an ASCII representation of a protocol message into a message.

  Args:
    lines: An iterable of lines of a message's ASCII representation.
    message: A protocol buffer message to merge into.

  Returns:
    The same message passed as argument.

  Raises:
    ParseError: On ASCII parsing problems.
  """
  _ParseOrMerge(lines, message, True)
  return message


def _MergeField(tokenizer, message, allow_multiple_scalars):
  """Merges a single protocol message field into a message.

  Args:
    tokenizer: A tokenizer to parse the field name and values.
    message: A protocol message to record the data.
    allow_multiple_scalars: Determines if repeated values for a non-repeated
      field are permitted, e.g., the string "foo: 1 foo: 2" for a
      required/optional field named "foo".

  Raises:
    ParseError: In case of ASCII parsing problems.
  """
  message_descriptor = message.DESCRIPTOR
  if tokenizer.TryConsume('['):
    name = [tokenizer.ConsumeIdentifier()]
    while tokenizer.TryConsume('.'):
      name.append(tokenizer.ConsumeIdentifier())
    name = '.'.join(name)

    if not message_descriptor.is_extendable:
      raise tokenizer.ParseErrorPreviousToken(
          'Message type "%s" does not have extensions.' %
          message_descriptor.full_name)
    # pylint: disable=protected-access
    field = message.Extensions._FindExtensionByName(name)
    # pylint: enable=protected-access
    if not field:
      raise tokenizer.ParseErrorPreviousToken(
          'Extension "%s" not registered.' % name)
    elif message_descriptor != field.containing_type:
      raise tokenizer.ParseErrorPreviousToken(
          'Extension "%s" does not extend message type "%s".' % (
              name, message_descriptor.full_name))
    tokenizer.Consume(']')
  else:
    name = tokenizer.ConsumeIdentifier()
    field = message_descriptor.fields_by_name.get(name, None)

    # Group names are expected to be capitalized as they appear in the
    # .proto file, which actually matches their type names, not their field
    # names.
    if not field:
      field = message_descriptor.fields_by_name.get(name.lower(), None)
      if field and field.type != descriptor.FieldDescriptor.TYPE_GROUP:
        field = None

    if (field and field.type == descriptor.FieldDescriptor.TYPE_GROUP and
        field.message_type.name != name):
      field = None

    if not field:
      raise tokenizer.ParseErrorPreviousToken(
          'Message type "%s" has no field named "%s".' % (
              message_descriptor.full_name, name))

  if field.cpp_type == descriptor.FieldDescriptor.CPPTYPE_MESSAGE:
    tokenizer.TryConsume(':')

    if tokenizer.TryConsume('<'):
      end_token = '>'
    else:
      tokenizer.Consume('{')
      end_token = '}'

    if field.label == descriptor.FieldDescriptor.LABEL_REPEATED:
      if field.is_extension:
        sub_message = message.Extensions[field].add()
      else:
        sub_message = getattr(message, field.name).add()
    else:
      if field.is_extension:
        sub_message = message.Extensions[field]
      else:
        sub_message = getattr(message, field.name)
      sub_message.SetInParent()

    while not tokenizer.TryConsume(end_token):
      if tokenizer.AtEnd():
        raise tokenizer.ParseErrorPreviousToken('Expected "%s".' % (end_token))
      _MergeField(tokenizer, sub_message, allow_multiple_scalars)
  else:
    _MergeScalarField(tokenizer, message, field, allow_multiple_scalars)

  # For historical reasons, fields may optionally be separated by commas or
  # semicolons.
  if not tokenizer.TryConsume(','):
    tokenizer.TryConsume(';')


def _MergeScalarField(tokenizer, message, field, allow_multiple_scalars):
  """Merges a single protocol message scalar field into a message.

  Args:
    tokenizer: A tokenizer to parse the field value.
    message: A protocol message to record the data.
    field: The descriptor of the field to be merged.
    allow_multiple_scalars: Determines if repeated values for a non-repeated
      field are permitted, e.g., the string "foo: 1 foo: 2" for a
      required/optional field named "foo".

  Raises:
    ParseError: In case of ASCII parsing problems.
    RuntimeError: On runtime errors.
  """
  tokenizer.Consume(':')
  value = None

  if field.type in (descriptor.FieldDescriptor.TYPE_INT32,
                    descriptor.FieldDescriptor.TYPE_SINT32,
                    descriptor.FieldDescriptor.TYPE_SFIXED32):
    value = tokenizer.ConsumeInt32()
  elif field.type in (descriptor.FieldDescriptor.TYPE_INT64,
                      descriptor.FieldDescriptor.TYPE_SINT64,
                      descriptor.FieldDescriptor.TYPE_SFIXED64):
    value = tokenizer.ConsumeInt64()
  elif field.type in (descriptor.FieldDescriptor.TYPE_UINT32,
                      descriptor.FieldDescriptor.TYPE_FIXED32):
    value = tokenizer.ConsumeUint32()
  elif field.type in (descriptor.FieldDescriptor.TYPE_UINT64,
                      descriptor.FieldDescriptor.TYPE_FIXED64):
    value = tokenizer.ConsumeUint64()
  elif field.type in (descriptor.FieldDescriptor.TYPE_FLOAT,
                      descriptor.FieldDescriptor.TYPE_DOUBLE):
    value = tokenizer.ConsumeFloat()
  elif field.type == descriptor.FieldDescriptor.TYPE_BOOL:
    value = tokenizer.ConsumeBool()
  elif field.type == descriptor.FieldDescriptor.TYPE_STRING:
    value = tokenizer.ConsumeString()
  elif field.type == descriptor.FieldDescriptor.TYPE_BYTES:
    value = tokenizer.ConsumeByteString()
  elif field.type == descriptor.FieldDescriptor.TYPE_ENUM:
    value = tokenizer.ConsumeEnum(field)
  else:
    raise RuntimeError('Unknown field type %d' % field.type)

  if field.label == descriptor.FieldDescriptor.LABEL_REPEATED:
    if field.is_extension:
      message.Extensions[field].append(value)
    else:
      getattr(message, field.name).append(value)
  else:
    if field.is_extension:
      if not allow_multiple_scalars and message.HasExtension(field):
        raise tokenizer.ParseErrorPreviousToken(
            'Message type "%s" should not have multiple "%s" extensions.' %
            (message.DESCRIPTOR.full_name, field.full_name))
      else:
        message.Extensions[field] = value
    else:
      if not allow_multiple_scalars and message.HasField(field.name):
        raise tokenizer.ParseErrorPreviousToken(
            'Message type "%s" should not have multiple "%s" fields.' %
            (message.DESCRIPTOR.full_name, field.name))
      else:
        setattr(message, field.name, value)


class _Tokenizer(object):
  """Protocol buffer ASCII representation tokenizer.

  This class handles the lower level string parsing by splitting it into
  meaningful tokens.

  It was directly ported from the Java protocol buffer API.
  """

  _WHITESPACE = re.compile('(\\s|(#.*$))+', re.MULTILINE)
  _TOKEN = re.compile(
      '[a-zA-Z_][0-9a-zA-Z_+-]*|'           # an identifier
      '[0-9+-][0-9a-zA-Z_.+-]*|'            # a number
      '\"([^\"\n\\\\]|\\\\.)*(\"|\\\\?$)|'  # a double-quoted string
      '\'([^\'\n\\\\]|\\\\.)*(\'|\\\\?$)')  # a single-quoted string
  _IDENTIFIER = re.compile(r'\w+')

  def __init__(self, lines):
    self._position = 0
    self._line = -1
    self._column = 0
    self._token_start = None
    self.token = ''
    self._lines = iter(lines)
    self._current_line = ''
    self._previous_line = 0
    self._previous_column = 0
    self._more_lines = True
    self._SkipWhitespace()
    self.NextToken()

  def AtEnd(self):
    """Checks the end of the text was reached.

    Returns:
      True iff the end was reached.
    """
    return not self.token

  def _PopLine(self):
    while len(self._current_line) <= self._column:
      try:
        self._current_line = self._lines.next()
      except StopIteration:
        self._current_line = ''
        self._more_lines = False
        return
      else:
        self._line += 1
        self._column = 0

  def _SkipWhitespace(self):
    while True:
      self._PopLine()
      match = self._WHITESPACE.match(self._current_line, self._column)
      if not match:
        break
      length = len(match.group(0))
      self._column += length

  def TryConsume(self, token):
    """Tries to consume a given piece of text.

    Args:
      token: Text to consume.

    Returns:
      True iff the text was consumed.
    """
    if self.token == token:
      self.NextToken()
      return True
    return False

  def Consume(self, token):
    """Consumes a piece of text.

    Args:
      token: Text to consume.

    Raises:
      ParseError: If the text couldn't be consumed.
    """
    if not self.TryConsume(token):
      raise self._ParseError('Expected "%s".' % token)

  def ConsumeIdentifier(self):
    """Consumes protocol message field identifier.

    Returns:
      Identifier string.

    Raises:
      ParseError: If an identifier couldn't be consumed.
    """
    result = self.token
    if not self._IDENTIFIER.match(result):
      raise self._ParseError('Expected identifier.')
    self.NextToken()
    return result

  def ConsumeInt32(self):
    """Consumes a signed 32bit integer number.

    Returns:
      The integer parsed.

    Raises:
      ParseError: If a signed 32bit integer couldn't be consumed.
    """
    try:
      result = ParseInteger(self.token, is_signed=True, is_long=False)
    except ValueError, e:
      raise self._ParseError(str(e))
    self.NextToken()
    return result

  def ConsumeUint32(self):
    """Consumes an unsigned 32bit integer number.

    Returns:
      The integer parsed.

    Raises:
      ParseError: If an unsigned 32bit integer couldn't be consumed.
    """
    try:
      result = ParseInteger(self.token, is_signed=False, is_long=False)
    except ValueError, e:
      raise self._ParseError(str(e))
    self.NextToken()
    return result

  def ConsumeInt64(self):
    """Consumes a signed 64bit integer number.

    Returns:
      The integer parsed.

    Raises:
      ParseError: If a signed 64bit integer couldn't be consumed.
    """
    try:
      result = ParseInteger(self.token, is_signed=True, is_long=True)
    except ValueError, e:
      raise self._ParseError(str(e))
    self.NextToken()
    return result

  def ConsumeUint64(self):
    """Consumes an unsigned 64bit integer number.

    Returns:
      The integer parsed.

    Raises:
      ParseError: If an unsigned 64bit integer couldn't be consumed.
    """
    try:
      result = ParseInteger(self.token, is_signed=False, is_long=True)
    except ValueError, e:
      raise self._ParseError(str(e))
    self.NextToken()
    return result

  def ConsumeFloat(self):
    """Consumes an floating point number.

    Returns:
      The number parsed.

    Raises:
      ParseError: If a floating point number couldn't be consumed.
    """
    try:
      result = ParseFloat(self.token)
    except ValueError, e:
      raise self._ParseError(str(e))
    self.NextToken()
    return result

  def ConsumeBool(self):
    """Consumes a boolean value.

    Returns:
      The bool parsed.

    Raises:
      ParseError: If a boolean value couldn't be consumed.
    """
    try:
      result = ParseBool(self.token)
    except ValueError, e:
      raise self._ParseError(str(e))
    self.NextToken()
    return result

  def ConsumeString(self):
    """Consumes a string value.

    Returns:
      The string parsed.

    Raises:
      ParseError: If a string value couldn't be consumed.
    """
    the_bytes = self.ConsumeByteString()
    try:
      return unicode(the_bytes, 'utf-8')
    except UnicodeDecodeError, e:
      raise self._StringParseError(e)

  def ConsumeByteString(self):
    """Consumes a byte array value.

    Returns:
      The array parsed (as a string).

    Raises:
      ParseError: If a byte array value couldn't be consumed.
    """
    the_list = [self._ConsumeSingleByteString()]
    while self.token and self.token[0] in ('\'', '"'):
      the_list.append(self._ConsumeSingleByteString())
    return ''.encode('latin1').join(the_list)  ##PY25
##!PY25    return b''.join(the_list)

  def _ConsumeSingleByteString(self):
    """Consume one token of a string literal.

    String literals (whether bytes or text) can come in multiple adjacent
    tokens which are automatically concatenated, like in C or Python.  This
    method only consumes one token.
    """
    text = self.token
    if len(text) < 1 or text[0] not in ('\'', '"'):
      raise self._ParseError('Expected string.')

    if len(text) < 2 or text[-1] != text[0]:
      raise self._ParseError('String missing ending quote.')

    try:
      result = text_encoding.CUnescape(text[1:-1])
    except ValueError, e:
      raise self._ParseError(str(e))
    self.NextToken()
    return result

  def ConsumeEnum(self, field):
    try:
      result = ParseEnum(field, self.token)
    except ValueError, e:
      raise self._ParseError(str(e))
    self.NextToken()
    return result

  def ParseErrorPreviousToken(self, message):
    """Creates and *returns* a ParseError for the previously read token.

    Args:
      message: A message to set for the exception.

    Returns:
      A ParseError instance.
    """
    return ParseError('%d:%d : %s' % (
        self._previous_line + 1, self._previous_column + 1, message))

  def _ParseError(self, message):
    """Creates and *returns* a ParseError for the current token."""
    return ParseError('%d:%d : %s' % (
        self._line + 1, self._column + 1, message))

  def _StringParseError(self, e):
    return self._ParseError('Couldn\'t parse string: ' + str(e))

  def NextToken(self):
    """Reads the next meaningful token."""
    self._previous_line = self._line
    self._previous_column = self._column

    self._column += len(self.token)
    self._SkipWhitespace()

    if not self._more_lines:
      self.token = ''
      return

    match = self._TOKEN.match(self._current_line, self._column)
    if match:
      token = match.group(0)
      self.token = token
    else:
      self.token = self._current_line[self._column]


def ParseInteger(text, is_signed=False, is_long=False):
  """Parses an integer.

  Args:
    text: The text to parse.
    is_signed: True if a signed integer must be parsed.
    is_long: True if a long integer must be parsed.

  Returns:
    The integer value.

  Raises:
    ValueError: Thrown Iff the text is not a valid integer.
  """
  # Do the actual parsing. Exception handling is propagated to caller.
  try:
    # We force 32-bit values to int and 64-bit values to long to make
    # alternate implementations where the distinction is more significant
    # (e.g. the C++ implementation) simpler.
    if is_long:
      result = long(text, 0)
    else:
      result = int(text, 0)
  except ValueError:
    raise ValueError('Couldn\'t parse integer: %s' % text)

  # Check if the integer is sane. Exceptions handled by callers.
  checker = _INTEGER_CHECKERS[2 * int(is_long) + int(is_signed)]
  checker.CheckValue(result)
  return result


def ParseFloat(text):
  """Parse a floating point number.

  Args:
    text: Text to parse.

  Returns:
    The number parsed.

  Raises:
    ValueError: If a floating point number couldn't be parsed.
  """
  try:
    # Assume Python compatible syntax.
    return float(text)
  except ValueError:
    # Check alternative spellings.
    if _FLOAT_INFINITY.match(text):
      if text[0] == '-':
        return float('-inf')
      else:
        return float('inf')
    elif _FLOAT_NAN.match(text):
      return float('nan')
    else:
      # assume '1.0f' format
      try:
        return float(text.rstrip('f'))
      except ValueError:
        raise ValueError('Couldn\'t parse float: %s' % text)


def ParseBool(text):
  """Parse a boolean value.

  Args:
    text: Text to parse.

  Returns:
    Boolean values parsed

  Raises:
    ValueError: If text is not a valid boolean.
  """
  if text in ('true', 't', '1'):
    return True
  elif text in ('false', 'f', '0'):
    return False
  else:
    raise ValueError('Expected "true" or "false".')


def ParseEnum(field, value):
  """Parse an enum value.

  The value can be specified by a number (the enum value), or by
  a string literal (the enum name).

  Args:
    field: Enum field descriptor.
    value: String value.

  Returns:
    Enum value number.

  Raises:
    ValueError: If the enum value could not be parsed.
  """
  enum_descriptor = field.enum_type
  try:
    number = int(value, 0)
  except ValueError:
    # Identifier.
    enum_value = enum_descriptor.values_by_name.get(value, None)
    if enum_value is None:
      raise ValueError(
          'Enum type "%s" has no value named %s.' % (
              enum_descriptor.full_name, value))
  else:
    # Numeric value.
    enum_value = enum_descriptor.values_by_number.get(number, None)
    if enum_value is None:
      raise ValueError(
          'Enum type "%s" has no value with number %d.' % (
              enum_descriptor.full_name, number))
  return enum_value.number
