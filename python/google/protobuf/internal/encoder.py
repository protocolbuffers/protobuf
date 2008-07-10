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

"""Class for encoding protocol message primitives.

Contains the logic for encoding every logical protocol field type
into one of the 5 physical wire types.
"""

__author__ = 'robinson@google.com (Will Robinson)'

import struct
from google.protobuf import message
from google.protobuf.internal import wire_format
from google.protobuf.internal import output_stream


# Note that much of this code is ported from //net/proto/ProtocolBuffer, and
# that the interface is strongly inspired by WireFormat from the C++ proto2
# implementation.


class Encoder(object):

  """Encodes logical protocol buffer fields to the wire format."""

  def __init__(self):
    self._stream = output_stream.OutputStream()

  def ToString(self):
    """Returns all values encoded in this object as a string."""
    return self._stream.ToString()

  # All the Append*() methods below first append a tag+type pair to the buffer
  # before appending the specified value.

  def AppendInt32(self, field_number, value):
    """Appends a 32-bit integer to our buffer, varint-encoded."""
    self._AppendTag(field_number, wire_format.WIRETYPE_VARINT)
    self._stream.AppendVarint32(value)

  def AppendInt64(self, field_number, value):
    """Appends a 64-bit integer to our buffer, varint-encoded."""
    self._AppendTag(field_number, wire_format.WIRETYPE_VARINT)
    self._stream.AppendVarint64(value)

  def AppendUInt32(self, field_number, unsigned_value):
    """Appends an unsigned 32-bit integer to our buffer, varint-encoded."""
    self._AppendTag(field_number, wire_format.WIRETYPE_VARINT)
    self._stream.AppendVarUInt32(unsigned_value)

  def AppendUInt64(self, field_number, unsigned_value):
    """Appends an unsigned 64-bit integer to our buffer, varint-encoded."""
    self._AppendTag(field_number, wire_format.WIRETYPE_VARINT)
    self._stream.AppendVarUInt64(unsigned_value)

  def AppendSInt32(self, field_number, value):
    """Appends a 32-bit integer to our buffer, zigzag-encoded and then
    varint-encoded.
    """
    self._AppendTag(field_number, wire_format.WIRETYPE_VARINT)
    zigzag_value = wire_format.ZigZagEncode(value)
    self._stream.AppendVarUInt32(zigzag_value)

  def AppendSInt64(self, field_number, value):
    """Appends a 64-bit integer to our buffer, zigzag-encoded and then
    varint-encoded.
    """
    self._AppendTag(field_number, wire_format.WIRETYPE_VARINT)
    zigzag_value = wire_format.ZigZagEncode(value)
    self._stream.AppendVarUInt64(zigzag_value)

  def AppendFixed32(self, field_number, unsigned_value):
    """Appends an unsigned 32-bit integer to our buffer, in little-endian
    byte-order.
    """
    self._AppendTag(field_number, wire_format.WIRETYPE_FIXED32)
    self._stream.AppendLittleEndian32(unsigned_value)

  def AppendFixed64(self, field_number, unsigned_value):
    """Appends an unsigned 64-bit integer to our buffer, in little-endian
    byte-order.
    """
    self._AppendTag(field_number, wire_format.WIRETYPE_FIXED64)
    self._stream.AppendLittleEndian64(unsigned_value)

  def AppendSFixed32(self, field_number, value):
    """Appends a signed 32-bit integer to our buffer, in little-endian
    byte-order.
    """
    sign = (value & 0x80000000) and -1 or 0
    if value >> 32 != sign:
      raise message.EncodeError('SFixed32 out of range: %d' % value)
    self._AppendTag(field_number, wire_format.WIRETYPE_FIXED32)
    self._stream.AppendLittleEndian32(value & 0xffffffff)

  def AppendSFixed64(self, field_number, value):
    """Appends a signed 64-bit integer to our buffer, in little-endian
    byte-order.
    """
    sign = (value & 0x8000000000000000) and -1 or 0
    if value >> 64 != sign:
      raise message.EncodeError('SFixed64 out of range: %d' % value)
    self._AppendTag(field_number, wire_format.WIRETYPE_FIXED64)
    self._stream.AppendLittleEndian64(value & 0xffffffffffffffff)

  def AppendFloat(self, field_number, value):
    """Appends a floating-point number to our buffer."""
    self._AppendTag(field_number, wire_format.WIRETYPE_FIXED32)
    self._stream.AppendRawBytes(struct.pack('f', value))

  def AppendDouble(self, field_number, value):
    """Appends a double-precision floating-point number to our buffer."""
    self._AppendTag(field_number, wire_format.WIRETYPE_FIXED64)
    self._stream.AppendRawBytes(struct.pack('d', value))

  def AppendBool(self, field_number, value):
    """Appends a boolean to our buffer."""
    self.AppendInt32(field_number, value)

  def AppendEnum(self, field_number, value):
    """Appends an enum value to our buffer."""
    self.AppendInt32(field_number, value)

  def AppendString(self, field_number, value):
    """Appends a length-prefixed string to our buffer, with the
    length varint-encoded.
    """
    self._AppendTag(field_number, wire_format.WIRETYPE_LENGTH_DELIMITED)
    self._stream.AppendVarUInt32(len(value))
    self._stream.AppendRawBytes(value)

  def AppendBytes(self, field_number, value):
    """Appends a length-prefixed sequence of bytes to our buffer, with the
    length varint-encoded.
    """
    self.AppendString(field_number, value)

  # TODO(robinson): For AppendGroup() and AppendMessage(), we'd really like to
  # avoid the extra string copy here.  We can do so if we widen the Message
  # interface to be able to serialize to a stream in addition to a string.  The
  # challenge when thinking ahead to the Python/C API implementation of Message
  # is finding a stream-like Python thing to which we can write raw bytes
  # from C.  I'm not sure such a thing exists(?).  (array.array is pretty much
  # what we want, but it's not directly exposed in the Python/C API).

  def AppendGroup(self, field_number, group):
    """Appends a group to our buffer.
    """
    self._AppendTag(field_number, wire_format.WIRETYPE_START_GROUP)
    self._stream.AppendRawBytes(group.SerializeToString())
    self._AppendTag(field_number, wire_format.WIRETYPE_END_GROUP)

  def AppendMessage(self, field_number, msg):
    """Appends a nested message to our buffer.
    """
    self._AppendTag(field_number, wire_format.WIRETYPE_LENGTH_DELIMITED)
    self._stream.AppendVarUInt32(msg.ByteSize())
    self._stream.AppendRawBytes(msg.SerializeToString())

  def AppendMessageSetItem(self, field_number, msg):
    """Appends an item using the message set wire format.

    The message set message looks like this:
      message MessageSet {
        repeated group Item = 1 {
          required int32 type_id = 2;
          required string message = 3;
        }
      }
    """
    self._AppendTag(1, wire_format.WIRETYPE_START_GROUP)
    self.AppendInt32(2, field_number)
    self.AppendMessage(3, msg)
    self._AppendTag(1, wire_format.WIRETYPE_END_GROUP)

  def _AppendTag(self, field_number, wire_type):
    """Appends a tag containing field number and wire type information."""
    self._stream.AppendVarUInt32(wire_format.PackTag(field_number, wire_type))
