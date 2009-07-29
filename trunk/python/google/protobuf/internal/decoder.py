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

"""Class for decoding protocol buffer primitives.

Contains the logic for decoding every logical protocol field type
from one of the 5 physical wire types.
"""

__author__ = 'robinson@google.com (Will Robinson)'

import struct
from google.protobuf import message
from google.protobuf.internal import input_stream
from google.protobuf.internal import wire_format



# Note that much of this code is ported from //net/proto/ProtocolBuffer, and
# that the interface is strongly inspired by WireFormat from the C++ proto2
# implementation.


class Decoder(object):

  """Decodes logical protocol buffer fields from the wire."""

  def __init__(self, s):
    """Initializes the decoder to read from s.

    Args:
      s: An immutable sequence of bytes, which must be accessible
        via the Python buffer() primitive (i.e., buffer(s)).
    """
    self._stream = input_stream.InputStream(s)

  def EndOfStream(self):
    """Returns true iff we've reached the end of the bytes we're reading."""
    return self._stream.EndOfStream()

  def Position(self):
    """Returns the 0-indexed position in |s|."""
    return self._stream.Position()

  def ReadFieldNumberAndWireType(self):
    """Reads a tag from the wire. Returns a (field_number, wire_type) pair."""
    tag_and_type = self.ReadUInt32()
    return wire_format.UnpackTag(tag_and_type)

  def SkipBytes(self, bytes):
    """Skips the specified number of bytes on the wire."""
    self._stream.SkipBytes(bytes)

  # Note that the Read*() methods below are not exactly symmetrical with the
  # corresponding Encoder.Append*() methods.  Those Encoder methods first
  # encode a tag, but the Read*() methods below assume that the tag has already
  # been read, and that the client wishes to read a field of the specified type
  # starting at the current position.

  def ReadInt32(self):
    """Reads and returns a signed, varint-encoded, 32-bit integer."""
    return self._stream.ReadVarint32()

  def ReadInt64(self):
    """Reads and returns a signed, varint-encoded, 64-bit integer."""
    return self._stream.ReadVarint64()

  def ReadUInt32(self):
    """Reads and returns an signed, varint-encoded, 32-bit integer."""
    return self._stream.ReadVarUInt32()

  def ReadUInt64(self):
    """Reads and returns an signed, varint-encoded,64-bit integer."""
    return self._stream.ReadVarUInt64()

  def ReadSInt32(self):
    """Reads and returns a signed, zigzag-encoded, varint-encoded,
    32-bit integer."""
    return wire_format.ZigZagDecode(self._stream.ReadVarUInt32())

  def ReadSInt64(self):
    """Reads and returns a signed, zigzag-encoded, varint-encoded,
    64-bit integer."""
    return wire_format.ZigZagDecode(self._stream.ReadVarUInt64())

  def ReadFixed32(self):
    """Reads and returns an unsigned, fixed-width, 32-bit integer."""
    return self._stream.ReadLittleEndian32()

  def ReadFixed64(self):
    """Reads and returns an unsigned, fixed-width, 64-bit integer."""
    return self._stream.ReadLittleEndian64()

  def ReadSFixed32(self):
    """Reads and returns a signed, fixed-width, 32-bit integer."""
    value = self._stream.ReadLittleEndian32()
    if value >= (1 << 31):
      value -= (1 << 32)
    return value

  def ReadSFixed64(self):
    """Reads and returns a signed, fixed-width, 64-bit integer."""
    value = self._stream.ReadLittleEndian64()
    if value >= (1 << 63):
      value -= (1 << 64)
    return value

  def ReadFloat(self):
    """Reads and returns a 4-byte floating-point number."""
    serialized = self._stream.ReadBytes(4)
    return struct.unpack(wire_format.FORMAT_FLOAT_LITTLE_ENDIAN, serialized)[0]

  def ReadDouble(self):
    """Reads and returns an 8-byte floating-point number."""
    serialized = self._stream.ReadBytes(8)
    return struct.unpack(wire_format.FORMAT_DOUBLE_LITTLE_ENDIAN, serialized)[0]

  def ReadBool(self):
    """Reads and returns a bool."""
    i = self._stream.ReadVarUInt32()
    return bool(i)

  def ReadEnum(self):
    """Reads and returns an enum value."""
    return self._stream.ReadVarUInt32()

  def ReadString(self):
    """Reads and returns a length-delimited string."""
    bytes = self.ReadBytes()
    return unicode(bytes, 'utf-8')

  def ReadBytes(self):
    """Reads and returns a length-delimited byte sequence."""
    length = self._stream.ReadVarUInt32()
    return self._stream.ReadBytes(length)

  def ReadMessageInto(self, msg):
    """Calls msg.MergeFromString() to merge
    length-delimited serialized message data into |msg|.

    REQUIRES: The decoder must be positioned at the serialized "length"
      prefix to a length-delmiited serialized message.

    POSTCONDITION: The decoder is positioned just after the
      serialized message, and we have merged those serialized
      contents into |msg|.
    """
    length = self._stream.ReadVarUInt32()
    sub_buffer = self._stream.GetSubBuffer(length)
    num_bytes_used = msg.MergeFromString(sub_buffer)
    if num_bytes_used != length:
      raise message.DecodeError(
          'Submessage told to deserialize from %d-byte encoding, '
          'but used only %d bytes' % (length, num_bytes_used))
    self._stream.SkipBytes(num_bytes_used)

  def ReadGroupInto(self, expected_field_number, group):
    """Calls group.MergeFromString() to merge
    END_GROUP-delimited serialized message data into |group|.
    We'll raise an exception if we don't find an END_GROUP
    tag immediately after the serialized message contents.

    REQUIRES: The decoder is positioned just after the START_GROUP
      tag for this group.

    POSTCONDITION: The decoder is positioned just after the
      END_GROUP tag for this group, and we have merged
      the contents of the group into |group|.
    """
    sub_buffer = self._stream.GetSubBuffer()  # No a priori length limit.
    num_bytes_used = group.MergeFromString(sub_buffer)
    if num_bytes_used < 0:
      raise message.DecodeError('Group message reported negative bytes read.')
    self._stream.SkipBytes(num_bytes_used)
    field_number, field_type = self.ReadFieldNumberAndWireType()
    if field_type != wire_format.WIRETYPE_END_GROUP:
      raise message.DecodeError('Group message did not end with an END_GROUP.')
    if field_number != expected_field_number:
      raise message.DecodeError('END_GROUP tag had field '
                                'number %d, was expecting field number %d' % (
          field_number, expected_field_number))
    # We're now positioned just after the END_GROUP tag.  Perfect.
