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

"""InputStream is the primitive interface for reading bits from the wire.

All protocol buffer deserialization can be expressed in terms of
the InputStream primitives provided here.
"""

__author__ = 'robinson@google.com (Will Robinson)'

import struct
from google.protobuf import message
from google.protobuf.internal import wire_format


# Note that much of this code is ported from //net/proto/ProtocolBuffer, and
# that the interface is strongly inspired by CodedInputStream from the C++
# proto2 implementation.


class InputStream(object):

  """Contains all logic for reading bits, and dealing with stream position.

  If an InputStream method ever raises an exception, the stream is left
  in an indeterminate state and is not safe for further use.
  """

  def __init__(self, s):
    # What we really want is something like array('B', s), where elements we
    # read from the array are already given to us as one-byte integers.  BUT
    # using array() instead of buffer() would force full string copies to result
    # from each GetSubBuffer() call.
    #
    # So, if the N serialized bytes of a single protocol buffer object are
    # split evenly between 2 child messages, and so on recursively, using
    # array('B', s) instead of buffer() would incur an additional N*logN bytes
    # copied during deserialization.
    #
    # The higher constant overhead of having to ord() for every byte we read
    # from the buffer in _ReadVarintHelper() could definitely lead to worse
    # performance in many real-world scenarios, even if the asymptotic
    # complexity is better.  However, our real answer is that the mythical
    # Python/C extension module output mode for the protocol compiler will
    # be blazing-fast and will eliminate most use of this class anyway.
    self._buffer = buffer(s)
    self._pos = 0

  def EndOfStream(self):
    """Returns true iff we're at the end of the stream.
    If this returns true, then a call to any other InputStream method
    will raise an exception.
    """
    return self._pos >= len(self._buffer)

  def Position(self):
    """Returns the current position in the stream, or equivalently, the
    number of bytes read so far.
    """
    return self._pos

  def GetSubBuffer(self, size=None):
    """Returns a sequence-like object that represents a portion of our
    underlying sequence.

    Position 0 in the returned object corresponds to self.Position()
    in this stream.

    If size is specified, then the returned object ends after the
    next "size" bytes in this stream.  If size is not specified,
    then the returned object ends at the end of this stream.

    We guarantee that the returned object R supports the Python buffer
    interface (and thus that the call buffer(R) will work).

    Note that the returned buffer is read-only.

    The intended use for this method is for nested-message and nested-group
    deserialization, where we want to make a recursive MergeFromString()
    call on the portion of the original sequence that contains the serialized
    nested message.  (And we'd like to do so without making unnecessary string
    copies).

    REQUIRES: size is nonnegative.
    """
    # Note that buffer() doesn't perform any actual string copy.
    if size is None:
      return buffer(self._buffer, self._pos)
    else:
      if size < 0:
        raise message.DecodeError('Negative size %d' % size)
      return buffer(self._buffer, self._pos, size)

  def SkipBytes(self, num_bytes):
    """Skip num_bytes bytes ahead, or go to the end of the stream, whichever
    comes first.

    REQUIRES: num_bytes is nonnegative.
    """
    if num_bytes < 0:
      raise message.DecodeError('Negative num_bytes %d' % num_bytes)
    self._pos += num_bytes
    self._pos = min(self._pos, len(self._buffer))

  def ReadString(self, size):
    """Reads up to 'size' bytes from the stream, stopping early
    only if we reach the end of the stream.  Returns the bytes read
    as a string.
    """
    if size < 0:
      raise message.DecodeError('Negative size %d' % size)
    s = (self._buffer[self._pos : self._pos + size])
    self._pos += len(s)  # Only advance by the number of bytes actually read.
    return s

  def ReadLittleEndian32(self):
    """Interprets the next 4 bytes of the stream as a little-endian
    encoded, unsiged 32-bit integer, and returns that integer.
    """
    try:
      i = struct.unpack(wire_format.FORMAT_UINT32_LITTLE_ENDIAN,
                        self._buffer[self._pos : self._pos + 4])
      self._pos += 4
      return i[0]  # unpack() result is a 1-element tuple.
    except struct.error, e:
      raise message.DecodeError(e)

  def ReadLittleEndian64(self):
    """Interprets the next 8 bytes of the stream as a little-endian
    encoded, unsiged 64-bit integer, and returns that integer.
    """
    try:
      i = struct.unpack(wire_format.FORMAT_UINT64_LITTLE_ENDIAN,
                        self._buffer[self._pos : self._pos + 8])
      self._pos += 8
      return i[0]  # unpack() result is a 1-element tuple.
    except struct.error, e:
      raise message.DecodeError(e)

  def ReadVarint32(self):
    """Reads a varint from the stream, interprets this varint
    as a signed, 32-bit integer, and returns the integer.
    """
    i = self.ReadVarint64()
    if not wire_format.INT32_MIN <= i <= wire_format.INT32_MAX:
      raise message.DecodeError('Value out of range for int32: %d' % i)
    return int(i)

  def ReadVarUInt32(self):
    """Reads a varint from the stream, interprets this varint
    as an unsigned, 32-bit integer, and returns the integer.
    """
    i = self.ReadVarUInt64()
    if i > wire_format.UINT32_MAX:
      raise message.DecodeError('Value out of range for uint32: %d' % i)
    return i

  def ReadVarint64(self):
    """Reads a varint from the stream, interprets this varint
    as a signed, 64-bit integer, and returns the integer.
    """
    i = self.ReadVarUInt64()
    if i > wire_format.INT64_MAX:
      i -= (1 << 64)
    return i

  def ReadVarUInt64(self):
    """Reads a varint from the stream, interprets this varint
    as an unsigned, 64-bit integer, and returns the integer.
    """
    i = self._ReadVarintHelper()
    if not 0 <= i <= wire_format.UINT64_MAX:
      raise message.DecodeError('Value out of range for uint64: %d' % i)
    return i

  def _ReadVarintHelper(self):
    """Helper for the various varint-reading methods above.
    Reads an unsigned, varint-encoded integer from the stream and
    returns this integer.

    Does no bounds checking except to ensure that we read at most as many bytes
    as could possibly be present in a varint-encoded 64-bit number.
    """
    result = 0
    shift = 0
    while 1:
      if shift >= 64:
        raise message.DecodeError('Too many bytes when decoding varint.')
      try:
        b = ord(self._buffer[self._pos])
      except IndexError:
        raise message.DecodeError('Truncated varint.')
      self._pos += 1
      result |= ((b & 0x7f) << shift)
      shift += 7
      if not (b & 0x80):
        return result
