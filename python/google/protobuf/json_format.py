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

"""Contains routines for printing protocol messages in JSON format."""

__author__ = 'jieluo@google.com (Jie Luo)'

import base64
from datetime import datetime
import json
import math
import re
import sys

from google.protobuf import descriptor

_TIMESTAMPFOMAT = '%Y-%m-%dT%H:%M:%S'
_NUMBER = re.compile(u'[0-9+-][0-9e.+-]*')
_INTEGER = re.compile(u'[0-9+-]')
_INT_TYPES = frozenset([descriptor.FieldDescriptor.CPPTYPE_INT32,
                        descriptor.FieldDescriptor.CPPTYPE_UINT32,
                        descriptor.FieldDescriptor.CPPTYPE_INT64,
                        descriptor.FieldDescriptor.CPPTYPE_UINT64])
_INT64_TYPES = frozenset([descriptor.FieldDescriptor.CPPTYPE_INT64,
                          descriptor.FieldDescriptor.CPPTYPE_UINT64])
_FLOAT_TYPES = frozenset([descriptor.FieldDescriptor.CPPTYPE_FLOAT,
                          descriptor.FieldDescriptor.CPPTYPE_DOUBLE])
if str is bytes:
  _UNICODETYPE = unicode
else:
  _UNICODETYPE = str


class SerializeToJsonError(Exception):
  """Thrown if serialization to JSON fails."""


class ParseError(Exception):
  """Thrown in case of parsing error."""


def MessageToJson(message, including_default_value_fields=False):
  """Converts protobuf message to JSON format.

  Args:
    message: The protocol buffers message instance to serialize.
    including_default_value_fields: If True, singular primitive fields,
        repeated fields, and map fields will always be serialized.  If
        False, only serialize non-empty fields.  Singular message fields
        and oneof fields are not affected by this option.

  Returns:
    A string containing the JSON formatted protocol buffer message.
  """
  js = _MessageToJsonObject(message, including_default_value_fields)
  return json.dumps(js, indent=2)


def _MessageToJsonObject(message, including_default_value_fields):
  """Converts message to an object according to Proto3 JSON Specification."""
  message_descriptor = message.DESCRIPTOR
  if _IsTimestampMessage(message_descriptor):
    return _TimestampMessageToJsonObject(message)
  if _IsDurationMessage(message_descriptor):
    return _DurationMessageToJsonObject(message)
  if _IsFieldMaskMessage(message_descriptor):
    return _FieldMaskMessageToJsonObject(message)
  if _IsWrapperMessage(message_descriptor):
    return _WrapperMessageToJsonObject(message)
  return _RegularMessageToJsonObject(message, including_default_value_fields)


def _IsMapEntry(field):
  return (field.type == descriptor.FieldDescriptor.TYPE_MESSAGE and
          field.message_type.has_options and
          field.message_type.GetOptions().map_entry)


def _RegularMessageToJsonObject(message, including_default_value_fields):
  """Converts normal message according to Proto3 JSON Specification."""
  js = {}
  fields = message.ListFields()

  try:
    for field, value in fields:
      name = field.camelcase_name
      if _IsMapEntry(field):
        # Convert a map field.
        js_map = {}
        for key in value:
          if isinstance(key, bool):
            if key:
              recorded_key = 'true'
            else:
              recorded_key = 'false'
          else:
            recorded_key = key
          js_map[recorded_key] = _ConvertFieldToJsonObject(
              field.message_type.fields_by_name['value'],
              value[key], including_default_value_fields)
        js[name] = js_map
      elif field.label == descriptor.FieldDescriptor.LABEL_REPEATED:
        # Convert a repeated field.
        repeated = []
        for element in value:
          repeated.append(_ConvertFieldToJsonObject(
              field, element, including_default_value_fields))
        js[name] = repeated
      else:
        js[name] = _ConvertFieldToJsonObject(
            field, value, including_default_value_fields)

    # Serialize default value if including_default_value_fields is True.
    if including_default_value_fields:
      message_descriptor = message.DESCRIPTOR
      for field in message_descriptor.fields:
        # Singular message fields and oneof fields will not be affected.
        if ((field.label != descriptor.FieldDescriptor.LABEL_REPEATED and
             field.cpp_type == descriptor.FieldDescriptor.CPPTYPE_MESSAGE) or
            field.containing_oneof):
          continue
        name = field.camelcase_name
        if name in js:
          # Skip the field which has been serailized already.
          continue
        if _IsMapEntry(field):
          js[name] = {}
        elif field.label == descriptor.FieldDescriptor.LABEL_REPEATED:
          js[name] = []
        else:
          js[name] = _ConvertFieldToJsonObject(field, field.default_value)

  except ValueError as e:
    raise SerializeToJsonError(
        'Failed to serialize {0} field: {1}'.format(field.name, e))

  return js


def _ConvertFieldToJsonObject(
    field, value, including_default_value_fields=False):
  """Converts field value according to Proto3 JSON Specification."""
  if field.cpp_type == descriptor.FieldDescriptor.CPPTYPE_MESSAGE:
    return _MessageToJsonObject(value, including_default_value_fields)
  elif field.cpp_type == descriptor.FieldDescriptor.CPPTYPE_ENUM:
    enum_value = field.enum_type.values_by_number.get(value, None)
    if enum_value is not None:
      return enum_value.name
    else:
      raise SerializeToJsonError('Enum field contains an integer value '
                                 'which can not mapped to an enum value.')
  elif field.cpp_type == descriptor.FieldDescriptor.CPPTYPE_STRING:
    if field.type == descriptor.FieldDescriptor.TYPE_BYTES:
      # Use base64 Data encoding for bytes
      return base64.b64encode(value).decode('utf-8')
    else:
      return value
  elif field.cpp_type == descriptor.FieldDescriptor.CPPTYPE_BOOL:
    if value:
      return True
    else:
      return False
  elif field.cpp_type in _INT64_TYPES:
    return str(value)
  elif field.cpp_type in _FLOAT_TYPES:
    if math.isinf(value):
      if value < 0.0:
        return '-Infinity'
      else:
        return 'Infinity'
    if math.isnan(value):
      return 'NaN'
  return value


def _IsTimestampMessage(message_descriptor):
  return (message_descriptor.name == 'Timestamp' and
          message_descriptor.file.name == 'google/protobuf/timestamp.proto')


def _TimestampMessageToJsonObject(message):
  """Converts Timestamp message according to Proto3 JSON Specification."""
  nanos = message.nanos % 1e9
  dt = datetime.utcfromtimestamp(
      message.seconds + (message.nanos - nanos) / 1e9)
  result = dt.isoformat()
  if (nanos % 1e9) == 0:
    # If there are 0 fractional digits, the fractional
    # point '.' should be omitted when serializing.
    return result + 'Z'
  if (nanos % 1e6) == 0:
    # Serialize 3 fractional digits.
    return result + '.%03dZ' % (nanos / 1e6)
  if (nanos % 1e3) == 0:
    # Serialize 6 fractional digits.
    return result + '.%06dZ' % (nanos / 1e3)
  # Serialize 9 fractional digits.
  return result + '.%09dZ' % nanos


def _IsDurationMessage(message_descriptor):
  return (message_descriptor.name == 'Duration' and
          message_descriptor.file.name == 'google/protobuf/duration.proto')


def _DurationMessageToJsonObject(message):
  """Converts Duration message according to Proto3 JSON Specification."""
  if message.seconds < 0 or message.nanos < 0:
    result = '-'
    seconds = - message.seconds + int((0 - message.nanos) / 1e9)
    nanos = (0 - message.nanos) % 1e9
  else:
    result = ''
    seconds = message.seconds + int(message.nanos / 1e9)
    nanos = message.nanos % 1e9
  result += '%d' % seconds
  if (nanos % 1e9) == 0:
    # If there are 0 fractional digits, the fractional
    # point '.' should be omitted when serializing.
    return result + 's'
  if (nanos % 1e6) == 0:
    # Serialize 3 fractional digits.
    return result + '.%03ds' % (nanos / 1e6)
  if (nanos % 1e3) == 0:
    # Serialize 6 fractional digits.
    return result + '.%06ds' % (nanos / 1e3)
    # Serialize 9 fractional digits.
  return result + '.%09ds' % nanos


def _IsFieldMaskMessage(message_descriptor):
  return (message_descriptor.name == 'FieldMask' and
          message_descriptor.file.name == 'google/protobuf/field_mask.proto')


def _FieldMaskMessageToJsonObject(message):
  """Converts FieldMask message according to Proto3 JSON Specification."""
  result = ''
  first = True
  for path in message.paths:
    if not first:
      result += ','
    result += path
    first = False
  return result


def _IsWrapperMessage(message_descriptor):
  return message_descriptor.file.name == 'google/protobuf/wrappers.proto'


def _WrapperMessageToJsonObject(message):
  return _ConvertFieldToJsonObject(
      message.DESCRIPTOR.fields_by_name['value'], message.value)


def _DuplicateChecker(js):
  result = {}
  for name, value in js:
    if name in result:
      raise ParseError('Failed to load JSON: duplicate key ' + name)
    result[name] = value
  return result


def Parse(text, message):
  """Parses a JSON representation of a protocol message into a message.

  Args:
    text: Message JSON representation.
    message: A protocol beffer message to merge into.

  Returns:
    The same message passed as argument.

  Raises::
    ParseError: On JSON parsing problems.
  """
  if not isinstance(text, _UNICODETYPE): text = text.decode('utf-8')
  try:
    if sys.version_info < (2, 7):
      # object_pair_hook is not supported before python2.7
      js = json.loads(text)
    else:
      js = json.loads(text, object_pairs_hook=_DuplicateChecker)
  except ValueError as e:
    raise ParseError('Failed to load JSON: ' + str(e))
  _ConvertFieldValuePair(js, message)
  return message


def _ConvertFieldValuePair(js, message):
  """Convert field value pairs into regular message.

  Args:
    js: A JSON object to convert the field value pairs.
    message: A regular protocol message to record the data.

  Raises:
    ParseError: In case of problems converting.
  """
  names = []
  message_descriptor = message.DESCRIPTOR
  for name in js:
    try:
      field = message_descriptor.fields_by_camelcase_name.get(name, None)
      if not field:
        raise ParseError(
            'Message type "{0}" has no field named "{1}".'.format(
                message_descriptor.full_name, name))
      if name in names:
        raise ParseError(
            'Message type "{0}" should not have multiple "{1}" fields.'.format(
                message.DESCRIPTOR.full_name, name))
      names.append(name)
      # Check no other oneof field is parsed.
      if field.containing_oneof is not None:
        oneof_name = field.containing_oneof.name
        if oneof_name in names:
          raise ParseError('Message type "{0}" should not have multiple "{1}" '
                           'oneof fields.'.format(
                               message.DESCRIPTOR.full_name, oneof_name))
        names.append(oneof_name)

      value = js[name]
      if value is None:
        message.ClearField(field.name)
        continue

      # Parse field value.
      if _IsMapEntry(field):
        message.ClearField(field.name)
        _ConvertMapFieldValue(value, message, field)
      elif field.label == descriptor.FieldDescriptor.LABEL_REPEATED:
        message.ClearField(field.name)
        if not isinstance(value, list):
          raise ParseError('repeated field {0} must be in [] which is '
                           '{1}'.format(name, value))
        for item in value:
          if item is None:
            continue
          if field.cpp_type == descriptor.FieldDescriptor.CPPTYPE_MESSAGE:
            sub_message = getattr(message, field.name).add()
            _ConvertMessage(item, sub_message)
          else:
            getattr(message, field.name).append(
                _ConvertScalarFieldValue(item, field))
      elif field.cpp_type == descriptor.FieldDescriptor.CPPTYPE_MESSAGE:
        sub_message = getattr(message, field.name)
        _ConvertMessage(value, sub_message)
      else:
        setattr(message, field.name, _ConvertScalarFieldValue(value, field))
    except ParseError as e:
      if field and field.containing_oneof is None:
        raise ParseError('Failed to parse {0} field: {1}'.format(name, e))
      else:
        raise ParseError(str(e))
    except ValueError as e:
      raise ParseError('Failed to parse {0} field: {1}'.format(name, e))
    except TypeError as e:
      raise ParseError('Failed to parse {0} field: {1}'.format(name, e))


def _ConvertMessage(value, message):
  """Convert a JSON object into a message.

  Args:
    value: A JSON object.
    message: A WKT or regular protocol message to record the data.

  Raises:
    ParseError: In case of convert problems.
  """
  message_descriptor = message.DESCRIPTOR
  if _IsTimestampMessage(message_descriptor):
    _ConvertTimestampMessage(value, message)
  elif _IsDurationMessage(message_descriptor):
    _ConvertDurationMessage(value, message)
  elif _IsFieldMaskMessage(message_descriptor):
    _ConvertFieldMaskMessage(value, message)
  elif _IsWrapperMessage(message_descriptor):
    _ConvertWrapperMessage(value, message)
  else:
    _ConvertFieldValuePair(value, message)


def _ConvertTimestampMessage(value, message):
  """Convert a JSON representation into Timestamp message."""
  timezone_offset = value.find('Z')
  if timezone_offset == -1:
    timezone_offset = value.find('+')
  if timezone_offset == -1:
    timezone_offset = value.rfind('-')
  if timezone_offset == -1:
    raise ParseError(
        'Failed to parse timestamp: missing valid timezone offset.')
  time_value = value[0:timezone_offset]
  # Parse datetime and nanos
  point_position = time_value.find('.')
  if point_position == -1:
    second_value = time_value
    nano_value = ''
  else:
    second_value = time_value[:point_position]
    nano_value = time_value[point_position + 1:]
  date_object = datetime.strptime(second_value, _TIMESTAMPFOMAT)
  td = date_object - datetime(1970, 1, 1)
  seconds = td.seconds + td.days * 24 * 3600
  if len(nano_value) > 9:
    raise ParseError(
        'Failed to parse Timestamp: nanos {0} more than '
        '9 fractional digits.'.format(nano_value))
  if nano_value:
    nanos = round(float('0.' + nano_value) * 1e9)
  else:
    nanos = 0
  # Parse timezone offsets
  if value[timezone_offset] == 'Z':
    if len(value) != timezone_offset + 1:
      raise ParseError(
          'Failed to parse timestamp: invalid trailing data {0}.'.format(value))
  else:
    timezone = value[timezone_offset:]
    pos = timezone.find(':')
    if pos == -1:
      raise ParseError(
          'Invalid timezone offset value: ' + timezone)
    if timezone[0] == '+':
      seconds += (int(timezone[1:pos])*60+int(timezone[pos+1:]))*60
    else:
      seconds -= (int(timezone[1:pos])*60+int(timezone[pos+1:]))*60
  # Set seconds and nanos
  message.seconds = int(seconds)
  message.nanos = int(nanos)


def _ConvertDurationMessage(value, message):
  """Convert a JSON representation into Duration message."""
  if value[-1] != 's':
    raise ParseError(
        'Duration must end with letter "s": ' + value)
  try:
    duration = float(value[:-1])
  except ValueError:
    raise ParseError(
        'Couldn\'t parse duration: ' + value)
  message.seconds = int(duration)
  message.nanos = int(round((duration - message.seconds) * 1e9))


def _ConvertFieldMaskMessage(value, message):
  """Convert a JSON representation into FieldMask message."""
  for path in value.split(','):
    message.paths.append(path)


def _ConvertWrapperMessage(value, message):
  """Convert a JSON representation into Wrapper message."""
  field = message.DESCRIPTOR.fields_by_name['value']
  setattr(message, 'value', _ConvertScalarFieldValue(value, field))


def _ConvertMapFieldValue(value, message, field):
  """Convert map field value for a message map field.

  Args:
    value: A JSON object to convert the map field value.
    message: A protocol message to record the converted data.
    field: The descriptor of the map field to be converted.

  Raises:
    ParseError: In case of convert problems.
  """
  if not isinstance(value, dict):
    raise ParseError(
        'Map fieled {0} must be in {} which is {1}.'.format(field.name, value))
  key_field = field.message_type.fields_by_name['key']
  value_field = field.message_type.fields_by_name['value']
  for key in value:
    key_value = _ConvertScalarFieldValue(key, key_field, True)
    if value_field.cpp_type == descriptor.FieldDescriptor.CPPTYPE_MESSAGE:
      _ConvertMessage(value[key], getattr(message, field.name)[key_value])
    else:
      getattr(message, field.name)[key_value] = _ConvertScalarFieldValue(
          value[key], value_field)


def _ConvertScalarFieldValue(value, field, require_quote=False):
  """Convert a single scalar field value.

  Args:
    value: A scalar value to convert the scalar field value.
    field: The descriptor of the field to convert.
    require_quote: If True, '"' is required for the field value.

  Returns:
    The converted scalar field value

  Raises:
    ParseError: In case of convert problems.
  """
  if field.cpp_type in _INT_TYPES:
    return _ConvertInteger(value)
  elif field.cpp_type in _FLOAT_TYPES:
    return _ConvertFloat(value)
  elif field.cpp_type == descriptor.FieldDescriptor.CPPTYPE_BOOL:
    return _ConvertBool(value, require_quote)
  elif field.cpp_type == descriptor.FieldDescriptor.CPPTYPE_STRING:
    if field.type == descriptor.FieldDescriptor.TYPE_BYTES:
      return base64.b64decode(value)
    else:
      return value
  elif field.cpp_type == descriptor.FieldDescriptor.CPPTYPE_ENUM:
    # Convert an enum value.
    enum_value = field.enum_type.values_by_name.get(value, None)
    if enum_value is None:
      raise ParseError(
          'Enum value must be a string literal with double quotes. '
          'Type "{0}" has no value named {1}.'.format(
              field.enum_type.full_name, value))
    return enum_value.number


def _ConvertInteger(value):
  """Convert an integer.

  Args:
    value: A scalar value to convert.

  Returns:
    The integer value.

  Raises:
    ParseError: If an integer couldn't be consumed.
  """
  if isinstance(value, float):
    raise ParseError('Couldn\'t parse integer: {0}'.format(value))

  if isinstance(value, _UNICODETYPE) and not _INTEGER.match(value):
    raise ParseError('Couldn\'t parse integer: "{0}"'.format(value))

  return int(value)


def _ConvertFloat(value):
  """Convert an floating point number."""
  if value == 'nan':
    raise ParseError('Couldn\'t parse float "nan", use "NaN" instead')
  try:
    # Assume Python compatible syntax.
    return float(value)
  except ValueError:
    # Check alternative spellings.
    if value == '-Infinity':
      return float('-inf')
    elif value == 'Infinity':
      return float('inf')
    elif value == 'NaN':
      return float('nan')
    else:
      raise ParseError('Couldn\'t parse float: {0}'.format(value))


def _ConvertBool(value, require_quote):
  """Convert a boolean value.

  Args:
    value: A scalar value to convert.
    require_quote: If True, '"' is required for the boolean value.

  Returns:
    The bool parsed.

  Raises:
    ParseError: If a boolean value couldn't be consumed.
  """
  if require_quote:
    if value == 'true':
      return True
    elif value == 'false':
      return False
    else:
      raise ParseError('Expect "true" or "false", not {0}.'.format(value))

  if not isinstance(value, bool):
    raise ParseError('Expected true or false without quotes.')
  return value
