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
      if not unsigned_value:
        self._buffer.append(bits)
        break
      self._buffer.append(0x80|bits)

  def ToString(self):
    """Returns a string containing the bytes in our internal buffer."""
    return self._buffer.tostring()
