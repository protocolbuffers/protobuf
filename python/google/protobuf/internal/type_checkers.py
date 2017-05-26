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

"""Provides type checking routines.

This module defines type checking utilities in the forms of dictionaries:

VALUE_CHECKERS: A dictionary of field types and a value validation object.
TYPE_TO_BYTE_SIZE_FN: A dictionary with field types and a size computing
  function.
TYPE_TO_SERIALIZE_METHOD: A dictionary with field types and serialization
  function.
FIELD_TYPE_TO_WIRE_TYPE: A dictionary with field typed and their
  coresponding wire types.
TYPE_TO_DESERIALIZE_METHOD: A dictionary with field types and deserialization
  function.
"""

__author__ = 'robinson@google.com (Will Robinson)'

import numbers
import six

if six.PY3:
  long = int

from google.protobuf.internal import api_implementation
from google.protobuf.internal import decoder
from google.protobuf.internal import encoder
from google.protobuf.internal import wire_format
from google.protobuf import descriptor

_FieldDescriptor = descriptor.FieldDescriptor

def SupportsOpenEnums(field_descriptor):
  return field_descriptor.containing_type.syntax == "proto3"

def GetTypeChecker(field):
  """Returns a type checker for a message field of the specified types.

  Args:
    field: FieldDescriptor object for this field.

  Returns:
    An instance of TypeChecker which can be used to verify the types
    of values assigned to a field of the specified type.
  """
  if (field.cpp_type == _FieldDescriptor.CPPTYPE_STRING and
      field.type == _FieldDescriptor.TYPE_STRING):
    return UnicodeValueChecker()
  if field.cpp_type == _FieldDescriptor.CPPTYPE_ENUM:
    if SupportsOpenEnums(field):
      # When open enums are supported, any int32 can be assigned.
      return _VALUE_CHECKERS[_FieldDescriptor.CPPTYPE_INT32]
    else:
      return EnumValueChecker(field.enum_type)
  return _VALUE_CHECKERS[field.cpp_type]


# None of the typecheckers below make any attempt to guard against people
# subclassing builtin types and doing weird things.  We're not trying to
# protect against malicious clients here, just people accidentally shooting
# themselves in the foot in obvious ways.

class TypeChecker(object):

  """Type checker used to catch type errors as early as possible
  when the client is setting scalar fields in protocol messages.
  """

  def __init__(self, *acceptable_types):
    self._acceptable_types = acceptable_types

  def CheckValue(self, proposed_value):
    """Type check the provided value and return it.

    The returned value might have been normalized to another type.
    """
    if not isinstance(proposed_value, self._acceptable_types):
      message = ('%.1024r has type %s, but expected one of: %s' %
                 (proposed_value, type(proposed_value), self._acceptable_types))
      raise TypeError(message)
    return proposed_value


class TypeCheckerWithDefault(TypeChecker):

  def __init__(self, default_value, *acceptable_types):
    TypeChecker.__init__(self, acceptable_types)
    self._default_value = default_value

  def DefaultValue(self):
    return self._default_value


# IntValueChecker and its subclasses perform integer type-checks
# and bounds-checks.
class IntValueChecker(object):

  """Checker used for integer fields.  Performs type-check and range check."""

  def CheckValue(self, proposed_value):
    if not isinstance(proposed_value, numbers.Integral):
      message = ('%.1024r has type %s, but expected one of: %s' %
                 (proposed_value, type(proposed_value), six.integer_types))
      raise TypeError(message)
    if not self._MIN <= int(proposed_value) <= self._MAX:
      raise ValueError('Value out of range: %d' % proposed_value)
    # We force 32-bit values to int and 64-bit values to long to make
    # alternate implementations where the distinction is more significant
    # (e.g. the C++ implementation) simpler.
    proposed_value = self._TYPE(proposed_value)
    return proposed_value

  def DefaultValue(self):
    return 0


class EnumValueChecker(object):

  """Checker used for enum fields.  Performs type-check and range check."""

  def __init__(self, enum_type):
    self._enum_type = enum_type

  def CheckValue(self, proposed_value):
    if not isinstance(proposed_value, numbers.Integral):
      message = ('%.1024r has type %s, but expected one of: %s' %
                 (proposed_value, type(proposed_value), six.integer_types))
      raise TypeError(message)
    if int(proposed_value) not in self._enum_type.values_by_number:
      raise ValueError('Unknown enum value: %d' % proposed_value)
    return proposed_value

  def DefaultValue(self):
    return self._enum_type.values[0].number


class UnicodeValueChecker(object):

  """Checker used for string fields.

  Always returns a unicode value, even if the input is of type str.
  """

  def CheckValue(self, proposed_value):
    if not isinstance(proposed_value, (bytes, six.text_type)):
      message = ('%.1024r has type %s, but expected one of: %s' %
                 (proposed_value, type(proposed_value), (bytes, six.text_type)))
      raise TypeError(message)

    # If the value is of type 'bytes' make sure that it is valid UTF-8 data.
    if isinstance(proposed_value, bytes):
      try:
        proposed_value = proposed_value.decode('utf-8')
      except UnicodeDecodeError:
        raise ValueError('%.1024r has type bytes, but isn\'t valid UTF-8 '
                         'encoding. Non-UTF-8 strings must be converted to '
                         'unicode objects before being added.' %
                         (proposed_value))
    return proposed_value

  def DefaultValue(self):
    return u""


class Int32ValueChecker(IntValueChecker):
  # We're sure to use ints instead of longs here since comparison may be more
  # efficient.
  _MIN = -2147483648
  _MAX = 2147483647
  _TYPE = int


class Uint32ValueChecker(IntValueChecker):
  _MIN = 0
  _MAX = (1 << 32) - 1
  _TYPE = int


class Int64ValueChecker(IntValueChecker):
  _MIN = -(1 << 63)
  _MAX = (1 << 63) - 1
  _TYPE = long


class Uint64ValueChecker(IntValueChecker):
  _MIN = 0
  _MAX = (1 << 64) - 1
  _TYPE = long


# Type-checkers for all scalar CPPTYPEs.
_VALUE_CHECKERS = {
    _FieldDescriptor.CPPTYPE_INT32: Int32ValueChecker(),
    _FieldDescriptor.CPPTYPE_INT64: Int64ValueChecker(),
    _FieldDescriptor.CPPTYPE_UINT32: Uint32ValueChecker(),
    _FieldDescriptor.CPPTYPE_UINT64: Uint64ValueChecker(),
    _FieldDescriptor.CPPTYPE_DOUBLE: TypeCheckerWithDefault(
        0.0, numbers.Real),
    _FieldDescriptor.CPPTYPE_FLOAT: TypeCheckerWithDefault(
        0.0, numbers.Real),
    _FieldDescriptor.CPPTYPE_BOOL: TypeCheckerWithDefault(
        False, bool, numbers.Integral),
    _FieldDescriptor.CPPTYPE_STRING: TypeCheckerWithDefault(b'', bytes),
    }


# Map from field type to a function F, such that F(field_num, value)
# gives the total byte size for a value of the given type.  This
# byte size includes tag information and any other additional space
# associated with serializing "value".
TYPE_TO_BYTE_SIZE_FN = {
    _FieldDescriptor.TYPE_DOUBLE: wire_format.DoubleByteSize,
    _FieldDescriptor.TYPE_FLOAT: wire_format.FloatByteSize,
    _FieldDescriptor.TYPE_INT64: wire_format.Int64ByteSize,
    _FieldDescriptor.TYPE_UINT64: wire_format.UInt64ByteSize,
    _FieldDescriptor.TYPE_INT32: wire_format.Int32ByteSize,
    _FieldDescriptor.TYPE_FIXED64: wire_format.Fixed64ByteSize,
    _FieldDescriptor.TYPE_FIXED32: wire_format.Fixed32ByteSize,
    _FieldDescriptor.TYPE_BOOL: wire_format.BoolByteSize,
    _FieldDescriptor.TYPE_STRING: wire_format.StringByteSize,
    _FieldDescriptor.TYPE_GROUP: wire_format.GroupByteSize,
    _FieldDescriptor.TYPE_MESSAGE: wire_format.MessageByteSize,
    _FieldDescriptor.TYPE_BYTES: wire_format.BytesByteSize,
    _FieldDescriptor.TYPE_UINT32: wire_format.UInt32ByteSize,
    _FieldDescriptor.TYPE_ENUM: wire_format.EnumByteSize,
    _FieldDescriptor.TYPE_SFIXED32: wire_format.SFixed32ByteSize,
    _FieldDescriptor.TYPE_SFIXED64: wire_format.SFixed64ByteSize,
    _FieldDescriptor.TYPE_SINT32: wire_format.SInt32ByteSize,
    _FieldDescriptor.TYPE_SINT64: wire_format.SInt64ByteSize
    }


# Maps from field types to encoder constructors.
TYPE_TO_ENCODER = {
    _FieldDescriptor.TYPE_DOUBLE: encoder.DoubleEncoder,
    _FieldDescriptor.TYPE_FLOAT: encoder.FloatEncoder,
    _FieldDescriptor.TYPE_INT64: encoder.Int64Encoder,
    _FieldDescriptor.TYPE_UINT64: encoder.UInt64Encoder,
    _FieldDescriptor.TYPE_INT32: encoder.Int32Encoder,
    _FieldDescriptor.TYPE_FIXED64: encoder.Fixed64Encoder,
    _FieldDescriptor.TYPE_FIXED32: encoder.Fixed32Encoder,
    _FieldDescriptor.TYPE_BOOL: encoder.BoolEncoder,
    _FieldDescriptor.TYPE_STRING: encoder.StringEncoder,
    _FieldDescriptor.TYPE_GROUP: encoder.GroupEncoder,
    _FieldDescriptor.TYPE_MESSAGE: encoder.MessageEncoder,
    _FieldDescriptor.TYPE_BYTES: encoder.BytesEncoder,
    _FieldDescriptor.TYPE_UINT32: encoder.UInt32Encoder,
    _FieldDescriptor.TYPE_ENUM: encoder.EnumEncoder,
    _FieldDescriptor.TYPE_SFIXED32: encoder.SFixed32Encoder,
    _FieldDescriptor.TYPE_SFIXED64: encoder.SFixed64Encoder,
    _FieldDescriptor.TYPE_SINT32: encoder.SInt32Encoder,
    _FieldDescriptor.TYPE_SINT64: encoder.SInt64Encoder,
    }


# Maps from field types to sizer constructors.
TYPE_TO_SIZER = {
    _FieldDescriptor.TYPE_DOUBLE: encoder.DoubleSizer,
    _FieldDescriptor.TYPE_FLOAT: encoder.FloatSizer,
    _FieldDescriptor.TYPE_INT64: encoder.Int64Sizer,
    _FieldDescriptor.TYPE_UINT64: encoder.UInt64Sizer,
    _FieldDescriptor.TYPE_INT32: encoder.Int32Sizer,
    _FieldDescriptor.TYPE_FIXED64: encoder.Fixed64Sizer,
    _FieldDescriptor.TYPE_FIXED32: encoder.Fixed32Sizer,
    _FieldDescriptor.TYPE_BOOL: encoder.BoolSizer,
    _FieldDescriptor.TYPE_STRING: encoder.StringSizer,
    _FieldDescriptor.TYPE_GROUP: encoder.GroupSizer,
    _FieldDescriptor.TYPE_MESSAGE: encoder.MessageSizer,
    _FieldDescriptor.TYPE_BYTES: encoder.BytesSizer,
    _FieldDescriptor.TYPE_UINT32: encoder.UInt32Sizer,
    _FieldDescriptor.TYPE_ENUM: encoder.EnumSizer,
    _FieldDescriptor.TYPE_SFIXED32: encoder.SFixed32Sizer,
    _FieldDescriptor.TYPE_SFIXED64: encoder.SFixed64Sizer,
    _FieldDescriptor.TYPE_SINT32: encoder.SInt32Sizer,
    _FieldDescriptor.TYPE_SINT64: encoder.SInt64Sizer,
    }


# Maps from field type to a decoder constructor.
TYPE_TO_DECODER = {
    _FieldDescriptor.TYPE_DOUBLE: decoder.DoubleDecoder,
    _FieldDescriptor.TYPE_FLOAT: decoder.FloatDecoder,
    _FieldDescriptor.TYPE_INT64: decoder.Int64Decoder,
    _FieldDescriptor.TYPE_UINT64: decoder.UInt64Decoder,
    _FieldDescriptor.TYPE_INT32: decoder.Int32Decoder,
    _FieldDescriptor.TYPE_FIXED64: decoder.Fixed64Decoder,
    _FieldDescriptor.TYPE_FIXED32: decoder.Fixed32Decoder,
    _FieldDescriptor.TYPE_BOOL: decoder.BoolDecoder,
    _FieldDescriptor.TYPE_STRING: decoder.StringDecoder,
    _FieldDescriptor.TYPE_GROUP: decoder.GroupDecoder,
    _FieldDescriptor.TYPE_MESSAGE: decoder.MessageDecoder,
    _FieldDescriptor.TYPE_BYTES: decoder.BytesDecoder,
    _FieldDescriptor.TYPE_UINT32: decoder.UInt32Decoder,
    _FieldDescriptor.TYPE_ENUM: decoder.EnumDecoder,
    _FieldDescriptor.TYPE_SFIXED32: decoder.SFixed32Decoder,
    _FieldDescriptor.TYPE_SFIXED64: decoder.SFixed64Decoder,
    _FieldDescriptor.TYPE_SINT32: decoder.SInt32Decoder,
    _FieldDescriptor.TYPE_SINT64: decoder.SInt64Decoder,
    }

# Maps from field type to expected wiretype.
FIELD_TYPE_TO_WIRE_TYPE = {
    _FieldDescriptor.TYPE_DOUBLE: wire_format.WIRETYPE_FIXED64,
    _FieldDescriptor.TYPE_FLOAT: wire_format.WIRETYPE_FIXED32,
    _FieldDescriptor.TYPE_INT64: wire_format.WIRETYPE_VARINT,
    _FieldDescriptor.TYPE_UINT64: wire_format.WIRETYPE_VARINT,
    _FieldDescriptor.TYPE_INT32: wire_format.WIRETYPE_VARINT,
    _FieldDescriptor.TYPE_FIXED64: wire_format.WIRETYPE_FIXED64,
    _FieldDescriptor.TYPE_FIXED32: wire_format.WIRETYPE_FIXED32,
    _FieldDescriptor.TYPE_BOOL: wire_format.WIRETYPE_VARINT,
    _FieldDescriptor.TYPE_STRING:
      wire_format.WIRETYPE_LENGTH_DELIMITED,
    _FieldDescriptor.TYPE_GROUP: wire_format.WIRETYPE_START_GROUP,
    _FieldDescriptor.TYPE_MESSAGE:
      wire_format.WIRETYPE_LENGTH_DELIMITED,
    _FieldDescriptor.TYPE_BYTES:
      wire_format.WIRETYPE_LENGTH_DELIMITED,
    _FieldDescriptor.TYPE_UINT32: wire_format.WIRETYPE_VARINT,
    _FieldDescriptor.TYPE_ENUM: wire_format.WIRETYPE_VARINT,
    _FieldDescriptor.TYPE_SFIXED32: wire_format.WIRETYPE_FIXED32,
    _FieldDescriptor.TYPE_SFIXED64: wire_format.WIRETYPE_FIXED64,
    _FieldDescriptor.TYPE_SINT32: wire_format.WIRETYPE_VARINT,
    _FieldDescriptor.TYPE_SINT64: wire_format.WIRETYPE_VARINT,
    }
