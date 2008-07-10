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

"""OutputStream is the primitive interface for sticking bits on the wire.

All protocol buffer serialization can be expressed in terms of
the OutputStream primitives provided here.
"""

__author__ = 'robinson@google.com (Will Robinson)'

import array
import struct
from google.protobuf import message
from google.protobuf.internal import wire_format



# Note that much of this code is ported from //net/proto/ProtocolBuffer, and
# that the interface is strongly inspired by CodedOutputStream from the C++
# proto2 implementation.


class OutputStream(object):

  """Contains all logic for writing bits, and ToString() to get the result."""

  def __init__(self):
    self._buffer = array.array('B')

  def AppendRawBytes(self, raw_bytes):
    """Appends raw_bytes to our internal buffer."""
    self._buffer.fromstring(raw_bytes)

  def AppendLittleEndian32(self, unsigned_value):
    """Appends an unsigned 32-bit integer to the internal buffer,
    in little-endian byte order.
    """
    if not 0 <= unsigned_value <= wire_format.UINT32_MAX:
      raise message.EncodeError(
          'Unsigned 32-bit out of range: %d' % unsigned_value)
    self._buffer.fromstring(struct.pack(
        wire_format.FORMAT_UINT32_LITTLE_ENDIAN, unsigned_value))

  def AppendLittleEndian64(self, unsigned_value):
    """Appends an unsigned 64-bit integer to the internal buffer,
    in little-endian byte order.
    """
    if not 0 <= unsigned_value <= wire_format.UINT64_MAX:
      raise message.EncodeError(
          'Unsigned 64-bit out of range: %d' % unsigned_value)
    self._buffer.fromstring(struct.pack(
        wire_format.FORMAT_UINT64_LITTLE_ENDIAN, unsigned_value))

  def AppendVarint32(self, value):
    """Appends a signed 32-bit integer to the internal buffer,
    encoded as a varint.  (Note that a negative varint32 will
    always require 10 bytes of space.)
    """
    if not wire_format.INT32_MIN <= value <= wire_format.INT32_MAX:
      raise message.EncodeError('Value out of range: %d' % value)
    self.AppendVarint64(value)

  def AppendVarUInt32(self, value):
    """Appends an unsigned 32-bit integer to the internal buffer,
    encoded as a varint.
    """
    if not 0 <= value <= wire_format.UINT32_MAX:
      raise message.EncodeError('Value out of range: %d' % value)
    self.AppendVarUInt64(value)

  def AppendVarint64(self, value):
    """Appends a signed 64-bit integer to the internal buffer,
    encoded as a varint.
    """
    if not wire_format.INT64_MIN <= value <= wire_format.INT64_MAX:
      raise message.EncodeError('Value out of range: %d' % value)
    if value < 0:
      value += (1 << 64)
    self.AppendVarUInt64(value)

  def AppendVarUInt64(self, unsigned_value):
    """Appends an unsigned 64-bit integer to the internal buffer,
    encoded as a varint.
    """
    if not 0 <= unsigned_value <= wire_format.UINT64_MAX:
      raise message.EncodeError('Value out of range: %d' % unsigned_value)
    while True:
      bits = unsigned_value & 0x7f
      unsigned_value >>= 7
      if unsigned_value:
        bits |= 0x80
      self._buffer.append(bits)
      if not unsigned_value:
        break

  def ToString(self):
    """Returns a string containing the bytes in our internal buffer."""
    return self._buffer.tostring()
