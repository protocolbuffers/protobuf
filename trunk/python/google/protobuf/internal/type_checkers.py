# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.
# http://code.google.com/p/protobuf/
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

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

from google.protobuf.internal import decoder
from google.protobuf.internal import encoder
from google.protobuf.internal import wire_format
from google.protobuf import descriptor

_FieldDescriptor = descriptor.FieldDescriptor

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
    if not isinstance(proposed_value, self._acceptable_types):
      message = ('%.1024r has type %s, but expected one of: %s' %
                 (proposed_value, type(proposed_value), self._acceptable_types))
      raise TypeError(message)


# IntValueChecker and its subclasses perform integer type-checks
# and bounds-checks.
class IntValueChecker(object):

  """Checker used for integer fields.  Performs type-check and range check."""

  def CheckValue(self, proposed_value):
    if not isinstance(proposed_value, (int, long)):
      message = ('%.1024r has type %s, but expected one of: %s' %
                 (proposed_value, type(proposed_value), (int, long)))
      raise TypeError(message)
    if not self._MIN <= proposed_value <= self._MAX:
      raise ValueError('Value out of range: %d' % proposed_value)


class Int32ValueChecker(IntValueChecker):
  # We're sure to use ints instead of longs here since comparison may be more
  # efficient.
  _MIN = -2147483648
  _MAX = 2147483647


class Uint32ValueChecker(IntValueChecker):
  _MIN = 0
  _MAX = (1 << 32) - 1


class Int64ValueChecker(IntValueChecker):
  _MIN = -(1 << 63)
  _MAX = (1 << 63) - 1


class Uint64ValueChecker(IntValueChecker):
  _MIN = 0
  _MAX = (1 << 64) - 1


# Type-checkers for all scalar CPPTYPEs.
VALUE_CHECKERS = {
    _FieldDescriptor.CPPTYPE_INT32: Int32ValueChecker(),
    _FieldDescriptor.CPPTYPE_INT64: Int64ValueChecker(),
    _FieldDescriptor.CPPTYPE_UINT32: Uint32ValueChecker(),
    _FieldDescriptor.CPPTYPE_UINT64: Uint64ValueChecker(),
    _FieldDescriptor.CPPTYPE_DOUBLE: TypeChecker(
        float, int, long),
    _FieldDescriptor.CPPTYPE_FLOAT: TypeChecker(
        float, int, long),
    _FieldDescriptor.CPPTYPE_BOOL: TypeChecker(bool, int),
    _FieldDescriptor.CPPTYPE_ENUM: Int32ValueChecker(),
    _FieldDescriptor.CPPTYPE_STRING: TypeChecker(str),
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


# Maps from field type to an unbound Encoder method F, such that
# F(encoder, field_number, value) will append the serialization
# of a value of this type to the encoder.
_Encoder = encoder.Encoder
TYPE_TO_SERIALIZE_METHOD = {
    _FieldDescriptor.TYPE_DOUBLE: _Encoder.AppendDouble,
    _FieldDescriptor.TYPE_FLOAT: _Encoder.AppendFloat,
    _FieldDescriptor.TYPE_INT64: _Encoder.AppendInt64,
    _FieldDescriptor.TYPE_UINT64: _Encoder.AppendUInt64,
    _FieldDescriptor.TYPE_INT32: _Encoder.AppendInt32,
    _FieldDescriptor.TYPE_FIXED64: _Encoder.AppendFixed64,
    _FieldDescriptor.TYPE_FIXED32: _Encoder.AppendFixed32,
    _FieldDescriptor.TYPE_BOOL: _Encoder.AppendBool,
    _FieldDescriptor.TYPE_STRING: _Encoder.AppendString,
    _FieldDescriptor.TYPE_GROUP: _Encoder.AppendGroup,
    _FieldDescriptor.TYPE_MESSAGE: _Encoder.AppendMessage,
    _FieldDescriptor.TYPE_BYTES: _Encoder.AppendBytes,
    _FieldDescriptor.TYPE_UINT32: _Encoder.AppendUInt32,
    _FieldDescriptor.TYPE_ENUM: _Encoder.AppendEnum,
    _FieldDescriptor.TYPE_SFIXED32: _Encoder.AppendSFixed32,
    _FieldDescriptor.TYPE_SFIXED64: _Encoder.AppendSFixed64,
    _FieldDescriptor.TYPE_SINT32: _Encoder.AppendSInt32,
    _FieldDescriptor.TYPE_SINT64: _Encoder.AppendSInt64,
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


# Maps from field type to an unbound Decoder method F,
# such that F(decoder) will read a field of the requested type.
#
# Note that Message and Group are intentionally missing here.
# They're handled by _RecursivelyMerge().
_Decoder = decoder.Decoder
TYPE_TO_DESERIALIZE_METHOD = {
    _FieldDescriptor.TYPE_DOUBLE: _Decoder.ReadDouble,
    _FieldDescriptor.TYPE_FLOAT: _Decoder.ReadFloat,
    _FieldDescriptor.TYPE_INT64: _Decoder.ReadInt64,
    _FieldDescriptor.TYPE_UINT64: _Decoder.ReadUInt64,
    _FieldDescriptor.TYPE_INT32: _Decoder.ReadInt32,
    _FieldDescriptor.TYPE_FIXED64: _Decoder.ReadFixed64,
    _FieldDescriptor.TYPE_FIXED32: _Decoder.ReadFixed32,
    _FieldDescriptor.TYPE_BOOL: _Decoder.ReadBool,
    _FieldDescriptor.TYPE_STRING: _Decoder.ReadString,
    _FieldDescriptor.TYPE_BYTES: _Decoder.ReadBytes,
    _FieldDescriptor.TYPE_UINT32: _Decoder.ReadUInt32,
    _FieldDescriptor.TYPE_ENUM: _Decoder.ReadEnum,
    _FieldDescriptor.TYPE_SFIXED32: _Decoder.ReadSFixed32,
    _FieldDescriptor.TYPE_SFIXED64: _Decoder.ReadSFixed64,
    _FieldDescriptor.TYPE_SINT32: _Decoder.ReadSInt32,
    _FieldDescriptor.TYPE_SINT64: _Decoder.ReadSInt64,
    }
